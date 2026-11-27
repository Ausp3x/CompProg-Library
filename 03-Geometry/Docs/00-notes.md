# 03 Geometry — Notes

Evidence is one document per header: [01-point.md](01-point.md), [02-line_segment.md](02-line_segment.md), [03-polygon.md](03-polygon.md), [04-convexhull.md](04-convexhull.md), [05-closestpair.md](05-closestpair.md), [06-circle.md](06-circle.md), [07-rotatingcalipers.md](07-rotatingcalipers.md), [08-coordinate_transform.md](08-coordinate_transform.md), [09-triangle.md](09-triangle.md), [10-halfplaneintersection.md](10-halfplaneintersection.md), [11-minkowskisum.md](11-minkowskisum.md), [13-segment_union.md](13-segment_union.md), [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md), [15-convex_polygon_query.md](15-convex_polygon_query.md), [16-circle_polygon.md](16-circle_polygon.md), [17-polygon_distance.md](17-polygon_distance.md), [19-lattice_geometry.md](19-lattice_geometry.md) and [54-fixed_radius_queries.md](54-fixed_radius_queries.md).

## Numeric Policy

- Integer predicates are exact on stated coordinate bounds. Integral dot products, determinants, squared distances and coordinate differences inside predicates widen to `lll` before any arithmetic; every intermediate must fit `lll`, and vector arithmetic must fit the stored `T`. Check the degree of every intermediate before choosing `lll` or arbitrary precision. `|coordinate| <= 10^9` is the shared sufficient domain for rows 01–09.
- Division-based constructions return either an exact rational (`RationalPoint2`: gcd-normalized, `d > 0`, no rational ordering) or a separately named `Approx` result in `long double`. A predicate never multiplies two rationals.
- Floating arithmetic (floating `T`, and every API ending in `Approx`, which returns `long double`) uses ordinary rounding with computed signs and comparisons. There is no hidden epsilon, no certified error bound and no adaptive precision, so near cancellation the topology can differ from exact real arithmetic. Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding.
- Only membership queries accept an explicit absolute length tolerance `>= 0`: row 06's `locate`, `onBoundary` and `contains`, each defaulting to zero. Sorting and map order never use epsilon equality; `polarLess` and the hulls are integral-only because floating rounding breaks a comparator's strict weak ordering. In rows 01–08 an approximate construction returns a line as two points (`array<dpoint, 2>`) whose left side is the positive side.
- Exact predicate signs do not make approximate intersections exact; the two guarantees stay separate. Adaptive and filtered floating predicates belong to `36-exact_predicates.hpp`, which any tier may depend on.

### GE01 Numeric Policy (P007)

Rows 01–04 (`01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp`, `04-convexhull.hpp`):

- `Point2<T>` and `Point3<T>` accept signed integral and floating `T`. `|x| <= 10^9` is sufficient for every operation, including the cubic `tetraVolume6`, the degree-3 intersection numerators (`<= 24 * 10^27`) and the polygon first moments (terms `<= 4 * 10^27`, summable over `INT_MAX` vertices).
- Exact line topology, `canonicalLine` and the lattice counts require integral coordinates with `|x| <= 10^9` and assert this bound. Point primitives, hulls and the other integral polygon routines also accept wider coordinates as long as every intermediate fits `lll`.

### GE02 Numeric Policy (P008)

Rows 05–08 (`05-closestpair.hpp`, `06-circle.hpp`, `07-rotatingcalipers.hpp`, `08-coordinate_transform.hpp`): the exact APIs are `closestPair`, `closestPairRandomized`, `closestPair3`, `circleRelation`, `Affine2` (over its field `R`), every support or optimum choice of `ConvexCalipers`, and `farthestPair`, all on signed integral coordinates. Every other API is an `Approx` API under the policy above.

## Topology and Domains

