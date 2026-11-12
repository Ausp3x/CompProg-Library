# Parallel test runner and integration build — evidence (P247)

Scope: [`01-run.py`](../01-run.py) runs the suites, and [`02-integration.py`](../02-integration.py) the integration builds, as parallel jobs bounded by CPUs and memory. Results and failure logs match a sequential run, and the integration compiles go through `ccache` when it is installed (P247, support batch SUP16; reason: [decision log](../../00-Guidelines/10-decisions.md), No polling; parallel test runs). The rule testers follow is in [05-testing.md](../../00-Guidelines/05-testing.md), Layout and modes; P224 keeps the end-of-plan audit.

## Design

`01-run.py`:

- **Jobs.** Tasks are rounds × sorted suites, as before. Up to `-j` suites run at once (default: logical CPUs, 16 here). Each runs as its own process group, with stdout and stderr merged into a temporary log file. Every suite still caps itself in its own systemd scope ([00-memory-safety.md](00-memory-safety.md)). The runner drops `CP_MEMORY_CAPPED` from its environment, so neither the suites nor the integration build run inside a cgroup of a parent.
- **Memory admission.** A suite reserves 11/10 of its last recorded peak plus 64 MB, or the main cap (`CP_TEST_MEMORY_MB`, 4096 MB) when unmeasured. It starts only when the reservations of the running suites plus its own fit `--memory-mb` (default: MemAvailable at start minus 2048 MB), and its own reservation fits the current MemAvailable. When nothing is running, the next suite always starts. A smaller suite may start past one that does not fit, so a large reservation at the head does not hold back the suites behind it.
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
  - The budget is MemAvailable at start minus 2048 MB, and a suite reserves 11/10 of its last peak plus 64 MB (user decision 2026-11-09).
  - Integration: 8192 MB cap; 512 and 1664 MB reservations.
  - A smaller suite may start past one that does not fit; a rerun alone keeps the queue head (user decision 2026-11-09).
  - The limits below are accepted as they stand.

Test builds (P274, support batch SUP25):

