# 10-offline_queries.hpp — Evidence

`10-offline_queries.hpp` (batch MI03, package P014) provides the offline threshold sweep and offline range threshold counting. Every implemented operation of the row has an independent-oracle test (map below); the row's `missing:` operations are not implemented. Prerequisites P002, P006 and P013 are verified.

## Contracts

Common rules: [00-notes.md](00-notes.md), Basic Contracts.

### `OfflineThreshold<K>` and `offlineSweep(updates, queries, apply, answer, cmp)`

A strict threshold accepts the update keys `u` with `cmp(u, key)`; an inclusive threshold also accepts keys equivalent to `key`. Keys need only `cmp`, with no arithmetic or equality. The sweep sorts updates by `(key, index)` and queries by `(key, strict before inclusive, index)`, then calls `apply(update index)` and `answer(query index)` so that exactly the eligible updates are applied at each answer. Equal-key updates keep input order, each update is applied at most once, and updates beyond the last answered query are never applied; with no queries no callback runs. Callbacks must not mutate keys or the comparator's order, and `answer` must not change the accumulated state. Captured state belongs to the caller and is not reset. Cost: O(n * log(n + 1) + q * log(q + 1)) time and O(n + q) space, plus the callbacks. Correctness: the applied updates are always exactly the eligible prefix of the sorted updates for the current query, since strict queries at a tied key run before that key's updates and inclusive ones after.

### `OfflineRangeThreshold<K>` and `rangeCountOffline(a, queries, cmp)`

Each query `{l, r, key, inclusive}` counts the values in `a[l, r)` that pass its threshold. Domain `0 <= l <= r <= n` (asserted); empty ranges are valid. Answers come back in query order. Each eligible index is activated once in a `Fenwick<int>`, so counts fit `int`.

### Legacy Mapping

- P276 rename, 2026-11-10: `offlineRangeCount` → `rangeCountOffline`.

## Feature-to-Test Map

| Operation | Oracle and cases |
|---|---|
| `OfflineThreshold`, `offlineSweep` | Repeated-minimum reference order; noncommutative state compared with direct eligibility at each answer; callback order; state unchanged after the last answer; ascending, descending, stateful and opaque comparators; 64- and 128-bit keys; exceptions; repeated calls |
| `OfflineRangeThreshold`, `rangeCountOffline` | Every subrange of exhaustive ternary arrays scanned directly in both comparator directions; random and full-width arrays; five range asserts; large duplicate and reverse fixtures |

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, Python 3.14.7, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds. The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 1032 MB; optimized 0.3 s compile + 0.6 s run, checked 42.6 s + 3.2 s, ASan-UBSan 48.1 s + 4.1 s; peak 1540 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 1078 MB; optimized 0.3 s + 0.8 s, checked 59.5 s + 8.8 s, ASan-UBSan 60.4 s + 12.4 s; peak 1563 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 57.0 s + 0.1 s, checked 41.8 s + 1.4 s; peak 1043 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 1045 MB; optimized 0.1 s + 1.5 s, checked 19.2 s + 8.8 s, ASan-UBSan 26.8 s + 20.9 s; peak 854 MB |
| 2026-11-10 (P276) | the tester in full mode on g++ and g++-14, stress, and `05-extraction.py` | passed (counts not recorded) |
| 2026-10-08 (P014) | `python3 '96-Local Testing/06-Miscellaneous/10-offline_queries_tester.py' --mode full --seed 20261008` | PASS in all three configurations, 1,414,709 checks each, 5 assertion probes; with `CXX=g++-14`: PASS in all three |
| 2026-10-08 (P014) | `python3 '96-Local Testing/06-Miscellaneous/10-offline_queries_tester.py' --mode stress --seed 7` (one round) | PASS in 88 s, 8,158,221 checks |
| 2026-10-08 (P014) | `python3 '96-Local Testing/02-integration.py'` | PASS: 102 standalone and aggregate headers, multi-TU builds, workspace |
| 2026-10-08 (P014) | `python3 '96-Local Testing/03-consistency.py'`, with `--braces` on the four P014 headers and testers | PASS |

## Benchmarks

No benchmark: there is no tuned threshold or specialized backend. The P014 sorting and selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs of the package; no external code was copied. The 2026-09-28 audit read OI Wiki 离线算法简介. The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md).

## Limits and Handoffs

- Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's and are not asserted.
- The code uses no `long` and no POSIX-only calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) (Basic Contracts, P014 omissions): offline set intersection, RMQ, range k-th and arithmetic-progression adds.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Renamed aliases (P276, 2026-11-10): `offlineRangeCount`.

Search aliases (P263, grab --find): 离线处理, 离线查询, 离线排序, 区间小于k的数个数, 二维数点.

## History

- 2026-09-28: first implementation and audit (MI03, P014); full passed.
- 2026-10-08: re-audit baseline; full with seed 1 passed in 3 configurations before changes.
- 2026-10-08: re-audit found no missing operation and fixed 10 findings; full with seed 20261008 passed on g++ and g++-14, stress with seed 7 passed, integration and consistency clean.
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-10: P276 renamed `offlineRangeCount` to `rangeCountOffline`; full on g++ and g++-14, stress and extraction passed.
