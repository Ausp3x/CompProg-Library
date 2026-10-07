# P007 / GE01 geometry foundations

This package owns `01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`. Its prerequisite is P002/C01's current template. The implementations use ordinary scalar GNU C++20 and do not depend on a Core rational type or on the later exact-predicate package (P128 / GE11). The 2026-10-07 re-audit implemented the ten operations the earlier inventory had left `missing`, and it added six more from a completeness sweep. Every operation in all four rows now has a test. See [Re-audit — 2026-10-07](#re-audit--2026-10-07).

## Contracts

### Common numeric policy

- `Point2<T>` and `Point3<T>` accept signed integral and floating `T`. Integral dot products, determinants, squared distances and coordinate differences inside predicates widen to `lll` before any arithmetic. Every intermediate must fit `lll`, and vector arithmetic must fit the stored `T`. Coordinates with `|x| <= 10^9` are sufficient for every operation in the package, including the cubic `tetraVolume6`, the degree-3 intersection numerators (`<= 24 * 10^27`) and the polygon first moments (terms `<= 4 * 10^27`, summable over `INT_MAX` vertices).
- Exact line topology, `canonicalLine` and the lattice counts require integral coordinates with `|x| <= 10^9`, and they assert this bound. Point primitives, hulls and the other integral polygon routines also accept wider coordinates as long as every intermediate fits `lll`.
- Floating inputs and every floating intermediate and result must be finite. These routines use ordinary rounded arithmetic and computed signs. They use no epsilon, no error certificate and no adaptive precision, so near cancellation their topology can differ from exact real arithmetic. `polarLess` and the hulls are integral-only because floating rounding can break a sort's strict weak ordering. Robust floating predicates belong to P128 / GE11.
- An API ending in `Approx` returns `long double` values computed with ordinary rounding and no certified error bound. An approximate construction returns a line as two points (`array<dpoint, 2>`); the left side of a line is the positive side.

### Point2, Point3, point, dpoint

Value types with a defaulted lexicographic `<`, `==`, unary `-`, and compound and binary `+ - * /`. Unary minus and `perp` convert back to `T` explicitly. For `int8_t` and `int16_t`, the negated value must therefore fit `T`; this is the same contract as other vector arithmetic. Integral `/` truncates toward zero and asserts `k != 0`; quotients must fit `T`. `cast<U>()` requires values in `U`'s range, and floating destinations may round. `point` is `Point2<lng>` and `dpoint` is `Point2<long double>`.

### dot, cross, perp, norm2, dist2, orient, triple, tetraVolume6

These return exact `lll` results for integral `T` (a `Point3<lll>` for the 3D `cross`) and `long double` results otherwise. `perp` rotates counterclockwise by a quarter turn and keeps `T`. `orient(a, b, c)` returns the sign of `cross(b - a, c - a)`, where `+1` means counterclockwise. `triple(a, b, c) = a · (b × c)`. `tetraVolume6` is six times the oriented volume of `abcd`.

### normApprox, distanceApprox, unitApprox

Rounded square roots of the exact or long-double squared quantities. `unitApprox` (2D and 3D) returns `v / |v|` as a long-double point and asserts a nonzero norm.

### argApprox, angleApprox, signedAngleApprox

- `argApprox(v) = atan2(y, x)`, in `(-pi, pi]`; the negative x-axis gives `+pi`, even when `y` is a floating negative zero.
- `angleApprox(a, b) = atan2(|a × b|, a · b)`, in `[0, pi]`, for both 2D and 3D. The 3D form converts the exact cross product to long double before taking the norm, so it cannot overflow `lll`.
- `signedAngleApprox(a, b) = atan2(a × b, a · b)`, in `(-pi, pi]`. It is positive counterclockwise and returns `+pi` for opposite vectors. Adding `+0` turns a floating `-0` cross product into `+0`, so floating opposite vectors such as `(-1, 0), (1, 0)` also give `+pi`.
- All three assert nonzero vectors. The atan2 form keeps small angles accurate, unlike `acos` of a normalized dot product.

### polarLess

Integral only. It is a strict weak order by angle in `[0, 2 * pi)` starting at `+x`. The zero vector comes before every nonzero vector, and equal angles are ordered by squared length.

### canonicalDirection