- **`Builder`** (`_00_cpp_test_runner.py`). Every C++ test build compiles each unit with `-c` through ccache and links with `$CXX`. A unit in the scratch directory compiles there under its relative name, any other from the tree root, and `CCACHE_NOHASHDIR=1` with `CCACHE_BASEDIR=<tree root>` keep the per-run directory out of the hash, so an unchanged unit hits in every run, shard and scratch copy. A compile that must fail stays a plain command, since ccache stores no failure. A missing or broken ccache prints one `SKIP ccache` and the builds run plain.
- **Concurrent builds.** `Builder.build_all` compiles and `Builder.concurrently` runs a suite's builds at most three at once, each holding its reserved MB of a budget of the cap in force minus 400 MB; a lone build always starts. A job's printed lines appear together, in job order. The first failure kills the descendants of the running jobs, skips those not started and is raised. Stress mode reserves the whole budget per build, so its builds run one at a time.
- **Reservations.** Measured peaks (VmHWM sampled each second over the 2026-11-09 baseline, `MEMORY` lines, `getrusage` per compile) plus about 10%. A contest suite reserves 1200 MB per build, 800 MB in quick mode, unless `memory=` declares more: `13-segment_union` 2300, `18-persistentsegmenttree` 2100, `35-subgraph_enumeration` 2000, `13-convolution` 1800, `00-monoids` 1400. A NOTE at 90% of the cap and the failure of a build the cap killed name that parameter. Each Core suite states its own, sanitized and other, in its `concurrently` call.
- **`_00_contest_runner.py`.** Replaces the six folder runners; their differences are the parameters of `main`: `oracle`, `extra`, `timeout`, `checked`, `memory`. Every suite compiles with `-Wall -Wextra -Wconversion -Werror`, and `--configuration` selects builds for `04-mutation.py`. `TIMING` adds the longest single command, the limit it ran under and the seconds spent in the oracle callback, so the half-limit rule can be read from a log.
- **Checked build.** `-O0` in quick mode, so every suite still runs unoptimized in each pre-flight, and `-O1` in full and stress, with the same debug containers and assertions (user decision 2026-11-09).
- **`19-fastio`** compiles eleven units per configuration (`FASTIO_PART` 0-10) instead of six. Parts 1-10 hold the integer bases, ten instead of five, so a sanitized unit peaks at 1617 MB instead of 1839 and two to three fit the budget at once. Reservations are per part: parts 0-4, then the rest.
- **Enforcement.** `03-consistency.py` rejects a contest tester without the shared runner, a line that runs the compiler outside `Builder` (`-fsyntax-only` rejection probes and compiler queries apart), and a tester that compiles its builds at once but runs them in order (`_00_foundation_runner.py` apart: its tests take seconds).
- **`01-run.py`** lists builds over 10 s in quick mode as a NOTE and reserves 11/10 of a suite's peak plus 64 MB.
- **Over half a limit.** Every test command runs through `timed_run` (`_00_cpp_test_runner.py`) and prints `OVERHALF <label>: <s> s of its <limit> s limit` when it took over half its limit; a label names the compile's source, the link's output or the run's binary and configuration. `01-run.py` gathers these lines from the suites, a nested tester (`02-debug_compile_tester.py`, echoed by its runner), integration and consistency into its end-of-run NOTE. It relays the last two through a pipe read on a thread, so their output stays in order with stderr when both go to one file, and still prints while an interrupt stops them.

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
| A smaller suite starts past one that does not fit | `test_smaller_suite_passes_one_that_does_not_fit` |
| Builds of a suite at once within the cap, output in job order, the first failure stops the rest | `test_concurrently_runs_together_in_job_order`, `test_concurrently_holds_the_cap`, `test_concurrently_stops_at_the_first_failure` |
| A tester that compiles outside `Builder` or skips the contest runner | `test_bypasses` |
| A tester that compiles its builds at once and runs them in order | `test_runs_in_order` |
| The contest checked build is unoptimized in quick mode only | `test_contest_runner_checked_level` |
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
| `supervise()` forwards SIGTERM and SIGHUP sent to the supervisor pid alone to the capped child in a real scope: the child's own status is returned and its pid is gone; the capped child is the supervisor's own child, because `systemd-run --scope` execs the command (explicit SKIP without systemd scopes) | `test_forward_sigterm`, `test_forward_sighup` |
| The same for the pre-flight quick run, whose failure skips the full run | `test_forward_preflight` |
| The same through the ad-hoc CLI (`_00_memory_cap.py -- <command>`), whose scope runs a reporter that forwards to the command: the command's status is returned, and neither the reporter nor the command is left behind | `test_forward_cli` |
| SIGHUP inherited as ignored (`nohup`) is not forwarded, and the child finishes normally | `test_forward_nohup_keeps_sighup_ignored` |
| `forwarding()` restores the previous SIGTERM and SIGHUP handlers and captures piped output | `test_forwarding_restores_handlers` |
| `parallel()`: weight budget, oversized task alone, command lists in order with timeouts | `test_parallel_budget` |
| `parallel()`: light tasks fill the budget beside heavy ones | `test_parallel_fills_budget` |
| `parallel()`: nothing starts after an interrupt | `test_parallel_interrupt` |
| `parallel()`: the exact budget boundary | `test_parallel_budget_boundary` |
| `parallel()`: a final failure stops later tasks only; an earlier task still runs and its failure is raised | `test_parallel_final_failure_order`, `test_parallel_sequential` |
| `parallel()`: rerun alone with its NOTE, earlier failure raised, `-j 1` sequential with `TimeoutExpired` and `OSError` | `test_parallel_rerun_alone`, `test_parallel_first_failure`, `test_parallel_sequential` |
| `main()` on a small fake repository with real compiles: split `-c` plus link, ccache only for `-c` and `-fsyntax-only`, `CCACHE_NOHASHDIR`, units named by header path, budget from `CP_MEMORY_CAPPED`, broken ccache probed once then bypassed, no ccache | `test_main_builds` |
| `Builder`: every unit compiled with `-c` (a generated one under its relative name where it lives, a tree one from the root), `CCACHE_NOHASHDIR` and `CCACHE_BASEDIR` on every compile, one probe, the link without ccache | `test_units_through_ccache_link_not` |
| `Builder`: a real ccache hit for a shard copy of the tree, a miss for a changed header | `test_real_hits_across_copies_of_the_tree` |
| `Builder`: a failure raises the caller's exception with the command and its output; a timeout is a failure; a missing or broken ccache reported once, then plain builds | `test_failure_reports_command_and_output`, `test_timeout_is_a_failure`, `test_fallback_reported_once` |
| `$CXX` read with `shlex.split`, `g++` when unset or blank | `test_compiler_command_splits` |
| `build_all`: three builds at once, fewer within the cap, the first failure in build order raised | `test_build_all_three_at_once`, `test_build_all_within_the_cap`, `test_build_all_first_failure_in_order` |
| SIGINT during `build_all` or `concurrently`, to the process alone or twice to its group (Ctrl-C): the running commands die, no queued build or job starts one, `KeyboardInterrupt` ends the process within 5 s | `test_interrupt_stops_builds_and_jobs` |
| `TIMING` lines: a nested build counts as compile time, not run time | `test_report_counts_builds_as_compile` |
| `over_half` and `OVERHALF` lines; `01-run.py` NOTEs for quick builds over 10 s (full and stress too, from a real pre-flight, once per build and command across stress rounds, its `OVERHALF` lines labelled `pre-flight quick`) and for commands over half their limit, the integration stage's included | `test_over_half`, `test_quick_bound_note`, `test_problems_note`, `test_integration_over_half` |
| The pre-flight's `PREFLIGHT` line carries its `TIMING` and labelled `OVERHALF` lines | `test_preflight_echoes_timing` |
| Shared contest runner: parameters (`extra`, `timeout`, checked-build defines, assertion probes), `TIMING` per build, `--configuration` selection and its usage error, `-Werror` on every suite | `test_contest_runner_parameters`, `test_contest_runner_configurations`, `test_contest_runner_warnings_are_errors` |
| `_00_memory_cap`: capped processes write no core dumps; a tester on the shared runner honours `CP_TEST_MODE` | `test_capped_process_skips_core_dumps`, `test_memory_cap_sees_the_shared_runner` |

