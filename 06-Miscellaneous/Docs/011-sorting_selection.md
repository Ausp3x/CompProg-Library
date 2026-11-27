# 011-sorting_selection.hpp — Evidence

`011-sorting_selection.hpp` (batch MI14, package P014) provides counting, stable counting and radix sorts, the three-way partition and worst-case linear median-of-medians selection; stable partition and introselect are `std::stable_partition` and `std::nth_element` (Legacy Mapping). Every implemented operation of the row has an independent-oracle test (map below); the row's `missing:` operations are not implemented. Prerequisites P002, P006 and P013 are verified.

## Contracts

Common rules: [00-notes.md](00-notes.md), Basic contracts.

### `sorting_detail`

`checkInteger` statically rejects `bool`, non-integral and wider-than-128-bit keys. `rank` flips the sign bit, which maps signed order monotonically onto unsigned order. `partition` is the Dutch-flag step used by `threeWayPartition` and `select`; `select` is the worst-case-linear selection loop, whose recursion on the medians uses O(log(n + 1)) stack.

### `countingSort(a, lo, hi)`

Sorts non-`bool` integers of up to 128 bits in place, for values with `lo <= value <= hi` and `lo <= hi`. `k = hi - lo + 1 <= INT_MAX` is asserted even for empty input, computed by unsigned 128-bit subtraction before adding one. The value domain is checked once at entry with `min_element`/`max_element` inside the `assert`, not per element. Empty input allocates no counters, and reconstruction never increments past `hi`.

### `stableCountingSort(a, alphabet, key)`

Sorts records stably by an integral key in `[0, S)`, `S = alphabet >= 0`; `S = 0` is valid only for empty input. `key` is called exactly once per record and cached. The key range is checked once after caching by `ulll(value) < ulll(S)`, which also rejects negative signed keys and avoids `-Wsign-compare`. Records must be copy-constructible and move-assignable, not default-constructible. The vector's storage is replaced, invalidating references and iterators.

### `radixSort(a, key = std::identity{})`

Stable LSD sort by full-width signed or unsigned 8–128-bit non-`bool` keys. It skips byte positions constant across the input and returns before allocating when all keys are equal or `n < 2`. `key` may be called several times. Record requirements and storage replacement are those of `stableCountingSort`. Cost: O(n + d * (n + 256)) time and O(n) space, `d <= 16` the number of varying bytes. Correctness: exclusive prefix offsets with forward scatter make each pass stable, so by induction the passes sort by the bytes processed so far; a constant byte cannot change order.

### `threeWayPartition(a, pivot, cmp)`

Takes `pivot` by value, so it may alias an element. Returns `{lt, gt}` with `[0, lt)` before the pivot, `[lt, gt)` equivalent and `[gt, n)` after; order within each group is unspecified. O(n) time with at most `2n` comparisons, O(1) space; works for `vector<bool>`.

### `medianOfMedians(a, k, cmp)`

Domain `0 <= k < n` (asserted); duplicates allowed. Returns a copy of the rank-`k` value in comparator order and permutes `a` so that nothing right of `k` compares before `a[k]` and nothing left compares after it. Worst case O(n) with groups of five and a three-way split; it needs copy construction of `T` for the pivot and is 1.6–12 times slower than `nth_element` in practice, so use it only when an adversarial worst case matters.

Correctness: each level costs at most `2n` comparisons for sorting groups of 5 and `2n` for the partition. The pivot exceeds at least `3(n/10) - O(1)` elements and is below as many, so recursion is on at most `n/5 + 1` medians plus at most `7n/10 + 6` elements: `C(n) <= C(n/5 + 1) + C(7n/10 + 6) + 4n`. Substitution gives `C(n) <= c * n` for any `c >= 40n / (n - 70)` once `n >= 140`: O(n) with leading constant 40 (the additive terms prevent a literal `40n` for every `n`). The worst measured count over sorted, reversed, organ-pipe, random, few-valued and multiplicative patterns was 11.4n (n <= 3000) and 11.2n (n = 200000). Large tests assert `<= 40n` as a regression bound, which detects a group-minimum pivot (43n). The insertion-sort cutoff 5 was measured against 10, 16, 24 and 40 on random, sorted and 16-value inputs at n = 1000 and 200000; all were within noise (4.65–5.65 ms at n = 200000), so the simplest value stays.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `stablePartition(a, pred)` | `int(std::stable_partition(a.begin(), a.end(), pred) - a.begin())`: the first false index, exactly `n` predicate calls, O(n) moves with a sufficient buffer and O(n * log(n + 1)) otherwise |
| `quickSelect(a, k, cmp)` | `std::nth_element(a.begin(), a.begin() + k, a.end(), cmp)`, then `a[k]`: GNU introselect, O(n) on average and O(n * log(n + 1)) worst case, deterministic; `0 <= k < n` is no longer asserted |

## Feature-to-Test Map

| Operation | Oracle and cases |
|---|---|
| `countingSort`, `stableCountingSort`, `radixSort` | Comparison-sort references; original ids prove stability; every key width through 128 bits; one-projection counter; copy-only records; empty and zero-alphabet input; large shapes; asserts for range, width and keys; compile rejections for bool and real keys |
| `std::stable_partition` (replaces `stablePartition`) | Direct two-pass filter on exhaustive and large record arrays, empty input, strings and `vector<bool>`; predicate counter; exception propagation |
| `threeWayPartition` | Group predicates against each rank's reference pivot, bounds equal to below/above counts, multiset preserved; `vector<bool>`; exception propagation |
| `medianOfMedians`; `std::nth_element` (replaces `quickSelect`) | Sorted reference at every rank of exhaustive arrays in both directions, partition property, multiset; strings and equivalence classes, comparator-only records, `vector<bool>`, exceptions; large shapes plus random input with the deterministic comparison count `<= 40n`; `medianOfMedians` asserts for negative, past-end and empty ranks |

