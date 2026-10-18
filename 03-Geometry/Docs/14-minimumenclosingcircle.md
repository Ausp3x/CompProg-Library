# 14-minimumenclosingcircle.hpp — evidence

Owned by package P037 / GE05, together with [13-segment_union.md](13-segment_union.md) and [15-convex_polygon_query.md](15-convex_polygon_query.md). Every operation in the row has a test.

## Contracts

### Numeric policy

Points are integral with `|coordinate| <= 10^9`, asserted. Every decision is an exact integer predicate:

- the diametral test `dot(a − q, b − q)`, below `8 * 10^18`;
- the incircle determinant `Σ |a'|^2 * cross(b', c')` with `x' = x − q`. Each of its three products is below `8 * 10^18 * 8 * 10^18 = 6.4 * 10^37`, so the sum of two fits `lll`. The sign of the full sum is the comparison of that partial sum with the negated third product.

The exact centre has numerators below `5 * 10^28` and denominator `2 * cross(b − a, c − a)`, below `1.6 * 10^19`. Only `circle` is rounded.

### EnclosingCircle

- `support`: 1 to 3 distinct input indices.
- `points`: their coordinates, in the same order.
- The circle is the point (one support point), the diametral circle (two) or the circumcircle (three, never collinear).
- `circle`: a `CircleApprox` with the rounded exact centre and the rounded distance to `points[0]`.
- `center()`: the exact centre as a `RationalPoint2`.
- `locate(q)`: `-1` inside, `0` on the circle, `1` outside, exact. It covers Library Checker `minimum_enclosing_circle`, which asks for every input point on the circle.
- `contains(q)`: `locate(q) <= 0`.

`locate` and `center` assert a nonempty support and query coordinates within `10^9`. Each is `O(1)`.

The support determines the circle, but it need not be minimal. When four or more input points are cocircular, three support points can form an obtuse triangle while other boundary points certify optimality. The optimality certificate is that the centre lies in the convex hull of all input points on the circle; the tests check it.

### minimumEnclosingCircle

Input: `1 <= n <= INT_MAX` points, duplicates allowed (asserted nonempty; under `NDEBUG` an empty input returns an empty `EnclosingCircle`). The function shuffles the indices with SplitMix64 from `seed`, a Fisher–Yates pass, so the run is reproducible. Then it runs the randomized incremental form of Welzl's algorithm: three nested scans in which the points `i` and `j` of the outer loops stay on the boundary. With exact predicates the inner circle never needs three collinear points. Welzl's lemma guarantees that the constrained circle exists and passes through both fixed points, and a collinear third point outside would contradict it. An `assert` guards this.

The algorithm is Las Vegas: the result is the unique minimum enclosing circle for every seed, and the seed affects only time. Time is `O(n)` expected over the shuffle (Welzl 1991), memory `O(n)`. The default seed is `CustomHash::rnd`, which varies by run so that a fixed shuffle cannot be attacked; pass a fixed seed to reproduce support indices.

### Legacy mapping

| Old name | Replacement |
|---|---|
| `WelzlMEC::getMEC(vector<complex<double>>)` (`OLD/Team Notebook/src/geometry/welzlmec.cpp`; double coordinates, clock seed, `variant` of two or three points) | `minimumEnclosingCircle(points, seed)` on integral points with exact predicates; `EnclosingCircle::points` holds the defining points |
| `WelzlMEC::isInside`, `indicator` | `EnclosingCircle::contains`, `locate` |
| `verifyEnclosing` (planned row entry) | `EnclosingCircle::contains` per point (decided 2026-10-18) |

## Feature-to-test map

`96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py` runs quick, full and stress modes in three configurations, with three assertion probes: empty input, a coordinate of `2 * 10^9`, and `locate` on an empty support.

