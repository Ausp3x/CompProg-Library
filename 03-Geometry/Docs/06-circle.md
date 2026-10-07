# 06-circle.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `05-closestpair.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`); see [00-notes.md](00-notes.md#p008--ge02-package-record) for the package record and the folder numeric policy. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `circleRelation` is the exact API; every other function is an `Approx` API using ordinary `long double` arithmetic. Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding. Classification uses computed comparisons with no hidden epsilon and no certified topology near degeneracy. Only membership queries (`locate`, `onBoundary`, `contains`) accept an explicit absolute length tolerance `>= 0`; every tolerance defaults to zero. Approximate constructions return a line as two points; its left side is the positive side. Counts use `-1` for infinitely many. In a fixed-size result array, only the prefix `[0, count)` is meaningful.

### CircleApprox

`CircleApprox` stores `dpoint c` and a finite, nonnegative `long double r`, checked by the constructor. A radius-zero boundary and disk both denote the singleton `{c}`. The fields may be changed directly if they keep the constructor's domain.

- `locate(p, tolerance)` returns `-1`, `0` or `+1` for inside, in the closed tolerance band, or outside, by comparing `|p - c| - r` with the tolerance. `onBoundary` tests the band; `contains(dpoint)` includes the band.
- `contains(CircleApprox b, tolerance)` is closed filled-disk containment, including internal tangency: `|b.c - c| + b.r - r <= tolerance`.
- `area` and `perimeter` are `pi r^2` and `2 pi r`.
- `arcLength(sweep)` and `sectorArea(sweep)` accept any finite signed sweep in radians, including repeated revolutions. Arc length is nonnegative; sector area is oriented, negative for clockwise traversal.
- `segmentArea(sweep)` is the area between a CCW arc and its chord, for `0 <= sweep <= 2 pi`, major segments included. It equals `r^2 / 2 * (sweep - sin(sweep))`. Below `0.5`, `sweep - sin(sweep)` is evaluated by its Taylor series through the `x^17` term (Horner form, factors `y / 20` to `y / 272`). The omitted relative term is below `7.5e-22` at `0.5`. Measured against `__float128` on `(1e-6, 1.6)`, the worst relative error is `1.1e-18`, at `0.526`, where the direct difference loses about 4.6 bits.

### CircleIntersectionApprox

`count` is `0`, `1`, `2` or `-1` (infinitely many points); only `points[0..count)` are meaningful.

### circleLineIntersectionApprox, circleSegmentIntersectionApprox

- `circleLineIntersectionApprox(c, a, b)` intersects the circle boundary with the infinite line through `a != b` (asserted). With `w = b - a`, `z = cross(w, a - c.c)` and `D = r^2 |w|^2 - z^2`, the count is 0, 1 or 2 by the computed sign of `D`. The foot is `c.c + perp(w) * z / |w|^2`, and the points are `foot -+ w * sqrt(D) / |w|^2`, in `a`-to-`b` order. Nothing is normalized first, so each classification quantity is rounded once. For integer-valued inputs with `|coordinates|, r <= 2^14`, every product is below `2^64`, and the classification, including tangency, is exact. Without normalization, `r^2 |w|^2` overflows sooner, near `1e1200`; that is within the finite-intermediate precondition.
- `circleSegmentIntersectionApprox(c, a, b)` intersects the boundary with the closed segment `[a, b]`. It returns the subset of the line points (same values and order) whose parameter lies in `[0, 1]`. With `g(t) = power(a + t w) = A t^2 + 2 B t + C`, the smaller root is in `[0, 1]` iff `B0 <= 0`, `C0 >= 0` and (`B1 >= 0` or `C1 <= 0`), and the larger root iff (`B0 <= 0` or `C0 <= 0`), `B1 >= 0` and `C1 >= 0`. Here `B0 = dot(a - c, w)`, `C0 = power(a)`, `B1 = dot(b - c, w)` and `C1 = power(b)`. The derivation compares `-B -+ sqrt(D)` with `0` and `A`, using `B1 = A + B0` and `(A + B0)^2 - D = A * C1`, so no square root is involved. In the same integer domain, the endpoint decisions are exact. `a == b` is a singleton, giving one point iff the computed power of `a` is zero.

### circleIntersectionApprox, circleCentersApprox

These intersect two boundaries. Nested disjoint circles give zero points. Coincident positive-radius circles give `-1`, coincident point circles give one point, and concentric distinct circles give zero. The chord coordinate comes from subtracting the circle equations. Tangency is decided by computed equality of `|v|` with `r1 + r2` or `|r1 - r2|`. `circleCentersApprox(a, b, r)` returns the centers of radius-`r` circles through `a` and `b`. A repeated point gives `-1` for `r > 0` and one center for `r == 0`.

### circleRelation

`circleRelation(a, ra, b, rb)` classifies two circles exactly, for signed integral `T` with `|coordinates|, radii <= 10^18` and radii `> 0`; `T` is at most 64 bits (static) and the bounds and positivity are asserted. Comparing `d^2` with `(ra + rb)^2` and `(ra - rb)^2` in `lll` gives `4` separate, `3` externally tangent, `2` crossing, `1` internally tangent, `0` nested (concentric distinct circles included), or `-1` for identical circles. For non-identical circles, this equals the number of common tangent lines.

