# Memory-safe test execution and the Compact rule — evidence (P231)

P231 (support batch SUP06) followed the P052 incident. A Python oracle row sent `n = 0` to an optimized (`-DNDEBUG`) build of `factorize`, whose stripped precondition left an infinite `push_back` loop. The process consumed all free memory and the kernel killed the user's editor. P231 makes such a failure impossible to escape a test run, removes its cause from library code, and adds the Compact-alternative rule for contest headers that use Core Barrett or Montgomery.

## Design

[`_00_memory_cap.py`](_00_memory_cap.py) provides `ensure()`. When a tester, benchmark driver or `02-integration.py` starts, `ensure()` turns the uncapped process into a small supervisor:

1. **Pre-flight.** For `full` or `stress` mode (`--mode`, an argparse abbreviation such as `--mo`, `--mode=`, `CP_TEST_MODE`, or the runners' default `full` when the entry point or its runner reads `CP_TEST_MODE`) it reruns the same entry point in quick mode under a 2048 MB cap (the heaviest quick suite, `01-Core/16-poly`, peaks at 1267 MB) and skips the long run if quick fails or is killed. Its output is captured and only the tail is shown on failure.
2. **Main run.** It reruns the command inside a transient systemd user scope: `MemoryMax` is the cap (4096 MB default), `MemorySwapMax=0`, `OOMPolicy=continue`, and `--expand-environment=no` so `$` in arguments stays literal. The kernel kills only the process that exceeds the cap (exit -9); the runner survives and reports a FAIL; nothing outside the scope is touched. Without `OOMPolicy=continue`, systemd stops the whole scope with SIGTERM and the runner cannot report.
3. **Report.** Inside the scope an `atexit` hook prints `MEMORY peak=... cap=... oom_kills=...` from the scope's cgroup (`memory.peak`, `memory.events`), plus a FAIL line when the cap killed anything.

`ensure()` acts only when the running `__main__` file is a tester, benchmark or `02-integration.py`, so modules that merely import a runner (for example `03-consistency.py` importing the integration script) are unaffected. A nested call inside a capped process only registers the report. `ensure(mb=...)` changes one suite's cap, and `CP_TEST_MEMORY_MB`, `CP_TEST_PREFLIGHT_MB` and `CP_TEST_PREFLIGHT=0` override it from the environment. `python3 '96-Local Testing/_00_memory_cap.py' [--mb N] -- <command>` caps any other program and reports its peak. Without a systemd user manager the run continues with an explicit `WARNING ... WITHOUT a memory cap` (local testing targets this Linux machine; judges are unaffected).

Wiring: the seven folder runners and `01-Core/_05_modint_test_runner.py` call `ensure()` on import, which covers the 71 testers that use them. The 11 standalone testers, all 20 benchmark drivers and `02-integration.py` call it directly. `01-run.py` needs no change, because every suite it launches caps itself.

## Rules added

- `05-testing.md`, Memory safety: every entry point capped, pre-flight before full/stress, peaks recorded in evidence, ad-hoc programs through the CLI, out-of-domain inputs only through checked-build probes.
- `03-cpp.md`, Toolchain: a violated precondition must stay bounded when `NDEBUG` removes the assertion (no infinite loop, no unbounded allocation).
- `03-cpp.md`, Headers, and `01-principles.md`: Compact twins. A contest header that uses Core `Barrett*`/`Montgomery*` defines dependency-free `<name>Compact` twins of the functions that perform the reduced arithmetic themselves.
- `CLAUDE.md`, the `/package` skill and the `@reviewer` agent name the capped CLI and the Compact rule.

`03-consistency.py` enforces both. An entry point without `_00_memory_cap.ensure()` (directly or through a capped runner) is an error. A verified contest header that uses Barrett or Montgomery without a `Compact` definition is an error. So is any `*Compact` body that names `Montgomery*`, `Barrett*` or a `mont*` helper. The check cannot tell which fast functions need a twin; `@reviewer` enforces that. Unverified ones are listed under `compact_pending`. At introduction the only flagged header was `05-Mathematics/07-primality_factorization.hpp`, rerun in P052 with its twins.

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
| Consistency on the repository | no errors; `compact_pending` empty after the P052 rerun |
| Whole-library quick run with every suite self-capped | 79 suites PASS, every one reports its peak, no kills |

## Commands and results

```bash
python3 '96-Local Testing/01-run.py' --mode quick --no-integration   # PASS, 79 suites, each self-capped: 79 MEMORY lines, oom_kills=0; largest peaks 1267 MB (01-Core/16-poly), 1027 MB (05-dynmodint64), 968 MB (11-sorting_selection)
python3 '96-Local Testing/02-integration.py'                         # PASS, 112 headers, self-capped, peak 654 MB
python3 '96-Local Testing/03-consistency.py'                         # no errors, compact_pending empty
```

## Review

`@reviewer` (2026-10-09) confirmed four P231 findings, all fixed and retested with the demo tester:

| Finding | Fix |
|---|---|
| Pre-flight skipped when the mode was defaulted or abbreviated (`--mo full`) | `mode_of` recognizes abbreviations and `--mode=`, and treats a missing mode as `full` when the entry point honours `CP_TEST_MODE` |
| Without systemd, the child still printed the editor scope's peak and the pre-flight was skipped | the fallback marks `CP_MEMORY_CAPPED=none` (no report, no recursion) and still runs the pre-flight, with a WARNING |
| Exit status 247 for a kill, and every SIGKILL blamed on the cap | exit statuses are shell-style (137); the message distinguishes a cap kill (`oom_kills > 0`) from an external kill |
| `--mb=128` and a missing command ended in a traceback | `--mb N` and `--mb=N` are parsed and validated, with the usage message on error |

Minor notes adopted: the Compact check now inspects twin bodies.

## Limits

- The cap needs a systemd user manager and cgroup v2. Elsewhere, runs proceed uncapped with a warning.
- Five standalone testers (`00-Tools/02-online`, `00-Tools/03-notebook`, `01-Core/02-debug_compile`, `07-infint`, `08-infintmini`) read no `CP_TEST_MODE`. When their mode is not given explicitly they get the main cap but no pre-flight.
- A SIGKILL is attributed to the cap only when the scope reports `oom_kills > 0`; otherwise the message says it may be an external kill.
- The guard was also inserted into `01-Core/16-poly_tester.py` and `16-poly_benchmark.py`, which belonged to another package's uncommitted work when P231 ran.

## History

- 2026-10-09: P231 introduced after the P052 incident.
