# 01-graph.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `02-traversal.hpp` through `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### GraphEdge and GraphArc

`GraphEdge {u, v, w}` is a logical edge in insertion order. `GraphArc {from, to, id, rev, w}` is an oriented arc: `id` is its logical edge, `rev` the reciprocal arc of an undirected edge and `-1` for a directed arc. An undirected edge, including a loop, has two distinct reciprocal arcs `a` and `a + 1`.

### Graph

`Graph(n, directed)` with `n >= 0`. `addEdge(u, v, w = 1)` returns the new logical edge ID; it invalidates references and iterators into the storage, and IDs are stable until assignment or destruction. `operator[](u)` returns the arc indices out of `u` in insertion order. `reverse()` is O(n + m): directed arcs swap endpoints, undirected edges keep their orientation, and logical edge IDs are preserved. `read(in, n, m, directed, weighted, base)` consumes exactly `m` records `u v [w]` with `base` 0 or 1; omitted weights become 1. Well-formed input, including successful extraction and in-range endpoints, is a precondition.

`fromAdjacency(adj, directed = false)` builds `adj.size()` vertices from `vector<vector<int>>` lists (weight 1) or `vector<vector<pair<int, lng>>>` lists; every entry must be in `[0, n)`. Directed: one arc `u -> v` per entry `v` of `adj[u]`, in vertex then list order. Undirected: the lists must be symmetric as multisets of (neighbour, weight), so every unordered pair `{u, v}` has the same weight multiset in `adj[u]` and `adj[v]` and every loop is listed twice in its own list with equal weights. Each mirrored pair becomes one edge, so parallel edges keep their multiplicity; the edges come in increasing `(u, v, w)` order with `u <= v`, independent of list order. An asymmetric input is an asserted precondition. `fromMatrix(w, directed = true)` takes a square `vector<vector<lng>>` and adds one arc `u -> v` of weight `w[u][v]` per nonzero entry in row-major order; undirected requires a symmetric matrix and adds one edge per nonzero entry with `u <= v`, a nonzero diagonal entry being a loop. A zero entry means no edge, so zero-weight edges need `addEdge`. Costs: O(n + m) with m the total list length, and O(n^2).

Correctness: `Graph` owns zero-based vertices, insertion-ordered logical edges and oriented arcs. All counts fit `int`; weights use the full `lng` domain. A directed graph's reverse is a separate object. Algorithms return arc indices so orientation and parallel edges remain distinguishable; `g.arcs[a].id` recovers the logical ID. Public storage is inspectable but must not be edited directly.

Correctness of `fromAdjacency`: undirected entries become records `(u, v, w)` of the listing vertex `u`. Least-significant-digit radix sort by weight (eight 8-bit passes on the sign-flipped value, each skipped when its byte is constant), then stable counting sorts by `max(u, v)` and by `min(u, v)`, order them by `(min(u, v), max(u, v), w)` in O(n + m) with word-size constants. A group of equal keys holds exactly the entries of the edges with that key: for `u < v` the forward entries (listed at `u`) and the backward ones must be equally many, and a loop group, all forward, must be even. Symmetry as multisets is exactly this group-wise condition, so the assertion accepts every valid input and rejects every other, and a valid group yields one edge per mirrored pair (`forward` edges, `forward / 2` for a loop). Under `NDEBUG` an asymmetric input with in-range entries still terminates, with at most one edge per forward entry; an out-of-range entry is undefined there, as in `addEdge`. Directed lists and matrices are direct scans; the unweighted overload skips the weight passes. `fromMatrix` checks row sizes and symmetry in one entry pass, so with assertions enabled an undirected matrix costs one extra O(n^2) pass with strided reads.

### CsrGraph

An immutable owning snapshot of a `Graph` with contiguous adjacency, keeping arc IDs, edge IDs and adjacency order. `operator[]` returns a `std::span` that borrows this object and expires on its assignment or destruction. Construction workspace is O(1) beyond the result; `reverse()` is O(n + m) including a transient adjacency-list graph.

### DenseGraph

