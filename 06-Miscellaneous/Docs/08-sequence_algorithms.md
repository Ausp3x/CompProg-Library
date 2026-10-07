# 08-sequence_algorithms.hpp — evidence

`08-sequence_algorithms.hpp` (batch MI03, package P014) covers maximum subarray, bounded, circular, rectangle and average variants, increasing-subsequence lengths, counts and weights, inversions and adjacent-swap distance, `mex`, monotone windows, sorted pair sums and majority/heavy hitters. It reuses DS02 prefix/difference sums, sliding extrema, Fenwick and SegmentTree, plus MI02 compression. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified. No online submission was made.

## Contracts

Common rules: sizes are below `INT_MAX`. Indices are zero-based and ranges half-open. Inputs are borrowed and never mutated unless a contract says so. Ties are deterministic but otherwise arbitrary. Comparators are pure strict weak orders, and equality callbacks are pure equivalences. Projections and predicates are pure and depend on element values, not addresses or positions. Callback exceptions propagate, with no rollback after mutation has begun. Each bound counts a comparison, projection, copy, move or `Count` operation as O(1).

### Sequence result records (`SubarraySum`, `CircularSubarraySum`, `SubrectangleSum`, `WeightedSubsequence`)

These are plain aggregates. `exists()` is false exactly for an absent optimum (`l = r = -1`, `start = -1, length = 0`, all rectangle coordinates `-1`, with `sum = 0`). A legitimate empty optimum is present and has zero coordinates. `WeightedSubsequence::found` plays the same role, and `indices` holds the witness in increasing order.

### `sequence_detail`

`checkedSize` asserts `n < INT_MAX`. `checkInteger` rejects non-integral or wider-than-64-bit sum and weight types at compile time. `maximum(n, get, empty)` returns the best nonempty `[l, r)` by prefix difference. With `empty`, it starts from `[0, 0)` and only a strictly larger sum replaces it. `restore` follows a predecessor array.

### `maximumSubarray(a, allow_empty = false)` and `kadane(a)`

This returns the best nonempty `[l, r)`. `n = 0` is absent unless `allow_empty` adds `[0, 0)`, which applies to every `n`. An all-negative nonempty optimum is a valid negative answer. Sums are exact in `lll`: the inputs are at most 64-bit values, `n < 2^31`, so `|sum| < 2^95`. `kadane` is the legacy API. It returns `0` for `n = 0` and otherwise the nonempty optimum, and asserts that the optimum fits `lng`. Correctness: the best nonempty subarray ending at `r` is `prefix[r]` minus the smallest earlier prefix.

### `boundedMaximumSubarray(a, lo, hi)`

The domain is `0 <= lo <= hi` (asserted). The result is the best sum with length in `[lo, hi]`. `lo > n` is a legitimate absence. `lo = 0` admits the empty interval. `hi` may exceed `n`, including `INT_MAX`. The function keeps two running prefixes and a monotone deque of eligible starts `[r - hi, r - lo]`. Correctness: a newer smaller prefix dominates an older one for every later end at which both are eligible, and expiry and dominance each remove an entry once.

### `circularMaximumSubarray(a, allow_empty = false)`

The result is `{sum, start, length}`, consuming `a[(start + i) % n]` for `i < length`, with at most one full turn (`length <= n`). An empty input is absent unless `allow_empty`. The allowed empty result has `start = length = 0`. Correctness: a circular optimum is either an ordinary one or the complement of a proper nonempty minimum segment. Excluding a full-array minimum avoids an illegal empty complement; if the full array is the minimum, every proper complement is nonpositive and is dominated by a single best element.

### `maximumSubrectangle(a, allow_empty = false)`

The input must be rectangular (asserted per row). The result is `[top, bottom) x [left, right)`. An empty dimension is absent unless `allow_empty`, which gives all-zero coordinates. Boundary pairs run over the smaller dimension and Kadane over the larger, in `O(min(n, m)^2 * max(n, m) + n)` time. Sums are below `2^126`. Correctness: collapsing each band of the smaller dimension to exact sums enumerates every rectangle.

### `maximumAverageSubarray(a, lo)`

