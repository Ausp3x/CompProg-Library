# 14-minimumenclosingcircle.hpp — evidence

Owned by package P037 / GE05, together with [13-segment_union.md](13-segment_union.md) and [15-convex_polygon_query.md](15-convex_polygon_query.md). Every operation in the row has a test.

## Contracts

### Numeric policy

`minimumEnclosingCircle` takes integral points with `|coordinate| <= 10^9`, asserted. Every decision is an exact integer predicate:

- the diametral test `dot(a − q, b − q)`, below `8 * 10^18`;
- the incircle determinant `Σ |a'|^2 * cross(b', c')` with `x' = x − q`. Each of its three products is below `8 * 10^18 * 8 * 10^18 = 6.4 * 10^37`, so the sum of two fits `lll`. The sign of the full sum is the comparison of that partial sum with the negated third product.

The exact centre has numerators below `5 * 10^28` and denominator `2 * cross(b − a, c − a)`, below `1.6 * 10^19`.

The exact squared radius of three support points is `|u|^2 |v|^2 |u − v|^2 / (4 cross(u, v)^2)` (circumradius `abc / (4K)`). Its numerator reaches `(8 * 10^18)^3 = 5.12 * 10^56 < 2^189` and its denominator `2.56 * 10^38 < 2^128`, beyond `lll`. `radius2` therefore returns Core's `Rational<InfInt>`: one exact value that reduces, compares (`<=>`, about 317-bit cross products for two radii), prints (`toString`) and rounds correctly (`toLongDouble`). A pair of fixed-width integers would leave every comparison to the caller. Only `circle` is rounded.

