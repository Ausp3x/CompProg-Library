# 004-sparsetable.hpp — Evidence

Package P006 (DS01, then DS02). The contracts common to rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SparseTable

An immutable sequence with `0 <= n <= INT_MAX`; empty construction is valid. `M` supplies `S` and an associative `op` under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract) (kept as `m`, passed as `SparseTable(vector<S> a = {}, M mon = {})`). `e` is never called, so a semigroup works and `S` need not be default-constructible.

- `prod(l, r)` is O(1) on a nonempty `[l, r)` and also requires idempotence. Noncommutative idempotent operations (rectangular bands) are valid, because the two overlapping blocks are combined in order.
- `associativeProd(l, r)` is O(log(n + 1)), needs only associativity, and combines disjoint blocks left to right.
- `maxRight(l, pred)` (`0 <= l <= n`) returns the largest `r` in `[l, n]` with `r == l` or `pred(associativeProd(l, r))`; `minLeft(r, pred)` (`0 <= r <= n`) returns the smallest `l` in `[0, r]` with `l == r` or `pred(associativeProd(l, r))`. Both need only associativity. `pred` must be deterministic and stay false once a growing range fails it. It is never called on an empty range, so no identity is needed, and it is called at most `bit_width(n)` times, each time on a nonempty range starting at `l` (ending at `r`).
- `operator<<` prints the legacy diagnostic format with inclusive block bounds in O(n * log(n + 1)).
- The public `v` holds only valid blocks: level `i` has `n - 2^i + 1` entries. The capacities of `v` and of its levels are unspecified, in both tables (user decision 2026-11-12).

### SparseTable2D

A rectangular `rows x cols` grid (fields `rows`, `cols`; `m` is the stored monoid) with `rows * cols <= INT_MAX`. Equal row lengths are asserted once at build, before the empty-grid return, and the size product is checked before narrowing. Grids with zero rows or zero columns are valid and admit no query. `M::op` must be associative, commutative and idempotent (min, max, gcd, and, or), because a 2D rectangle has no canonical operand order and the four overlapping corner blocks repeat cells. `prod(x1, y1, x2, y2)` folds the nonempty half-open rectangle `[x1, x2) x [y1, y2)` in O(1) and asserts its bounds once. No identity or default-constructible `S` is needed. Level `(i, j)` stores the `2^i x 2^j` block folds for every valid top-left corner, row-major with width `cols - 2^j + 1`; total storage is at most `rows * cols * bit_width(rows) * bit_width(cols)` values. The input grid is copied.

### Correctness and Cost

Dyadic blocks preserve input order. The overlap duplicates a contiguous aggregate B, so associativity and B·B = B suffice, even without commutativity. `associativeProd` partitions the range into disjoint blocks in order. Build and storage are O(n log(n+1)), `prod` O(1), `associativeProd` O(log(n+1)). Only valid blocks are stored.

`maxRight` finds the largest length `L <= n - l` with `g(L)`, where `g(L) = L == 0 or pred(associativeProd(l, l + L))` is true on a prefix of lengths. It tries `2^i` for `i` from `bit_width(n) - 1` down, with `acc` the product of the accepted blocks; a step to `L + 2^i` costs one `op` with block `v[i][l + L]`. A failed `2^i` leaves less than `2^i` to gain, so the greedy result is the binary expansion of the largest true length. `minLeft` mirrors it with `v[i][l - 2^i]` on the left of `acc`. Each costs O(log(n + 1)) `op` and `pred` calls. maspypy binary-searches the endpoint over the idempotent `prod` and asserts `check(e)`; the descent needs neither.

SparseTable2D: level `(i, j)` folds the `2^i x 2^j` block at each corner. It is built from `(i, j - 1)` by joining two horizontally adjacent halves, or for `j = 0` from `(i - 1, 0)` by joining two vertical halves, so each entry costs one `op` call. For `2^i <= x2 - x1 < 2^(i + 1)` the row intervals `[x1, x1 + 2^i)` and `[x2 - 2^i, x2)` cover `[x1, x2)`, and the same holds for columns, so the four corner blocks cover the rectangle exactly, with overlaps that idempotence absorbs. Commutativity and associativity make the combination order irrelevant. Build and storage are `sum over (i, j) of (rows - 2^i + 1) * (cols - 2^j + 1)`, which is O(rows * cols * log(rows + 1) * log(cols + 1)); a query is O(1).

### Legacy Mapping

| Legacy | Replacement |
|---|---|
| `queryFast(l, r)` over inclusive `[l, r]` (removed by the 2026-10-13 prune) | `prod(l, r + 1)` |
| `querySlow(l, r)` over inclusive `[l, r]` (removed by the 2026-10-13 prune) | `associativeProd(l, r + 1)` |
| `query(l, r)`, `fold(l, r)` (renamed 2026-10-17) | `prod(l, r)`, `associativeProd(l, r)`; the callable argument is the monoid |
| Flat padded public `v` | per-level `v` holding only valid blocks; read through `prod` and `associativeProd` |

