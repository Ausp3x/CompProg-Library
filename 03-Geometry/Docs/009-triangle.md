# 009-triangle.hpp — Evidence

Implementation: [009-triangle.hpp](../009-triangle.hpp), owned by package P009 / GE23 and included by both geometry aggregates. Its prerequisites C01/P002, C05 and C06/P019 and GE02/P008 are verified; GE01/P007 is in progress, and the row 01 and 02 operations this header calls are verified there. Every operation in the row has a test with an independent oracle.

## Contracts

### Scope

The row covers planar single-triangle measurements, coordinates, orientation and point location, vertex angles, affine images, the angle kind and the named centers and circles below, exact for the circumcenter, squared circumradius and orthocenter of an integral triangle. It has no approximate `locate`, since a boundary is not decidable from rounded coordinates ([00-notes.md](00-notes.md#p009-completeness-decisions-2026-10-07)). Arbitrary catalogs of named triangle centers are not implied. Triangle and polygon clipping, triangulation, geometric medians and the Fermat point, 3D line-triangle intersection, spherical triangles and certified predicates belong to their own inventory rows.

### Common Numeric Policy

- Every operation takes O(1) time and storage, except that the rational results of `Triangle2` (`centroid`, `circumcenter`, `orthocenter`, `altitudes2`, `altitudeFeet`, `medians2`, `medianFeet`) normalize by gcd in O(log(C)) word operations, C <= 6.4 * 10^37 (the `altitudes2` numerators; 1.5 * 10^29 for the points), and `circumradius2`, and `altitudes2` and `medians2` with a big-integer `Q`, compute with a fixed number of big integers of O(1) words, which allocate. No other operation allocates. With no size threshold, competing algorithm or ISA path, the header has nothing to benchmark.
- Exact APIs (`Triangle2`, `Barycentric2`) use integral `point` coordinates in [-10^9, 10^9]; determinants widen to `lll` before multiplication. Public fields must keep that domain.
- Approximate APIs (`TriangleApprox`, `heronAreaApprox`) follow the `CircleApprox` policy of [00-notes.md](00-notes.md#ge02-numeric-policy-p008). Overflow, or a nonzero divisor erased by rounding, is a precondition violation, not a no-answer result.
- A failed bool query leaves its output unchanged, even when the output aliases a vertex.

### Barycentric2

Exact homogeneous weights `w` relative to `(a, b, c)` with denominator `d > 0` and `sum(w) == d`. The weights are not gcd-normalized. Negative weights denote points outside the triangle. The default `{1, 0, 0} / 1` is vertex `a`. `approx()` returns `w[i] / d` in `long double` and promises no exact downstream predicate.

### TriangleApprox

- Constructor: finite `dpoint` vertices, checked; the public vertices keep that domain.
- `area2()` is the signed twice-area, CCW positive. `area()` is its absolute half. `sides()` gives the side lengths opposite `(a, b, c)` in that order, and `perimeter()` their sum. `centroid()` is the vertex average. All five are defined for collinear and collapsed triangles.
- `angles()` gives the vertex angles at `(a, b, c)` in [0, pi], each `angleApprox` (`001-point.hpp`, `atan2(|cross|, dot)`) of the two edges leaving that vertex, so no angle loses accuracy near 0 or pi as an `acos` would. The three vertices must be distinct (asserted by `angleApprox`); collinear distinct vertices give 0, 0 and pi. The sum is pi only up to rounding.
- `altitudes()` gives the distance from each vertex `(a, b, c)` to the line through the other two, `|cross(e, p - u)| / hypot(e)` for the vertex `p`, the next vertex `u` and the side `e` from `u` to the third vertex; it is 0 for collinear vertices. `altitudeFeet()` gives the matching feet, `u + d * dot(p - u, d)` with the unit direction `d = e / hypot(e)`. Neither forms a squared length, so a side whose square is subnormal or overflows keeps its accuracy (user decision 2026-11-27, against row 02's `signedLineDistanceApprox` and `projectLineApprox`, which divide by `norm2(e)`; follow-up on P007). Both assert a positive finite side length, so the vertices must be distinct, and a finite `cross` or `dot`; overflow of a vertex difference or of the result is a precondition violation (policy above).
- `medians()` gives the distance from each vertex to the midpoint of its opposite side, half the `hypot` length of the sum of the two edges leaving it, so no side length cancels as in Apollonius' formula. `medianFeet()` gives those midpoints, `(b + c) / 2` and its rotations, rounded once. Both are defined for every triangle, repeated vertices included.
- `transform(f)` returns the triangle whose vertices are `f.applyPoint` of `(a, b, c)`, for an `Affine2Approx` from `008-coordinate_transform.hpp`; the vertex order is kept, so a reflection flips the sign of `area2`.
- `circumcircleApprox(out)` and `incircleApprox(out)` are member wrappers that return exactly `circumcircleApprox(a, b, c, out)` and `incircleApprox(a, b, c, out)` of `006-circle.hpp`, false with `out` unchanged included (user decision 2026-11-14, `00-Guidelines/23-Sweep Ledgers/omissions.md`). `orthocenter` and `ninePointCircle` go through the first.
- `barycentric(p, out)`: normalized signed-area coordinates of a finite `p`. Returns false when the computed area is zero.
- `fromBarycentric(w, out)`: finite homogeneous weights, possibly negative, normalized by their computed sum. Returns false when the sum is zero (a point at infinity). A degenerate triangle still reconstructs a point.
- `orthocenter(out)`: built from the GE02 circumcenter (`circumcircleApprox`). Returns false on a computed-degenerate triangle. Right, acute and obtuse triangles are supported; the orthocenter of a right triangle is its right-angle vertex.
- `excircle(opposite, out)`: `opposite` is the vertex index 0, 1 or 2 (asserted). `out.c` is the excenter and `out.r` the exradius. Returns false when the twice-area computed from the opposite vertex, `cross(u, v)`, is zero. No step cancels beyond `cross` and `dot` themselves. When these are exact (for example integer coordinates with differences below 2^31, possibly scaled by a power of two), the exradius is accurate to a few ulp, even for thin triangles with huge answers. Otherwise rounding in `cross` gives a relative error of O(eps * L^2 / |area2|), where `L` is the longest side, which is the problem's own conditioning. The center has absolute error O(eps * (|o| + r * L^2 / |area2|)). Two intermediates must be representable, both preconditions: `g = x * y + w` (about `area2^2 / L^2` for an obtuse angle at `o`) must be a normal number, asserted (user decision 2026-11-16: a subnormal `g` has lost precision and would give a finite but inaccurate result), which fails only when `|area2|` is below about `1.8e-2466 * L` or the sides are near `1e2466`; and `k = (x + y + z) / (2 * g)` must be finite, which fails only when `|area2|` is below about `1e-2466 * L^1.5`, caught by the `CircleApprox` constructor assertion.
- Pinned rounding (user decision 2026-11-15): when the angle at the opposite vertex is exactly right (`dot(u, v) == 0`), the gap takes the `x * y + w` branch, so the result is bit-reproducible there.
- `ninePointCircle(out)`: center halfway between the circumcenter and orthocenter, radius `R / 2`. Returns false on a computed-degenerate triangle.
- A thin triangle can have side lengths that round to an exact equality even though its determinant is nonzero. A side-only query cannot recover that lost information.

### Triangle2

- Constructor and `barycentric` query: integral coordinates in [-10^9, 10^9], asserted. Repeated and collinear vertices are allowed.
- `area2()` is the exact signed twice-area in `lll`, CCW positive. `areaApprox()` is its absolute half, converted to `long double`.
- `orient()` is the sign of `area2()`: `1` counterclockwise, `0` degenerate, `-1` clockwise.
- `locate(p)` classifies an integral `p` in [-10^9, 10^9] (asserted) against the closed triangle: `-1` interior, `0` boundary (edges and vertices), `1` outside. A degenerate triangle has no interior: `0` on any of its three edges as segments, else `1`.
- `circumcenter()` is the exact circumcenter as a gcd-normalized `RationalPoint2`, `a + (|v|^2 * perp(u) - |u|^2 * perp(v)) / (2 * cross(u, v))` with `u = b - a`, `v = c - a` (the formula of row 14's `EnclosingCircle::center`). Every numerator stays below 5 * 10^28 and the denominator below 1.6 * 10^19, inside `lll`.
- `circumradius2<I>()` is the exact squared circumradius `|ab|^2 * |ac|^2 * |bc|^2 / (4 * area2^2)` as a `Rational<I>`, `I` a big integer type defaulting to `InfIntMini` (`03-cpp.md`, Core type parameters); `Rational<InfInt>` is the full alternative. The numerator reaches about 5 * 10^56, beyond `lll`. A float conversion needs `InfInt` while `Rational<InfIntMini>::toLongDouble` does not compile ([Limits and Handoffs](#limits-and-handoffs)).
- `orthocenter()` is the exact orthocenter `a + b + c - 2 * circumcenter()` over the circumcenter's denominator; numerators stay below 1.5 * 10^29.
- `circumcenter`, `circumradius2` and `orthocenter` assert a nonzero area, like `angleKind`.
- `altitudes2<Q>()` is the exact squared distance from each vertex `(a, b, c)` to the line through the other two, `area2^2 / |side|^2`, as `Q`, defaulting to `Rational<lll>` like `linearDistance2` (`002-line_segment.hpp`), which computes it on the vertex as a one-point segment; `Q` is `Rational<lll>` or a big-integer `Rational`, and `Rational<lng>`, which would truncate, is rejected at compile time (also in `medians2`). Numerators stay below 6.4 * 10^37, inside `lll`; collinear vertices give 0. `altitudeFeet()` is the exact gcd-normalized foot `u + e * dot(p - u, e) / |e|^2` of each vertex `p` on the line through the other two `u` and `v`, `e = v - u`, with numerators below 2.4 * 10^28 and denominator `|e|^2 <= 8 * 10^18`; a vertex on that line is its own foot. Both assert distinct vertices.
- `medians2<Q>()` is the exact squared median `|(b - a) + (c - a)|^2 / 4` from each vertex, with numerators at most 3.2 * 10^19, and `medianFeet()` the exact side midpoints `(b + c) / 2` and their rotations. Both are defined for every triangle, repeated vertices included.
- `centroid()` is the exact `RationalPoint2` vertex average, also defined for degenerate triangles. This differs from an area-centroid query on a zero-area polygon.
- `angleKind()` returns `1` for an acute, `0` for a right and `-1` for an obtuse triangle. It is the sign of the smallest of the three vertex dot products, each at most 8 * 10^18 in magnitude. Zero area is a precondition violation (asserted), since a degenerate triangle has no angles.
- `barycentric(p, out)` returns false on zero area. Otherwise it returns the signed homogeneous `Barycentric2`; when `area2` is negative it flips all signs so that `d` is positive.
- `approx()` converts the triangle to a `TriangleApprox`.

### heronAreaApprox

`heronAreaApprox(a, b, c, out)` takes finite nonnegative sides in any order. An impossible triangle returns false with `out` unchanged. Triangle-inequality equality, including collapsed and zero-side cases, returns true with area zero. The four Kahan factors must be finite, and a positive result must be representable; the quartic product itself need not fit the floating range. Heron improves the evaluation of the supplied lengths but does not certify the original coordinates.

### Correctness and Arithmetic Bounds

Integral differences have magnitude at most 2 * 10^9, so every determinant and dot product is at most 8 * 10^18 and fits `lll`. Centroid numerators are at most 3 * 10^9. The barycentric numerators are the three oriented subtriangle areas. Their sum equals the full determinant, and their coordinate-weighted sum equals the full determinant times the query point. The exact circumcenter solves `2 * dot(u, x) = |u|^2`, `2 * dot(v, x) = |v|^2` for `x = O - a` by Cramer's rule, whose determinant is `4 * cross(u, v)`; the squared radius is the law of sines `R = |bc| / (2 sin A)` with `sin A = |cross(u, v)| / (|u| |v|)`. `locate` multiplies each edge orientation `orient(a, b, p)`, `orient(b, c, p)`, `orient(c, a, p)` by the triangle's own sign `o`. For `o != 0` the closed triangle is the intersection of the three closed halfplanes left of its edges after that normalization, so a negative product means outside, all positive means interior, and otherwise `p` lies on an edge line inside the other two halfplanes, which is that edge. For `o == 0` the closed point set is the convex hull of collinear points, which the union of any two of its three edges covers (the middle point splits it, or one edge spans it). `angleKind` uses the fact that the angle at a vertex is acute, right or obtuse exactly when the dot product of its two edge vectors is positive, zero or negative. At most one angle is non-acute, so the smallest dot product decides the triangle.

The floating barycentrics use the same signed-area identities, and the affine reconstruction translates to the first vertex before applying the other two weights. GE02's `circumcircleApprox` solves the equal-distance equations. The orthocenter identity `H = A + B + C - 2 * O` satisfies each altitude equation; the code translates by `A` to avoid cancellation from a common offset. The nine-point center is `(O + H) / 2` with radius `R / 2`. An altitude foot `F = u + e * t` with `t = dot(p - u, e) / |e|^2` lies on the line through `u` and `u + e`, and `dot(p - F, e) = dot(p - u, e) - t * |e|^2 = 0`, so it is the orthogonal projection; the altitude is `|cross(e, p - u)| / |e|`. The floating forms divide by `hypot(e)`, which is accurate to an ulp for every finite `e`, instead of `norm2(e)`, so the rounding error stays that of the vertex differences, one `cross` or `dot` and a division, relative O(eps * L^2 / |area2|) for the altitude like `area2` itself. The median from `p` ends at `(u + v) / 2`, so its vector is half the sum of the two edges leaving `p`.

Excircle opposite `o`: let `u` and `v` be the edges to the other two vertices, `x = |v|`, `y = |u|`, `z = |v - u|`, `s = cross(u, v)` and `w = dot(u, v)`. The excenter is `o + (u * x + v * y) / (x + y - z)` with radius `|s| / (x + y - z)` (weights `(-z, x, y)`). Since `(x + y)^2 - z^2 = 2 * (x * y + w)`, the gap is `x + y - z = 2 * g / (x + y + z)` with `g = x * y + w`. When `w >= 0`, `g` is a sum of nonnegative terms. When `w < 0`, `x * y + w = s^2 / (x * y - w)`, from `x^2 * y^2 = w^2 + s^2`. That form is evaluated as `s / (x * y - w) * s`, so neither branch cancels and no intermediate exceeds the size of `area2`. The code multiplies by `k = (x + y + z) / (2 * g)` instead of dividing by the gap. A direct `min(x, y) - (z - max(x, y))` would lose about `log2(L^2 / gap)` bits. The tests hold this form within 8 eps relative of `(sqrt(1 + h^2) + 1) / h` from `h = 1e-1` down to `1e-2000` (Feature-to-test map).

Heron sorts `a >= b >= c`, then tests `c - (a - b)`: negative is impossible and zero is degenerate. Otherwise it evaluates Kahan's grouping, `16 * area^2 = (a + (b + c)) * (c - (a - b)) * (c + (a - b)) * (a + (b - c))`. `frexp` splits each factor into a mantissa in [1/2, 1) and an exponent. The four mantissas multiply to a value in [1/16, 1), and the exponents add in an `int`. An odd exponent is made even before the square root, and `scalbn` applies the remaining power of two and the division by four. This avoids an overflowing or underflowing quartic product, and it never normalizes by the largest side, which could erase a tiny third side. The Kahan parentheses require ordinary floating semantics with no unsafe reassociation flags.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `t.circumcircle(out)`, `t.incircle(out)` | `circumcircleApprox(t.a, t.b, t.c, out)`, `incircleApprox(t.a, t.b, t.c, out)` |
| `t.circumcenter(p)`, `t.circumradius(r)` | `.c`, `.r` of `circumcircleApprox(t.a, t.b, t.c, out)` |
| `t.incenter(p)`, `t.inradius(r)` | `.c`, `.r` of `incircleApprox(t.a, t.b, t.c, out)` |

The circle queries returned on 2026-11-15 as the members `t.circumcircleApprox(out)` and `t.incircleApprox(out)` (user decision 2026-11-14); the legacy names stay removed.

## Feature-to-Test Map

Entry: [009-triangle_tester.py](<../../96-Local Testing/03-Geometry/009-triangle_tester.py>), with its C++ feature suite and the shared contest runner; the Python side uses only the standard library. Failures print the seed, the reproducing input and operation, and the expected and actual values; the runner adds command, configuration and crash or timeout details.

| Feature or edge class | Independent verification |
|---|---|
| `Triangle2` area2, areaApprox | Bounded exhaustive grids and Python bigint expanded shoelace determinants; both orientations, repeated and collinear vertices, coordinate-bound corners |
| `Triangle2::centroid` | Rational identities and independently normalized Python `Fraction` answers, including degenerate triangles |
| `Triangle2::angleKind` | Law of cosines on sorted squared sides (C++ `lll` over the exhaustive grid, Python bigints over the fixtures); right, acute and obtuse examples at the coordinate bound; degenerate-input assertion probe |
| `Triangle2::barycentric`, `Barycentric2` w, d, approx | Independent subdeterminants; positive denominator, exact partition of unity and weighted reconstruction; outside and vertex points; unchanged output on degeneracy |
| `Triangle2::approx`, `TriangleApprox` area2, area, sides, perimeter, centroid | Exact agreement on the binary integer corpus, the 3-4-5 values, opposite-vertex side order, orientation, scaling and the independent mean |
| `TriangleApprox` barycentric, fromBarycentric | Exact rational weights across the grid and fixture corpus, vertex Kronecker weights, random round trips and affine invariance, signed and negated weights, zero sums, degenerate reconstruction and output aliasing |
| `circumcircleApprox`, `incircleApprox` on triangles (through the members) | Known right-triangle values; equal vertex distances and `4 * area * R = a * b * c`; interior orientation, equal side-line distances and `r * perimeter = 2 * area`; all six vertex permutations, similarities and scaled triangles; thin triangles (circumcircle only); unchanged output on degeneracy; output center passed as a vertex |
| `TriangleApprox::angles` | Python Decimal law of cosines on exact squared sides within 8 eps relative (corner, right, thin, collinear and random triangles up to the 10^9 bound); exactly pi / 2 at the vertex opposite a Pythagorean side, matching `angleKind() == 0`; exact acute/obtuse classification; collinear `{0, pi, 0}` with pi at the middle vertex; sum pi within 16 eps; bit-identical permutation equivariance; laws of cosines and sines on random constructions; thin triangles `h` from `1e-3` to `1e-4000` against an 80-digit arctangent; scaling, similarity and exact-map invariance |
| `TriangleApprox::transform` | Bit-identical to per-vertex `applyPoint` and close to an independent reverse-order matrix evaluation; `area2` scaled by the determinant (exactly flipped by reflections on integer triangles); `t.transform(g).transform(f)` against `t.transform(f * g)`, bit-identical on exact maps; identity, translation, self-assigned scaling, degenerate triangles; nine map kinds per random construction |
| `TriangleApprox::circumcircleApprox`, `incircleApprox` (members) | Every triangle case of the free functions runs through the members, which must match the free function bit for bit, false with `out` unchanged on every degenerate triangle included; a `-0` vertex coordinate whose circumcenter keeps `+0` |
| `Triangle2::orient` | Sign of an independently expanded determinant over the exhaustive grid and every exact fixture; Python bigint signs at the coordinate bound, flipped for odd vertex orders across all six permutations |
| `Triangle2::locate` | C++ oracle from the signs of exact barycentric weights, and for zero area a collinear-plus-bounding-box test on each edge, over the exhaustive grid; the same oracle in Python on every exact fixture and on bound-scale fixtures with points placed exactly on edges (vertex plus k primitive steps, k from -2 to g + 2) and nudged by one each way, random and vertex queries, collinear and repeated vertices; equal answers for all six vertex orders |
| `Triangle2::circumcenter` | Python `Fraction` Cramer solution of the perpendicular-bisector equations, compared exactly; equal exact squared distances to the vertices in `InfIntMini`, positive normalized denominator, hypotenuse midpoint on right triangles, six-permutation invariance; coordinate-bound corners and thin triangles at 10^9 (values near 10^28) |
| `Triangle2::circumradius2` | Python `Fraction` squared distance from the independent circumcenter, compared exactly as numerator and denominator for `InfIntMini` (default) and `InfInt`; cross-multiplied against the exact circumcenter's squared distance in C++ |
| `Triangle2::orthocenter` | Python `Fraction` intersection of two altitudes, compared exactly; exact perpendicularity to all three sides; right-angle vertex on right triangles; six-permutation invariance; agreement with `TriangleApprox::orthocenter` within tolerance when the smallest angle exceeds about 7 degrees |
| `Triangle2::altitudes2`, `altitudeFeet` | Over the exhaustive grid (collinear triangles included), every exact fixture and the bound-scale center fixtures, in `InfIntMini`: each foot on its side line and perpendicular to it, normalized; `altitudes2 * \|side\|^2 == area2^2` and `altitudes2` equal to the squared vertex-to-foot distance; `Rational<InfInt>` equal to the `lll` default; repeated-vertex assertion probes |
| `Triangle2::medians2`, `medianFeet` | Same corpus: Apollonius `4 m^2 == 2 b^2 + 2 c^2 - a^2` in `InfIntMini`; feet equal to the side midpoints, normalized; `Rational<InfInt>` equal to the `lll` default |
| `TriangleApprox` altitudes, altitudeFeet, medians, medianFeet | Against the exact `Triangle2` values over the same corpus, repeated vertices included for the medians; on random constructions the foot against the projection formula and perpendicular to its side, `altitude * side == 2 * area`, the median against the vertex-to-midpoint length and Apollonius on the side lengths, the bit-identical midpoint and the centroid two thirds along each median; the 3-4-5 values at scales `1e-100` to `1e100`; regressions within 8 eps on a side `(3, 4) * 1e-2474` whose square is subnormal and on a leg `4e2470` whose square overflows; repeated-vertex and overflow assertion probes |
| `TriangleApprox::orthocenter` | Altitude perpendicularity, right-triangle vertex, Euler-line identity and conditioned thin-isosceles formulas |
| `TriangleApprox::excircle` | Bit-exact regression of the pinned right-angle branch; Python Decimal reference from the direct gap at 200 digits or more: exradius within 64 eps relative, excenter within the conditioning bound above. Thin obtuse closed form `(sqrt(1 + h^2) + 1) / h` within 8 eps, for `h` from `1e-1` to `1e-2000`. Product identity `r_a * r_b * r_c = r * s^2` and sum identity `r_a + r_b + r_c = 4 * R + r` within 512 eps relative; equal distances to all three side lines; halfplane placement; the three 3-4-5 answers |
| `TriangleApprox::ninePointCircle` | Side midpoints, altitude feet and vertex-orthocenter midpoints on the circle; center and radius identities |
| heronAreaApprox | Exact `Fraction` classification and Heron factors with a Decimal square root; side permutations, impossible, degenerate and zero sides, exactly degenerate dyadic sides, nearly degenerate sides, huge and tiny scales, mixed exponents; direct subnormal regressions |
| State and preconditions | Defaults, copy and move, public-field mutation, vertex-output aliasing, every bool no-answer preserving output; 34 checked-build assertion probes for coordinate bounds (vertices and the `locate` query), an overflowing or subnormal excircle gap, degenerate `angleKind`, `circumcenter`, `circumradius2` and `orthocenter`, a repeated vertex in `angles`, `altitudes`, `altitudeFeet` and the exact `altitudes2` and `altitudeFeet`, an overflowing `cross` in `altitudes` and side in `altitudeFeet`, an overflowing `transform`, nonfinite points and weights, bad excircle indices, an unrepresentable exradius past a normal gap (`k` overflows, caught by the `CircleApprox` constructor), invalid sides, Heron factor overflow and unrepresentable areas |

Approximate constructions without a tighter oracle use a scaled absolute or relative residual of 4e-13, with conditioned equations for very thin, small or large triangles. Heron uses a purely relative 8e-16 against Decimal references of up to 11,000 digits. These are test tolerances, not certified error bounds. Exact answers are compared as integers or rationals before any conversion.

| Mode | Corpus |
|---|---|
| quick | 6,561 grid cases, 109 exact, 79 Heron, about 26 excircle, 34 angle, 203 orient/locate and 27 center fixtures, 100 random constructions; optimized and checked builds |
| full | 18,225 grid cases, 2,009 exact, 369 Heron, 406 excircle, 380 angle, 4,635 orient/locate and 367 center fixtures, 2,000 random constructions; optimized, checked and ASan/UBSan builds |
| stress | 390,625 grid cases, 12,009 exact, 1,869 Heron, 2,406 excircle, 2,217 angle, 26,539 orient/locate and 2,161 center fixtures, 12,000 random constructions |

## Commands and Results

Package run, 2026-11-27 (P009 follow-up, altitudes and medians, experiment none): Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and the floor compiler GCC 14.4.1 (`CXX=g++-14`), Python 3.14, GNU++20, with other worktree sessions running; times are wall-clock under that load.

| Command | Result |
|---|---|
| Full, seed 20261127 (optimized, checked, ASan/UBSan) | PASS, 1,796,174 checks per configuration, 34 assertion probes; pre-flight quick peak 959 MB; peak 1,445 MB |
| Full, seed 20261127, `CXX=g++-14` | PASS, 1,796,174 checks per configuration, 34 assertion probes; peak 1,413 MB |
| Stress, seed 42, optimized | PASS, 24,021,570 checks, 27.9 s run; peak 39 MB |
| `05-extraction.py ops --filter 03-Geometry/009-triangle.hpp` | PASS, 43 operations in 38 extractions on g++ and g++-14 |
| `05-extraction.py testers --filter 009-triangle` | PASS, 20 names, quick run on the extracted build |
| `04-mutation.py run 03-Geometry/009-triangle.hpp` (changed lines) | See Mutation Testing |
| `02-integration.py` | PASS, 192 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, workspace; peak 1,633 MB |
| `03-consistency.py`; `--braces` on header and tester | No errors; no closing-brace, comment-cap or free-function violations |

```bash
python3 '96-Local Testing/03-Geometry/009-triangle_tester.py' --mode full --seed 20261127
CXX=g++-14 python3 '96-Local Testing/03-Geometry/009-triangle_tester.py' --mode full --seed 20261127
python3 '96-Local Testing/03-Geometry/009-triangle_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/009-triangle.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 009-triangle
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/009-triangle.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/009-triangle.hpp '96-Local Testing/03-Geometry/009-triangle_tester.cpp'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `b7e7178b9a9736e0ec85879f77a54862ca3eb371` (2026-11-27, seed 1, 17 lines changed since blob `90a91bda96b8`, sample of 106: 5 high, 7 medium, 38 low): 50 mutants, 48 caught (0 by timeout), 2 survived, 0 untested.

Stage times: quick 223 s, full optimized 34 s, full checked and ASan/UBSan 291 s, including the survivor retests after the non-dyadic altitude tests were added. This campaign covered the review's `hypot` forms. The altitude and median operations were first covered by a 2026-11-27 campaign on blob `90a91bda96b8` (50 of 228 changed-line mutants). The undated survivors come from the 2026-11-15 campaigns: a whole-file sample of 500 of 1,008 mutants, then 30 changed-line mutants, then 52 changed-line mutants with none surviving. Each survivor is equivalent, and line numbers are those of the current blob.

| Mutant | Line | Why equivalent |
|---|---|---|
| `AS 215`, `AS 233`, `AS 235` | 32, 96 | `angleApprox` uses `abs(cross)` and `dot`, and `dot` is symmetric; swapping its arguments gives the same bits |
| `AS 253` (2026-11-27) | 36 | `cross(p - u, e) == -cross(e, p - u)` exactly (the same two products, subtracted the other way), and only `abs(s)` uses it |
| `COS 1118` (2026-11-27) | 42 | a zero `l` makes `d` and `t` NaN, which the next `isfinite(t)` assertion rejects, so the repeated-vertex probe still fails; under `NDEBUG` nothing else changes |
| `CR 181` (2026-11-27, blob `90a91bda96b8`) | 115 | a repeated vertex makes one rotation's `e` zero, and `RationalPoint2` asserts its denominator `d = norm2(e) != 0`, so the `exact-altitude-feet-repeated-vertex` probe still fails; under `NDEBUG` nothing else changes |
| `AS 226` | 70 | `cross(v, u) == -cross(u, v)` exactly; `s` is used through `s == 0`, `abs(s)` and `s / q * s`, all sign-blind |
| `COS 928` | 131 | `d == 0` already returned false two lines earlier |
| `AOS 789`, `AOS 793`, `AOS 797` | 136 | `o` is `1` or `-1` in that branch, so `* o` equals `/ o` |
| `AS 251`, `AS 252`, `AS 253` | 144 | `swap(b, a)` is `swap(a, b)` |
| `LOS 864`, `COS 980` | 152 | for finite sides with `gap > 0` every Kahan factor is positive; an overflowing factor reaches the final `isfinite(area)` assertion, which the `heron-factor-overflow` probe still trips |

## Sources

Read on 2026-09-27, with the completeness sweep added on 2026-10-07; the 2026-11-15 re-audit sources (CGAL `circumcenter`, `squared_radius`, `side_of_bounded_circle` and `Triangle_2`; hitonanode `triangle.hpp`; suisen `geometry.hpp`) are in [00-sources.md](00-sources.md) under "P009 re-audit, fetched 2026-11-15". The implementation follows geometric and arithmetic identities; no external code was copied. Completeness-sweep candidates not adopted are in [00-notes.md](00-notes.md#p009-completeness-decisions-2026-10-07).

- [William Kahan, *Miscalculating Area and Angles of a Needle-like Triangle*](https://people.eecs.berkeley.edu/~wkahan/Triangle.pdf), revision of September 4, 2014, sections 1–4 (printed pages 2–4): naive Heron cancellation, decreasing-side ordering, the triangle-inequality test and the parenthesized four-factor formula. Its error analysis excludes overflow and underflow; the exponent-separated product is this implementation's own extension.
- MathWorld [Barycentric Coordinates](https://mathworld.wolfram.com/BarycentricCoordinates.html), [Triangle Centroid](https://mathworld.wolfram.com/TriangleCentroid.html), [Orthocenter](https://mathworld.wolfram.com/Orthocenter.html), [Incenter](https://mathworld.wolfram.com/Incenter.html) eq. (2), [Inradius](https://mathworld.wolfram.com/Inradius.html) eqs. (2)–(7), [Exradius](https://mathworld.wolfram.com/Exradius.html) eqs. (1)–(6) (area / semiperimeter gap, product and sum identities), [Nine-Point Center](https://mathworld.wolfram.com/Nine-PointCenter.html) and [Nine-Point Circle](https://mathworld.wolfram.com/Nine-PointCircle.html) eq. (3).
- CGAL Kernel `angle` (see [00-sources.md](00-sources.md)): acute, right and obtuse classification by the sign of a dot product.
- Preserved `OLD/Team Notebook/main.tex`, lines 232–234, has the elementary Heron formula; `heronAreaApprox` covers it. No other legacy triangle code needs migration, and the archive bytes are unchanged.

## Limits and Handoffs

- `Rational<InfIntMini>::toLongDouble` does not compile, so a caller who needs `circumradius2` as a float passes `InfInt` (follow-up on P019).
- Row 14's `EnclosingCircle::center` and `radius2` repeat the formulas of `Triangle2::circumcenter` and `circumradius2` for three support points and could call them (follow-up on P037).

## Aliases

Search aliases (P263, grab --find): 海伦公式, 海龙公式, 三角形面积, 重心坐标, 三角形重心, 垂心, 旁切圆, 九点圆, 三角形的高, 三角形中线, 中线长.

## History

- 2026-09-27: original P009, full suite (seed 20260927, 560,820 checks, 16 probes) passed on g++ 16.
- 2026-10-07: re-audit, added `angleKind`; 4 findings fixed (excircle gap cancellation, Heron oracle misclassifying dyadic degenerate sides, `; }` closings, excircle contract conditioning); full suite passed on g++ and g++-14.
- 2026-10-13: prune, removed `TriangleApprox` circumcircle, incircle, circumcenter, incenter, circumradius and inradius (cases moved to `circumcircleApprox` and `incircleApprox`); full suite passed on g++ and g++-14.
- 2026-10-30: SUP18 arm64 portability (user decision 2026-10-30, fixed in P250): excircle references printed to 40 digits instead of 30, which binary128 `long double` needs; x86-64 output unchanged; quick passed on x86-64 and on aarch64 under qemu-user, full passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (common names, Simplified and Taiwan or Hong Kong terms); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed. The header gained `[[maybe_unused]]` on names only an `assert` uses, with no logic change and no mutation campaign (user decision 2026-11-10).
- 2026-11-15: P009 re-audit, added `TriangleApprox::angles`, `transform`, `circumcircleApprox`, `incircleApprox` and `Triangle2::orient`, `locate`, `circumcenter`, `circumradius2`, `orthocenter` (user decisions 2026-11-14 and 2026-11-15); first mutation campaign; @reviewer found 2 wrong equivalence claims (COS 895, rounding pinned by the user; AS 221, signed zero), both closed by regressions, plus ledger and documentation fixes; 3 probes added; full suite passed on g++ and g++-14, stress and extraction passed.
- 2026-11-15: /finish P009: excircle asserts a normal gap `g` (2 probes; the unrepresentable-exradius probe moved past it), `circumradius2` rejects integral `I` at compile time, the member-circle failure messages print both circles, a signed-zero regression, the circumcenter formula sign and the excircle accuracy and range claims fixed, row 09 grouped by struct; fixes in `001-point.hpp` and `002-line_segment.hpp` by user decision (their evidence); changed-lines mutation campaign caught all 52 mutants; full suite passed on g++ and g++-14, stress, extraction and `02-integration.py` passed.
- 2026-11-27: P009 follow-ups: limits recheck (scope and the omitted approximate `locate` moved to Contracts, the P019 and row 14 handoffs filed as follow-ups on P019 and P037); added `TriangleApprox::altitudes`, `altitudeFeet`, `medians`, `medianFeet` and `Triangle2::altitudes2`, `altitudeFeet`, `medians2`, `medianFeet` (user decision 2026-11-27); @reviewer found no wrong answer, and its fixes gave the floating altitudes `hypot` forms (user decision 2026-11-27, row 02's gap filed on P007), a compile-time rejection of `Rational<lng>`, median tests on repeated vertices, bit-exact altitude tests and 2 overflow probes; changed-line campaigns 47 of 50 and 48 of 50 caught, survivors equivalent; full suite passed on g++ and g++-14, stress, extraction and `02-integration.py` passed.
