# 07-rotatingcalipers.hpp — Evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, with `05-closestpair.hpp`, `06-circle.hpp` and `08-coordinate_transform.hpp`). `degenerateBox` is in the row because the contest struct exposes it. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric Policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. Every support and optimum choice of `ConvexCalipers`, and `farthestPair`, is exact: signed integral coordinates, with every predicate widened to `lll` before arithmetic. Metrics and vertices of the `Approx` results are `long double`.

All selection predicates are exact on `|x|, |y| <= 10^9`:

- Edge components and differences are at most `2 * 10^9` in absolute value.
- Dot and cross products, squared lengths and projection ranges are at most `8 * 10^18`.
- Width and area numerators are at most `6.4 * 10^37`, and the squared half-perimeter numerator at most `2.56 * 10^38`; both fit unsigned 128 bits.

Cross-multiplying two such fractions can exceed 128 bits, so `ratioLess` compares them by continued fractions.

### ConvexDiameter, ConvexWidthApprox, CaliperBoxApprox

- `ConvexDiameter { first, second, squared }` stores an exact squared distance and sorted input indices. Empty input gives `-1, -1, 0`; a singleton gives `0, 0, 0`.
- `ConvexWidthApprox { width, edge, opposite }` stores a rounded width with its supporting edge and opposite vertex. The witnesses are `-1/-1` for an empty hull, `0/0` for a singleton and `0/1` for a segment, all with width `0`.
- `CaliperBoxApprox { exists, vertices, area, perimeter, edges }` stores CCW approximate vertices. `exists` is false only for an empty hull. A singleton box repeats `p[0]` four times. A segment box is `{p0, p1, p1, p0}`, with area `0` and perimeter twice the segment length. With fewer than three hull points `edges` stays `{-1, -1}`, because no hull edge supports the box. Rectangles use `edges[0]` (the flush edge); a parallelogram uses both.

### ConvexCalipers

The constructor copies a strictly convex CCW cyclic hull: no repeated start vertex, no collinear vertices, any starting vertex; use `convexHull(points, false)` for an arbitrary cloud. It accepts signed integral `T` with `|x|, |y| <= 10^9` and `n <= INT_MAX / 2`. Empty, singleton and two-distinct-point hulls are supported. Assertions catch the coordinate bound, a repeated two-point input and nonpositive consecutive turns; simplicity and convexity of the cycle remain preconditions. Construction is `O(n)` with a monotone opposite-support pointer, and no query mutates state. Indices refer to the input hull.

- `ratioLess(a, b, c, d)` compares `a / b < c / d` exactly for `b, d > 0` (asserted) by continued fractions, in `O(1)` words for 128-bit operands.
- `antipodalPairs()` returns each distinct unordered pair `i < j` that admits parallel opposite support lines, including all four endpoint combinations at parallel-edge ties: `O(n)` pairs. A singleton has none; a segment has `{0, 1}`.
- `diameter()` returns the maximum exact squared distance and the lexicographically smallest sorted index pair among ties.
- `minimumWidthApprox()`, `minimumAreaRectangleApprox()`, `minimumPerimeterRectangleApprox()` and `minimumAreaParallelogramApprox()` (with `rectangleApprox(bool perimeter)` as the shared rectangle routine and `degenerateBox()` for fewer than three points) select their optimum exactly; equal optima keep the first support configuration encountered. Metrics and vertices are converted to `long double` only after selection, so they may lose relative accuracy on very thin hulls.
- `expectedBoxAreaApprox()` returns the expected area of the axis-aligned bounding box of the hull rotated by an angle uniform on `[0, 2 pi)`, in rounded `long double` (CF 1578F). Fewer than two points give 0, and a segment of length `L` gives `L^2 / pi`. It takes `O(n)` time, `2n` `atan2` calls and `O(n)` memory, and selects nothing exactly: it is an `Approx` metric only.

Correctness:

- Expected box area: with `h` the support function about `p[0]` and `w(t) = h(t) + h(t + pi)` the width in direction `t`, the box area is `w(t) w(t + pi / 2)`. Expanding the product and shifting the four terms over a full period gives `E = (2 / pi) * I`, with `I` the integral of `h(psi - pi / 2) h(psi)` over `[0, 2 pi)`. Parametrized by an edge-direction angle `psi`, `h(psi - pi / 2)` is attained by the vertex after the last edge whose direction is at most `psi`, and `h(psi)` by the vertex after the last edge whose direction is at most `psi + pi / 2`. Both sequences are the cyclically sorted edge angles, merged in `O(n)`. The second list takes `atan2` of each integer direction rotated exactly by `-pi / 2`, `(y, -x)`, and each list starts at the edge `polarLess` puts first, so a list never wraps inside even when two nearly parallel edges round to the same angle. Within `|x|, |y| <= 10^9` consecutive edge angles differ by at least about `5e-19` against an ulp of at most `4.3e-19`, a margin that rounding in `atan2` could still close; rounding then only reorders events by about an ulp, adding an integral over a span of that size. Between events the integrand is `(u . (sin psi, -cos psi)) (v . (cos psi, sin psi))` for the two fixed vertices `u`, `v`, whose antiderivative is closed form in `sin^2`, `sin * cos` and `psi`. Taking `h` about `p[0]` keeps every term within the hull's diameter. A segment is the 2-gon of its two opposite edges.
- Opposite support indices advance monotonically around a convex polygon. For vertex `i`, every antipodal partner lies between the support of the preceding edge and that of the following edge, including the second endpoint of a tied support edge. Enumerating that interval and keeping `i < j` yields each unordered pair once. Every diameter pair is antipodal.
- The minimum-width strip has a hull edge on one boundary, and a minimum-area rectangle has a hull edge flush with one side. A minimum-perimeter rectangle does too: between support events the half-perimeter is `a cos(theta) + b sin(theta)`, which is concave, so its minimum is at an event. Edge-aligned boxes use three monotone support pointers and exact rational objective comparisons.
- Minimum parallelogram: for `D = P - P` and a normal `a`, let `w(a)` be the unnormalized strip width. The parallelogram with normals `a` and `b` has area `w(a) w(b) / |det(a, b)|`, the reciprocal of `|det(a / w(a), b / w(b))|`, a determinant of two boundary points of the polar of `D`. Its maximum over a compact convex polygon is attained at vertices, which are exactly the distinct rays `+-e_i / w_i` (rotated by a common quarter turn). After merging the cyclic orders of `e_i` and `-e_i` and removing same-ray duplicates, the maximizing partner advances monotonically, so the search is `O(n)` including ties. The final corners intersect the two exact support bands before conversion to `long double`.

### farthestPair

`farthestPair(const vector<Point2<T>> &p)` takes any point cloud with integral `|x|, |y| <= 10^9`, duplicates and collinear points included. It returns the original indices of a farthest pair: the lexicographically smallest sorted pair among all pairs of maximum exact squared distance. For `n >= 2` the indices are distinct, so if all points coincide the result is `{0, 1, 0}`. Empty input gives `{-1, -1, 0}` and a singleton `{0, 0, 0}`. It runs in `O(n * log(n))` time and `O(n)` memory: `convexHullIndices(p, false)` (first-occurrence indices), then the calipers' antipodal pairs mapped back. Every farthest pair joins two strict hull vertices, and each vertex's smallest index is its first occurrence, so the scan finds the lexicographically smallest pair. The header includes `04-convexhull.hpp` for this function.

### farthestPairNaive

`farthestPairNaive(p)` scans every pair `i < j` in increasing order and keeps the first strictly larger squared distance, so it returns exactly `farthestPair`'s result on the same domain: signed integral `|x|, |y| <= 10^9`, asserted for every point through `calipers_detail::bounded`, the check the `ConvexCalipers` constructor uses. It is the lighter all-pairs form (user decision 2026-11-14, Priority): `O(n^2)` time, `O(1)` memory, no hull.

## Feature-to-Test Map

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

## Commands and Results

Package run, 2026-11-14 (P008, experiment none): Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns.

