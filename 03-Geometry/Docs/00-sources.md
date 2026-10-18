# 03 Geometry — sources

## Catalog pages fetched on 2026-10-06

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker geo problems (repository listing; the SPA home page did not render) | https://github.com/yosupo06/library-checker-problems/tree/master/geo | 2026-10-06 | all_furthest_neighbors, closest_pair, convex_layers, count_points_in_triangle, euclidean_mst, furthest_pair, manhattanmst, minimum_enclosing_circle, sort_points_by_argument, static_convex_hull mapped to rows 29, 05, 20, 22, 07, 21, 14, 01, 04 |
| cp-algorithms index | https://cp-algorithms.com/ | 2026-10-06 | Geometry section: line/segment/circle intersections, tangents, union of segments, convex polygon O(log n), Minkowski, Pick, non-lattice polygon lattice points, hull, sweep intersection, planar faces, point location, closest pair, Delaunay/Voronoi, vertical decomposition, half-plane S&I, Manhattan distance, minimum enclosing circle |
| KACTL geometry directory | https://github.com/kth-competitive-programming/kactl/tree/main/content/geometry | 2026-10-06 | Angle, CirclePolygonIntersection, PolygonCut, PolygonUnion, LineHullIntersection, PointInsideHull, kdTree, sphericalDistance, 3dHull, PolyhedronVolume, FastDelaunay, ManhattanMST, linearTransformation |
| OI Wiki geometry index | https://oi-wiki.org/geometry/ | 2026-10-06 | 2D/3D basics, distance, Pick, triangulation, hull, sweep line, calipers, half-plane, closest pair, randomized incremental, enclosing circle, inversion |
| OI Wiki distance | https://oi-wiki.org/geometry/distance/ | 2026-10-06 | Manhattan/Chebyshev conversion and L1 diameter (row 21) |
| OI Wiki inversion | https://oi-wiki.org/geometry/inverse/ | 2026-10-06 | invertPoint/invertCircle/invertLine and Apollonius reduction (row 24) |
| Nyaan library index | https://nyaannyaan.github.io/library/ | 2026-10-06 | geometry/circle, geometry-base, integer-geometry, line, polygon, segment |
| Nyaan circle.hpp, integer-geometry.hpp, polygon.hpp | https://nyaannyaan.github.io/library/geometry/circle.hpp (and sibling pages) | 2026-10-06 | intersect/crosspoint, ArgumentSort, LowerHull/UpperHull, contains_polygon, contains_convex, convex_polygon_diameter |
| maspypy library index | https://maspypy.github.io/library/ | 2026-10-06 | geo/ and geo3d/ files: angle_sort, apollonian_circle, closest_pair, convex_layers, convex_polygon, convex_polygon_dp_order, convex_polygon_union_area, count_points_in_triangles, delaunay (incl. convex polygon variant), dynamic_upper_hull, furthest_pair, incremental_convex_hull, lower_convex_hull_segtree, manhattan_mst, manhattan_nns, max_norm_sum, min_slope_sum, minimum_enclosing_circle, minkowski_sum, perpendicular_bisector, pick_formula, polygon_triangulation, range_closest_pair_query, rotating_swaps |
| maspypy convex_polygon.hpp | https://maspypy.github.io/library/geo/convex_polygon.hpp | 2026-10-06 | side, min_dot/max_dot, visible_range, boundary_cross_line, area_between, left_area (row 15 operations) |
| ei1333 library index | https://ei1333.github.io/library/ | 2026-10-06 | geometry/ files: angle, convex-layers, convex_polygon_contains/cut/diameter, cross_point_cc/cl/cs/ll, distance_ll/lp/pp/sp/ss, is_intersect_*, is_orthogonal, is_parallel, common_area_cp |
| suisen library index | https://suisen-cp.github.io/cp-library-cpp/ | 2026-10-06 | geom/ (closest_pair, convex_hull, geometry, geometry3d, segment_intersections) and integral_geom/ (farthest_pair, convex_hull_inclusion, inclusion, sort_points_by_argument) |
| suisen geometry.hpp, geometry3d.hpp | https://suisen-cp.github.io/cp-library-cpp/library/geom/geometry.hpp (and geometry3d.hpp) | 2026-10-06 | excircles, tangent_num, convex_cut, intersection_area; 3D dist/projection/on_triangle/cross_point |
| hitonanode library index and geometry.hpp | https://hitonanode.github.io/cplib-cpp/ and https://hitonanode.github.io/cplib-cpp/geometry/geometry.hpp | 2026-10-06 | point2d_w_error, point3d, problem_of_apollonius, sort_by_argument, triangle; convex_cut, open-segment intersection |
| tko919 library index, Enclosing.hpp, geometry.hpp | https://tko919.github.io/library/ (and Geometry/Enclosing.hpp, Geometry/geometry.hpp) | 2026-10-06 | Enclosing (Welzl), HalfplaneIntersection, Cut, Closest, InterArea, UnionArea, tangent |
| noshi91 library index | https://noshi91.github.io/Library/ | 2026-10-06 | no geometry directory |
| AtCoder Library docs | https://atcoder.github.io/ac-library/production/document_en/ | 2026-10-06 | no geometry module |
| CSES problem set | https://cses.fi/problemset/ | 2026-10-06 | Geometry section: point location test, segment intersection, polygon area, point in polygon, polygon lattice points, minimum Euclidean distance, convex hull, maximum/all Manhattan distances, intersection points, line segments trace I/II, lines and queries I/II, area of rectangles, robot path |
| CGAL package overview | https://doc.cgal.org/latest/Manual/packages.html | 2026-10-06 | Contest-relevant packages: kernels, 2D/3D/dD hulls, polygons (partition, straight skeleton, Minkowski, polyline simplification, Fréchet, visibility), arrangements/envelopes, triangulations (2D/3D, constrained, alpha shapes), Voronoi (segment, L-infinity, Apollonius), spatial searching, bounding volumes/optimization, kinetic data structures |
| Shewchuk robust predicates | https://www.cs.cmu.edu/~quake/robust.html | 2026-10-06 | orient2d/orient3d/incircle/insphere families, extended-precision register caveat (row 36) |

