# CompProg Library (agent entry for non-Claude tools)

`CLAUDE.md` is the canonical entry point; this file mirrors it for Codex and other agents, which do not load path-scoped rules automatically.

Maximal competitive-programming library. C++20 headers in `01-Core` … `07-Strings`, a PyPy-oriented Python subset in `08-Python`, contest tools in `09-Contest Testing`, library tests in `96-Local Testing`, judge solutions in `97-Online Testing`, notebook in `98-Team Notebook`, the standalone `99-Workspace/template.cpp`, archived originals in `OLD`.

## Start here

- Package brief: `python3 '00-Guidelines/13-Plan/plan.py' show Pxxx`, then follow the workflow in `.claude/skills/package/SKILL.md`. Next ready package: `plan.py next`. Progress: `plan.py status`.
- Read `00-Guidelines/01-principles.md` always, then only the guides for the files you touch: `02-structure.md` (inventories, aggregates, plan), `03-cpp.md` (`.hpp`/`.cpp`), `04-python.md` (`08-Python`), `05-testing.md` (`96-Local Testing`), `06-online.md` (`97`, `99`), `07-notebook.md` (`98`), `08-contest-tools.md` (`09`), `09-sources.md` (research). The decision log `10-decisions.md` is on demand only.
- Each algorithm folder has `00-index.md` (inventory rows: header, operations, status) and a `Docs/` directory: `notes.md` (contracts), `sources.md` (source ledger) and one `<NN-name>.md` per header (evidence). Read only the rows and documents the package names.

## Non-negotiables

- Every implementation meets Correctness, Optimality, Completeness and Elegance. Existing code is reference, never proof.
- Status words: `planned`, `partial`, `existing-unverified`, `legacy-reference`, `verified`. `verified` requires recorded evidence for every operation in the row.
- Stay inside the assigned package. Update inventory rows, tests, aggregates, evidence and the plan in the same change. Never mark a package verified with missing operations; use `partial` and a note.
- Never submit to an online judge. Never delete `OLD` or `97-Legacy` material before its features are accounted for.
- Do not read whole inventories, the JSON maps under `00-Guidelines`, the decision log or PDFs in `95-Resources` unless the task needs them.

## Commands

```bash
python3 '00-Guidelines/13-Plan/plan.py' show P018
python3 '00-Guidelines/13-Plan/plan.py' set P018 verified --evidence '01-Core/26-infint.md'
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' --check
```

## Toolchain

Local: GCC 16.2, CPython 3.14. Floors: GCC 14.2 with `-std=gnu++20` (Codeforces MSYS2 Windows, AtCoder and Library Checker GCC 15.2 Linux), Python 3.10. PyPy, oj-bundle and lualatex are not installed; report SKIP, do not fake a pass.
