# 02-line_segment.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#ge01-numeric-policy-p007) for the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### RationalPoint2

Normalized homogeneous coordinates `(x/d, y/d)` with `d > 0` and `gcd(|x|, |y|, d) = 1`; equality compares this canonical form. The constructor asserts `d != 0` and that no component equals the minimum `lll` value. Code that assigns the public fields must preserve the invariant. It is a construction result, not a rational arithmetic package. `approx()` converts each component to long double independently. Rational normalization takes `O(log(C))` Euclidean steps.

### LinearKind, Linear2, IntersectionKind, LinearIntersection2

`Linear2` is a closed segment, a ray from `a` through `b`, or a full line. All endpoints are closed. For every kind, `a == b` is a singleton point. Integral endpoints must satisfy `|coordinate| <= 10^9`, and every linear operation asserts this and that the kind is valid. In `LinearIntersection2`, only the field matching `kind` has meaning: `p` for `Point`, `overlap` for `Segment`, `Ray` and `Line`. A segment overlap stores its endpoints in lexicographic order. An overlap reuses the original integral endpoints, so it is itself a valid `Linear2`.

### contains, parallel, orthogonal, collinear

Exact. Membership covers singleton degeneracies. A zero direction is both parallel and orthogonal to every direction. `collinear` asks whether all endpoints lie on one common line, so any two singletons are collinear.

### intersect

Handles all nine segment/ray/line kind pairs and reports empty, point, segment, ray or line. A nonparallel crossing is an exact `RationalPoint2`. Predicates use degree-2 products and point numerators are degree 3. No rational is ever multiplied by another.

Correctness: for nonparallel linear objects, Cramer's rule gives two parameters over the same determinant. Making the denominator positive reduces segment and ray membership to exact integer comparisons. For collinear non-singleton objects, lexicographic order is monotone along their shared line, so intersecting their finite or infinite endpoint bounds gives the exact overlap kind. Singleton membership is handled before any division.

### canonicalLine

For `s.a != s.b` (asserted), returns `{A, B, K}` with `A x + B y = K` describing the supporting line, ignoring the kind. `(A, B) = canonicalDirection(perp(b - a))`, so `gcd(A, B) = 1` and `B > 0`, or `B == 0` and `A > 0`. Then `K = A a.x + B a.y`, with `|K| <= 4 * 10^18`. Two linear objects have equal keys exactly when their supporting lines coincide. The key is suitable as a map key for counting distinct lines.

Correctness: `canonicalDirection` applied to the normal `perp(b - a)` yields one primitive representative per pair of opposite normals. `K` is then determined by any point of the line, so equal keys mean equal normals up to sign and equal offsets, which is the same supporting line.

### lineFromEquation

`lineFromEquation(a, b, c, res)` (user decision 2026-11-12) builds a `Linear2` line for `a x + b y = c`. It asserts `(a, b) != (0, 0)`; every other `lng` triple is valid, `LLONG_MIN` included. A `Linear2` needs two distinct lattice points with `|coordinate| <= 10^9`, so it returns false when the line holds fewer than two of them, including when `gcd(a, b)` does not divide `c`; `res` is then unspecified. Otherwise `res = Linear2::line(P, Q)` with `P < Q` the two lexicographically smallest such points; `Q - P` is the primitive direction, with positive x or zero x and positive y. `canonicalLine(res)` is `(a, b, c) / gcd(a, b)` with its sign fixed, so the two operations are inverse on lines that fit.

Correctness: `extendedGcd(a, b)` (Mathematics row 01) gives `g = gcd(a, b) > 0` and `a x + b y = g`; with `k = c / g` the points `(k x + t q, k y - t p)`, `p = a / g`, `q = b / g`, are all lattice points of the line, and `(q, -p)` is primitive, sign-fixed lexicographically positive, so they increase lexicographically with `t`. Each coordinate bound gives a range of `t` by exact `ceilDiv` and `floorDiv` (a zero step keeps or rejects its fixed coordinate), and the two smallest are `t = lo` and `lo + 1` when `lo < hi`. Magnitudes: `|k|, |x|, |y| <= 2^63`, so `k x` fits `lll`, and every product `t q` evaluated lies within `10^9` of `-k x`. Cost: `O(log(max(|a|, |b|)))` time, `O(1)` storage.

