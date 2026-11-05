#!/usr/bin/env python3
"""PostToolUse hook: run the library consistency validator after edits to Markdown, plan and source files.
shell.py does the same after shell commands."""
import fnmatch
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PATTERNS = ('*.md', '00-Guidelines/13-Plan/*', '*.hpp', '08-Python/*.py')


def failure(text):
    print(json.dumps({'hookSpecificOutput': {'hookEventName': 'PostToolUse',
                                             'additionalContext': 'Consistency validator FAILED:\n' + text}}))


def validate():
    """The validator's error lines joined, or '' when it passes."""
    try:
        run = subprocess.run([sys.executable, '96-Local Testing/03-consistency.py'], cwd=ROOT,
                             capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        return 'timed out after 60 s'
    except OSError as exc:
        return str(exc)
    if not run.returncode:
        return ''
    try:
        lines = json.loads(run.stdout)['errors']
    except (ValueError, KeyError, TypeError):
        lines = (run.stdout + run.stderr).strip().splitlines()[-20:]
    return '\n'.join(lines) or f'exit code {run.returncode}'


def watched(rel):
    return any(fnmatch.fnmatchcase(rel, p) for p in PATTERNS)


def main():
    try:
        path = Path(json.load(sys.stdin)['tool_input']['file_path'])
        rel = (path if path.is_absolute() else ROOT / path).resolve().relative_to(ROOT).as_posix()
    except (ValueError, KeyError, TypeError, OSError):
        return
    if watched(rel) and (err := validate()):
        failure(err)


if __name__ == '__main__':
    try:
        main()
    except Exception:  # A hook must never fail the tool call.
        pass
    sys.exit(0)
