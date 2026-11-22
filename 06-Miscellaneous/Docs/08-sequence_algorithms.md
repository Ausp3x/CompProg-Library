# 08-sequence_algorithms.hpp — Evidence

`08-sequence_algorithms.hpp` (batch MI03, package P014) covers maximum subarray with its bounded, circular, rectangle and average variants, increasing-subsequence lengths, counts and weights, inversions and adjacent-swap distance, `mex`, monotone windows, sorted pair sums, and majority and heavy hitters. It uses the DS02 Fenwick tree, `SegmentTree` and `CoordinateCompression`, and includes no prefix-sum or monotone-stack header. Every implemented operation of the row has an independent-oracle test (map below); the row's `missing:` operations are not implemented. Prerequisites P002, P006 and P013 are verified.

## Contracts

Common rules: [00-notes.md](00-notes.md), Basic Contracts.

### Sequence Result Records (`SubarraySum`, `CircularSubarraySum`, `SubrectangleSum`, `WeightedSubsequence`)

Plain aggregates. `exists()` is false exactly for an absent optimum (`l = r = -1`, `start = -1, length = 0`, all rectangle coordinates `-1`, with `sum = 0`). A legitimate empty optimum is present and has zero coordinates. `WeightedSubsequence::found` plays the same role, and `indices` holds the witness in increasing order.

### `sequence_detail`

`checkedSize` asserts `n < INT_MAX`. `checkInteger` rejects non-integral or wider-than-64-bit sum and weight types at compile time. `maximum(n, get, empty)` returns the best nonempty `[l, r)` by prefix difference; with `empty` it starts from `[0, 0)`, and only a strictly larger sum replaces it. `restore` follows a predecessor array.

### `maximumSubarray(a, allow_empty = false)`

Returns the best nonempty `[l, r)`. `n = 0` is absent unless `allow_empty`, which adds `[0, 0)` for every `n`. An all-negative nonempty optimum is a valid negative answer. Sums are exact in `lll`: inputs are at most 64-bit and `n < 2^31`, so `|sum| < 2^95`. Correctness: the best nonempty subarray ending at `r` is `prefix[r]` minus the smallest earlier prefix.

### `boundedMaximumSubarray(a, lo, hi)`

Domain `0 <= lo <= hi` (asserted). Returns the best sum with length in `[lo, hi]`; `lo > n` is a legitimate absence, `lo = 0` admits the empty interval, and `hi` may exceed `n`, up to `INT_MAX`. It keeps two running prefixes and a monotone deque of eligible starts `[r - hi, r - lo]`. Correctness: a newer smaller prefix dominates an older one at every later end where both are eligible, and expiry and dominance each remove an entry once.

### `circularMaximumSubarray(a, allow_empty = false)`

Returns `{sum, start, length}`, consuming `a[(start + i) % n]` for `i < length`, at most one full turn (`length <= n`). Empty input is absent unless `allow_empty`, whose empty result has `start = length = 0`. Correctness: a circular optimum is either an ordinary one or the complement of a proper nonempty minimum segment. Excluding a full-array minimum avoids an illegal empty complement; if the full array is the minimum, every proper complement is nonpositive and a single best element dominates it.

### `maximumSubrectangle(a, allow_empty = false)`

The input must be rectangular (asserted per row). Returns `[top, bottom) x [left, right)`; an empty dimension is absent unless `allow_empty`, which gives all-zero coordinates. Boundary pairs run over the smaller dimension and Kadane over the larger: `O(min(n, m)^2 * max(n, m) + n)` time. Sums are below `2^126`. Correctness: collapsing each band of the smaller dimension to exact sums enumerates every rectangle.

### `maximumAverageSubarray(a, lo)`

Domain `lo >= 1` (asserted), integral input of up to 64 bits. Maximizes the exact rational `sum / (r - l)` over windows of length at least `lo`, with no tolerance or floating point; the caller reads the average as `res.sum / (res.r - res.l)`. `lo > n` is absent. Comparisons cross-multiply in `lll`: a prefix difference is below `2^95` and a length below `2^31`, so every product is below `2^126`. O(n) time and space. Real inputs are outside the domain; scale them to integers.

