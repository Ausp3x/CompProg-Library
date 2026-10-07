# 01-point.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#p007--ge01-package-record) for the package record and the folder numeric policy. The 2026-10-07 re-audit implemented the operations the earlier row had left `missing` (`canonicalDirection`, `angleApprox`, `signedAngleApprox`) and added `unitApprox` and `argApprox` from the completeness sweep. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies: signed integral and floating `T`, widening to `lll` before arithmetic, `|x| <= 10^9` sufficient for every operation (including the cubic `tetraVolume6`), finite floating inputs with computed signs and no epsilon, and `long double` results for every `Approx` API. `polarLess` is integral-only because floating rounding can break a sort's strict weak ordering.

### Point2, Point3, point, dpoint

Value types with a defaulted lexicographic `<`, `==`, unary `-`, and compound and binary `+ - * /`. Unary minus and `perp` convert back to `T` explicitly. For `int8_t` and `int16_t`, the negated value must therefore fit `T`; this is the same contract as other vector arithmetic. Integral `/` truncates toward zero and asserts `k != 0`; quotients must fit `T`. `cast<U>()` requires values in `U`'s range, and floating destinations may round. `point` is `Point2<lng>` and `dpoint` is `Point2<long double>`.

### dot, cross, perp, norm2, dist2, orient, triple, tetraVolume6

These return exact `lll` results for integral `T` (a `Point3<lll>` for the 3D `cross`) and `long double` results otherwise. `perp` rotates counterclockwise by a quarter turn and keeps `T`. `orient(a, b, c)` returns the sign of `cross(b - a, c - a)`, where `+1` means counterclockwise. `triple(a, b, c) = a · (b × c)`. `tetraVolume6` is six times the oriented volume of `abcd`.

Correctness: these are the standard determinant and scalar-product identities, with promotion before any subtraction or multiplication. Fixed-size point and metric operations take constant time and storage.

### normApprox, distanceApprox, unitApprox

Rounded square roots of the exact or long-double squared quantities. `unitApprox` (2D and 3D) returns `v / |v|` as a long-double point and asserts a nonzero norm.

### argApprox, angleApprox, signedAngleApprox

- `argApprox(v) = atan2(y, x)`, in `(-pi, pi]`; the negative x-axis gives `+pi`, even when `y` is a floating negative zero.
- `angleApprox(a, b) = atan2(|a × b|, a · b)`, in `[0, pi]`, for both 2D and 3D. The 3D form converts the exact cross product to long double before taking the norm, so it cannot overflow `lll`.
- `signedAngleApprox(a, b) = atan2(a × b, a · b)`, in `(-pi, pi]`. It is positive counterclockwise and returns `+pi` for opposite vectors. Adding `+0` turns a floating `-0` cross product into `+0`, so floating opposite vectors such as `(-1, 0), (1, 0)` also give `+pi`.
- All three assert nonzero vectors. The atan2 form keeps small angles accurate, unlike `acos` of a normalized dot product.

Correctness: the angle functions use `atan2(sin-proportional, cos-proportional)`, which is accurate at every angle. `acos` and `asin` lose accuracy near 0 and `pi`.

### polarLess

Integral only. It is a strict weak order by angle in `[0, 2 * pi)` starting at `+x`. The zero vector comes before every nonzero vector, and equal angles are ordered by squared length.

Correctness: polar sorting splits directions into two half-planes and compares exact determinant signs within each half; squared length breaks equal-direction ties, with the zero vector first. This gives a strict weak order without computing an angle.

### canonicalDirection

Integral `T`; each coordinate must be above the minimum of `T` (asserted). The result is the input divided by `gcd(|x|, |y|)`, with its sign flipped so that `y > 0`, or `y == 0` and `x > 0`. Two vectors get equal keys exactly when they are parallel or antiparallel, so a line direction and its opposite share one key. `(0, 0)` maps to itself.

Correctness: dividing by the gcd leaves a primitive vector. Two primitive vectors are parallel exactly when one is `±` the other, and the sign rule picks one representative per pair. It takes `O(log(C))` Euclidean steps.

## Feature-to-test map

`96-Local Testing/03-Geometry/01-point_tester.py` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them. The entry resolves paths from its own location.

