# 01-point.hpp — Evidence

Owned by package P007 / GE01 (geometry foundations, with `02-line_segment.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`). Every operation in the row has a test.

## Contracts

### Numeric Policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### Point2, Point3, Point, Dpoint

Value types with a defaulted lexicographic `<`, `==`, unary `-`, and compound and binary `+ - * /`. Unary minus and `perp` convert back to `T` explicitly, so for `int8_t` and `int16_t` the negated value must fit `T`, as for all vector arithmetic. Integral `/` truncates toward zero and asserts `k != 0`; quotients must fit `T`. `cast<U>()` requires values in `U`'s range, and floating destinations may round. `point` is `Point2<lng>` and `dpoint` is `Point2<long double>`.

Streams (user decision 2026-11-12): `os << p` writes the coordinates separated by single spaces, each through unary `+`, so `int8_t` prints as a number; the stream's format flags and precision apply. `is >> p` reads the coordinates in order, `int8_t` through `int`, and assigns `p` only when every coordinate was read and fits `T`. After a failed extraction, or an `int8_t` value out of range (failbit set), `p` keeps its prior value. Each operator exists only when the stream has that operation for `T` (not for `lll` or `__float128`), so a requires-check such as the debug header's sees no `<<` there.

### dot, cross, perp, norm2, dist2, orient, triple, tetraVolume6

These return exact `lll` results for integral `T` (a `Point3<lll>` for the 3D `cross`) and `long double` otherwise. `perp` rotates counterclockwise by a quarter turn and keeps `T`. `rotateCW(a) = -perp(a) = (a.y, -a.x)` is the clockwise quarter turn (user decision 2026-11-12); as a composition of `perp` and unary minus it needs both `-a.x` and `-a.y` to fit `T`. `orient(a, b, c)` is the sign of `cross(b - a, c - a)`, `+1` meaning counterclockwise. `triple(a, b, c) = a · (b × c)`. `tetraVolume6` is six times the oriented volume of `abcd`.

Correctness: these are the standard determinant and scalar-product identities, with promotion before any subtraction or multiplication. Each takes constant time and storage.

### normApprox, distanceApprox, unitApprox

Rounded square roots of the exact or long-double squared quantities. `unitApprox` (2D and 3D) returns `v / |v|` as a long-double point and asserts a nonzero norm.

### argApprox, angleApprox, signedAngleApprox

- `argApprox(v) = atan2(y, x)`, in `(-pi, pi]`; the negative x-axis gives `+pi`, even when `y` is a floating negative zero.
- `angleApprox(a, b) = atan2(|a × b|, a · b)`, in `[0, pi]`, in 2D and 3D. The 3D form converts the exact cross product to long double before taking the norm, so it cannot overflow `lll`.
- `signedAngleApprox(a, b) = atan2(a × b, a · b)`, in `(-pi, pi]`, positive counterclockwise, `+pi` for opposite vectors. Adding `+0` turns a floating `-0` cross product into `+0`, so floating opposite vectors such as `(-1, 0), (1, 0)` also give `+pi`.
- All three assert nonzero vectors. The two angle forms also assert that both arguments they pass to `atan2` are finite (P009 /finish, user decision 2026-11-15; extended to the 3D form and `signedAngleApprox`, user decision 2026-11-16): the dot product, and the cross product, which the 3D form passes as `normApprox` of the cross vector. The 3D form therefore also asserts when that norm's square overflows (`abs(a x b)` above about `1.1e2466` while each component is still finite). A floating input whose products overflow is a precondition violation, not a finite wrong angle.

Correctness: `atan2(sin-proportional, cos-proportional)` is accurate at every angle, while `acos` and `asin` of a normalized product lose accuracy near 0 and `pi`.

### polarLess

Integral only. A strict weak order by angle in `[0, 2 * pi)` starting at `+x`. The zero vector comes before every nonzero vector, and equal angles are ordered by squared length.

Correctness: directions split into two half-planes, and exact determinant signs compare within each half; squared length breaks equal-direction ties. The zero vector needs no branch: it is in the upper half, its cross product with every vector is 0, and its squared length 0 is the least (P007 removed the former zero test, 2026-11-12). No angle is computed.

