# P007 / GE01 geometry foundations

This package owns `01-point.hpp`, `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`. Its prerequisite is P002/C01's current template. The implementations use ordinary scalar GNU C++20 and have no dependency on a planned Core rational type or on the later exact-predicate package. All four owned headers are verified on their documented domains; no GE01 implementation or verification gaps remain.

## Numeric and topology contracts

`Point2<T>` and `Point3<T>` support signed integral and floating coordinate types. Integral dot products, determinants, squared distances and coordinate differences inside predicates widen to `lll` before arithmetic. General primitive inputs must keep every intermediate representable in that promoted type; vector arithmetic itself must fit its stored `T`. Cast values must be in the destination type's range; floating destinations may round. Coordinates of absolute value at most `10^9` are a sufficient shared domain, including the cubic tetrahedral determinant. Integral squared quantities are exact; roots and APIs ending in `Approx` return ordinary approximate `long double` values.

Exact line topology and lattice-count APIs require integral coordinates with absolute value at most `10^9`. Point primitives, hulls and the other integral polygon routines also permit wider coordinates when every intermediate fits `lll`. Linear predicate products have degree two. An intersection numerator has degree three and magnitude at most `24 * 10^27`, below the signed 128-bit limit. Polygon first-moment sums have terms of magnitude at most `4 * 10^27`; even `INT_MAX` total vertices fit signed 128 bits. These bounds cover intermediate arithmetic, not just final answers.

Floating coordinates and all floating intermediates must be finite. These routines use ordinary rounded arithmetic and computed signs, without epsilon equality, an error certificate, or adaptive precision. Near cancellation, their topology may differ from exact real arithmetic. Floating lexicographic ordering compares coordinates directly. `polarLess` and both hull algorithms use integral coordinates so rounding cannot violate sorting's strict weak ordering. Robust predicates for arbitrary floating input remain GE11's separate scope.

`RationalPoint2` stores normalized homogeneous coordinates `(x/d, y/d)` with positive `d`; equality compares this canonical representation. Its constructor rejects zero denominator and the minimum signed 128-bit value. Public fields must retain the normalization invariant. It is a construction result, not a general rational arithmetic package, and deliberately avoids potentially overflowing rational cross-products. Its `approx()` explicitly converts to floating coordinates.

`Linear2` describes a closed segment, a ray from `a` through `b`, or a complete line. Equal endpoints describe a singleton for all three kinds. `intersect` handles all nine kind combinations and distinguishes empty, point, segment, ray and line results. Only the result field associated with its kind is meaningful. Overlap endpoints reuse integral inputs; segment overlap endpoints are lexicographically sorted. Nonparallel crossings return a rational point. Zero directions count as parallel; `collinear` asks whether all endpoints lie on one common line, so any two singleton objects are collinear. Projection, reflection and signed line distance require distinct endpoints; positive signed distance is to the directed line's left. Segment metrics allow singleton segments.

Polygon rings omit a repeated closing vertex. Signed moments and winding can describe an oriented closed walk; geometric area, centroid, convexity and Pick interpretations require the topology documented by each function. Simple-polygon validation is intentionally owned by GE04's intersection sweep. For a polygon with holes, the outer ring is counterclockwise and holes clockwise, with simple, disjoint, strictly interior, non-touching holes. Signed moment sums subtract holes. Zero signed area makes centroid construction return `false`. Boundary, interior and exterior are separate containment results; crossings use a half-open vertical interval to avoid counting vertices twice.

Both hull functions remove duplicate points and return a canonical counterclockwise cycle beginning at the lexicographically smallest vertex, without a repeated closing vertex. The default removes intermediate collinear boundary points; the retention option keeps each distinct boundary point. Empty/singleton inputs remain empty/singleton, and an entirely collinear input returns sorted distinct points when retaining them, or its two endpoints otherwise. The caller's input is unchanged.

## Correctness and cost

The point predicates are the standard determinant and scalar-product identities, with promotion occurring before subtraction or multiplication. Polar sorting partitions directions into two half-planes and compares exact determinant signs within each half; squared length breaks equal-direction ties, with the zero vector first. This supplies a strict weak order without an angle calculation.

For nonparallel linear objects, Cramer's rule gives two parameters over the same determinant. Making the denominator positive reduces segment/ray membership to exact integer comparisons. For collinear non-singleton objects, lexicographic order is monotone along their shared line; intersecting their finite or infinite endpoint bounds yields the exact overlap kind. Singleton membership is handled before division. A disjoint segment pair attains its minimum distance at an endpoint projected onto the other segment, while an intersecting pair has distance zero.

