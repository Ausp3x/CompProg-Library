# 07-rotatingcalipers.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `05-closestpair.hpp`, `06-circle.hpp` and `08-coordinate_transform.hpp`); see [00-notes.md](00-notes.md#p008--ge02-package-record) for the package record and the folder numeric policy. `degenerateBox` is listed in the row because the contest struct exposes it. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. Every support/optimum choice of `ConvexCalipers`, and `farthestPair`, is exact: signed integral coordinates, and every predicate widens to `lll` before arithmetic. Metrics and vertices of the `Approx` results are `long double`.

All selection predicates are exact on `|x|, |y| <= 10^9`:

- Edge components and differences are at most `2 * 10^9` in absolute value.
- Dot and cross products, squared lengths and projection ranges are at most `8 * 10^18`.
- Width and area numerators are at most `6.4 * 10^37`, and the squared half-perimeter numerator is at most `2.56 * 10^38`. Both fit unsigned 128 bits.

Cross-multiplying two such fractions can exceed 128 bits, which is why `ratioLess` compares them by continued fractions.

### ConvexDiameter, ConvexWidthApprox, CaliperBoxApprox

- `ConvexDiameter { first, second, squared }` stores an exact squared distance and sorted input indices. Empty input gives `-1, -1, 0`; a singleton gives `0, 0, 0`.
- `ConvexWidthApprox { width, edge, opposite }` stores a rounded width with its supporting edge and opposite vertex. The witnesses are `-1/-1` for an empty hull, `0/0` for a singleton and `0/1` for a segment, all with width `0`.
- `CaliperBoxApprox { exists, vertices, area, perimeter, edges }` stores CCW approximate vertices. `exists` is false only for an empty hull. A singleton box repeats `p[0]` four times. A segment box is `{p0, p1, p1, p0}`, with area `0` and perimeter twice the segment length. For fewer than three hull points, `edges` stays `{-1, -1}`, because no hull edge supports the box. Rectangles use `edges[0]` (the flush edge); a parallelogram uses both.

### ConvexCalipers

The constructor copies a strictly convex CCW cyclic hull: no repeated start vertex, no collinear vertices, any starting vertex. Use `convexHull(points, false)` for an arbitrary cloud. It accepts signed integral `T` with `|x|, |y| <= 10^9` and `n <= INT_MAX / 2`. Empty, singleton and two-distinct-point hulls are supported. Assertions catch the coordinate bound, a repeated two-point input and nonpositive consecutive turns. Simplicity and convexity of the cycle remain input preconditions. Construction is `O(n)` with a monotone opposite-support pointer, and no query mutates state. Indices refer to the input hull.

- `ratioLess(a, b, c, d)` compares `a / b < c / d` exactly for `b, d > 0` (asserted) by continued fractions, `O(1)` words for 128-bit operands.
- `antipodalPairs()` returns each distinct unordered pair `i < j` that admits parallel opposite support lines, including all four endpoint combinations at parallel-edge ties. There are `O(n)` pairs. A singleton has none; a segment has `{0, 1}`.
- `diameter()` returns the maximum exact squared distance and the lexicographically smallest sorted index pair among ties.
- `minimumWidthApprox()`, `minimumAreaRectangleApprox()`, `minimumPerimeterRectangleApprox()` and `minimumAreaParallelogramApprox()` (with `rectangleApprox(bool perimeter)` as the shared rectangle routine and `degenerateBox()` for fewer than three points) select their optimum exactly. Equal optima keep the first support configuration encountered. Metrics and vertices are converted to long double only after selection, so they may lose relative accuracy on very thin hulls; selection is exact.

Correctness:

