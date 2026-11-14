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
- Mathematics owns determinants and the matrix-tree counts (23: spanning trees, arborescences, rooted forests and BEST Eulerian circuits, user decision 2026-11-08), permanent/Pfaffian/hafnian (48), convolution (13), polynomial and generating-function kernels (14, 40) and linear algebra (16). `53-graph_counting.hpp` adapts graphs to the other kernels; `16-centroiddecomposition.hpp` uses Mathematics convolution for the all-distance histogram. Labeled graph-class counts by vertex count (connected, biconnected, DAG, Eulerian, forest, strong digraph, tree, unicyclic) are Mathematics generating-function work, not graph rows.
- Miscellaneous owns small-to-large/DSU-on-tree merging (`15-smalltolarge.hpp`), generic Floyd/Brent cycle finding (`14-cyclefinding.hpp`), exchange-rule scheduling (39) and subset DP scaffolding (32). `14-tree_algorithms.hpp` supplies Euler-tour layouts to Miscellaneous 15 and does not duplicate its container merging.
- Geometry owns Manhattan MST (`21-manhattan_geometry.hpp`) and geometric graph construction (`22-geometricmst.hpp`). Geometric arrangements and point location stay in Geometry; `29-planar_graph.hpp` works on a given combinatorial embedding, and its `fromDrawing` builds one from a straight-line drawing (cp-algorithms faces, decided 2026-10-28) without resolving nested components.
- Walks versus simple paths (`32` vs `59`), cardinality versus weighted general matching (`22` vs `64`), given planar embedding versus recognition (`29` vs `62`), offline versus online fully dynamic connectivity (`26` vs `63`), fundamental versus minimum cycle basis (`28` vs `72`), and edge connectivity versus vertex triconnectivity (`74` vs `61`) are separate rows.

## Bounded-flow ownership

`30-flow_with_demands.hpp` owns lower/upper-bound and vertex-supply reductions and their reconstruction. Its costed-network adapter starts negative-cost edges saturated, so its backend sees nonnegative costs only, which keeps `20-min_cost_flow.hpp` (successive shortest paths, no negative cycles; [20-min_cost_flow.md](20-min_cost_flow.md)) inside its domain at a cost pseudo-polynomial in the total positive excess after the reversal ([30-flow_with_demands.md](30-flow_with_demands.md)); polynomial general signed-cost circulation engines belong to `65-costscalingflow.hpp` and `66-networksimplex.hpp`. Those backends may use the bounded-flow model, so the model must not depend back on them. `65` also owns min-cost b-flow with supplies and demands. `39-graphicalsequence.hpp` `directedDegreeRealization` is a flow reduction: paired out-/in-degree copies, admissible-arc capacities, a saturating flow; its contract must state the loop and parallel-edge restrictions.

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

- Degree arrays (maspypy `deg_array`, `deg_array_inout`): `g[u].size()` is the out-degree, and an in-degree array is one pass over `g.arcs`. (returned by P275 2026-11-10 into row 01 `inDegrees`)
- `edgeId(u, v)` lookup by endpoints: an adjacency scan or `DenseGraph::best` gives it; a hashed endpoint index is a caller choice with its own memory trade-off. (returned by P275 2026-11-10 into row 01)
- Vertex path to edge path (maspypy `vs_to_es`): every path witness here already returns arc IDs.
- DFS entry/exit times and edge classification: positions in `order` and `postorder` are the entry and exit ranks; tree, back, forward and cross arcs follow from `parent_arc` and those ranks in O(n + m). (returned by P275 2026-11-10 into row 02 `classifyArcs`)
- Chordless (minimal) cycle witness (maspypy `find_cycle` with minimality): adopted on 2026-11-08 into row 68 as `findChordlessCycle`.
- Directed odd cycle (maspypy `find_odd_cycle`): adopted on 2026-11-08 into row 68 as `findOddCycle (directed)`.
- Vertices and edges on some shortest s–t path: the shortest-path DAG of `13-shortest_path_advanced.hpp`.
- Lexicographically largest topological order: `topologicalSortSmallest` on the graph relabelled by `v -> n - 1 - v` (`contract` with that labelling), mapped back. (returned by P275 2026-11-10 into row 03 `topologicalSortLargest`)
- Congruence shortest path (OI Wiki 同余最短路): adopted on 2026-11-08 into Mathematics row 03 as `residueShortestPath` (round robin, no Dijkstra) with `frobeniusNumber` and `countRepresentable`.
- 0/c-weight BFS (hitonanode `zero_one_bfs` with a constant): divide the weights by c and use `zeroOneBfs`. (returned by P275 2026-11-10 into row 05)
- Fibonacci-heap or skew-heap Dijkstra and Prim (ei1333, Nyaan): no practical gain over the binary heap in contest sizes.

