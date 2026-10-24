#!/usr/bin/env python3
"""Facts for /pause: target key, handoff path and the keep-alive schedule.

Usage: pause.py [hours] [session id]. One-shot pings fall every 50 minutes while earlier than `hours` from now
(default 2; 0 for none). Each ping reads the cached context, so the cache stays warm until an hour after the last.
The key is the one /finish uses (gaps.py target) for the session's last /package, /prune or /finish argument.
"""
import datetime
import importlib.util
import os
import subprocess
import sys
from pathlib import Path

STEP = 50  # minutes: inside the one-hour cache lifetime; one-shot jobs off :00 and :30 fire without jitter


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main():
    args = sys.argv[1:] + ['', '']
    try:
        hours = float(args[0]) if args[0].strip() else 2.0
    except ValueError:
        raise SystemExit(f'/pause takes a number of hours, not "{args[0]}"')
    if not 0 <= hours <= 24:
        raise SystemExit('/pause takes 0 to 24 hours (a session holds at most 50 scheduled jobs)')
    session = args[1] if args[1] and not args[1].startswith('$') else os.environ.get('CLAUDE_CODE_SESSION_ID', '')
    root = Path(subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True).stdout.strip()
                or Path(__file__).resolve().parents[3])
    resume = module('resume_hook', root / '.claude/hooks/resume.py')
    gaps = module('finish_gaps', root / '.claude/skills/finish/gaps.py')
    transcript = next(Path.home().glob(f'.claude/projects/*/{session}.jsonl'), None) if session else None
    key = gaps.target(root, resume.last_command_id(transcript))[0]
    handoff = root / '.claude/pause' / f'{key}.md'
    written = (f'exists, written {datetime.datetime.fromtimestamp(handoff.stat().st_mtime):%Y-%m-%d %H:%M}; replace it'
               if handoff.is_file() else 'none yet')
    now = datetime.datetime.now().replace(second=0, microsecond=0)
    pings = []
    for minutes in range(STEP, int(hours * 60 - 1e-9) + 1, STEP):
        at = now + datetime.timedelta(minutes=minutes)
        pings.append(at - datetime.timedelta(minutes=1) if at.minute in (0, 30) else at)
    print(f'### Pause\nkey {key} | handoff {handoff.relative_to(root)} ({written}) | now {now:%Y-%m-%d %H:%M} local')
    if not pings:
        print('No keep-alive pings: write the handoff only.')
        return
    print(f'Keep-alive pings for {hours:g} h; the cache stays warm until about {pings[-1] + datetime.timedelta(hours=1):%H:%M}:')
    print('\n'.join(f'- `{p.minute} {p.hour} {p.day} {p.month} *` ({p:%H:%M})' for p in pings))


if __name__ == '__main__':
    main()