- Intersection and feasibility results distinguish empty, point, segment/ray/line/plane overlap, coincident and unbounded states. Endpoints are closed; `a == b` is a singleton for every linear kind; a zero direction is parallel to everything; two singletons are always collinear.
- Polygon rings have no repeated closing vertex and `n <= INT_MAX`. Area, moments and winding apply algebraically to any closed walk; their geometric meaning needs a simple polygon. Convexity assumes simplicity, and Pick's theorem needs nonzero area. A polygon with holes is `{outer CCW, holes CW}` with simple, disjoint, non-touching rings. `12-segmentintersection.hpp` owns simplicity validation, not `03-polygon.hpp`.
- Hull output starts at the lexicographic minimum, proceeds CCW and never repeats its start; duplicates are removed; `keep_collinear` retains every distinct boundary point; `convexHullIndices` names the first input occurrence of each hull point. Calipers require a strictly convex CCW hull (`convexHull(..., false)`), accept empty, singleton and segment inputs, and return original indices with lexicographically smallest ties.
- Circles are boundaries and disks are filled sets. Zero radius denotes a point. Intersection counts use `-1` for infinitely many; in a fixed-size result array only the prefix `[0, count)` is meaningful.
- Halfplane and halfspace APIs never disguise unbounded feasibility with an arbitrary bounding box; unbounded answers keep recession data. Clipping needs a justified finite initial polytope.
- Polygon Boolean and arrangement APIs state their handling of multi-polygons, lower-dimensional contacts, touching or overlapping edges, ring orientation and fill rule, and they share one arrangement representation.
- Reporting APIs state detection, counting and reporting costs separately.
- Degeneracy policies are row contracts, not operations: coincident and vertical segments (`12-segmentintersection.hpp`, including `multiwayCrossing`), cocircular sites (`31-delaunay.hpp`), tangent sight lines (`39-visibilitygraph.hpp`), simultaneous kinetic events (`40-kinetic_geometry.hpp`) and cospherical sites (`47-delaunay3d.hpp`).

## Ownership Boundaries

- Data Structures owns `43-rangetree.hpp` (orthogonal count and report), `11-fenwick_tree_advanced.hpp`, `33-offline_rectangle_queries.hpp`, `21-lichao.hpp`, `22-convexhulltrick.hpp` and `24-interval_set.hpp`. Geometry owns the point-facing kd-tree, R-tree, quadtree and nearest-neighbour APIs and reuses those engines without wrapping or duplicating them (`00-Guidelines/03-cpp.md`, One API per Operation). `29-point_set_queries.hpp` owns range-restricted closest pair and may reuse `28-spatial_index.hpp`.
- Graphs `29-planar_graph.hpp` owns combinatorial half-edge embeddings, faces and duals. Geometry subdivisions own coordinates and events, and `33-linearrangement.hpp` exports to the planar graph instead of duplicating face logic. `22-geometricmst.hpp` reuses the Graphs MST engines; generic DP engines stay in Miscellaneous even when row 41 supplies an angular event order.
- Mathematics owns floor sums and lattice-count kernels (`18-floorsum.hpp`, `55-floor_sum_polynomial.hpp`), real and complex root isolation (`29-polynomial_roots.hpp`), ternary and golden-section search (`02-search_algorithms.hpp`), Simplex LP (`28-simplex.hpp`), numerical integration (`17-numerical_methods.hpp`), sums of two squares (`34-quadratic_congruence.hpp`) and Gaussian integers (`57-quadratic_integer.hpp`). Rows 19, 23, 25, 42 and 44 reuse them without duplicating them.
- `42-randomizedlp.hpp` stays in Geometry because Seidel's fixed-dimension LP is the feasibility engine of rows 38 and 49; general LP is Mathematics `28-simplex.hpp`.
- `44-algebraic_geometry.hpp` stays in Geometry for conic, arc and Bézier intersections only; polynomial root isolation itself is Mathematics.
- Chan's output-sensitive hull is a named variant in row 41, not a requirement of `04-convexhull.hpp`.
- Python mirrors are documented reductions: `08-Python/_11_geometry.py` (rows 01–04, 15), `_41_geometry_float.py` (01, 02, 06, 08, 14), `_44_exact_geometry.py` (12) and `_55_geometry_advanced.py` (05, 07, 10, 11, 13, 15).

