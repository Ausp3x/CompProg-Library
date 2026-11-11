# 04-sparsetable.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SparseTable

Immutable sequence, `0 <= n <= INT_MAX`; empty construction is valid. `M` supplies `S` and an associative `op` under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract) (kept as `m`, passed as `SparseTable(vector<S> a = {}, M mon = {})`); `e` is never called, so a semigroup works, and no default-constructible `S` is needed. `prod(l, r)` is O(1) on a nonempty `[l, r)` and also requires idempotence; noncommutative idempotent operations (rectangular bands) are valid because the two overlapping blocks are combined in order. `associativeProd(l, r)` is O(log(n + 1)), needs only associativity, and combines disjoint blocks left to right. `operator<<` prints the legacy diagnostic format with inclusive block bounds in O(n * log(n + 1)). Public `v` holds only valid blocks: level `i` has `n - 2^i + 1` entries. `maxRight(l, pred)` (`0 <= l <= n`) returns the largest `r` in `[l, n]` with `r == l` or `pred(associativeProd(l, r))`, and `minLeft(r, pred)` (`0 <= r <= n`) the smallest `l` in `[0, r]` with `l == r` or `pred(associativeProd(l, r))`. pred must be deterministic and stay false once a growing range fails it; it is never called on an empty range, so no identity is needed, and it is called at most `bit_width(n)` times, each time on a nonempty range starting at `l` (ending at `r`). Both need only associativity, not idempotence. The capacities of `v` and of its levels are unspecified, in both tables (user decision 2026-11-12).

### SparseTable2D

Rectangular `rows x cols` grid (fields `rows`, `cols`; `m` is the stored monoid) with `rows * cols <= INT_MAX`; rows of equal length are asserted once at build, before the empty-grid return (the size product is checked before narrowing). Grids with zero rows or zero columns are valid and admit no query. `M::op` must be associative, commutative and idempotent (min, max, gcd, and, or). A 2D rectangle has no canonical operand order, and the four overlapping corner blocks repeat cells. `prod(x1, y1, x2, y2)` folds the nonempty half-open rectangle `[x1, x2) x [y1, y2)` in O(1) and asserts its bounds once. No identity or default-constructible `S` is needed. Level `(i, j)` stores the `2^i x 2^j` block folds for every valid top-left corner, row-major with width `cols - 2^j + 1`. Total storage is at most `rows * cols * bit_width(rows) * bit_width(cols)` values. The input grid is copied.

### Correctness and cost

Dyadic blocks preserve input order. Overlap duplicates a contiguous aggregate B, so associativity and B·B=B suffice, even without commutativity. `associativeProd` partitions into disjoint blocks in order. `maxRight` finds the largest length `L <= n - l` with `g(L)`, where `g(L) = L == 0 or pred(associativeProd(l, l + L))` is true on a prefix of lengths: trying `2^i` for `i` from `bit_width(n) - 1` down, with `acc` the product of the accepted blocks, a step to `L + 2^i` costs one `op` with block `v[i][l + L]`, and since a failed `2^i` leaves less than `2^i` to gain the greedy result is the binary expansion of the largest true length. `minLeft` mirrors it with `v[i][l - 2^i]` on the left of `acc`. Each costs O(log(n + 1)) `op` and pred calls. maspypy binary-searches the endpoint over the idempotent `prod` and asserts `check(e)`; the descent needs neither. Build/storage O(n log(n+1)), `prod` O(1), `associativeProd` O(log(n+1)). Only valid blocks are stored.

SparseTable2D: level `(i, j)` folds the `2^i x 2^j` block at each corner. It is built from `(i, j - 1)` by joining two horizontally adjacent halves, or for `j = 0` from `(i - 1, 0)` by joining two vertical halves, so each entry costs one `op` call. For `2^i <= x2 - x1 < 2^(i + 1)` the row intervals `[x1, x1 + 2^i)` and `[x2 - 2^i, x2)` cover `[x1, x2)`, and the same holds for columns. The four corner blocks therefore cover the rectangle exactly, with overlaps that idempotence absorbs. Commutativity and associativity make the combination order irrelevant. Build and storage are `sum over (i, j) of (rows - 2^i + 1) * (cols - 2^j + 1)`, which is O(rows * cols * log(rows + 1) * log(cols + 1)); a query is O(1).

### Legacy mapping

