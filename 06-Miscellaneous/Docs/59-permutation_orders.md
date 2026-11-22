# 59-permutation_orders.hpp — Evidence

`59-permutation_orders.hpp` provides the Trotter–Johnson (Steinhaus–Johnson–Trotter) order of permutations, in which consecutive permutations differ by one adjacent transposition: rank, unrank, successor and an enumerator. Lexicographic and Myrvold–Ruskey orders are row 07 (`07-permutation.hpp`), which this header includes for `isPermutation`, `permutationInverse` and `permutationSign`; it also includes row 06 (`06-bit_operations.hpp`) for `BitOps::select`. Batch MI02, package P013; the row was added by user decision 2026-11-17, in Esoteric because SymPy is the only catalog naming it. Dependencies: P002 (template) and rows 06 and 07 of the same package.

## Contracts

### Order

Permutations map positions to values on `[0,n)`, as in row 07 (`p[i]` is the value at position `i`). The order for `n` takes each permutation `q` of the order for `n-1` in turn, at rank `r'`, and inserts the value `n-1` into each of its `n` gaps, from right to left when `r'` is even and from left to right when `r'` is odd. The order for `n <= 1` is the one permutation. It starts at the identity and, for `n >= 2`, ends at the identity with its first two values swapped, which also differs from the identity by one adjacent transposition. For n = 3 it is 012, 021, 201, 210, 120, 102 (Wikipedia's 123, 132, 312, 321, 231, 213, lowered by one).

### trotterJohnsonRank

`trotterJohnsonRank(p)` returns the rank of `p`, in `[0, n!)`, for `n <= 20` (asserted, with `isPermutation`). O(n) time and memory: one inverse, then for `j = 1..n` the number `k` of values below `j-1` to its left, counted by `popcount` over a bitmask of the positions of the values seen so far.

Correctness: the restriction of `p` to the values `0..j-1` is the order-`j` permutation of the recursion, and `k` is the gap of `j-1` in it. By the definition its rank is `r_j = j * r_{j-1} + (r_{j-1}` even `? j-1-k : k)`, since the sweep from the right visits gap `j-1` first. The loop computes `r_1 .. r_n` from `r_0 = 0`; `r_n < 20! < 2^63`, so no product overflows.

### trotterJohnsonUnrank

`trotterJohnsonUnrank(n, rank, out)` writes the permutation of rank `rank` to `out` and returns `true`, for `0 <= n <= 20` (asserted). If `rank >= n!`, it returns `false` and leaves `out` unchanged. O(n) time and memory: one `BitOps<uint>::select` per value over a mask of the free positions (`06-bit_operations.hpp`). Under `NDEBUG` an `n` outside `[0,20]` returns `false` with `out` unchanged; the assertion sits inside that guard, so the checked-build probes exercise the guard itself.

Correctness: `r_{j-1} = floor(r_j / j)`, and the gap of `j-1` is `r_j - j * r_{j-1}` read through the same parity rule. A rank below `n!` gives `r_0 = 0` and one at or above it `r_0 >= 1`, so `r_0 != 0` is exactly the out-of-range test. Values are placed from `n-1` down: when `j-1` is placed, the free positions are exactly those the values `0..j-1` occupy in the result, in increasing order, so the decoded gap indexes the free positions.

### nextTrotterJohnson

`nextTrotterJohnson(p)` replaces `p` by its successor and returns `true`; from the last permutation it wraps to the identity and returns `false`, like `std::next_permutation`. The empty and one-element permutations wrap at once. O(n) time and memory per call (the inverse and the sign), then one pass from the largest value down.

Correctness: values `m, ..., n-1` that have finished their sweep are parked at the two ends of the array, so the values below `m` occupy one block `[st, st+m)`. At level `m` the loop holds the parity of `r_m`, the rank of the block's restriction. It starts as the parity of `p`, since every step of the order is one adjacent transposition from the identity. Dropping `m-1` removes its `m-1-d` inversions, `d` being its position in the block, so the update yields the parity of `r_{m-1}`, which gives the sweep direction of `m-1` (left to right when odd). If `m-1` can still move that way, the successor is that single swap; otherwise `m-1` is parked at the end it reached, the block shrinks from that side, and the successor is that of the smaller restriction. If no level can move, `p` was the last permutation.

### forEachTrotterJohnson

`forEachTrotterJohnson(n, visit)` calls `visit(p, i)` on the `n!` permutations of `[0,n)` in order, for `n >= 0` (asserted; the direction and step tables have `n + 1` entries sized in `size_t`, so no `n` overflows them). The first call has `i = -1` and `p` the identity; every later call has `0 <= i < n-1`, and `p` is the previous permutation with positions `i` and `i+1` swapped. `p` is a const reference valid during the call. `visit` returns `true` to continue; `false` stops at once and makes the function return `false`, even at the last permutation. Otherwise the function returns `true` after the last permutation. O(n) memory, O(n + n!) time: O(1) amortized per permutation. The convention is that of `forEachGrayProduct` (`12-enumeration.hpp`).

Correctness: this is Even's plain changes (Knuth's Algorithm P), with `c[j]` the steps value `j-1` has taken in its current sweep across the `j` gaps of the restriction to `0..j-1` and `o[j]` its direction. A level whose next step would leave `[0, j)` reverses its direction and passes the step to level `j-1`, which is the recursion of the order: the largest value sweeps once while the restriction stays fixed, then the restriction takes one step. While the step moves down the levels, `s` counts the levels whose values are parked at the left end, shifting the block of smaller values. Value `j-1` sits at 1-based position `j - c[j]` of the block of values below `j`, which starts at 0-based position `s`, and moves to position `j - q`; the two are adjacent, so the swapped 0-based positions are `min(j - c[j], j - q) - 1 + s` and the next one. A step from level `j` resets the scan to level `n`; the levels below `n` are visited once per `n` steps on average in a sweep of length `n`, so the total is O(n!) and each permutation costs O(1) amortized. When every level reverses, the enumeration is complete.

