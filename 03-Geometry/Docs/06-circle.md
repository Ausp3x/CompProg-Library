# 06-circle.hpp — Evidence

Package P008 / GE02 owns this header together with `05-closestpair.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric Policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `circleRelation` is the exact API; every other function is an `Approx` API.

### CircleApprox

`CircleApprox` stores `dpoint c` and a finite, nonnegative `long double r`, checked by the constructor. A radius-zero boundary and disk both denote the singleton `{c}`. The fields may be changed directly if they stay in the constructor's domain.

- `locate(p, tolerance)` returns `-1`, `0` or `+1` for inside, in the closed tolerance band, or outside, by comparing `|p - c| - r` with the tolerance. `onBoundary` tests the band; `contains(dpoint)` includes the band.
- `contains(CircleApprox b, tolerance)` is closed filled-disk containment, including internal tangency: `|b.c - c| + b.r - r <= tolerance`.
- `area` and `perimeter` are `pi r^2` and `2 pi r`.
- `arcLength(sweep)` and `sectorArea(sweep)` accept any finite signed sweep in radians, including repeated revolutions. Arc length is nonnegative; sector area is oriented, negative for clockwise traversal.
- `segmentArea(sweep)` is the area between a CCW arc and its chord, for `0 <= sweep <= 2 pi`, major segments included: `r^2 / 2 * (sweep - sin(sweep))`. Below `0.5`, `sweep - sin(sweep)` is evaluated by its Taylor series through the `x^17` term (Horner form, factors `y / 20` to `y / 272`). The omitted relative term is below `7.5e-22` at `0.5`. Measured against `__float128` on `(1e-6, 1.6)`, the worst relative error is `1.1e-18`, at `0.526`, where the direct difference loses about 4.6 bits.

### CircleIntersectionApprox

`count` is `0`, `1`, `2` or `-1` (infinitely many points); only `points[0..count)` are meaningful.

### circleLineIntersectionApprox, circleSegmentIntersectionApprox

- `circleLineIntersectionApprox(c, a, b)` intersects the circle boundary with the infinite line through `a != b` (asserted). With `w = b - a`, `z = cross(w, a - c.c)` and `D = r^2 |w|^2 - z^2`, the computed sign of `D` gives the count 0, 1 or 2. The foot is `c.c + perp(w) * z / |w|^2`, and the points are `foot -+ w * sqrt(D) / |w|^2`, in `a`-to-`b` order. Nothing is normalized first, so each classification quantity is rounded once. For integer-valued inputs with `|coordinates|, r <= 2^14`, every product is below `2^64` and the classification, tangency included, is exact. Without normalization `r^2 |w|^2` overflows sooner, near `1e1200`, which is within the finite-intermediate precondition.
- `circleSegmentIntersectionApprox(c, a, b)` intersects the boundary with the closed segment `[a, b]`. It returns the line points (same values and order) whose parameter lies in `[0, 1]`. With `g(t) = power(a + t w) = A t^2 + 2 B t + C`, `B0 = dot(a - c, w)`, `C0 = power(a)`, `B1 = dot(b - c, w)` and `C1 = power(b)`, the smaller root is in `[0, 1]` iff `B0 <= 0`, `C0 >= 0` and (`B1 >= 0` or `C1 <= 0`), and the larger root iff (`B0 <= 0` or `C0 <= 0`), `B1 >= 0` and `C1 >= 0`. The derivation compares `-B -+ sqrt(D)` with `0` and `A`, using `B1 = A + B0` and `(A + B0)^2 - D = A * C1`, so no square root is involved. In the same integer domain the endpoint decisions are exact. Outside it, rounding is pinned (user decision 2026-11-14): the computed signs of these four quantities and of the line's `D` decide independently, even where they disagree with exact geometry, so at a rounded tangency the segment can keep a second line point that the exact segment misses (the tester's `computed signs decide the second root` regression). `a == b` is a singleton, giving one point iff the computed power of `a` is zero.

### circleIntersectionApprox, circleCentersApprox

These intersect two boundaries. Nested disjoint circles give zero points. Coincident positive-radius circles give `-1`, coincident point circles one point, and concentric distinct circles zero. The chord coordinate comes from subtracting the circle equations. Tangency is decided by computed equality of `|v|` with `r1 + r2` or `|r1 - r2|`. `circleCentersApprox(a, b, r)` returns the centers of radius-`r` circles through `a` and `b`; a repeated point gives `-1` for `r > 0` and one center for `r == 0`.

### circleRelation

`circleRelation(a, ra, b, rb)` classifies two circles exactly, for signed integral `T` of at most 64 bits (static) with `|coordinates|, radii <= 10^18` and radii `> 0` (asserted). Comparing `d^2` with `(ra + rb)^2` and `(ra - rb)^2` in `lll` gives `4` separate, `3` externally tangent, `2` crossing, `1` internally tangent, `0` nested (concentric distinct circles included), or `-1` for identical circles. For non-identical circles this equals the number of common tangent lines.

### CircleTangentApprox, CircleTangentsApprox, commonTangentsApprox, pointTangentsApprox

- `CircleTangentApprox` holds the contact points `a` (on the first circle) and `b` (on the second) and the unit `direction` of the tangent line, which stays meaningful when the two contacts coincide.
- `CircleTangentsApprox` holds at most two lines of the requested family; `count == -1` means infinitely many distinct lines.
- `commonTangentsApprox(a, b, inner)` gives the outer family (`inner == false`, centers on the same side) or the inner family (centers on opposite sides). Tangent normals solve `dot(n, c2 - c1) = r1 - signed r2`. A radius-zero circle denotes a point, and every line through it is tangent, so the two families coincide when either radius is zero: two distinct points give one line, identical points give `-1`.
- With two lines, `lines[0]` is the right-hand tangent and `lines[1]` the left-hand one, seen from `a.c` toward `b.c`: each line's unit normal `n = rotateCW(direction)` (the direction from `a.c` toward its contact on `a`) satisfies `cross(b.c - a.c, n) < 0` for `lines[0]` and `> 0` for `lines[1]` (user decision 2026-11-14, P008).
- `pointTangentsApprox(c, p)` is `commonTangentsApprox(c, CircleApprox(p, 0))`. The contact on `c` is in `.a`; `p` is in `.b`.

### circumcircleApprox, incircleApprox

Both return `false` for computed-collinear or repeated vertices and then leave `out` unchanged. The circumcenter solves the two equal-distance equations in coordinates translated to the first vertex; a nearly collinear triangle can be arbitrarily ill-conditioned. The incenter uses the side-length barycentric weights, so it does not depend on vertex orientation; the inradius is twice the area divided by the perimeter.

### diskOverlapAreaApprox

The area of the intersection of the two filled disks. Disjoint or tangent disks give `0`; a contained disk contributes its whole area. Otherwise the result is the sum of the two chord caps, from `atan2` angles and `segmentArea`.

### powerApprox, radicalAxisApprox

- `powerApprox(c, p) = |p - c|^2 - r^2`: negative inside, zero on the boundary, positive outside, and the squared tangent length from an exterior point. It is exact for integer-valued inputs whose squares are below `2^64`.
- `radicalAxisApprox(a, b, out)` returns `false` for concentric circles, leaving `out` unchanged. Otherwise `out = {p, p + perp(b.c - a.c)}`, the line of equal power, where `p` is its foot on the center line. Its left side holds the points with smaller power to `a`. For intersecting circles the line passes through both intersection points.

## Feature-to-Test Map

`96-Local Testing/03-Geometry/06-circle_tester.py` drives the suite; its docstring states the mode sizes. Residuals use a `2e-12` scaled absolute or relative allowance on the stated finite domain; slice integration allows `8e-6` of the smaller area. Failures print the seed, phase, operation, reproducing input and predicate.

| Operation | Test | Oracle |
|---|---|---|
| `circleLineIntersectionApprox` | Exhaustive integer discriminant counts with axis directions; Pythagorean directions `(3,4)`, `(4,3)`, `(-3,4)`, `(5,12)`, `(12,5)`, `(8,15)`, `(-15,8)` with `\|a\| <= 75` (full): every tangent and missing line and every fifth secant; boundary and line residuals; `a`-to-`b` order; regressions `(-7,-4)-(-3,-1)` and `(-53,-41)-(-49,-38)`; reversal | Exact integer discriminant; random algebraic quadratic oracle |
| `circleSegmentIntersectionApprox` | Exhaustive integer segments (endpoints in `[-5,5]^2`, radii `0..5`, full) and all Pythagorean-direction segments; residuals, closed-segment membership, `a`-to-`b` order, reversal reverses the points; singleton, inside, outside-on-secant, chord-endpoint and endpoint-tangency (both orders) regressions; the pinned second root at a rounded tangency | Exact root-placement oracle that squares `0 <= -B -+ sqrt(D) <= A` directly |
| `CircleIntersectionApprox`, `circleIntersectionApprox` | Point-set symmetry; both radial residuals; empty, nested, concentric, coincident, zero-radius and tangent cases | Exhaustive integer squared-distance classification |
| `circleRelation` | Exhaustive grid; regressions for every value, `10^18` coordinates and `int`; the `relation-bound` probe | Tangent-count formula `2[q > (r-s)^2] + [q = (r-s)^2 > 0] + 2[q > (r+s)^2] + [q = (r+s)^2]`; supporting check that it equals the outer plus inner `commonTangentsApprox` counts |
| `CircleTangentApprox`, `CircleTangentsApprox`, `commonTangentsApprox`, `pointTangentsApprox` | Contacts on both boundaries, unit direction, perpendicular radii, collinear contacts, center sides, distinct lines and the right-hand line first; random point-tangent right angles; point-circle cases | Exhaustive exact squared-distance tangent counts |
| `circleCentersApprox` | Diameter, impossible, repeated-point and zero-radius cases | Constructed 3-4-5 geometry |
| `circumcircleApprox`, `incircleApprox` | Collinear and repeated inputs return false with `out` unchanged; 1,600 random triangles checked for equal distances, interior incenters and permutation invariance | 3-4-5 answers; geometric properties; triangle cases in `09-triangle_tester` |
| `CircleApprox`: `locate`, `onBoundary`, `contains` (point, disk), `area`, `perimeter`, `arcLength`, `sectorArea`, `segmentArea` | Inside, boundary and outside (`onBoundary` false inside and outside), tolerance band, internal tangency; zero, half, quarter and full sweeps, negative sweeps, repeated revolutions | Exhaustive disk containment against exact squared distances; closed forms |
| `segmentArea` series switch | Relative error `<= 4e-18` on 1,436 geometric sweeps from `1e-6` to `1.6`, across the `0.5` switch; `segmentArea(0.5)` equals the direct form `(0.5 - sin(0.5)) / 2` exactly | Direct alternating Taylor summation, accurate to `3.1e-19` against `__float128` |
| `diskOverlapAreaApprox` | 75 lenses; containment (seven off-centre nested pairs equal the inner `area()` exactly), disks disjoint by `6e-20` giving exactly 0, zero-radius, disjoint and touching cases; symmetry, rigid motion and scale covariance; small external-lens asymptotics | Independent midpoint integration of vertical slices; unit-lens closed form |
| `powerApprox` | Exhaustive grid; random exterior points | Exact integer power; squared tangent length |
| `radicalAxisApprox` | Exists iff the centers differ; concentric leaves `out` unchanged; the left side has smaller power to `a`; lens-chord regression | Intersection points lie on the axis (exhaustive); equal powers at four points along random axes |
| Floating scale | Constructions at scales `1e-100` through `1e100`; external tangency gaps `1e-4`, `1e-8`, `1e-12` | Scaled residuals |
| Preconditions | Fourteen checked-build probes: negative or NaN radius, infinite center, repeated line endpoints, NaN first line point, negative tolerance, NaN point, infinite sweep, out-of-range segment sweeps, NaN segment endpoint, NaN power point, zero relation radius, relation coordinate above `10^18` | Expected assertion failure |

## Commands and Results

The header's logic is unchanged since 2026-10-07 (P274 added only `[[maybe_unused]]`). Package runs: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns, experiment none.

| Command | Result |
|---|---|
| 2026-11-15 `/finish P008`: full on g++ and g++-14, stress, extraction, `02-integration.py` | PASS (counts not recorded) |
| `06-circle_tester.py --mode full --seed 20261114` (optimized, checked, ASan/UBSan; GCC 16.2.1), 2026-11-14 | PASS, 7,261,398 checks per configuration; 14 assertion probes; `MEMORY peak=828MB` |
| `CXX=g++-14 06-circle_tester.py --mode full --seed 20261114` (GCC 14.4.1), 2026-11-14 | PASS, the same counts; `MEMORY peak=882MB` |
| `06-circle_tester.py --mode stress --seed 42` (all three configurations), 2026-11-14 | PASS, 80,654,252 checks per configuration; `MEMORY peak=528MB` |
| `05-extraction.py testers --filter 03-Geometry/0`, 2026-11-14 | PASS, 20 names for this tester, quick on the extracted build |
| `02-integration.py --sanitizers`, 2026-11-14 | PASS |
| `01-run.py --mode full`, library-wide, 2026-11-10 (P274, seed 42, build cache emptied before each cold run) | PASS; GCC 16.2.1: optimized 0.2 s compile + 2.3 s run, checked 13.0 s + 2.7 s, ASan/UBSan 14.2 s + 35.6 s, peak 835 MB (pre-flight quick 703 MB); GCC 14.4.1: optimized 0.2 s + 2.9 s, checked 17.5 s + 3.6 s, ASan/UBSan 18.6 s + 24.9 s, peak 884 MB (pre-flight quick 759 MB) |
| `01-run.py --mode quick`, library-wide, 2026-11-10 | PASS; optimized 20.3 s compile + 0.1 s run, checked 15.6 s + 0.4 s; peak 700 MB |
| `01-run.py --mode stress --rounds 1`, library-wide, 2026-11-10 | PASS; optimized 0.1 s compile + 7.5 s run, checked 4.5 s + 10.1 s, ASan/UBSan 6.0 s + 134.1 s; peak 528 MB (pre-flight quick 718 MB) |
| `04-mutation.py run 03-Geometry/06-circle.hpp --whole` | See Mutation Testing |

```bash
python3 '96-Local Testing/03-Geometry/06-circle_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/03-Geometry/06-circle_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/03-Geometry/06-circle_tester.py' --mode stress --seed 42
python3 '96-Local Testing/05-extraction.py' testers --filter 03-Geometry/0
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/06-circle.hpp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `6a8811de0672f1a044906f720dd4b06685ca67c5` (2026-11-15, seed 1, whole file, uniform sample of 1177): 500 mutants, 487 caught (0 by timeout), 13 survived, 0 untested.

