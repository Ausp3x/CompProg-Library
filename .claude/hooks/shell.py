#!/usr/bin/env python3
"""Pre/PostToolUse hook for Bash: hold shell commands to the rules guard.py and consistency.py apply to the edit tools.

`pre` saves HEAD and the paths git status lists, with their content as git blob ids, in .claude/hooks/.state; `post`
compares the tree after the command with that snapshot, over the listed paths and every path that differs between the
old and new HEAD, so edits a command commits are seen too. A command that changed a file under OLD/ or edited or added a
97-Legacy/ excerpt (their 00-index.md excepted; deleting an excerpt is the archive rule, 02-structure.md) away from its
content at the old HEAD is blocked with the restore commands, as is a 99-Workspace/template.cpp that
`03-workspace.py --check` rejects; a changed Markdown, plan, header or 08-Python file runs the consistency validator.
Results are checked, not the command text, so no spelling of a command escapes; staging changes no content and runs
nothing. A background command is compared when it starts, so edits it makes later are not checked. `pre` also refuses a
`git push` in command position (after a separator, variable assignments and env/sudo/command/exec/nohup/time, with git's
own options between), since pushing is the user's (CLAUDE.md, Git); quoted text and heredoc bodies are not commands, and an encoded
command can still get past this check, which only stops the plain spellings."""
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import consistency  # noqa: E402

ROOT = consistency.ROOT
STATE = ROOT / '.claude/hooks/.state'
TEMPLATE = '99-Workspace/template.cpp'


def git(*args):
    return subprocess.run(['git', *args], cwd=ROOT, capture_output=True, timeout=20).stdout


def blob(rel):
    """The git blob id of rel's current content (a symlink's target), or None when it is absent."""
    p = ROOT / rel
    data = os.readlink(p).encode('utf-8', 'surrogateescape') if p.is_symlink() else p.read_bytes() if p.is_file() else None
    return None if data is None else hashlib.sha1(b'blob %d\0' % len(data) + data).hexdigest()


def tree(rev, paths):
    """{path: blob id} at rev for those of paths it has."""
    out, paths = {}, sorted(paths)
    for i in range(0, len(paths) if rev else 0, 200):
        for e in git('ls-tree', '-z', rev, '--', *paths[i:i + 200]).split(b'\0'):
            if b'\t' in e:
                meta, rel = e.split(b'\t', 1)
                out[rel.decode('utf-8', 'surrogateescape')] = meta.split()[2].decode()
    return out


def snapshot():
    """(HEAD or '', {path: [git status XY, blob id]}) with untracked files listed one by one."""
    out = subprocess.run(['git', 'status', '--porcelain=v1', '-z', '-uall', '--no-renames'], cwd=ROOT,
                         capture_output=True, timeout=20, check=True).stdout
    paths = {rel: [e[:2].decode(), blob(rel)] for e in out.split(b'\0') if e for rel in [e[3:].decode('utf-8', 'surrogateescape')]}
    return git('rev-parse', '-q', '--verify', 'HEAD').decode().strip(), paths


def protected(rel, now):
    """True when rel may not end up with content `now` (None: deleted) other than its content at the old HEAD."""
    if rel.startswith('OLD/'):
        return rel != 'OLD/00-index.md'
    return '/97-Legacy/' in '/' + rel and not rel.endswith('97-Legacy/00-index.md') and now is not None


SEPARATORS = {';', '&&', '||', '|', '&', '(', ')', '{', '}', '!', '|&', ';;'}
PREFIXES = {'env', 'sudo', 'command', 'exec', 'nohup', 'time', 'then', 'do', 'else'}


HEREDOC = re.compile(r'(?<!<)<<(-?)\s*([\'"]?)([A-Za-z_]\w*)\2')


def code(command):
    """command without its heredoc bodies, which are data."""
    out, lines, i = [], command.split('\n'), 0
    while i < len(lines):
        out.append(lines[i])
        for m in HEREDOC.finditer(lines[i]):
            i += 1
            while i < len(lines) and (lines[i].lstrip('\t') if m[1] else lines[i]) != m[3]:
                i += 1
        i += 1
    return '\n'.join(out)


