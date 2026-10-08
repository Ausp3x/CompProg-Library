# 11-fenwick_tree_advanced.hpp — evidence

Package P027 (batch DS04). Replaces the unchanged extractions `FenTreeRangeAdd1D` and `FenTree` (`OLD/algorithms.cpp:1481–1860`) with named structures; the Basic `Fenwick` (row 02) keeps point add / prefix sum. All indices are zero-based, every range is half-open, and every precondition is an `assert` checked once at the entry of an operation. A moved-from object may only be assigned to or destroyed.

## Contracts

### FenwickRangeAdd<T>

`T` is a commutative ring (`+`, `-`, `*`, `T(int)`): `lng`, `mint`, `double`. Two Fenwick arrays store the difference sequence `d` and `d * index`, so `prefixSum(r) = r * sum(d[0..r)) - sum(d[j] * j)`; the intermediate `r * sum(d)` equals `r * a[r - 1]`, and `sum(d[j] * j)` is bounded by `2 * n * max|a_i|`, so for integral `T` the domain is `2 * n * max|a_i| < 2^63` over the whole history (modular `T` has no bound). `add(l, r, x)` adds `x` to `[l, r)`; `sum(l, r)`, `prefixSum(r)` and `get(i)` read; `update(i, x)` is the point helper. Constructors: size (zeros) and `vector<T>` with an O(n) build from first differences.

### DualFenwick<T>

`T` is a commutative group. One Fenwick array over first differences: `add(l, r, x)` in O(log(n)), `get(i)` = prefix of differences. No range sum (use `FenwickRangeAdd`). Size and vector constructors.

### FenwickRangeArithmeticAdd<T>

`addProgression(l, r, a, d)` adds `a + d * (i - l)` to `a[i]` for `i` in `[l, r)`; `add(l, r, x)` is the constant case. Three coefficient arrays `(A, B, C)` make `prefixSum(r) = A(r) + B(r) * r + C(r) * r(r-1)/2`, where the triangular number is computed exactly in `lng` before conversion to `T`, so `mint` works. Integral domain: `2 * n^2 * max(|a_i|, |a|, |d|)` over the history fits `T` (the coefficient sums carry `n * a` and `n^2 * d / 2` terms). `sum`, `prefixSum`, `get`; size and vector constructors.

### Fenwick2D<T>, Fenwick3D<T>

Dense `n x m` (`n x m x h`) tables, flat storage, `n * m (* h) <= INT_MAX` (the 3D check tests `n * m` first so the product cannot overflow `lng`). `add(i, j[, k], x)` point add; `prefixSum(i, j[, k])` over the half-open prefix box; `sum(i1, j1, [k1,] i2, j2, [k2])` over `[i1, i2) x [j1, j2) [x [k1, k2))` by inclusion–exclusion (asserts ordered bounds); `get` is the unit box. The matrix constructors infer `m` (and `h`) from the first row (plane) and assert every row has that size; an empty outer vector yields zero inner sizes, so use the size constructor for `0 x m`. The O(n * m) build runs one Fenwick pass per axis; the axis transforms are linear and commute, so each pass is applied exactly once (a fused build double-transforms carried-over contributions, which the tests reproduce as a mutant).

### Fenwick2DRangeAdd<T>

`add(i1, j1, i2, j2, x)` adds `x` on `[i1, i2) x [j1, j2)`, `sum`/`prefixSum` over rectangles. Four coefficient sums per cell (`d`, `d*i`, `d*j`, `d*i*j`): `prefixSum(i, j) = i*j*D - j*Di - i*Dj + Dij`. `T` is a commutative ring; integral domain `4 * n * m * max|a|` over the history fits `T` (the four coefficient sums carry index products up to `n * m`). Size constructor only (initial zeros).

### CompressedFenwick2D<T>

Construction takes the point universe `vector<pair<lng, lng>>` (duplicates allowed); `add(x, y, w)` asserts `(x, y)` is a registered point once at entry (registration in the first column implies every ancestor column); `sum(x1, y1, x2, y2)` and `prefixSum(x, y)` accept any `lng` bounds. Each Fenwick node over the distinct `x` values holds the sorted distinct `y` values of the points it covers (built from the points sorted by `y`, so no per-node sort); storage is `O(n log n)`, operations `O(log^2 n)`. `T` is a commutative group.

### FenwickPrefixMonoid<T, F>

`f` is associative and commutative with identity `id`. `apply(i, x)` sets `a[i] = f(a[i], x)` (point combine, never overwrite); `prefix(r)` folds `[0, r)`. Max/min, gcd, bitwise or, products of units: anything commutative. No range query and no inverse. Size and vector constructors.

### Fenwick01 (alias FenwickSet)

A set of integers in `[0, n)`: one bit per element in 64-bit words plus a Fenwick array over word popcounts. `get`/`contains`, `size`, `add(i, ±1)` (asserts the bit changes), `insert`/`erase` returning whether the set changed, `rank(r)` = count below `r` for `r` in `[0, n]`, `sum(l, r)` count in `[l, r)`, `kth(k)` = k-th smallest (0-based) or `n` when `k >= size()`, `next(i)` = smallest element `>= i` or `n` (`i` in `[0, n]`), `prev(i)` = largest element `<= i` or `-1` (`i` in `[-1, n)`). The in-word select is a 6-step popcount binary search, so `kth` is `O(log(n / 64) + log 64)`. The `vector<bool>` constructor builds in `O(n)` (bit packing) with its own complexity comment.

