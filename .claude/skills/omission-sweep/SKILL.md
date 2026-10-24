---
name: omission-sweep
description: Library-wide completeness sweep. A read-only workflow checks every inventory row and folder against the reference catalogs and writes 00-Guidelines/23-Sweep Ledgers/omissions.md; `apply` moves the decided items into rows, packages and folder notes.
disable-model-invocation: true
argument-hint: "[folder prefixes or inventory paths] (default: 01 to 08) | apply"
model: claude-opus-5-5
effort: high
---

The ledger is `00-Guidelines/23-Sweep Ledgers/omissions.md`; its "Recorded decisions" are the user's and bind every step. `$ARGUMENTS` starting with `apply` selects Apply; anything else is a sweep scope.

## Sweep

This session writes only the ledger.

1. **Scope.** `python3 .claude/skills/omission-sweep/scope.py /tmp/omission-sweep $ARGUMENTS` writes the chunk files and prints the manifest. A run starts about three agents per chunk and per folder swept whole (some 380 for the library); if that exceeds the session's workflow size setting, say so and sweep folder by folder.
2. **Scan.** Call Workflow with `scriptPath: .claude/skills/omission-sweep/scan.js` and the manifest as `args`. It returns every candidate with a decision; relaunch with `resumeFromRunId` while `missingItems` is not empty.
3. **Write.** Keep "Recorded decisions" and replace earlier rows for the swept scope. Per owning package (`plan.py show Pxxx`) a section `## Pxxx — <label> (<status>)` with `| Id | Operation | Row | Purpose | Sources | Decision | Reason |`; proposed rows under `## New rows` with `| Id | Proposed header | Tier | Operations | Sources | Owner | Decision | Reason |`, Owner being a batch of that folder whose package is `planned` and whose focus covers it, or `new`. Merge duplicates; list every `ask` under `## Open questions` with the fact to decide.
4. **Report** counts by folder and decision, every `ask`, failed fetches and `git add '00-Guidelines/23-Sweep Ledgers/omissions.md' && git commit -m "Omission sweep <scope>"`.

List finished scopes under a temporary `## Sweep progress` in the ledger as they finish; after a compaction (`CLAUDE.md`, Compact instructions), resume from it.

## Apply

1. **Settle** each `ask` with AskUserQuestion; record the answer in its row and, dated, under "Recorded decisions".
2. **Adopt into a row:** add the operation to the row; a `verified` row and its package reopen (`01-principles.md`, Evidence). Every owning package gets `Omission sweep YYYY-MM-DD: implement <operations> in <header>.` appended to its existing note (`plan.py set`).
3. **Adopt a new row:** next free prefix of its folder (`02-structure.md`, Names), status `planned; <source keys>`, owned by exactly one batch: the named batch whose package is `planned` (widen its `focus` and `tiers`) or, through a `python3` script on `batches.json` and `packages.json`, a new batch (folder letters, next number, prerequisites from its includes) and package (next `Pnnn`; phase `Core full types`, `Python contest layer`, `Specialist and research families` for Esoteric, else `Contest core`; model per `CLAUDE.md`, Models and delegation; status `planned`). Then `plan.py rank`, which schedules it and renders the checklist.
4. **Omit:** one bullet per item, `- <operation> (<source keys>): <reason>.`, under `## Omission sweep YYYY-MM-DD` in the folder's `Docs/00-notes.md`.
5. **Sources:** `## Fetched YYYY-MM-DD (omission sweep)` in each touched `Docs/00-sources.md`, listing the fetched URLs behind steps 2–4; rows of the swept scope that have no source keys get the keys of the catalogs that showed their family, added to the folder's key list when missing.
6. **Ledger:** delete applied rows and empty sections.
7. **Validate and report:** `plan.py check` and `96-Local Testing/03-consistency.py` print no errors; report the changes and end with `git add <every touched path> && git commit -m "Omission sweep: apply <scope>"`.
