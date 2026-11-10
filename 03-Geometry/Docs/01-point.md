# 01-point.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#ge01-numeric-policy-p007) for the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

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

`96-Local Testing/03-Geometry/01-point_tester.py` runs the geometry runner's optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds ([`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>)). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

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

Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 841 MB; optimized 0.1 s compile + 0.6 s run, checked 26.7 s compile + 0.7 s run, ASan-UBSan 43.9 s compile + 5.7 s run; MEMORY peak 1033 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 905 MB; optimized 0.2 s compile + 0.7 s run, checked 39.2 s compile + 0.7 s run, ASan-UBSan 55.8 s compile + 4.4 s run; MEMORY peak 1064 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 37.6 s compile + 0.2 s run, checked 21.5 s compile + 0.7 s run; MEMORY peak 850 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 843 MB; optimized 0.1 s compile + 0.8 s run, checked 10.0 s compile + 1.0 s run, ASan-UBSan 17.6 s compile + 6.9 s run; MEMORY peak 653 MB. Dated rows below stay those commands' latest results.

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20; the floor-check row ran on 2026-10-14 with GCC 14.4.1.

| Command | Result |
|---|---|
| `01-point_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 884,220 checks per configuration, 8 assertion probes |
| `CXX=g++-14 01-point_tester.py --mode full --seed 20261014` (GCC 14.4.1, 2026-10-14) | PASS, 884,185 checks per configuration; 8 assertion probes; `MEMORY peak=610MB` |
| `01-point_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 3,949,165 checks |
| `02-integration.py --sanitizers` (102 headers, scalar and AVX2 multi-TU builds, workspace) | PASS |
| Two-translation-unit smoke mixing all four GE01 headers | PASS |
| Warning sweep, every operation for all eight `T`, `-Wall -Wextra -Wconversion` | No warnings |
| `03-consistency.py --braces ...` and `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

The shared contest runner compiles with `-Wall -Wextra -Wconversion -Werror`.

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (point and angle) and p. 20 (Point3D, PolyhedronVolume). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/point.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their 2D vector arithmetic and dot and cross products, with explicit widening contracts. Historical names (`Point`, `PointD`, `sgn`) are not compatibility APIs. All legacy bytes remain untouched.

## Limits and handoffs

Adaptive and exact floating predicates belong to P128 / GE11.

## Aliases

Search aliases (P263, grab --find): basic geometry, vector, cross product, argument sort; Library Checker `sort_points_by_argument`; 距离, 内积和外积, 二维计算几何基础, 三维计算几何基础, 叉积, 点积, 极角排序, 叉乘, 点乘, 混合积.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 242,646 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, implemented `canonicalDirection`, `angleApprox`, `signedAngleApprox`, `unitApprox`, `argApprox`; 6 findings fixed; full suite and stress passed on g++ 16; independent review: confirmed three package defects, all fixed (here the `signedAngleApprox` negative-zero case).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename P = point>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 03-Geometry` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed. Header edit for `-Werror` without demotions: explicit `T(...)` assignments in `operator/=`; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