## P011 re-audit omissions

The 2026-10-07 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit)) added `LCAFold: pathFold`, `pathIntersection`, the lexicographic `eulerianTrail` option, `LowlinkResult: cut_components`, `FunctionalGraph: jumpAll` and `FunctionalGraphFold: maxStep`. The other candidates stay out:

- Ancestor or path binary search by predicate (hitonanode `max_length`, maspypy `max_path`): adopted on 2026-11-08 into row 15 as `maxPath`.
- DFS-order LCA (OI Wiki): adopted on 2026-11-08 into row 07 as `SparseTableLCA` (lighter than `EulerLCA` under Priority).
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
- DAG dominator tree by LCA (OI Wiki): adopted on 2026-11-08 into row 24 as `dominatorTreeDag` (online vertex appends in topological order).
- Dominated counts (OI Wiki P5180): `tout[v] − tin[v]` of `DominatorTree`. (returned by P275 2026-11-10 into row 24 `DominatorTree::dominatedCount`)
- Post-dominators, multiple roots and edge dominators: reverse graph, super-source and edge subdivision. Exposed semidominators (internal in Koosaga) and iterated dominance frontiers (SSA construction) are outside contest use.

## P044 max flow omissions

The 2026-10-10 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-10-p044-max-flow)) added the reverse capacity of `addEdge`, `MaxFlow(Graph)`, `changeCapacity` (increase and decrease with repair), `minCutEdges` and `pathDecomposition` to row 19. The other candidates stay out:

- Residual-graph export: `g` is public and `edges()`/`edge` give every arc pair; a copied adjacency adds nothing.
- Fewest-edge minimum cut: a reduction, not an engine. Scale capacities to `c * (m + 1) + 1` (with `lll` if needed); the cut value is `res / (m + 1)` and its edge count `res % (m + 1)`. (returned by P275 2026-11-10 into row 19 `MaxFlow::minCutFewestEdges`)
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
- Convex costs given by a callable over a large capacity (capacity scaling, Ahuja–Magnanti–Orlin): piecewise lists only here; capacity scaling was adopted on 2026-11-08 into row 65 as `minCostBFlow (capacity scaling)` and `capacityScalingMinCostFlow`.
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
- Minimum arborescence with a free root: the minimum of `arborescence(r)` over roots, or a super-root with arcs heavier than `n` times the largest weight spread while that stays below 2^64; maximum arborescence: `DirectedMst(n, arcs)` with negated `lll` weights; forced or forbidden arcs: drop the other in-arcs of the head, or the arc. (returned by P275 2026-11-10 into row 25)
- Gabow–Galil–Spencer–Tarjan O(m + n log n): Fibonacci heaps, no catalog implementation, no contest gain over O(m log m).
- Grid decremental connectivity (maspypy, CF 1864I): a grid-only dual technique from one catalog. Component counts per time and XOR-path presets: `run` callbacks over `count()` or a weighted DSU (Data Structures 29).
- Offline 2-edge-connectivity over time: in no catalog.
- OGDF `insertNode`, `repVertex` and an incremental `blockOf`: SPQR maintenance internals; incremental block ids change under merges, so `sameBlock` is the stable query.
- Vertex-biconnectivity augmentation: only NetworkX's general k-connectivity augmentation; set-of-terminals block queries (SDOI2018) are a row 14 virtual tree over `BlockCutTree::tree`.
- Mirsky partition (minimum antichain cover): level sets of longest paths (`dagShortestPaths`, longest mode). Antichain enumeration (NetworkX `antichains`): exponential output.
- Flow-based chain cover without closure (maspypy `dag_path_cover`): the same result as `dagMinimumChainCover`; the closure-free antichain is `maximumWeightedAntichain` with unit weights.
- Chain covers of digraphs with cycles: condense with row 08 (`condensationDag`), cover the DAG and expand each component inside its chain. Vertex-disjoint path cover with cycles is NP-hard. Greene–Kleitman k-antichains of posets: in no catalog.

