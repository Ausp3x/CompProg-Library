# 03-polygon.hpp — Evidence

Owned by package P007 / GE01 (geometry foundations, with `01-point.hpp`, `02-line_segment.hpp` and `04-convexhull.hpp`). Every operation in the row has a test.

## Contracts

### Numeric Policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### Polygon Conventions

- A ring has no repeated closing vertex, and the total vertex count is at most `INT_MAX`.
- Area, moments and winding apply algebraically to any oriented closed walk. Geometric area, centroid, convexity and Pick interpretations need a simple polygon; simplicity validation belongs to P129 / GE04's sweep.
- A polygon with holes is `{outer CCW, holes CW}`. Its rings must be simple and disjoint, with holes that do not touch and lie strictly inside the outer ring. Signed sums subtract holes.
- In complexity comments, `n` is the total vertex count and `r` the ring count (`r = 1` for a single ring).
- Traversals are linear, with constant auxiliary storage apart from the cut's output. Lattice gcd work adds `O(log(C))` per edge.

### PolygonMoments, polygonMoments, signedArea2, signedAreaApprox

`area2` is twice the signed area, and `x6` and `y6` are six times the signed first moments, all exact for integral `T` within the common bound. The area is positive for a CCW simple polygon and zero for an empty walk. `signedAreaApprox` halves the area in long double.

Correctness: area and first moments sum signed edge wedges, and linear summation handles oriented holes.

### makeCCW

`makeCCW(p)` (user decision 2026-11-12) takes one ring, a `vector<Point2<T>>` (a vector of rings does not compile), by value. It returns the ring reversed (`std::reverse`, so the start vertex becomes the last) when `signedArea2(p) < 0`, and unchanged otherwise, including zero area and fewer than three points. For integral `T` the result's signed area is therefore `>= 0` and `makeCCW` is idempotent. For floating `T` the rounded area of the reversed ring need not be the negation of the input's, so both can be negative and neither claim holds. For a self-intersecting ring the sign is that of the total signed area. It composes `signedArea2` and `reverse`, with their domain: integral exact, floating as rounded. Cost: `O(n)` time, `O(1)` beyond the returned ring.

### centroidExact, centroidApprox

Both return `false` when the signed area is zero (computed zero for the floating form) and then leave `out` unchanged. The exact form requires integral coordinates and supports signed rings; the `log(n * C^3)` term in its cost is the gcd normalization.

Correctness: dividing the first-moment numerators by `3 * area2` gives the centroid.

### PolygonLocation, WindingResult, polygonWinding, polygonContains

The boundary is checked first; winding then counts upward and downward crossings with opposite signs, using half-open vertical intervals. When `boundary` is true, `winding` is unspecified. Empty, point and segment walks have no interior. The rings overload uses nonzero fill, and a point on any ring's boundary is reported as boundary.

### polygonPerimeter

Closed-walk length: an empty walk or a single point gives 0, two points give twice their distance, and the rings overload includes hole boundaries.

### polygonConvex

Assumes a simple ring, in either orientation; it is not a simplicity test. `strict` rejects collinear consecutive vertices, while the weak form allows forward-collinear ones. Fewer than 3 vertices, zero area, zero-length edges and backtracking always return `false`.

Correctness: a simple polygon is convex exactly when its turn signs are consistent and nonzero, with backtracking and repeated edges rejected.

### latticeBoundary, latticeInterior

Integral coordinates with `|coordinate| <= 10^9`, asserted per vertex. `latticeBoundary` sums the edge gcds, which counts distinct boundary points only for simple rings of nonzero area. `latticeInterior` applies Pick's theorem to a simple ring of nonzero area in either orientation. The rings overload requires one CCW outer ring and `r - 1` CW holes (asserted) and applies the Euler correction `1 - h`.

Correctness: Pick's theorem gives `I = (abs(area2) - B) / 2 + 1` for a simple lattice ring, where `B` is the sum of edge gcds; subtracting holes changes the constant to `1 - h`.

### polygonCutApprox

Takes a ring `p` and a directed line `a -> b` (`a != b`, asserted) and returns, by the Sutherland–Hodgman step, the part of `p` in the closed left half-plane. A kept vertex has `orient >= 0`, and a crossing point is inserted only on a strict sign change, so no crossing is duplicated. The output has at most `2 * n` vertices. It may contain collinear vertices and zero-width bridges along the cut line, and for a nonconvex input it may be a non-simple walk; for a simple input its signed area equals the area of the clipped region. Signs are exact for integral `T`. For `|coordinate| <= 10^9` each crossing parameter is the quotient of two exactly computed long-double crosses, so it is rounded once. Floating input uses computed signs.

Correctness: the cut replaces each excursion into the open right half-plane with a path along the cut line. Every point in the open left half-plane keeps its winding number, and every point in the open right half-plane gets winding 0, because the new walk lies in the closed left half-plane. The signed area is the integral of the winding number, so it equals the area of the polygon intersected with the half-plane, even with bridges present.

