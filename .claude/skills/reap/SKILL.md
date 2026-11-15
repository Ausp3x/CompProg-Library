---
name: reap
description: End the processes ended Claude Code sessions of this project left running and delete stale test temporaries; /package and /finish run its script last with --own.
disable-model-invocation: true
argument-hint: "[--dry-run] [--own]"
---

The script's docstring holds the rules: what counts as a leftover, what is only reported, and what is deleted. Its result is below.

Report it in at most five lines: what it ended (with the session and CPU time), what it deleted and how much memory that freed (`/tmp` is RAM on this machine), and each LEFT line with its reason. Never end a process it left alone yourself: give the user the `kill` command for it instead. A FAIL line names what could not be ended or deleted; say so.

## Result

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/reap/reap.py" $ARGUMENTS`
