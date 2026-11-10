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

This audit kept industrial CAD, molecular surfaces, remeshing and visualization, and machine-learning reconstruction outside the inventory; the 2026-11-08 omission sweep dropped that exclusion (user decision, `00-Guidelines/23-Sweep Ledgers/omissions.md`), so an application area no longer excludes a family and `01-principles.md` (Priority) decides as for any other. Catalog items of the [2026-09-27 audit](00-sources.md#2026-09-27-audit-provenance) left out:

- maspypy `minimum_three_distance_sum`: single-problem curiosity without a reusable family. (returned by P275 2026-11-10 into row 25)
- maspypy `definite_integral`, `modint_real`: numerical integration is Mathematics `17-numerical_methods.hpp`; modular reals are not geometry.
- hitonanode `point2d_w_error` as a public type: absorbed as the `IntervalPoint2` filter inside row 36 rather than a separate header.
- KACTL `Angle` class: `polarLess` and `canonicalDirection` cover its use; no separate angle type. (returned by P275 2026-11-10 into row 01)
- cp-algorithms "Finding faces of a planar graph": Graphs `29-planar_graph.hpp`.
- CGAL 2D Snap Rounding, Polygon Repair, Movable Separability, Interval Skip List, natural-neighbour Interpolation, Spatial Sorting, periodic/hyperbolic triangulations, mesh generation, shape reconstruction, polygon mesh processing, point set processing: no contest algorithmic use case.
- CGAL Linear and Quadratic Programming Solver: Mathematics `28-simplex.hpp`.
- OI Wiki 随机增量法 as a separate header: covered by `14` (Welzl) and `31` (incremental Delaunay).
- Lattice points on a circle boundary (sum of two squares): Mathematics `34-quadratic_congruence.hpp` (`twoSquaresCount`, `twoSquaresAll`); only disk counts are in row 19.

## P007 completeness decisions (2026-10-07)

- Not adopted for `01-point.hpp`: `normalApprox` (it is `unitApprox(perp(v))` or `unitApprox(cross(u, v))`); 3D `phi`/`theta` (adopted on 2026-11-08 into row 27 as `toSphericalApprox` and `fromSphericalApprox`); `operator!=` (synthesized from `==`); clockwise rotation (`-perp`); epsilon `sgn`/`eq` (contradicts the exact-first policy; row 36 owns robust predicates); rotation by an angle (row 08). (returned by P275 2026-11-10: `normalApprox`)
- Not adopted for `02-line_segment.hpp`: a line from equation coefficients (integral `Linear2` endpoints need not exist inside the coordinate domain; `canonicalLine` gives the equation in the other direction); five-way `ccw` (`orient` plus `contains` on a ray or segment answers it); epsilon `sideOf` (use `orient` or `signedLineDistanceApprox`). (returned by P275 2026-11-10: `ccw`)
- Not adopted for `03-polygon.hpp`: `segmentInPolygon` and nonconvex polygon-line pieces (adopted on 2026-11-08 into row 30 as `segmentInPolygon` and `polygonLineIntersection`); `makeCCW` (`signedArea2` plus `reverse`).
- Not adopted for `04-convexhull.hpp`: separate lower/upper hull (internal passes of `convexHull`; row 20 owns range hulls; returned by P275 2026-11-10 as `lowerHull`, `upperHull`); Quickhull and Akl–Toussaint (no better contest bound). The presorted O(n) hull, Melkman's simple-chain hull and Jarvis were adopted on 2026-11-08 into row 04 (`convexHullSorted`, `convexHullMelkman`, `convexHullJarvis`).

## P008 completeness decisions (2026-10-07)

- Not adopted for `05-closestpair.hpp`: the minimum-perimeter triangle (a rare extension of the same recursion). The randomized expected-linear grid and floating-coordinate closest pair were adopted on 2026-11-08 into row 05 (`closestPairRandomized`, `closestPairApprox`). (returned by P275 2026-11-10 into row 05)
- Not adopted for `06-circle.hpp`: the Apollonian ratio-locus circle (adopted on 2026-11-08 into row 24 as `apollonianCircle`).
- Not adopted for `07-rotatingcalipers.hpp`: two-polygon calipers problems (maximum distance between convex polygons, hull merge bridges, critical support lines, widest separating strip, Grenander distance). Row 17 owns the maximum distance (`convexPolygonMaxDistance`) and row 53 the bridges; the others are not adopted (P036 decisions for row 17).
- Not adopted for `08-coordinate_transform.hpp`: line and direction images (apply the map to two points or use `applyVector`), map equality and how-built predicates (field comparison), and exact rational transforms (no contest need, and they conflict with the `Approx` contract).

## P009 completeness decisions (2026-10-07)

- Not adopted for `09-triangle.hpp`: an approximate angle kind (a right angle is not decidable from rounded coordinates; use the exact type); vertex angles (`angleApprox` in `01-point.hpp`); altitude and median lengths and feet (one projection or `area2 / side`); orientation, bounded side and transforms of a triangle (rows 02, 03 and 08); Fermat point (row 25). (returned by P275 2026-11-10 into row 09)

## P036 completeness decisions (2026-10-10)

- **Added to `10-halfplaneintersection.hpp`:** `polygonKernelLinear` (Lee–Preparata, `O(n)`) next to `polygonKernel`, a different algorithm kept by user decision 2026-10-10.
- **Not adopted for `10-halfplaneintersection.hpp`:**
  - halfplanes from coefficients `A x + B y + C <= 0` (integral endpoints need not exist inside the domain; same reason as the P007 line decision);
  - a bounding-box helper (four `Halfplane` values; unbounded results are exact here).
- **Not adopted for `11-minkowskisum.hpp`:** exact sums of nonconvex polygons by decomposition or reduced convolution (CGAL; needs rows 18 and 30, Esoteric) and optimal convex decomposition (row 18).
- **Not adopted for `16-circle_polygon.hpp`:**
  - circle boundary length inside a polygon (sum of the returned arcs times `r`);
  - polygon perimeter inside a disk and intersection point listing (`06-circle` per edge);
  - region-valued disk ∩ polygon (adopted on 2026-11-08 into row 30 as `diskPolygonIntersectionApprox`).
- **Not adopted for `17-polygon_distance.hpp`:**
  - widest separating strip (it is the closest pair's bisector strip, from the witnesses);
  - inner and outer common tangents of two convex polygons, and Grenander distance (rare calipers applications);
  - exact rational squared distance (exceeds 128 bits);
  - distance between hulls of point sets (`convexHull` composition; the fixed-dimension `polytopeDistance` was adopted on 2026-11-08 into row 42);
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
  - anchored and general box union volume in 3D (adopted on 2026-11-08 into Esoteric row 58 as `boxUnionVolume` and `boxUnionVolumeSweep`);
  - the all-depth rectangle distribution (filed in round 3 as `58-rectangle_depth.hpp`, P235; its quadratic plain-array sweep was adopted on 2026-11-08 into row 58 as `rectangleDepthDistributionQuadratic`);
  - orthogonal any-intersection with collinear overlaps (CSES 1742 only).
- **Not adopted for `14-minimumenclosingcircle.hpp`:**
  - the enclosing circle of circles (filed in round 3 as `57-enclosing_shapes.hpp`, P234);
  - incremental insertion (`O(n)` worst case per point);
  - Megiddo's deterministic linear algorithm (filed in round 3, P234);
  - the smallest circle covering `k` points (filed in round 3 as `56-circle_placement.hpp`, P233);
  - the rectangular p-centre (adopted on 2026-11-08 into row 21 as `rectangularPCenter`);
  - extreme-point seeding (Smolik–Skala; measured in round 3).
- **Not adopted for `15-convex_polygon_query.hpp`** (the linear largest triangle and quadrilateral were adopted in round 3, and the minimum enclosing triangle filed as P234):
  - circumscribing k-gons (the minimum-area k-gon on polygon edges was adopted on 2026-11-08 into row 59 as `minAreaEnclosingKgon`), largest inscribed homothets and axis rectangles (no contest catalog);
  - common tangents of two convex polygons (row 53 bridges).
- **Not adopted for `19-lattice_geometry.hpp`:** the generic convex-region lattice count and the lattice hull under a line. Mathematics row 55 owns the engines (`convexLatticeHull`, `convexFloorSum`, `floorSumPolynomial`). Polynomial lattice sums over halfplane polygons were adopted on 2026-11-08 into row 19 (`latticePolynomialSumInHalfplanePolygon`).
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
  - `57-enclosing_shapes.hpp` (P234): Megiddo's deterministic linear enclosing circle, the enclosing circle of circles, the minimum enclosing triangle (row 07 owns the minimum-area parallelogram);
  - `58-rectangle_depth.hpp` (P235): the all-depth distribution (Barbay et al.) and Güting's linear-memory contour sweep;
  - `59-kgon_klink.hpp` (P236): large-k inscribed polygons through k-link paths;
  - `60-wordram_geometry.hpp` (P237): word-RAM bounds (Chan–Pătraşcu), filed by user decision although expected to be slower at contest sizes.
- **Decided by the user:** a deterministic grid (sorted cells) is added to row 54 as a variant of `GridHash`; rational lattice domains are not widened again.

## Validation rules

- Beside the `05-testing.md` oracles, check topology against independent constructions and area/volume conservation.
- Always include repeated/collinear/coplanar points, vertical/coincident segments, cocircular/cospherical sets, exact boundary contacts, holes, reversed orientation, huge coordinates and zero-area/volume results.

## Omission sweep 2026-11-08

Left out by the 2026-11-08 sweep (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded decisions): exact duplicates, second implementations and items another row owns. Handoff notes name where a candidate is implemented instead.

- `minimumEnclosingParallelogram` (CGAL): row 07 `minimumAreaParallelogramApprox` already computes it by Rote's linear algorithm with exact selection, so row 57 dropped its planned copy too.
- `yaoGraph`, `thetaGraph` in row 22 (CGAL): row 63 `geometric_spanners` owns them.
- `gaussCircleFast (convex hull)` in row 19: found in the library, no catalog source; Mathematics row 55 owns it; row 19 keeps `latticeInDiskFast`.

## P275 re-review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per operation, 2026-11-08), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). The items below returned to rows and supersede the earlier bullets that name them; every other omission recorded here stands ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)).

- `01-point.hpp`: Angle: operator+, operator-, operator<, angleDiff, segmentAngles (turn count); normalApprox.
- `02-line_segment.hpp`: ccw (five-way: orient and contains).
- `04-convexhull.hpp`: lowerHull, upperHull.
- `05-closestpair.hpp`: minimumPerimeterTriangleApprox (divide and conquer).
- `08-coordinate_transform.hpp`: rotateApprox (Affine2Approx::rotation); Affine2: applyPoint, applyVector, operator*, determinant, inverse (exact rational).
- `09-triangle.hpp`: TriangleApprox::angles, TriangleApprox::transform (Affine2Approx); Triangle2::orient, Triangle2::locate.
- `11-minkowskisum.hpp`: minkowskiSumReducedConvolution (nonconvex polygons, holes).
- `12-segmentintersection.hpp`: firstSelfIntersection (polyline prefixes, collinear overlaps).
- `13-segment_union.hpp`: IncrementalRectangleUnion: add (anchored [0, x] × [0, y]), area.
- `17-polygon_distance.hpp`: widestSeparatingStripApprox; commonTangents (two convex polygons, outer and inner); polygonsIntersect (rings).
- `25-geometric_median.hpp`: minimumThreeDistanceSumApprox (fermatPoint).
- `28-spatial_index.hpp`: hilbertSort (2D, 3D; median, middle); spatialSort (BRIO); hilbertSortOnSphere.
- `30-polygon_boolean.hpp`: FillRule (nonzero, evenodd, union, intersection); polygonRepair; polygonUnion (family), polygonIntersection (family); polygonsIntersect (MultiPolygon).
- `33-linearrangement.hpp`: snapRounding; snapRoundingIterated; hotPixels.
- `45-constraineddelaunay.hpp`: refine (size bound); lloydOptimizeMesh2Approx.
- new Esoteric row `68-movable_separability.hpp`: castingTopEdges; pulloutDirections; isPulloutDirection (CGAL).
- new Esoteric row `69-natural_neighbour.hpp`: naturalNeighbourCoordinatesApprox; regularNeighbourCoordinatesApprox; linearInterpolationApprox; quadraticInterpolationApprox; sibsonC1InterpolationApprox; farinC1InterpolationApprox; sibsonGradientFittingApprox; surfaceNeighbours3; surfaceNeighbourCoordinates3Approx (CGAL).
- new Esoteric row `70-periodic_triangulation.hpp`: PeriodicOffset2, PeriodicOffset3; PeriodicDelaunay2: insert, remove, locate, nearestVertex, voronoiDual; PeriodicDelaunay3: insert, remove, locate, nearestVertex, voronoiDual; PeriodicRegular3: insert, remove, locate (CGAL).
- new Esoteric row `71-hyperbolic_triangulation.hpp`: HyperbolicDelaunay2: insert, remove, locate, voronoiDualApprox; BolzaDelaunay2: insert, remove, locate, voronoiDualApprox; HyperbolicTranslation (CGAL).
- new Esoteric row `72-mesh3.hpp`: PolyhedralMeshDomain3; LabeledMeshDomain3; makeMesh3Approx; refineMesh3Approx; exudeMesh3; perturbMesh3Approx; lloydOptimizeMesh3Approx; odtOptimizeMesh3Approx (CGAL).
- new Esoteric row `73-surface_reconstruction.hpp`: advancingFrontReconstruction; scaleSpaceSmoothApprox; scaleSpaceReconstruction; PoissonFunctionApprox: evaluate; poissonReconstructionApprox (CGAL).
- new Esoteric row `74-polygon_mesh_processing.hpp`: selfIntersections; meshesIntersect; faceNormalsApprox, vertexNormalsApprox; meshComponents; SideOfTriangleMesh: locate; meshHausdorffDistanceApprox; gaussianCurvaturesApprox, meanCurvaturesApprox; isotropicRemeshingApprox; triangulateFaces (CGAL).
- new Esoteric row `75-point_set_processing.hpp`: averageSpacingApprox; gridSimplify; hierarchySimplify; removeOutliers; pcaEstimateNormalsApprox; jetEstimateNormalsApprox; mstOrientNormals; jetSmoothApprox; bilateralSmoothApprox; clusterPointSet (CGAL).
- new Esoteric row `76-polygon_distance_advanced.hpp`: grenanderDistanceApprox (inner common tangents and boundary chains) (WIKI).
- The 2026-09-27 exclusion of CGAL packages for no contest use is lifted (user decision 2026-11-10): rows 28, 30, 33 and 45 take Spatial Sorting, Polygon Repair, Snap Rounding and 2D mesh refinement, and rows 68–75 the other families; Interval Skip List is Data Structures row 68 `contains`.
- maspypy `minimum_three_distance_sum` is the sum at `fermatPoint` (row 25); the incremental anchored rectangle union is row 13 (no Data Structures row owned it); common tangents of two convex polygons are row 17 (row 53 lists only `DynamicHull::bridge`).
- Not returned after the P275 review: `TriangleApprox::circumcircle` and `incircle` (the P009 prune removed these exact names; `omissions.md` Open questions, with P013's `multisetPermutationCount`).