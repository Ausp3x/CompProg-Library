# Local library verification

Layout, modes, coverage and evidence rules: [05-testing.md](../00-Guidelines/05-testing.md). `plan.py doctor` reports verified rows without a tester and testers not registered for quick mode.

## Commands

From the library root (scripts resolve paths independently of the working directory):

```bash
python3 '96-Local Testing/01-run.py' --mode quick                 # smoke subsets (table below), then integration and consistency
python3 '96-Local Testing/01-run.py' --mode full                  # every operation and configuration; add CXX=g++-14 for the floor check
python3 '96-Local Testing/01-run.py' --mode stress --seed 42 --rounds 3
python3 '96-Local Testing/01-run.py' --mode full --filter 04-Graphs
python3 '96-Local Testing/<folder>/<NN-name>_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers         # header-alone, aggregates, two translation units, scalar/AVX2, workspace
python3 '96-Local Testing/03-consistency.py'                      # records, links, plan, brace and comment rules
python3 '96-Local Testing/03-consistency.py' --braces <files>     # brace rule and comment cap for arbitrary files
python3 '96-Local Testing/_00_memory_cap.py' [--mb N] -- <cmd>    # run any other program memory-capped
python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full
python3 '96-Local Testing/00-Tools/02-online_tester.py'
python3 '96-Local Testing/00-Tools/03-notebook_tester.py'
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full
```

| Mode | Coverage |
|---|---|
| quick | Smoke subsets of every non-Core suite and the `QUICK` Core suites, tool fixtures, integration and consistency; excludes the big-integer corpora |
| full | Every suite completely, big-integer differential corpora, available scalar and AVX2 paths, integration with ASan/UBSan self-tests |
| stress | The full set repeated for `--rounds` seeds, then integration with sanitizers |

`CXX` selects the compiler (`g++` locally, `g++-14` for the floor). `--filter` selects suite path substrings; `--no-integration` skips integration and consistency. Test binaries and raw benchmark logs are temporary or git-ignored; nothing is written next to canonical headers or the workspace.

## Layout

| Path | Contents |
|---|---|
| `_00_cpp_test_runner.py` | Shared compile-and-run infrastructure, progress output, PASS/FAIL/SKIP reporting |
| `_00_memory_cap.py` | Memory-capped execution: `ensure()` for entry points, CLI `-- <command>` for ad-hoc programs. Rules: [05-testing.md](../00-Guidelines/05-testing.md), Memory safety |
| `01-run.py` | Suite discovery, modes, `QUICK` registration set |
| `02-integration.py` | Standalone, aggregate, multi-translation-unit, ISA and workspace builds |
| `03-consistency.py` | Repository record and style validator; imports `plan.py check` |
| `Docs/` | Support-pass evidence ([00-memory-safety.md](Docs/00-memory-safety.md), P231) |
| `00-Tools/` | Regression fixtures for the contest tools, the online expander and workspace generator, the notebook and the header dependency graph |
| `01-Core/` … `07-Strings/` | Per-header testers, helper `.cpp` files and benchmark drivers |

Known limits: suite discovery scans one folder level; there is no checked-in CI. The mode names describe actual coverage; a suite that does not exist is not claimed by any mode.
