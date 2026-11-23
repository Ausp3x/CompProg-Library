#!/usr/bin/env python3
"""PostToolBatch hook: run the library consistency validator once after a batch of tool calls (main session or subagent)
whose Edit or Write calls touched a Markdown, plan or source file, so a multi-edit batch costs one run (user decision
2026-11-12). Input without a tool_calls list (a client that renamed it, or a hook wired to another event) is said in the
context, so the validator never stops unnoticed; unreadable input stays silent. shell.py runs the validator after each
shell command that changes such a file. A batch that edited a skill's SKILL.md also gets the validator's skill_reattach
warnings (SUP33), which never fail it."""
import fnmatch
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PATTERNS = ('*.md', '00-Guidelines/13-Plan/*', '*.hpp', '08-Python/*.py')


NO_CALLS = ("Consistency hook: its input carries no tool_calls list, so the validator did not run; run python3 "
            "'96-Local Testing/03-consistency.py' and check the PostToolBatch input (.claude/hooks/consistency.py).")


def context(text):
    print(json.dumps({'hookSpecificOutput': {'hookEventName': 'PostToolBatch', 'additionalContext': text}}))


def results(root=ROOT):
    """(the validator's error lines joined, or '' when it passes; its skill_reattach warnings) for the checkout at root."""
    try:
        run = subprocess.run([sys.executable, '96-Local Testing/03-consistency.py'], cwd=root,
                             capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        return 'timed out after 60 s', []
    except OSError as exc:
        return str(exc), []
    try:
        out = json.loads(run.stdout)
        lines, notes = out['errors'], out.get('skill_reattach') or []
    except (ValueError, KeyError, TypeError, AttributeError):
        lines, notes = (run.stdout + run.stderr).strip().splitlines()[-20:], []
    return ('\n'.join(lines) or f'exit code {run.returncode}') if run.returncode else '', notes


def validate(root=ROOT):
    """The validator's error lines joined, or '' when it passes, for the checkout at root."""
    return results(root)[0]


def watched(rel):
    return any(fnmatch.fnmatchcase(rel, p) for p in PATTERNS)


def edited(calls):
    """Root-relative paths of the batch's Edit and Write calls; a malformed call or a path outside the root is skipped."""
    for call in calls:
        try:
            if call['tool_name'] in ('Edit', 'Write'):
                path = Path(call['tool_input']['file_path'])
                yield (path if path.is_absolute() else ROOT / path).resolve().relative_to(ROOT).as_posix()
        except (ValueError, KeyError, TypeError, OSError):
            continue


def main():
    try:
        payload = json.load(sys.stdin)
    except ValueError:
        return
    calls = payload.get('tool_calls') if isinstance(payload, dict) else None
    if not isinstance(calls, list):
        context(NO_CALLS)
    elif any(map(watched, paths := list(edited(calls)))):
        err, notes = results()
        notes = notes if any(p.startswith('.claude/skills/') and p.endswith('/SKILL.md') for p in paths) else []
        text = '\n\n'.join(filter(None, ['Consistency validator FAILED:\n' + err if err else '', 'Skill re-attachment warning (SUP33; '
                                          "03-consistency.py, skill_reattach):\n" + '\n'.join(notes) if notes else '']))
        if text:
            context(text)


if __name__ == '__main__':
    try:
        main()
    except Exception:  # A hook must never fail the tool call.
        pass
    sys.exit(0)
