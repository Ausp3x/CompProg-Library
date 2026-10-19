# 17-polygon_distance.hpp — evidence

Owned by package P036 / GE25. See [10-halfplaneintersection.md](10-halfplaneintersection.md) for the package's other headers. Every operation in the row has a test.

## Contracts

### Numeric policy

Inputs have integral coordinates with `|coordinate| <= 10^9`. The convex functions assert this; the general functions inherit it from the `02-line_segment` primitives. All topology is exact: intersection, containment, separation, and choices among vertices. Distances and witness points are rounded `long double`. Selecting the minimum among edges is also rounded, because exact squared rational distances need about `10^56` for cross-multiplication. The vertices of `D = P ⊕ (−Q)` stay within `2 * 10^9`, so its orientations stay within `3.2 * 10^19`.

### PolygonDistanceApprox

Fields `distance >= 0` and witnesses `p` (in the first polygon) and `q` (in the second). `|p - q|` equals `distance` up to rounding. When the polygons meet, `distance == 0` and `p == q` is a common point.

### convexPolygonDistanceApprox

Input: two nonempty strictly convex CCW rings, any start, a point or two points allowed (asserted as for `11-minkowskisum`). The difference `D` comes from the `11-minkowskisum` merge, which keeps vertex index pairs. This merge is the rotating-calipers sweep over antipodal edge directions. If the origin lies in the closed set `D` (exact orientations), the polygons meet. The common point is the barycentric combination, in `P`'s vertices, of the fan triangle `(D_0, D_k, D_{k+1})` that contains the origin; segment and point cases interpolate. Otherwise the result is the closest point of `D`'s boundary to the origin. Its edge parameter `t` maps back to `p = a_i + t * (a_{i'} - a_i)` and to `q` through the same `t` on `Q`'s chain. When both pointers advance, any split of a combined edge is a valid witness. `O(n + m)`.

### convexSeparated

Same domain. Returns true iff the closed polygons are disjoint (touching counts as meeting). Only when it returns true, it sets `axis` to a primitive integral vector with `min over P of dot(axis, x) > max over Q of dot(axis, y)`. For a polygonal `D` this is the left normal of an edge of `D` with the origin strictly to its right. For a segment `D` it is the normal oriented away from the origin, or `D_0` when `D` is collinear with the origin. For a single-point `D` it is that point. `O(n + m)`.

### convexPolygonMaxDistance

Same domain. Exact farthest pair `(first in p, second in q)` as input indices, with `squared` the exact squared distance and lexicographically smallest indices on ties. It is the maximum of `norm2` over `D`'s vertices. Every maximizing pair `(i, j)` is a vertex of `D`, because a strictly convex function attains its maximum at extreme points and a vertex of a Minkowski sum splits uniquely. `O(n + m)`. It reuses `ConvexDiameter` from `07-rotatingcalipers.hpp`, where `first` and `second` index the two polygons instead of one hull.

### pointPolygonDistanceApprox

Input: a ring or rings (nonempty; a single point allowed), with nonzero-winding fill. Returns 0 when `polygonContains` finds the point inside or on the boundary, else the minimum `pointSegmentDistanceApprox` over every edge of every ring. A point inside a hole therefore measures to the nearest boundary of any ring. Exact topology for integral `T`. `O(n + r)`.

### polygonsIntersect

Input: two simple rings (either orientation, a point or a segment allowed). Returns true iff the closed regions share a point. That happens when some pair of edges touches (exact orientation and closed-segment tests) or when one region contains a vertex of the other. `O(n * m)`.

### polygonDistanceApprox

Same domain. Returns 0, with a common witness from the first touching edge pair or the contained vertex, or else the minimum over vertex-to-edge distances in both directions with their nearest points. The minimum distance between non-crossing segments is attained at an endpoint of one of them. `O(n * m)`.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `convexSeparated(p, q)` | `point axis; convexSeparated(p, q, axis)` |

## Feature-to-test map

`96-Local Testing/03-Geometry/17-polygon_distance_tester.py` provides quick, full and stress modes and five assertion probes: an empty polygon, a clockwise ring, the coordinate bound, an empty point polygon and an empty intersect input. It builds the optimized, checked and ASan/UBSan configurations.

**Oracle.** Meeting is decided by candidate points: every vertex plus every exact edge intersection from `02` `intersect`, tested for closed membership as rational points with degree-4 `lll` terms. Distances come from `02` `segmentDistanceApprox` over all edge pairs. The farthest pair comes from an exact all-pairs search. Point queries use exact rational squared distances, compared with `ConvexCalipers::ratioLess`. Each witness must lie in its polygon (within `1e-15 * scale * 8`) and reproduce the distance.

