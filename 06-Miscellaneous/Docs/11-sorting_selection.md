# 11-sorting_selection.hpp — evidence

`11-sorting_selection.hpp` (batch MI14, package P014) provides counting, stable counting and radix sorts, stable and three-way partitions, and introselect and median-of-medians selection. First implemented and verified on 2026-09-28. Re-audited on 2026-10-08 against the function-level inventory; the earlier record called the package complete while six inventory operations were missing (`/reaudit-review` findings 1–4). Header contracts moved here, and the code now follows the current comment cap, closing-brace rule and `res` naming. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified. No online submission was made. The re-audit implemented the missing `medianOfMedians`; the `@researcher` sweep added `threeWayPartition`.

## Contracts

Common rules: sizes are below `INT_MAX`. Indices are zero-based and ranges half-open. Inputs are borrowed and never mutated unless a contract says so. Ties are deterministic but otherwise arbitrary. Comparators are pure strict weak orders, and equality callbacks are pure equivalences. Projections and predicates are pure and depend on element values, not addresses or positions. Callback exceptions propagate, with no rollback after mutation has begun. Each bound counts a comparison, projection, copy, move or `Count` operation as O(1).

### `sorting_detail`

`checkInteger` statically rejects `bool`, non-integral and wider-than-128-bit keys. `rank` flips the sign bit to map signed order onto unsigned order. `partition` is the Dutch-flag step, used by `threeWayPartition` and `select`. `select` is the worst-case-linear selection loop. Recursion on the medians uses O(log(n + 1)) stack.

### `countingSort(a, lo, hi)`

This sorts non-`bool` integers of up to 128 bits in place, for values with `lo <= value <= hi` and `lo <= hi`. `k = hi - lo + 1 <= INT_MAX` is asserted even for empty input, computed by unsigned 128-bit subtraction before adding one. The value domain is checked once at entry with `min_element`/`max_element` inside the `assert`, not per element (finding 13). Empty input allocates no counters. Reconstruction never increments past `hi`.

### `stableCountingSort(a, alphabet, key)`

This sorts records stably by an integral key in `[0, S)`, where `S = alphabet >= 0`. `S = 0` is valid only for empty input. `key` is called exactly once per record and the results are cached. The key range is checked once after caching by `ulll(value) < ulll(S)`, which also rejects negative signed keys and avoids `-Wsign-compare` (finding 14). Records must be copy-constructible and move-assignable; default construction is not needed. The function replaces the vector's storage, invalidating references and iterators.

### `radixSort(a, key = std::identity{})`

This is a stable LSD sort by full-width signed or unsigned 8–128-bit non-`bool` keys. It skips byte positions that are constant across the input and returns before allocating when all keys are equal or `n < 2`. `key` may be called several times. Records have the same requirements and storage replacement as `stableCountingSort`. The cost is O(n + d * (n + 256)) time and O(n) space, where `d <= 16` is the number of varying bytes.

### `stablePartition(a, pred)` and `threeWayPartition(a, pivot, cmp)`

`stablePartition` wraps `std::stable_partition`. It returns the first false index, calls `pred` exactly `n` times, does O(n) moves with a sufficient buffer and O(n * log(n + 1)) moves otherwise. `threeWayPartition` takes `pivot` by value, so it may alias an element. It returns `{lt, gt}` with `[0, lt)` before the pivot, `[lt, gt)` equivalent and `[gt, n)` after. Order within each group is unspecified. It runs in O(n) time with at most `2n` comparisons and O(1) space, and works for `vector<bool>`.

### `quickSelect(a, k, cmp)` and `medianOfMedians(a, k, cmp)`

The domain is `0 <= k < n` (asserted). Both return a copy of the rank-`k` value in comparator order and permute `a` so that nothing right of `k` compares before `a[k]` and nothing left compares after it. Duplicates are allowed. `quickSelect` is GNU introselect (`std::nth_element`): O(n) on average and O(n * log(n + 1)) in the worst case, deterministic, with no seed. `medianOfMedians` is O(n) in the worst case (leading constant 40 comparisons per element in the recurrence below; at most 11.4n measured), using groups of five and a three-way split. It needs copy construction of `T` for the pivot and is 1.6–12 times slower than `nth_element` in practice. Use it only when an adversarial worst case matters.

## Correctness and optimality

