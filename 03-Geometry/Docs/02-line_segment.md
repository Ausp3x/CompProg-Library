# 02-line_segment.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `03-polygon.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#p007--ge01-package-record) for the package record and the folder numeric policy. The 2026-10-07 re-audit implemented the seven operations the earlier row had left `missing` (`orthogonal`, `lineDistanceApprox`, `perpendicularBisectorApprox`, `angleBisectorApprox`, `reflectDirectionApprox`, `latticeOnSegment`, `gridCellsCrossed`) and added `canonicalLine` from the completeness sweep. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies. Exact line topology, `canonicalLine` and the lattice counts require integral coordinates with `|x| <= 10^9`, and they assert this bound; the degree-3 intersection numerators are then `<= 24 * 10^27`. An API ending in `Approx` returns `long double` values computed with ordinary rounding and no certified error bound. An approximate construction returns a line as two points (`array<dpoint, 2>`); the left side of a line is the positive side.

### RationalPoint2

Normalized homogeneous coordinates `(x/d, y/d)` with `d > 0` and `gcd(|x|, |y|, d) = 1`; equality compares this canonical form. The constructor asserts `d != 0` and that no component equals the minimum `lll` value. Code that assigns the public fields must preserve the invariant. It is a construction result, not a rational arithmetic package. `approx()` converts each component to long double independently. Rational normalization takes `O(log(C))` Euclidean steps.

### LinearKind, Linear2, IntersectionKind, LinearIntersection2

`Linear2` is a closed segment, a ray from `a` through `b`, or a full line. All endpoints are closed. For every kind, `a == b` is a singleton point. Integral endpoints must satisfy `|coordinate| <= 10^9`, and every linear operation asserts this and that the kind is valid. In `LinearIntersection2`, only the field matching `kind` has meaning: `p` for `Point`, `overlap` for `Segment`, `Ray` and `Line`. A segment overlap stores its endpoints in lexicographic order. An overlap reuses the original integral endpoints, so it is itself a valid `Linear2`.

### contains, onSegment, parallel, orthogonal, collinear

Exact. Membership covers singleton degeneracies. A zero direction is both parallel and orthogonal to every direction. `collinear` asks whether all endpoints lie on one common line, so any two singletons are collinear.

### intersect

Handles all nine segment/ray/line kind pairs and reports empty, point, segment, ray or line. A nonparallel crossing is an exact `RationalPoint2`. Predicates use degree-2 products and point numerators are degree 3. No rational is ever multiplied by another.

Correctness: for nonparallel linear objects, Cramer's rule gives two parameters over the same determinant. Making the denominator positive reduces segment and ray membership to exact integer comparisons. For collinear non-singleton objects, lexicographic order is monotone along their shared line, so intersecting their finite or infinite endpoint bounds gives the exact overlap kind. Singleton membership is handled before any division.

### canonicalLine

For `s.a != s.b` (asserted), returns `{A, B, K}` with `A x + B y = K` describing the supporting line, ignoring the kind. `(A, B) = canonicalDirection(perp(b - a))`, so `gcd(A, B) = 1` and `B > 0`, or `B == 0` and `A > 0`. Then `K = A a.x + B a.y`, with `|K| <= 4 * 10^18`. Two linear objects have equal keys exactly when their supporting lines coincide. The key is suitable as a map key for counting distinct lines.

Correctness: `canonicalDirection` applied to the normal `perp(b - a)` yields one primitive representative per pair of opposite normals. `K` is then determined by any point of the line, so equal keys mean equal normals up to sign and equal offsets, which is the same supporting line.

### latticeOnSegment, gridCellsCrossed

The endpoints are lattice points with `|coordinate| <= 10^9`. `latticeOnSegment` counts the lattice points on the closed segment, `gcd(|dx|, |dy|) + 1`; a singleton gives 1. `gridCellsCrossed` counts the unit cells whose open interior meets the segment, `|dx| + |dy| - gcd(|dx|, |dy|)`. An axis-parallel or singleton segment crosses no cell interior and gives 0. Both return `lng`.