Correctness: with `P_j = (j, prefix[j])`, the average of `[j, r)` is the slope from `P_j` to `P_r`. For each `r >= lo` the candidates `j <= r - lo` form a lower convex chain: back pops remove points on or above a chord, which by the mediant inequality never beat their neighbours. The slope to a point on the right is unimodal along the chain, so popping the front while the next start is at least as good finds the best start. Discarding front start `q_i` is safe for every later `r'`: its average to `r` is at most that of the surviving front `q_k`, hence `avg[q_i, q_k) <= avg[q_k, r)`, an already recorded candidate, and `avg[q_i, r') <= max(avg[q_i, q_k), avg[q_k, r'))`. Each index is pushed and popped once. Binary search on the answer would need a tolerance; this is exact.

### `increasingSubsequenceLengths(a, strict = true, cmp)` and `longestIncreasingSubsequence(a, strict = true, cmp)`

`res[i]` is the length of the longest chain ending at `i`. A strict chain needs `cmp(a[j], a[i])` between consecutive items; a non-strict chain needs `!cmp(a[i], a[j])`, so equivalent keys may repeat. Indices with equal `res` form a chain of the dual order (non-increasing for strict chains, strictly decreasing for non-strict), so grouping by `res` is a minimum cover of `a` by dual chains, of size `max(res)` (Dilworth/Mirsky; CSES Towers). `longestIncreasingSubsequence` returns the indices of one longest chain, empty for `n = 0`. No numeric sentinels, so any copyable key type works.

Correctness: patience tails keep the best final key per length, and the binary-search position plus one is the longest chain ending at `i`. Reconstruction works backward: `len[i] = L` implies an earlier `j` with `len[j] = L - 1` chaining to `i`, and the first such `j` scanning down from `i - 1` keeps the invariant. Equal-length indices `i < j` cannot chain, so equal-length classes are dual chains, and their number equals the longest chain length, a lower bound on any dual cover (Mirsky).

### `countLongestIncreasingSubsequences<Count>(a, strict = true, cmp)` and `countIncreasingSubsequences<Count>(a, strict = true, cmp)`

The first returns `{length, count}` of index-distinct longest chains, `{0, 1}` for `n = 0`. The second counts all nonempty index-distinct chains of any length, `0` for `n = 0`. The caller chooses `Count` explicitly; it needs `Count(int)` and `+` or `+=`, and every addition must fit unless `Count` is intentionally modular (`mint`, or wrapping `ulng` for mod `2^64`). The shared `SegmentTree` limits distinct comparator classes to `2^29`. Value-distinct counts are a different problem and are not provided. Correctness: indices are processed in order, querying counts at smaller ranks (strict) or no-greater ranks (non-strict); the longest-count variant adds only optimal-length predecessors, each ending index once, and the all-chain count adds one for starting fresh.

### `weightedIncreasingSubsequence(a, w, strict = true, allow_empty = false, cmp)`

Domain `w.size() == a.size()` (asserted), integral weights of up to 64 bits. Maximizes the total weight of a chain. The empty chain counts only with `allow_empty`; otherwise empty input is absent (`found = false`) and an all-negative input picks one best item. No secondary length objective. The `2^29` class limit applies. Correctness: each index extends the best positive predecessor.

### `inversionCount(a, cmp)` and `adjacentSwapDistance(a, b, cmp)`

`inversionCount` counts pairs `i < j` with `cmp(a[j], a[i])`, equivalent keys excluded, exactly in `lng` (at most `n(n-1)/2 < 2^61`). `adjacentSwapDistance` returns the fewest adjacent swaps turning `a` into `b`, or `-1` when `b` is not a rearrangement of `a` under comparator equivalence (a size mismatch included). The k-th equivalent copy in `a` goes to the k-th in `b`, which is optimal. Correctness: a Fenwick tree counts earlier strictly greater keys. In-order matching of equivalent copies avoids crossing equal elements, any other matching adds swaps, and a permutation's minimum swap count is its inversion count.

### `mex(a)`

The smallest nonnegative integer absent from an integral `a`; negative values are ignored, and the result is at most `n`. Dynamic mex belongs to Data Structures `24`, offline range mex to `32`. Correctness: values in `[0, n]` are marked and the first unmarked one is returned.

### `monotoneWindowEnds(n, can_add, add, remove)` and `nonnegativeSumWindowEnds(a, limit)`