| Legacy | Replacement |
|---|---|
| `queryFast(l, r)` over inclusive `[l, r]` (removed by the 2026-10-13 prune) | `prod(l, r + 1)` |
| `querySlow(l, r)` over inclusive `[l, r]` (removed by the 2026-10-13 prune) | `associativeProd(l, r + 1)` |
| `query(l, r)`, `fold(l, r)` (renamed 2026-10-17) | `prod(l, r)`, `associativeProd(l, r)`; the callable argument is the monoid |
| Flat padded public `v` | per-level `v` holding only valid blocks; read through `prod` and `associativeProd` |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`04-sparsetable_tester.cpp` group) | Oracle |
|---|---|---|
| `prod`, `associativeProd`, `operator<<` | `exhaustive` | Ternary arrays through length 7, all min/max/sum folds and exact diagnostic output |
| Random and sizes | `randomized` | 80 random arrays and powers of two ±1 through 129 |
| Operand order, payloads, runtime state | `noncommutative` | Ordered concatenation/direct affine evaluation, noncommutative idempotent rectangular bands, nondefault elements, ownership/copy/move; a stateful `ArgminByKey{key}` (index of the leftmost smallest key in a runtime array) checked by `prod` and `associativeProd` on every range of 10 (quick) to 250 (stress) arrays; the semigroup structs have no `e` |
| `maxRight`, `minLeft` | `exhaustive`, `randomized`, `noncommutative`, `searchDiscipline` | Every `l` and `r` of every ternary array through length 7 with `sum <= cap` and `sum < cap` for every cap on the nonnegative shift, `min >= t`, `max <= t`, always-false and always-true, against a direct outward scan with `op`; random arrays and powers of two ±1 through 129; string prefix/suffix targets, a range-tagged affine `Chain` that rejects nonadjacent operands and is not default-constructible, `Ends`, stateful `ArgminByKey`; a counting pred asserting at most `bit_width(n)` calls, each on a recorded nonempty fold from `l` (to `r`); 5 search probes |
| Cost and size | `opCount` | `associativeProd` on every range of `n` in {0, 1, 2, 3, 7, 8, 9, 63, 64, 65, 300} (80 quick) with an `op`-counting sum: the value and exactly `popcount(r - l) - 1` calls; every table holds `h` levels; `n = 2^16 + 3` against a direct minimum on 200 random ranges (20 quick); probe `oversize` (`2^31` `bool` values) |
| `SparseTable2D::prod` | `grids` | Every ternary grid with n * m <= 8 (all shapes including 0 x m and n x 0), every rectangle for min/max/gcd against incremental brute force; 40 random grids up to 20 x 20 with input ownership; shapes 1x33, 33x1, 8x9, 16x16, 17x15, 2x64; `lng` extremes; nondefault payload and the stateful key-array monoid; 10 `grid-*` probes, including `grid-ragged-empty-first` (a ragged grid whose first row is empty), `grid-reversed-rows`, `grid-reversed-columns` and `grid-oversize` (2 rows of `2^30` `bool` values, past `rows * cols <= INT_MAX`) |

