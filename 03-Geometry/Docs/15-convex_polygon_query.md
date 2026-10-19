# 15-convex_polygon_query.hpp — evidence

Owned by package P037 / GE05, together with [13-segment_union.md](13-segment_union.md) and [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md). Every operation in the row has a test.

## Contracts

### Numeric policy

Polygons and query points are integral with `|coordinate| <= 10^9`, asserted. Edge vectors stay below `2 * 10^9` and orientations below `8 * 10^18`. Line parameters `num / den` have `|num|, den <= 8 * 10^18`, so comparing two of them takes products below `6.4 * 10^37`. Crossing points are `RationalPoint2` with numerators below `5 * 10^28`. Every topological answer is exact: location, tangents, extreme vertices, intersection points, cut vertices, `prefixArea2`, `convexIntersection` and the area k-gon. `pointDistanceApprox`, `areaLeftOfLineApprox` and the perimeter k-gon are rounded `long double`; `pointDistance2` and `areaLeftOfLine2` are their exact forms.

### ConvexPolygonQuery

The constructor takes a strictly convex counterclockwise ring with `n >= 3`, any starting vertex; this is what `convexHull(..., false)` returns. It asserts `polygonConvex(p, true)` and a positive area in `O(n)`. It stores the ring as given, so every index refers to the input. It also stores the prefix sums of `cross(p[i], p[i + 1])` and the index `s0` of the edge with the smallest direction angle. `O(n)` time and memory. `at`, `edge`, `leftChain` and `crossing` are helpers.

- `contains(q)`: `PolygonLocation` (closed boundary). Binary search over the fan of triangles `(p[0], p[i], p[i + 1])`. `O(log(n))`.
- `batchedLocate(qs)`: locations for queries sorted by `polarLess(q − p[0])`, the library's angle order (asserted in `O(q)`). The results are in query order. Queries equal to `p[0]` are on the boundary. The rest, rotated to start at the direction of `p[1] − p[0]`, cross the wedge at `p[0]` counterclockwise, so one pointer walks the fan forward. It gallops (steps 1, 2, 4, … then halves), so a query that skips `g` wedges costs `O(log(g + 2))` and the batch `O(q * log(2 + n / q))` by concavity: never worse than `O(n + q)` or `q` calls of `contains`. `O(q)` memory. Kept by decision (2026-10-18) as a different algorithm and domain from `q` calls of `contains`.
- `extreme(d)`: the vertex maximizing `dot(d, p[i])`, for `d != 0` with `|d| <= 10^18`. When two vertices tie, it returns the first counterclockwise. The edge directions, rotated to start at `s0`, increase in angle, so this is a lower bound for `perp(d)`. `O(log(n))`.
- `tangents(q)`: for `q` strictly outside (asserted), `{a, b}`. The boundary visible from `q` runs counterclockwise from `p[a]` to `p[b]`. The polygon lies left of `q → p[b]` and right of `q → p[a]`. An edge on a line through `q` is not visible, so ties take the vertex nearer `q`. The search first finds one visible edge, from the fan at `p[0]` or its two edges. It also finds one invisible edge: an edge at `p[0]`, or the exit edge of the ray from `q` through `p[0]`. Two monotone binary searches then find the ends of the visible chain. `O(log(n))`.
- `pointDistanceApprox(q)`: 0 inside or on the boundary. Otherwise the distance to the visible chain: on its edges, "the edge points toward `q`" is true and then false, so a binary search finds the nearest edge. `O(log(n))`.
- `pointDistance2(q)`: the exact squared distance as a reduced fraction `{num, den}`, `{0, 1}` inside or on the boundary. The nearest point lies on the last edge that points toward `q`, past its start: its projection (`cross^2 / |edge|^2`, numerator below `6.4 * 10^37`, denominator below `8 * 10^18`) or its end. No two candidates are compared. `O(log(n))`.
- `lineIntersection(s)`: the intersection of the closed polygon with a segment, ray or line `s` (`s.a == s.b` is a point). It returns `ConvexLineIntersection` with `count` 0, 1 or 2 and exact points ordered along `s.a → s.b`. The two extreme vertices for the normal of `s` split the boundary into two monotone chains. One binary search on each finds the exit and entry crossings, and the parameter interval is then clipped to `s`. Touching at a vertex or along an edge gives one point or a segment. `O(log(n))`.
- `cut(a, b)`: the closed left side of `a → b`, `a != b`, as exact vertices. It is a counterclockwise ring starting at the crossing where the boundary enters the left side, or the whole polygon from index 0. When the line only touches it is one or two points, and it is empty when disjoint. `O(log(n) + out)`. Kept by decision (2026-10-18) for this bound beside `clipConvex` and `polygonCutApprox`.
- `areaLeftOfLineApprox(a, b)`: the area, not doubled, of the same region. The chain part is exact (`prefixArea2`) and the two crossings are rounded. `O(log(n))`.
- `areaLeftOfLine2(a, b)`: twice that area, exactly, as `ExactArea2` (`whole + num / den`). With the origin at `a`, both crossings lie on `a + t (b − a)`, so their mutual term vanishes and each adds `n_i / d_i`: its edge parameter times the cross product of its edge's ends. Here `|n_i| < 6.4 * 10^37` and `0 < d_i = |cross(b − a, edge)| <= 8 * 10^18`. The parts are reduced to `floor` quotients and remainders, so `num < den = d_1 d_2 < 2^126` (before reduction) and every intermediate fits `lll`. A single fraction would need about 190 bits. `O(log(n))`.
- `prefixArea2(i, j)`: twice the area of the polygon `p[i], p[i + 1], ..., p[j]`, cyclic, closed by the chord from `p[j]` back to `p[i]`; `i == j` gives 0. Exact, `O(1)`.

