# Parallel test runner and integration build — evidence (P247)

Scope: [`01-run.py`](../01-run.py) runs the suites, and [`02-integration.py`](../02-integration.py) the integration builds, as parallel jobs bounded by CPUs and memory. Results and failure logs match a sequential run, and the integration compiles go through `ccache` when it is installed (P247, support batch SUP16; reason: [decision log](../../00-Guidelines/10-decisions.md), No polling; parallel test runs). The rule testers follow is in [05-testing.md](../../00-Guidelines/05-testing.md), Layout and modes; P224 keeps the end-of-plan audit.

## Design

`01-run.py`:

- **Jobs.** Tasks are rounds × sorted suites, as before. Up to `-j` suites run at once (default: logical CPUs, 16 here). Each runs as its own process group, with stdout and stderr merged into a temporary log file. Every suite still caps itself in its own systemd scope ([00-memory-safety.md](00-memory-safety.md)). The runner drops `CP_MEMORY_CAPPED` from its environment, so neither the suites nor the integration build run inside a cgroup of a parent.
- **Memory admission.** A suite reserves 5/4 of its last recorded peak plus 256 MB, or the main cap (`CP_TEST_MEMORY_MB`, 4096 MB) when unmeasured. It starts only when the reservations of the running suites plus its own fit `--memory-mb` (default: MemAvailable at start minus 2048 MB), and its own reservation fits the current MemAvailable. When nothing is running, the next suite always starts. Admission is strictly in queue order, so a large reservation at the head holds back smaller suites behind it.
- **Order.** With `-j` above 1, the longest recorded runtime starts first and unmeasured suites go first of all. With `-j 1`, suites run in task order. `.run-stats.json` (git-ignored, written atomically, malformed content ignored) keeps `[peak MB, seconds]` per mode and suite. The runtime is recorded even without systemd scopes, with peak 0 meaning unmeasured. The file affects scheduling only.
- **Equivalence to a sequential run.**
  - Logs print in task order, each after its `RUN` line; with `-j 1` the running suite streams live.
  - A suite that fails after running beside another reruns alone, so only a failure alone counts. A failure that ran alone is final.
  - Later suites that are still running stop (SIGINT to their groups, SIGKILL after 10 s) and restart after the rerun.
  - The printed rerun is preceded by `NOTE <suite> exited <code> beside other suites; the last lines of that run, then its rerun alone:` and the last 20 lines of the first run, each prefixed `| `.
  - The exit status is the first failing task's, after every earlier task finished; no later task's log is printed.
- **Differences from the old sequential output.** The NOTE block is new. Each suite's stderr is merged into stdout, so redirecting `2>` now leaves its FAIL lines in stdout. Suites write to a file, not a terminal, so they print plain lines without in-place progress or colour, as in a redirected sequential run.
- **Interrupts.** SIGINT, SIGTERM and SIGHUP stop every running group together: the suites, or in the last stage the integration build or consistency, each run as its own process group. Their own cleanup runs. A signal is held while a job starts and while jobs stop, until the job is recorded or every group has stopped (SIGKILL included), and is then raised. A signal inherited as ignored stays ignored, so `nohup` still works.
- **Changes to `_00_memory_cap.py`** (user decision 2026-10-25):
  - `supervise()` and the CLI reporter ignore SIGINT themselves, so a capped program's cleanup is not cut short; the old 0.25 s SIGKILL of `subprocess.run` is gone.
  - An interrupted pre-flight prints `INTERRUPTED` instead of a FAIL.
  - A second `ensure()` (an imported entry point) no longer prints a second `MEMORY` line.
- **Integration.** Runs once after the suites, then consistency, as before; `-j` passes through.

`02-integration.py`:

