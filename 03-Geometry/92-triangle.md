# P009 / GE23 — triangle

**Verified: P009 / GE23 is complete.** Implementation: [09-triangle.hpp](09-triangle.hpp). Prerequisites C01/P002, GE01/P007 and GE02/P008 are verified. The header is included by both geometry aggregates. All owned features and verification are complete; no package gap or continuation handoff remains.

## Features and contracts

Operations take O(1) time and O(1) storage, with fixed-size value results and no allocation, except that the exact centroid's `RationalPoint2` normalization takes O(log(C)) word operations, where C is at most 3*10^9. There is no size threshold, competing asymptotic algorithm or ISA specialization to benchmark for this Contest-profile header. Numerical comparisons against independent high-precision formulas verify stability without claiming a universal runtime advantage.

| API | Domain and result |
|---|---|
| `Triangle2(a,b,c)` | Integral `point` vertices with each coordinate in [-10^9,10^9]. Public fields must retain this invariant. Repeated and collinear vertices are allowed. |
| `area2()`, `areaApprox()` | Exact signed twice-area in `lll`, CCW positive; approximate nonnegative area obtained by conversion/division. |
| `centroid()` | Exact `RationalPoint2` vertex average, also defined for degenerate triangles. This degenerate extension differs from an area-centroid query on an arbitrary zero-area polygon. |
| `barycentric(p,out)` | Same coordinate bound on the integral query. False on zero area, with `out` unchanged; otherwise `Barycentric2` stores signed homogeneous numerators `w` relative to `(a,b,c)` and positive `d`, with `sum(w)==d`. It is not gcd-normalized. Negative weights represent points outside the triangle. |
| `Barycentric2::approx()`, `Triangle2::approx()` | Convert to normalized long-double weights or a `TriangleApprox`; these conversions do not promise exact downstream floating predicates. |
| `TriangleApprox(a,b,c)` | Finite `dpoint` vertices. All intermediates/results must be finite; nonzero divisors and relevant geometric distinctions must survive rounding, as in `CircleApprox`. Computed signs are used without a hidden epsilon or certified topology. |
| `area2()`, `area()`, `sides()`, `perimeter()`, `centroid()` | Signed twice-area, nonnegative area, side lengths opposite `(a,b,c)`, perimeter and vertex average. All remain defined for degeneracies. |
| `barycentric(p,out)` | Normalized signed-area coordinates; computed-zero area returns false with output unchanged. |
| `fromBarycentric(w,out)` | Finite homogeneous weights, including negative weights. Normalizes by their computed sum; zero sum returns false with output unchanged. A degenerate triangle can still reconstruct a point. Outputs may alias a vertex. |
| `circumcircle`, `incircle`, `circumcenter`, `incenter`, `circumradius`, `inradius`, `orthocenter` | Bool/output queries; computed-degenerate triangles return false without changing outputs. Circle wrappers reuse the GE02 constructors. Right, acute and obtuse triangles are supported; the orthocenter of a right triangle is its right-angle vertex. |
| `excircle(opposite,out)` | `opposite` is vertex index 0, 1 or 2. Returns its excircle, whose `.c` and `.r` are the excenter and exradius. False on computed-zero area. The positive semiperimeter gap must survive rounding. |
| `ninePointCircle(out)` | Center halfway between circumcenter and orthocenter, radius half the circumradius. False on computed-zero area. |
| `heronAreaApprox(a,b,c,out)` | Finite nonnegative sides in any order. An impossible triangle returns false with output unchanged; triangle-inequality equality, including collapsed/zero-side cases, returns true and area zero. The four Kahan factors must be finite; a positive result must be representable. The quartic product need not fit the floating range. |

The floating contracts expose ordinary rounding and ill-conditioning. In particular, a thin triangle can have side lengths that round to an exact equality even though a determinant is nonzero; a side-only query cannot recover that lost information. Heron's method improves evaluation of the supplied lengths and does not certify the original coordinates. Floating overflow or erased nonzero divisors outside the stated domain are precondition violations, not no-answer results. All failed bool queries preserve their output.

## Correctness and arithmetic bounds

For integral vertices/query points, each difference has magnitude at most 2*10^9; each determinant has magnitude at most 8*10^18. `cross` promotes before multiplication, so these fit `lll`. Centroid numerators have magnitude at most 3*10^9. The barycentric numerators are the three oriented subtriangle areas; their sum equals the full determinant, and their weighted coordinate sum equals the full determinant times the query point. Flipping all signs when the full determinant is negative gives a positive denominator without changing the point.

