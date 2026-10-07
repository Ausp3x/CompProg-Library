# 03 Geometry — notes

Contracts and ownership rules moved out of the inventory. Evidence is one document per header: [01-point.md](01-point.md), [02-line_segment.md](02-line_segment.md), [03-polygon.md](03-polygon.md), [04-convexhull.md](04-convexhull.md), [05-closestpair.md](05-closestpair.md), [06-circle.md](06-circle.md), [07-rotatingcalipers.md](07-rotatingcalipers.md), [08-coordinate_transform.md](08-coordinate_transform.md) and [09-triangle.md](09-triangle.md).

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

## GE01 numeric policy (P007)

Applies to `01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`.

- `Point2<T>` and `Point3<T>` accept signed integral and floating `T`. Integral dot products, determinants, squared distances and coordinate differences inside predicates widen to `lll` before any arithmetic. Every intermediate must fit `lll`, and vector arithmetic must fit the stored `T`. Coordinates with `|x| <= 10^9` are sufficient for every operation in the package, including the cubic `tetraVolume6`, the degree-3 intersection numerators (`<= 24 * 10^27`) and the polygon first moments (terms `<= 4 * 10^27`, summable over `INT_MAX` vertices).
- Exact line topology, `canonicalLine` and the lattice counts require integral coordinates with `|x| <= 10^9`, and they assert this bound. Point primitives, hulls and the other integral polygon routines also accept wider coordinates as long as every intermediate fits `lll`.
- Floating inputs and every floating intermediate and result must be finite. These routines use ordinary rounded arithmetic and computed signs. They use no epsilon, no error certificate and no adaptive precision, so near cancellation their topology can differ from exact real arithmetic. `polarLess` and the hulls are integral-only because floating rounding can break a sort's strict weak ordering. Robust floating predicates belong to P128 / GE11.
- An API ending in `Approx` returns `long double` values computed with ordinary rounding and no certified error bound. An approximate construction returns a line as two points (`array<dpoint, 2>`); the left side of a line is the positive side.

## P007 / GE01 package record

- Scope: `01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`. Prerequisite: P002/C01's current template. The implementations use ordinary scalar GNU C++20 and do not depend on a Core rational type or on the later exact-predicate package (P128 / GE11).
- 2026-10-07 re-audit: every operation in the four rows was compared against the code and tests before any change. Gaps: the ten operations listed as `missing` (`canonicalDirection`, `angleApprox`, `signedAngleApprox`, `orthogonal`, `lineDistanceApprox`, `perpendicularBisectorApprox`, `angleBisectorApprox`, `reflectDirectionApprox`, `latticeOnSegment`, `gridCellsCrossed`) and untested `point`/`dpoint` aliases. The completeness sweep (four `@researcher` passes, sources in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07)) added `unitApprox`, `argApprox`, `canonicalLine`, `polygonCutApprox` and `convexHullIndices` (the package summary counted six additions; these five are the ones named). Every operation in all four rows now has a test. Per-header findings, commands and results are in the four evidence documents.
- Testing harness: each of the four Python entries under `96-Local Testing/03-Geometry` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses; full mode runs optimized, checked and ASan/UBSan builds.
- Independent review (2026-10-07) confirmed three defects, all fixed: the `signedAngleApprox` negative-zero case (see `01-point.md`), unescaped `|` in the package feature map that broke the table width check, and pre-existing `-Wsign-compare` warnings in five polygon asserts (see `03-polygon.md`). The shared geometry runner still compiles without warning flags, so the explicit warning sweep is the warning evidence. The grouped complexity comment (finding 7) follows the restyle practice and passes the validator; the decision log does not record it yet.
- GCC 14 was unavailable for both the 2026-09-27 and 2026-10-07 runs; no GCC 14 run is claimed.

## GE02 numeric policy (P008)

Applies to `05-closestpair.hpp`, `06-circle.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`.

- Exact APIs: `closestPair`, `closestPair3`, `circleRelation`, every support/optimum choice of `ConvexCalipers`, and `farthestPair`. They use signed integral coordinates, and every predicate widens to `lll` before arithmetic.
- An API ending in `Approx` uses ordinary `long double` arithmetic. Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding. Classification uses computed comparisons with no hidden epsilon and no certified topology near degeneracy. Only membership queries (`locate`, `onBoundary`, `contains`) accept an explicit absolute length tolerance `>= 0`; every tolerance defaults to zero. Approximate constructions return a line as two points; its left side is the positive side.
- Counts use `-1` for infinitely many. In a fixed-size result array, only the prefix `[0, count)` is meaningful.

## P008 / GE02 package record

- Scope: `05-closestpair.hpp`, `06-circle.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`. Prerequisites: the verified P002/C01 template and P007/GE01 geometry foundations.
- 2026-10-07 re-audit: before any edit, the operations in the four rows were compared against the code and tests. The gaps were the five `missing` operations (`closestPair3`, `circleSegmentIntersectionApprox`, `powerApprox`, `radicalAxisApprox`, `farthestPair`), the untested degenerate caliper witnesses, the circle-line classification at non-axis tangencies, and contract prose that claimed nothing was missing. The baseline full run reproduced the 2026-09-27 counts exactly. A completeness sweep (four `@researcher` passes) added `circleRelation`. Every confirmed finding was fixed and the header contracts moved into the evidence documents; per-header findings, commands and results are in the four evidence documents.
- Mutation checks applied each mutation temporarily, ran the optimized full suite and restored the header; the caught and equivalent survivors are listed per header.
- Independent review (2026-10-07) reproduced every full count and probe and confirmed one defect (the `circleRelation` domain assert, see `06-circle.md`). Its second item (the package was not yet recorded as verified) was closed by the 2026-10-07 record.
- Remaining owners: GE11 (adaptive and exact floating predicates), row 24 / GE29 (algebraic circle constructions, including the Apollonian circle), GE05 (minimum enclosing circles and convex polygon queries), GE10 and GE24 (circle unions and circle-polygon operations), rows 04, 11, 17 and 25 (two-polygon calipers problems), GE25 (polygon distance), GE31 (3D frames), row 29 (range-restricted closest pair). No claim of universally certified floating geometry is made.

## Validation rules

- Validate sweeps and queries against brute force on small sets, topology against independent constructions, and invariants such as area/volume conservation.
- Always include repeated/collinear/coplanar points, vertical/coincident segments, cocircular/cospherical sets, exact boundary contacts, holes, reversed orientation, huge coordinates and zero-area/volume results.
- Verified rows record test commands, seeds and configurations in their evidence documents; a row that gains operations becomes `partial` and keeps its evidence link until the additions are verified.