Integral `T`; each coordinate must be above the minimum of `T` (asserted). The result is the input divided by `gcd(|x|, |y|)`, with its sign flipped so that `y > 0`, or `y == 0` and `x > 0`. Two vectors get equal keys exactly when they are parallel or antiparallel, so a line direction and its opposite share one key. `(0, 0)` maps to itself.

### RationalPoint2

Normalized homogeneous coordinates `(x/d, y/d)` with `d > 0` and `gcd(|x|, |y|, d) = 1`; equality compares this canonical form. The constructor asserts `d != 0` and that no component equals the minimum `lll` value. Code that assigns the public fields must preserve the invariant. It is a construction result, not a rational arithmetic package. `approx()` converts each component to long double independently.

### LinearKind, Linear2, IntersectionKind, LinearIntersection2

`Linear2` is a closed segment, a ray from `a` through `b`, or a full line. All endpoints are closed. For every kind, `a == b` is a singleton point. Integral endpoints must satisfy `|coordinate| <= 10^9`, and every linear operation asserts this and that the kind is valid. In `LinearIntersection2`, only the field matching `kind` has meaning: `p` for `Point`, `overlap` for `Segment`, `Ray` and `Line`. A segment overlap stores its endpoints in lexicographic order. An overlap reuses the original integral endpoints, so it is itself a valid `Linear2`.

### contains, onSegment, parallel, orthogonal, collinear

Exact. Membership covers singleton degeneracies. A zero direction is both parallel and orthogonal to every direction. `collinear` asks whether all endpoints lie on one common line, so any two singletons are collinear.

### intersect

Handles all nine segment/ray/line kind pairs and reports empty, point, segment, ray or line. A nonparallel crossing is an exact `RationalPoint2`. Predicates use degree-2 products and point numerators are degree 3. No rational is ever multiplied by another.

### canonicalLine

For `s.a != s.b` (asserted), returns `{A, B, K}` with `A x + B y = K` describing the supporting line, ignoring the kind. `(A, B) = canonicalDirection(perp(b - a))`, so `gcd(A, B) = 1` and `B > 0`, or `B == 0` and `A > 0`. Then `K = A a.x + B a.y`, with `|K| <= 4 * 10^18`. Two linear objects have equal keys exactly when their supporting lines coincide. The key is suitable as a map key for counting distinct lines.

### latticeOnSegment, gridCellsCrossed

The endpoints are lattice points with `|coordinate| <= 10^9`. `latticeOnSegment` counts the lattice points on the closed segment, `gcd(|dx|, |dy|) + 1`; a singleton gives 1. `gridCellsCrossed` counts the unit cells whose open interior meets the segment, `|dx| + |dy| - gcd(|dx|, |dy|)`. An axis-parallel or singleton segment crosses no cell interior and gives 0. Both return `lng`.

### projectLineApprox, reflectLineApprox, signedLineDistanceApprox

The line endpoints must differ and the direction's squared norm must be finite and nonzero; this also excludes squared underflow. The signed distance is positive to the left of the directed line `a -> b`.

### reflectDirectionApprox

Reflects the direction vector `v` across the direction of line `ab` (`a != b`): `2 (v · u / |u|^2) u - v` with `u = b - a`. This is the outgoing direction of a ray that hits a mirror along `ab`. `v` follows the same coordinate domain as points.

### lineDistanceApprox

Returns the distance between line `ab` (`a != b`) and line `cd`. It is 0 when the lines are not parallel. Otherwise it is `|signed distance of c from ab|`. `c == d` is allowed and gives the distance from point `c` to line `ab`. For integral inputs the parallel test is exact, because long-double differences and products of `|coordinate| <= 10^9` integers are exact.

### perpendicularBisectorApprox

For `a != b`, returns `{m, m + perp(b - a)}` with `m = (a + b) / 2`. Points closer to `a` lie to the left.

### angleBisectorApprox

For `a != o` and `b != o`, returns `{o, o + w * max(|oa|, |ob|)}`, where `w` is along the internal bisector of the angle `aob` and `|w|` lies in `[sqrt(2), 2]`. For angles of at most a right angle, `w` is the sum of the two unit arms. For obtuse angles it is the perpendicular of their difference, which avoids cancellation near a straight angle. At an exactly straight angle (exact for integral inputs), it is the counterclockwise normal of `oa`. The second point lies at least the longer arm's length away. The direction recovered from the two points therefore has relative error about `ulp(coordinate) / arm length`.

