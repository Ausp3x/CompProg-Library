# Memory-Safe Test Execution — Evidence (P231)

Scope: memory-capped execution of every test entry point and its consistency check (P231, support batch SUP06; reason: [decision log](../../00-Guidelines/10-decisions.md), Memory cap). The Compact-twin check P231 also added retired on 2026-11-05 with the Compact rule (P253; the mini-first checks replacing it: [05-testing.md](../../00-Guidelines/05-testing.md), Extraction).

## Design

[`_00_memory_cap.py`](../_00_memory_cap.py) provides `ensure()`. When a tester, benchmark driver or `02-integration.py` starts, `ensure()` turns the uncapped process into a small supervisor:

1. **Pre-flight.** In `full` or `stress` mode it reruns the same entry point in quick mode under a 2048 MB cap and skips the long run if quick fails or is killed. The mode is read from `--mode` (an argparse abbreviation such as `--mo` and `--mode=` included), from `CP_TEST_MODE`, or from the runners' default `full` when the entry point or its runner reads `CP_TEST_MODE`. The heaviest quick suite, `01-Core/16-poly`, peaked at 1267 MB in the 2026-10-09 run of 79 suites. The pre-flight's output is captured, and only its tail is shown on failure.
2. **Main run.** It reruns the command inside a transient systemd user scope with `MemoryMax` at the cap (4096 MB default), `MemorySwapMax=0`, `OOMPolicy=continue`, and `--expand-environment=no` so that `$` in arguments stays literal. The kernel kills only the process that exceeds the cap (exit -9). The runner survives and reports a FAIL, and nothing outside the scope is touched. Without `OOMPolicy=continue`, systemd would stop the whole scope with SIGTERM and the runner could not report.
3. **Report.** Inside the scope an `atexit` hook prints `MEMORY peak=... cap=... oom_kills=...` from the scope's cgroup (`memory.peak`, `memory.events`), plus a FAIL line when the cap killed anything.
4. **Admission** (SUP33, after the out-of-memory kills of 2026-11-17 and 2026-11-18). Before the pre-flight, `supervise()` waits in [`_00_memory_gate.py`](../_00_memory_gate.py) until the machine's free memory covers the run's expected peak beside the gated runs of every session; the contracts and tests are in [00-harness-review.md](../../00-Guidelines/13-Plan/Docs/00-harness-review.md), Memory Admission Gate.

`ensure()` acts only when the running `__main__` file is a tester, benchmark, `02-integration.py` or `06-compare.py` (`ENTRY_NAMES`), so modules that merely import a runner (for example `03-consistency.py` importing the integration script) are unaffected. A nested call inside a capped process only registers the report. Without a systemd user manager the run continues with an explicit `WARNING ... WITHOUT a memory cap` (local testing targets this Linux machine; judges are unaffected).

Signals and supervisor death:

- **Forwarding** (P252, user decision 2026-11-04). `supervise()` forwards SIGTERM and SIGHUP to the capped child through `forwarding()`, in the pre-flight and the main run, so a signal sent to a capped entry point's top-level pid does not kill the supervisor alone and orphan the capped run. A signal inherited as ignored stays ignored (`nohup`), and the previous handlers are restored when the child exits. SIGINT handling is unchanged: `supervise()` and the CLI reporter ignore SIGINT, so a capped child finishes its own cleanup (P247, user decision 2026-10-25). `systemd-run --scope` (systemd 262) execs the command, so the forwarded signal reaches the capped process itself. The CLI reporter runs its command through `forwarding()` too, so `_00_memory_cap.py -- <command>` does not orphan it either.
- **Supervisor death** (user decision 2026-11-16; [decision log](../../00-Guidelines/10-decisions.md), Leftover processes). A SIGKILL cannot be forwarded: a scratch driver run through the CLI with `subprocess.run(..., timeout=60)` once burned a core for 35 hours after its session ended, its scope reparented to the user manager. `supervise()` therefore gives the scope's first process SIGUSR2 as its parent-death signal (`prctl(PR_SET_PDEATHSIG)`, kept across `systemd-run`'s exec) and sets `CP_MEMORY_SUPERVISOR`. `lead()`, called by `ensure()` inside the cap and by the CLI reporter, turns that signal, or a supervisor already gone, into `end_scope()`, which writes `cgroup.kill` of the `run-*.scope` (one process at a time through `cgroup.procs` on kernels before 5.14).
- **`/reap`** ([`reap.py`](../../.claude/skills/reap/reap.py), tester [`17-reap_tester.py`](../00-Tools/17-reap_tester.py), feature map in its docstring) ends what still escapes, by the `CLAUDE_PID` every session's processes carry, and deletes the temporaries killed runs leave in `/tmp` (on 2026-11-16, 597 `cp-contest-*` directories, 164 gcc intermediates and 50 clangd preambles held 3.5 GB, 2.4 GB of it stale). `--scratchpads [HOURS]`, which `/package` and `/finish` pass, deletes ended sessions' scratchpads idle for 24 hours (user decision 2026-11-16; rules in `reap.py`'s docstring). The detached harness audit scan carries `CP_REAP_KEEP=1`, and project paths match on directory boundaries, so a sibling checkout such as `CompProg Library2` is not taken for this project.
- **Core dumps.** Capped runs set `RLIMIT_CORE=1`, so a killed mutant or death probe skips the piped `core_pattern` (P274, [00-parallel.md](00-parallel.md)).

Wiring: `_00_contest_runner.py` and `01-Core/_05_modint_test_runner.py` call `ensure()` on import, which covers the testers that use them. Standalone testers, benchmark drivers and `02-integration.py` call it directly. `01-run.py` needs no change, because every suite it launches caps itself.

## Rules

P231's rules are owned by [05-testing.md](../../00-Guidelines/05-testing.md) (Memory Safety), [03-cpp.md](../../00-Guidelines/03-cpp.md) (Toolchain and Judges: bounded precondition failures under `NDEBUG`) and [01-principles.md](../../00-Guidelines/01-principles.md) (Profiles).

`03-consistency.py` enforces the cap: an entry point without `_00_memory_cap.ensure()` (directly or through a capped runner) is an error.

## Verification

| Check | Result |
|---|---|
| Unbounded allocation under `--mb 200` through the CLI | killed alone, `MEMORY peak=200MB oom_kills=1`, FAIL line, exit 137 |
| Normal command through the CLI, argument `'hello $HOME'` | passes unchanged (no `$` expansion), peak 7 MB, exit 0 |
| Demo tester, full mode, no blowup | pre-flight quick passes, full passes, both peaks reported |
| Demo tester allocating in every mode | pre-flight killed at its cap; full run skipped with an explicit FAIL |
| Demo tester with no `--mode` (defaults to full via `CP_TEST_MODE`), and with `--mo full` | pre-flight runs in both cases |
| Demo tester with `systemd-run` removed from `PATH` | WARNING, uncapped run, pre-flight still runs, no misleading `MEMORY` line, no re-exec loop |
| CLI with `--mb=128` and no command, `--mb 128` and no command, `--mb=128 -- true` | usage message, usage message, pass with the 128 MB cap |
| Demo tester allocating only in full/stress | pre-flight passes; main run's child killed at the main cap; FAIL reported, runner survives |
| Consistency negative test (a temporary tester without the guard) | rejected as `Uncapped test entry point` |
| Consistency on the repository | no errors |
| Whole-library quick run with every suite self-capped | 79 suites PASS, every one reports its peak, no kills |

## Commands and Results

```bash
python3 '96-Local Testing/00-Tools/05-runner_tester.py'                         # 2026-11-22: PASS, 64 tests, 86 s (05-modint.md, P005 /finish)
python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode full --seed 1    # 2026-11-16: PASS, 63 tests, test_supervisor_death_ends_scope included
python3 '96-Local Testing/00-Tools/17-reap_tester.py' --mode full              # 2026-11-16: PASS, 10 tests, quick too; caught each of 25 seeded defects
python3 '96-Local Testing/01-run.py' --mode quick --no-integration              # 2026-10-09: PASS, 79 suites, each self-capped: 79 MEMORY lines, oom_kills=0; largest peaks 1267 MB (01-Core/16-poly), 1027 MB (05-dynmodint64), 968 MB (11-sorting_selection)
python3 '96-Local Testing/02-integration.py'                                    # 2026-10-09: PASS, 112 headers, self-capped, peak 654 MB
python3 '96-Local Testing/03-consistency.py'                                    # 2026-10-09: no errors
```

Seeded defects: `test_supervisor_death_ends_scope` fails on the file before 2026-11-16 (the scope and its grandchild survive, in both forms). The forwarding tests (`test_forward_sigterm`, `test_forward_sighup`, `test_forward_preflight`, `test_forward_cli`, `test_forward_nohup_keeps_sighup_ignored`, `test_forwarding_restores_handlers` in [`05-runner_tester.py`](../00-Tools/05-runner_tester.py)) each caught at least one of seven seeded defects (2026-11-04): the old `subprocess.run` in either run or in the CLI reporter, forwarding an ignored SIGHUP, no restore, SIGTERM only. On 2026-11-16 `/reap` deleted 27 scratchpads of ended sessions (585 MB) and kept the 3 live sessions'.

## Limits

- A capped command that changes its own session (`setsid`) still ends with the scope; one started outside a scope by hand is only reported by `/reap`.
- The cap needs a systemd user manager and cgroup v2. Elsewhere, runs proceed uncapped with a warning.
- Three standalone testers (`00-Tools/02-online`, `00-Tools/03-notebook`, `01-Core/02-debug_compile`) read no `CP_TEST_MODE`. When their mode is not given explicitly they get the main cap but no pre-flight.
- A SIGKILL is attributed to the cap only when the scope reports `oom_kills > 0`; otherwise the message says it may be an external kill.

## History

- 2026-10-09: P231 introduced after the P052 incident (also guarding `01-Core/16-poly_tester.py` and `16-poly_benchmark.py`, then another package's uncommitted work); review fixed four findings; quick run of all suites, integration and consistency passed.
- 2026-10-24: P072 `/finish` (user decision 2026-10-24): the six folder runners (`02`–`07`) accept `--configuration ASan-UBSan` in quick mode, so the pre-flight of a full or stress ASan retry no longer fails; checked with `55-fractional_programming_tester.py --mode full --configuration ASan-UBSan`.
- 2026-10-24/25: P247 (user decision 2026-10-25: kept, [00-parallel.md](00-parallel.md)): `supervise()` and the CLI reporter ignore SIGINT; an interrupted pre-flight prints `INTERRUPTED` instead of a FAIL; a second `ensure()` in one process prints its `MEMORY` line once.
- 2026-11-04: P252 `/finish` (user decision 2026-11-04): SIGTERM and SIGHUP forwarding in `supervise()` and the CLI reporter; six tests, seven seeded defects caught.
- 2026-11-05: P253 retired the Compact-twin check with the Compact rule.
- 2026-11-09: P274: the six folder runners merged into `_00_contest_runner.py`, one directory above its testers, so `honours_mode_env` also reads the runner beside the tester's folder; every suite runs up to three builds at once within its cap; `RLIMIT_CORE=1` in capped runs.
- 2026-11-16 (user decisions 2026-11-16): parent-death signal and `end_scope()` after a 35-hour leftover driver; `/reap` with `--scratchpads`; `05-runner_tester.py` full passed (63 tests), `17-reap_tester.py` quick and full passed (10 tests, 6 then 25 seeded defects caught).
- 2026-11-22: P005 `/finish` ran `05-runner_tester.py`: 64 tests pass.
- 2026-11-23: SUP33: the memory admission gate in `supervise()` (`--need`, `ensure(need=...)`); `05-runner_tester.py` full passed (82 tests, after both reviews' gate fixes) on both compilers; `06-compare.py` became an entry point (user decision 2026-11-23).
