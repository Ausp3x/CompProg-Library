# 049-timer.hpp — Evidence

`049-timer.hpp` (batch MI32, package P229) provides a `steady_clock` stopwatch for time-limited loops and a scope-exit reporter. The row's `missing:` operation `Timer::measure` is not implemented. Prerequisite: the C01 template (P002); independent contest-profile code.

## Contracts

### Timer

`Timer` stores one `std::chrono::steady_clock::time_point`, taken at construction and by `reset()`. The clock is monotonic on every target (Linux `clock_gettime(CLOCK_MONOTONIC)`, MinGW `QueryPerformanceCounter`), so readings are nonnegative and never decrease between calls. Copies keep the start point. `elapsed()` is the raw `Clock::duration`; `elapsedUs()` is the integer microsecond count truncated toward zero; `elapsedMs()` and `elapsedSec()` are untruncated `double` values. Limits are milliseconds as `double`: `expired(limit)` is `elapsedMs() >= limit`, so a zero or negative limit is expired at once; `remaining(limit)` is `max(0, limit - elapsedMs())`; `progress(limit)` is `min(1, elapsedMs() / limit)` and asserts `limit > 0`. Each call reads the clock once (about 18 ns here, see Benchmarks); a hot loop that needs a cheaper check samples every `2^k` iterations, for example `if ((it & 1023) == 0 && t.expired(1900)) { break; }`. `double` keeps microsecond precision for any contest-scale time (53 significant bits cover `2^53` ms). There is no state beyond the start point and no threads, signals or platform headers.

### ScopedTimer

`ScopedTimer(label, os = cerr)` starts a `Timer` and on destruction writes exactly `label + ": " + <milliseconds with three fixed decimals> + " ms\n"` to `os` in one `operator<<` call. The line is formatted in a private `ostringstream`, so the flags, precision and fill of `os` stay untouched and no partial write interleaves. Objects nested in one scope report in reverse construction order. `ScopedTimer` is non-copyable; `label`, `os` and `timer` are public and may be inspected or changed before destruction. The default stream `cerr` is ignored by judges but still costs a write, so `ScopedTimer` stays out of inner loops.

## Feature-to-Test Map

| Public feature | Coverage |
|---|---|
| `Timer` construction, `reset`, `elapsed`, `elapsedUs`, `elapsedMs`, `elapsedSec` | Every reading is bracketed by independent `steady_clock` samples around the reset and the query and must lie in `[query start - reset end, query end - reset start]` (truncated for microseconds, with 1e-9 ms and 1e-12 s slack for the `double` conversion); random busy waits up to 2, 8 or 20 ms and random resets; copies keep `start`; 100,000 consecutive readings nondecreasing; a 3 ms wait is observed. |
| `expired`, `remaining`, `progress` | Limits −5, 0, 0.25, 1, 3, 20 and 1e6 ms on every bracket: `expired` must be true when the lower bound reaches the limit and false when the upper bound is below it; `remaining` lies in the bracket and is clamped at 0; `progress` lies in the bracket and is clamped to `[0, 1]`; `progress(0)` and `progress(-1)` are assertion probes. |
| `ScopedTimer` | Output captured in an `ostringstream`: exact prefix `label: `, exactly three decimals, suffix ` ms\n`, parsed value within the bracket of the enclosing scope, stream flags and precision unchanged, empty label, nested scopes report inner first, default stream `cerr`. |
| Clock contract | `static_assert` that `Timer::Clock` is `steady_clock` and `is_steady`. |

Quick, full and stress run 20, 120 and 600 bracket rounds. The bracket oracle is independent of the header (raw `steady_clock` arithmetic) and exact, with no fixed timing thresholds, so the suite does not depend on machine speed.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 659 MB; optimized 0.1 s compile + 0.6 s run, checked 10.3 s + 0.6 s, ASan-UBSan 14.4 s + 0.9 s; peak 735 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 708 MB; optimized 0.2 s + 0.6 s, checked 14.0 s + 0.6 s, ASan-UBSan 13.7 s + 0.8 s; peak 800 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 11.4 s + 0.1 s, checked 13.3 s + 0.1 s; peak 643 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 675 MB; optimized 0.1 s + 6.9 s, checked 3.5 s + 6.9 s, ASan-UBSan 4.1 s + 6.9 s; peak 389 MB |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/049-timer_tester.py' --mode quick --seed 1` | PASS, 2 configurations, 100,989 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/049-timer_tester.py' --mode full --seed 1` | PASS, 3 configurations, 105,709 checks, 2 assertion probes |
| 2026-10-08 (P229) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/049-timer_tester.py' --mode full --seed 2` | PASS, 3 configurations, 105,639 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/049-timer_tester.py' --mode stress --seed 3` | PASS, 3 configurations, 127,924 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS (standalone, 998-basic, 999-all, scalar/AVX2 two-TU, workspace, sanitizers); the header compiles warning-free with `-Wall -Wextra -Wconversion` on GCC 16.2 and 14.4 in a two-TU build through `999-all.hpp` |
| 2026-10-08 (P229) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

Not required by the contest profile. One one-off probe, with no driver or command kept, backs the sampling advice above: 10,000,000 `expired()` calls in a loop, GCC 16.2 `-O2`, i9-11900H, Linux vDSO clock, seven repetitions, median 17.8 ns per call (min 16.9, max 19.5).

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); the code was written independently:

- Nyaan `misc/timer.hpp`, ei1333 `other/chrono-timer.hpp` and `other/timer.hpp`, suisen `util/timer.hpp`, hitonanode `utilities/timer.hpp`, tko919 `Utility/timer.hpp`: elapsed time in ms, us and s plus reset are the common surface; ei1333's `rdtsc` timer is ISA-specific and excluded by the profile.
- shindannin, [AHC annealing template](https://shindannin.hatenadiary.com/entry/2021/03/06/115415): `progress` as the temperature input and a clock check every 10,000 steps (adopted as `progress`; sampling stays at the call site).

## Limits and Handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): a throttled cached expiry check, `elapsed<Unit>()`, `operator()`, iteration-count budgets, laps.
- `elapsedUs` truncates; `elapsedMs` and `elapsedSec` are exact to the clock's resolution. `expired` uses `>=`, so `expired(0)` is always true.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 卡时, 计时器, 计时, 运行时间.

## History

- 2026-10-08: created and verified (P229).
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
