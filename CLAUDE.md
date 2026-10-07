# CompProg Library

Maximal competitive-programming library. C++20 headers in `01-Core` … `07-Strings`, a PyPy-oriented Python subset in `08-Python`, contest tools in `09-Contest Testing`, library tests in `96-Local Testing`, judge solutions in `97-Online Testing`, notebook in `98-Team Notebook`, the standalone `99-Workspace/template.cpp`, archived originals in `OLD`.

## Start here

- Start a session with the model preset for the package: `claude --agent core-session` (fable, Core full types) or `claude --agent contest-session` (opus, everything else), then `/package Pxxx` to implement, continue or re-audit it. Next ready package: `python3 '00-Guidelines/13-plan/plan.py' next`. Progress: `/plan-status`.
- Mechanical restyle of a verified package to the current comment cap and brace rule: `/restyle Pxxx` (any model). Completeness research for one family: `/research <folder or header>`. Confirmed-defect sweep over audit packages before re-auditing: `/reaudit-review` (parallel read-only reviewers with adversarial refutation).
- Hooks enforce: no edits under `OLD/` or `97-Legacy/`, no hand edits of `99-Workspace/template.cpp`, no whole-file reads of PDFs, JSON ledgers or files over 48 KB, and the consistency validator runs after every edit to an index, header, module or plan file.
- Rules load automatically when you open matching files: `00-Guidelines/01-principles.md` (always), `02-structure.md` (inventories, aggregates, plan), `03-cpp.md`, `04-python.md`, `05-testing.md`, `06-online.md`, `07-notebook.md`, `08-contest-tools.md`. Research guidance is `09-sources.md`. The decision log `10-decisions.md` is on demand only.
- Each algorithm folder has `00-index.md` (inventory rows: header, operations, status) and a `docs/` directory: `notes.md` (contracts), `sources.md` (source ledger) and one `<NN-name>.md` per header (evidence). Read only the rows and documents the package names.

## Non-negotiables

- Every implementation meets Correctness, Optimality, Completeness and Elegance. Existing code is reference, never proof.
- Status words: `planned`, `partial`, `existing-unverified`, `legacy-reference`, `verified`. `verified` requires recorded evidence for every operation in the row.
- Stay inside the assigned package. Update inventory rows, tests, aggregates, evidence and the plan in the same change. Never mark a package verified with missing operations; use `partial` and a note.
- Never submit to an online judge. Never delete `OLD` or `97-Legacy` material before its features are accounted for.
- Do not read whole inventories, the JSON maps under `00-Guidelines`, the decision log or PDFs in `95-Resources` unless the task needs them. Prefer `rg` for lookups.

## Git

- Single-branch workflow: when asked to commit, commit directly to local `master`. Do not create a branch unless asked.
- Never push; the user pushes `master` to `origin` themselves.

## Models and delegation

- Core full types (`01-Core` non-mini headers): Claude Fable 5.1 (`/model fable`), effort max.
- Everything else: Claude Opus 5.5 (`/model opus`), effort high.
- `@researcher` performs source sweeps for completeness; `@reviewer` gives an independent review before a package is marked verified. Both return summaries, keeping the main context small.

## Commands

```bash
python3 '00-Guidelines/13-plan/plan.py' show P018       # package brief
python3 '00-Guidelines/13-plan/plan.py' set P018 verified --evidence '01-Core/26-infint.md'
python3 '96-Local Testing/01-run.py' --mode quick        # all suites, quick
python3 '96-Local Testing/<folder>/<NN-name>_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'             # runs automatically after edits to indexes, plan, headers
python3 '97-Online Testing/03-workspace.py' --check      # workspace template is generated; never hand-edit
```

## Toolchain

Local: GCC 16.2 (`g++`), GCC 14.4.1 (`g++-14`), CPython 3.14. Floors: GCC 14.2 with `-std=gnu++20` (Codeforces MSYS2 Windows, AtCoder and Library Checker GCC 15.2 Linux), Python 3.10. Every package also runs its suites in full mode with `CXX=g++-14` (the runners read `CXX`) as the floor check and records the result in its evidence; exact 14.2 and the Windows build stay unrun. PyPy, oj-bundle and lualatex are not installed; report SKIP, do not fake a pass.
