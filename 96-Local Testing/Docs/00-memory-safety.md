# Memory-safe test execution and the Compact rule — evidence (P231)

Scope: memory-capped execution of every test entry point and its consistency check, plus the Compact-twin check for contest headers that use Core Barrett or Montgomery (P231, support batch SUP06; reason: [decision log](../../00-Guidelines/10-decisions.md), Memory cap).

## Design

[`_00_memory_cap.py`](../_00_memory_cap.py) provides `ensure()`. When a tester, benchmark driver or `02-integration.py` starts, `ensure()` turns the uncapped process into a small supervisor:

1. **Pre-flight.** For `full` or `stress` mode (`--mode`, an argparse abbreviation such as `--mo`, `--mode=`, `CP_TEST_MODE`, or the runners' default `full` when the entry point or its runner reads `CP_TEST_MODE`) it reruns the same entry point in quick mode under a 2048 MB cap (the heaviest quick suite, `01-Core/16-poly`, peaked at 1267 MB in the 2026-10-09 run, 79 suites) and skips the long run if quick fails or is killed. Its output is captured and only the tail is shown on failure.
2. **Main run.** It reruns the command inside a transient systemd user scope: `MemoryMax` is the cap (4096 MB default), `MemorySwapMax=0`, `OOMPolicy=continue`, and `--expand-environment=no` so `$` in arguments stays literal. The kernel kills only the process that exceeds the cap (exit -9); the runner survives and reports a FAIL; nothing outside the scope is touched. Without `OOMPolicy=continue`, systemd stops the whole scope with SIGTERM and the runner cannot report.
3. **Report.** Inside the scope an `atexit` hook prints `MEMORY peak=... cap=... oom_kills=...` from the scope's cgroup (`memory.peak`, `memory.events`), plus a FAIL line when the cap killed anything.

`ensure()` acts only when the running `__main__` file is a tester, benchmark or `02-integration.py`, so modules that merely import a runner (for example `03-consistency.py` importing the integration script) are unaffected. A nested call inside a capped process only registers the report. Without a systemd user manager the run continues with an explicit `WARNING ... WITHOUT a memory cap` (local testing targets this Linux machine; judges are unaffected).

Wiring: the folder runners and `01-Core/_05_modint_test_runner.py` call `ensure()` on import, which covers the testers that use them. Standalone testers, benchmark drivers and `02-integration.py` call it directly. `01-run.py` needs no change, because every suite it launches caps itself.

## Rules

P231's rules are owned by [05-testing.md](../../00-Guidelines/05-testing.md) (Memory safety), [03-cpp.md](../../00-Guidelines/03-cpp.md) (Toolchain and judges: bounded precondition failures under `NDEBUG`; Headers: Compact twins) and [01-principles.md](../../00-Guidelines/01-principles.md) (Profiles).

`03-consistency.py` enforces the cap and the Compact rule. An entry point without `_00_memory_cap.ensure()` (directly or through a capped runner) is an error. A verified contest header that uses Barrett or Montgomery without a `Compact` definition is an error. So is any `*Compact` body that names `Montgomery*`, `Barrett*` or a `mont*` helper. The check cannot tell which fast functions need a twin; `@reviewer` enforces that. Unverified ones are listed under `compact_pending`.

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

## Commands and results (2026-10-09)

```bash
python3 '96-Local Testing/01-run.py' --mode quick --no-integration   # PASS, 79 suites, each self-capped: 79 MEMORY lines, oom_kills=0; largest peaks 1267 MB (01-Core/16-poly), 1027 MB (05-dynmodint64), 968 MB (11-sorting_selection)
python3 '96-Local Testing/02-integration.py'                         # PASS, 112 headers, self-capped, peak 654 MB
python3 '96-Local Testing/03-consistency.py'                         # no errors, compact_pending empty
```

## Limits

- The cap needs a systemd user manager and cgroup v2. Elsewhere, runs proceed uncapped with a warning.
- Three standalone testers (`00-Tools/02-online`, `00-Tools/03-notebook`, `01-Core/02-debug_compile`) read no `CP_TEST_MODE`. When their mode is not given explicitly they get the main cap but no pre-flight.
- A SIGKILL is attributed to the cap only when the scope reports `oom_kills > 0`; otherwise the message says it may be an external kill.

## History

- 2026-10-09: P231 introduced after the P052 incident (also guarding `01-Core/16-poly_tester.py` and `16-poly_benchmark.py`, then another package's uncommitted work); review fixed four findings; quick run of all suites, integration and consistency passed.
- 2026-10-24: P072 `/finish` (user decision 2026-10-24): the six folder runners (`02`–`07`) accept `--configuration ASan-UBSan` in quick mode, so the pre-flight of a full or stress ASan retry no longer fails; checked with `55-fractional_programming_tester.py --mode full --configuration ASan-UBSan`.
- 2026-10-24/25: P247 (user decision 2026-10-25: kept), recorded in [00-parallel.md](00-parallel.md):
  - `supervise()` and the CLI reporter ignore SIGINT, so a capped child finishes its own cleanup instead of being killed 0.25 s after Ctrl-C or a group SIGINT.
  - An interrupted pre-flight prints `INTERRUPTED` instead of a FAIL.
  - A second `ensure()` in one process prints its `MEMORY` line once.