Owns a copy of the full multigraph in `graph` plus `best[u][v]`: the arc index of the lightest `u -> v` arc, or the heaviest when constructed with `maximum = true`, `-1` when there is none. Equal weights choose the earliest arc. There are no implicit diagonal entries; loops are stored like any other arc. `dijkstraDense` requires the minimum view; `primDense` builds a minimum or maximum forest according to `maximum`.

Correctness: the O(n²) matrix selects the best arc per ordered pair with earliest-arc ties and `-1` for absence, while the full graph is kept, so a dense projection does not lose parallel-edge provenance.

### Subgraph, lineGraph, inducedSubgraph, contract and simplify

`Subgraph {graph, vertex, edge}` maps each new vertex `i` back to the source vertex `vertex[i]` and each new edge `i` to the source edge `edge[i]`; `inducedSubgraph` and `egoGraph` relabel vertices, the other builders keep the numbering, so `vertex` is the identity there. All utilities in this file accept `Graph` or `CsrGraph` (two-graph operations take any mix) and return a new owning `Graph`.

- `lineGraph(g)` has one vertex per source edge, every edge of weight 1. Directed: an arc `i -> j` for every pair of arcs with `head(i) == tail(j)`, so a directed loop gives a line-graph loop. Undirected: for every vertex `v` and every unordered pair of distinct edges incident to `v` (a loop counts once), one edge; parallel edges therefore give two parallel line edges, and `simplify(lineGraph(g))` is the simple line graph. Edge order (user decision 2026-11-14): directed, arc `i` in ID order, then the arcs leaving `head(i)` in adjacency (ID) order; undirected, vertex `v` in increasing order, its incident edge IDs `e_0 < e_1 < ...` in adjacency order, and for each `i` then each `j < i` the edge `(e_j, e_i)`. Cost O(n + m + out) with `out` the line-graph edge count; `addEdge` asserts that it fits `int`.
- `inducedSubgraph(g, vs)` takes distinct in-range vertices (asserted). New vertex `i` is `vs[i]`, so the caller's list is the vertex map. Kept edges are those with both ends in `vs`, with orientation and weight preserved, ordered by the position in `vs` of the scanning endpoint (`u` for undirected edges) and then by adjacency order. Cost O(n + k + d) with `d` the degree sum of `vs`; the O(n) term is the position array.
- `contract(g, label, k)` requires `label.size() == n`, `k >= 0` and every label in `[0, k)`. Edge `i` of the result is `(label[u], label[v], w)` of source edge `i`, so edge IDs are identical and loops and parallel edges are kept. `simplify(contract(...))` gives the quotient graph, for example a bridge tree from 2-edge-connected labels or a condensation from SCC labels. Cost O(n + k + m), since the label checks are O(n).
- `simplify(g)` drops loops and keeps, per ordered pair (directed) or unordered pair (undirected), the edge with the smallest `(w, id)`. Kept edges keep their original orientation and appear in increasing source ID order. Cost O(n + m).

Correctness: these are direct constructions. `lineGraph` enumerates, per vertex, the pairs of incident edges (undirected) or the arcs leaving each edge's head (directed), so it emits each line-graph edge exactly once in O(n + m + out). `inducedSubgraph` scans only the chosen vertices' adjacency and keeps an undirected edge from its first arc, which has the smaller index, so each edge is taken once with its stored orientation. `contract` relabels endpoints edge by edge. `simplify` scans each vertex's adjacency with a stamp array keyed by the scanning vertex, keeps the smallest `(w, id)` per neighbour (an undirected pair only from its smaller endpoint), then emits the kept IDs in increasing order; every arc is touched twice, so it is O(n + m) without sorting.

### addPath, addCycle and addStar

`Graph` members built from `addEdge`, each returning the first new edge ID (the edge count before the call; nothing is added for a path of fewer than two vertices or an empty star). `addPath(vs, w)` adds `vs[i] -> vs[i + 1]`; `addCycle(vs, w)` asserts `vs` non-empty (an empty list adds nothing under `NDEBUG`) and closes the path with `vs.back() -> vs[0]`, so one vertex gives a loop and two give two parallel edges; `addStar(center, vs, w)` adds `center -> v` per `v`. Vertex ranges are `addEdge`'s assertions. Cost O(k) amortized.

### edgeSubgraph and restrictedView

