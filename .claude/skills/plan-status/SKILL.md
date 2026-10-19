---
name: plan-status
description: Show library plan progress and the next ready package.
disable-model-invocation: true
effort: low
---

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" status`

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show next | head -1`

Summarize the counts in two sentences and name the next ready package with its recommended model.
