# 15-convex_polygon_query.hpp — evidence

Owned by package P037 / GE05, together with [13-segment_union.md](13-segment_union.md) and [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md). Every operation in the row has a test.

## Contracts

### Numeric policy

Polygons and query points are integral with `|coordinate| <= 10^9`, asserted. Edge vectors stay below `2 * 10^9` and orientations below `8 * 10^18`. Line parameters `num / den` have `|num|, den <= 8 * 10^18`, so comparing two of them takes products below `6.4 * 10^37`. Crossing points are `RationalPoint2` with numerators below `5 * 10^28`. Every topological answer is exact: location, tangents, extreme vertices, intersection points, cut vertices, `prefixArea2`, `convexIntersection` and the area k-gon. `pointDistanceApprox`, `areaLeftOfLineApprox` and the perimeter k-gon are rounded `long double`.

### ConvexPolygonQuery

The constructor takes a strictly convex counterclockwise ring with `n >= 3`, any starting vertex; this is what `convexHull(..., false)` returns. It asserts `polygonConvex(p, true)` and a positive area in `O(n)`. It stores the ring as given, so every index refers to the input. It also stores the prefix sums of `cross(p[i], p[i + 1])` and the index `s0` of the edge with the smallest direction angle. `O(n)` time and memory. `at`, `edge`, `leftChain` and `crossing` are helpers.

- `contains(q)`: `PolygonLocation` (closed boundary). Binary search over the fan of triangles `(p[0], p[i], p[i + 1])`. `O(log(n))`.
- `batchedLocate(qs)`: locations for queries sorted by `polarLess(q − p[0])`, the library's angle order (asserted in `O(q)`). The results are in query order. Queries equal to `p[0]` are on the boundary. The rest, rotated to start at the direction of `p[1] − p[0]`, cross the wedge at `p[0]` counterclockwise, so one pointer walks the fan. `O(n + q)` time, `O(q)` memory. Kept by decision (2026-10-18) as a different algorithm and domain from `q` calls of `contains`.
- `extreme(d)`: the vertex maximizing `dot(d, p[i])`, for `d != 0` with `|d| <= 10^18`. When two vertices tie, it returns the first counterclockwise. The edge directions, rotated to start at `s0`, increase in angle, so this is a lower bound for `perp(d)`. `O(log(n))`.
- `tangents(q)`: for `q` strictly outside (asserted), `{a, b}`. The boundary visible from `q` runs counterclockwise from `p[a]` to `p[b]`. The polygon lies left of `q → p[b]` and right of `q → p[a]`. An edge on a line through `q` is not visible, so ties take the vertex nearer `q`. The search first finds one visible edge, from the fan at `p[0]` or its two edges. It also finds one invisible edge: an edge at `p[0]`, or the exit edge of the ray from `q` through `p[0]`. Two monotone binary searches then find the ends of the visible chain. `O(log(n))`.
- `pointDistanceApprox(q)`: 0 inside or on the boundary. Otherwise the distance to the visible chain: on its edges, "the edge points toward `q`" is true and then false, so a binary search finds the nearest edge. `O(log(n))`.
- `lineIntersection(s)`: the intersection of the closed polygon with a segment, ray or line `s` (`s.a == s.b` is a point). It returns `ConvexLineIntersection` with `count` 0, 1 or 2 and exact points ordered along `s.a → s.b`. The two extreme vertices for the normal of `s` split the boundary into two monotone chains. One binary search on each finds the exit and entry crossings, and the parameter interval is then clipped to `s`. Touching at a vertex or along an edge gives one point or a segment. `O(log(n))`.
- `cut(a, b)`: the closed left side of `a → b`, `a != b`, as exact vertices. It is a counterclockwise ring starting at the crossing where the boundary enters the left side, or the whole polygon from index 0. When the line only touches it is one or two points, and it is empty when disjoint. `O(log(n) + out)`. Kept by decision (2026-10-18) for this bound beside `clipConvex` and `polygonCutApprox`.
- `areaLeftOfLineApprox(a, b)`: the area, not doubled, of the same region. The chain part is exact (`prefixArea2`) and the two crossings are rounded. `O(log(n))`.
- `prefixArea2(i, j)`: twice the area of the polygon `p[i], p[i + 1], ..., p[j]`, cyclic, closed by the chord from `p[j]` back to `p[i]`; `i == j` gives 0. Exact, `O(1)`.

### ConvexLineIntersection

A result record: `count` and `points`; only `points[0..count)` are meaningful.

### convexIntersection