The first returns the maximal valid right end for each left end. Validity must hold for empty windows and be hereditary when either end is deleted; this is the caller's proof obligation and is not checked. Callback state starts empty; `can_add(r)` is pure and tests appending `r`, while `add` and `remove` keep exactly the active window. An invalid singleton is skipped without callbacks. Total cost O(n) callback calls. The adapter needs nonnegative values and `0 <= limit <= LLL_MAX` (asserted) and tests `a[r] <= limit - sum`, so `sum + a[r]` never overflows. Correctness: hereditary validity makes the maximal right end monotone, and each end advances at most `n` times.

### `sortedPairSum(a, target)`

The input must be ascending (asserted). Returns any `i < j` with `a[i] + a[j] == target` exactly in `lll`, or `{-1, -1}`. Correctness: a too-small sum rules out its left element and a too-large sum its right one.

### `majorityElement(a, eq)` and `heavyHitters(a, k, cmp)`

`majorityElement` returns the Boyer–Moore `{representative index, exact frequency}` of a class with frequency above `n / 2`, or `{-1, 0}`. `heavyHitters` (Misra–Gries, `k >= 1` asserted) returns representatives and exact frequencies of the classes above `floor(n / k)`, sorted by `cmp`; `k = 1` returns none and `k > n` every class. Both verify every candidate in a mandatory second pass.

Correctness: Boyer–Moore cancels pairs of different classes and Misra–Gries groups of `k` distinct classes. At most `floor(n / k)` groups can be cancelled, so any class above the threshold survives, and the second pass makes the answers exact. Every counter unit a decrement scan visits was created earlier, so total work is linear.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `kadane(a)` | `maximumSubarray(a).sum`, an exact `lll`: `0` for `n = 0` (the absent sum), with no `lng` fit assert |

## Feature-to-Test Map

| Operation | Oracle and cases |
|---|---|
| `maximumSubarray`, `boundedMaximumSubarray`, `circularMaximumSubarray` | Every subinterval and circular window of all ternary arrays of length ≤ 7 (full) and 350 random arrays; every `(lo, hi)` up to `n + 1` and `hi = INT_MAX`; signed and unsigned 64-bit extremes; witnesses re-summed; the removed `kadane` cases: sum 0 for empty input and the exact `2^64 - 1` it rejected |
| `maximumSubrectangle` | Every rectangle of all `±1` matrices up to 3x3 and random matrices up to 5x6, both orientations; full-width magnitudes; ragged assert |
| `maximumAverageSubarray` | Every window of length ≥ `lo` for each `lo` in `[1, n + 1]`, compared as exact fractions; witness re-summed; absence for `lo > n`; full-width 64-bit triples; large increasing array's last window, constant array; `lo = 0` assert |
| `increasingSubsequenceLengths` | Subset-mask enumeration of the longest chain ending at each index; equal-length pairs checked as dual chains; strict and non-strict, reversed and equivalence comparators, strings |
| `longestIncreasingSubsequence` | Subset-mask optimum length plus witness order and indices; large increasing array |
| `countLongestIncreasingSubsequences`, `countIncreasingSubsequences` | Exact mask counts (index-distinct) for both strictnesses; exact `lll` `2^100`, `3^40 - 1`, `2^80 - 1` and equal-key counts; intentional `ulng` mod `2^64` |
| `weightedIncreasingSubsequence` | Mask optimum with negative and zero weights, empty allowed or not; witness sum; size assert |
| `inversionCount` | All-pairs count; equivalent keys, reversed comparator, strings, large reversed permutation |
| `adjacentSwapDistance` | BFS over all arrangements of arrays of length ≤ 5 against sorted, rotated and reversed targets and ±1-changed non-rearrangements; size mismatch; large reversed permutation `n(n-1)/2`; large alternating 0/1 against sorted `m(m+1)/2` (detects unstable matching) |
| `mex` | Set scan over every exhaustive, random and window array; large permutation and gap; full-width values |
| `monotoneWindowEnds`, `nonnegativeSumWindowEnds` | Direct per-left scans for limits `0..8` and distinct-key windows; callback balance; maximum `lll` limit; negative value or limit asserts |
| `sortedPairSum` | Pairwise oracle for targets `-1..10`; full-width unsigned pair; unsorted assert |
| `majorityElement`, `heavyHitters` | Exact frequency maps for every `k` in `1..n+2` and `INT_MAX`; equivalence comparators, strings; `k = 0` assert |

Hand mutation probes (2026-10-08, quick mode) were caught at the hull pop, lengths, chain counts, the reconstruction break, the average start, a one-sided multiset check and unstable matching; the two survivors, a tie-only front pop and a redundant mex bound, are equivalent. The header has no mewt record yet.