| Command | Result |
|---|---|
| `07-rotatingcalipers_tester.py --mode full --seed 1` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 660,832 checks per configuration; 3,296 Python integer cases; Python box-area oracle on 423 hulls (1,689 calls with cyclic starts, quarter turns and translations); 7 assertion probes; 6 type-domain compiles; `MEMORY peak=378MB` |
| `CXX=g++-14 07-rotatingcalipers_tester.py --mode full --seed 2` (GCC 14.4.1) | PASS, 662,872 checks per configuration; the same Python counts, probes and compiles; `MEMORY peak=392MB` |
| `07-rotatingcalipers_tester.py --mode stress --seed 3` (all three configurations) | PASS, 32,114,132 checks per configuration; 21,296 Python integer cases; box-area oracle on 2,024 hulls (8,093 calls); `MEMORY peak=583MB`; run before the type-domain compiles were added, which change no other check |
| `05-extraction.py ops --filter 07-rotatingcalipers`; `testers --filter 03-Geometry/0` | PASS: 22 operations in 14 extractions on g++ and g++-14; 12 names, quick on the extracted build |
| `04-mutation.py run 03-Geometry/07-rotatingcalipers.hpp` | See Mutation Testing |

Library-wide runs, 2026-11-10 (P274; same machine, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), this suite's lines, all PASS:

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | `MEMORY` peak |
|---|---|---|---|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | 815 MB | 0.2 s compile + 0.5 s run | 16.2 + 0.5 s | 18.2 + 1.0 s | 1005 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | 856 MB | 0.3 + 0.5 s | 28.0 + 0.9 s | 28.5 + 2.3 s | 1049 MB |
| `01-run.py --mode quick` | — | 20.4 + 0.1 s | 23.1 + 0.3 s | — | 811 MB |
| `01-run.py --mode stress --rounds 1` | 822 MB | 0.1 + 2.5 s | 8.4 + 7.9 s | 8.6 + 25.3 s | 611 MB |

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

## Mutation Testing

Record: mewt 4.0.0 on blob `a855804b8a12d1225173f1aabf766b94f82b0725` (2026-11-15, seed 1, 18 lines changed since blob `5ec6796491ab`): 79 mutants, 69 caught (1 by timeout), 10 survived, 0 untested.

Stage times, summed over the campaign and a `--workdir` retest after the type-domain compiles were added: quick 202 s, full optimized 63 s, full on the other builds 297 s. The open survivors below come from this record (C) and from three earlier campaigns of 2026-11-14 whose lines it did not cover: a whole-file sample of 500 of 1,214 mutants (W), a 2-line campaign after the first campaign's removals (R), and a 10-line campaign on the exact polar starts of `expectedBoxAreaApprox` (P). Line numbers are those of the current blob.

