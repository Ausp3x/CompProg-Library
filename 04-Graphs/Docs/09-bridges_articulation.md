# 09-bridges_articulation.hpp — Evidence

Owned by package P011 / GR03 (lowlink, strong orientation, streaming bridges, even orientation, local bridges); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions) and [00-notes.md](00-notes.md#p011-gr03-omissions-2026-11-15); GR03 sources are in [00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03).

## Contracts

### LowlinkResult, lowlink, StrongOrientationResult and strongOrientation

The header computes static undirected multigraph lowlink in `O(n + m)` time and memory, output included. Inputs are the verified `Graph` or `CsrGraph`; vertices, edge IDs and arc IDs keep their foundation contracts, and weights are ignored across the full signed 64-bit domain. The graph is never changed, and traversals are iterative. Returned arrays own their storage, while edge and arc references use the input numbering. The number of returned vertex blocks must fit `int`, checked when a block is appended. Result storage is read-only to callers.

| API / fields | Meaning |
|---|---|
| `lowlink(g)` | Complete `LowlinkResult` for every connected component, isolated vertices included. Directed input violates a checked precondition. |
| `tin`, `low`, `parent_arc`, `roots` | DFS discovery permutation; lowest ancestor discovery reachable by a descendant's back edge; entering arc (`-1` at roots); original component roots in increasing minimum-vertex order. |
| `bridges`, `is_bridge` | Logical edge IDs whose deletion increases the number of connected components, in DFS finish order; parallel edges stay distinct. |
| `is_art` | Vertex deletion increases the number of connected components. Neither an isolated vertex nor a loop makes a vertex an articulation point. |
| `cut_components` | `cut_components[u]` is the number of connected components of the graph with `u` and its incident edges deleted. With `c` original components and `k(u)` DFS children `x` of `u` with `low[x] >= tin[u]`, it equals `c - 1 + k(u)` at a DFS root and `c + k(u)` elsewhere; `is_art[u]` is `k(u) > (u is a root)`. The count after deleting edge `e` is the component count plus `is_bridge[e]`, so it needs no field. |
| `component`, `components` | Vertex partition after deleting every bridge (two-edge-connected or edge-biconnected components), singletons included. Component IDs follow increasing minimum vertex. |
| `block_edges`, `block_vertices`, `edge_block` | Vertex-biconnected blocks with edge membership and unique vertex lists; every edge occurs in exactly one block. A bridge is a two-vertex, one-edge block, each loop its own one-vertex block, and a vertex with no incident edge an empty-edge singleton block. Blocks follow DFS emission order. |
| `orientation` | One arc ID per logical edge: DFS tree edges point away from the root, other non-loop edges toward an ancestor, loops take their first encountered arc. |
| `strongOrientation(g)` | A `StrongOrientationResult` with the same per-edge arc choices (`arcs`), the SCC vertex partition (`component`, `groups`) and the minimum attainable number of SCCs (`count`). The orientation is returned even when the graph cannot be strongly connected. |
| `ok`, `bridge`, `disconnected` | Robbins test: connected and bridgeless. The empty graph succeeds vacuously and has zero SCCs. `bridge` is an obstruction edge ID or `-1`; `disconnected` is a pair of vertices in different components or `{-1,-1}`. Both witnesses can be present. |

Vertex blocks overlap at articulation vertices, so they are not a partition. Under the loop convention a vertex may lie in several blocks only because of loops, so callers must use `is_art` rather than count block memberships. An isolated vertex with loops has one block per loop and no empty-edge block. Distinct non-loop blocks share at most one vertex. A connected non-loop graph's bridge-deleted components are strongly orientable even when they contain articulation vertices (two cycles meeting at one vertex).

The result is a static snapshot with no incremental state, shared cache or borrowed adjacency; call again after changing the graph. IDs and member order are deterministic for a fixed adjacency order, but no sorted block or bridge order is promised.

### BridgesStreaming

`BridgesStreaming(n)` reads an edge stream on `n` vertices in O(n) memory, for inputs whose edges do not fit in memory (OI Wiki 双连通分量, LOJ #2788). `add(u, v)` asserts `0 <= u, v < n` and fewer than `INT_MAX` edges, and returns the edge's stream index `0, 1, ...`; loops and parallel edges are allowed. `bridges()` returns the increasing stream indices of the bridges among all edges added so far and may be called between adds; it costs O(n) (a `lowlink` on the kept edges, read in stream order), each `add` O(α(n)) amortized. `kept` (at most `2(n − 1)` edges) and the two DSUs are public state that callers must not mutate.

An edge is kept when it joins two trees of the first forest `F1` or, failing that, of the second forest `F2` built from the edges `F1` rejected; `F1` is a maximal spanning forest of the stream and `F2` one of the rest. An edge in neither is not a bridge, since `F2` already joins its endpoints. For a kept edge `e`, a bridge of the stream is a bridge of the kept subgraph `H`. Conversely, suppose `e` lies on a cycle of the stream but is a bridge of `H`, splitting `H − e` into sides `A` and `B`. Some other stream edge `f` crosses the cut. `f` is not kept, so `F1` and `F2` each join its endpoints by a path, which must cross the cut through `e`; then `e` lies in both forests, impossible since `F2` takes only edges `F1` rejected. So `e` is a bridge of the stream exactly when it is one of `H` (the two-forest certificate of Nishizeki and Poljak).

### evenOutdegreeOrientation

`evenOutdegreeOrientation(g, arcs)` asserts undirected input (loops and parallel edges allowed) and fills `arcs` with one arc ID per edge (`g.arcs[arcs[e]].id == e`; the edge points from that arc's `from`). It returns true exactly when every out-degree is even, which is possible exactly when every connected component has an even number of edges (each edge adds one to one out-degree; a loop adds one to its vertex). Otherwise the orientation still has the fewest odd vertices: exactly the smallest vertex of each odd-edge component. It uses the forest of `dfsForest(g)` (row 02), rooted at each component's smallest vertex; directed input under `NDEBUG` keeps its tree arcs, a meaningless but in-bounds result. Non-tree edges take their first arc, and tree edges are fixed in reverse preorder, children before parents: a child whose out-degree is odd takes its parent edge, otherwise the parent does. Every non-root vertex ends even, and the root's parity is the component's edge count. O(n + m) time and memory (CSES Even Outdegree Edges).

### localBridges

`localBridges(g, weighted = false)` asserts undirected input and, when `weighted`, every edge weight `>= 0`. It returns `{e, span}` in increasing edge ID for every non-loop edge `e = (u, v)` with no third vertex adjacent to both `u` and `v` (no triangle; parallel copies of `u–v` are separate local bridges). `span` (`lll`) is the fewest edges, or with `weighted` the least total weight, of a `u`–`v` path avoiding `e` (other parallel copies may be used), and `-1` when none exists, which is exactly when `e` is a bridge (NetworkX `local_bridges(G, with_span=True, weight)`).

Cost: triangle membership costs O(m · sqrt(m)). Each distinct vertex pair joined by an edge scans the neighbours of its endpoint of smaller (degree, index) once against the marked neighbours of the other, and every parallel copy takes that result; degrees count copies, and at most 2·sqrt(m) vertices have degree above sqrt(m), which bounds the scans between them. Scanning per copy would cost O(m²) on a bundle of parallel edges. Bridges come from `lowlink` and need no search. The other local bridges are handled once per vertex pair, at the copy of least ID: its copies are listed from the endpoint of smaller degree (O(m · sqrt(m)) over all pairs, by the triangle argument), and one BFS (Dijkstra when `weighted`) from `u` skips every `u → v` arc and stops at `v`, in O(n + m) (O((n + m) · log(n + m)), each vertex settled once). A path avoiding `e` that uses another copy is that copy alone, since it enters `v` from `u`, so each copy's span is the least of the search result and the least weight among the other copies (1 unweighted); unweighted, a pair with several copies spans 1 and runs no search. Total O(n + m · sqrt(m) + b · (n + m)) for `b` searched pairs, memory O(n + m); per-copy searches would cost O(k · (n + m)) on a bundle of k copies. Under `NDEBUG` a negative weight still ends within that bound, since no settled vertex is relaxed again.

### Correctness

The explicit DFS stack keeps the next adjacency position of each active vertex and simulates recursive discovery and return exactly. Skipping only the reverse arc of the tree edge keeps a second parallel edge as a back edge. An undirected DFS has no cross edges between finished subtrees. On return from child `u` of `v`, `low[u]` is the minimum discovery index reachable from `u`'s subtree by tree edges and at most one upward non-tree edge. So `(v,u)` is a bridge exactly when `low[u] > tin[v]`, a non-root `v` is an articulation point exactly when some child has `low[u] >= tin[v]`, and a DFS root is one exactly when it has at least two children. Loops affect none of these conditions.

Each non-loop edge enters the edge stack once: on tree discovery or from the descendant endpoint of a back edge. At `low[u] >= tin[v]`, the edges above and including `(v,u)` form exactly one maximal vertex block: a connection from its child-side vertices to an earlier open block avoiding `v` would force `low[u] < tin[v]`, and the open back-edge chains link every popped part without a separating internal vertex. Loops are emitted at once as one-edge blocks and do not disturb the stack; bridge and isolated-vertex singleton blocks complete the convention. A marking array deduplicates each block's vertices without sorting, so output work is linear.

Deleting all bridges and traversing the rest gives the two-edge-connected partition. In the reported orientation, each such component's root reaches all its vertices by downward tree edges, and every non-root vertex reaches an ancestor through the back-edge escape of its entering tree edge, so repeated escapes reach the root and the component is strongly connected. No bridge lies on a directed cycle in any orientation, and different original components cannot communicate, so every orientation has at least one SCC per bridge-deleted component; this construction attains that bound. This also proves Robbins' connected-and-bridgeless criterion and the obstruction witnesses.

Each vertex and arc is scanned a constant number of times, each edge is pushed and popped once, and every block's vertex list is at most twice its edge list except isolated singletons. All bounds are worst-case, output included.

### Legacy Mapping

The originals of the removed adapters (bridge and articulation lowlink, not SCC) stay in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp` and the Team Notebook sources under `OLD`.

- `Tarjan(n, adj)` → `auto low = lowlink(g)` with `g = Graph::fromAdjacency(adj)`: `is_art`, `low` and `tin` (was `t_in`); the bridge pairs are `minmax(g.edges[e].u, g.edges[e].v)` over `e` in `low.bridges`. `adj` must hold exactly `n` lists, because lists past `n` become vertices, and be symmetric as multisets with a loop listed twice; a loop changes no bridge or articulation answer, so dropping it is equally valid. `tin` and `low` keep their meanings, but the DFS may number vertices differently. `vst` (all true) and `timer` (`n`) have no counterpart.
- `BridgeAlgo(n, adj)`, `build(adj)` and `dfs(cur, prv, adj)` → the same call, with the bridge pairs collected in a `set`. Each call returns a fresh result, so nothing needs a reset.
- `lowlink_detail::graph(n, adj)` → `Graph::fromAdjacency(adj)` under the list conditions of the `Tarjan` line; it keeps loops and adds the edges in increasing `(min, max)` order, where the old builder dropped loops and kept list order.
- `StrongOrientation(n, m)`, `addEdge(u, v, id)`, `solve()` (`OLD/Team Notebook/src/algs.cpp:1474`, `algsbetter.cpp:1477`, `test.cpp:315`) → `strongOrientation(g)` with the edges added in ID order: its `{comp_cnt + bridge_cnt, orient}` is `count` and `arcs`, where `orient[id] == '>'` exactly when `g.arcs[arcs[id]].from == g.edges[id].u`.
- `OLD/Team Notebook/src/graph/old_cutpoints-bridges.cpp`: its vertex-block feature is covered by `block_vertices`; isolated vertices, exact parallel-edge parent skipping and stable bridge IDs repair gaps in that reference. The bridge-only `old_bridgealgo.cpp` is covered by `bridges`/`is_bridge`.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/09-bridges_articulation_tester.py` uses the shared contest runner. Exhaustive cases run in increasing graph size, then reproducible random cases and named large shapes. Quick covers all looped simple graphs through three vertices, all looped graphs of multiplicity at most two through two vertices, 100 random multigraphs through seven vertices and 20,000-vertex shapes. Full extends the exhaustive bounds to four and three vertices, uses 1,500 random multigraphs through eight vertices and 200,000-vertex shapes. Stress adds all loopless simple graphs through six vertices, 12,000 random trials and 500,000-vertex shapes. Random trials periodically duplicate an existing edge as a metamorphic fixture. The sanitizer build (`-O1 -g -D_GLIBCXX_ASSERTIONS`) also enables leak detection, `-fno-sanitize-recover=all`, frame pointers and non-PIE linking.

| Feature | Independent verification |
|---|---|
| Bridge flags and IDs | Delete each logical edge and recount components, loops and parallel edges included. |
| Articulation vertices | Delete each vertex and compare the global component count, including roots, isolated vertices and disconnected graphs. |
| Vertex blocks and edge membership | Enumerate every vertex subset, keep maximal subsets that stay connected after each one-vertex deletion, recover all internal non-loop edges, then add the documented loop and isolated blocks. Compare vertex and edge lists and every edge-to-block index. |
| Edge components | Independent traversal after removing the deletion-oracle bridges; labels, complete groups and minimum-vertex order. |
| `lowlink`: DFS diagnostics (`tin`, `low`, `parent_arc`, `roots`) | Discovery permutation, exact root set, acyclic parent arcs, and independently enumerated descendant and back-edge pairs for every lowlink value. |
| `strongOrientation`, `StrongOrientationResult`, `orientation`: Robbins and witnesses | Every selected arc ID validated; transitive closure checks the exact SCC equivalence. Obstruction edges and vertices against independent deletion and component oracles and the minimum-component lower bound. |
| `lowlink` on `Graph::fromAdjacency` lists (the removed `Tarjan`/`BridgeAlgo` cases) | Every enumerated and random case rebuilt from its symmetric lists (loops listed twice): sorted bridge endpoint pairs and articulation flags against the deletion oracles. Repeated calls after adding a parallel edge and then a new edge return fresh bridges. `tin`/`low` are checked independently on the original multigraph: `tin` must be a permutation, the DFS forest is rebuilt from `tin` alone (each vertex's parent is its neighbour with the largest earlier discovery time, a property of every undirected DFS), every non-loop edge must join an ancestor and a descendant, and each `low[u]` must equal the minimum discovery time over `u`'s subtree and the back edges (including extra parallel copies of tree edges) leaving it. |
| `cut_components` | Deletion oracle: BFS component count of the graph without each vertex, for every enumerated and random case (Graph and CSR), plus the 200,000-vertex chain, cycle and parallel star. |
| Boundaries and lifetime | Empty graphs, singleton loops, dyads, parallel parent edges, several cycles meeting at a cutpoint, disconnected isolated vertices, full-range ignored weights, copied and moved results, graph mutation and repeated calls. |
| Scale and stack safety | Long chain, closed chain and parallel-edge star (plus root loop), with exact known block, bridge and articulation expectations. |
| `BridgesStreaming`: `add`, `bridges` | Every small case streamed edge by edge: `add` returns 0, 1, ..., `kept.size() <= 2(n − 1)` after each add, `bridges()` after every prefix against an edge-deletion union-find oracle and twice at the end (Graph and CSR cases). Full: about 260,000 shuffled edges on 200,000 vertices (chain, random forest, short cycles, parallel edges, loops) checked at one third and at the end against a tester-side iterative Tarjan that every small case validates against the deletion oracle; a 300,001-edge dense stream on 1,000 vertices ending in one pendant bridge |
| `evenOutdegreeOrientation` | `arcs` prefilled with junk (every edge gets an arc whatever the forest); size, `g.arcs[arcs[e]].id == e`, result true iff every component's edge count is even, odd out-degree exactly at each odd-edge component's smallest vertex (components by tester BFS, union-find on large cases); the random large graph (Graph and CSR), chain and cycle; triangle and triangle-with-loop fixtures |
| `localBridges` (unweighted and weighted) | Triangles from explicit neighbour sets, spans by 128-bit Bellman–Ford on the graph minus `e`; unweighted (default argument, weights ignored) and weighted with weights 0..3 and near the 64-bit maximum, Graph and CSR; zero-weight square, triangle, parallel copies with a loop and bridge fixtures; a 1000×100 grid with a doubled edge and a loop (weighted spans to 1.2e19) plus a 100,000-vertex chain tail (quick: a 10×100 grid); a 200,000-vertex wheel with a pendant edge for the degree-ranked triangle test; 200,000 parallel copies of `0–1` closing one triangle (one scan per distinct pair); 200,000 copies of `0–1` beside the path `0–2–3–1` with no triangle (exact unweighted and weighted spans, one search per pair) |
| Checked preconditions | Nine death probes: `stream-end`, `even-directed`, `local-directed`, `local-negative` (a negative weight with `weighted`), directed Graph and CSR passed to each free function, and an out-of-range entry in undirected lists through `Graph::fromAdjacency` (`adjacency-endpoint`). Unmirrored lists and a loop listed once are `01-graph_tester` probes. The `INT_MAX` block-count boundary cannot be allocated and is a documented limit, not a runtime fixture. |

## Mutation Testing

Record: mewt 4.0.0 on blob `9f62e1d0e59f2b576ea7bd206ba69c42005353f5` (2026-11-16, seed 1, 43 lines changed since blob `94ecf79b50ad`): 202 mutants, 174 caught (8 by timeout), 28 survived, 0 untested. Stage seconds: quick 419, full on the optimized build 259, full on the checked and ASan-UBSan builds 1032.

The record covers /finish GR03's rewrite of `evenOutdegreeOrientation` and `localBridges`: 7 survivors are runtime only and 21 equivalent. The other survivors come from the whole-file campaign of 2026-11-16 (seed 1, uniform sample of 500 of 574; 442 caught, 12 by timeout, 58 survived), on lines the newer record did not cover. Line numbers are the current header's.

Runtime only, each with identical spans, measured with `-O2 -DNDEBUG` on 2026-11-16 by a scratch driver that is not kept (best of three, identical checksums against the unmutated header):

| Mutant | Line | Work argument and measured ratio |
|---|---|---|
| `COS 552`, `553`, `555`, `556`, the end that lists a pair's copies chosen by `==`, `!=`, `>` or `>=` instead of `<=` | 164 | The copies are the same edges from either end, but listing them from the higher-degree end costs that degree per pair, up to O(m * max degree) in all instead of O(m * sqrt(m)). On a star whose k leaves each hang on two parallel copies, `COS 555` and `556` take 4.4 → 792.2 and 794.4 ms at k = 20000 (about 180×); `COS 553` slows the same on that star and `COS 552` on the star with each pair's leaf end listed first (3.3 → 813.0 ms), the other order showing no change for each. |
| `IT 197`, `COS 581`, the unweighted search run for every pair instead of only single edges | 177 | A pair with two copies has span 1 whatever the search finds, so only the work changes: an O(n + m) search per such pair, beyond the stated `b`. On the same star, 4.4 → 4872.8 ms at k = 20000 (about 1100×) and 1.1 → 296.9 ms at k = 5000. |
| `IF 156`, the weighted search's settled check `if (done[u])` forced false | 172 | A stale queue entry rescans its vertex's arcs, whose relaxations all fail because `dist` is final; up to `deg(u)` stale entries per vertex can exceed the stated O((n + m) * log(n + m)) per search. On a hub relaxed k times with falling keys, holding k pendant leaves and reaching the far end only by a heavy edge, 11.0 → 45.2 ms at k = 300 (4.1×) and 228.9 → 1797.3 ms at k = 1000 (7.9×). |

Equivalent, or accepted by user decision:

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 296`, `361`, `376`, `386`, `426`, `441`, `471`, `481` `<` → `!=` | 30, 57, 61, 65, 108, 120, 137, 140 | Each counter steps by 1 from at most its bound; `group` and `order` only grow, and the index stops at their final size. |
| `COS 303`, `333`, `383`, `463` `!= -1` → `> -1`; `COS 317`, `352`, `367`, `392`, `457` `== -1` → `<= -1` | 31, 47, 62, 127, 40, 51, 58, 68, 126 | The operand is `-1` or an index or nonnegative distance, never below `-1`. |
| `COS 281`, `282`, `306`, the block-count assertions loosened; `COS 421`, `count < INT_MAX` → `!=` | 24, 34, 101 | Unreachable: they bind only at `INT_MAX` blocks or added edges, and the assertions vanish under `NDEBUG`. |
| `CR 89`, `COS 407`, `416`, `417`, `add`'s range assertion removed or loosened | 101 | `DSU::findSet` asserts the same range on both endpoints, so the checked build aborts on the same inputs (the `stream-end` probe), and under `NDEBUG` both assertions vanish, leaving the same code. |
| `CR 107`, `localBridges`' direction assertion removed | 135 | `lowlink(g)` on the next line asserts it first. |
| `BL 226`, `is_art.assign(n, false)` → `true` | 20 | The loop at line 59 assigns every entry. |
| `COS 291`, `marked[v] != b` → `<` | 29 | `marked[v]` is `-1` or an earlier, smaller block. |
| `COS 314`, `pos[u] == size` → `>=` | 38 | `pos[u]` never passes the degree. |
| `LC 213`, `continue` → `break` on popping a root | 40 | A root is the bottom of `path`, so the stack is empty and the loop ends either way. |
| `AS 199`, `200`, swapped `min` arguments | 42, 55 | `min` is symmetric. |
| `COS 357`, `tin[v] < tin[u]` → `<=` | 54 | Equal times mean `v == u`, a loop the branch at line 48 already took. |
| `MR 570`, `571`, `572`, `std::move` removed | 87, 88 | Only a copy is added. |
| `AS 202`, `203`, swapped `uniteSets` arguments | 102 | The returned flag and the partition do not depend on the order, so the kept edges are the same. |
| `AS 204`, swapped `addEdge` endpoints | 106 | The kept graph's edge `e` is the same undirected edge, so `is_bridge[e]` is unchanged. |
| `COS 464`, `rev != -1` → `>= -1` | 127 | Accepted survivor (user decision 2026-11-16). A tree arc of an undirected graph joins two distinct vertices, so `rev` is never `-1` on valid input. The guard matters only for directed input under `NDEBUG`, past the violated precondition at line 115, where the mutant reads `arcs[-1]` out of bounds, so the `NDEBUG` exemption of `05-testing.md` (Mutation Testing) does not cover it. No tester may give an `NDEBUG` build such input (`05-testing.md`, Memory Safety), such guards are checked by review rather than by tests, and no `NDEBUG` probe package is planned; the scratch `NDEBUG` reproducer of the first review (2026-11-15) ran the guarded path in bounds. |
| `COS 469`, `find(...) == end` → `>=` | 129 | `find` returns at most `end`. |
| `COS 489`, `rank(y) >= rank(x)` → `>` | 145 | Ranks pair the degree with the vertex, so they tie only on a loop `y == x`, whose edge line 160 skips; the extra scan of `x`'s own list happens once per `x` (`scanned`), O(m) in all. |
| `COS 491`, `scanned[y] != x` → `<` | 146 | `scanned[y]` is `-1` or an earlier, smaller `x`. |
| `LC 221`, `break` → `continue` | 150 | Scanning the rest of the list finds the same `hit`, within the counted O(min degree) per pair. |
| `COS 509`, `mark[z] == x` → `>=` | 150 | `mark[z]` is `-1` or an `x` not larger than the current one. |
| `COS 450` `!= -1` → `> -1`; `COS 525`, `534`, `549`, `604`, `614`, `629`, `634`, an `== -1`, `!= -1` or `== -2` test loosened to the inequality (newer record) | 118, 154, 155, 162, 184, 185, 188 | The operand is `-1` (`span` also `-2`), an arc index, a distance, or a weight, never below its sentinel: weights are at least 0 (asserted at line 137; under `NDEBUG` past that precondition the loops still end within their bound). |
| `COS 466`, `i >= 0` → `i > 0` (newer record) | 124 | `forest.order[0]` is the first root of `dfsForest`, with no parent arc, which the body skips. |
| `COS 529`, `dist[v] <= d` → `<` (newer record) | 154 | An equal distance keeps `dist`; the unweighted search ignores the flag, and the weighted one pushes one more entry for that arc, which the settled check discards: at most one per arc, within the bound. |
| `IT 188`, `COS 536`, every improved vertex appended to `seen` (newer record) | 155 | Breadth-first distances never improve, so the unweighted `seen` is unchanged; the weighted search appends one entry per improvement, at most one per arc, and uses `seen` only to reset `dist` and `done`. |
| `COS 554`, `<=` → `<` (newer record) | 164 | Only ties change the listing end, and tied ends have the same degree; the copies are the same edges, and `lo`, `hi` and the spans do not depend on their order. |
| `COS 579`, `copies.size() == 1` → `<= 1` (newer record) | 177 | `copies` holds at least `e` itself. |
| `COS 583`, `584`, `i < seen.size()` → `!=` or `<=`; `COS 589`, `dist[t] == -1` → `<= -1` (newer record) | 178 | `i` steps by 1 from 0; a searched single edge is no bridge, so `t` is reached by another path and the loop stops on `dist[t]` before `i` reaches the end. |
| `COS 609`, `619`, `w < lo` or `w < hi` → `<=` (newer record) | 184, 185 | On a tie the two least weights take the same values. |
| `COS 624`, `== lo` → `<= lo` (newer record) | 187 | `lo` is the least copy weight, so no copy weighs less. |
| `AS 222`, swapped `min` arguments (newer record) | 188 | `min` is symmetric. |

## Commands and Results

2026-11-16 (P011 /finish GR03). Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, the runner's optimized, checked and ASan/UBSan builds; GCC 16.2.1 unless stated, beside other suites and a running mutation campaign. No timing benchmark is claimed.

| Command | Result |
|---|---|
| `09-bridges_articulation_tester.py --mode quick` (full preflight) | PASS, 228 cases, 325,299 checks, 9 probes; optimized run 0.14 s, checked 2.71 s; peak 56 MB (build cache warm) |
| `09-bridges_articulation_tester.py --mode full --seed 20261116` | PASS, 3 configurations, 3,650 cases and 3,803,580 checks per build, 9 probes; runs 2.52 s optimized, 42.96 s checked, 37.77 s ASan-UBSan; peak 1436 MB |
| `CXX=g++-14 09-bridges_articulation_tester.py --mode full --seed 20261116` (GCC 14.4.1, floor) | PASS, 3 configurations, same counts; peak 1442 MB |
| `09-bridges_articulation_tester.py --mode stress --seed 20261117` | PASS, 3 configurations, 49,960 cases and 27,225,174 checks per build; runs 12.26 s optimized, 121.46 s checked, 204.80 s ASan-UBSan; peak 1631 MB |
| `05-extraction.py ops --filter 04-Graphs/09-bridges_articulation.hpp` | PASS, 8 operations in 8 distinct extractions on g++ and g++-14 |
| `05-extraction.py testers --filter 09-bridges_articulation_tester` | PASS, 1 quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace; peak 3129 MB |
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

## Sources

The implementation is independent code from the stated invariants; no source body was copied. These references were retrieved and read on 2026-09-27, including their degenerate-input conventions:

- [cp-algorithms, “Strong Orientation”](https://cp-algorithms.com/graph/strong-orientation.html), last update 2024-06-24: Robbins' theorem, DFS orientation, and the extension minimizing the SCC count to original components plus bridges.
- [KACTL, “BiconnectedComponents.h”](https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/BiconnectedComponents.h), Simon Lindholm, dated 2017-04-17, CC0: edge-ID parent exclusion and linear edge-stack decomposition. It omits bridges from callback blocks; this API includes bridge dyads and accounts for loops and isolates.
- [OI Wiki, “双连通分量”](https://oi-wiki.org/graph/bcc/), page reports update 2026-08-17: edge- versus vertex-biconnectivity, bridge-deletion decomposition, lowlink vertex blocks, singleton convention, and the difference and tree-edge coverage formulation. Its sample code drops loops; this API keeps loop edge ownership.

These sources establish the static scope of `lowlink` and `strongOrientation`; streaming bridges are `BridgesStreaming`, and higher or dynamic connectivity belong to other rows (Limits and Handoffs).

Sources for `cut_components` (fetched 2026-10-07) are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

## Limits and Handoffs

Block-cut and bridge-tree queries and two-edge-connectivity augmentation are row 27, incremental bridge maintenance is row 26, and dominators, st-numbering and ear decomposition, triconnectivity/SPQR and three-edge components are rows 24, 60, 61 and 74; static lowlink implies none of them.

Named but not fetched ([00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03)): Nishizeki–Poljak's two-forest certificate and the LOJ #2788 statement behind `BridgesStreaming`, and the CSES Even Outdegree Edges task page; the proofs under Contracts stand alone and the oracles check them. `localBridges` ignores a loop as a common neighbour, which NetworkX counts. The runtime-only measurements (Mutation Testing) used scratch drivers that are not kept; their graphs are described there.

## Aliases

Search aliases (P263, grab --find): BCC (biconnected components), 2-edge-connected components, bridges, articulation points, cut vertices (OI Wiki: 割点和桥); Library Checker `biconnected_components`, `two_edge_connected_components`; 双连通分量, 割边, 点双连通分量, 边双连通分量, 点双, 边双, 关节点, 双连通组件.

Search aliases (P011 GR03, grab --find): streaming bridges, bridges in linear memory, two spanning forests certificate, even out-degree orientation, Even Outdegree Edges, local bridges, edges in no triangle, NetworkX `local_bridges`; 偶数出度定向, 局部桥.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,643 cases, 1,450,632 checks, 9 probes) and integration passed on g++ 16; stress not run.
- 2026-10-07: re-audit, added `LowlinkResult::cut_components`; 3 findings fixed (independent oracle for legacy `t_in`/`low`, lone closing braces, `lowlink_detail::graph` complexity line); full suite, stress and integration passed on g++ 16; hand mutations of `cut_components` failed the quick suites; independent review: all eight package findings confirmed fixed, its own brute-force checks of every addition passed under ASan/UBSan/`_GLIBCXX_DEBUG`.
- 2026-10-12: prune, removed `Tarjan`, `BridgeAlgo` and `lowlink_detail::graph` (cases moved to `lowlink` on `Graph::fromAdjacency` lists, five legacy probes replaced by `adjacency-endpoint`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect, equivalence confirmed on its own 30,000 random forests and multigraphs.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the search aliases; no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-15: P011 GR03, added `BridgesStreaming`, `evenOutdegreeOrientation` and `localBridges` (user decision); independent review: 3 defects fixed (unbounded weighted search on a negative weight under `NDEBUG`, quadratic triangle scan on parallel bundles, out-of-bounds directed input under `NDEBUG`) and the `bridges()` bound corrected to O(n); full (seed 20261115) on g++ and g++-14, stress, extraction and integration passed after the fixes; whole-file mutation campaign (2026-11-16): 56 equivalent, one runtime-only survivor (measured) and one accepted by user decision.
- 2026-11-16: P011 /finish GR03: review fixes: `evenOutdegreeOrientation` reuses `dfsForest` from `02-traversal.hpp`; `localBridges` searches each vertex pair once (copies listed from the lower-degree end, a breadth-first search only for single edges, spans from the two least copy weights) and its bound names the searched pairs; a bare parallel bundle beside a path joined the tests; `COS 464` accepted by user decision. Full (seed 20261116) on g++ and g++-14, stress (seed 20261117), extraction and integration passed; changed-line mutation campaign: 21 equivalent and 7 runtime-only survivors, measured.