Stage times: quick 382 s, full optimized 14 s, full rest 125 s (baselines 3.8 s, 4.3 s and 15.7 s). This `/finish P008` campaign reran the whole file after regressions closed nine survivors of the first campaign (2026-11-14, same blob and sample). The 13 survivors are equivalent:

| Mutant | Line | Why equivalent |
|---|---|---|
| `AS 253`, `AS 265` | 75, 108 | Commuted symmetric arguments: `dot(a - c.c, w)` and the exact `dist2(a, b)` |
| `AS 246`, `AS 266` | 9, 127 | `max(0.L, y)` → `max(y, 0.L)` differs only for a NaN or negative-zero `y`. Inputs are finite, and `(r - x) * (r + x)` with `r >= 0` and `(1 - x) * (1 + x)` are never `-0`: a zero factor is `+0` (a difference of equal values rounds to `+0`), and the other factor is then `2r` or 2, not negative |
| `COS 932`, `COS 1112`, `COS 1137`, `COS 1152` | 59, 124, 129, 146 | `==` → `<=` or `>=` in a comparison with zero of a value that is never negative: the squared norm `d`, the length `d`, the square root `h` and the absolute value `z` |
| `COS 1039`, `COS 1042` | 90 | `d == a.r + b.r` → `>=` and `d == abs(a.r - b.r)` → `<=`: line 87 already returned unless `abs(a.r - b.r) <= d <= a.r + b.r` |
| `COS 1109` | 110 | `d == s` → `d >= s`: `d > s` returned before |
| `CR 192`, `CR 193` | 138 | Removed validity assertions of `a` and `b` in `circumcircleApprox`: a NaN or infinite `a` or `b` makes the centre `a + w` NaN or infinite, so the checked build still aborts in the `CircleApprox` constructor's assertion; `NDEBUG` builds differ only past the violated precondition, in O(1) (user decision 2026-11-12) |

