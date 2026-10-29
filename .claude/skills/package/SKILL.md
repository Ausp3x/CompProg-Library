---
name: package
description: Implement or continue one work package (Pxxx, or `next` for the next ready one) or one batch (C04, DS07, ...) from the library plan. Loads only that package's brief.
disable-model-invocation: true
argument-hint: "Pxxx | next | <batch id>"
---

## Workflow

Work through every step. The brief at the end is the whole scope; do not open other packages, whole inventories or the JSON maps.

1. **Model and prerequisites.** If the brief's first line reports a MISMATCH, or this session's model differs from the brief's, stop and give the user the brief's launch command; continue only if they confirm. If the brief names a batch to run instead, stop and give the user that command. If the brief prints a prerequisite WARNING, stop and report which package must run first; you may still do independent owned work. If it prints STOP (claimed by someone else, or maintainer-only), stop and report it; if it prints Unclaimed or Claims unknown, give the user its claim commands and continue once they confirm; if Claimed by you names a switch, stop, give the user that command and rerun the brief on the claim branch.
2. **Context.** Read the target headers that exist, the evidence documents linked in the brief, the folder's `Docs/00-notes.md` if present, and the legacy excerpts the brief names. Rules for the files you touch load automatically when you open them. Keep notes short; do not paste file contents into your replies. If `00-Guidelines/23-Sweep Ledgers/duplicates.md` has a section for this package, settle its `ask` rows now as `/prune` (Procedure) describes.
3. **Completeness research.** For each target row delegate to `@researcher` with the folder, the header name and the row's operation list. It returns operations present in the reference catalogs but absent from the row, with sources. Operations already in the row stay in scope (the `/omission-sweep` decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md` bind). Add the justified ones to the row's operations before implementing, cite the sources in `<folder>/Docs/00-sources.md`, and leave the rest out with a one-line reason in `Docs/00-notes.md`. List `aliases` in the evidence document only.
4. **Implement** every operation in each row under the applicable rule (`03-cpp.md` or `04-python.md`) and the profile from `01-principles.md`. Keep legacy originals untouched. Never port or add a wrapper (`03-cpp.md`, One API per operation); map each old name to its replacement in the evidence document. If `00-Guidelines/23-Sweep Ledgers/duplicates.md` has a section or a recorded change for this package, apply it as `/prune` steps 2 to 5 describe and record it there as its step 8 does.
5. **Tests** per `05-testing.md` (one entry per header or module, real quick/full/stress coverage, independent oracles, its case classes and builds, memory caps); a `01-Core` tester joins `QUICK` in `96-Local Testing/01-run.py`. Delegate writing a tester to a general-purpose subagent with the header path, the row's operations, the contracts (domains, sentinels, preconditions) and `05-testing.md`; it returns the tester path, its feature-to-test map and the commands it ran. Check the map against the row instead of reading the whole tester, and keep failure analysis and fixes in this session. Run quick, full, full again with `CXX=g++-14` (`05-testing.md`, Builds) and at least one stress round, all under `05-testing.md`, Memory safety.
6. **Benchmarks** when the profile or the Barrett/Montgomery rule requires them; record conditions and medians. Delegate the runs the same way; they return the conditions and medians.
7. **Evidence document** `<folder>/Docs/<NN-name>.md` for each header, with the content and latest-run form `02-structure.md` (Names) defines, including the exact commands, known limits and handoff notes.
8. **Bookkeeping**: update the inventory row operations and status with evidence links; update `98-basic.hpp`/`99-all.hpp` or the Python aggregates; once a row is verified, delete its `97-Legacy` excerpt (`02-structure.md`, Archive); if the Core template or debug header changed, run `python3 '97-Online Testing/03-workspace.py'` then `--check`.
9. **Independent review.** Delegate to `@reviewer` with the brief's id and the model `CLAUDE.md` gives it. Fix every confirmed finding and rerun the affected tests.
10. **Record and validate.** `python3 '00-Guidelines/13-Plan/plan.py' set Pxxx verified --evidence <paths> --note '<what stays true, or empty>'`, which replaces the progress note; if any operation is missing, use `partial` on the row, `in-progress` on the package, and `--note` with the exact remaining work and the failing reproducer, if any. Then `python3 '96-Local Testing/03-consistency.py'` must print no errors.
11. **Report**: operations implemented, commands run with pass/fail, benchmark headlines, review findings fixed, remaining gaps. No file dumps. End by telling the user to run `/finish` with the brief's id, which closes the remaining gaps, commits and lands the work.

## Context discipline

- Never paste a header, a test log or an inventory into a reply; refer to paths and line numbers.
- Run testers with output redirected to a file under `/tmp` and read only the failing lines. A `FAIL memory cap` or pre-flight failure means unbounded allocation or an oversized mode: fix the cause, do not raise the cap without a recorded reason.
- State lives in files, so a compaction (`CLAUDE.md`, Compact instructions) or a fresh session loses nothing: after steps 3, 4, 5 and 9, record the operations done and remaining, open decisions, failing reproducers with seeds and the exact next step with `plan.py set Pxxx in-progress --note "..."`, keeping whatever in the existing note is still open, and keep the evidence document current.

## Batches

A package with several batches runs one batch per session, always under its batch id; a package id's brief only names the next open batch. Complete the batch the same way, then, unless it was the package's last open batch, set the package `in-progress` with a note naming the finished batch and the next one instead of step 10's `verified`.

## Package brief

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show $ARGUMENTS`
