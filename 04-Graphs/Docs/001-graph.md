# 001-graph.hpp — Evidence

Owned by package P010 / GR01 (graph foundations, with `002-traversal.hpp` through `006-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Scope

The row covers graph storage, construction and graph operations; `Graph::addEdge` may grow a graph between queries. Algorithms that maintain their answers under edge updates belong to their own rows: `026-dynamic_connectivity.hpp`, `063-fully_dynamic_connectivity.hpp`, `070-incrementalscc.hpp` and `071-dynamicmst.hpp` ([00-notes.md](00-notes.md#placement-decisions-recorded)).

### GraphEdge and GraphArc

`GraphEdge {u, v, w}` is a logical edge in insertion order. `GraphArc {from, to, id, rev, w}` is an oriented arc: `id` is its logical edge, `rev` the reciprocal arc of an undirected edge and `-1` for a directed arc. An undirected edge, including a loop, has two distinct reciprocal arcs `a` and `a + 1`.

### Graph

`Graph(n, directed)` with `n >= 0`. `addEdge(u, v, w = 1)` returns the new logical edge ID; it invalidates references and iterators into the storage, and IDs are stable until assignment or destruction. `operator[](u)` returns the arc indices out of `u` in insertion order. `reverse()` is O(n + m): directed arcs swap endpoints, undirected edges keep their orientation, and logical edge IDs are preserved. `read(in, n, m, directed, weighted, base)` consumes exactly `m` records `u v [w]` with `base` 0 or 1; omitted weights become 1. Well-formed input, including successful extraction and in-range endpoints, is a precondition.

`fromAdjacency(adj, directed = false)` builds `adj.size()` vertices from `vector<vector<int>>` lists (weight 1) or `vector<vector<pair<int, lng>>>` lists; every entry must be in `[0, n)`. Directed: one arc `u -> v` per entry `v` of `adj[u]`, in vertex then list order. Undirected: the lists must be symmetric as multisets of (neighbour, weight), so every pair `{u, v}` has the same weight multiset in `adj[u]` and `adj[v]`, and every loop is listed twice in its own list with equal weights. Each mirrored pair becomes one edge, so parallel edges keep their multiplicity; edges come in increasing `(u, v, w)` order with `u <= v`, independent of list order. Asymmetric input is an asserted precondition. `fromMatrix(w, directed = true)` takes a square `vector<vector<lng>>` and adds one arc `u -> v` of weight `w[u][v]` per nonzero entry in row-major order; undirected requires a symmetric matrix and adds one edge per nonzero entry with `u <= v`, a nonzero diagonal entry being a loop. A zero entry means no edge, so zero-weight edges need `addEdge`. Costs: O(n + m) with m the total list length, and O(n^2).

Correctness: `Graph` owns zero-based vertices, insertion-ordered logical edges and oriented arcs. All counts fit `int`; weights use the full `lng` domain. A directed graph's reverse is a separate object. Algorithms return arc indices so orientation and parallel edges stay distinguishable; `g.arcs[a].id` recovers the logical ID. Public storage is inspectable but must not be edited directly.

Correctness of `fromAdjacency`: undirected entries become records `(u, v, w)` of the listing vertex `u`. A least-significant-digit radix sort by weight (eight 8-bit passes on the sign-flipped value, each skipped when its byte is constant), then stable counting sorts by `max(u, v)` and by `min(u, v)`, order them by `(min(u, v), max(u, v), w)` in O(n + m). A group of equal keys holds exactly the entries of the edges with that key: for `u < v` the forward entries (listed at `u`) and the backward ones must be equally many, and a loop group, all forward, must be even. Multiset symmetry is exactly this group-wise condition, so the assertion accepts every valid input and rejects every other, and a valid group yields one edge per mirrored pair (`forward` edges, `forward / 2` for a loop). Under `NDEBUG` an asymmetric input with in-range entries still terminates, with at most one edge per forward entry; an out-of-range entry is undefined there, as in `addEdge`. Directed lists and matrices are direct scans; the unweighted overload skips the weight passes. `fromMatrix` checks row sizes and symmetry in one pass, so with assertions enabled an undirected matrix costs one extra O(n^2) pass with strided reads.

### CsrGraph

An immutable owning snapshot of a `Graph` with contiguous adjacency, keeping arc IDs, edge IDs and adjacency order. `operator[]` returns a `std::span` that borrows this object and expires on its assignment or destruction. Construction needs O(1) workspace beyond the result; `reverse()` is O(n + m) including a transient adjacency-list graph.

### DenseGraph

Owns a copy of the full multigraph in `graph` plus `best[u][v]`: the arc index of the lightest `u -> v` arc, or the heaviest when constructed with `maximum = true`, and `-1` when there is none. Equal weights choose the earliest arc. There are no implicit diagonal entries; loops are stored like any other arc. `dijkstraDense` requires the minimum view; `primDense` builds a minimum or maximum forest according to `maximum`. Because the full graph is kept, the O(n²) projection loses no parallel-edge provenance.

### Subgraph, lineGraph, inducedSubgraph, contract and simplify

`Subgraph {graph, vertex, edge}` maps each new vertex `i` to the source vertex `vertex[i]` and each new edge `i` to the source edge `edge[i]`. `inducedSubgraph` and `egoGraph` relabel vertices; the other builders keep the numbering, so `vertex` is the identity there. Every utility in this file accepts `Graph` or `CsrGraph` (two-graph operations take any mix) and returns a new owning `Graph`.

- `lineGraph(g)` has one vertex per source edge and edges of weight 1. Directed: an arc `i -> j` for every pair of arcs with `head(i) == tail(j)`, so a directed loop gives a line-graph loop. Undirected: one edge for every vertex `v` and every unordered pair of distinct edges incident to `v` (a loop counts once); parallel edges give two parallel line edges, and `simplify(lineGraph(g))` is the simple line graph. Edge order (user decision 2026-11-14): directed, arc `i` in ID order, then the arcs leaving `head(i)` in adjacency (ID) order; undirected, vertex `v` in increasing order, its incident edge IDs `e_0 < e_1 < ...`, and for each `i` then each `j < i` the edge `(e_j, e_i)`. Cost O(n + m + out) with `out` the line-graph edge count; `addEdge` asserts that it fits `int`.
- `inducedSubgraph(g, vs)` takes distinct in-range vertices (asserted). New vertex `i` is `vs[i]`, so the caller's list is the vertex map. Kept edges are those with both ends in `vs`, with orientation and weight preserved, ordered by the position in `vs` of the scanning endpoint (`u` for undirected edges) and then by adjacency order. Cost O(n + k + d) with `d` the degree sum of `vs`; the O(n) term is the position array.
- `contract(g, label, k)` requires `label.size() == n`, `k >= 0` and every label in `[0, k)`. Edge `i` of the result is `(label[u], label[v], w)` of source edge `i`, so edge IDs are identical and loops and parallel edges are kept. `simplify(contract(...))` gives the quotient graph, for example a bridge tree from 2-edge-connected labels or a condensation from SCC labels. Cost O(n + k + m).
- `simplify(g)` drops loops and keeps, per ordered pair (directed) or unordered pair (undirected), the edge with the smallest `(w, id)`. Kept edges keep their orientation and appear in increasing source ID order. Cost O(n + m).

Correctness: these are direct constructions. `lineGraph` enumerates, per vertex, the pairs of incident edges (undirected) or the arcs leaving each edge's head (directed), emitting each line-graph edge once. `inducedSubgraph` scans only the chosen vertices' adjacency and keeps an undirected edge from its first arc, which has the smaller index, so each edge is taken once with its stored orientation. `contract` relabels endpoints edge by edge. `simplify` scans each vertex's adjacency with a stamp array keyed by the scanning vertex, keeps the smallest `(w, id)` per neighbour (an undirected pair only from its smaller endpoint), then emits the kept IDs in increasing order; every arc is touched twice, so no sort is needed.

### addPath, addCycle and addStar

`Graph` members built from `addEdge`, each returning the first new edge ID (the edge count before the call); a path of fewer than two vertices or an empty star adds nothing. `addPath(vs, w)` adds `vs[i] -> vs[i + 1]`. `addCycle(vs, w)` asserts `vs` non-empty (an empty list adds nothing under `NDEBUG`) and closes the path with `vs.back() -> vs[0]`, so one vertex gives a loop and two give two parallel edges. `addStar(center, vs, w)` adds `center -> v` per `v`. Vertex ranges are checked by `addEdge`'s assertions. Cost O(k) amortized.

### edgeSubgraph and restrictedView

`edgeSubgraph(g, ids)` keeps all `n` vertices and, as edge `i`, source edge `ids[i]` with its orientation and weight; the IDs must be distinct and in range, asserted with an O(m) mark array, so the cost is O(n + m + k). `restrictedView(g, vs, ids)` keeps all `n` vertices and drops the edges listed in `ids` and every edge touching a vertex in `vs` (duplicates allowed, ranges asserted); the rest keep their ID order. It composes `edgeSubgraph`. Hidden vertices stay as isolated vertices, so the numbering and every array indexed by it stay valid.

### complementGraph, graphPower, inDegrees and edgeId

- `complementGraph(g)` is the simple complement: for each pair joined by no edge of `g`, an edge `u-v` (`u < v`) of weight 1 when undirected or an arc `u -> v` (`u != v`) when directed, in `(u, v)` order. Loops and multiplicities of `g` are ignored and no loop is added. A stamp array marks each vertex's neighbours: O(n^2 + m).
- `graphPower(g, k)` with `k >= 0`: an edge (arc) of weight 1 for every pair with `0 < hops(u, v) <= k`, hops along out-arcs, in `(u, v)` order with `u < v` when undirected. `k = 0` gives no edges and `k = 1` the simple underlying graph without loops. One BFS per vertex truncated at depth `k` with a stamp array gives O(n * (n + m)); the result has at most `n^2` edges.
- `inDegrees(g)` counts the arcs entering each vertex: the in-degree when directed, the degree (a loop counted twice) when undirected. Out-degrees are `g[u].size()`.
- `edgeId(g, u, v)` returns the smallest ID of an edge from `u` to `v` (either orientation when undirected), or `-1`, by one scan of `g[u]`, whose insertion order is increasing edge ID. A hashed endpoint index is the caller's trade-off; `DenseGraph::best` gives O(1) lookups by weight.

### disjointUnion, graphJoin, toUndirected and toDirected

- `disjointUnion(g, h)` asserts equal directedness and that `g.n + h.n` fits `int`: `g`'s edges, then `h`'s with vertices shifted by `g.n`, so `h`'s edge `i` becomes edge `g.edges.size() + i`.
- `graphJoin(g, h)` is `disjointUnion(g, h)` followed by a weight-1 edge `u-(g.n + x)` for every `u` of `g` and `x` of `h` in `(u, x)` order; when directed, the arcs `u -> x` and `x -> u` (NetworkX `full_join`). O(n_g * n_h + m).
- `toUndirected(g)` keeps every edge and ID as an undirected edge. `toDirected(g)` copies a directed graph and turns undirected edge `i` into the arcs `2i` (`u -> v`) and `2i + 1` (`v -> u`), a loop into two loop arcs. O(n + m).

### Edge-Set Operations

`graphUnion`, `graphIntersection`, `graphDifference` and `graphSymmetricDifference` take two graphs on the same vertex set (equal `n` and directedness, asserted). Edges are matched by endpoint pair, ordered when directed and unordered when undirected: the `i`-th edge of a pair in `g`, in ID order, matches the `i`-th edge of that pair in `h`, whatever their weights. With multiplicities `c_g` and `c_h` per pair, the results hold `max`, `min`, `max(c_g - c_h, 0)` and `|c_g - c_h|` edges per pair; on simple graphs these are NetworkX `compose`, `intersection`, `difference` and `symmetric_difference`. Union is all of `g`'s edges then `h`'s unmatched ones; intersection is `g`'s matched edges; difference is `g`'s unmatched edges; symmetric difference is `g`'s unmatched then `h`'s unmatched edges; each part in ID order with its source weights.

Correctness: the edges of both graphs, tagged with source and ID, are stably counting-sorted by the normalized pair, so each pair's records are contiguous with `g`'s first, each side in ID order; the rank within a side is compared with the other side's count. O(n + m_g + m_h).

### Products

Every product asserts equal directedness and that `n_g * n_h` fits `int`, numbers vertex `(u, x)` as `u * h.n + x`, and keeps loops and parallel edges by building from edge lists. An edge made from a `g` edge (alone or paired with an `h` edge) takes that `g` edge's weight, one made from an `h` edge alone takes the `h` edge's weight, and added structural edges have weight 1.

- `cartesianProduct(g, h)`: per `g` edge `e` (ID order) and layer `x`, `(e.u, x)-(e.v, x)`; then per `u`, per `h` edge `f`, `(u, f.u)-(u, f.v)`. Directed arcs keep their orientation.
- `tensorProduct(g, h)`: per pair of edges `e`, `f`, the edge `(e.u, f.u)-(e.v, f.v)`, and when undirected and neither is a loop also `(e.u, f.v)-(e.v, f.u)`. This is the Kronecker product of the adjacency matrices with a loop counted once, so a looped single vertex is an identity factor.
- `strongProduct(g, h)`: the Cartesian edges, then the tensor edges.
- `lexicographicProduct(g, h)` (`g[h]`): per `g` edge `e`, an edge `(e.u, x)-(e.v, y)` for every `x`, `y` (an undirected loop uses each unordered pair `x <= y` once), then the `h` edges of every layer as in the Cartesian product. O(n_g * n_h + m_g * n_h^2 + n_g * m_h).
- `rootedProduct(g, h, root)` with `0 <= root < h.n`: each `g` edge joins `(u, root)` and `(v, root)`, then the `h` edges of every layer (NetworkX `rooted_product`).
- `coronaProduct(g, h)`: `g` on vertices `[0, g.n)`, then for each `u` a copy of `h` on `g.n + u * h.n + x` followed by weight-1 edges `u-(copy of x)` for every `x` (arcs both ways when directed); `g.n * (h.n + 1)` must fit `int`.
- `modularProduct(g, h)`: undirected only; a weight-1 edge between `(u, x) < (v, y)` iff `u != v`, `x != y` and `u, v` are adjacent in `g` exactly when `x, y` are adjacent in `h` (adjacency as a set: loops and multiplicities ignored). Its cliques are the common induced subgraphs, so maximum common induced subgraph reduces to maximum clique (row 49). Boolean adjacency matrices give O(n_g^2 * n_h^2 + m_g + m_h) time and O(n_g^2 + n_h^2) workspace.

Correctness: each product is the direct construction of its definition over edge lists (Hammack, Imrich and Klavžar, *Handbook of Product Graphs*, as summarized by NetworkX's operator documentation), so every product edge is emitted exactly once per generating edge or edge pair.

### mycielskian

`mycielskian(g)` (undirected, `2n + 1` fitting `int`): vertices `v`, copies `n + v` and the apex `2n`. `g`'s edges keep their IDs; then, per edge `(u, v)` in ID order, the edges `(n + u, v)` and `(u, n + v)` with its weight; then `(n + v, 2n)` of weight 1 per `v`. On a loopless graph it raises the chromatic number by one and keeps a triangle-free graph triangle-free; a loop yields two parallel edges `(n + u, u)`. O(n + m).

### Legacy Mapping

- `GraphLabels<T>(labels)` → `CoordinateCompression<T>(labels)` (`02-Data Structures/007-ordered_set.hpp`, same strict ordering): `index(x)` returns -1 for an absent label where `GraphLabels` asserted, `value(i)` is `decode(i)` and `labels` is `v`.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/001-graph_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Quick tests exhaustive graphs through two vertices, 70 random cases, 100 builder cases and one random second factor per graph (11.4 million checks). Full extends to three vertices (606 exhaustive graphs), 500 random multigraphs, 1,000 builder cases and two second factors (117 million checks). Stress uses 4,000 random and 8,000 builder cases.

| Operation | Test | Oracle |
|---|---|---|
| `GraphEdge`, `GraphArc`, `Graph: addEdge, operator[]` | Edge and arc counts and identities, defaults, full-range weights | Independent edge-list records |
| `Graph: reverse`, `CsrGraph: reverse` | Reverse involution | Involution and edge-ID identity |
| `CsrGraph: operator[]` | CSR equivalence; snapshots, copy and move | Adjacency of the source `Graph` |
| `DenseGraph: best` (minimum or maximum view) | Dense minima and maxima with earliest ties | Independent dense minima and maxima |
| `Graph: read` (base, weighted and directed adapters) | Input adapters | Expected edge lists |
| `Graph: fromAdjacency, fromMatrix` | Every exhaustive and random graph as shuffled weighted and unit lists; builder cases with full-range weights differing in one random byte, loops and parallel edges, and random directed or symmetric matrices; fixtures for defaults, mirrored parallel edges listed in different orders, loops and empty inputs | Directed: the list scan. Undirected: the source edge multiset normalized to `(min, max, w)` and sorted with `std::sort`. Matrices: the row-major nonzero scan, and identity with `fromAdjacency` of the same lists |
| Legacy `GraphLabels` cases (DS row 07 `CoordinateCompression`) | Integer extremes, strings with the empty label, keys with only `operator<`, empty set, copy and move, absent labels | Sorted distinct labels; -1 for an absent label |
| `lineGraph`, `Subgraph` | Each graph and its CSR snapshot | Exact edge sequence built from the edge list in the stated order |
| `inducedSubgraph` | Empty, full and three shuffled random subsets | Exact edge sequence (source edges with both ends in `vs`, grouped by the position of `u` in `vs`, IDs increasing) with relabelled endpoints |
| `contract` | Identity, all-merged and random labels (two spare labels) | Relabelled edges |
| `simplify` | Every graph and every contraction | Ordered pair map of `(w, id)` minima |
| `Subgraph: vertex` | Every `inducedSubgraph`, `simplify`, `edgeSubgraph` and `restrictedView` result | `vs` for `inducedSubgraph`, the identity otherwise |
| `Graph: addPath, addCycle, addStar` | Appended paths, cycles (one and two vertices) and stars on every graph | Expected appended edges and returned IDs |
| `edgeSubgraph`, `restrictedView` | All, empty and shuffled ID subsets; hidden vertices and IDs with duplicates | Set-based filtering of the edge list |
| `inDegrees`, `edgeId` | Every graph, every pair `(u, v)` | Incidence counts; a linear scan for the first matching edge |
| `complementGraph` | Every graph | Adjacency matrix |
| `graphPower` | `k` in {0, 1, 2, 3, n, `INT_MAX`} | Floyd hop distances |
| `toUndirected`, `toDirected`, `subdivideEdges`, `mycielskian` | Every graph | Edge lists from the definitions; fixtures (Mycielskian of K2 is C5) |
| `disjointUnion`, `graphJoin`, `cartesianProduct`, `tensorProduct`, `strongProduct`, `lexicographicProduct`, `rootedProduct` (every root), `coronaProduct`, `modularProduct` | Each graph with random second factors of 0–3 vertices on both sides; fixtures (K2 × K2 as C4, 2K2 and K4; P4 squared; corona as a triangle) | Exact expected edge sequence from the definition, and an adjacency-count identity from the factors' matrices |
| `graphUnion`, `graphIntersection`, `graphDifference`, `graphSymmetricDifference` | Both argument orders and each graph with itself | Count-map matching per endpoint pair; max, min, excess and absolute-difference pair counts |
| Size bounds | `disjointUnion`, `cartesianProduct`, `coronaProduct`, `mycielskian` and `subdivideEdges` with a result of exactly `INT_MAX` vertices, on a graph type holding only `n`, `directed` and `edges`, in a child process limited to 1 GiB of address space (optimized and checked builds; not under ASan, whose shadow memory needs more) | The call passes every assertion and ends in `bad_alloc` |
| Preconditions | 85 assertion probes, among them `induced-negative`, `induced-end`, `induced-duplicate`, `contract-size`, `contract-count`, `contract-negative`, `contract-end`, `adjacency-negative`, `adjacency-end`, `adjacency-asymmetric`, `adjacency-multiplicity`, `adjacency-loop-once`, `adjacency-weight`, `adjacency-loop-weight`, `matrix-ragged`, `matrix-asymmetric` | Expected assertion failure |

Probe design: the `*-past-end` probes (for `operator[]` of both graphs, `inducedSubgraph` and `restrictedView`) and the root probes use operands past the end or edgeless graphs, so no later assertion can stand in for the checked one. `corona-overflow-window` (65535 × 32768) also requires the failing assertion to name `coronaProduct` on stderr, since a weakened check would end at Graph's `n >= 0` assertion instead; `union-overflow`, `mycielskian-overflow` and `subdivide-overflow` do the same one vertex past the bound of `disjointUnion`, `mycielskian` and `subdivideEdges`, on the storage-free graph type of the size-bound row.

## Commands and Results

2026-11-27 (P010 limits recheck, experiment none) for the tester runs, extraction of the tester and integration; 2026-11-14 (P010 GR01) for the rows so marked, the header being unchanged since. Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`.

| Run | Result |
|---|---|
| Full, seed 20261127, g++ | PASS, all three builds, 113,094,467 checks (optimized, checked) and 113,094,462 (ASan-UBSan, which skips the five size bounds), 85 probes; optimized 1.9 s, checked 6.1 s, ASan-UBSan 25.9 s; pre-flight quick peak 1,090 MB; peak 1,575 MB |
| Full, seed 20261127, `CXX=g++-14` | PASS, same counts; optimized 3.1 s, checked 5.5 s, ASan-UBSan 23.9 s; peak 1,560 MB |
| Stress, seed 7 | PASS, all three builds, 906,034,952 checks (906,034,947 under ASan-UBSan), 85 probes; optimized 10.4 s, checked 35.9 s, ASan-UBSan 188.8 s; peak 687 MB |
| Hand-applied size-bound mutants (scratch copy, checked build) | 7 of 7 caught: `COS 1635`, `AOS 939`, and `<` or `-` in the `disjointUnion`, `mycielskian` and `productBase` bounds |
| `05-extraction.py ops --filter 04-Graphs/001-graph.hpp` (2026-11-14) | PASS on g++ and g++-14 |
| `05-extraction.py testers --filter 001-graph_tester` | PASS |
| `02-integration.py` (192 standalone and aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 3,438 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/001-graph_tester.py' --mode full --seed 20261127
CXX=g++-14 python3 '96-Local Testing/04-Graphs/001-graph_tester.py' --mode full --seed 20261127
python3 '96-Local Testing/04-Graphs/001-graph_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '04-Graphs/001-graph.hpp'
python3 '96-Local Testing/05-extraction.py' testers --filter 001-graph_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Mutation Testing

Record: mewt 4.0.0 on blob `a2d829d3fa367f3b3b176a7cd381684ef722f56e` (2026-11-14, seed 1, 2 lines changed since blob `6e0e4e552eb6`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested. /finish GR01 changed only the comment lines above `GraphEdge` and `GraphArc`.

The survivors below come from the whole-file campaign of 2026-11-14 (seed 1, uniform sample of 500 of 1723 mutants; after the rerun with the extended tester, 458 caught, 9 by timeout, and 42 survived, of which /finish GR01 closed `AOS 853` and `COS 1634` with the `corona-overflow-window` probe, and the 2026-11-27 size-bound checks closed `COS 1635` and `AOS 939`, each applied by hand to a scratch copy and caught by the checked build). The newer record covers only comment lines, so their arguments stand. All 38 are equivalent; line numbers are the current header's.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `CR 203`, `AOS 632`, the counting sort's early return removed or never taken | 16 | The early return skips only a sort whose keys are all equal, which is the identity; the full pass stays within O(m + k). |
| `COS 1034`, `1109`, `1169`, `1184`, `1229`, `1289`, `1294`, `1379`, `1439`, `1494`, `1559`, `1579`, `1589`, `1644`, `1654` `<` → `!=` | 17, 65, 76, 78, 123, 165, 166, 206, 257, 314, 364, 377, 386, 451, 467 | Each counter steps by 1 from at most its bound (line 65: `i = j` with `j > i`, since `es[i]` matches its own key). |
| `COS 1484`, `depth < k` → `!=` | 309 | BFS depths along the queue start at 0 and rise by at most 1, and `k >= 0` is asserted, so both stop at the first vertex of depth `k`. |
| `CR 221`, the directed early return removed | 58 | A directed build leaves the buffer empty, so the sorts and the grouping loop do nothing; the O(n) counter arrays stay within O(n + m). |
| `IT 435`, the weight passes forced for `int` lists | 59 | Every weight is 1, so each pass returns at its all-keys-equal check. |
| `AS 473`, `476`, swapped `min` arguments | 63, 64 | `min` is symmetric. |
| `COS 1127`, `u <= v` → `u >= v` when counting mirrored entries | 67 | On a non-loop key the two counts sum to `j - i`, so `2 * forward == j - i` holds for one exactly when it holds for the other; on a loop key both count every entry. |
| `COS 1149`, `k > 0` → `k != 0` | 69 | `k` starts at least 0 and steps down by 1. |
| `COS 1154`, `w.size() <= INT_MAX` → `!=` | 72 | Unreachable: it differs only for at least `INT_MAX` rows (48 GiB of row vectors), and the assertion vanishes under `NDEBUG`. |
| `CR 236`, `AOS 671`, `672`, the arc-count assertion removed or loosened | 87 | Unreachable: it binds only at `INT_MAX - 2` arcs (64 GiB), and the assertion vanishes under `NDEBUG`. |
| `CR 258`, `adj.reserve` removed | 124 | Only capacity changes. |
| `COS 1255`, `b == -1` → `<=` | 146 | `b` is `-1` or an arc index. |
| `COS 1358`, `1360`, `1361`, `1362`, `v < u` → `==`, `<=`, `>`, `>=` in `simplify` | 200 | `v == u` was skipped just before. Scanning an undirected pair from the other endpoint, or from both, finds the same minimum `(w, id)` over its parallel edges, so the kept set and its ID order are identical. The extra scan is O(m). |
| `COS 1364`, `seen[v] != u` → `<` | 201 | Stamps are `-1` or earlier scanning vertices, all below `u`. |
| `AOS 824`, `s + 1` → `s * 1` | 314 | The loop then starts at `s`, which `v != s` excludes. |
| `COS 1595`, `es[j].u == es[i].u` → `<=` | 386 | The buffer is stably sorted by `u`, so `es[j].u >= es[i].u` for `j >= i`. |
| `AS 594`, `productBase(h, g)` | 460 | `modularProduct` takes undirected factors and `productBase` asserts equal direction, so the swapped call builds the same `h.n * g.n` vertices. |

Earlier hand-planted mutation checks (2026-10-07 and 2026-10-12) are superseded by this campaign.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/00-foundations_benchmark.py>) compares the same BFS kernel on adjacency-list and CSR storage, on chain, star and connected random multigraphs at 32, 3,000 and 100,000 vertices, seed 20260927, with one checked warmup and five samples (repeated for smaller graphs). CSR conversion is timed separately; input construction is excluded. All traversal checksums agree. Both representations store O(n+m), and conversion keeps the input beside the output. Conditions, from the git-ignored `00-foundations_benchmark.json` log that also holds the raw samples: 11th Gen Intel Core i9-11900H (shared host), GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`.

| 100,000-vertex random graph (499,999 logical edges) | Median |
|---|---|
| BFS on adjacency lists | 14.34 ms |
| BFS on CSR | 9.27 ms |
| CSR construction | 4.48 ms |

Tiny cases slightly favoured lists, and chain and star results varied. This supports an explicit reusable CSR option, not an automatic threshold or a general speed claim.

```bash
python3 '96-Local Testing/04-Graphs/00-foundations_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied. The 2026-11-14 additions follow the pages in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr01) (definitions only, no code copied).

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 图的存储](https://oi-wiki.org/graph/save/) (page revision 2026-09-12) | Matrix, list and forward-star trade-offs and paired reverse arcs; used to assess storage coverage. |
| [Boost 1.89, Compressed Sparse Row Graph](https://www.boost.org/doc/libs/1_89_0/libs/graph/doc/compressed_sparse_row.html) | Source-group offsets, O(n+m) storage and immutable CSR use. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions).

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): 图的存储, 邻接表, 邻接矩阵, 前向星, 邻接串列, 线图, 导出子图, 诱导子图, 图的积, 笛卡尔积, 张量积, 强积, 字典积, 补图, 图的幂, 不交并, 图的并, 图的交, 边细分, 自我中心网络; graph product, Cartesian product, Kronecker product, complement graph, graph power, disjoint union, graph join, Mycielski graph, edge subdivision, compose (NetworkX).

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-06: inventory rewrite reopened the row as partial (line graph, induced subgraph, contraction, simplification).
- 2026-10-07: re-audit, implemented `lineGraph`, `inducedSubgraph`, `contract`, `simplify`; 5 findings fixed (missing operations, evidence claim, complexity lines, brace rule, T/M placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, added `Graph::fromAdjacency` and `fromMatrix`, removed `GraphLabels` (cases moved to `CoordinateCompression`); full suite passed on g++ and g++-14, stress passed; hand-planted builder mutants all killed but one equivalent; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR01, added `Graph::addPath`, `addCycle`, `addStar`, `Subgraph::vertex`, `edgeSubgraph`, `restrictedView`, `subdivideEdges`, `complementGraph`, `disjointUnion`, `graphJoin`, the four edge-set operations, seven products, `mycielskian`, `graphPower`, `toUndirected`, `toDirected`, `inDegrees`, `edgeId`; full suite passed on g++ and g++-14, stress passed; independent review: 8 findings (allTopologicalSorts unbounded under NDEBUG for a negative limit, comment cap, missing complexity symbols and output terms, addCycle on an empty list under NDEBUG, duplicated in-degree and layer loops, evidence wording, an unqualified std name) fixed; whole-file mutation campaign, 12 survivors closed by new probes and the rest recorded equivalent (lineGraph edge order specified, user decision 2026-11-14).
- 2026-11-14: /finish GR01, independent audit: 12 findings (1 medium: two bfs/dfs range-check mutants wrongly recorded equivalent; 11 low) fixed; GraphArc got its own complexity line; past-end probes for both `operator[]`, `inducedSubgraph` and `restrictedView`, a `corona-overflow-window` probe that requires the corona assertion (closing AOS 853 and COS 1634), and an exact `inducedSubgraph` edge-sequence oracle; full suite passed on g++ and g++-14, stress passed; mutation record on the changed lines (comments only, 0 mutants). Review of the fix diff: 8 low findings (evidence wording, probe text match, stderr restore, a notes line for the bounded BFS) fixed.
- 2026-11-27: P010 limits recheck (experiment none): the ownership boundaries moved from Limits and Handoffs to Contracts (Scope); the review found the size-bound survivors reachable, so five accepted-bound checks at exactly `INT_MAX` vertices and the `union-overflow`, `mycielskian-overflow` and `subdivide-overflow` probes closed `COS 1635` and `AOS 939`; header unchanged; full suite passed on g++ and g++-14, stress passed, extraction of the tester and integration passed.