25 assertion probes in total. Quick: length 4, 2D n * m <= 4, 10 search rounds. Stress: length 9, 2D n * m <= 9, 150 grids, 300 search rounds.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 862 MB; optimized 0.3 s compile + 4.2 s run, checked 23.3 s compile + 4.8 s run, ASan-UBSan 25.5 s compile + 48.2 s run; MEMORY peak 1067 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 935 MB; optimized 0.2 s compile + 3.6 s run, checked 21.9 s compile + 4.1 s run, ASan-UBSan 23.1 s compile + 18.5 s run; MEMORY peak 1044 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 25.8 s compile + 0.3 s run, checked 22.7 s compile + 1.9 s run; MEMORY peak 864 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 871 MB; optimized 0.1 s compile + 4.4 s run, checked 9.7 s compile + 5.5 s run, ASan-UBSan 9.7 s compile + 59.3 s run; MEMORY peak 564 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (DS01; experiment `none`), Linux 7.2 x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `04-sparsetable_tester.py --mode full --seed 20261111` (GCC 16.2.1) | PASS, 3 configurations, 25 assertion probes, MEMORY peak 1242 MB, 12,500,736 checks per configuration |
| `CXX=g++-14 04-sparsetable_tester.py --mode full --seed 20261111` (GCC 14.4.1, floor) | PASS, 3 configurations, 25 assertion probes, MEMORY peak 1248 MB |
| `04-sparsetable_tester.py --mode stress --seed 7` | PASS, 3 configurations, 88,311,413 checks per configuration, MEMORY peak 785 MB |
| `05-extraction.py ops --filter '02-Data Structures/04-sparsetable.hpp'`; `05-extraction.py testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run on the extracted build passes |
| `01-run.py --mode quick` (after the death-check change in `_00_contest_runner.py`) | PASS, 187 suites |
| `02-integration.py` (after the descent loops changed) | PASS: 190 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 2817 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode full --seed 20261111
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode full --seed 20261111
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/04-sparsetable.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| [maspypy sparse_table](https://raw.githubusercontent.com/maspypy/library/main/ds/sparse_table/sparse_table.hpp) | Fetched 2026-11-11: `max_right`/`min_left` by binary search over the idempotent `prod` with `check(e)`; this header descends the levels instead, needing neither idempotence nor `e`. |
| [cp-algorithms: Sparse Table](https://cp-algorithms.com/data_structures/sparse-table.html) | Dyadic preprocessing, disjoint logarithmic folds, overlapping idempotent O(1) query, alternative static RMQ structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | RMQ reference. |

References inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun are not claims that the original proofs were inspected.

## Mutation testing

Record: mewt 4.0.0 on blob `b14ffc342dfc0f69940fe43d1e632296c839901e` (2026-11-12, seed 1, whole file, uniform sample of 630): 500 mutants, 472 caught (5 by timeout), 28 survived, 0 untested.

Stage seconds of that campaign: quick 846, full on the optimized build 91, full on the checked and ASan-UBSan builds 1235. Two campaigns on the previous blob `87b1f78f5ff3` (49 survivors, then 45 and 36 after test rounds) closed these gaps before it: probes `grid-reversed-rows` and `grid-reversed-columns` (`x1 < x2`, `y1 < y2` → `!=` on line 104), `oversize` (1D size assert removed or `<=` → `!=`) and `grid-oversize` (the 2D product guard removed, `/` → `+`, `-`, `*`, `&&` → `||`, `<=` → `!=`); the `op` count of `opCount`, first `< bit_width(r - l)` and then exact, for steps of one element (`- 1` → `% 1` on lines 33 and 36, `r - l` → `r / l`); the level count for the n = 0 early return; `n = 2^16 + 3` for `n - len + 1` → `n * len + 1`, whose reserve overflows. The descents of `maxRight` and `minLeft` started at `h - 1`, and `h + 1` or `h` survived, being skipped for every n < 2^30; on the user's decision (2026-11-12) both loops became `for (int i = h; i-- > 0;)`, which has no start expression to mutate. Of the 28 left, 26 are equivalent and 2 untestable:

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 623`–`628` `std::move(...)` removed | 14, 18, 24, 48, 59, 85 | equivalent: a copy instead of a move, same values |
| `CR 52`, `CR 86` (`reserve` removed); `AOS 123`–`125`, `128`, `129` (`n - len + 1` → `(n - len) * 1`, `/ 1`, `% 1`, `n / len + 1`, `n % len + 1`); `AOS 218`, `220` (`rows * cols` → `+`, `/`); `AOS 286`, `288`, `289` (`h * w` → `+`, `/`, `%`) | 18, 21, 92, 99 | equivalent under the contract (the capacity of `v` is unspecified, user decision 2026-11-12): only capacities change, each ending between the size and about twice it; `push_back` grows the vectors and no pointer into them is held during the build |
| `COS 462` `l < r` → `!=` | 35 | equivalent: each block has length at most `r - l`, so `l` reaches `r` exactly |
| `AOS 177` `r - l` → `r % l` | 36 | equivalent: after the first block of length `2^i`, `l >= 2^i > r - l`, so `l <= r < 2 * l` and `r % l == r - l` |
| `COS 477`, `COS 497` `i-- > 0` → `!=` | 45, 56 | equivalent: `i` starts at `h >= 0` and steps down by 1 |
| `COS 517`, `537`, `552` `<` → `!=` | 71, 94, 101 | equivalent: counters step by 1 from at most their bound (`j <= n - len`, `i` from 0 to `hr >= 1`, `y` from 0 to `w >= 1`) |
| `AS 121` swapped bottom corners | 108 | equivalent: the contract requires a commutative `op` |
| `COS 523`, `COS 528` `<=` → `<` | 86 | untestable (user decision 2026-11-12, as level ancestor's 2026-11-09): `COS 523` differs only at `rows == INT_MAX` (about 48 GiB of row vectors); `COS 528` rejects a valid grid with `cols == INT_MAX / rows`, whose levels need about 7.5 GiB even with `bool` (rows = 1, cols = INT_MAX), and on smaller invalid input (`{vector<bool>(INT_MAX / 2), {}}`, 263 MB) only moves the abort from line 88 to line 86, which a death probe that accepts any glibc assert cannot tell apart; the assert stays |

## Limits and handoffs

The printed diagnostic format remains available. Disjoint sparse tables (row 28), Cartesian trees (row 27) and linear-preprocessing RMQ (row 54) are separate rows, not gaps of this row; no claim is made that ordinary sparse-table preprocessing is the theoretical minimum for RMQ.

## Aliases

Search aliases (P263, grab --find): ST table (OI Wiki: ST 表), RMQ (range minimum query); Library Checker `staticrmq`; 倍增, 稀疏表, 静态区间最值, 区间最值查询, 二维ST表, 可重复贡献.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 5, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed (here `SparseTable2D` ragged rows with an empty first row).
- 2026-10-13: prune, removed `queryFast` and `querySlow` (cases moved to `query` and `fold`); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `query`/`fold` renamed `prod`/`associativeProd`, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 540/548/573 MB.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS01: `maxRight` and `minLeft` added (descent over the levels, no identity); mutation closed the test gaps above and the descents became `for (int i = h; i-- > 0;)` (user decision); full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (26 equivalent, 2 untestable survivors).
- 2026-11-12: DS01 `/finish`: the capacity of `v` declared unspecified and `COS 528` classified untestable (user decisions), the maspypy source row added; header and tests unchanged.
