# Local Library Verification

Layout, modes, coverage and evidence rules: [05-testing.md](../00-Guidelines/05-testing.md). `plan.py doctor` reports verified rows without a tester and testers not registered for quick mode.

## Commands

From the library root (scripts resolve paths independently of the working directory):

```bash
python3 '96-Local Testing/01-run.py' --mode quick                 # smoke subsets (table below), then integration and consistency
python3 '96-Local Testing/01-run.py' --mode full                  # every operation and configuration; add CXX=g++-14 for the floor check
python3 '96-Local Testing/01-run.py' --mode stress --seed 42 --rounds 3
python3 '96-Local Testing/01-run.py' --mode full --filter 04-Graphs
ulimit -c 0; CXX=aarch64-linux-gnu-g++ QEMU_LD_PREFIX=/usr/aarch64-linux-gnu python3 '96-Local Testing/01-run.py' --mode quick   # arm64 under qemu-user (binfmt); qemu writes guest cores to the working directory unless ulimit -c is 0
python3 '96-Local Testing/<folder>/<NN-name>_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers         # header-alone, aggregates, two translation units, scalar/AVX2, workspace
python3 '96-Local Testing/01-run.py' --mode full -j 1             # one suite at a time, live output (same for 02-integration.py -j 1)
python3 '96-Local Testing/03-consistency.py'                      # records, links, plan, brace and comment rules
python3 '96-Local Testing/03-consistency.py' --braces <files>     # brace rule and comment cap for arbitrary files
python3 '96-Local Testing/04-mutation.py' run <header>            # mutation campaign (05-testing.md, Mutation Testing); check <header>... compares records, install fetches mewt
python3 '96-Local Testing/05-extraction.py' --mode full           # every operation extracted alone and every C++ tester on its extracted build (05-testing.md, Extraction)
python3 '96-Local Testing/06-compare.py' run '96-Local Testing/01-Core/<NN-name>_compare.cpp' --gate   # a header against its previous version (Docs/00-compare.md)
python3 '96-Local Testing/_00_memory_cap.py' [--mb N] [--need N] -- <cmd>    # run any other program memory-capped, once the memory gate admits it
python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full
python3 '96-Local Testing/00-Tools/02-online_tester.py'
python3 '96-Local Testing/00-Tools/03-notebook_tester.py'
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full
python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode full
python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode full
python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode full
python3 '96-Local Testing/00-Tools/08-plan_tester.py' --mode full
python3 '96-Local Testing/00-Tools/09-setup_tester.py' --mode full    # setup_check.py and the x86 target gate
python3 '96-Local Testing/00-Tools/10-merge_tester.py' --mode full    # /merge-pr and worktree.py's packages.json merge
python3 '96-Local Testing/00-Tools/11-mutation_tester.py' --mode full # 04-mutation.py
python3 '96-Local Testing/00-Tools/12-hooks_tester.py' --mode full    # the Bash hook shell.py and consistency.py's entry
python3 '96-Local Testing/00-Tools/13-grab_tester.py' --mode full     # 10-grab.py and 05-extraction.py's functions
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode full   # _11_search.py and grab --find (Docs/00-search.md)
python3 '96-Local Testing/00-Tools/15-harness_tester.py' --mode full  # the model guard harness.py, its status line warnings and the weekly audit scan
python3 '96-Local Testing/00-Tools/16-submit_tester.py' --mode full   # 97-Online Testing/04-submit.py against a fake judge
python3 '96-Local Testing/00-Tools/17-reap_tester.py' --mode full     # /reap's reap.py on synthetic tables and fake sessions
python3 '96-Local Testing/00-Tools/18-compare_tester.py' --mode full  # 06-compare.py and 00-perf.hpp
python3 '96-Local Testing/00-Tools/07-judge.py' --help             # Library Checker fetch, reproduce and compare (/speed-sweep)
```