## Feature-to-Test Map

The Python entry and the C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; the runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`004-sparsetable_tester.cpp` group) | Oracle |
|---|---|---|
| `prod`, `associativeProd`, `operator<<` | `exhaustive` | Ternary arrays through length 7, all min/max/sum folds and exact diagnostic output |
| Random and sizes | `randomized` | 80 random arrays and powers of two ±1 through 129 |
| Operand order, payloads, runtime state | `noncommutative` | Ordered concatenation and direct affine evaluation, noncommutative idempotent rectangular bands, nondefault elements, ownership, copy and move; a stateful `ArgminByKey{key}` (index of the leftmost smallest key in a runtime array) checked by `prod` and `associativeProd` on every range of 10 (quick) to 250 (stress) arrays; the semigroup structs have no `e` |
| `maxRight`, `minLeft` | `exhaustive`, `randomized`, `noncommutative`, `searchDiscipline` | Every `l` and `r` of every ternary array through length 7 with `sum <= cap` and `sum < cap` for every cap on the nonnegative shift, `min >= t`, `max <= t`, always-false and always-true, against a direct outward scan with `op`; random arrays and powers of two ±1 through 129; string prefix and suffix targets, a range-tagged affine `Chain` that rejects nonadjacent operands and is not default-constructible, `Ends`, stateful `ArgminByKey`; a counting `pred` asserting at most `bit_width(n)` calls, each on a recorded nonempty fold from `l` (to `r`); 5 search probes |
| Cost and size | `opCount` | `associativeProd` on every range of `n` in {0, 1, 2, 3, 7, 8, 9, 63, 64, 65, 300} (80 quick) with an `op`-counting sum: the value and exactly `popcount(r - l) - 1` calls; every table holds `h` levels; `n = 2^16 + 3` against a direct minimum on 200 random ranges (20 quick); probe `oversize` (`2^31` `bool` values) |
| `SparseTable2D::prod` | `grids` | Every ternary grid with n * m <= 8 (all shapes, including 0 x m and n x 0) and every rectangle for min/max/gcd against incremental brute force; 40 random grids up to 20 x 20 with input ownership; shapes 1x33, 33x1, 8x9, 16x16, 17x15, 2x64; `lng` extremes; nondefault payload and the stateful key-array monoid; 10 `grid-*` probes, including `grid-ragged-empty-first` (a ragged grid whose first row is empty), `grid-reversed-rows`, `grid-reversed-columns` and `grid-oversize` (2 rows of `2^30` `bool` values, past `rows * cols <= INT_MAX`) |

There are 25 assertion probes in total. Quick mode uses length 4, 2D n * m <= 4 and 10 search rounds; stress uses length 9, 2D n * m <= 9, 150 grids and 300 search rounds.

## Commands and Results

Machine: Linux 7.2 x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.8; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 862 MB | 0.3 + 4.2 | 23.3 + 4.8 | 25.5 + 48.2 | 1067 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 935 MB | 0.2 + 3.6 | 21.9 + 4.1 | 23.1 + 18.5 | 1044 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 25.8 + 0.3 | 22.7 + 1.9 | | 864 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 871 MB | 0.1 + 4.4 | 9.7 + 5.5 | 9.7 + 59.3 | 564 MB |