`edgeSubgraph(g, ids)` keeps all `n` vertices and, as edge `i`, source edge `ids[i]` with its orientation and weight; the IDs must be distinct and in range (asserted with an O(m) mark array, so the cost is O(n + m + k)). `restrictedView(g, vs, ids)` keeps all `n` vertices and drops the edges listed in `ids` and every edge touching a vertex in `vs` (duplicates allowed, ranges asserted); the rest keep their ID order. It composes `edgeSubgraph`. Hidden vertices stay as isolated vertices so that numbering, and every array indexed by it, stays valid.

### complementGraph, graphPower, inDegrees and edgeId

- `complementGraph(g)` is the simple complement: an edge `u-v` (`u < v`) of weight 1 when undirected, an arc `u -> v` (`u != v`) when directed, for each pair joined by no edge of `g`, in `(u, v)` order. Loops and multiplicities of `g` are ignored and no loop is added. A stamp array marks each vertex's neighbours, so the cost is O(n^2 + m).
- `graphPower(g, k)` with `k >= 0`: an edge (arc) of weight 1 for every pair with `0 < hops(u, v) <= k`, hops along out-arcs, in `(u, v)` order with `u < v` when undirected. `k = 0` gives no edges and `k = 1` the simple underlying graph without loops. One BFS per vertex truncated at depth `k` and a stamp array give O(n * (n + m)); the result has at most `n^2` edges.
- `inDegrees(g)` counts the arcs entering each vertex: the in-degree when directed, the degree (a loop counted twice) when undirected. Out-degrees are `g[u].size()`.
- `edgeId(g, u, v)` returns the smallest ID of an edge from `u` to `v` (either orientation when undirected) or `-1`, by one scan of `g[u]`, whose insertion order is increasing edge ID. A hashed endpoint index is a caller trade-off; `DenseGraph::best` gives O(1) lookups by weight.

### disjointUnion, graphJoin, toUndirected and toDirected

- `disjointUnion(g, h)` asserts equal directedness and `g.n + h.n` fitting `int`: `g`'s edges, then `h`'s with vertices shifted by `g.n`, so `h`'s edge `i` is edge `g.edges.size() + i`.
- `graphJoin(g, h)` is `disjointUnion(g, h)` followed by a weight-1 edge `u-(g.n + x)` for every `u` of `g` and `x` of `h` in `(u, x)` order; when directed, the arcs `u -> x` and `x -> u` (NetworkX `full_join`). O(n_g * n_h + m).
- `toUndirected(g)` keeps every edge and ID as an undirected edge. `toDirected(g)` copies a directed graph and turns undirected edge `i` into the arcs `2i` (`u -> v`) and `2i + 1` (`v -> u`), a loop into two loop arcs. O(n + m).

### Edge-set operations

`graphUnion`, `graphIntersection`, `graphDifference` and `graphSymmetricDifference` take two graphs on the same vertex set (equal `n` and directedness, asserted). Edges are matched by endpoint pair, ordered when directed and unordered when undirected: the `i`-th edge of a pair in `g`, in ID order, matches the `i`-th edge of that pair in `h`, whatever their weights. With multiplicities `c_g` and `c_h` per pair, the results hold `max`, `min`, `max(c_g - c_h, 0)` and `|c_g - c_h|` edges per pair, which on simple graphs are the set operations of NetworkX `compose`, `intersection`, `difference` and `symmetric_difference`. Union is all of `g`'s edges then `h`'s unmatched ones; intersection `g`'s matched edges; difference `g`'s unmatched edges; symmetric difference `g`'s unmatched then `h`'s unmatched edges; each part in ID order with its source weights.

Correctness: the edges of both graphs, tagged with source and ID, are stably counting-sorted by the normalized pair, so each pair's records are contiguous with `g`'s first, each side in ID order; the ranks within a side are then compared with the other side's count. O(n + m_g + m_h).

### Products

All products assert equal directedness and `n_g * n_h` fitting `int`, number vertex `(u, x)` as `u * h.n + x`, and keep loops and parallel edges by building from edge lists. An edge made from a `g` edge (alone or paired with an `h` edge) takes that `g` edge's weight; one made from an `h` edge alone takes the `h` edge's weight; added structural edges have weight 1.