- Opposite support indices advance monotonically around a convex polygon. For vertex `i`, every antipodal partner lies between the support of the preceding edge and that of the following edge, including the second endpoint of a tied support edge. Enumerating that interval and keeping `i < j` yields each unordered pair once. Every diameter pair is antipodal.
- The minimum-width strip has a hull edge on one boundary. Minimum-area rectangles have a hull edge flush with one side. Minimum-perimeter rectangles do too: between support events the half-perimeter is `a cos(theta) + b sin(theta)`, which is concave, so its minimum is at an event. Edge-aligned boxes use three monotone support pointers and exact rational objective comparisons.
- Minimum parallelogram: for `D = P - P` and a normal `a`, let `w(a)` be the unnormalized strip width. The parallelogram with normals `a` and `b` has area `w(a) w(b) / |det(a, b)|`, the reciprocal of `|det(a / w(a), b / w(b))|`, a determinant of two boundary points of the polar of `D`. Its maximum over a compact convex polygon is attained at vertices, and those vertices are exactly the distinct rays `+-e_i / w_i` (rotated by a common quarter turn). After merging the two cyclic orders of `e_i` and `-e_i` and removing same-ray duplicates, the maximizing partner advances monotonically, so the whole search is `O(n)` including ties. The final corners intersect the two exact support bands before conversion to long double.

### farthestPair

`farthestPair(const vector<Point2<T>> &p)` takes any point cloud with integral `|x|, |y| <= 10^9`, duplicates and collinear points included. It returns the original indices of a farthest pair: the lexicographically smallest sorted pair among all pairs of maximum exact squared distance. For `n >= 2` the indices are distinct, so if all points coincide the result is `{0, 1, 0}`. Empty input gives `{-1, -1, 0}` and a singleton `{0, 0, 0}`. It runs in `O(n * log(n))` time and `O(n)` memory: `convexHullIndices(p, false)` (first-occurrence indices), then the calipers' antipodal pairs mapped back. Every farthest pair joins two strict hull vertices, and each vertex's smallest index is its first occurrence, so the scan finds the lexicographically smallest pair. The header includes `04-convexhull.hpp` for this function.

## Feature-to-test map

`96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py` drives the C++ suite and Python integer cases. The corpus: all 512 subsets of a 3x3 grid, each hull at every cyclic start; 1,000 random clouds with cyclic, rotation and scale/translation variants; 250 thin high-dynamic-range hulls; 10,000 full-128-bit ratio checks; and 3,296 Python integer cases per build. Stress uses all 65,536 4x4 subsets and eight times the random rounds.

| Operation | Test | Oracle |
|---|---|---|
| `ConvexDiameter`, `diameter` and its index ties | Empty, singleton and segment cases | All vertex pairs with independently expanded exact products |
| `antipodalPairs` and tie events | Every input edge direction, all min/max support vertex combinations | Exact set equality and no duplicates |
| `ConvexWidthApprox`, `minimumWidthApprox` | Exact optimum and opposite-support witness; degenerate witnesses `-1/-1`, `0/0`, `0/1` | All edges and vertices, independent 256-bit fraction oracle |
| `CaliperBoxApprox`, `rectangleApprox`, `minimumAreaRectangleApprox`, `minimumPerimeterRectangleApprox` | Every edge and point as the four supports; exact selected objective; corners, containment, orthogonality, area and perimeter | Exhaustive support enumeration |
| `minimumAreaParallelogramApprox` | Exact optimum; corner and enclosure properties | Every distinct edge pair with independent 256-bit comparisons |
| `degenerateBox` | For fewer than three points, all three box queries have `edges == {-1, -1}` and the stated vertex layout | Stated layout |
| `ratioLess` | Edge cases and random full 128-bit operands | Python arbitrary-precision products and the C++ schoolbook 256-bit product |
| `farthestPair` | Fixed clouds and 1,000 random clouds (dense, `{-1,0,1}^2`, collinear `y = 3` and `10^9`-scale, half duplicated), each also shuffled; input preserved; `int` coordinates | All-pairs scan (first strictly larger distance in `(i, j)` order) |
| Preconditions | Six checked-build probes: clockwise, collinear, duplicate two-point input, coordinate bound, zero fraction denominator, `farthestPair` coordinate bound | Expected assertion failure |
| Scale | Strict parabola with 10,001 vertices (full), every query, linear antipodal output bound | Same oracles |

