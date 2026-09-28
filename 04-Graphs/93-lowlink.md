# Lowlink and strong orientation — P011 / GR03

## Scope and contracts

`09-bridges_articulation.hpp` implements static undirected multigraph lowlink in `O(n + m)` time and memory, including output. Inputs are the verified `Graph` or `CsrGraph`; vertices, edge IDs and arc IDs retain their foundation contracts, and weights are ignored across the full signed 64-bit domain. Graph data is never changed. Traversals are iterative. Returned arrays own their storage, while edge/arc references describe the input numbering. The number of returned vertex blocks must fit `int`; this additional bound is checked when a block is appended. Public result storage should be treated as read-only.

| API / fields | Meaning |
|---|---|
| `lowlink(g)` | Complete `LowlinkResult` for every connected component, including isolated vertices. Directed input violates a checked precondition. |
| `tin`, `low`, `parent_arc`, `roots` | DFS discovery permutation, lowest ancestor discovery reachable by a descendant's back edge, entering arc (`-1` at roots), and original connected-component roots in increasing minimum-vertex order. |
| `bridges`, `is_bridge` | Original logical edge IDs whose deletion increases the number of connected components, in DFS finish order; parallel edges remain distinct. |
| `is_art` | Vertex deletion increases the number of connected components. Neither an isolated vertex nor a loop makes a vertex an articulation point. |
| `component`, `components` | Vertex partition after deleting every bridge, including singleton components. Component IDs follow increasing minimum vertex. The partition is also called two-edge-connected / edge-biconnected components. |
| `block_edges`, `block_vertices`, `edge_block` | Vertex-biconnected blocks with edge membership and unique vertex lists; every original edge occurs exactly once. Nonloop maximal blocks use the convention that a bridge is a two-vertex, one-edge block. Each self-loop is its own one-vertex block. A vertex with no incident edges receives an empty-edge singleton block. Blocks follow DFS emission order. |
| `orientation` | One original arc ID for each logical edge: DFS tree edges point away from the root; other nonloop edges point toward an ancestor. Loops choose their first encountered arc. |
| `strongOrientation(g)` | A `StrongOrientationResult` containing the same per-edge arc choices (`arcs`), SCC vertex partition (`component`, `groups`) and minimum attainable number of SCCs (`count`). This orientation is returned even when the whole graph cannot be strongly connected. |
| `ok`, `bridge`, `disconnected` | Robbins test: connected and bridgeless. The empty graph succeeds by explicit vacuous convention and has zero SCCs. `bridge` is an obstruction edge ID or `-1`; `disconnected` is a pair of vertices in different original components or `{-1,-1}`. Both witnesses can be present. |

Vertex blocks overlap at articulation vertices, so they are not a vertex partition. Under the stated loop convention, a vertex may occur in multiple blocks solely because of loops; clients must use `is_art` and cannot infer articulation by counting block memberships. An isolated vertex with loops has one block per loop and no additional empty-edge block. Distinct nonloop blocks overlap at at most one vertex. An ordinary connected nonloop graph's bridge-deleted components are strongly orientable, although they may contain articulation vertices (for example two cycles meeting at one vertex).

The result is a static snapshot. Reinvoke either free function after changing the graph; there is no incremental state, shared cache or borrowed adjacency. ID values and member order are deterministic for the same input adjacency order, but no sorted block/bridge order is promised.

## Correctness argument

The explicit DFS stack retains the next adjacency position of each active vertex and exactly simulates recursive discovery and return events. Skipping only the reverse arc of the tree edge preserves a second parallel edge as a back edge. An undirected DFS has no cross edges between distinct finished subtrees. On return from child `u` of `v`, `low[u]` is the minimum discovery index reachable from `u`'s subtree using tree edges and at most one upward non-tree edge. Thus `(v,u)` is a bridge exactly when `low[u] > tin[v]`, and a nonroot `v` is an articulation point exactly when some child has `low[u] >= tin[v]`. A DFS root is an articulation point exactly when it has at least two children. Loops affect none of these conditions.

Each nonloop edge enters the edge stack once: on tree discovery or from the descendant endpoint of a back edge. At `low[u] >= tin[v]`, the edges above and including `(v,u)` form exactly one maximal vertex block. A connection from its child-side vertices to an earlier open block avoiding `v` would force `low[u] < tin[v]`; conversely the still-open back-edge chains link every popped portion without a separating internal vertex. Immediate one-edge treatment of loops cannot disturb the pending stack. Singleton bridge and isolated-vertex blocks complete the output convention. The marking array deduplicates vertices within each emitted block without sorting, so total output work is linear.

