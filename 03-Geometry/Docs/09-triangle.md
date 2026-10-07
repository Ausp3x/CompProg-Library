# 09-triangle.hpp — evidence

Implementation: [09-triangle.hpp](../09-triangle.hpp). Prerequisites C01/P002, GE01/P007 and GE02/P008 are verified. Both geometry aggregates include the header. Owned by package P009 / GE23. Every operation in the row has a test with an independent oracle.

## Contracts

### Common numeric policy

- Every operation takes O(1) time and O(1) storage and allocates nothing. The one exception is `Triangle2::centroid`, whose `RationalPoint2` gcd normalization takes O(log(C)) word operations, C <= 3 * 10^9. There is no size threshold, competing asymptotic algorithm or ISA path, so this contest-profile header has nothing to benchmark.
- Exact APIs (`Triangle2`, `Barycentric2`) use integral `point` coordinates in [-10^9, 10^9]; determinants widen to `lll` before multiplication. Public fields must keep that domain.
- Approximate APIs (`TriangleApprox`, `heronAreaApprox`) follow the `CircleApprox` policy of [00-notes.md](00-notes.md#ge02-numeric-policy-p008). Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding. Signs are computed with no hidden epsilon and no certified topology. Overflow, or a nonzero divisor erased by rounding, is a precondition violation, not a no-answer result.
- Every failed bool query leaves its output unchanged, even when that output aliases a vertex.

### Barycentric2

Exact homogeneous weights `w` relative to `(a, b, c)` with denominator `d > 0` and `sum(w) == d`. The weights are not gcd-normalized. Negative weights denote points outside the triangle. The default `{1, 0, 0} / 1` is vertex `a`. `approx()` returns `w[i] / d` in `long double` and promises no exact downstream predicate.

### TriangleApprox

- Constructor: finite `dpoint` vertices, checked; the public vertices keep that domain.
- `area2()` is the signed twice-area, CCW positive. `area()` is its absolute half. `sides()` gives the side lengths opposite `(a, b, c)` in that order, and `perimeter()` their sum. `centroid()` is the vertex average. All five are defined for collinear and collapsed triangles.
- `barycentric(p, out)`: normalized signed-area coordinates of a finite `p`. Returns false when the computed area is zero.
- `fromBarycentric(w, out)`: finite homogeneous weights, possibly negative, normalized by their computed sum. Returns false when the sum is zero (a point at infinity). A degenerate triangle still reconstructs a point.
- `circumcircle`, `incircle`, `circumcenter`, `incenter`, `circumradius`, `inradius`, `orthocenter`: wrappers over the GE02 constructors `circumcircleApprox` and `incircleApprox`. They return false on a computed-degenerate triangle. Right, acute and obtuse triangles are supported; the orthocenter of a right triangle is its right-angle vertex.
- `excircle(opposite, out)`: `opposite` is the vertex index 0, 1 or 2 (asserted). `out.c` is the excenter and `out.r` the exradius. Returns false when the twice-area computed from the opposite vertex, `cross(u, v)`, is zero. No step cancels beyond `cross` and `dot` themselves. When these are exact (for example integer coordinates with differences below 2^31, possibly scaled by a power of two), the exradius is accurate to a few ulp, even for thin triangles with huge answers. Otherwise rounding in `cross` gives a relative error of O(eps * L^2 / |area2|), where `L` is the longest side, which is the problem's own conditioning. The center has absolute error O(eps * (|o| + r * L^2 / |area2|)). The intermediate `k = r / |cross(u, v)|` must be finite. It overflows only when |area2| < about 1e-2466 * L^2, and then the `CircleApprox` constructor asserts.
- `ninePointCircle(out)`: center halfway between the circumcenter and orthocenter, radius `R / 2`. Returns false on a computed-degenerate triangle.
- A thin triangle can have side lengths that round to an exact equality even though its determinant is nonzero. A side-only query cannot recover that lost information.

### Triangle2

- Constructor and `barycentric` query: integral coordinates in [-10^9, 10^9], asserted. Repeated and collinear vertices are allowed.
- `area2()` is the exact signed twice-area in `lll`, CCW positive. `areaApprox()` is its absolute half, converted to `long double`.
- `centroid()` is the exact `RationalPoint2` vertex average, also defined for degenerate triangles. This differs from an area-centroid query on a zero-area polygon.
- `angleKind()` returns `1` for an acute, `0` for a right and `-1` for an obtuse triangle. It is the sign of the smallest of the three vertex dot products, each at most 8 * 10^18 in magnitude. Zero area is a precondition violation (asserted), since a degenerate triangle has no angles.
- `barycentric(p, out)` returns false on zero area. Otherwise it returns the signed homogeneous `Barycentric2`; when `area2` is negative it flips all signs so that `d` is positive.
- `approx()` converts the triangle to a `TriangleApprox`.

### heronAreaApprox

`heronAreaApprox(a, b, c, out)` takes finite nonnegative sides in any order. An impossible triangle returns false with `out` unchanged. Triangle-inequality equality, including collapsed and zero-side cases, returns true with area zero. The four Kahan factors must be finite, and a positive result must be representable; the quartic product itself need not fit the floating range. Heron improves the evaluation of the supplied lengths but does not certify the original coordinates.

### Correctness and arithmetic bounds

Integral differences have magnitude at most 2 * 10^9, so every determinant and dot product is at most 8 * 10^18 and fits `lll`. Centroid numerators are at most 3 * 10^9. The barycentric numerators are the three oriented subtriangle areas. Their sum equals the full determinant, and their coordinate-weighted sum equals the full determinant times the query point. `angleKind` uses the fact that the angle at a vertex is acute, right or obtuse exactly when the dot product of its two edge vectors is positive, zero or negative. At most one angle is non-acute, so the smallest dot product decides the triangle.

The floating barycentrics use the same signed-area identities, and the affine reconstruction translates to the first vertex before applying the other two weights. GE02's circumcircle solves the equal-distance equations. Its incircle uses side-length weights and the radius twice-area / perimeter. The orthocenter identity `H = A + B + C - 2 * O` satisfies each altitude equation; the code translates by `A` to avoid cancellation from a common offset. The nine-point center is `(O + H) / 2` with radius `R / 2`.

Excircle opposite `o`: let `u` and `v` be the edges to the other two vertices, `x = |v|`, `y = |u|`, `z = |v - u|`, `s = cross(u, v)` and `w = dot(u, v)`. The excenter is `o + (u * x + v * y) / (x + y - z)` with radius `|s| / (x + y - z)` (weights `(-z, x, y)`). Since `(x + y)^2 - z^2 = 2 * (x * y + w)`, the gap is `x + y - z = 2 * g / (x + y + z)` with `g = x * y + w`. When `w >= 0`, `g` is a sum of nonnegative terms. When `w < 0`, `x * y + w = s^2 / (x * y - w)`, from `x^2 * y^2 = w^2 + s^2`. That form is evaluated as `s / (x * y - w) * s`, so neither branch cancels and no intermediate exceeds the size of `area2`. The code multiplies by `k = (x + y + z) / (2 * g)` instead of dividing by the gap. A direct `min(x, y) - (z - max(x, y))` would lose about `log2(L^2 / gap)` bits. This form stays within 2.5 ulp of `(sqrt(1 + h^2) + 1) / h` from `h = 1e-1` down to `1e-2000`.

Heron sorts `a >= b >= c`, then tests `c - (a - b)`: negative is impossible and zero is degenerate. Otherwise it evaluates Kahan's grouping, `16 * area^2 = (a + (b + c)) * (c - (a - b)) * (c + (a - b)) * (a + (b - c))`. `frexp` splits each factor into a mantissa in [1/2, 1) and an exponent. The four mantissas multiply to a value in [1/16, 1), and the exponents add in an `int`. An odd exponent is made even before the square root, and `scalbn` applies the remaining power of two and the division by four. This avoids an overflowing or underflowing quartic product, and it never normalizes by the largest side, which could erase a tiny third side. The Kahan parentheses require ordinary floating semantics with no unsafe reassociation flags.

## Feature-to-test map

Entry: [09-triangle_tester.py](<../../96-Local Testing/03-Geometry/09-triangle_tester.py>), with its C++ feature suite and the shared geometry runner. Python uses only the standard library. Failures stay active under `NDEBUG` and print the seed, the reproducing input and operation, and the expected and actual values. The runner adds command, configuration and crash or timeout details.

| Feature or edge class | Independent verification |
|---|---|
| `Triangle2` area2, areaApprox | Bounded exhaustive grids and Python bigint expanded shoelace determinants; both orientations, repeated and collinear vertices, coordinate-bound corners |
| `Triangle2::centroid` | Rational identities and independently normalized Python `Fraction` answers, including degenerate triangles |
| `Triangle2::angleKind` | Law of cosines on sorted squared sides (C++ `lll` over the exhaustive grid, Python bigints over the fixtures); right, acute and obtuse examples at the coordinate bound; degenerate-input assertion probe |
| `Triangle2::barycentric`, `Barycentric2` w, d, approx | Independent subdeterminants; positive denominator, exact partition of unity and weighted reconstruction; outside and vertex points; unchanged output on degeneracy |
| `Triangle2::approx`, `TriangleApprox` area2, area, sides, perimeter, centroid | Exact agreement on the binary integer corpus, the 3-4-5 values, opposite-vertex side order, orientation, scaling and the independent mean |
| `TriangleApprox` barycentric, fromBarycentric | Exact rational weights across the grid and fixture corpus, vertex Kronecker weights, random round trips and affine invariance, signed and negated weights, zero sums, degenerate reconstruction and output aliasing |
| circumcircle, circumcenter, circumradius | Known right-triangle values, equal vertex distances, `4 * area * R = a * b * c`, all six vertex permutations and similarities |
| incircle, incenter, inradius | Known values, interior orientation, equal side-line distances, `r * perimeter = 2 * area`, permutations and similarities |
| orthocenter | Altitude perpendicularity, right-triangle vertex, Euler-line identity and conditioned thin-isosceles formulas |
| excircle | Python Decimal reference from the direct gap at 200 digits or more: exradius within 64 eps relative, excenter within the conditioning bound above. Thin obtuse closed form `(sqrt(1 + h^2) + 1) / h` within 8 eps, for `h` from `1e-1` to `1e-2000`. Product identity `r_a * r_b * r_c = r * s^2` and sum identity `r_a + r_b + r_c = 4 * R + r` within 512 eps relative; equal distances to all three side lines; halfplane placement; the three 3-4-5 answers |
| ninePointCircle | Side midpoints, altitude feet and vertex-orthocenter midpoints on the circle; center and radius identities |
| heronAreaApprox | Exact `Fraction` classification and Heron factors with a Decimal square root; side permutations, impossible, degenerate and zero sides, exactly degenerate dyadic sides, nearly degenerate sides, huge and tiny scales, mixed exponents; direct subnormal regressions |
| State and preconditions | Defaults, copy and move, public-field mutation, vertex and radius-output aliasing, every bool no-answer preserving output; 17 checked-build assertion probes for coordinate bounds, degenerate `angleKind`, nonfinite points and weights, bad excircle indices, unrepresentable exradius, invalid sides, Heron factor overflow and unrepresentable areas |

Approximate constructions without a tighter oracle use a scaled absolute or relative residual of 4e-13, with conditioned equations for very thin, small or large triangles. Heron uses a purely relative 8e-16 against Decimal references of up to 11,000 digits. These are test tolerances, not certified error bounds. Exact answers are compared as integers or rationals before any conversion.

| Mode | Corpus |
|---|---|
| quick | 6,561 grid cases, 109 exact, 79 Heron and about 26 excircle fixtures, 100 random constructions; optimized and checked builds |
| full | 18,225 grid cases, 2,009 exact, 369 Heron and 406 excircle fixtures, 2,000 random constructions; optimized, checked and ASan/UBSan builds |
| stress | 390,625 grid cases, 12,009 exact, 1,869 Heron and 2,406 excircle fixtures, 12,000 random constructions |

## Commands and results

2026-10-07, Linux x86-64, GCC 16.2.1 and the floor compiler GCC 14.4.1 20260915 (`/usr/bin/g++-14`), Python 3.14, GNU++20.

| Command | Result |
|---|---|
| Full, seed 20260927 (optimized, checked, ASan/UBSan) | PASS, 576,007 checks per configuration, 17 assertion probes |
| Full, seed 20260927, `CXX=g++-14` | PASS, 576,007 checks per configuration, 17 assertion probes |
| Full, seeds 1 to 6, optimized | PASS |
| Stress, seed 42, optimized | PASS, 8,240,487 checks |
| Quick, seed 1 | PASS, 110,055 checks in two configurations |
| Two-translation-unit program, optimized and ASan/UBSan; header alone under `-Wall -Wextra -Wconversion` | PASS, no warnings |
| Two-translation-unit program, `g++-14 -Wall -Wextra -Wconversion` | Builds without warnings, runs correctly |
| Mutation: the direct gap formula | Fails the Decimal fixtures, the product identity alone and the closed-form loop |
| `03-consistency.py --braces` on header and tester | Clean |
| `@reviewer`: full seeds 1, 7, 12345 and quick seed 3 | PASS |

```bash
python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode full --seed 1 --configuration optimized   # likewise seeds 2-6
python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/03-Geometry/09-triangle_tester.py' --mode quick --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/09-triangle.hpp '96-Local Testing/03-Geometry/09-triangle_tester.cpp'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Read on 2026-09-27; the completeness sweep was added on 2026-10-07. The implementation is written from geometric and arithmetic identities, and no external code was copied. Completeness-sweep candidates not adopted are in [00-notes.md](00-notes.md#p009-completeness-decisions-2026-10-07).

- [William Kahan, *Miscalculating Area and Angles of a Needle-like Triangle*](https://people.eecs.berkeley.edu/~wkahan/Triangle.pdf), revision of September 4, 2014, sections 1–4 (printed pages 2–4): naive Heron cancellation, decreasing-side ordering, the triangle-inequality test and the parenthesized four-factor formula. Its error analysis excludes overflow and underflow; the exponent-separated product is this implementation's own extension.
- MathWorld [Barycentric Coordinates](https://mathworld.wolfram.com/BarycentricCoordinates.html), [Triangle Centroid](https://mathworld.wolfram.com/TriangleCentroid.html), [Orthocenter](https://mathworld.wolfram.com/Orthocenter.html), [Incenter](https://mathworld.wolfram.com/Incenter.html) eq. (2), [Inradius](https://mathworld.wolfram.com/Inradius.html) eqs. (2)–(7), [Exradius](https://mathworld.wolfram.com/Exradius.html) eqs. (1)–(6) (area / semiperimeter gap, product and sum identities), [Nine-Point Center](https://mathworld.wolfram.com/Nine-PointCenter.html) and [Nine-Point Circle](https://mathworld.wolfram.com/Nine-PointCircle.html) eq. (3).
- CGAL Kernel `angle` (see [00-sources.md](00-sources.md)): acute, right and obtuse classification by the sign of a dot product.
- Preserved `OLD/Team Notebook/main.tex`, lines 232–234, has the elementary Heron formula; `heronAreaApprox` covers it. No other legacy triangle code needs migration, and the archive bytes are unchanged.

## Limits and handoffs

The row covers planar single-triangle measurements, coordinates, the angle kind and the named centers and circles above. Arbitrary catalogs of named triangle centers are not implied. Triangle and polygon clipping, triangulation, geometric medians and the Fermat point, 3D line-triangle intersection, spherical triangles and certified predicates belong to their later inventory rows.

## History

- 2026-09-27: original P009, full suite (seed 20260927, 560,820 checks, 16 probes) passed on g++ 16.
- 2026-10-07: re-audit, added `angleKind`; 4 findings fixed (excircle gap cancellation, Heron oracle misclassifying dyadic degenerate sides, `; }` closings, excircle contract conditioning); full suite passed on g++ and g++-14.
