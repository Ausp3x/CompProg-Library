# 016-circle_polygon.hpp — Evidence

Owned by package P036 / GE24; the package's other headers are listed in [010-halfplaneintersection.md](010-halfplaneintersection.md). Every operation in the row has a test.

## Contracts

### Numeric Policy

Every operation is approximate. It takes a `CircleApprox` (finite center, radius `r >= 0`, asserted by its constructor) and integral or floating polygon points, and computes in ordinary rounded `long double` with computed signs, no hidden epsilon and no certified bound; the GE02 policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. The two topological decisions in `circleArcsInPolygonApprox` use exact equality: a vertex lies on the circle when `norm2(v - c) == r * r`, and a tangency is `r^2 * |d|^2 - cross^2 == 0`. Both quantities are exact in `long double` (all products below `2^64`) when the center, the radius and the vertices are integers, `r <= 2^14` and every coordinate differs from the center by at most `2^14`; touches are then classified exactly. Elsewhere touches and near-tangent crossings are computed classifications; for example, the reviewer's case at `r = 484968289` misses a true `2.9e-10` rad arc.

### diskTriangleAreaApprox

The signed area of the disk intersected with the triangle `(c.c, a, b)`: positive when `c.c, a, b` turn CCW, zero for a degenerate triangle or `r = 0`. On each edge the segment is clipped to the chord interval `[m - s, m + s]` of its line through the disk. The result sums circular-sector terms `r^2 / 2 * atan2(cross, dot)` on the outer parts and the triangle term `cross(u, v) / 2` on the inner chord. `O(1)`.

### diskPolygonAreaApprox, diskRingsAreaApprox

`diskPolygonAreaApprox(c, ring)` sums the fan terms over the edges of a closed walk (no repeated closing vertex). The result is the signed area: positive for a CCW simple ring, negative for a CW one, and for any closed walk the winding-weighted area inside the disk. `diskRingsAreaApprox(c, rings)` sums the rings, so `{outer CCW, holes CW}` gives the area of the disk intersected with the polygon with holes. Rings are polygon rings (the 003-polygon convention), not annuli. `O(n + r)`.

### circleArcsInPolygonApprox

Input: a polygon as a ring or as rings, nonzero winding counting as inside (consistent with `polygonContains`), and `r > 0` (asserted). Output: the circle's arcs inside the polygon as CCW angle pairs `(begin, end)` about `c.c`, with `begin` in `[-π, π)` and `begin < end <= begin + 2π`, sorted by `begin` and disjoint; touching points do not split an arc. A circle entirely inside gives `{(-π, π)}`, one entirely outside no arcs.

Each crossing contributes an event, a winding step of ±1 by the sign of `dot(edge direction, crossing - c.c)`. The number of crossings on an edge comes from its endpoint statuses (inside, on, outside), each computed once per vertex, so adjacent edges always agree. A vertex on the circle counts as inside a radius `r + ε` circle; edges leaving or arriving tangentially there produce a step at the vertex. An outside-outside edge crosses twice exactly when its closest point lies inside the segment and the discriminant is positive. Roots are clamped to the segment and used only for angles. Touches (on-circle vertices and tangencies) add zero steps, which keeps the reference angle, the midpoint of the largest gap between event angles, off the boundary. A single `polygonWinding` query there seeds the sweep. Arcs carry the original event angles; only a `begin` of exactly π is shifted to −π. `O(n * log(n))` time, `O(n)` memory.

Near-degenerate rounded inputs can split or join an arc across a gap of about `1e-19` rad. The same happens when two edges cross the circle at one point that is not a shared vertex, for example rings crossing each other on the circle. By the numeric policy no epsilon merges such events. A single simple integral ring within the exact range above produces no slivers, and the tests check that.

### Renames

| Planned name | Implemented as | Reason |
|---|---|---|
| `circleBoundaryArcsInPolygon` | `circleArcsInPolygonApprox` | Naming policy: rounded `long double` angles |

## Feature-to-Test Map

`96-Local Testing/03-Geometry/016-circle_polygon_tester.py` provides quick, full and stress modes in the optimized, checked and ASan/UBSan configurations, with two assertion probes (zero radius for arcs, negative radius).

**Area oracle.** Polar integration about the center. For a ray at angle `t`, the winding intervals come from walking the edge crossings in from infinity, and the integrand is the winding-weighted `sum w * (min(s2, r)^2 - min(s1, r)^2) / 2`, the signed area the contract specifies, overlapping rings included. The integral runs over pieces between vertex angles and crossing angles found by the verified `006-circle` `circleSegmentIntersectionApprox`. Each piece gets adaptive bisection with 16-point Gauss–Legendre, needed because edges passing near the center make the integrand nearly singular: a fixed-rule version was 4e-4 off on such a case, where the header agreed with a 2·10⁷-sample midpoint sum to 1.6e-13. Tolerance `1e-13 * scale^2`.

**Arc oracle.** Membership by `polygonContains` at midpoints between consecutive breakpoints. Both lists are tidied before comparison: slivers and hairline gaps under `1e-12` rad are dropped, and comparison is modulo the ±π branch cut. Endpoints must then agree within `1e-12`. Integral inputs must already be free of slivers.