## P048 GR13 omissions

The 2026-10-28 sweeps ([sources](00-sources.md#pages-fetched-on-2026-10-28-p048-gr13)) added `XorWalk` and `CutSpaceHash` to row 28, and `fromDrawing`, `outer`, `commonFace`, per-arc capacities and the flow witness of `planarMinCutDual` to row 29; `primalDualTraversal` became `treeCotree` (decided 2026-10-28). Lipton–Tarjan planar separators (OGDF) and maspypy's dynamic fundamental cut became planned rows 89 and 90 (package P249, user decision 2026-10-28). The other candidates stay out (user decision 2026-10-28: every omission below and the agent's design calls recorded in [28-cycle_basis.md](28-cycle_basis.md) and [29-planar_graph.md](29-planar_graph.md) are confirmed):

- Covering counts per forest edge: sizes of `fundamentalCuts`; tree differences over the chords give them in O(n + m) without a dedicated API.
- Cycle-space and cut-space membership, coordinates and enumeration (ABC419 G): an even-degree check, the chords or forest edges a set contains, and Gray-code XOR over `cycles`.
- Integer (gcd) cycle values of signed walks: no source.
- Next/previous half-edge around a vertex or face, twins, left and right faces, bridges, face sizes and counts: `next`, `prev`, `arcs[a].rev`, `face` and `faces`.
- Embedding validity (NetworkX `check_structure`): the constructor builds twins from the graph; planarity of the embedding is `genus[c] == 0` for every component.
- Half-edge insertion and connecting components inside a face (NetworkX, Boost): permute `g.adj` and rebuild.
- OGDF embedding edits (split/join faces, subdivide, contract), Boost biconnected/maximal planar augmentation, canonical ordering and straight-line grid drawing: no contest use in any fetched catalog.
- Itai–Shiloach planar flow: no better bound than the dual shortest path; planar global min cut: `dualGraph` with row 68 `weightedGirth`; all-pairs planar min cut and multi-terminal outer-face cuts (Luogu P7916): research level or problem-specific.

## P048 GR27 omissions

The 2026-10-28 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-28-p048-gr27)) added the `value` and `cost` fields to `FlowWithDemandsResult` and the signed lower-bound domain (hitonanode `B_Flow`) to row 30. `feasibleFlow` (ei1333 `can_flow(s, t)`) and the `stCut` and `potentials` witnesses were added on user decision 2026-10-29. The other candidates stay out (user decision 2026-10-29):

- Lower-bounded s–t flow of a fixed value k: an added edge `(t, s, k, k)`.
- Minimum flow by binary search on the t→s capacity (cp-algorithms): O(log U) max flows, dominated by the second residual max flow.
- Builder structs with `add_edge(from, to, lo, hi)` (every catalog): an argument shape; the free functions take the edge list.
- maspypy `cyclic_b_flow`: a problem-specific costed ring flow. Min-cost b-flow with a dual certificate (Library Checker `min_cost_b_flow`): row 65.

## P048 GR32 omissions