Polygon area and first moments sum signed edge wedges. Dividing the first-moment numerators by `3 * area2` gives the centroid; linear summation handles oriented holes. Winding counts upward and downward crossings with opposite signs, checking boundary membership first. Pick's theorem gives `I = (abs(area2) - B) / 2 + 1` for a simple lattice ring, with `B` the sum of edge gcds; subtracting holes changes the constant to `1 - h`. Simple-polygon convexity follows consistent nonzero turn signs, with backtracking and repeated-edge degeneracies rejected.

Monotone chain maintains the lower/upper hull by removing turns that violate convexity. Each point enters and leaves each stack at most once. Graham scan uses an extreme pivot, exact angular order and a final-ray reversal when retaining collinear boundary points. Both take `O(n * log(n))` time and `O(n)` storage including output. Polygon traversal costs are linear with constant auxiliary storage; lattice gcd work adds `O(log(C))` per edge. Fixed-size point and metric operations take constant time/storage; rational normalization takes `O(log(C))` Euclidean steps.

## Feature-to-test map

The four matching Python entries under `96-Local Testing/03-Geometry` provide quick/full/stress modes, deterministic seeds, non-removable test oracles and assertion-failure subprocesses. Full mode runs optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer builds. Quick omits sanitizers and reduces exhaustive/random corpora; stress extends the corpora. Each entry resolves paths from its own location.

| Header | Covered public features and boundaries | Independent evidence |
|---|---|---|
| `01-point.hpp` | 2D/3D construction, casts, arithmetic/mutation, equality and lexicographic order; dot/cross, squared and approximate norms/distances, orientation, perpendicular vectors, scalar triple product, tetrahedral determinant; integral polar order, zero vectors and equal-angle ties | Exact integer reference records, small exhaustive predicates/order properties, algebraic identities, coordinate-boundary and floating cases |
| `02-line_segment.hpp` | Rational normalization/equality/conversion; constructors and membership; parallel/collinear and every line/segment/ray intersection combination; all five output kinds, reversed operands/directions, singleton degeneracies; projection/reflection and all distances | Independent implicit-line equations and rational membership, exhaustive small endpoints, seeded boundary/random cases and exact reference metric calculations |
| `03-polygon.hpp` | Signed area/moments, exact and approximate centroid including unchanged output on zero area; winding/nonzero-fill containment/boundary; perimeter, strict/weak convexity, lattice gcd counts/Pick, oriented holes and all ring overloads | Unions of unit grid cells independently supply area, first moments, occupancy, perimeter and enumerated lattice points; reversal, triangle barycenters, floating dyadic and wide-coordinate regressions |
| `04-convexhull.hpp` | Both hulls, canonical orientation/start, duplicates, empty/singleton, all-collinear and both retention policies; original-input preservation | Independent supporting-edge/Jarvis oracle, exhaustive subsets and seeded adversarial point sets; hull-to-hull agreement is supplemental |

Finite tests complement the arguments above; they do not prove all inputs. Approximate checks use scale-aware tolerances on the specified fixtures and do not establish a universal error bound for ill-conditioned floating inputs.

## Verification results

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (`20260810`), Python 3.14.7. GNU++20 is used throughout. GCC14 was unavailable, so this records GCC16 execution rather than claiming a GCC14 run. No online submission was made.

| Entry | Full seed `20260927` | Coverage detail |
|---|---|---|
| `01-point_tester.py` | Passed optimized, checked and ASan/UBSan; 242,646 checks per configuration, 2 checked assertion probes | 6,022 arbitrary-precision Python records; integer `int`/`lng`/`lll`, `float`/`double`/`long double`, constexpr behavior, exhaustive polar order and scale variation |
| `02-line_segment_tester.py` | Passed optimized, checked and ASan/UBSan; 1,152,578 checks per configuration, 15 checked assertion probes | 54,256 exact Fraction topology fixtures and 3,902 80-digit Decimal metric fixtures; all factories and singleton kinds |
| `03-polygon_tester.py` | Passed optimized, checked and ASan/UBSan; 289,431 checks per configuration, 5 checked assertion probes | 120 exhaustive and 180 seeded histogram-cell polygons, two holes, half-grid/lattice enumeration, 10,000 extreme-triangle repetitions for moment accumulation |
| `04-convexhull_tester.py` | Passed optimized, checked and ASan/UBSan; 1,151,038 checks per configuration | 512 subsets of a 3-by-3 grid, 1,000 seeded/permuted/rotated sets and 20,001-point collinear/parabolic inputs |

Point additionally passed optimized stress with seed `42`: 857,926 checks, 30,022 Python records and 15,000 seeded C++ cases. Full geometry suites use leak checking; sanitizer runs affected by the sandbox's LeakSanitizer/ptrace restriction were retried outside the sandbox with approval, preserving sanitizer coverage.

Reproduction commands from the repository root:

```bash
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/02-line_segment_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/03-polygon_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

The repository integration run passed 69 standalone/aggregate headers, combined aggregates linked across two translation units in scalar and available AVX2 configurations, and the workspace in LOCAL/non-LOCAL builds. Geometry itself has no ISA-specific paths. A final focused check also passed all four headers, both geometry aggregates and a two-translation-unit point/polygon/rational-intersection smoke. The shared runner passed all four quick suites from `/tmp` with seed `20260927`, verifying discovery, environment-provided mode/seed and working-directory independence. Repository consistency passed after the final inventory updates.

## Hull comparison

The [benchmark driver](<../96-Local Testing/03-Geometry/90-foundations_benchmark.py>) and [record](<../96-Local Testing/03-Geometry/90-foundations_benchmark.json>) compare both implementations at 32, 2,048 and 32,768 points on random coordinates, duplicate-heavy grids, collinear points and parabolic convex-position points, with both retention policies: 48 measurements. Each uses one warmup and five measured calls, reports the median, includes input copy/output allocation and checks every complete canonical output outside the timed interval. Compiler flags are `-std=gnu++20 -O2 -DNDEBUG`; the record includes seed, CPU/compiler, input generation settings, code hashes and an observed checksum. Both algorithms use `O(n)` storage; no measured peak-RSS claim is made.

The measurements support monotone chain as the default. Graham remains an explicit alternative. Collinear inputs share preprocessing and their small timing differences are noise-sensitive. Timings are observations on a shared machine, not universal speed claims or pass/fail gates. At 32,768 points without collinear retention, the recorded medians were:

| Distribution | Monotone chain | Graham scan |
|---|---|---|
| Random | 3.52 ms | 7.77 ms |
| Duplicate-heavy grid | 2.83 ms | 4.51 ms |
| Collinear | 2.15 ms | 2.10 ms |
| Convex parabola | 2.43 ms | 4.44 ms |

The benchmark record's hashes match the final point/hull/benchmark sources.

```bash
python3 '96-Local Testing/03-Geometry/90-foundations_benchmark.py' --seed 20260927
```

## Sources and legacy accounting

Implementations were independently written from the geometric identities and algorithms below; notebook implementations were reviewed as comparison evidence, not copied as correctness guarantees.

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 point/angle, line/segment, polygon area/centroid/containment and hull sections, plus p. 20 Point3D/PolyhedronVolume. Relevant snippets were read for operations, overflow warnings and degeneracy conventions. The local download's provenance is in [resource metadata](../95-Resources/99-sources.json).
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7: projection, line/segment intersection, centroid and point-in-polygon. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- [cp-algorithms: Convex hull construction](https://cp-algorithms.com/geometry/convex-hull.html), displayed update September 19, 2026: Graham/Andrew algorithms, final angular-ray reversal for collinear retention and Andrew's all-collinear special case, inspected September 27, 2026.
- [cp-algorithms: Area of simple polygon](https://cp-algorithms.com/geometry/area-of-simple-polygon.html), displayed update June 8, 2022: edge-sum area formulas, inspected September 27, 2026.
- [cp-algorithms: Pick's theorem](https://cp-algorithms.com/geometry/picks-theorem.html), displayed update September 18, 2026: simple lattice-polygon formula and its scope, inspected September 27, 2026. The holes formula above is derived by subtracting each hole's interior and boundary lattice points; it is not attributed to an unread source extension.

The preserved `OLD/Team Notebook/src/geometry/point.cpp`, `intersection.cpp` and `polyarea.cpp`, plus their counterparts in `algs.cpp` and `algsbetter.cpp`, were inspected. Their 2D vector arithmetic, dot/cross, segment intersection, line intersection and absolute area are covered by the new APIs with explicit widening and degeneracy contracts. Historical names such as `Point`, `PointD`, `sgn`, `polyArea` and `isIntscLinSeg` are not active compatibility APIs; no active header depends on them. `point` remains the integral point alias, and `dpoint` now explicitly denotes long-double approximation.

Circle-line and circle-circle routines located in the old `intersection.cpp` are now covered by GE02 / P008 in `06-circle.hpp`; [P008 evidence](91-ge02.md) resolves that handoff outside GE01. The old coefficient-form line intersection should be migrated by consumers to endpoint-defined `Linear2`; this package does not reinterpret inexact coefficient input as exact geometry. All legacy bytes remain untouched.

## Remaining ownership and handoff

GE01 is complete with no failing reproducer or unfinished owned feature. Separate planned owners retain adaptive/exact floating predicates (P128 / GE11), simple-polygon validation and intersection sweeps (P129 / GE04), general rational-lattice counts (P037 / GE27), and higher-dimensional hulls (P137 / GE12). Output-sensitive Chan hull research remains a separately scoped esoteric alternative, as specified by the Basic hull inventory; it is not required by GE01. These are not silent reductions of the four owned Basic contracts.