### pointSegmentDistanceApprox, segmentDistanceApprox

Singleton segments are allowed. An interior distance uses the perpendicular height, which avoids subtracting nearly equal projected coordinates. `segmentDistanceApprox` takes integral endpoints and decides intersection exactly before computing approximate endpoint distances.

### Polygon conventions

- A ring has no repeated closing vertex, and the total vertex count is at most `INT_MAX`.
- Area, moments and winding apply algebraically to any oriented closed walk. Geometric area, centroid, convexity and Pick interpretations need a simple polygon. Simplicity validation belongs to P129 / GE04's sweep.
- A polygon with holes is `{outer CCW, holes CW}`. Its rings must be simple and disjoint, with holes that do not touch and lie strictly inside the outer ring. Signed sums subtract holes.
- In complexity comments, `n` is the total vertex count and `r` the ring count (`r = 1` for a single ring).

### PolygonMoments, polygonMoments, signedArea2, signedAreaApprox

`area2` is twice the signed area, and `x6` and `y6` are six times the signed first moments. All are exact for integral `T` within the common bound. The result is positive for a CCW simple polygon and zero for an empty walk. `signedAreaApprox` halves the area in long double.

### centroidExact, centroidApprox

Both return `false` when the signed area is zero (computed zero for the floating form) and then leave `out` unchanged. The exact form requires integral coordinates and supports signed rings. The `log(n * C^3)` term in its cost is the gcd normalization.

### PolygonLocation, WindingResult, polygonWinding, polygonContains

Winding counts upward and downward crossings using half-open vertical intervals, and checks the boundary first. When `boundary` is true, `winding` is unspecified. Empty, point and segment walks have no interior. The rings overload uses nonzero fill, and a point on any ring's boundary is reported as boundary.

### polygonPerimeter

Closed-walk length. An empty walk or a single point gives 0, two points give twice their distance, and the rings overload includes hole boundaries.

### polygonConvex

Assumes a simple ring, in either orientation; this is not a simplicity test. `strict` rejects collinear consecutive vertices, while the weak form allows forward-collinear ones. Fewer than 3 vertices, zero area, zero-length edges and backtracking always return `false`.

### latticeBoundary, latticeInterior

Integral coordinates with `|coordinate| <= 10^9`, asserted per vertex. `latticeBoundary` sums the edge gcds, which counts distinct boundary points only for simple, nonzero-area rings. `latticeInterior` uses Pick's theorem on a simple ring of nonzero area in either orientation. The rings overload requires one CCW outer ring and `r - 1` CW holes (asserted) and applies the Euler correction `1 - h`.

### polygonCutApprox

Takes a ring `p` and a directed line `a -> b` (`a != b`, asserted). Using the Sutherland–Hodgman step, it returns the part of `p` in the closed left half-plane. A kept vertex has `orient >= 0`, and a crossing point is inserted only on a strict sign change, so no crossing is duplicated. The output has at most `2 * n` vertices. It may contain collinear vertices and zero-width bridges along the cut line, and for a nonconvex input it may be a non-simple walk. For a simple input its signed area equals the area of the clipped region. Signs are exact for integral `T`. For `|coordinate| <= 10^9`, each crossing parameter is the quotient of two exactly computed long-double crosses, so it is rounded only once. Floating input uses computed signs.

### convexHull, convexHullIndices, convexHullGraham

Exact integral hulls, `n <= INT_MAX`. The orientation and squared-distance intermediates must fit `lll`, which `|coordinate| <= 10^9` guarantees. Each function works on a copy and removes duplicates. The output starts at the lexicographic minimum, runs CCW and never repeats its start. `keep_collinear` keeps every distinct boundary input point. For an entirely collinear set, the output is the two sorted endpoints, or every sorted distinct point when collinear points are kept. `convexHullIndices` returns the same cycle as `convexHull`, with each point named by its first occurrence in the input. Graham is the angular-scan alternative. Its integral angle comparison avoids non-transitive floating predicates.

## Correctness and cost

The point predicates are the standard determinant and scalar-product identities, with promotion before any subtraction or multiplication. Polar sorting splits directions into two half-planes and compares exact determinant signs within each half; squared length breaks equal-direction ties, with the zero vector first. This gives a strict weak order without computing an angle.

