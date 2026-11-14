# 08-coordinate_transform.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `05-closestpair.hpp`, `06-circle.hpp` and `07-rotatingcalipers.hpp`); see [00-notes.md](00-notes.md#ge02-numeric-policy-p008) for the folder numeric policy. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `Affine2Approx`, `rotateApprox` and `cartesianApprox` are `Approx` APIs using ordinary `long double` arithmetic; `Affine2` is exact over its field `R`.

### Affine2Approx

`Affine2Approx { a, b, c, d, t }` represents `p -> [[a, b], [c, d]] * p + t`. The default is the identity. All inputs, intermediates and results must be finite, and angles are CCW radians.

- `applyPoint` adds `t`; `applyVector` does not. `applyHomogeneous((x, y, w))` multiplies the translation by `w` and preserves `w`. Points have `w != 0` and directions have `w == 0`; the algebraic zero vector is allowed but is not a projective point.
- `determinant()` is `a d - b c`. `orientation()` is its computed sign: `+1` preserves orientation, `-1` reverses it, `0` collapses a dimension. No epsilon or exact-predicate guarantee applies; even a mathematically singular map built through rounding can have a small nonzero computed determinant.
- `f * g` applies `g` first, then `f`; singular maps compose normally. `inverse(out)` returns `false` when the computed determinant is zero, leaving `out` unchanged; `out` may alias `*this`. Near singularity, inversion is arbitrarily ill-conditioned.
- The factories:
  - `translation(v)`.
  - `scaling(x, y, center)`: independent axis scaling about a center.
  - `rotation(angle, center)`: the angle must be finite.
  - `projection(p, q)` and `reflection(p, q)`: orthogonal projection and reflection in the infinite line through `p != q`, which must have a nonzero finite computed length.
  - `similarity(p, q, u, v)`: the direct similarity mapping `p -> u` and `q -> v`. It needs `q != p`; `u == v` gives a constant map, and reflections are not included.

Correctness: the implementation follows multiplication of homogeneous 3x3 affine matrices, stored as six coefficients. The inverse is the adjugate over the determinant, followed by the negated image of the translation. Projection is the outer product of a unit direction, and reflection is twice the projection minus the identity. Dot and cross products of the source and target directions give the direct similarity.

### cartesianApprox

`cartesianApprox((x, y, w), out)` returns `false` for `w == 0`, leaving `out` unchanged. Otherwise it sets `out = (x / w, y / w)`; no normalization is required. Finite input and finite quotients are required.

### rotateApprox

`rotateApprox(p, angle, center = {})` rotates `p` by `angle` CCW radians about `center`: exactly `Affine2Approx::rotation(angle, center).applyPoint(p)`, which it calls, so it has `rotation`'s domain (a finite angle, asserted) and rounding.

### Affine2

`Affine2<R> { a, b, c, d, tx, ty }` represents `p -> [[a, b], [c, d]] * p + (tx, ty)` exactly over a field type `R`. The default `R = Rational<lll>` (`01-Core/09-rational.hpp`) asserts on overflow, `Rational<lng>` likewise in 64 bits, and `Rational<InfInt>` never overflows; any type with field arithmetic, `==` and construction from `1` works. The default is the identity. Points and vectors are `array<R, 2>`. Every operation costs `O(1)` operations of `R`.

- `applyPoint` adds the translation and `applyVector` does not. `determinant()` is `a d - b c`; its sign is the orientation, exactly.
- `f * g` applies `g` first, then `f`. `inverse(out)` returns `false` exactly when the determinant is zero, leaving `out` unchanged; otherwise `out` is the exact inverse, and it may alias `*this`.
- The factories mirror `Affine2Approx`'s with exact coefficients:
  - `translation(v)`.
  - `scaling(x, y, center = {})`: independent axis scaling about a center; zero factors give singular maps.
  - `rotation(co, si, center = {})`: the rotation with cosine `co` and sine `si`, which must satisfy `co^2 + si^2 == 1` (asserted); over the rationals these are the Pythagorean-triple angles such as `(3/5, 4/5)`.
  - `projection(p, q)` and `reflection(p, q)`: orthogonal projection onto, and reflection in, the line through `p != q` (asserted). With `v = q - p`, the projection's linear part is `v v^T / (v . v)`, rational for rational input.
  - `similarity(p, q, u, v)`: the direct similarity mapping `p -> u` and `q -> v`, with `q != p` (asserted). Its linear part is `[[co, -si], [si, co]]` with `co = (x . y) / |x|^2` and `si = cross(x, y) / |x|^2` for `x = q - p`, `y = v - u`, rational for rational input; `u == v` gives a constant map, and reflections are not included.