### latticeOnSegment, gridCellsCrossed

The endpoints are lattice points with `|coordinate| <= 10^9`. `latticeOnSegment` counts the lattice points on the closed segment, `gcd(|dx|, |dy|) + 1`; a singleton gives 1. `gridCellsCrossed` counts the unit cells whose open interior meets the segment, `|dx| + |dy| - gcd(|dx|, |dy|)`. An axis-parallel or singleton segment crosses no cell interior and gives 0. Both return `lng`.

Correctness: the lattice points on a segment with primitive step `(dx, dy) / g` are its `g + 1` multiples. The segment crosses `|dx|` vertical and `|dy|` horizontal grid lines in its interior. Each crossing enters a new cell, except at the `g - 1` interior lattice points, where a vertical and a horizontal crossing coincide. Hence `|dx| + |dy| - g` cells.

### projectLineApprox, reflectLineApprox, signedLineDistanceApprox

The line endpoints must differ and the direction's squared norm must be finite and nonzero; this also excludes squared underflow. The signed distance is positive to the left of the directed line `a -> b`.

Rounding is pinned (user decision 2026-11-12): the projection is `a + u * ((p - a) · u / |u|^2)` measured from `a`, and `reflectLineApprox(p, a, b)` is `projectLineApprox(p, a, b) * 2 - p` in that order. Taken from `b` instead, the same value can round differently (the tester's `reflection rounds through the projection from a` regression).

### reflectDirectionApprox

Reflects the direction vector `v` across the direction of line `ab` (`a != b`): `2 (v · u / |u|^2) u - v` with `u = b - a`. This is the outgoing direction of a ray that hits a mirror along `ab`. `v` follows the same coordinate domain as points. The identity is `r = 2 proj_u(v) - v`.

### lineDistanceApprox

Returns the distance between line `ab` (`a != b`) and line `cd`. It is 0 when the lines are not parallel. Otherwise it is `|signed distance of c from ab|`. `c == d` is allowed and gives the distance from point `c` to line `ab`. For integral inputs the parallel test is exact, because long-double differences and products of `|coordinate| <= 10^9` integers are exact.

### perpendicularBisectorApprox

For `a != b`, returns `{m, m + perp(b - a)}` with `m = (a + b) / 2`. Points closer to `a` lie to the left.

### angleBisectorApprox

For `a != o` and `b != o`, returns `{o, o + w * max(|oa|, |ob|)}`, where `w` is along the internal bisector of the angle `aob` and `|w|` lies in `[sqrt(2), 2]`. For angles of at most a right angle, `w` is the sum of the two unit arms. For obtuse angles it is the perpendicular of their difference, which avoids cancellation near a straight angle. At an exactly straight angle (exact for integral inputs), it is the counterclockwise normal of `oa`. The second point lies at least the longer arm's length away. The direction recovered from the two points therefore has relative error about `ulp(coordinate) / arm length`.

Correctness: `|u| = |v| = 1` makes `u + v` orthogonal to `u - v`. The vector `u + v` has length at least `sqrt(2)` when `u · v >= 0`, and `u - v` has length at least `sqrt(2)` when `u · v < 0`. Each branch therefore takes the well-conditioned vector. The sign of `perp(±(u - v))` follows `cross(u - v, u + v) = 2 cross(u, v)`.

### pointSegmentDistanceApprox, segmentDistanceApprox

Singleton segments are allowed. An interior distance uses the perpendicular height, which avoids subtracting nearly equal projected coordinates. Rounding is pinned (user decision 2026-11-12): with `n = (p - a) · (b - a)` and `d = |b - a|^2`, the result is `|p - a|` for `n <= 0`, `|p - b|` for `n >= d` and `|cross(b - a, p - a)| / sqrt(d)` strictly between; at `n == d` the height can round differently from `|p - b|` (the tester's `distance at the far endpoint` regression). For integral inputs every quantity before the square root is exact, so swapping a segment's endpoints gives identical bits. `segmentDistanceApprox` takes integral endpoints and returns 0 on a proper crossing (exact `orient` signs), else the least of the four endpoint-to-segment distances. Every other contact puts an endpoint on the other segment, and there that distance is exactly 0: with `|coordinate| <= 10^9` the differences, dot product, squared length and cross product are integers below `2^64` and exact in long double. P007 removed the former explicit touching tests for that reason (mutation survivors, 2026-11-12).

Correctness: a disjoint segment pair attains its minimum distance at an endpoint projected onto the other segment, while an intersecting pair has distance zero. Fixed-size operations take constant time and storage.

### ccw

`ccw(a, b, c)` takes exact integral points with `|coordinate| <= 10^9` and asserts `a != b`. It returns ei1333's five codes, which are AOJ CGL_1_C's five answers:

| Code | Meaning |
|---|---|
| `+1` | `c` strictly counterclockwise of `a → b` |
| `-1` | `c` strictly clockwise of `a → b` |
| `+2` | `c` on the line strictly behind `a` |
| `-2` | `c` on the line strictly beyond `b` |
| `0` | `c` on the closed segment, endpoints included |

It is a composition: `orient`, then `contains` on `Linear2::segment(a, b)`, then the sign of `(c - a) · (b - a)` to separate behind from beyond.

### lineIntersectionApprox

`lineIntersectionApprox(a, b, c, d)` intersects the lines `ab` and `cd`. It accepts any `Point2<T>`, floating `T` included, which makes it wider than `intersect`, whose domain is integral. Integral inputs need `|coordinate| <= 10^9` (asserted); floating inputs must be finite. After the conversion to long double it asserts that `|u|^2` and `|v|^2` are finite and positive and `|c - a|^2` is finite (probes `line-intersection-underflow`, `-overflow`, `-far`), as the other floating constructions do: a squared length that underflows to 0 or overflows makes the rounded crossing meaningless. It returns:

- `{Point, x}` with the crossing `x = a + u * (((c - a) × v) / (u × v))`, where `u = b - a` and `v = d - c`;
- `{Empty, {}}` for distinct parallel lines;
- `{Line, {}}` for coincident lines.

The arithmetic is rounded long double with no tolerance. The parallel test is the rounded `u × v == 0`, so nearly parallel lines give a far, ill-conditioned crossing. KACTL `lineInter` returns `1`, `0` and `-1` for these three cases.

### linearDistance2, linearDistanceApprox

`linearDistance2<Q = Rational<lll>>(s, t)` is the exact squared Euclidean distance between two closed `Linear2` sets, for all nine kind pairs. A singleton (`a == b`, any kind) is a point, so this one operation covers every point, segment, ray and line pair, which is CGAL `squared_distance`. The result is 0 exactly when `intersect(s, t)` is not `Empty`. The domain is that of `Linear2` (`|coordinate| <= 10^9`, asserted by `intersect`). `Q` is Core `Rational<lll>` (`01-Core/09-rational.hpp`); another type needs construction from `lll` and `(lll, lll)` and `<`, and `linearDistanceApprox` also `toLongDouble()`. `linearDistanceApprox(s, t)` is its square root after that one long double conversion.

Correctness:
- For two disjoint closed convex sets, a nearest pair whose points both lie in the relative interiors forces the connecting segment to be perpendicular to both directions. The objects are then parallel, and the distance is constant along the overlap, so an endpoint also attains it. Hence the minimum over the endpoints of each object (the ray origin; both segment ends; the point of a singleton) against the other object is exact, and two disjoint lines are parallel, where `s.a` against `t` attains it.
- For a point `p` against `z = (a, b)`, the parameter `n = (p - a) · (b - a)` against `d = |b - a|^2` picks the nearest endpoint when it clamps. Otherwise the squared height is `((b - a) × (p - a))^2 / d`.
- Magnitudes: the cross product is at most `|b - a| * |p - a| <= 8 * 10^18`, so its square, at most `6.4 * 10^37`, fits `lll`, and `d <= 1.6 * 10^19`.
- `Rational` normalizes by the gcd and compares exactly with wide products.
- Cost: `intersect` is `O(log(C))` and the candidates are `O(1)` rationals, each normalized in `O(log(C))`.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `onSegment(p, a, b)` | `contains(Linear2::segment(a, b), p)` |

## Feature-to-test map

`96-Local Testing/03-Geometry/02-line_segment_tester.py` runs the geometry runner's optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds ([`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>)). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

| Operation | Test | Oracle |
|---|---|---|
| `RationalPoint2`: normalization, equality, `approx` | `02-line_segment_tester` | Python Fraction |
| `Linear2` factories, `LinearKind`, `contains` | Membership including singleton degeneracies; every fixture also checks `contains(Linear2::segment(s.a, s.b), p)` whatever the kind of `s` | Python Fraction topology via implicit line equations |
| `parallel`, `orthogonal`, `collinear` | `02-line_segment_tester` | Python Fraction topology; an orthogonality bit from exact dot products |
| `intersect`, `IntersectionKind`, `LinearIntersection2` | Every line/segment/ray intersection pair with all five output kinds, reversed operands, and singleton degeneracies (54,256 Fraction topology fixtures) | Python Fraction topology via implicit line equations |
| `canonicalLine` | Key equality against same-line bits, normalization, endpoint incidence and extreme constants | Python Fraction same-line bits |
| `lineFromEquation` | `equationFixture()` (tag `E`): all 12^3 triples of the extremes `LLONG_MIN`, `LLONG_MAX`, `±2^62`, `±(10^9 + 1)`, `±10^9`, `±1`, 0; box-edge lines with exactly 0, 1 or 2 points (steps `2 * 10^9`, `2 * 10^9 ± 1`, corners), scaled up to the `lng` limit; seeded small, large-gcd, non-divisible, axis and huge triples; round trip through `canonicalLine` and `contains` | Python modular-inverse solution with exact Fraction ceil/floor parameter bounds, itself checked against brute enumeration of a 7 x 7 box and five hand-worked cases |
| `projectLineApprox`, `reflectLineApprox`, `signedLineDistanceApprox`, `pointSegmentDistanceApprox`, `segmentDistanceApprox` | Projection, reflection, signed and unsigned distances (3,902 metric fixtures) | 80-digit Decimal references |
| `lineDistanceApprox` | Parallel, antiparallel, crossing and point-line inputs | 80-digit Decimal references |
| `perpendicularBisectorApprox`, `angleBisectorApprox`, `reflectDirectionApprox` | Both bisectors including near-straight and exactly straight angles; ray reflection (6,044 construction fixtures) | 80-digit Decimal references, including a bisector reference built from the plain unit sum |
| `ccw` | All grid triples, random 10^9 points, collinear points at 10^9 scale, extreme corners; antisymmetry `ccw(b, a, c) == -ccw(a, b, c)` | Line-equation side and an exact rational parameter along one axis |
| `lineIntersectionApprox` | Integral pairs as `lng`, `int`, `double`, `long double`; each pair swapped and reversed; exactly representable floating fixtures (general, parallel, coincident) and random well-separated lines | Python `Fraction` solution of the two line equations; exact kind, scale-aware point tolerance |
| `linearDistance2` | All grid pairs of every kind (singletons, touching, crossing, same line, parallel, rays toward and away), extreme 10^9 pairs, near-parallel pairs, a fixed 4·10^18 cross product squared before reduction; reduced fraction with positive denominator, zero exactly when `intersect` is not `Empty`, symmetry and reversed endpoints | Exact Python minimization of the quadratic over each parameter box or half-line, face by face, with its own contact oracle and no call to `intersect` |
| `linearDistanceApprox` | The `linearDistance2` cases | 80-digit square root, relative tolerance 4 ulp |
| `latticeOnSegment`, `gridCellsCrossed` | All pairs of a 5×5 grid, random `[-12, 12]` pairs, and the `±10^9` diagonal and coprime extremes (681 lattice/cell fixtures) | Lattice-point enumeration and exact Fraction cell-interior tests that do not use gcd |
| Preconditions | 39 assertion probes (`line-equation-zero` for `(a, b) == (0, 0)`) | Expected assertion failure in a subprocess |

Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 713 MB; optimized 0.2 s compile + 2.1 s run, checked 13.8 s compile + 1.6 s run, ASan-UBSan 16.6 s compile + 16.6 s run; MEMORY peak 845 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 747 MB; optimized 0.2 s compile + 1.5 s run, checked 14.3 s compile + 1.7 s run, ASan-UBSan 16.3 s compile + 7.4 s run; MEMORY peak 905 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 14.9 s compile + 0.3 s run, checked 15.5 s compile + 1.1 s run; MEMORY peak 704 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 712 MB; optimized 0.1 s compile + 1.1 s run, checked 5.1 s compile + 1.3 s run, ASan-UBSan 6.1 s compile + 9.0 s run; MEMORY peak 539 MB. Dated rows below stay those commands' latest results.

2026-11-12 (/finish P007, experiment: none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14, GNU++20, seed 42, optimized, checked and ASan-UBSan configurations; the 18 `03-Geometry` suites ran in sequence on the final headers and testers, with the mutation campaigns finished and the benchmark not running.

| Command | Result |
|---|---|
| `01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` (GCC 16.2.1) | PASS, 2,426,394 checks per configuration, 39 assertion probes; pre-flight quick peak 51 MB; `MEMORY peak=588MB` |
| `CXX=g++-14 01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` | PASS, 2,426,394 checks per configuration, 39 probes; pre-flight quick peak 50 MB; `MEMORY peak=585MB` |
| `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 5,590,814 checks per configuration; `MEMORY peak=650MB` |
| `05-extraction.py ops --filter 03-Geometry/02-line_segment.hpp` and `05-extraction.py testers --filter '96-Local Testing/03-Geometry/02-line_segment_tester.py'` | PASS, 33 operations on g++ and g++-14 |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3241MB` |

```bash
python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/02-line_segment.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter '96-Local Testing/03-Geometry/02-line_segment_tester.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

`segmentDistanceApprox` against `linearDistanceApprox` on random segment pairs, from the shared GE01 driver: [04-convexhull.md](04-convexhull.md#benchmarks) (2026-11-12: 16.2 to 18.5 times faster).

## Mutation testing

Record: mewt 4.0.0 on blob `1998a54ac1b3ae75b170f2e8666502ecd3bc45ac` (2026-11-12, seed 1, 20 lines changed since blob `fd48f8c4c560`): 174 mutants, 161 caught (0 by timeout), 13 survived, 0 untested.

Stage seconds of that campaign (`lineFromEquation`, the `other` rename, the Mathematics include; /finish P007): quick 330, full-optimized 58, full-rest 146. Its 13 survivors are all equivalent:
- Lines 122–123 (`linearDistance2`'s candidate loop, touched only by the rename): `IT 321`, `IT 322`, `COS 1088`, `COS 1089`, `COS 1099`, `COS 1101` add candidates. For a line or ray, `z.a` and `z.b` lie on `z`, so their distance to `other` is at least the true distance and cannot lower the minimum; `<` and `<=` against `Line`, or `<=` and `>=` against `Segment`, are the same tests or always true by the `LinearKind` order. `AS 371` and `AS 373` swap symmetric `min` arguments.
- Line 138: `COS 1114` (`q == 0` → `q <= 0`, already covered by `q < 0`) and `COS 1121` (`p > 0` → `p >= 0`; `p == q == 0` is excluded by the assertion).
- Line 140: `COS 1124` (`s < 0` → `s <= 0`): negating a zero step and its fixed coordinate leaves the symmetric box test unchanged.
- Line 141: `AS 377`, `AS 379` swap symmetric `max` and `min` arguments.

The intermediate campaign on blob `fd48f8c4c560` (2 lines changed since `2b57d7675618`; the simplified `gcd` and crossing test): 36 mutants, 31 caught, 5 survived; stage seconds quick 58, full-optimized 29, full-rest 62. Its survivors, on that blob's lines 13 and 192: `AS 297`, `AS 298` (symmetric `gcd`); `COS 1088`, `COS 1093` (one `orient` product `< 0` → `<= 0`: a zero product with the other negative puts the crossing at that endpoint, on the other segment, where the distances below are 0 anyway); `RDV 1120` (`0` → `0.0`).

Earlier campaign: mewt 4.0.0 on blob `2b57d7675618af8d5f88014933467e93a59e83f1` (2026-11-12, seed 1, whole file, uniform sample of 1153): 500 mutants, 445 caught (5 by timeout), 55 survived, 0 untested.

P007's first campaign (stages quick 627 s, full-optimized 241 s, full-rest 627 s) left 61 survivors. Probes `invalid-kind-negative`, `parallel-large`, `orthogonal-large`, `collinear-large` and `ccw-large-turn` were added and caught `COS 806`, `COS 816`, `CR 143`, `CR 146`, `CR 150` and `CR 154` on the retest of the same workdir (cumulative 704, 380, 1059 s). The redundant absolute values before `gcd` and the touching tests of `segmentDistanceApprox` were then removed (follow-up on the changed lines below). Line numbers are those of the recorded blob.

| Mutants | Line | Why they survive |
|---|---|---|
| `COS 762`, `COS 767` (`!= min` → `> min`) | 11 | Equal for every `lll`. |
| `AS 301`, `AS 302`, `COS 774`, `COS 778`, `COS 781`, `COS 782` | 13 | `std::gcd` takes absolute values itself, so the hand-written ones were dead; removed. |
| `COS 823`, `COS 913`, `COS 916`, `COS 928`, `COS 946`, `COS 948`, `COS 977`, `COS 1001`, `COS 1016` | 46, 92–97, 110, 115, 122 | `==` → `<=` or `>=` on `LinearKind` or `IntersectionKind`: `check` admits only the declared kinds, `Segment` and `Empty` are the least and `Line` the largest; in the endpoint branches a ray has `z.a < z.b` (or `>`), so `min` or `max` picks `z.a` as before. |
| `AS 303`, `AS 304`, `AS 311`, `AS 313`, `AS 314`, `AS 315`, `AS 352`, `AS 353` | 59, 69, 70, 159, 161 | Swaps that negate `orient`, a signed distance or `cross`, used only as zero tests or under `abs`. |
| `AS 307`, `AS 320`, `AS 330`, `AS 334`, `AS 336`, `AS 341`, `AS 343`, `AS 346`, `AS 349`, `AS 356`, `AS 370`, `AS 371` | 60, 76, 94, 113, 116, 122, 124, 146, 152, 172, 193 | Symmetric `dot`, `min`, `max`, a squared `cross`, the symmetric distance `linearDistance2(t, s)`, and segments built from swapped endpoints (the same set). |
| `COS 891` (`< 0` → `<= 0` in `ccw`) | 76 | `dot == 0` for a collinear `c` means `c == a`, which `contains` already returned. |
| `COS 986`, `COS 998` (`n < 0` → `<=`, `n > d` → `>=` in `linearDistance2`) | 114, 115 | At `n == 0` (or `n == d`) the height `c^2 / d` equals the endpoint's squared distance exactly, the same `Rational`. |
| `COS 1030`, `COS 1035`, `COS 1090` (`d > 0` → `d != 0`) | 151, 156, 185 | `d` is a squared norm, and `isfinite` rules out NaN. |
| `LOS 737`, `COS 1050`, `COS 1053` (`angleBisectorApprox`'s nonzero assert weakened) | 168 | A zero norm reaches `x / s`, and `operator/=` asserts `k != 0` on the same inputs (probe `angle-bisector-singleton`). |
| `COS 1116`, `NR 1134`, `NR 1136`, `RDV 1152` | 194, 195 | The touching tests were redundant (Contracts, segmentDistanceApprox), and a crossing at an endpoint already gives distance 0; removed. |
| `AS 377`, `AS 379`, `AS 381` (`pointSegmentDistanceApprox` on a segment with swapped endpoints) | 196, 197 | Identical bits for integral inputs: every quantity before the square root is exact, and the swap only exchanges which branch measures which endpoint (Contracts, pointSegmentDistanceApprox; a 20,000,000-input scratch search found no difference). |
| `AS 348` (`projectLineApprox(p, b, a)` in `reflectLineApprox`), `COS 1113` (`n >= d` → `>`) | 148, 187 | Rounding only, so the user pinned the rounding (2026-11-12, Contracts). Regressions on inputs found by a scratch search were added; the mutants, applied to the final header in a scratch tree, fail them (`reflection rounds through the projection from a`, `distance at the far endpoint is the endpoint distance`). This blob's workdir cannot be retested after the header changed. |

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). The P007 re-audit sources are in [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11) and its decisions in [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (line and segment). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/intersection.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their segment intersection and line intersection, with explicit widening and degeneracy contracts. The historical name `isIntscLinSeg` is not a compatibility API. Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`. Circle routines from the old `intersection.cpp` are covered by `06-circle.hpp` ([evidence](06-circle.md)). All legacy bytes remain untouched.

## Limits and handoffs

Other planned packages own adaptive and exact floating predicates (P128 / GE11) and segment intersection sweeps (P129 / GE04). General rational-lattice counts are row 19 (`19-lattice_geometry.hpp`).

`lineFromEquation` calls `extendedGcd`, `floorDiv` and `ceilDiv` of `05-Mathematics/01-mod_arithmetic.hpp` (batch MA01 of P012, row `partial` on 2026-11-12, these three listed and implemented): the header's first include outside Core and Geometry, recorded as the batch prerequisite GE01 -> MA01. Its own fixtures check the results against an independent big-integer oracle, so a change there that breaks them fails this suite.

## Aliases

Search aliases (P263, grab --find): 二维计算几何基础, 线段相交, 直线交点, 点到直线距离, 点到线段距离, 线段间距离, 点到直线投影, 中垂线, 线段上整点.
Aliases (P007): ccw, counterclockwise test, exact segment distance, squared segment distance, line-line intersection, line from equation, line from coefficients, ax + by = c, 顺逆时针判断, 两直线交点, 直线一般式.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 1,152,578 checks) passed on g++ 16.
- 2026-10-07: re-audit, implemented the seven missing line operations plus `canonicalLine`; 5 findings fixed (complexity comments, explicit `approx` casts, `LinearIntersection2` comment, braces, comment cap); full suite and stress passed on g++ 16; two-translation-unit GE01 smoke and a `-Wall -Wextra -Wconversion` sweep over all eight `T` passed, `02-integration.py --sanitizers` passed; independent review: confirmed three package defects, all fixed (none in this header).
- 2026-10-12: prune, removed `onSegment` (cases moved to `contains` on `Linear2::segment`; call sites in `segmentDistanceApprox`, `17-polygon_distance.hpp` and the 10 and 16 testers migrated); full suite passed on g++ and g++-14, stress passed.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed. Header edit for `-Werror` without demotions: `[[maybe_unused]]` on names only an `assert` uses; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
- 2026-11-12: P007 re-audit (GE01, started 2026-11-11): added `ccw` (its first form checked coordinates only on the collinear path; it now checks all three points), `lineIntersectionApprox`, `linearDistance2` (Core `Rational<lll>` as `Q`, user decision 2026-11-11) and `linearDistanceApprox`; the header includes `01-Core/09-rational.hpp`; independent review: 4 findings here fixed (`lineIntersectionApprox` finiteness assert with 3 probes, `linearDistanceApprox` forwarding `Q`, `segmentDistanceApprox` recorded as kept in `duplicates.md`, the probe count); first mutation campaign, whose survivors added 5 probes and 2 pinned-rounding regressions (user decision 2026-11-12) and removed the dead absolute values before `gcd` and the touching tests of `segmentDistanceApprox`; full, `CXX=g++-14` full, stress, extraction and `02-integration.py` passed.
- 2026-11-12: /finish P007: added `lineFromEquation` by user decision (include of `05-Mathematics/01-mod_arithmetic.hpp`, batch prerequisite GE01 -> MA01); the `Q` contract names `toLongDouble`; `w` renamed; the `segmentDistanceApprox` keep measured (16.2 to 18.5 times faster); changed-lines mutation campaign; full suite passed on g++ and g++-14, stress passed.