`canonicalDirection` divides by the gcd, which leaves a primitive vector. Two primitive vectors are parallel exactly when one is `±` the other, and the sign rule picks one representative per pair. `canonicalLine` applies this to the normal `perp(b - a)`. `K` is then determined by any point of the line, so equal keys mean equal normals up to sign and equal offsets, which is the same supporting line. The angle functions use `atan2(sin-proportional, cos-proportional)`, which is accurate at every angle. `acos` and `asin` lose accuracy near 0 and `pi`.

For lattice counts: the lattice points on a segment with primitive step `(dx, dy) / g` are its `g + 1` multiples. The segment crosses `|dx|` vertical and `|dy|` horizontal grid lines in its interior. Each crossing enters a new cell, except at the `g - 1` interior lattice points, where a vertical and a horizontal crossing coincide. Hence `|dx| + |dy| - g` cells.

For the bisectors: `|u| = |v| = 1` makes `u + v` orthogonal to `u - v`. The vector `u + v` has length at least `sqrt(2)` when `u · v >= 0`, and `u - v` has length at least `sqrt(2)` when `u · v < 0`. Each branch therefore takes the well-conditioned vector. The sign of `perp(±(u - v))` follows `cross(u - v, u + v) = 2 cross(u, v)`. The reflection identity is `r = 2 proj_u(v) - v`.

For nonparallel linear objects, Cramer's rule gives two parameters over the same determinant. Making the denominator positive reduces segment and ray membership to exact integer comparisons. For collinear non-singleton objects, lexicographic order is monotone along their shared line, so intersecting their finite or infinite endpoint bounds gives the exact overlap kind. Singleton membership is handled before any division. A disjoint segment pair attains its minimum distance at an endpoint projected onto the other segment, while an intersecting pair has distance zero.

Polygon area and first moments sum signed edge wedges. Dividing the first-moment numerators by `3 * area2` gives the centroid, and linear summation handles oriented holes. Winding counts upward and downward crossings with opposite signs, after checking boundary membership. Pick's theorem gives `I = (abs(area2) - B) / 2 + 1` for a simple lattice ring, where `B` is the sum of edge gcds; subtracting holes changes the constant to `1 - h`. Simple-polygon convexity means consistent nonzero turn signs, with backtracking and repeated edges rejected.

`polygonCutApprox` replaces each excursion into the open right half-plane with a path along the cut line. Every point in the open left half-plane keeps its winding number, and every point in the open right half-plane gets winding 0, because the new walk lies in the closed left half-plane. The signed area is the integral of the winding number. It therefore equals the area of the polygon intersected with the half-plane, even when bridges are present.

Monotone chain maintains the lower and upper hulls by removing turns that violate convexity. Each item enters and leaves each stack at most once. `hull_detail::chain` runs this over a vector of points or of indices through an accessor, so `convexHull` and `convexHullIndices` share one implementation. Ordering indices by `(point, index)` and then removing duplicates by point keeps the first occurrence. Graham scan uses an extreme pivot, exact angular order and a final-ray reversal when keeping collinear boundary points. All three hulls take `O(n * log(n))` time and `O(n)` storage including the output. Polygon traversals are linear, with constant auxiliary storage apart from the cut's output. Lattice gcd work adds `O(log(C))` per edge. Fixed-size point and metric operations take constant time and storage. Rational normalization and `canonicalDirection` take `O(log(C))` Euclidean steps.

## Feature-to-test map

Each of the four Python entries under `96-Local Testing/03-Geometry` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them. Each entry resolves paths from its own location.

