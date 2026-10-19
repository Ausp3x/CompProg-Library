# 04 Graphs — notes

## Shared contracts

- Every graph API states n, 0-based vertices, directedness, edge identity (edge ID and arc ID with reverse arc), self-loops, parallel edges, disconnected cases, weight/capacity type and overflow policy. Weights are full-range signed `lng`; path totals and flow values that can exceed 64 bits use `lll`.
- Return witnesses (paths, cuts, matchings, cycles, certificates) wherever they cost no more than the objective.
- Nonnegative, arbitrary signed and time-dependent weights are different domains. Dijkstra variants reject or forbid negative arcs; Bellman–Ford, DAG and Floyd–Warshall variants mark negative-infinite vertices explicitly. Capacity and cost are separate quantities; residual mutation rules are documented per flow engine.
- Heuristic engines (SPFA, D'Esopo–Pape, Howard, k-pop Dijkstra) carry their worst-case bound in the contract and are never advertised as generally faster.
- Exponential exact engines (clique, coloring, Hamiltonian, treewidth, feedback sets, isomorphism canonical forms, linear extensions) state their size limit and failure mode.

## Verification rules

- Cross-check small graphs by brute force: shortest paths against Floyd–Warshall, MST against subset enumeration, lowlink against edge/vertex deletion, flow against cuts, matching against enumeration. Include disconnected graphs, loops and multiedges.
- Legacy excerpts may use template globals, 1-based indexing or ambiguous directed conventions; their replacements are tested on the canonical `Graph`/`CsrGraph` domain, and legacy input formats convert through `Graph::read`, `Graph::fromAdjacency` or `Graph::fromMatrix`.

## Ownership boundaries

- Data Structures owns DSU (01, 09 rollback, 29 weighted), link-cut trees (37), Euler-tour trees (38), top trees (39), radix heaps (31), persistent leftist heaps (48), PQ-trees (56), matroid oracles (46) and Mo's algorithm including Mo on trees (`23-range_query_offline.hpp`, `TreeMo`). `04-dsu.hpp` and `51-lct.hpp` are adapters keyed by graph vertices and edge IDs.
- Mathematics owns determinants and matrix-tree (23), permanent/Pfaffian/hafnian (48), convolution (13), polynomial and generating-function kernels (14, 40) and linear algebra (16). `53-graph_counting.hpp` adapts graphs to them; `16-centroiddecomposition.hpp` uses Mathematics convolution for the all-distance histogram. Labeled graph-class counts by vertex count (connected, biconnected, DAG, Eulerian, forest, strong digraph, tree, unicyclic) are Mathematics generating-function work, not graph rows.
- Miscellaneous owns small-to-large/DSU-on-tree merging (`15-smalltolarge.hpp`), generic Floyd/Brent cycle finding (`14-cyclefinding.hpp`), exchange-rule scheduling (39) and subset DP scaffolding (32). `14-tree_algorithms.hpp` supplies Euler-tour layouts to Miscellaneous 15 and does not duplicate its container merging.
- Geometry owns Manhattan MST (`21-manhattan_geometry.hpp`) and geometric graph construction (`22-geometricmst.hpp`). Geometric arrangements stay in Geometry; `29-planar_graph.hpp` works on a given combinatorial embedding only.
- Walks versus simple paths (`32` vs `59`), cardinality versus weighted general matching (`22` vs `64`), given planar embedding versus recognition (`29` vs `62`), offline versus online fully dynamic connectivity (`26` vs `63`), fundamental versus minimum cycle basis (`28` vs `72`), and edge connectivity versus vertex triconnectivity (`74` vs `61`) are separate rows.

## Bounded-flow ownership

`30-flow_with_demands.hpp` owns lower/upper-bound and vertex-supply reductions and their reconstruction. Its costed-network adapter states the backend domain: `20-min_cost_flow.hpp` (successive shortest paths) excludes negative cycles ([20-min_cost_flow.md](20-min_cost_flow.md)); general signed-cost circulation belongs to `65-costscalingflow.hpp` and `66-networksimplex.hpp`. Those backends may use the bounded-flow model, so the model must not depend back on them. `65` also owns min-cost b-flow with supplies and demands. `39-graphicalsequence.hpp` `directedDegreeRealization` is a flow reduction: paired out-/in-degree copies, admissible-arc capacities, a saturating flow; its contract must state the loop and parallel-edge restrictions.

## Tree ownership

- `07-lca.hpp` owns static LCA, k-th ancestor by binary lifting, path jumps and rerooted LCA. `85-levelancestor.hpp` owns O(1) level ancestor (ladder decomposition) and offline batched level-ancestor queries.
- `14-tree_algorithms.hpp` owns diameter/center/centroid, static Euler tours, reroot and subtree DP, size-bounded subtree merge DP, virtual trees and Prüfer codes. `15-hld.hpp` owns path/subtree folds and actions; `16-centroiddecomposition.hpp` owns contour queries; `84-dynamictreedp.hpp` owns point-update tree DP through static top trees, including dynamic diameter under weight updates.
- `11-functionalgraph.hpp` owns directed successor graphs. Undirected unicyclic (namori/pseudoforest) decomposition belongs to `41-cactusgraph.hpp`.
- `79-tree_ordering.hpp` stays in Graphs: its objectives are defined on rooted trees and its engines are DSU/heap merges over tree structure; Miscellaneous 39 references it for precedence scheduling.

## Legacy migration obligations

- Existing-unverified headers (`12`, `13`, `15`) are pinned unchanged monolith excerpts; `00-Guidelines/Ledgers/monolith-map.json` records their source ranges and hashes (`00-Guidelines/History/2026-09-27-monolith-transfer.md`).
- The `MinCostMaxFlow`/`MaxFlowMinCut` copies in `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `test.cpp` are mapped in `20-min_cost_flow.md`; the other archive leads are the `legacy:` paths of rows `12`–`15` and `30`. `OLD/[1] algorithms.cpp` TODO comments motivated rows `14` and `17`.

## Placement decisions recorded

- `04-dsu.hpp` stays: it is a verified adapter with a real operation (`graphComponents`), not a bare re-export.
- `51-lct.hpp` `DynamicForest` repeats Data Structures 37 `LinkCutTree` operations with edge-ID keys; whether it stays is an open question in `00-Guidelines/23-Sweep Ledgers/duplicates.md` (P155).
- Shortest-path counting, shortest-path DAG, widest/minimax paths and Goldberg negative-weight scaling were placed in `13-shortest_path_advanced.hpp` rather than reopening the verified `05-shortest_path.hpp`.
- `lineGraph`, `inducedSubgraph`, `contract` and `simplify` belong to `01-graph.hpp` because they are representation utilities; maximum-weight spanning forests are separate `06-mst.hpp` engines because negating full-range `lng` weights overflows.
- Incremental SCC, dynamic connectivity and bridge maintenance, block-cut and bridge-tree queries, tree/HLD aggregates, Euler-trail counting and postman augmentation belong to rows 70, 26, 27, 15, 53 and 69; functional successor updates are outside row 11's static contract.

## P010 common contract

Applies to `01-graph.hpp` through `06-mst.hpp`. Every algorithm accepts `Graph` or `CsrGraph` unless it names `DenseGraph`. Vertices are `[0, n)`, logical edges `[0, m)` and arcs `[0, arcs.size())`, and every count fits `int`. Weights span the full `lng` range; path and forest totals are exact `lll`. Results hold arc and edge indices into the input graph, so they stay meaningful only while that graph's arc numbering and content are unchanged; result arrays themselves own their storage. Public storage of graphs and results is inspectable but must not be edited behind an algorithm's invariants. Violated preconditions (out-of-range vertex, wrong directedness, negative weights for Dijkstra) are assertions; a valid input with no answer returns a status field or an empty witness. Complexity comments use `m` for the arc count where it differs from the edge count only by a factor of two. Unvisited result fields are `-1`.

## P010 re-audit omissions

The 2026-10-07 completeness sweep ([sources](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit)) added `TopologicalResult: unique` and `KruskalReconstruction: leafRange`. The other candidates stay out:

- Degree arrays (maspypy `deg_array`, `deg_array_inout`): `g[u].size()` is the out-degree, and an in-degree array is one pass over `g.arcs`.
- `edgeId(u, v)` lookup by endpoints: an adjacency scan or `DenseGraph::best` gives it; a hashed endpoint index is a caller choice with its own memory trade-off.
- Vertex path to edge path (maspypy `vs_to_es`): every path witness here already returns arc IDs.
- DFS entry/exit times and edge classification: positions in `order` and `postorder` are the entry and exit ranks; tree, back, forward and cross arcs follow from `parent_arc` and those ranks in O(n + m).
- Chordless (minimal) cycle witness (maspypy `find_cycle` with minimality): rarely needed; a candidate for `68-cycle_enumeration.hpp`.
- Directed odd cycle (maspypy `find_odd_cycle`): belongs with `68-cycle_enumeration.hpp`.
- Vertices and edges on some shortest s–t path: the shortest-path DAG of `13-shortest_path_advanced.hpp`.
- Lexicographically largest topological order: run the lexicographic sort on the graph relabelled by `v -> n - 1 - v` (`contract` with that labelling) and map the order back.
- Congruence shortest path (OI Wiki 同余最短路): a modelling reduction to Dijkstra over residues, not a separate operation.
- 0/c-weight BFS (hitonanode `zero_one_bfs` with a constant): divide the weights by c and use `zeroOneBfs`.
- Fibonacci-heap or skew-heap Dijkstra and Prim (ei1333, Nyaan): no practical gain over the binary heap in contest sizes.

## P011 re-audit omissions

The 2026-10-07 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit)) added `LCAFold: pathFold`, `pathIntersection`, the lexicographic `eulerianTrail` option, `LowlinkResult: cut_components`, `FunctionalGraph: jumpAll` and `FunctionalGraphFold: maxStep`. The other candidates stay out:

- Ancestor or path binary search by predicate (hitonanode `max_length`, maspypy `max_path`): path search belongs to row 15 (HLD).
- DFS-order LCA (OI Wiki): a constant-factor alternative to `EulerLCA` with the same contract.
- Mixed directed/undirected Euler circuits: need max flow, so they belong with rows 19/30/69.
- Two-edge-connectivity augmentation and extended block-cut trees: row 27 (block-cut and bridge trees).
- Exporting the functional graph's reverse forest as a tree: `successor`, `depth` and `tin`/`tout` already describe it.
- `stepUntil` (smallest step where a monotone predicate becomes true): `maxStep` of the negated predicate plus one.
- Counting functional graphs (CSES Functional Graph Distribution): combinatorics, not a graph operation.

## P043 omissions

The 2026-10-09 sweeps ([sources](00-sources.md#pages-fetched-on-2026-10-09-p043)) added `addVar`, `setValue`, `addEquivalent` and the O(k) form of `addAtMostOne` to row 18; dynamic `addEdge`/`eraseEdge`/`augment` to row 21; and `kBestAssignments` and the unrestricted-size option with cost curve to row 23. The other candidates stay out:

- Lexicographically smallest 2-SAT assignment: needs O(n·m) incremental propagation; no catalog provides it. Incremental 2-SAT, solution counting (#P-hard) and Horn-SAT (suisen) are different problems.
- `addNand` (ei1333): it is `addClause(~a, ~b)`.
- Lexicographically extremal maximum matching and lexicographically smallest vertex cover (ei1333): rare; greedy fixing of allowed edges from `essentialEdges` gives the matching in O(V·(V + E)).
- Rank-maximal matching (maspypy): row 80 (`80-rankedmatching.hpp`).
- Automatic bipartition overloads (hitonanode, maspypy): run `bipartiteCheck` (row 02) and map the sides.
- Maximum antichain, DAG path and chain covers: row 31. Bipartite edge colouring: row 34.
- Incremental row addition and dynamic Hungarian cost updates (hitonanode `augment`, OI Wiki): the internal single-row `search` exists, but a public stateful solver with update semantics is not part of any contest catalog interface fetched; rerun `hungarian` or use `kBestAssignments`.
- Bottleneck assignment and the auction algorithm: in no fetched catalog; bottleneck assignment is a binary search over `hopcroftKarp` or `bipartiteMatchingDense`.
- DAG dominator tree by LCA (OI Wiki): Lengauer–Tarjan covers DAGs with a better bound; online vertex appending is not part of this row.
- Dominated counts (OI Wiki P5180): `tout[v] − tin[v]` of `DominatorTree`.
- Post-dominators, multiple roots and edge dominators: reverse graph, super-source and edge subdivision. Exposed semidominators (internal in Koosaga) and iterated dominance frontiers (SSA construction) are outside contest use.

## P044 max flow omissions

The 2026-10-10 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-10-p044-max-flow)) added the reverse capacity of `addEdge`, `MaxFlow(Graph)`, `changeCapacity` (increase and decrease with repair), `minCutEdges` and `pathDecomposition` to row 19. The other candidates stay out:

- Residual-graph export: `g` is public and `edges()`/`getEdge` give every arc pair; a copied adjacency adds nothing.
- Fewest-edge minimum cut: a reduction, not an engine. Scale capacities to `c * (m + 1) + 1` (with `lll` if needed); the cut value is `res / (m + 1)` and its edge count `res % (m + 1)`.
- Super source/sink and vertex capacities: modelling reductions (one extra vertex with arcs; split v into v_in -> v_out carrying the vertex capacity). Row 45 owns vertex-disjoint path extraction.
- MPM, ISAP and plain Ford–Fulkerson: no better bound than Dinic or push-relabel; `flow(s, t, limit)` already costs O(F * (n + m)) for a small flow value F.
- Floating-point capacities: `MaxFlow` requires a signed integer `Cap`; scale to integers.
- `limit` for push-relabel: use `flow(s, t, limit)`.
- Mixed directed/undirected Euler circuits (flow application): row 69 with row 10, using this header.
- ei1333 bipartite flow with edge erasure and lexicographic matching: row 21; lower bounds: row 30; Gomory–Hu, global min cut, dynamic star min cut: rows 46, 47, 76.

## P045 min cost flow omissions

The 2026-10-13 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-13-p045-min-cost-flow)) added `potentials`, the `below` bound of `flow`/`slope` (min-cost flow of any amount at `below = 0`) and the DAG-order initialisation to row 20. `minCostMaxFlow` (`flow(s, t)`), `bellmanFordPotentials` (automatic inside `flow`, `slope` and `potentials`) and `pathReconstruction` (`MaxFlow::pathDecomposition`) left the row ([20-min_cost_flow.md](20-min_cost_flow.md#legacy-mapping)). No catalog has a convex-cost edge API, so `addConvexEdge` follows the row's piecewise-linear wording. The other candidates stay out:

- Target flow with an infeasible status (ei1333, cp-algorithms): `flow(s, t, k).first < k`.
- Max-cost (max-gain) flow: negate the costs; the result is minus the minimum.
- Dense O(n^2) Dijkstra (OI Wiki footnote): measured slower than the heap on every benchmarked workload, including a 300-vertex half-dense graph.
- Primal–dual blocking flow on the zero-reduced-cost network (zkw style): same worst case; one path per Dijkstra as ACL.
- Changing an edge's capacity or cost after a flow: no SSP catalog has it; rebuild, or cancel with row 65.
- Convex costs given by a callable over a large capacity (capacity scaling, Ahuja–Magnanti–Orlin): piecewise lists only; scaling belongs with row 65.
- Floating-point capacities or costs (legacy `old_mcmf.cpp`): integers only; scale.
- Multi-source/sink, supplies and negative cycles: rows 30 and 65.

## P056 omissions

The 2026-10-16 sweeps ([sources](00-sources.md#pages-fetched-on-2026-10-16-p056)) added `NearestMarked: unmark`, `DistanceSum`, `ContourFold`, `ContourApply` and `countPathsWithin` to row 16, and `allRootsClassification` and `rootedTreeIsomorphism` to row 17. The legacy recursive `par`/`sz` build left row 16 ([16-centroiddecomposition.md](16-centroiddecomposition.md#legacy-mapping)). The other candidates stay out:

- Per-centroid callback (maspypy, hitonanode): `tin`, `order` and `size` expose every component and its child blocks; a callback adds nothing.
- Static contour sum (suisen `contour_sum`): `ContourSum` without updates; prefix arrays change only the constant.
- Edge centroid decomposition (OI Wiki 边分治): no catalog exposes a reusable API beyond the vertex version.
- Incremental centroid (maspypy): a dynamic centroid of a weighted tree, a problem-specific construction without a reusable family.
- Rerooted (all-roots) Monte Carlo hash (maspypy, hitonanode, OI Wiki): `allRootsClassification` is exact and linear within one tree; across trees, `unrootedTreeIsomorphism` maps the orbits.
- Forest isomorphism and vertex-coloured trees: no catalog API; join each forest's roots to a new vertex, and encode colours as leaf gadgets or compare ids per colour.
- Path rolling hash on trees (maspypy `Rolling_Hash_on_Tree`) and unlabeled-tree enumeration (maspypy): Strings and Mathematics work, not tree isomorphism.

## P047 omissions

The 2026-10-18 sweeps ([sources](00-sources.md#pages-fetched-on-2026-10-18-p047)) added the all-roots `DirectedMst`, `IncrementalBridgeConnectivity: find`, `componentSizeAfterRemovingVertex`/`Edge`, `mandatoryVertexCount`, `simplePathVertexCount` and `twoEdgeConnectivityAugmentation`. The other candidates stay out:

- k-best arborescences (NetworkX `ArborescenceIterator`): O(k) constrained Edmonds runs over forced/forbidden arcs; in no contest catalog.
- Minimum arborescence with a free root: the minimum of `arborescence(r)` over roots, or a super-root with arcs heavier than `n` times the largest weight spread while that stays below 2^64; maximum arborescence: `DirectedMst(n, arcs)` with negated `lll` weights; forced or forbidden arcs: drop the other in-arcs of the head, or the arc.
- Gabow–Galil–Spencer–Tarjan O(m + n log n): Fibonacci heaps, no catalog implementation, no contest gain over O(m log m).
- Grid decremental connectivity (maspypy, CF 1864I): a grid-only dual technique from one catalog. Component counts per time and XOR-path presets: `run` callbacks over `count()` or a weighted DSU (Data Structures 29).
- Offline 2-edge-connectivity over time: in no catalog.
- OGDF `insertNode`, `repVertex` and an incremental `blockOf`: SPQR maintenance internals; incremental block ids change under merges, so `sameBlock` is the stable query.
- Vertex-biconnectivity augmentation: only NetworkX's general k-connectivity augmentation; set-of-terminals block queries (SDOI2018) are a row 14 virtual tree over `BlockCutTree::tree`.
- Mirsky partition (minimum antichain cover): level sets of longest paths (`dagShortestPaths`, longest mode). Antichain enumeration (NetworkX `antichains`): exponential output.
- Flow-based chain cover without closure (maspypy `dag_path_cover`): the same result as `dagMinimumChainCover`; the closure-free antichain is `maximumWeightedAntichain` with unit weights.
- Chain covers of digraphs with cycles: condense with row 08 (`condensationDag`), cover the DAG and expand each component inside its chain. Vertex-disjoint path cover with cycles is NP-hard. Greene–Kleitman k-antichains of posets: in no catalog.