The domain is `lo >= 1` (asserted) on an integral input of up to 64 bits. The function maximizes the exact rational `sum / (r - l)` over windows of length at least `lo`, with no tolerance and no floating point. The caller reads the average as `res.sum / (res.r - res.l)`. `lo > n` is absent. Comparisons cross-multiply in `lll`: a prefix difference is below `2^95` and a length below `2^31`, so every product is below `2^126`. It runs in O(n) time and O(n) space. Real-valued inputs are outside the domain; scale them to integers. Correctness: with `P_j = (j, prefix[j])`, the average of `[j, r)` is the slope from `P_j` to `P_r`. For each `r >= lo` the candidates `j <= r - lo` form a lower convex chain (back pops remove points on or above a chord; by the mediant inequality they never beat their neighbours). Slope to a point on the right is unimodal along the chain, so popping the front while the next start is at least as good finds the best start. Discarding front start `q_i` is safe for every later `r'`: its average to `r` is at most that of the surviving front `q_k`, hence `avg[q_i, q_k) <= avg[q_k, r)`, an already recorded candidate, and `avg[q_i, r') <= max(avg[q_i, q_k), avg[q_k, r'))`. Each index is pushed and popped once. Binary search on the answer would need a tolerance; this is exact.

### `increasingSubsequenceLengths(a, strict = true, cmp)` and `longestIncreasingSubsequence(a, strict = true, cmp)`

`res[i]` is the length of the longest chain ending at `i`. A strict chain needs `cmp(a[j], a[i])` between consecutive items. A non-strict chain needs `!cmp(a[i], a[j])`, so comparator-equivalent keys may repeat. Indices with equal `res` values form a chain of the dual order: non-increasing for strict chains, strictly decreasing for non-strict ones. Grouping by `res` is therefore a minimum cover of `a` by dual chains, of size `max(res)` (Dilworth/Mirsky; CSES Towers). `longestIncreasingSubsequence` returns the indices of one longest chain, empty for `n = 0`. Neither function uses numeric sentinels, so any copyable key type works. Correctness: patience tails keep the best final key per length; the binary-search position plus one is the longest chain ending at `i`. Backward reconstruction works because `len[i] = L` implies an earlier `j` with `len[j] = L - 1` chaining to `i`; scanning down from `i - 1`, the first such `j` keeps the invariant. Equal-length indices `i < j` cannot chain, so equal-length classes are dual chains, and their number equals the longest chain length, a lower bound on any dual cover (Mirsky).

### `countLongestIncreasingSubsequences<Count>(a, strict = true, cmp)` and `countIncreasingSubsequences<Count>(a, strict = true, cmp)`

The first returns `{length, count}` of index-distinct longest chains, with `{0, 1}` for `n = 0`. The second counts all nonempty index-distinct chains of any length, with `0` for `n = 0`. The caller chooses `Count` explicitly. It needs `Count(int)` and `+` or `+=`. Every addition must fit, or `Count` must intentionally be modular (`mint`, or wrapping `ulng` for mod `2^64`). Distinct comparator classes are limited to `2^29` by the shared SegmentTree. Value-distinct counts are a different problem and are not provided. Correctness: indices are processed in order, querying counts at smaller ranks (strict) or no-greater ranks (non-strict). The longest-count variant adds only optimal-length predecessors, each ending index once; the all-chain count adds one for starting fresh.

### `weightedIncreasingSubsequence(a, w, strict = true, allow_empty = false, cmp)`

The domain is `w.size() == a.size()` (asserted), with integral weights of up to 64 bits. The function maximizes the total weight of a chain. The empty chain counts only when `allow_empty`. Otherwise empty input is absent (`found = false`), and an all-negative input picks one best item. There is no secondary length objective. The `2^29` class limit applies. Correctness: each index extends the best positive predecessor.

### `inversionCount(a, cmp)` and `adjacentSwapDistance(a, b, cmp)`

`inversionCount` counts pairs `i < j` with `cmp(a[j], a[i])`, excluding equivalent keys, exactly in `lng` (at most `n(n-1)/2 < 2^61`). `adjacentSwapDistance` returns the fewest adjacent swaps that turn `a` into `b`. It returns `-1` when `b` is not a rearrangement of `a` under comparator equivalence, including a size mismatch. Equivalent keys are matched in order of occurrence: the k-th copy in `a` goes to the k-th copy in `b`, which is optimal. Correctness: a Fenwick tree counts how many earlier keys are strictly greater. For adjacent swaps, in-order matching of equivalent copies avoids crossing equal elements, any other matching adds swaps, and the minimum swap count of a permutation is its inversion count.