| Header | Operations and boundaries covered | Independent evidence |
|---|---|---|
| `01-point.hpp` | 2D/3D construction, casts, arithmetic and mutation for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double`; equality and lexicographic order; `point`/`dpoint` alias identities (`static_assert`); dot/cross, squared and approximate norms and distances, `orient`, `perp`, `triple`, `tetraVolume6`; `polarLess` with zero vectors and equal-angle ties; `canonicalDirection` exhaustively on `[-8, 8]^2` in full mode (`[-12, 12]^2` in stress), on scaled primitives, random 10^18 values, and the `int`, `lng`, `lll`, `int8_t` and `int16_t` limits; `unitApprox` 2D/3D; `argApprox`, `angleApprox` 2D/3D, `signedAngleApprox` on the small lattice, on random 10^9 coordinates, and at straight, quarter, near-parallel (cross `-1`) and `1e-300` angles; 8 assertion probes (division by zero, zero vectors for each angle/unit form, `INT_MIN` direction); narrowing compiled as an error | Exact Python integer records; Leibniz homogeneous determinants; a parallel-class oracle that uses `cross == 0` and a brute-force divisor search instead of gcd; trigonometric identities (cosine and sine of the angle times both norms equal dot and cross) and agreement of `argApprox` with the exact `polarLess` order |
| `02-line_segment.hpp` | Rational normalization, equality and conversion; the factories and membership; `parallel`, `orthogonal`, `collinear`; every line/segment/ray intersection pair with all five output kinds, reversed operands, and singleton degeneracies; `canonicalLine` key equality against same-line bits, normalization, endpoint incidence and extreme constants; projection, reflection, signed and unsigned distances; `lineDistanceApprox` on parallel, antiparallel, crossing and point-line inputs; both bisectors including near-straight and exactly straight angles; `reflectDirectionApprox`; `latticeOnSegment` and `gridCellsCrossed` on all pairs of a 5×5 grid, random `[-12, 12]` pairs, and the `±10^9` diagonal and coprime extremes; 23 assertion probes | Python Fraction topology via implicit line equations; an orthogonality bit from exact dot products; lattice-point enumeration and exact Fraction cell-interior tests that do not use gcd; 80-digit Decimal references for every approximate construction, including a bisector reference built from the plain unit sum |
| `03-polygon.hpp` | Signed area and moments; exact and approximate centroids, including `out` left unchanged on zero area; winding, nonzero-fill containment and boundary; perimeter; strict/weak convexity; lattice gcd counts and Pick; oriented holes and every ring overload; `polygonCutApprox` against 14 lines per histogram polygon plus empty, edge-line, whole, wide-diagonal, nonconvex-bridge and floating regressions, the output-size bound, vertices on the closed left side, and an assertion probe for `a == b` | Unions of unit grid cells supply area, first moments, occupancy, perimeter and enumerated lattice points. The cut's expected area integrates each cell's clipped column heights piecewise-linearly, self-checked by complementary half-planes summing to 1 per cell. Also reversal, triangle barycenters, floating dyadic and wide-coordinate regressions |
| `04-convexhull.hpp` | All three hulls; canonical orientation and start; duplicates; empty and singleton input; all-collinear input and both retention policies; the caller's input unchanged; `convexHullIndices` ranges, first-duplicate indices, and agreement with the oracle hull on every case, including 20,001-point parabolas | Independent supporting-edge (Jarvis) oracle, exhaustive 3×3-grid subsets and seeded adversarial point sets; agreement between hull implementations is supplemental |

Finite tests supplement the arguments above; they do not prove correctness for all inputs. Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Verification record — 2026-09-27 (original P007)

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (`20260810`), Python 3.14.7, GNU++20. GCC 14 was unavailable. Full mode with seed `20260927` passed every configuration: point 242,646 checks, line_segment 1,152,578, polygon 289,431, hull 1,151,038. Point stress with seed `42` (optimized) also passed. The 2026-10-07 baseline rerun before the re-audit edits reproduced exactly these check counts.

## Hull comparison

The [benchmark driver](<../96-Local Testing/03-Geometry/90-foundations_benchmark.py>) and [record](<../96-Local Testing/03-Geometry/90-foundations_benchmark.json>) compare monotone chain and Graham scan at 32, 2,048 and 32,768 points. The inputs are random coordinates, duplicate-heavy grids, collinear points and parabolic convex-position points, each with both retention policies: 48 measurements. Each measurement uses one warmup and five timed calls and reports the median. The timing includes the input copy and output allocation, and every complete canonical output is checked outside the timed interval. Compiler flags are `-std=gnu++20 -O2 -DNDEBUG`. The record includes the seed, CPU and compiler, input generation settings, code hashes and an observed checksum. Both algorithms use `O(n)` storage; no peak-RSS claim is made.

The measurements support monotone chain as the default, and Graham remains an explicit alternative. Collinear inputs share preprocessing, so their small timing differences are noise-sensitive. Timings are observations on a shared machine, not universal speed claims or pass/fail gates. The table below gives medians at 32,768 points without collinear retention.

| Distribution | Monotone chain 2026-10-07 | Graham scan 2026-10-07 | Monotone chain 2026-09-27 | Graham scan 2026-09-27 |
|---|---|---|---|---|
| Random | 3.68 ms | 8.05 ms | 3.52 ms | 7.77 ms |
| Duplicate-heavy grid | 2.95 ms | 4.63 ms | 2.83 ms | 4.51 ms |
| Collinear | 2.21 ms | 2.21 ms | 2.15 ms | 2.10 ms |
| Convex parabola | 2.61 ms | 4.76 ms | 2.43 ms | 4.44 ms |

The 2026-10-07 rerun measured the shared `hull_detail::chain` refactor. Both algorithms moved by a similar 3–7%, including Graham, whose code did not change, and the monotone/Graham ratio held: 0.457 against 0.453 on random, 0.548 against 0.548 on parabola. The shift is attributed to machine state, not to the refactor. The record's hashes match the current point, hull and benchmark sources.

```bash
python3 '96-Local Testing/03-Geometry/90-foundations_benchmark.py' --seed 20260927
```

## Sources and legacy accounting

The implementations were written independently from the geometric identities and algorithms below. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The sources for the 2026-10-07 completeness sweep are listed in [81-sources.md](81-sources.md#p007-completeness-sweep-fetched-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (point and angle, line and segment, polygon area/centroid/containment, hull) and p. 20 (Point3D, PolyhedronVolume). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- [cp-algorithms: Convex hull construction](https://cp-algorithms.com/geometry/convex-hull.html), [Area of simple polygon](https://cp-algorithms.com/geometry/area-of-simple-polygon.html) and [Pick's theorem](https://cp-algorithms.com/geometry/picks-theorem.html), inspected September 27, 2026. The holes formula is derived here by subtracting each hole's interior and boundary lattice points.
- [KACTL PolygonCut.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PolygonCut.h), fetched 2026-10-07. KACTL keeps the right side and inserts on a `(a < 0) != (b < 0)` change. This library keeps the closed left side and inserts only on strict sign changes, so no crossing is duplicated.

The preserved `OLD/Team Notebook/src/geometry/point.cpp`, `intersection.cpp` and `polyarea.cpp`, and their counterparts in `algs.cpp` and `algsbetter.cpp`, were inspected. The new APIs cover their 2D vector arithmetic, dot and cross products, segment intersection, line intersection and absolute area, with explicit widening and degeneracy contracts. Historical names (`Point`, `PointD`, `sgn`, `polyArea`, `isIntscLinSeg`) are not compatibility APIs. Circle routines from the old `intersection.cpp` are covered by P008 in `06-circle.hpp` ([evidence](91-ge02.md)). Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`. All legacy bytes remain untouched.

