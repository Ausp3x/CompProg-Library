# 16-circle_polygon.hpp — evidence

Owned by package P036 / GE24. See [10-halfplaneintersection.md](10-halfplaneintersection.md) for the package's other headers. Every operation in the row has a test.

## Contracts

### Numeric policy

Every operation is approximate. It takes a `CircleApprox` (finite center, radius `r >= 0`, asserted by its constructor) and integral or floating polygon points. Arithmetic is ordinary rounded `long double`, with computed signs, no hidden epsilon and no certified bound. The GE02 policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. The two topological decisions in `circleArcsInPolygonApprox` use exact equality. A vertex lies on the circle when `norm2(v - c) == r * r`, and a tangency is `r^2 * |d|^2 - cross^2 == 0`. Both quantities are exact in `long double` (all products below `2^64`) when the center, the radius and the vertices are integers, `r <= 2^14` and every coordinate differs from the center by at most `2^14`; touches are then classified exactly. Elsewhere touches, and near-tangent crossings, are computed classifications. For example, the reviewer's case at `r = 484968289` misses a true `2.9e-10` rad arc.

### diskTriangleAreaApprox

Signed area of the disk intersected with the triangle `(c.c, a, b)`. It is positive when `c.c, a, b` turn CCW and zero for a degenerate triangle or `r = 0`. On each edge the segment is clipped to the chord interval `[m - s, m + s]` of the line through the disk. The result sums circular-sector terms `r^2 / 2 * atan2(cross, dot)` on the outer parts and the triangle term `cross(u, v) / 2` on the inner chord. `O(1)`.

### diskPolygonAreaApprox, diskRingsAreaApprox

`diskPolygonAreaApprox(c, ring)` sums the fan terms over the edges of a closed walk (no repeated closing vertex). The result is the signed area: positive for a CCW simple ring, negative for a CW one. For any closed walk it is the winding-weighted area inside the disk. `diskRingsAreaApprox(c, rings)` sums the rings, so `{outer CCW, holes CW}` gives the area of the disk intersected with the polygon with holes. The row's "signed, holes" item is the signed ring form combined through rings, and "rings" means polygon rings (the 03-polygon convention), not annuli. `O(n + r)`.

### circleArcsInPolygonApprox

Input: a polygon given as a ring or as rings, nonzero winding counting as inside (consistent with `polygonContains`), and `r > 0` (asserted). Output: the circle's arcs inside the polygon as CCW angle pairs `(begin, end)` about `c.c`, with `begin` in `[-π, π)` and `begin < end <= begin + 2π`. The arcs are sorted by `begin` and disjoint, and touching points do not split an arc. A circle entirely inside gives `{(-π, π)}`; a circle entirely outside gives no arcs.

Each crossing contributes an event, a winding step of ±1 by the sign of `dot(edge direction, crossing - c.c)`. The number of crossings on an edge comes from its endpoint statuses (inside, on, outside), each computed once per vertex, so adjacent edges always agree. A vertex on the circle counts as inside a radius `r + ε` circle; edges leaving or arriving tangentially there produce a step at the vertex. An outside-outside edge crosses twice exactly when its closest point lies inside the segment and the discriminant is positive. Roots are clamped to the segment and used only for angles. Touches (on-circle vertices and tangencies) add zero steps. That keeps the reference angle, the midpoint of the largest gap between event angles, off the polygon's boundary. A single `polygonWinding` query there seeds the sweep. Arcs carry the original event angles; only a `begin` of exactly π is shifted to −π. `O(n * log(n))` time, `O(n)` memory.

Near-degenerate rounded inputs can split or join an arc across a gap of about `1e-19` rad. The same happens when two edges cross the circle at one point that is not a shared vertex, for example rings crossing each other on the circle. No epsilon merges such events, by the numeric policy. A single simple integral ring within the exact range above produces no slivers, and the tests check that.

## Feature-to-test map

`96-Local Testing/03-Geometry/16-circle_polygon_tester.py` provides quick, full and stress modes and two assertion probes (zero radius for arcs, negative radius). It builds the optimized, checked and ASan/UBSan configurations.

**Area oracle.** Polar integration about the center. For a ray at angle `t`, the winding intervals come from walking the edge crossings in from infinity, and the integrand is the winding-weighted `sum w * (min(s2, r)^2 - min(s1, r)^2) / 2`, which is the signed area that the contract specifies, including overlapping rings. The integral runs over pieces between vertex angles and crossing angles found by the verified `06-circle` `circleSegmentIntersectionApprox`. Each piece gets adaptive bisection with 16-point Gauss–Legendre, which is needed because edges passing near the center make the integrand nearly singular. The fixed-rule version of this oracle was 4e-4 off on such a case; the header agreed with a 2·10⁷-sample midpoint sum to 1.6e-13. Tolerance `1e-13 * scale^2`.

