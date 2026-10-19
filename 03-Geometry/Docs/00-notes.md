# 03 Geometry — notes

Evidence is one document per header: [01-point.md](01-point.md), [02-line_segment.md](02-line_segment.md), [03-polygon.md](03-polygon.md), [04-convexhull.md](04-convexhull.md), [05-closestpair.md](05-closestpair.md), [06-circle.md](06-circle.md), [07-rotatingcalipers.md](07-rotatingcalipers.md), [08-coordinate_transform.md](08-coordinate_transform.md), [09-triangle.md](09-triangle.md), [10-halfplaneintersection.md](10-halfplaneintersection.md), [11-minkowskisum.md](11-minkowskisum.md), [13-segment_union.md](13-segment_union.md), [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md), [15-convex_polygon_query.md](15-convex_polygon_query.md), [16-circle_polygon.md](16-circle_polygon.md), [17-polygon_distance.md](17-polygon_distance.md), [19-lattice_geometry.md](19-lattice_geometry.md) and [54-fixed_radius_queries.md](54-fixed_radius_queries.md).

## Numeric policy

- Integer predicates are exact on stated coordinate bounds. Integral dot products, determinants, squared distances and coordinate differences inside predicates widen to `lll` before any arithmetic; every intermediate must fit `lll` and vector arithmetic must fit the stored `T`. Account for the degree of every intermediate before choosing `lll` or arbitrary precision. `|coordinate| <= 10^9` is the shared sufficient domain for rows 01–09.
- Division-based constructions return exact rationals (`RationalPoint2`, gcd-normalized, `d > 0`, no rational ordering) or a separately named `Approx` result in `long double`. Never multiply two rationals inside a predicate.
- Floating arithmetic (floating `T`, and every API ending in `Approx`, which returns `long double`) uses ordinary rounding with computed signs and comparisons: no hidden epsilon, no certified error bound and no adaptive precision, so near cancellation its topology can differ from exact real arithmetic. Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding.
- Only membership queries accept an explicit absolute length tolerance `>= 0` (row 06: `locate`, `onBoundary`, `contains`, each defaulting to zero). Epsilon equality is never used in sorting or map order; `polarLess` and the hulls are integral-only because floating rounding breaks a comparator's strict weak ordering. In rows 01–08 an approximate construction returns a line as two points (`array<dpoint, 2>`) whose left side is the positive side.
- Exact predicate signs do not make approximate intersections exact; the two guarantees are always separate. Adaptive/filtered floating predicates belong to `36-exact_predicates.hpp`, which may be a dependency of any tier.

### GE01 numeric policy (P007)

Rows 01–04 (`01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp`, `04-convexhull.hpp`):

- `Point2<T>` and `Point3<T>` accept signed integral and floating `T`. `|x| <= 10^9` is sufficient for every operation, including the cubic `tetraVolume6`, the degree-3 intersection numerators (`<= 24 * 10^27`) and the polygon first moments (terms `<= 4 * 10^27`, summable over `INT_MAX` vertices).
- Exact line topology, `canonicalLine` and the lattice counts require integral coordinates with `|x| <= 10^9`, and they assert this bound. Point primitives, hulls and the other integral polygon routines also accept wider coordinates as long as every intermediate fits `lll`.

### GE02 numeric policy (P008)

Rows 05–08 (`05-closestpair.hpp`, `06-circle.hpp`, `07-rotatingcalipers.hpp`, `08-coordinate_transform.hpp`): the exact APIs are `closestPair`, `closestPair3`, `circleRelation`, every support/optimum choice of `ConvexCalipers` and `farthestPair`, on signed integral coordinates; every other API is an `Approx` API under the policy above.

## Topology and domains

