# 03 Geometry — notes

Contracts and ownership rules moved out of the inventory. Package evidence stays in [90-foundations.md](90-foundations.md), [91-ge02.md](91-ge02.md) and [92-triangle.md](92-triangle.md).

## Numeric policy

- Integer predicates are exact on stated coordinate bounds. Dot/cross/squared-distance intermediates widen to `lll` before arithmetic; account for the degree of every intermediate before choosing `lll` or arbitrary precision. `|coordinate| <= 10^9` is the shared sufficient domain for rows 01–09, including the cubic tetrahedral determinant and degree-3 rational intersection numerators (`<= 24 * 10^27`).
- Division-based constructions return exact rationals (`RationalPoint2`, gcd-normalized, `d > 0`, no rational ordering) or a separately named `Approx` result in `long double`. Never multiply two rationals inside a predicate.
- Approximate APIs use ordinary long-double arithmetic with computed signs: no hidden epsilon, no certified error bound, finite inputs/intermediates/outputs required, nonzero divisors and geometric distinctions must survive rounding. Only membership queries accept an explicit absolute tolerance. Epsilon equality is never used in sorting or map order; `polarLess` is integral-only because floating determinants break comparator transitivity.
- Exact predicate signs do not make approximate intersections exact; the two guarantees are always separate. Adaptive/filtered floating predicates belong to `36-exact_predicates.hpp`, which may be a dependency of any tier; no ISA intrinsics outside Core.
- Contest-profile geometry is scalar; accelerated arithmetic is only reachable through Core under the shared policy.

## Topology and domains

- Intersection/feasibility results distinguish empty, point, segment/ray/line/plane overlap, coincident and unbounded states. Endpoints are closed; `a == b` is a singleton for every linear kind; zero directions are parallel to everything; two singletons are always collinear.
- Polygon rings have no repeated closing vertex, `n <= INT_MAX`. Area/moments/winding apply algebraically to any closed walk; geometric interpretation needs a simple polygon. Convexity assumes simplicity; Pick needs nonzero area. A polygon with holes is `{outer CCW, holes CW}` with simple, disjoint, non-touching rings. Simplicity validation is owned by `12-segmentintersection.hpp`, not by `03-polygon.hpp`.
- Hull output starts at the lexicographic minimum, proceeds CCW, never repeats its start; duplicates removed; `keep_collinear` retains every distinct boundary point; `convexHullIndices` names the first input occurrence of each hull point. Calipers require a strictly convex CCW hull (`convexHull(..., false)`), accept empty/singleton/segment inputs, and return original indices with lexicographically smallest ties.
- Circles are boundaries; disks are filled sets. Zero radius denotes a point. Intersection counts use `-1` for infinitely many.
- Halfplane and halfspace APIs must not disguise unbounded feasibility with an arbitrary bounding box; unbounded answers keep recession data. Clipping needs a justified finite initial polytope.
- Polygon Boolean and arrangement APIs state multi-polygons, lower-dimensional contacts, touching/overlapping edges, ring orientation and fill rule; they share one arrangement representation.
- Inscribed k-gon optimization (`15`) states k, strict-convexity/collinearity assumptions and its own complexity; no linear-time triangle shortcut is assumed.
- Enumeration/reporting APIs are charged by output size and separate detection, counting and reporting costs.

## Ownership boundaries

- Mathematics: floor sums and lattice-count kernels (`18-floorsum.hpp`, `55-floor_sum_polynomial.hpp`), real/complex root isolation (`29-polynomial_roots.hpp`), ternary/golden search (`02-search_algorithms.hpp`), Simplex LP (`28-simplex.hpp`), Simpson/Gauss quadrature (`17-numerical_methods.hpp`), sums of two squares and Gaussian integers (`57-quadratic_integer.hpp`). Rows 19, 23, 25, 42 and 44 reuse these.
- Data Structures: `43-rangetree.hpp` (orthogonal count/report), `11-fenwick_tree_advanced.hpp`, `33-offline_rectangle_queries.hpp`, `21-lichao.hpp`, `22-convexhulltrick.hpp`, `24-interval_set.hpp`. Geometry owns point-facing kd/R/quadtree and nearest-neighbour APIs and may wrap, never duplicate, those engines. Range-restricted closest pair is owned by `29-point_set_queries.hpp`, which may reuse `28-spatial_index.hpp`.
- Graphs: `29-planar_graph.hpp` owns combinatorial half-edge embeddings, faces and duals; Geometry subdivisions own coordinates and events. Graph MST engines are reused by `22-geometricmst.hpp`; generic DP engines stay in Miscellaneous even when `41` supplies an angular event order.
- `42-randomizedlp.hpp` stays in Geometry: Seidel's fixed-dimension LP is the feasibility engine for `38` and `49`; general LP is Mathematics `28-simplex.hpp`.
- `44-algebraic_geometry.hpp` stays in Geometry but only for conic/arc/Bézier intersections; polynomial root isolation itself is Mathematics.
- Chan's output-sensitive hull is a named `41` variant, not a requirement of `04-convexhull.hpp`.

