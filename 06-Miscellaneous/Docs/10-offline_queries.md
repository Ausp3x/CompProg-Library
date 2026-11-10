# 10-offline_queries.hpp — evidence

`10-offline_queries.hpp` (batch MI03, package P014) provides the offline threshold sweep and offline range threshold counting. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified.

## Contracts

Common rules: see [00-notes.md](00-notes.md), Basic contracts.

### `OfflineThreshold<K>` and `offlineSweep(updates, queries, apply, answer, cmp)`

A strict threshold accepts update keys `u` with `cmp(u, key)`. An inclusive threshold also accepts keys equivalent to `key`. Keys need only `cmp`: no arithmetic or equality. The sweep sorts updates by `(key, index)` and queries by `(key, strict before inclusive, index)`. It calls `apply(update index)` and then `answer(query index)`, with exactly the eligible updates applied. Equal-key updates keep input order. Each update is applied at most once. Updates beyond the last answered query are never applied. With no queries, no callback runs. Callbacks must not mutate keys or the comparator's order, and `answer` must not change the accumulated state. Captured state belongs to the caller and is not reset. It costs O(n * log(n + 1) + q * log(q + 1)) time and O(n + q) space, plus callbacks. Correctness: the applied updates are always exactly the eligible prefix of the sorted updates for the current query; strict queries at a tied key run before the updates at that key, inclusive ones after.

### `OfflineRangeThreshold<K>` and `rangeCountOffline(a, queries, cmp)`

Each query `{l, r, key, inclusive}` counts the values in `a[l, r)` that pass its threshold. The domain is `0 <= l <= r <= n` (asserted), and empty ranges are valid. Answers come back in query order. Each eligible index is activated once in a `Fenwick<int>`, so counts fit `int`.

### Legacy mapping

- P276 rename, 2026-11-10: `offlineRangeCount` → `rangeCountOffline`.

## Feature-to-test map

| Operation | Oracle and cases |
|---|---|
| `OfflineThreshold`, `offlineSweep` | Repeated-minimum reference order; noncommutative state compared with direct eligibility at each answer; callback order; state unchanged after the last answer; ascending, descending, stateful and opaque comparators; 64/128-bit keys; exceptions; repeated calls |
| `OfflineRangeThreshold`, `rangeCountOffline` | Every subrange of exhaustive ternary arrays scanned directly in both comparator directions; random and full-width arrays; five range asserts; large duplicate and reverse fixtures |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 1032 MB; optimized 0.3 s compile + 0.6 s run, checked 42.6 s compile + 3.2 s run, ASan-UBSan 48.1 s compile + 4.1 s run; MEMORY peak 1540 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 1078 MB; optimized 0.3 s compile + 0.8 s run, checked 59.5 s compile + 8.8 s run, ASan-UBSan 60.4 s compile + 12.4 s run; MEMORY peak 1563 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 57.0 s compile + 0.1 s run, checked 41.8 s compile + 1.4 s run; MEMORY peak 1043 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 1045 MB; optimized 0.1 s compile + 1.5 s run, checked 19.2 s compile + 8.8 s run, ASan-UBSan 26.8 s compile + 20.9 s run; MEMORY peak 854 MB. Dated rows below stay those commands' latest results.

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08. The P014 integration and consistency checks of that date also passed: `python3 '96-Local Testing/02-integration.py'` (102 standalone and aggregate headers, multi-TU builds, workspace) and `python3 '96-Local Testing/03-consistency.py'` with `--braces` on the four P014 headers and testers.

- Full: `python3 '96-Local Testing/06-Miscellaneous/10-offline_queries_tester.py' --mode full --seed 20261008`: PASS (table below); with `CXX=g++-14` (GCC 14.4.1): PASS in all three configurations.
- Stress, one round: `python3 '96-Local Testing/06-Miscellaneous/10-offline_queries_tester.py' --mode stress --seed 7`: PASS.

| Suite | Full, seed 20261008 (optimized, checked, ASan/UBSan) | Stress, seed 7, one round (same three) | Probes |
|---|---|---|---|
| `10-offline_queries` | PASS: 1414709 checks per configuration | PASS in 88 s: 8158221 checks | 5 |

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting/selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs in the package; no external code was copied. Sources read in the 2026-09-28 audit: OI Wiki 离线算法简介. The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md).

## Limits and handoffs

- Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- The code uses no `long` or POSIX-only calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): offline set intersection, RMQ, range k-th and arithmetic-progression adds.

## Aliases

Renamed aliases (P276, 2026-11-10): `offlineRangeCount`.

Search aliases (P263, grab --find): 离线处理, 离线查询, 离线排序, 区间小于k的数个数, 二维数点.

## History

- 2026-09-28: first implementation and audit (MI03, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before changes.
- 2026-10-08: re-audit, no operation missing, 10 findings fixed; full suite seed 20261008 passed on g++ and g++-14, stress seed 7 passed, integration and consistency clean.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-10: P276 rename (`offlineRangeCount` → `rangeCountOffline`); full on g++ and g++-14, stress and extraction passed.
