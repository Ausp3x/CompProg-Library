---
name: finish
description: Close the package, batch or branch this session worked on. Collects every remaining gap, fixes the defects, asks every other decision, loops until nothing is open, then commits, rebases onto master, fast-forwards master and removes the worktree on the user's yes.
disable-model-invocation: true
argument-hint: "[Pxxx | batch id] (default: from the branch or worktree name)"
allowed-tools: Bash(git add *) Bash(git commit *) Bash(git rebase *) Bash(git rm *) Bash(git mv *)
---

The run ends only when the ledger is empty and the done checklist passes, or when the user chooses otherwise; it never stops silently. The facts below the procedure are this run's starting state.

## Rules

- **Questions** go through AskUserQuestion only, never as a plain reply that waits for a typed answer: the git grant above ends at the user's next message. While a subagent runs, end the turn with a one-line status; its result resumes the run.
- **Defect**: breaks a written rule, contract or test requirement and has exactly one fix that changes no API, domain, scope or plan. Fix it without asking.
- **Decision**: everything else, including kinds of gap these rules do not name. Ask it, unless an owning file already records a user decision on it. Recommend implementing research-level additions now, unless they need another model (`CLAUDE.md`, Models and delegation) or a source that cannot be fetched; then recommend a package. For a finding in another package's files, offer fixing it here (`01-principles.md`, Scope discipline) or recording it in that package's note.
- **Record** each answer as `user decision <date>` in its owning file: a contract or limit in the evidence document, a scope choice in `Docs/00-notes.md`, a rule in its guideline with the reason in `00-Guidelines/10-decisions.md`. It is never asked again.

## Ledger

`.claude/finish/<key>.txt` (git-ignored; key printed below), one line per item: `id | defect or decision | file:line | gap | source | state`, the state `open`, `ask`, `fixed` or `answered` (the compaction hook shows only rows not fixed or answered). Write it in step 2 and update it after every item; a new `/finish` resumes from it.

## Procedure

1. **Model and scope.** As `/package` step 1. If the facts name no package but this session worked on one, rerun `gaps.py <id>` and use its facts. Without a package, the branch changes are the scope; after `/prune`, only its items and checks (`/prune` changes nothing else).
2. **Collect** into the ledger: the recorded gaps below, plan notes and rows not verified, open review findings, failing or skipped checks, and a fresh `@reviewer` audit of the target plus every changed file. Ask the reviewer for limits as well as defects: domain edges, inputs no test reaches, operations without an independent oracle, claims without a recorded run, sources not fetched, decisions made without the user. A target row without a recorded `@researcher` sweep in `Docs/00-sources.md` gets `/package` step 3.
3. **Classify** each item as defect or decision; ask all open decisions in one batch.
4. **Fix** the defects and carry out the answers through `/package` steps 4 to 10. Anything found along the way enters the ledger. A decision that blocks all remaining work is asked at once; the rest wait for the end of the round.
5. **Re-audit** what changed in this round: affected suites and a fresh `@reviewer`. New items return to step 3. If a round leaves the ledger no shorter or an item recurs, ask how to proceed.
6. **Done checklist**: everything `/package` steps 5 to 10 require holds for every target (a batch as its Batch argument says), and the last `@reviewer` pass found nothing.
7. **Land.** Ask once: commit, rebase onto master, merge and remove the worktree? On no, stop and report. On yes:
   - If the user lands with items open, first set the package `in-progress` with a note listing them.
   - Commit with the session's attribution lines: in a worktree every change, so that `git status` is clean; in the main checkout only this run's paths. The message is `Pxxx: <label> — <verified|partial>`, after `/prune` the one its step 9 gives, and without a package `<branch>: <summary>`. Then delete the ledger; in the main checkout, stop there.
   - `git rebase master`. A conflict inside the plan block of `01-prompts.md` is resolved by `python3 '00-Guidelines/13-Plan/plan.py' render`; any marker left after it, and any other conflict, by hand, keeping both sides' intent. After a rebase that applied master commits, rerun the validator and the target's quick suites; a failure goes to a new ledger and back to step 3.
   - Call ExitWorktree with action `keep`, then from the main checkout run `python3 '00-Guidelines/13-Plan/worktree.py' '<worktree path>' --finish`. On failure show the error and ask: retry, fix it (EnterWorktree with the worktree path, then repeat from the rebase) or stop. If ExitWorktree reports no active worktree session, print that command for the user to run from a terminal in the main checkout.
8. **Report**: fixes, decisions recorded, accepted contracts, commands with results, the landing result. No file dumps.

## Facts

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/finish/gaps.py" $ARGUMENTS`
