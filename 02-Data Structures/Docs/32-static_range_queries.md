# 32-static_range_queries.hpp — evidence

Package P032 (batch DS28). New header of free functions and query structs over a fixed array: distinct counts, value frequency, mex, strict majority and threshold frequency. Contest profile: no ISA code, no Barrett/Montgomery reduction and no tuned threshold, so no benchmark is required; the scratch timings under [Benchmarks](#benchmarks) record the constant. Dependencies: `00-monoids.hpp` (`Min`, `MajorityVote`), `02-fenwick.hpp`, `03-segmenttree.hpp`, `17-waveletmatrix.hpp`, `18-persistentsegmenttree.hpp`. Every algorithm is deterministic and exact.

## Contracts

Ranges are zero-based and half-open, `0 <= l <= r <= n`, asserted once per query (offline functions assert every query before answering). Offline functions return answers in query order and accept queries in any order, duplicates and empty ranges included. Values need only the operators named per item; no hashing is used.

### staticRangeCountDistinct

`staticRangeCountDistinct(a, qs)`: for each `(l, r)` the number of distinct values in `a[l, r)`; an empty range gives 0. `T` needs only `<` (equality is `!(x < y) && !(y < x)`); `n < 2^31`.

- Correctness: sort `(a[i], i)` stably by value to get `prv[i]`, the previous index with an equal value (or -1). Sweep `r` upward. After positions `< r` are added, the Fenwick tree holds +1 exactly at the last occurrence before `r` of each value (adding `i` moves the mark from `prv[i]` to `i`). So `sum(l, r)` counts values whose last occurrence before `r` is at least `l`, which is the number of distinct values in the range.
- Queries are bucketed by `r` with a counting sort (`static_range_queries_detail::byRight`), O(n + q).
- Cost: O(n * log(n)) sort, O((n + q) * log(n)) sweep, O(n + q) memory.

### StaticRangeFrequency

`StaticRangeFrequency<T>(a)`, `count(l, r, x)`: the number of `i` in `[l, r)` with `a[i]` equivalent to `x` under `<`. `x` need not occur in `a` (gives 0). `T` needs only `<`; `n < 2^31`; default construction gives `n = 0`.

- Storage: the pairs `(a[i], i)` sorted by value, then position. `count` is the difference of two lower bounds on `(x, r)` and `(x, l)`. O(n * log(n)) build, O(log(n)) query, O(n) memory.
- Beside row 17 `WaveletMatrix::countEqual`, row 16 `MergeSortTree::countEqual` and row 18 `PersistentValueTree::countEqual`, this is kept as a constant-factor variant (ruling 4 of `duplicates.md`): sorted (value, position) pairs, O(n) words, one pair of binary searches, and only `<` needed.

### rangeMex

`rangeMex(a, qs)`: for each `(l, r)` the least nonnegative integer absent from `a[l, r)`; an empty range gives 0. `T` is a signed or unsigned integer type other than `bool` and the character types (`std::cmp_less` rejects those); values below 0 or at least `n` are ignored, because a range of length at most `n` has mex at most `n`. `n <= 2^29` (the segment tree's size bound).

- Correctness: sweep `r`; a `SegmentTree<Min<int>>` over values `[0, n)` holds the last occurrence before `r` (or -1). Then `v` is in `a[l, r)` iff `last[v] >= l`, so the mex is the first `v` with `last[v] < l`. `maxRight(0, x >= l)` returns that `v`, or `n` when all of `[0, n)` occur (then the range is the whole array). The predicate holds on `e() = INT_MAX` and stays false once it fails, as `maxRight` requires.
- Cost: O((n + q) * log(n)) time, O(n + q) memory.

### PersistentRangeMex

`PersistentRangeMex(a)`, `mex(l, r)`: the same results as `rangeMex`, online. `T` is as for `rangeMex`; `n < 2^26`, so the reserved `(m + 2) * (bit_width(n) + 1)` pool nodes, for `m` values in `[0, n)`, fit `int` indices. That bound covers the fill build's at most `2 * (bit_width(n) + 1)` nodes and at most `bit_width(n) + 1` path copies per set, so the pool never grows during the sets; ignored values reserve nothing. Default construction gives `n = 0`.

- Version `ver[r]` of a `PersistentSegmentTree<Min<int>>` (filled with -1) is `rangeMex`'s `last` array at `r`; an ignored value reuses the previous version. A query runs `maxRight` on `ver[r]`.
- Cost: O(n * log(n)) build and memory (the pool is reserved up front), O(log(n)) per query. Choose it over `rangeMex` when queries arrive online (forced decoding); the offline sweep uses O(n) memory.

### RangeMajority

`RangeMajority<T>(a)`, `majority(l, r, res)`: returns true and stores in `res` the value occurring more than `(r - l) / 2` times (`2 * count > r - l`), or returns false and leaves `res` untouched. An empty range gives false. `T` is default-constructible (`MajorityVote::e()`), ordered by `<` and comparable by `==`, where `==` agrees with the equivalence of `<` (`x == y` iff neither is less); `n <= 2^29`.

- Candidate: a `SegmentTree<MajorityVote<T>>` product over the range. `MajorityVote::op` is not associative, but every bracketing keeps the invariant of [00-monoids.md](00-monoids.md) (`s_y >= count_y - (len - count_y)` for every value `y`), so a strict majority ends as the candidate. `SegmentTree::prod` combines the range's leaves in order under some bracketing (its left and right accumulators), which is all the invariant needs; row 03's associativity requirement is relaxed here on that implementation property; row 32's majority tests exercise it with the tree's real bracketings. Verification: `StaticRangeFrequency::count`. Exact, deterministic, O(n * log(n)) build (the sort), O(log(n)) query, O(n) memory.
- Not the wavelet median (`RangeThresholdFrequency::frequent` with `t = (r - l) / 2 + 1` gives the same answer by another algorithm, kept by user decision 2026-11-02).

### RangeThresholdFrequency

`RangeThresholdFrequency<T>(a)`, `frequent(l, r, t)`: every `(value, count)` with `count >= t` in `[l, r)`, by increasing value; `t >= 1` is asserted. `t = 1` lists all distinct values with their counts, and `t > r - l` gives an empty list. `T` needs `<` and an `==` that agrees with its equivalence (`WaveletMatrix` deduplicates with `unique`); `n < 2^31`.

- Correctness: in sorted order, a value with count `c >= t` fills a block of `c` consecutive ranks, which contains a rank `k` with `k % t == t - 1`. So the candidates are `kth(l, r, k)` at such ranks. One `WaveletMatrix::descend` on the candidate gives both the count below it and its own count (the run it leaves), so the scan reports it when its count is at least `t` and jumps to the first sampled rank after its block. Every candidate block is visited once. The rank counter is `lng`, so the jump cannot overflow.
- Cost: O((1 + min(d, (r - l) / t)) * log(n)) per query for `d` distinct values in the range, with at most `(r - l) / t` reported pairs; the wavelet matrix takes O(n * log(n)) build and O(n * log(n)) bits. `descend` is row 17's public helper; its contract is in [17-waveletmatrix.md](17-waveletmatrix.md) (user decision 2026-11-02), and the scan relies only on it. The candidate's rank is recomputed by `lowerRank` (one binary search per candidate, dominated by the descents) rather than read from the address `kth` returns, which would rely on an undocumented internal. A k-majority query (count above `(r - l) / k`) is `t = (r - l) / k + 1`.

### Aliases

- Static range frequency (Library Checker `static_range_frequency`): `StaticRangeFrequency::count`. Distinct count (`static_range_count_distinct`, ei1333 `static-range-count-distinct`): `staticRangeCountDistinct`.
- k-majority and Misra–Gries frequent elements: `frequent` with `t = (r - l) / k + 1`. All-distinct test: `staticRangeCountDistinct == r - l`.
- Online distinct count: row 18 `PersistentDistinctCount` (online, kept by user decision 2026-10-27).

Search aliases (P263, grab --find): 主元素问题, 线段树与离线询问, 区间数颜色, 区间不同数个数, 区间出现次数, 区间mex, 区间绝对众数, 区间频率.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/32-static_range_queries_tester.cpp`, entry `32-static_range_queries_tester.py` (driver `_00_runner.py`; optimized, checked and ASan/UBSan builds; 24 assertion probes). BF means a brute force over the range with an ordered map or a direct count; the literal hand cases check the brute forces themselves. Seven deliberately broken scratch copies of the header (truncated mex values, an off-by-one majority test, wrong `frequent` ranks and others) all failed the tester.

| Operation | Groups | Oracle |
|---|---|---|
| `staticRangeCountDistinct` | hand, edges, exhaustive (`int`, `string`, a key with only `<`, a point struct), random, scaleGeneric | literals; BF; Mo's algorithm at scale |
| `StaticRangeFrequency::count` | the same groups; `x` below all, above all, between values, absent, present | BF; an offline prefix sweep at scale |
| `rangeMex` | hand, edges, mexExhaustive (`int`, `lng`, `uint`, `ulng` with -1, 2^32 + 1, `LLONG_MIN`, `UINT_MAX`, `ULLONG_MAX`, values at `n - 1`, `n`, `n + 1`), randomMex (six profiles), scaleMex | BF; Mo's algorithm with sqrt blocks at scale |
| `PersistentRangeMex::mex` | as `rangeMex`, plus default construction and `n = 0` | the same |
| `RangeMajority::majority` | hand, edges, exhaustive, random (`int`, `lng`, `string`, point struct; dominant value at 50, 51, 60, 70, 95%), scaleGeneric samples | BF; a counting tally at scale; `res` checked untouched on false |
| `RangeThresholdFrequency::frequent` | the same; exhaustive tries every `t` in 1 … len + 1; random tries `t` in {1, 2, len/3 + 1, len/2, len/2 + 1, len, len + 1, `INT_MAX`} | BF; a counting tally at scale |
| Preconditions | probes: `l > r`, `r > n`, `l < 0` for every operation; `t = 0` and `t = -1` for `frequent` | checked build aborts |

Arrays: tiny and wide alphabets with extremes, one dominant value, all equal, sorted, reversed, runs, permutations of `0 … n - 1` and gapped `0 … k`. Queries: random order with duplicates, empty ranges, `l == r == n`, full, prefix, suffix and short ranges.

Modes: quick runs exhaustive lengths up to 4, 10 random rounds per type and profile with `n <= 24`, and a scale test at n = q = 1000. Full runs exhaustive lengths up to 6 (mex up to 5 over 6 values), 60 generic and 100 mex rounds with `n <= 64`, and scale n = q = 2 · 10^5. Stress runs exhaustive lengths up to 7 (mex 6), 400 and 600 rounds with `n <= 160`, and scale n = 5 · 10^5, q = 10^5; the ASan build runs 1/8 of the rounds at n = q = 2 · 10^5. The checked build's scale tests use n = 3000 in full and stress, since debug `lower_bound` is linear. `mexExhaustive` also covers `int8_t` with -128, -1 and 127.

## Benchmarks

Scratch timing only, not a gate. Conditions: 2026-11-02, AMD Ryzen AI 9 HX 370 (WSL2), `g++` 16.0.1 `-std=gnu++20 -O2`, one run, n = q = 5 · 10^5, uniform values in `[0, σ)` and uniform ranges from `mt19937` seed 1. Each time includes the build, and every answer is summed so the work stays live.

| Operation | σ = 10^9 | σ = 1000 | σ = 2 |
|---|---|---|---|
| `staticRangeCountDistinct` | 51 ms | 56 ms | 50 ms |
| `StaticRangeFrequency` (build + q counts) | 135 ms | 127 ms | 113 ms |
| `rangeMex` | 39 ms | 46 ms | 58 ms |
| `PersistentRangeMex` | 21 ms | 325 ms | 298 ms |
| `RangeMajority` | 194 ms | 207 ms | 266 ms |
| `frequent`, `t = max(1, (r - l) / 4)` | 797 ms | 306 ms | 35 ms |

The `PersistentRangeMex` and `frequent` rows were rerun after the review changes (pool reserve, block jump). Peak memory was 132 MB. The Library Checker limit for both problems is 5 s.

## Commands and results

Latest run 2026-11-02, Linux x86-64 (AMD Ryzen AI 9 HX 370, WSL2), GNU++20, CPython 3.14.4:

| Command | Result |
|---|---|
| `32-static_range_queries_tester.py --mode quick` | PASS, 2 configurations, 478,667 checks (optimized), 478,424 (checked), 24 assertion probes; MEMORY peak 805 MB |
| `32-static_range_queries_tester.py --mode full --seed 20261102` (`g++` 16.0.1) | PASS, 3 configurations; 8,584,909 checks (optimized and ASan), 4,240,709 (checked); MEMORY peak 955 MB |
| `CXX=g++-14 32-static_range_queries_tester.py --mode full --seed 20261102` (g++-14 14.3.0, floor) | PASS, the same check counts; MEMORY peak 860 MB (pre-flight) |
| `32-static_range_queries_tester.py --mode stress --seed 7` | PASS, 3 configurations; 29,103,590 checks (optimized), 21,409,440 (checked), 25,753,609 (ASan); MEMORY peak 955 MB |
| `02-integration.py` | PASS, 185 standalone and aggregate headers, scalar and AVX2 multi-TU, workspace; MEMORY peak 1540 MB |
| `03-consistency.py --braces` on the header and tester | No closing-brace, comment-cap or free-function violations |
| `03-consistency.py`, `plan.py check` | No errors (maintainer gate `merge_pr.py gates 1`, 2026-11-03); `plan.py doctor --fix` declared the DS05 prerequisite for the `00-monoids.hpp` include |

The memory peaks cover the whole runner scope, compiles included; the test binaries stay under 170 MB.

```bash
python3 '96-Local Testing/02-Data Structures/32-static_range_queries_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/32-static_range_queries_tester.py' --mode full --seed 20261102
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/32-static_range_queries_tester.py' --mode full --seed 20261102
python3 '96-Local Testing/02-Data Structures/32-static_range_queries_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `static_range_count_distinct`, `static_range_frequency`, `majority_voting` (task statements and `info.toml`) | Statements and limits; `majority_voting` has point updates and stays outside the row. |
| ei1333 `other/static-range-count-distinct.hpp` | The offline Fenwick sweep over last occurrences. |
| maspypy `ds/static_range_frequency.hpp` | Position lists per value (a hash map there; a sorted pair array here, which needs only `<`). |

The completeness sweep fetched these on 2026-11-02 ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds28-sweep)). The mex sweep, both mex structures, the majority verification and the threshold sampling argument were written from the contracts above.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p032-omissions) (user decision 2026-11-02): weighted distinct sum, closest equal pair, exact-`k` frequency counts, the all-distinct test, xored mex, `RangeMexChange` and the k-th mex.
- Violated preconditions under `NDEBUG`: an out-of-range query bound makes `byRight` index outside its bucket array, so the offline functions are undefined behaviour there rather than merely meaningless. The asserts catch it on every judge, because judges keep assertions. `PersistentRangeMex::mex` with a bad `r` reads `ver[r]` out of bounds the same way, and a constructor with `n` above about 7.7 · 10^7 (beyond the `2^26` bound) overflows the `int` reserve size. `frequent` with `t <= 0` returns an empty list (the loop guard; the start rank is computed in `lng`, so `t = INT_MIN` does not overflow); a scratch `-O0 -DNDEBUG -fsanitize=undefined` probe of `t = 0`, `-1` and `INT_MIN` terminated without a report on 2026-11-02.
- `rangeMex` and `RangeMajority` inherit the `2^29` size bound of `SegmentTree`, and `PersistentRangeMex` is limited to `2^26` by its pool indices. None of these limits matters at contest sizes, and no test reaches sizes near them.
- Integer types: `bool`, `char` and `char8_t` are rejected at compile time (checked ad hoc by the `/finish` audit, 2026-11-02); `int8_t`, `int`, `uint`, `lng`, `ulng` are tested; `int16_t`, `uint16_t` and `__int128` compile but are untested.
- The scratch mutant run (7 broken header copies), the `NDEBUG` probes and the timing table were one-off scratch runs with no committed driver.
- API names (`count`, `mex`, `majority` with a `bool` status, `frequent` returning `(value, count)` pairs) kept by user decision 2026-11-02.
- `00-Guidelines/Ledgers/library-checker-coverage.json`: `majority_voting` now targets rows 00, 03 and 16 and `point_set_range_frequency` row 16 only, since row 32 has no point update (user decision 2026-11-02).
- No `97-Online Testing` solution uses this header yet. The Library Checker problems named above were not submitted (`01-principles.md`).