## Legacy obligations

- Superseded and accounted for by GE01/GE02 (bytes preserved, no active dependency): `OLD/Team Notebook/src/geometry/point.cpp`, `intersection.cpp`, `polyarea.cpp`, `getnearestpair.cpp`, plus their counterparts in `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp`. Historical names (`Point`, `PointD`, `sgn`, `polyArea`, `isIntscLinSeg`) are not compatibility APIs. Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`.
- Pending until their rows are implemented: `halfplane.cpp` (row 10), `getminkowskisum.cpp` (11), `getintersectingsegs.cpp` (12), `getlensegunion.cpp` (13), `welzlmec.cpp` (14), `getmanhattammstedges.cpp` (21). Legacy epsilon predicates (`1e-9`) are not adopted; each successor states its own exact/approximate policy.
- `lichaotree.cpp` in the same directory belongs to Data Structures `21-lichao.hpp`.
- `OLD/Team Notebook/main.tex` lines 232–234 (Heron) are covered by `heronAreaApprox`.
- No correctness claim in any legacy snippet is adopted.

## P007 completeness decisions (2026-10-07)

- Not adopted for `01-point.hpp`: `normalApprox` (it is `unitApprox(perp(v))` or `unitApprox(cross(u, v))`); 3D `phi`/`theta` (spherical coordinates belong to the 3D rows); `operator!=` (synthesized from `==`); clockwise rotation (`-perp`); epsilon `sgn`/`eq` (contradicts the exact-first policy; row 36 owns robust predicates); rotation by an angle (row 08).
- Not adopted for `02-line_segment.hpp`: a line from equation coefficients (integral `Linear2` endpoints need not exist inside the coordinate domain; `canonicalLine` gives the equation in the other direction); five-way `ccw` (`orient` plus `contains` on a ray or segment answers it); epsilon `sideOf` (use `orient` or `signedLineDistanceApprox`).
- Not adopted for `03-polygon.hpp`: `segmentInPolygon` and nonconvex polygon-line pieces (rare and maspypy-only; candidates for the arrangement row 30); `makeCCW` (`signedArea2` plus `reverse`).
- Not adopted for `04-convexhull.hpp`: separate lower/upper hull (internal passes of `convexHull`; row 20 owns range hulls); presorted O(n) hull (the sort is not the bottleneck for one call); Melkman's simple-chain hull (needs a simple input chain, rare in contests); Jarvis, Quickhull and Akl–Toussaint (no better contest bound).

## P008 completeness decisions (2026-10-07)

- Added: `circleRelation` (exact integer five-way relation, AOJ CGL_7_A) to `06-circle.hpp`; `degenerateBox` is listed in the `07-rotatingcalipers.hpp` row because the contest struct exposes it.
- Not adopted for `05-closestpair.hpp`: the minimum-perimeter triangle (a rare extension of the same recursion); the randomized expected-linear grid (it needs floor-hashing, its runtime is hackable with a fixed seed, and the deterministic O(n * log(n)) bound is optimal for comparison-based algorithms); floating-coordinate closest pair (the exact-first policy scales or rounds inputs to integers).
- Not adopted for `06-circle.hpp`: the Apollonian ratio-locus circle (a construction for `24-circle_constructions.hpp`).
- Not adopted for `07-rotatingcalipers.hpp`: two-polygon calipers problems (maximum distance between convex polygons, hull merge bridges, critical support lines, widest separating strip, Grenander distance). They belong to the two-polygon rows 04, 11, 17 and 25.
- Not adopted for `08-coordinate_transform.hpp`: line and direction images (apply the map to two points or use `applyVector`), map equality and how-built predicates (field comparison), and exact rational transforms (no contest need, and they conflict with the `Approx` contract).

## P009 completeness decisions (2026-10-07)

- Added: `Triangle2::angleKind` (exact acute/right/obtuse classification by the smallest vertex dot product; CGAL `angle`).
- Not adopted for `09-triangle.hpp`: an approximate angle kind (a right angle is not decidable from rounded coordinates; use the exact type); vertex angles (`angleApprox` in `01-point.hpp`); altitude and median lengths and feet (one projection or `area2 / side`); orientation, bounded side and transforms of a triangle (rows 02, 03 and 08); Fermat point (row 25).

## Validation rules

- Validate sweeps and queries against brute force on small sets, topology against independent constructions, and invariants such as area/volume conservation.
- Always include repeated/collinear/coplanar points, vertical/coincident segments, cocircular/cospherical sets, exact boundary contacts, holes, reversed orientation, huge coordinates and zero-area/volume results.
- Verified rows record test commands, seeds and configurations in their evidence documents; a row that gains operations becomes `partial` and keeps its evidence link until the additions are verified.