### ConvexLineIntersection

A result record: `count` and `points`; only `points[0..count)` are meaningful.

### ExactArea2

A result record for an exact doubled area: `whole + num / den` with `0 <= num < den` and `gcd(num, den) = 1`. Comparing two values exactly needs products of about 252 bits (Core `UInt` or `InfInt`).

### convexIntersection

Input: two convex CCW polygons, collinear vertices allowed, positive area, `|coordinate| <= 10^9`, as in `clipConvex`. The edges of each polygon, rotated into angular order by `halfplane_detail::edges` (factored out of `clipConvex` in this package), are merged in `O(n + m)` and passed to the halfplane deque sweep. The result is an exact `HalfplaneResult`: Empty, Point, Segment or Polygon. `O(n + m)` time and memory. Kept by decision (2026-10-18) beside `clipConvex`, which sorts its second list and costs `O(n + m * log(m))`.

### InscribedKgon, maxAreaInscribedKgon, maxPerimeterInscribedKgonApprox, maxAreaInscribedKgonSmawk, maxPerimeterInscribedKgonSmawkApprox, maxAreaInscribedTriangle, maxAreaInscribedQuadrilateral

Input: a strictly convex CCW ring and `k >= 3` (asserted; under `NDEBUG` smaller `k` is raised to 3). The result has increasing vertex indices and a value: twice the area as `lll`, or the perimeter as `long double`. When `k >= n`, every vertex is taken, in `O(n)` time and memory whatever `k` is. Ties in area return one of the optimal k-gons.

The four general versions maximize a cyclic `k`-link path through increasing indices, with weights `w(i, j) = cross(p[i], p[j])` (twice the area, by the shoelace formula) or `|p[i] − p[j]|`. They follow Boyce–Dobkin–Drysdale–Guibas (STOC 1982, read from the conference version; SIAM J. Comput. 1985) in the form of Aggarwal–Klawe–Moran–Shor–Wilber (Algorithmica 1987, §6.1):

1. **Rooted optimum.** For a root `r`, layer `t` computes `F_t(j) = max over i < j of F_{t−1}(i) + w(i, j)`. The plain versions find each layer's rightmost row maxima by divide and conquer. The `Smawk` versions use SMAWK, which treats a column at or after its row as invalid and ranked below every column to its left. Each comparison evaluates one new value, the other being cached with its stack entry or the row's current best. Every recursion level pushes its columns, rows and answers on one workspace reserved per layer (`m + 6n + 64` indices), so SMAWK allocates nothing per level.
2. **First root.** A maximum k-gon `a` through vertex 0 is computed. Some global maximum `G` has `a ≤ G ≤ a⁺` (below), so its first vertex lies in `[0, a[1]]`.
3. **Remaining roots.** Divide and conquer over the roots `s` in `[1, a[1]]`. A subproblem keeps two bounding k-gons, `L` (lower root) and `U` (upper root). For its middle root it computes the best k-gon `Q` rooted at `s` with vertex `t` in `[L[t], U[t]]`. `Q` becomes the upper bound on the left and the lower bound on the right.

