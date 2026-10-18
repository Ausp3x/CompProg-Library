# 13-segment_union.hpp — evidence

Owned by package P037 / GE05, together with [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md) and [15-convex_polygon_query.md](15-convex_polygon_query.md). Every operation in the row has a test.

## Contracts

### Numeric policy

All results are exact for integral input. Rectangles are `array<lng, 4>` `{x1, y1, x2, y2}`, meaning the half-open box `[x1, x2) × [y1, y2)`. Segments are `array<lng, 3>`: horizontal `{y, x1, x2}` and vertical `{x, y1, y2}`, closed, endpoints in either order. Rectangle coordinates satisfy `|coordinate| <= 4 * 10^18` (asserted), so widths fit `lng` and areas, perimeters and depth measures fit `lll` (at most `(8 * 10^18)^2 = 6.4 * 10^37`). Segments accept any `lng`: lengths are taken in `lll`, and orthogonal segments are only compared. Depth counters are `int` (at most `n`).

### segmentUnionLength, coverageCount

Input: segments `pair<T, T>` with endpoints in either order, for signed integral or floating `T`; segments with equal endpoints are ignored. `segmentUnionLength` returns the length of the union, `lll` for integral `T` and rounded `long double` for floating `T`. Open and closed ends give the same length. `coverageCount` returns a vector `c` of size `n + 1`. For `k >= 1`, `c[k]` is the length covered by exactly `k` segments. `c[0]` is the length of the gaps between the extreme endpoints of the nonempty segments. Both sort `2 * n` endpoint events: `O(n * log(n))` time and `O(n)` memory.

`segmentUnionLength` is kept beside `06-Miscellaneous` `intervalUnionMeasure` by decision (2026-10-18, `duplicates.md`): it also takes floating endpoints. `coverageCount` is the 1D depth histogram by decision (2026-10-18); the other readings of the name are owned by `06-Miscellaneous/09-interval_algorithms.hpp`.

### rectangleUnionArea, rectangleUnionPerimeter

Both use a sweep over x with a segment tree over the compressed y coordinates. Each node keeps its cover count and its covered length (no lazy propagation). Empty rectangles are ignored. The area adds the covered length times the gap to the next event x.

The perimeter sums the vertical boundary length of the sweep and of the transposed sweep. At one x, additions are applied before removals, and the vertical boundary there is `(after adds − before) + (after adds − after removes)`. This equals the measure of the symmetric difference of the covered sets before and after x. Two rectangles that share a side therefore contribute nothing on it, and two that meet only at a corner contribute both sides. `O(n * log(n))`, `O(n)`.

### rectangleCoverageArea

`k >= 1` (asserted; raised to 1 under `NDEBUG`). Returns `{area covered by exactly k rectangles, area covered by at least k}`; `k > n` gives `{0, 0}`. The sweep keeps `len[j]` for `j = 0..k + 1` in every node: the measure covered at least `j` times, counting the cover counts of the node and its descendants. `O(n * k * log(n))` time and `O(n * k)` memory. Nothing faster is known for one `k`; see Limits.

### maxRectangleOverlap

Returns `{maximum number of rectangles sharing a point, a lattice witness}`, or `{0, (0, 0)}` when every rectangle is empty. The witness is the lower-left corner of a deepest cell, and the depth at it, counted with half-open boxes, equals the result. The tree supports range add with a max of children and no push-down. At each x, removals are applied before additions. Closed integral rectangles map to `[x1, x2 + 1) × [y1, y2 + 1)` with the same depth at every lattice point. `O(n * log(n))`.

### rectangleUnionBoundary

Returns the boundary of the closure of the union as maximal axis-parallel segments `{a, b}`, directed so that the union lies on the left. Outer boundaries therefore run counterclockwise and holes clockwise. Segments meet only at endpoints. At a pinch, where two rectangles share only a corner, the boundary passes the corner twice and the pieces stay separate.

At each event x, the event intervals are merged. The uncovered runs of each merged interval are reported before all updates at x and again after them. Every y of an event interval is covered just before x or just after x, so every reported run is a boundary piece. A run uncovered before is a left side, directed downward. A run uncovered after is a right side, directed upward. The horizontal pieces come from the transposed sweep: the reflection is mapped back and the pieces are reversed, because a reflection flips orientation.

Reporting the uncovered runs of an interval visits `O((1 + runs) * log(n))` nodes. The total is `O((n + out) * log(n))` time and `O(n + out)` memory.

### rectangleUnionBoundaryGuting

Returns the same segments as `rectangleUnionBoundary` (as a multiset; the order differs), by Güting's optimal divide and conquer (Acta Informatica 21, 1984, pp. 271–291; the contour algorithm on pp. 277–288).