### canonicalDirection

Integral `T`; each coordinate must be above the minimum of `T` (asserted). The result is the input divided by `gcd(|x|, |y|)`, with its sign flipped so that `y > 0`, or `y == 0` and `x > 0`. Two vectors get equal keys exactly when they are parallel or antiparallel, so a line direction and its opposite share one key. `(0, 0)` maps to itself.

Correctness: dividing by the gcd leaves a primitive vector. Two primitive vectors are parallel exactly when one is `±` the other, and the sign rule picks one representative per pair. Cost: `O(log(C))` Euclidean steps.

### argumentSort

Integral only. Takes the points by value and returns them sorted by argument in `(-pi, pi]`, Library Checker's `sort_points_by_argument` order: angles in `(-pi, 0)`, then angle 0, then `(0, pi]`. The zero vector counts as angle 0 and comes before every `(x > 0, 0)`. Equal angles are ordered by squared length, so the output is unique up to equal points. Cost: `O(n * log(n))` time and the `O(n)` returned copy.

Correctness: in `polarLess` order the points with `y < 0` (angles in `(pi, 2 * pi)`) form a contiguous suffix, exactly the points with angle in `(-pi, 0)`. Rotating that suffix to the front gives the `(-pi, pi]` order and keeps every tie rule.

### argumentSortApprox

Any `T`, floating included (user decision 2026-11-12), so wider than the integral `argumentSort`. It returns a sorted copy of `p` under the key `(argApprox(q), norm2(q), index)`, with angle 0 for the zero vector (`argApprox` asserts a nonzero vector). The order is by rounded argument in `(-pi, pi]`: the negative x-axis comes last, and a `-0.0` coordinate changes nothing (`argApprox` adds `+0`). Ties on the rounded argument go by squared length, then input position; points whose arguments differ by less than the long double rounding of `atan2` may be swapped. For integral `|coordinate| <= 10^4` the angular blocks equal `argumentSort`'s (tested). On the tested toolchain exact multiples of one direction received identical keys and so ran by length; this is an empirical property, not a promise. Cost: `O(n * log(n))` time, `O(n)` keys plus the result.

### scalarProjectionApprox, normalApprox

- `scalarProjectionApprox(a, b) = (a · b) / |b|`, the signed length of `a` projected on `b`, in 2D and 3D (cp-algorithms `proj`). It asserts a positive rounded norm of `b`, so a nonzero floating `b` whose norm underflows also fails.
- `normalApprox(a)` is the unit left normal `unitApprox(perp(a))` in 2D; `-a.y` must fit `T`.
- `normalApprox(a, b)` is the unit vector of `a × b` in 3D. It converts the exact cross product to long double before normalizing, so it cannot overflow `lll`.
- All three assert a positive rounded norm (the normals through `unitApprox`) and return rounded long double.

### Angle

`Angle<T>` takes integral signed `T` (default `lng`). It stores a direction `v`, nonzero as the explicit constructor `Angle(v, t = 0)` asserts, and a turn count `t`; its total angle is `arg(v)` in `[0, 2 * pi)` plus `2 * pi * t`. This is KACTL's `Angle` with the direction held as a `Point2<T>`.

- `half()` is 0 for `arg(v)` in `[0, pi)` and 1 for `[pi, 2 * pi)`.
- `t90()`, `t180()` and `t360()` add a quarter, a half and a full turn. The rotation wraps past `2 * pi` exactly when `arg(v) >= 3 * pi / 2` (respectively `>= pi`, always), and the turn count carries it.
- `operator<` compares total angles: turn count, then half, then the exact cross sign. Directions with the same angle but different lengths are equivalent; the order is strict weak.
- `a + b` is the turn-tracked angle of the point `a.v + b`, whose total angle lies in `(a - pi, a + pi)`. Its precondition, that the segment from `a.v` to `a.v + b` avoids the origin, is asserted as `cross(a.v, b) != 0 || dot(a.v, a.v + b) > 0` (probes `angle-step-zero`, `angle-step-through-origin`, `angle-step-back-through-origin`). `a - b` is `a + (-b)`. Vector sums must fit `T`, and turn counts `lng`.
- Unlike KACTL, `operator-` is not the untracked `Angle - Angle` vector difference, which is the construction `Angle(a.v - b.v, a.t)`; moving by a vector, tracked like `+`, is the kept meaning (user decision 2026-11-12).

