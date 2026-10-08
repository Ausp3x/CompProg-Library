# 49-timer.hpp — evidence

`49-timer.hpp` (batch MI32, package P229) provides a `steady_clock` stopwatch for time-limited loops and a scope-exit reporter. Prerequisite: the C01 template (P002). Independent contest-profile code; no online submission was made.

## Contracts

### Timer

`Timer` stores one `std::chrono::steady_clock::time_point`, taken at construction and by `reset()`. The clock is monotonic on every target (Linux `clock_gettime(CLOCK_MONOTONIC)`, MinGW `QueryPerformanceCounter`), so every reading is nonnegative and readings never decrease between calls. Copies keep the start point. Readings: `elapsed()` is the raw `Clock::duration`; `elapsedUs()` is the integer microsecond count truncated toward zero; `elapsedMs()` and `elapsedSec()` are `double` values with fractional precision (no truncation). Limits are milliseconds as `double`: `expired(limit)` is `elapsedMs() >= limit`, so a zero or negative limit is expired immediately; `remaining(limit)` is `max(0, limit - elapsedMs())`; `progress(limit)` is `min(1, elapsedMs() / limit)` and asserts `limit > 0`. Each call reads the clock once (about 18 ns locally, see Benchmarks); callers that need a cheaper check in a hot loop sample every `2^k` iterations, for example `if ((it & 1023) == 0 && t.expired(1900)) { break; }`. `double` keeps microsecond precision for any contest-scale elapsed time (53 significant bits cover `2^53` ms). No state beyond the start point; no threads, signals or platform headers.

### ScopedTimer

`ScopedTimer(label, os = cerr)` starts a `Timer` and, on destruction, writes exactly `label + ": " + <milliseconds with three fixed decimals> + " ms\n"` to `os` in one `operator<<` call. The line is formatted in a private `ostringstream`, so the flags, precision and fill of `os` are left untouched and the line is not interleaved with partial writes. Objects nested in one scope report in reverse construction order. Non-copyable; `label`, `os` and `timer` are public and may be inspected or changed before destruction. Output goes to `cerr` by default, which judges ignore but which costs a write; keep `ScopedTimer` out of inner loops.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| `Timer` construction, `reset`, `elapsed`, `elapsedUs`, `elapsedMs`, `elapsedSec` | Every reading is bracketed by independent `steady_clock` samples taken around the reset and around the query: it must lie in `[query start - reset end, query end - reset start]` (truncated for microseconds, with 1e-9 ms / 1e-12 s slack for `double` conversion); random busy waits up to 2/8/20 ms, random resets; copies keep `start`; 100,000 consecutive readings nondecreasing; a 3 ms wait is observed. |
| `expired`, `remaining`, `progress` | Limits −5, 0, 0.25, 1, 3, 20 and 1e6 ms on every bracket: expired forced true when the lower bound reaches the limit and false when the upper bound is below it; `remaining` within the bracket and clamped at 0; `progress` within the bracket and clamped to `[0, 1]`; `progress(0)` and `progress(-1)` are assertion probes. |
| `ScopedTimer` | Output captured in an `ostringstream`: exact prefix `label: `, digits with exactly three decimals, suffix ` ms\n`, parsed value within the bracket of the enclosing scope, stream flags and precision unchanged, empty label, nested scopes report inner first, default stream is `cerr`. |
| Clock contract | `static_assert` that `Timer::Clock` is `steady_clock` and `is_steady`. |

## Commands and results

Run 2026-10-08: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` with leak checking.

```bash
python3 '96-Local Testing/06-Miscellaneous/49-timer_tester.py' --mode quick --seed 1             # PASS, 2 configurations, 100,989 checks
python3 '96-Local Testing/06-Miscellaneous/49-timer_tester.py' --mode full --seed 1              # PASS, 3 configurations, 105,709 checks, 2 assertion probes
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/49-timer_tester.py' --mode full --seed 2   # PASS, 3 configurations, 105,639 checks
python3 '96-Local Testing/06-Miscellaneous/49-timer_tester.py' --mode stress --seed 3            # PASS, 3 configurations, 127,924 checks
python3 '96-Local Testing/02-integration.py' --sanitizers                                        # PASS (standalone, 98-basic, 99-all, scalar/AVX2 two-TU, workspace, sanitizers)
python3 '96-Local Testing/03-consistency.py'                                                     # no errors
```

Quick/full/stress use 20/120/600 bracket rounds. The bracket oracle is independent of the header (raw `steady_clock` arithmetic) and is exact: no fixed timing thresholds are used, so the suite does not depend on machine speed. The header compiles warning-free with `-Wall -Wextra -Wconversion` under GCC 16.2 and 14.4 in a two-translation-unit build through `99-all.hpp`.

## Benchmarks

Not required by the contest profile. One observation for the throttling advice above: 10,000,000 `expired()` calls in a loop, GCC 16.2 `-O2`, i9-11900H, Linux vDSO clock, seven repetitions, median 17.8 ns per call (min 16.9, max 19.5). Windows `QueryPerformanceCounter` was not measured.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); code written independently:

- Nyaan `misc/timer.hpp`, ei1333 `other/chrono-timer.hpp` and `other/timer.hpp`, suisen `util/timer.hpp`, hitonanode `utilities/timer.hpp`, tko919 `Utility/timer.hpp`: elapsed in ms/us/s and reset are the common surface; ei1333's `rdtsc` timer is ISA-specific and excluded by the profile.
- shindannin, [AHC annealing template](https://shindannin.hatenadiary.com/entry/2021/03/06/115415): `progress` as the temperature input and checking the clock every 10,000 steps (adopted as `progress`; throttling stays at the call site).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): throttled cached expiry check, `measure(f)`, `elapsed<Unit>()`, `operator()`, iteration-count budgets, laps.
- `elapsedUs` truncates; `elapsedMs`/`elapsedSec` are exact to clock resolution. `expired` uses `>=`, so `expired(0)` is always true.
- Exact GCC 14.2 and the Windows build were not run; `steady_clock` resolution on MinGW was not measured.

## History

- 2026-10-08: created and verified (P229).
