# 09-bridges_articulation.hpp — evidence

Owned by package P011 / GR03 (lowlink, strong orientation, streaming bridges, even orientation, local bridges); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions) and [00-notes.md](00-notes.md#p011-gr03-omissions-2026-11-15); GR03 sources are in [00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03).

## Contracts

### LowlinkResult, lowlink, StrongOrientationResult and strongOrientation

`09-bridges_articulation.hpp` implements static undirected multigraph lowlink in `O(n + m)` time and memory, including output. Inputs are the verified `Graph` or `CsrGraph`; vertices, edge IDs and arc IDs retain their foundation contracts, and weights are ignored across the full signed 64-bit domain. Graph data is never changed. Traversals are iterative. Returned arrays own their storage, while edge/arc references describe the input numbering. The number of returned vertex blocks must fit `int`; this additional bound is checked when a block is appended. Public result storage should be treated as read-only.

| API / fields | Meaning |
|---|---|
| `lowlink(g)` | Complete `LowlinkResult` for every connected component, including isolated vertices. Directed input violates a checked precondition. |
| `tin`, `low`, `parent_arc`, `roots` | DFS discovery permutation, lowest ancestor discovery reachable by a descendant's back edge, entering arc (`-1` at roots), and original connected-component roots in increasing minimum-vertex order. |
| `bridges`, `is_bridge` | Original logical edge IDs whose deletion increases the number of connected components, in DFS finish order; parallel edges remain distinct. |
| `is_art` | Vertex deletion increases the number of connected components. Neither an isolated vertex nor a loop makes a vertex an articulation point. |
| `cut_components` | `cut_components[u]` is the number of connected components of the graph with `u` and its incident edges deleted. With `c` original components and `k(u)` DFS children `x` of `u` with `low[x] >= tin[u]`, it equals `c - 1 + k(u)` at a DFS root and `c + k(u)` elsewhere; `is_art[u]` is `k(u) > (u is a root)`. The count after deleting edge `e` is the component count plus `is_bridge[e]`, so it needs no field. |
| `component`, `components` | Vertex partition after deleting every bridge, including singleton components. Component IDs follow increasing minimum vertex. The partition is also called two-edge-connected / edge-biconnected components. |
| `block_edges`, `block_vertices`, `edge_block` | Vertex-biconnected blocks with edge membership and unique vertex lists; every original edge occurs exactly once. Nonloop maximal blocks use the convention that a bridge is a two-vertex, one-edge block. Each self-loop is its own one-vertex block. A vertex with no incident edges receives an empty-edge singleton block. Blocks follow DFS emission order. |
| `orientation` | One original arc ID for each logical edge: DFS tree edges point away from the root; other nonloop edges point toward an ancestor. Loops choose their first encountered arc. |
| `strongOrientation(g)` | A `StrongOrientationResult` containing the same per-edge arc choices (`arcs`), SCC vertex partition (`component`, `groups`) and minimum attainable number of SCCs (`count`). This orientation is returned even when the whole graph cannot be strongly connected. |
| `ok`, `bridge`, `disconnected` | Robbins test: connected and bridgeless. The empty graph succeeds by explicit vacuous convention and has zero SCCs. `bridge` is an obstruction edge ID or `-1`; `disconnected` is a pair of vertices in different original components or `{-1,-1}`. Both witnesses can be present. |

Vertex blocks overlap at articulation vertices, so they are not a vertex partition. Under the stated loop convention, a vertex may occur in multiple blocks solely because of loops; clients must use `is_art` and cannot infer articulation by counting block memberships. An isolated vertex with loops has one block per loop and no additional empty-edge block. Distinct nonloop blocks overlap at at most one vertex. An ordinary connected nonloop graph's bridge-deleted components are strongly orientable, although they may contain articulation vertices (for example two cycles meeting at one vertex).

The result is a static snapshot. Reinvoke either free function after changing the graph; there is no incremental state, shared cache or borrowed adjacency. ID values and member order are deterministic for the same input adjacency order, but no sorted block/bridge order is promised.

### BridgesStreaming

`BridgesStreaming(n)` reads an edge stream on `n` vertices in O(n) memory, for inputs whose edges do not fit in memory (OI Wiki 双连通分量, LOJ #2788). `add(u, v)` asserts `0 <= u, v < n` and fewer than `INT_MAX` edges, and returns the edge's stream index `0, 1, ...`; loops and parallel edges are allowed. `bridges()` returns the increasing stream indices of the bridges of every edge added so far and may be called between adds; it costs O(n) (a `lowlink` on the kept edges, read in stream order), each `add` O(α(n)) amortized. `kept` (at most `2(n − 1)` edges) and the two DSUs are public state, not to be mutated.

An edge is kept when it joins two trees of the first forest `F1` or, failing that, of the second forest `F2` built from the edges `F1` rejected; `F1` is a maximal spanning forest of the stream and `F2` one of the rest. An edge in neither is not a bridge, since `F2` already joins its endpoints. For a kept edge `e`: a bridge of the stream is a bridge of the kept subgraph `H`. Conversely, suppose `e` lies on a cycle of the stream but is a bridge of `H`, splitting `H − e` into sides `A` and `B`. Some other stream edge `f` crosses the cut. `f` is not kept, so `F1` and `F2` each join its endpoints by a path, which must cross the cut through `e`; then `e` lies in both forests, impossible since `F2` takes only edges `F1` rejected. So `e` is a bridge of the stream exactly when it is one of `H` (the two-forest certificate of Nishizeki and Poljak).

### evenOutdegreeOrientation

`evenOutdegreeOrientation(g, arcs)` asserts undirected input (loops and parallel edges allowed) and fills `arcs` with one arc ID per edge (`g.arcs[arcs[e]].id == e`; the edge points from that arc's `from`). It returns true exactly when every out-degree is even, which is possible exactly when every connected component has an even number of edges (each edge adds one to one out-degree; a loop adds one to its vertex). Otherwise the orientation still has the fewest odd vertices: exactly the smallest vertex of each odd-edge component, and every other vertex even. The forest of `dfsForest(g)` (row 02), rooted at each component's smallest vertex, is used (directed input under `NDEBUG` keeps its tree arcs, a meaningless but in-bounds result), non-tree edges are oriented by their first arc, and tree edges are fixed in reverse preorder, children before parents: a child whose out-degree is odd takes its parent edge, otherwise the parent does. Every non-root vertex ends even, and the root's parity is the component's edge count. O(n + m) time and memory (CSES Even Outdegree Edges).

### localBridges

`localBridges(g, weighted = false)` asserts undirected input and, when `weighted`, every edge weight `>= 0`. It returns `{e, span}` in increasing edge ID for every non-loop edge `e = (u, v)` with no third vertex adjacent to both `u` and `v` (no triangle; parallel copies of `u–v` are separate local bridges). `span` (`lll`) is the fewest edges, or with `weighted` the least total weight, of a `u`–`v` path avoiding `e` (other parallel copies may be used), and `-1` when none exists, which is exactly when `e` is a bridge. NetworkX `local_bridges(G, with_span=True, weight)`. Triangle membership costs O(m · sqrt(m)): each distinct vertex pair joined by an edge scans the neighbours of its endpoint of smaller (degree, index) once against the marked neighbours of the other, and every parallel copy takes that result (degrees count copies; at most 2·sqrt(m) vertices have degree above sqrt(m), which bounds the scans between them). Scanning per copy instead would cost O(m²) on a bundle of parallel edges. Bridges come from `lowlink` and need no search. The other local bridges are handled once per vertex pair, at the copy of least ID: its copies are listed from the endpoint of smaller degree (at most O(m · sqrt(m)) over all pairs, by the triangle argument), and one BFS (Dijkstra when `weighted`) from `u` skips every `u → v` arc and stops at `v`, O(n + m) (O((n + m) · log(n + m)), each vertex settled once). A path avoiding `e` that uses another copy is that copy alone, since it enters `v` from `u`, so each copy's span is the least of the search result and the least weight among the other copies (1 unweighted). Unweighted, a pair with several copies spans 1 and runs no search. Total O(n + m · sqrt(m) + b · (n + m)) for `b` searched pairs, memory O(n + m); per-copy searches cost O(k · (n + m)) on a bundle of k copies. Under `NDEBUG` a negative weight still ends within that bound, since no settled vertex is relaxed again.

### Correctness

The explicit DFS stack retains the next adjacency position of each active vertex and exactly simulates recursive discovery and return events. Skipping only the reverse arc of the tree edge preserves a second parallel edge as a back edge. An undirected DFS has no cross edges between distinct finished subtrees. On return from child `u` of `v`, `low[u]` is the minimum discovery index reachable from `u`'s subtree using tree edges and at most one upward non-tree edge. Thus `(v,u)` is a bridge exactly when `low[u] > tin[v]`, and a nonroot `v` is an articulation point exactly when some child has `low[u] >= tin[v]`. A DFS root is an articulation point exactly when it has at least two children. Loops affect none of these conditions.

Each nonloop edge enters the edge stack once: on tree discovery or from the descendant endpoint of a back edge. At `low[u] >= tin[v]`, the edges above and including `(v,u)` form exactly one maximal vertex block. A connection from its child-side vertices to an earlier open block avoiding `v` would force `low[u] < tin[v]`; conversely the still-open back-edge chains link every popped portion without a separating internal vertex. Immediate one-edge treatment of loops cannot disturb the pending stack. Singleton bridge and isolated-vertex blocks complete the output convention. The marking array deduplicates vertices within each emitted block without sorting, so total output work is linear.

Deleting all bridges and traversing the remaining graph gives the two-edge-connected vertex partition. In the reported orientation, the root of each such component reaches all its vertices by downward tree edges. Every nonroot vertex can reach an ancestor across its entering tree edge because that edge has a back-edge escape; repeatedly taking escapes reaches the component root. Each component is therefore strongly connected. No bridge can belong to a directed cycle, regardless of its orientation, and disconnected original components cannot communicate. Every orientation consequently has at least one SCC per bridge-deleted component; this construction attains that lower bound. This also proves Robbins' connected-and-bridgeless criterion and the obstruction witnesses.

Each vertex/arc is scanned a constant number of times, each edge is pushed/popped once, and every block's vertex list is bounded by twice its edge list except isolated singletons. All bounds are worst-case, including output. The implementation uses ordinary contest-profile containers; no new arithmetic acceleration or performance-comparison claim is involved.

### Legacy mapping

The originals of the removed adapters (bridge/articulation lowlink, not SCC) stay in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp` and the Team Notebook sources under `OLD`.

- `Tarjan(n, adj)` → `auto low = lowlink(g)` with `g = Graph::fromAdjacency(adj)`: `is_art`, `low` and `tin` (was `t_in`); the bridge pairs are `minmax(g.edges[e].u, g.edges[e].v)` over `e` in `low.bridges`. `adj` must hold exactly `n` lists, because lists past `n` become vertices, and be symmetric as multisets with a loop listed twice; a loop changes no bridge or articulation answer, so dropping it is equally valid. `tin` and `low` keep their meanings, but the DFS may number vertices differently. `vst` (all true) and `timer` (`n`) have no counterpart.
- `BridgeAlgo(n, adj)`, `build(adj)` and `dfs(cur, prv, adj)` → the same call, with the bridge pairs collected in a `set`. Each call returns a fresh result, so nothing needs a reset.
- `lowlink_detail::graph(n, adj)` → `Graph::fromAdjacency(adj)` under the list conditions of the `Tarjan` line; it keeps loops and adds the edges in increasing `(min, max)` order, where the old builder dropped loops and kept list order.
- `StrongOrientation(n, m)`, `addEdge(u, v, id)`, `solve()` (`OLD/Team Notebook/src/algs.cpp:1474`, `algsbetter.cpp:1477`, `test.cpp:315`) → `strongOrientation(g)` with the edges added in ID order: its `{comp_cnt + bridge_cnt, orient}` is `count` and `arcs`, where `orient[id] == '>'` exactly when `g.arcs[arcs[id]].from == g.edges[id].u`.
- `OLD/Team Notebook/src/graph/old_cutpoints-bridges.cpp`: its vertex-block feature is covered by `block_vertices`; isolated vertices, exact parallel-edge parent skipping and stable bridge IDs repair gaps in that reference. The bridge-only `old_bridgealgo.cpp` is covered by `bridges`/`is_bridge`.

## Feature-to-test map

`96-Local Testing/04-Graphs/09-bridges_articulation_tester.py` uses the shared contest runner. Exhaustive cases run in increasing graph size, followed by reproducible random cases and named large shapes.

| Feature | Independent verification |
|---|---|
| Bridge flags/IDs | Delete each logical edge and recount components, including loops/parallel edges. |
| Articulation vertices | Delete each vertex and compare the global component count, including roots, isolated vertices and disconnected graphs. |
| Vertex blocks / edge membership | Enumerate every vertex subset, retain maximal subsets that remain connected after each one-vertex deletion, recover all internal nonloop edges, then add the documented loop and isolated blocks. Compare both vertex/edge lists and every edge-to-block index. |
| Edge components | Independent traversal after removing deletion-oracle bridges; verify labels, complete groups and minimum-vertex order. |
| `lowlink`: DFS diagnostics (`tin`, `low`, `parent_arc`, `roots`) | Discovery permutation, exact root set, acyclic input parent arcs, and independently enumerate descendant/back-edge pairs to check every lowlink value. |
| `strongOrientation`, `StrongOrientationResult`, `orientation`: Robbins / witnesses | Validate every selected original arc ID; transitive closure verifies the exact SCC equivalence relation. Check obstruction edges/vertices against independent deletion/component oracles and the minimum-component lower bound. |
| `lowlink` on `Graph::fromAdjacency` lists (the removed `Tarjan`/`BridgeAlgo` cases) | Every enumerated and random case is rebuilt from its symmetric lists (loops listed twice): sorted bridge endpoint pairs and articulation flags against the deletion oracles. Repeated calls after adding a parallel edge and then a new edge return fresh bridges. `tin`/`low` are checked independently on the original multigraph: `tin` must be a permutation, the DFS forest is reconstructed from `tin` alone (each vertex's parent is its neighbor with the largest earlier discovery time, a property of every undirected DFS), every nonloop edge must join an ancestor and a descendant, and each `low[u]` must equal the minimum discovery time over `u`'s subtree and the back edges (including extra parallel copies of tree edges) leaving it. |
| `cut_components` | Deletion oracle: BFS component count of the graph without each vertex, for every enumerated and random case (Graph and CSR), plus the 200,000-vertex chain, cycle and parallel star. |
| Boundaries / lifetime | Empty graphs, singleton loops, dyads, parallel parent edges, multiple cycles meeting at a cutpoint, disconnected isolated vertices, full-range ignored weights, copied/moved owning results, graph mutation and repeated calls. |
| Scale / stack safety | Long chain, closed chain and parallel-edge star (plus root loop), with exact known block/bridge/articulation expectations. |
| `BridgesStreaming`: `add`, `bridges` | Every small case streamed edge by edge: `add` returns 0, 1, ..., `kept.size() <= 2(n − 1)` after each add, `bridges()` after every prefix against an edge-deletion union-find oracle and twice at the end (Graph and CSR cases). Full: about 260,000 shuffled edges on 200,000 vertices (chain, random forest, short cycles, parallel edges, loops) checked at one third and at the end against a tester-side iterative Tarjan that every small case validates against the deletion oracle; a 300,001-edge dense stream on 1,000 vertices ending in one pendant bridge |
| `evenOutdegreeOrientation` | `arcs` prefilled with junk (every edge gets an arc whatever the forest); size, `g.arcs[arcs[e]].id == e`, result true iff every component's edge count is even, odd out-degree exactly at each odd-edge component's smallest vertex (components by tester BFS, union-find on large cases); the random large graph (Graph and CSR), chain and cycle; triangle and triangle-with-loop fixtures |
| `localBridges` (unweighted and weighted) | Triangles from explicit neighbour sets, spans by 128-bit Bellman–Ford on the graph minus `e`; unweighted (default argument, weights ignored) and weighted with weights 0..3 and near the 64-bit maximum, Graph and CSR; zero-weight square, triangle, parallel copies with a loop and bridge fixtures; a 1000×100 grid with a doubled edge and a loop (weighted spans to 1.2e19) plus a 100,000-vertex chain tail (quick: a 10×100 grid), and a 200,000-vertex wheel with a pendant edge for the degree-ranked triangle test, and 200,000 parallel copies of `0–1` closing one triangle (one scan per distinct pair; per-copy scans were quadratic), and 200,000 copies of `0–1` beside the path `0–2–3–1` with no triangle (exact unweighted and weighted spans; one search per pair, per-copy searches were quadratic) |
| Checked preconditions | Nine death probes: `stream-end`, `even-directed`, `local-directed`, `local-negative` (a negative weight with `weighted`), and directed Graph/CSR passed to each free function, and an out-of-range entry in undirected lists through `Graph::fromAdjacency` (`adjacency-endpoint`). Unmirrored lists and a loop listed once are `01-graph_tester` probes. The infeasible-to-allocate `INT_MAX` block-count boundary is a documented limit rather than a claimed runtime fixture. |

Quick covers all looped simple graphs through three vertices, all multiplicity-at-most-two looped graphs through two vertices, 100 random multigraphs through seven vertices and 20,000-vertex shapes. Full extends exhaustive bounds to four and three vertices respectively, uses 1,500 random multigraphs through eight vertices, and 200,000-vertex shapes. Stress adds all loopless simple graphs through six vertices, 12,000 random trials and 500,000-vertex shapes. Random trials periodically duplicate an existing edge as a separate metamorphic fixture.

The sanitizer configuration (`-O1 -g -D_GLIBCXX_ASSERTIONS`) also enables leak detection, `-fno-sanitize-recover=all`, frame pointers and non-PIE linking. Hand mutations of `cut_components` fail the quick suites.

## Commands and results

Latest run 2026-11-16 (P011 /finish GR03; experiment: none). Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, the runner's optimized, checked and ASan/UBSan builds; GCC 16.2.1 unless stated, beside other suites and a running mutation campaign:

| Command | Result |
|---|---|
| `09-bridges_articulation_tester.py --mode quick` (full pre-flight) | PASS, 228 cases, 325,299 checks, 9 probes; optimized run 0.14 s, checked 2.71 s; MEMORY peak 56 MB (build cache warm) |
| `09-bridges_articulation_tester.py --mode full --seed 20261116` | PASS, 3 configurations, 3,650 cases and 3,803,580 checks per build, 9 probes; runs 2.52 s optimized, 42.96 s checked, 37.77 s ASan-UBSan; MEMORY peak 1436 MB |
| `CXX=g++-14 09-bridges_articulation_tester.py --mode full --seed 20261116` (GCC 14.4.1, floor) | PASS, 3 configurations, same counts; MEMORY peak 1442 MB |
| `09-bridges_articulation_tester.py --mode stress --seed 20261117` | PASS, 3 configurations, 49,960 cases and 27,225,174 checks per build; runs 12.26 s optimized, 121.46 s checked, 204.80 s ASan-UBSan; MEMORY peak 1631 MB |
| `05-extraction.py ops --filter 04-Graphs/09-bridges_articulation.hpp` | PASS, 8 operations in 8 distinct extractions on g++ and g++-14 |
| `05-extraction.py testers --filter 09-bridges_articulation_tester` | PASS, 1 quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar and available AVX2 multi-TU, workspace; MEMORY peak 3129 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/09-bridges_articulation_tester.py' --mode full --seed 20261116
CXX=g++-14 python3 '96-Local Testing/04-Graphs/09-bridges_articulation_tester.py' --mode full --seed 20261116
python3 '96-Local Testing/04-Graphs/09-bridges_articulation_tester.py' --mode stress --seed 20261117
python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/09-bridges_articulation.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 09-bridges_articulation_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/09-bridges_articulation.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

No timing benchmark is claimed.

## Sources

The implementation is independent code from the stated invariants; no external source body was copied. These references were retrieved and read on 2026-09-27, including differences in degenerate-input conventions:

- [cp-algorithms, “Strong Orientation”](https://cp-algorithms.com/graph/strong-orientation.html), last update 2024-06-24: Robbins' theorem, DFS orientation, and the extension minimizing SCC count to original components plus bridges.
- [KACTL, “BiconnectedComponents.h”](https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/BiconnectedComponents.h), Simon Lindholm, dated 2017-04-17, CC0: edge-ID parent exclusion and linear edge-stack decomposition. Its convention omits bridges from callback blocks; this API explicitly includes bridge dyads and accounts for loops and isolates.
- [OI Wiki, “双连通分量”](https://oi-wiki.org/graph/bcc/), page reports update 2026-08-17: edge- versus vertex-biconnectivity, bridge deletion decomposition, lowlink vertex blocks, singleton convention, and the alternative difference/tree-edge coverage formulation. Its sample code drops loops; this API preserves loop edge ownership explicitly.

These sources establish the static decomposition/orientation scope of `lowlink` and `strongOrientation`; streaming bridges are `BridgesStreaming`, and higher or dynamic connectivity are other rows (Limits and handoffs).

Sources for `cut_components` (fetched 2026-10-07) are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

## Limits and handoffs

Block-cut and bridge-tree queries and two-edge-connectivity augmentation are row 27, incremental bridge maintenance is row 26, and dominators, st-numbering/ear decomposition, triconnectivity/SPQR and three-edge components are rows 24, 60, 61 and 74; none is implied by static lowlink.

Named but not fetched ([00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03)): Nishizeki–Poljak's two-forest certificate and the LOJ #2788 statement behind `BridgesStreaming`, and the CSES Even Outdegree Edges task page; the proofs under Contracts stand alone and the oracles check them. `localBridges` ignores a loop as a common neighbour, which NetworkX counts. The runtime-only measurements (Mutation testing) used scratch drivers that are not kept; their graphs are described there.

## Aliases

Search aliases (P263, grab --find): BCC (biconnected components), 2-edge-connected components, bridges, articulation points, cut vertices (OI Wiki: 割点和桥); Library Checker `biconnected_components`, `two_edge_connected_components`; 双连通分量, 割边, 点双连通分量, 边双连通分量, 点双, 边双, 关节点, 双连通组件.

Search aliases (P011 GR03, grab --find): streaming bridges, bridges in linear memory, two spanning forests certificate, even out-degree orientation, Even Outdegree Edges, local bridges, edges in no triangle, NetworkX `local_bridges`; 偶数出度定向, 局部桥.

## Mutation testing

Record: mewt 4.0.0 on blob `9f62e1d0e59f2b576ea7bd206ba69c42005353f5` (2026-11-16, seed 1, 43 lines changed since blob `94ecf79b50ad`): 202 mutants, 174 caught (8 by timeout), 28 survived, 0 untested.

Stage seconds: quick 419, full on the optimized build 259, full on the checked and ASan-UBSan builds 1032. Seven survivors are runtime only and 21 are equivalent (line numbers of that blob).

Runtime only, each with identical spans and measured with `-O2 -DNDEBUG` on 2026-11-16 by a scratch driver (best of three, identical checksums against the unmutated header):
- `COS 552`, `553`, `555`, `556` (line 164), the end that lists a pair's copies chosen by `==`, `!=`, `>` or `>=` instead of `<=`: the copies are the same edges from either end, but listing them from the higher-degree end costs that degree per pair, up to O(m * max degree) in all instead of the O(m * sqrt(m)) the minimum allows. On a star whose k leaves each hang on two parallel copies, `COS 555` and `556` take 4.4 → 792.2 and 794.4 ms at k = 20000 (about 180×); `COS 553` slows the same on that star and `COS 552` on the star with each pair's leaf end listed first (3.3 → 813.0 ms), the other order showing no change for each.
- `IT 197`, `COS 581` (line 177), the unweighted search run for every pair instead of only single edges: a pair with two copies has span 1 whatever the search finds, so only the work changes, an O(n + m) search per such pair, beyond the stated `b`. On the same star, 4.4 → 4872.8 ms at k = 20000 (about 1100×) and 1.1 → 296.9 ms at k = 5000.
- `IF 156` (line 172), the weighted search's settled check `if (done[u])` forced false: a stale queue entry rescans its vertex's arcs, whose relaxations all fail because `dist` is already final; up to `deg(u)` stale entries per vertex can exceed the stated O((n + m) * log(n + m)) per search. On a hub relaxed k times with falling keys, holding k pendant leaves and reaching the far end only by a heavy edge, 11.0 → 45.2 ms at k = 300 (4.1×) and 228.9 → 1797.3 ms at k = 1000 (7.9×).

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 450` `!= -1` → `> -1`; `COS 525`, `534`, `549`, `604`, `614`, `629`, `634`, an `== -1`, `!= -1` or `== -2` test loosened to the inequality | 118, 154, 155, 162, 184, 185, 188 | The operand is `-1` (`span` also `-2`), an arc index, a distance, or a weight, never below its sentinel: weights are at least 0 (asserted at line 137; under `NDEBUG` past that precondition the loops still end within their bound). |
| `COS 466`, `i >= 0` → `i > 0` | 124 | `forest.order[0]` is the first root of `dfsForest`, with no parent arc, which the body skips. |
| `COS 529`, `dist[v] <= d` → `<` | 154 | An equal distance keeps `dist`; the unweighted search ignores the flag, and the weighted one pushes one more entry for that arc, which the settled check discards: at most one per arc, within the bound. |
| `IT 188`, `COS 536`, every improved vertex appended to `seen` | 155 | Breadth-first distances never improve, so the unweighted `seen` is unchanged; the weighted search appends one entry per improvement, at most one per arc, and uses `seen` only to reset `dist` and `done`. |
| `COS 554`, `<=` → `<` | 164 | Only ties change the listing end, and tied ends have the same degree; the copies are the same edges, and `lo`, `hi` and the spans do not depend on their order. |
| `COS 579`, `copies.size() == 1` → `<= 1` | 177 | `copies` holds at least `e` itself. |
| `COS 583`, `584`, `i < seen.size()` → `!=` or `<=`; `COS 589`, `dist[t] == -1` → `<= -1` | 178 | `i` steps by 1 from 0; a searched single edge is no bridge, so `t` is reached by another path and the loop stops on `dist[t]` before `i` reaches the end. |
| `COS 609`, `619`, `w < lo` or `w < hi` → `<=` | 184, 185 | On a tie the two least weights take the same values. |
| `COS 624`, `== lo` → `<= lo` | 187 | `lo` is the least copy weight, so no copy weighs less. |
| `AS 222`, swapped `min` arguments | 188 | `min` is symmetric. |

Earlier whole-file campaign: mewt 4.0.0 on blob `94ecf79b50adee7685d5b15015c558d483ed946a` (2026-11-16, seed 1, whole file, uniform sample of 574): 500 mutants, 442 caught (12 by timeout), 58 survived, 0 untested. Stage seconds: quick 1082, full on the optimized build 390, full on the checked and ASan-UBSan builds 2431. One survivor is runtime only, `COS 464` is accepted by user decision, and 56 are equivalent (line numbers of that blob).

Its runtime-only `IF 153` (line 173) is `IF 156` above, measured there again: 12.3 → 50.0 ms at k = 300 and 283.4 → 1860.2 ms at k = 1000 on that blob.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 296`, `361`, `376`, `386`, `426`, `431`, `436`, `441`, `471`, `481`, `546` `<` → `!=` | 29, 56, 60, 64, 107, 117, 120, 125, 142, 145, 179 | Each counter steps by 1 from at most its bound; `group`, `order` and `seen` only grow, and the index stops at their final size. |
| `COS 303`, `333`, `383`, `463`, `523` `!= -1` → `> -1`; `COS 317`, `352`, `367`, `392`, `457`, `532`, `552` `== -1` → `<= -1` | 30, 46, 61, 132, 164, 39, 50, 57, 67, 131, 165, 179 | The operand is `-1` or an index or nonnegative distance, never below `-1`. |
| `COS 281`, `282`, `306`, the block-count assertions loosened; `COS 421`, `count < INT_MAX` → `!=` | 23, 33, 100 | Unreachable: they bind only at `INT_MAX` blocks or added edges, and the assertions vanish under `NDEBUG`. |
| `CR 89`, `COS 407`, `416`, `417`, `add`'s range assertion removed or loosened | 100 | `DSU::findSet` asserts the same range on both endpoints, so the checked build aborts on the same inputs (the `stream-end` probe), and under `NDEBUG` both assertions vanish, leaving the same code. |
| `CR 107`, `localBridges`' direction assertion removed | 140 | `lowlink(g)` on the next line asserts it first. |
| `BL 226`, `is_art.assign(n, false)` → `true` | 19 | The loop at line 58 assigns every entry. |
| `COS 291`, `marked[v] != b` → `<` | 28 | `marked[v]` is `-1` or an earlier, smaller block. |
| `COS 314`, `pos[u] == size` → `>=` | 37 | `pos[u]` never passes the degree. |
| `LC 213`, `continue` → `break` on popping a root | 39 | A root is the bottom of `path`, so the stack is empty and the loop ends either way. |
| `AS 199`, `200`, swapped `min` arguments | 41, 54 | `min` is symmetric. |
| `COS 357`, `tin[v] < tin[u]` → `<=` | 53 | Equal times mean `v == u`, a loop the branch at line 47 already took. |
| `MR 570`, `571`, `572`, `std::move` removed | 86, 87 | Only a copy is added. |
| `AS 202`, `203`, swapped `uniteSets` arguments | 101 | The returned flag and the partition do not depend on the order, so the kept edges are the same. |
| `AS 204`, swapped `addEdge` endpoints | 105 | The kept graph's edge `e` is the same undirected edge, so `is_bridge[e]` is unchanged. |
| `COS 454`, `i >= 0` → `i > 0` | 129 | `order[0]` is vertex 0, a BFS root with no parent arc, which the body skips. |
| `COS 464`, `rev != -1` → `>= -1` | 132 | A tree arc of an undirected graph joins two distinct vertices, so `rev` is never `-1` on valid input. The guard matters only for directed input under `NDEBUG`, past the violated precondition at line 114, where the mutant reads `arcs[-1]` out of bounds, so the `NDEBUG` exemption of `05-testing.md` does not cover it. Accepted survivor (user decision 2026-11-16): no tester may give an `NDEBUG` build such input (`05-testing.md`, Memory safety), such guards are checked by review rather than by tests, and no `NDEBUG` probe package is planned; the scratch `NDEBUG` reproducer of the first review (2026-11-15) ran the guarded path in bounds. |
| `COS 469`, `find(...) == end` → `>=` | 134 | `find` returns at most `end`. |
| `COS 489`, `rank(y) >= rank(x)` → `>` | 150 | Ranks pair the degree with the vertex, so they tie only on a loop `y == x`, whose edge line 161 skips; the extra scan of `x`'s own list happens once per `x` (`scanned`), O(m) in all. |
| `COS 491`, `scanned[y] != x` → `<` | 151 | `scanned[y]` is `-1` or an earlier, smaller `x`. |
| `LC 221`, `break` → `continue` | 155 | Scanning the rest of the list finds the same `hit`, within the counted O(min degree) per pair. |
| `COS 509`, `mark[z] == x` → `>=` | 155 | `mark[z]` is `-1` or an `x` not larger than the current one. |
| `BL 233`, `COS 527`, an equal or worse distance reported as improved | 164 | `dist` keeps its value; the unweighted search ignores the flag, and the weighted one pushes one more entry for that arc, which the settled check discards: at most one per arc, within the bound. |
| `IT 185`, `COS 534`, every reached vertex appended to `seen` | 165 | Breadth-first distances never improve, so the unweighted `seen` is unchanged; the weighted search appends one entry per improvement, at most one per arc, and uses `seen` only to reset `dist` and `done`. |
| `COS 547`, `i < seen.size()` → `<=` | 179 | A searched edge is no bridge, so `t` is reached by another path and the loop stops on `dist[t]` before `i` reaches the end. |

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,643 cases, 1,450,632 checks, 9 probes) and integration passed on g++ 16; stress not run.
- 2026-10-07: re-audit, added `LowlinkResult::cut_components`; 3 findings fixed (independent oracle for legacy `t_in`/`low`, lone closing braces, `lowlink_detail::graph` complexity line); full suite, stress and integration passed on g++ 16; independent review: all eight package findings confirmed fixed, its own brute-force checks of every addition passed under ASan/UBSan/`_GLIBCXX_DEBUG`.
- 2026-10-12: prune, removed `Tarjan`, `BridgeAlgo` and `lowlink_detail::graph` (cases moved to `lowlink` on `Graph::fromAdjacency` lists, five legacy probes replaced by `adjacency-endpoint`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect, equivalence confirmed on its own 30,000 random forests and multigraphs.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-15: P011 GR03, added `BridgesStreaming`, `evenOutdegreeOrientation` and `localBridges` (user decision); independent review: 3 defects fixed (unbounded weighted search on a negative weight under `NDEBUG`, quadratic triangle scan on parallel bundles, out-of-bounds directed input under `NDEBUG`) and the `bridges()` bound corrected to O(n); full (seed 20261115) on g++ and g++-14, stress, extraction and integration passed after the fixes; whole-file mutation campaign (2026-11-16): 56 equivalents, one runtime-only survivor (measured) and one accepted by user decision (below).
- 2026-11-16: P011 /finish GR03: review fixes: `evenOutdegreeOrientation` reuses `dfsForest` from `02-traversal.hpp`; `localBridges` searches each vertex pair once (copies listed from the lower-degree end, a breadth-first search only for single edges, spans from the two least copy weights) and its bound names the searched pairs; a bare parallel bundle beside a path joined the tests; `COS 464` accepted by user decision. Full (seed 20261116) on g++ and g++-14, stress (seed 20261117), extraction and integration passed; changed-line mutation campaign: 21 equivalents and 7 runtime-only survivors, measured (above).
