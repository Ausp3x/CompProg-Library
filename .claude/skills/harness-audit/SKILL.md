---
name: harness-audit
description: Scan Claude Code's records of this clone for changes in record formats, prompts, experiments, flags, settings and caps since the last scan, explain the new findings and mark them reviewed. Runs weekly by itself; the status line shows `audit N new` until reviewed.
disable-model-invocation: true
argument-hint: "[canary] (also runs the opt-in canary now, enabled or not)"
effort: high
---

1. **Scan.** Run `python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/harness-audit/harness_audit.py" scan --no-canary` in the foreground with a 600000 ms timeout: a plain scan never runs the canary, which the weekly scan runs once it is enabled (user decision 2026-11-11). When the argument is `canary`, confirm with the user first, since its headless runs spend plan usage, then run `scan --canary` with `run_in_background`, since they can take longer. It waits for nothing: when another scan holds the lock it says so; rerun it once that one ends. The first scan of a clone reads every transcript and is the baseline, with no diff.
2. **Read** the report it names (`.claude/harness-audit/report.md` of the main checkout). It holds account data: never commit, paste or publish it, and quote no e-mail address, account or organization id from it.
3. **Explain** each new finding in at most three lines: what changed, which `harness.py` check or documented limit it touches ([00-harness-audit.md](../../../00-Guidelines/13-Plan/Docs/00-harness-audit.md), Weekly scan, explains each kind), and whether it calls for a re-audit (package P287) or a change of `.claude/harness.json`. Removed fields a guard check needs come first. A hide-from-user text goes into `accepted_texts` only on the user's yes. Then give the per-experiment table's headline in one line.
4. **Mark reviewed**: `python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/harness-audit/harness_audit.py" reviewed`, which clears `audit N new`. Edit nothing else unless the user asks.