Correctness: the lattice points on a segment with primitive step `(dx, dy) / g` are its `g + 1` multiples. The segment crosses `|dx|` vertical and `|dy|` horizontal grid lines in its interior. Each crossing enters a new cell, except at the `g - 1` interior lattice points, where a vertical and a horizontal crossing coincide. Hence `|dx| + |dy| - g` cells.

### projectLineApprox, reflectLineApprox, signedLineDistanceApprox

The line endpoints must differ and the direction's squared norm must be finite and nonzero; this also excludes squared underflow. The signed distance is positive to the left of the directed line `a -> b`.

### reflectDirectionApprox

Reflects the direction vector `v` across the direction of line `ab` (`a != b`): `2 (v · u / |u|^2) u - v` with `u = b - a`. This is the outgoing direction of a ray that hits a mirror along `ab`. `v` follows the same coordinate domain as points. The identity is `r = 2 proj_u(v) - v`.

### lineDistanceApprox

Returns the distance between line `ab` (`a != b`) and line `cd`. It is 0 when the lines are not parallel. Otherwise it is `|signed distance of c from ab|`. `c == d` is allowed and gives the distance from point `c` to line `ab`. For integral inputs the parallel test is exact, because long-double differences and products of `|coordinate| <= 10^9` integers are exact.

### perpendicularBisectorApprox

For `a != b`, returns `{m, m + perp(b - a)}` with `m = (a + b) / 2`. Points closer to `a` lie to the left.

### angleBisectorApprox

For `a != o` and `b != o`, returns `{o, o + w * max(|oa|, |ob|)}`, where `w` is along the internal bisector of the angle `aob` and `|w|` lies in `[sqrt(2), 2]`. For angles of at most a right angle, `w` is the sum of the two unit arms. For obtuse angles it is the perpendicular of their difference, which avoids cancellation near a straight angle. At an exactly straight angle (exact for integral inputs), it is the counterclockwise normal of `oa`. The second point lies at least the longer arm's length away. The direction recovered from the two points therefore has relative error about `ulp(coordinate) / arm length`.

Correctness: `|u| = |v| = 1` makes `u + v` orthogonal to `u - v`. The vector `u + v` has length at least `sqrt(2)` when `u · v >= 0`, and `u - v` has length at least `sqrt(2)` when `u · v < 0`. Each branch therefore takes the well-conditioned vector. The sign of `perp(±(u - v))` follows `cross(u - v, u + v) = 2 cross(u, v)`.

### pointSegmentDistanceApprox, segmentDistanceApprox

Singleton segments are allowed. An interior distance uses the perpendicular height, which avoids subtracting nearly equal projected coordinates. `segmentDistanceApprox` takes integral endpoints and decides intersection exactly before computing approximate endpoint distances.

Correctness: a disjoint segment pair attains its minimum distance at an endpoint projected onto the other segment, while an intersecting pair has distance zero. Fixed-size operations take constant time and storage.

## Feature-to-test map

`96-Local Testing/03-Geometry/02-line_segment_tester.py` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them. The entry resolves paths from its own location.

| Operation | Test | Oracle |
|---|---|---|
| `RationalPoint2`: normalization, equality, `approx` | `02-line_segment_tester` | Python Fraction |
| `Linear2` factories, `contains`, `onSegment` | Membership including singleton degeneracies | Python Fraction topology via implicit line equations |
| `parallel`, `orthogonal`, `collinear` | `02-line_segment_tester` | Python Fraction topology; an orthogonality bit from exact dot products |
| `intersect`, `IntersectionKind`, `LinearIntersection2` | Every line/segment/ray intersection pair with all five output kinds, reversed operands, and singleton degeneracies (54,256 Fraction topology fixtures) | Python Fraction topology via implicit line equations |
| `canonicalLine` | Key equality against same-line bits, normalization, endpoint incidence and extreme constants | Python Fraction same-line bits |
| `projectLineApprox`, `reflectLineApprox`, `signedLineDistanceApprox`, `pointSegmentDistanceApprox`, `segmentDistanceApprox` | Projection, reflection, signed and unsigned distances (3,902 metric fixtures) | 80-digit Decimal references |
| `lineDistanceApprox` | Parallel, antiparallel, crossing and point-line inputs | 80-digit Decimal references |
| `perpendicularBisectorApprox`, `angleBisectorApprox`, `reflectDirectionApprox` | Both bisectors including near-straight and exactly straight angles; ray reflection (6,044 construction fixtures) | 80-digit Decimal references, including a bisector reference built from the plain unit sum |
| `latticeOnSegment`, `gridCellsCrossed` | All pairs of a 5×5 grid, random `[-12, 12]` pairs, and the `±10^9` diagonal and coprime extremes (681 lattice/cell fixtures) | Lattice-point enumeration and exact Fraction cell-interior tests that do not use gcd |
| Preconditions | 23 assertion probes | Expected assertion failure in a subprocess |