def pushes(command):
    """True when command runs `git push` in command position; unparsable text never counts."""
    try:
        lex = shlex.shlex(code(command).replace('\n', ' ; '), posix=True, punctuation_chars=True)
        lex.whitespace_split = True
        words = list(lex)
    except ValueError:
        return False
    start = True
    for i, w in enumerate(words):
        if w in SEPARATORS:
            start = True
        elif start and (w in PREFIXES or re.fullmatch(r'[A-Za-z_]\w*=.*', w)):
            continue
        elif start and (w == 'git' or w.endswith('/git')):
            j = i + 1
            while j < len(words) and words[j].startswith('-'):
                j += 1 + (words[j] in ('-C', '-c') and j + 1 < len(words))
            if j < len(words) and words[j] == 'push':
                return True
            start = False
        else:
            start = False
    return False


def state_file(data):
    key = ''.join(c for c in str(data.get('tool_use_id') or data.get('session_id') or 'session') if c.isalnum() or c in '-_')
    return STATE / f'{key or "session"}.json'


def pre(data):
    if pushes(str((data.get('tool_input') or {}).get('command', ''))):
        print(json.dumps({'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                          'Pushing is the user\'s (CLAUDE.md, Git): print the push command for them to run instead.'}}))
        return
    STATE.mkdir(parents=True, exist_ok=True)
    for old in STATE.glob('*.json'):  # snapshots of commands that never finished
        if old.stat().st_mtime < time.time() - 86400:
            old.unlink(missing_ok=True)
    state_file(data).write_text(json.dumps(snapshot()))


def post(data):
    f = state_file(data)
    try:
        old, before = json.loads(f.read_text())
    except (OSError, ValueError):
        return
    f.unlink(missing_ok=True)
    new, after = snapshot()
    paths = set(before) | set(after)
    if old and new != old:
        paths |= {p.decode('utf-8', 'surrogateescape') for p in git('diff', '--name-only', '-z', '--no-renames', old, new or 'HEAD').split(b'\0') if p}
    base = tree(old, paths)
    now = {p: blob(p) for p in paths}
    changed = sorted(p for p in paths if now[p] != (before[p][1] if p in before else base.get(p)))
    bad = [p for p in changed if now[p] != base.get(p) and protected(p, now[p])]
    if TEMPLATE in changed and subprocess.run([sys.executable, '97-Online Testing/03-workspace.py', '--check'], cwd=ROOT,
                                              capture_output=True, timeout=60).returncode:
        bad.append(TEMPLATE)
    out = {}
    if bad:
        fresh = [p for p in bad if after.get(p, ['  '])[0] == '??']
        known = [p for p in bad if p not in fresh]
        fix = [f'git restore --source={old[:12] or "HEAD"} --staged --worktree -- {" ".join(map(shlex.quote, known))}'] * bool(known)
        fix += [f'rm -- {" ".join(map(shlex.quote, fresh))}'] * bool(fresh)
        out = {'decision': 'block', 'reason': (
            'This command changed protected files: ' + ', '.join(bad) + '. OLD/ and 97-Legacy/ excerpts are immutable '
            '(01-principles.md, Scope discipline) and template.cpp is generated (97-Online Testing/03-workspace.py). Restore: '
            + '; '.join(fix) + ('; then amend or revert the commit that holds them' if new != old else '') + '.')}
    if any(consistency.watched(p) for p in changed) and (err := consistency.validate()):
        out['hookSpecificOutput'] = {'hookEventName': 'PostToolUse', 'additionalContext': 'Consistency validator FAILED:\n' + err}
    if out:
        print(json.dumps(out))


if __name__ == '__main__':
    try:
        (pre if sys.argv[1:] == ['pre'] else post if sys.argv[1:] == ['post'] else None)(json.load(sys.stdin))
    except Exception:  # A hook must never fail the tool call.
        pass
    sys.exit(0)