## Feature-to-Test Map

`96-Local Testing/03-Geometry/03-polygon_tester.py` runs the optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds of the shared runner [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

The base oracle is unions of unit grid cells, which supply area, first moments, occupancy, perimeter and enumerated lattice points. Reversal, triangle barycenters, floating dyadic and wide-coordinate regressions supplement it.

| Operation | Test | Oracle |
|---|---|---|
| `PolygonMoments` (`+=` through the rings overload), `polygonMoments`, `signedArea2`, `signedAreaApprox` (ring, rings) | Signed area and moments, oriented holes, every ring overload | Grid-cell union area and first moments; reversal |
| `centroidExact`, `centroidApprox` | Exact and approximate centroids, including `out` left unchanged on zero area | Grid-cell first moments; triangle barycenters |
| `PolygonLocation`, `WindingResult`, `polygonWinding`, `polygonContains` | Winding, nonzero-fill containment and boundary, holes | Grid-cell occupancy |
| `polygonPerimeter` (ring, rings) | Perimeter including holes | Grid-cell union perimeter |
| `polygonConvex` (strict, weak) | Strict/weak convexity | Grid-cell histogram fixtures |
| `latticeBoundary`, `latticeInterior` (ring, rings) | Lattice gcd counts and Pick, with holes | Enumerated lattice points |
| `makeCCW` | `verifyCCW` on 17 fixed `lng` rings (empty, 1 and 2 points, collinear and backtracking, both square orientations, zero-area bowtie, area2 -8 self-intersecting ring, doubled square, ±10^9 slivers and thin triangles), `int` and long double fixed rings, the default `vector<point>` call, and random `lng`, `int` and quarter-step long double rings with their reverses and self-cancelling concatenations | Exact trapezoid-sum sign in `lll` (not `cross` or `signedArea2`) decides reversed or unchanged; idempotence; nonnegative result area |
| `polygonCutApprox` | 14 lines per histogram polygon plus empty, edge-line, whole, wide-diagonal, nonconvex-bridge and floating regressions; the output-size bound; vertices on the closed left side | Expected area integrates each cell's clipped column heights piecewise-linearly, self-checked by complementary half-planes summing to 1 per cell |
| Preconditions | 9 assertion probes, including `a == b` for the cut and `holes-flat`, `holes-zero-total`, `holes-negative-total` for the rings form of `latticeInterior` | Expected assertion failure in a subprocess |

Approximate checks use scale-aware tolerances on these fixtures; they do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and Results

2026-11-12 (/finish P007), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14, GNU++20, seed 42; optimized, checked and ASan-UBSan configurations. The 18 `03-Geometry` suites ran in sequence on the final headers and testers; after the last tester change this suite was rerun alone (`03-polygon_tester.py --mode full --seed 42`), and the three test rows are those reruns.

| Command | Result |
|---|---|
| `01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` (GCC 16.2.1) | PASS, 462,245 checks per configuration, 9 assertion probes; pre-flight quick peak 53 MB; `MEMORY peak=80MB` |
| same with `CXX=g++-14` | PASS, 462,245 checks per configuration, 9 probes; pre-flight quick peak 920 MB; `MEMORY peak=1044MB` |
| `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 3,044,318 checks per configuration; `MEMORY peak=285MB` |
| `05-extraction.py ops --filter 03-Geometry/03-polygon.hpp` and `05-extraction.py testers --filter '96-Local Testing/03-Geometry/03-polygon_tester.py'` | PASS, 19 operations on g++ and g++-14 |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3241MB` |

Build and run times of the P274 library-wide runs (2026-11-10; seed 42, beside the other suites, build cache emptied before each cold run; seconds as compile + run; every run passed):

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | 762 MB | 0.3 + 0.2 | 14.7 + 0.5 | 16.5 + 1.4 | 952 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | 789 MB | 0.2 + 0.1 | 23.3 + 0.6 | 23.2 + 1.2 | 1019 MB |
| `01-run.py --mode quick` | — | 18.7 + 0.0 | 21.3 + 0.3 | — | 772 MB |
| `01-run.py --mode stress --rounds 1` | 786 MB | 0.1 + 0.3 | 6.5 + 1.1 | 7.0 + 3.0 | 523 MB |

```bash
python3 '96-Local Testing/03-Geometry/03-polygon_tester.py' --mode full --seed 42
python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/03-polygon.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter '96-Local Testing/03-Geometry/03-polygon_tester.py'
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/03-polygon.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `38e2c99e3fd4ea33ea55cc34e8dca823ea3c7a04` (2026-11-12, seed 1, whole file, uniform sample of 818): 500 mutants, 451 caught (1 by timeout), 49 survived, 0 untested.

Stage times, cumulative over one retest of the same workdir: quick 824 s, full optimized 29 s, full checked and ASan/UBSan 561 s. The run gave the header its first record (/finish P007, user decision). An earlier whole-file campaign that day, on the same sample before `makeCCW`'s signature changed, found seven test gaps, all closed. Lines are those of the current blob.

| Mutants | Line | Why open |
|---|---|---|
| `COS 517`, `COS 532`, `AAOS 446`–`449`, `CR 97`, `COS 536`, `COS 537`, `AAOS 460`, `AAOS 461`, `COS 552`, `AOS 298`, `AAOS 466`, `AAOS 469`, `COS 636`, `CR 129`, `COS 642`, `AAOS 480`, `AAOS 481`, `CR 138`, `COS 662`, `COS 707`, `AOS 354`, `AAOS 492`, `AAOS 493`, `COS 731`, `COS 781` | 17, 25, 29, 34, 77, 88, 93, 98, 115, 123, 138 | Untestable rather than equivalent: the `INT_MAX` vertex-count assertions and the running total `n` that feeds only them need about 2^31 vertices (over 32 GB of points), beyond the memory cap. |
| `COS 521`, `COS 541`, `COS 671`, `COS 711` (`i < n` → `i != n`); `COS 529`, `COS 549`, `COS 584`, `COS 654`, `COS 684`, `COS 799` (`i + 1 == n` → `>=`) | 19, 20, 30, 67, 89, 100, 101, 116, 141 | Equivalent: `i` steps by one from 0, and `i + 1 >= n` holds only at `i = n - 1`. |
| `AS 205`, `AS 211`, `AS 212` | 68, 107, 119 | Equivalent: symmetric `min`, `dot` and `gcd` arguments. |
| `IF 174` (`n < 3` → false), `IF 175` (`a == b` → false), `COS 692` (`<= 0` → `< 0`) in `polygonConvex` | 99, 103, 107 | Equivalent: with fewer than three vertices or a zero edge the loop still returns false (a repeated vertex has `orient == 0` and `dot <= 0`), and `dot == 0` with `orient == 0` needs a zero edge, rejected at the next vertex. |
| `CR 152`, `COS 741`, `CR 156`, `LOS 513`, `COS 766` | 126, 129, 132 | Masked: the weakened `latticeInterior` assertions (`size >= 3`, nonempty rings, an outer ring of either sign) leave the total area `<= 0`, and `assert(a > 0)` fires on the same probes (`pick-empty`, `pick-collinear`, `holes-empty`, `holes-orientation`); `abs` makes `a > 0` and `a != 0` equal. |

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were compared, not copied as correctness guarantees. Completeness-sweep sources: [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted: [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). P007 re-audit sources: [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11); its decisions: [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (polygon area, centroid, containment), read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- [cp-algorithms: Area of simple polygon](https://cp-algorithms.com/geometry/area-of-simple-polygon.html) and [Pick's theorem](https://cp-algorithms.com/geometry/picks-theorem.html), inspected September 27, 2026. The holes formula is derived here by subtracting each hole's interior and boundary lattice points.
- [KACTL PolygonCut.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PolygonCut.h), fetched 2026-10-07. KACTL keeps the right side and inserts on a `(a < 0) != (b < 0)` change; this library keeps the closed left side and inserts only on strict sign changes, so no crossing is duplicated.
- Legacy: `OLD/Team Notebook/src/geometry/polyarea.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp`. This header covers their absolute area. The historical name `polyArea` is not a compatibility API. The legacy bytes are untouched.

## Limits and Handoffs

Simple-polygon validation and intersection sweeps belong to P129 / GE04, and adaptive and exact floating predicates to P128 / GE11. General rational-lattice counts are row 19 (`19-lattice_geometry.hpp`).

Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): Pick's theorem, shoelace formula, polygon area; Pick 定理, 二维计算几何基础, 多边形面积, 点在多边形内, 鞋带公式, 皮克定理, 多边形重心, 环绕数, 凸多边形判定, 多边形切割.
Aliases (P007): make counterclockwise, orient polygon, reverse orientation, 逆时针化.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 289,431 checks) passed on g++ 16.
- 2026-10-07: re-audit, added `polygonCutApprox`; 5 findings fixed; full suite and stress passed on g++ 16; independent review confirmed three package defects, all fixed (here pre-existing `-Wsign-compare` warnings in five polygon asserts).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename Polygon = vector<point>>`), so `grab` of a function brings that type; quick, full and `CXX=g++-14` full runs of `01-run.py --filter 03-Geometry` and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the English search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese search aliases: OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs passed.
- 2026-11-12: P007 re-audit (GE01): header unchanged; full, `CXX=g++-14` full, stress, extraction (`testers`) and `02-integration.py` passed against the changed `01-point.hpp` and `02-line_segment.hpp`; no mutation campaign, since the header was unchanged.
- 2026-11-12: /finish P007: added `makeCCW` by user decision (one ring; a vector of rings does not compile); first mutation record (whole file, 7 test gaps closed, 3 probes added); full suite passed on g++ and g++-14, stress passed.