For floating barycentrics, the same signed-area identities give the normalized weights. Affine reconstruction translates to the first vertex before applying the two remaining normalized weights. The centroid is the vertex average and therefore the common point of the medians for nondegenerate triangles. GE02's circumcircle solves the equal-distance equations, and its incircle uses side-length barycentric weights and twice-area/perimeter. The orthocenter identity `H=A+B+C-2*O` satisfies each altitude equation; the implementation translates by A to avoid unnecessary cancellation from a common offset. The excircle opposite A uses weights `(-a,b,c)` and radius `2*area/(-a+b+c)`; cyclic permutation covers all three. The nine-point center is `(O+H)/2`, with radius `R/2`.

Heron sorts `a>=b>=c`, then tests `c-(a-b)`. A negative value is an impossible triangle; zero is degenerate. For positive values it evaluates Kahan's grouping:

```text
16*area^2 = (a+(b+c)) * (c-(a-b)) * (c+(a-b)) * (a+(b-c)).
```

Each factor is split by `frexp` into a mantissa in [1/2,1) and an exponent. Multiplying four mantissas stays in [1/16,1); exponents add in an `int`. An odd exponent is made even before the square root, and `scalbn` applies the remaining power of two and division by four. This avoids an overflowing or underflowing quartic product even when the area is representable. It also avoids normalizing all sides by the largest one, which can erase a tiny third side. The Kahan parentheses are intentional and require ordinary floating semantics, without unsafe reassociation flags.

## Verification

Entry: [09-triangle_tester.py](<../96-Local Testing/03-Geometry/09-triangle_tester.py>), with its C++ feature suite and the shared geometry runner. Python uses only the standard library. Failures remain active under NDEBUG and print seed, reproducing input/operation and expected/actual results; the shared runner supplies command, configuration and crash/timeout details.

| Feature or edge class | Independent verification |
|---|---|
| Integral signed/unsigned area | Bounded exhaustive grids and Python arbitrary-integer expanded shoelace determinants; both orientations, repeated/collinear vertices and coordinate-bound corners |
| Exact centroid | Rational identities and independently normalized Python `Fraction` answers, including degenerate triangles |
| Exact barycentrics and approximation | Independent subdeterminants; positive denominator, exact partition of unity and weighted reconstruction; outside/vertex points and unchanged output on degeneracy |
| Approximate area/sides/perimeter/centroid | Known 3-4-5 values, opposite-vertex ordering, signed orientation, scaling and independent mean |
| Approximate barycentrics and reconstruction | Independent exact rational weights across the grid/fixture corpus, vertex Kronecker weights, random round trips and affine invariance, signed/negated homogeneous weights, zero sums, reconstruction on degenerate triangles and output aliasing |
| Circumcircle, circumcenter and circumradius | Known right-triangle values, equal vertex distances, `4*area*R=a*b*c`, all six vertex permutations and similarities |
| Incircle, incenter and inradius | Known values, interior orientation, equal side-line distances, `r*perimeter=2*area`, permutations and similarities |
| Orthocenter | Independent altitude perpendicularity, right-triangle vertex, Euler-line identity and conditioned thin-isosceles formulas |
| Excircles, excenters and exradii | All three known 3-4-5 answers, equal distances to all three side lines, signed halfplane placement and semiperimeter-gap identity |
| Nine-point circle | Side midpoints, altitude feet and vertex/orthocenter midpoints on the circle; center and radius identities |
| Stable Heron | Independent high-precision Decimal ordinary-semiperimeter formula; side permutations, impossible/degenerate/zero sides, nearly degenerate dyadic sides, huge/tiny scales and mixed exponents; direct subnormal regression |
| State, absence and preconditions | Defaults, copy/move, legal public-field mutation, vertex/radius-output aliasing, every bool no-answer preserving output; checked-build assertion probes for coordinate bounds, nonfinite points/weights, bad excircle index, rounded-zero excircle gap, invalid side domains, Heron factor overflow and unrepresentable areas |

Approximate construction comparisons allow a scaled absolute/relative residual of 4e-13; conditioned equations and normalized values are used for very thin/small/large triangles. The independently formed exradius identity additionally allows `64*epsilon*exradius*perimeter` for cancellation in its ordinary semiperimeter-minus-side reference; independent side-distance and halfplane witnesses remain checked. Heron comparisons use a separate purely relative allowance of 8e-16 against up to 11,000-digit Decimal calculations, including ratios that would erase a small side under naive normalization. These are test tolerances, not a certified global error bound. Exact answers are compared as integers/rationals before any approximate conversion.