### CircleTangentApprox, CircleTangentsApprox, commonTangentsApprox, pointTangentsApprox

- `CircleTangentApprox` holds the contact points `a` (on the first circle) and `b` (on the second) and the unit `direction` of the tangent line. The direction stays meaningful when the two contacts coincide.
- `CircleTangentsApprox` holds at most two lines of the requested family; `count == -1` means infinitely many distinct lines.
- `commonTangentsApprox(a, b, inner)` gives the outer family (`inner == false`, centers on the same side) or the inner family (centers on opposite sides). Tangent normals solve `dot(n, c2 - c1) = r1 - signed r2`. A radius-zero circle denotes a point, and every line through it is tangent. The two families therefore coincide when either radius is zero: two distinct points give one line, and identical points give `-1`.
- `pointTangentsApprox(c, p)` is `commonTangentsApprox(c, CircleApprox(p, 0))`. The contact on `c` is in `.a`; `p` is in `.b`.

### circumcircleApprox, incircleApprox

Both return `false` for computed-collinear or repeated vertices, and then leave `out` unchanged. The circumcenter solves the two equal-distance equations in coordinates translated to the first vertex; a nearly collinear triangle can be arbitrarily ill-conditioned. The incenter uses the side-length barycentric weights, so it does not depend on vertex orientation; the inradius is twice the area divided by the perimeter.

### diskOverlapAreaApprox

This is the area of the intersection of the two filled disks. Disjoint or tangent disks give `0`; a contained disk contributes its entire area. Otherwise the result is the sum of the two chord caps, using `atan2` angles and `segmentArea`.

### powerApprox, radicalAxisApprox

- `powerApprox(c, p) = |p - c|^2 - r^2`. It is negative inside, zero on the boundary and positive outside, and it equals the squared tangent length from an exterior point. For integer-valued inputs whose squares are below `2^64` it is exact.
- `radicalAxisApprox(a, b, out)` returns `false` for concentric circles, leaving `out` unchanged. Otherwise `out = {p, p + perp(b.c - a.c)}`, the line of equal power, where `p` is its foot on the center line. The left side contains the points with smaller power to `a`. For intersecting circles the line passes through both intersection points.

## Feature-to-test map

`96-Local Testing/03-Geometry/06-circle_tester.py` drives the suite; mode sizes are stated in the tester docstring. Residuals use a `2e-12` scaled absolute or relative allowance on the stated finite domain; slice integration allows `8e-6` of the smaller area. Failures print the seed, phase, operation, reproducing input and predicate.

| Operation | Test | Oracle |
|---|---|---|
| `circleLineIntersectionApprox` | Exhaustive integer discriminant counts with axis directions; Pythagorean directions `(3,4)`, `(4,3)`, `(-3,4)`, `(5,12)`, `(12,5)`, `(8,15)`, `(-15,8)` with `\|a\| <= 75` (full): every tangent and missing line and every fifth secant; boundary and line residuals; `a`-to-`b` order; regressions `(-7,-4)-(-3,-1)` and `(-53,-41)-(-49,-38)`; reversal | Exact integer discriminant; random algebraic quadratic oracle |
| `circleSegmentIntersectionApprox` | Exhaustive integer segments (endpoints in `[-5,5]^2`, radii `0..5`, full) and all Pythagorean-direction segments; residuals, closed-segment membership, `a`-to-`b` order, reversal reverses the points; singleton, inside, outside-on-secant and chord-endpoint regressions | Exact root-placement oracle that squares `0 <= -B -+ sqrt(D) <= A` directly |
| `CircleIntersectionApprox`, `circleIntersectionApprox` | Point-set symmetry; both radial residuals; empty, nested, concentric, coincident, zero-radius and tangent cases | Exhaustive integer squared-distance classification |
| `circleRelation` | Exhaustive grid; regressions for every value, `10^18` coordinates and `int`; the `relation-bound` probe | Tangent-count formula `2[q > (r-s)^2] + [q = (r-s)^2 > 0] + 2[q > (r+s)^2] + [q = (r+s)^2]`; supporting check that it equals the outer plus inner `commonTangentsApprox` counts |
| `CircleTangentApprox`, `CircleTangentsApprox`, `commonTangentsApprox`, `pointTangentsApprox` | Contacts on both boundaries, unit direction, perpendicular radii, collinear contacts, center sides and distinct lines; random point-tangent right angles; point-circle cases | Exhaustive exact squared-distance tangent counts |
| `circleCentersApprox` | Diameter, impossible, repeated-point and zero-radius cases | Constructed 3-4-5 geometry |
| `circumcircleApprox`, `incircleApprox` | Collinear/repeated inputs return false with `out` unchanged; 1,600 random triangles checked for equal distances, interior incenters and permutation invariance | 3-4-5 answers; geometric properties |
| `CircleApprox`: `locate`, `onBoundary`, `contains` (point, disk), `area`, `perimeter`, `arcLength`, `sectorArea`, `segmentArea` | Inside/boundary/outside, tolerance band, internal tangency; zero/half/quarter/full sweeps, negative sweeps, repeated revolutions | Exhaustive disk containment against exact squared distances; closed forms |
| `segmentArea` series switch | Relative error `<= 4e-18` on 1,436 geometric sweeps from `1e-6` to `1.6`, across the `0.5` switch | Direct alternating Taylor summation, accurate to `3.1e-19` against `__float128` |
| `diskOverlapAreaApprox` | 75 lenses; containment, zero-radius, disjoint and touching cases; symmetry, rigid motion and scale covariance; small external-lens asymptotics | Independent midpoint integration of vertical slices; unit-lens closed form |
| `powerApprox` | Exhaustive grid; random exterior points | Exact integer power; squared tangent length |
| `radicalAxisApprox` | Exists iff the centers differ; concentric leaves `out` unchanged; the left side has smaller power to `a`; lens-chord regression | Intersection points lie on the axis (exhaustive); equal powers at four points along random axes |
| Floating scale | Constructions at scales `1e-100` through `1e100`; external tangency gaps `1e-4`, `1e-8`, `1e-12` | Scaled residuals |
| Preconditions | Thirteen checked-build probes: negative or NaN radius, infinite center, repeated line endpoints, negative tolerance, NaN point, infinite sweep, out-of-range segment sweeps, NaN segment endpoint, NaN power point, zero relation radius, relation coordinate above `10^18` | Expected assertion failure |