## Commands and results

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 is not installed; no GCC 14 run is claimed.

| Command | Result |
|---|---|
| `07-rotatingcalipers_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 631,872 checks per configuration; 3,296 Python cases; 6 assertion probes |
| `07-rotatingcalipers_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 31,047,696 checks; 21,296 Python cases |
| Mutation checks, optimized full suite per mutant | Caught: `farthestPair`'s index sorting and initial pair, the singleton width witness |
| Warning sweep, every exact operation for `int8_t`, `int16_t`, `int`, `lng` plus every approximate operation, `-Wall -Wextra -Wconversion` | No header warnings (the tester has one `-Wconversion` warning) |
| Two-translation-unit program mixing all four GE02 headers (optimized, ASan/UBSan); standalone header | PASS |
| `02-integration.py --sanitizers`, geometry quick run, `03-consistency.py` (with `--braces`) | PASS, no errors |
| `@reviewer`: 300k `farthestPair` clouds against an all-pairs oracle | No mismatch |

```bash
python3 '96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[5-8]*.hpp '96-Local Testing/03-Geometry/'0[5-8]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

```sh
g++ -std=gnu++20 -O2 -DNDEBUG '96-Local Testing/03-Geometry/07-rotatingcalipers_benchmark.cpp' -o /tmp/p008-calipers-benchmark
/tmp/p008-calipers-benchmark
```

The input is a strictly convex integer parabola (every point a hull vertex). Setup is timed separately, and a warmup precedes seven measurements; the table gives medians. The brute force checks every edge pair with the same exact comparator, and its objective is checked against the polar result. The checksum is `2.00271e+12`. 2026-10-07, i9-11900H, GCC 16.2.1:

| n | setup µs | polar µs | all edge pairs µs | brute / polar |
|---:|---:|---:|---:|---:|
| 32 | 2.355 | 4.199 | 2.975 | 0.71 |
| 128 | 5.383 | 16.077 | 44.362 | 2.76 |
| 512 | 11.186 | 67.293 | 736.326 | 10.94 |
| 2,048 | 42.230 | 380.741 | 13,935.3 | 36.60 |
| 8,192 | 217.343 | 1,746.10 | 248,167 | 142.13 |

The linear algorithm is kept at every size, without a second implementation or threshold.

## Sources

Inspected on 2026-09-27:

- [KACTL HullDiameter.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/HullDiameter.h): the strict hull contract and the `O(n)` antipodal scan. No code was copied.
- CGAL `min_quadrilateral_2` [documentation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/doc/Bounding_volumes/CGAL/min_quadrilateral_2.h) and [implementation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/include/CGAL/min_quadrilateral_2.h) (`min_parallelogram_2`, lines 444–623): the API contracts and linear claims. The polar reduction used here was derived independently. The papers CGAL cites were not read.
- The `farthestPair` sources are listed in [00-sources.md](00-sources.md#p008-completeness-sweep-fetched-2026-10-07); candidates not adopted (two-polygon problems) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07). `OLD/algorithms.cpp` and `OLD/[1] algorithms.cpp` contain no caliper code.

## Limits and handoffs

Other packages own convex polygon queries (GE05), two-polygon calipers problems (rows 04, 11, 17 and 25) and polygon distance (GE25).

## History

- 2026-09-27: original P008, full suite (seed 20260927, 626,364 checks) passed on g++ 16; `farthestPair` missing, degenerate witnesses untested.
- 2026-10-07: re-audit, implemented `farthestPair`; 4 findings fixed (degenerate witnesses documented and tested, bare closing braces in header, tester and benchmark); full suite, stress and benchmark passed on g++ 16.