- Intersection/feasibility results distinguish empty, point, segment/ray/line/plane overlap, coincident and unbounded states. Endpoints are closed; `a == b` is a singleton for every linear kind; zero directions are parallel to everything; two singletons are always collinear.
- Polygon rings have no repeated closing vertex, `n <= INT_MAX`. Area/moments/winding apply algebraically to any closed walk; geometric interpretation needs a simple polygon. Convexity assumes simplicity; Pick needs nonzero area. A polygon with holes is `{outer CCW, holes CW}` with simple, disjoint, non-touching rings. Simplicity validation is owned by `12-segmentintersection.hpp`, not by `03-polygon.hpp`.
- Hull output starts at the lexicographic minimum, proceeds CCW, never repeats its start; duplicates removed; `keep_collinear` retains every distinct boundary point; `convexHullIndices` names the first input occurrence of each hull point. Calipers require a strictly convex CCW hull (`convexHull(..., false)`), accept empty/singleton/segment inputs, and return original indices with lexicographically smallest ties.
- Circles are boundaries; disks are filled sets. Zero radius denotes a point. Intersection counts use `-1` for infinitely many; in a fixed-size result array only the prefix `[0, count)` is meaningful.
- Halfplane and halfspace APIs must not disguise unbounded feasibility with an arbitrary bounding box; unbounded answers keep recession data. Clipping needs a justified finite initial polytope.
- Polygon Boolean and arrangement APIs state multi-polygons, lower-dimensional contacts, touching/overlapping edges, ring orientation and fill rule; they share one arrangement representation.
- Reporting APIs separate detection, counting and reporting costs.
- Degeneracy policies are row contracts, not operations: coincident and vertical segments (`12-segmentintersection.hpp`, including `multiwayCrossing`), cocircular sites (`31-delaunay.hpp`), tangent sight lines (`39-visibilitygraph.hpp`), simultaneous kinetic events (`40-kinetic_geometry.hpp`) and cospherical sites (`47-delaunay3d.hpp`).

## Ownership boundaries

- Data Structures: `43-rangetree.hpp` (orthogonal count/report), `11-fenwick_tree_advanced.hpp`, `33-offline_rectangle_queries.hpp`, `21-lichao.hpp`, `22-convexhulltrick.hpp`, `24-interval_set.hpp`. Geometry owns point-facing kd/R/quadtree and nearest-neighbour APIs and reuses those engines without wrapping or duplicating them (`00-Guidelines/03-cpp.md`, One API per operation). Range-restricted closest pair is owned by `29-point_set_queries.hpp`, which may reuse `28-spatial_index.hpp`.
- Graphs: `29-planar_graph.hpp` owns combinatorial half-edge embeddings, faces and duals; Geometry subdivisions own coordinates and events, and `33-linearrangement.hpp` exports to it instead of duplicating face logic. Graph MST engines are reused by `22-geometricmst.hpp`; generic DP engines stay in Miscellaneous even when `41` supplies an angular event order.
- Mathematics: floor sums and lattice-count kernels (`18-floorsum.hpp`, `55-floor_sum_polynomial.hpp`), real and complex root isolation (`29-polynomial_roots.hpp`), ternary/golden search (`02-search_algorithms.hpp`), Simplex LP (`28-simplex.hpp`), numerical integration (`17-numerical_methods.hpp`), sums of two squares (`34-quadratic_congruence.hpp`) and Gaussian integers (`57-quadratic_integer.hpp`); rows 19, 23, 25, 42 and 44 reuse them and do not duplicate them.
- `42-randomizedlp.hpp` stays in Geometry: Seidel's fixed-dimension LP is the feasibility engine for `38` and `49`; general LP is Mathematics `28-simplex.hpp`.
- `44-algebraic_geometry.hpp` stays in Geometry but only for conic/arc/Bézier intersections; polynomial root isolation itself is Mathematics.
- Chan's output-sensitive hull is a named `41` variant, not a requirement of `04-convexhull.hpp`.
- Python: `08-Python/_11_geometry.py` (rows 01–04, 15), `_41_geometry_float.py` (01, 02, 06, 08, 14), `_44_exact_geometry.py` (12) and `_55_geometry_advanced.py` (05, 07, 10, 11, 13, 15) are documented reductions.

## Legacy obligations