### `mex(a)`

The result is the smallest nonnegative integer absent from an integral `a`. Negative values are ignored, and the result is at most `n`. Dynamic mex belongs to Data Structures `24`/`32`. Correctness: values in `[0, n]` are marked and the first unmarked one is returned.

### `monotoneWindowEnds(n, can_add, add, remove)` and `nonnegativeSumWindowEnds(a, limit)`

The first returns the maximal valid right end for each left end. Validity must hold for empty windows and be hereditary when either end is deleted; this is the caller's proof obligation and is not checked. Callback state starts empty. `can_add(r)` is pure and tests appending `r`, while `add` and `remove` keep exactly the active window. An invalid singleton is skipped without callbacks. Total cost is O(n) callback calls. The adapter requires nonnegative values and `0 <= limit <= LLL_MAX` (asserted). It tests `a[r] <= limit - sum`, so `sum + a[r]` never overflows. Correctness: hereditary validity makes the maximal right end monotone, and each end advances at most `n` times.

### `sortedPairSum(a, target)`

The input must be ascending (asserted). The result is any `i < j` with `a[i] + a[j] == target` exactly in `lll`, or `{-1, -1}` when none exists. Correctness: a too-small sum rules out its left element and a too-large sum its right one.

### `majorityElement(a, eq)` and `heavyHitters(a, k, cmp)`

Both are verified. `majorityElement` returns Boyer–Moore `{representative index, exact frequency}` of a class with frequency above `n / 2`, or `{-1, 0}`. `heavyHitters` uses Misra–Gries with `k >= 1` (asserted). It returns representatives and exact frequencies of classes above `floor(n / k)`, sorted by `cmp`. `k = 1` returns none, and `k > n` returns every class. A mandatory second pass verifies every candidate in both functions.

Prefix and difference sums and sliding extrema come from their Data Structures headers, whose contracts apply. Correctness: Boyer–Moore cancels pairs of different classes and Misra–Gries groups of `k` distinct classes. At most `floor(n / k)` groups can be cancelled, so any class above the threshold survives, and the second pass makes both answers exact. Across decrement scans every counter unit visited was created earlier, giving linear total work.

## Feature-to-test map

Every oracle is independent of the implementation and uses non-removable checks. Tests run in the optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (the last only in full and stress). Assertion probes run as SIGABRT subprocesses in the checked build.

| Operation | Oracle and cases |
|---|---|
| `maximumSubarray`, `kadane`, `boundedMaximumSubarray`, `circularMaximumSubarray` | Every subinterval and circular window enumerated for all ternary arrays of length ≤ 7 (full) and 350 random arrays; every `(lo, hi)` up to `n + 1` and `hi = INT_MAX`; signed/unsigned 64-bit extremes; witnesses re-summed; legacy fit assert |
| `maximumSubrectangle` | Every rectangle of all `±1` matrices with dimensions up to 3x3 and random matrices up to 5x6, both orientations; full-width magnitudes; ragged assert |
| `maximumAverageSubarray` | Every window of length ≥ `lo` for each `lo` in `[1, n + 1]`, compared as exact fractions; witness re-summed; absence for `lo > n`; full-width 64-bit triples; large: increasing array's last window, constant array; `lo = 0` assert |
| `increasingSubsequenceLengths` | Subset-mask enumeration gives the longest chain ending at each index; equal-length pairs checked as dual chains; strict and non-strict, reversed and equivalence comparators, strings |
| `longestIncreasingSubsequence` | Subset-mask optimum length plus witness order and indices; large increasing array |
| `countLongestIncreasingSubsequences`, `countIncreasingSubsequences` | Exact mask counts (index-distinct) for both strictnesses; exact `lll` `2^100`, `3^40 - 1`, `2^80 - 1` and equal-key counts; intentional `ulng` mod `2^64` |
| `weightedIncreasingSubsequence` | Mask optimum with negative and zero weights, empty allowed or not; witness sum; size assert |
| `inversionCount` | All-pairs count; equivalent keys, reversed comparator, strings, large reversed permutation |
| `adjacentSwapDistance` | BFS over all arrangements for arrays of length ≤ 5 against the sorted, rotated and reversed targets and against ±1-changed non-rearrangements; size mismatch; large reversed permutation `n(n-1)/2`; large alternating 0/1 against sorted `m(m+1)/2` (detects unstable matching) |
| `mex` | Set scan over every exhaustive, random and window array; large permutation and gap; full-width values |
| `monotoneWindowEnds`, `nonnegativeSumWindowEnds` | Direct per-left scans for limits `0..8` and distinct-key windows; callback balance; maximum `lll` limit; negative value or limit asserts |
| `sortedPairSum` | Pairwise oracle for targets `-1..10`; full-width unsigned pair; unsorted assert |
| `majorityElement`, `heavyHitters` | Exact frequency maps for every `k` in `1..n+2` and `INT_MAX`; equivalence comparators, strings; `k = 0` assert |

