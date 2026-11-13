# CompProg Library

Maximal competitive-programming library: C++20 headers in `01-Core` … `07-Strings`, a PyPy-oriented Python subset in `08-Python`, contest tools in `09-Contest Testing`, library tests in `96-Local Testing`, judge solutions in `97-Online Testing`, the notebook in `98-Team Notebook`, the standalone `99-Workspace/template.cpp` and archived originals in `OLD`.

## Start here

- Workflow (session presets, every skill and when to use it, parallel worktrees, commits): `01-prompts.md`, Workflow. Next ready package: `python3 '00-Guidelines/13-Plan/plan.py' next`; what waits on you and on others (pushes, pull requests, open `/finish` questions): `plan.py waiting`.
- `00-Guidelines/01-principles.md` is always loaded and holds the requirements, profiles, evidence and scope rules; `02`–`08` load when you open a matching file (`00-Guidelines/00-index.md`). Read `09-sources.md` for research; read the decision log `10-decisions.md` only on demand.
- Each algorithm folder has `00-index.md` (inventory rows: header, operations, status) and `Docs/` with `00-notes.md` (contracts), `00-sources.md` (source ledger) and one `<NN-name>.md` evidence document per header. Read only the rows and documents the package names; read whole inventories, the JSON maps under `00-Guidelines` or the PDFs in `95-Resources` only when the task needs them, and prefer `rg`.
- Hooks block edits under `OLD/` and `97-Legacy/` (except their indexes), hand edits of `99-Workspace/template.cpp`, and whole-file reads of PDFs, JSON ledgers and files over 48 KB; the consistency validator runs once after each batch of tool calls that edits a Markdown, header, module or plan file. `.claude/hooks/shell.py` holds shell commands to the same rules by comparing the tree before and after each one, and refuses pushes.
- A user correction that changes how the project works goes into its owning file in the same session (`01-principles.md`, Scope discipline), never only into Claude's auto-memory, which stays on one machine; auto-memory keeps only what concerns one user: their machine, accounts and personal preferences.

## Git

`git config compprog.role` is `contributor` unless the owner has set `maintainer` (once per clone). The maintainer, when asked to commit, commits directly to local `master` without creating a branch unless asked; in a worktree, `/finish` commits to the worktree branch and lands it on `master` (its step 7). A contributor claims a package before starting, with a draft pull request on a `Pxxx-<batch>-<user>` branch created in the checkout the session works in. All their commits go to that branch, and their `/finish` stops before landing; the maintainer lands their pull request with `/merge-pr <N>`. Core full packages are maintainer-only. Never push: the user pushes.

## Models and delegation

- Core full types (`01-Core` non-mini headers): Claude Fable 5.2 (`/model fable`; the picker's Fable row can still select Fable 5.1, which the guard refuses), effort high, and xhigh for a Core XL package (size `XL`). Everything else: Claude Opus 5.5 (`/model opus`), effort high. `@reviewer` runs at xhigh, and for Core full packages on Fable (Agent `model: fable`) at the package's effort, passed as the Agent `effort`; `@researcher` runs at high; `@speedscout` runs on Fable at high, and at xhigh only on an escalation (`/speed-sweep`, Verify). Pass `--effort` at launch: a main session ignores an agent file's `effort`, while a subagent's own `effort` overrides the session's. `/package`, `/prune` and `/finish` stop when the session does not match the package; the other skills except `/pause` set their effort, and the sweeps and `/tidy` their model, in their front matter; `/pause` sets neither, so it never switches the model and resets the cache it keeps warm. The session profiles `core-session` and `contest-session` set only model and effort and keep an empty body: an agent file with a body replaces Claude Code's system prompt.
- Model guard (`.claude/hooks/harness.py`, user decisions 2026-11-11): a tool call is refused and the turn stopped when the request in flight asks for a model outside the allowed set, or the reply before it, or a subagent's newest reply not yet refused, came from a model other than the one requested or outside that set: `models` in `packages.json`, the agent files' and skill front matter's `model:`, and `models` in `.claude/harness.json` (Haiku for built-in agents). A model change, such as a new `claude-fable-5-2`, replaces the old id in those files and, while sessions on both run, lists both ids in `.claude/harness.json`; request-setting deviations only warn (status line, startup message outside the terminal). Evidence and limits: `00-Guidelines/13-Plan/Docs/00-harness-audit.md`.
- Output economy (Anthropic's Fable 5.1 guidance, which its Fable 5.2 guidance builds on; kept for every model): edit a file surgically unless a rewrite changes the result; settle structure and hard decisions while reasoning and write a long file or document once, never drafted in full in reasoning first; report anything outside the package as a follow-up (`01-principles.md`, Scope discipline) and keep scratch checks under `/tmp`, never as extra permanent tests.
- Fable main sessions (Anthropic's Fable 5.2 prompting guidance; read-only agents keep their own rules):
  - Ownership, in `/package`, `/prune` and `/finish`: you are the owner and long-term maintainer of the package's headers, tests and evidence, and their design is up to you. Restructure them, merge repeated code and fix their defects instead of reporting them; a shortfall against the brief is still raised with the user, and the public contract (the inventory row, `Docs/00-notes.md`) changes only with the user's yes. Other skills and prompts keep the edit scope they state.
  - Finishing: carry the task to its last step without stopping to offer to continue or to list what remains. Diagnose an ordinary obstacle (a failed build, a timeout, a held lock, an empty result) and work through it; stop only where the skill says to stop, for a decision that is the user's, or at a deliberate blocker (a hook refusal, `OLD/`, never push, never submit), and say plainly which.
  - No approval needed for: rerunning a build, test, benchmark or mutation stage that failed or timed out; editing the files the task may edit; the subagents a skill step names. Start no other subagent except for independent parallel work, and none to review your own work outside the skills' `@reviewer` steps: subagents are a large share of a Fable session's cost.
  - Final reports (`/finish`, `/pause`, a skill's report) are for a reader who saw none of the tool calls: key points first, plain full sentences, no term coined during the work, paragraphs where ideas connect and bullets for separate items.
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
python3 '96-Local Testing/04-mutation.py' run <header>   # mutation campaign, /package step 6; check <header>... for records
python3 '97-Online Testing/03-workspace.py' --check      # workspace template is generated; never hand-edit
```

Run a command that may exceed two minutes in the foreground with a `timeout` of up to 600000 ms, or with `run_in_background` and let its completion notice resume the session; never poll with `sleep`, since every poll re-reads the whole context.

## Toolchain

Local: GCC 16.2 (`g++`), GCC 14.4.1 (`g++-14`), CPython 3.14. Floors: `03-cpp.md` (Toolchain and judges) and `04-python.md` (Baseline) for the library; every tool the scripts run, with its floor and its Ubuntu and Arch packages, in `TOOLS` of `.claude/skills/setup-check/setup_check.py`, which `03-consistency.py` holds complete; the per-package `CXX=g++-14` full run: `05-testing.md` (Builds). PyPy 8.0.1 (`pypy3`, Python 3.12) and mewt 4.0.0 (mutation testing, pinned and installed by `04-mutation.py install`) are installed; `oj-bundle` and `lualatex` are optional, and a missing tool is reported as SKIP, never a faked pass. When work needs a missing or blocked tool, ask the user with its install command (`/setup-check` prints them) and continue once it is installed; a cheap workaround may run meanwhile if named.
