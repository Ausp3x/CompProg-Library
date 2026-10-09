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

- A row is verified only when every operation in it has a test with an independent oracle and the recorded commands pass; see the per-header evidence documents `Docs/<NN-name>.md` for the current verified rows.
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

- Existing-unverified headers (`12`, `13`, `15`, `16`, `19`, `26`) contain unchanged monolith excerpts; see `97-Legacy/00-index.md` and `00-Guidelines/History/2026-09-27-monolith-transfer.md`. Compilation does not verify semantics, completeness, performance, indexing or complexity claims. The source-range/hash map `00-Guidelines/Ledgers/monolith-map.json` is authoritative for provenance.
- Legacy-reference rows (`20`, `27`) point at Team Notebook sources `OLD/Team Notebook/src/graph/old_mcmf.cpp` and `old_block-cut-tree.cpp` (`old_kuhn.cpp` is accounted for by the verified row `21`); `old_bellman-ford.cpp` is superseded by `05-shortest_path.hpp`.
- Unchanged archive leads in `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `test.cpp`: `LcaO1`, `LcaLog`, `DEsopoPape`, `TarjanSCC`, `StrongOrientation`, `EulerPath`, `Kruskal`, `PrimDense`, `TreeEdgePainting`, `Dinic`, `FlowWithDemands`, `PushRelabel`, `Kuhn` (symbols vary by file). They are behavioral references, never specifications. `OLD/[1] algorithms.cpp` TODO comments motivated rows `14` and `17`.
- Delete from `OLD` or `97-Legacy` only after an audited implementation accounts for every feature the file contained.

## Placement decisions recorded

- `04-dsu.hpp` stays: it is a verified adapter with a real operation (`graphComponents`), not a bare re-export.
- `51-lct.hpp` stays as a thin adapter row with concrete operations; merging it into Data Structures 37 or `84-dynamictreedp.hpp` is a candidate scoped rename task, not part of this rewrite.
- Shortest-path counting, shortest-path DAG, widest/minimax paths and Goldberg negative-weight scaling were placed in `13-shortest_path_advanced.hpp` rather than reopening the verified `05-shortest_path.hpp`.
- `lineGraph`, `inducedSubgraph`, `contract` and `simplify` reopened `01-graph.hpp` as partial because they are representation utilities; maximum-weight spanning forests reopened `06-mst.hpp` because negating full-range `lng` weights overflows.

## P010 common contract

Applies to `01-graph.hpp` through `06-mst.hpp`. Every algorithm accepts `Graph` or `CsrGraph` unless it names `DenseGraph`. Vertices are `[0, n)`, logical edges `[0, m)` and arcs `[0, arcs.size())`, and every count fits `int`. Weights span the full `lng` range; path and forest totals are exact `lll`. Results hold arc and edge indices into the input graph, so they stay meaningful only while that graph's arc numbering and content are unchanged; result arrays themselves own their storage. Public storage of graphs and results is inspectable but must not be edited behind an algorithm's invariants. Violated preconditions (out-of-range vertex, wrong directedness, negative weights for Dijkstra) are assertions; a valid input with no answer returns a status field or an empty witness. Complexity comments use `m` for the arc count where it differs from the edge count only by a factor of two. Unvisited result fields are `-1`.

## P010 package record

- Scope: the six Basic headers `01-graph.hpp` (GR01) through `06-mst.hpp` (GR02), evidence in [01-graph.md](01-graph.md), [02-traversal.md](02-traversal.md), [03-toposort.md](03-toposort.md), [04-dsu.md](04-dsu.md), [05-shortest_path.md](05-shortest_path.md) and [06-mst.md](06-mst.md). Prerequisites: the verified P002 template and P006 canonical DSU. Implementations are independent contest-profile code; archived snippets remain unchanged.
- First verified on 2026-09-27 with GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. The 2026-10-06 inventory rewrite reopened `01-graph.hpp` and `06-mst.hpp` as partial.
- 2026-10-07 re-audit: the previous verification was treated as existing-unverified. Before any edit the rows were compared with the code and testers, and the unchanged suites passed full mode with seed 20260927 in all three builds. The 14 confirmed findings in `00-Guidelines/23-Reaudit Findings/p010.md` were fixed or resolved (per-header dispositions are in each evidence document). Every header keeps at most two comment lines per declaration and at most 8% comment lines. The six entries compile with `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner, because the P011 header `07-lca.hpp` still had an unused parameter under `NDEBUG`). New probes brought the package to 71 assertion probes, up from 61. Mutation checks were planted in a temporary copy and run in full mode. The suites also ran with GCC 14.4.1 (`CXX=g++-14`); GCC 14.2 itself was not run.
- Independent review (`@reviewer`, 2026-10-07) confirmed findings 1–3, 5 and 7–14 fixed and found no correctness defect. It ran 22 mutants of its own: 17 were killed, and the 5 survivors are equivalent or change only runtime (listed per header). Its three items (stale feature map/probe count/GCC 14 statement, the shared-line resolution of findings 6 and 11, the `contract` bound) were resolved.
- Scope boundaries: P010 does not own all-pairs/advanced/implicit/temporal/ranked shortest paths, SCCs, dynamic graphs, directed MSTs, sensitivity/second-best MSTs, spanning-tree counting, or fully dynamic MSTs. These remain explicitly planned or existing-unverified in their current inventory rows. Reconstruction-tree bottleneck queries belong to P010; general LCA and dynamic-tree APIs retain their separate owners. The exact GCC 14.2 runtime check and fractional/custom-weight domains are not claimed. No automatic judge submissions are part of this package.