## History

- 2026-11-02: P032 DS28 first implementation; tester follow-ups fixed (type requirements of `RangeMajority` and `RangeThresholdFrequency`, `lng` rank counter in `frequent`); independent review, 6 findings and 5 nits fixed (comment cap, `NDEBUG` loop on `t <= 0`, majority proof from the bracketing invariant, pool reserve, `duplicates.md` keeps, mex type domain with an `int8_t` case; `byRight` size, `n <= 2^29`, test-mode and benchmark wording, `frequent` jumps past each block); 7/7 scratch mutants caught; quick, full (seed 20261102) on g++ and g++-14, stress (seed 7) and `02-integration.py` passed.
- 2026-11-02: `/finish` audit, 3 defects and 2 nits fixed (`t = INT_MIN` start rank, `n <= 2^29` wording, pool reserve only for values in `[0, n)`), limits recorded; user decisions recorded (`descend` contract in row 17, API names kept, Library Checker ledger retargeted); diff review, 4 nits fixed; quick, full (seed 20261102) on g++ and g++-14, stress (seed 7), row 17 quick, `02-integration.py` and `03-consistency.py` passed.
- 2026-11-03: maintainer `/merge-pr 1`: the real `03-consistency.py` and `plan.py check` results recorded and the clone-specific limit dropped (P250's note keeps the follow-up); row 17 lists and tests `descend` and `lowerRank`, which `frequent` uses; the first maintainer gate (2026-11-03) passed quick and full on g++ and g++-14 and `02-integration.py --sanitizers`.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