## Commands and results

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 is not installed; no GCC 14 run is claimed.

| Command | Result |
|---|---|
| `06-circle_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 7,256,308 checks per configuration; 13 assertion probes |
| `06-circle_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 80,626,052 checks |
| Mutation checks, optimized full suite per mutant | Caught: the normalized line formula, the `0.01` series switch, two segment root conditions, the radical-axis sign, the relation's tangent case |
| Warning sweep, every exact operation for `int8_t`, `int16_t`, `int`, `lng` plus every approximate operation, `-Wall -Wextra -Wconversion` | No header warnings (the tester has `-Wnarrowing` warnings from integer literals in braced `dpoint` initializers) |
| Two-translation-unit program mixing all four GE02 headers (optimized, ASan/UBSan); standalone header | PASS |
| `02-integration.py --sanitizers`, geometry quick run, `03-consistency.py` (with `--braces`) | PASS, no errors |
| `@reviewer`: 6.7M integer segments in 11 directions against an exact root-placement oracle; 100k radical-axis side checks | No mismatch |

```bash
python3 '96-Local Testing/03-Geometry/06-circle_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/06-circle_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[5-8]*.hpp '96-Local Testing/03-Geometry/'0[5-8]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Sources

The implementation is written from geometric identities (2026-09-27, extended 2026-10-07). No code or prose was copied, and references were compared only for equations and degeneracy contracts.

- [cp-algorithms, Circle-Line Intersection](https://cp-algorithms.com/geometry/circle-line-intersection.html) and [Circle-Circle Intersection](https://cp-algorithms.com/geometry/circle-circle-intersection.html): the projected foot plus half-chord and the radical-line reduction. The origin-only representation and the epsilon contract were replaced.
- [KACTL `CircleTangents.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/CircleTangents.h) (CC0): the signed second radius for inner and point tangents.
- [Stanford ICPC notebook 2015–16](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), printed pages 6–7 (saved as `95-Resources/06-stanford_notebook.pdf`): circle intersection and circumcenter routines. Its perturbed square root and missing concentric guard were not adopted.
- MathWorld [Circle-Circle Intersection](https://mathworld.wolfram.com/Circle-CircleIntersection.html) and [Incenter](https://mathworld.wolfram.com/Incenter.html): the cap decomposition and the side-length incenter formula.
- The 2026-10-07 additions (segment, power, radical axis, relation) are cited in [00-sources.md](00-sources.md#p008-completeness-sweep-fetched-2026-10-07); candidates not adopted (the Apollonian circle) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07).
- `OLD/Team Notebook/src/geometry/intersection.cpp`: its circle-line and circle-circle functions are superseded here; its other functions belong to P007 (`02-line_segment.hpp`, [evidence](02-line_segment.md)). The bytes are unchanged.

## Limits and handoffs

No claim of universally certified floating geometry is made. Other packages own adaptive and exact floating predicates (GE11), algebraic circle constructions including the Apollonian circle (row 24 / GE29), minimum enclosing circles (GE05), and circle unions and circle-polygon operations (GE10 and GE24).

## History

- 2026-09-27: original P008, full suite (seed 20260927, 146,311 checks) passed on g++ 16; segment, power and radical axis missing.
- 2026-10-07: re-audit, implemented `circleSegmentIntersectionApprox`, `powerApprox`, `radicalAxisApprox`, `circleRelation`; 7 findings fixed (series switch accuracy, non-axis tangency misclassification, non-axis tangency tests, tangent complexity line, tester braces, `circleRelation` domain assert); full suite and stress passed on g++ 16.