**Oracles.**
- Small coordinates: every circle through 1, 2 or 3 input points (non-collinear), with exact rational squared radius and centre, the smallest enclosing one chosen by exact rational comparison.
- Medium (`10^8`) and full (`10^9`) coordinates: the optimality criterion, checked with exact rational orientation tests that compare products instead of subtracting them: the centre lies in the closed hull of the points on the circle.
- At `10^9`, an independent 256-bit side test, `|q d − N|^2` against `|s d − N|^2` for the exact centre `N / d` and a support point `s`, checks `locate` on every point.

| Operation | Test | Oracle |
|---|---|---|
| `minimumEnclosingCircle` | Fixed: one point, three duplicates, collinear points, a square with its centre, obtuse and acute triangles. Random sets of 1–10 points at scales 3 and 20, some with duplicates. 12 cocircular lattice points on `x^2 + y^2 = 625`, shuffled. Medium random sets. Extremes at `±10^9`. `int` coordinates. 400,000 sorted points in a disk. 100,000 duplicates. A 100,000-point collinear run | Brute-force minimum; hull criterion; known centres and radii |
| `EnclosingCircle::support`, `points` | Every case: 1–3 distinct indices naming their points, each on the circle | Direct |
| `EnclosingCircle::center` | Every brute-force case; known centres `(2, 0)`, `(12/6, 5/6)`, `(0, 0)` | Brute-force centre |
| `EnclosingCircle::locate` | Every point of every brute-force case; every point of the random `10^9` sets | Exact rational side of the brute-force circle; 256-bit side test |
| `EnclosingCircle::contains` | Every point of every case | Must hold for all input points |
| `EnclosingCircle::circle` | Every case | Within `10^-15` times the coordinate scale of the exact centre and support distance |
| Seed | Seeds 7 and 8 on random sets | Same circle for every seed; the same support when the seed repeats |

Mutation check (2026-10-18, quick, optimized): negating the incircle orientation factor is caught.

## Commands and results

2026-10-18, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `14-minimumenclosingcircle_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 3 probes) | PASS, 676,132 checks per configuration, 3 probes, `MEMORY peak=414MB` |
| same with `CXX=g++-14` | PASS, 676,132 checks per configuration, 3 probes, `MEMORY peak=419MB` |
| `14-minimumenclosingcircle_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 3,926,268 checks, `MEMORY peak=335MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=650MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/14-minimumenclosingcircle_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

None required by the contest profile. The 400,000-point case runs inside the full suite.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- Library Checker `minimum_enclosing_circle` ([task](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/task.md) and [reference solution](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/geo/minimum_enclosing_circle/sol/correct.cpp)): it uses exact `long long` predicates and asks for every point on the circle, which `locate` provides.
- [KACTL MinimumEnclosingCircle.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/geometry/MinimumEnclosingCircle.h), [cp-algorithms enclosing circle](https://cp-algorithms.com/geometry/enclosing-circle.html), [OI Wiki 最小圆覆盖](https://oi-wiki.org/geometry/random-incremental/), [maspypy minimum_enclosing_circle](https://maspypy.github.io/library/geo/minimum_enclosing_circle.hpp) and [tko919 Enclosing.hpp](https://tko919.github.io/library/Geometry/Enclosing.hpp): the three-loop form, with epsilon predicates in KACTL and OI Wiki and exact ones in maspypy.
- [CGAL Min_circle_2](https://doc.cgal.org/latest/Bounding_volumes/classCGAL_1_1Min__circle__2.html): exact bounded-side queries and support sets.
- `OLD/Team Notebook/src/geometry/welzlmec.cpp`: legacy, superseded. It used double arithmetic.

## Limits and handoffs

- The squared radius is not returned exactly. Its numerator has degree 6, which exceeds `lll` at `10^9`. Compare `locate` signs instead.
- Not adopted (see 00-notes): the enclosing circle of circles, incremental insertion, Megiddo's deterministic linear algorithm, the smallest circle covering `k` points (row 54 territory) and the rectangular p-centre. The 3D ball belongs to row 42.

## History

- 2026-10-18: first implementation (P037). Independent review: the comment share was reduced, empty input under `NDEBUG` returns an empty result, and `10^9` sets gained the hull criterion and a 256-bit side oracle. Full suite passed on g++ and g++-14, stress passed.