## P011 package record

- Scope: GR03 (`07-lca.hpp`, `08-scc.hpp`, `09-bridges_articulation.hpp`, `10-euleriantrail.hpp`) and GR26 (`11-functionalgraph.hpp`), evidence in [07-lca.md](07-lca.md), [08-scc.md](08-scc.md), [09-bridges_articulation.md](09-bridges_articulation.md), [10-euleriantrail.md](10-euleriantrail.md) and [11-functionalgraph.md](11-functionalgraph.md). Prerequisites P002/C01 and P010/GR01 were verified before the package began. GR03 was implemented and passed all its full feature configurations before GR26 implementation began; work within GR03 used disjoint header ownership.
- All headers accept the documented canonical Graph/CSR domains and own their results/preprocessing. Original `OLD` bytes remain unchanged. The monolith map describes the two replaced active headers as audited replacements while retaining historical source hashes. Shared Basic/All aggregates include all five headers. The tester entries are discovered automatically by the filtered/shared runner.
- 2026-09-27 verification: feature suites use GNU++20 with GCC 16.2.1 in optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan `-O1 -g` builds with leak detection; each suite exposes quick/full/stress modes, deterministic seeds, non-removable failure checks and subprocess timeouts. All five headers passed every configuration; 64 checked precondition probes passed in total. Every header covers empty/no-answer inputs, loops/multiedges where permitted, owning snapshots and 200,000-vertex adversarial shapes. Sandbox LeakSanitizer restrictions required approved outside-sandbox sanitizer retries, which passed with leak detection retained. Final integration passed 82 standalone/aggregate headers. Exact GCC 14 execution and extended stress mode were not claimed.
- 2026-10-07 re-audit: the code was treated as existing-unverified. All five headers were brought to the closing-brace rule and the two-line comment cap, the testers to the brace rule, and the removed contract text moved to each document's Contracts. Baseline before any change: all five full suites passed in all three configurations (seed 1). Eight confirmed findings were fixed (per header). The completeness sweep ([sources](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit)) added six operations. Ten hand mutations of the new and changed code were run against the quick suites: nine failed as required; the tenth was an equivalent mutant (see `11-functionalgraph.md`). `@reviewer` confirmed all eight findings fixed and wrote its own brute-force checks for every addition, all passing under ASan/UBSan/`_GLIBCXX_DEBUG`. No online solution was automatically submitted or acceptance claimed.
- Scope boundaries: dynamic SCC, dynamic connectivity/bridge maintenance, block-cut/bridge-tree query wrappers, tree/HLD aggregates, Euler-trail counting and postman augmentation remain with their separate batch owners. Functional successor updates and specialized collision schedules are outside GR26's static contracts. These are explicit ownership boundaries, not unfinished P011 features.

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
- Congruence shortest path (OI Wiki 同余最短路): a reduction to Dijkstra over residues; candidate for `13-shortest_path_advanced.hpp`, not added to that row here.
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

## P043 package record