- **Pool.** The same checks as before become 197 tasks with `--sanitizers` on this tree: one per header syntax check, multi-TU build and run, workspace check, and sanitizer build and run. At most `-j` run at once, inside a weighted budget: a syntax check reserves 512 MB, a linked build 1664 MB. Those are the measured maxima below (393 and 1613 MB) with a margin. The main thread starts every pending task that fits, heaviest first and then by source size, which puts `16-poly` first among the heavy builds. Light tasks fill whatever budget the heavy ones leave. On an interrupt no task starts, and no running task starts its next command. The budget is the cap in force (`CP_MEMORY_CAPPED`: the main, outer or pre-flight cap) minus 512 MB.
- **Failures.** A task that failed beside others prints `NOTE <error type> beside other builds; the last lines, then its rerun alone:` and the last 20 lines of the error, each prefixed `| `. It then reruns alone after the pool, in task order. A failure that ran alone is final: later tasks no longer start, and the first such failure in task order is raised. With `-j 1` every task runs in order and stops at the first failure, as before.
- **ccache.** ccache caches `-c` compiles and `-fsyntax-only` checks, never a one-step compile-and-link (probe with ccache 4.14.1: "Called for linking"). So each linked build compiles every unit with `-c` through `ccache <CXX>` and then links with `<CXX>`; syntax checks also go through ccache. One probe compile checks that ccache works; if it fails, the run prints `SKIP ccache: <reason>` and compiles without it.
- **Cache hits across runs.** Commands run from the build directory, and the generated sources have relative names that depend only on what they contain: one unit per header, named after its path (`02-Data_Structures__01-dsu.cpp`), plus `a.cpp` and `b.cpp`. So their preprocessed text is the same in every run, and adding a header does not rename the others. `CCACHE_NOHASHDIR=1` keeps the per-run directory out of the hash of `-g` builds. Without ccache the commands are the same minus the prefix.
- **Raised cap.** `02-integration.py` calls `ensure(mb=8192)`, because all its jobs share one scope. With the 4096 MB cap only two linked builds fit at once, and the 412 s of linked builds could not finish within the 116 s of `16-poly`, the longest single task. With 8192 MB, four fit. Measured parallel peaks: 2172–5489 MB.
- **Defaults** (user decision 2026-10-25):
  - `-j` is the number of logical CPUs.
  - The budget is MemAvailable at start minus 2048 MB, and a suite reserves 5/4 of its last peak plus 256 MB.
  - Integration: 8192 MB cap; 512 and 1664 MB reservations.
  - Admission in strict queue order.
  - The limits below are accepted as they stand.

## Feature-to-test map

The tester is [`00-Tools/05-runner_tester.py`](../00-Tools/05-runner_tester.py). It uses fake suites that sleep and record their intervals, environment and exit codes. Its oracles are the expected log, rebuilt from the suite parameters, and interval overlaps; it never measures speedups.

| Feature | Tests |
|---|---|
| Task-order output, byte-identical at `-j 1` and `-j 4`, cold and with learned stats | `test_order_identity`, `test_random_runs` |
| `-j 1` keeps task order when the stats rank a later suite first | `test_warm_sequential_order` |
| First failure's status, logs up to it, later tasks never start | `test_first_failure_status`, `test_earlier_failure_wins` |
| Rerun alone, NOTE block with the first run's tail, later running task stopped and restarted after the rerun | `test_retry_alone`, `test_first_failure_status`, `test_random_runs` |
| A failure that ran alone is not rerun | `test_alone_failure_final` |
| At most `-j` suites | `test_jobs_bound`, `test_random_runs` |
| Memory admission from learned peaks at the exact budget boundary | `test_memory_admission` |
| Unmeasured suites reserve the cap | `test_unmeasured_reservation`, `test_random_runs` |
| Malformed or missing stats tolerated, valid JSON afterwards | `test_stats_tolerance` |
| Stress rounds and seeds, `CP_TEST_MODE`, `CP_MEMORY_CAPPED` removed | `test_stress_seeds_env` |
| Discovery, `_compile_` exclusion, quick filter, `--filter` | `test_quick_discovery` |
| SIGINT stops every suite and runs its cleanup | `test_sigint_stops` |
| SIGTERM stops the integration stage (its own group) and its cleanup runs | `test_integration_stage_stopped` |
| A second signal during a stop still ends in SIGKILL for suites that ignore SIGINT | `test_second_signal_still_kills` |
| A signal held during the stop for a rerun is raised afterwards, and the rerun never starts | `test_signal_held_during_restart_stop` |
| SIGHUP inherited as ignored (`nohup`) stays ignored | `test_nohup_keeps_sighup_ignored` |
| Real capped suites (`ensure()`): pre-flight under the runner, peaks learned from real `MEMORY` lines; SIGTERM lets a 1 s cleanup finish in each scope (explicit SKIP without systemd scopes) | `test_real_scope` |
| `parallel()`: weight budget, oversized task alone, command lists in order with timeouts | `test_parallel_budget` |
| `parallel()`: light tasks fill the budget beside heavy ones | `test_parallel_fills_budget` |
| `parallel()`: nothing starts after an interrupt | `test_parallel_interrupt` |
| `parallel()`: the exact budget boundary | `test_parallel_budget_boundary` |
| `parallel()`: a final failure stops later tasks only; an earlier task still runs and its failure is raised | `test_parallel_final_failure_order`, `test_parallel_sequential` |
| `parallel()`: rerun alone with its NOTE, earlier failure raised, `-j 1` sequential with `TimeoutExpired` and `OSError` | `test_parallel_rerun_alone`, `test_parallel_first_failure`, `test_parallel_sequential` |
| `main()` on a small fake repository with real compiles: split `-c` plus link, ccache only for `-c` and `-fsyntax-only`, `CCACHE_NOHASHDIR`, units named by header path, budget from `CP_MEMORY_CAPPED`, broken ccache probed once then bypassed, no ccache | `test_main_builds` |

