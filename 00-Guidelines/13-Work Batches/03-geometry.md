# 03-Geometry implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../03-Geometry/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| GE01 | `01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp`, `04-convexhull.hpp` | L | C01 | Point, intersection, polygon and convex hull foundations; settle exact/floating policy. |
| GE02 | `05-closestpair.hpp`, `06-circle.hpp`, `07-rotatingcalipers.hpp`, `08-coordinate_transform.hpp` | L | C01, GE01 | Closest pair, circle, calipers and transforms, including minimum enclosing rectangles/parallelograms and support-event ties. |
| GE03 | `10-halfplaneintersection.hpp`, `11-minkowskisum.hpp` | L | C01, GE01 | Halfplanes and Minkowski sum; GE01. |
| GE04 | `12-segmentintersection.hpp` | XL | C01, GE01, GE11 | Segment intersection sweep with exact events; GE01, GE11. |
| GE05 | `13-segment_union.hpp`, `14-minimumenclosingcircle.hpp`, `15-convex_polygon_query.hpp` | L | C01, GE01, GE02 | Segment/rectangle sweep, enclosing circle and convex queries; inscribed k-gon optimization has separate domain and complexity contracts. |
| GE06 | `30-polygon_boolean.hpp` | XL | C01, GE01, GE11, GE16 | Polygon Boolean operations, holes and robust topology using the common arrangement representation. |
| GE07 | `31-delaunay.hpp` | XL | C01, GE01, GE11 | Delaunay triangulation and predicates; GE11. |
| GE08 | `32-voronoi.hpp` | L | C01, GE01, GE07 | Voronoi from Delaunay; GE07. |
| GE09 | `21-manhattan_geometry.hpp`, `22-geometricmst.hpp` | M | C01, GE01, GE07 | Manhattan and Euclidean geometric MST; GE01/GE07 where useful. |
| GE10 | `23-circleunion.hpp` | L | C01, GE01, GE02 | Circle union/coverage area, perimeter, intersection and maximum-depth variants with distinct bounds. |
| GE11 | `36-exact_predicates.hpp` | XL | C01, GE01 | Adaptive exact predicates; central geometry dependency. |
| GE12 | `37-convexhull3d.hpp` | XL | C01, GE01, GE11 | 3D convex hull and oriented adjacency; GE11. |
| GE13 | `38-halfspaceintersection3d.hpp` | XL | C01, GE01, GE11, GE12 | 3D halfspace intersection; GE11–GE12. |
| GE14 | `39-visibilitygraph.hpp` | L | C01, GE01 | Visibility graph with obstacle-boundary policy. |
| GE15 | `42-randomizedlp.hpp` | L | C01, GE01 | Low-dimensional randomized LP. |
| GE16 | `33-linearrangement.hpp` | L | C01, GE01, GE11 | Line/segment arrangements and rational crossings. |
| GE17 | `28-spatial_index.hpp` | L | C01, GE01 | Spatial indexing and range/nearest queries. |
| GE18 | `27-spherical_geometry.hpp` | M | C01, GE01 | Spherical geometry and antipodal cases. |
| GE19 | `40-kinetic_geometry.hpp` | L | C01, GE01 | Kinetic geometry and exact-time certificates. |
| GE20 | `41-geometric_duality.hpp` | M | C01, GE01 | Geometric duality and envelope research. |
| GE21 | `43-minimumwidthannulus.hpp` | XL | C01, GE01, GE16, GE36 | Minimum-width annulus and arrangement optimization. |
| GE22 | `44-algebraic_geometry.hpp` | XL | C01, GE01 | Algebraic curves/root isolation after a contest use case. |
| GE23 | `09-triangle.hpp` | M | C01, GE01, GE02 | triangle. Complete all selected inventory contracts and variants. point; circle |
| GE24 | `16-circle_polygon.hpp` | M | C01, GE01, GE02 | circle polygon. Complete all selected inventory contracts and variants. circle; polygon |
| GE25 | `17-polygon_distance.hpp` | L | C01, GE01, GE02, GE03 | polygon distance. Complete all selected inventory contracts and variants. rotatingcalipers; minkowskisum |
| GE26 | `18-polygontriangulation.hpp` | XL | C01, GE01, GE11 | polygontriangulation. Complete all selected inventory contracts and variants. polygon; exact_predicates |
| GE27 | `19-lattice_geometry.hpp` | L | C01, GE01 | lattice geometry. Complete all selected inventory contracts and variants. polygon; Mathematics floor_sum |
| GE28 | `20-convex_hull_updates.hpp` | XL | C01, GE01, GE05 | convex hull updates. Complete all selected inventory contracts and variants. convexhull; convex_polygon_query |
| GE29 | `24-circle_constructions.hpp` | L | C01, GE01, GE02, GE11 | circle constructions. Complete all selected inventory contracts and variants. circle; exact_predicates |
| GE30 | `25-geometric_median.hpp` | L | C01, GE01, GE23 | geometric median. Complete all selected inventory contracts and variants. triangle; Mathematics numerical_optimization |
| GE31 | `26-geometry3d.hpp` | XL | C01, GE01, GE11 | geometry3d. Complete all selected inventory contracts and variants. point; exact_predicates |
| GE32 | `29-point_set_queries.hpp` | XL | C01, GE01, GE17 | Point-set range, nearest/farthest and angular queries; largest empty rectangle with a bounding box, obstacle-contact policy and optimum witness. |
| GE33 | `34-pointlocation.hpp` | XL | C01, GE01, GE11, GE16 | pointlocation. Complete all selected inventory contracts and variants. linearrangement; exact_predicates |
| GE34 | `35-visibilitypolygon.hpp` | L | C01, GE01, GE04, GE26 | visibilitypolygon. Complete all selected inventory contracts and variants. polygontriangulation; segmentintersection |
| GE35 | `45-constraineddelaunay.hpp` | XL | C01, GE01, GE07, GE26 | constraineddelaunay. Complete all selected inventory contracts and variants. delaunay; polygontriangulation |
| GE36 | `46-weighted_voronoi.hpp` | XL | C01, GE01, GE08, GE11 | weighted voronoi. Complete all selected inventory contracts and variants. voronoi; exact_predicates |
| GE37 | `47-delaunay3d.hpp` | XL | C01, GE01, GE11, GE31 | delaunay3d. Complete all selected inventory contracts and variants. geometry3d; exact_predicates |
| GE38 | `48-polygonoffset.hpp` | XL | C01, GE01, GE03, GE06 | polygonoffset. Complete all selected inventory contracts and variants. polygon_boolean; minkowskisum |
| GE39 | `49-minimumenclosingellipse.hpp` | XL | C01, GE01, GE15 | minimumenclosingellipse. Complete all selected inventory contracts and variants. randomizedlp; Core matrix |
| GE40 | `50-alphashape.hpp` | XL | C01, GE01, GE07, GE37 | alphashape. Complete all selected inventory contracts and variants. delaunay; delaunay3d |
| GE41 | `51-polyhedron_boolean.hpp` | XL | C01, GE01, GE11, GE13, GE31 | polyhedron boolean. Complete all selected inventory contracts and variants. geometry3d; halfspaceintersection3d; exact_predicates |
| GE42 | `52-curve_distance.hpp` | XL | C01, GE01 | curve distance. Complete all selected inventory contracts and variants. line_segment; Mathematics numerical_search |
| GE43 | `53-dynamicconvexhull.hpp` | XL | C01, GE01, GE11, GE28 | dynamicconvexhull. Complete all selected inventory contracts and variants. convex_hull_updates; exact_predicates |