The tester was checked by mutation (2026-10-24): each copy of the code with one seeded defect failed it. The defects were:
- the reservation one MB short
- the alone guard removed
- `CP_MEMORY_CAPPED` kept
- `<=` jobs
- `supervise()` without its SIGINT handler (`test_real_scope`)
- `stop()` without the group SIGINT (`test_retry_alone`)
- 2026-10-25, one each: the held signal never raised; `pending = []` after a final failure; no truncation; `< budget`; SIGHUP always caught. They were caught in turn by `test_signal_held_during_restart_stop`, `test_parallel_final_failure_order`, `test_parallel_sequential`, `test_parallel_budget_boundary` and `test_nohup_keeps_sighup_ignored`.

The concurrency audit (2026-10-24) checked all 158 discovered testers and their runners and tools. None writes, deletes or locks a fixed path: every build or fixture directory comes from `tempfile`, `mkdtemp` or a uuid. The 01-Core testers run with `cwd=/tmp`, but every output is absolute. The audit found timing risks under load instead, and the rerun alone covers both:
- `00-Tools/01-contest_tester.py` uses timeouts of 0.1–0.3 s.
- Contest commands near a limit: they now run under the limits in Decisions to review, and `01-run.py` lists any command over half its limit.

## Commands and results (2026-11-10, P274)

Machine: i9-11900H, 8 cores / 16 threads, 31 GB RAM, GCC 16.2.1, GCC 14.4.1, ccache 4.14.1, `-j 16`. Before is `2346cad` with the three whole-library fixes named under Decisions to review, measured 2026-11-09, with no suite cached and a 180 s contest limit. Cold means the cache was emptied before that command, warm means the same command again. Seconds of wall time, one sample each, every run exiting 0.

