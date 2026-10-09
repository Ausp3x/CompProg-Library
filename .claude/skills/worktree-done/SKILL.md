---
name: worktree-done
description: Check whether this package worktree is finished (clean, merged into master, package verified) and can be removed.
disable-model-invocation: true
---

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/worktree.py"`

Report the verdict in two sentences. If it is NOT DONE, state the listed remaining step(s) exactly. If the only remaining steps are syncing with master, fast-forwarding and removal, point the user to `python3 '00-Guidelines/13-Plan/worktree.py' <this path> --finish` from the main checkout (a worktree cannot rebase or remove itself from inside its own session). Only a conflict outside the generated plan file needs this session.