| Operation | Test | Oracle |
|---|---|---|
| `Point2`, `Point3`: construction, casts, arithmetic and mutation | `01-point_tester` for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double`; `operators<int8_t>`/`operators<int16_t>`; narrowing compiled as an error (`-Wnarrowing`) | Exact Python integer records |
| Equality and lexicographic order | `01-point_tester` | Exact Python integer records |
| `point`, `dpoint` | `static_assert` of both alias identities | Type identity |
| `dot`, `cross`, `norm2`, `dist2`, `normApprox`, `distanceApprox`, `orient`, `perp`, `triple`, `tetraVolume6` | `01-point_tester` | Exact Python integer records; Leibniz homogeneous determinants |
| `polarLess` | Zero vectors and equal-angle ties | Exact Python integer records |
| `canonicalDirection` | Exhaustive on `[-8, 8]^2` in full mode (`[-12, 12]^2` in stress), scaled primitives, random 10^18 values (primitivity also checked), and the `int`, `lng`, `lll`, `int8_t` and `int16_t` limits | A parallel-class oracle that uses `cross == 0` and a brute-force divisor search instead of gcd |
| `unitApprox` (2D, 3D) | `01-point_tester` | Exact Python integer records |
| `argApprox`, `angleApprox` (2D, 3D), `signedAngleApprox` | The small lattice, random 10^9 coordinates, and straight, quarter, near-parallel (cross `-1`) and `1e-300` angles; regressions for `(-1, 0), (1, 0)` and negative-zero inputs | Trigonometric identities (cosine and sine of the angle times both norms equal dot and cross) and agreement of `argApprox` with the exact `polarLess` order |
| Preconditions | 8 assertion probes: division by zero, zero vectors for each angle/unit form, `INT_MIN` direction | Expected assertion failure in a subprocess |

Finite tests supplement the arguments above; they do not prove correctness for all inputs. Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

### 2026-09-27 (original P007)

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (`20260810`), Python 3.14.7, GNU++20. GCC 14 was unavailable. Full mode with seed `20260927` passed every configuration with 242,646 checks. Stress with seed `42` (optimized) also passed. The 2026-10-07 baseline rerun before the re-audit edits reproduced exactly this check count.

### 2026-10-07 re-audit

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable, so these are GCC 16 runs; no GCC 14 run is claimed. No online submission was made.

| Entry | Full, seed `20260927` (optimized, checked, ASan/UBSan) | Stress, seed `42`, optimized |
|---|---|---|
| `01-point_tester.py` | PASS, 884,220 checks per configuration, 8 assertion probes | PASS, 3,949,165 checks |

Point stress and every full run were repeated after the review fixes. Also run (package-wide):

- The sanitized integration (102 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) passed.
- A two-translation-unit smoke that mixes all four GE01 headers passed.
- A warning sweep instantiated every operation for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double` under `-Wall -Wextra -Wconversion`. The header produced no warnings. The shared geometry runner still compiles without warning flags, so this explicit sweep is the warning evidence.
- The brace and comment-cap check and repository consistency report no errors.

```bash
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

The re-audit compared every operation in the row against the code and tests before making changes.

| # | Finding | Disposition |
|---|---|---|
| 1 | `canonicalDirection`, `angleApprox`, `signedAngleApprox` absent | Implemented (plus `unitApprox`, `argApprox`), with an exhaustive parallel-class oracle and identity oracles |
| 3, 4 | Evidence claimed no gaps while rows were partial | Superseded: every operation is implemented and mapped above; status and handoff text rewritten |
| 5 | `point`/`dpoint` untested | `static_assert` of both alias identities in the point tester |
| 6 | Unary minus and `perp` narrow for `int8_t`/`int16_t` | `T(-x)` conversions (in range under the arithmetic contract). The tester compiles with `-Wnarrowing` as an error and runs `operators<int8_t>`/`operators<int16_t>` |
| 7 | Free functions lack complexity comments | Every function sits under a complexity line, one per group of functions with the same bound (the restyle convention, as in `08-monotone_stack.hpp`). One comment per function would exceed the 8% comment cap. Contracts moved to `## Contracts` |
| 10 | `; }` closings | Header, C++ tester and the hull benchmark normalized; `03-consistency.py --braces` is clean |

Other changes: the multi-line contract comment blocks moved out of the header into `## Contracts`, and the header meets the comment cap. No public name or documented behavior changed.

Independent review (`@reviewer`, 2026-10-07) confirmed one defect in this header, now fixed: `signedAngleApprox` returned `-pi` for floating opposite vectors, because the cross product evaluated to `-0`. Adding `+0.L` to the cross (and to `y` in `argApprox`) fixed it, with regressions for `(-1, 0), (1, 0)` and negative-zero inputs. Following the reviewer's notes, qualified `std::abs` became unqualified `abs` (on the allowed list), and the random `canonicalDirection` cases now also check primitivity.

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The 2026-10-07 completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (point and angle) and p. 20 (Point3D, PolyhedronVolume). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/point.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their 2D vector arithmetic and dot and cross products, with explicit widening contracts. Historical names (`Point`, `PointD`, `sgn`) are not compatibility APIs. All legacy bytes remain untouched.

## Limits and handoffs

No failing reproducer and no unfinished owned operation. Adaptive and exact floating predicates belong to P128 / GE11.
