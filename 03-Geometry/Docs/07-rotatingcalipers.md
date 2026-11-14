# 07-rotatingcalipers.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `05-closestpair.hpp`, `06-circle.hpp` and `08-coordinate_transform.hpp`); see [00-notes.md](00-notes.md#ge02-numeric-policy-p008) for the folder numeric policy. `degenerateBox` is listed in the row because the contest struct exposes it. Every operation in the row has a test with an independent oracle.

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
- `expectedBoxAreaApprox()` returns the expected area of the axis-aligned bounding box of the hull rotated by an angle uniform on `[0, 2 pi)`, in rounded `long double` (CF 1578F). Fewer than two points give 0, and a segment of length `L` gives `L^2 / pi`. It takes `O(n)` time, `2n` `atan2` calls and `O(n)` memory, and does not select anything exactly: it is an `Approx` metric only.

Correctness:

- Expected box area: with `h` the support function about `p[0]` and `w(t) = h(t) + h(t + pi)` the width in direction `t`, the box area is `w(t) w(t + pi / 2)`. Expanding the product and shifting the four terms over a full period gives `E = (1 / (2 pi)) * 4 * I = (2 / pi) * I`, with `I` the integral of `h(psi - pi / 2) h(psi)` over `[0, 2 pi)`. Parametrized by an edge-direction angle `psi`, `h(psi - pi / 2)` is attained by the vertex after the last edge whose direction is at most `psi`, and `h(psi)` by the vertex after the last edge whose direction is at most `psi + pi / 2`. Both sequences are the cyclically sorted edge angles, merged in `O(n)`: the second list takes `atan2` of each integer direction rotated exactly by `-pi / 2`, `(y, -x)`, and each list starts at the edge `polarLess` puts first, so a list never wraps inside, even when two nearly parallel edges round to the same angle (within `|x|, |y| <= 10^9` consecutive edge angles differ by at least about `5e-19` against an ulp of at most `4.3e-19`, a margin that rounding in `atan2` could still close); rounding then only reorders events by about an ulp, adding an integral over a span of that size. Between events the integrand is `(u . (sin psi, -cos psi)) (v . (cos psi, sin psi))` for the two fixed vertices `u`, `v`, whose antiderivative is closed form in `sin^2`, `sin * cos` and `psi`. Taking `h` about `p[0]` keeps every term within the hull's diameter. A segment is the 2-gon of its two opposite edges.
- Opposite support indices advance monotonically around a convex polygon. For vertex `i`, every antipodal partner lies between the support of the preceding edge and that of the following edge, including the second endpoint of a tied support edge. Enumerating that interval and keeping `i < j` yields each unordered pair once. Every diameter pair is antipodal.
- The minimum-width strip has a hull edge on one boundary. Minimum-area rectangles have a hull edge flush with one side. Minimum-perimeter rectangles do too: between support events the half-perimeter is `a cos(theta) + b sin(theta)`, which is concave, so its minimum is at an event. Edge-aligned boxes use three monotone support pointers and exact rational objective comparisons.
- Minimum parallelogram: for `D = P - P` and a normal `a`, let `w(a)` be the unnormalized strip width. The parallelogram with normals `a` and `b` has area `w(a) w(b) / |det(a, b)|`, the reciprocal of `|det(a / w(a), b / w(b))|`, a determinant of two boundary points of the polar of `D`. Its maximum over a compact convex polygon is attained at vertices, and those vertices are exactly the distinct rays `+-e_i / w_i` (rotated by a common quarter turn). After merging the two cyclic orders of `e_i` and `-e_i` and removing same-ray duplicates, the maximizing partner advances monotonically, so the whole search is `O(n)` including ties. The final corners intersect the two exact support bands before conversion to long double.

### farthestPair

