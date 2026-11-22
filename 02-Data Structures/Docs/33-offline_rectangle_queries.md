# 33-offline_rectangle_queries.hpp — Evidence

Package P033 (batch DS29). New header; no legacy source. One offline engine for point and rectangle updates against point and rectangle queries on the integer lattice, in time order. Each call asserts its preconditions once.

## Contracts

### OfflineRectangleSweep<T>

Events are recorded in time order and answered by `solve()`; an update reaches exactly the queries added after it.

- `addPoint(x, y, w)`: adds `w` at the lattice point `(x, y)`; `x, y < INT64_MAX`.
- `addRectangle(x1, y1, x2, y2, w)`: adds `w` at every lattice point of `[x1, x2) x [y1, y2)`; asserts `x1 <= x2` and `y1 <= y2` (an empty side adds nothing).
- `addQuery(x1, y1, x2, y2)` asks for the sum over the rectangle's lattice points (asserting the same order), and `addQuery(x, y)` for the value at one point (`x, y < INT64_MAX`). Each returns its query id, `0, 1, 2, ...` in call order.
- `solve()` returns the answers by query id; it is `const` and repeatable, and the engine accepts more events afterwards.
- `addPoint` and point `addQuery` are named special cases of the unit rectangle `[x, x + 1) x [y, y + 1)`, as `Fenwick::get` is of `sum`; they choose cheaper kernels.
- Algebra: `T` is a commutative ring with `T(0)` and a conversion from `lng`. Only `+`, `-` and `+=` are used unless rectangle updates meet rectangle queries; then weights are multiplied by coordinates and Fenwick cells hold sums of `w * x * y` over corners, so use a modular or wrapping `T` (`ulng` gives the answer mod 2^64) unless those fit. Each sweep clears what it added, so cells hold only partial sums of the current sweep, and `T = lng` overflows only when such a true partial sum does. Results are exact for exact `T`, and coordinates span all of `lng`.

### Algorithm

- Every event becomes weighted corners, and a query corner `(X, Y)` collects the update corners with `x < X` and `y < Y` (strict dominance).
- The kernel is chosen from the event kinds present:
  - With no rectangle query (dual form), updates become the four corners of their 2D difference array, and a point query is one corner `(x + 1, y + 1)` with one coefficient.
  - With rectangle queries and no rectangle update, a point update is one corner and a query has four corners, `F(x2, y2) - F(x1, y2) - F(x2, y1) + F(x1, y1)`, with one coefficient.
  - With both, a rectangle update corner `(x, y, v)` contributes `v (X - x)(Y - y)`, which is stored as four coefficients `(v, -v y, -v x, v x y)` and evaluated as `c0 X Y + c1 X + c2 Y + c3`; a point update is the constant coefficient.
- Time order: the events form `r` maximal runs of updates or of queries. A divide and conquer over runs, split at the middle run index, sweeps at each node the update corners of the left runs against the query corners of the right runs, in x order with queries first at equal x, over a Fenwick tree on the compressed y of all update corners; each event looks its one or two y values up once. The node then clears the cells it touched and, unless both halves hold a single run, stably partitions its slice of the x-sorted corners by run in place. When every update precedes every query, `r <= 2` and one sweep remains.
- Bounds: `k <= 4 * events` corners, one O(k log k) sort, depth `ceil(log2(r))`, and O(k log k) Fenwick work per depth level. Total O(k log(k) log(r + 1)) and O(k) memory. Static input costs O(k log k).
- Bounds for comparison: row 11 `CompressedFenwick2D` answers time-ordered point add and rectangle sum online on registered points in O(log^2) per operation with O(n log n) memory; row 17 `RectangleWaveletMatrix` answers static weighted counts online in O(log n). This engine is the offline sweep family with O(k) memory.

### Legacy Mapping