- Scope: GR07 (`18-twosat.hpp`), GR10 (`21-matching_bipartite.hpp`, `23-assignment.hpp`) and GR11 (`24-dominatortree.hpp`); evidence in [18-twosat.md](18-twosat.md), [21-matching_bipartite.md](21-matching_bipartite.md), [23-assignment.md](23-assignment.md) and [24-dominatortree.md](24-dominatortree.md). Prerequisites P002, P010 and P011 were verified. The unchanged `TwoSat` and `Hungarian` monolith excerpts were replaced, and their legacy interfaces kept as adapters; the monolith map records both as P043 audited replacements. Row 24 was new. Row 21's `old_kuhn.cpp` is defective and served only as a feature list.
- 2026-10-09 verification: the completeness sweeps ([sources](00-sources.md#pages-fetched-on-2026-10-09-p043)) added `addVar`, `setValue`, `addEquivalent` and the O(k) form of `addAtMostOne` to row 18; dynamic `addEdge`/`eraseEdge`/`augment` to row 21; and `kBestAssignments` and the unrestricted-size option with cost curve to row 23. Row 18's `addExactlyOneOfTwo` is the same constraint as `addXor` and was merged into it. Row 21's `hopcroftKarp` and `kuhn` became methods of `BipartiteMatching`, so they can grow any current matching. Row 24's simple algorithm is vertex deletion with a provable O(V·(V + E)) bound instead of Cooper–Harvey–Kennedy iteration.
- All four suites use independent exhaustive or enumerative oracles plus certificate checks (LP duals, König covers, Hall sets, refutation walks). They pass quick, full (GCC 16 and GCC 14.4) and stress in optimized, `_GLIBCXX_DEBUG` and ASan/UBSan builds. Development defects found and fixed: the dominance-frontier stamp grouping and Lengauer–Tarjan `eval` at virtual roots (both caught by the suite), and the `augment` path walk (caught in self-review before the first run). Benchmarking then replaced truncated Hopcroft–Karp phases on edge-ID adjacency with relaxed phases on a CSR snapshot (proof in the evidence), about 6× faster on sparse random graphs. Planted mutants: 26 of 27 killed; the survivor (the balancing swap in `link`) affects only the α bound.
- Independent review (`@reviewer`, 2026-10-09) found no wrong answers and re-verified each algorithm with its own probes: Cooper–Harvey–Kennedy, remove/contract matching-number, Bellman–Ford min-cost-flow and brute-force 2-SAT oracles, plus overflow probes at the stated domain bounds. It confirmed the following, all fixed and covered by new tests: an uninitialised `AssignmentResult` read when copying an unsolved `Hungarian`; untested `mate` and warm-started `hopcroftKarp`/`kuhn`; an unchecked `DominatorTree` constructor range; a wasted BFS under `NDEBUG` in `minimumEdgeCover`; `TwoSat` literals unbounded under `NDEBUG`; the `get`/uppercase-local style; four complexity comments; and evidence wording. The P043 monolith-map entries were set to `verified`.

## P043 omissions

- Lexicographically smallest 2-SAT assignment: needs O(n·m) incremental propagation; no catalog provides it. Incremental 2-SAT, solution counting (#P-hard) and Horn-SAT (suisen) are different problems.
- `addNand` (ei1333): it is `addClause(~a, ~b)`.
- Lexicographically extremal maximum matching and lexicographically smallest vertex cover (ei1333): rare; greedy fixing of allowed edges from `essentialEdges` gives the matching in O(V·(V + E)).
- Rank-maximal matching (maspypy): rare, unweighted-lexicographic; candidate for a future row.
- Automatic bipartition overloads (hitonanode, maspypy): run `bipartiteCheck` (row 02) and map the sides.
- Maximum antichain, DAG path and chain covers: row 31. Bipartite edge colouring: row 34.
- Incremental row addition and dynamic Hungarian cost updates (hitonanode `augment`, OI Wiki): the internal single-row `search` exists, but a public stateful solver with update semantics is not part of any contest catalog interface fetched; rerun `hungarian` or use `kBestAssignments`.
- Bottleneck assignment and the auction algorithm: in no fetched catalog; bottleneck assignment is a binary search over `hopcroftKarp` or `bipartiteMatchingDense`.
- DAG dominator tree by LCA (OI Wiki): Lengauer–Tarjan covers DAGs with a better bound; online vertex appending is not part of this row.
- Dominated counts (OI Wiki P5180): `tout[v] − tin[v]` of `DominatorTree`.
- Post-dominators, multiple roots and edge dominators: reverse graph, super-source and edge subdivision. Exposed semidominators (internal in Koosaga) and iterated dominance frontiers (SSA construction) are outside contest use.