| Mode | Coverage |
|---|---|
| quick | Smoke subsets of every non-Core suite and the `QUICK` Core suites, tool fixtures, integration and consistency; excludes the big-integer corpora |
| full | Every suite completely, big-integer differential corpora, available scalar and AVX2 paths, integration with ASan/UBSan self-tests |
| stress | The full set repeated for `--rounds` seeds, then integration with sanitizers |

`CXX` selects the compiler (`g++` locally, `g++-14` for the floor). `--filter` selects suite path substrings; `--no-integration` skips integration and consistency. `-j` (default: logical CPUs) and `--memory-mb` (default: MemAvailable minus 2048 MB) bound the parallel suites; design and measurements: [00-parallel.md](Docs/00-parallel.md). Test binaries and raw benchmark logs are temporary or git-ignored; nothing is written next to canonical headers or the workspace.

## Layout

| Path | Contents |
|---|---|
| `_00_cpp_test_runner.py` | Shared build helper (`Builder`: every C++ test build through ccache), architecture probes, per-build `TIMING` lines |
| `_00_contest_runner.py` | The one driver of the `02`–`07` C++ suites; per-suite differences (`extra`, `timeout`, `checked`, oracle callbacks) are parameters |
| `_00_memory_cap.py` | Memory-capped execution: `ensure()` for entry points, CLI `-- <command>` for ad-hoc programs. Rules: [05-testing.md](../00-Guidelines/05-testing.md), Memory Safety |
| `_00_memory_gate.py` | Memory admission gate every capped run waits in (one reservations file per user under `$XDG_RUNTIME_DIR`, `CP_MEMORY_GATE`). Rules: [05-testing.md](../00-Guidelines/05-testing.md), Memory Safety |
| `01-run.py` | Suite discovery, modes, `QUICK` registration set, parallel memory-bounded jobs; `.run-stats.json` (git-ignored) keeps each suite's last peak and runtime for scheduling |
| `02-integration.py` | Standalone, aggregate, multi-translation-unit, ISA and workspace builds as parallel jobs, through `ccache` when installed |
| `03-consistency.py` | Repository record and style validator; imports `plan.py check` |
| `04-mutation.py` | Mutation campaigns with the pinned mewt in scratch shards, record checks for `/finish` and `/merge-pr`, mewt install ([00-mutation.md](Docs/00-mutation.md)) |
| `05-extraction.py` | Extraction harness: every operation extracted alone and every C++ tester on its extracted build ([00-extraction.md](Docs/00-extraction.md)) |
| `06-compare.py`, `00-perf.hpp` | A/B speed comparison of a Core full header against its previous version on the deciding and gate compilers, from a `<NN-name>_compare.cpp` rig with hardware counters ([00-compare.md](Docs/00-compare.md)) |
| `Docs/` | Support-pass evidence ([00-memory-safety.md](Docs/00-memory-safety.md), P231; [00-parallel.md](Docs/00-parallel.md), P247; [00-judge.md](Docs/00-judge.md), the judge tool; [00-mutation.md](Docs/00-mutation.md), P251 and P252; [00-compare.md](Docs/00-compare.md), P005; [00-extraction.md](Docs/00-extraction.md), P253; [00-search.md](Docs/00-search.md), P263) |
| `00-Tools/` | Regression fixtures for the contest tools, the online expander and workspace generator, the notebook, the header dependency graph, the parallel runner, the documentation site, the judge tool, the plan's roles and claims, the setup check, the hooks, `/merge-pr`, `04-mutation.py`, grab, the concept search, the model guard, `04-submit.py`, `/reap` and `06-compare.py` |
| `00-Tools/07-judge.py` | Library Checker tool for `/speed-sweep`: fastest distinct submissions, sources and per-case times, test regeneration and judge-flag reproduction, per-case comparison; cache `.judge-cache/` (git-ignored) |
| `01-Core/` … `07-Strings/` | Per-header testers, helper `.cpp` files and benchmark drivers |

Known limits: suite discovery scans one folder level; CI (`.github/workflows/checks.yml`) runs only the path rules, the template check and the validator, never the suites. The mode names describe actual coverage; a suite that does not exist is not claimed by any mode.