**Sorting and selection.** Flipping the sign bit maps signed order monotonically onto unsigned order. Exclusive prefix offsets with forward scatter make each pass stable, and by induction LSD passes sort by the bytes processed so far. A byte that is constant across the input cannot change order. For `medianOfMedians`, each level costs at most `2n` comparisons for sorting groups (at most 10 comparisons per group of 5) and `2n` for the three-way partition. The pivot exceeds at least `3(n/10) - O(1)` elements and is below at least as many, so the recursion is on at most `n/5 + 1` medians plus at most `7n/10 + 6` elements. Hence `C(n) <= C(n/5 + 1) + C(7n/10 + 6) + 4n`. The standard substitution gives `C(n) <= c * n` for any `c >= 40n / (n - 70)` once `n >= 140`, so `C(n)` is O(n) with leading constant 40. The additive terms prevent a literal `40n` bound for every `n`. The worst count measured over sorted, reversed, organ-pipe, random, few-valued and multiplicative patterns was 11.4n (independent review, n <= 3000) and 11.2n (n = 200000). The large tests assert `<= 40n` as a deterministic regression bound, which detects a group-minimum pivot (43n). The insertion-sort cutoff (5) was measured against 10, 16, 24 and 40 on random, sorted and 16-value inputs at n = 1000 and 200000. All were within noise (4.65–5.65 ms at n = 200000), so the simplest value was kept.

## Re-audit findings (P014, 2026-10-08)

| # | Finding | Resolution |
|---|---|---|
| 3 | Evidence said complete while rows were partial | Rewritten; rows are verified only after implementation |
| 4 | `medianOfMedians` missing | Implemented, with an O(n) proof and benchmark |
| 6, 10, 12 | Missing complexity lines on structs and detail helpers | Every struct and helper has a `T:`/`M:` line. Closely related declarations with no blank line between them (result records, a struct and its only producer, `kadane` under `maximumSubarray`) share one line, as in the verified `07-permutation.hpp`, to stay within the 8% comment cap. |
| 8, 11, 15, 17 | Closing braces | All four headers and all `08`–`11` testers and the benchmark normalized; `--braces` clean |
| 13 | Per-element domain asserts | Entry `min_element`/`max_element` assert in `countingSort`; a single post-loop `assert(!bad)` in `stableCountingSort` |
| 14 | `-Wsign-compare` at the key check | `ulll(value) < ulll(alphabet)` |
| 16 | Split or off-vocabulary complexity comments | Single lines using `S`, a defined `d`, worst case first, average labelled |

The full P014 findings table is in [00-notes.md](00-notes.md).

The independent `@reviewer` pass (2026-10-08) found no correctness defects. Its own sanitized brute-force program ran 400k cases over every new operation (maximum average, interval cover, partition and nesting across both domains and all flags, selection, chains, swaps, mex) and reported no failures or sanitizer reports. It confirmed one issue here, fixed: a literal `40n` proof that ignored additive constants is now O(n) with leading constant 40 plus measured counts. Its note on grouped complexity lines is recorded under findings 6/10/12.

## Feature-to-test map

Every oracle is independent of the implementation and uses non-removable checks. Tests run in the optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (the last only in full and stress). Assertion probes run as SIGABRT subprocesses in the checked build.

| Operation | Oracle and cases |
|---|---|
| `countingSort`, `stableCountingSort`, `radixSort` | Comparison-sort references; original ids prove stability; every key width through 128 bits; one-projection counter; copy-only records; empty and zero-alphabet input; large shapes; asserts for range, width and keys; compile rejections for bool and real keys |
| `stablePartition` | Direct two-pass filter; predicate counter |
| `threeWayPartition` | Group predicates checked against each rank's reference pivot, bounds equal to below/above counts, multiset preserved; `vector<bool>`; exception propagation |
| `quickSelect`, `medianOfMedians` | Sorted reference at every rank of exhaustive arrays in both directions, partition property, multiset; strings and equivalence classes, comparator-only records, `vector<bool>`, exceptions; large shapes plus random input with the deterministic comparison count `<= 40n` (regression bound); asserts for negative, past-end and empty ranks |

Mutation probes (2026-10-08, quick mode): a group-minimum pivot in `medianOfMedians`, a performance-only mutant, reached 43n comparisons on random input and is now detected by the `40n` count. The `assert(!bad)` removal survives as an equivalent mutant in the quick binary; it is covered by the `key-*` death probes.

## Commands and results

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08.

