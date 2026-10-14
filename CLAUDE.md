# CompProg Library

Maximal competitive-programming library: C++20 headers in `01-Core` … `07-Strings`, a PyPy-oriented Python subset in `08-Python`, contest tools in `09-Contest Testing`, library tests in `96-Local Testing`, judge solutions in `97-Online Testing`, the notebook in `98-Team Notebook`, the standalone `99-Workspace/template.cpp` and archived originals in `OLD`.

## Start here

- Workflow (session presets, every skill and when to use it, parallel worktrees, commits): `01-prompts.md`, Workflow. Next ready package: `python3 '00-Guidelines/13-Plan/plan.py' next`.
- `00-Guidelines/01-principles.md` is always loaded and holds the requirements, profiles, evidence and scope rules; `02`–`08` load when you open a matching file (`00-Guidelines/00-index.md`). Read `09-sources.md` for research; read the decision log `10-decisions.md` only on demand.
- Each algorithm folder has `00-index.md` (inventory rows: header, operations, status) and `Docs/` with `00-notes.md` (contracts), `00-sources.md` (source ledger) and one `<NN-name>.md` evidence document per header. Read only the rows and documents the package names; read whole inventories, the JSON maps under `00-Guidelines` or the PDFs in `95-Resources` only when the task needs them, and prefer `rg`.
- Hooks block edits under `OLD/` and `97-Legacy/` (except their indexes), hand edits of `99-Workspace/template.cpp`, and whole-file reads of PDFs, JSON ledgers and files over 48 KB; the consistency validator runs after every edit to a Markdown, header, module or plan file.

## Git

When asked to commit, commit directly to local `master`; create no branch unless asked. Never push: the user pushes `master` to `origin`.

## Models and delegation

- Core full types (`01-Core` non-mini headers): Claude Fable 5.1 (`/model fable`), effort max. Everything else: Claude Opus 5.5 (`/model opus`), effort xhigh; `@reviewer` runs at xhigh and `@researcher` at high. Pass `--effort` at launch: a main session ignores an agent file's `effort`, and `max` cannot be saved as a default. `/package` and `/prune` stop when the session does not match the package; the other skills set their model and effort in their front matter.
- `@researcher` sweeps sources for completeness and `@reviewer` reviews a package independently before it is marked verified; both return summaries, keeping the main context small.

## Commands

```bash
python3 '00-Guidelines/13-Plan/plan.py' show P018       # package brief
python3 '00-Guidelines/13-Plan/plan.py' set P018 verified --evidence '01-Core/26-infint.md'
python3 '96-Local Testing/01-run.py' --mode quick        # all suites, quick
python3 '96-Local Testing/_00_memory_cap.py' -- <cmd>    # run any ad-hoc binary, probe or benchmark memory-capped (testers cap themselves)
python3 '96-Local Testing/<folder>/<NN-name>_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'             # runs automatically after edits to Markdown, plan, headers, modules
python3 '97-Online Testing/03-workspace.py' --check      # workspace template is generated; never hand-edit
```

## Toolchain

Local: GCC 16.2 (`g++`), GCC 14.4.1 (`g++-14`), CPython 3.14. Floors: GCC 14 (`g++-14`) with `-std=gnu++20` (judges in `03-cpp.md`), Python 3.10. Every package also runs its suites in full mode with `CXX=g++-14` (the runners read `CXX`) as the floor check and records the result in its evidence. PyPy 8.0.1 (`pypy3`, Python 3.12) is installed; `oj-bundle` and `lualatex` are optional, and a missing tool is reported as SKIP, never a faked pass.