1. **Division.** The `2n` vertical edges are sorted by x, with left edges first at equal x, and split at the median index. Each subproblem returns:
   - its unmatched left and right edges;
   - the stripe partition `P` of the y-ranks of its edges (plus ±∞);
   - for every stripe, a binary tree whose in-order leaves are the true x transitions of the union inside the subproblem's slab. A leaf is `(x, start of coverage)` or `(x, end of coverage)`; an empty tree means no transition.
2. **Merge.**
   - A rectangle whose left edge is in the left half and right edge in the right half is completed.
   - The left half's unmatched left edges cover the whole right half on their y-range; the right half's unmatched right edges cover the whole left half. Those stripes of the other half are blackened (their trees emptied).
   - Both stripe lists are refined to the union of the partitions, with refined stripes sharing tree pointers, and each stripe's two trees are joined under one new node.
   - Blackening removes every leaf that a spanning rectangle covers, so leaves stay true transitions. Coverage between leaves follows from the neighbouring leaf, and a root stripe starts uncovered at −∞.
3. **Queries.** Bottom edges at one y, merged when they touch, ask the stripe just below for the uncovered runs of their x-range; top edges ask the stripe just above. A query costs `O(log(n) + runs)`: one descent for the state before the range, then an in-order walk of the leaves inside it, each a true transition.
4. **Vertical pieces** come from the same procedure on the transposed rectangles, reversed because a reflection flips orientation.

Time `O(n * log(n) + out)`; memory `O(n * log(n) + out)` for the shared tree nodes. Kept beside the sweep by decision (2026-10-19). In the measurements below it is faster on the larger random sets and the sweep is faster on the largest bar lattice.

### orthogonalIntersectionCount, orthogonalIntersections

These count or list the pairs `(horizontal i, vertical j)` that share a point (closed segments, so touching counts). Parallel overlaps are not pairs. The sweep orders events at equal x as horizontal starts, then vertical queries, then horizontal ends. The count uses a `Fenwick<int>` over the compressed y values of the horizontals: `O((n + m) * log(n + m))` time, `O(n + m)` memory, `lng` result. The report keeps a `set` of active `(y, index)` pairs. Each vertical lists the active horizontals with `y` in its range, in increasing `y`, so the output is grouped by vertical in increasing x. That costs `O((n + m) * log(n + m) + out)`.

`orthogonalIntersectionCount` is kept beside Data Structures row 33 `OfflineRectangleSweep` by decision (2026-10-18, `duplicates.md`).

### Legacy mapping

| Old name | Replacement |
|---|---|
| `getLenSegUnion(vector<pair<int, int>> &segs)` (`OLD/Team Notebook/src/geometry/getlensegunion.cpp`; it swapped endpoints in place and returned `lng`) | `segmentUnionLength(segs)` (input is not modified; returns `lll`) |

## Feature-to-test map

`96-Local Testing/03-Geometry/13-segment_union_tester.py` runs quick, full and stress modes in the optimized, checked and ASan/UBSan configurations, with two assertion probes: a coordinate beyond `4 * 10^18`, and `k = 0` for `rectangleCoverageArea`.

**Oracle.** The depth of every elementary cell of the compressed grid, counted directly against every rectangle or segment, and all pairs for the orthogonal segments. The grid oracle is validated by fixed cases, including the POJ 1177 sample perimeter 228.

| Operation | Test | Oracle |
|---|---|---|
| `segmentUnionLength` | Random segments at scale 10 and `4 * 10^18`, reversed endpoints, empty segments; `long double` endpoints at halves | Cell union; exact halves |
| `coverageCount` | Same corpus; `double` endpoints at halves | Cell depths |
| `rectangleUnionArea` | Fixed: empty, degenerate, side-sharing, corner-touching, nested and frame shapes; random scale 5, 12 and `4 * 10^18`; 100,000-square staircase | Grid union area; closed form `3n + 1` |
| `rectangleUnionPerimeter` | Same corpus; POJ 1177 sample; staircase | Grid boundary length; 228; `4n + 4` |
| `rectangleCoverageArea` | Every `k` from 1 to `n + 1` on the random corpus; staircase with `k = 2` | Grid depths; `n − 1` |
| `maxRectangleOverlap` | Same corpus | Grid maximum depth; depth at the witness counted directly |
| `rectangleUnionBoundary` | Same corpus; staircase segment count; a lattice of 400 thin horizontal and 400 thin vertical bars | Directed elementary boundary pieces of the grid; no two output segments mergeable; `4n`; total length equals `rectangleUnionPerimeter` |
| `rectangleUnionBoundaryGuting` | Every rectangle case above, the staircase and the bar lattice | The same multiset as `rectangleUnionBoundary`, which is checked against the grid |
| `orthogonalIntersectionCount` | Random segments at scale 6 and `4 * 10^18`, reversed endpoints, touching; 2,000 × 2,000 grid plus 100,000 disjoint segments | All pairs; `g^2` |
| `orthogonalIntersections` | Same corpus | All pairs, and the grouping order |

