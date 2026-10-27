# 29-planar_graph.hpp — evidence

Owned by package P048 / GR13; omissions are in [00-notes.md](00-notes.md#p048-gr13-omissions). Contest profile: a half-edge rotation system over the canonical `Graph` arcs, with face tracing, a builder from straight-line drawings, a dual graph, tree–cotree decomposition, and the s–t minimum cut of a plane network by a dual shortest path, with a max-flow witness. A new row with no legacy code; planarity testing is row 62.

## Contracts

### PlanarEmbedding

`PlanarEmbedding(g)` takes an undirected `Graph` or `CsrGraph` (asserted; a directed graph under `NDEBUG` gets components, `outer` and `genus` but no faces, so every cut query returns the zero cut; this and genus-1 cuts with negative capacities under `NDEBUG` were checked by ad-hoc ASan probes in the 2026-10-28 review, since `05-testing.md` keeps optimized-build test inputs inside the domain) and reads `g[v]` as `v`'s arcs in counter-clockwise order. Any rotation system is accepted, of any genus, with loops and parallel edges. Half-edges are the arcs of `g` (arc `a` runs from `arcs[a].from` to `arcs[a].to`, with twin `arcs[a].rev`), so arc and edge IDs match the input graph. To fix a rotation, permute `g.adj[v]` before building. Fields:

- `next[a]`: the next arc on the face to the left of `a`. For `a = u→v`, it is the arc clockwise after `v→u` around `v`. `prev` is its inverse, and `next[arcs[a].rev]` steps clockwise around `a`'s tail.
- `first[v]`: `g[v][0]`, or −1 for an isolated vertex.
- `faces`: the orbits of `next` as arc lists in boundary order. Faces are numbered by scanning `v = 0..n−1` and `v`'s arcs in rotation order, starting a face at each unassigned arc. An isolated vertex gets one empty face at its scan position, so that `n − m + F = 2 · components − 2 · Σ genus[c]` holds for every input. A bridge appears twice on its face.
- `face[a]`: the face to the left of `a`. The face to the right of `a` is `face[arcs[a].rev]`.
- `component`, `components`: connected components, numbered by increasing minimum vertex.
- `genus[c]`: the genus of component `c`'s rotation system, `(2 − (V_c − E_c + F_c)) / 2`, where an isolated vertex's empty face counts in its component; computed during face tracing.
- `outer[c]`: the outer face of component `c`. It is −1 unless `c` is a single isolated vertex (its empty face) or the embedding came from `fromDrawing`. Nested components are not resolved; that is point location (Geometry row 34).

Construction costs O(n + m) time and memory. `eulerCharacteristic()` is `n − m + F` in O(1); the total genus `Σ genus[c]` is `(2 · components − eulerCharacteristic()) / 2`.

### fromDrawing

`PlanarEmbedding::fromDrawing(g, xy)` takes a crossing-free straight-line drawing of a simple graph, with distinct integer points and `|x|, |y| ≤ 2^61`. The coordinate bound, and the absence of zero and repeated directions around each vertex (loops, coincident edge endpoints, parallel edges), are asserted in O(n + m); coincident points that share no edge and crossings are not checked. It sorts each rotation by exact counter-clockwise angle from direction (1, 0) inclusive: half 0 is `[0, π)` and half 1 is `[π, 2π)`, and ties are broken by the sign of an `lll` cross product of differences, which stay below 2^62. A zero vector goes to half 2, so the comparator stays a strict weak order under `NDEBUG`. It then builds the embedding and sets `outer[c]` to the face with nonpositive doubled signed area. Bounded faces trace counter-clockwise with positive area, the outer face traces clockwise, and a tree's single face has area 0. The area sum wraps in `ulll`, which is exact because the final value is bounded by the bounding box area, below 2^125. O(n + m log m).

### commonFace

`commonFace(u, v)` (`u ≠ v`, asserted) returns the smallest face whose boundary contains both vertices, or −1 when there is none. An isolated vertex shares no face with another vertex. It walks both rotations, sorts `u`'s faces and searches them: O((deg u + deg v) · log deg u).

### dualGraph

An undirected `Graph` with dual vertex `f` = face `f`. Dual edge `e` joins the faces to the left and right of edge `e`'s first arc, and keeps weight `w`. Bridges become dual loops, and a primal loop becomes a dual bridge unless its two sides coincide. O(F + m).

### TreeCotree and treeCotree

Three lists of edge IDs, ascending, that partition `[0, m)`:

- `tree`: a spanning forest taken greedily in ID order with a DSU over vertices.
- `cotree`: a spanning forest of the dual taken greedily over the remaining edges, by their dual endpoints.
- `leftover`: the rest.

Euler's formula gives `|leftover| = 2 · Σ genus[c]` (Eppstein's tree–cotree decomposition), so planar embeddings have no leftovers. The dual of the complement of a primal spanning tree is then a spanning tree. O((n + m + F) · α).