**Why step 1 holds.** Both layer methods need total monotonicity: for columns `a < b` and rows `b < j1 < j2`, if column `b` is at least as good as `a` in row `j1`, it is so in row `j2` too.
- *Perimeter:* the weights are inverse Monge, `w(a, c) + w(b, d) >= w(a, d) + w(b, c)`, because crossing diagonals of a convex quadrilateral are at least as long as two opposite sides.
- *Area:* the weights are not Monge. `cross(p[b] − p[a], p[d] − p[c])` is negative when the chords turn by more than π, for example (0,0), (20,0), (20,2), (0,1). The proof uses the optimal chains instead.
  - In fan form around `r`, let `Δ(x) = −cross(p[b] − p[a], p[x] − p[r]) / 2`. The hypothesis says `F(b) − F(a) >= Δ(j1)`; suppose `F(b) − F(a) < Δ(j2)`.
  - If `Δ(j2) <= Δ(j1)`, this is a contradiction.
  - Otherwise, replacing `a` by `b` in `a`'s optimal chain, whose last edge is `(v, a)`, gives `F(b) − F(a) >= Δ(v)`. `Δ` is linear, so along the arc from `b` to `a` (which holds `j1`, `j2`, `r` and `v` in that order) it first falls and then rises. `Δ(j2) > Δ(j1)` puts `j2` on the rising part, so `Δ(v) >= Δ(j2)`, again a contradiction.
  - The replacement stays inside the column ranges of step 3, so restricted layers are totally monotone too.
- *Probes:* 1.17M 2×2 checks on exact DP values and 1.27M rooted layers on thin and rounded hulls, with 391k quadruples violating the Monge inequality. No violation was found.

**Notation for steps 2 and 3.** A k-gon is an index sequence `v_0 < … < v_{k−1} < v_0 + n` (indices mod `n` name vertices, `v_k = v_0 + n`), with value `m(V) = Σ_t w(v_t, v_{t+1})`. For k-gons `L ≤ U ≤ L⁺` (componentwise, `L⁺_t = L_{t+1}`) the family `F(L, U)` holds the k-gons with `L_t ≤ v_t ≤ U_t`. The componentwise `min` and `max` of two k-gons are k-gons.

**Vertex lemma.** Let `A, D` be consecutive vertices of a k-gon `X` (`k ≥ 3`) with successor `E` of `D` and predecessor `E'` of `A`, and let `A ≤ B < C ≤ D` be vertices of the strictly convex polygon. If moving `D` to `C`, or moving `A` to `B`, does not increase the area of `X`, then `cross(B − A, D − C) ≥ 0`.
- Write `ψ(x, y)` for the direction of the chord from vertex `x` to a later vertex `y`; it increases strictly in each endpoint, and `ψ(y, x + n) = ψ(x, y) + π`.
- Moving `D` to `C` changes twice the area by `cross(C − D, E − A)`. Since `ψ(A, E) − π < ψ(A, B) < ψ(C, D) < ψ(E, A + n) = ψ(A, E) + π`, "no increase" means `ψ(C, D) ≤ ψ(A, E)`. Then `ψ(A, B) < ψ(C, D) ≤ ψ(A, E) < ψ(B, A + n) = ψ(A, B) + π`, so the turn from `B − A` to `D − C` lies in `(0, π)`.
- Moving `A` to `B` changes it by `cross(B − A, D − E')`; by the same range argument (`ψ(E', D) − π < ψ(E', A) < ψ(A, B) < ψ(D, E' + n)`) "no increase" means `ψ(E', D) ≤ ψ(A, B)`, and then `ψ(A, B) < ψ(C, D) < ψ(D, E' + n) = ψ(E', D) + π`. Both cases give a positive turn when `A < B` and `C < D`, and 0 otherwise.