| Mutants (record) | Line | Why equivalent |
|---|---|---|
| `AOS 321` (W; `INT_MAX / 2` → `INT_MAX - 2`) | 31 | A distinguishing input needs at least 2^30 hull points (16 GiB), beyond every memory cap; the bound was inspected. |
| `COS 1223`, `COS 1224` (C; `p.size() <= INT_MAX` → `!=`, `<`) | 204 | A distinguishing input needs at least `2^31` points, beyond every memory cap; the bound was inspected. |
| `AS 252`, `AS 253`, `AS 299` (W) | 57, 153 | `swap` operands commuted. |
| `AS 257`, `AS 266`, `AS 268`, `AS 274`, `AS 275`, `AS 300`, `AS 301`, `AS 311` (W); `AS 260`, `AS 261` (R); `AS 333`, `AS 334` (C) | 77, 100, 102, 103, 107, 155, 197, 207, 210 | Commuted arguments of the symmetric `dot`, `dist2` and `distanceApprox`. |
| `AS 259`, `AS 284` (W) | 85, 137 | The cross-multiplied order of a `ratioLess` comparison with positive operands: `a / b < c / d` and `a / c < b / d` agree for positive values. |
| `AS 275` (R) | 132 | `cross(q.back().dir, s.dir)` with its arguments commuted is only tested against zero. |
| `COS 852`, `COS 862`, `COS 967`, `COS 1017`, `COS 1042`, `COS 1052`, `COS 1117`, `COS 1147`, `COS 1152`, `COS 1162`, `COS 1167` (W); `COS 976` (R); `COS 1135`, `COS 1145` (P); `COS 1233`, `COS 1234`, `COS 1238` (C) | 39, 43, 84, 100, 111, 123, 127, 149, 165, 168, 174, 182, 208, 209 | Counting loops whose index rises by one to the bound, `<` → `!=` (or `<=` where the last iteration does nothing: at `i = n` on line 208 the inner loop runs no iteration, and `j` starts at `i + 1 <= n` on line 209). |
| `COS 1060` (W; `j == n` → `j >= n`) | 130 | `j <= n`. |
| `COS 848`, `COS 913` (W; `n == 2` → `n <= 2`) | 38, 61 | They follow the `n < 2` return. |
| `COS 869` (W; `>` → `<=`) | 43 | The opposite pointer starts at vertex 1, from which the monotone loop of line 45 climbs to the same first maximum. |
| `COS 893` (W; `q < r` → `q <= r`) | 54 | Under `q != r`. |
| `COS 923` (W; `i < j` → `i <= j`) | 66 | No vertex is antipodal to itself. |
| `COS 948` (W; diameter tie `d == ans.squared` → `<=`) | 78 | `antipodalPairs` emits pairs in increasing lexicographic order: `i` rises and, for one `i`, the partners `j > i` rise (a partner range that wrapped past `n - 1` would contain `i`, which is never its own partner), so `pair < ans` never holds for a later pair. |
| `COS 982`, `COS 984` (R) | 100 | The initial search for `lo`: `<` → `<=` starts `lo` at the last vertex of the least projection on `e[0]`, which has the same value. `<` → `>=` starts it at the last vertex of the greatest projection, from which the projection on `e[0]` falls strictly, going forward around the strictly convex hull, until the least one; the monotone loop of line 102 therefore carries `lo` to the first vertex of the least projection before it is read. |
| `COS 1003`, `COS 1010` (W) | 102, 103 | `lo` and `hi` stop on another vertex of the same extreme projection or are carried to it by the monotone loops; only the extreme values `left` and `right` are used. |
| `LOS 799`, `COS 1049`, `COS 1054` (W; the merge loop stopping when either list ends) | 127 | The dropped entries are the last of one cyclic list, whose negatives are kept, and `det(-x, -y) = det(x, y)`, so the kept polar vertices reach the same maximum. |
| `COS 1078`, `COS 1090` (W; `u < 0` → `u <= 0`, `v >= 0` → `v > 0`) | 136 | They change only the class of a partner parallel to `q[i]`, which is never the maximizing partner: its determinant is 0, below the positive optimum the assertion of line 142 checks. |
| `COS 1110` (W; `assert(d > 0)` → `d >= 0`) | 142 | A weaker assertion on a value that is always positive. |
| `RDV 1212` (W; `return 0` → `0.0`) | 160 | The same `long double` zero. |
| `COS 1200` (W; `==` → `>=` in `farthestPair`) | 198 | The added case `d > ans.squared` is the first disjunct. |
| `AOS 792`, `AOS 793`, `AOS 794` (C; `j = i + 1` → `i * 1`, `i / 1`, `i % 1`) | 209 | The scan also visits pairs with `j <= i`, whose distance is 0 or was already seen as `(j, i)`, so the strict `>` never takes one, and the improving pairs come in the same order; at most `2n^2` distance computations, within the `O(n^2)` bound. |

A differential run backs the arguments for lines 78, 127, 136 and 198: each mutant and the original, built with `-O2 -DNDEBUG`, gave bit-identical diameters, `farthestPair` results, rectangles and parallelograms (edges, areas and vertices) on 60,000 random lattice clouds of 1 to 14 points in boxes of side 4, 8 and 60, with random hull starts. The same run showed bit-identical results after the redundant code on lines 100 and 132 was removed.

## Benchmarks

```sh
g++ -std=gnu++20 -O2 -DNDEBUG '96-Local Testing/03-Geometry/07-rotatingcalipers_benchmark.cpp' -o '<scratchpad>/calipers-benchmark'
'<scratchpad>/calipers-benchmark'
```

The input is a strictly convex integer parabola, every point a hull vertex. Setup is timed separately, and a warmup precedes seven measurements; the table gives medians. The brute force checks every edge pair with the same exact comparator, and its objective is checked against the polar result. Checksum `2.00271e+12`. Latest run 2026-11-14 (`/finish P008`, experiment none), i9-11900H, GCC 16.2.1, `MEMORY peak=9MB`. Mutation campaigns of this and other sessions ran beside it (load average 8.3 on 16 threads), so absolute times are about 1.8 times the unloaded 2026-10-07 medians; the ratios stayed within 3% of those except the largest.

