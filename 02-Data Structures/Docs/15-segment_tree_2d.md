# 15-segment_tree_2d.hpp — evidence

Package P028 (batch DS08). Replaces the unchanged extraction `SegTree2D` (`OLD/algorithms.cpp:2727–2856`). Indices are zero-based, every range is half-open (`[xl, xr) x [yl, yr)`), the first coordinate is the row (x), and preconditions are `assert`s checked once per operation.

## Contracts

All four structures combine values with an operation that must be commutative: a two-dimensional decomposition visits cells in row-of-column order, not in row-major order.

### SegmentTree2DDense<M>

- `M` supplies `S`, a commutative associative `op` and identity `e()`. Size constructor `(rows, cols)` (all cells `e()`) or `vector<vector<S>>` build in O(n m) (rows must have equal length; an empty outer vector gives a 0 x 0 grid). `n * m <= 2^28`.
- Layout: a `2n x 2m` flat array; row `i` and column `j` follow the bottom-up (non-power-of-two) segment tree, so cell `(n + i, m + j)` is the leaf and `(1, 1)` is the whole grid. Memory `4nm` values.
- `get(i, j)` O(1); `set(i, j, x)` and `apply(i, j, x)` (`set(i, j, op(get(i, j), x))`) O(log n log m); `prod(xl, xr, yl, yr)` O(log n log m), `e()` when empty; `allProd()` O(1).

### DualSegmentTree2DDense<A>

- `A` supplies `F`, a commutative `composition` and `id()`. Size constructor only (every cell starts at `id()`); `n * m <= 2^28`.
- `apply(xl, xr, yl, yr, f)` composes `f` into the O(log n log m) canonical cells; `get(i, j)` composes the O(log n log m) ancestor pairs; `values()` returns the whole grid in O(n m) by propagating along columns in every row and then along rows in every leaf column.

### SegmentTree2DSparse<M>

- Points are fixed at construction: `SegmentTree2DSparse(points, weights)`; duplicate points merge into one cell and their weights combine with `op`; without weights every point starts at `e()`. Coordinates are any `lng` values and query bounds may be any `lng`.
- Layout: points sorted by `(x, y)` index a bottom-up outer tree; node `v` stores its points sorted by `(y, index)` in `id[off[v] .. off[v + 1])`, their `y` in `ys`, and an inner bottom-up tree in `t[2 off[v] ..]`. Memory O(k log k) for k distinct points; build O(k log k).
- `get(x, y)` O(log k) and `set`, `apply` O(log^2 k) on registered points (asserted); `prod(xl, xr, yl, yr)` O(log^2 k), `e()` when empty; `allProd()` O(1).

### LazyKdTree<A>