## Legacy Obligations

- Superseded and accounted for by GE01 and GE02 (bytes preserved, no active dependency): `OLD/Team Notebook/src/geometry/point.cpp`, `intersection.cpp`, `polyarea.cpp`, `getnearestpair.cpp`, and their counterparts in `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp`. The historical names (`Point`, `PointD`, `sgn`, `polyArea`, `isIntscLinSeg`) are not compatibility APIs. Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`.
- Superseded and accounted for by P036 (bytes preserved, no active dependency): `halfplane.cpp` (row 10; its `±2 * 10^9` box and `1e-9` predicates are not adopted) and `getminkowskisum.cpp` (row 11).
- Superseded and accounted for by P037 (bytes preserved, no active dependency): `getlensegunion.cpp` (row 13 `segmentUnionLength`) and `welzlmec.cpp` (row 14; its double predicates and clock seed are not adopted).
- Pending until their rows are implemented: `getintersectingsegs.cpp` (row 12) and `getmanhattammstedges.cpp` (row 21). The legacy epsilon predicates (`1e-9`) are not adopted; each successor states its own exact or approximate policy.
- `lichaotree.cpp` in the same directory belongs to Data Structures `21-lichao.hpp`.
- No correctness claim in any legacy snippet is adopted.

## 2026-09-27 Audit: Not Adopted

The 2026-09-27 audit kept industrial CAD, molecular surfaces, remeshing and visualization, and machine-learning reconstruction out of the inventory. The 2026-11-08 omission sweep dropped that exclusion (user decision, `00-Guidelines/23-Sweep Ledgers/omissions.md`): an application area no longer excludes a family, and `01-principles.md` (Priority) decides as for any other. These catalog items of the [2026-09-27 audit](00-sources.md#2026-09-27-audit-provenance) stay out:

- maspypy `definite_integral` and `modint_real`: numerical integration is Mathematics `17-numerical_methods.hpp`, and modular reals are not geometry.
- hitonanode `point2d_w_error` as a public type: it is absorbed as the `IntervalPoint2` filter inside row 36 rather than a separate header.
- cp-algorithms "Finding faces of a planar graph": Graphs `29-planar_graph.hpp`.
- CGAL Linear and Quadratic Programming Solver: Mathematics `28-simplex.hpp`.
- OI Wiki 随机增量法 as a separate header: row 14 (Welzl) and row 31 (incremental Delaunay) cover it.
- Lattice points on a circle boundary (sums of two squares): Mathematics `34-quadratic_congruence.hpp` (`twoSquaresCount`, `twoSquaresAll`); row 19 has only the disk counts.

## P007 Completeness Decisions (2026-10-07)

- Not adopted for `01-point.hpp`: `operator!=` (synthesized from `==`); epsilon `sgn` and `eq` (they contradict the exact-first policy, and row 36 owns robust predicates); rotation by an angle (row 08).
- Not adopted for `02-line_segment.hpp`: epsilon `sideOf` (use `orient` or `signedLineDistanceApprox`).
- Not adopted for `04-convexhull.hpp`: Quickhull and Akl–Toussaint (no better contest bound).

## P008 Completeness Decisions (2026-10-07)

- Not adopted for `08-coordinate_transform.hpp`: line and direction images (apply the map to two points or use `applyVector`) and map equality or how-built predicates (compare the fields).

## P009 Completeness Decisions (2026-10-07)

- Not adopted for `09-triangle.hpp` (confirmed by user decision 2026-11-27): an approximate angle kind (a right angle is not decidable from rounded coordinates; use the exact type); the Fermat point (row 25 `fermatPoint`). Altitude and median lengths and feet, first left out here, joined row 09 by the same decision. Two of them keep their own arithmetic instead of composing: `TriangleApprox::altitudes` and `altitudeFeet` divide by `hypot` where row 02's `signedLineDistanceApprox` and `projectLineApprox` divide by `norm2`, which loses precision on subnormal squares (user decision 2026-11-27); `Triangle2::altitudeFeet` projects in `lll` with one gcd into a `RationalPoint2`, where row 08's `Affine2<>::projection(b, c).applyPoint(a)` gives the same values as `Rational<lll>` pairs through a dozen normalized rational operations.

The P009 re-audit (2026-11-15, user decisions 2026-11-15) added `Triangle2::circumcenter` and `circumradius2` (CGAL; exact, while row 14's `EnclosingCircle` is exact only for its own support) and `orthocenter` (exact `A + B + C - 2O`) to row 09. It did not adopt:

- `Triangle2::circumcircleLocate` (CGAL `side_of_bounded_circle`): row 36 owns `sideOfOrientedCircle`, and its determinant exceeds `lll` at the coordinate bound;
- CGAL `Triangle_2` `opposite`, `is_degenerate`, `vertex` and `operator==` up to rotation (trivial forms of existing members), and `bbox` (row 28);
- an approximate `locate` (a boundary is not decidable from rounded coordinates);
- the 3D circumcenter and squared radius, which row 26 provides as `circumsphere3` (user decision 2026-11-22).

`Triangle2::locate` keeps its own three orientations instead of calling row 03's bounded side: it is O(1) on the struct's fields with no vertex vector, returns an `int` -1/0/1 like `CircleApprox::locate`, and locates a degenerate triangle as its edges.

## P036 Completeness Decisions (2026-10-10)

- **Added to `10-halfplaneintersection.hpp`:** `polygonKernelLinear` (Lee–Preparata, `O(n)`) beside `polygonKernel`, a different algorithm kept by user decision 2026-10-10.
- **Not adopted for `10-halfplaneintersection.hpp`:**
  - halfplanes from coefficients `A x + B y + C <= 0` (integral endpoints need not exist inside the coordinate domain);
  - a bounding-box helper (it is four `Halfplane` values, and unbounded results are exact here).
- **Not adopted for `11-minkowskisum.hpp`:** exact sums of nonconvex polygons by convex decomposition (CGAL; needs rows 18 and 30) and optimal convex decomposition (row 18).
- **Not adopted for `16-circle_polygon.hpp`:**
  - circle boundary length inside a polygon (the sum of the returned arcs times `r`);
  - polygon perimeter inside a disk and intersection point listing (`06-circle` per edge).
- **Not adopted for `17-polygon_distance.hpp`:**
  - exact rational squared distance (exceeds 128 bits);
  - distance between hulls of point sets (a `convexHull` composition; row 42 has the fixed-dimension `polytopeDistance`).
- **General `polygonsIntersect` and `polygonDistanceApprox` stay `O(n * m)`** by decision on 2026-10-10. Rows 12 and 45/46 own the optimal sweeps; see the handoff in [17-polygon_distance.md](17-polygon_distance.md).

## P037 Completeness Decisions (2026-10-18)

Five `@researcher` passes, one per row; sources are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18). The user settled the duplicate questions, recorded in `duplicates.md` (Keeps).

- **Decided by the user (2026-10-18):**
  - `coverageCount` is the 1D depth histogram;
  - `batchedLocate` is the merge for angle-sorted queries;
  - both disk counts take a rational centre;
  - `verifyEnclosing` becomes the exact `EnclosingCircle::contains`;
  - `segmentUnionLength`, `orthogonalIntersectionCount`, `latticeUnderLine`, `latticeInHalfplanePolygon`, `convexIntersection`, `cut` and `maxPointsInDisk` stay.
- **Not adopted for `14-minimumenclosingcircle.hpp`:**
  - incremental insertion (`O(n)` worst case per point);
  - extreme-point seeding (Smolik–Skala; measured in round 3).
- **Not adopted for `15-convex_polygon_query.hpp`:** circumscribing k-gons beyond row 59, largest inscribed homothets and axis rectangles (no contest catalog).
- **Not adopted for `19-lattice_geometry.hpp`:** the generic convex-region lattice count and the lattice hull under a line. Mathematics row 55 owns the engines (`convexLatticeHull`, `convexFloorSum`, `floorSumPolynomial`).
- **Not adopted for `54-fixed_radius_queries.hpp`:**
  - approximation schemes (papers only);
  - unit-disk components (row 22 `euclideanBottleneck`) and hop distances (Esoteric, not filed).

## P037 Re-Audit Decisions (2026-10-19)

The user requested this audit after the first verification; the decisions are recorded in `duplicates.md` (Keeps).

- **Variants kept beside the first versions:**
  - `maxAreaInscribedKgonSmawk` and `maxPerimeterInscribedKgonSmawkApprox` beside the divide-and-conquer k-gons (a different algorithm, kept although the plain versions are faster at the measured sizes);
  - `rectangleUnionBoundaryGuting` beside the sweep.
- **Domains widened:**
  - `maxPointsInDisk` to `r2 <= 4 * 10^18`;
  - rational lattice counts to `V * D <= 4 * 10^18` (primitive directions, a floored line constant, 256-bit products and Mathematics row 18 `floorSumWide`).

## P037 Round-3 Decisions (2026-10-19)

A user-requested audit compared every operation of rows 13, 14, 15, 19 and 54 (and Mathematics `floorSumWide`) with the best known algorithms for bound, exactness, accepted inputs, memory and test gaps; sources are in [00-sources.md](00-sources.md#p037-round-3-audit-sources-fetched-2026-10-19). Proven lower bounds, and randomized expected bounds kept beside their deterministic variants, stay by design.

- **Filed as Esoteric rows** (bounds impractical now, and scope items the user asked to track): `56-circle_placement.hpp` (P233), `57-enclosing_shapes.hpp` (P234), `58-rectangle_depth.hpp` (P235), `59-kgon_klink.hpp` (P236) and `60-wordram_geometry.hpp` (P237); the user filed the word-RAM bounds (Chan–Pătraşcu) although they are expected to be slower at contest sizes.
- **Decided by the user:** a deterministic grid with sorted cells joins row 54 as a variant of `GridHash`; rational lattice domains are not widened again.

## Validation Rules

- Beside the `05-testing.md` oracles, check topology against independent constructions and check area and volume conservation.
- Always include repeated, collinear and coplanar points, vertical and coincident segments, cocircular and cospherical sets, exact boundary contacts, holes, reversed orientation, huge coordinates and zero-area or zero-volume results.

## Omission Sweep 2026-11-08

The 2026-11-08 sweep left out exact duplicates, second implementations and items another row owns (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions). Each item names where the candidate is implemented instead.

- `minimumEnclosingParallelogram` (CGAL): row 07 `minimumAreaParallelogramApprox` already computes it by Rote's linear algorithm with exact selection, so row 57 dropped its planned copy too.
- `yaoGraph` and `thetaGraph` in row 22 (CGAL): row 63 `geometric_spanners` owns them.
- `gaussCircleCountHull (convex hull)` in row 19: found in the library with no catalog source; Mathematics row 55 owns it, and row 19 keeps `latticeInDiskHull`.

## P275 Re-Review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per Operation), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). It returned operations to rows 01, 02, 04, 05, 08, 09, 11, 12, 13, 17, 25, 28, 30, 33 and 45 and added Esoteric rows 68–76; the omissions still listed in this file stand ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)).

- The 2026-09-27 exclusion of CGAL packages for lack of contest use is lifted (user decision 2026-11-10): rows 28, 30, 33 and 45 take Spatial Sorting, Polygon Repair, Snap Rounding and 2D mesh refinement, and rows 68–75 the other families. Interval Skip List is Data Structures row 68 `contains`.
- maspypy `minimum_three_distance_sum` is the sum at `fermatPoint` (row 25). The incremental anchored rectangle union is row 13, since no Data Structures row owned it. Common tangents of two convex polygons are row 17, since row 53 lists only `DynamicHull::bridge`.
- `TriangleApprox::circumcircleApprox` and `incircleApprox` (row 09) are member wrappers over the row 06 operations (user decision 2026-11-14, `omissions.md`); the legacy names `circumcircle` and `incircle`, which the P009 prune removed, do not return.

## P007 Re-Audit Decisions (2026-11-11)

Four `@researcher` passes, one per GE01 row; sources are in [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11).

- **Added to `01-point.hpp`:** `Angle` fields `v`, `t` and members `half`, `t90`, `t180`, `t360` (KACTL); `argumentSort` (Library Checker order, a composition of `polarLess`); `scalarProjectionApprox` (cp-algorithms `proj`); `normalApprox` in 2D and 3D forms. By user decision 2026-11-12: `rotateCW`, stream `<<` and `>>` for `Point2` and `Point3` (KACTL, maspypy), and `argumentSortApprox` (any `T`, floating included, so wider than `argumentSort`).
- **Added to `02-line_segment.hpp`:** `lineIntersectionApprox` (any `Point2<T>`, floating included, so wider than `intersect`); `linearDistance2` (exact squared distance of every point, segment, ray and line pair as Core `Rational<lll>`; user decision 2026-11-11) and `linearDistanceApprox`; by user decision 2026-11-12, `lineFromEquation` (false when fewer than two lattice points fit).
- **Added to `03-polygon.hpp`:** `makeCCW` (user decision 2026-11-12).
- **Added to `04-convexhull.hpp`:** `lowerHullSorted` and `upperHullSorted` (maspypy `convex_hull.hpp` sorted mode; they reuse the caller's sort like `convexHullSorted`, under the presorted rule of `omissions.md`, a 2026-11-11 review finding); by user decision 2026-11-12, `lowerHullIndices` and `upperHullIndices` (index forms beside `convexHullIndices`).
- **`keep_collinear` for `convexHullMelkman`:** added by user decision 2026-11-12 and dropped by user decision 2026-11-22; P007 reopens for the code change.
- **Kept beside `linearDistanceApprox`:** `segmentDistanceApprox` (long double from exact topology, without `Rational` normalization; a constant-factor variant, ruling 4 of `duplicates.md`).
- **Not adopted:** KACTL's untracked `Angle - Angle` (it is `Angle(a.v - b.v, a.t)`; here `operator-` moves by a vector and tracks turns like `+`; user decision 2026-11-12); OI Wiki ray casting and angle-sum winding for row 03 (same bound, narrower or inexact; `polygonWinding` covers them); the OI Wiki bounding-box prefilter for segment tests (the same exact test); Quickhull and Akl–Toussaint (decided 2026-10-07) and Bykat (Quickhull's divide and conquer, same bound); CGAL's partial Jarvis march between two extreme points (a form of `convexHullJarvis`; user decision 2026-11-12).
- **Follow-ups outside GE01 (/finish P007, 2026-11-12):** `Rational<InfIntMini>::toLongDouble` does not compile because `InfIntMini` has no shift `<<` (`01-Core/09-rational.hpp`, P019), so `linearDistanceApprox<Rational<InfIntMini>>` is unavailable. `15-convex_polygon_query.hpp` keeps its own copies of `half` and `angleLess`, which `geometry_detail::half` and `polarLess` now provide.

## P008 Re-Audit Decisions (2026-11-14)

Four `@researcher` passes, one per GE02 row; sources are in [00-sources.md](00-sources.md#p008-re-audit-fetched-2026-11-14). Rows 05 and 06 had no missing operation.

- **Added to `07-rotatingcalipers.hpp`:** `ConvexCalipers::expectedBoxAreaApprox` (OI Wiki's calipers exercise CF 1578F). It is a one-problem algorithm with its own `atan2` event sweep over the struct's hull, kept as a `ConvexCalipers` member in Basic row 07 rather than an Esoteric header (user decision 2026-11-14).
- **Added to `08-coordinate_transform.hpp`:** the `Affine2` coefficients and its exact factories `translation`, `scaling`, `rotation` (rational cosine and sine), `projection`, `reflection` and `similarity` (CGAL `Aff_transformation_2`; the exact counterparts of the `Affine2Approx` factories).
- **Not adopted, dominated (Priority):** KACTL's ordered-set sweep for `closestPair` (with the row's index tie rule and span checks it is not half the lines of the shared divide and conquer); the incremental randomized grid (Khuller–Matias, maspypy `closest_pair`), suisen's bottom-up divide and conquer and the ternary-search diameter (same bounds, no wider domain); Freeman–Shapira's quadratic rectangle (not lighter by half once ties and witnesses are kept); Nyaan's `acos` circle intersection.
- **Not adopted:** CGAL's rational rotation that approximates a direction to a given error (no contest use).
- **Added, all-pairs scans (a Priority exception, user decision 2026-11-14, recorded in `omissions.md`):** `closestPairNaive`, `closestPair3Naive` and `minimumPerimeterTriangleApproxNaive` in row 05 and `farthestPairNaive` in row 07. Each is a short scan beside the divide and conquer or the hull and calipers it shadows, with the same domain, result and tie rule (row 12's `reportIntersectionsNaive` is the precedent).
- **Follow-ups outside GE02 (user decisions):** `fromPolarApprox` (OI Wiki 坐标系; row 01 would own it beside `argApprox` and `normApprox`); the onion and spiral triangulations, quadrangulation, art gallery guarding and wedge placement of the Wikipedia calipers list (no row owns them). The thinnest-strip and shortest transversals are already listed under Limits in `00-Guidelines/23-Sweep Ledgers/Docs/00-duplicate-rereview.md`.

## Omission Sweep 2026-11-27

Left out from the candidates of the 2026-11-27 catalog survey (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`; user decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions): second implementations and slower algorithms that Priority does not admit.

