---
name: finish
description: Close the package, batch or branch this session worked on in one pass. One full review, one round of questions (landing included), fixes, then the suites and a review of the fixes; then commits, rebases onto master, fast-forwards master and removes the worktree as answered; a contributor's run stops on its claim branch.
disable-model-invocation: true
argument-hint: "[Pxxx | batch id] (default: from the branch or worktree name)"
allowed-tools: Bash(git add *) Bash(git commit *) Bash(git rebase *) Bash(git rm *) Bash(git mv *)
---

The run is one pass: collect, ask once, fix, verify, land. It never stops silently and repeats only the checks a fix needs. The facts below the procedure are this run's starting state.

## Rules

- **Questions** go through AskUserQuestion only, never as a plain reply that waits for a typed answer: the git grant above ends at the user's next message. While a subagent runs, end the turn with a one-line status; its result resumes the run.
- **Defect**: breaks a written rule, contract or test requirement and has exactly one fix that changes no API, domain, scope or plan. Fix it without asking.
- **Decision**: everything else, including kinds of gap these rules do not name. Ask it, unless an owning file already records a user decision on it. Recommend implementing research-level additions now, unless they need another model (`CLAUDE.md`, Models and delegation) or a source that cannot be fetched; then recommend a package. For a finding in another package's files, offer fixing it here (`01-principles.md`, Scope discipline) or recording it in that package's note.
- **Record** each answer as `user decision <date>` in its owning file: a contract or limit in the evidence document, a scope choice in `Docs/00-notes.md`, a rule in its guideline with the reason in `00-Guidelines/10-decisions.md`. It is never asked again.

## Ledger

`.claude/finish/<key>.txt` (git-ignored; key printed below), one line per item: `id | defect or decision | file:line | gap | source | state`, the state `open`, `ask`, `fixed` or `answered` (the compaction hook shows only rows not fixed or answered). Write it in step 2 and update it after every item; a new `/finish` resumes from it.

## Procedure

1. **Model and scope.** As `/package` step 1. If the facts name no package but this session worked on one, rerun `gaps.py <id>` and use its facts. Without a package, the branch changes are the scope; after `/prune`, only its items and checks (`/prune` changes nothing else).
2. **Collect** into the ledger: the recorded gaps below, plan notes and rows not verified, open review findings, failing or skipped checks, and one fresh `@reviewer` audit of the target plus every changed file, the run's only full audit. Ask the reviewer for limits as well as defects: domain edges, inputs no test reaches, operations without an independent oracle, claims without a recorded run, sources not fetched, decisions made without the user. A target row without a recorded `@researcher` sweep in `Docs/00-sources.md` gets `/package` step 3.
3. **Ask once.** Classify each item as defect or decision. Put every open decision in one round of consecutive AskUserQuestion calls, the last question being whether to land (commit, rebase, merge, remove the worktree; a contributor only commits on the claim branch) when the run ends clean; keep that answer as an `open` ledger row until step 7.
4. **Fix** the defects and carry out the answers through `/package` steps 4 to 8 and 10. Anything found along the way enters the ledger.
5. **Verify** what changed: the affected suites in quick and full mode, full again with `CXX=g++-14`, the integration build `05-testing.md` (Coverage) requires, `96-Local Testing/03-consistency.py`, and, if code or tests changed, an `@reviewer` pass limited to this run's diff. Fix a new defect and rerun its checks; ask a new decision in one final round. No further full audit.
6. **Done checklist**: everything `/package` steps 5 to 10 require holds for every target (a batch as `/package`, Batches, says), and step 5 left nothing open.
7. **Land** when step 3 answered yes and step 6 holds; otherwise ask once: land with the open items or stop. On stop, report. To land:
   - If items stay open, first set the package `in-progress` with a note listing them.
   - Append one tab-separated line to `00-Guidelines/13-Plan/finish-log.tsv`: date, key, session model, effort, confirmed findings of the step 2 review, defects fixed, decisions asked, result (`verified` or `partial`, a contributor's included; `branch` only without a package).
   - A contributor (the Facts' `role` line) commits on the claim branch that line names. When the current branch differs, they first switch: `git switch <branch>` if it exists locally, `git fetch origin <branch> && git switch <branch>` if only on `origin`, otherwise `git switch -c <branch>`. They commit as below, delete the ledger and handoffs, and stop without rebasing. Then print `git push -u origin <branch>` for the user and, when the role line says last open batch and the result is `verified`, `gh pr ready <number> --repo <repository>` with the role line's pull request number (or `gh pr create --fill --repo <repository>` if it names none). The maintainer rebases and lands it.
   - Commit with the session's attribution lines: in a worktree every change, so that `git status` is clean; in the main checkout only this run's paths. The message is `Pxxx: <label> — <verified|partial>`, after `/prune` the one its step 9 gives, and without a package `<branch>: <summary>`. Then delete the ledger and the `/pause` handoffs of this key and its package; in the main checkout, stop there.
   - `git rebase master`. A conflict inside the plan block of `01-prompts.md` is resolved by `python3 '00-Guidelines/13-Plan/plan.py' render`; any marker left after it, and any other conflict, by hand, keeping both sides' intent. After a rebase that applied master commits, rerun the validator and the target's quick suites; a failure goes to a new ledger and back to step 3.
   - Call ExitWorktree with action `keep`, then from the main checkout run `python3 '00-Guidelines/13-Plan/worktree.py' '<worktree path>' --finish`. On failure show the error and ask: retry, fix it (EnterWorktree with the worktree path, then repeat from the rebase) or stop. If ExitWorktree reports no active worktree session, print that command for the user to run from a terminal in the main checkout.
8. **Report**: fixes, decisions recorded, accepted contracts, commands with results, the landing result. No file dumps.

## Facts

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/finish/gaps.py" $ARGUMENTS`