`farthestPair(const vector<Point2<T>> &p)` takes any point cloud with integral `|x|, |y| <= 10^9`, duplicates and collinear points included. It returns the original indices of a farthest pair: the lexicographically smallest sorted pair among all pairs of maximum exact squared distance. For `n >= 2` the indices are distinct, so if all points coincide the result is `{0, 1, 0}`. Empty input gives `{-1, -1, 0}` and a singleton `{0, 0, 0}`. It runs in `O(n * log(n))` time and `O(n)` memory: `convexHullIndices(p, false)` (first-occurrence indices), then the calipers' antipodal pairs mapped back. Every farthest pair joins two strict hull vertices, and each vertex's smallest index is its first occurrence, so the scan finds the lexicographically smallest pair. The header includes `04-convexhull.hpp` for this function.

### farthestPairNaive

`farthestPairNaive(p)` scans every pair `i < j` in increasing order and keeps the first strictly larger squared distance, so it returns exactly `farthestPair`'s result: the same domain (signed integral, `|x|, |y| <= 10^9`, asserted for every point through `calipers_detail::bounded`, the check the `ConvexCalipers` constructor uses), the lexicographically smallest farthest pair, `{0, 0, 0}` for one point and `{-1, -1, 0}` for none. It is the lighter all-pairs form (user decision 2026-11-14, Priority): `O(n^2)` time, `O(1)` memory, no hull.

## Feature-to-test map

`96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py` drives the C++ suite and Python integer cases. The corpus: all 512 subsets of a 3x3 grid, each hull at every cyclic start; 1,000 random clouds with cyclic, rotation and scale/translation variants; 250 thin high-dynamic-range hulls; 10,000 full-128-bit ratio checks; and 3,296 Python integer cases per build. Stress uses all 65,536 4x4 subsets and eight times the random rounds.

