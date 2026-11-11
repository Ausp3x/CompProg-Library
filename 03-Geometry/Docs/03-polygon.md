# 03-polygon.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `02-line_segment.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#ge01-numeric-policy-p007) for the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### Polygon conventions

- A ring has no repeated closing vertex, and the total vertex count is at most `INT_MAX`.
- Area, moments and winding apply algebraically to any oriented closed walk. Geometric area, centroid, convexity and Pick interpretations need a simple polygon. Simplicity validation belongs to P129 / GE04's sweep.
- A polygon with holes is `{outer CCW, holes CW}`. Its rings must be simple and disjoint, with holes that do not touch and lie strictly inside the outer ring. Signed sums subtract holes.
- In complexity comments, `n` is the total vertex count and `r` the ring count (`r = 1` for a single ring).
- Polygon traversals are linear, with constant auxiliary storage apart from the cut's output. Lattice gcd work adds `O(log(C))` per edge.

### PolygonMoments, polygonMoments, signedArea2, signedAreaApprox

`area2` is twice the signed area, and `x6` and `y6` are six times the signed first moments. All are exact for integral `T` within the common bound. The result is positive for a CCW simple polygon and zero for an empty walk. `signedAreaApprox` halves the area in long double.

Correctness: polygon area and first moments sum signed edge wedges, and linear summation handles oriented holes.

### makeCCW

`makeCCW(p)` (user decision 2026-11-12) takes one ring, a `vector<Point2<T>>` (a vector of rings does not compile), by value and returns it reversed (`std::reverse`, so the start vertex becomes the last) when `signedArea2(p) < 0`, and unchanged otherwise, including zero area and fewer than three points. For integral `T` the result's signed area is therefore `>= 0` and `makeCCW` is idempotent; for floating `T` the rounded area of the reversed ring need not be the negation of the input's, so both can be negative and neither claim holds. For a self-intersecting ring the sign is that of the total signed area. It is a composition of `signedArea2` and `reverse`, with their domain: integral exact, floating as rounded. Cost: `O(n)` time, `O(1)` beyond the returned ring.

### centroidExact, centroidApprox

Both return `false` when the signed area is zero (computed zero for the floating form) and then leave `out` unchanged. The exact form requires integral coordinates and supports signed rings. The `log(n * C^3)` term in its cost is the gcd normalization.

Correctness: dividing the first-moment numerators by `3 * area2` gives the centroid.

### PolygonLocation, WindingResult, polygonWinding, polygonContains

Winding counts upward and downward crossings using half-open vertical intervals, and checks the boundary first. When `boundary` is true, `winding` is unspecified. Empty, point and segment walks have no interior. The rings overload uses nonzero fill, and a point on any ring's boundary is reported as boundary.

Correctness: winding counts upward and downward crossings with opposite signs, after checking boundary membership.

### polygonPerimeter

Closed-walk length. An empty walk or a single point gives 0, two points give twice their distance, and the rings overload includes hole boundaries.

### polygonConvex

Assumes a simple ring, in either orientation; this is not a simplicity test. `strict` rejects collinear consecutive vertices, while the weak form allows forward-collinear ones. Fewer than 3 vertices, zero area, zero-length edges and backtracking always return `false`.

Correctness: simple-polygon convexity means consistent nonzero turn signs, with backtracking and repeated edges rejected.

### latticeBoundary, latticeInterior

Integral coordinates with `|coordinate| <= 10^9`, asserted per vertex. `latticeBoundary` sums the edge gcds, which counts distinct boundary points only for simple, nonzero-area rings. `latticeInterior` uses Pick's theorem on a simple ring of nonzero area in either orientation. The rings overload requires one CCW outer ring and `r - 1` CW holes (asserted) and applies the Euler correction `1 - h`.

Correctness: Pick's theorem gives `I = (abs(area2) - B) / 2 + 1` for a simple lattice ring, where `B` is the sum of edge gcds; subtracting holes changes the constant to `1 - h`.

### polygonCutApprox

Takes a ring `p` and a directed line `a -> b` (`a != b`, asserted). Using the Sutherland–Hodgman step, it returns the part of `p` in the closed left half-plane. A kept vertex has `orient >= 0`, and a crossing point is inserted only on a strict sign change, so no crossing is duplicated. The output has at most `2 * n` vertices. It may contain collinear vertices and zero-width bridges along the cut line, and for a nonconvex input it may be a non-simple walk. For a simple input its signed area equals the area of the clipped region. Signs are exact for integral `T`. For `|coordinate| <= 10^9`, each crossing parameter is the quotient of two exactly computed long-double crosses, so it is rounded only once. Floating input uses computed signs.