- `fixing(p, u)` is the factories' shared helper: the same linear part with the translation that maps `p` to `u`.

Correctness: the same homogeneous 3x3 algebra as `Affine2Approx`, evaluated exactly. Projection, reflection and similarity avoid the square root of `Affine2Approx`'s unit direction by dividing by the squared length once, which keeps every coefficient rational.

## Feature-to-test map

`96-Local Testing/03-Geometry/08-coordinate_transform_tester.py` drives the suite. Ordinary `Fraction` comparisons use `2e-15`. Inverse checks use `2e-14` times the larger of one, the output magnitude and the absolute pre-cancellation terms over the absolute determinant. Geometric constructions use `2e-12` times the larger of the output and input scales.

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
| `Affine2` preconditions | Six checked-build probes: rotation with the non-unit pairs `(3/5, 3/5)` and `(4/5, 4/5)` (sum of squares below and above 1); projection, reflection and similarity with `p == q`; a `Rational<lll>` overflow from squaring a scaling by 2^100; plus `rotateApprox` with a NaN angle | Expected assertion failure |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 670 MB; optimized 0.3 s compile + 1.2 s run, checked 10.1 s compile + 1.0 s run, ASan-UBSan 13.8 s compile + 3.2 s run; MEMORY peak 740 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 739 MB; optimized 0.3 s compile + 1.0 s run, checked 13.0 s compile + 1.0 s run, ASan-UBSan 15.8 s compile + 3.7 s run; MEMORY peak 811 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 14.9 s compile + 0.0 s run, checked 13.3 s compile + 0.1 s run; MEMORY peak 664 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 681 MB; optimized 0.1 s compile + 2.3 s run, checked 3.6 s compile + 2.3 s run, ASan-UBSan 4.6 s compile + 7.2 s run; MEMORY peak 573 MB. Dated rows below stay those commands' latest results.