Input: two convex CCW polygons, collinear vertices allowed, positive area, `|coordinate| <= 10^9`, as in `clipConvex`. The edges of each polygon, rotated into angular order by `halfplane_detail::edges` (factored out of `clipConvex` in this package), are merged in `O(n + m)` and passed to the halfplane deque sweep. The result is an exact `HalfplaneResult`: Empty, Point, Segment or Polygon. `O(n + m)` time and memory. Kept by decision (2026-10-18) beside `clipConvex`, which sorts its second list and costs `O(n + m * log(m))`.

### InscribedKgon, maxAreaInscribedKgon, maxPerimeterInscribedKgonApprox, maxAreaInscribedKgonSmawk, maxPerimeterInscribedKgonSmawkApprox

Input: a strictly convex CCW ring and `k >= 3` (asserted; under `NDEBUG` smaller `k` is raised to 3). The result has increasing vertex indices and a value: twice the area as `lll`, or the perimeter as `long double`. When `k >= n`, every vertex is taken, in `O(n)` time and memory whatever `k` is. Ties in area return one of the optimal k-gons.

All four maximize a cyclic `k`-link path through increasing indices, with weights `w(i, j) = cross(p[i], p[j])` (twice the area, by the shoelace formula) or `|p[i] − p[j]|`. They follow the framework of Boyce–Dobkin–Drysdale–Guibas (SIAM J. Comput. 1985) in the form of Aggarwal–Klawe–Moran–Shor–Wilber (Algorithmica 1987, §6.1):

1. **Rooted optimum.** For a root `r`, layer `t` computes `F_t(j) = max over i < j of F_{t−1}(i) + w(i, j)`. The plain versions find each layer's rightmost row maxima by divide and conquer. The `Smawk` versions use SMAWK, which treats a column at or after its row as invalid and ranked below every column to its left.
2. **First root.** An optimal k-gon `a` through vertex 0 is computed. A global optimum interleaves it, so its first vertex lies in `[0, a[1]]`.
3. **Remaining roots.** Divide and conquer over the roots `s` in `[1, a[1]]`. A subproblem keeps two bounding k-gons, `L` (lower root) and `U` (upper root). For its middle root it computes the best k-gon whose vertex `t` lies in `[L[t], U[t]]`, which means interleaving both bounds. That k-gon becomes the upper bound on the left and the lower bound on the right.

**Why step 1 holds.** Both layer methods need total monotonicity: for columns `a < b` and rows `b < j1 < j2`, if column `b` is at least as good as `a` in row `j1`, it is so in row `j2` too.
- *Perimeter:* the weights are inverse Monge, `w(a, c) + w(b, d) >= w(a, d) + w(b, c)`, because crossing diagonals of a convex quadrilateral are at least as long as two opposite sides.
- *Area:* the weights are not Monge. `cross(p[b] − p[a], p[d] − p[c])` is negative when the chords turn by more than π, for example (0,0), (20,0), (20,2), (0,1). The proof uses the optimal chains instead.
  - In fan form around `r`, let `Δ(x) = −cross(p[b] − p[a], p[x] − p[r]) / 2`. The hypothesis says `F(b) − F(a) >= Δ(j1)`; suppose `F(b) − F(a) < Δ(j2)`.
  - If `Δ(j2) <= Δ(j1)`, this is a contradiction.
  - Otherwise, replacing `a` by `b` in `a`'s optimal chain, whose last edge is `(v, a)`, gives `F(b) − F(a) >= Δ(v)`. `Δ` is linear, so along the arc from `b` to `a` (which holds `j1`, `j2`, `r` and `v` in that order) it first falls and then rises. `Δ(j2) > Δ(j1)` puts `j2` on the rising part, so `Δ(v) >= Δ(j2)`, again a contradiction.
  - The replacement stays inside the column ranges of step 3, so restricted layers are totally monotone too.
- *Probes:* 1.17M 2×2 checks on exact DP values and 1.27M rooted layers on thin and rounded hulls, with 391k quadruples violating the Monge inequality. No violation was found.

**Steps 2 and 3** use only Boyce et al.'s Lemma 2.2, quoted by AKMSW as Fact 2: a globally maximum k-gon and a maximum rooted k-gon interleave, for area and for perimeter. AKMSW apply it inside the family of k-gons that interleave the bounding polygons, which is what step 3 searches.
- **What is not used:** the claim that maximum rooted k-gons at nearby roots interleave each other (restated by Jin–Zhu–Luo). Boyce et al. reportedly say it can fail. Only a search snippet supports that; the paper itself could not be read.
- **Status of the lemma:** Keikha et al. (arXiv 1705.11035) list Lemma 2.2 among established results (their Lemma 8). They place the flaw of Boyce et al.'s and AKMSW's k-gon algorithms only in their Dobkin–Snyder rooted triangle seed; step 2 here computes the true rooted optimum.
- **Not proved here:** the crossing-transform proof of Lemma 2.2 for area could not be read (the SIAM and ACM copies returned 403), so it is cited.
- **Checks:** exhaustive subsets, an all-roots dynamic program, both Keikha counterexamples, and a probe of 157,756 adversarial (polygon, k) cases that found no mismatch.