Deleting all bridges and traversing the remaining graph gives the two-edge-connected vertex partition. In the reported orientation, the root of each such component reaches all its vertices by downward tree edges. Every nonroot vertex can reach an ancestor across its entering tree edge because that edge has a back-edge escape; repeatedly taking escapes reaches the component root. Each component is therefore strongly connected. No bridge can belong to a directed cycle, regardless of its orientation, and disconnected original components cannot communicate. Every orientation consequently has at least one SCC per bridge-deleted component; this construction attains that lower bound. This also proves Robbins' connected-and-bridgeless criterion and the obstruction witnesses.

Each vertex/arc is scanned a constant number of times, each edge is pushed/popped once, and every block's vertex list is bounded by twice its edge list except isolated singletons. All bounds are worst-case, including output. The implementation uses ordinary contest-profile containers; no new arithmetic acceleration or performance-comparison claim is involved.

## Legacy migration

Original `BridgeAlgo` and `Tarjan` bytes remain in `OLD/algorithms.cpp` and corresponding older references; they are not correctness evidence. The maintained names retain their constructor signatures and public result fields: `n`, `timer`, `vst`, `t_in`, `low`, normalized endpoint bridge pairs, and `Tarjan::is_art`. `Tarjan` remains the historical undirected lowlink adapter; it is not an SCC algorithm. The new canonical APIs carry original edge IDs, unlike the legacy normalized pairs.

Adapters accept zero-based symmetric adjacency with at least `n` lists and in-range endpoints; nonloop multiplicities must match in both directions. Size, nonnegative `n` and endpoints are assertion-checked. Symmetry is an explicit unchecked input representation precondition. Loops may be listed once or twice and are ignored by the bridge/articulation-only adapters. Reconstructing edge IDs can change DFS adjacency/discovery order, so `t_in`, `low`, and bridge-vector order may differ while retaining their stated meanings. The original traversals also exposed implementation diagnostics rather than a canonical traversal numbering.

`BridgeAlgo::build(adj)` recomputes every field and clears the previous bridge set. The historical public `dfs(cur,prv,adj)` helper keeps its signature and now invokes this complete recomputation: `cur` is an in-range vertex and `prv` is an in-range vertex or the root sentinel `-1`. Manually mutating DFS arrays and resuming a partial recursive traversal is not supported. This explicit migration removes the old helper's unsafe dependence on partially edited traversal state. `BridgeAlgo` keeps an ordered `set`, so constructing its `b` bridge pairs costs `O(n + m + b * log(b + 1))`; `Tarjan` remains `O(n + m)`. Both store `O(n)` results and use `O(n + m)` transient space.

The old Team Notebook cutpoint/bridge code's vertex-block feature is covered by `block_vertices`; isolated vertices, exact parallel-edge parent skipping and stable bridge IDs repair gaps in that reference. The old bridge-only algorithm is covered by `BridgeAlgo` and the canonical `bridges`/`is_bridge` results. Block-cut forest / bridge-tree query wrappers remain assigned to GR12. Dynamic bridge maintenance (GR12), directed dominators, st-numbering/open-ear decomposition, triconnectivity/SPQR and three-edge components are distinct inventory families; none is implied by static lowlink completion.

## Sources inspected

The implementation is independent code from the stated invariants; no external source body was copied. These references were retrieved and read on 2026-09-27, including differences in degenerate-input conventions:

- [cp-algorithms, “Strong Orientation”](https://cp-algorithms.com/graph/strong-orientation.html), last update 2024-06-24: Robbins' theorem, DFS orientation, and the extension minimizing SCC count to original components plus bridges.
- [KACTL, “BiconnectedComponents.h”](https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/BiconnectedComponents.h), Simon Lindholm, dated 2017-04-17, CC0: edge-ID parent exclusion and linear edge-stack decomposition. Its convention omits bridges from callback blocks; this API explicitly includes bridge dyads and accounts for loops and isolates.
- [OI Wiki, “双连通分量”](https://oi-wiki.org/graph/bcc/), page reports update 2026-08-17: edge- versus vertex-biconnectivity, bridge deletion decomposition, lowlink vertex blocks, singleton convention, and the alternative difference/tree-edge coverage formulation. Its sample code drops loops; this API preserves loop edge ownership explicitly.

These sources establish the relevant static decomposition/orientation scope. Resource-limited streaming bridge extraction and higher/dynamic connectivity require separate representations and algorithms; they are not alternate implementations required for this in-memory Graph/CSR contract. No online solution was submitted or acceptance claimed.

## Feature-to-test map and evidence

`96-Local Testing/04-Graphs/09-bridges_articulation_tester.py` uses the shared graph runner and includes the actual header. Assertions are not test oracles; all correctness checks throw/report failures in `-DNDEBUG` builds as well. The command reports seed, mode, configuration, graph edge list, failing operation and expected/actual success, plus subprocess command and timeout/crash details. Exhaustive cases run in increasing graph size, followed by reproducible random cases and named large shapes.

| Feature | Independent verification |
|---|---|
| Bridge flags/IDs | Delete each logical edge and recount components, including loops/parallel edges. |
| Articulation vertices | Delete each vertex and compare the global component count, including roots, isolated vertices and disconnected graphs. |
| Vertex blocks / edge membership | Enumerate every vertex subset, retain maximal subsets that remain connected after each one-vertex deletion, recover all internal nonloop edges, then add the documented loop and isolated blocks. Compare both vertex/edge lists and every edge-to-block index. |
| Edge components | Independent traversal after removing deletion-oracle bridges; verify labels, complete groups and minimum-vertex order. |
| DFS diagnostics | Discovery permutation, exact root set, acyclic input parent arcs, and independently enumerate descendant/back-edge pairs to check every lowlink value. |
| Orientation / Robbins / witnesses | Validate every selected original arc ID; transitive closure verifies the exact SCC equivalence relation. Check obstruction edges/vertices against independent deletion/component oracles and the minimum-component lower bound. |
| Legacy adapters / reset | Compare normalized bridge pairs and articulation flags to deletion oracles; check diagnostics, `build` and `dfs` complete recomputation, including clearing prior bridges. |
| Boundaries / lifetime | Empty graphs, singleton loops, dyads, parallel parent edges, multiple cycles meeting at a cutpoint, disconnected isolated vertices, full-range ignored weights, copied/moved owning results, graph mutation and repeated calls. |
| Scale / stack safety | Long chain, closed chain and parallel-edge star (plus root loop), with exact known block/bridge/articulation expectations. |
| Checked preconditions | Nine death probes: directed Graph/CSR passed to each free function; invalid legacy size/sign/endpoint; invalid legacy DFS arguments. The representation's unchecked symmetry and infeasible-to-allocate `INT_MAX` block-count boundary are documented limits rather than claimed runtime fixtures. |

Quick covers all looped simple graphs through three vertices, all multiplicity-at-most-two looped graphs through two vertices, 100 random multigraphs through seven vertices and 20,000-vertex shapes. Full extends exhaustive bounds to four and three vertices respectively, uses 1,500 random multigraphs through eight vertices, and 200,000-vertex shapes. Stress adds all loopless simple graphs through six vertices, 12,000 random trials and 500,000-vertex shapes. Random trials periodically duplicate an existing edge as a separate metamorphic fixture.

Final verification on 2026-09-27 used GCC 16.2.1, `-std=gnu++20`, and seed `20260927`:

```text
python3 '96-Local Testing/04-Graphs/09-bridges_articulation_tester.py' --mode full --seed 20260927
PASS optimized -O2 -DNDEBUG: 3,643 cases, 1,450,632 checks
PASS checked -O0 -g -D_GLIBCXX_DEBUG: same cases/checks, nine assertion probes
PASS ASan/UBSan -O1 -g -D_GLIBCXX_ASSERTIONS: same cases/checks
```

The sanitizer configuration also enables leak detection, `-fno-sanitize-recover=all`, frame pointers and non-PIE linking. An initial sandbox run completed optimized/checked coverage but LeakSanitizer could not run under the sandbox's ptrace restriction. The final complete command above ran outside that restriction and all three configurations passed. Standalone header compilation is inherent in this suite; repository aggregate/multiple-translation-unit/consistency checks are recorded by the P011 integration owner. No timing benchmark or stress-mode execution is claimed. All features owned by this header are complete; there is no lowlink handoff gap.
