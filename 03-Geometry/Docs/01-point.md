# 01-point.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#ge01-numeric-policy-p007) for the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### Point2, Point3, point, dpoint

Value types with a defaulted lexicographic `<`, `==`, unary `-`, and compound and binary `+ - * /`. Unary minus and `perp` convert back to `T` explicitly. For `int8_t` and `int16_t`, the negated value must therefore fit `T`; this is the same contract as other vector arithmetic. Integral `/` truncates toward zero and asserts `k != 0`; quotients must fit `T`. `cast<U>()` requires values in `U`'s range, and floating destinations may round. `point` is `Point2<lng>` and `dpoint` is `Point2<long double>`.

Streams (user decision 2026-11-12): `os << p` writes the coordinates separated by single spaces, each through unary `+`, so `int8_t` prints as a number; the stream's own format flags and precision apply. `is >> p` reads the coordinates in order, `int8_t` through `int`, and assigns `p` only when every coordinate was read and fits `T`: after a failed extraction, or an `int8_t` value out of range (failbit set), `p` keeps its prior value. Each operator exists only when the stream has that operation for `T` (not for `lll` or `__float128`), so a requires-check such as the debug header's sees no `<<` there.

### dot, cross, perp, norm2, dist2, orient, triple, tetraVolume6

These return exact `lll` results for integral `T` (a `Point3<lll>` for the 3D `cross`) and `long double` results otherwise. `perp` rotates counterclockwise by a quarter turn and keeps `T`; `rotateCW(a) = -perp(a) = (a.y, -a.x)` is the clockwise quarter turn (user decision 2026-11-12); as a composition of `perp` and unary minus it needs both `-a.x` and `-a.y` to fit `T`. `orient(a, b, c)` returns the sign of `cross(b - a, c - a)`, where `+1` means counterclockwise. `triple(a, b, c) = a · (b × c)`. `tetraVolume6` is six times the oriented volume of `abcd`.

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

Correctness: polar sorting splits directions into two half-planes and compares exact determinant signs within each half; squared length breaks equal-direction ties, with the zero vector first. The zero vector needs no branch: it is in the upper half, its cross product with every vector is 0, and its squared length 0 is the least (P007 removed the former zero test, 2026-11-12). This gives a strict weak order without computing an angle.

### canonicalDirection

Integral `T`; each coordinate must be above the minimum of `T` (asserted). The result is the input divided by `gcd(|x|, |y|)`, with its sign flipped so that `y > 0`, or `y == 0` and `x > 0`. Two vectors get equal keys exactly when they are parallel or antiparallel, so a line direction and its opposite share one key. `(0, 0)` maps to itself.

Correctness: dividing by the gcd leaves a primitive vector. Two primitive vectors are parallel exactly when one is `±` the other, and the sign rule picks one representative per pair. It takes `O(log(C))` Euclidean steps.

### argumentSort

Integral only. It takes the points by value and returns them sorted by argument in `(-pi, pi]`, which is Library Checker's `sort_points_by_argument` order. Angles in `(-pi, 0)` come first, then angle 0, then `(0, pi]`. The zero vector counts as angle 0 and comes before every `(x > 0, 0)`. Equal angles are ordered by squared length, so the output sequence is unique up to equal points. Cost: `O(n * log(n))` time and the `O(n)` returned copy.

Correctness: `polarLess` orders `[0, 2 * pi)` with the zero vector first. In that order the points with `y < 0` (angles in `(pi, 2 * pi)`) form a contiguous suffix, and they are exactly the points with angle in `(-pi, 0)`. Rotating that suffix to the front therefore gives the `(-pi, pi]` order and keeps every tie rule. This is a composition of `polarLess`.

### argumentSortApprox