| Command | Before | After, cold | After, warm |
|---|---|---|---|
| `01-run.py --mode quick` | 1036.3 | 872.7 | 140.6 |
| `01-run.py --mode full` | 10512.4 | 3996.3 | 1712.6 |
| `CXX=g++-14 01-run.py --mode full` | not measured | 3602.7 (no g++-14 entries in the cache) | |
| `02-Data Structures/01-dsu_tester.py` quick | 8.9 | 4.9 | 2.1 |
| same, full | 45.7 | 24.8 | 19.1 |
| same, `CXX=g++-14` full | 38.8 | 19.1 | 12.0 |
| same, stress | 132.7 | 82.3 | 71.4 |
| `01-Core/06-modintmini_tester.py` quick | 38.6 | 17.2 | 2.9 |
| same, full | 101.1 | 42.5 | 6.4 |
| same, `CXX=g++-14` full | 117.2 | 47.9 | 7.8 |
| same, stress | 105.0 | 47.4 | 11.0 |
| `05-extraction.py testers --mode full` | 1174.9 | 673.7 | 125.6 |
| `04-mutation.py run '02-Data Structures/01-dsu.hpp' --whole --jobs 6` | 823.1 | 436.4 (its own emptied `CCACHE_DIR`, 2026-11-10) | 350.2 (same cache) |
| `01-run.py --mode stress --rounds 1` | not measured | 8517 (stopped at `07-Strings/08-palindrome_queries`, below) | |
| `03-consistency.py`, idle | 7.1 | 4.4, 4.4, 4.4 (three runs; JSON byte-identical to the unmemoized validator; the working tree's `strip_cpp` equal to `2346cad`'s on 301,289 texts: the tree's C++ files, the `04-dependencies_tester` fixture strings and 300,000 random ones, 0 differences, 2026-11-10) | |

Results of those runs:

- **Reruns.** None in the cold, warm or g++-14 runs: no suite failed beside others.
- **Over half a limit.** The cold runs listed five compiles over half their limit: on g++ `16-poly_tester-0.o` (93 of 180 s) and `17-polymini_tester-0.o` (90 of 180 s) in `02-integration.py`, and on g++-14 three `05-modint` sanitized compiles (167, 206, 207 of 300 s). Those limits are now 360 s and 600 s, and no other command, run or compile, passed half its limit.
- **Quick bound.** `01-run.py --mode quick -j 1` (every suite alone, warm cache, 2026-11-10, 551 s, exit 0) timed 366 quick builds; two exceeded 10 s, `13-segment_union` checked (10.75 s) and `14-polynomial_algorithms` checked (10.03 s), and the next longest were `13-convolution` checked (8.95 s), Core InfInt `checked-scalar` (8.68 s) and `16-centroiddecomposition` checked (8.46 s). `/finish` cut the two suites' quick inputs (their evidence documents), and their full runs' pre-flights then took 5.95 s and 8.93 s on g++, 7.42 s and 8.73 s on g++-14.
- **Cache footprint.** A full run takes 866 MB on g++ and 1737 MB with g++-14 added. By 2026-11-12 the shared cache stood at 5.0 of 5.0 GiB with 928 cleanups, beside parallel worktrees and mutation campaigns; campaigns now cache apart ([00-mutation.md](00-mutation.md), Procedure), and this machine's `max_size` is 30 GiB (`ccache -M 30Gi`, user decision 2026-11-12); `/setup-check` warns once the cache holds 90% of its limit (user decision 2026-11-12). It held 5.8 GiB at 13:00 that day, with the worktree sessions' campaigns still caching there.
- **Validator** (2026-11-12, load 8–14, CPU seconds in process, old form against new on the same tree): the setup lint 3.40 against 0.31 with one script changed (0.02 with none), the same pairs; the Markdown locations 6.34 against 0.17 over 189,749 lines, identical strings. The validator's JSON was byte-identical to the old one's on a cold cache and on two warm runs; a whole run took 9.0 s of wall time beside the other sessions' campaigns.
- **Warm-cache misses.** A warm full run logs 51 compile misses, all from tool testers' per-run scratch: 34 `13-grab_tester` oracle sources, 12 `05-runner_tester` fixtures and 5 ccache probes. The suites themselves miss none.
- **Quick suites.** The longest in the warm run are `00-Tools/13-grab` (136 s), `11-mutation` (70 s), `14-search` (65 s) and `08-plan` (63 s), and the highest peaks `04-dependencies` (2103 MB) and `01-Core/16-poly` (842 MB).
- **Full suites** (the cold g++-14 run). The longest are `01-Core/19-fastio` (3395 s), `07-infint` (3033 s), `16-poly` (1866 s) and `05-modint` (1350 s), and the highest peaks `19-fastio` (3620 MB), `07-infint` (3509 MB) and `05-Mathematics/13-convolution` (2882 MB).
- **Stress.** One round with seed 42 on the warm cache: every suite before `07-Strings/08-palindrome_queries` in task order passed, 169 in all. That suite fails at seed 42 on `2346cad` as well: its sampled hash oracle demands exact agreement from `StringHash64`, which is Monte Carlo, and accepts a non-palindrome. The run stops at a failure, so the 16 suites after it ran directly with `--mode stress --seed 42`, three at a time, and passed. In `/finish` the user chose to fix it here (user decision 2026-11-10): the sampled oracle counts `StringHash64` false positives instead of failing on them, the other kinds stay exact, and its stress run at seed 42 then passed with 2 counted (`07-Strings/Docs/08-palindrome_queries.md`). Two stress commands passed half their limit, `13-segment_union` (601 s) and `11-functionalgraph` (685 s) ASan-UBSan, and now have 1500 s.
- **Mutation.** `01-dsu.hpp`, cold and warm alike: 140 mutants, 134 caught (5 by timeout), 6 survived, 0 untested; record in [01-dsu.md](../../02-Data%20Structures/Docs/01-dsu.md#mutation-testing). The cold campaign made 464 compiler calls: 139 misses, 18 mutants that fail to compile (never cached) and 307 hits (256 direct, 51 preprocessed), each the same unit compiled again by a later stage, a retest, a baseline or another shard; a hit needs identical input, the tree root is `base_dir` and the shard path stays out of the hash (`test_real_hits_across_copies_of_the_tree`). The warm rerun hit all 446 cacheable calls and missed none. Stage seconds cold and warm: quick with its alone retests 256 and 207, full optimized 11 and 11, the checked and ASan-UBSan builds 140 and 103.
- **Tool testers** (`unittest` tests in warm quick and cold full; g++ and g++-14 alike): `01-contest` 8 and 28, `03-notebook` 12 and 12, `04-dependencies` 30 and 32, `05-runner` 7 and 60, `06-site` 62 and 63, `07-judge` 8 and 12, `08-plan` 33 and 40, `09-setup` 66 and 72, `10-merge` 11 and 80, `11-mutation` 51 and 60, `12-hooks` 15 and 15, `13-grab` 68 and 71, `14-search` 50 and 50; `02-online` passes its own checks.
- **Extraction.** `05-extraction.py testers`: 172 quick runs on extracted builds, 0 failures; `05-extraction.py ops --filter <header>` passes for each of the 14 headers edited for `-Werror`.

Integration tasks (2026-10-24, measured one at a time):

| Tasks | Count | Max peak | Total time | Longest |
|---|---|---|---|---|
| Syntax checks (headers, workspace) | 179 | 393 MB | 313 s | 3 s |
| Multi-TU builds and sanitizer builds and runs | 18 | 1613 MB (`17-polymini`) | 412 s | 116 s (`16-poly`) |

**Default jobs** (2026-10-24). `-j 16` beat `-j 8` on a warm quick run, 892 s against 957 s, so the default stays at the number of logical CPUs.

## Limits

- **Cold runs.** The first run of a mode, or one after `.run-stats.json` is deleted, reserves the full cap per suite and so runs about three suites at once.
- **The budget is an estimate, not a cgroup.** A suite whose peak grew since its last run can exceed its reservation. Its own cap still holds it, and MemAvailable is checked at each start.
- **Wall time.** Bounded by the longest suite (`13-grab` in a warm quick run, `19-fastio` and `07-infint` in full mode), and in integration by the `16-poly` sanitizer task on a cold cache.
- **Reruns.** A genuinely failing suite that ran beside others costs one extra run alone, and the later suites it stops lose their progress. Reruns hide only failures that disappear without parallel load; a NOTE marks every one.
- **Uncovered kills.** SIGKILL of the runner alone leaves its suites running, since they are in their own sessions. Processes that a suite moves into a session of their own (the contest tester's tool fixtures) miss the group SIGINT and outlive it until their own sleep ends, at most about 60 s.
- **Concurrent runs.** Two runs in one checkout keep the stats of whichever wrote last; only scheduling is affected.
- **Integration memory.** The 8192 MB integration cap assumes that much is free, because integration never reads MemAvailable. It runs alone after the suites, and its measured peaks stayed at or below 5489 MB.
- **Heavy build order.** Integration orders the heavy builds by source bytes, which ranks `05-*_cases.cpp` and `a.cpp`/`b.cpp` low although they include large headers. This affects wall time only.
- **Stress builds run one at a time**, their peaks being unmeasured.
- **Shorter ASan traces were measured and not adopted.** On ten slow suites, `malloc_context_size` 30 (the default), 5 and 1 ran in 720, 739 and 722 s: within noise, and shorter traces cost report detail.

## Decisions to review (P274)

Decided without asking while the user was away, or as the smallest reading of the brief; any can change.

- **`06-site_tester` retries** a strict build lost to a network failure (name resolution, a font download timing out) up to four times, then reports SKIP; a build with any other warning, a broken link beside a lost download included, fails at once. Before it, a DNS blip failed whole-library runs. The network dependence itself is the site's, left as it was.
- **Default contest limit 720 s** per command instead of 180, with the former per-suite raises dropped. On a cold full run on g++, the longest command of 57 of 478 builds passed 90 s, half of 180 s, and 17 passed 180 s itself (33 and 5 of the compiles), and g++-14 is alike: per-suite raises would cover most of the slow builds, and a build's time under load is not its time alone. The cost is a hang taking four times as long to fail. A limit is a hang detector, so `TIMING` reports the longest command against it.
- **Mutation retest width.** A timed-out mutant retests beside others, each with its own CPU cores at the same limit, instead of alone: about 300 s of a 725 s `01-dsu` campaign was alone-retests.
- **`full-rest` and `--configuration`** implement the last mutation stage running only the configurations the earlier stages did not; a Core tester has no build option and keeps quick then full.
- **ccache is optional** (`TOOLS`, `/setup-check`), and `base_dir` is set so scratch and shard copies hit.
- **`-Werror` without demotions** (user decision 2026-11-10). No suite keeps a `-Wno-error=`. 21 suites had one, from 14 headers: names used only in an `assert` once `-DNDEBUG` removes it gained `[[maybe_unused]]`, and `01-point.hpp` (`/=` under UBSan) and `06-segmentedsieve.hpp` (a chained assignment through `lll`) were rewritten with the same result. Two tester constructs that made g++ 16 raise `array-bounds` and `stringop-overflow` inside libstdc++ were rephrased. All 21 suites compile with no warning in the optimized, `-O0` checked, `-O1` checked and ASan-UBSan builds on both compilers.
- **Those 14 headers get no mutation campaign in P274** (user decision 2026-11-10): none has a record, no logic changed, and they pass as a `/prune` or `/restyle` edit does (`04-mutation.py check --existing`); each header's first campaign stays with its own package. `03-Geometry`: `01-point`, `02-line_segment`, `05-closestpair`, `06-circle`, `09-triangle`, `10-halfplaneintersection`, `15-convex_polygon_query`, `19-lattice_geometry`; `04-Graphs/11-functionalgraph`; `05-Mathematics`: `02-search_algorithms`, `06-segmentedsieve`, `17-numerical_methods`; `06-Miscellaneous`: `08-sequence_algorithms`, `50-interactive`.
- **Limits raised for measured load** (`05-testing.md`, Layout and modes). On a cold full run `35-subgraph_enumeration`'s ASan run passed 720 s and `16-poly`'s sanitized compile passed 400 s (about 100 s alone), both on g++ and g++-14, and `18-editdistance` (586 s) and `07-lca` (416 s) passed half of 720 s: up to 48 processes share 16 threads. `35-subgraph_enumeration` now has 2400 s, `18-editdistance` 1200 s and a reused DP table (ASan alone 126 s to 108 s), `07-lca` 1000 s, and the 400 and 600 s compile limits of `07-infint`, `08-infintmini`, `09-rational`, `16-poly` and `17-polymini` are 1200 s. The final cold runs then flagged five more compiles over half a limit, so `02-integration.py`'s commands have 360 s instead of 180 (`16-poly_tester-0.o` 93 s, `17-polymini_tester-0.o` 90 s) and every `05-modint` command 600 s instead of 300 (207 s on g++-14). The stress round raised `13-segment_union` and `11-functionalgraph` to 1500 s (601 and 685 s beside the other suites).
- **`-Wshadow`** stays a per-suite `extra` where a suite had it; the brief's flags are the required floor.
- **Rejection probes stay plain** `-fsyntax-only` commands: ccache stores no failing compile.
- **Oracle callbacks.** A build a callback makes counts as that build's run time, and `oracle=` reports it.
- **`run_cpp_suite`** was dead code and is gone. **`_00_factor.py`** gathers the exact primality and factorization that several mathematics Python oracles repeated.
- **Benchmark drivers do not go through `Builder`**: they time their own builds at fixed flags.
- **Warm-cache misses stay.** The 17 of a warm quick run and 51 of a warm full run are all tool testers' per-run scratch (`13-grab_tester`'s oracle sources, whose whole test is 4.6 s, the `05-runner` fixtures and the ccache probes); the suites miss none, and giving those fixtures stable paths would save seconds. Through `Builder` with content-named units (2026-11-10), `13-grab_tester`'s warm quick run misses 19 of 103 compiles (CCACHE_DIR of its own, third run): its fixture trees, under a per-run temporary directory reached by absolute `-I` paths.
- **Core dumps.** Capped runs set `RLIMIT_CORE=1`, which skips the piped `core_pattern`: a crashing process otherwise spends 0.1 to 0.2 s in `systemd-coredump` (median of 5, idle, 1 and 100 MB resident), against 0.001 s, and a limit of 0 does not skip the pipe.
- **Tool suites over 10 s**, profiled in-process, quick mode, alone, warm cache: `13-grab` 54 s (146 s when its headers changed), `11-mutation` 28 s, `06-site` 16 s, `08-plan` 16 s. The time is real compiles (82 builds and runs in `13-grab`), one Python start per CLI call (829 in `08-plan`) and fixtures waiting on real timeouts (`11-mutation`); no test was removed. `13-grab` fell to 32 s once its oracle builds went through `Builder` (2026-11-10). `04-dependencies` is 6 s.
- **Fixes outside the brief** that a whole-library run needed: the `02-online_tester` `_11_search.py` fixture and `04-dependencies_tester` `cycle_nodes`. The baselines carry these two and the site retry as patches on `2346cad`, so they could finish.
- **Evidence links.** 74 references to the deleted `_00_runner.py` now name `_00_contest_runner.py`; dated History lines stay.
- **The foundation suites** (`01-template`, `02-debug`) run their tests in order: a few seconds in all.

## History

- 2026-10-24: P247 introduced. Baseline and parallel timings measured; runner tester passed quick, full and stress. Review: 7 findings and 2 plausible defects fixed, 5 regression tests added. User decision: install ccache and split the integration builds.
- 2026-10-25: ccache 4.14.1 installed; integration builds split into `-c` plus link, with relative generated sources. Cold and warm cache measured, runner tester passed quick, full, g++-14 and stress, and the full library run passed.
- 2026-10-25: `/finish` audit and diff review: 20 defects fixed (integration interrupt and scheduling, stage signals, unit names, ccache probe, pre-flight interrupt, signal during stop, CLI reporter, evidence numbers, rerun noise, integration tests; then nohup, held signals, NOTE detail, 7 tests); 5 user decisions recorded (plan.py re-ranks, editdistance 360 s, defaults accepted, `_00_memory_cap.py` changes kept, no full baseline).
- 2026-11-10: P274 (SUP25, test speed): shared `Builder` with ccache for every C++ test build, one contest runner, concurrent builds, the 10 s quick bound, `OVERHALF` lines and the end-of-run NOTE; before and after measured cold and warm; whole-library quick, full, g++-14 full and one stress round passed except `08-palindrome_queries` stress, which failed on `2346cad` too and passes since the `/finish` fix. Second independent review of the later edits: 7 findings fixed (the `05-modint` compile limits, output relay on interrupt and stderr order in `01-run.py`, command labels, a nested tester's and a probe's over-half lines, the site retry's SKIP scope).
- 2026-11-12: run speed (user decisions 2026-11-12): `03-consistency.py` derives each Markdown path once and lints through a content-keyed cache (output byte-identical, timings under Validator), campaigns cache apart and this machine's shared cache is 30 GiB; `09-setup_tester.py` and `11-mutation_tester.py` full mode on python3 and with `CXX=g++-14` and stress seeds 1 and 2 passed, and `03-consistency.py` reports no errors.
- 2026-11-12: validator per batch and cache warning (user decisions 2026-11-12): `.claude/hooks/consistency.py` runs from `PostToolBatch`, once per batch with an Edit or Write on a watched path (live: one report for a batch of two edits; the event fires in subagents, probed on Claude Code 2.1.322), and says so when its input carries no `tool_calls` list; `/setup-check` warns at 90% of the cache limit; a `@reviewer` pass, 7 findings fixed (the 02-z stage figures in 00-mutation.md, the setup contract's ccache entries, a `..` path test gap, a silent stop on renamed input, stale wording); 10 hand mutants of the final hook killed, one equivalent guard removed; `12-hooks_tester.py` (15 tests) and `09-setup_tester.py` (74 tests) full mode on python3 and with `CXX=g++-14` and stress seeds 1 and 2 passed, `15-harness_tester.py` quick passed (105 tests), `03-consistency.py` reports no errors and `--selftest` passed.