Finite tests supplement the arguments above; they do not prove correctness for all inputs. Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

### 2026-09-27 (original P007)

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (`20260810`), Python 3.14.7, GNU++20. GCC 14 was unavailable. Full mode with seed `20260927` passed every configuration with 1,152,578 checks. The 2026-10-07 baseline rerun before the re-audit edits reproduced exactly this check count.

### 2026-10-07 re-audit

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable, so these are GCC 16 runs; no GCC 14 run is claimed. No online submission was made.

| Entry | Full, seed `20260927` (optimized, checked, ASan/UBSan) | Stress, seed `42`, optimized |
|---|---|---|
| `02-line_segment_tester.py` | PASS, 1,497,841 checks per configuration, 23 assertion probes; 54,256 Fraction topology, 3,902 metric, 681 lattice/cell and 6,044 construction fixtures | PASS, 2,362,935 checks |

The stress round ran before the review fixes, which did not change behavior in this header; the full run was repeated after them. Also run (package-wide):

- The sanitized integration (102 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) passed.
- A two-translation-unit smoke that mixes all four GE01 headers passed.
- A warning sweep instantiated every operation for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double` under `-Wall -Wextra -Wconversion`. The header produced no warnings.
- The brace and comment-cap check and repository consistency report no errors.

```bash
python3 '96-Local Testing/03-Geometry/02-line_segment_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/02-line_segment_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

The re-audit compared every operation in the row against the code and tests before making changes.

| # | Finding | Disposition |
|---|---|---|
| 2 | Seven line operations absent | All seven implemented, plus `canonicalLine`; enumeration, exact-bit and Decimal oracles |
| 3, 4 | Evidence claimed no gaps while rows were partial | Superseded: every operation is implemented and mapped above; status and handoff text rewritten |
| 7 | Free functions lack complexity comments | Every function sits under a complexity line, one per group of functions with the same bound (the restyle convention). Contracts moved to `## Contracts` |
| 8 | `RationalPoint2::approx` implicit `lll` to long double | Both operands cast explicitly; the header is warning-free under `-Wall -Wextra -Wconversion` |
| 9 | `LinearIntersection2` lacks a comment | Complexity and field-validity line added |
| 10 | `; }` closings | Header and C++ tester normalized; `03-consistency.py --braces` is clean |

Other changes: the multi-line contract comment blocks moved out of the header into `## Contracts`, and the header meets the comment cap. `RationalPoint2`'s constructor parameters are now lowercase and normalized before they are stored. No public name or documented behavior changed.

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The 2026-10-07 completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (line and segment). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/intersection.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their segment intersection and line intersection, with explicit widening and degeneracy contracts. The historical name `isIntscLinSeg` is not a compatibility API. Consumers of the old coefficient-form line intersection migrate to endpoint-defined `Linear2`. Circle routines from the old `intersection.cpp` are covered by `06-circle.hpp` ([evidence](06-circle.md)). All legacy bytes remain untouched.

## Limits and handoffs

No failing reproducer and no unfinished owned operation. Other planned packages own adaptive and exact floating predicates (P128 / GE11), segment intersection sweeps (P129 / GE04) and general rational-lattice counts (P037 / GE27).