Any `T`, floating included (user decision 2026-11-12): wider than `argumentSort`, which is integral only. It returns a sorted copy of `p` under the key `(argApprox(q), norm2(q), index)`, with key angle 0 for the zero vector (`argApprox` asserts a nonzero vector). The order is by rounded argument in `(-pi, pi]`, so the negative x-axis comes last and a `-0.0` coordinate changes nothing (`argApprox` adds `+0`). Ties on the rounded argument go by squared length, then by input position; points whose arguments differ by less than the long double rounding of `atan2` may be swapped. For integral `|coordinate| <= 10^4` the angular blocks equal `argumentSort`'s (tested), and on the tested toolchain exact multiples of one direction received identical keys, so they ran by length (an empirical property, not a promise). Cost: `O(n * log(n))` time, `O(n)` keys plus the result.

### scalarProjectionApprox, normalApprox

- `scalarProjectionApprox(a, b) = (a · b) / |b|` is the signed length of `a` projected on `b`, in 2D and 3D (cp-algorithms `proj`). It asserts a positive rounded norm of `b`, so a nonzero floating `b` whose norm underflows also fails.
- `normalApprox(a)` is the unit left normal `unitApprox(perp(a))` in 2D; `-a.y` must fit `T`.
- `normalApprox(a, b)` is the unit vector of `a × b` in 3D. It converts the exact cross product to long double before normalizing, so it cannot overflow `lll`.
- All three assert a positive rounded norm (the normals through `unitApprox`) and are rounded long double.

### Angle

`Angle<T>` takes integral signed `T` (default `lng`). It stores a direction `v` (nonzero, asserted by the explicit constructor `Angle(v, t = 0)`) and a turn count `t`. Its total angle is `arg(v)` in `[0, 2 * pi)` plus `2 * pi * t`. This is KACTL's `Angle`, with the direction held as a `Point2<T>`.

- `half()` is 0 for `arg(v)` in `[0, pi)` and 1 for `[pi, 2 * pi)`.
- `t90()`, `t180()` and `t360()` add a quarter, a half and a full turn. The rotation wraps past `2 * pi` exactly when `arg(v) >= 3 * pi / 2` (respectively `>= pi`, always), and the turn count carries it.
- `operator<` compares total angles: turn count, then half, then the exact cross sign. Directions with the same angle but different lengths are equivalent. It is a strict weak order.
- `a + b` is the turn-tracked angle of the point `a.v + b`, the total angle within `(a - pi, a + pi)`. Its precondition, that the segment from `a.v` to `a.v + b` avoids the origin, is asserted as `cross(a.v, b) != 0 || dot(a.v, a.v + b) > 0` (probes `angle-step-zero`, `angle-step-through-origin`, `angle-step-back-through-origin`). `a - b` is `a + (-b)`. Vector sums must fit `T`, and turn counts must fit `lng`.
- Unlike KACTL, `operator-` is not the untracked `Angle - Angle` vector difference. That is the construction `Angle(a.v - b.v, a.t)`. Moving by a vector, tracked like `+`, is the kept meaning (user decision 2026-11-12).

Correctness of `+`: moving along a segment that avoids the origin changes the true angle by `delta` in `(-pi, pi)`. The raw result `Angle(a.v + b, a.t)` differs from the truth by `-2 * pi`, `0` or `2 * pi`. It is too large exactly when it exceeds `a + pi`, and too small exactly when it is below `a - pi`, which are the two corrections.

### angleDiff, segmentAngles

