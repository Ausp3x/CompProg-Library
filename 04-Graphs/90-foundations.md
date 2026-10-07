# P010 graph foundations — GR01 then GR02

This record covers the six Basic graph headers owned by P010. The prerequisite P002 template and P006 canonical DSU are verified. Implementations are independent contest-profile code; archived snippets remain unchanged. The package was first verified on 2026-09-27. The 2026-10-06 inventory rewrite reopened `01-graph.hpp` (line graph, induced subgraph, contraction, simplification) and `06-mst.hpp` (maximum-weight forests) as partial; the [re-audit of 2026-10-07](#re-audit--2026-10-07) implemented those operations, added the research additions `TopologicalResult::unique` and `KruskalReconstruction::leafRange`, and closes both rows.

## Contracts

### Common

Every algorithm accepts `Graph` or `CsrGraph` unless it names `DenseGraph`. Vertices are `[0, n)`, logical edges `[0, m)` and arcs `[0, arcs.size())`, and every count fits `int`. Weights span the full `lng` range; path and forest totals are exact `lll`. Results hold arc and edge indices into the input graph, so they stay meaningful only while that graph's arc numbering and content are unchanged; result arrays themselves own their storage. Public storage of graphs and results is inspectable but must not be edited behind an algorithm's invariants. Violated preconditions (out-of-range vertex, wrong directedness, negative weights for Dijkstra) are assertions; a valid input with no answer returns a status field or an empty witness. Complexity comments use `m` for the arc count where it differs from the edge count only by a factor of two.

### GraphEdge and GraphArc

`GraphEdge {u, v, w}` is a logical edge in insertion order. `GraphArc {from, to, id, rev, w}` is an oriented arc: `id` is its logical edge, `rev` the reciprocal arc of an undirected edge and `-1` for a directed arc. An undirected edge, including a loop, has two distinct reciprocal arcs `a` and `a + 1`.

### Graph

`Graph(n, directed)` with `n >= 0`. `addEdge(u, v, w = 1)` returns the new logical edge ID; it invalidates references and iterators into the storage, and IDs are stable until assignment or destruction. `operator[](u)` returns the arc indices out of `u` in insertion order. `reverse()` is O(n + m): directed arcs swap endpoints, undirected edges keep their orientation, and logical edge IDs are preserved. `read(in, n, m, directed, weighted, base)` consumes exactly `m` records `u v [w]` with `base` 0 or 1; omitted weights become 1. Well-formed input, including successful extraction and in-range endpoints, is a precondition.

### CsrGraph

An immutable owning snapshot of a `Graph` with contiguous adjacency, keeping arc IDs, edge IDs and adjacency order. `operator[]` returns a `std::span` that borrows this object and expires on its assignment or destruction. Construction workspace is O(1) beyond the result; `reverse()` is O(n + m) including a transient adjacency-list graph.

### DenseGraph

Owns a copy of the full multigraph in `graph` plus `best[u][v]`: the arc index of the lightest `u -> v` arc, or the heaviest when constructed with `maximum = true`, `-1` when there is none. Equal weights choose the earliest arc. There are no implicit diagonal entries; loops are stored like any other arc. `dijkstraDense` requires the minimum view; `primDense` builds a minimum or maximum forest according to `maximum`.

### GraphLabels

Sorted label compression under a strict weak ordering; labels equivalent under `!(a < b) && !(b < a)` merge. `index(x)` asserts that `x` is present and runs in O(log(k)); `value(i)` returns a reference into this object's storage in O(1). The labels must include every endpoint the caller compresses.

### Subgraph, lineGraph, inducedSubgraph, contract and simplify

`Subgraph {graph, edge}` maps each new edge `i` back to the source edge `edge[i]`. All four utilities accept `Graph` or `CsrGraph` and return a new owning `Graph` with the same directedness.

- `lineGraph(g)` has one vertex per source edge, every edge of weight 1. Directed: an arc `i -> j` for every pair of arcs with `head(i) == tail(j)`, so a directed loop gives a line-graph loop. Undirected: for every vertex `v` and every unordered pair of distinct edges incident to `v` (a loop counts once), one edge; parallel edges therefore give two parallel line edges, and `simplify(lineGraph(g))` is the simple line graph. Cost O(n + m + out) with `out` the line-graph edge count; `addEdge` asserts that it fits `int`.
- `inducedSubgraph(g, vs)` takes distinct in-range vertices (asserted). New vertex `i` is `vs[i]`, so the caller's list is the vertex map. Kept edges are those with both ends in `vs`, with orientation and weight preserved, ordered by the position in `vs` of the scanning endpoint (`u` for undirected edges) and then by adjacency order. Cost O(n + k + d) with `d` the degree sum of `vs`; the O(n) term is the position array.
- `contract(g, label, k)` requires `label.size() == n`, `k >= 0` and every label in `[0, k)`. Edge `i` of the result is `(label[u], label[v], w)` of source edge `i`, so edge IDs are identical and loops and parallel edges are kept. `simplify(contract(...))` gives the quotient graph, for example a bridge tree from 2-edge-connected labels or a condensation from SCC labels.
- `simplify(g)` drops loops and keeps, per ordered pair (directed) or unordered pair (undirected), the edge with the smallest `(w, id)`. Kept edges keep their original orientation and appear in increasing source ID order. Cost O(n + m).

### CycleWitness and TraversalResult

A present cycle satisfies `vertices.size() == arcs.size() + 1`, `vertices.front() == vertices.back()`, and repeats no other vertex or logical edge; an empty arc list means no cycle. A loop is a one-arc cycle; two parallel undirected edges form a two-arc cycle. `TraversalResult(n)` asserts `n >= 0`. Unreached `parent`, `parent_arc`, `depth` and `root` entries are `-1`; a root has depth 0, no parent and its own vertex as root. `order` is discovery order. DFS also fills `postorder` and its first cycle; BFS leaves both empty. The internal `traversal_detail::treeCycle` closes the tree paths between the ends of a non-tree arc; it is covered through `dfs`, `findCycle` and `bipartiteCheck`.

### bfs

`bfs(g, sources)` seeds every source, in list order with duplicates ignored, before traversal, so `depth` is the unweighted shortest distance from the source set. Ties follow source order and then arc order: `order`, `parent`, `parent_arc` and `root` equal those of a plain FIFO queue. Every source must be in `[0, n)`; the single-source overload is `bfs(g, s)`. An empty source list reaches nothing.

### dfs, dfsForest and findCycle

`dfs(g, sources)` is an iterative DFS with exactly the recursive discovery order, finishing order and parents. Sources are considered in turn, skipping already reached ones, so `depth` is tree depth. Directed cycles are detected by gray ancestors; undirected DFS skips only the exact reverse of the parent arc, so loops and parallel edges are detected. `dfsForest(g)` seeds every vertex in order; `findCycle(g)` returns its first cycle, directed or undirected, or an empty witness.

### connectedComponents and bipartiteCheck

Both require an undirected graph. Components are numbered by increasing minimum vertex; members follow traversal order. `bipartiteCheck` colors BFS layers; on success `color` is a complete 0/1 coloring, on failure it is partial (`-1` unvisited) and `cycle` is an odd cycle.

### TopologicalResult, topologicalSort and topologicalSortDfs

All require a directed graph and ignore weights. `topologicalSort(g)` is Kahn's algorithm with a FIFO queue; `topologicalSort(g, true)` uses a min-heap and returns the lexicographically smallest order. Parallel arcs count separately in in-degrees. `topologicalSortDfs(g)` returns the reverse DFS finishing order. On a cycle, `acyclic` is false, `order` is empty and `cycle` is a directed witness. `unique` is true exactly when the order is the only topological order; an empty or one-vertex graph has a unique order, and a cyclic graph has `unique = false`.

### graphComponents

Returns the canonical P006 `DSU` after uniting the ends of every edge: connected components, or weak components of a directed graph. Loops are allowed. The DSU is re-exported by inclusion, with no duplicate implementation.

### PathWitness and ShortestPathResult

`PathWitness.exists` distinguishes an absent path from a source's zero-edge path, whose `vertices` holds that one source. `ShortestPathResult(n, sources)` asserts in-range sources and marks them reachable at distance 0. `dist[v]` is meaningful only when `finite(v)`. `reachable` includes negative-infinite vertices, and `negative` marks exactly those. Parents of finite vertices lead to a source, whose parent is `-1`. `negative_cycle` is one reachable negative directed arc-cycle; an undirected negative edge can contribute both of its reciprocal arcs. `path(g, v)` needs the graph that produced the result. `relax(e, a, longest)` is the public relaxation step: it asserts in-range endpoints, a reachable `e.from` and `a >= 0`, and needs a representable candidate sum. The algorithms use the unchecked `shortest_path_detail::relax` after validating their input once at entry.

### bfsShortestPaths, zeroOneBfs, dijkstra and dijkstraDense

`bfsShortestPaths` ignores weights, counts arcs, and has the `bfs` tie order. `zeroOneBfs` requires every weight in {0, 1}; its deque holds stale entries, so workspace is O(n + m). `dijkstra` is a lazy binary heap and requires every arc weight nonnegative, including arcs outside the reached component. `dijkstraDense` is the O(n^2) array scan over a minimum `DenseGraph` view and requires every source-graph weight nonnegative, even for discarded parallel arcs; it excludes the O(n^2 + m) view construction. Each has a single-source overload.

### dagShortestPaths and dagLongestPaths

Directed graphs with arbitrary signed weights. `longest = true`, or `dagLongestPaths`, maximizes instead of minimizing. A cyclic graph returns `acyclic = false`, a result that reaches nothing, and a directed cycle witness. Signed paths may improve a source through another source.

### bellmanFord and findNegativeCycle

`bellmanFord(g, sources)` runs at most n in-place passes over all arcs. An improvement in pass n seeds negative-infinity propagation along every reachable arc. In-place distances sum at most n * m `lng` weights; with `int`-bounded n and m their magnitude is below 2^125, so `lll` is exact. `findNegativeCycle(g)` seeds every vertex and returns a negative directed arc-cycle anywhere in `g`, or an empty witness.

### Dijkstra (legacy adapter)

Compatibility with the extracted one-based API: vertices `[0, n]` with `n < INT_MAX`, and adjacency with at least `n + 1` lists. Nonnegative weights and every reachable shortest distance below `INF64` are preconditions; the new APIs have no `INF64` bound. Each `runGraph`/`runDijkstra` resets `dis` and `is_proc`, and `unproc` is empty after a completed run. The unweighted overload ignores weights by construction. Run workspace is O(n + m).

### SpanningForest, kruskal, primSparse, primDense and boruvka

All require an undirected graph, ignore loops and accept parallel and negative edges. `maximum = true` (or a maximum `DenseGraph` view for `primDense`) builds a maximum-weight spanning forest; the comparator reverses the weight order instead of negating weights, since `-w` overflows for `lng` minimum. `SpanningForest` holds the exact `lll` weight, the component count including isolated vertices, and the original logical edge IDs; an empty graph has zero components. Ties compare weights, then edge IDs. Returned edge order follows each algorithm. `primSparse` keys its set by `~w` for maximum, which is an order-reversing bijection on `lng`.

### BottleneckResult and KruskalReconstruction

`BottleneckResult` distinguishes a disconnected pair, a valid empty self-path, and a nonempty path whose `value` and `edge` are its bottleneck weight and merge edge. `KruskalReconstruction(g, maximum)` requires an undirected graph with `n <= (INT_MAX + 1) / 2`. Leaves `[0, n)` are the original vertices; each merge node has two children, its joining edge ID and weight. `parent` is `-1` at roots, `root[u]` is the tree root, `leaf_count[u]` counts original leaves, and leaf `value` 0 is unused. Ascending construction answers minimax paths, `maximum = true` maximin. `lca(u, v)` accepts all nodes and returns `-1` across trees. `bottleneck` and `componentAt` accept original vertices. `componentAt(u, t)` is the highest ancestor whose merges all have weight `<= t` (`>= t` for maximum), so its leaves form u's component. `leaves` lists the original vertices so that every node's leaves are contiguous, and `leafRange(node)` returns that half-open interval `[start[node], start[node] + leaf_count[node])` into `leaves`. Construction workspace is O(n + m).

## Correctness and cost — GR01

`Graph` owns zero-based vertices, insertion-ordered logical edges and oriented arcs. All counts fit `int`; weights use the full `lng` domain. An undirected edge has two reciprocal arcs even for a self-loop. A directed arc has `rev == -1`; its reverse graph is a separate object. Reversal preserves logical edge IDs. Algorithms return arc indices so orientation and parallel edges remain distinguishable; `g.arcs[a].id` recovers the logical ID. Public storage is inspectable but must not be edited directly.

`CsrGraph` is an owning immutable snapshot with contiguous adjacency indices. It preserves arc IDs and adjacency order. `DenseGraph` keeps the full graph and an O(n²) matrix selecting the minimum-weight arc per ordered pair, with earliest-arc ties and `-1` for absence. There are no implicit diagonal edges. Thus a dense projection does not lose parallel-edge provenance. `Graph::read` accepts exactly the requested number of weighted/unweighted records and base 0 or 1; malformed input is a precondition violation. `GraphLabels<T>` provides sorted coordinate compression with ordering-equivalent labels merged.

`bfs` seeds all sources before traversal, yielding unweighted shortest distances; `dfs` considers seeds sequentially and returns recursive-order discovery/finishing order without recursion. Duplicate sources are ignored, and empty source lists reach nothing. Unvisited fields are `-1`; roots have depth zero, no parent and their own root ID. DFS depth is tree depth. `dfsForest` covers all vertices. DFS detects a gray ancestor and skips only the reverse parent arc in undirected graphs, so loops and parallel-edge cycles are handled correctly. `CycleWitness` has one more vertex than arcs, repeated first/last vertex, and no other repeated vertex; an empty arc list means no cycle.

`connectedComponents` and `bipartiteCheck` require undirected input. Components are numbered by increasing minimum vertex; member order follows traversal. The bipartite algorithm colors BFS layers and returns an odd cycle when an edge joins equal colors. Parent-tree paths meet at their common ancestor, so the resulting cycle is contiguous and simple. Coloring on failure may be partial. The graph-facing `graphComponents` adapter uses the canonical P006 `DSU`, treating a directed graph as weak connectivity; it neither duplicates nor modifies the DSU implementation.

The graph utilities are direct constructions. `lineGraph` enumerates, per vertex, the pairs of incident edges (undirected) or the arcs leaving each edge's head (directed), so it emits each line-graph edge exactly once in O(n + m + out). `inducedSubgraph` scans only the chosen vertices' adjacency and keeps an undirected edge from its first arc, which has the smaller index, so each edge is taken once with its stored orientation. `contract` relabels endpoints edge by edge. `simplify` scans each vertex's adjacency with a stamp array keyed by the scanning vertex, keeps the smallest `(w, id)` per neighbour (an undirected pair only from its smaller endpoint), then emits the kept IDs in increasing order; every arc is touched twice, so it is O(n + m) without sorting.

A topological order is unique exactly when every consecutive pair is joined by an arc: if some consecutive pair `x, y` has no arc `x -> y`, no path joins them either (any path from `x` to `y` would pass through vertices strictly between them in the order), so swapping them gives a second order; conversely, a Hamiltonian path fixes the order. `toposort_detail::ordered` checks this in O(n + m) for both Kahn and DFS orders.

Kahn topological sorting counts every arc, including duplicates. A FIFO queue gives linear time; a minimum heap yields the lexicographically smallest valid order. DFS reverse postorder is a second implementation. Both require directed input and return an explicit acyclic flag; cyclic inputs return an empty order plus an oriented cycle. Empty directed graphs succeed with empty orders.

## Correctness and cost — GR02

All new weighted algorithms use exact signed-integer weights from the full `lng` range and `lll` accumulated distances/costs. Status fields distinguish absent, finite and unbounded answers without reserving a numeric weight. Fractional/custom-weight instantiations of archived notebook templates remain outside this specified integer API.

`ShortestPathResult` stores `dist`, `reachable`, `negative`, `parent`, `parent_arc` and one reachable `negative_cycle`. A distance is meaningful only when `finite(v)` is true; reachable negative-infinite vertices have both flags set. `path(g,v)` returns an explicit existence flag and oriented vertex/arc sequence; a valid zero-edge source path has one vertex. The input graph's arc IDs/content must remain unchanged. Sources are zero-initialized, duplicate sources are ignored, and an empty source set reaches nothing. Signed DAG/Bellman–Ford paths may improve a source through another source.

`bfsShortestPaths` counts arcs while ignoring weights. `zeroOneBfs` requires all weights in {0,1}; deque ordering processes nondecreasing distances and stale entries avoid rescanning improved candidates. Sparse `dijkstra` uses a lazy binary heap; both it and `dijkstraDense` require every original edge weight to be nonnegative, including edges in unreachable components. Dense construction selects the best parallel edge while retaining its original arc ID. For general multigraphs, sparse heap time is O(k+(n+m) log(n+m+1)) and workspace O(n+m), where m is the arc count and k is the source count. Dense time is O(n²+m+k) beyond dense-view construction. BFS/0-1 BFS are linear in n+m+k.

`dagShortestPaths`/`dagLongestPaths` relax a directed graph in topological order with arbitrary signed weights. A cyclic graph is a valid failure result with an oriented witness and no reachable path result. Topological induction establishes the minimum/maximum over all source paths. `bellmanFord` performs at most n in-place passes; an improvement in pass n is reachable from a negative cycle. It marks all such destinations and every vertex reachable from them as negative-infinite. Stepping n predecessors from the final changed vertex reaches a negative cycle, which is recovered with original arc orientation. This witness is a directed arc-cycle; an undirected negative edge legitimately supplies both reciprocal arcs of the same logical edge. `findNegativeCycle` seeds every vertex, so it also detects cycles outside any single source's reachable component.

Every Bellman–Ford intermediate represents a walk with at most n·m edge additions. Since n,m≤INT_MAX and |w|≤2^63, its magnitude is less than 2^125; exact `lll` arithmetic needs no saturating sentinels. Finite optimal paths, DAG paths and forest totals have smaller bounds. Strict relaxations preserve finite parent chains and do not form spurious zero-weight parent cycles.

The historical `Dijkstra(n)` adapter retains vertices [0,n], `dis`, `is_proc`, `unproc` and repeated `runDijkstra` resets. Weighted and notebook-style unweighted adjacency overloads are supported. This compatibility interface requires n<INT_MAX, at least n+1 adjacency lists, nonnegative weights and each reachable shortest distance strictly below `INF64`; its queue is empty after a completed run. The new result API supports larger exact distances and explicit absence.

`kruskal`, `primSparse`, `primDense` and `boruvka` return a `SpanningForest` with its exact weight, component count and original logical edge IDs. They require undirected input, ignore loops, accept parallel/negative edges and include isolated vertices as components. The empty graph returns weight zero, zero components and no edges. Ties compare logical edge IDs; returned edge order follows each algorithm. A forest contains exactly n−components edges, so its full-range signed sum fits comfortably in `lll`.

Kruskal adds sorted edges only across distinct components. Prim repeatedly chooses the minimum crossing edge; its sparse set keeps one candidate per unused vertex, and its dense form scans the already-built minimum-edge matrix. Both restart at isolated/disconnected components. Borůvka caches phase-start representatives and chooses each active component's minimum outgoing edge; union guards suppress duplicates/cycles. The cut property justifies every accepted edge. Active components halve per phase until each original connected component is merged. Bounds are O(n+m log(m+1)) for Kruskal, O((n+m) log(n+1)) for sparse Prim/Borůvka, and O(n²) for dense Prim with O(n²+m) dense-view setup charged separately.

Maximum-weight forests reuse the same engines with the comparator reversed on weight and edge-ID ties kept ascending. Negating weights is avoided because `-w` overflows at the `lng` minimum; `primSparse` keys its set by `~w = -w - 1`, a bijection that reverses the order on all of `lng`. The cut property holds for maximum forests with the inequalities reversed.

`KruskalReconstruction` stores a binary union forest. Leaves are original vertices, each internal node records the accepted merge edge/weight, and parents have larger IDs than children. Descending node IDs build depth/root/binary-lifting data without recursion. The LCA merge weight is the minimum possible maximum edge on a path; descending construction (`maximum=true`) gives the maximum possible minimum edge. `bottleneck` explicitly distinguishes disconnected pairs, a valid empty self-path and a nonempty path; the latter also returns the merge edge ID. `lca` accepts all reconstruction nodes, while `bottleneck` and `componentAt` accept original vertices. `componentAt` climbs every threshold-eligible ancestor, including equal-weight merges, and `leaf_count` gives the component size. Increasing/decreasing merge weights make binary lifting valid. The same descending pass assigns `start`: a root starts after the previous roots' leaves, a first child at its parent's start, a second child after the first child's leaves. Every subtree is therefore one contiguous interval of `leaves`, which `leafRange` returns, in O(n) extra time and memory. Construction requires n≤(INT_MAX+1)/2 so at most 2n−1 nodes fit `int`; preprocessing/storage is O(n log(n+1)) beyond edge sorting, with O(log(n+1)) queries.

## Source review and legacy accounting

Sources below were retrieved and inspected on 2026-09-27. They support the stated algorithms and contracts; no source code was copied. A source review is distinct from verification or online acceptance.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 图的存储](https://oi-wiki.org/graph/save/) (page revision 2026-09-12) | Matrix/list/forward-star tradeoffs and paired reverse arcs; used to assess storage coverage. |
| [Boost 1.89, Compressed Sparse Row Graph](https://www.boost.org/doc/libs/1_89_0/libs/graph/doc/compressed_sparse_row.html) | Source-group offsets, O(n+m) storage and immutable CSR use. |
| [cp-algorithms, Breadth-first search](https://cp-algorithms.com/graph/breadth-first-search.html) | Queue layers, parent recovery and unweighted shortest distances. |
| [cp-algorithms, Bipartite check](https://cp-algorithms.com/graph/bipartite-check.html) (updated 2026-04-02) | BFS coloring and parity criterion. |
| [cp-algorithms, Finding a cycle](https://cp-algorithms.com/graph/finding-cycle.html) | Gray-state cycle recovery. Its simple-graph limitation is extended here using reverse-arc identities. |
| [OI Wiki, 拓扑排序](https://oi-wiki.org/graph/topo/) (updated 2026-04-23) | Kahn, DFS and heap-based lexicographic ordering. |
| [cp-algorithms, Bellman–Ford](https://cp-algorithms.com/graph/bellman_ford.html) | The nth-pass negative-cycle criterion, predecessor recovery, downstream unbounded vertices and all-zero initialization. |
| [KACTL, BellmanFord.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/BellmanFord.h) (Simon Lindholm, 2015-02-23; CC0) | Independent comparison of finite/unreachable/unbounded contracts and downstream propagation. Its specialized pass ordering is not adopted. |
| [Princeton Algorithms, §4.4 Shortest Paths](https://algs4.cs.princeton.edu/44sp/) | Nonnegative Dijkstra and signed DAG shortest/longest topological relaxation. |
| [cp-algorithms, 0-1 BFS](https://cp-algorithms.com/graph/01_bfs.html) | Deque ordering and zero/one front/back insertion. |
| [OI Wiki, 最小生成树](https://oi-wiki.org/graph/mst/) | Kruskal/Prim/Borůvka cut choices, forest behavior, reconstruction trees and bottleneck LCA interpretation. |
| [cp-algorithms, Prim](https://cp-algorithms.com/graph/mst_prim.html) | Dense quadratic and sparse priority-queue alternatives. |

The original `Dijkstra` in `OLD/algorithms.cpp:4488–4526` and its duplicate in `OLD/[1] algorithms.cpp` are preserved. Its n+1 indexing, multisource runs and reset behavior require an explicit compatibility adapter; the new graph APIs are zero-based. The old notebook's Kruskal forest behavior and dense Prim are covered by the new spanning-forest family without adopting its disconnected `-1` sentinel or empty-graph bug. The notebook's global negative-cycle detector is covered by all-vertex Bellman–Ford initialization.

The old Team Notebook `old_topo.cpp` and `old_cycle-find-dfs.cpp` were inspected: recursive reverse postorder and gray-state recovery are replaced by iterative implementations. The broken exploratory destructive cycle-deletion snippet remains archival; it is not an active API or a claimed reusable cycle-enumeration engine. General cycle enumeration has its separate inventory owner.

The legacy bipartite snippet remains in `97-Legacy`; its coloring feature is covered by the new result API with a witness. Canonical P006 DSU already assigns historical potential/parity and component edge-count metadata to their Advanced owners; the graph adapter does not claim those variants.

## Feature-to-test map and execution

The entries below live under `96-Local Testing/04-Graphs`. The shared driver builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. Fresh runs used GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. The 2026-10-07 re-audit also ran the suites with GCC 14.4.1 (`CXX=g++-14`); see [Re-audit](#re-audit--2026-10-07).

| Header / tester stem | Independent coverage | Full result, seed 20260927 |
|---|---|---|
| `01-graph` | 606 exhaustive small graphs, 500 random multigraphs; edge/arc counts and identities, reverse involution, CSR equivalence, independent dense minima and maxima with earliest ties, snapshots/copy/move, defaults, input/label adapters (`index`, `value`) and full-range weights. Each graph and its CSR snapshot also run `lineGraph` against brute-force incidence counting, `inducedSubgraph` on the empty, full and three shuffled random subsets against a filtered edge list with relabelled endpoints, `contract` with identity, all-merged and random labels (two spare labels) against relabelled edges, and `simplify` (also on every contraction) against an ordered pair map of `(w, id)` minima. | All three builds passed, 19,364,200 checks each; 27 assertion probes. |
| `02-traversal` | 4,160 exhaustive/random graph cases against Floyd reachability/hop distances, a plain FIFO-queue BFS reference (exact `order`, `parent`, `parent_arc`, `root`), recursive traversal order and enumerated colorings; independently checked simple/odd-cycle witnesses, multiedges/loops, duplicate and repeated source lists and 200,000-vertex chains. | All three builds passed, 6,549,903 checks each; eight assertion probes. |
| `03-toposort` | 6,951 cases: exhaustive loopless graphs through four vertices and looped graphs through three; 1,200 random graphs/parallel-edge variants; independent permutation enumeration through seven vertices gives existence, the first valid permutation and `unique` (fewer than two valid permutations); oriented witnesses; 200,000-vertex chains and closed chains, a chain with skip arcs with and without one missing link, and 2,000 simultaneously ready vertices. | All three builds passed, 13,478,134 checks each; three assertion probes. |
| `04-dsu` | 67,166 exhaustive directed/undirected graphs through four vertices and 500 random graphs against transitive closure; CSR/reversal, canonical DSU identity and 100,000-vertex chain. | All three builds passed, 6,355,359 checks each; three assertion probes. |
| `05-shortest_path` | 4,096 signed three-vertex digraphs × all eight source subsets; 1,000 random multigraphs and 1,000 signed DAGs; exact Floyd reachability/distances/negative-cycle closure, independent simple-path maxima, oriented path/negative-cycle witnesses, CSR/overloads, full-range sums, helper/legacy reset tests and 100,000-vertex chains. | All three builds passed, 34,808 cases and 4,790,366 checks each; 16 assertion probes (new `dense-maximum`). |
| `06-mst` | 1,099 exhaustive weighted graphs and 400 random multigraphs; every engine in minimum and maximum form (Graph and CSR, minimum and maximum `DenseGraph` views) against independent forest-subset minimum/maximum; Floyd minimax/maximin and threshold closures, every-node LCA by ancestor sets, `leafRange` intervals against explicit subtree leaf sets, merge-edge certificates, explicit tie IDs for both orders, full-range weights/totals, copy/move/snapshots, 100,000-vertex chains and a triangle chain with closed-form minimum and maximum totals. | All three builds passed, 869,242 checks each; 14 assertion probes. |

Graph quick uses exhaustive graphs through two vertices and 70 random cases; full extends to three vertices/500 random cases, stress to 4,000 random cases. DSU quick uses three vertices/50 random cases, full four vertices/500 cases and stress four vertices/4,000 cases. Traversal/toposort quick use smaller exhaustive domains, 100 random cases and 20,000-vertex chains; full uses the tabled corpora, and stress extends random counts to 12,000/10,000 and chains to 500,000. MST quick uses n≤3/40 random cases/1,000-vertex chains; full uses the tabled corpora and stress 2,500 random cases/300,000-vertex chains. Shortest-path quick uses two-vertex exhaustive graphs, 100 multigraphs/100 DAGs and 2,000-vertex chains; full uses the tabled corpus, while stress adds 19,683 looped signed digraphs, 5,000 multigraphs/5,000 DAGs and 300,000-vertex chains. Exact mode coverage is also stated in each runnable entry. Full includes sanitizer builds; quick omits them. Sanitizer runs for all six entries encountered the environment's ptrace/LeakSanitizer restriction and passed after an approved execution outside the sandbox with leak detection enabled. These are completed retries, not skipped configurations. The initial 200,000-vertex all-ready topological fixture exceeded the checked-build timeout because libstdc++ validates the entire heap on each operation; the final fixture uses 2,000 all-ready vertices while retaining the 200,000-vertex deep cases, and passed all three builds.

Reproduce each entry with `python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20260927`, replacing the stem with the corresponding row. `--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry. The shared quick runner passed all six entries with seed 42 from `/tmp`, validating discovery and caller-directory independence:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Original 2026-09-27 record: integration passed 79 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage/execution, and LOCAL/non-LOCAL workspace compilation. Graph ASan/UBSan coverage comes from the six dedicated full suites; unrelated Core sanitizer suites were not rerun. Repository consistency passed with no errors after the final inventory, aggregate, provenance and checklist updates. All 61 checked precondition probes passed. Extended stress modes are implemented and documented but were not run in this completion pass. Independent review found no remaining correctness, domain, complexity or meaningful coverage gap.

## Representation benchmark

The [benchmark driver](<../96-Local Testing/04-Graphs/90-foundations_benchmark.py>) and [record](<../96-Local Testing/04-Graphs/90-foundations_benchmark.json>) compare the same BFS kernel on adjacency-list and CSR storage. It uses chain, star and connected random multigraphs at 32, 3,000 and 100,000 vertices, seed 20260927, one checked warmup and five samples with repetition for smaller graphs. CSR conversion is timed separately; input graph construction is excluded. All traversal checksums agree. Both representations store O(n+m), and conversion retains the input plus the output.

On this shared host, the 100,000-vertex random graph (499,999 logical edges) had median traversal times 14.34 ms for lists and 9.27 ms for CSR, plus 4.48 ms to construct CSR. Tiny cases slightly favored lists; chain/star results also varied. This supports an explicit reusable CSR option, not an automatic threshold or universal speed claim. Full compiler/CPU/flags and raw samples are in the record. Reproduce with `python3 '96-Local Testing/04-Graphs/90-foundations_benchmark.py'`.

## Dense/sparse algorithm benchmark

The [driver](<../96-Local Testing/04-Graphs/91-path_mst_benchmark.py>) and [record](<../96-Local Testing/04-Graphs/91-path_mst_benchmark.json>) compare sparse/dense Dijkstra and Prim on chains, random sparse multigraphs, complete graphs and disconnected blocks of at most eight vertices. Sizes are 24, 200 and 800; weights are uniformly drawn from [0,10^6], with seed 20260927. Five samples follow checked warmups, with repeated runs for smaller cases. All distances and forest totals/component counts are compared, with Kruskal providing the forest baseline. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately.

| 800-vertex workload | Dijkstra sparse/dense median ms | Prim sparse/dense median ms | Dense setup ms |
|---|---|---|---|
| Chain | 0.013 / 0.868 | 0.021 / 1.216 | 0.216 |
| Random sparse | 0.233 / 1.944 | 0.464 / 2.660 | 0.319 |
| Complete | 8.699 / 9.759 | 9.410 / 8.044 | 4.537 |
| Disconnected blocks | 0.002 / 0.009 | 0.018 / 1.053 | 0.174 |

These shared-host observations support keeping workload-specific choices explicit. Dense setup can outweigh its traversal saving for a single call. No automatic density threshold, universal superiority or timing gate is claimed. The record includes compiler/CPU/flags, memory accounting and every sample. Reproduce with `python3 '96-Local Testing/04-Graphs/91-path_mst_benchmark.py'`.

## Scope boundaries and remaining work

No owned P010 implementation or verification gap remains after the 2026-10-07 re-audit, so no continuation handoff is needed. The exact GCC14 runtime check and fractional/custom-weight domains are not claimed; the tested environment and supported integer contracts are stated above. Research candidates left out are listed with reasons in [80-notes.md](80-notes.md#p010-re-audit-omissions).

P010 does not own all-pairs/advanced/implicit/temporal/ranked shortest paths, SCCs, dynamic graphs, directed MSTs, sensitivity/second-best MSTs, spanning-tree counting, or fully dynamic MSTs. These remain explicitly planned or existing-unverified in their current inventory rows. Reconstruction-tree bottleneck queries belong here; general LCA and dynamic-tree APIs retain their separate owners. No automatic judge submissions are part of this package.

## Re-audit — 2026-10-07

Package P010 was re-audited under the current rules, treating the previous verification as existing-unverified. Before any edit, the inventory rows were compared with the code and testers: `01-graph.hpp` lacked `lineGraph`, `inducedSubgraph`, `contract` and `simplify`; `06-mst.hpp` lacked the four maximum-weight engines; `TraversalResult: treeCycle` named an internal helper; `GraphLabels::value` existed but was not listed. The unchanged suites passed full mode with seed 20260927 in all three builds. The 14 confirmed findings in `00-Guidelines/23-reaudit-findings/P010.md` were then fixed or resolved as follows.

### Confirmed findings and their disposition

| # | Finding | Disposition |
|---|---|---|
| 1 | Row lists `TraversalResult: treeCycle`, an internal helper | Removed from the row; documented in [CycleWitness and TraversalResult](#cyclewitness-and-traversalresult) as covered through `dfs`, `findCycle` and `bipartiteCheck`. |
| 2 | `lineGraph`, `inducedSubgraph`, `contract`, `simplify` absent | Implemented with the [contracts above](#subgraph-linegraph-inducedsubgraph-contract-and-simplify) and tested against the four independent oracles the finding names. |
| 3 | Maximum-weight `kruskal`, `primSparse`, `primDense`, `boruvka` absent | `bool maximum = false` on `kruskal`, `primSparse` and `boruvka`; `primDense` follows a new `DenseGraph(g, true)` maximum view, as the finding suggested. `primSparse` keys by `~w`, avoiding `-w` overflow. All checked against `oracle.maximum`. |
| 4 | Evidence claimed GR01 complete while the row was partial | Intro rewritten; both reopened rows are closed by this re-audit. |
| 5 | BFS tie order had no oracle; reversed-adjacency mutant passed | A plain FIFO `std::queue` reference compares `order`, `parent`, `parent_arc` and `root` exactly; the reversed-adjacency and keep-duplicate-sources mutants now fail. |
| 6 | `GraphEdge`/`GraphArc` without a complexity line; method bounds without T/M | The two adjacent one-line aggregates share one `T:/M:` line, the form P006 accepted for adjacent pairs; the `reverse`/`read` bounds moved into the struct line. |
| 7, 12, 14 | Closing-brace rule (`}} }`, `; }}}`, `; }`) | Fixed in all six headers, testers and both benchmarks; `03-consistency.py --braces` reports nothing for the package. |
| 8 | `treeCycle` without a complexity line; T/M lines not directly above | Every declaration now has its complexity line directly above it; contract prose moved to [Contracts](#contracts). |
| 9 | Iterative DFS called the asserting `g[u]` twice per arc | The adjacency is fetched once per frame resume and the scan continues until a child is pushed; the skipped reverse parent arc is computed once per resume. |
| 10 | `relax` asserted inside every algorithm's inner loop | Algorithms call the unchecked `shortest_path_detail::relax`; the public `ShortestPathResult::relax` keeps its asserts (its three probes still fire). |
| 11 | Eight overloads without a complexity comment | `dagLongestPaths` has its own line. Each single-source overload directly follows its multi-source primary and shares its line (bound with k = 1), and `mst_detail::edgeOrder` shares the `edgeLess` line: seven more lines would put `05-shortest_path.hpp` at 10.9% comment lines, over the 8% cap, so the rules conflict and the shared form is used. |
| 13 | `SpanningForest`, `BottleneckResult` without complexity lines | Added. |

### Changes beyond the findings

- Completeness sweep ([81-sources.md](81-sources.md#pages-fetched-on-2026-10-07-p010-re-audit), omissions in [80-notes.md](80-notes.md#p010-re-audit-omissions)) added `TopologicalResult::unique` (Hamiltonian-path test in `toposort_detail::ordered`) and `KruskalReconstruction::leafRange` with the `start`/`leaves` layout.
- Comment cap: every header keeps at most two comment lines per declaration and at most 8% comment lines. `04-dsu.hpp` has 9 non-blank lines, so its one required complexity line is 11%; the validator and `03-cpp.md` now always allow one comment line.
- The six P010 entries compile with `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner, because the P011 header `07-lca.hpp` still has an unused parameter under `NDEBUG`). This exposed `ShortestPathResult::path`'s parameter used only in an assert, now `[[maybe_unused]]`.
- Review fix: `contract`'s bound is O(n + k + m), since the label checks are O(n).
- The removed assert and the DFS frame change are constant-factor only; they were not benchmarked.

### Feature-to-test additions

Covered in the updated [feature-to-test map](#feature-to-test-map-and-execution): `lineGraph`, `inducedSubgraph`, `contract`, `simplify` and the maximum `DenseGraph` view in `01-graph`; the FIFO BFS reference in `02-traversal`; `unique` in `03-toposort`; the maximum engines, `leafRange` and a triangle chain with closed-form totals in `06-mst`. New probes: `induced-negative`, `induced-end`, `induced-duplicate`, `contract-size`, `contract-count`, `contract-negative`, `contract-end`, `dense-maximum`, `leaf-range-negative`, `leaf-range-end`; 71 assertion probes in total, up from 61. Mutation checks (planted in a temporary copy, full mode) killed: line-graph loop double count and self-pairs, simplify keeping the maximum, reversed induced orientation, maximum-view tie order, reversed BFS adjacency, duplicate BFS sources, `unique` off by one, `-w` instead of `~w` keys, Kruskal/primDense/Borůvka ignoring `maximum`, and a wrong second-child leaf offset.

### Independent review — 2026-10-07

`@reviewer` confirmed findings 1–3, 5 and 7–14 fixed and found no correctness defect. It ran 22 mutants of its own: 17 were killed, and the 5 survivors are equivalent or change only runtime (simplify tie on an equal ID, `treeCycle` `>=` vs `>`, an n−1-step Bellman–Ford walk, zero-one BFS `push_front` on weight 1, longest-path tie parent). It raised three items, all resolved:

| Finding | Resolution |
|---|---|
| Feature-to-test map, probe count and GCC 14 statement stale | Map rows rewritten for the current suites; GCC 14.4.1 runs recorded below. |
| Findings 6 and 11 not fully applied (shared lines) | Recorded above as resolved by the shared-line form under the 8% cap. |
| `contract` bound omitted the O(n) label checks | Now `T: O(n + k + m)`. |

### Commands and results

```bash
python3 '96-Local Testing/04-Graphs/<stem>_tester.py' --mode full --seed 20260927        # each stem, before any edit
python3 '96-Local Testing/04-Graphs/<stem>_tester.py' --mode stress --seed 7             # each stem
python3 '96-Local Testing/04-Graphs/<stem>_tester.py' --mode full --seed 20261007        # each stem, after the review fixes
CXX=g++-14 python3 '96-Local Testing/04-Graphs/<stem>_tester.py' --mode full --seed 20261007
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used CPython 3.14 on Linux x86-64 and passed. Before any edit, all six suites passed full mode in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG` with probes) and ASan/UBSan builds. After the changes every P010 build adds `-Wall -Wextra -Wconversion -Werror`. Stress (seed 7) passed all six suites in all three builds, with per-build checks of 157,165,740 graph, 56,673,337 traversal (79,896 cases), 39,310,158 topological (89,130 cases), 9,925,404 DSU, 9,967,673 shortest-path (62,491 cases) and 2,052,048 MST. The final full run (seed 20261007) passed all six suites in all three builds with GCC 16.2.1 and again with GCC 14.4.1 20260915, including all 71 probes; per-build checks were 19,639,546 graph, 6,541,575 traversal, 13,478,426 topological, 6,346,458 DSU, 4,793,422 shortest-path and 872,516 MST. The shared quick runner passed all eleven graph entries from `/tmp`. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage, the workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors, and `--braces` reports nothing for the six headers, six testers and two benchmarks. GCC 14.2 itself (the judge floor) was not run; 14.4.1 is the closest available. No online submission was made.