| Planned | Replacement |
|---|---|
| `pointAddRectangleSum` | `addPoint`, `addQuery` (rectangle), `solve` in input order (Library Checker `point_add_rectangle_sum`) |
| `rectangleAddPointGet` | `addRectangle`, `addQuery` (point), `solve` (`rectangle_add_point_get`) |
| `rectangleAddRectangleSum (static/offline)` | `addRectangle`, `addQuery` (rectangle), `solve` (`static_rectangle_add_rectangle_sum` when every update comes first; any interleaving otherwise) |
| `staticRectangleSum` | `addPoint`, then `addQuery` (rectangle), `solve` (`rectangle_sum`): one sweep |
| `rectangleCount` | `OfflineRectangleSweep<int>` with unit weights |

Each would call the engine with another argument shape (`03-cpp.md`, One API per Operation), so none was added. The research sweep lists the five as aliases of the engine, and their cases are tests of the kept operations (kind mixes `0b0101`, `0b1010`, `0b0110` with the layouts "updates first" and "random").

### Violated Preconditions

Under `NDEBUG` a reversed rectangle contributes with the opposite sign and `INT64_MAX` point coordinates overflow `x + 1`; every loop stays bounded by the event count.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/33-offline_rectangle_queries_tester.cpp`, entry `33-offline_rectangle_queries_tester.py` (driver `_00_contest_runner.py`; three builds, the checked build with `_GLIBCXX_ASSERTIONS` because debug `lower_bound` checks partitioning in O(n) per call and the engine searches the compressed y once or twice per event; 8 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `addPoint`, `addRectangle`, `addQuery` (both), `solve`, for 12 mixes of the four event kinds and four layouts (random, updates first, queries first, alternating runs of 1–3) | `small` (coordinates in [-3, 4), up to 30 events) | Lattice simulation on a 7 x 7 grid; the direct formula below is checked against the same grid, so it is itself validated |
| Same with large coordinates and weights | `randomLarge` (coordinates in [-1000, 1000] with `lng`; all of `lng` with `ulng`, a mod 998244353 ring and, for one-coefficient kernels, `lng`) | Direct formula: each earlier update adds `w` times the lattice points it shares with the query, computed with 128-bit overlap widths in the target ring |
| Query ids, repeatable `solve` | every `run` | Ids `0, 1, ...`; two `solve` calls agree |
| Single-sweep and divide-and-conquer bounds | `work` (point/rectangle, rectangle/point, rectangle/rectangle kinds; updates first and alternating runs) | Counted Fenwick additions at most `2 K k (bit_width(k) + 1)` times 1 (static) or `ceil(log2(r))` (alternating); answers against the formula on the four-coefficient alternating case |
| Cleared cells (bounded intermediates) | `magnitude` | With unit point weights, no Fenwick cell exceeds the number of points |
| Empty engine, updates only, queries before updates, empty rectangles, `INT64_MIN`/`INT64_MAX` coordinates, half-open edges, copies | `edges` | Direct values |
| Preconditions | `invalid` | Reversed rectangles and queries on either axis, `INT64_MAX` coordinates of point updates and point queries on either axis |

Hand mutation check (2026-10-14, quick mode, temporary copies): 8 of 8 injected faults detected (updates before queries at equal x, a wrong four-coefficient sign, all events in one run, an unbalanced split, the dual kernel chosen for rectangle queries, no clearing after a sweep, non-strict y dominance, stopping when either half has one run). Clearing is invisible in exact rings, because stale cells cancel between the paired x corners of every query (primal) or update (dual). It is caught by the `magnitude` group.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 830 MB; optimized 0.2 + 3.9 s, checked 22.3 + 5.4 s, ASan-UBSan 40.1 + 25.4 s; MEMORY peak 995 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 846 MB; optimized 0.2 + 5.8 s, checked 33.5 + 7.8 s, ASan-UBSan 57.7 + 44.5 s; MEMORY peak 1003 MB |
| `01-run.py --mode quick` | Optimized 36.9 + 0.1 s, checked 24.5 + 0.8 s; MEMORY peak 833 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 834 MB; optimized 0.1 + 11.3 s, checked 8.6 + 12.3 s, ASan-UBSan 16.2 + 59.4 s; MEMORY peak 699 MB |

Latest per-header run 2026-10-14, after the review fixes, on the same machine with CPython 3.14, in the optimized, checked (`_GLIBCXX_ASSERTIONS`) and ASan/UBSan builds. MEMORY peaks are dominated by compilation.

| Command (2026-10-14) | Result |
|---|---|
| `33-offline_rectangle_queries_tester.py --mode full --seed 20261014` (GCC 16.2.1) | PASS, 1,145,792 checks per configuration, 8 assertion probes, MEMORY peak 628 MB |
| `CXX=g++-14 33-offline_rectangle_queries_tester.py --mode full --seed 20261014` (GCC 14.4.1) | PASS, 1,145,792 checks per configuration, 8 assertion probes, MEMORY peak 619 MB |
| `01-run.py --mode stress --seed 7 --rounds 1` | PASS, 3 configurations, 6,699,211 checks per configuration, MEMORY peak 649 MB |
| `02-integration.py --sanitizers` | PASS: 143 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; MEMORY peak 2758 MB |
| `03-consistency.py` | No errors |

Informal timing on 2026-10-14 (ad-hoc driver, not a benchmark driver; best of 5, `-O2 -DNDEBUG`, GCC 16.2.1, random coordinates below 10^9, N = Q = 2 * 10^5, add calls included): static rectangle sum 219 ms, interleaved point add and rectangle sum 519 ms, static rectangle add and rectangle sum (mod 998244353) 592 ms, interleaved rectangle add and point get 300 ms.

```bash
python3 '96-Local Testing/02-Data Structures/33-offline_rectangle_queries_tester.py' --mode full --seed 20261014
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/33-offline_rectangle_queries_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/33' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```


## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `point_add_rectangle_sum`, `rectangle_add_point_get`, `static_rectangle_add_rectangle_sum`, `rectangle_sum` task statements, `categories.toml` | Half-open rectangles, time order (an update is seen by later queries), duplicate points; no time-ordered rectangle-add rectangle-sum problem exists. |
| maspypy `ds/offline_query/point_add_rectangle_sum.hpp`, `rectangle_add_point_sum.hpp`, `rectangle_add_rectangle_sum.hpp`, `coefficient_query_2d.hpp`; ei1333 `other/static-point-add-rectangle-sum.hpp`, `dynamic-point-add-rectangle-sum.hpp`, `dynamic-rectangle-add-point-get.hpp`; hitonanode `data_structure/rectangle_sum.hpp`, `rectangle_add_rectangle_sum.hpp`; Nyaan `data-structure-2d/rectangle-add-rectangle-sum.hpp`; suisen `fenwick_tree/static_rectangle_add_rectangle_sum.hpp`, `offline_rectangle_add_point_get.hpp`; tko919 `DataStructure/dynamicrectsum.hpp` | Static sweeps, the four-coefficient `(1, x, y, xy)` decomposition, CDQ over time for the dynamic forms. |
| OI Wiki CDQ divide and conquer (`misc/cdq-divide`), scanning line (`geometry/scanning`) | Time divide and conquer for rectangle add and rectangle sum, O(n log^2 n). |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Row 33 is partial: `CoefficientQuery2D` (higher-order coefficient queries, maspypy `coefficient_query_2d`) is missing, returned by P275. Online rectangle add on registered points and the dynamic 2D Fenwick are row 11 (`DualCompressedFenwick2D`, `CompressedFenwick2DRangeAdd`, `DynamicFenwick2D`), and the k-dimensional compressed structure is row 43 `RangeTreeK`.
- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): block-decomposed Fenwick trees.
- Miscellaneous `21-cdq.hpp` (planned) owns the generic time divide and conquer and 3D dominance; this header keeps its own run-based recursion because its sweep shares the corner list and the Fenwick tree across levels. Geometry `13-segment_union.hpp` owns rectangle-union area and Geometry `29` triangle counting.

## Aliases

Search aliases (P263, grab --find): 二维数点, 单点加矩形求和, 矩形加矩形求和, 矩形加单点查询, 离线扫描线, 二维偏序, 矩形求和.

## History

- 2026-10-14: P033 first implementation and verification; the review found no defects, 3 assertion probes and the `std::midpoint` split were added; full (seed 20261014) on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
