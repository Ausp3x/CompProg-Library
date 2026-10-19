---
name: prune
description: Remove one package's confirmed wrappers and second implementations (00-Guidelines/23-Sweep Ledgers/duplicates.md) after moving their tests to the kept operations, and apply its recorded changes. Nothing else changes.
disable-model-invocation: true
argument-hint: "Pxxx"
---

## Package brief

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show $ARGUMENTS`

## Items

!`python3 "${CLAUDE_PROJECT_DIR}/.claude/skills/prune/items.py" $ARGUMENTS`

## Procedure

Scope: the items above and this package's recorded changes; no restyling, renaming or other features. If this session's model or effort differs from the brief's (`CLAUDE.md`, Models and delegation; the effort is `CLAUDE_EFFORT`), stop and give the user the launch command from `CLAUDE.md`; continue only if they confirm. First settle the section's `ask` rows: put each to the user with AskUserQuestion (at most four per call, the fact to decide, your recommendation first), then record each answer in its row (`remove`, `remove (row only)` or `keep`, dated), in the Keeps or Changes it creates, and by deleting its Open questions entry; rows answered `remove` join this run. Stop if there is nothing to do. A package that is not `verified` gets only its `remove (row only)` items: edit the row, run `python3 '96-Local Testing/03-consistency.py'`, record those rows as step 8 does, and report; `/package` does the rest when the package runs.

1. **Recheck.** Read the target headers, evidence documents, testers, benchmarks and `Docs/00-notes.md`. Confirm each `remove` item is still a wrapper or second implementation (`00-Guidelines/03-cpp.md`, One API per operation); skip and report any that is not. Never touch `ask` or `keep` items.
2. **Dependents.** `rg` each symbol outside `OLD/` and `97-Legacy/` and migrate every call site in this change; in another package's file change only the call site and rerun that package's tester in step 6. `99-Workspace/template.cpp` is regenerated, never edited.
3. **Tests.** Reproduce the removed operation's cases against the kept operation (`05-testing.md`), then delete its tester blocks, probe names and benchmark entries.
4. **Code.** Delete each item and the helpers only it used; implement recorded changes with their contract, complexity line and an independent-oracle test. A `remove (row only)` item edits the row; the pinned body stays. Removing a whole header also removes it from `98-basic.hpp`/`99-all.hpp`, its inventory row (prefix retired, `02-structure.md`), its tester and `96-Local Testing/01-run.py` registration, its evidence document, and its plan entries: with a `python3` script (both JSON files are too large to edit by hand) drop the path from its batch's targets in `00-Guidelines/13-Plan/batches.json`, deleting a batch left empty and its id from the package's `batches`, and drop the header's evidence document and name from the package's `evidence` and `label` in `packages.json`; then run `python3 '00-Guidelines/13-Plan/plan.py' render` and `check`.
5. **Documents.** In the evidence document delete the removed contracts and test-map rows and list each removed name with its replacement under `### Legacy mapping` in `## Contracts`; in `Docs/00-notes.md` and the folder's other evidence documents delete sentences that present a removed name as available; remove the names from the row; apply the latest-run rule.
6. **Verify.** `python3 '96-Local Testing/03-consistency.py' --braces <each touched header>`; the package tester in `full` mode locally and with `CXX=g++-14`; the testers named in step 2; the integration build `05-testing.md` (Coverage) requires; `python3 '97-Online Testing/03-workspace.py'` then `--check` if the Core template or debug header changed; `python3 '96-Local Testing/03-consistency.py'` with no errors.
7. **Review.** `@reviewer` with the package id; fix confirmed findings in scope, rerun the affected tests, report the rest.
8. **Record.** In `duplicates.md` delete this package's finished rows, move its `keep` rows into "Kept by the sweep" (package, item with its header, reason; skip those already kept), leave its `ask` rows in its section, delete the section once no row is left, and delete each recorded change it completed. Leave the package status unchanged.
9. **Report** items removed and changed, call sites migrated, cases moved, commands with results, review findings and skipped items. End by telling the user to run `/finish`, which commits with the message `Pxxx: prune duplicates`.