## Sources

The implementation is written from geometric identities (2026-09-27, extended 2026-10-07). No code or prose was copied; references were compared only for equations and degeneracy contracts.

- [cp-algorithms, Circle-Line Intersection](https://cp-algorithms.com/geometry/circle-line-intersection.html) and [Circle-Circle Intersection](https://cp-algorithms.com/geometry/circle-circle-intersection.html): the projected foot plus half-chord and the radical-line reduction. Their origin-only representation and epsilon contract were replaced.
- [KACTL `CircleTangents.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/CircleTangents.h) (CC0): the signed second radius for inner and point tangents.
- [Stanford ICPC notebook 2015–16](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), printed pages 6–7 (saved as `95-Resources/07-stanford_notebook.pdf`): circle intersection and circumcenter routines. Its perturbed square root and missing concentric guard were not adopted.
- MathWorld [Circle-Circle Intersection](https://mathworld.wolfram.com/Circle-CircleIntersection.html) and [Incenter](https://mathworld.wolfram.com/Incenter.html): the cap decomposition and the side-length incenter formula.
- The 2026-10-07 additions (segment, power, radical axis, relation) are cited in [00-sources.md](00-sources.md#p008-completeness-sweep-fetched-2026-10-07); the 2026-11-14 re-audit sources are in [00-sources.md](00-sources.md#p008-re-audit-fetched-2026-11-14) and its decisions in [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).
- `OLD/Team Notebook/src/geometry/intersection.cpp`: its circle-line and circle-circle functions are superseded here; its other functions belong to P007 (`02-line_segment.hpp`, [evidence](02-line_segment.md)). The bytes are unchanged.

## Limits and Handoffs

No claim of universally certified floating geometry is made. Other packages own adaptive and exact floating predicates (GE11), algebraic circle constructions including the Apollonian circle (row 24 / GE29), minimum enclosing circles (GE05), and circle unions and circle-polygon operations (GE10 and GE24).

## Aliases

Search aliases (P263, grab --find): 圆的切线, 公切线, 两圆交点, 圆与直线交点, 外接圆, 内切圆, 两圆相交面积, 圆的位置关系.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 146,311 checks) passed on g++ 16; segment, power and radical axis missing.
- 2026-10-07: re-audit, implemented `circleSegmentIntersectionApprox`, `powerApprox`, `radicalAxisApprox` and `circleRelation`; 7 findings fixed; full suite and stress passed on g++ 16; the independent review reproduced every full count and probe and confirmed one defect (the `circleRelation` domain assertion), fixed.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; `[[maybe_unused]]` on names only an `assert` uses, no logic change; no mutation campaign in P274 (user decision 2026-11-10); library-wide full, quick and stress passed.
- 2026-11-14: P008 re-audit (experiment none): the completeness sweep found no operation missing from the row; first mutation campaign; five tests closed its gaps (`onBoundary`, the right-hand-first order of `commonTangentsApprox`, now documented by user decision, the `nan-line` probe, the exact `segmentArea` switch at 0.5, exact nested-disk areas) and an endpoint-tangency segment regression was added; no header change; full on g++ and g++-14, stress, extraction and `02-integration.py --sanitizers` passed; the independent review found no defect in this header.
- 2026-11-15: `/finish P008` (experiment none): the independent review showed four survivors recorded as equivalent were distinguishable (IF 221 and COS 1045 by disks disjoint by `6e-20`, COS 982 and 994 by near-tangent segments); each is now a regression, and the computed signs those segments depend on are pinned in Contracts (user decision 2026-11-14); second whole-file campaign caught 487 of 500, 13 equivalent survivors; no header change; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