- `angleDiff(a, b)` returns `Angle<lll>` for integral `T` (`Wide<T>`). Its direction is `(a.v · b.v, a.v × b.v)`, which has angle `arg(b.v) - arg(a.v)` mod `2 * pi`. Its turn count is `b.t - a.t`, minus one when `arg(b.v) < arg(a.v)`. The total is therefore exactly `total(b) - total(a)` and may be negative. The products must fit `lll`, which `|coordinate| <= 10^9` guarantees, and so do the comparisons on the result.
- `segmentAngles(a, b)` asserts `a.t == b.t` (kept by user decision 2026-11-12: KACTL's form takes plain directions, and differing turn counts have no single swept arc). It returns `(lo, hi)` with `lo <= hi` and `hi - lo <= pi`: the arc of directions swept by the segment between the points `a.v` and `b.v`. If the sorted pair is less than half a turn apart, the result is `(a, b)`; otherwise it is `(b, a.t360())`. `hi - lo == pi` exactly when the origin lies on the segment, and either arc is then returned.

## Feature-to-test map

`96-Local Testing/03-Geometry/01-point_tester.py` runs the geometry runner's optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds ([`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>)). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

| Operation | Test | Oracle |
|---|---|---|
| `Point2`, `Point3`: construction, casts, arithmetic and mutation | `01-point_tester` for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double`; `operators<int8_t>`/`operators<int16_t>`; narrowing compiled as an error (`-Wnarrowing`) | Exact Python integer records |
| Equality and lexicographic order | `01-point_tester` | Exact Python integer records |
| `Point2`, `Point3`: `<<`, `>>` | `streams()` for `lng`, `int`, `int16_t`, `int8_t` (numeric, not characters), `Point3`, long double at `setprecision(21)` over the whole exponent range with signed zeros, default-precision text, mixed whitespace, several points from one stream; `failedRead` on empty, malformed, partial, overflowing and already-failed input; `Writable`/`Readable` concepts reject `lll` and `__float128` and accept the rest | `std::to_string` and `snprintf("%.21Lg")` text, bitwise round trip, failbit plus unchanged prior value |
| `point`, `dpoint` | `static_assert` of both alias identities | Type identity |
| `dot`, `cross`, `norm2`, `dist2`, `normApprox`, `distanceApprox`, `orient`, `perp`, `triple`, `tetraVolume6` | `01-point_tester` | Exact Python integer records; Leibniz homogeneous determinants |
| `rotateCW` | `clockwise()` over `int8_t`, `int`, `lng`, `lll`, `float`, `double`, long double: lattice, signed zeros, random to 10^18 and 2^62 | `(y, -x)` computed directly with zero signs; `cross` equals the exact squared length, `dot` 0; composition with `perp`, two and four turns |
| `polarLess` | Zero vectors and equal-angle ties | Exact Python integer records |
| `canonicalDirection` | Exhaustive on `[-8, 8]^2` in full mode (`[-12, 12]^2` in stress), scaled primitives, random 10^18 values (primitivity also checked), and the `int`, `lng`, `lll`, `int8_t` and `int16_t` limits | A parallel-class oracle that uses `cross == 0` and a brute-force divisor search instead of gcd |
| `unitApprox` (2D, 3D) | `01-point_tester` | Exact Python integer records |
| `argApprox`, `angleApprox` (2D, 3D), `signedAngleApprox` | The small lattice, random 10^9 coordinates, and straight, quarter, near-parallel (cross `-1`) and `1e-300` angles; regressions for `(-1, 0), (1, 0)` and negative-zero inputs | Trigonometric identities (cosine and sine of the angle times both norms equal dot and cross) and agreement of `argApprox` with the exact `polarLess` order |
| `argumentSort` | `argumentOrder()`: the whole lattice doubled and shuffled, empty, singleton and all-zero inputs, a hand-ordered sample, random sets for `int8_t` (with -128), `int16_t`, `int` and `lng` (±3, 10^9, 10^18), collinear rays | An exact key written apart from `polarLess` (group, cross, length), itself checked against `atan2` on the lattice |
| `argumentSortApprox` | `approxOrder()`: lattice, shuffled doubles, empty and zero inputs, the Library Checker sample, `int8_t` extremes, random `int8_t` to `lng`, every multiple of each primitive direction up to 80 inside 10^4 (stress); `approxFloatingCase` for `dpoint` and `Point2<double>` with signed zeros, power-of-two scalings and duplicates | Exact `argumentSort` direction-class sequence for `\|coordinate\| <= 10^4` and the exact multiples order; Python `math.atan2` keys with explicit signed-zero axes, `Fraction` direction groups and exact norm ranks (order checked where keys differ by more than 1e-12); permutation; `length-inversions=0` counted (empirical on this toolchain) |
| `Angle`: `v`, `t`, ctor, `half`, `t90`, `t180`, `t360`, `<`, `+`, `-` | `turnAngles()`: exhaustive lattices for `int8_t`, `int16_t`, `int`, `lng` over `t` in {-1, 0, 1}; random 10^9 and 10^18 `lng`, full-range `int`, turn counts to ±10^6; random closed lattice walks | An exact (turn, quadrant, cross) model with its own half-turn rule, checked against `atan2`; rotation identities (`t90` four times = `t180` twice = `t360`); walk turn counts equal a crossing-number winding (-3 to 3) |
| `angleDiff` | `turnAngles()` | Complex product rotates `a` onto `b`; exact turn carry; `atan2` totals |
| `segmentAngles` | `turnAngles()` | Nine sampled segment points inside the arc; arc at most `pi`, exactly `pi` only with the origin on the segment |
| `scalarProjectionApprox`, `normalApprox` (2D, 3D) | `projections()`: exhaustive small `int`/`int8_t` inputs, random 10^9 `lng` (near-parallel included), `int8_t`, `int16_t`, `float`, `double`, `long double` | `hypot` and exact `lll` dot and cross: unit length, orthogonality, orientation sign (3D triple product) |
| Preconditions | 18 assertion probes: division by zero, zero vectors for each angle/unit/normal/projection form, `INT_MIN` direction, zero `Angle` direction (constructor and `+` step), `+` and `-` steps through the origin, `segmentAngles` with different turn counts, parallel 3D normal | Expected assertion failure in a subprocess |

Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 841 MB; optimized 0.1 s compile + 0.6 s run, checked 26.7 s compile + 0.7 s run, ASan-UBSan 43.9 s compile + 5.7 s run; MEMORY peak 1033 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 905 MB; optimized 0.2 s compile + 0.7 s run, checked 39.2 s compile + 0.7 s run, ASan-UBSan 55.8 s compile + 4.4 s run; MEMORY peak 1064 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 37.6 s compile + 0.2 s run, checked 21.5 s compile + 0.7 s run; MEMORY peak 850 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 843 MB; optimized 0.1 s compile + 0.8 s run, checked 10.0 s compile + 1.0 s run, ASan-UBSan 17.6 s compile + 6.9 s run; MEMORY peak 653 MB. Dated rows below stay those commands' latest results.

2026-11-12 (/finish P007, experiment: none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14, GNU++20, seed 42, optimized, checked and ASan-UBSan configurations; the 18 `03-Geometry` suites ran in sequence on the final headers and testers, with the mutation campaigns finished and the benchmark not running.

| Command | Result |
|---|---|
| `01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` (GCC 16.2.1) | PASS, 6,001,969 checks per configuration, 18 assertion probes; pre-flight quick peak 61 MB; `MEMORY peak=696MB` |
| `CXX=g++-14 01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` | PASS, 6,001,969 checks per configuration, 18 probes; pre-flight quick peak 1130 MB; `MEMORY peak=1692MB` |
| `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 23,982,603 checks per configuration; `MEMORY peak=1174MB` |
| `05-extraction.py ops --filter 03-Geometry/01-point.hpp` and `05-extraction.py testers --filter '96-Local Testing/03-Geometry/01-point_tester.py'` | PASS, 68 operations on g++ and g++-14 |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3241MB` |

```bash
python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/01-point.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter '96-Local Testing/03-Geometry/01-point_tester.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

The shared contest runner compiles with `-Wall -Wextra -Wconversion -Werror`.

## Mutation testing

Record: mewt 4.0.0 on blob `bf747164b9a1e3e5457597116fb14723bc9dee7b` (2026-11-12, seed 1, 24 lines changed since blob `b05f7bdfc05b`): 106 mutants, 103 caught (0 by timeout), 3 survived, 0 untested.

Stage seconds of that campaign (the stream operators, `rotateCW`, `argumentSortApprox`; /finish P007), cumulative over the retest of the same workdir: quick 455, full-optimized 15, full-rest 307. The first pass left 10 survivors; seven were `int8_t` range checks in `>>` (`T(x) == x` → `<=` or `>=` on lines 35 and 65) that the tests hit in one direction only, and out-of-range reads below and above the range on every coordinate were added (`failedRead`). The 3 left are equivalent: `COS 647`, `COS 692` (`sizeof(T) == 1` → `<= 1`, lines 33 and 63; every size is at least 1) and `COS 781` (`i < n` → `i != n` in `argumentSortApprox`'s key loop, line 140).

The intermediate campaign on blob `b05f7bdfc05b` (2 lines changed since `0a67a7169d5e`): 15 mutants, 14 caught, 1 survived. Stage seconds of that campaign (the removed zero branch of `polarLess`): quick 46, full-optimized 4, full-rest 70. Its survivor, `COS 683` (`half(a) < half(b)` → `<=`, line 114 of that blob), runs only under `half(a) != half(b)`. A scratch check of `COS 584` below on the final header (`p.x < 0` → `<=` in `half`) fails the tester's small-lattice angular oracle (`a=(-2,1) b=(0,0)`), so the removal made it detectable.

Earlier campaign: mewt 4.0.0 on blob `0a67a7169d5eed29d76907a1dc36940157b0e212` (2026-11-11, seed 1, whole file, uniform sample of 844): 500 mutants, 475 caught (0 by timeout), 25 survived, 0 untested.

P007's first campaign (stages quick 913 s, full-optimized 31 s, full-rest 576 s, after the P006 campaign released the lock) left 27 survivors. Division by a negative scalar (2D and 3D) was added to the tester and caught `COS 605` and `COS 635` (`k != 0` → `k > 0` in `/=`) on the retest of the same workdir (cumulative 1001, 42, 920 s). The dead zero branch of `polarLess` was then removed (follow-up on the changed lines below). Line numbers are those of the recorded blob.

| Mutants | Line | Why they survive |
|---|---|---|
| `COS 579` (`p.y == 0` → `<=`) | 7 | After `p.y < 0 \|\|`, the two agree. |
| `COS 584` (`p.x < 0` → `<=`) | 7 | Differs only at the zero vector, which no caller of `half` passed on this blob (`polarLess` returned first, `Angle` asserts `v != 0`). The follow-up removed that branch, so `polarLess` now reaches `half(0)` and its zero-first cases decide it. |
| `LOS 548` (`&&` → `\|\|` in the `static_assert`) | 13 | Compile time only: it admits more types (unsigned `T`) and changes no compiled program. |
| `COS 614`, `COS 624` (`<` → `<=` in `Point3::operator<`) | 43 | Each comparison runs only under its `!=` guard. |
| `AS 229`, `AS 233`, `AS 236`, `AS 237`, `AS 239`, `AS 259`, `AS 261`, `AS 264` | 81, 89, 98, 101, 139, 149, 159 | Swapped arguments of symmetric functions: `dot`, `dist2`, `gcd`, and `cross` under `abs` or a 3D norm. In `angleDiff` (149) the surviving swap is the `dot`; swapping the `cross` reflects the result and is caught. |
| `CR 172` (`assert(n > 0)` removed in `unitApprox`) | 91 | The division by `n` asserts `k != 0` in `operator/=`, and `n >= 0`, so every zero vector still aborts (probes `unit-zero`, `normal-zero`, `normal3-zero`, `normal3-parallel`); only a NaN input, outside the finite domain, differs. |
| `IF 215`, `LOS 554` (zero branch of `polarLess`) | 114 | Dead code: the general path already orders the zero vector first (Contracts, polarLess). Removed. |
| `COS 709`, `COS 716`, `COS 744` | 115, 117, 136 | `<` → `<=` or `>` → `>=` under a guard that makes the operands unequal (`half(a) != half(b)`, `c != 0`, `a.t != b.t`). |
| `COS 771` (`> 0` → `>= 0` in the `Angle::operator+` assert) | 139 | Equality needs `cross(v, b) == 0` and `dot(v, v + b) == 0`, so `v + b == 0`, which the constructor's assert rejects (probe `angle-step-zero`). |
| `COS 805` (`a.x != min` → `a.x > min`) | 158 | Equal for every value of `T`. |
| `CR 212`, `COS 811` (the `canonicalDirection` minimum assert removed or weakened) | 158 | Not equivalent: without it `std::gcd` overflows on `T`'s minimum. The checked build that runs the probes (`-D_GLIBCXX_DEBUG`) asserts the same input inside `std::gcd`, and the runner accepts any assertion, so probe `canonical-minimum` cannot tell them apart; judges, without libstdc++ assertions, rely on this assert. |
| `COS 819`, `COS 824` | 160 | `a.y == 0` → `<=` agrees after `a.y < 0 \|\|`; `a.x < 0` → `<=` differs only at the zero vector, where `-a == a`. |

## Sources

The point operations were written from the geometric identities; `Angle`, `angleDiff` and `segmentAngles` are adapted from KACTL's `Angle` (printed p. 17), with an exact turn correction for `+` and a vector-move `-`. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). The P007 re-audit sources are in [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11) and its decisions in [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (point and angle) and p. 20 (Point3D, PolyhedronVolume). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/point.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their 2D vector arithmetic and dot and cross products, with explicit widening contracts. Historical names (`Point`, `PointD`, `sgn`) are not compatibility APIs. All legacy bytes remain untouched.

## Limits and handoffs

Adaptive and exact floating predicates belong to P128 / GE11.

## Aliases

Search aliases (P263, grab --find): basic geometry, vector, cross product, argument sort; Library Checker `sort_points_by_argument`; 距离, 内积和外积, 二维计算几何基础, 三维计算几何基础, 叉积, 点积, 极角排序, 叉乘, 点乘, 混合积.
Aliases (P007): angle with turn count, KACTL Angle, polar angle sort, floating polar sort, atan2 sort, normal vector, unit normal, scalar projection, clockwise rotation, rotate 90 clockwise, point input output, 法向量, 投影长度, 顺时针旋转.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 242,646 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, implemented `canonicalDirection`, `angleApprox`, `signedAngleApprox`, `unitApprox`, `argApprox`; 6 findings fixed; full suite and stress passed on g++ 16; independent review: confirmed three package defects, all fixed (here the `signedAngleApprox` negative-zero case).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename P = point>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 03-Geometry` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed. Header edit for `-Werror` without demotions: explicit `T(...)` assignments in `operator/=`; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
- 2026-11-12: P007 re-audit (GE01, started 2026-11-11): added `argumentSort`, `scalarProjectionApprox`, `normalApprox` (2D, 3D), `Angle` (`half`, `t90`, `t180`, `t360`, `+`, `-`, `<`), `angleDiff`, `segmentAngles`, with `geometry_detail::half` shared with `polarLess`; independent review: 3 findings here fixed (the `Angle` step assert with 2 probes, its proof comment moved into the body, the `scalarProjectionApprox` wording); first mutation campaign, whose survivors added division by negative scalars and removed `polarLess`'s dead zero branch; full, `CXX=g++-14` full, stress, extraction and `02-integration.py` passed.
- 2026-11-12: /finish P007: added `rotateCW`, `Point2`/`Point3` `<<` and `>>` (constrained to stream-capable `T`, range-checked `int8_t` reads, the point unchanged on failure) and `argumentSortApprox` by user decision; the angleDiff bound and KACTL provenance fixed; 2 review defects and 1 tester-found defect fixed; changed-lines mutation campaign (7 gaps closed); full suite passed on g++ and g++-14, stress passed.