### Replaced legacy operations

`FenTreeRangeAdd1D` → `FenwickRangeAdd`; `FenTree` 1D with `f`/`f_inv` → Basic `Fenwick<T>` with a group `T`, or `FenwickPrefixMonoid` when only a prefix monoid is needed; `FenTree` 2D/3D → `Fenwick2D`/`Fenwick3D` including the linear builds; `FenTree::lowerBound` → `Fenwick::lowerBound`; `getIdx` was an internal flattening helper; the `operator<<` node printers are not ported (the structures expose their arrays and the debug header prints vectors). Rectangle add with point get is `Fenwick2D::add` at the four corners plus `prefixSum(i + 1, j + 1)`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.cpp`, entry `11-fenwick_tree_advanced_tester.py` (driver `_00_runner.py`: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan; `-Wall -Wextra -Wconversion -Werror`; 31 assertion probes under the checked build).

| Operation | Group | Oracle |
|---|---|---|
| FenwickRangeAdd `add`, `sum`, `prefixSum`, `get`, vector build; DualFenwick `add`, `get`, vector build; FenwickRangeArithmeticAdd `addProgression`, `add`, `sum`, `prefixSum`, `get`, vector build | `rangeAdd<lng>`, `rangeAdd<mint>` | Plain vectors updated element by element (`a[i] += x + s * (i - l)`) with zero, negative and positive values, adds and differences (shifted only for `mint`), every prefix and point re-read for `n <= 17` and every 50 steps; sizes 0–100 (513 in stress) |
| Fenwick2D `add`, `sum`, `prefixSum`, `get`, matrix build; Fenwick2DRangeAdd `add`, `sum`, `prefixSum`; Fenwick3D `add`, `sum`, `prefixSum`, `get`, box build | `grids` | Dense matrices/boxes with brute rectangle and box sums; dimensions include `0 x 3`, `3 x 0`, `1 x 1`, `32 x 33`, `16 x 3 x 17` |
| CompressedFenwick2D `add`, `sum`, `prefixSum` | `compressed` | `map<point, weight>` scanned per query; coordinate spans 4, 1000 and 1e18 with negative values, duplicate registered points, unregistered query bounds |
| FenwickPrefixMonoid `apply`, `prefix`, vector build | `prefixMonoid` | Max (identity `INT64_MIN`) and gcd folds recomputed by scanning |
| Fenwick01 `get`, `contains`, `size`, `add`, `insert`, `erase`, `rank`, `sum`, `kth`, `next`, `prev`, vector<bool> build; alias `FenwickSet` | `bitset01` | Sorted element vector with `lower_bound`/`upper_bound`; `n` in {0, 1, 2, 63, 64, 65, 127, 128, 129, 200, 1000, 4096, 20000 (stress)}, densities empty/sparse/half/full |
| Preconditions | `invalid` probes | 31 probes: negative sizes, oversize grid and box, reversed or out-of-range ranges, ragged matrix rows, unregistered compressed points, double insert/remove, `kth(-1)` |

Quick: 6 sizes, 150 steps, no sanitizer build. Full: 14 sizes, 1500 steps, three builds. Stress: sizes up to 513/20000, 4000 steps (sized so the `-O0` debug-iterator build stays inside the runner's 180 s limit). Mutation check (2026-10-08, quick mode, temporary copies): 7 of 8 injected faults detected (fused 3D build pass, select comparison, triangular number, 2D range-add prefix formula, `prev` sentinel, difference index weight, 3D inclusion–exclusion sign); the survivor (keeping duplicate `y` values per compressed node) is functionally equivalent.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/11-fenwick_tree_advanced_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/1' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-08 (re-audit pass, after its fixes): full mode seed 20261008 passed on both compilers, 2,138,402 checks per configuration and 31 assertion probes; stress mode seed 7 passed all three configurations with 6,848,592 checks per configuration. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, with `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). `02-integration.py --sanitizers` passed 108 standalone/aggregate headers (including this one alone, in `99-all.hpp` and in the two-translation-unit build), the workspace build and the sanitizer self-tests; `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header and its tester. Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

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

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): hash-map dynamic Fenwick, persistent Fenwick, compressed rectangle-add/rectangle-sum, `O(log^2 n)` Fenwick range max with overwrite, 2D dual Fenwick, K-dimensional compressed prefix monoid.
- `FenwickRangeArithmeticAdd` with floating `T` is exact only up to rounding of the three coefficient sums.
- Fenwick01 is single-word-select only; a `pdep`-based select is an ISA kernel and stays out of the contest profile.

## History

- 2026-10-08: P027 first verification; legacy `FenTreeRangeAdd1D`/`FenTree` replaced.
- 2026-10-08: independent review (P027) — fixed the overflowing 3D size check (finding 3, new `box-oversize` probe), restated the integral domains with their constant factors (finding 4) and added zero/negative inputs to the range-add suites (finding 6); the formulas, builds and compressed node lists were confirmed.
- 2026-10-08: P027 re-audit pass (second session): formulas and builds reconfirmed; the `CompressedFenwick2D::add` registration assert moved out of the per-level loop and the `Fenwick01` `vector<bool>` constructor got its own `O(n)` comment (style findings only).
