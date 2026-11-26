#!/usr/bin/env python3
"""SessionStart and PreCompact hook. After a compaction (argument `compact`) it re-injects what the summary may lose,
namely this target's /finish ledger (its well-formed open rows), the git state and the package or batch brief, in that order of
priority. The target is the argument of the session's last /package, /prune or /finish call, else the id in the branch
or worktree name (gaps.py, target). At startup and after /clear (argument `startup` or `clear`) it injects the newest
/pause handoff with the git state, and nothing when there is none. Before a compaction (argument `precompact`) it
prints instructions for the summary: the target, the skill to resume at its step, the ledger path and what to keep
(CLAUDE.md, Compact Instructions). Plain stdout becomes context, or the summary's instructions; output over 10,000
characters would be replaced by a file path or cut, so it stays below that. In a linked worktree it first links SHARED,
the git-ignored source folders, to the main checkout's (user decision 2026-11-26)."""
import datetime
import functools
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

CAP = 9000
SHARED = ('95-Resources/00-Inbox', '95-Resources/13-Papers', '95-Resources/14-Web Pages')  # git-ignored, no tracked file


def run(*cmd, cwd):
    try:
        return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=60).stdout.rstrip()  # status lines start with a space
    except (OSError, subprocess.TimeoutExpired):
        return ''


def last_command(transcript):
    """(skill, first argument, the command that gave it) of the session's last /package, /prune or /finish call (a call
    without an argument keeps the earlier one's), or ('', '', '') (it names the target)."""
    found = ('', '', '')
    try:
        for line in open(transcript, errors='replace'):
            if '<command-name>/' not in line:
                continue
            try:
                e = json.loads(line)
            except ValueError:
                continue
            m = e.get('message') if isinstance(e, dict) and e.get('type') == 'user' else None
            content = m.get('content') if isinstance(m, dict) else None
            if isinstance(content, str):  # typed commands only, never tool output that quotes them
                for skill, args in re.findall(r'<command-name>/(package|prune|finish)</command-name>.*?<command-args>([^<]*)</command-args>', content, re.S):
                    found = (skill, args.split()[0], f'/{skill} {args.split()[0]}') if args.split() else (skill, *found[1:])
    except (OSError, TypeError):
        pass
    return found


@functools.cache
def gaps_module(root):
    spec = importlib.util.spec_from_file_location('finish_gaps', root / '.claude/skills/finish/gaps.py')
    gaps = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gaps)
    return gaps


def ledgers(root, arg):
    """(key, package, existing /finish ledgers, whether arg named the target); /finish without an argument keys its ledger by
    the names."""
    gaps = gaps_module(root)
    key, pkg, _, warning = gaps.target(root, arg)
    keys = dict.fromkeys([key, gaps.target(root)[0]])
    return key, pkg, [l for l in (root / '.claude/finish' / f'{k}.txt' for k in keys) if l.is_file()], bool(arg and pkg and not warning)


def precompact(root, transcript):
    """Instructions for the compaction summary (CLAUDE.md, Compact Instructions)."""
    skill, arg, given = last_command(transcript)
    key, pkg, found, typed = ledgers(root, arg)
    lines = ['Compaction instructions from .claude/hooks/resume.py (CLAUDE.md, Compact Instructions).']
    if pkg:
        lines.append(f'Target: {key} (package {pkg["id"]}, from '
                     + (f'the typed command {given[:80]}).' if typed else 'the branch or worktree name).'))
    if skill:
        lines.append(f'Skill in progress: /{skill}. State the number of its step in progress and the exact next step.')
    if pkg or found:
        lines.append('/finish ledger: ' + (', '.join(str(l.relative_to(root)) for l in found) + ' (its rows hold the state)'
                                           if found else f'.claude/finish/{key}.txt, not written yet') + '.')
    lines.append('Keep the package or batch id, the skill and step in progress, decisions made and by whom, operations done '
                 'and remaining, failing reproducers with seeds, and the exact next step. Drop file contents, passing test '
                 'output and superseded attempts.')
    return '\n'.join(lines)[:CAP]


def git_state(root):
    status = run('git', 'status', '--short', cwd=root).splitlines()
    return (f'git: branch {run("git", "branch", "--show-current", cwd=root) or "(detached)"}; commits on it:\n'
            f'{run("git", "log", "--oneline", "-10", "master..HEAD", cwd=root) or "(none)"}\nuncommitted ({len(status)}):\n'
            + '\n'.join(status[:30]) + ('\n...' if len(status) > 30 else ''))


