#!/usr/bin/env python3
"""SessionStart hook. After a compaction (argument `compact`) it re-injects what the summary may lose, namely this
target's /finish ledger (open rows), the git state and the package or batch brief, in that order of priority. The
target is the argument of the session's last /package, /prune or /finish call, else the id in the branch or worktree
name (gaps.py, target). At startup and after /clear (argument `startup` or `clear`) it injects the newest /pause
handoff with the git state, and nothing when there is none. Plain stdout becomes context; output over 10,000
characters would be replaced by a file path, so it stays below that."""
import datetime
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

CAP = 9000
CLOSED = re.compile(r'\|\s*(fixed|answered|closed|done|dropped)\b[^|]*$', re.I)


def run(*cmd, cwd):
    try:
        return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=60).stdout.rstrip()  # status lines start with a space
    except (OSError, subprocess.TimeoutExpired):
        return ''


def last_command_id(transcript):
    """First argument of the session's last /package, /prune or /finish call, or '' (it names the target)."""
    found = ''
    try:
        for line in open(transcript, errors='replace'):
            if '<command-name>/' not in line:
                continue
            try:
                e = json.loads(line)
            except ValueError:
                continue
            content = (e.get('message') or {}).get('content') if e.get('type') == 'user' else None
            if isinstance(content, str):  # typed commands only, never tool output that quotes them
                for args in re.findall(r'<command-name>/(?:package|prune|finish)</command-name>.*?<command-args>([^<]*)</command-args>', content, re.S):
                    found = args.split()[0] if args.split() else found
    except (OSError, TypeError):
        pass
    return found


def git_state(root):
    status = run('git', 'status', '--short', cwd=root).splitlines()
    return (f'git: branch {run("git", "branch", "--show-current", cwd=root) or "(detached)"}; commits on it:\n'
            f'{run("git", "log", "--oneline", "-10", "master..HEAD", cwd=root) or "(none)"}\nuncommitted ({len(status)}):\n'
            + '\n'.join(status[:30]) + ('\n...' if len(status) > 30 else ''))


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
    root = Path(run('git', 'rev-parse', '--show-toplevel', cwd=hook.get('cwd') or '.') or Path(__file__).resolve().parents[2])
    if (sys.argv[1] if len(sys.argv) > 1 else hook.get('source', 'compact')) in ('startup', 'clear'):
        print(handoff(root), end='')
        return
    spec = importlib.util.spec_from_file_location('finish_gaps', root / '.claude/skills/finish/gaps.py')
    gaps = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gaps)
    key, pkg, _, _ = gaps.target(root, last_command_id(hook.get('transcript_path')))
    keys = dict.fromkeys([key, gaps.target(root)[0]])  # /finish without an argument keys its ledger by the names
    ledgers = [l for l in (root / '.claude/finish' / f'{k}.txt' for k in keys) if l.is_file()]
    parts = ['Restored after compaction by .claude/hooks/resume.py. The plan note and evidence documents hold the recorded state; '
             're-read a file before editing it again.']
    for ledger in ledgers:
        rows = ledger.read_text().strip().splitlines()
        open_rows = [r for r in rows if not CLOSED.search(r)]
        parts.append(f'If /finish was running, continue it from its ledger {ledger.relative_to(root)} '
                     f'({len(rows) - len(open_rows)} closed rows not shown; open rows):\n' + '\n'.join(open_rows[:40])
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
    except Exception:  # Never break session start because the hook failed.
        pass
    sys.exit(0)
