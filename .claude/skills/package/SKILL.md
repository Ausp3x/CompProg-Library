---
name: package
description: Implement, continue or re-audit one work package (Pxxx, or `next` for the next ready one) or one batch (C04, DS07, ...) from the library plan. Loads only that package's brief.
disable-model-invocation: true
argument-hint: "Pxxx | next | <batch id>"
---

## Package brief

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show $ARGUMENTS`

## Workflow

Work through every step. The brief above is the whole scope; do not open other packages, whole inventories or the JSON maps.

1. **Model and prerequisites.** If the brief recommends a model different from this session's, say so once and continue. If the brief prints a prerequisite WARNING, stop and report which package must run first; you may still do independent owned work.
2. **Context.** Read the target headers that exist, the evidence documents linked in the brief, the folder's `Docs/00-notes.md` if present, and the legacy excerpts the brief names. Rules for the files you touch load automatically when you open them. Keep notes short; do not paste file contents into your replies.
3. **Completeness research.** For each target row delegate to `@researcher` with the folder, the header name and the row's operation list. It returns operations present in the reference catalogs but absent from the row, with sources. Add the justified ones to the row's operations before implementing, cite the sources in `<folder>/Docs/00-sources.md`, and leave the rest out with a one-line reason in `Docs/00-notes.md`.
4. **Implement** every operation in each row under the applicable rule (`03-cpp.md` or `04-python.md`) and the profile from `01-principles.md`. Core full types: compile-time ISA kernels plus scalar fallback plus measured thresholds. Minis: independent copy, fewest lines, identical results on the shared domain. Keep legacy originals untouched.
5. **Tests** per `05-testing.md`: one tester entry per header or module, quick/full/stress modes with real coverage, independent oracles, the case classes listed there, optimized and sanitized builds, header-alone and two-translation-unit builds, scalar and ISA paths. Register the entry in `96-Local Testing/01-run.py`. Run quick and full on the local GCC, then full once more with `CXX=g++-14` as the floor check; run stress for at least one round.
6. **Benchmarks** when the profile or the Barrett/Montgomery rule requires them; record conditions and medians.
7. **Evidence document** `<folder>/Docs/<NN-name>.md`, one per header with the header's prefix and stem (split a legacy `Docs/pNNN-*.md` into per-header files when you touch it): contracts and domains, feature-to-test map covering every operation, exact commands with results, benchmark table, sources, known limits and handoff notes.
8. **Bookkeeping**: update the inventory row operations and status with evidence links; update `98-basic.hpp`/`99-all.hpp` or the Python aggregates; once a row is verified, `git rm` its `97-Legacy` excerpt and drop its row from that legacy index (delete the folder when empty; `OLD` stays); if the Core template or debug header changed, run `python3 '97-Online Testing/03-workspace.py'` then `--check`.
9. **Independent review.** Delegate to `@reviewer` with the package id. Fix every confirmed finding and rerun the affected tests.
10. **Validate and record.** `python3 '96-Local Testing/03-consistency.py'` must print no errors. Then `python3 '00-Guidelines/13-Plan/plan.py' set Pxxx verified --evidence <paths>`; if any operation is missing, use `partial` on the row, `in-progress` on the package, and `--note` with the exact remaining work and the failing reproducer, if any.
11. **Report and commit command**: operations implemented, commands run with pass/fail, benchmark headlines, review findings fixed, remaining gaps. No file dumps. End with the exact one-package commit for the user to run or confirm:

    ```bash
    git add -A && git commit -m "Pxxx: <label> — <verified|partial>"
    ```

## Context discipline

- Never paste a header, a test log or an inventory into a reply; refer to paths and line numbers.
- Run testers with output redirected to a file under `/tmp` and read only the failing lines.
- If context runs low before step 10, run `/compact` with the instruction: keep the package id, API decisions made, the list of operations done and remaining, failing reproducers with seeds, and the exact next step; drop file contents and passing test output. Before compacting, record the same facts with `plan.py set Pxxx in-progress --note "..."` so a fresh session can resume.

## Re-audit (status `audit`)

The package was verified under the previous system. Treat its code as existing-unverified and run steps 2 to 11 with these additions: compare every operation in the current row against the code and the tests and list gaps before changing anything; bring the code to the current `03-cpp.md` rules (closing-brace rule, naming vocabulary, `std::` list, complexity comments, judge portability) without changing public names or behavior unless a bug is found; reconcile the existing evidence document rather than writing a new one; rerun its full suite before and after. If the brief's note names a file under `00-Guidelines/23-Reaudit Findings/`, read it in step 2 and treat every finding as a confirmed defect: fix it, or record in the evidence why it is rejected; in step 10 delete that file, remove its row from that folder's `00-index.md` and clear the note in the same `plan.py set` call (`--note ''`). The note may also add owned work (for example the P002 closing-brace checker); that work is in scope.

## Batch argument

When the argument is a batch id, the brief shows only that batch. Complete it the same way, then set the owning package to `in-progress` with a note naming the finished batch.