## Re-audit — 2026-10-07

The re-audit compared every operation in the four rows against the code and tests before making changes. Gaps found: the ten operations listed as `missing` (`canonicalDirection`, `angleApprox`, `signedAngleApprox`, `orthogonal`, `lineDistanceApprox`, `perpendicularBisectorApprox`, `angleBisectorApprox`, `reflectDirectionApprox`, `latticeOnSegment`, `gridCellsCrossed`), and untested `point`/`dpoint` aliases. The completeness sweep (four `@researcher` passes, sources in [81-sources.md](81-sources.md)) added `unitApprox`, `argApprox`, `canonicalLine`, `polygonCutApprox` and `convexHullIndices`. Operations considered and not adopted are listed with reasons in [80-notes.md](80-notes.md#p007-completeness-decisions-2026-10-07).

### Confirmed findings and their disposition

| # | Finding | Disposition |
|---|---|---|
| 1 | `canonicalDirection`, `angleApprox`, `signedAngleApprox` absent | Implemented (plus `unitApprox`, `argApprox`), with an exhaustive parallel-class oracle and identity oracles |
| 2 | Seven line operations absent | All seven implemented, plus `canonicalLine`; enumeration, exact-bit and Decimal oracles |
| 3, 4 | Evidence claimed no gaps while rows were partial | Superseded: every operation is implemented and mapped above; status and handoff text rewritten |
| 5 | `point`/`dpoint` untested | `static_assert` of both alias identities in the point tester |
| 6 | Unary minus and `perp` narrow for `int8_t`/`int16_t` | `T(-x)` conversions (in range under the arithmetic contract). The tester compiles with `-Wnarrowing` as an error and runs `operators<int8_t>`/`operators<int16_t>` |
| 7 | Free functions lack complexity comments | Every function sits under a complexity line, one per group of functions with the same bound (the restyle convention, as in `08-monotone_stack.hpp`). One comment per function would exceed the 8% comment cap. Contracts moved to `## Contracts` |
| 8 | `RationalPoint2::approx` implicit `lll` to long double | Both operands cast explicitly; the header is warning-free under `-Wall -Wextra -Wconversion` |
| 9 | `LinearIntersection2` lacks a comment | Complexity and field-validity line added |
| 10 | `; }` closings | All four headers, the four C++ testers and the benchmark normalized; `03-consistency.py --braces` is clean |
| 11 | Undefined `r` in polygon comments | Every grouped polygon comment defines `n total vertices, r rings` |
| 12 | `polygonContains`/`WindingResult` lack comments | Both covered: `WindingResult` has its own line, and `polygonContains` sits in the winding group |

### Other changes

- The multi-line contract comment blocks were moved out of all four headers into `## Contracts`, and every header meets the comment cap.
- `RationalPoint2`'s constructor parameters are now lowercase and normalized before they are stored.
- The monotone chain was factored into `hull_detail::chain`, which `convexHull` and `convexHullIndices` share. `convexHull`'s behavior is unchanged, and its tester and benchmark outputs match.
- No public name or documented behavior changed.

### Commands and results

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable, so these are GCC 16 runs; no GCC 14 run is claimed. No online submission was made.

| Entry | Full, seed `20260927` (optimized, checked, ASan/UBSan) | Stress, seed `42`, optimized |
|---|---|---|
| `01-point_tester.py` | PASS, 884,220 checks per configuration, 8 assertion probes | PASS, 3,949,165 checks |
| `02-line_segment_tester.py` | PASS, 1,497,841 checks per configuration, 23 assertion probes; 54,256 Fraction topology, 3,902 metric, 681 lattice/cell and 6,044 construction fixtures | PASS, 2,362,935 checks |
| `03-polygon_tester.py` | PASS, 389,280 checks per configuration, 6 assertion probes | PASS, 2,439,453 checks |
| `04-convexhull_tester.py` | PASS, 1,292,646 checks per configuration | PASS, 10,264,103 checks |

The baseline before any edits passed with the 2026-09-27 counts. The line_segment, polygon and hull stress rounds ran before the review fixes. Those fixes did not change behavior in those headers: unqualified `abs` and a `lng` cast in five polygon size asserts. Point stress and every full run above were repeated after the fixes. Also run:

- The sanitized integration (102 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) passed.
- A two-translation-unit smoke that mixes all four headers passed.
- A warning sweep instantiated every operation for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double` under `-Wall -Wextra -Wconversion`. The headers produced no warnings.
- The brace and comment-cap check and repository consistency report no errors.

```bash
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/02-line_segment_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/03-polygon_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode stress --seed 42 --configuration optimized   # likewise 02, 03, 04
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

### Independent review — 2026-10-07

The `@reviewer` pass confirmed three defects, all now fixed:

1. `signedAngleApprox` returned `-pi` for floating opposite vectors, because the cross product evaluated to `-0`. Adding `+0.L` to the cross (and to `y` in `argApprox`) fixed it, with regressions for `(-1, 0), (1, 0)` and negative-zero inputs.
2. Unescaped `|` in the feature map broke the table width check.
3. `-Wsign-compare` fired on `p.size() <= INT_MAX - n` in five polygon asserts; these warnings predated the re-audit.

The reviewer's notes were also acted on. Qualified `std::abs` became unqualified `abs`, which is on the allowed list. The random `canonicalDirection` cases now also check primitivity. The shared geometry runner still compiles without warning flags, so the explicit sweep above is the warning evidence. The grouped complexity comment (finding 7) follows the restyle practice and passes the validator; the decision log does not record it yet.

## Remaining ownership and handoff

GE01 has no failing reproducer and no unfinished owned operation. Other planned packages own the following: adaptive and exact floating predicates (P128 / GE11), simple-polygon validation and intersection sweeps (P129 / GE04), general rational-lattice counts (P037 / GE27) and higher-dimensional hulls (P137 / GE12). Chan's output-sensitive hull is a separately scoped variant in row 41.