- M-tree (WIKI): a paged dynamic metric index with no bound beyond row 28 `CoverTree`, and neither lighter, wider nor more capable (`01-principles.md`, Priority).

## Omission Sweep 2026-11-28

Left out from the candidates of the 2026-11-28 sample of the sources the user placed (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`, Sampled and Applied 2026-11-28).

- BAR trees (MEHTASAHNI): A second implementation of BbdTree (DS2-030) approximate queries with the same bounds (One API per Operation).
- Lin–Canny closest features and the Voronoi-based marching algorithm (MEHTASAHNI): Second implementations of Geometry 42 polytopeDistance and Geometry 17 convexSeparated with no better bound (03-cpp.md, One API per Operation).
- Virtual quadtrees and the translation-invariant data structure (MEHTASAHNI): Other encodings of the binary images RegionQuadtree (DS2-025) handles, with the same operations (One API per Operation).
- Separating chains and fractional cascading for point location (MEHTASAHNI): A second implementation of row 34's O(log n) point location (SlabLocator, TrapezoidalMap, KirkpatrickLocator) with no better bound (One API per Operation).
- Triangulation by monotone mountains (OROURKE): One API per Operation: it has the same O(n log n) bound and result as row 18's monotonePartition plus triangulateMonotone, and no Priority criterion applies.
- Preparata–Hong divide-and-conquer convex hull (OROURKE): One API per Operation and Priority: it has the same O(n log n) bound and output as convexHull, like Quickhull, Akl–Toussaint and Bykat, which 03-Geometry/Docs/00-notes.md (P007) already rejects.
- Local-search k-median and metric facility location (EOA): Priority (01-principles.md): a slower, worse-ratio algorithm for the same task as the kept facilityLocationGreedy, and not lighter once its kMedianLocalSearch dependency is counted. It needs a recorded line in 03-Geometry/Docs/00-notes.md.