- `cartesianProduct(g, h)`: per `g` edge `e` (ID order) and layer `x`, `(e.u, x)-(e.v, x)`; then per `u`, per `h` edge `f`, `(u, f.u)-(u, f.v)`. Directed arcs keep their orientation.
- `tensorProduct(g, h)`: per pair of edges `e`, `f`, the edge `(e.u, f.u)-(e.v, f.v)`, and when undirected and neither is a loop also `(e.u, f.v)-(e.v, f.u)`. This is the Kronecker product of the adjacency matrices with a loop counted once, so a looped single vertex is an identity factor.
- `strongProduct(g, h)`: the Cartesian edges, then the tensor edges.
- `lexicographicProduct(g, h)` (`g[h]`): per `g` edge `e`, an edge `(e.u, x)-(e.v, y)` for every `x`, `y` (an undirected loop uses each unordered pair `x <= y` once), then the `h` edges of every layer as in the Cartesian product. O(n_g * n_h + m_g * n_h^2 + n_g * m_h).
- `rootedProduct(g, h, root)` with `0 <= root < h.n`: each `g` edge joins `(u, root)` and `(v, root)`, then the `h` edges of every layer (NetworkX `rooted_product`).
- `coronaProduct(g, h)`: `g` on vertices `[0, g.n)`, then for each `u` a copy of `h` on `g.n + u * h.n + x` followed by weight-1 edges `u-(copy of x)` for every `x` (arcs both ways when directed); `g.n * (h.n + 1)` must fit `int`.
- `modularProduct(g, h)`: undirected only; a weight-1 edge between `(u, x) < (v, y)` iff `u != v`, `x != y` and `u, v` are adjacent in `g` exactly when `x, y` are adjacent in `h` (adjacency as a set: loops and multiplicities ignored). Its cliques are the common induced subgraphs, so maximum common induced subgraph reduces to maximum clique (row 49). Boolean adjacency matrices give O(n_g^2 * n_h^2 + m_g + m_h) time and O(n_g^2 + n_h^2) workspace.