**Exchange lemma.** Let `P` be a maximum over a family (all k-gons, or `F(L, U)`) and `Q` a maximum over its members that have the vertex `r` at a fixed position (rooted at `r`), labelled so that one of `min(P, Q)`, `max(P, Q)` has `r` at that position. Then `m(min(P, Q)) + m(max(P, Q)) ≥ m(P) + m(Q)`.
- Edge by edge: where `P` and `Q` keep their order at `t` and `t + 1`, `min` and `max` use the same two edges. Otherwise one polygon's edge `(A, D)` contains the other's edge `(B, C)`, `A ≤ B < C ≤ D`, and `min` and `max` use `(A, C)` and `(B, D)` instead. The change is `w(A, C) + w(B, D) − w(A, D) − w(B, C)`.
- *Perimeter:* `≥ 0`, the diagonals of the convex quadrilateral `ABCD` are at least as long as two opposite sides.
- *Area:* the change of twice the area is `cross(B − A, D − C)`. Moving `D` to `C` or `A` to `B` keeps the outer polygon in the family (the new vertex comes from the other polygon at the same position). If the outer polygon is `P`, maximality forbids an increase. If it is `Q`, at least one of the two moves keeps `r` at its position, because `A` and `D` are not both `r`. The vertex lemma then gives `≥ 0`.

**Why step 2 holds.** If a global maximum `G` avoids vertex 0, label it inside `(0, n)`. The exchange lemma with `Q = a` makes `G' = max(G, a)` a global maximum, since `min(G, a)` passes through 0 and is worth at most `m(a)`. Applied again with `Q = a⁺ = (a_1, …, a_{k−1}, n)`, the same k-gon labelled so that vertex 0 comes last, it makes `min(G', a⁺)` a global maximum. That k-gon lies between `a` and `a⁺`, so its first vertex lies in `[1, a[1]]`.

**Why step 3 holds.** Invariant: some global maximum `G` lies in `F(L, U)` with first vertex in `[sl, sr]`. If `g_0 = s`, `Q` is optimal. If `g_0 < s`, the exchange lemma makes `min(G, Q)` a global maximum, since `max(G, Q)` is rooted at `s` inside `F(L, U)`. It lies in `F(L, Q)` with first vertex `g_0`. The case `g_0 > s` is symmetric, with `max(G, Q)` in `F(Q, U)`. No tie rule is needed: any maximum rooted k-gon works as a bound.