## P007 completeness sweep, fetched 2026-10-07

| Source | URL | Used for |
|---|---|---|
| KACTL Point.h, Point3D.h | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/Point.h, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/Point3D.h | `unitApprox` (`unit`), `argApprox` (`angle`) |
| maspypy geo/base.hpp | https://maspypy.github.io/library/geo/base.hpp | `argApprox` (`angle`), `canonicalLine` (`normalize`) |
| Nyaan geometry-base.hpp | https://nyaannyaan.github.io/library/geometry/geometry-base.hpp | `argApprox` (`arg`) |
| suisen geom/geometry.hpp | https://suisen-cp.github.io/cp-library-cpp/library/geom/geometry.hpp | `argApprox` (`arg`) |
| cp-algorithms, segment to line | https://cp-algorithms.com/geometry/segment-to-line.html | `canonicalLine` integer normalization |
| ei1333 line.hpp, ccw.hpp | https://ei1333.github.io/library/geometry/line.hpp, https://ei1333.github.io/library/geometry/ccw.hpp | line equation form; five-way ccw (not adopted) |
| KACTL PolygonCut.h | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PolygonCut.h | `polygonCutApprox` (keeps the left side here, KACTL keeps the right) |
| maspypy geo/convex_hull.hpp, suisen geom/convex_hull.hpp | https://maspypy.github.io/library/geo/convex_hull.hpp, https://suisen-cp.github.io/cp-library-cpp/library/geom/convex_hull.hpp | `convexHullIndices` (index-returning hull) |
| maspypy geo/polygon.hpp, inside_polygon.hpp | https://maspypy.github.io/library/geo/polygon.hpp, https://maspypy.github.io/library/geo/inside_polygon.hpp | segment-in-polygon and polygon-line pieces (not adopted) |
| CGAL Convex_hull_2 reference | https://doc.cgal.org/latest/Convex_hull_2/group__PkgConvexHull2Ref.html | Melkman hull and lower/upper hull (not adopted) |

## P008 completeness sweep, fetched 2026-10-07