### PlanarCut and planarMinCutDual

`planarMinCutDual(s, t, cap = {})` asserts that `s ≠ t` are in range and, when they are connected, that their component has genus 0 (`genus`, an O(1) check, so other components may be non-planar; user decision 2026-10-28) and that they share a face (`commonFace`). `cap[a] ≥ 0` is the capacity of arc `a`, so directed networks are allowed, up to `LLONG_MAX`; an empty `cap` gives both arcs the edge weight (undirected). If `s` and `t` are disconnected, the result is a zero cut with zero flow. Under `NDEBUG`, a missing common face returns the zero cut, and negative capacities (or a `cap` of the wrong size, which falls back to the weights) are clamped to 0, so Dijkstra never meets a negative cycle.

The method inserts a virtual chord `s–t` into the smallest common face `f`. The arcs of `f` from `t` round to `s` move to a new dual vertex `k`, and the rest of `f` stays at `f`. Each arc `a` becomes a dual arc from its left side to its right side with length `cap[a]`. A dual path from `f` to `k` crosses arcs from the `s` side to the `t` side, so its length is the capacity of a cut, and every cut arises this way. Dijkstra (row 05) therefore returns the minimum cut. This is the OI Wiki 平面图最小割 / BZOJ 1001 reduction with oriented dual arcs (NOI2010 海拔).

Result:

- `value`: the minimum cut, `lll`.
- `arcs`: the dual path's arcs, ascending, whose capacities sum to `value`.
- `flow[e]`: `d(right) − d(left)` for edge `e`'s first arc. Dual shortest-path distances are potentials (Hassin 1981). Feasibility follows from `d(right) ≤ d(left) + cap[a]` and `d(left) ≤ d(right) + cap[rev]`. Conservation holds because around any vertex other than `s` and `t` the differences telescope over the cyclic sequence of corner faces. At `s` and `t` the virtual chord breaks the cycle and leaves `value`.

Faces of other components get no flow. O((n + m) log(n + m)).

## Aliases

- NetworkX `traverse_face`, Boost `planar_face_traversal`, maspypy `get_face_data` are `faces`. OGDF `rightFace` and maspypy `left_face` are `face`. OGDF `findCommonFace` is `commonFace`, and OGDF `externalFace` and maspypy face 0 are `outer`.
- OGDF `MinSTCutDijkstra` over `DualGraph` and `MaxFlowSTPlanarDigraph` are `planarMinCutDual` (cut and flow).
- Row name `primalDualTraversal` became `treeCotree` (decided 2026-10-28). `faceOf(a)` left the row in review: it was `face[a]` behind a range assert (One API per operation).

## Feature-to-test map