Correctness of `+`: moving along a segment that avoids the origin changes the true angle by some `delta` in `(-pi, pi)`. The raw result `Angle(a.v + b, a.t)` differs from the truth by `-2 * pi`, `0` or `2 * pi`: it is too large exactly when it exceeds `a + pi`, and too small exactly when it is below `a - pi`, which are the two corrections.

### angleDiff, segmentAngles

- `angleDiff(a, b)` returns `Angle<lll>` for integral `T` (`Wide<T>`). Its direction is `(a.v · b.v, a.v × b.v)`, with angle `arg(b.v) - arg(a.v)` mod `2 * pi`, and its turn count is `b.t - a.t`, minus one when `arg(b.v) < arg(a.v)`. The total is therefore exactly `total(b) - total(a)` and may be negative. The products, and the comparisons on the result, must fit `lll`, which `|coordinate| <= 10^9` guarantees.
- `segmentAngles(a, b)` asserts `a.t == b.t` (kept by user decision 2026-11-12: KACTL's form takes plain directions, and differing turn counts have no single swept arc). It returns `(lo, hi)` with `lo <= hi` and `hi - lo <= pi`, the arc of directions swept by the segment between the points `a.v` and `b.v`: `(a, b)` if the sorted pair is less than half a turn apart, else `(b, a.t360())`. `hi - lo == pi` exactly when the origin lies on the segment, and either arc is then returned.

## Feature-to-Test Map

`96-Local Testing/03-Geometry/01-point_tester.py` runs the optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds of the shared runner [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

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
| Preconditions | 22 assertion probes: division by zero, zero vectors for each angle/unit/normal/projection form, an overflowing cross product for `angleApprox` (2D, 3D) and `signedAngleApprox` and an overflowing dot product for the 2D `angleApprox`, `INT_MIN` direction, zero `Angle` direction (constructor and `+` step), `+` and `-` steps through the origin, `segmentAngles` with different turn counts, parallel 3D normal | Expected assertion failure in a subprocess |

Approximate checks use scale-aware tolerances on these fixtures; they do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and Results

Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14, GNU++20; optimized, checked and ASan-UBSan configurations.

| Date | Command | Result |
|---|---|---|
| 2026-11-15 (/finish P009, other worktree sessions running) | `01-point_tester.py --mode full --seed 20261115` (GCC 16.2.1) | PASS, 5,999,906 checks per configuration, 22 assertion probes; pre-flight quick peak 1106 MB; `MEMORY peak=1658MB` |
| 2026-11-15 | same with `CXX=g++-14` | PASS, 5,999,906 checks per configuration, 22 assertion probes; pre-flight quick peak 1124 MB; `MEMORY peak=1680MB` |
| 2026-11-15 | `05-extraction.py ops --filter 03-Geometry/01-point.hpp` and `05-extraction.py testers --filter 01-point` | PASS, 68 operations on g++ and g++-14; tester quick run on the extracted build |
| 2026-11-15 | `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3204MB` |
| 2026-11-15 | `04-mutation.py run 03-Geometry/01-point.hpp` (changed lines) | Record under Mutation Testing; `check` PASS |
| 2026-11-12 (/finish P007, seed 42, the 18 `03-Geometry` suites in sequence) | `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 23,982,603 checks per configuration; `MEMORY peak=1174MB` |

Build and run times of the P274 library-wide runs (2026-11-10; seed 42, beside the other suites, build cache emptied before each cold run; seconds as compile + run; every run passed):

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | 841 MB | 0.1 + 0.6 | 26.7 + 0.7 | 43.9 + 5.7 | 1033 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | 905 MB | 0.2 + 0.7 | 39.2 + 0.7 | 55.8 + 4.4 | 1064 MB |
| `01-run.py --mode quick` | — | 37.6 + 0.2 | 21.5 + 0.7 | — | 850 MB |
| `01-run.py --mode stress --rounds 1` | 843 MB | 0.1 + 0.8 | 10.0 + 1.0 | 17.6 + 6.9 | 653 MB |

```bash
python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20261115
CXX=g++-14 python3 '96-Local Testing/03-Geometry/01-point_tester.py' --mode full --seed 20261115
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/01-point.hpp
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/01-point.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 01-point
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `3879bc415d0804c2e1da7bc4ae30b053e400e1da` (2026-11-15, seed 1, 9 lines changed since blob `bf747164b9a1`): 37 mutants, 32 caught (0 by timeout), 5 survived, 0 untested.

Stage times: quick 108 s, full optimized 8 s, full checked and ASan/UBSan 202 s. This run covered the finite-product assertions of the three angle functions (/finish P009). The open survivors below include those of three earlier campaigns on lines this record did not cover: P007's whole-file campaign (2026-11-11, 500 of 844 mutants), its changed-lines campaign on the removed `polarLess` zero branch (2026-11-12) and the /finish P007 changed-lines campaign (2026-11-12). Lines are those of the current blob.

| Mutants | Line | Campaign | Why open |
|---|---|---|---|
| `AS 281`, `AS 282` (2D `angleApprox`), `AS 284`, `AS 285` (3D `angleApprox`), `AS 288` (`signedAngleApprox`) | 113, 118, 123 | This record | Equivalent swaps of `a, b`: `dot` sums the same products, and a swapped `cross` is its exact negation (`x - y == -(y - x)` in IEEE arithmetic, exact for integers), used only under `abs`, a 3D norm or the finiteness test. Swapping `signedAngleApprox`'s `cross` flips the sign and is caught. |
| `COS 647`, `COS 692` (`sizeof(T) == 1` → `<= 1`) | 33, 63 | /finish P007 | Equivalent: every type has size at least 1. |
| `COS 781` (`i < n` → `i != n` in `argumentSortApprox`'s key loop) | 146 | /finish P007 | Equivalent: `i` steps by one from 0. |
| `COS 683` (`half(a) < half(b)` → `<=`; `COS 709` in the whole-file campaign) | 135 | `polarLess` zero branch | Equivalent: it runs only under `half(a) != half(b)`. |
| `COS 579` (`p.y == 0` → `<=`) | 7 | Whole-file | Equivalent: after `p.y < 0 \|\|` the two agree. |
| `LOS 548` (`&&` → `\|\|` in the `static_assert`) | 13 | Whole-file | Compile time only: it admits more types (unsigned `T`) and changes no compiled program. |
| `COS 614`, `COS 624` (`<` → `<=` in `Point3::operator<`) | 50 | Whole-file | Equivalent: each comparison runs only under its `!=` guard. |
| `AS 229`, `AS 233`, `AS 236`, `AS 237`, `AS 239`, `AS 259`, `AS 261`, `AS 264` | 96, 104, 167, 177, 187 | Whole-file | Equivalent swaps of symmetric `dot`, `dist2` and `gcd` arguments. In `angleDiff` (177) the surviving swap is the `dot`; swapping the `cross` reflects the result and is caught. The swaps this campaign left on lines 113 and 118 were retested by the current record. |
| `CR 172` (`assert(n > 0)` removed in `unitApprox`) | 106 | Whole-file | Equivalent on the finite domain: the division by `n` asserts `k != 0` in `operator/=`, and `n >= 0`, so every zero vector still aborts (probes `unit-zero`, `normal-zero`, `normal3-zero`, `normal3-parallel`); only a NaN input differs. |
| `COS 716`, `COS 744` (`>` → `>=`, `<` → `<=`) | 137, 164 | Whole-file | Equivalent: a guard makes the operands unequal (`c != 0`, `a.t != b.t`). |
| `COS 771` (`> 0` → `>= 0` in the `Angle::operator+` assert) | 167 | Whole-file | Equivalent: equality needs `cross(v, b) == 0` and `dot(v, v + b) == 0`, so `v + b == 0`, which the constructor's assert rejects (probe `angle-step-zero`). |
| `COS 805` (`a.x != min` → `a.x > min`) | 186 | Whole-file | Equivalent for every value of `T`. |
| `CR 212`, `COS 811` (the `canonicalDirection` minimum assert removed or weakened) | 186 | Whole-file | Not equivalent: without the assert `std::gcd` overflows on `T`'s minimum, and judges, built without libstdc++ assertions, rely on it. The campaign's probe `canonical-minimum` could not tell them apart because the checked build (`-D_GLIBCXX_DEBUG`) asserts the same input inside `std::gcd`. That argument predates the 2026-11-12 runner fix, after which only glibc's assert counts; P007's next campaign rechecks both mutants. |
| `COS 819`, `COS 824` | 188 | Whole-file | Equivalent: `a.y == 0` → `<=` agrees after `a.y < 0 \|\|`; `a.x < 0` → `<=` differs only at the zero vector, where `-a == a`. |

## Sources

The point operations were written from the geometric identities. `Angle`, `angleDiff` and `segmentAngles` are adapted from KACTL's `Angle` (printed p. 17), with an exact turn correction for `+` and a vector-move `-`. Notebook implementations were compared, not copied as correctness guarantees. Completeness-sweep sources: [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted: [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). P007 re-audit sources: [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11); its decisions: [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (point and angle) and p. 20 (Point3D, PolyhedronVolume), read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: `OLD/Team Notebook/src/geometry/point.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp`. This header covers their 2D vector arithmetic and dot and cross products, with explicit widening contracts. Historical names (`Point`, `PointD`, `sgn`) are not compatibility APIs. The legacy bytes are untouched.

## Limits and Handoffs

Adaptive and exact floating predicates belong to P128 / GE11. P007's next mutation campaign rechecks `CR 212` and `COS 811` (Mutation Testing).

## Aliases

Search aliases (P263, grab --find): basic geometry, vector, cross product, argument sort; Library Checker `sort_points_by_argument`; 距离, 内积和外积, 二维计算几何基础, 三维计算几何基础, 叉积, 点积, 极角排序, 叉乘, 点乘, 混合积.
Aliases (P007): angle with turn count, KACTL Angle, polar angle sort, floating polar sort, atan2 sort, normal vector, unit normal, scalar projection, clockwise rotation, rotate 90 clockwise, point input output, 法向量, 投影长度, 顺时针旋转.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 242,646 checks) and stress passed on g++ 16.
- 2026-10-07: re-audit, implemented `canonicalDirection`, `angleApprox`, `signedAngleApprox`, `unitApprox`, `argApprox`; 6 findings fixed; full suite and stress passed on g++ 16; independent review confirmed three package defects, all fixed (here the `signedAngleApprox` negative-zero case).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename P = point>`), so `grab` of a function brings that type; quick, full and `CXX=g++-14` full runs of `01-run.py --filter 03-Geometry` and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the English search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese search aliases: OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs passed. Header edit for `-Werror`: explicit `T(...)` assignments in `operator/=`, no logic change; no mutation campaign in P274 (user decision 2026-11-10).
- 2026-11-12: P007 re-audit (GE01, started 2026-11-11): added `argumentSort`, `scalarProjectionApprox`, `normalApprox` (2D, 3D), `Angle` (`half`, `t90`, `t180`, `t360`, `+`, `-`, `<`), `angleDiff`, `segmentAngles`, with `geometry_detail::half` shared with `polarLess`; independent review: 3 findings fixed (the `Angle` step assert with 2 probes, its proof comment moved into the body, the `scalarProjectionApprox` wording); first mutation campaign, whose survivors added division by negative scalars and removed `polarLess`'s dead zero branch; full, `CXX=g++-14` full, stress, extraction and `02-integration.py` passed.
- 2026-11-12: /finish P007: added `rotateCW`, `Point2`/`Point3` `<<` and `>>` (constrained to stream-capable `T`, range-checked `int8_t` reads, the point unchanged on failure) and `argumentSortApprox` by user decision; the `angleDiff` bound and KACTL provenance fixed; 2 review defects and 1 tester-found defect fixed; changed-lines mutation campaign (7 gaps closed); full suite passed on g++ and g++-14, stress passed.
- 2026-11-15: /finish P009 (user decision 2026-11-15): `angleApprox` (2D, 3D) and `signedAngleApprox` assert that the cross and dot products passed to `atan2` are finite; 4 overflow probes added; changed-lines mutation campaign; full suite passed on g++ and g++-14, extraction and `02-integration.py` passed.