**Bounds.**
- Plain versions: time `O(k * n * log(n) + n * log(n)^2)`.
- `Smawk` versions: time `O(k * n + n * log(n))`. Each level of the root recursion costs `O(n + k * roots)`, and there are `log(n)` levels.
- All four: memory `O(k * n)`.
- The plain versions are 1.2–1.7× faster at the measured sizes (see Benchmarks); both pairs are kept by decision (2026-10-19).
- The perimeter versions use rounded weights, so monotonicity holds only up to rounding, and near ties may return a near-optimal k-gon.

### Renames and legacy mapping

| Planned name | Implemented as | Reason |
|---|---|---|
| `areaLeftOfLine` | `areaLeftOfLineApprox` | It returns a rounded `long double`; the exact rational area exceeds `lll` |
| `prefixArea` | `prefixArea2` | Doubled area, as `signedArea2` |
| `maxPerimeterInscribedKgon` | `maxPerimeterInscribedKgonApprox` | Rounded value; near ties may change the choice |
| `lineIntersection (boundary crossings)` | `lineIntersection(Linear2)` | One call covers segments, rays and lines |

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py` runs quick, full and stress modes in three configurations, with twelve assertion probes: a clockwise ring, a collinear vertex, two vertices, a pentagram (convex turns, two windings), `tangents` from an interior point, a zero direction, a direction beyond `10^18`, a degenerate cut line, unsorted batch queries, `k = 2` for both k-gon versions, and a clockwise `convexIntersection` input.

| Operation | Test | Oracle |
|---|---|---|
| `contains` | Random hulls at scale 8 (queries at scale 11, every vertex, reflected vertices) and at `10^9`; a 200,000-vertex hull | 03 `polygonContains` |
| `batchedLocate` | Random sorted query sets, vertices and repeated `p[0]`; 200,000 sorted queries | `polygonContains`; `contains` |
| `extreme` | Random directions at scales 3, `10^9` and `10^18`; ± every edge normal (ties) | Argmax scan, first of a tied pair |
| `tangents` | Every outside query above | Ends of the visible edge chain from a scan |
| `pointDistanceApprox` | Every query above | Minimum `pointSegmentDistanceApprox` over all edges, `10^-17` relative |
| `lineIntersection` | Random segments, rays and lines including points; every edge line, reversed edge segment and an outside ray along an edge; random and edge lines on `10^9` hulls | 02 `intersect` with every edge, exact rational parameters, clipped; at `10^9`, `clipConvex` with both halfplanes of the line |
| `cut` | Random lines, every edge in both directions, edge normals; random and edge lines on `10^9` hulls | 10 `clipConvex` with one halfplane (same exact vertex set and kind); CCW and start on the line |
| `areaLeftOfLineApprox` | Same lines | Shoelace area of the exact cut, `10^-12 * scale^2` |
| `prefixArea2` | Every `(i, j)` on scale-8 and `10^9` hulls | `signedArea2` of the chain |
| `convexIntersection` | Random pairs at scale 6 and `10^9`, with and without collinear vertices, both orders, a polygon with itself | 10 `clipConvex` with the other polygon's edges |
| `maxAreaInscribedKgon` | Every `k` in `3..min(n + 1, 8)` on random hulls (`n <= 14`) at scales 20 and `10^9`; regular 8-, 12- and 16-gons; an octagon with parallel sides; Keikha et al.'s 9-vertex (triangle) and 16-vertex (quadrilateral) counterexamples; 30–90 vertex hulls with `k` in `{3, 4, 7, n / 2}`; `k = 2^31 − 1` | Exhaustive subsets; the known optima; an `O(k * n^2)` dynamic program over every root |
| `maxAreaInscribedKgonSmawk` | The same corpus; 20,000-vertex hulls with `k` in `{3, 5, 40}` | The same oracles; identical vertices to `maxAreaInscribedKgon` |
| `maxPerimeterInscribedKgonApprox` | The exhaustive corpus | Exhaustive best perimeter, `10^-15` relative |
| `maxPerimeterInscribedKgonSmawkApprox` | The exhaustive corpus | Exhaustive best perimeter; equal to the plain version within `10^-15` relative |

Mutation checks (2026-10-18; SMAWK and allocation checks 2026-10-19; quick, optimized):
- Caught: stopping the roots before `a[1]`; choosing the wrong smallest edge; leftmost ties in the SMAWK comparison; allocating the `k`-sized layer array before the `k >= n` return (killed at the memory cap).
- Not caught, runtime only: SMAWK's reduce step keeping more columns than rows.
- Not caught: leftmost instead of rightmost layer maxima. The argument needs the rightmost rule only on exact ties, which the random corpus does not produce.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `15-convex_polygon_query_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 12 probes) | PASS, 1,123,660 checks per configuration, 12 probes, `MEMORY peak=706MB` |
| same with `CXX=g++-14` | PASS, 1,123,660 checks per configuration, 12 probes, `MEMORY peak=674MB` |
| `15-convex_polygon_query_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 6,552,079 checks, `MEMORY peak=405MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=654MB` |
| `02-integration.py --sanitizers` | PASS, 167 headers, sanitizer self-tests, `MEMORY peak=1831MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