| Source | URL | Used for |
|---|---|---|
| CP-Algorithms, nearest points; OI Wiki nearest points | https://cp-algorithms.com/geometry/nearest_points.html, https://oi-wiki.org/geometry/nearest-points/ | 3D closest pair (divide and conquer); minimum-perimeter triangle and randomized grid (not adopted) |
| maspypy geo/closest_pair.hpp; suisen geom/closest_pair.hpp | https://maspypy.github.io/library/geo/closest_pair.hpp, https://suisen-cp.github.io/cp-library-cpp/library/geom/closest_pair.hpp | randomized expected-linear closest pair and floating closest pair (not adopted) |
| AOJ CGL_7_A, Nyaan geometry/circle.hpp | https://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=CGL_7_A, https://nyaannyaan.github.io/library/geometry/circle.hpp | `circleRelation` (4, 3, 2, 1, 0 common tangents) |
| ei1333 geometry/circle.hpp | https://ei1333.github.io/library/geometry/circle.hpp | `circleSegmentIntersectionApprox` (`cross_point_cs`), circle-line/circle-circle cross-checks |
| maspypy geo/apollonian_circle.hpp | https://maspypy.github.io/library/geo/apollonian_circle.hpp | ratio locus circle (not adopted here; row 24) |
| maspypy geo/furthest_pair.hpp; Library Checker furthest_pair | https://maspypy.github.io/library/geo/furthest_pair.hpp, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/furthest_pair/task.md | `farthestPair` (point-cloud wrapper with original indices) |
| Toussaint 1983, Solving geometric problems with the rotating calipers; Wikipedia, Rotating calipers | https://www.cs.swarthmore.edu/~adanner/cs97/s08/pdf/calipers.pdf, https://en.wikipedia.org/wiki/Rotating_calipers | two-polygon calipers problems (not adopted; rows 04, 11, 17, 25) |
| CGAL Aff_transformation_2 | https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Aff__transformation__2.html | line/direction images, map equality and exact rational transforms (not adopted) |

## P009 completeness sweep, fetched 2026-10-07

| Source | URL | Used for |
|---|---|---|
| CGAL Kernel global functions (`angle`, `compare_angle`) | https://doc.cgal.org/latest/Kernel_23/group__kernel__global__function.html | `Triangle2::angleKind` (exact acute/right/obtuse classification) |
| suisen geom/geometry.hpp; hitonanode geometry/triangle.hpp | https://suisen-cp.github.io/cp-library-cpp/library/geom/geometry.hpp, https://hitonanode.github.io/cplib-cpp/geometry/triangle.hpp | centroid, orthocenter and excenters (already in the row); incenter and circumcenter through row 06 `incircleApprox` and `circumcircleApprox` |
| ei1333 geometry/angle.hpp; OI Wiki 2D geometry | https://ei1333.github.io/library/geometry/angle.hpp, https://oi-wiki.org/geometry/2d/ | vertex angles (covered by `01-point.hpp` `angleApprox`; not adopted) |
| CGAL Triangle_2; KACTL geometry listing; maspypy triangle_area.hpp | https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Triangle__2.html, https://github.com/kth-competitive-programming/kactl/tree/main/content/geometry, https://maspypy.github.io/library/geo/triangle_area.hpp | orientation, bounded side and transforms (owned by rows 02, 03 and 08); no further triangle operations |

## P036 completeness sweep, fetched 2026-10-10