| Operation | Test | Oracle |
|---|---|---|
| `convexPolygonDistanceApprox`, `PolygonDistanceApprox` | All pairs of a square, a triangle, a point, a segment and a far point; touching, contained and identical polygons; ±`10^9` slabs and a triangle; random hulls at scales 6 and `10^9` with random shifts and rotations; 200,000-vertex disjoint circles (distance `2 * 10^8`) | All-pairs distance, meeting oracle, witness checks |
| `convexSeparated` | The same corpus; the `clockwise` probe | Meeting oracle; exact projection test of the axis; axis untouched when the polygons meet |
| `convexPolygonMaxDistance` | The same corpus; wide slabs (exact `8 * 10^18`) | Exact all-pairs maximum with smallest indices |
| `pointPolygonDistanceApprox` (ring, rings) | Points inside, on the boundary, in a notch and in a hole; random simple polygons at scales 5 and `10^9` | Exact rational minimum |
| `polygonsIntersect` | U-shaped polygons with points and triangles in the slot, outside and enclosing; random 2-opt simple polygons against simple polygons, points and segments, in both argument orders; every convex case | Candidate-point oracle |
| `polygonDistanceApprox` | The same general and convex corpora | All-pairs distance, witness checks |

## Commands and results

2026-10-12, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), Python 3.14.7, GNU++20.

| Command | Result |
|---|---|
| `17-polygon_distance_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 5 probes) | PASS, 127,536 checks per configuration, 5 probes, `MEMORY peak=466MB` |
| same with `CXX=g++-14` | PASS, 127,536 checks per configuration, 5 probes, `MEMORY peak=473MB` |
| `17-polygon_distance_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 952,932 checks, `MEMORY peak=339MB` |
| `02-integration.py` (136 headers, scalar and AVX2 multi-TU builds, workspace) | PASS, `MEMORY peak=648MB` |
| `17-polygon_distance.hpp` alone, `-Wall -Wextra -Wconversion`, g++ and g++-14 | No warnings |
| `03-consistency.py --braces` on the header and tester; `03-consistency.py` | No violations; no errors |

Mutation check (2026-10-12, quick mode on a scratch copy): `orient < 0` changed to `<= 0` in the containment test, and the separating axis left unflipped. Both are caught.

```bash
python3 '96-Local Testing/03-Geometry/17-polygon_distance_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/17-polygon_distance_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/17-polygon_distance_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/02-integration.py'
for c in g++ g++-14; do echo '#include "03-Geometry/17-polygon_distance.hpp"' | $c -std=gnu++20 -Wall -Wextra -Wconversion -fsyntax-only -I. -x c++ -; done
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/17-polygon_distance.hpp '96-Local Testing/03-Geometry/17-polygon_distance_tester.cpp'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required. The convex operations are one linear merge; the general ones are the documented `O(n * m)` scan.

## Sources

Written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [cp-algorithms: Minkowski sum](https://cp-algorithms.com/geometry/minkowski.html): distance between convex polygons as the origin's distance to `P ⊕ (−Q)`.
- [Wikipedia: Rotating calipers](https://en.wikipedia.org/wiki/Rotating_calipers): the catalogue of two-polygon calipers problems (minimum and maximum distance, separation).
- [CGAL Polytope_distance_d](https://doc.cgal.org/latest/Polytope_distance_d/index.html) and [Boolean `do_intersect`](https://doc.cgal.org/latest/Boolean_set_operations_2/group__boolean__do__intersect.html): witness conventions, and closed versus open intersection semantics (CGAL tests interiors; this row tests closed sets).

## Limits and handoffs

- **General functions.** `polygonsIntersect` and `polygonDistanceApprox` are `O(n * m)` by decision on 2026-10-10. The optimal `O((n + m) * log(n + m))` intersection test needs the Shamos–Hoey sweep of row 12 (`anyIntersection`, `polygonSimple`). The optimal general distance needs a constrained Delaunay triangulation or a segment Voronoi diagram (rows 45 and 46). Rewire both functions when those rows are verified.
- **Exact rational squared distance between convex polygons.** Not provided: comparing candidates exceeds 128 bits at `|coordinate| <= 10^9`.
- **Convex point queries.** `O(log(n))` point distance and containment belong to row 15.

## History

- 2026-10-11: first implementation (P036). Independent review: one finding (ring overload memory) fixed; full suite passed on g++ and g++-14, stress passed; `02-integration.py --sanitizers` stopped at an unrelated P025 failure, so the header-alone, aggregate and two-translation-unit checks ran directly and passed.
- 2026-10-12: prune, removed the `convexSeparated(p, q)` wrapper (its cases and the `clockwise` probe moved to the `axis` form); the P007 prune migrated the `onSegment` call site in `polygon_distance_detail::touch` ([02-line_segment.md](02-line_segment.md)); full suite passed on g++ and g++-14, stress passed.