Final verification on 2026-09-27 used Linux x86-64, `g++ (GCC) 16.2.1 20260810`, GNU++20 and seed 20260927:

```bash
python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

The full triangle suite passed **560,820 non-removable checks per configuration**: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Each build covered **18,225** exhaustive exact/floating grid cases, **2,009** Python arbitrary-integer/Fraction fixtures, **369** Decimal Heron fixtures and **2,000** random construction/permutation/similarity cases, plus fixed regressions. All **16** assertion probes passed in the checked build. Leak detection remained enabled. An initial sanitizer attempt encountered LeakSanitizer's sandbox/ptrace limitation; the final complete three-configuration run was approved outside the sandbox and passed.

During verification, the independently evaluated `exradius*(semiperimeter-side)` identity needed the explicit cancellation allowance documented above; direct line-distance and halfplane witnesses passed. This corrected the reference comparison, with no implementation change. Two independent read-only formula/domain/API reviews also found no correctness blocker.

Integration passed all **74** current standalone/aggregate headers, scalar/available-AVX2 multiple-translation-unit linkage and LOCAL/non-LOCAL Workspace compilation. The repository consistency check passed with zero errors and preserved archive hashes. The shared runner automatically discovers the new suite in all modes without infrastructure changes.

| Mode | Actual configured corpus |
|---|---|
| quick | 6,561 grid cases, 109 exact fixtures, 79 Decimal fixtures, 100 random constructions; optimized and checked builds |
| full | The completed coverage above; all three configurations |
| stress | 390,625 grid cases, 12,009 exact fixtures, 1,869 Decimal fixtures, 12,000 random constructions; all three configurations |

An earlier quick run passed during development; the final recorded verification is the full run above. Stress is exposed for extended runs and was not required or run for this completion. No online submissions or comparative timing claims were made.

## Read sources and provenance

Read on 2026-09-27. The implementation is independently written from geometric and arithmetic identities; no external code was copied. References establish the reviewed formulas and scope, separately from executable verification.

- [William Kahan, *Miscalculating Area and Angles of a Needle-like Triangle*](https://people.eecs.berkeley.edu/~wkahan/Triangle.pdf), revision September 4, 2014, 7:24am, title/abstract and sections 1–4 (printed pages 2–4): naive Heron cancellation, decreasing-side ordering, triangle-inequality test and the exact parenthesized four-factor formula. Its error discussion excludes overflow/underflow; exponent-separated multiplication is the implementation's independently added range extension.
- [Eric W. Weisstein, MathWorld: Barycentric Coordinates](https://mathworld.wolfram.com/BarycentricCoordinates.html): normalization, signed-area interpretation and center-weight table; [Triangle Centroid](https://mathworld.wolfram.com/TriangleCentroid.html): `(1,1,1)` barycentrics, medians and Euler-line relationships; [Orthocenter](https://mathworld.wolfram.com/Orthocenter.html): altitude definition and right-triangle behavior.
- MathWorld [Incenter](https://mathworld.wolfram.com/Incenter.html), equation (2), and [Inradius](https://mathworld.wolfram.com/Inradius.html), equations (2)–(7): side weights and area/semiperimeter identity. [Exradius](https://mathworld.wolfram.com/Exradius.html), equations (1)–(6): area/semiperimeter-gap, reciprocal and product identities.
- MathWorld [Nine-Point Center](https://mathworld.wolfram.com/Nine-PointCenter.html), opening definition, and [Nine-Point Circle](https://mathworld.wolfram.com/Nine-PointCircle.html), opening and equation (3): midpoint of O/H, half circumradius and side/altitude/Euler midpoint incidence.
- Preserved `OLD/Team Notebook/main.tex`, lines 232–234, contains elementary Heron's formula. Its functionality is covered by the stable side-length query. The scoped legacy source search found no triangle implementation requiring migration; original archive bytes remain unchanged.

## Scope and remaining owners

The Basic triangle row covers planar single-triangle measurements, coordinates and the named center/circle constructions above. Three excircles and the nine-point circle are explicit additions discovered during scope review. Arbitrary catalogs of named triangle centers are not implied by this API. Triangle/polygon clipping, triangulation, geometric medians, 3D line/triangle intersections, spherical triangles, algebraic constructions and certified predicates retain their existing later inventory owners. No online submission is part of this package.
