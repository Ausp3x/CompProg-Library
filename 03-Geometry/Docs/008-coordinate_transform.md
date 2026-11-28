# 008-coordinate_transform.hpp — Evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, with `005-closestpair.hpp`, `006-circle.hpp` and `007-rotatingcalipers.hpp`). Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric Policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `Affine2Approx`, `rotateApprox` and `cartesianApprox` use ordinary `long double` arithmetic; `Affine2` is exact over its field `R`.

### Affine2Approx

`Affine2Approx { a, b, c, d, t }` represents `p -> [[a, b], [c, d]] * p + t`. The default is the identity. All inputs, intermediates and results must be finite, and angles are CCW radians.

- `applyPoint` adds `t`; `applyVector` does not, and is the image of a direction up to positive scale (CGAL's `Direction_2` image). `applyDirection(v)` is `unitApprox(applyVector(v))`, the unit image direction; the computed image must be nonzero, which `unitApprox` asserts.
- `applyLinear({p, q})` is `{applyPoint(p), applyPoint(q)}`: the image of a segment, ray or line in the folder's two-point form, keeping the endpoint roles, so the caller's kind still applies. A singular map may send it to a single point (`p == q`), which `Linear2`'s convention reads as a singleton.
- `applyLine({A, B, K}, out)` maps the line `A x + B y = K`. It returns `false` exactly when `inverse` does (a computed zero determinant), leaving `out` unchanged. Otherwise, with `g` the computed inverse, it sets `out = {x, y, K + x t.x + y t.y}` for `(x, y) = (g.a A + g.c B, g.b A + g.d B)`, the inverse transpose of the normal; `out` may alias the input. Since `A x' + B y'` of a preimage equals `x X + y Y` of its image, the image line keeps the sign of `A x + B y - K` at every point, so the positive side maps to the positive side. `(A, B) = (0, 0)` stays `(0, 0)`.
- `operator==` compares the six computed coefficients exactly (`-0 == 0`).
- No factory-membership predicates: rounding breaks the coefficient identities they would test (user decision 2026-11-27; `Affine2` has them exactly).
- `applyHomogeneous((x, y, w))` multiplies the translation by `w` and preserves `w`. Points have `w != 0` and directions have `w == 0`; the algebraic zero vector is allowed but is not a projective point.
- `determinant()` is `a d - b c`, and `orientation()` its computed sign: `+1` preserves orientation, `-1` reverses it, `0` collapses a dimension. No epsilon or exact-predicate guarantee applies: a mathematically singular map built through rounding can have a small nonzero computed determinant.
- `f * g` applies `g` first, then `f`; singular maps compose normally. `inverse(out)` returns `false` when the computed determinant is zero, leaving `out` unchanged; `out` may alias `*this`. Near singularity, inversion is arbitrarily ill-conditioned.
- The factories:
  - `translation(v)`.
  - `scaling(x, y, center)`: independent axis scaling about a center.
  - `rotation(angle, center)`: the angle must be finite.
  - `projection(p, q)` and `reflection(p, q)`: orthogonal projection and reflection in the infinite line through `p != q`, which must have a nonzero finite computed length.
  - `similarity(p, q, u, v)`: the direct similarity mapping `p -> u` and `q -> v`. It needs `q != p`; `u == v` gives a constant map, and reflections are not included.

Correctness: the maps multiply as homogeneous 3x3 affine matrices stored as six coefficients. The inverse is the adjugate over the determinant, followed by the negated image of the translation. Projection is the outer product of a unit direction, and reflection is twice the projection minus the identity. Dot and cross products of the source and target directions give the direct similarity.

### cartesianApprox

`cartesianApprox((x, y, w), out)` returns `false` for `w == 0`, leaving `out` unchanged, and otherwise sets `out = (x / w, y / w)`; no normalization is needed. Input and quotients must be finite.

### rotateApprox

`rotateApprox(p, angle, center = {})` rotates `p` by `angle` CCW radians about `center` by calling `Affine2Approx::rotation(angle, center).applyPoint(p)`, so it has `rotation`'s domain (a finite angle, asserted) and rounding.

### Affine2

`Affine2<R> { a, b, c, d, tx, ty }` represents `p -> [[a, b], [c, d]] * p + (tx, ty)` exactly over a field type `R`. The default `R = Rational<lll>` (`01-Core/09-rational.hpp`) asserts on overflow, as does `Rational<lng>` in 64 bits; `Rational<InfInt>` never overflows. Any type with field arithmetic, `==` and construction from `1` works. The default is the identity. Points and vectors are `array<R, 2>`. Every operation costs `O(1)` operations of `R`.

- `applyPoint` adds the translation and `applyVector` does not. `determinant()` is `a d - b c`; its sign is the orientation, exactly.
- `applyLinear` and `applyLine` are `Affine2Approx`'s, evaluated exactly: `applyLine` returns `false` exactly when the determinant is zero, and otherwise the exact image line with the same positive side. There is no exact unit direction; `applyVector` is the direction image.
- `operator==` is equality of the six coefficients under `R`'s `==`, which for gcd-normalized `Rational` is equality of maps.
- `isTranslation`, `isScaling`, `isRotation`, `isProjection`, `isReflection` and `isSimilarity` (CGAL's `is_translation`, `is_scaling`, `is_rotation`, `is_reflection`, decided from the coefficients rather than a stored constructor tag) are each true exactly when the factory of that name builds the map from some arguments over `R`. With linear part `L = [[a, b], [c, d]]` and translation `t`:
  - translation: `L = I`;
  - scaling: `b = c = 0`, and `t_x = 0` where `a = 1`, `t_y = 0` where `d = 1` (otherwise the center `t_x / (1 - a)`, `t_y / (1 - d)` exists);
  - rotation: a similarity with `a^2 + c^2 = 1` and `t = 0` when `a = 1` (then `L = I`); otherwise `I - L` has determinant `(1 - a)^2 + c^2 > 0`, so a center exists;
  - projection: `L` symmetric with trace 1 and determinant 0, so `L = v v^T / (v . v)` for a rational column `v` of `L`, and `L t = 0`, so `p = t` is a fixed point and `p`, `q = p + v` build it;
  - reflection: `a = -d`, `b = c`, `a^2 + b^2 = 1`, so `(L + I) / 2` is such a projection, and `(L + I) t = 0`, which excludes glide reflections; `p = t / 2` is fixed and `q = p + v`;
  - similarity: `a = d`, `b = -c` (any translation through `fixing`; the zero map included).
  Each costs `O(1)` operations of `R`; `Rational<lll>` asserts on overflow of the products.
- `f * g` applies `g` first, then `f`. `inverse(out)` returns `false` exactly when the determinant is zero, leaving `out` unchanged; otherwise `out` is the exact inverse, and it may alias `*this`.
- The factories mirror `Affine2Approx`'s with exact coefficients:
  - `translation(v)`.
  - `scaling(x, y, center = {})`: independent axis scaling about a center; zero factors give singular maps.
  - `rotation(co, si, center = {})`: the rotation with cosine `co` and sine `si`, which must satisfy `co^2 + si^2 == 1` (asserted); over the rationals these are the Pythagorean-triple angles such as `(3/5, 4/5)`.
  - `projection(p, q)` and `reflection(p, q)`: orthogonal projection onto, and reflection in, the line through `p != q` (asserted). With `v = q - p`, the projection's linear part is `v v^T / (v . v)`, rational for rational input.
  - `similarity(p, q, u, v)`: the direct similarity mapping `p -> u` and `q -> v`, with `q != p` (asserted). Its linear part is `[[co, -si], [si, co]]` with `co = (x . y) / |x|^2` and `si = cross(x, y) / |x|^2` for `x = q - p`, `y = v - u`, rational for rational input; `u == v` gives a constant map, and reflections are not included.
- `fixing(p, u)` is the factories' shared helper: the same linear part with the translation that maps `p` to `u`.

Correctness: the same homogeneous 3x3 algebra as `Affine2Approx`, evaluated exactly. Projection, reflection and similarity divide once by the squared length instead of normalizing the direction with a square root, which keeps every coefficient rational.

### Ownership

3D frames are row 26 (`026-geometry3d.hpp`, `Frame3`, GE31).

## Feature-to-Test Map

`96-Local Testing/03-Geometry/008-coordinate_transform_tester.py` drives the suite. Ordinary `Fraction` comparisons use `2e-15`. Inverse checks use `2e-14` times the larger of one, the output magnitude and the absolute pre-cancellation terms over the absolute determinant. Geometric constructions use `2e-12` times the larger of the output and input scales.

| Operation | Test | Oracle |
|---|---|---|
| Coefficients, `applyPoint`, `applyVector`, `operator*` (composition order) | Identity and composed maps | Python `Fraction` matrix multiplication with separate point and vector homogeneous coordinates |
| `translation`, `rotation` (centered), `scaling` | Fixed centers, inverse rotations, negative-scale orientation | Direct formulas, polar-coordinate rotation |
| `projection`, `reflection` | Fixed points, idempotence, involution, determinant | Implicit-normal equations |
| `similarity` | Both mapped endpoints, rotated and scaled displacement, constant maps | Direct formulas |
| `determinant`, `orientation`, `inverse` | Singular fixtures, unchanged failure output, self-aliasing, rank zero and one, tiny determinants | `Fraction` determinant and inverse |
| `applyHomogeneous`, `cartesianApprox` | Negative and zero weights, scale invariance, unchanged no-point output | `Fraction` products |
| Preconditions and scale | Six assertion probes (non-finite angle, `p == q` for projection, reflection and similarity, and an infinite length for projection and similarity from the span `-LDBL_MAX` to `LDBL_MAX`); fixtures from `1e-8` to `1e8` with arithmetic-aware tolerances | Expected assertion failure; scaled tolerances |
| `rotateApprox` (default and explicit center) | The geometry loop, 100/5,000/40,000 cases (quick/full/stress) | Bitwise equal (value and sign bit) to `Affine2Approx::rotation(angle, center).applyPoint(p)`; polar-coordinate oracle within `2e-12` of the point scale |
| `Affine2` default (identity, `R = Rational<lll>`) | `Affine2<>{}.applyPoint(p) == p` on every record; `static_assert` of the default type | Definition |
| `Affine2` factories `translation`, `scaling`, `rotation`, `projection`, `reflection`, `similarity`, and `fixing` | Coefficients of both maps of every record (147/3,027/12,027 records over `Rational<lng>`, `Rational<lll>` and `Rational<InfInt>`, each record on the narrowest type that holds its intermediates); `fixing(p, u)` maps `p` to `u` | Python `Fraction` 3x3 homogeneous matrices built pointwise from the definitions (complex arithmetic for rotation, scaling and similarity; the implicit normal equation for projection), exact equality |
| `Affine2` `applyPoint`, `applyVector`, `determinant`, `operator*` | Every record; `f * g`, `g * f` and `(f * g)(p) == f(g(p))` | Matrix products; Gauss–Jordan determinant |
| `Affine2::inverse` | Existence flag, coefficients, `f * inv` and `inv * f` equal to the identity, self-aliased `h.inverse(h)`; singular maps of rank 0 and 1 (zero scalings, projections, constant similarities) leave a sentinel `out`, and a self-aliased call leaves the map, unchanged | Gauss–Jordan inverse |
| `Affine2` metamorphic identities | Projection idempotent, reflection an involution of determinant -1, rotation fixing its center with determinant 1, similarity mapping `p -> u` and `q -> v` | Definitions |
| `Affine2` scale | `lll` records near 2^126 (one at least 2^120 is required), Pythagorean rotations from `m, n` near 2^30; `InfInt` records from 2^130 to 2^200 (one past 2^127 required), rotations from `m, n` near 2^90 | As above |
| `Affine2Approx::applyDirection` | Every `Fraction` record, the image normalized with an 80-digit square root, within `2e-15`, and bitwise equal to `unitApprox(applyVector(v))`; rotation against the polar direction and similarity against `unitApprox(perp(v))` in the geometry loop; probe `direction-zero` (`scaling(0, 1).applyDirection({1, 0})`) | `Fraction` image; definition; expected assertion failure |
| `Affine2Approx::applyLinear`, `Affine2::applyLinear` | Every record: bitwise equal to the two `applyPoint` calls and to the `Fraction` (exact: `Fraction`, exact equality) images of both endpoints; projection feet in the geometry loop | `Fraction` images |
| `Affine2Approx::applyLine`, `Affine2::applyLine` | Every record, on the line through two random integer points (exact: through `u` and `v`): coefficients within `2e-14` of the inverse's pre-cancellation terms (exact: equal), both line points on the image line, a positive-side point keeping its value and sign, the value at `f(p)` equal to the value at `p`; singular maps (782 of the 3,078 exact records with seed 2) return `false` with a sentinel `out` unchanged; self-aliased calls; four-point value invariance under rotation, reflection, scaling and similarity, and projections returning `false` exactly on a computed zero determinant | `Fraction` image line from the exact images of two points scaled by a third point's value, never through the inverse |
| `operator==` (both maps) | Every record: equality of `f` and `g` against `Fraction` equality, a copy, a one-ulp (exact: any single coefficient) change; `-0.0` coefficients and the identity built as `translation({})`, `scaling(1, 1)`, `rotation(0, c)`; `f * inverse` and `inverse * f` equal `Affine2{}` | `Fraction` equality |
| `isTranslation`, `isScaling`, `isRotation`, `isProjection`, `isReflection`, `isSimilarity` | Every predicate on `f`, `g`, `f * g` and `g * f` of every exact record (73,681 verdicts with seed 2, 14,141 true), plus 17 boundary pairs per type: identity, pure translation, zero and constant maps, axis scalings with factor 1 and a translation, glide reflections, a projection moved along its line, projections and reflections moved along their normal, oblique rank-one idempotents, `(1/2) I`, half and quarter turns, an opposite similarity as a glide and as a true reflection, non-unit forms, axis-parallel lines moved along themselves, two reflections composing to a rotation and to a translation | Constructive `Fraction` oracle: it solves for each factory's arguments (fixed points by a 2x2 solver for singular systems, the line direction from a nonzero column, scaling centres axis by axis) and rebuilds the map pointwise, exact equality |
| `Affine2` preconditions | Six checked-build probes: rotation with the non-unit pairs `(3/5, 3/5)` and `(4/5, 4/5)` (sum of squares below and above 1); projection, reflection and similarity with `p == q`; a `Rational<lll>` overflow from squaring a scaling by 2^100; plus `rotateApprox` with a NaN angle | Expected assertion failure |

## Commands and Results

Package run, 2026-11-27 (P008 follow-ups, `/package`; experiment none): Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' runs.

| Command | Result |
|---|---|
| `008-coordinate_transform_tester.py --mode full --seed 20261114` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 908,598 checks per configuration; Python `Fraction` oracle on 3,000 records and 5,000 geometric cases; exact `Affine2` oracle on 3,078 records (567 `lng`, 893 `lll`, 1,618 `InfInt`; `applyLine` 775 singular, 2,277 invertible; predicates 13,929 true, 59,744 false); 14 assertion probes; `MEMORY peak=1264MB` |
| `CXX=g++-14 008-coordinate_transform_tester.py --mode full --seed 2` (GCC 14.4.1) | PASS, 908,146 checks per configuration; 3,078 exact records (533 `lng`, 888 `lll`, 1,657 `InfInt`); 14 probes; `MEMORY peak=1246MB` |
| `008-coordinate_transform_tester.py --mode stress --seed 3` (all three configurations) | PASS, 5,539,847 checks per configuration; 20,000 `Fraction` records, 12,078 exact `Affine2` records (2,087 `lng`, 3,420 `lll`, 6,571 `InfInt`); `MEMORY peak=784MB` |
| `05-extraction.py ops --filter 008-coordinate_transform`; `testers --filter 03-Geometry/0` | PASS: 50 operations in 33 extractions on g++ and g++-14; 12 names for this tester, quick on the extracted build |
| `02-integration.py` | PASS, 192 standalone and aggregate headers; `MEMORY peak=3498MB` |
| `04-mutation.py run 03-Geometry/008-coordinate_transform.hpp` | See Mutation Testing |

Library-wide runs, 2026-11-10 (P274; same machine, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), this suite's lines, all PASS:

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | `MEMORY` peak |
|---|---|---|---|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | 670 MB | 0.3 s compile + 1.2 s run | 10.1 + 1.0 s | 13.8 + 3.2 s | 740 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | 739 MB | 0.3 + 1.0 s | 13.0 + 1.0 s | 15.8 + 3.7 s | 811 MB |
| `01-run.py --mode quick` | — | 14.9 + 0.0 s | 13.3 + 0.1 s | — | 664 MB |
| `01-run.py --mode stress --rounds 1` | 681 MB | 0.1 + 2.3 s | 3.6 + 2.3 s | 4.6 + 7.2 s | 573 MB |

```bash
python3 '96-Local Testing/03-Geometry/008-coordinate_transform_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/03-Geometry/008-coordinate_transform_tester.py' --mode full --seed 2
python3 '96-Local Testing/03-Geometry/008-coordinate_transform_tester.py' --mode stress --seed 3
python3 '96-Local Testing/05-extraction.py' ops --filter 008-coordinate_transform
python3 '96-Local Testing/05-extraction.py' testers --filter 03-Geometry/0
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/008-coordinate_transform.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `4005663b0842eb4db1cb0ece257e741b4717e620` (2026-11-27, seed 1, 23 lines changed since blob `1ceaa772fd45`, sample of 383: 10 high, 25 medium, 15 low): 50 mutants, 49 caught (0 by timeout), 1 survived, 0 untested.

Stage times: quick 168 s, full optimized 32 s, full on the other builds 89 s. The open survivors below come from this record (N), from the 2026-11-14 changed-lines record (C) and from the earlier whole-file campaign of 2026-11-14 (W) on lines no later record covered. Line numbers are those of the current blob.

| Mutants (record) | Line | Why equivalent |
|---|---|---|
| `COS 700`, `COS 705` (W; `len > 0` → `!= 0`) | 45, 53 | `hypot` never returns a negative value, and for NaN `isfinite` fails either way. |
| `COS 703`, `COS 708` (W; `len > 0` → `>= 0`); `CR 113`, `CR 118`, `COS 722`, `COS 723`, `COS 727`, `COS 728` (C; `len2 != R()` → `>`, `>=` or removed) | 45, 53, 79, 86 | `len2` is a sum of squares, so `>` is unchanged. The weaker forms only move the abort for a degenerate input from this assertion to the next statement: `Point2::operator/=`'s `k != 0` or the zero-divisor assertion of `Rational`'s division, and the degenerate-input probes still fail as expected. Under `NDEBUG` the floating division yields an infinity or NaN and `Rational::mulTo` returns its failure flag, both in `O(1)`, past the violated precondition (user decision 2026-11-12, `05-testing.md`, Mutation Testing). |
| `AS 142` (W) | 54 | `dot(x, y)` with its arguments commuted. |
| `AS 145` (W); `AS 146` (C) | 76, 80 | `fixing(center, center)` and `fixing(p, p)` with their arguments commuted are the same expression. |
| `COS 1033` (N; `a != R(1)` → `a < R(1)` in `isRotation`) | 110 | The conjunct before it requires `a^2 + c^2 = 1`, so `a <= 1` and `a != 1` is `a < 1`. |
| `AS 147` (W; `projection(q, p)` for `projection(p, q)`) | 82 | Negating the direction leaves `x^2 / len2`, `xy / len2` and `y^2 / len2` unchanged, and fixing `q` instead of `p` gives the same exact map, since a projection fixes every point of its line. |

## Sources

The implementation is derived independently and was compared with KACTL `Point::rotate` and `linearTransformation.h` (printed pp. 17–18, 2024 edition) and Stanford's rotation and projection routines (section 2.2). The archived `point.cpp` and `intersection.cpp` contain no affine API. `Affine2`'s factories, the line and direction images, map equality and the `is` predicates follow CGAL `Aff_transformation_2` (Translation, Scaling, Reflection, Rotation from a rational sine and cosine; `transform` of points, vectors, directions and lines; `is_translation` and the others), added by user decisions 2026-11-27; CGAL's rational rotation approximation, not adopted, and these decisions are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07) and [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): 坐标变换, 仿射变换, 点的旋转, 平移变换, 缩放变换, 反射变换, 齐次坐标, 变换矩阵.

Search aliases (P008, grab --find): exact affine transformation, rational affine map, Aff_transformation_2, rotate point, 有理仿射变换, 精确坐标变换.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 265,418 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, 1 finding fixed, contract comments moved to this document; full suite and stress passed on g++ 16; independent review reproduced every full count and probe and confirmed one package defect (in `006-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (common names, Simplified and Taiwan or Hong Kong terms); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed.
- 2026-11-14: P008 re-audit (GE02, experiment none): added `rotateApprox` and the exact `Affine2` on `Rational` (fields, builders, application, composition, determinant, inverse) after the completeness sweep; first mutation campaign, whose survivors added three assertion probes; independent review nits fixed (`inverse` before `operator*`, `len2`); changed-lines campaign; full on g++ and g++-14, stress, extraction and `02-integration.py --sanitizers` passed.
- 2026-11-27: `/package P008`, Limits recheck (experiment none): 3D frames (row 26) moved to Contracts (Ownership), and the uncertified floating claim is the folder policy the Numeric Policy links; line and direction images and map equality are to be added (user decision 2026-11-27, filed as a P008 follow-up); independent review of the documents; no code or test change.
- 2026-11-27: `/package P008` follow-ups (experiment none): added `applyDirection`, `applyLinear`, `applyLine` and `operator==` to `Affine2Approx`, and `applyLinear`, `applyLine`, `operator==` and the six exact factory-membership predicates to `Affine2` (user decisions 2026-11-27; no `Affine2Approx` predicates); tests with a constructive `Fraction` oracle; changed-lines mutation campaign, 1 equivalent survivor; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
- 2026-11-28: `/finish P008` (experiment none): the independent review of the follow-up diff found no defect in this header; its kept forms entered `duplicates.md` (Keeps).
