# 10-offline_queries.hpp — evidence

`10-offline_queries.hpp` (batch MI14, package P014) provides the offline threshold sweep and offline range threshold counting. First implemented and verified on 2026-09-28. Re-audited on 2026-10-08 against the function-level inventory; the earlier record called the package complete while six inventory operations were missing (`/reaudit-review` findings 1–4). Header contracts moved here, and the code now follows the current comment cap, closing-brace rule and `res` naming. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified. No online submission was made. No operation of this row was missing.

## Contracts

Common rules: sizes are below `INT_MAX`. Indices are zero-based and ranges half-open. Inputs are borrowed and never mutated unless a contract says so. Ties are deterministic but otherwise arbitrary. Comparators are pure strict weak orders, and equality callbacks are pure equivalences. Projections and predicates are pure and depend on element values, not addresses or positions. Callback exceptions propagate, with no rollback after mutation has begun. Each bound counts a comparison, projection, copy, move or `Count` operation as O(1).

### `OfflineThreshold<K>` and `offlineSweep(updates, queries, apply, answer, cmp)`

A strict threshold accepts update keys `u` with `cmp(u, key)`. An inclusive threshold also accepts keys equivalent to `key`. Keys need only `cmp`: no arithmetic or equality. The sweep sorts updates by `(key, index)` and queries by `(key, strict before inclusive, index)`. It calls `apply(update index)` and then `answer(query index)`, with exactly the eligible updates applied. Equal-key updates keep input order. Each update is applied at most once. Updates beyond the last answered query are never applied. With no queries, no callback runs. Callbacks must not mutate keys or the comparator's order, and `answer` must not change the accumulated state. Captured state belongs to the caller and is not reset. It costs O(n * log(n + 1) + q * log(q + 1)) time and O(n + q) space, plus callbacks.

### `OfflineRangeThreshold<K>` and `offlineRangeCount(a, queries, cmp)`

Each query `{l, r, key, inclusive}` counts the values in `a[l, r)` that pass its threshold. The domain is `0 <= l <= r <= n` (asserted), and empty ranges are valid. Answers come back in query order. Each eligible index is activated once in a `Fenwick<int>`, so counts fit `int`.

## Correctness and optimality

**Offline.** The applied updates are always exactly the eligible prefix of the sorted updates for the current query. Strict queries at a tied key run before the updates at that key, and inclusive ones after.

## Re-audit findings (P014, 2026-10-08)

| # | Finding | Resolution |
|---|---|---|
| 3 | Evidence said complete while rows were partial | Rewritten; rows are verified only after implementation |
| 5 | Unused tail updates untested | `checkSweep` checks that the state is unchanged after the last answer; the suggested mutant fails |
| 6, 10, 12 | Missing complexity lines on structs and detail helpers | Every struct and helper has a `T:`/`M:` line. Closely related declarations with no blank line between them (result records, a struct and its only producer, `kadane` under `maximumSubarray`) share one line, as in the verified `07-permutation.hpp`, to stay within the 8% comment cap. |
| 7 | `ans` naming | Renamed to `res` (also `out` and `result` in `09`/`10`) |
| 8, 11, 15, 17 | Closing braces | All four headers and all `08`–`11` testers and the benchmark normalized; `--braces` clean |

The full P014 findings table is in [00-notes.md](00-notes.md).

The independent `@reviewer` pass (2026-10-08) found no correctness defects. Its own sanitized brute-force program ran 400k cases over every new operation (maximum average, interval cover, partition and nesting across both domains and all flags, selection, chains, swaps, mex) and reported no failures or sanitizer reports.

## Feature-to-test map

Every oracle is independent of the implementation and uses non-removable checks. Tests run in the optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (the last only in full and stress). Assertion probes run as SIGABRT subprocesses in the checked build.

| Operation | Oracle and cases |
|---|---|
| `OfflineThreshold`, `offlineSweep` | Repeated-minimum reference order; noncommutative state compared with direct eligibility at each answer; callback order; state unchanged after the last answer (finding 5); ascending, descending, stateful and opaque comparators; 64/128-bit keys; exceptions; repeated calls |
| `OfflineRangeThreshold`, `offlineRangeCount` | Every subrange of exhaustive ternary arrays scanned directly in both comparator directions; random and full-width arrays; five range asserts; large duplicate and reverse fixtures |

## Commands and results

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08.

- Baseline before changes: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 1` for `08`, `09`, `10`, `11`: all PASS in 3 configurations.
- After changes, full: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008`. Results in the table below.
- Stress, one round: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode stress --seed 7`. Results in the table below.
- `python3 '96-Local Testing/03-consistency.py' --braces` on the four P014 headers (`08`–`11`), the four C++ testers and the benchmark: no violations.
- `g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion`: the headers and testers emit no warnings (the only P014 warning is a GCC 16 false positive in the sorting tester).

| Suite | Full, seed 20261008 (optimized, checked, ASan/UBSan) | Stress, seed 7, one round (same three) | Probes |
|---|---|---|---|
| `10-offline_queries` | PASS: 1414709 checks per configuration | PASS in 88 s: 8158221 checks | 5 |

Judge floor: `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008` with GCC 14.4.1 20260915 passed for all four suites in all three configurations, with every assertion probe and the six compile rejections. Under `g++-14 -std=gnu++20 -O2 -Wall -Wextra -Wconversion` the four testers and headers emit no warnings (the GCC 16 tester false positive does not occur).

After the review fixes (`std::max_element` qualification, a tester message), the following passed. `python3 '96-Local Testing/01-run.py' --mode quick --seed 3 --no-integration --filter '06-Miscellaneous/<NN>'` for `08` and `11`. `python3 '96-Local Testing/02-integration.py'`: 102 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, and the workspace. `python3 '96-Local Testing/03-consistency.py'`: no errors.

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting/selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs in the package; no external code was copied. Sources read in the 2026-09-28 audit: OI Wiki 离线算法简介. The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md).

## Limits and handoffs

- Finite corpora support the proofs above; they are not proofs themselves. Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- The Graphs notes' claim that "Mo on tree is Miscellaneous `10`" should point to Data Structures `23` (outside this package).
- Compilers run: GCC 16.2.1 (full, stress, benchmark) and GCC 14.4.1 (full). GCC 14.2 exactly and Codeforces' Windows MSYS2 build were not run; the code uses no Windows-sensitive types or POSIX calls.

Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): offline set intersection, RMQ, range k-th and arithmetic-progression adds.