Five `@researcher` passes: one each for rows 10, 11, 16 and 17, and one for the Lee–Preparata case analysis. Decisions are in [00-notes.md](00-notes.md#p036-completeness-decisions-2026-10-10).

| Source | URL | Used for |
|---|---|---|
| cp-algorithms half-plane intersection; OI Wiki 半平面交; tko919 geometry.hpp and FracCoord.hpp | https://cp-algorithms.com/geometry/halfplane-intersection.html, https://oi-wiki.org/geometry/half-plane/, https://tko919.github.io/library/Geometry/geometry.hpp | Row 10: deque conventions, same-direction handling, coefficient constructor (not adopted), box helper (not adopted) |
| CGAL Nef_2 | https://doc.cgal.org/latest/Nef_2/index.html | Row 10: symbolic infimaximal box (contrast with exact unbounded kinds) |
| Preparata & Shamos, Section 7.2.6 (book pp. 299–306) | https://www.cs.kent.edu/~dragan/CG/CG-Book.pdf | Row 10: `polygonKernelLinear` case analysis and linear-bound argument |
| Bornstein & Bruckstein (Pattern Recognition 24(11), 1991); Schuierer (Nordic J. Computing 1, 1994) | https://freddy.cs.technion.ac.il/wp-content/uploads/2018/01/finding-the-kernel-of-planar-shapes.pdf, https://www.cs.helsinki.fi/njc/njc1_papers/number4/paper4.pdf | Row 10: existence of the wrap test and turning-angle conditions (the 6π guard here is derived independently) |
| cp-algorithms Minkowski sum; maspypy geo/minkowski_sum.hpp; OI Wiki convex hull (闵可夫斯基和); CGAL Minkowski_sum_2 | https://cp-algorithms.com/geometry/minkowski.html, https://maspypy.github.io/library/geo/minkowski_sum.hpp, https://oi-wiki.org/geometry/convex-hull/, https://doc.cgal.org/latest/Minkowski_sum_2/index.html | Row 11: edge merge, start conventions, family sums (CF 87E), nonconvex methods (not adopted) |
| KACTL CirclePolygonIntersection.h; ei1333 common_area_cp.hpp; AOJ CGL_7_H; CGAL Boolean_set_operations_2 | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/CirclePolygonIntersection.h, https://ei1333.github.io/library/geometry/common_area_cp.hpp, https://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=CGL_7_H, https://doc.cgal.org/latest/Boolean_set_operations_2/index.html | Row 16: fan decomposition; region-valued circle-segment booleans (not adopted) |
| Wikipedia rotating calipers; CGAL Polytope_distance_d and do_intersect | https://en.wikipedia.org/wiki/Rotating_calipers, https://doc.cgal.org/latest/Polytope_distance_d/index.html, https://doc.cgal.org/latest/Boolean_set_operations_2/group__boolean__do__intersect.html | Row 17: maximum distance (adopted), separating strip and common tangents (not adopted), closed versus open intersection semantics |

## P037 completeness sweep, fetched 2026-10-18

Five `@researcher` passes, one each for rows 13, 14, 15, 19 and 54. Decisions are in [00-notes.md](00-notes.md#p037-completeness-decisions-2026-10-18). Pages that failed to load are listed as such.

| Source | URL | Used for |
|---|---|---|
| OI Wiki 扫描线; cp-algorithms segment union length | https://oi-wiki.org/geometry/scanning/, https://cp-algorithms.com/geometry/length-of-segments-union.html | Row 13: area and perimeter sweeps (POJ 1177), 1D union |
| CSES 1740, 1741, 1742; AOJ CGL_6_A, DSL_4_A, DSL_5_B | https://cses.fi/problemset/task/1740, https://cses.fi/problemset/task/1741, https://cses.fi/problemset/task/1742, https://judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=CGL_6_A&lang=en | Row 13: orthogonal counts (touching semantics), union area; Robot Path (not adopted) |
| Library Checker area_of_union_of_rectangles task | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/data_structure/area_of_union_of_rectangles/task.md | Row 13: `rectangleUnionArea` |
| maspypy ds/rectangle_union.hpp, ds/incremental_rectangle_union.hpp, other/cuboid_union_volume.hpp; ei1333 union-rectangle.hpp; hitonanode integer_segments.hpp; suisen geom/segment_intersections.hpp | https://maspypy.github.io/library/ds/rectangle_union.hpp, https://maspypy.github.io/library/ds/incremental_rectangle_union.hpp, https://maspypy.github.io/library/other/cuboid_union_volume.hpp, https://ei1333.github.io/library/structure/others/union-rectangle.hpp, https://hitonanode.github.io/cplib-cpp/utilities/integer_segments.hpp, https://suisen-cp.github.io/cp-library-cpp/library/geom/segment_intersections.hpp | Row 13: union area; incremental and 3D unions (not adopted); integer coverage reading; closed orthogonal count |
| Barbay, Pérez-Lantero, Rojas-Ledesma, depth distribution | https://arxiv.org/abs/1705.10022 | Row 13: all-depth distribution bound (not adopted) |
| Library Checker minimum_enclosing_circle (task, checker, reference solution) | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/task.md, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/sol/correct.cpp | Row 14: exact predicates, every point on the circle (`locate`) |
| KACTL MinimumEnclosingCircle.h; cp-algorithms enclosing circle; OI Wiki random incremental; maspypy minimum_enclosing_circle; tko919 Enclosing.hpp | https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/MinimumEnclosingCircle.h, https://cp-algorithms.com/geometry/enclosing-circle.html, https://oi-wiki.org/geometry/random-incremental/, https://maspypy.github.io/library/geo/minimum_enclosing_circle.hpp, https://tko919.github.io/library/Geometry/Enclosing.hpp | Row 14: three-loop form, epsilon versus exact predicates |
| CGAL Bounding_volumes, Min_circle_2, Min_sphere_of_spheres_d | https://doc.cgal.org/latest/Bounding_volumes/index.html, https://doc.cgal.org/latest/Bounding_volumes/classCGAL_1_1Min__circle__2.html, https://doc.cgal.org/latest/Bounding_volumes/classCGAL_1_1Min__sphere__of__spheres__d.html | Row 14: exact bounded side and support; circles of circles, p-centres (not adopted) |
| KACTL LineHullIntersection.h, PointInsideHull.h; cp-algorithms point in convex polygon | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/LineHullIntersection.h, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PointInsideHull.h, https://cp-algorithms.com/geometry/point-in-convex-polygon.html | Row 15: line intersection results, fan location |
| maspypy geo/convex_polygon.hpp; ei1333 convex_polygon_contains, convex_polygon_cut; hitonanode geometry.hpp; Nyaan polygon.hpp | https://maspypy.github.io/library/geo/convex_polygon.hpp, https://ei1333.github.io/library/geometry/convex_polygon_contains.hpp, https://ei1333.github.io/library/geometry/convex_polygon_cut.hpp, https://hitonanode.github.io/cplib-cpp/geometry/geometry.hpp, https://nyaannyaan.github.io/library/geometry/polygon.hpp | Row 15: `side`, `min_dot`/`max_dot`, `visible_range`, `boundary_cross_line`, `area_between`, `left_area`, `O(n)` cuts |
| CGAL Inscribed_areas; AKMSW (SMAWK applications); Keikha et al. 1705.11035; Kallus 1706.03049; Jin et al. 1707.04071; Rote 1905.11203 | https://doc.cgal.org/latest/Inscribed_areas/group__PkgInscribedAreasRef.html, https://hcsoso.github.io/teaching/topology-f21/papers/SMAWK.pdf, https://arxiv.org/abs/1705.11035, https://arxiv.org/html/1706.03049, https://arxiv.org/abs/1707.04071, https://arxiv.org/html/1905.11203v1 | Row 15: k-gon bounds, the Dobkin–Snyder flaw, linear triangle and quadrilateral algorithms and enclosing triangles (not adopted) |
| cp-algorithms lattice points; cp-algorithms continued fractions; OI Wiki 类欧几里得 and Pick | https://cp-algorithms.com/geometry/lattice-points.html, https://cp-algorithms.com/algebra/continued-fractions.html, https://oi-wiki.org/math/number-theory/euclidean/, https://oi-wiki.org/geometry/pick/ | Row 19: per-edge floor sums, boundary treatment, lattice hull under a line (not adopted) |
| maspypy convex/lattice_point_count.hpp, lattice_point_sum_polynomial.hpp, nt/convex_floor_sum.hpp; Nyaan enumerate-convex.hpp; 259-momone blog | https://maspypy.github.io/library/convex/lattice_point_count.hpp, https://maspypy.github.io/library/convex/lattice_point_sum_polynomial.hpp, https://maspypy.github.io/library/nt/convex_floor_sum.hpp, https://nyaannyaan.github.io/library/math/enumerate-convex.hpp, https://259-momone.hatenablog.com/entry/2021/07/25/025655 | Row 19: halfplane counts, Stern–Brocot hull walks, polynomial sums (not adopted) |
| AtCoder ABC191 D; sethigris/gauss-circle | https://atcoder.jp/contests/abc191/tasks/abc191_d, https://github.com/sethigris/gauss-circle | Row 19: rational-centre disk task; origin hull walk |
| USACO Guide LeetCode 1453 solution; arXiv 1312.0389; arXiv 2509.16008 | https://usaco.guide/problems/lc-maximum-number-of-darts-inside-of-a-circular-dartboard/solution, https://arxiv.org/html/1312.0389v1, https://arxiv.org/html/2509.16008 | Row 54: floating angular sweep and tie rule; `O(n^2)` bound and variants (not adopted) |
| CGAL Spatial_searching; OI Wiki nearest points; maspypy geo/closest_pair.hpp | https://doc.cgal.org/latest/Spatial_searching/index.html, https://oi-wiki.org/geometry/nearest-points/, https://maspypy.github.io/library/geo/closest_pair.hpp | Row 54: fixed-radius search ownership (row 28), grid hashing |
| Failed fetches | LeetCode 1453 (HTTP 403), poj.org 1981 (refused), min-25 blog (404), QOJ 6188 (403), Springer BF02574380 (login) | Statements of those pages are not relied on |

## Source keys used in the inventory

KACTL, STANFORD (jaehyunp/stanfordacm notebook), CPALG (cp-algorithms), OI-GEO (OI Wiki geometry), CGAL, SHEWCHUK, MASPYPY, EI1333, SUISEN, HITONANODE, TKO (tko919), LC (Library Checker), CSES. Keys identify research starting points, not correctness evidence. CGAL supplies breadth and contracts, never an automatic dependency.

## Previous audit narrative (condensed)

The 2026-09-27 cross-library audit ([2026-09-27-inventory-audit.md](../../00-Guidelines/History/2026-09-27-inventory-audit.md), metadata in [research-sources.json](../../00-Guidelines/Ledgers/research-sources.json)) inspected catalogs, selected API/comments and named paper abstracts; proofs remain part of the implementation batches. Package-level provenance for the verified rows (KACTL PDF pp. 16–20, Stanford notebook pp. 6–7, cp-algorithms hull/area/Pick/closest-pair/circle pages, KACTL CircleTangents/HullDiameter/ClosestPair, CGAL min_quadrilateral_2, MathWorld circle/triangle pages, Kahan's needle-triangle paper) is recorded with read scope in the per-header evidence documents [01-point.md](01-point.md) through [09-triangle.md](09-triangle.md). Industrial CAD, molecular surfaces, remeshing/visualization and machine-learning reconstruction remain outside this contest inventory. Geometry snippets in `OLD/Team Notebook/src/geometry`, `algs.cpp` and `algsbetter.cpp` are legacy references only.

## Not adopted

- maspypy `minimum_three_distance_sum`: single-problem curiosity without a reusable family.
- maspypy `convex_polygon_edge_voronoi`: nearest-edge diagram of a convex polygon; no contest use case beyond `15` distance queries.
- maspypy `definite_integral`, `modint_real`: numerical integration is Mathematics `17-numerical_methods.hpp`; modular reals are not geometry.
- hitonanode `point2d_w_error` as a public type: absorbed as the `IntervalPoint2` filter inside row 36 rather than a separate header.
- Nyaan `LowerHull`/`UpperHull` as public API: internal passes of `convexHull`; `HullSegtree` in row 20 covers range lower/upper hulls.
- KACTL `Angle` class: `polarLess` and `canonicalDirection` cover its use; no separate angle type.
- CSES "Lines and Queries I/II": convex hull trick, owned by Data Structures `22-convexhulltrick.hpp`.
- cp-algorithms "Finding faces of a planar graph": Graphs `29-planar_graph.hpp`.
- cp-algorithms "Convex hull trick and Li Chao tree": Data Structures `21-lichao.hpp`, `22-convexhulltrick.hpp`.
- CGAL 2D Snap Rounding, Polygon Repair, Movable Separability, Interval Skip List, natural-neighbour Interpolation, Spatial Sorting, periodic/hyperbolic triangulations, mesh generation, shape reconstruction, polygon mesh processing, point set processing: no contest algorithmic use case.
- CGAL Linear and Quadratic Programming Solver: Mathematics `28-simplex.hpp`.
- OI Wiki 随机增量法 as a separate header: covered by `14` (Welzl) and `31` (incremental Delaunay).
- Lattice points on a circle boundary (sum of two squares): Mathematics `57-quadratic_integer.hpp`; only disk counts are in row 19.
