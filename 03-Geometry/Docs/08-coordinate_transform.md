# 08-coordinate_transform.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `05-closestpair.hpp`, `06-circle.hpp` and `07-rotatingcalipers.hpp`); see [00-notes.md](00-notes.md#p008--ge02-package-record) for the package record and the folder numeric policy. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. Both APIs are `Approx` APIs using ordinary `long double` arithmetic. Inputs, intermediates and results must be finite, and nonzero divisors and geometric distinctions must survive rounding; there is no hidden epsilon.

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
| Preconditions and scale | Four assertion probes; fixtures from `1e-8` to `1e8` with arithmetic-aware tolerances | Expected assertion failure; scaled tolerances |

## Commands and results

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 is not installed; no GCC 14 run is claimed.

| Command | Result |
|---|---|
| `08-coordinate_transform_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 265,418 checks per configuration; 4 assertion probes |
| `08-coordinate_transform_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 2,042,850 checks |
| Warning sweep, every approximate operation, `-Wall -Wextra -Wconversion` | No header warnings |
| Two-translation-unit program mixing all four GE02 headers (optimized, ASan/UBSan); standalone header | PASS |
| `02-integration.py --sanitizers`, geometry quick run, `03-consistency.py` (with `--braces`) | PASS, no errors |

```bash
python3 '96-Local Testing/03-Geometry/08-coordinate_transform_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/08-coordinate_transform_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[5-8]*.hpp '96-Local Testing/03-Geometry/'0[5-8]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Sources

The implementation is independently derived. It was compared with KACTL `Point::rotate` and `linearTransformation.h` (printed pp. 17–18, 2024 edition) and Stanford's rotation and projection routines (section 2.2). The archived `point.cpp` and `intersection.cpp` contain no affine API. Candidates not adopted (line images, map equality, exact rational transforms) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07).

## Limits and handoffs

3D frames belong to GE31. No claim of universally certified floating geometry is made.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 265,418 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, 1 finding fixed (tester brace closings), contract comments moved to this document; full suite and stress passed on g++ 16.
