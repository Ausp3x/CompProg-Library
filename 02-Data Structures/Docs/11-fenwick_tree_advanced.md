# 11-fenwick_tree_advanced.hpp — Evidence

Package P027 (batch DS04). The header replaces the unchanged extractions `FenTreeRangeAdd1D` and `FenTree` (`OLD/algorithms.cpp:1481–1860`) with named structures; the Basic `Fenwick` (row 02) keeps point add and prefix sum. All indices are zero-based, every range is half-open, and every precondition is an `assert` checked once at the entry of an operation. A moved-from object may only be assigned to or destroyed.

## Contracts

### FenwickRangeAdd<T>

`T` is a commutative ring (`+`, `-`, `*`, `T(int)`), such as `lng`, `mint` or `double`. Two Fenwick arrays store the difference sequence `d` and `d * index`, so `prefixSum(r) = r * sum(d[0..r)) - sum(d[j] * j)`. The intermediate `r * sum(d)` equals `r * a[r - 1]` and `sum(d[j] * j)` is bounded by `2 * n * max|a_i|`, so for integral `T` the domain is `2 * n * max|a_i| < 2^63` over the whole history (modular `T` has no bound). `add(l, r, x)` adds `x` to `[l, r)`; `sum(l, r)`, `prefixSum(r)` and `get(i)` read. `update(i, x)` adds `x` to every `a[j]` with `j >= i` (one difference point); it asserts `0 <= i <= n`, and `i == n` is a no-op that `add(l, n, x)` relies on. Constructors: size (zeros) and `vector<T>` with an O(n) build from first differences.

### DualFenwick<T>

`T` is a commutative group. One Fenwick array over first differences gives `add(l, r, x)` in O(log(n)) and `get(i)` as a prefix of differences. `update(i, x)` adds `x` to every `a[j]` with `j >= i`; it asserts `0 <= i <= n`, and `i == n` is a no-op that `add(l, n, x)` relies on. There is no range sum (use `FenwickRangeAdd`). Size and vector constructors.

### FenwickRangeArithmeticAdd<T>

`addProgression(l, r, a, d)` adds `a + d * (i - l)` to `a[i]` for `i` in `[l, r)`; `add(l, r, x)` is the constant case. `update(i, a, b, c)` adds `a + b * r + c * r(r-1)/2` to `prefixSum(r)` for every `r > i`; it asserts `0 <= i <= n`, and `i == n` is a no-op that `addProgression(l, n, a, d)` relies on. Three coefficient arrays `(A, B, C)` make `prefixSum(r) = A(r) + B(r) * r + C(r) * r(r-1)/2`, where the triangular number is computed exactly in `lng` before conversion to `T`, so `mint` works. Integral domain: `2 * n^2 * max(|a_i|, |a|, |d|)` over the history fits `T` (the coefficient sums carry `n * a` and `n^2 * d / 2` terms). `sum`, `prefixSum`, `get`; size and vector constructors.

### Fenwick2D<T>, Fenwick3D<T>

Dense `n x m` (`n x m x h`) tables in flat storage with `n * m (* h) <= INT_MAX`; the 3D check tests `n * m` first so the product cannot overflow `lng`. `add(i, j[, k], x)` is a point add; `prefixSum(i, j[, k])` sums the half-open prefix box; `sum(i1, j1, [k1,] i2, j2, [k2])` sums `[i1, i2) x [j1, j2) [x [k1, k2))` by inclusion–exclusion and asserts ordered bounds; `get` is the unit box. The matrix constructors infer `m` (and `h`) from the first row (plane) and assert that every row has that size; an empty outer vector yields zero inner sizes, so use the size constructor for `0 x m`. The O(n * m) build runs one Fenwick pass per axis; the axis transforms are linear and commute, so each pass is applied exactly once (a fused build double-transforms carried-over contributions, which the tests reproduce as a mutant).

### Fenwick2DRangeAdd<T>

`add(i1, j1, i2, j2, x)` adds `x` on `[i1, i2) x [j1, j2)`; `sum` and `prefixSum` read rectangles. Four coefficient sums per cell (`d`, `d*i`, `d*j`, `d*i*j`) give `prefixSum(i, j) = i*j*D - j*Di - i*Dj + Dij`. `T` is a commutative ring; for integral `T`, `4 * n * m * max|a|` over the history fits `T` (the four coefficient sums carry index products up to `n * m`). Size constructor only (initial zeros).

### CompressedFenwick2D<T>

Construction takes the point universe `vector<pair<lng, lng>>` (duplicates allowed). `add(x, y, w)` asserts once at entry that `(x, y)` is a registered point; registration in the first column implies every ancestor column. `sum(x1, y1, x2, y2)` and `prefixSum(x, y)` accept any `lng` bounds. Each Fenwick node over the distinct `x` values holds the sorted distinct `y` values of the points it covers, built from the points sorted by `y`, so no per-node sort. Storage is `O(n log n)`, operations `O(log^2 n)`. `T` is a commutative group.