## Commands and Results

Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 (floor, `CXX=g++-14`), GNU++20, CPython 3.14.7, the runner's optimized, checked and ASan-UBSan builds. The 2026-11-10 library-wide runs (P274) used seed 42, beside the other suites, with the build cache emptied before each cold run; the times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; preflight quick peak 1055 MB; optimized 0.1 s compile + 0.9 s run, checked 40.0 s + 3.2 s, ASan-UBSan 41.6 s + 16.3 s; peak 1554 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; preflight quick peak 1087 MB; optimized 0.2 s + 1.2 s, checked 57.0 s + 5.5 s, ASan-UBSan 64.1 s + 20.6 s; peak 1515 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 52.2 s compile + 0.1 s run, checked 33.5 s + 2.8 s; peak 1028 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; preflight quick peak 1057 MB; optimized 0.1 s + 1.8 s, checked 17.4 s + 3.8 s, ASan-UBSan 19.7 s + 28.2 s; peak 864 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/06-Miscellaneous/08-sequence_algorithms_tester.py' --mode full --seed 20261017` | PASS, 3 configurations, 9 assertion probes, peak 709 MB |
| 2026-10-17 (P027) | same with `CXX=g++-14` | PASS, peak 742 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/06-Miscellaneous/08-sequence_algorithms_tester.py' --mode stress --seed 7` | PASS, peak 710 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, workspace, sanitizer self-tests; peak 1834 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/03-consistency.py'` | No errors |

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting and selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

Code written independently from the recurrences and proofs here; no external code copied. Read in the 2026-09-28 audit: cp-algorithms maximum/minimum sum segment (revision 2022-06-08; prefix minimum, constrained length, rectangle bands), LIS (revision 2026-09-18) and Fenwick tree; OI Wiki 主元素问题 (cached); Misra and Gries, *Finding Repeated Elements* (1982), pp. 143–150. The 2026-10-08 `@researcher` sweep, which fetched the CSES problem set, maspypy `seq` and KACTL `various` pages, is recorded in [00-sources.md](00-sources.md). The maximum-average hull method is proved here, not taken from those pages.

## Limits and Handoffs

- Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's and are not asserted.
- Checked builds cap the large sequence and interval fixtures at 2000, because debug-STL partition checks make repeated `lower_bound` calls linear.
- No `long` or POSIX-only calls.
- Legacy `OLD/Team Notebook/src/misc/old_kadane.cpp` is accounted for by `maximumSubarray(a).sum` (Legacy Mapping).
- Value-distinct LIS counts, all-LIS enumeration and subcubic rectangle methods remain unowned research items; the other P014 omissions and their reasons are in [00-notes.md](00-notes.md), Basic Contracts.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): LIS (longest increasing subsequence), two pointers (双指针), sliding window, maximum subarray (Kadane); 主元素问题, 最长上升子序列, 最长递增子序列, 逆序对, 最大子段和, 最长不下降子序列, 最大子矩阵和, 摩尔投票, 尺取法.

## History

- 2026-09-28: MI03 (P014) first implementation and audit; full passed.
- 2026-10-08: re-audit; full seed 1 passed in 3 configurations before changes; `maximumAverageSubarray`, `mex`, `increasingSubsequenceLengths`, `countIncreasingSubsequences` and `adjacentSwapDistance` added, 10 findings fixed; full seed 20261008 passed on g++ and g++-14, stress seed 7, integration and consistency passed.
- 2026-10-12: unused prefix-sum and monotone-stack includes and their tester checks removed; full seed 1 passed on g++ and g++-14, stress seed 3 passed.
- 2026-10-12: P013 prune migrated `Compression` to `CoordinateCompression`; full seed 20260927 passed on g++ and g++-14.
- 2026-10-13: prune removed `kadane` (cases moved to `maximumSubarray`); full passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (`SegmentTree` over the `Monoid` adapter, no API change); full seed 20261017 on g++ and g++-14 and stress seed 7 passed.
- 2026-11-07: P263 added the search aliases, then Chinese names (OI Wiki page titles and common names, Simplified and Taiwan or Hong Kong terms); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; `[[maybe_unused]]` on names only an `assert` uses, no logic change; no mutation campaign in P274 (user decision 2026-11-10: the first campaign stays with the header's own package); the library-wide runs passed.
