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
own options between, and the options of env and sudo), since pushing is the user's (CLAUDE.md, Git), and `97-Online
Testing/04-submit.py` run in command position (directly or after a python or pypy interpreter and its options), since
submitting is the user's (01-principles.md, Scope discipline); quoted text and heredoc bodies are not commands, and an
encoded command can still get past these checks, which only stop the plain spellings. It also refuses any command whose
text names CP_USER (the user's override below), CLAUDE_CODE_CHILD_SESSION, `--no-verify`, a value for core.hooksPath or
its removal, or `env -i` (which would clear the environment the guards read) (GUARD_TEXT). `pre` runs for the Monitor tool too, whose commands carry the same environment; only Bash commands get the
snapshot, since `post` runs for Bash alone.

`push` is the git pre-push hook (.claude/githooks/pre-push) and `claude_shell()` the guard 04-submit.py loads (SUP32, user
decision 2026-11-23): both refuse in a shell Claude Code started (CLAUDE_CODE_CHILD_SESSION=1), which closes the spellings
`pre` misses (`sh -c "git push"`, `g=git; $g push`). A `!` command the user types shares that environment exactly (all 89
variables compared, 2026-11-22), so the user's own push or submission inside a session sets CP_USER=1 (CLAUDE.md, Git).

`commit` is the git pre-commit hook (.claude/githooks/pre-commit), which holds every commit, by any agent or person, to the
same rules (user decision 2026-11-16): it refuses a commit whose staged changes, against HEAD, touch a protected file, stage
a template.cpp that differs from its working copy or fails `--check`, stage a watched file that differs from its working
copy (the validator reads the working tree), or touch a watched file while the validator fails.
It checks the checkout git runs it in (its working directory), with the main checkout's copy of this script, since
core.hooksPath names that one; in a /merge-pr worktree (.claude/worktrees/PR<N>) only the path rules apply, as running
its validator or generator would run the pull request's untrusted code (`merge_pr.py gates` runs them sandboxed). The
template and the validator are checked in the working tree, so other files' unstaged edits still count; the CI workflow
(.github/workflows/checks.yml) checks the pushed tree itself, with `range <base>` for the path rules over the push. An
internal error refuses the commit."""
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
SUBMIT = '04-submit.py'


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
OPTARG = {'-u', '-C', '-g', '-S', '--unset', '--chdir', '--split-string', '--user', '--group'}  # prefix options taking a word
# the override, the variable the guards read, and the ways around the git hooks: refused wherever they appear (SUP32)
GUARD_TEXT = re.compile(r'\b(?:%s|CLAUDE_CODE_CHILD_SESSION)\b|--no-verify\b|(?i:core\.hookspath)(?:[\'"]?\s*=|\s+[^\s;&|)-])'
                        r'|(?i:(?:--unset(?:-all)?|\bunset)\s+core\.hookspath|--remove-section\s+[\'"]?core\b)'
                        r'|\benv(?:\s+(?:-[uCS]\s+\S+|--\w[\w-]*=\S*|-\S+|\w+=\S*))*?\s+(?:-i|--ignore-environment|-)(?=\s|$)'
                        % ('CP' + '_USER'))


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


def words(command):
    """The shell words of command without heredoc bodies, separators as their own words; None when unparsable."""
    try:
        lex = shlex.shlex(code(command).replace('\n', ' ; '), posix=True, punctuation_chars=True)
        lex.whitespace_split = True
        return list(lex)
    except ValueError:
        return None


def commands(command):
    """(index, word) of each word in command position, after separators, prefixes with their options, and variable
    assignments."""
    ws, start, prefixed, skip = words(command) or [], True, False, False
    for i, w in enumerate(ws):
        if w in SEPARATORS:
            start, prefixed, skip = True, False, False
        elif skip:
            skip = False
        elif start and (w in PREFIXES or re.fullmatch(r'[A-Za-z_]\w*=.*', w)):
            prefixed = prefixed or w in PREFIXES
        elif start and prefixed and w.startswith('-') and w != '-':
            skip = w in OPTARG
        elif start:
            start = False
            yield ws, i, w


def pushes(command):
    """True when command runs `git push` in command position; unparsable text never counts."""
    for ws, i, w in commands(command):
        if w == 'git' or w.endswith('/git'):
            j = i + 1
            while j < len(ws) and ws[j].startswith('-'):
                j += 1 + (ws[j] in ('-C', '-c') and j + 1 < len(ws))
            if j < len(ws) and ws[j] == 'push':
                return True
    return False


def submits(command):
    """True when command runs 04-submit.py itself or through a python or pypy interpreter (options between)."""
    for ws, i, w in commands(command):
        if re.fullmatch(r'(python|pypy)[\d.]*', w.rsplit('/', 1)[-1]):
            i += 1
            while i < len(ws) and ws[i].startswith('-') and ws[i] != '-':
                i += 1 + (ws[i] in ('-X', '-W') and i + 1 < len(ws))
            w = ws[i] if i < len(ws) else ''
        if w.rsplit('/', 1)[-1] == SUBMIT:
            return True
    return False


def claude_shell(env=None):
    """True in a shell Claude Code started, unless the command set CP_USER=1 (the user's own `!` command)."""
    env = os.environ if env is None else env
    return env.get('CLAUDE_CODE_CHILD_SESSION') == '1' and env.get('CP_USER') != '1'


def state_file(data):
    key = ''.join(c for c in str(data.get('tool_use_id') or data.get('session_id') or 'session') if c.isalnum() or c in '-_')
    return STATE / f'{key or "session"}.json'


def deny(reason):
    print(json.dumps({'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason': reason}}))


def pre(data):
    """Bash and Monitor: the text checks; Bash alone also the snapshot `post` compares against."""
    command = str((data.get('tool_input') or {}).get('command', ''))
    if m := GUARD_TEXT.search(command):
        return deny(f'This command names {m[0]!r}, which would get past the push and submit guards or the git hooks '
                    '(CLAUDE.md, Git and Start Here): pushing and submitting are the user\'s, and the hooks are never bypassed. '
                    'Read, search or edit such text with the Read, Grep or Edit tools.')
    if pushes(command):
        return deny("Pushing is the user's (CLAUDE.md, Git): print the push command for them to run instead.")
    if submits(command):
        return deny("Submitting is the user's (01-principles.md, Scope Discipline): ask them to run 04-submit.py themselves, "
                    "for example with `! CP_USER=1 python3 '97-Online Testing/04-submit.py'`.")
    if data.get('tool_name', 'Bash') != 'Bash':  # post runs for Bash only
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
            '(01-principles.md, Scope Discipline) and template.cpp is generated (97-Online Testing/03-workspace.py). Restore: '
            + '; '.join(fix) + ('; then amend or revert the commit that holds them' if new != old else '') + '.')}
    if any(consistency.watched(p) for p in changed) and (err := consistency.validate()):
        out['hookSpecificOutput'] = {'hookEventName': 'PostToolUse', 'additionalContext': 'Consistency validator FAILED:\n' + err}
    if out:
        print(json.dumps(out))


EMPTY_TREE = '4b825dc642cb6eb9a060e54bf8d69288fbee4904'


def changes(root, *args):
    """{path: status letter} of `git diff --name-status --no-renames *args` in root."""
    fields = subprocess.run(['git', 'diff', '--name-status', '-z', '--no-renames', *args], cwd=root, capture_output=True,
                            timeout=60, check=True).stdout.split(b'\0')
    return {fields[i + 1].decode('utf-8', 'surrogateescape'): fields[i].decode() for i in range(0, len(fields) - 1, 2)}


def offending(changed):
    """The protected paths among changed ({path: status}), sorted."""
    return sorted(p for p, status in changed.items() if protected(p, None if status == 'D' else status))


def pushed(base):
    """Exit code of the CI path check (.github/workflows/checks.yml): the protected files HEAD changes against base."""
    if bad := offending(changes(Path.cwd(), base, 'HEAD')):
        print(f'Protected files changed since {base[:12]}: ' + ', '.join(bad) + '. OLD/ and 97-Legacy/ excerpts are immutable '
              '(01-principles.md, Scope Discipline).', file=sys.stderr)
    return int(bool(bad))


def commit():
    """Exit code of the pre-commit check of the checkout in the working directory; reasons go to stderr."""
    root = Path.cwd()

    def run(*args, check=True):
        return subprocess.run(['git', *args], cwd=root, capture_output=True, timeout=20, check=check).stdout
    head = run('rev-parse', '-q', '--verify', 'HEAD', check=False).decode().strip() or EMPTY_TREE
    staged = changes(root, '--cached', head)
    untrusted = re.fullmatch(r'PR\d+', root.name) and root.parent.as_posix().endswith('/.claude/worktrees')
    errors = []
    if bad := offending(staged):
        errors.append('Protected files: ' + ', '.join(bad) + '. OLD/ and 97-Legacy/ excerpts are immutable (01-principles.md, '
                      'Scope discipline). Unstage them: git restore --staged -- ' + ' '.join(map(shlex.quote, bad)))
    if TEMPLATE in staged and not untrusted and (staged[TEMPLATE] == 'D' or run('diff', '--name-only', '--', TEMPLATE) or subprocess.run(
            [sys.executable, '97-Online Testing/03-workspace.py', '--check'], cwd=root, capture_output=True, timeout=60).returncode):
        errors.append(f"{TEMPLATE} is generated: run python3 '97-Online Testing/03-workspace.py', then stage the whole file.")
    if (watched := [p for p in staged if consistency.watched(p)]) and not untrusted:
        dirty = set(run('diff', '--name-only', '-z').decode('utf-8', 'surrogateescape').split('\0'))
        if stale := sorted(p for p in watched if p in dirty or (staged[p] == 'D' and (root / p).exists())):
            errors.append('Staged files differ from their working copies, which the validator reads: ' + ', '.join(stale)
                          + '. Stage their current versions: git add -A -- ' + ' '.join(map(shlex.quote, stale)))
        elif err := consistency.validate(root):
            errors.append('Consistency validator FAILED:\n' + err)
    if errors:
        print('Commit refused by .claude/hooks/shell.py commit (CLAUDE.md, Start Here):', *errors, sep='\n', file=sys.stderr)
    return int(bool(errors))


if __name__ == '__main__':
    if sys.argv[1:2] == ['range'] and len(sys.argv) == 3:
        sys.exit(pushed(sys.argv[2]))
    if sys.argv[1:2] == ['push']:  # git passes the remote's name and URL
        if claude_shell():
            sys.exit('Push refused by .claude/githooks/pre-push: this shell was started by Claude Code, and pushing is the '
                     "user's (CLAUDE.md, Git). Typed yourself with `!`? Run it as `CP_USER=1 git push ...`.")
        sys.exit(0)
    if sys.argv[1:] == ['commit']:
        try:
            sys.exit(commit())
        except Exception as exc:  # unchecked commits are what this hook exists to stop
            sys.exit(f'Commit refused: .claude/hooks/shell.py commit failed ({exc!r}); fix the cause and commit again.')
    try:
        (pre if sys.argv[1:] == ['pre'] else post if sys.argv[1:] == ['post'] else None)(json.load(sys.stdin))
    except Exception:  # A hook must never fail the tool call.
        pass
    sys.exit(0)