The 2026-10-29 sweep ([sources](00-sources.md#pages-fetched-on-2026-10-29-p048-gr32)) and the `/finish` audit led to these additions to row 35, all by user decision 2026-10-29:
- `countC4PerEdge` (Library Checker `counting_c4`), `countC3C4PerVertex` (maspypy pointwise counts), `countTrianglesPerEdge` (OI Wiki) and `triadicCensus` (NetworkX);
- `countPathsP3P4P5` fixed as per-vertex counts;
- `localClustering`, `directedClustering`, `localClusteringWeighted`, `directedClusteringWeighted`, `averageClustering`, `transitivity`, `squareClustering` and `generalizedDegree` (NetworkX `cluster.py`);
- `enumerateC4` and `enumerateK4` (no catalog source; enumeration counterparts of the counts);
- `enumerateTriangleEdges` beside `enumerateTriangles`, with the edge IDs of every combination of parallel copies (tko919 `EnumTriangle` passes edge indices).

`trussDecomposition` (NetworkX `k_truss`) became a planned operation of row 42 (package P051, same decision). The other candidates stay out (user decision 2026-10-29):

- Totals of the path counts (`sum / 2`) and other pointwise sums of the per-vertex and per-edge counts: projections. The C4 total is kept as `countC4`. A weighted `averageClustering` would need a flag choosing between operations; it is the mean of `localClusteringWeighted`.
- NetworkX `all_triangles`: the enumeration.
- Enumerating cliques, counting all cliques and cycles over vertex subsets: rows 49 and 53. Elementary cycles and girth: row 68. Subgraph isomorphism: row 54.
- Connected 4-edge subgraph counts (OI Wiki, Gym 102028L) and CF 985G: problem-specific inclusion-exclusion.

## Omission sweep 2026-11-08

Left out by the 2026-11-08 sweep (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded decisions): exact duplicates, second implementations and items another row owns. Handoff notes name where a candidate is implemented instead.

- `PlusMinusOneRMQ` in row 07 (EI, CPA): Data Structures row 54 keeps it public (duplicates.md, Keeps); `EulerLCA` calls it instead of keeping a copy of its own.
- `kShortestWalksAll` (k-pop Dijkstra, every target) (OI): the same k-pop Dijkstra as row 32 `kShortestWalksSimple`, which row 32 lists for every target.
- `optimalTopologicalOrder` (tree precedence) (TKO, LC, MAS): another name for row 79 `rootedTreeMinimumInversionOrder` (an in-tree reduces to it by reversal); removed from row 79.
- `Unicyclic: distance` (MAS): implement through row 41 `CactusDecomposition: distance` (a unicyclic component is a cactus); row 41 says so.
- Random graph generators `randomTree` (Prüfer), `randomBinaryTree`, `randomGraph` and the other maspypy and OGDF generators (MAS, OGDF): test generators: `09-Contest Testing` plans seeded graph generators for when a task needs them (`00-index.md`, Coverage and extension inventory), so no Graphs row lists them.

## P042 GR04 omissions

In the P042 records, a "contributor proposal" is mattblferrer's call, on which the maintainer has not ruled (`/merge-pr 4`, 2026-11-09): it does not bind.

The 2026-11-03 sweeps ([sources](00-sources.md#pages-fetched-on-2026-11-03-p042-gr04)) added `top2Dijkstra` (maspypy) to row 13 and put suisen's O(n^3 / w) bitset closure behind `minimaxFloydWarshall`. `apspPath` became `AllPairsResult::path` (user decision 2026-11-09). The other candidates stay out:

- NetworkX `floyd_warshall_tree`: same O(n^3) worst case; only its expected bound on random complete digraphs is better.
- Shortest walk with at most k arcs (cp-algorithms): `fixedLengthShortestPath` on the graph with a zero-weight loop added at every vertex.
- Fixed-length walk counts (cp-algorithms): an ordinary matrix power (Core Matrix). The min-plus product moves to Core Matrix `mulSemiring` once that row is verified.
- All-pairs Bellman–Ford and the enumeration of all shortest paths (NetworkX): the first is not lighter, wider or more capable than `floydWarshall`, which takes the same negative weights; the second is a bounded enumeration over `shortestPathDag`.
- Wiener, hyper-Wiener, Schultz and Gutman indices and the harmonic diameter (NetworkX): one-line folds of the distance matrix. Resistance distance and the Kemeny constant need a Laplacian pseudo-inverse (Mathematics 16).
- Goldberg–Radzik (NetworkX) and the SLF, LLL, stack and heap SPFA variants (OI Wiki): none is lighter, wider, measured faster or more capable than `spfa` (`01-principles.md`, Priority); `desopoPape` represents the deque heuristics.
- Least or greatest arc count among shortest paths (CSES Investigation): a DP over `shortestPathDag`.
- Minimum-weight cycle by Floyd and Floyd transitive closure (OI Wiki): rows 68 and 40.

## P042 GR05 omissions

The 2026-11-03 sweeps ([sources](00-sources.md#pages-fetched-on-2026-11-03-p042-gr05)) added `BfsNumbering: levelRange, diskRanges` and `Rerooting: get(v, p)` to row 14, and `maxPath`, `TreeMonoid: get, set` and `edgeVertex` to row 15. The other candidates stay out (contributor proposal 2026-11-04, `/finish GR05`):

- Invertible-merge rerooting (suisen): the same O(n) bound and results; it only drops the prefix and suffix products.
- HLD `jump` and median `meet(a, b, c)`: row 07 (`LCA::jump`); `meet` is filed as a row 07 follow-up in P011's plan note (user decision 2026-11-09).
- A weighted tree center inside an edge (maspypy `tree_center`): `treeEccentricities` reports vertex centers; the point center is row 75 `absoluteCenter`.

Tree-DP presets, swept 2026-11-08 ([sources](00-sources.md#pages-fetched-on-2026-11-08-p042-tree-dp-presets)): `subtreeDepthSum` (maspypy) is the only shipped preset row 14 owns. `subtreeHash` (maspypy) is row 17's, and the static `tree_path_composite_sum` is row 84's `pathCompositeSum`. Connected-set counts, linear-extension counts, vertex-offset eccentricities, tour lengths and weighted independent sets appear only in catalog tests or documentation, as single `Rerooting` instances.

## P042 GR70 omissions

The 2026-11-04 sweep ([sources](00-sources.md#pages-fetched-on-2026-11-04-p042-gr70)) found no operation to add to row 85. The candidates stay out:

- Macro-micro O(n) preprocessing (Bender–Farach-Colton): filed as row 94 `LinearLevelAncestor`, package P277, because the paper was not readable (user decision 2026-11-09); row 85 keeps jumps at long-path bottoms, O(n + l * log(n)).
- Online leaf addition (Dietz): no swept catalog ships it, so it is not a standard alternative under `01-principles.md`, Priority.
- `query(v, d)` by target depth: `query(v, dep[v] - d)`.

## P275 re-review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per operation, 2026-11-08), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). The items below returned to rows and supersede the earlier bullets that name them; every other omission recorded here stands ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)).

- `01-graph.hpp`: inDegrees; edgeId (u, v).
- `02-traversal.hpp`: classifyArcs (tree, back, forward, cross).
- `03-toposort.hpp`: topologicalSortLargest; topologicalGenerations (Kahn layers, Mirsky).
- `05-shortest_path.hpp`: zeroOneBfs ({0, c} weights); voronoiCells (centers).
- `12-all_pairs_shortest_path.hpp`: fixedLengthShortestPathAtMost; wienerIndex, hyperWienerIndex, schultzIndex, gutmanIndex; harmonicDiameter.
- `13-shortest_path_advanced.hpp`: onSomeShortestPath (s, t); shortestPathsAll (bounded); shortestPathArcExtremes.
- `14-tree_algorithms.hpp`: connectedSubtreeCounts; treeLinearExtensionCounts; treeEccentricities (vertex offsets); treeMaximumWeightIndependentSet.
- `18-twosat.hpp`: TwoSat::addNand.
- `19-max_flow.hpp`: MaxFlow::minCutFewestEdges.
- `21-matching_bipartite.hpp`: maximumMatchingLexicographic; konigVertexCoverLexicographic (order).
- `23-assignment.hpp`: bottleneckAssignment (threshold search over hopcroftKarp).
- `24-dominatortree.hpp`: DominatorTree::dominatedCount; postDominatorTree (g, exit).
- `25-directedmst.hpp`: kBestArborescences; maximumArborescence; DirectedMst::arborescenceAnyRoot.
- `27-blockcuttree.hpp`: BlockCutTree::mandatoryVertexCount (vertex set).
- `28-cycle_basis.hpp`: CycleBasis::inCycleSpace, CycleBasis::cycleSpaceCoordinates, CycleBasis::enumerateCycleSpace.
- `30-flow_with_demands.hpp`: mixedEulerCircuit (mixed directed and undirected edges, by feasibleFlow).
- `31-path_cover.hpp`: minimumChainCover (digraph with cycles).
- `35-subgraph_enumeration.hpp`: averageClusteringWeighted.
- `62-planarity.hpp`: planarCanonicalOrdering; straightLineDrawing; isStraightLineDrawing; makeBiconnectedPlanar; makeMaximalPlanar.
- new Esoteric row `95-graph_problems.hpp`: IncrementalCentroid: addLeaf, centroid; GridDecrementalConnectivity: erase (cell), isSameSet, count; EdgeCentroidDecomposition: build; shortestPathThroughVertices; maximumVertexEdgeMatching; cycleCommonVertices; treeFromEulerTour; countFourEdgeSubgraphs; outerFaceMultiTerminalCut; antichains (bounded enumeration) (MAS, OI, CF, NX).
- new Esoteric row `96-flow_problems.hpp`: cyclicMinCostBFlow; longestShortestPath; minCostMatchingOnLine; unbalancedTransportation (MAS).
- new Esoteric row `97-network_measures.hpp`: effectiveResistanceApprox, kemenyConstantApprox, effectiveGraphResistanceApprox; betweennessCentralityApprox, closenessCentralityApprox, harmonicCentralityApprox; isDSeparator, moralGraph; isAtFree, isPerfectGraph; richClubCoefficientApprox, sMetric, degreeAssortativityApprox; louvainCommunities, labelPropagationCommunities, modularityApprox; pageRankApprox, hitsApprox; jaccardCoefficientApprox, simRankApprox; snapAggregation (summarization); smallWorldSigmaApprox, smallWorldOmegaApprox; effectiveSizeApprox, constraintApprox (structural holes); doubleEdgeSwap (degree-preserving); harmonicFunction, localAndGlobalConsistency (node classification) (NX).
- Fixed-length walk counts are row 53 `countWalks`; lexicographic bipartite matching is row 21; NetworkX has no vertex-connectivity augmentation (only `k_edge_augmentation`); the GR13, GR27 and GR32 confirmations of 2026-10-28/29 yield to the 2026-11-08 adoption (user decision 2026-11-10).
- Not returned after the P275 review: `TwoSat::answerLexicographic` and `maxCostFlow` wait for their source questions (`omissions.md`, Open questions, P043 and P045); `effectiveResistanceApprox` takes Mathematics row 23's word.
- Not returned after the P275 review: `TwoSat::answerLexicographic` and `maxCostFlow` wait for their source questions (`omissions.md`, Open questions, P043 and P045); `effectiveResistanceApprox` takes Mathematics row 23's word. Row 97 also takes the NetworkX families `Docs/00-sources.md` left out as "not contest algorithms" (user decision 2026-11-10).

## P010 GR01 omissions (2026-11-14)

The GR01 sweeps ([sources](00-sources.md#fetched-2026-11-14-p010-gr01)) left nothing out of rows 01–04 (user decision 2026-11-14: every candidate is adopted). Placed elsewhere: `egoGraphWeighted` and `criticalPath` in row 05 (a Dijkstra radius and the AOE longest-path passes); `attractingComponents` and `isSemiconnected` in row 08; the one-problem `inverseLineGraph`, `pathCycleDecomposition` and `bipartiteAfterEdgeRemoval` in row 95; `stochasticGraphApprox` (floating-point arc weights, which `Graph` cannot hold) in row 97. Aliases only: NetworkX `contracted_nodes`, `quotient_graph` (`contract`, `simplify`), `non_edges` (`complementGraph`), `union`/`disjoint_union` (`disjointUnion`), the `*_all` forms (repeated binary calls), `create_empty_copy` (the constructor), maspypy `rearrange` (`inducedSubgraph`).

Copied code (`03-cpp.md`, One API per operation): `egoGraph` runs its own radius-bounded BFS instead of calling `bfs`, so that it costs O(n + d) for the degree sum `d` of the ball rather than a scan of the whole component. `graphPower` keeps its own bounded BFS for the same reason.