**Relation to the paper.** The exchange is Boyce et al.'s crossing transform (Lemmas 2.2 and 2.3, pp. 284–285), applied at every crossing at once. For area their sketch derives the needed inequality from moving one vertex of the polygon that holds edge `AD`. With a fixed root that move may be forbidden; the vertex lemma's second move closes this case. Boyce et al. also show (p. 287, Fig. 10) that maximum rooted k-gons at different roots need not interleave; that claim is not used. Keikha et al. (arXiv 1705.11035) place the flaw of the published linear-time methods in the Dobkin–Snyder rooted triangle (Boyce et al.'s Lemma 3.2); step 1 computes the true rooted optimum.

**Checks.** Exhaustive subsets, an all-roots dynamic program, the Keikha counterexamples, Boyce et al.'s Fig. 10 and Fig. 12, a corpus with many exact ties, and a probe of 157,756 adversarial (polygon, k) cases. A separate exact-integer probe checked 4,792 instances of the vertex lemma and the exchange inequality for 7,889 pairs (every tied maximizer) on random lattice polygons.

**Memory.** Only the first rooted search is wide: the interleaved searches of step 3 have layer widths summing to at most `n + k`. A path solver stores its argmax table when it holds at most `max(2 (span + layers), 2^23)` entries (32 MB of indices, a constant). Otherwise its forward pass carries, for every state, its vertex at the middle layer, and the two halves are solved again (Hirschberg). The forward pass's buffers are freed before the halves are solved, so the live memory is one forward pass plus `O(layers)` bounds per recursion level. The halves' spans partition the original span, so the recomputation at most doubles the first search, which dominates the total for large `k`; below the constant cap nothing is recomputed. The detail functions take the cap as a parameter, and the tests force it to 0. The recursion over roots keeps `O(log(n))` bounding k-gons.

**Linear special cases.**
- `maxAreaInscribedTriangle`: Jin–Zhu–Luo's Rotate-and-Kill (arXiv 1707.04071 v7, Algorithms 3, 4 and 1), over the clockwise relabelling the paper uses. Algorithm 3 finds the maximum triangle rooted at vertex 0 by two pointers. Algorithm 4 moves it to a 3-stable triangle: all three corners are moved forward (or all backward) in rounds until none moves, each move raising the area. By step 2 with `k = 3` there is a global maximum `G` (3-stable) with `T0 <= G <= T0⁺` for the rooted start `T0`. The first maximizer of each corner advances monotonically with the other two corners, so forward moves never pass `G`'s corners, and the moves total at most `n` (backward moves symmetrically, against a global maximum between `T0⁻` and `T0`; at most `0.75 n` were measured). Stability and the exact stop of the main loop are asserted. Algorithm 1 then visits every 3-stable triangle, which includes the maximum. Kill compares the distance of `a` to line `bd` with that of `I`, where the rays `b + x e_d` and `d − y e_b` (`x, y ≥ 0`) meet; rays that do not meet put `I` at infinity. The rays and the comparison are exact sign tests with degree-4 products below `6.4 * 10^37`. The loop is bounded by `2n + 1` rounds. A stress run of 199,850 hulls (three distributions, up to 40 vertices) matched a cubic scan everywhere; the rays never met with a negative denominator in 3.7 million kill tests.
- `maxAreaInscribedQuadrilateral`: Rote's antipodal sweep (arXiv 1905.11203 v2, Appendix B): for every antipodal pair `(a, c)` the farthest vertices `b` and `d` on each side advance monotonically. Every comparison is one cross product. On valid input `a` and `c` advance `n` times in total; the loop runs exactly `n` rounds, so a violated precondition under `NDEBUG` still stops, and the end state is asserted.
- Both replace the `O(n * log(n))` cost of the general versions for `k = 3, 4`; the Dobkin–Snyder area-only advance, which fails (Keikha et al.), is not used.

**Bounds.**
- Plain versions: time `O(k * n * log(n) + n * log(n)^2)`.
- `Smawk` versions: time `O(k * n + n * log(n))`. Each level of the root recursion costs `O(n + k * roots)`, and there are `log(n)` levels.
- All four: memory `O(n + k * log(n))`.
- Triangle and quadrilateral: time and memory `O(n)`.
- The plain versions are faster than the `Smawk` versions at the measured sizes (see Benchmarks); both pairs are kept by decision (2026-10-19).
- The perimeter versions use rounded weights, so monotonicity holds only up to rounding, and near ties may return a near-optimal k-gon. An exact perimeter version would compare sums of square roots, an open problem.

### Renames and legacy mapping

| Planned name | Implemented as | Reason |
|---|---|---|
| `areaLeftOfLine` | `areaLeftOfLineApprox` | It returns a rounded `long double`; `areaLeftOfLine2` is the exact doubled area in mixed form |
| `prefixArea` | `prefixArea2` | Doubled area, as `signedArea2` |
| `maxPerimeterInscribedKgon` | `maxPerimeterInscribedKgonApprox` | Rounded value; near ties may change the choice |
| `lineIntersection (boundary crossings)` | `lineIntersection(Linear2)` | One call covers segments, rays and lines |

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py` runs quick, full and stress modes in three configurations, with twelve assertion probes: a clockwise ring, a collinear vertex, two vertices, a pentagram (convex turns, two windings), `tangents` from an interior point, a zero direction, a direction beyond `10^18`, a degenerate cut line, unsorted batch queries, `k = 2` for both k-gon versions, and a clockwise `convexIntersection` input.

| Operation | Test | Oracle |
|---|---|---|
| `contains` | Random hulls at scale 8 (queries at scale 11, every vertex, reflected vertices) and at `10^9`; a 200,000-vertex hull | 03 `polygonContains` |
| `batchedLocate` | Random sorted query sets, vertices and repeated `p[0]`; 200,000 sorted queries; 1, 2, 7 and 60 sorted queries on the 200,000-vertex hull (long gallops) | `polygonContains`; `contains` |
| `extreme` | Random directions at scales 3, `10^9` and `10^18`; ± every edge normal (ties) | Argmax scan, first of a tied pair |
| `tangents` | Every outside query above | Ends of the visible edge chain from a scan |
| `pointDistanceApprox` | Every query above | Minimum `pointSegmentDistanceApprox` over all edges, `10^-17` relative |
| `pointDistance2` | Every query above, scales 8 and `10^9` | Exact minimum over all edges of the clamped squared segment distance, compared with 256-bit products, reduced |
| `lineIntersection`, `ConvexLineIntersection` | Random segments, rays and lines including points; every edge line, reversed edge segment and an outside ray along an edge; random and edge lines on `10^9` hulls | 02 `intersect` with every edge, exact rational parameters, clipped; at `10^9`, `clipConvex` with both halfplanes of the line |
| `cut` | Random lines, every edge in both directions, edge normals; random and edge lines on `10^9` hulls | 10 `clipConvex` with one halfplane (same exact vertex set and kind); CCW and start on the line |
| `areaLeftOfLineApprox` | Same lines | Shoelace area of the exact cut, `10^-12 * scale^2` |
| `areaLeftOfLine2`, `ExactArea2` | Same lines, both directions | Reduced proper fraction; the two sides sum exactly to `signedArea2`; exact rational shoelace of the cut ring at scale 8; the rounded area at `10^9` |
| `prefixArea2` | Every `(i, j)` on scale-8 and `10^9` hulls | `signedArea2` of the chain |
| `convexIntersection` | Random pairs at scale 6 and `10^9`, with and without collinear vertices, both orders, a polygon with itself | 10 `clipConvex` with the other polygon's edges |
| `maxAreaInscribedKgon` | Every `k` in `3..min(n + 1, 8)` on random hulls (`n <= 14`) at scales 20 and `10^9`; regular 8-, 12- and 16-gons; an octagon with parallel sides; Keikha et al.'s 9-vertex (triangle) and 16-vertex (quadrilateral) counterexamples; 30–90 vertex hulls with `k` in `{3, 4, 7, n / 2}`; `k = 2^31 − 1` | Exhaustive subsets; the known optima; an `O(k * n^2)` dynamic program over every root; both layer methods again with the Hirschberg pass forced (cap 0) |
| `maxAreaInscribedKgonSmawk` | The same corpus; 20,000-vertex hulls with `k` in `{3, 5, 40}` | The same oracles; identical vertices to `maxAreaInscribedKgon` |
| `maxPerimeterInscribedKgonApprox` | The exhaustive corpus | Exhaustive best perimeter, `10^-15` relative |
| `maxPerimeterInscribedKgonSmawkApprox` | The exhaustive corpus | Exhaustive best perimeter; equal to the plain version within `10^-15` relative |
| All k-gons (`InscribedKgon`), ties | Centrally symmetric hulls (scales 3 and 6) and their shears, lattice-disk hulls of radius 1–5, an affine-regular hexagon; every `k` in `3..min(n + 1, 8)` | Exhaustive subsets; identical vertices from the plain and `Smawk` versions |
| All k-gons, Boyce et al. | Fig. 10 (the largest perimeter triangle C'BC) and Fig. 12 (the largest pentagon A D B D' C), every `k` | The paper's optima; exhaustive subsets |
| `maxAreaInscribedTriangle` | The exhaustive, ties and Boyce corpora; Keikha's 9-gon; 30–90 vertex hulls; 20,000- and 200,000-vertex hulls; the 19-gon regression; quick 400, full 6,000, stress 40,000 near-circular, flattened and uniform hulls up to 40 vertices | Exhaustive subsets; the all-roots dynamic program; the general k-gon; a cubic scan |
| `maxAreaInscribedQuadrilateral` | The same, Keikha's 16-gon; hulls up to 24 vertices in the stress phase | The same oracles; a quartic scan |

Mutation checks (2026-10-18 and 2026-10-19 on the final header, in a private copy of the tree; quick mode, optimized and checked builds):
- Caught by an independent oracle: stopping the roots before `a[1]`; choosing the wrong smallest edge; allocating the `k`-sized layer array before the `k >= n` return (memory cap); the Hirschberg pass not carrying the middle vertex (forced recomputation against the exhaustive optimum); the triangle's kill always advancing `b`, ignoring whether the rays meet, or comparing areas instead (the cubic-scan stress phase); skipping the stabilising pass, or not revisiting `r` in it (the 3-stability assertion on the 19-gon); the quadrilateral skipping `d`; galloping without the halving phase; `pointDistance2` on the wrong edge; `areaLeftOfLine2` rounding toward zero or dropping the carry; SMAWK's workspace without its reserve (a crash).
- Caught only by the cross-check that the plain and `Smawk` versions return identical vertices: leftmost instead of rightmost maxima in the divide and conquer, in SMAWK's reduce step and in its interpolation. They pick another optimal k-gon, which the contract allows: with the cross-check removed no oracle catches them, and a search of 1,345,715 tie-heavy (polygon, `k`) cases against the all-roots dynamic program found no wrong value. The proof of step 1 uses the rightmost rule.
- Not caught, equivalent in results: a looser Hirschberg clip that lets the left half end on the middle vertex (that path has a zero edge and one vertex fewer, so it is strictly worse); ignoring the kill denominator's sign (the rays never meet with a negative denominator in the stress run); advancing `c` instead of `a` on an antipodal tie in the quadrilateral sweep (Rote: ties need no care).
- Not caught, runtime only: always using the Hirschberg pass.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `15-convex_polygon_query_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 12 probes) | PASS, 1,504,912 checks per configuration, 12 probes, `MEMORY peak=1230MB` (the ASan build of the full-hull comparisons) |
| same with `CXX=g++-14` | PASS, 1,504,912 checks per configuration, 12 probes, `MEMORY peak=1229MB` |
| `15-convex_polygon_query_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 9,133,475 checks, `MEMORY peak=452MB` |
| `-Wall -Wextra -Wconversion` build of the tester and benchmark with `g++` and `g++-14`, and the header with `-DNDEBUG` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 168 headers, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/15-convex_polygon_query_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`15-convex_polygon_query_benchmark.py --seed 20260927` (2026-10-19; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; run serially on the final code, other sessions keeping the load average near 2.7). Hulls of random points on a circle of radius `9 * 10^8`; input generation and hulls are excluded. The plain and `Smawk` k-gons must agree (perimeters within `10^-15` relative), the linear triangle and quadrilateral must equal the general `k = 3, 4`, and batches must equal per-query `contains`.

| Hull vertices | k | `maxAreaInscribedKgon` (µs) | `maxAreaInscribedKgonSmawk` (µs) | perimeter, k = 10: plain / `Smawk` (µs) |
|---|---|---|---|---|
| 1,000 | 3 / 10 / 100 | 247 / 560 / 3,762 | 379 / 889 / 6,380 | 1,330 / 2,200 |
| 9,949 | 3 / 10 / 100 | 4,103 / 8,859 / 58,915 | 5,972 / 12,238 / 73,337 | 18,888 / 24,666 |
| 84,430 | 3 / 10 / 100 | 40,614 / 93,896 / 581,251 | 57,070 / 122,031 / 665,200 | 223,586 / 258,254 |

The divide-and-conquer layers win at every measured size; the gap shrinks with `n` (1.5–1.7× at 1,000 vertices, 1.1–1.4× at 84,430).

| Hull vertices | `maxAreaInscribedTriangle` | `maxAreaInscribedQuadrilateral` | `maxAreaInscribedKgon`, k = 4 (µs) |
|---|---|---|---|
| 1,000 | 90 | 44 | 363 |
| 9,949 | 981 | 530 | 4,883 |
| 84,430 | 8,064 | 4,635 | 51,127 |

The linear triangle is 2.7–5.0× faster than the general `k = 3` (table above) and the quadrilateral 8–11× faster than the general `k = 4`.

Large `k`, one run each in its own process, 84,304 hull vertices:

| k | plain (s) | `Smawk` (s) | peak resident set |
|---|---|---|---|
| 1,000 | 8.6 | 10.2 | 30 MB |
| 10,000 | 77.7 | 98.8 | 21 MB |

A stored table would hold `k * (n − k)` predecessors: about 330 MB and 3 GB of indices.

| Located queries (135,175 hull vertices) | `batchedLocate` (µs) | `contains` per query (µs) |
|---|---|---|
| 10 | 2 | 1 |
| 1,000 | 104 | 150 |
| 200,000 | 6,199 | 20,421 |

100,000 calls on the same hull: `pointDistance2` 62 ms against `pointDistanceApprox` 52 ms; `areaLeftOfLine2` 150 ms against `areaLeftOfLineApprox` 203 ms.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [KACTL PointInsideHull.h and LineHullIntersection.h](https://github.com/kth-competitive-programming/kactl/tree/main/content/geometry), [cp-algorithms point in convex polygon](https://cp-algorithms.com/geometry/point-in-convex-polygon.html) and [maspypy geo/convex_polygon.hpp](https://maspypy.github.io/library/geo/convex_polygon.hpp) (`side`, `min_dot`/`max_dot`, `visible_range`, `boundary_cross_line`, `area_between`, `left_area`): the query family. The algorithms here use angular lower bounds and chain splits instead of KACTL's `extrVertex` case analysis.
- [ei1333 convex_polygon_cut](https://ei1333.github.io/library/geometry/convex_polygon_cut.hpp): an `O(n)` cut; the cut here locates its two crossings in `O(log(n))`.
- [CGAL Inscribed_areas](https://doc.cgal.org/latest/Inscribed_areas/group__PkgInscribedAreasRef.html), [Aggarwal–Klawe–Moran–Shor–Wilber (SMAWK)](https://hcsoso.github.io/teaching/topology-f21/papers/SMAWK.pdf), Boyce et al. (STOC 1982, pp. 282–289, read from a copy the user supplied; SIAM J. Comput. 1985), and [Keikha et al., arXiv 1705.11035](https://arxiv.org/abs/1705.11035): the k-gon bounds, the crossing transform, the non-interleaving rooted optima (Fig. 10) and the Dobkin–Snyder flaw.
- [Jin–Zhu–Luo, arXiv 1707.04071 v7](https://arxiv.org/abs/1707.04071) (Algorithms 1, 3 and 4, Lemmas 2, 6 and 22) and [Rote, arXiv 1905.11203 v2](https://arxiv.org/abs/1905.11203) (Appendix B, Theorem 5): the linear triangle and quadrilateral; extracted by `@researcher`, with the stabilising pass corrected and stress-tested here.

## Limits and handoffs

- The `Smawk` k-gons reach the standard bound `O(k * n + n * log(n))`; for `k` well above `log(n)`, k-link path methods (Aggarwal–Schieber–Tokuyama, Schieber) are faster in theory and are planned as `59-kgon_klink.hpp` (P236). `k = 3` and `k = 4` are linear.
- Scope: an exact maximum-perimeter k-gon would compare sums of square roots, an open problem; distances and perimeters stay rounded, their squares and doubled areas are exact.
- Degenerate hulls with `n <= 2` are rejected; use `02-line_segment.hpp`.
- Common tangents of two convex polygons belong to row 53; the minimum enclosing triangle and parallelogram are planned as `57-enclosing_shapes.hpp` (P234); circumscribing k-gons, inscribed homothets and axis rectangles are not adopted (see 00-notes).

## History

- 2026-10-18: first implementation (P037). Independent review: the false inverse-Monge argument for areas was replaced by a monotonicity proof and the tie break removed; `k < 3` under `NDEBUG` is raised to 3; `extreme` asserts its direction bound; the constructor rejects pentagrams; `10^9` line, cut and prefix tests were added; the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit added the `Smawk` k-gons, the interleaving proof and Keikha et al.'s counterexamples. Independent review: a `k`-sized array was allocated before the `k >= n` return (now after it, with a `k = 2^31 − 1` regression). Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19 (round 3): the cited interleaving lemmas replaced by a proof (vertex and exchange lemmas; checked by an independent reviewer and an exact-integer probe), with Boyce et al.'s Fig. 10 and Fig. 12 as regressions; k-gon memory `O(n + k * log(n))` through a Hirschberg pass beyond a `2^23`-entry table; SMAWK with cached values and one workspace per layer; linear `maxAreaInscribedTriangle` (Jin–Zhu–Luo) and `maxAreaInscribedQuadrilateral` (Rote); galloping `batchedLocate`; exact `pointDistance2` and `areaLeftOfLine2`; a tie corpus and a cubic-scan stress phase. A stress run found that the first transcription of Jin et al.'s Algorithm 4 could stop at an unstable triangle (fixed; 19-gon regression). Independent review: the quadrilateral loop bounded under `NDEBUG`, the Hirschberg buffers freed before recursing, SMAWK's per-level allocations removed, the exchange-lemma wording and the stabilising-pass bound corrected, ledgers updated. Final full suite passed on g++ and g++-14, stress passed.