**Arc oracle.** Membership by `polygonContains` at midpoints between consecutive breakpoints. Both lists are tidied before comparison: slivers and hairline gaps under `1e-12` rad are dropped, and comparison is modulo the ±π branch cut. Endpoints must then agree within `1e-12`. Integral inputs must already be free of slivers.

| Operation | Test | Oracle |
|---|---|---|
| `diskTriangleAreaApprox` | Random fans with the center at a vertex, including zero radius and degenerate triangles; antisymmetry under `a <-> b` | Polar integration |
| `diskPolygonAreaApprox` | Inscribed (π) and circumscribed (4) circles of a square; zero radius; disjoint; clockwise negative; empty; random simple polygons with integral and real circles; Pythagorean polygons with vertices on the circle | Exact values, polar integration |
| `diskRingsAreaApprox` | A square hole inside a disk (`9π - 4`); fixed and random holes | Exact value, sum of rings, polar integration |
| `circleArcsInPolygonApprox` | Full circle; outside; the tangent sides of the rectangle `[-5, 5] x [-3, 3]` with `r = 5` (exact `asin(0.6)` arcs); a box inscribed in the circle; a diamond, a star and a triangle touching at vertices; a wedge from the center; a near-degenerate `r = sqrt(2)` hole; a diamond hole touching the circle from inside (full arc); two rings crossing on the circle at (3, 4); random and Pythagorean cases; ring equals rings; a 200,000-vertex star with one arc per spike | Midpoint membership, exact values |

## Commands and results

2026-10-11, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.7, GNU++20.

| Command | Result |
|---|---|
| `16-circle_polygon_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 2 probes) | PASS, 92,058 checks per configuration, 2 probes, `MEMORY peak=547MB` |
| same with `CXX=g++-14` | PASS, 92,058 checks per configuration, 2 probes, `MEMORY peak=543MB` |
| `16-circle_polygon_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 684,333 checks, `MEMORY peak=381MB` |
| `02-integration.py --sanitizers` | Stops at its first failure, outside P036: `96-Local Testing/01-Core/16-poly_tester.cpp` (P025) needs its second unit `16-poly_tester_tu.cpp`, which the script does not link. For this package the same checks were run directly: header alone for `10`, `11`, `16`, `17`, `98-basic` and `99-all`, and a two-translation-unit program using every operation, with `g++` and `g++-14` under `-Wall -Wextra -Wconversion` and with ASan/UBSan. All pass with no warnings |
| `03-consistency.py --braces` on the four headers, testers and benchmark; `03-consistency.py` | No violations; no errors |

Mutation check (2026-10-11), optimized full suite on a copy of the tree. 5 mutants.

- **Caught (3):**
  - the tangency branch of outside-outside edges dropped;
  - the tangent zero step dropped;
  - on-circle vertex zero steps dropped, caught by the regression with a diamond hole touching the circle from inside.
- **Equivalent (2):**
  - `dot >= 0` changed to `> 0` at an on-circle vertex: at an exact tangency the clamped root equals the vertex angle.
  - The chord end nudged by `1e-12`: the area is stationary at the true crossing, so the effect is second order.

```bash
python3 '96-Local Testing/03-Geometry/16-circle_polygon_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/16-circle_polygon_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/16-circle_polygon_tester.py' --mode stress --seed 42
```

## Benchmarks

None required: the areas are `O(n)` and the arc sweep is one sort.

## Sources

Written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [KACTL `CirclePolygonIntersection.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/CirclePolygonIntersection.h): the edge-by-edge fan decomposition with chord clipping.
- [ei1333 `geometry/common_area_cp.hpp`](https://ei1333.github.io/library/geometry/common_area_cp.hpp) (verified on AOJ CGL_7_H) and tko919 `Geometry/geometry.hpp`: the same decomposition.
- [AOJ CGL_7_H](https://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=CGL_7_H): problem domain (CCW, not necessarily convex).

## Limits and handoffs

- A region-valued disk intersected with a polygon (arcs plus segments) belongs to row 30 or a CGAL-style circle-segment boolean, not to this row.
- Unions of circles belong to row 23.
- Perimeter of a polygon inside a disk, and circle–polygon intersection point listing: `06-circle` `circleSegmentIntersectionApprox` applied per edge.

## History

- 2026-10-11: first implementation (P036). Independent review: three findings fixed (exactness range of touch classification narrowed, sliver claim limited to single simple rings, a two-line body comment).