2026-11-14 (P008; experiment none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns.

| Command | Result |
|---|---|
| `08-coordinate_transform_tester.py --mode full --seed 20261114` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 527,775 checks per configuration; Python `Fraction` oracle on 3,000 records and 5,000 geometric cases; exact `Affine2` oracle on 3,027 records (550 `lng`, 876 `lll`, 1,601 `InfInt`); 13 assertion probes; `MEMORY peak=1160MB` |
| `CXX=g++-14 08-coordinate_transform_tester.py --mode full --seed 20261114` (GCC 14.4.1) | PASS, the same counts; `MEMORY peak=1143MB` |
| `08-coordinate_transform_tester.py --mode stress --seed 42` (all three configurations) | PASS, 3,206,141 checks per configuration; 20,000 `Fraction` records, 40,000 geometric cases, 12,027 exact `Affine2` records (2,098 `lng`, 3,475 `lll`, 6,454 `InfInt`); `MEMORY peak=741MB` |
| `05-extraction.py ops --filter 08-coordinate_transform`; `testers --filter 08-coordinate_transform` | PASS: 37 operations in 22 extractions on g++ and g++-14; 11 names, quick on the extracted build |
| `04-mutation.py run 03-Geometry/08-coordinate_transform.hpp` | See Mutation testing |

```bash
python3 '96-Local Testing/03-Geometry/08-coordinate_transform_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/03-Geometry/08-coordinate_transform_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/03-Geometry/08-coordinate_transform_tester.py' --mode stress --seed 42
python3 '96-Local Testing/05-extraction.py' ops --filter 08-coordinate_transform
python3 '96-Local Testing/05-extraction.py' testers --filter 08-coordinate_transform
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/08-coordinate_transform.hpp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

`python3 '96-Local Testing/04-mutation.py' run 03-Geometry/08-coordinate_transform.hpp`, rerun on the lines the review fixes changed (`inverse` moved before `operator*`, the local `n` renamed `len2`):

Record: mewt 4.0.0 on blob `1ceaa772fd45cfd311e6536970b3b290d9308eae` (2026-11-14, seed 1, 18 lines changed since blob `f0f3da877921`): 273 mutants, 266 caught (0 by timeout), 7 survived, 0 untested.

Stage seconds: quick 599, full-optimized 39, full-rest 162 (baselines 11.0 s, 18.4 s and 34.5 s). The 7 survivors are the earlier campaign's equivalents on the same statements, now on lines 71, 72 and 78: `len2 != R()` → `>`, `>=` or removed (CR 113, 118; COS 722, 723, 727, 728), equivalent by the argument below for `n`, and `fixing(p, p)` with its arguments commuted (AS 146), the same expression.

Earlier campaign, whole file, then the survivors retested with `--workdir` after three assertion probes were added: mewt 4.0.0 on blob `f0f3da8779212295a280055314e4ebd676cecce9` (2026-11-14, seed 1, whole file, uniform sample of 738): 500 mutants, 488 caught (0 by timeout), 12 survived, 0 untested.

Stage seconds: quick 1288, full-optimized 100, full-rest 436 (summed over the campaign and the retest; the retest's baselines 17.9 s, 24.1 s and 41.4 s).

The first pass left 17 survivors. Three probes closed five of them: `projection-infinite` and `similarity-infinite`, whose span `LDBL_MAX - (-LDBL_MAX)` overflows `hypot` to infinity, catch the removed or `||`-weakened length assertions of lines 37 and 45 (CR 88, 97; LOS 692, 693), and `exact-rotation-over`, `rotation(4/5, 4/5)`, catches `co^2 + si^2 == 1` → `>=` on line 67 (COS 718). The other 12 survivors are equivalent:

- Commuted symmetric arguments: `dot(x, y)` on line 46 (AS 142) and `fixing(center, center)` on line 68 (AS 145).
- Line 74, `projection(q, p)` for `projection(p, q)` (AS 147): negating the direction leaves `x^2 / n`, `xy / n` and `y^2 / n` unchanged, and fixing `q` instead of `p` gives the same exact map, since a projection fixes every point of its line.
- Lines 37 and 45, `len > 0` → `!= 0` (COS 700, 705): `hypot` never returns a negative value, and for NaN `isfinite` fails either way.
- Lines 37 and 45, `len > 0` → `>= 0` (COS 703, 708), and lines 71 and 78, `n != R()` → `>`, `>=` or removed (CR 113; COS 722, 723, 727, 728): `n` is a sum of squares, so `>` is unchanged, and the weaker forms only move the abort for a degenerate input from this assertion to the next statement, `Point2::operator/=`'s `k != 0` or the zero-divisor assertion of `Rational`'s division. The degenerate-input probes still fail as expected. Under `NDEBUG` the floating division yields an infinity or NaN and `Rational::mulTo` returns its failure flag, both in O(1), past the violated precondition (`05-testing.md`, user decision 2026-11-12).

## Sources

The implementation is independently derived. It was compared with KACTL `Point::rotate` and `linearTransformation.h` (printed pp. 17–18, 2024 edition) and Stanford's rotation and projection routines (section 2.2). The archived `point.cpp` and `intersection.cpp` contain no affine API. `Affine2`'s factories follow CGAL `Aff_transformation_2` (Translation, Scaling, Reflection, and Rotation from a rational sine and cosine). Candidates not adopted (line images, map equality, CGAL's rational rotation approximation) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07) and [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Limits and handoffs

3D frames belong to GE31. No claim of universally certified floating geometry is made.

## Aliases

Search aliases (P263, grab --find): 坐标变换, 仿射变换, 点的旋转, 平移变换, 缩放变换, 反射变换, 齐次坐标, 变换矩阵.

Search aliases (P008, grab --find): exact affine transformation, rational affine map, Aff_transformation_2, rotate point, 有理仿射变换, 精确坐标变换.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 265,418 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, 1 finding fixed, contract comments moved to this document; full suite and stress passed on g++ 16; independent review: reproduced every full count and probe, confirmed one package defect (in `06-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: P008 re-audit (GE02, experiment none): added `rotateApprox` and the exact `Affine2` on `Rational` (fields, builders, application, composition, determinant, inverse) after the completeness sweep; first mutation campaign with three assertion probes; independent review nits fixed (`inverse` before `operator*`, `len2`); changed-lines campaign; full on g++ and g++-14, stress, extraction and `02-integration.py --sanitizers` passed.