Correctness: each product is the direct construction of its definition over edge lists (Hammack, Imrich and Klavžar, *Handbook of Product Graphs*, as summarized by NetworkX's operator documentation), so every product edge is emitted exactly once per generating edge or edge pair.

### mycielskian

`mycielskian(g)` (undirected, `2n + 1` fitting `int`): vertices `v`, copies `n + v` and the apex `2n`; `g`'s edges keep their IDs, then per edge `(u, v)` in ID order the edges `(n + u, v)` and `(u, n + v)` with its weight, then `(n + v, 2n)` of weight 1 per `v`. On a loopless graph it raises the chromatic number by one and keeps the graph triangle-free if it was; a loop yields two parallel edges `(n + u, u)`. O(n + m).

### Legacy mapping

- `GraphLabels<T>(labels)` → `CoordinateCompression<T>(labels)` (`02-Data Structures/07-ordered_set.hpp`, under the same strict ordering): `index(x)` returns -1 for an absent label where `GraphLabels` asserted, `value(i)` is `decode(i)` and `labels` is `v`.

## Feature-to-test map

`96-Local Testing/04-Graphs/01-graph_tester.py` uses the shared contest runner, which builds optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and full/stress `-O1` ASan/UBSan variants. The corpus is 606 exhaustive small graphs, 500 random multigraphs and 1,000 builder cases; quick uses exhaustive graphs through two vertices, 70 random cases, 100 builder cases and one random second factor per graph (11.4 million checks), full extends to three vertices, 500 random and 1,000 builder cases and two second factors (117 million checks), stress to 4,000 random and 8,000 builder cases. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `GraphEdge`, `GraphArc`, `Graph: addEdge, operator[]` | Edge/arc counts and identities, defaults, full-range weights | Independent edge-list records |
| `Graph: reverse`, `CsrGraph: reverse` | Reverse involution | Involution and edge-ID identity |
| `CsrGraph: operator[]` | CSR equivalence; snapshots/copy/move | Adjacency of the source `Graph` |
| `DenseGraph: best` (minimum or maximum view) | Dense minima and maxima with earliest ties | Independent dense minima and maxima |
| `Graph: read` (base/weighted/directed adapters) | Input adapters | Expected edge lists |
| `Graph: fromAdjacency, fromMatrix` | Every exhaustive and random graph as shuffled weighted and unit lists; builder cases with full-range weights differing in one random byte, loops and parallel edges, and random directed or symmetric matrices; fixtures for defaults, mirrored parallel edges listed in different orders, loops and empty inputs | Directed: the list scan. Undirected: the source edge multiset normalized to `(min, max, w)` and sorted with `std::sort`. Matrices: the row-major nonzero scan, and identity with `fromAdjacency` of the same lists |
| Legacy `GraphLabels` cases (DS row 07 `CoordinateCompression`) | Label adapters: integer extremes, strings with the empty label, keys with only `operator<`, empty set, copy/move, absent labels | Sorted distinct labels; -1 for an absent label |
| `lineGraph`, `Subgraph` | Each graph and its CSR snapshot | Exact edge sequence built from the edge list by the stated order (incident IDs per vertex, pairs `(e_j, e_i)`, `j < i`) |
| `inducedSubgraph` | Empty, full and three shuffled random subsets | The exact edge sequence (source edges with both ends in `vs`, grouped by the position of `u` in `vs`, IDs increasing) with relabelled endpoints |
| `contract` | Identity, all-merged and random labels (two spare labels) | Relabelled edges |
| `simplify` | Every graph and every contraction | Ordered pair map of `(w, id)` minima |
| `Subgraph: vertex` | Every `inducedSubgraph`, `simplify`, `edgeSubgraph` and `restrictedView` result | `vs` for `inducedSubgraph`, the identity otherwise |
| `Graph: addPath, addCycle, addStar` | Appended paths, cycles (one and two vertices) and stars on every graph | Expected appended edges and returned IDs |
| `edgeSubgraph`, `restrictedView` | All, empty and shuffled ID subsets; hidden vertices and IDs with duplicates | Set-based filtering of the edge list |
| `inDegrees`, `edgeId` | Every graph, every pair `(u, v)` | Incidence counts; a linear scan for the first matching edge |
| `complementGraph` | Every graph | Adjacency matrix |
| `graphPower` | `k` in {0, 1, 2, 3, n, `INT_MAX`} | Floyd hop distances |
| `toUndirected`, `toDirected`, `subdivideEdges`, `mycielskian` | Every graph | Edge lists from the definitions; fixtures (Mycielskian of K2 is C5) |
| `disjointUnion`, `graphJoin`, `cartesianProduct`, `tensorProduct`, `strongProduct`, `lexicographicProduct`, `rootedProduct` (every root), `coronaProduct`, `modularProduct` | Each graph with random second factors of 0–3 vertices on both sides; fixtures (K2 × K2 as C4, 2K2 and K4; P4 squared; corona as a triangle) | The exact expected edge sequence from the definition, and an adjacency-count identity from the factors' matrices |
| `graphUnion`, `graphIntersection`, `graphDifference`, `graphSymmetricDifference` | Both argument orders and each graph with itself | Count-map matching per endpoint pair; max, min, excess and absolute-difference pair counts |
| Preconditions | 82 assertion probes (42 for the 2026-11-14 operations; the eight added after the mutation campaign and the five `*-past-end` probes /finish added for `operator[]` of both graphs, `inducedSubgraph` and `restrictedView` take operands past the end or edgeless, so no later assertion stands in for the checked one; `corona-overflow-window` (65535 × 32768) also requires the failing assertion to name `coronaProduct` on stderr, since a weakened check would end at Graph's `n >= 0` assertion instead; the `int` overflow checks of `disjointUnion`, `mycielskian` and `subdivideEdges` would need about 2^30 vertices and are not probed), including `induced-negative`, `induced-end`, `induced-duplicate`, `contract-size`, `contract-count`, `contract-negative`, `contract-end`, `adjacency-negative`, `adjacency-end`, `adjacency-asymmetric`, `adjacency-multiplicity`, `adjacency-loop-once`, `adjacency-weight`, `adjacency-loop-weight`, `matrix-ragged`, `matrix-asymmetric` | Expected assertion failure |

Mutation checks (planted in a temporary copy, full mode) killed: line-graph loop double count and self-pairs, simplify keeping the maximum, reversed induced orientation and maximum-view tie order. A surviving mutant (simplify tie on an equal ID) is equivalent.

Builder mutants (2026-10-12, same method; the two assertion mutants against the checked probes) were all killed except one equivalent: no sign flip, seven weight passes, an unstable counting scatter, grouping without the weight, loops not halved, full matrix rows, and dropped list and matrix symmetry assertions. The survivor removes the directed early return, which only skips sorting an empty buffer.

## Commands and results

2026-11-14 (P010 GR01; experiment: none), Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`. Each full run's pre-flight quick passed.

| Run | Result |
|---|---|
| Full, seed 20261114, g++ | PASS, all three builds, 117,431,920 checks per build, 82 probes; optimized 1.6 s, checked 5.0 s, ASan-UBSan 22.5 s; peak 1,569 MB |
| Full, seed 20261114, `CXX=g++-14` | PASS, all three builds, same counts; optimized 1.6 s, checked 4.9 s, ASan-UBSan 21.0 s; peak 1,562 MB |
| Stress, seed 7 | PASS, all three builds, 906,034,947 checks per build; optimized 11.3 s, checked 39.0 s, ASan-UBSan 195.3 s; peak 706 MB |
| `01-run.py --mode quick --filter 04-Graphs --seed 42 --no-integration` | PASS, all 32 graph suites (before /finish; its rerun was stopped for low system memory, and the suites /finish changed passed quick as their full runs' pre-flight) |
| `05-extraction.py ops --filter 04-Graphs/01-graph.hpp`; `testers --filter 01-graph_tester` | PASS on g++ and g++-14; PASS |
| `02-integration.py` (190 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 3,077 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode stress --seed 7
python3 '96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter '04-Graphs/01-graph.hpp'
python3 '96-Local Testing/05-extraction.py' testers --filter 01-graph_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Mutation testing

Record: mewt 4.0.0 on blob `a2d829d3fa367f3b3b176a7cd381684ef722f56e` (2026-11-14, seed 1, 2 lines changed since blob `6e0e4e552eb6`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested. /finish GR01 changed only the comment lines above `GraphEdge` and `GraphArc`.

Earlier whole-file campaign (GR01): mewt 4.0.0 on blob `6e0e4e552eb6a4f18f9b5501fc9b6839afaacbc9` (2026-11-14, seed 1, whole file, uniform sample of 1723): 500 mutants, 458 caught (9 by timeout), 42 survived, 0 untested. Stage seconds: quick 1370, full on the optimized build 69, full on the checked and ASan-UBSan builds 1375. It left 54 survivors. The rerun with the extended tester (same `--workdir`) caught twelve of them:
- Weakened range and symmetry assertions: `COS 1050` (line 38), `COS 1140` (67), `COS 1177` (75), `COS 1204` (85), `COS 1344` (186), `COS 1389` (215) and `COS 1524` (327). Each now has a probe with an operand past the end or on the other side: `input-base-negative`, `adjacency-asymmetric-reverse`, `matrix-asymmetric-lower`, `add-past-end`, `contract-past-end`, `edge-subgraph-past-end` and `edge-id-past-end`.
- `LOS 1023` and `COS 1625` (line 435): `rooted-negative` and `rooted-end` now use edgeless operands, so a later `addEdge` assertion cannot stand in for the root check.
- `AOS 731` (line 243): `product-overflow` uses 65536 × 65537 vertices, which an unchecked `int` product would wrap to 65536 and build silently.
- `COS 1630` (line 444): `corona-direction-reverse`.
- `AS 498` (line 165, a reversed undirected line-graph edge): the exact edge-sequence oracle, after the order was specified (user decision 2026-11-14).

/finish GR01 closed two survivors this table first called equivalent through a wrapping signed overflow: `AOS 853` and `COS 1634` (line 444, the corona size assertion loosened) pass 65535 × 32768, whose vertex count then overflows `int`. The `corona-overflow-window` probe closes them. Hand-applied to a scratch copy (2026-11-14), `(h.n - 1)`, `(h.n * 1)` and `!= INT_MAX` each failed the probe, while the unmutated header passed it. The same pass added past-end probes for the range checks of `Graph::operator[]`, `CsrGraph::operator[]`, `inducedSubgraph` and `restrictedView` (outside that sample); each `<` → `!=` mutant there, applied by hand, failed its probe.

The other 40 are equivalent (line numbers of that blob):

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `CR 203`, `AOS 632`, the counting sort's early return removed or never taken | 15 | The early return skips only a sort whose keys are all equal, which is the identity; the full pass stays within O(m + k). |
| `COS 1034`, `1109`, `1169`, `1184`, `1229`, `1289`, `1294`, `1379`, `1439`, `1494`, `1559`, `1579`, `1589`, `1644`, `1654` `<` → `!=` | 16, 64, 75, 77, 122, 164, 165, 205, 256, 313, 363, 376, 385, 450, 466 | Each counter steps by 1 from at most its bound (line 64: `i = j` with `j > i`, since `es[i]` matches its own key). |
| `COS 1484`, `depth < k` → `!=` | 308 | BFS depths along the queue start at 0 and rise by at most 1, and `k >= 0` is asserted, so both stop at the first vertex of depth `k`. |
| `CR 221`, the directed early return removed | 57 | A directed build leaves the buffer empty, so the sorts and the grouping loop do nothing; the O(n) counter arrays stay within O(n + m). |
| `IT 435`, the weight passes forced for `int` lists | 58 | Every weight is 1, so each pass returns at its all-keys-equal check. |
| `AS 473`, `476`, swapped `min` arguments | 62, 63 | `min` is symmetric. |
| `COS 1127`, `u <= v` → `u >= v` when counting mirrored entries | 66 | On a non-loop key the two counts sum to `j - i`, so `2 * forward == j - i` holds for one exactly when it holds for the other, and both equal half on valid input; on a loop key both count every entry. |
| `COS 1149`, `k > 0` → `k != 0` | 68 | `k` starts at least 0 and steps down by 1. |
| `COS 1154`, `w.size() <= INT_MAX` → `!=` | 71 | Unreachable: it differs only for at least `INT_MAX` rows (48 GiB of row vectors), and the assertion vanishes under `NDEBUG`. |
| `CR 236`, `AOS 671`, `672`, the arc-count assertion removed or loosened | 86 | Unreachable: it binds only at `INT_MAX - 2` arcs (64 GiB), and the assertion vanishes under `NDEBUG`. |
| `CR 258`, `adj.reserve` removed | 123 | Only capacity changes. |
| `COS 1255`, `b == -1` → `<=` | 145 | `b` is `-1` or an arc index. |
| `COS 1358`, `1360`, `1361`, `1362`, `v < u` → `==`, `<=`, `>`, `>=` in `simplify` | 199 | `v == u` was skipped just before. Scanning an undirected pair from the other endpoint, or from both, finds the same minimum `(w, id)` over its parallel edges, so the kept set and its ID order are identical. The extra scan is O(m). |
| `COS 1364`, `seen[v] != u` → `<` | 200 | Stamps are `-1` or earlier scanning vertices, all below `u`. |
| `AOS 824`, `s + 1` → `s * 1` | 313 | The loop then starts at `s`, which `v != s` excludes. |
| `COS 1595`, `es[j].u == es[i].u` → `<=` | 385 | The buffer is stably sorted by `u`, so `es[j].u >= es[i].u` for `j >= i`. |
| `COS 1635`, `<= INT_MAX` → `< INT_MAX` | 444 | Unreachable: `INT_MAX` is prime, so equality needs a factor with about 2^31 vertices (48 GiB). |
| `AS 594`, `productBase(h, g)` | 459 | `modularProduct` takes undirected factors, and `productBase` asserts equal direction, so the swapped call builds the same `h.n * g.n` vertices. |
| `AOS 939`, `n + m` → `n - m` in the subdivision size assertion | 484 | Unreachable: the two differ only when `n + m > INT_MAX`, which needs about 2^31 vertices (24 bytes of adjacency each) or edges (16 bytes plus two 24-byte arcs each), at least 32 GiB, far past the 4 GiB test cap and any judge. |

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/00-foundations_benchmark.py>) and `96-Local Testing/04-Graphs/00-foundations_benchmark.json` compare the same BFS kernel on adjacency-list and CSR storage. It uses chain, star and connected random multigraphs at 32, 3,000 and 100,000 vertices, seed 20260927, one checked warmup and five samples with repetition for smaller graphs. CSR conversion is timed separately; input graph construction is excluded. All traversal checksums agree. Both representations store O(n+m), and conversion retains the input plus the output.

| 100,000-vertex random graph (499,999 logical edges) | Median |
|---|---|
| BFS on adjacency lists | 14.34 ms |
| BFS on CSR | 9.27 ms |
| CSR construction | 4.48 ms |

Tiny cases slightly favored lists; chain/star results also varied. This supports an explicit reusable CSR option, not an automatic threshold or universal speed claim. Conditions from the git-ignored `00-foundations_benchmark.json` log, which also holds the raw samples: 11th Gen Intel Core i9-11900H, GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`. Measured on a shared host.

```bash
python3 '96-Local Testing/04-Graphs/00-foundations_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied. The 2026-11-14 additions follow the pages listed in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr01) (definitions only, no code copied).

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 图的存储](https://oi-wiki.org/graph/save/) (page revision 2026-09-12) | Matrix/list/forward-star tradeoffs and paired reverse arcs; used to assess storage coverage. |
| [Boost 1.89, Compressed Sparse Row Graph](https://www.boost.org/doc/libs/1_89_0/libs/graph/doc/compressed_sparse_row.html) | Source-group offsets, O(n+m) storage and immutable CSR use. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions).

## Limits and handoffs

Dynamic graphs are owned by their separate inventory rows.

## Aliases

Search aliases (P263, grab --find): 图的存储, 邻接表, 邻接矩阵, 前向星, 邻接串列, 线图, 导出子图, 诱导子图, 图的积, 笛卡尔积, 张量积, 强积, 字典积, 补图, 图的幂, 不交并, 图的并, 图的交, 边细分, 自我中心网络; graph product, Cartesian product, Kronecker product, complement graph, graph power, disjoint union, graph join, Mycielski graph, edge subdivision, compose (NetworkX).

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-06: inventory rewrite reopened the row as partial (line graph, induced subgraph, contraction, simplification).
- 2026-10-07: re-audit, implemented `lineGraph`, `inducedSubgraph`, `contract`, `simplify`; 5 findings fixed (missing operations, evidence claim, complexity lines, brace rule, T/M placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, added `Graph::fromAdjacency` and `fromMatrix`, removed `GraphLabels` (cases moved to `CoordinateCompression`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: P010 GR01, added `Graph::addPath`, `addCycle`, `addStar`, `Subgraph::vertex`, `edgeSubgraph`, `restrictedView`, `subdivideEdges`, `complementGraph`, `disjointUnion`, `graphJoin`, the four edge-set operations, seven products, `mycielskian`, `graphPower`, `toUndirected`, `toDirected`, `inDegrees`, `edgeId`; full suite passed on g++ and g++-14, stress passed; independent review: 8 findings (allTopologicalSorts unbounded under NDEBUG for a negative limit, comment cap, missing complexity symbols and output terms, addCycle on an empty list under NDEBUG, duplicated in-degree and layer loops, evidence wording, an unqualified std name) fixed; mutation campaign in Mutation testing (survivors closed with tests or recorded equivalent; lineGraph edge order specified, user decision 2026-11-14).
- 2026-11-14: /finish GR01, independent audit: 12 findings (1 medium: two bfs/dfs range-check mutants wrongly recorded equivalent; 11 low) fixed; GraphArc got its own complexity line; past-end probes for both `operator[]`, `inducedSubgraph` and `restrictedView`, a `corona-overflow-window` probe that requires the corona assertion (closing AOS 853 and COS 1634), and an exact `inducedSubgraph` edge-sequence oracle; full suite passed on g++ and g++-14, stress passed; mutation record on the changed lines (comments only, 0 mutants). Review of the fix diff: 8 low findings (evidence wording, probe text match, stderr restore, a notes line for the bounded BFS) fixed.
