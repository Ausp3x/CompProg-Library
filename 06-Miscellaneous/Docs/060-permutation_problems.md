# 060-permutation_problems.hpp — Evidence

`060-permutation_problems.hpp` holds standalone one-problem permutation constructions that are not compositions of row 07's operations (`02-structure.md`, Tiers and Core Order). It has one today: the lexicographically largest permutation with a given number of inversions. Batch MI02, package P013 (moved from row 07 by user decision 2026-11-17); dependency P002 (template).

## Contracts

### permutationWithInversions

`permutationWithInversions(n, k)` returns the lexicographically largest permutation of `[0,n)` with exactly `k` inversions, for `n >= 0` and `0 <= k <= n(n-1)/2` (asserted; the bound is computed in `lng`, so it holds for every `int` n). O(n) time, O(n) memory for the result. CSES 2214 (Inverse Inversions) accepts any permutation with `k` inversions, on values `1..n`.

Correctness: the first value `x` among `m` remaining values contributes `x` inversions, and the other `m-1` values at most `(m-1)(m-2)/2`. The greedy places the largest remaining value while `k >= m-1`; the rest can still absorb what is left because `k <= m(m-1)/2`. When `k < m-1`, it places the value with exactly `k` smaller ones left and then the rest ascending, with no further inversion. Each placed value is the largest that keeps `k` reachable, so the result is the lexicographically largest. Equivalently, its Lehmer code is the greedy code `d_i = min(rest, n-1-i)`, the largest code with digit sum `k`, and permutations order as their Lehmer codes do.

## Feature-to-Test Map

| Feature | Verification |
|---|---|
| `permutationWithInversions` | Exhaustive: for every n through 6 (quick), 8 (full) or 9 (stress) and every k in [0, n(n-1)/2], equality with the lexicographically largest permutation with k inversions from a recursive enumeration, itself checked against a brute O(n^2) count and the Mahonian numbers; the greedy Lehmer-code oracle (digits `min(rest, n-1-i)`, decoded in O(n^2)) is checked against the same enumeration. Random n < 70 (quick 100, full 3000, stress 30,000 cases) with k uniform, at descending-prefix ends ±1 and in {0, 1, max-1, max}: a permutation of [0,n) with k inversions by brute force and by a merge-sort counter, equal to the decoded greedy code (maximality beyond the enumeration). Large n: random n in [70, 3000] (quick 5, full 100, stress 300); n = 65537 with k = 2^31 - 2, 2^31 - 1 and 2^31 (its maximum exceeds 2^31); n = 1000 (quick), 100,000 (full) or 500,000 (stress) with k in {0, 1, max/2, max-1, max, random} and three prefix ends; each checked as a permutation, by the merge-sort count and by its Fenwick-computed Lehmer code against the greedy code. A Python greedy Lehmer decode, validated against `itertools.permutations` through n = 7, covers random n up to 300 with random, extreme and prefix-end k (quick 300, full 3000, stress 12,000 cases). |
| Preconditions | 4 checked-build assertion probes: `inversions-large` (n = 3, k = 4), `inversions-large-wide` (n = 65537, k = max + 1), `inversions-negative-k` (n = 3, k = -1), `inversions-negative-n` (n = -1, k = 0) |

A scratch copy of the header returning the reverse complement (the same inversion count, a wrong permutation) failed at once at n = 3, k = 1.

## Commands and Results

Conditions (2026-11-17, P013 MI02 `/finish`, experiment: none): Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan-UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), beside other test runs.

| Date | Command | Result |
|---|---|---|
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/060-permutation_problems_tester.py' --mode quick` | PASS, 2 configurations, 300 Python cases; run optimized 0.06 s, checked 1.33 s; `MEMORY peak=700MB` |
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/060-permutation_problems_tester.py' --mode full --seed 1` | PASS, 3 configurations, 3000 Python cases, 4 probes; run optimized 0.28 s, checked 1.53 s, ASan-UBSan 0.86 s; `MEMORY peak=813MB` |
| 2026-11-17 | the same with `CXX=g++-14` | PASS, the same counts; run optimized 0.25 s, checked 1.28 s, ASan-UBSan 0.88 s; `MEMORY peak=868MB` |
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/060-permutation_problems_tester.py' --mode stress --seed 2` | PASS, 12,000 Python cases, 4 probes; run optimized 0.74 s, checked 4.77 s, ASan-UBSan 2.72 s; `MEMORY peak=400MB` |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' ops --filter 06-Miscellaneous/060-permutation_problems.hpp` | PASS, 1 operation in 1 extraction on g++ and g++-14, 0 failures |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' testers --filter 060-permutation_problems_tester` | PASS, quick run on the extracted build (1 name) |
| 2026-11-17 | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS on the final headers: 192 standalone and aggregate headers (the new `059-permutation_orders.hpp` and `060-permutation_problems.hpp` included), scalar and available AVX2 multi-TU builds, workspace, sanitizer self-tests; `MEMORY peak=2703MB` |
| 2026-11-17 | `python3 '96-Local Testing/04-mutation.py' run 06-Miscellaneous/060-permutation_problems.hpp` | Record in Mutation Testing |

## Mutation Testing

Record: mewt 4.0.0 on blob `807abcd7db1b0863fe61d62b85844ca281e2fee3` (2026-11-17, seed 1, whole file): 91 mutants, 84 caught (0 by timeout), 7 survived, 0 untested.

This first record tested every generated mutant; stage seconds: quick 43, full on the optimized build 8, full on the checked and ASan-UBSan builds 61. The 7 survivors are equivalent:

| Mutant | Line | Why it survives |
|---|---|---|
| `AOS 34`, `AOS 35`, `COS 76` | 8 | The loop condition `k >= m - 1` becomes `k >= m` (`m * 1`, `m / 1`, `k > m - 1`), which differs only at `k == m - 1`. There the original places `m - 1`, leaves k = 0 and appends the rest ascending; the mutant leaves the loop and places `k = m - 1` and the rest ascending, the same permutation. |
| `COS 68`, `COS 78`, `COS 83` | 8, 9, 10 | `!=` for `>` or `<`: `m` starts at n >= 0 and falls by one to at least 0, and `v` rises by one from 0 to `m`. |
| `CR 11` | 6 | Dropping `p.reserve(n)` changes capacity only. |

## Sources

Read 2026-11-17 and implemented locally; no code copied.

- [CSES 2214, Inverse Inversions](https://cses.fi/problemset/task/2214): any permutation of `1..n` with exactly k inversions, n up to 10^6, k up to n(n-1)/2; listed in [00-sources.md](00-sources.md) under the P013 MI02 `/finish` sweep.

## Limits and Handoffs

- Only the lexicographically largest answer is returned. The smallest is the largest answer for k' = n(n-1)/2 - k, reversed with values complemented, a one-line caller composition.
- Counting permutations with k inversions (the Mahonian numbers, CSES 2229) is handed to P054.

## Aliases

Search aliases (grab --find): Inverse Inversions, permutation with k inversions, construct permutation inversions, 逆序对构造, Lehmer code greedy.

## History

- 2026-11-17: P013 MI02 `/finish` moved `permutationWithInversions` here from row 07 (user decision: a standalone one-problem algorithm is Esoteric) and moved and strengthened its tests (maximality at every size, the n = 65537 wide-k cases, the `inversions-large-wide` probe); full passed on g++ and g++-14, stress on g++; first whole-file mutation campaign.
