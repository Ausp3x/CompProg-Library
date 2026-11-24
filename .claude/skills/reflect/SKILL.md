---
name: reflect
description: Find the user's corrections in this clone's transcripts since the last run and whether each reached its owning file, then propose an edit for each that the user approves row by row. Run before /tidy.
disable-model-invocation: true
argument-hint: "[--since YYYY-MM-DD] (default: since the last run, else 30 days)"
model: claude-opus-5-5
effort: high
---

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/reflect/reflect.py" list $ARGUMENTS`

The chunk files above quote the user's messages: they stay in the git-ignored `.claude/reflect/`, and no file, commit or external request carries them. This skill edits nothing until the user approves a row (user decision 2026-11-19).

1. **Read.** One read-only `general-purpose` agent per chunk file, at most 8 at a time, each given its path and this step. It returns every correction in the chunk: an item where the user changed how the project works, rejected or redirected what Claude did, or restated a rule. A one-off instruction for the task at hand and an answer the package already recorded are not corrections. For each it gives the date and session, the user's words (at most 200 characters), the rule in one sentence, its owning file under `01-principles.md` (Scope Discipline; auto-memory only for what concerns one user: their machine, accounts and personal preferences), and whether that file states it now: `reached` with `file:line`, `partial` with what is missing, or `missing`, found with `rg` in the repository and the auto-memory folder. It edits nothing.
2. **Merge** the rows that state the same rule, keeping the newest wording, and set the `reached` rows aside.
3. **Propose** for each other row the smallest edit: the file and the sentence to add or change, stated once in its owning file with `user decision <date>`, and for a rule its reason in `00-Guidelines/10-decisions.md`.
4. **Ask** row by row with AskUserQuestion, at most four per call, each with apply, change or skip, the recommendation first.
5. **Apply** only the approved edits; `96-Local Testing/03-consistency.py` prints no errors.
6. **Record** once every row is decided: `python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/reflect/reflect.py" done`. A stopped run records nothing, so the next one reads the same transcripts again.
7. **Report** the rows applied, changed, skipped and already reached, the commit command for the edited paths (`git add <paths> && git commit -m "Reflect <date>"`), and last the list `01-principles.md` (Scope Discipline) ends every report with.