| n | setup µs | polar µs | all edge pairs µs | brute / polar |
|---:|---:|---:|---:|---:|
| 32 | 4.082 | 7.733 | 5.513 | 0.71 |
| 128 | 11.625 | 30.698 | 83.540 | 2.72 |
| 512 | 22.414 | 128.546 | 1,430.06 | 11.12 |
| 2,048 | 78.931 | 632.975 | 22,347.6 | 35.31 |
| 8,192 | 352.874 | 3,089.91 | 360,641 | 116.72 |

At `n = 32` the all-pairs brute force is faster by about 2 µs; the linear algorithm is kept at every size, without a second implementation or threshold.

## Sources

Inspected on 2026-09-27:

- [KACTL HullDiameter.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/HullDiameter.h): the strict hull contract and the `O(n)` antipodal scan. No code was copied.
- CGAL `min_quadrilateral_2` [documentation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/doc/Bounding_volumes/CGAL/min_quadrilateral_2.h) and [implementation](https://github.com/CGAL/cgal/blob/master/Bounding_volumes/include/CGAL/min_quadrilateral_2.h) (`min_parallelogram_2`, lines 444–623): the API contracts and linear claims. The polar reduction used here was derived independently.
- Schwarz, Teich, Vainshtein, Welzl and Evans, SoCG 1995, read on 2026-11-22 ([ledger](00-sources.md#fetched-2026-11-22-source-check)): the minimum parallelogram takes `O(n)`, and some optimal parallelogram has one side of each opposite pair flush with a polygon edge. The polar reduction yields the same fact, since its optimal normals are hull edge normals. The two-page abstract gives no proofs and scans a different characterization, so the argument above cannot be checked against it. The other papers CGAL cites were not read.
- The `farthestPair` sources are in [00-sources.md](00-sources.md#p008-completeness-sweep-fetched-2026-10-07) and the sweep's decisions in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07). `OLD/algorithms.cpp` and `OLD/[1] algorithms.cpp` contain no caliper code.

## Limits and Handoffs

Other packages own convex polygon queries (GE05), two-polygon distance, separation and farthest pair (row 17, GE25) and hull bridges (row 53); row 17 owns the inner and outer common tangents and the widest separating strip, and row 76 the Grenander distance ([00-notes.md](00-notes.md#p275-re-review-2026-11-10)). `expectedBoxAreaApprox` comes from OI Wiki's calipers exercises (CF 1578F); the 2026-11-14 sweep's other decisions are in [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Aliases

Search aliases (P263, grab --find): 旋转卡壳, 旋转卡尺, 凸包直径, 平面最远点对, 最小矩形覆盖, 最小外接矩形, 凸包宽度, 对踵点.

Search aliases (P008, grab --find): expected bounding box area, random rotation bounding box, Framing Pictures, 随机旋转包围盒期望面积, brute force farthest pair, all pairs diameter, 暴力最远点对.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 626,364 checks) passed on g++ 16; `farthestPair` missing, degenerate witnesses untested.
- 2026-10-07: re-audit, implemented `farthestPair`; 4 findings fixed; full suite, stress and benchmark passed on g++ 16; independent review reproduced every full count and probe and confirmed one package defect (in `06-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (OI Wiki page titles and common names, Simplified and Taiwan or Hong Kong terms); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed.
- 2026-11-14: P008 re-audit (GE02, experiment none): added `expectedBoxAreaApprox` after the completeness sweep; the first mutation campaign removed the redundant initial maximum search of `rectangleApprox` and the parallelogram merge's opposite-direction clause; independent review: exact polar starts for `expectedBoxAreaApprox` (rounded angles of nearly parallel edges could tie) and a shorter comment; two changed-lines campaigns; full on g++ and g++-14, stress, extraction and `02-integration.py --sanitizers` passed.
- 2026-11-15: `/finish P008` (experiment none): added `farthestPairNaive` (user decision 2026-11-14, a Priority exception recorded in `omissions.md`), checked beside `farthestPair` on every cloud, with `int` and `int8_t` rounds and a bound probe; the coordinate check moved to `calipers_detail::bounded`; review fixes to stale counts and seeds in Commands; the changed-lines campaign's one non-equivalent survivor (a loosened `static_assert`, `LOS 830`) is caught by six new type-domain compiles; benchmark rerun; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
