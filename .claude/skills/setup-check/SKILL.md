---
name: setup-check
description: Check this machine for every tool the repository runs, with floors, functional probes and install commands.
disable-model-invocation: true
argument-hint: "[ubuntu | arch] (default: from /etc/os-release)"
effort: low
---

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/setup-check/setup_check.py" "$ARGUMENTS" || true`

Report in at most six lines: PASS or FAIL, each FAIL with its install command from the output, then the WARN features that are off and what each would enable. A FAIL blocks packages; a WARN disables only its feature. Install nothing yourself.