Hand mutation probes (2026-10-08, quick mode): a group-minimum pivot in `medianOfMedians` (43n comparisons on random input) is caught by the `40n` count. Removing `assert(!bad)` survives in the quick binary as an equivalent mutant; the `key-*` death probes cover it.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14.7; optimized, checked and ASan/UBSan builds. The 2026-11-10 runs are library-wide (seed 42, beside the other suites, build cache emptied before each cold run); their figures are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; quick preflight peak 1579 MB; optimized 0.1 s compile + 29.8 s run, checked 92.3 s + 59.4 s, ASan-UBSan 91.9 s + 77.4 s; peak 2495 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 1591 MB; optimized 0.3 s + 54.5 s, checked 156.9 s + 159.4 s, ASan-UBSan 150.2 s + 101.1 s; peak 2475 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 105.2 s + 0.1 s, checked 80.4 s + 3.4 s; peak 1596 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 1603 MB; optimized 0.1 s + 13.6 s, checked 42.9 s + 46.5 s, ASan-UBSan 41.8 s + 88.8 s; peak 1509 MB |
| 2026-10-13 | `python3 '96-Local Testing/06-Miscellaneous/011-sorting_selection_tester.py' --mode quick --seed 1` | PASS, 2 builds, 13 probes |
| 2026-10-13 | `python3 '96-Local Testing/06-Miscellaneous/011-sorting_selection_tester.py' --mode full --seed 1` | PASS in all three builds: 184 s (g++, peak 1034 MB) and 224 s (g++-14, peak 1016 MB); 18800460 checks per build, 19531 exhaustive arrays, six compile rejections, 13 probes |
| 2026-10-13 | `python3 '96-Local Testing/02-integration.py'` | PASS: 135 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, workspace |
| 2026-10-13 | `python3 '96-Local Testing/03-consistency.py'`, and `--braces` on the header and its C++ tester | PASS, no violations |
| 2026-10-13 | `g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion` on the header; benchmark with `-O2 -DNDEBUG -Wall -Wextra -Wconversion` on both compilers | No warnings; the benchmark binary ran with every output verified (timings below are from 2026-10-07) |

## Benchmarks

The [driver](<../../96-Local Testing/06-Miscellaneous/011-sorting_selection_benchmark.py>) and [workload](<../../96-Local Testing/06-Miscellaneous/011-sorting_selection_benchmark.cpp>), with raw log `96-Local Testing/06-Miscellaneous/011-sorting_selection_benchmark.json`, ran on 2026-10-07 UTC (local 2026-10-08) on an i9-11900H with GCC 16.2.1, `-O2 -DNDEBUG`, seed 20260928. Each row has one warmup and five repetitions with method order rotated; the median is reported. Timing includes copying the input into a fresh output; sizes 32 and 4096 are batched 256 and 8 times. Every output is verified (132 sorting and 30 selection rows). Baselines: `std::sort` for scalars, `std::stable_sort` for records, `std::nth_element` for selection.

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

Dense counting and fixed-width radix win substantially on their intended workloads; small or presorted inputs favour the standard algorithms, so callers choose explicitly and no cutoff is inferred. `medianOfMedians` buys a worst-case guarantee, not speed. Memory per call: counting uses `k` int counters; stable counting `n` cached keys, an `n`-record buffer and `S` counters; radix an `n`-record buffer and 256 stack counters; selection O(log n) stack.

## Sources

The code was written independently from the recurrences and proofs here; no external code was copied. Read in the 2026-09-28 audit: the cp-algorithms K-th order statistic page (revision 2025-04-16); OI Wiki 计数排序 and 基数排序; the installed GCC 16.2.1 `bits/stl_algo.h` (`__introselect`, `__stable_partition_adaptive`). The 2026-10-08 `@researcher` sweep is in [00-sources.md](00-sources.md); it fetched the OI Wiki quick-sort pages. The median-of-medians bound is proved here, not taken from those pages.

## Limits and Handoffs

- Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's and are not asserted.
- The code uses no `long` or POSIX-only calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): partial sort, k-way merge and multikey radix.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): k-th order statistic, selection, median of medians; 基数排序, 计数排序, 快速排序, 三路划分, 荷兰国旗问题, 中位数的中位数, 线性时间选择, 第k小元素.

## History

- 2026-09-28: first implementation and audit (MI14, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in three builds before changes.
- 2026-10-08: re-audit added `medianOfMedians` and `threeWayPartition`, 12 findings fixed; full seed 20261008 passed on g++ and g++-14, stress seed 7 and the benchmark passed, integration and consistency clean.
- 2026-10-13: prune removed `stablePartition` and `quickSelect` (cases moved to `std::stable_partition` and `std::nth_element`, `select-*` probes dropped); full suite passed on g++ and g++-14; benchmark rebuilt warning-free with outputs verified.
- 2026-11-07: P263 added the search aliases, then their Chinese names (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; every library-wide run passed.