`96-Local Testing/04-Graphs/29-planar_graph_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with 17 assertion probes, ASan/UBSan in full and stress).

| Operation | Test | Oracle |
|---|---|---|
| Constructor fields `next`, `prev`, `first`, `faces`, `face`, `component`, `components`, `outer`, `genus` | `randomRotations` (multigraphs n ≤ 7, m ≤ 10, loops, multiedges, isolated vertices, random rotations of any genus); `large` (random trees and cycles up to 600,000 vertices); hand cases (n = 0, 1, single edge, loop, parallel pair, K3,3) | Own orbit computation and face numbering scan, self-checked (permutation, closed walks, lengths sum to 2m); BFS components; per-component V − E + F for `genus`; Graph and CsrGraph agree |
| `fromDrawing` rotation and `outer` | `randomDrawings` (crossing-free n ≤ 12), `midDrawings` (n ≤ 200), 300×300 grid with diagonals, `hugeDrawings` at \|x\|, \|y\| ≤ 2^61 | Own exact quarter-plane and cross-product order (validated against `atan2`), shuffle invariance, scaling and shift invariance; outer face from the lowest-then-leftmost vertex and exact face areas |
| `eulerCharacteristic` | Every embedding | `n − m + F`, even genus ≥ 0, exactly `2c` on drawings, genus ≥ 1 on K3,3 |
| `commonFace` | All pairs n ≤ 12; sampled pairs on mid drawings; grid pair with no common face | Brute face scan |
| `dualGraph` | Every embedding, repeated | Endpoints from the oracle faces, weights, bridges as loops |
| `treeCotree`, `TreeCotree` | Every embedding, repeated | Own greedy DSU over vertices then faces; sizes `n − c`, `F − c`, `2 · Σ genus[c]` |
| `planarMinCutDual`, `PlanarCut` | Every ordered face-sharing pair on drawings n ≤ 12 and genus-0 multigraph rotations, undirected plus directed capacities (zeros, up to 2^61, capacities up to `LLONG_MAX`, totals beyond 64 bits); sampled pairs on mid drawings and the grid; trees and cycles; disconnected pairs; `mixedComponents` (a drawing beside K3,3 or a genus-1 K4) and genus-0 components of mixed-genus rotations | Brute minimum over vertex subsets, `MaxFlow` (row 19), closed forms on trees and cycles; cut arcs sum to `value`, leave no positive s→t path and point away from the s side; flow bounds, conservation, net `value`, zero flow outside the s–t component |
| Assertion probes | `--invalid` | directed input, `commonFace(u, u)` and range, `s = t`, range, genus-1 rotations (K3,3, K4, and s, t inside the K3,3 of a mixed graph), no common face, negative capacity or weight, `cap` size, `xy` size, drawing loop, parallel edge, coincident points, coordinate 2^62 |

## Commands and results

Run 2026-10-28 on the local machine (GCC 16.2 `g++`, GCC 14.4.1 `g++-14`):

| Command | Result | Memory peak |
|---|---|---|
| `python3 '96-Local Testing/04-Graphs/29-planar_graph_tester.py' --mode quick` | PASS, 2 configurations, 17 probes | 532 MB |
| `python3 '96-Local Testing/04-Graphs/29-planar_graph_tester.py' --mode full --seed 1` | PASS, 3 configurations; 442,357 cut queries (optimized, ASan), 440,138 (checked) | 882 MB |
| `CXX=g++-14 python3 '96-Local Testing/04-Graphs/29-planar_graph_tester.py' --mode full --seed 2` | PASS, 3 configurations; 440,434 cut queries (optimized, ASan), 438,213 (checked) | 898 MB |
| `python3 '96-Local Testing/04-Graphs/29-planar_graph_tester.py' --mode stress --seed 3` | PASS, 3 configurations; 2,176,068 cut queries (optimized), 867,736 (checked), 872,355 (ASan) | 954 MB |

`MaxFlow` cross-checks run only when every capacity is at most 2^61, because `MaxFlow::addEdge` asserts `cap + rev_cap ≤ LLONG_MAX`; the brute subset cuts cover the rest. The checked build uses quick-size mid drawings and a 40×40 grid because libstdc++ debug mode re-checks the heap on every push, which makes Dijkstra quadratic. In stress the checked and ASan builds run 2× rounds; the optimized build runs 5× rounds with a 450×450 grid and 600,000-vertex trees and cycles.

`python3 '96-Local Testing/02-integration.py'`: PASS, 180 standalone and aggregate headers, multi-TU, workspace (2026-10-28).

Benchmarks: none required (Contest profile, no Barrett or Montgomery reduction).

## Sources

[00-sources.md](00-sources.md#pages-fetched-on-2026-10-28-p048-gr13): maspypy planar_graph, cp-algorithms planar faces, OI Wiki planar graphs, NetworkX PlanarEmbedding, Boost planar graphs, OGDF embedding, dual graph, flow and separator pages. Written independently from the algorithm descriptions; no catalog code was copied. Not inspected: the papers of Hassin (1981, known here through the OGDF flow page), Eppstein (tree–cotree) and Reif / Italiano et al., and BZOJ 1001, NOI2010 海拔 and Luogu P7916 (search snippets only).

## Limits and handoffs

- Design calls confirmed by the user (user decision 2026-10-28): rotation `g[v]` counter-clockwise with faces on the left, an empty face per isolated vertex, `outer` −1 unless built by `fromDrawing`, `commonFace` returns the smallest face, per-arc capacities with the Hassin flow witness, greedy ID-order `treeCotree`, and clamping of negative or missized capacities under `NDEBUG`.
- Planar separators are planned row 89 (P249).
- `planarMinCutDual` needs `s` and `t` on a common face. General s–t pairs in plane graphs (Reif, Italiano et al.) are not covered; use row 19 `MaxFlow`.
- Embedding edits, augmentation to biconnected or maximal planar, and canonical orderings are not provided.
- Planarity testing and embedding construction are row 62; Geometry owns arrangements and point location.

## History

- 2026-10-28: new row (P048 / GR13); `primalDualTraversal` → `treeCotree`; sweep added fromDrawing, outer, commonFace, directed capacities and the flow witness; review: 8 findings fixed (NDEBUG bounds, drawing preconditions, `faceOf` removed, cost lines, tests); full suite passed on g++ and g++-14, stress passed.
- 2026-10-28: `/finish` audit: per-component `genus` and O(1) genus check, `NDEBUG`-bounded directed input, F defined, docs and provenance; full suite passed on g++ and g++-14, stress passed.