The tester was checked by mutation (2026-10-24): each copy of the code with one seeded defect failed it. The defects were:
- reservation `+255`
- the alone guard removed
- `CP_MEMORY_CAPPED` kept
- `<=` jobs
- `supervise()` without its SIGINT handler (`test_real_scope`)
- `stop()` without the group SIGINT (`test_retry_alone`)
- 2026-10-25, one each: the held signal never raised; `pending = []` after a final failure; no truncation; `< budget`; SIGHUP always caught. They were caught in turn by `test_signal_held_during_restart_stop`, `test_parallel_final_failure_order`, `test_parallel_sequential`, `test_parallel_budget_boundary` and `test_nohup_keeps_sighup_ignored`.

The concurrency audit (2026-10-24) checked all 158 discovered testers and their runners and tools. None writes, deletes or locks a fixed path: every build or fixture directory comes from `tempfile`, `mkdtemp` or a uuid. The 01-Core testers run with `cwd=/tmp`, but every output is absolute. The audit found timing risks under load instead, and the rerun alone covers both:
- `00-Tools/01-contest_tester.py` uses timeouts of 0.1–0.3 s.
- Some 02–07 checked or stress configurations come close to the folder runners' 180 s per command. One, `07-Strings/18-editdistance`'s ASan-UBSan full run, exceeded it in the warm full run, and now has a 360 s limit (user decision 2026-10-25; see [18-editdistance.md](../../07-Strings/Docs/18-editdistance.md)).

## Commands and results (2026-10-24 and 2026-10-25)

Machine: i9-11900H, 8 cores / 16 threads, 31 GB RAM with about 13–15 GB available, GCC 16.2, ccache 4.14.1 (installed by the user on 2026-10-25). Desktop load varied, so only rows taken in the same session are comparable. CPU is user plus sys from `time`. Hit counts are `ccache -s` after `ccache -z`.

| Run (session) | Before (sequential) | After (`-j 16`) |
|---|---|---|
| `01-run.py --mode quick`, cold, no `.run-stats.json` (10-24) | 3382 s (suites 3029 s, integration 347 s, consistency 6 s), CPU 3060 s | 1371 s, CPU 4440 s |
| `01-run.py --mode quick`, warm (10-24) | same | 892 s (3.8×), CPU 8960 s |
| `01-run.py --mode quick -j 8`, warm (10-24) | same | 957 s, CPU 6737 s |
| `01-run.py --mode full`, cold (10-24) | not measured (user decision 2026-10-25: estimated 3–5 h sequential) | 6760 s, CPU 20107 s, no reruns |
| `01-run.py --mode full`, warm (10-25) | same | 4738 s, CPU 28907 s; 1 rerun (below) |
| `02-integration.py --sanitizers`, one-step builds, no ccache (10-24) | 860 s, peak 1835 MB | 199 s, peak 5489 MB of 8192 |
| `02-integration.py --sanitizers` (10-25, loaded desktop) | — | one-step builds 309 s; split, ccache disabled 230 s; split, cold cache 278 s (0/204 hits); split, warm cache 52 s (204/204 hits, peak 2252 MB) |
| `02-integration.py --sanitizers`, final scheduler (10-25, loaded desktop) | — | cold cache 250 s (0/205 hits, CPU 1562 s, peak 4748 MB); warm cache 59 s (205/205 hits, peak 1925 MB) |

Integration tasks, measured one at a time:

| Tasks | Count | Max peak | Total time | Longest |
|---|---|---|---|---|
| Syntax checks (headers, workspace) | 179 | 393 MB | 313 s | 3 s |
| Multi-TU builds and sanitizer builds and runs | 18 | 1613 MB (`17-polymini`) | 412 s | 116 s (`16-poly`) |