### FenwickPrefixMonoid<M>

`M` is a commutative monoid (`S`, `op`, `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract), kept as `m` and passed last as `mon = {}` (`FenwickPrefixMonoid(N = 0, mon = {})`, `FenwickPrefixMonoid(vector<S>, mon = {})`). `apply(i, x)` sets `a[i] = op(a[i], x)` (point combine, never overwrite); `prefix(r)` folds `[0, r)`. It suits max, min, gcd, bitwise or, saturating sums and products of units: anything commutative. There is no range query and no inverse.

### Fenwick01 (Alias FenwickSet)

A set of integers in `[0, n)`: one bit per element in 64-bit words plus a Fenwick array over word popcounts. `get`, `size`, `add(i, ±1)` (asserts the bit changes), and `insert`/`erase` returning whether the set changed. `rank(r)` counts elements below `r` for `r` in `[0, n]`; `sum(l, r)` counts `[l, r)`; `kth(k)` is the k-th smallest (0-based), or `n` when `k >= size()`; `next(i)` is the smallest element `>= i` or `n` (`i` in `[0, n]`); `prev(i)` is the largest element `<= i` or `-1` (`i` in `[-1, n)`). The in-word select is a 6-step popcount binary search, so `kth` is `O(log(n / 64) + log 64)`. The `vector<bool>` constructor builds in `O(n)` (bit packing) with its own complexity comment.

### Legacy Mapping

| Legacy | Replacement |
|---|---|
| `FenTreeRangeAdd1D` | `FenwickRangeAdd` |
| `FenTree` 1D with `f`/`f_inv` | Basic `Fenwick<T>` with a group `T`, or `FenwickPrefixMonoid` when only a prefix monoid is needed |
| `FenwickPrefixMonoid<T, F>` with `(id, f)` (P027 monoid pass, 2026-10-17) | `FenwickPrefixMonoid<M>` with one monoid |
| `FenTree` 2D/3D | `Fenwick2D`/`Fenwick3D`, including the linear builds |
| `FenTree::lowerBound` | `Fenwick::lowerBound` |
| `getIdx` | none: an internal flattening helper |
| `operator<<` node printers | not ported: the structures expose their arrays and the debug header prints vectors |
| `Fenwick01::contains(i)` (removed by the 2026-10-13 prune) | `get(i)` |

Rectangle add with point get is `Fenwick2D::add` at the four corners plus `prefixSum(i + 1, j + 1)`.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.cpp`, entry `11-fenwick_tree_advanced_tester.py` (driver `_00_contest_runner.py`, three builds, 37 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| FenwickRangeAdd `add`, `update` (suffix add, `i` in `[0, n]`), `sum`, `prefixSum`, `get`, vector build; DualFenwick `add`, `update` (suffix add, `i` in `[0, n]`), `get`, vector build; FenwickRangeArithmeticAdd `addProgression`, `add`, `update` (raw coefficients, `i` in `[0, n]`), `sum`, `prefixSum`, `get`, vector build | `rangeAdd<lng>`, `rangeAdd<mint>` | Plain vectors updated element by element (`a[i] += x + s * (i - l)`; `update(i, x)` adds `x` to `a[i..n)` and `b[i..n)`; the arithmetic `update(i, a, b, c)` adds `P(j + 1) - P(j)` to `c[j]` with `P(r) = a + b * r + c * r(r-1)/2` for `r > i`, else 0) with zero, negative and positive values, adds and differences (shifted only for `mint`); every prefix and point re-read for `n <= 17` and every 50 steps; sizes 0–100 (513 in stress) |
| Fenwick2D `add`, `sum`, `prefixSum`, `get`, matrix build; Fenwick2DRangeAdd `add`, `sum`, `prefixSum`; Fenwick3D `add`, `sum`, `prefixSum`, `get`, box build | `grids` | Dense matrices and boxes with brute rectangle and box sums; dimensions include `0 x 3`, `3 x 0`, `1 x 1`, `32 x 33`, `16 x 3 x 17` |
| CompressedFenwick2D `add`, `sum`, `prefixSum` | `compressed` | A `map<point, weight>` scanned per query; coordinate spans 4, 1000 and 1e18 with negative values, duplicate registered points, unregistered query bounds |
| FenwickPrefixMonoid `apply`, `prefix`, size and vector builds | `prefixMonoid` | `Max<lng>` (identity `INT64_MIN`), gcd through `Monoid`, and a stateful `CappedSum{cap}` (sum saturated at a runtime cap) recomputed by scanning |
| Fenwick01 `get`, `size`, `add`, `insert`, `erase`, `rank`, `sum`, `kth`, `next`, `prev`, vector<bool> build; alias `FenwickSet` | `bitset01` | A sorted element vector with `lower_bound`/`upper_bound`; `n` in {0, 1, 2, 63, 64, 65, 127, 128, 129, 200, 1000, 4096, 20000 (stress)}; densities empty, sparse, half and full |
| Preconditions | `invalid` probes | 37 probes: negative sizes, `update` at `-1` and `n + 1` on `FenwickRangeAdd`, `DualFenwick` and `FenwickRangeArithmeticAdd`, oversize grid and box, reversed or out-of-range ranges, ragged matrix rows, unregistered compressed points, double insert or remove, `kth(-1)` |

Quick mode runs 6 sizes and 150 steps without the sanitizer build; full runs 14 sizes and 1500 steps in three builds; stress runs sizes up to 513/20000 and 4000 steps, sized so the `-O0` debug-iterator build stays inside the runner's 180 s limit. A hand mutation check (2026-10-08, quick mode, temporary copies) detected 7 of 8 injected faults (fused 3D build pass, select comparison, triangular number, 2D range-add prefix formula, `prev` sentinel, difference index weight, 3D inclusion–exclusion sign); the survivor, keeping duplicate `y` values per compressed node, is functionally equivalent.

## Commands and Results

Machine: Linux x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.7; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 887 MB | 0.2 + 1.2 | 23.2 + 8.4 | 25.4 + 14.9 | 1091 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 955 MB | 0.3 + 2.1 | 33.9 + 11.1 | 38.8 + 11.4 | 1057 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 36.2 + 0.1 | 27.2 + 1.4 | | 872 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 894 MB | 0.1 + 1.5 | 9.8 + 43.4 | 10.4 + 18.2 | 574 MB |

Package runs of 2026-10-17 (P027 monoid pass):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.py' --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 2,044,911 checks per configuration, 37 assertion probes, MEMORY peak 551 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.py' --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 2,044,911 checks per configuration, MEMORY peak 551 MB |
| `python3 '96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.py' --mode stress --seed 7` | PASS, 6,553,341 checks per configuration, MEMORY peak 551 MB |
| `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace, sanitizer self-tests; MEMORY peak 1834 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| OI Wiki Fenwick (https://oi-wiki.org/ds/fenwick/) | Range add / range sum decomposition, 2D range add with four coefficient trees. |
| maspypy `fenwicktree_range_add.hpp`, `fenwicktree_01.hpp`, `fenwicktree_2d_dense.hpp` | Range-add API with linear build; bit-packed 0/1 set with `kth`, `next`, `prev`; dense 2D build. |
| suisen `fenwick_tree_set.hpp`, `fenwick_tree_2d_sparse.hpp`, `fenwick_tree_prefix.hpp` | Set operation names (`rank`, `kth`), compressed 2D construction from a registered point universe, prefix-monoid `apply`. |
| ei1333 `abstract-binary-indexed-tree.hpp` | Commutative-monoid prefix Fenwick contract. |
| KACTL `FenwickTree2d.h` | Offline point universe per node with sorted `y` lists. |
| AtCoder Library fenwicktree | Supported value types and prefix contract. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)); the code is written from the contracts above.

## Limits and Handoffs

- Row 11 is partial: `DualCompressedFenwick2D`, `CompressedFenwick2DRangeAdd`, `DynamicFenwick`, `DynamicFenwick2D`, `PersistentFenwick`, `FenwickRangeMax` and `DualFenwick2D` are listed under `missing:` in the inventory row.
- `FenwickRangeArithmeticAdd` with floating `T` is exact only up to rounding of the three coefficient sums.
- Fenwick01 uses a single-word select only; a `pdep`-based select is an ISA kernel and stays out of the contest profile.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 树状数组, 树状数组区间修改, 区间加区间求和, 二维树状数组, 三维树状数组, 权值树状数组, 树状数组求第k小, 区间加等差数列, 区间修改单点查询.

## History

- 2026-10-08: P027 first verification; legacy `FenTreeRangeAdd1D`/`FenTree` replaced.
- 2026-10-08: independent review (P027): fixed the overflowing 3D size check (finding 3, new `box-oversize` probe), restated the integral domains with their constant factors (finding 4) and added zero and negative inputs to the range-add suites (finding 6); the formulas, builds and compressed node lists were confirmed.
- 2026-10-08: P027 re-audit (second session): formulas and builds reconfirmed; the `CompressedFenwick2D::add` registration assert moved out of the per-level loop and the `Fenwick01` `vector<bool>` constructor got its own `O(n)` comment (style findings only).
- 2026-10-12: re-audit fix: `update` on `FenwickRangeAdd`, `DualFenwick` and `FenwickRangeArithmeticAdd` asserts `0 <= i <= n`, with direct oracle tests and 6 new probes; full passed on g++ and g++-14.
- 2026-10-13: prune removed `Fenwick01::contains` (its checks were the `get` checks on the same inputs); full passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract for `FenwickPrefixMonoid`, stateful-monoid test); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 551/551/551 MB.
- 2026-11-07: P263 added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