def link_shared(root):
    """In a linked worktree, make each SHARED folder a link to the main checkout's (created there when missing), so the
    files the user supplies and the sources filed from them are one set for every session; a folder the worktree holds
    itself (not empty) is kept and named in the returned note ('' when there is nothing to say)."""
    common = Path(run('git', 'rev-parse', '--path-format=absolute', '--git-common-dir', cwd=root) or root)
    main = common.parent if common.name == '.git' else root
    if main == root or not (main / '95-Resources').is_dir():
        return ''
    kept = []
    for rel in SHARED:
        here, there = root / rel, main / rel
        if here.is_symlink():
            continue
        if here.is_dir() and not any(here.iterdir()):
            here.rmdir()
        if here.exists():
            kept.append(rel)
            continue
        there.mkdir(parents=True, exist_ok=True)
        here.parent.mkdir(parents=True, exist_ok=True)
        here.symlink_to(there, target_is_directory=True)
    return (f'Note from .claude/hooks/resume.py: {", ".join(kept)} in this worktree are folders of its own, not links to the '
            f'main checkout\'s; move their files to {main / "95-Resources"} and delete them, so every session shares one set.') if kept else ''


def handoff(root):
    """The newest /pause handoff with the git state, or '' when there is none."""
    notes = sorted((root / '.claude/pause').glob('*.md'), key=lambda f: f.stat().st_mtime, reverse=True)
    if not notes:
        return ''
    when = datetime.datetime.fromtimestamp(notes[0].stat().st_mtime)
    others = f' Older handoffs: {", ".join(f.name for f in notes[1:])}.' if notes[1:] else ''
    head = (f'Loaded by .claude/hooks/resume.py: the newest /pause handoff, {notes[0].relative_to(root)} (written '
            f'{when:%Y-%m-%d %H:%M}). Continue from it with the skill it names; re-read a file before editing it.{others}')
    return '\n\n'.join([head, notes[0].read_text().strip()[:CAP - 2500], git_state(root)])[:CAP]


def main():
    try:
        hook = json.load(sys.stdin)
    except ValueError:
        hook = {}
    hook = hook if isinstance(hook, dict) else {}
    root = Path(run('git', 'rev-parse', '--show-toplevel', cwd=hook.get('cwd') or '.') or Path(__file__).resolve().parents[2])
    mode = sys.argv[1] if len(sys.argv) > 1 else hook.get('source', 'compact')
    if mode in ('startup', 'clear'):
        print('\n\n'.join(filter(None, [link_shared(root), handoff(root)])), end='')
        return
    if mode == 'precompact':
        print(precompact(root, hook.get('transcript_path')), end='')
        return
    key, pkg, found, _ = ledgers(root, last_command(hook.get('transcript_path'))[1])
    parts = ['Restored after compaction by .claude/hooks/resume.py. The plan note and evidence documents hold the recorded state; '
             're-read a file before editing it again.']
    for ledger in found:  # well-formed rows only (gaps.py, ledger_rows): stray notes would crowd out the open items
        open_rows, closed, stray = gaps_module(root).ledger_rows(ledger.read_text(errors='replace'))
        parts.append(f'If /finish was running, continue it from its ledger {ledger.relative_to(root)} ({len(closed)} closed rows'
                     + (f' and {len(stray)} lines that are not rows' if stray else '') + ' not shown; open rows):\n'
                     + ('\n'.join(open_rows[:40]) or '(none)')
                     + (f'\n... {len(open_rows) - 40} more in the ledger' if len(open_rows) > 40 else ''))
    parts.append(git_state(root))
    text = '\n\n'.join(parts)
    if pkg:
        brief = run(sys.executable, str(root / '00-Guidelines/13-Plan/plan.py'), 'show', key, cwd=root)
        room = CAP - len(text) - 2
        text += '\n\n' + (brief if len(brief) <= room else brief[:max(room - 16, 0)] + '\n... (truncated)')
    print(text[:CAP])


if __name__ == '__main__':
    try:
        main()
    except Exception:  # Never break a session start or a compaction (exit 2 would block it) because the hook failed.
        pass
    sys.exit(0)