| Operation | Test | Oracle |
|---|---|---|
| `diskTriangleAreaApprox` | Random fans with the center at a vertex, including zero radius and degenerate triangles; antisymmetry under `a <-> b` | Polar integration |
| `diskPolygonAreaApprox` | Inscribed (π) and circumscribed (4) circles of a square; zero radius; disjoint; clockwise negative; empty; random simple polygons with integral and real circles; Pythagorean polygons with vertices on the circle | Exact values, polar integration |
| `diskRingsAreaApprox` | A square hole inside a disk (`9π - 4`); fixed and random holes | Exact value, sum of rings, polar integration |
| `circleArcsInPolygonApprox` | Full circle; outside; the tangent sides of the rectangle `[-5, 5] x [-3, 3]` with `r = 5` (exact `asin(0.6)` arcs); a box inscribed in the circle; a diamond, a star and a triangle touching at vertices; a wedge from the center; a near-degenerate `r = sqrt(2)` hole; a diamond hole touching the circle from inside (full arc); two rings crossing on the circle at (3, 4); random and Pythagorean cases; ring equals rings; a 200,000-vertex star with one arc per spike | Midpoint membership, exact values |

A hand mutation check (2026-10-11, optimized full suite on a copy of the tree) tried 5 mutants. Three were caught: the tangency branch of outside-outside edges dropped, the tangent zero step dropped, and the on-circle vertex zero steps dropped (caught by the diamond hole touching the circle from inside). Two are equivalent: `dot >= 0` changed to `> 0` at an on-circle vertex (at an exact tangency the clamped root equals the vertex angle), and the chord end nudged by `1e-12` (the area is stationary at the true crossing, so the effect is second order).

## Commands and Results

Package runs 2026-10-11: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.7, GNU++20. Later runs recorded only pass or fail; the library-wide rows are the latest results of those commands (2026-11-10, P274; seed 42, beside the other suites, build cache emptied before each cold run).

| Command | Result |
|---|---|
| 2026-10-11: `016-circle_polygon_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 92,058 checks per configuration, 2 probes, `MEMORY peak=547MB` |
| 2026-10-11: same with `CXX=g++-14` | PASS, 92,058 checks per configuration, 2 probes, `MEMORY peak=543MB` |
| 2026-10-11: `016-circle_polygon_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 684,333 checks, `MEMORY peak=381MB` |
| 2026-10-11: `02-integration.py --sanitizers` | Stopped at a failure outside P036 (`96-Local Testing/01-Core/16-poly_tester.cpp` of P025 needs its second unit `16-poly_tester_tu.cpp`, which the script did not link). The same checks ran directly and passed with no warnings: each of `10`, `11`, `16`, `17`, `998-basic` and `999-all` alone, and a two-translation-unit program using every operation, with `g++` and `g++-14` under `-Wall -Wextra -Wconversion` and with ASan/UBSan |
| 2026-10-11: `03-consistency.py --braces` on the four headers, testers and benchmark; `03-consistency.py` | No violations; no errors |
| 2026-11-06 P253: `01-run.py --filter 03-Geometry` quick, full and `CXX=g++-14` full; `02-integration.py` | PASS (counts not recorded) |
| 2026-11-10 P274: `01-run.py --mode full` (GCC 16.2.1) | PASS (counts not recorded); pre-flight quick peak 879 MB; optimized 0.1 s compile + 10.1 s run, checked 21.4 s + 59.0 s, ASan/UBSan 21.9 s + 67.3 s; MEMORY peak 1096 MB |
| 2026-11-10 P274: `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS (counts not recorded); pre-flight quick peak 942 MB; optimized 0.2 s compile + 11.6 s run, checked 25.5 s + 85.0 s, ASan/UBSan 23.4 s + 73.9 s; MEMORY peak 1040 MB |
| 2026-11-10 P274: `01-run.py --mode quick` | PASS (counts not recorded); optimized 31.3 s compile + 0.8 s run, checked 24.8 s + 15.8 s; MEMORY peak 880 MB |
| 2026-11-10 P274: `01-run.py --mode stress --rounds 1` | PASS (counts not recorded); pre-flight quick peak 907 MB; optimized 0.1 s compile + 32.2 s run, checked 10.3 s + 276.8 s, ASan/UBSan 11.1 s + 220.3 s; MEMORY peak 721 MB |

```bash
python3 '96-Local Testing/03-Geometry/016-circle_polygon_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/016-circle_polygon_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/016-circle_polygon_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

None required: the areas are `O(n)` and the arc sweep is one sort.

## Sources

Written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [KACTL `CirclePolygonIntersection.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/CirclePolygonIntersection.h): the edge-by-edge fan decomposition with chord clipping.
- [ei1333 `geometry/common_area_cp.hpp`](https://ei1333.github.io/library/geometry/common_area_cp.hpp) (verified on AOJ CGL_7_H) and tko919 `Geometry/geometry.hpp`: the same decomposition.
- [AOJ CGL_7_H](https://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=CGL_7_H): problem domain (CCW, not necessarily convex).

## Limits and Handoffs

- A region-valued disk intersected with a polygon (arcs plus segments) belongs to row 30 or a CGAL-style circle-segment boolean, not to this row.
- Unions of circles belong to row 23.
- The perimeter of a polygon inside a disk and the listing of circle–polygon intersection points are `006-circle` `circleSegmentIntersectionApprox` applied per edge.
- The row is verified but holds no `04-mutation.py` record; P036's `/finish` adds one.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 圆与多边形面积交, 圆与三角形面积交, 圆与多边形交.

## History

- 2026-10-11: first implementation (P036). Independent review: three findings fixed (exactness range of touch classification narrowed, sliver claim limited to single simple rings, a two-line body comment).
- 2026-10-12: P007 prune migrated `onSegment` call sites in the tester; full suite passed ([002-line_segment.md](002-line_segment.md)).
- 2026-11-06: P253 `/finish` (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename Polygon = vector<point>>`), so `grab` of a function brings that type and calls deduce as before; `01-run.py --filter 03-Geometry` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; inputs and checks unchanged; library-wide runs passed.