- Superseded and accounted for by GE01/GE02 (bytes preserved, no active dependency): `OLD/Team Notebook/src/geometry/point.cpp`, `intersection.cpp`, `polyarea.cpp`, `getnearestpair.cpp`, plus their counterparts in `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp`. Historical names (`Point`, `PointD`, `sgn`, `polyArea`, `isIntscLinSeg`) are not compatibility APIs. Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`.
- Superseded and accounted for by P036 (bytes preserved, no active dependency): `halfplane.cpp` (row 10; its `±2 * 10^9` box and `1e-9` predicates are not adopted) and `getminkowskisum.cpp` (row 11).
- Superseded and accounted for by P037 (bytes preserved, no active dependency): `getlensegunion.cpp` (row 13 `segmentUnionLength`) and `welzlmec.cpp` (row 14; its double predicates and clock seed are not adopted).
- Pending until their rows are implemented: `getintersectingsegs.cpp` (12), `getmanhattammstedges.cpp` (21). Legacy epsilon predicates (`1e-9`) are not adopted; each successor states its own exact/approximate policy.
- `lichaotree.cpp` in the same directory belongs to Data Structures `21-lichao.hpp`.
- No correctness claim in any legacy snippet is adopted.

## 2026-09-27 audit: not adopted

Industrial CAD, molecular surfaces, remeshing/visualization and machine-learning reconstruction remain outside this contest inventory. Catalog items of the [2026-09-27 audit](00-sources.md#2026-09-27-audit-provenance) left out:

- maspypy `minimum_three_distance_sum`: single-problem curiosity without a reusable family.
- maspypy `convex_polygon_edge_voronoi`: nearest-edge diagram of a convex polygon; no contest use case beyond `15` distance queries.
- maspypy `definite_integral`, `modint_real`: numerical integration is Mathematics `17-numerical_methods.hpp`; modular reals are not geometry.
- hitonanode `point2d_w_error` as a public type: absorbed as the `IntervalPoint2` filter inside row 36 rather than a separate header.
- KACTL `Angle` class: `polarLess` and `canonicalDirection` cover its use; no separate angle type.
- cp-algorithms "Finding faces of a planar graph": Graphs `29-planar_graph.hpp`.
- CGAL 2D Snap Rounding, Polygon Repair, Movable Separability, Interval Skip List, natural-neighbour Interpolation, Spatial Sorting, periodic/hyperbolic triangulations, mesh generation, shape reconstruction, polygon mesh processing, point set processing: no contest algorithmic use case.
- CGAL Linear and Quadratic Programming Solver: Mathematics `28-simplex.hpp`.
- OI Wiki 随机增量法 as a separate header: covered by `14` (Welzl) and `31` (incremental Delaunay).
- Lattice points on a circle boundary (sum of two squares): Mathematics `34-quadratic_congruence.hpp` (`twoSquaresCount`, `twoSquaresAll`); only disk counts are in row 19.

## P007 completeness decisions (2026-10-07)

- Not adopted for `01-point.hpp`: `normalApprox` (it is `unitApprox(perp(v))` or `unitApprox(cross(u, v))`); 3D `phi`/`theta` (spherical coordinates belong to the 3D rows); `operator!=` (synthesized from `==`); clockwise rotation (`-perp`); epsilon `sgn`/`eq` (contradicts the exact-first policy; row 36 owns robust predicates); rotation by an angle (row 08).
- Not adopted for `02-line_segment.hpp`: a line from equation coefficients (integral `Linear2` endpoints need not exist inside the coordinate domain; `canonicalLine` gives the equation in the other direction); five-way `ccw` (`orient` plus `contains` on a ray or segment answers it); epsilon `sideOf` (use `orient` or `signedLineDistanceApprox`).
- Not adopted for `03-polygon.hpp`: `segmentInPolygon` and nonconvex polygon-line pieces (rare and maspypy-only; candidates for the arrangement row 30); `makeCCW` (`signedArea2` plus `reverse`).
- Not adopted for `04-convexhull.hpp`: separate lower/upper hull (internal passes of `convexHull`; row 20 owns range hulls); presorted O(n) hull (the sort is not the bottleneck for one call); Melkman's simple-chain hull (needs a simple input chain, rare in contests); Jarvis, Quickhull and Akl–Toussaint (no better contest bound).

## P008 completeness decisions (2026-10-07)

- Not adopted for `05-closestpair.hpp`: the minimum-perimeter triangle (a rare extension of the same recursion); the randomized expected-linear grid (it needs floor-hashing, its runtime is hackable with a fixed seed, and the deterministic O(n * log(n)) bound is optimal for comparison-based algorithms); floating-coordinate closest pair (the exact-first policy scales or rounds inputs to integers).
- Not adopted for `06-circle.hpp`: the Apollonian ratio-locus circle (a construction for `24-circle_constructions.hpp`).
- Not adopted for `07-rotatingcalipers.hpp`: two-polygon calipers problems (maximum distance between convex polygons, hull merge bridges, critical support lines, widest separating strip, Grenander distance). Row 17 owns the maximum distance (`convexPolygonMaxDistance`) and row 53 the bridges; the others are not adopted (P036 decisions for row 17).
- Not adopted for `08-coordinate_transform.hpp`: line and direction images (apply the map to two points or use `applyVector`), map equality and how-built predicates (field comparison), and exact rational transforms (no contest need, and they conflict with the `Approx` contract).

## P009 completeness decisions (2026-10-07)

- Not adopted for `09-triangle.hpp`: an approximate angle kind (a right angle is not decidable from rounded coordinates; use the exact type); vertex angles (`angleApprox` in `01-point.hpp`); altitude and median lengths and feet (one projection or `area2 / side`); orientation, bounded side and transforms of a triangle (rows 02, 03 and 08); Fermat point (row 25).

## P036 completeness decisions (2026-10-10)

- **Added to `10-halfplaneintersection.hpp`:** `polygonKernelLinear` (Lee–Preparata, `O(n)`) next to `polygonKernel`, a different algorithm kept by user decision 2026-10-10.
- **Not adopted for `10-halfplaneintersection.hpp`:**
  - halfplanes from coefficients `A x + B y + C <= 0` (integral endpoints need not exist inside the domain; same reason as the P007 line decision);
  - a bounding-box helper (four `Halfplane` values; unbounded results are exact here).
- **Not adopted for `11-minkowskisum.hpp`:** exact sums of nonconvex polygons by decomposition or reduced convolution (CGAL; needs rows 18 and 30, Esoteric) and optimal convex decomposition (row 18).
- **Not adopted for `16-circle_polygon.hpp`:**
  - circle boundary length inside a polygon (sum of the returned arcs times `r`);
  - polygon perimeter inside a disk and intersection point listing (`06-circle` per edge);
  - region-valued disk ∩ polygon (row 30).
- **Not adopted for `17-polygon_distance.hpp`:**
  - widest separating strip (it is the closest pair's bisector strip, from the witnesses);
  - inner and outer common tangents of two convex polygons, and Grenander distance (rare calipers applications);
  - exact rational squared distance (exceeds 128 bits);
  - distance between hulls of point sets (`convexHull` composition);
  - polygons with holes and ranges of polygons in `polygonsIntersect` (CGAL breadth).
- **General `polygonsIntersect` and `polygonDistanceApprox` stay `O(n * m)`** by decision on 2026-10-10. Rows 12 and 45/46 own the optimal sweeps; see the handoff in [17-polygon_distance.md](17-polygon_distance.md).

## P037 completeness decisions (2026-10-18)

Five `@researcher` passes, one per row; sources in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18). Duplicate questions were settled by the user and recorded in `duplicates.md` (Keeps).

- **Decided by the user (2026-10-18):**
  - `coverageCount` is the 1D depth histogram;
  - `batchedLocate` is the merge for angle-sorted queries;
  - both disk counts take a rational centre;
  - `verifyEnclosing` becomes the exact `EnclosingCircle::contains`;
  - `segmentUnionLength`, `orthogonalIntersectionCount`, `latticeUnderLine`, `latticeInHalfplanePolygon`, `convexIntersection`, `cut` and `maxPointsInDisk` stay.
- **Not adopted for `13-segment_union.hpp`:**
  - the incremental anchored rectangle union (an online staircase map that maspypy and ei1333 file under data structures);
  - anchored and general box union volume in 3D (one problem, CF 815D; Klee's measure is Esoteric);
  - the all-depth rectangle distribution (filed in round 3 as `58-rectangle_depth.hpp`, P235);
  - orthogonal any-intersection with collinear overlaps (CSES 1742 only).
- **Not adopted for `14-minimumenclosingcircle.hpp`:**
  - the enclosing circle of circles (filed in round 3 as `57-enclosing_shapes.hpp`, P234);
  - incremental insertion (`O(n)` worst case per point);
  - Megiddo's deterministic linear algorithm (filed in round 3, P234);
  - the smallest circle covering `k` points (filed in round 3 as `56-circle_placement.hpp`, P233);
  - the rectangular p-centre (row 21);
  - extreme-point seeding (Smolik–Skala; measured in round 3).
- **Not adopted for `15-convex_polygon_query.hpp`** (the linear largest triangle and quadrilateral were adopted in round 3, and the minimum enclosing triangle filed as P234):
  - circumscribing k-gons, largest inscribed homothets and axis rectangles (no contest catalog);
  - common tangents of two convex polygons (row 53 bridges).
- **Not adopted for `19-lattice_geometry.hpp`:** the generic convex-region lattice count, the lattice hull under a line and polynomial lattice sums. Mathematics row 55 owns the engines (`convexLatticeHull`, `convexFloorSum`, `floorSumPolynomial`).
- **Not adopted for `54-fixed_radius_queries.hpp`:**
  - approximation schemes (papers only); the weighted, coloured and two-disk variants, Chazelle–Lee's `O(n^2)` sweep and the smallest disk covering `k` points were filed in round 3 as `56-circle_placement.hpp` (P233);
  - unit-disk components (row 22 `euclideanBottleneck`) and hop distances (Esoteric, not filed).

## P037 re-audit decisions (2026-10-19)

Requested by the user after the first verification; recorded in `duplicates.md` (Keeps).

- **Variants kept beside the first versions:**
  - `maxAreaInscribedKgonSmawk` and `maxPerimeterInscribedKgonSmawkApprox` beside the divide-and-conquer k-gons (a different algorithm, kept even though the plain versions are faster at the measured sizes);
  - `rectangleUnionBoundaryGuting` beside the sweep.
- **Domains widened:**
  - `maxPointsInDisk` to `r2 <= 4 * 10^18`;
  - rational lattice counts to `V * D <= 4 * 10^18` (primitive directions, a floored line constant, 256-bit products and Mathematics row 18 `floorSumWide`).

## P037 round-3 decisions (2026-10-19)

A user-requested audit compared every operation of rows 13, 14, 15, 19 and 54 (and Mathematics `floorSumWide`) with the best known algorithms, exactness, accepted inputs, memory and test gaps; sources in [00-sources.md](00-sources.md#p037-round-3-audit-sources-fetched-2026-10-19). Proven lower bounds, and randomized expected bounds kept beside their deterministic variants, stay by design.

- **Filed as Esoteric packages** (limits that are impractical now, and scope items the user asked to track):
  - `56-circle_placement.hpp` (P233): Chazelle–Lee O(n^2), output-sensitive O(n K), batched unit-disk counting, the smallest disk covering k points, weighted, coloured and two-disk coverage;
  - `57-enclosing_shapes.hpp` (P234): Megiddo's deterministic linear enclosing circle, the enclosing circle of circles, the minimum enclosing triangle and parallelogram;
  - `58-rectangle_depth.hpp` (P235): the all-depth distribution (Barbay et al.) and Güting's linear-memory contour sweep;
  - `59-kgon_klink.hpp` (P236): large-k inscribed polygons through k-link paths;
  - `60-wordram_geometry.hpp` (P237): word-RAM bounds (Chan–Pătraşcu), filed by user decision although expected to be slower at contest sizes.
- **Decided by the user:** a deterministic grid (sorted cells) is added to row 54 as a variant of `GridHash`; rational lattice domains are not widened again.

## Validation rules

- Beside the `05-testing.md` oracles, check topology against independent constructions and area/volume conservation.
- Always include repeated/collinear/coplanar points, vertical/coincident segments, cocircular/cospherical sets, exact boundary contacts, holes, reversed orientation, huge coordinates and zero-area/volume results.