Mutation probes (2026-10-08, quick mode) detected changes to the hull pop, lengths, chain counts, reconstruction break, average start, one-sided multiset check and unstable matching. The surviving equivalent mutants are a tie-only front pop and a redundant mex bound.

## Commands and results

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08. Commands are shared by the P014 suites `08`–`11`.

- Full: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008`: PASS (table below).
- Stress, one round: `python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode stress --seed 7`: PASS (table below).
- Floor: `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/<NN>_tester.py' --mode full --seed 20261008` with GCC 14.4.1 20260915: PASS for `08`–`11` in all three configurations, with every assertion probe and the six compile rejections.
- Quick after the final review fixes: `python3 '96-Local Testing/01-run.py' --mode quick --seed 3 --no-integration --filter '06-Miscellaneous/<NN>'` for `08` and `11`: PASS.
- `python3 '96-Local Testing/02-integration.py'`: PASS, 102 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, workspace.
- `python3 '96-Local Testing/03-consistency.py'`: no errors; `--braces` on the four P014 headers, the four C++ testers and the benchmark: no violations.
- `g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion` and the same with `g++-14`: headers and testers emit no warnings (except a GCC 16-only false positive in the sorting tester).

| Suite | Full, seed 20261008 (optimized, checked, ASan/UBSan) | Stress, seed 7, one round (same three) | Probes |
|---|---|---|---|
| `08-sequence_algorithms` | PASS: 3280 exhaustive ternary arrays (length ≤ 7), 350 random rounds, rectangles ≤ 3x3, large n = 100000 | PASS in 75 s: 9841 arrays (length ≤ 8), 1800 rounds, n = 500000 | 10 |

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting/selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs in the package; no external code was copied. Sources read in the 2026-09-28 audit: cp-algorithms maximum/minimum sum segment (revision 2022-06-08; prefix-minimum, constrained length, rectangle bands), LIS (revision 2026-09-18) and Fenwick tree; OI Wiki 主元素问题 (cached); Misra and Gries, *Finding Repeated Elements* (1982), pp. 143–150. The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md); its agents fetched the CSES problem set, maspypy `seq` and KACTL `various` pages. The maximum-average hull method is proved here rather than taken from those pages.

## Limits and handoffs

- Finite corpora support the proofs above; they are not proofs themselves. Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- Checked builds cap the large sequence fixtures (and the interval fixtures) at 2000, because debug-STL partition checks make repeated `lower_bound` calls linear.
- GCC 14.2 exactly and Codeforces' Windows MSYS2 build were not run; the code uses no Windows-sensitive types or POSIX calls.
- Legacy `OLD/Team Notebook/src/misc/old_kadane.cpp` is accounted for by `kadane`.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): rotation and all-range inversions; subarray-sum counting; three- and four-sum. Value-distinct LIS counts, all-LIS enumeration and subcubic rectangle methods remain unowned research items.

## History

- 2026-09-28: first implementation and audit (MI03, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before changes; re-audit then added `maximumAverageSubarray`, `mex`, `increasingSubsequenceLengths`, `countIncreasingSubsequences` and `adjacentSwapDistance` (`/reaudit-review` findings 2, 3, 6–8, 10–12, 15, 17 fixed).