Quick mode:
- **Peaks** (warm `-j 16` run). The heaviest suites are `01-Core/17-polymini` (1431 MB), `16-poly` (1315 MB), `02-Data Structures/00-monoids` (1229 MB) and `00-Tools/04-dependencies` (1208 MB).
- **Longest suite.** `01-Core/05-modint`: 196 s alone, 461 s at `-j 8` and 542 s at `-j 16`. It starts first but shares the cores with every other suite, and it sets the floor for a quick run.
- **Default jobs.** `-j 16` beats `-j 8` despite that slowdown, so the default stays at the number of logical CPUs.

Full mode (warm run):
- **Longest suite.** `01-Core/16-poly`, 1853 s under load; the next are `07-infint` (1191 s) and `05-modint` (900 s).
- **Peak.** `07-infint`, 3352 MB.

One suite reran across all library runs: in the warm full run, `07-Strings/18-editdistance` exceeded the folder runner's 180 s limit in its ASan-UBSan full configuration beside other suites, and passed alone. Its NOTE block showed the timeout line.

```bash
python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode quick                        # PASS, 6 tests, peak 46 MB
python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode full --seed 61               # PASS, 28 tests, 48 s, peak 53 MB
CXX=g++-14 python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode full --seed 62    # PASS, 28 tests, 51 s
python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode stress --seed 63 --rounds 10  # PASS, 28 tests, 57 s, peak 52 MB
python3 '96-Local Testing/02-integration.py' --sanitizers                                   # PASS, 177 headers, 250 s cold cache, 52 s warm (final code)
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full                   # PASS, 39 tests incl. test_plan_set_reranks; g++-14 full also PASS
python3 '96-Local Testing/07-Strings/18-editdistance_tester.py' --mode full                 # PASS, 3 configurations, 229 s; g++-14 320 s
python3 '96-Local Testing/01-run.py' --mode quick                                           # PASS, 159 suites, integration, consistency; 892 s warm
python3 '96-Local Testing/01-run.py' --mode full                                            # PASS, 159 suites, integration --sanitizers, consistency; 4738 s warm, 1 rerun
```

## Limits

- **Cold runs.** The first run of a mode, or one after `.run-stats.json` is deleted, reserves the full cap per suite and so runs about three suites at once.
- **The budget is an estimate, not a cgroup.** A suite whose peak grew since its last run can exceed its reservation. Its own cap still holds it, and MemAvailable is checked at each start.
- **Wall time.** Bounded by the longest suite (`05-modint` in quick mode, `16-poly` in full mode), and in integration by the `16-poly` sanitizer task on a cold cache.
- **Reruns.** A genuinely failing suite that ran beside others costs one extra run alone, and the later suites it stops lose their progress. Reruns hide only failures that disappear without parallel load; a NOTE marks every one.
- **Uncovered kills.** SIGKILL of the runner alone leaves its suites running, since they are in their own sessions. Processes that a suite moves into a session of their own (the contest tester's tool fixtures) miss the group SIGINT and outlive it until their own sleep ends, at most about 60 s.
- **Concurrent runs.** Two runs in one checkout keep the stats of whichever wrote last; only scheduling is affected.
- **Integration memory.** The 8192 MB integration cap assumes that much is free, because integration never reads MemAvailable. It runs alone after the suites, and its measured peaks stayed at or below 5489 MB.
- **Heavy build order.** Integration orders the heavy builds by source bytes, which ranks `05-*_cases.cpp` and `a.cpp`/`b.cpp` low although they include large headers. This affects wall time only.
- **`18-editdistance` on g++-14** has its 360 s limit, but was not timed under parallel load (320 s alone for all three configurations).
- **No ccache for the suites.** Their testers build in one compile-and-link step, which ccache does not cache; only the integration build uses it.

## History

- 2026-10-24: P247 introduced. Baseline and parallel timings measured; runner tester passed quick, full and stress. Review: 7 findings and 2 plausible defects fixed, 5 regression tests added. User decision: install ccache and split the integration builds.
- 2026-10-25: ccache 4.14.1 installed; integration builds split into `-c` plus link, with relative generated sources. Cold and warm cache measured, runner tester passed quick, full, g++-14 and stress, and the full library run passed.
- 2026-10-25: `/finish` audit and diff review: 20 defects fixed (integration interrupt and scheduling, stage signals, unit names, ccache probe, pre-flight interrupt, signal during stop, CLI reporter, evidence numbers, rerun noise, integration tests; then nohup, held signals, NOTE detail, 7 tests); 5 user decisions recorded (plan.py re-ranks, editdistance 360 s, defaults accepted, `_00_memory_cap.py` changes kept, no full baseline).
