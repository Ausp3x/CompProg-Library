# 05-prefix_sum.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### PrefixSum and PrefixSum2D

Zero-based half-open ranges and rectangles `[x1, x2) x [y1, y2)` in (row, column) order; 2D arguments are `(row1, col1, row2, col2)`. Each dimension is in `[0, INT_MAX - 1]`. T is an additive commutative group with zero `T(0)`. Every stored prefix and intermediate must fit T, or T is modular. Input vectors are copied. An empty outer vector means 0 x 0; the dimension constructor also represents 0 x m and n x 0. Rows must have equal length (asserted). The 1D types take a braced list (`std::initializer_list<T>`; [00-notes.md](00-notes.md#common-foundation-contracts)).

### DifferenceArray and DifferenceArray2D

Offline range or rectangle additions over the same domains; `add` on an empty range or rectangle is a no-op. `values()` reconstructs the array in O(n) or O(n * m) without changing the state. `clear` resets to zeros in place. Unused far-border subtractions are skipped, so a whole-array (singleton) update by `LLONG_MIN` never computes its unrepresentable negation. Intermediate sums and differences must still fit T. The 2D update delta is named `w` so that it does not clash with the coordinates `x1` and `x2`.

### DifferenceArray::addArithmetic

`addArithmetic(l, r, x, step)` adds `x + (i - l) * step` at each `i` in `[l, r)`, with `0 <= l <= r <= n` asserted and an empty range a no-op. It needs `T(int)` and `*` beside the group operations, so T is a commutative ring here. The first call after construction or `clear` allocates the slope-difference array `s` (O(n)); later calls are O(1). The value change is a first difference of `+x` at `l`, `+step` on `[l + 1, r)` and `-(x + (r - 1 - l) * step)` at `r`; the middle run is itself a range add on the first differences, kept in `s`. `values()` folds the running slope into the first differences, then integrates, still O(n). Updates at `r == n` are skipped, as in `add`. `clear` zeroes `d` in place and empties `s` (its capacity stays). Every value, difference and running slope must fit T, and so must the product `(r - 1 - l) * step` on its own (n = 4, `addArithmetic(0, 3, -2^62, 2^62)` keeps every value in `lng` but overflows it).

### PrefixProduct

`PrefixProduct<G>(a = {}, mon = {})` over a group `G` (`S`, `op`, `e`, `inv`; [00-monoids.md](00-monoids.md#group-contract)), stored as `[[no_unique_address]] G m`; `op` need not commute. It keeps `p[i] = a[0] * ... * a[i - 1]` and `prod(l, r)` returns `inv(p[l]) * p[r] = a[l] * ... * a[r - 1]` in order, `e()` for an empty range, with `0 <= l <= r <= n` asserted. The input is copied. Build O(n + 1) operations, query two operations. It has no size constructor (a product of identities has no use), so it takes no braced-list constructor either.

### PrefixSum3D and DifferenceArray3D

Boxes `[x1, x2) x [y1, y2) x [z1, z2)`, arguments `(x1, y1, z1, x2, y2, z2)`. Storage is flat in row-major `(x, y, z)` order. `PrefixSum3D` needs `(n + 1) * (m + 1) * (k + 1) <= INT_MAX` and `DifferenceArray3D` needs `n * max(m, 1) * max(k, 1) <= INT_MAX` (both asserted), so every flat index fits `int`; with no empty axis every per-axis loop bound such as `m * k` fits too, and with an empty axis `integrate3D` (behind `values()`) returns before forming one, while the shapes the vector constructor passes to `differentiate3D` are those of a real nested vector, so `m * k` fits there (`DifferenceArray3D<char>(0, 50000, 50000)` is valid). `values()` builds each plane's rows only inside its plane loop, so no planes cost nothing however large m is. `DifferenceArray3D::values()` returns n planes of m rows, so it costs O(n * m * k + n * m + n). The vector constructors take an n x m x k nested vector with equal plane and row lengths (asserted); an empty outer vector is 0 x 0 x 0, n planes of no rows are n x 0 x 0, and the dimension constructors also represent the other empty shapes. `PrefixSum3D` copies the cells into the zero-bordered array and runs one prefix pass per axis; `sum` takes two z-differences per column and combines four columns, so each intermediate is a box sum. `DifferenceArray3D` updates the eight corners with inclusion-exclusion signs, skipping corners on the far border; the vector constructor takes backward differences per axis, and `values()` integrates a copy per axis and returns a nested vector. `clear` zeroes in place. T is as in the 2D types; every box sum and intermediate difference must fit T.

### StepSum

`StepSum<T>(a, step)` asserts `step >= 1` (any step, also above n; n may be 0) and keeps one `PrefixSum` per residue class present, `min(step, n)` chains holding n values in total. `sum(k, l, r)` asserts `0 <= k < step` and `0 <= l <= r <= n` and returns the sum of `a[i]` over `i` in `[l, r)` with `i % step == k`, `T(0)` for a class with no element. Chain k holds `a[k], a[k + step], ...`, and `before(x) = ceil((x - k) / step)` for `x > k` (0 otherwise) counts its elements below index x, so the answer is the chain's sum over `[before(l), before(r))`. T is as in `PrefixSum`. O(n + 1) setup and space, O(1) query.

### Correctness and cost

Prefix accumulation and inclusion-exclusion cancel exactly the unwanted intervals. Difference endpoint/four-corner updates cancel outside the added region; axis-by-axis integration is their inverse. 1D build/storage/materialization O(n+1), 2D O((n+1) * (m+1)), 3D O((n+1) * (m+1) * (k+1)); queries/updates O(1). Returned arrays use a further copy of the corresponding storage. `checkedSize` carries `// T: O(1), M: O(1)`; the detail helpers `integrate3D`, `differentiate3D` (per-axis passes, one the inverse of the other) and `shape3D` carry their own bounds.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `rebuild(a)` of `PrefixSum`, `PrefixSum2D`, `DifferenceArray`, `DifferenceArray2D` | `x = PrefixSum<T>(a)` and likewise for each type; the new object is built before the assignment, so `a` may be `x`'s own storage, with one more copy of the storage alive meanwhile |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`05-prefix_sum_tester.cpp` group) | Oracle |
|---|---|---|
| `PrefixSum`/`DifferenceArray`: `prefixSum`, `sum`, `add`, `values` | `arrays` | Ternary arrays through length 7, all range sums/updates and depth-3 histories |
| `PrefixSum2D`/`DifferenceArray2D` | `matrices` | Every ternary matrix through 2x3 and every rectangle |
| Rectangle update histories | `randomized` | 100 histories of 120 rectangle updates against literal cell loops |
| `clear`, construction, types | `arrays`, `matrices`, `randomized`, `boundaries` | Empty dimensions, clear/copy/move/repeated reconstruction (the removed `rebuild` cases: reconstruction after copy/move in `arrays` and `matrices`, periodic reconstruction in `randomized`, reconstruction from the object's own storage in `boundaries`), argument/input aliases, int64/128-bit/modular/exact dyadic cases, braced singleton/list and non-convertibility from `int`; 16 probes |

| `DifferenceArray::addArithmetic` (mixed with `add`, `clear`, `values`; lazy slope array) | `arithmeticHistories`, `arithmeticRandom`, `arithmeticBoundaries` | Literal loops `a[i] += x + (i - l) * step`; every history of length <= 6 and depth 3 (2.40M nodes), 100 random trials each over int64, 128-bit and `uint32_t` (n <= 40, 150 operations), far-border and extreme cases |
| `PrefixProduct::prod` | `products` | Left-to-right fold over every range: every sequence of 4-element permutations through length 3 (14,425), and 150 random trials over permutations, 2x2 matrices mod 7, a stateful 2x2 group mod 1e9+7, `Sum<lng>`, `Xor<ulng>`, `Sum<uint>`; inverse laws and noncommutativity checked on the oracle groups |
| `PrefixSum3D`: both constructors, `prefixSum`, `sum` | `boxes`, `randomBoxes`, `boxBoundaries` | Triple loops over every box of every shape with each dimension <= 3 (4,875 arrays, exhaustive ternary through 6 cells), 60 random trials up to 7 per dimension |
| `DifferenceArray3D`: both constructors, `add`, `values`, `clear` | `boxes`, `randomBoxes`, `boxBoundaries` | Literal cell loops; every history of non-empty boxes and `clear` through depth 3 (223,896 nodes), random histories of 80 operations; boxes empty in each axis on a cell holding `LLONG_MAX` leave it untouched (corner updates there would overflow, which the ASan-UBSan build traps) |
| New preconditions | probes | 28 probes: `arith-negative`, `arith-end`, `arith-reversed`, `product-negative`, `product-end`, `product-reversed`, `3d-negative-dimension`, `3d-oversize` (1290^3), `3d-difference-negative`, `3d-difference-oversize` (1291^3), `3d-ragged-plane`, `3d-ragged-row`, `3d-prefix`, `3d-range-reversed`, `3d-range-end`, `3d-add-reversed`, `3d-add-end`, `3d-difference-flat-oversize` (n * max(m, 1) * max(k, 1) with k = 0), `step-zero`, `step-class-end`, `step-class-negative`, `step-range-reversed`, `step-range-end`, `step-class-past-end` (k = step + 1, where the class lookup alone would return 0), `difference-negative-size`, and `difference-max-size`, `2d-difference-max-rows`, `2d-difference-max-columns` (`INT_MAX` packed bools, or `INT_MAX` empty rows); 44 probes in all. Full and stress mode also build `PrefixSum3D<bool>(INT_MAX - 1, 0, 0)`, the largest admitted cell count (`INT_MAX`, prime), 256 MiB |
| `StepSum`: ctor, `sum` | `stepSums` | Direct loop over `[l, r)` keeping `i % step == k`: every n <= 8 (quick 5), step 1..n + 3, every k and l <= r, on distinct powers of two (each index set has its own total) and random ternary values (126 arrays); 100 random trials (n <= 200, 200 queries); `PrefixSum::sum` at step 1; 128-bit and wrapping `uint32_t`, n = 0, step `INT_MAX`, k >= n, int64 extremes that cancel, copy, move and reconstruction |

Quick: length 4, 2x2, depth 2, 15 cases; arithmetic histories length 4 depth 2, products through length 2, 3D dimensions <= 2, 3D depth 2. Stress: length 8, history depth 4, 400 cases; products through length 4 (346,201), 600 product trials, 3D depth 4 (2x2x2 capped at depth 3; 2.13M nodes), 250 random 3D trials.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 788 MB; optimized 0.2 s compile + 1.0 s run, checked 18.0 s compile + 1.1 s run, ASan-UBSan 19.7 s compile + 7.4 s run; MEMORY peak 1011 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 852 MB; optimized 0.2 s compile + 1.2 s run, checked 21.4 s compile + 1.2 s run, ASan-UBSan 23.1 s compile + 6.7 s run; MEMORY peak 1053 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 29.4 s compile + 0.1 s run, checked 20.2 s compile + 0.5 s run; MEMORY peak 783 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 790 MB; optimized 0.1 s compile + 0.9 s run, checked 7.8 s compile + 1.1 s run, ASan-UBSan 8.0 s compile + 9.7 s run; MEMORY peak 664 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (P006 DS02; experiment: none): GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H, one suite at a time after the DS02 `/finish` mutation campaigns, beside other sessions, so times are indicative.

```bash
python3 '96-Local Testing/02-Data Structures/05-prefix_sum_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/05-prefix_sum_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/05-prefix_sum_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --mode full --filter '02-Data Structures/05-prefix_sum.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter 05-prefix_sum_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/05-prefix_sum.hpp' '96-Local Testing/02-Data Structures/05-prefix_sum_tester.cpp'
```

| Command | Result |
|---|---|
| full, g++ | PASS, 3 configurations, 8,032,365 checks per configuration, 44 probes; run optimized 3.2 s, checked 5.7 s, ASan-UBSan 43.1 s; MEMORY peak 1478 MB |
| full, g++-14 | PASS; run 3.4 / 5.6 / 31.1 s; MEMORY peak 1496 MB |
| stress, seed 7 | PASS, 25,563,652 checks; run 12.0 / 15.6 / 134.6 s (ASan-UBSan 19% of its 720 s limit); MEMORY peak 1117 MB |
| extraction ops | PASS: 21 operations in 21 extractions on g++ and g++-14, 0 failures |
| extraction testers | PASS: 11 names, quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multiple-translation-unit builds, workspace; MEMORY peak 1600 MB |
| `03-consistency.py` | No errors |
| `--braces` | No closing-brace, comment-cap or free-function violations |

Quick builds run in 0.1–0.2 s (optimized) and 0.9–1.9 s (checked).

## Sources

| Source | Actual reading and use |
|---|---|
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | pp84–85/93 independently support 1D/2D prefix inclusion-exclusion and endpoint differences. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SubMatrix reference. Its empty-input constructor is not copied; empty matrices require an explicit contract here. |
| [OI Wiki: 前缀和 & 差分](https://oi-wiki.org/basic/prefix-sum/) | Read 1D/2D prefix sums, four-corner difference signs and offline reconstruction; page update 2026-03-26. |
| [hitonanode offline_sum_of_linear](https://hitonanode.github.io/cplib-cpp/utilities/offline_sum_of_linear.hpp) | Arithmetic-progression range adds through a second-order difference: `DifferenceArray::addArithmetic`. |
| [maspypy Static_Range_Product_Group](https://maspypy.github.io/library/ds/static_range_product_group.hpp) | Group prefix products `inv(p[l]) * p[r]`: `PrefixProduct`. |
| [OI Wiki: 前缀和 & 差分](https://oi-wiki.org/basic/prefix-sum/), reread 2026-11-12 | Higher-dimensional prefix sums one axis at a time: `PrefixSum3D`, `DifferenceArray3D`. |
| [suisen step_sum](https://suisen-cp.github.io/cp-library-cpp/library/util/step_sum.hpp) | Prefix sums per residue class: `StepSum`. |

References inspected on 2026-09-27; the DS02 rows on 2026-11-12 ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)).

## Mutation testing

Record: mewt 4.0.0 on blob `460b31b05dfcae262d1bcaf282c8ebdccd401a04` (2026-11-12, seed 20261112, 2 lines changed since blob `44048da166f8`): 4 mutants, 4 caught (0 by timeout), 0 survived, 0 untested.

The DS02 `/finish` campaigns covered its fixes: `integrate3D`'s empty-axis return and the `DifferenceArray3D` complexity line on blob `44048da166f8` (9 mutants, 9 caught, stage seconds quick 27), then `values()` building each plane's rows inside its plane loop (quick 14), which the test `DifferenceArray3D<lng>(0, INT_MAX, 0).values()` needs (the old prototype of m rows was 48 GiB). A first pass on an intermediate blob left that return and the same return in `differentiate3D` alive (`CR 110`, `IF 215`, `CR 115`, `IF 216`); the test `DifferenceArray3D<lng>(0, 50000, 50000)` (no planes, `m * k > INT_MAX`) now needs the first, and the second was dead (its only caller passes a real nested vector's shape, so `m * k` fits and an empty axis leaves every loop empty) and is gone. It follows the whole-file campaign on blob `20688137b203` (2026-11-12, uniform sample of 500 of 1591 mutants, 479 caught, 21 survived), whose survivors on unchanged lines stay in the table. The probes `difference-max-size`, `2d-difference-max-rows`, `2d-difference-max-columns` and the full-mode build `PrefixSum3D<bool>(INT_MAX - 1, 0, 0)` closed that campaign's `COS 1104`, `COS 1199` and `COS 1329`, each checked against the mutant applied by hand to a scratch copy (both readings of `COS 1199`).

Whole-file campaign stage seconds: first run quick 1003, full on the optimized build 142, full on the checked and ASan-UBSan builds 1411 (25 survivors); the survivor retest with the same workdir quick 1064, 168, 2010. Between them the probe `difference-negative-size` closed the removed `DifferenceArray` size assert (line 86, where `assign(-1)` threw instead), `step-class-past-end` closed `k < step` → `!=` (line 246, where `k > step` returned 0), and boxes empty in one axis on a `LLONG_MAX` cell closed the removed early return of `DifferenceArray3D::add` and its `y1 == y2` → `>` (line 216): the eight corner updates cancel in value but overflow in between, which the ASan-UBSan build traps.

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 938`, `1173`, `1258`, `1263`, `1333`, `1448`, `1533` `<` → `!=` | 23, 112, 141, 142, 183, 209, 227 | equivalent: each counter steps by 1 from at most its bound, and `x2 <= n`, `y2 <= m` are asserted |
| `COS 978` `n < INT_MAX` → `!=` | 46 | equivalent: an `int` never exceeds `INT_MAX` |
| `COS 1156`, `1251`, `1256`, `1501`, `1511` `==` → `>=`; `COS 1521` `x == n` → `>=` | 100, 139, 216, 219 | equivalent: `l <= r`, `x1 <= x2`, `y1 <= y2`, `z1 <= z2` are asserted and corners never exceed the dimensions |
| `MR 1588` `std::move(mon)` removed; `AS 287` `max(k, 1)` swapped | 163, 203 | equivalent: a copy instead of a move; a symmetric function |
| `AOS 547`, `548` (`p.reserve(n + 1)` → `n * 1`, `n / 1`) | 164 | equivalent under the contract (the capacity of a public vector is unspecified, user decision 2026-11-12, `00-notes.md`): only the capacity changes |

## Limits and handoffs

Online range-update/query structures are rows 11 and 12; difference arrays reconstruct offline. Tree prefix sums and path differences belong to Graphs ([00-notes.md](00-notes.md#p006-re-audit-omissions)); the DS02 sweep left out the constructor, clamping and `build` forms ([00-notes.md](00-notes.md#ds02-sweep-2026-11-12)). Dimensions above three apply the per-axis pass once more.

## Aliases

Search aliases (P263, grab --find): cumulative sum, difference array (OI Wiki: 前缀和 & 差分); Library Checker `static_range_sum`, 二维前缀和, 二维差分, 差分数组, 子矩阵和, 静态区间和; 3D prefix sum, 三维前缀和, 三维差分, 高维前缀和, arithmetic progression add, 等差数列加, 二阶差分, prefix product, 前缀积, step sum, residue class sum, 等间隔求和.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 3, 4, 10–12, 13, 14, 15, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: prune, removed the four `rebuild` methods (cases moved to reconstruction by assignment); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS02, added `DifferenceArray::addArithmetic`, `PrefixProduct`, `PrefixSum3D`, `DifferenceArray3D` (sweep: hitonanode, maspypy, OI Wiki) and `StepSum` (suisen; adopted by user decision after the review), 25 probes; independent review: product bound, `DifferenceArray3D` flat bound and the `PrefixSum` keep row fixed or recorded; first mutation campaign, three gaps closed by tests; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-12: P006 DS02 `/finish`, `integrate3D` returns before forming `m * k` on an empty axis (`DifferenceArray3D` with no planes and wide planes overflowed), `values()` no longer builds an m-row prototype when there are no planes (the review's `(0, 50000000, 1)` took 1.2 GB), the `DifferenceArray3D` complexity line gained its shape terms; three max-size probes, the largest admitted `PrefixSum3D` and the wide-plane test closed three earlier survivors and two new ones; changed-line mutation campaign; full on g++ and g++-14, stress, extraction, integration and consistency passed.
