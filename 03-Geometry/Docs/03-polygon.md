# 03-polygon.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `02-line_segment.hpp` and `04-convexhull.hpp`); see [00-notes.md](00-notes.md#p007--ge01-package-record) for the package record and the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies. With `|x| <= 10^9`, the polygon first-moment terms are `<= 4 * 10^27`, summable over `INT_MAX` vertices. The lattice counts require integral coordinates with `|x| <= 10^9` and assert this bound; the other integral polygon routines also accept wider coordinates as long as every intermediate fits `lll`. Floating inputs use computed signs with no epsilon.

### Polygon conventions

- A ring has no repeated closing vertex, and the total vertex count is at most `INT_MAX`.
- Area, moments and winding apply algebraically to any oriented closed walk. Geometric area, centroid, convexity and Pick interpretations need a simple polygon. Simplicity validation belongs to P129 / GE04's sweep.
- A polygon with holes is `{outer CCW, holes CW}`. Its rings must be simple and disjoint, with holes that do not touch and lie strictly inside the outer ring. Signed sums subtract holes.
- In complexity comments, `n` is the total vertex count and `r` the ring count (`r = 1` for a single ring).
- Polygon traversals are linear, with constant auxiliary storage apart from the cut's output. Lattice gcd work adds `O(log(C))` per edge.

### PolygonMoments, polygonMoments, signedArea2, signedAreaApprox

`area2` is twice the signed area, and `x6` and `y6` are six times the signed first moments. All are exact for integral `T` within the common bound. The result is positive for a CCW simple polygon and zero for an empty walk. `signedAreaApprox` halves the area in long double.

Correctness: polygon area and first moments sum signed edge wedges, and linear summation handles oriented holes.

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

`96-Local Testing/03-Geometry/03-polygon_tester.py` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

The base oracle is unions of unit grid cells, which supply area, first moments, occupancy, perimeter and enumerated lattice points. Reversal, triangle barycenters, floating dyadic and wide-coordinate regressions supplement it.

| Operation | Test | Oracle |
|---|---|---|
| `PolygonMoments`, `polygonMoments`, `signedArea2`, `signedAreaApprox` (ring, rings) | Signed area and moments, oriented holes, every ring overload | Grid-cell union area and first moments; reversal |
| `centroidExact`, `centroidApprox` | Exact and approximate centroids, including `out` left unchanged on zero area | Grid-cell first moments; triangle barycenters |
| `PolygonLocation`, `WindingResult`, `polygonWinding`, `polygonContains` | Winding, nonzero-fill containment and boundary, holes | Grid-cell occupancy |
| `polygonPerimeter` (ring, rings) | Perimeter including holes | Grid-cell union perimeter |
| `polygonConvex` (strict, weak) | Strict/weak convexity | Grid-cell histogram fixtures |
| `latticeBoundary`, `latticeInterior` (ring, rings) | Lattice gcd counts and Pick, with holes | Enumerated lattice points |
| `polygonCutApprox` | 14 lines per histogram polygon plus empty, edge-line, whole, wide-diagonal, nonconvex-bridge and floating regressions; the output-size bound; vertices on the closed left side | Expected area integrates each cell's clipped column heights piecewise-linearly, self-checked by complementary half-planes summing to 1 per cell |
| Preconditions | 6 assertion probes, including `a == b` for the cut | Expected assertion failure in a subprocess |

Finite tests supplement the arguments above; they do not prove correctness for all inputs. Approximate checks use scale-aware tolerances on the specified fixtures. They do not establish a universal error bound for ill-conditioned floating inputs.

## Commands and results

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable; no GCC 14 run is claimed.

| Command | Result |
|---|---|
| `03-polygon_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 389,280 checks per configuration, 6 assertion probes |
| `03-polygon_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 2,439,453 checks |
| `02-integration.py --sanitizers` (102 headers, scalar and AVX2 multi-TU builds, workspace) | PASS |
| Two-translation-unit smoke mixing all four GE01 headers | PASS |
| Warning sweep, every operation for all eight `T`, `-Wall -Wextra -Wconversion` | No warnings |
| `03-consistency.py --braces ...` and `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/03-polygon_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/03-polygon_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Sources

The implementation was written independently from the geometric identities. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (polygon area/centroid/containment). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [Stanford ICPC notebook, 2015–16 PDF](https://raw.githubusercontent.com/jaehyunp/stanfordacm/master/notebook.pdf), miscellaneous geometry section 2.2, pp. 6–7. Its approximate EPS predicates are not adopted for exact integer topology or sorting.
- [cp-algorithms: Area of simple polygon](https://cp-algorithms.com/geometry/area-of-simple-polygon.html) and [Pick's theorem](https://cp-algorithms.com/geometry/picks-theorem.html), inspected September 27, 2026. The holes formula is derived here by subtracting each hole's interior and boundary lattice points.
- [KACTL PolygonCut.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/geometry/PolygonCut.h), fetched 2026-10-07. KACTL keeps the right side and inserts on a `(a < 0) != (b < 0)` change. This library keeps the closed left side and inserts only on strict sign changes, so no crossing is duplicated.
- Legacy: the preserved `OLD/Team Notebook/src/geometry/polyarea.cpp` and its counterparts in `algs.cpp` and `algsbetter.cpp` were inspected. This header covers their absolute area. The historical name `polyArea` is not a compatibility API. All legacy bytes remain untouched.

## Limits and handoffs

Other planned packages own simple-polygon validation and intersection sweeps (P129 / GE04), general rational-lattice counts (P037 / GE27) and adaptive and exact floating predicates (P128 / GE11).

## History

- 2026-09-27: original P007, full suite (seed 20260927, 289,431 checks) passed on g++ 16.
- 2026-10-07: re-audit, added `polygonCutApprox`; 5 findings fixed (complexity comments, braces, `r` definition, missing comments, `-Wsign-compare` in size asserts); full suite and stress passed on g++ 16.