## Feature-to-Test Map

| Feature | Verification |
|---|---|
| The order itself | A recursive list built from the definition, checked to hold `n!` distinct permutations, start at the identity and step by one adjacent transposition, through n = 6 (quick), 8 (full) and 9 (stress); an independent Even-style walk with a direction and a sweep counter per value (Algorithm P in the tester's naming), checked against that list, is the oracle at larger n |
| `trotterJohnsonRank` | Every list entry gives its index; unrank round trips at random n in [9, 20] and at n = 19 and 20 (identity 0, last permutation n! - 1, reversed round trip); rank + 1 at every step of the walks below; Python exact ranks through n = 20 by the recursive formula, itself checked against a recursive list through n = 6 and a largest-mobile-element walk through n = 7 |
| `trotterJohnsonUnrank` | Every list entry, with `out` prefilled with values of another size; ranks n!, n! + 1, 7n! + 3 and 2^64 - 1 return false with `out` unchanged; at n in [9, 20] an independent decoder building the walk's state from the rank's mixed-radix digits, and absent ranks up to 2^64 - 1; Python unrank including n!, n! + 1, 2^64 - 1 and random ranks of at least n! |
| `nextTrotterJohnson` | Every step of every list follows it and wraps to the identity after n! steps, n = 0 and 1 included; 1000-step walks at n in [9, 20] against rank + 1 mod n!, one adjacent transposition and the walk oracle, across the wrap too; in full and stress, long walks at n = 100,000 and 1,000,000 against the walk oracle from the identity, from states whose middle levels sit at their sweep ends (so one step cascades through them) and from the last permutation, which must wrap; Python exact rank + 1 up to n = 200 at random and cascade ranks |
| `forEachTrotterJohnson` | For every n through 6 (quick), 8 (full) and 9 (stress), n = 0 and 1 included, the visited sequence equals the recursive list (n! calls in order, return true); the first `i` is -1 with the identity and every later `0 <= i < n-1` with `p` the previous permutation swapped at `i`, `i+1` (whole `p` compared for n <= 64, else the swapped pair every call and the whole every n calls); the visitor receives `const vector<int> &` and `int` (`static_assert`). Early stop at call t ends after exactly t + 1 calls at the t-th list entry and returns false: every t through n = 7 (the last permutation included), t in {0, 1, n!-2, n!-1} and 64 random t at n = 8 and 9, n!-2 and n!-1 at n = 10 and 20 random t at n = 20 against unrank, t = 0 and 1 at n = 10^5, with an lvalue functor. Large n step by step against the walk oracle's swap index, the first calls also against `nextTrotterJohnson` and `trotterJohnsonRank` = call index (n <= 20): n = 10 whole (quick 40,000 calls), n = 11 and 12 1e6 calls (stress 3e6), n = 25 3e6 calls (stress 1e7; quick 40,000), n = 10^6 4e6 calls (stress 1e7; quick n = 10^5, 300,000). The levels passed (n - max(p[i], p[i+1]) per step) total at most 2·calls + n, the amortized bound as a count, not a timing. Python oracle through n = 7 against Even's largest-mobile-element algorithm: swap indices, call count and return flag, stopping at 2^64 - 1, n!, n! - 1, 0 and a random call. Nine hand mutants of the loop (direction flip, `s` update and reset, `min`/`max`, first `i`, stop handling, return value) were each killed in quick mode |
| Preconditions | 6 checked-build assertion probes: `rank-size` (identity of length 21), `rank-invalid` ({0, 2}), `unrank-negative` (n = -1), `unrank-size` (n = 21), `next-invalid` ({0, 0}), `foreach-negative` (n = -1) |

Python oracle cases: 1,134 quick, 8,334 full, 32,334 stress. Stress runs 1000 random cases at n in [9, 20] to keep the ASan-UBSan build under half of its 720 s limit.

## Commands and Results

P013 MI02 `/finish` runs: Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, optimized, checked and ASan-UBSan builds from [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), beside other test runs and mutation campaigns.

| Date | Command | Result |
|---|---|---|
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/59-permutation_orders_tester.py' --mode quick` | PASS, 2 configurations, 1,134 Python cases; run optimized 0.15 s, checked 3.03 s; `MEMORY peak=826MB` |
| 2026-11-17 | `... --mode full --seed 1` | PASS, 3 configurations, 8,334 Python cases, 6 probes; run optimized 2.8 s, checked 19.2 s, ASan-UBSan 74.1 s; `MEMORY peak=1017MB` (preflight quick 822 MB) |
| 2026-11-17 | same with `CXX=g++-14` | PASS, same counts; run optimized 1.8 s, checked 13.6 s, ASan-UBSan 55.0 s; `MEMORY peak=1056MB` (preflight quick 884 MB) |
| 2026-11-17 | `... --mode stress --seed 2` | PASS, 32,334 Python cases, 6 probes; run optimized 3.4 s, checked 23.4 s, ASan-UBSan 114.8 s of the 720 s limit; `MEMORY peak=1070MB` |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' ops --filter 06-Miscellaneous/59-permutation_orders.hpp` | PASS, 4 operations in 4 extractions on g++ and g++-14, 0 failures (no extraction candidate instantiates the template `forEachTrotterJohnson`; the tester does) |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' testers --filter 59-permutation_orders_tester` | PASS, quick run on the extracted build (5 names) |
| 2026-11-17 | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 192 standalone and aggregate headers (with the new `59-permutation_orders.hpp` and `60-permutation_problems.hpp`), scalar and available AVX2 multi-TU builds, workspace, sanitizer self-tests, rerun on the final headers; `MEMORY peak=2703MB` |
| 2026-11-17 | `python3 '96-Local Testing/04-mutation.py' run 06-Miscellaneous/59-permutation_orders.hpp` | Mutation Testing |

## Mutation Testing

Record: mewt 4.0.0 on blob `90b178dc9024c357109a1211b2b20e9664bb0ab1` (2026-11-17, seed 1, whole file): 368 mutants, 345 caught (0 by timeout), 23 survived, 0 untested.

Stage seconds: quick 353, full on the optimized build 27, full on the checked and ASan-UBSan builds 760. An earlier campaign the same day left two survivors in the `NDEBUG` clamp of `trotterJohnsonUnrank`, closed by moving its assertion inside the guard. The 23 survivors are equivalent:

| Mutant | Line | Why it survives |
|---|---|---|
| `BL 87` | 17 | `return true` for `return false` in the guard: only an `NDEBUG` run past the violated precondition reaches it (otherwise the assertion fires first), and it still returns at once with `out` unchanged. |
| `COS 292`, `COS 294`, `COS 297`, `COS 299` | 17 | `==` or `<` for `<=` in the assertion inside the guard: the guard is entered only when n < 0 or n > 20, where the mutated condition is false as well. |
| `BAOS 261` | 13 | `seen ^= bit` for `seen \|= bit` in `trotterJohnsonRank`: positions are distinct, so the bit is clear before. |
| `COS 303`, `COS 308` | 19, 22 | `j != 0` for `j > 0`: `j` falls by one from n >= 0. |
| `MR 364` | 25 | Copy instead of `std::move` into `out`: one O(n) copy within the bound. |
| `CR 48` | 28 | Dropping `assert(isPermutation(p))` in `nextTrotterJohnson`: `permutationInverse` on line 28 asserts the same (and `permutationSign` on line 29), aborting the probe `next-invalid`. |
| `COS 314` | 29 | `permutationSign(p) <= 0`: the sign is ±1. |
| `COS 321` | 30 | `m >= 1`: at m = 1 the block holds one value at d = 0, so neither swap applies and the loop ends. |
| `AOS 180`, `AOS 184` | 32 | `m + 1 - d` or `m - 1 + d` for `m - 1 - d`: the same parity. |
| `AS 76`, `AS 77`, `AS 84` | 33, 34, 49 | Swapped `std::swap` arguments. |
| `COS 323`, `COS 328` | 33, 34 | `d != m - 1` for `d < m - 1` and `d != 0` for `d > 0`: d lies in [0, m - 1]. |
| `COS 341` | 43 | `j >= 1` in `forEachTrotterJohnson`: level 1 is reached only after every higher level has turned, and it turns as well (`q` is 1 or -1), ending the loop. |
| `COS 351`, `COS 356` | 45, 46 | `q >= j` for `q == j`: c[j] <= j - 1 and o[j] is 1 or -1, so q <= j. |
| `AS 83` | 48 | Swapped `min` arguments. |

## Sources

Read 2026-11-17; implemented locally, no code copied.

- [Wikipedia, Steinhaus–Johnson–Trotter algorithm](https://en.wikipedia.org/wiki/Steinhaus%E2%80%93Johnson%E2%80%93Trotter_algorithm): the recursive definition (the largest value inserted sweeping alternately), the n = 3 and n = 4 sequences, the first and last permutations, and Even's speedup with directed values (the tester's independent walk).
- Knuth, TAOCP Vol. 4A §7.2.1.2, Algorithm P (plain changes): named in the tester as the source of its walk oracle, written from recall; the text was not read, and the oracle is validated against the recursive list instead.
- [SymPy, Permutations](https://docs.sympy.org/latest/modules/combinatorics/permutations.html): `rank_trotterjohnson`, `unrank_trotterjohnson` and `next_trotterjohnson`, the catalog entry behind this row; the Kreher–Stinson text it names was not read.

## Limits and Handoffs

- Ranks need `n <= 20` to fit `ulng`; a modular or big-integer rank of longer permutations is not provided (no catalog names one).
- `nextTrotterJohnson` takes a plain permutation, so each call pays O(n) to recover the sweep directions; a whole traversal is `forEachTrotterJohnson`, which keeps them between steps (user decision 2026-11-17).

## Aliases

Search aliases (grab --find): Steinhaus-Johnson-Trotter, Johnson-Trotter, SJT, plain changes, Even's algorithm, Algorithm P, adjacent transposition Gray code, rank_trotterjohnson, unrank_trotterjohnson, next_trotterjohnson, 邻位对换法.

## History

- 2026-11-17: P013 MI02 `/finish` created the row (user decision 2026-11-17) with `trotterJohnsonRank`, `trotterJohnsonUnrank` (O(n) with `BitOps::select` after the review), `nextTrotterJohnson` and Even's enumerator `forEachTrotterJohnson` (user decision); the unrank's size assertion moved inside its `NDEBUG` guard after a first mutation campaign; full and stress passed on g++ and g++-14; whole-file mutation campaign, 23 equivalent survivors.
