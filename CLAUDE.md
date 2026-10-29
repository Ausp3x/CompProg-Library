# CompProg Library

Maximal competitive-programming library: C++20 headers in `01-Core` … `07-Strings`, a PyPy-oriented Python subset in `08-Python`, contest tools in `09-Contest Testing`, library tests in `96-Local Testing`, judge solutions in `97-Online Testing`, the notebook in `98-Team Notebook`, the standalone `99-Workspace/template.cpp` and archived originals in `OLD`.

## Start here

- Workflow (session presets, every skill and when to use it, parallel worktrees, commits): `01-prompts.md`, Workflow. Next ready package: `python3 '00-Guidelines/13-Plan/plan.py' next`.
- `00-Guidelines/01-principles.md` is always loaded and holds the requirements, profiles, evidence and scope rules; `02`–`08` load when you open a matching file (`00-Guidelines/00-index.md`). Read `09-sources.md` for research; read the decision log `10-decisions.md` only on demand.
- Each algorithm folder has `00-index.md` (inventory rows: header, operations, status) and `Docs/` with `00-notes.md` (contracts), `00-sources.md` (source ledger) and one `<NN-name>.md` evidence document per header. Read only the rows and documents the package names; read whole inventories, the JSON maps under `00-Guidelines` or the PDFs in `95-Resources` only when the task needs them, and prefer `rg`.
- Hooks block edits under `OLD/` and `97-Legacy/` (except their indexes), hand edits of `99-Workspace/template.cpp`, and whole-file reads of PDFs, JSON ledgers and files over 48 KB; the consistency validator runs after every edit to a Markdown, header, module or plan file.

## Git

`git config compprog.role` is `contributor` unless the owner has set `maintainer` (once per clone). The maintainer, when asked to commit, commits directly to local `master` without creating a branch unless asked; in a worktree, `/finish` commits to the worktree branch and lands it on `master` (its step 7). A contributor claims a package before starting, with a draft pull request on a `Pxxx-<batch>-<user>` branch created in the checkout the session works in. All their commits go to that branch, and their `/finish` stops before landing. Core full packages are maintainer-only. Never push: the user pushes.

## Models and delegation

- Core full types (`01-Core` non-mini headers): Claude Fable 5.1 (`/model fable`), effort xhigh. Everything else: Claude Opus 5.5 (`/model opus`), effort high. `@reviewer` runs at xhigh, on Fable (Agent `model: fable`) for Core full packages; `@researcher` runs at high; `@speedscout` runs on Fable at xhigh. Pass `--effort` at launch: a main session ignores an agent file's `effort`, while a subagent's own `effort` overrides the session's. `/package`, `/prune` and `/finish` stop when the session does not match the package; the other skills except `/pause` set their effort, and the sweeps and `/tidy` their model, in their front matter; `/pause` sets neither, so no model or effort change resets the cache it keeps warm.
- `@researcher` sweeps sources for completeness, `@reviewer` reviews a package independently before it is marked verified, and `@speedscout` explains for `/speed-sweep` how the fastest judge submissions beat a Core full type. All three return summaries, keeping the main context small.
- Questions to the user go through AskUserQuestion: at most four per call, recommendation first.

## Compact instructions

Sessions auto-compact at 400k tokens (`autoCompactWindow` in `.claude/settings.json`), and the `compact` SessionStart hook re-injects the `/finish` ledger, the package brief and the git state; at startup and after `/clear` it injects the newest `/pause` handoff (`.claude/pause/`, git-ignored) with the git state. A summary keeps the package or batch id, the skill and step in progress, decisions made and by whom, operations done and remaining, failing reproducers with seeds, and the exact next step; it drops file contents, passing test output and superseded attempts.

## Commands

```bash
python3 '00-Guidelines/13-Plan/plan.py' show P018       # package brief
python3 '00-Guidelines/13-Plan/plan.py' set P018 verified --evidence '01-Core/Docs/07-infint.md'
python3 '96-Local Testing/01-run.py' --mode quick        # all suites, quick
python3 '96-Local Testing/_00_memory_cap.py' -- <cmd>    # run any ad-hoc binary, probe or benchmark memory-capped (testers cap themselves)
python3 '96-Local Testing/<folder>/<NN-name>_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'             # runs automatically after edits to Markdown, plan, headers, modules
python3 '97-Online Testing/03-workspace.py' --check      # workspace template is generated; never hand-edit
```

Run a command that may exceed two minutes in the foreground with a `timeout` of up to 600000 ms, or with `run_in_background` and let its completion notice resume the session; never poll with `sleep`, since every poll re-reads the whole context.

## Toolchain

Local: GCC 16.2 (`g++`), GCC 14.4.1 (`g++-14`), CPython 3.14. Floors: `03-cpp.md` (Toolchain and judges) and `04-python.md` (Baseline); the per-package `CXX=g++-14` full run: `05-testing.md` (Builds). PyPy 8.0.1 (`pypy3`, Python 3.12) is installed; `oj-bundle` and `lualatex` are optional, and a missing tool is reported as SKIP, never a faked pass.