Checked builds (`_GLIBCXX_DEBUG`) run the large phase at `n = 4000` and a 300 × 300 grid, because debug `lower_bound` checks the whole range on every call. ASan builds cap the large phase at `n = 25,000` (which also selects the 300 × 300 grid), because the quarantine keeps the contour trees' freed reallocations alive (a 3.4 GB peak at `n = 100,000`). Domains are unchanged.

Mutation checks (2026-10-19, quick, optimized):
- Caught: removing the adds-first split from the perimeter; making `maxRectangleOverlap` keep right ends closed; Güting without blackening the left half; Güting losing the state before the query range.
- Not caught, equivalent in results: right edges first at equal x in Güting. The extra transitions it leaves bound zero-length runs, which are never reported.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `13-segment_union_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 2 probes) | PASS, 429,884 checks per configuration, 2 probes, `MEMORY peak=1186MB` |
| same with `CXX=g++-14` | PASS, 429,884 checks per configuration, 2 probes, `MEMORY peak=1193MB` |
| `13-segment_union_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 8,380,122 checks, `MEMORY peak=414MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=654MB` |
| `02-integration.py --sanitizers` | PASS, 167 headers, sanitizer self-tests, `MEMORY peak=1831MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`13-segment_union_benchmark.py --seed 20260927` (2026-10-19; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; both versions must return the same segment multiset):
- random rectangles have sides up to `3 * 10^7` in a `10^9` square;
- the bar lattice is `g` thin horizontal and `g` thin vertical bars, so the output is `Theta(g^2)` segments.

| Case | n | Output | `rectangleUnionBoundary` (µs) | `rectangleUnionBoundaryGuting` (µs) |
|---|---|---|---|---|
| random | 1,000 | 3,900 | 3,479 | 3,524 |
| random | 10,000 | 14,452 | 51,510 | 46,024 |
| random | 100,000 | 1,120 | 682,472 | 497,675 |
| bar lattice | 200 | 40,002 | 896 | 972 |
| bar lattice | 600 | 360,002 | 10,259 | 8,901 |
| bar lattice | 2,000 | 4,000,002 | 88,453 | 127,957 |

The divide and conquer saves the logarithmic factor on the output, but its shared trees cost memory traffic. In this range neither version dominates.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [cp-algorithms: length of the union of segments](https://cp-algorithms.com/geometry/length-of-segments-union.html) and [OI Wiki 扫描线](https://oi-wiki.org/geometry/scanning/): the 1D union and the area and perimeter sweeps (POJ 1177).
- [CSES 1740 Intersection Points](https://cses.fi/problemset/task/1740), [CSES 1741 Area of Rectangles](https://cses.fi/problemset/task/1741) and [suisen geom/segment_intersections.hpp](https://suisen-cp.github.io/cp-library-cpp/library/geom/segment_intersections.hpp): the orthogonal count (closed ranges there too) and the union area.
- [Barbay, Pérez-Lantero, Rojas-Ledesma, Depth distribution in high dimensions](https://arxiv.org/abs/1705.10022): the `O(n^1.5 * log(n))` all-depth distribution, which is not adopted.
- [Güting, Optimal divide-and-conquer to compute measure and contour for a set of iso-rectangles, Acta Informatica 21 (1984) 271–291](https://radhesh-sarma.github.io/DAA_Assignment/assets/Guting%20paper.pdf) and Preparata & Shamos §8.5: the divide-and-conquer contour (implemented from the paper's description, pp. 277–288) and the Lipski–Preparata sweep family the plain version belongs to.
- `OLD/Team Notebook/src/geometry/getlensegunion.cpp`: legacy, superseded. Its events at equal x sorted removals first, which does not affect the length.

## Limits and handoffs

- `rectangleCoverageArea` costs `O(n * k * log(n))`. The whole depth distribution takes `O(n^1.5 * log(n))` (Barbay et al.) and is not adopted.
- `rectangleUnionBoundary` is `O((n + out) * log(n))`; `rectangleUnionBoundaryGuting` reaches the optimal `O(n * log(n) + out)` at `O(n * log(n))` memory. Neither chains the pieces into rings.
- Floating rectangles are not supported; scale them to integers. Online anchored unions (maspypy `incremental_rectangle_union`) belong to Data Structures.

## History

- 2026-10-18: first implementation (P037). Independent review: the documented `4 * 10^18` bounds for segments were removed (lengths are taken in `lll`), `k` is raised to 1 under `NDEBUG`, and the order check of the report test was tightened. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit added `rectangleUnionBoundaryGuting`. Independent review: page references unified, the ASan cap documented. Full suite passed on g++ and g++-14, stress passed.