`minimumEnclosingCircleApprox` follows the floating policy in [00-notes.md](00-notes.md#numeric-policy); see its subsection.

### EnclosingCircle

- `support`: 1 to 3 distinct input indices.
- `points`: their coordinates, in the same order.
- The circle is the point (one support point), the diametral circle (two) or the circumcircle (three, never collinear).
- `circle`: a `CircleApprox` with the rounded exact centre and the rounded distance to `points[0]`.
- `center()`: the exact centre as a `RationalPoint2`.
- `radius2()`: the exact squared radius as `Rational<InfInt>`: 0, `|a − b|^2 / 4`, or the circumradius formula above.
- `locate(q)`: `-1` inside, `0` on the circle, `1` outside, exact. It covers Library Checker `minimum_enclosing_circle`, which asks for every input point on the circle.
- `contains(q)`: `locate(q) <= 0`.

`locate`, `center` and `radius2` assert a nonempty support; `locate` also asserts query coordinates within `10^9`. Under `NDEBUG` an empty support gives the origin, radius 0, and `locate` 1 (outside). Each is `O(1)`.

The support determines the circle, but it need not be minimal. When four or more input points are cocircular, three support points can form an obtuse triangle while other boundary points certify optimality. The optimality certificate is that the centre lies in the convex hull of all input points on the circle; the tests check it.

### minimumEnclosingCircle

Input: `1 <= n < INT_MAX` integral points (the linked list holds `n + 1` entries), duplicates allowed (asserted nonempty; under `NDEBUG` an empty input returns an empty `EnclosingCircle`). The indices are shuffled with SplitMix64 from `seed`, a Fisher–Yates pass, so the run is reproducible.

The algorithm is Welzl's, as three nested scans in which the points `i` and `j` of the outer scans stay on the boundary, over a linked list of the shuffled positions (`enclosing_detail::welzl`, driven by `enclosing_detail::run`):

- **First pass, move-to-front.** Each point that becomes a support point moves to the front of the list, so later scans meet it first (Welzl's MTF heuristic). The pass stops after `32 n + 64` point tests.
- **Fallback.** If the budget runs out, the positions are reshuffled (the seed continues) and the scans run again without move-to-front and without a budget.
- **Expected `O(n)`.** The first pass costs at most `32 n + 64` tests. The fallback is Welzl's randomized incremental algorithm over a fresh uniform shuffle, expected `O(n)` (backward analysis: at each level the next point is outside with probability at most `3 / i`, `2 / i`, `1 / i`). At most two passes run. Move-to-front alone has no proven bound (Welzl: "we do not know how to analyze" it); the budget keeps the guarantee.
- **Measured.** In a one-off instrumented run (not kept in the driver), move-to-front made 3.4–5.3 tests per point where the plain scans made 6–15 (`n` up to `10^6`, four distributions), so the budget is 6–9 times its measured cost. The benchmark below shows 1.07–2.4× less time.

With exact predicates the inner circle never needs three collinear points: Welzl's lemma guarantees that the constrained circle exists and passes through both fixed points, and a collinear third point outside would contradict it. An `assert` guards this.

The algorithm is Las Vegas: the result is the unique minimum enclosing circle for every seed, and the seed affects only time and which support is reported. The support is reproducible for a fixed seed. Memory `O(n)`. The default seed is `CustomHash::rnd`, which varies by run so that a fixed shuffle cannot be attacked; pass a fixed seed to reproduce support indices.

### minimumEnclosingCircleApprox

Input: `vector<Point2<T>>` with floating `T` (`float`, `double`, `long double`), finite coordinates (asserted in `O(n)`), `1 <= n < INT_MAX` (asserted; under `NDEBUG` an empty input returns a default `CircleApprox`). It returns the `CircleApprox` (centre and radius) of the minimum enclosing circle. Floating problems ask for the centre and radius, so no support is returned.

- **Algorithm.** The same seeding, passes and budget as `minimumEnclosingCircle`, in `long double` with ordinary rounding and no epsilon. The circle through 1, 2 or 3 support points is the point, the midpoint, or the circumcentre `a + (v.y |u|^2 − u.y |v|^2, u.x |v|^2 − v.x |u|^2) / (2 cross(u, v))`.
- **Radius.** The squared radius is the largest rounded squared distance from the rounded centre to the support points. A support point and its exact duplicates therefore always test inside, so rounding never makes a scan pick them again.
- **Domain.** As the numeric policy states, every decision must survive rounding. Out of domain: points a few double ulps apart and nearly tangent to the current circle lie inside or outside by about `δ^2 / R`, below `long double` rounding, so a decision can flip and pin two near-duplicates to the boundary. In 300,000 sets of collinear doubles moved by up to 3 ulps, one returned a circle `4.9 * 10^-4` too large; no point was left outside. `long double` inputs that differ only in their last bits can also round a three-point support to collinear; its centre is then not finite and `CircleApprox` asserts.
- **Accuracy measured in domain.** Worst centre or radius error `1.7 * 10^-18` of the coordinate scale on integer sets up to `10^9` (against the exact circle); `6.8 * 10^-19` on random doubles at scales `10^-3` to `10^15` (against the Python exact oracle).

Kept beside `minimumEnclosingCircle` (`03-cpp.md`, One API per operation): floating inputs are a domain the exact version rejects.

### Legacy mapping

| Old name | Replacement |
|---|---|
| `WelzlMEC::getMEC(vector<complex<double>>)` (`OLD/Team Notebook/src/geometry/welzlmec.cpp`; double coordinates, clock seed, `variant` of two or three points) | `minimumEnclosingCircleApprox(points, seed)` on floating points; `minimumEnclosingCircle(points, seed)` on integral points with exact predicates, where `EnclosingCircle::points` holds the defining points |
| `WelzlMEC::isInside`, `indicator` | `EnclosingCircle::contains`, `locate`; `CircleApprox::locate` for the floating circle |

## Feature-to-test map

`96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py` runs quick, full and stress modes in three configurations, with six assertion probes: empty input, a coordinate of `2 * 10^9`, `locate` on an empty support, `radius2` on an empty support, empty floating input, and a NaN among ten floating points with a fixed seed. A NaN that the shuffle does not place first is never selected by the tests, so only the entry check rejects it.

**Oracles.**
- Small coordinates: every circle through 1, 2 or 3 input points (non-collinear), with exact rational squared radius and centre, the smallest enclosing one chosen by exact rational comparison.
- Medium (`10^8`) and full (`10^9`) coordinates: the optimality criterion, checked with exact rational orientation tests that compare products instead of subtracting them: the centre lies in the closed hull of the points on the circle.
- At `10^9`, an independent 256-bit side test, `|q d − N|^2` against `|s d − N|^2` for the exact centre `N / d` and a support point `s`, checks `locate` on every point.
- `radius2` at every scale: `|s d − N|^2 / d^2` in `InfInt` from the exact centre, for every support point; a different derivation from the side lengths.
- The rounded circle on integer sets, as `double`, `long double` and (below `2^24`) `float`: the three agree bit for bit, and they match the exact centre and `sqrt(radius2)` within `10^-9` of the scale.
- A Python oracle, an exact `Fraction` brute force over every 1-, 2- and 3-point candidate with the textbook circumcentre formula. It checks `radius2` and `center` exactly on integer sets up to `10^9`, a quarter of them cocircular lattice sets on `x^2 + y^2 = 1105^2` scaled by 900,000. It checks the rounded circle within `10^-12` of the scale on random doubles at scales `10^-3` to `10^15`. Quick 80, full 800, stress 8,000 rows. The binary runs both versions with the run's `--seed`, so any floating discrepancy reproduces from the printed seed.
- Forced budgets: `enclosing_detail::run` with budgets from 0 to `2n` on small sets and `n` on the large set, against the public result; `welzl` with budget 0 reports exhaustion.

| Operation | Test | Oracle |
|---|---|---|
| `minimumEnclosingCircle` | Fixed: one point, three duplicates, collinear points, a square with its centre, obtuse and acute triangles. Random sets of 1–10 points at scales 3 and 20, some with duplicates. 12 cocircular lattice points on `x^2 + y^2 = 625`, shuffled. Medium random sets. Extremes at `±10^9`. `int` coordinates. 400,000 sorted points in a disk. 100,000 duplicates. A 100,000-point collinear run. Forced fallback runs | Brute-force minimum; hull criterion; known centres and radii; Python `Fraction` brute force; the public result |
| `EnclosingCircle::support`, `points` | Every case: 1–3 distinct indices naming their points, each on the circle | Direct |
| `EnclosingCircle::center` | Every brute-force case; known centres `(2, 0)`, `(12/6, 5/6)`, `(0, 0)`; Python rows | Brute-force centre; `Fraction` centre |
| `EnclosingCircle::radius2` | Every case at every scale; known values 0, 4, `169/36`, 625, `2 * 10^18`; Python rows | Brute-force squared radius; `InfInt` distance from the exact centre; `Fraction` brute force |
| `EnclosingCircle::locate` | Every point of every brute-force case; every point of the random `10^9` sets | Exact rational side of the brute-force circle; 256-bit side test |
| `EnclosingCircle::contains` | Every point of every case | Must hold for all input points |
| `EnclosingCircle::circle` | Every case | Within `10^-15` times the coordinate scale of the exact centre and support distance; `sqrt(radius2)` |
| `minimumEnclosingCircleApprox` | Every integer case above as `double`, `long double` and `float`; 1–40 random dyadic doubles of mixed exponents; all-equal points; Python random doubles; the 400,000-point disk, duplicates and collinear run | The exact circle within `10^-9` of the scale; enclosure; seed independence; bit equality across input types; `Fraction` brute force within `10^-12` |
| Seed | Seeds 7 and 8 on random sets | Same circle for every seed; the same support when the seed repeats |

Mutation checks (2026-10-19, quick, optimized and checked builds, on a copy of the tree): 26 mutants, 22 caught.
- Caught: the diametral sign, the single-point test, the incircle orientation and tie direction; support indices left as positions; a forgotten second support point; the third scan's bound; both move-to-front pointer updates; a budget never spent; a fallback that keeps the budget (timeout); loading the points before the shuffle or not at all; three `radius2` formula errors; the centre of one point; the floating circumcentre sign, diametral centre and one-point radius; a non-strict floating outside test; the floating finiteness check (the NaN probe).
- Not caught, equivalent or runtime only: removing the collinear assertion (unreachable in exact arithmetic); a biased shuffle (the circle is unique; only time and the reported support move); no move-to-front for the second support point; move-to-front left on in the fallback (only the `O(n)` proof is lost).

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `14-minimumenclosingcircle_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 6 probes) | PASS, 1,890,454 checks and 800 Python rows per configuration, 6 probes, `MEMORY peak=600MB` |
| same with `CXX=g++-14` | PASS, same counts, 6 probes, `MEMORY peak=613MB` |
| `14-minimumenclosingcircle_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 10,452,444 checks and 8,000 Python rows, `MEMORY peak=445MB` |
| `-Wall -Wextra -Wconversion` build of the tester and benchmark with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 168 headers, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py` | No errors |
| `03-consistency.py --braces` on the header, tester and benchmark | No violations |

```bash
python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_benchmark.py' --seed 20260927
```

## Benchmarks

`14-minimumenclosingcircle_benchmark.py --seed 20260927` (2026-10-19; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five, shuffle included). Run serially on the final code; other sessions kept the load average near 2.2 on the 8-core CPU. Both exact versions must return the same centre and `radius2`, and the floating radius must agree within `10^-9` of the scale. Coordinates are within `10^9`: uniform in the square, uniform in the disk of radius `9 * 10^8`, on that circle (rounded), and Gaussian with deviation `2 * 10^8`.

| Points | n | `minimumEnclosingCircle` (µs) | plain three scans, no move-to-front (µs) | `minimumEnclosingCircleApprox` (µs) |
|---|---|---|---|---|
| square | 10^3 / 10^4 / 10^5 / 10^6 | 48 / 375 / 2,581 / 56,351 | 60 / 576 / 2,764 / 90,375 | 24 / 248 / 2,278 / 40,112 |
| disk | 10^3 / 10^4 / 10^5 / 10^6 | 46 / 556 / 4,884 / 53,047 | 101 / 1,278 / 10,336 / 103,880 | 25 / 263 / 2,370 / 35,621 |
| circle | 10^3 / 10^4 / 10^5 / 10^6 | 64 / 684 / 4,017 / 53,473 | 131 / 1,608 / 6,823 / 85,085 | 31 / 309 / 2,146 / 34,158 |
| Gaussian | 10^3 / 10^4 / 10^5 / 10^6 | 63 / 420 / 6,107 / 69,730 | 121 / 725 / 13,203 / 129,352 | 33 / 224 / 3,313 / 41,768 |

Budgeted move-to-front is faster in every row, by 1.07–2.4×. The floating version is 1.1–2.2× faster than the exact one: one squared distance per test instead of an exact incircle determinant.

A scratch comparison (not kept as a driver) also seeded the shuffle with the four axis-extreme points. Alone it helped only Gaussian inputs (2.3× at `10^6`) and was neutral or slower elsewhere; with move-to-front it gave no consistent gain, so it was not adopted.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- Library Checker `minimum_enclosing_circle` ([task](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/task.md) and [reference solution](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/sol/correct.cpp)): it uses exact `long long` predicates and asks for every point on the circle, which `locate` provides.
- [KACTL MinimumEnclosingCircle.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/MinimumEnclosingCircle.h), [cp-algorithms enclosing circle](https://cp-algorithms.com/geometry/enclosing-circle.html), [OI Wiki 最小圆覆盖](https://oi-wiki.org/geometry/random-incremental/), [maspypy minimum_enclosing_circle](https://maspypy.github.io/library/geo/minimum_enclosing_circle.hpp) and [tko919 Enclosing.hpp](https://tko919.github.io/library/Geometry/Enclosing.hpp): the three-loop form, with epsilon predicates in KACTL and OI Wiki and exact ones in maspypy.
- [CGAL Min_circle_2](https://doc.cgal.org/latest/Bounding_volumes/classCGAL_1_1Min__circle__2.html): exact bounded-side queries and support sets.
- [Welzl 1991, Smallest enclosing disks (balls and ellipsoids)](https://www.stsci.edu/~RAB/Backup%20Oct%2022%202011/f_3_CalculationForWFIRSTML/Bob1.pdf), read 2026-10-19: the expected-linear analysis, the move-to-front heuristic, its measured gains (Table 1) and the absence of an analysis for it.
- `OLD/Team Notebook/src/geometry/welzlmec.cpp`: legacy, superseded. It used double arithmetic.

## Limits and handoffs

- `circle` is rounded because the radius is irrational in general; `center` and `radius2` are exact.
- `minimumEnclosingCircleApprox` is exact only up to rounding, and only where every decision survives rounding (see its contract).
- Megiddo's deterministic linear algorithm, the enclosing circle of circles and the smallest circle covering `k` points are planned as row 57 `minimumEnclosingCircleDeterministic` and `minimumEnclosingCircleOfCircles` (P234) and row 56 `smallestDiskCoveringK` (P233). Incremental insertion and the rectangular p-centre (row 21) stay not adopted. The 3D ball belongs to row 42.

## History

- 2026-10-18: first implementation (P037). Independent review: the comment share was reduced, empty input under `NDEBUG` returns an empty result, and `10^9` sets gained the hull criterion and a 256-bit side oracle. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit rerun, header unchanged; full suite passed on g++ and g++-14, stress passed.
- 2026-10-19 (round 3): exact `radius2` as `Rational<InfInt>`; `minimumEnclosingCircleApprox`; budgeted move-to-front (1.07–2.4× faster in the serial benchmark); `center` defined on an empty support under `NDEBUG`; six probes, a Python exact oracle and 26 mutants (22 caught, 4 equivalent or runtime only). Full suite passed on g++ and g++-14, stress passed. Independent review: `locate` on an empty support returns 1 under `NDEBUG`, `n < INT_MAX` stated and asserted, the Python oracle seeded from `--seed`, benchmark ratios corrected. Final full suite passed on g++ and g++-14, stress passed.