Correctness: the cut replaces each excursion into the open right half-plane with a path along the cut line. Every point in the open left half-plane keeps its winding number, and every point in the open right half-plane gets winding 0, because the new walk lies in the closed left half-plane. The signed area is the integral of the winding number. It therefore equals the area of the polygon intersected with the half-plane, even when bridges are present.

## Feature-to-test map

`96-Local Testing/03-Geometry/03-polygon_tester.py` runs the geometry runner's optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds ([`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>)). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

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

Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 762 MB; optimized 0.3 s compile + 0.2 s run, checked 14.7 s compile + 0.5 s run, ASan-UBSan 16.5 s compile + 1.4 s run; MEMORY peak 952 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 789 MB; optimized 0.2 s compile + 0.1 s run, checked 23.3 s compile + 0.6 s run, ASan-UBSan 23.2 s compile + 1.2 s run; MEMORY peak 1019 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 18.7 s compile + 0.0 s run, checked 21.3 s compile + 0.3 s run; MEMORY peak 772 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 786 MB; optimized 0.1 s compile + 0.3 s run, checked 6.5 s compile + 1.1 s run, ASan-UBSan 7.0 s compile + 3.0 s run; MEMORY peak 523 MB. Dated rows below stay those commands' latest results.

2026-11-12 (/finish P007, experiment: none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14, GNU++20, seed 42, optimized, checked and ASan-UBSan configurations; the 18 `03-Geometry` suites ran in sequence on the final headers and testers, with the mutation campaigns finished and the benchmark not running.

| Command | Result |
|---|---|
| `01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` (GCC 16.2.1) | PASS, 462,245 checks per configuration, 9 assertion probes; pre-flight quick peak 53 MB; `MEMORY peak=80MB` (rerun of this suite alone after the last tester change, `03-polygon_tester.py --mode full --seed 42`) |
| `CXX=g++-14 01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` | PASS, 462,245 checks per configuration, 9 probes; pre-flight quick peak 920 MB; `MEMORY peak=1044MB` (this suite alone, as above) |
| `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 3,044,318 checks per configuration; `MEMORY peak=285MB` (this suite alone, as above) |
| `05-extraction.py ops --filter 03-Geometry/03-polygon.hpp` and `05-extraction.py testers --filter '96-Local Testing/03-Geometry/03-polygon_tester.py'` | PASS, 19 operations on g++ and g++-14 |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3241MB` |

```bash
python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/03-polygon.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter '96-Local Testing/03-Geometry/03-polygon_tester.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

Record: mewt 4.0.0 on blob `38e2c99e3fd4ea33ea55cc34e8dca823ea3c7a04` (2026-11-12, seed 1, whole file, uniform sample of 818): 500 mutants, 451 caught (1 by timeout), 49 survived, 0 untested.

Stage seconds, cumulative over one retest of the same workdir: quick 824, full-optimized 29, full-rest 561 (`/finish P007`, run with the user's decision to give the unchanged header its first record).

Earlier campaign: mewt 4.0.0 on blob `f6706d4e573d1abe09424315ffd0cd658b8b5f6a` (2026-11-12, seed 1, whole file, the same sample): 500 mutants, 443 caught, 57 survived. Seven of its survivors were test gaps, now caught: `COS 567` (`centroidApprox` on a clockwise ring, check `clockwise centroid`), `COS 665` and `COS 667` (`polygonConvex` on triangles, `strict triangle` and `clockwise triangle`), `COS 756` (a triangle hole, `triangle hole Pick against enumeration`), and `COS 772`, `COS 776`, `COS 779` (probes `holes-flat`, `holes-negative-total`, `holes-zero-total`). The header then changed only in `makeCCW`'s signature, so the final campaign above reran the whole file.

| Mutants | Line | Why they survive |
|---|---|---|
| `COS 517`, `COS 532`, `AAOS 446`–`449`, `CR 97`, `COS 536`, `COS 537`, `AAOS 460`, `AAOS 461`, `COS 552`, `AOS 298`, `AAOS 466`, `AAOS 469`, `COS 636`, `CR 129`, `COS 642`, `AAOS 480`, `AAOS 481`, `CR 138`, `COS 662`, `COS 707`, `AOS 354`, `AAOS 492`, `AAOS 493`, `COS 731`, `COS 781` | 17, 25, 29, 34, 77, 88, 93, 98, 115, 123, 138 | The `INT_MAX` vertex-count assertions and the running total `n` that feeds only them: reaching them needs about 2^31 vertices (over 32 GB of points), beyond the memory cap, so they are untestable rather than equivalent. |
| `COS 521`, `COS 541`, `COS 671`, `COS 711` (`i < n` → `i != n`); `COS 529`, `COS 549`, `COS 584`, `COS 654`, `COS 684`, `COS 799` (`i + 1 == n` → `>=`) | 19, 20, 30, 67, 89, 100, 101, 116, 141 | Equivalent: `i` steps by one from 0, and `i + 1 >= n` holds only at `i = n - 1`. |
| `AS 205`, `AS 211`, `AS 212` | 68, 107, 119 | Equivalent: symmetric `min`, `dot` and `gcd` arguments. |
| `IF 174` (`n < 3` → false), `IF 175` (`a == b` → false), `COS 692` (`<= 0` → `< 0`) in `polygonConvex` | 99, 103, 107 | Equivalent: with fewer than three vertices or a zero edge the loop still returns false (a repeated vertex has `orient == 0` and `dot <= 0`), and `dot == 0` with `orient == 0` needs a zero edge, rejected at the next vertex. `COS 690` (`dot <= 0` → `== 0`) was caught on the retest by the backtracking ring `collinear backtrack between consistent turns` (independent review 2026-11-12). |
| `CR 152`, `COS 741`, `CR 156`, `LOS 513`, `COS 766` | 126, 129, 132 | Masked: the weakened `latticeInterior` assertions (`size >= 3`, nonempty rings, an outer ring of either sign) leave the total area `<= 0`, and `assert(a > 0)` fires on the same probes (`pick-empty`, `pick-collinear`, `holes-empty`, `holes-orientation`); `abs` makes `a > 0` and `a != 0` equal. |

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). The P007 re-audit sources are in [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11) and its decisions in [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (polygon area/centroid/containment). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- [cp-algorithms: Area of simple polygon](https://cp-algorithms.com/geometry/area-of-simple-polygon.html) and [Pick's theorem](https://cp-algorithms.com/geometry/picks-theorem.html), inspected September 27, 2026. The holes formula is derived here by subtracting each hole's interior and boundary lattice points.
- [KACTL PolygonCut.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PolygonCut.h), fetched 2026-10-07. KACTL keeps the right side and inserts on a `(a < 0) != (b < 0)` change. This library keeps the closed left side and inserts only on strict sign changes, so no crossing is duplicated.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/polyarea.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their absolute area. The historical name `polyArea` is not a compatibility API. All legacy bytes remain untouched.

## Limits and handoffs

Other planned packages own simple-polygon validation and intersection sweeps (P129 / GE04) and adaptive and exact floating predicates (P128 / GE11). General rational-lattice counts are row 19 (`19-lattice_geometry.hpp`).

## Aliases

Search aliases (P263, grab --find): Pick's theorem, shoelace formula, polygon area; Pick 定理, 二维计算几何基础, 多边形面积, 点在多边形内, 鞋带公式, 皮克定理, 多边形重心, 环绕数, 凸多边形判定, 多边形切割.
Aliases (P007): make counterclockwise, orient polygon, reverse orientation, 逆时针化.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 289,431 checks) passed on g++ 16.
- 2026-10-07: re-audit, added `polygonCutApprox`; 5 findings fixed; full suite and stress passed on g++ 16; independent review: confirmed three package defects, all fixed (here pre-existing `-Wsign-compare` warnings in five polygon asserts).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<typename Polygon = vector<point>>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 03-Geometry` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P007 re-audit (GE01): header unchanged; full, `CXX=g++-14` full, stress, extraction (`testers`) and `02-integration.py` passed against the changed `01-point.hpp` and `02-line_segment.hpp`; no mutation campaign, since the header is unchanged.
- 2026-11-12: /finish P007: added `makeCCW` by user decision (one ring; a vector of rings does not compile); first mutation record (whole file, 7 test gaps closed, 3 probes added); full suite passed on g++ and g++-14, stress passed.
