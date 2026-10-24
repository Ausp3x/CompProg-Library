---
name: pause
description: Before stepping away from an open session. Writes a handoff of the work in progress and keeps the prompt cache warm with one-shot keep-alive pings for a few hours (default 2).
disable-model-invocation: true
argument-hint: "[hours] (default 2; 0 writes the handoff only)"
---

Interrupt a running turn first (Esc): pings fire only while the session is idle.

1. **Handoff.** Write the handoff file the facts name, replacing an older one, in at most 40 lines: what `CLAUDE.md` (Compact instructions) says a summary keeps, plus what no file holds yet: hypotheses under test, decisions asked but not recorded, and half-done edits with their files. If `/finish` is running, bring its ledger up to date first.
2. **Keep warm.** Call CronCreate once per ping the facts list, with that cron expression, `recurring: false`, `durable: false` and the prompt `Keep-alive ping from /pause: reply with exactly "ok"; call no tools and do not resume work.`
3. **Report** in at most three lines: the handoff path, the ping times and when the cache goes cold, and that pings need the computer awake and plan usage within its limit (beyond it the cache lasts five minutes and only the handoff helps).

When a message other than a keep-alive ping arrives, first call CronList and CronDelete every keep-alive job still pending, then continue. A new session or `/clear` loads the newest handoff by itself (`.claude/hooks/resume.py`); `/finish` deletes it on landing.

## Facts

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/pause/pause.py" "$ARGUMENTS" "${CLAUDE_SESSION_ID}"`