Measured by `96-Local Testing/03-Geometry/19-lattice_geometry_benchmark.py --seed 20260927` (2026-10-19, conditions in [19-lattice_geometry.md](19-lattice_geometry.md#benchmarks)); random points on a circle of radius `9 * 10^8`, hull taken first; the area versions return the same area and the perimeters agree within `10^-15` relative.

| Hull vertices | k | `maxAreaInscribedKgon` (µs) | `maxAreaInscribedKgonSmawk` (µs) | perimeter, k = 10: plain / `Smawk` (µs) |
|---|---|---|---|---|
| 1,000 | 3 / 10 / 100 | 246 / 570 / 3,781 | 400 / 960 / 5,700 | 1,228 / 1,973 |
| 9,949 | 3 / 10 / 100 | 3,495 / 8,640 / 60,926 | 5,065 / 12,863 / 76,027 | 19,310 / 24,897 |
| 84,428 | 3 / 10 / 100 | 41,009 / 98,659 / 640,044 | 59,276 / 139,890 / 818,196 | 259,964 / 311,107 |

The divide-and-conquer layers win at every measured size. The gap shrinks with `n` (1.5–1.7× at 1,000 vertices, 1.2–1.45× at 84,428), as SMAWK's missing `log(n)` factor starts to pay.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [KACTL PointInsideHull.h and LineHullIntersection.h](https://github.com/kth-competitive-programming/kactl/tree/main/content/geometry), [cp-algorithms point in convex polygon](https://cp-algorithms.com/geometry/point-in-convex-polygon.html) and [maspypy geo/convex_polygon.hpp](https://maspypy.github.io/library/geo/convex_polygon.hpp) (`side`, `min_dot`/`max_dot`, `visible_range`, `boundary_cross_line`, `area_between`, `left_area`): the query family. The algorithms here use angular lower bounds and chain splits instead of KACTL's `extrVertex` case analysis.
- [ei1333 convex_polygon_cut](https://ei1333.github.io/library/geometry/convex_polygon_cut.hpp): an `O(n)` cut; the cut here locates its two crossings in `O(log(n))`.
- [CGAL Inscribed_areas](https://doc.cgal.org/latest/Inscribed_areas/group__PkgInscribedAreasRef.html), [Aggarwal–Klawe–Moran–Shor–Wilber (SMAWK)](https://hcsoso.github.io/teaching/topology-f21/papers/SMAWK.pdf), Boyce et al. (SIAM J. Comput. 1985), and [Keikha et al., arXiv 1705.11035](https://arxiv.org/abs/1705.11035): the k-gon bounds, interleaving and the Dobkin–Snyder flaw.

## Limits and handoffs

- The `Smawk` k-gons reach the best known bound `O(k * n + n * log(n))`. The plain versions cost `O(k * n * log(n) + n * log(n)^2)` but are faster at measured sizes. Linear-time triangle algorithms (Chandran–Mount, Kallus, Jin) are not used, by the notes' contract.
- Degenerate hulls with `n <= 2` are rejected; use `02-line_segment.hpp`.
- Common tangents of two convex polygons, enclosing triangles and inscribed homothets are not adopted (see 00-notes).

## History

- 2026-10-18: first implementation (P037). Independent review: the false inverse-Monge argument for areas was replaced by a monotonicity proof and the tie break removed; `k < 3` under `NDEBUG` is raised to 3; `extreme` asserts its direction bound; the constructor rejects pentagrams; `10^9` line, cut and prefix tests were added; the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit added the `Smawk` k-gons, the interleaving proof and Keikha et al.'s counterexamples. Independent review: a `k`-sized array was allocated before the `k >= n` return (now after it, with a `k = 2^31 − 1` regression). Full suite passed on g++ and g++-14, stress passed.