Package runs of 2026-11-12 (DS01; experiment `none`):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/004-sparsetable_tester.py' --mode full --seed 20261111` (GCC 16.2.1) | PASS, 3 configurations, 25 assertion probes, 12,500,736 checks per configuration, MEMORY peak 1242 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/004-sparsetable_tester.py' --mode full --seed 20261111` (GCC 14.4.1, floor) | PASS, 3 configurations, 25 assertion probes, MEMORY peak 1248 MB |
| `python3 '96-Local Testing/02-Data Structures/004-sparsetable_tester.py' --mode stress --seed 7` | PASS, 3 configurations, 88,311,413 checks per configuration, MEMORY peak 785 MB |
| `python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/004-sparsetable.hpp'`; `python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run passes on the extracted build |
| `python3 '96-Local Testing/01-run.py' --mode quick` (after the death-check change in `_00_contest_runner.py`) | PASS, 187 suites |
| `python3 '96-Local Testing/02-integration.py'` (after the descent loops changed) | PASS: 190 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace; MEMORY peak 2817 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| [maspypy sparse_table](https://raw.githubusercontent.com/maspypy/library/main/ds/sparse_table/sparse_table.hpp) | Fetched 2026-11-11: `max_right`/`min_left` by binary search over the idempotent `prod` with `check(e)`; this header descends the levels instead, needing neither idempotence nor `e`. |
| [cp-algorithms: Sparse Table](https://cp-algorithms.com/data_structures/sparse-table.html) | Dyadic preprocessing, disjoint logarithmic folds, overlapping idempotent O(1) query, alternative static RMQ structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | RMQ reference. |

References inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun are not claims that the original proofs were inspected.

## Mutation Testing

Record: mewt 4.0.0 on blob `b14ffc342dfc0f69940fe43d1e632296c839901e` (2026-11-12, seed 1, whole file, uniform sample of 630): 500 mutants, 472 caught (5 by timeout), 28 survived, 0 untested.

Stage seconds: quick 846, full on the optimized build 91, full on the checked and ASan-UBSan builds 1235. Two earlier campaigns of 2026-11-12 on the previous blob closed the test gaps behind the new probes and `opCount` checks. The descents of `maxRight` and `minLeft` became `for (int i = h; i-- > 0;)` by user decision (2026-11-12), because a start of `h - 1` mutated to `h + 1` or `h` survived, being skipped for every n < 2^30. Of the 28 survivors, 26 are equivalent and 2 untestable:

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 623`–`628` `std::move(...)` removed | 14, 18, 24, 48, 59, 85 | equivalent: a copy instead of a move, same values |
| `CR 52`, `CR 86` (`reserve` removed); `AOS 123`–`125`, `128`, `129` (`n - len + 1` → `(n - len) * 1`, `/ 1`, `% 1`, `n / len + 1`, `n % len + 1`); `AOS 218`, `220` (`rows * cols` → `+`, `/`); `AOS 286`, `288`, `289` (`h * w` → `+`, `/`, `%`) | 18, 21, 92, 99 | equivalent under the contract (the capacity of `v` is unspecified, user decision 2026-11-12): only capacities change, each ending between the size and about twice it; `push_back` grows the vectors and no pointer into them is held during the build |
| `COS 462` `l < r` → `!=` | 35 | equivalent: each block has length at most `r - l`, so `l` reaches `r` exactly |
| `AOS 177` `r - l` → `r % l` | 36 | equivalent: after the first block of length `2^i`, `l >= 2^i > r - l`, so `l <= r < 2 * l` and `r % l == r - l` |
| `COS 477`, `COS 497` `i-- > 0` → `!=` | 45, 56 | equivalent: `i` starts at `h >= 0` and steps down by 1 |
| `COS 517`, `537`, `552` `<` → `!=` | 71, 94, 101 | equivalent: counters step by 1 from at most their bound (`j <= n - len`, `i` from 0 to `hr >= 1`, `y` from 0 to `w >= 1`) |
| `AS 121` swapped bottom corners | 108 | equivalent: the contract requires a commutative `op` |
| `COS 523`, `COS 528` `<=` → `<` | 86 | untestable (user decision 2026-11-12, as for level ancestor on 2026-11-09): `COS 523` differs only at `rows == INT_MAX` (about 48 GiB of row vectors). `COS 528` rejects a valid grid with `cols == INT_MAX / rows`, whose levels need about 7.5 GiB even with `bool` (rows = 1, cols = INT_MAX); on smaller invalid input (`{vector<bool>(INT_MAX / 2), {}}`, 263 MB) it only moves the abort from line 88 to line 86, which a death probe accepting any glibc assert cannot tell apart. The assert stays. |

## Limits and Handoffs

The printed diagnostic format remains available. Disjoint sparse tables (row 28), Cartesian trees (row 27) and linear-preprocessing RMQ (row 54) are separate rows, not gaps of this row. No claim is made that ordinary sparse-table preprocessing is the theoretical minimum for RMQ.

## Aliases

Search aliases (P263, grab --find): ST table (OI Wiki: ST 表), RMQ (range minimum query); Library Checker `staticrmq`; 倍增, 稀疏表, 静态区间最值, 区间最值查询, 二维ST表, 可重复贡献.

## History

- 2026-09-27: original P006 verification; full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit: findings 5, 10–12, 14, 17, 19, 20, 23 and 24 (of 24 package findings) addressed in this header; full and stress passed on g++. The independent review confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1 and raised five findings, all fixed (here `SparseTable2D` ragged rows with an empty first row).
- 2026-10-13: prune removed `queryFast` and `querySlow` (cases moved to `query` and `fold`); full passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `query`/`fold` renamed `prod`/`associativeProd`, stateful-monoid tests); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 540/548/573 MB.
- 2026-11-07: P263 added the search aliases, including OI Wiki's page titles and common Chinese names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-12: P006 DS01: `maxRight` and `minLeft` added (descent over the levels, no identity); mutation closed the test gaps (probes `grid-reversed-rows`, `grid-reversed-columns`, `oversize`, `grid-oversize`, the exact `op` count, the n = 0 level count, `n = 2^16 + 3`) and the descents became `for (int i = h; i-- > 0;)` (user decision); full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (26 equivalent, 2 untestable survivors).
- 2026-11-12: DS01 `/finish`: the capacity of `v` declared unspecified and `COS 528` classified untestable (user decisions), the maspypy source row added; header and tests unchanged.