| Operation | Test | Oracle |
|---|---|---|
| `ConvexDiameter`, `diameter` and its index ties | Empty, singleton and segment cases | All vertex pairs with independently expanded exact products |
| `antipodalPairs` and tie events | Every input edge direction, all min/max support vertex combinations | Exact set equality and no duplicates |
| `ConvexWidthApprox`, `minimumWidthApprox` | Exact optimum and opposite-support witness; degenerate witnesses `-1/-1`, `0/0`, `0/1` | All edges and vertices, independent 256-bit fraction oracle |
| `CaliperBoxApprox`, `minimumAreaRectangleApprox`, `minimumPerimeterRectangleApprox` | Every edge and point as the four supports; exact selected objective; corners, containment, orthogonality, area and perimeter | Exhaustive support enumeration |
| `minimumAreaParallelogramApprox` | Exact optimum; corner and enclosure properties | Every distinct edge pair with independent 256-bit comparisons |
| `degenerateBox` | For fewer than three points, all three box queries have `edges == {-1, -1}` and the stated vertex layout | Stated layout |
| `ratioLess` | Edge cases and random full 128-bit operands | Python arbitrary-precision products and the C++ schoolbook 256-bit product |
| `farthestPair`, `farthestPairNaive` | Fixed clouds and 1,000 random clouds (dense, `{-1,0,1}^2`, collinear `y = 3` and `10^9`-scale, half duplicated), each also shuffled; input preserved; `int` coordinates (both) and `int8_t` extremes (`farthestPairNaive`) | All-pairs scan (first strictly larger distance in `(i, j)` order) |
| `expectedBoxAreaApprox` | n = 0 and 1 on every hull (exactly 0); segments including both 10^9 diagonals, reversed (`L^2 / pi`); every hull `verify` sees (regressions, all 3x3 or 4x4 subsets, random and thin 10^9 hulls): `D^2 / pi <= E <= D^2` and `E` at least the minimum-area rectangle; rectangles from 1x1 to 2e9 x 2e9 (`ab + (a^2 + b^2) / pi`); tiny triangles at the origin and at a 10^9 corner; needles 10^9 long and 1 wide; lattice circles of radius 32,045 (324 vertices) and, in full and stress, 1,185,665 (972 vertices) scaled near 10^9 (`E` scales by `k^2`); parabolas of 101 to 30,001 vertices; every cyclic start, quarter turns, translations and a 7x scale | C++ quadrature: the rotated vertices' box area integrated by 10-point Gauss–Legendre between the kinks (edge normals and multiples of `pi / 2`), within `1e-15 D^2` (largest seen `2.8e-17 D^2`); Python oracle: critical angles collected independently, extreme vertices found by brute force at each interval's midpoint, the product integrated exactly with integer extents, within `1e-12 D^2` (largest seen `2.4e-15 D^2`); closed forms; invariances |
| Preconditions | Seven checked-build probes: clockwise, collinear, duplicate two-point input, coordinate bound, zero fraction denominator, `farthestPair` and `farthestPairNaive` coordinate bounds | Expected assertion failure |
| Type domains (`static_assert`) | Once per full or stress run, 6 `-fsyntax-only` compiles: `farthestPair` and `farthestPairNaive` compile for `Point2<lng>` and `Point2<int8_t>` and fail their `static_assert` for `Point2<double>` (unsigned types already fail `Point2`'s own) | Compiler verdict |
| Scale | Strict parabola with 10,001 vertices (full), every query, linear antipodal output bound | Same oracles |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 815 MB; optimized 0.2 s compile + 0.5 s run, checked 16.2 s compile + 0.5 s run, ASan-UBSan 18.2 s compile + 1.0 s run; MEMORY peak 1005 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 856 MB; optimized 0.3 s compile + 0.5 s run, checked 28.0 s compile + 0.9 s run, ASan-UBSan 28.5 s compile + 2.3 s run; MEMORY peak 1049 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 20.4 s compile + 0.1 s run, checked 23.1 s compile + 0.3 s run; MEMORY peak 811 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 822 MB; optimized 0.1 s compile + 2.5 s run, checked 8.4 s compile + 7.9 s run, ASan-UBSan 8.6 s compile + 25.3 s run; MEMORY peak 611 MB. Dated rows below stay those commands' latest results.

2026-11-14 (P008; experiment none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns.

| Command | Result |
|---|---|
| `07-rotatingcalipers_tester.py --mode full --seed 1` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 660,832 checks per configuration; 3,296 Python integer cases; Python box-area oracle on 423 hulls (1,689 calls with cyclic starts, quarter turns and translations); 7 assertion probes; 6 type-domain compiles; `MEMORY peak=378MB` |
| `CXX=g++-14 07-rotatingcalipers_tester.py --mode full --seed 2` (GCC 14.4.1) | PASS, 662,872 checks per configuration; the same Python counts, probes and compiles; `MEMORY peak=392MB` |
| `07-rotatingcalipers_tester.py --mode stress --seed 3` (all three configurations) | PASS, 32,114,132 checks per configuration; 21,296 Python integer cases; box-area oracle on 2,024 hulls (8,093 calls); `MEMORY peak=583MB`; run before the type-domain compiles were added, which change no other check |
| `05-extraction.py ops --filter 07-rotatingcalipers`; `testers --filter 03-Geometry/0` | PASS: 22 operations in 14 extractions on g++ and g++-14; 12 names, quick on the extracted build |
| `04-mutation.py run 03-Geometry/07-rotatingcalipers.hpp` | See Mutation testing |

```bash
python3 '96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py' --mode full --seed 2
python3 '96-Local Testing/03-Geometry/07-rotatingcalipers_tester.py' --mode stress --seed 3
python3 '96-Local Testing/05-extraction.py' ops --filter 07-rotatingcalipers
python3 '96-Local Testing/05-extraction.py' testers --filter 03-Geometry/0
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/07-rotatingcalipers.hpp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

`python3 '96-Local Testing/04-mutation.py' run 03-Geometry/07-rotatingcalipers.hpp`, rerun on the lines `/finish` changed (`farthestPairNaive` and `calipers_detail::bounded`), then the survivors retested with `--workdir` after the type-domain compiles were added:

Record: mewt 4.0.0 on blob `a855804b8a12d1225173f1aabf766b94f82b0725` (2026-11-15, seed 1, 18 lines changed since blob `5ec6796491ab`): 79 mutants, 69 caught (1 by timeout), 10 survived, 0 untested.

Stage seconds: quick 202, full-optimized 63, full-rest 297 (summed over the campaign and its retest; the retest's baselines 5.8 s, 15.9 s and 22.8 s). The first pass left 11 survivors; the `static_assert` with `&&` → `||` on line 203 (LOS 830), which admits floating coordinates, is now caught by the type-domain compiles. The other 10 are equivalent:

- Line 204, `p.size() <= INT_MAX` → `!=` or `<` (COS 1223, 1224): a distinguishing input needs at least `2^31` points, beyond every memory cap; the bound was inspected.
- Lines 207 and 210, the exact `dist2` with its points commuted (AS 333, 334).
- Line 208, `i < n` → `i != n` or `i <= n` (COS 1233, 1234): the index rises by one from 0, and at `i = n` the inner loop runs no iteration. Line 209, `j < n` → `j != n` (COS 1238): `j` starts at `i + 1 <= n`.
- Line 209, `j = i + 1` → `i * 1`, `i / 1` or `i % 1` (AOS 792, 793, 794): the scan also visits pairs with `j <= i`, whose distance is 0 or was already seen as `(j, i)`, so the strict `>` never takes one, and the pairs that do improve the maximum come in the same order; at most `2n^2` distance computations, within the `O(n^2)` bound.

Earlier campaign, on the lines the review fixes changed (the exact polar starts of `expectedBoxAreaApprox`): mewt 4.0.0 on blob `5ec6796491ab0c85e21f206aff1df10f090fc50f` (2026-11-14, seed 1, 10 lines changed since blob `7c24fe4c0a33`): 64 mutants, 62 caught (0 by timeout), 2 survived, 0 untested. Stage seconds: quick 81, full-optimized 3, full-rest 28 (baselines 5.9 s, 6.0 s and 12.1 s). The 2 survivors are equivalent: lines 164 and 170, `s < 2` → `s != 2` (COS 1135, 1145), counting loops whose index rises by one from 0 to the bound.

Earlier campaign, on the lines the first campaign's removals changed: mewt 4.0.0 on blob `7c24fe4c0a3360cd20b632053e6215a14a9e3ddb` (2026-11-14, seed 1, 2 lines changed since blob `9bbacaa54054`): 22 mutants, 16 caught (0 by timeout), 6 survived, 0 untested.

Stage seconds: quick 49, full-optimized 5, full-rest 66 (baselines 8.6 s, 8.7 s and 19.8 s). The 6 survivors are equivalent:

- Line 96, the initial search for `lo`: the commuted `dot` arguments (AS 260, 261) and `j < n` → `j != n` (COS 976) change nothing. `<` → `<=` (COS 982) starts `lo` at the last vertex of the least projection on `e[0]`, which has the same value. `<` → `>=` (COS 984) starts it at the last vertex of the greatest projection, from which the projection on `e[0]` falls strictly, going forward around the strictly convex hull, until the least one; the monotone loop of line 97 therefore carries `lo` to the first vertex of the least projection before it is read.
- Line 128, `cross(q.back().dir, s.dir)` with its arguments commuted (AS 275): the value is only tested against zero.

The differential run described below also compared these three changes of lines 96 and 128 with the current blob: the hashes were bit-identical.

Earlier campaign: mewt 4.0.0 on blob `9bbacaa5405422bf366f4a095a72668d73f88c33` (2026-11-14, seed 1, whole file, uniform sample of 1214): 500 mutants, 452 caught (11 by timeout), 48 survived, 0 untested.

Stage seconds: quick 1046, full-optimized 26, full-rest 411 (baselines 5.5 s, 6.5 s and 11.8 s).

Three survivors exposed redundant code, which was then removed: the initial maximum search of `rectangleApprox` (IF 193, AS 265; from vertex 0 the monotone `hi` pointer climbs to the same first maximum, since `e[0]` points from `p[0]` to `p[1]`) and the opposite-direction clause of the parallelogram merge (COS 1066; two consecutive merged directions of a strictly convex polygon are never opposite, since the centrally symmetric set of `+-e_i` has every angular gap below `pi`). The other 45 survivors are equivalent, by blob line:

- Commuted symmetric arguments: `swap` operands (AS 252, 253, 299), `dot`, `dist2` and `distanceApprox` arguments (AS 257, 262, 266, 268, 274, 275, 300, 301, 311), and the cross-multiplied order of a `ratioLess` comparison with positive operands (AS 259 at 81, AS 284 at 135: `a / b < c / d` and `a / c < b / d` agree for positive values).
- Counting loops whose index rises by one to the bound, `<` → `!=` (COS 852, 862, 967, 1017, 1042, 1052, 1117, 1147, 1152, 1162, 1167), and `j == n` → `j >= n` with `j <= n` (COS 1060).
- After the `n < 2` return, `n == 2` → `n <= 2` (COS 848, 913).
- Line 50, `q < r` → `q <= r` under `q != r` (COS 893). Line 62, `i < j` → `i <= j`: no vertex is antipodal to itself (COS 923).
- Line 39, `>` → `<=` (COS 869): the start of the opposite pointer becomes vertex 1, from which the monotone loop of line 41 climbs to the same first maximum. Lines 97, 100 and 101 (COS 988, 990, 1003, 1010): `lo` and `hi` start on or stop at another vertex of the same extreme projection, or are carried to it by the monotone loops; only the extreme values `left` and `right` are used.
- Line 74, the diameter tie `d == ans.squared` → `<=` (COS 948): `antipodalPairs` emits pairs in increasing lexicographic order, because `i` rises and, for one `i`, the partners `j > i` rise (a partner range that wrapped past `n - 1` would contain `i`, which is never its own partner), so `pair < ans` never holds for a later pair. Line 194, `==` → `>=` in `farthestPair` (COS 1200): the added case `d > ans.squared` is the first disjunct.
- Line 125, the merge loop stopping when either list ends (LOS 799, COS 1049, 1054): the dropped entries are the last of one cyclic list, whose negatives are kept, and `det(-x, -y) = det(x, y)`, so the kept polar vertices reach the same maximum.
- Line 134, `u < 0` → `u <= 0` and `v >= 0` → `v > 0` (COS 1078, 1090): they change only the class of a partner parallel to `q[i]`, which is never the maximizing partner (its determinant is 0, below the positive optimum the assertion of line 140 checks).
- Line 140, `assert(d > 0)` → `d >= 0` (COS 1110): a weaker assertion on a value that is always positive.
- Line 27, `INT_MAX / 2` → `INT_MAX - 2` (AOS 321): a distinguishing input needs at least 2^30 hull points (16 GiB), beyond every memory cap; the bound was inspected.
- Line 159, `return 0` → `0.0` (RDV 1212): the same `long double` zero.

A differential run backs the arguments for lines 74, 125, 130, 134 and 194 and for the removed maximum search: each mutant and the original, built with `-O2 -DNDEBUG`, gave bit-identical diameters, `farthestPair` results, rectangles and parallelograms (edges, areas and vertices) on 60,000 random lattice clouds of 1 to 14 points in boxes of side 4, 8 and 60, with random hull starts.

## Benchmarks

```sh
g++ -std=gnu++20 -O2 -DNDEBUG '96-Local Testing/03-Geometry/07-rotatingcalipers_benchmark.cpp' -o /tmp/p008-calipers-benchmark
/tmp/p008-calipers-benchmark
```

The input is a strictly convex integer parabola (every point a hull vertex). Setup is timed separately, and a warmup precedes seven measurements; the table gives medians. The brute force checks every edge pair with the same exact comparator, and its objective is checked against the polar result. The checksum is `2.00271e+12`. 2026-11-14 (`/finish P008`, experiment none), i9-11900H, GCC 16.2.1, `MEMORY peak=9MB`, beside mutation campaigns of this and other sessions (load average 8.3 on 16 threads), so absolute times run about 1.8 times the unloaded 2026-10-07 medians while the ratios stay within 3% of them except the largest (116.7 against 142.1):

| n | setup µs | polar µs | all edge pairs µs | brute / polar |
|---:|---:|---:|---:|---:|
| 32 | 4.082 | 7.733 | 5.513 | 0.71 |
| 128 | 11.625 | 30.698 | 83.540 | 2.72 |
| 512 | 22.414 | 128.546 | 1,430.06 | 11.12 |
| 2,048 | 78.931 | 632.975 | 22,347.6 | 35.31 |
| 8,192 | 352.874 | 3,089.91 | 360,641 | 116.72 |

At `n = 32` the all-pairs brute force is faster (ratio 0.71), by about 2 µs; the linear algorithm is kept at every size, without a second implementation or threshold.

## Sources

Inspected on 2026-09-27:

- [KACTL HullDiameter.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/HullDiameter.h): the strict hull contract and the `O(n)` antipodal scan. No code was copied.
- CGAL `min_quadrilateral_2` [documentation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/doc/Bounding_volumes/CGAL/min_quadrilateral_2.h) and [implementation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/include/CGAL/min_quadrilateral_2.h) (`min_parallelogram_2`, lines 444–623): the API contracts and linear claims. The polar reduction used here was derived independently. The papers CGAL cites were not read.
- The `farthestPair` sources are listed in [00-sources.md](00-sources.md#p008-completeness-sweep-fetched-2026-10-07); candidates not adopted (two-polygon problems) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07). `OLD/algorithms.cpp` and `OLD/[1] algorithms.cpp` contain no caliper code.

## Limits and handoffs

Other packages own convex polygon queries (GE05), two-polygon distance, separation and farthest pair (row 17, GE25) and hull bridges (row 53); row 17 owns the inner and outer common tangents and the widest separating strip, and row 76 the Grenander distance ([00-notes.md](00-notes.md#p275-re-review-2026-11-10)). `expectedBoxAreaApprox` comes from OI Wiki's calipers exercises (CF 1578F); the 2026-11-14 sweep's other decisions are in [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Aliases

Search aliases (P263, grab --find): 旋转卡壳, 旋转卡尺, 凸包直径, 平面最远点对, 最小矩形覆盖, 最小外接矩形, 凸包宽度, 对踵点.

Search aliases (P008, grab --find): expected bounding box area, random rotation bounding box, Framing Pictures, 随机旋转包围盒期望面积, brute force farthest pair, all pairs diameter, 暴力最远点对.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 626,364 checks) passed on g++ 16; `farthestPair` missing, degenerate witnesses untested.
- 2026-10-07: re-audit, implemented `farthestPair`; 4 findings fixed; full suite, stress and benchmark passed on g++ 16; independent review: reproduced every full count and probe, confirmed one package defect (in `06-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: P008 re-audit (GE02, experiment none): added `expectedBoxAreaApprox` after the completeness sweep; the first mutation campaign removed the redundant initial maximum search and the parallelogram's opposite-direction clause; independent review: exact polar starts for `expectedBoxAreaApprox` (rounded angles of nearly parallel edges could tie) and a shorter comment; two changed-lines campaigns; full on g++ and g++-14, stress, extraction and `02-integration.py --sanitizers` passed.
- 2026-11-15: `/finish P008` (experiment none): added `farthestPairNaive` (user decision 2026-11-14, a Priority exception recorded in `omissions.md`), checked beside `farthestPair` on every cloud, typed `int` and `int8_t` rounds and a bound probe; the coordinate check moved to `calipers_detail::bounded`; review fixes: stale counts and seeds in Commands; the changed-lines campaign's one non-equivalent survivor (a loosened `static_assert`) is caught by six new type-domain compiles; benchmark rerun; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
