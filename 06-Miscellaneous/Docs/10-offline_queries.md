# 10-offline_queries.hpp — evidence

`10-offline_queries.hpp` (batch MI14, package P014) provides the offline threshold sweep and offline range threshold counting. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified. No online submission was made.

## Contracts

Common rules: sizes are below `INT_MAX`. Indices are zero-based and ranges half-open. Inputs are borrowed and never mutated unless a contract says so. Ties are deterministic but otherwise arbitrary. Comparators are pure strict weak orders, and equality callbacks are pure equivalences. Projections and predicates are pure and depend on element values, not addresses or positions. Callback exceptions propagate, with no rollback after mutation has begun. Each bound counts a comparison, projection, copy, move or `Count` operation as O(1).

### `OfflineThreshold<K>` and `offlineSweep(updates, queries, apply, answer, cmp)`

A strict threshold accepts update keys `u` with `cmp(u, key)`. An inclusive threshold also accepts keys equivalent to `key`. Keys need only `cmp`: no arithmetic or equality. The sweep sorts updates by `(key, index)` and queries by `(key, strict before inclusive, index)`. It calls `apply(update index)` and then `answer(query index)`, with exactly the eligible updates applied. Equal-key updates keep input order. Each update is applied at most once. Updates beyond the last answered query are never applied. With no queries, no callback runs. Callbacks must not mutate keys or the comparator's order, and `answer` must not change the accumulated state. Captured state belongs to the caller and is not reset. It costs O(n * log(n + 1) + q * log(q + 1)) time and O(n + q) space, plus callbacks. Correctness: the applied updates are always exactly the eligible prefix of the sorted updates for the current query; strict queries at a tied key run before the updates at that key, inclusive ones after.

### `OfflineRangeThreshold<K>` and `offlineRangeCount(a, queries, cmp)`

Each query `{l, r, key, inclusive}` counts the values in `a[l, r)` that pass its threshold. The domain is `0 <= l <= r <= n` (asserted), and empty ranges are valid. Answers come back in query order. Each eligible index is activated once in a `Fenwick<int>`, so counts fit `int`.

## Feature-to-test map

Every oracle is independent of the implementation and uses non-removable checks. Tests run in the optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (the last only in full and stress). Assertion probes run as SIGABRT subprocesses in the checked build.

| Operation | Oracle and cases |
|---|---|
| `OfflineThreshold`, `offlineSweep` | Repeated-minimum reference order; noncommutative state compared with direct eligibility at each answer; callback order; state unchanged after the last answer; ascending, descending, stateful and opaque comparators; 64/128-bit keys; exceptions; repeated calls |
| `OfflineRangeThreshold`, `offlineRangeCount` | Every subrange of exhaustive ternary arrays scanned directly in both comparator directions; random and full-width arrays; five range asserts; large duplicate and reverse fixtures |

## Commands and results

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08. The commands, floor run and integration are shared with the other P014 suites and recorded in [08-sequence_algorithms.md](08-sequence_algorithms.md).

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

- Finite corpora support the proofs above; they are not proofs themselves. Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- The Graphs notes' claim that "Mo on tree is Miscellaneous `10`" should point to Data Structures `23` (outside this package).
- GCC 14.2 exactly and Codeforces' Windows MSYS2 build were not run; the code uses no Windows-sensitive types or POSIX calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): offline set intersection, RMQ, range k-th and arithmetic-progression adds.

## History

- 2026-09-28: first implementation and audit (MI14, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before changes; no operation was missing (`/reaudit-review` findings 3, 5–8, 10–12, 15, 17 fixed).