- `A` is an acted monoid ([12-lazysegmenttree.md](12-lazysegmenttree.md#acted-monoid-contract-00-monoidshpp)) whose `op` is commutative. Points keep their input index `i`; they start at `w[i]`, or `e()` when `w` is empty (an inactive point, which an acted monoid maps to itself).
- Build: median split by `nth_element`, alternating x and y by depth; every node stores the tight bounding box of its points. Memory O(k); build O(k log k).
- `get(i)`, `set(i, s)` O(log k) (push along the leaf's ancestors, then pull); `prod` and `apply` on a rectangle O(sqrt(k)): a node is skipped when its box misses the rectangle, used whole when its box lies inside, and split otherwise. With alternating median splits, a rectangle edge crosses O(sqrt(k)) boxes; equal coordinates do not break this, because two siblings split on y both straddling a horizontal edge would need the median below and above the edge at once. `allProd()` O(1).
- The Library Checker task `dynamic_point_set_rectangle_affine_rectangle_sum` is `LazyKdTree<RangeAffineRangeSum<mint>>` over every point the input ever inserts (registered at `e()`); inserting or setting point `i` with weight `w` is `set(i, leaf(w))`.

### Legacy mapping

`SegTree2D<Mon>` (recursive `4n x 4m` arrays, inclusive ranges, `Mon::init/ope/map/idS`, silent no-op on bad ranges) becomes `SegmentTree2DDense<M>`: `update(i, j, f)` is `set(i, j, mapping(f, get(i, j)))` or `apply` for a combining update, `query(i1, j1, i2, j2)` is `prod(i1, i2 + 1, j1, j2 + 1)`, `queryAll` is `allProd`. The row's `FenwickOfSegmentTrees` is dropped (see Limits).

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/15-segment_tree_2d_tester.cpp`, entry `15-segment_tree_2d_tester.py` (driver `_00_runner.py`; three builds; 17 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| SegmentTree2DDense size and vector builds, `get`, `set`, `apply`, `prod`, `allProd` | `denseRun<Sum>`, `denseRun<Min>` for every n, m in {0, 1, 2, 3, 4, 5, 7, 8, 9, 13, 16, 17} | Brute grid folded over the rectangle |
| DualSegmentTree2DDense `apply`, `get`, `values` | `dualRun<AddAct>`, `dualRun<MaxAct>` | Brute grid of composed actions |
| SegmentTree2DSparse build with and without weights (duplicates combine), `get`, `set`, `apply`, `prod`, `allProd` | `sparseRun<Sum>`, `sparseRun<Min>` for k up to 257 points in boxes of side 5, 13 and 2·10^18, including unbounded queries | `std::map` of registered cells |
| LazyKdTree build with and without weights, `get`, `set`, `apply`, `prod`, `allProd` | `kdRun` (random and degenerate: half the points on one x, a third on one y) | Per-point values mod 998244353 with active flags; affine maps applied to active points inside the rectangle |
| Empty grids and point sets, 1 x 1, copy independence, extreme coordinates, duplicate kd points | `edges` | Direct values |
| Preconditions | `invalid` | Negative and oversize grids, ragged rows, out-of-range cells and rectangles, reversed rectangles, weight-count mismatches, unregistered points, kd indices |

Mutation check (2026-10-10, quick mode, temporary copies): 7 of 8 injected faults detected (column path skipped in `set`, skipped root-row propagation in `values`, sparse position ignoring index ties, wrong `firstX` bound, closed rectangle edge in `cover`, kd tag overwrite, missing root push); the survivor (`rowProd(xr - 1)` without decrementing `xr`) is equivalent, because the next `xr >>= 1` discards the low bit.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/15-segment_tree_2d_tester.py' --mode full --seed 20261010
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/15-segment_tree_2d_tester.py' --mode full --seed 20261010
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/15' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-10 (after the review fixes): full mode seed 20261010 passed on GCC 16.2 and GCC 14.4.1 (`CXX=g++-14`) with 304,832 checks per configuration and 17 assertion probes, MEMORY peak 535 MB and 516 MB; stress mode seed 7 (one round) passed all three configurations with 1,357,924 checks per configuration, MEMORY peak 533 MB. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds. MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (112 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 2693 MB), and a separate two-unit probe instantiating `SegTreeBeats`, `HistoricSegTree`, `SegmentTree2DDense`, `SWAGQueue`, `SWAGDeque` and `MergeSortTree` in both units linked and ran. `03-consistency.py` cannot run inside this worktree (seven gitignored binaries under `OLD/` and one benchmark log exist only in the main checkout); on a copy with those files restored after rebasing onto master (97ed2db) it reported no errors, and the four suites passed quick mode again. GCC 14.2 exactly, the Windows build and PyPy were not run.

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `segtree_2d`, `segtree_2d_dense`, `dual_segtree_2d_dense`, `kdtree_acted_monoid` and its `dynamic_point_set_rectangle_affine_rectangle_sum` test | Commutative-monoid requirement, dense layout, `values` (getAll), registering every point first. |
| Library Checker `dynamic_point_set_rectangle_affine_rectangle_sum` task statement | Query types and the half-open rectangle convention. |
| suisen `segment_tree_2d_sparse`; Nyaan `2d-segment-tree`, `abstract-range-tree`; OI Wiki seg-in-seg | Sparse layout (sorted y per outer node) and weighted build. |

Fetched 2026-10-10 by the completeness sweep ([00-sources.md](00-sources.md)); the implementation was written from the contract.

## Limits and handoffs

- `FenwickOfSegmentTrees` is dropped: `SegmentTree2DSparse::prod` answers the same prefix-in-x, range-in-y queries at the same O(log^2 k) bound, and `CompressedFenwick2D` (row 11) is the group-valued Fenwick of Fenwicks.
- Left out with reasons in [00-notes.md](00-notes.md#p028-omissions): sparse `maxUp`/`minDown` searches, online rectangle-apply/point-get on registered points, fully online 2D trees, k-dimensional compressed trees, a pluggable range tree.
- Geometry `28-spatial_index.hpp` keeps point-facing kd-tree queries (nearest, kNearest, radius, box reporting); `LazyKdTree` is the rectangle fold/apply engine.

## History

- 2026-10-10: P028 first verification; legacy `SegTree2D` replaced by `SegmentTree2DDense`, dual dense, sparse and lazy kd-tree added.
- 2026-10-10: independent review (P028): no wrong answers; replaced two `size_t` loop counters; the reviewer's kd scaling probe on a 64 x 64 grid with heavy duplicates took 448, 1022 and 2245 ms for k = 2^16, 2^18, 2^20, about 2.2x per 4x, consistent with O(sqrt(k)).
