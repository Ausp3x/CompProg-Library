# 04 Graphs — notes

Contracts, ownership boundaries and migration obligations moved out of the inventory on 2026-10-06. The inventory lists operations; this file states the rules every row must honour.

## Shared contracts

- Every graph API states n, 0-based vertices, directedness, edge identity (edge ID and arc ID with reverse arc), self-loops, parallel edges, disconnected cases, weight/capacity type and overflow policy. Weights are full-range signed `lng`; path totals and flow values that can exceed 64 bits use `lll`.
- Return witnesses (paths, cuts, matchings, cycles, certificates) wherever they cost no more than the objective. A violated precondition is an assert; a valid input with no answer is a sentinel or status field.
- Nonnegative, arbitrary signed and time-dependent weights are different domains. Dijkstra variants reject or forbid negative arcs; Bellman–Ford, DAG and Floyd–Warshall variants mark negative-infinite vertices explicitly. Capacity and cost are separate quantities; residual mutation rules are documented per flow engine.
- All-solution and enumeration APIs return compact parameterizations or explicitly bounded enumerations and are charged by output size.
- Heuristic engines (SPFA, D'Esopo–Pape, Howard, k-pop Dijkstra) carry their worst-case bound in the contract and are never advertised as generally faster.
- Exponential exact engines (clique, coloring, Hamiltonian, treewidth, feedback sets, isomorphism canonical forms, linear extensions) state their size limit and failure mode.

## Verification rules

- A row is verified only when every operation in it has a test with an independent oracle and the recorded commands pass; see `90`–`95` evidence documents for the current verified rows.
- Cross-check small graphs by brute force: shortest paths against Floyd–Warshall, MST against subset enumeration, lowlink against edge/vertex deletion, flow against cuts, matching against enumeration. Include disconnected graphs, loops and multiedges.
- Legacy excerpts may use template globals, 1-based indexing or ambiguous directed conventions. Migration from `existing-unverified` or `legacy-reference` to `verified` requires a standalone compile, contract tests on the canonical `Graph`/`CsrGraph` domain and a recorded legacy-adapter decision.

## Ownership boundaries

- Data Structures owns DSU (01, 09 rollback, 29 weighted), link-cut trees (37), Euler-tour trees (38), top trees (39), radix heaps (31), persistent/skew heaps (48), PQ-trees (56) and matroid oracles (46). `04-dsu.hpp` and `51-lct.hpp` are adapters keyed by graph vertices and edge IDs. The legacy `97-Legacy/03-subtree_queries.cpp` is a static Euler-tour flattening, not an Euler-tour tree; its features belong to `14-tree_algorithms.hpp`.
- Mathematics owns determinants and matrix-tree (23), permanent/Pfaffian/hafnian (48), convolution (13), polynomial and generating-function kernels (14, 40) and linear algebra (16). `53-graph_counting.hpp` adapts graphs to them; `16-centroiddecomposition.hpp` uses Mathematics convolution for the all-distance histogram. Labeled graph-class counts by vertex count (connected, biconnected, DAG, Eulerian, forest, strong digraph, tree, unicyclic) are Mathematics generating-function work, not graph rows.
- Miscellaneous owns small-to-large/DSU-on-tree merging (15), generic Floyd/Brent cycle finding (14), exchange-rule scheduling (39), Mo-style offline queries (10) and subset DP scaffolding (32). `14-tree_algorithms.hpp` supplies Euler-tour layouts to Miscellaneous 15 and does not duplicate its container merging.
- Geometry owns Manhattan MST (22) and geometric graph construction. Geometric arrangements stay in Geometry; `29-planar_graph.hpp` works on a given combinatorial embedding only.
- Walks versus simple paths (`32` vs `59`), cardinality versus weighted general matching (`22` vs `64`), given planar embedding versus recognition (`29` vs `62`), offline versus online fully dynamic connectivity (`26` vs `63`), fundamental versus minimum cycle basis (`28` vs `72`), and edge connectivity versus vertex triconnectivity (`74` vs `61`) are separate rows.

## Bounded-flow ownership

`30-flow_with_demands.hpp` owns lower/upper-bound and vertex-supply reductions and their reconstruction. Its costed-network adapter states the backend domain: `20-min_cost_flow.hpp` (successive shortest paths) may have a restricted negative-cycle contract; general signed-cost circulation belongs to `65-costscalingflow.hpp` and `66-networksimplex.hpp`. Those backends may use the bounded-flow model, so the model must not depend back on them. `65` also owns min-cost b-flow with supplies and demands.

## Tree ownership

- `07-lca.hpp` owns static LCA, k-th ancestor by binary lifting, path jumps and rerooted LCA. `85-levelancestor.hpp` owns O(1) level ancestor (ladder decomposition) and offline batched level-ancestor queries.
- `14-tree_algorithms.hpp` owns diameter/center/centroid, static Euler tours, reroot and subtree DP, size-bounded subtree merge DP, virtual trees and Prüfer codes. `15-hld.hpp` owns path/subtree folds and actions; `16-centroiddecomposition.hpp` owns contour queries; `84-dynamictreedp.hpp` owns point-update tree DP through static top trees, including dynamic diameter under weight updates.
- `11-functionalgraph.hpp` owns directed successor graphs. Undirected unicyclic (namori/pseudoforest) decomposition belongs to `41-cactusgraph.hpp`.
- `79-tree_ordering.hpp` stays in Graphs: its objectives are defined on rooted trees and its engines are DSU/heap merges over tree structure; Miscellaneous 39 references it for precedence scheduling.

## Legacy migration obligations

- Existing-unverified headers (`12`, `13`, `15`, `16`, `18`, `19`, `23`, `26`) contain unchanged monolith excerpts; see `97-Legacy/00-index.md` and `00-Guidelines/14-monolith-transfer.md`. Compilation does not verify semantics, completeness, performance, indexing or complexity claims. The source-range/hash map `00-Guidelines/15-monolith-map.json` is authoritative for provenance.
- Legacy-reference rows (`20`, `21`, `27`) point at Team Notebook sources `OLD/Team Notebook/src/graph/old_mcmf.cpp`, `old_kuhn.cpp` and `old_block-cut-tree.cpp`; `old_bellman-ford.cpp` is superseded by `05-shortest_path.hpp`.
- Unchanged archive leads in `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `test.cpp`: `LcaO1`, `LcaLog`, `DEsopoPape`, `TarjanSCC`, `StrongOrientation`, `EulerPath`, `Kruskal`, `PrimDense`, `TreeEdgePainting`, `Dinic`, `FlowWithDemands`, `PushRelabel`, `Kuhn` (symbols vary by file). They are behavioral references, never specifications. `OLD/[1] algorithms.cpp` TODO comments motivated rows `14` and `17`.
- Delete from `OLD` or `97-Legacy` only after an audited implementation accounts for every feature the file contained.

## Placement decisions recorded

- `04-dsu.hpp` stays: it is a verified adapter with a real operation (`graphComponents`), not a bare re-export.
- `51-lct.hpp` stays as a thin adapter row with concrete operations; merging it into Data Structures 37 or `84-dynamictreedp.hpp` is a candidate scoped rename task, not part of this rewrite.
- Shortest-path counting, shortest-path DAG, widest/minimax paths and Goldberg negative-weight scaling were placed in `13-shortest_path_advanced.hpp` rather than reopening the verified `05-shortest_path.hpp`.
- `lineGraph`, `inducedSubgraph`, `contract` and `simplify` reopened `01-graph.hpp` as partial because they are representation utilities; maximum-weight spanning forests reopened `06-mst.hpp` because negating full-range `lng` weights overflows.

## P010 re-audit omissions

The 2026-10-07 completeness sweep ([sources](81-sources.md#pages-fetched-on-2026-10-07-p010-re-audit)) added `TopologicalResult: unique` and `KruskalReconstruction: leafRange`. The other candidates stay out:

- Degree arrays (maspypy `deg_array`, `deg_array_inout`): `g[u].size()` is the out-degree, and an in-degree array is one pass over `g.arcs`.
- `edgeId(u, v)` lookup by endpoints: an adjacency scan or `DenseGraph::best` gives it; a hashed endpoint index is a caller choice with its own memory trade-off.
- Vertex path to edge path (maspypy `vs_to_es`): every path witness here already returns arc IDs.
- DFS entry/exit times and edge classification: positions in `order` and `postorder` are the entry and exit ranks; tree, back, forward and cross arcs follow from `parent_arc` and those ranks in O(n + m).
- Chordless (minimal) cycle witness (maspypy `find_cycle` with minimality): rarely needed; a candidate for `68-cycle_enumeration.hpp`.
- Directed odd cycle (maspypy `find_odd_cycle`): belongs with `68-cycle_enumeration.hpp`.
- Vertices and edges on some shortest s–t path: the shortest-path DAG of `13-shortest_path_advanced.hpp`.
- Lexicographically largest topological order: run the lexicographic sort on the graph relabelled by `v -> n - 1 - v` (`contract` with that labelling) and map the order back.
- Congruence shortest path (OI Wiki 同余最短路): a reduction to Dijkstra over residues; candidate for `13-shortest_path_advanced.hpp`, not added to that row here.
- 0/c-weight BFS (hitonanode `zero_one_bfs` with a constant): divide the weights by c and use `zeroOneBfs`.
- Fibonacci-heap or skew-heap Dijkstra and Prim (ei1333, Nyaan): no practical gain over the binary heap in contest sizes.

## P011 re-audit omissions

The 2026-10-07 sweep ([sources](81-sources.md#pages-fetched-on-2026-10-07-p011-re-audit)) added `LCAFold: pathFold`, `pathIntersection`, the lexicographic `eulerianTrail` option, `LowlinkResult: cut_components`, `FunctionalGraph: jumpAll` and `FunctionalGraphFold: maxStep`. The other candidates stay out:

- Ancestor or path binary search by predicate (hitonanode `max_length`, maspypy `max_path`): path search belongs to row 15 (HLD).
- DFS-order LCA (OI Wiki): a constant-factor alternative to `EulerLCA` with the same contract.
- Mixed directed/undirected Euler circuits: need max flow, so they belong with rows 19/30/69.
- Two-edge-connectivity augmentation and extended block-cut trees: row 27 (block-cut and bridge trees).
- Exporting the functional graph's reverse forest as a tree: `successor`, `depth` and `tin`/`tout` already describe it.
- `stepUntil` (smallest step where a monotone predicate becomes true): `maxStep` of the negated predicate plus one.
- Counting functional graphs (CSES Functional Graph Distribution): combinatorics, not a graph operation.