- Baseline before changes: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 1` for `08`, `09`, `10`, `11`: all PASS in 3 configurations.
- After changes, full: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008`. Results in the table below.
- Stress, one round: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode stress --seed 7`. Results in the table below.
- `python3 '96-Local Testing/03-consistency.py' --braces` on the four P014 headers (`08`–`11`), the four C++ testers and the benchmark: no violations.
- `g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion`: the headers and testers emit no warnings except a pre-existing GCC 16 `-Warray-bounds`/`-Wstringop-overflow` false positive in the sorting tester's `vector<signed char>` construction (tester code, not the header).
- `python3 '96-Local Testing/06-Miscellaneous/11-sorting_selection_benchmark.py'`: PASS, 132 sorting and 30 selection rows, all outputs verified.

| Suite | Full, seed 20261008 (optimized, checked, ASan/UBSan) | Stress, seed 7, one round (same three) | Probes |
|---|---|---|---|
| `11-sorting_selection` | PASS: 18774747 checks per configuration, six compile rejections | PASS in 270 s: 61501539 checks, 97656 exhaustive arrays | 16 |

Judge floor: `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008` with GCC 14.4.1 20260915 passed for all four suites in all three configurations, with every assertion probe and the six compile rejections. Under `g++-14 -std=gnu++20 -O2 -Wall -Wextra -Wconversion` the four testers and headers emit no warnings (the GCC 16 tester false positive does not occur).

After the review fixes (`std::max_element` qualification, a tester message), the following passed. `python3 '96-Local Testing/01-run.py' --mode quick --seed 3 --no-integration --filter '06-Miscellaneous/<NN>'` for `08` and `11`. `python3 '96-Local Testing/02-integration.py'`: 102 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, and the workspace. `python3 '96-Local Testing/03-consistency.py'`: no errors.

## Benchmarks

The [driver](<../../96-Local Testing/06-Miscellaneous/11-sorting_selection_benchmark.py>), [workload](<../../96-Local Testing/06-Miscellaneous/11-sorting_selection_benchmark.cpp>) and `96-Local Testing/06-Miscellaneous/11-sorting_selection_benchmark.json` were run on 2026-10-07 UTC (local 2026-10-08) on an i9-11900H with GCC 16.2.1 and `-O2 -DNDEBUG`, seed 20260928. Each row has one warmup and five repetitions with method order rotated, and the median is reported. Timing includes copying the input into a fresh output; sizes 32 and 4096 are batched 256 and 8 times. Every output is verified completely. Medians:

| n / input | Method | Method ms | Standard ms | Ratio |
|---|---|---|---|---|
| 32, dense signed64 | counting | 0.000169 | 0.000083 | 2.03 |
| 32, random full-width signed64 | radix | 0.000983 | 0.000088 | 11.19 |
| 200000, dense signed64 | counting | 0.4800 | 5.8451 | 0.082 |
| 200000, dense 32-byte records | stable counting | 2.7954 | 11.1056 | 0.252 |
| 200000, random full-width signed64 | radix | 3.0788 | 10.1995 | 0.302 |
| 200000, random full-width records | radix | 7.3400 | 13.9175 | 0.527 |
| 200000, sorted full-width signed64 | radix | 3.2620 | 1.4975 | 2.18 |
| 200000, random full-width signed128 | radix | 10.2135 | 11.8639 | 0.861 |
| 200000, reverse full-width signed128 | radix | 9.7156 | 1.9400 | 5.01 |
| 200000, random dense signed64 rank n/2 | medianOfMedians | 2.1276 | 1.3182 (`nth_element`) | 1.61 |
| 200000, random full-width signed64 rank n/2 | medianOfMedians | 5.5888 | 1.4008 | 3.99 |
| 200000, sorted full-width | medianOfMedians | 2.2140 | 0.2120 | 10.44 |
| 200000, reverse full-width | medianOfMedians | 2.4238 | 0.2012 | 12.05 |
| 200000, equal keys | medianOfMedians | 0.4511 | 0.2062 | 2.19 |

Scalar baselines are `std::sort`, record baselines `std::stable_sort`, and selection baselines `std::nth_element`. Dense counting and fixed-width radix win substantially on their intended workloads. Small or presorted inputs favour the standard algorithms, so callers choose explicitly and no cutoff is inferred. `medianOfMedians` costs 1.6–12 times `nth_element`; it buys a worst-case guarantee, not speed. Memory per call: counting uses `k` int counters; stable counting uses `n` cached keys, an `n`-record buffer and `S` counters; radix uses an `n`-record buffer and 256 stack counters; selection uses O(log n) stack.

## Sources

The code was written independently from the recurrences and proofs in the package; no external code was copied. Sources read in the 2026-09-28 audit: the cp-algorithms K-th order statistic page (revision 2025-04-16); OI Wiki 计数排序 and 基数排序; and the installed GCC 16.2.1 `bits/stl_algo.h` (`__introselect`, `__stable_partition_adaptive`). The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md); its agents fetched the OI Wiki quick-sort pages. The median-of-medians bound is proved here rather than taken from those pages.

## Limits and handoffs

- Finite corpora support the proofs above; they are not proofs themselves. Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- Compilers run: GCC 16.2.1 (full, stress, benchmark) and GCC 14.4.1 (full). GCC 14.2 exactly and Codeforces' Windows MSYS2 build were not run; the code uses no Windows-sensitive types or POSIX calls.

Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): partial sort, k-way merge and multikey radix.
