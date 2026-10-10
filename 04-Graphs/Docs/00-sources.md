# 04 Graphs — sources

Catalog inspection establishes operation names and testable families, never implementation correctness.

## Pages fetched on 2026-10-06

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker home | https://judge.yosupo.jp/ | 2026-10-06 | Returned no problem list through the fetch tool; replaced by the problem repository below |
| Library Checker graph problems | https://github.com/yosupo06/library-checker-problems/tree/master/graph | 2026-10-06 | 38 problem ids mapped to rows 02, 08, 09, 10, 21, 22, 23, 24, 25, 26, 32, 33, 34, 35, 36, 43, 49, 50, 53, 60, 64, 65, 67, 70, 71, 74, 75, 76 |
| Library Checker tree problems | https://github.com/yosupo06/library-checker-problems/tree/master/tree | 2026-10-06 | 20 problem ids mapped to rows 07, 14, 15, 16, 17, 79, 84 and to Data Structures 27, 37, 39, 55 |
| cp-algorithms index | https://cp-algorithms.com/ | 2026-10-06 | Graph section article list: fixed-length shortest paths (12), D'Esopo–Pape (13), second-best MST (52), Prüfer (14), online bridges (26), edge/vertex connectivity (45), MPM (not adopted) |
| AtCoder Library index | https://atcoder.github.io/ac-library/production/document_en/ | 2026-10-06 | Module list |
| ACL maxflow | https://atcoder.github.io/ac-library/production/document_en/maxflow.html | 2026-10-06 | `flow (limit)`, `min_cut`, `get_edge`, `edges`, `change_edge` for row 19 |
| ACL mincostflow | https://atcoder.github.io/ac-library/production/document_en/mincostflow.html | 2026-10-06 | `flow (limit)`, `slope`, `edges` for row 20 |
| ACL scc | https://atcoder.github.io/ac-library/production/document_en/scc.html | 2026-10-06 | Confirms row 08 coverage |
| ACL twosat | https://atcoder.github.io/ac-library/production/document_en/twosat.html | 2026-10-06 | `satisfiable`, `answer` for row 18 |
| KACTL graph chapter | https://github.com/kth-competitive-programming/kactl/tree/main/content/graph | 2026-10-06 | 29 headers: CompressTree (14), MaximumIndependentSet/MaximalCliques/MaximumClique (49), MinimumVertexCover (21), GeneralMatching Tutte-matrix (22), EdgeColoring (34), GlobalMinCut (47), GomoryHu (46), DirectedMST (25) |
| OI Wiki graph section | https://oi-wiki.org/graph/ | 2026-10-06 | Tree center/centroid/diameter (14), tree hashing (17), difference constraints (44), node splitting (45), ring counting (35), minimum cycle (68), chordal (33), planar (29/62), LGV and matrix tree (53), dominator (24), stable matching (38); modular shortest path and random walks not adopted |
| Nyaan library index | https://nyaannyaan.github.io/library/ | 2026-10-06 | graph/ and tree/ paths: namori (41), dimension-expanded graph (43), offline dynamic connectivity (26), auxiliary tree, rerooting, Prüfer, Euler tour, tree-query (14), dynamic diameter and dynamic rerooting (84), rooted tree hash (17), process-of-merging-tree (06); the flow/ directory page returned 404 |
| maspypy library index | https://maspypy.github.io/library/ | 2026-10-06 | graph/, graph/bitset, graph/blackbox, graph/count, graph/ds, graph/shortest_path, graph/tree_dp, flow/: fast LCA, find C4/even cycle, K4 and P3–P5 counts, vertex cover, maximum antichain, line-graph matching and path/cycle decomposition, unicyclic graph, blackbox complement/interval/MST, bitset BFS/SCC/closure/reduction, count/* families, fundamental cut, range edge components, remove-one-edge/vertex connectivity, top2 Dijkstra, dual MCF, dense bipartite matching, incremental maxflow, min-cost b-flow, rank-maximal matching |
| ei1333 library index | https://ei1333.github.io/library/ | 2026-10-06 | graph/* paths: three-edge components (74), bipartite flow with Dulmage–Mendelsohn (21), capacity-scaling Dinic and push-relabel (19), primal-dual MCF (20), extreme vertex sets (83), dynamic star min cut (76), offline DAG reachability (40), complement shortest path (43), radix-heap Dijkstra (13), k-shortest path/walk (59/32), tree decomposition width 2 (50), static top tree DP (84) |
| suisen library index | https://suisen-cp.github.io/cp-library-cpp/ | 2026-10-06 | library/graph and library/tree: level ancestor (85), minmax Floyd–Warshall (12), remove multiedges (01), SCC tournament (77), segment-tree graph (43), tree decomposition tw2 DP (50), contour sums (16), tree isomorphism classification (17), rerooting invertible (14) |
| hitonanode library index | https://hitonanode.github.io/cplib-cpp/ | 2026-10-06 | graph/, flow/, tree/, combinatorial_opt/: extended block-cut trees (27), shortest cycle 0/1 (68), segment-edge shortest path (43), bitset SCC (40), b-flow and cost scaling (65), network simplex (66), lower-bound maxflow (30), submodular optimization via cut (37), edge-disjoint spanning forests and matroid union/parity (48), linear sum assignment (23) |
| tko919 library index | https://tko919.github.io/library/ | 2026-10-06 | Graph/: bipolar (60), chromatic polynomial (67), contour (16), count Euler/spanning (53), DM decomposition (21), general weighted matching (64), optimal toposort (79), shortest path with removed edge (52), static top tree (84), Steiner (36) |
| noshi91 library index | https://noshi91.github.io/Library/ | 2026-10-06 | Three-edge-connected component decomposition (74), incremental bridge connectivity (26); potentialized union find is Data Structures 29 |
| Koosaga library codes | https://github.com/koosaga/olympiad/tree/master/Library/codes | 2026-10-06 | Directory list only; subdirectories fetched separately |
| Koosaga graph | https://github.com/koosaga/olympiad/tree/master/Library/codes/graph | 2026-10-06 | 20 files: chordal (33), count 4-cycles (35), disjoint spanning trees (48), dominator (24), Vizing and bipartite edge coloring (34), extreme vertex sets (83), k shortest paths (59), negative-cycle scaling O(m√n log W) (13), offline dynamic MST (71), st-order (60), tree decomposition width 2 (50), tree isomorphism (17), triedge connectivity (74) |
| Koosaga combinatorial optimization | https://github.com/koosaga/olympiad/tree/master/Library/codes/combinatorial_optimization | 2026-10-06 | 13 files: Dinitz/Gomory–Hu (19/46), cost flows (20/65), bipartite and general matching (21/22), weighted matching dense/sparse (64), skew-symmetric flow (82); LP files are Mathematics 28 |
| NetworkX algorithm reference | https://networkx.org/documentation/stable/reference/algorithms/index.html | 2026-10-06 | 72 category headings: chains and ear decompositions (60), cores and greedy coloring (42), covering (21/22), dominance frontier (24), dominating sets (49), graphical degree sequences (39), isomorphism VF2/WL (54), simple paths (59), time dependent (55), tournament (77), walks (53), threshold graphs (87); centrality, communities, link analysis, planar drawing and similarity not adopted |
| OGDF decomposition group | https://ogdf.github.io/doc/ogdf/group__decomp.html | 2026-10-06 | 9 classes: static/dynamic SPQR trees (61), DynamicBCTree (27 incremental block-cut tree); GraphReduction and planar SPQR variants not adopted |

## Pages fetched on 2026-10-07 (P010 re-audit)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| maspypy graph base | https://maspypy.github.io/library/graph/base.hpp | 2026-10-07 | Degree arrays, endpoint edge lookup and `rearrange` with kept edge IDs (row 01; `inducedSubgraph` keeps an edge map, the rest omitted in 80-notes) |
| maspypy find_cycle, find_odd_cycle | https://maspypy.github.io/library/graph/find_cycle.hpp | 2026-10-07 | Minimal cycle and directed odd cycle candidates, omitted for row 68 |
| Nyaan graph template and utility | https://nyaannyaan.github.io/library/graph/graph-template.hpp | 2026-10-07 | Row 01 storage comparison; tree utilities belong to row 14 |
| cp-algorithms, Depth-first search | https://cp-algorithms.com/graph/depth-first-search.html | 2026-10-07 | Entry/exit times and edge classification, derivable from `order`/`postorder` |
| cp-algorithms, Breadth-first search | https://cp-algorithms.com/graph/breadth-first-search.html | 2026-10-07 | Shortest-path DAG vertices/edges (row 13) and BFS tie order |
| OI Wiki, 拓扑排序 | https://oi-wiki.org/graph/topo/ | 2026-10-07 | Unique-order check (`TopologicalResult::unique`), lexicographic orders |
| OI Wiki, 最小生成树 | https://oi-wiki.org/graph/mst/ | 2026-10-07 | Kruskal reconstruction tree DFS-order leaf intervals (`leafRange`), maximum spanning forests |
| OI Wiki, 同余最短路 | https://oi-wiki.org/graph/mod-shortest-path/ | 2026-10-07 | Congruence shortest path, candidate for row 13 |
| hitonanode shortest_path | https://hitonanode.github.io/cplib-cpp/graph/shortest_path.hpp | 2026-10-07 | 0/c-weight BFS, omitted as a scaling of `zeroOneBfs` |
| maspypy minimum_spanning_tree | https://maspypy.github.io/library/graph/minimum_spanning_tree.hpp | 2026-10-07 | MST operation list; cycle data and second-best MST belong to row 52 |

## Pages fetched on 2026-10-07 (P011 re-audit)

Fetched by the completeness sweeps for rows 07–11; Library Checker task lists were read from the GitHub problem repository because the judge site needs JavaScript.

| Source | URL | Fetched | Used for |
|---|---|---|---|
| hitonanode binary lifting | https://hitonanode.github.io/cplib-cpp/other_algorithms/binary_lifting.hpp | 2026-10-07 | Path monoid products on lifting tables (`LCAFold::pathFold`); ancestor predicate search left to row 15 |
| maspypy tree | https://maspypy.github.io/library/graph/tree.hpp | 2026-10-07 | `path_intersection` and `meet` (`pathIntersection`; `meet` equals `rerootedLCA`) |
| maspypy fast_lca, ei1333 doubling LCA, Nyaan tree-query, suisen LCA | https://maspypy.github.io/library/graph/fast_lca.hpp | 2026-10-07 | Row 07 operation comparison; nothing else missing |
| OI Wiki, 最近公共祖先 | https://oi-wiki.org/graph/lca/ | 2026-10-07 | DFS-order LCA, recorded as a constant-factor alternative only |
| OI Wiki, 欧拉图 | https://oi-wiki.org/graph/euler/ | 2026-10-07 | Lexicographically smallest Euler trail by greedy Hierholzer (`eulerianTrail(..., true)`) |
| maspypy euler_walk, suisen directed_eulerian_graph | https://maspypy.github.io/library/graph/euler_walk.hpp | 2026-10-07 | Row 10 comparison; existence and start modes already covered |
| cp-algorithms, SCC and Euler path; ACL scc; OI Wiki SCC | https://cp-algorithms.com/graph/strongly-connected-components.html | 2026-10-07 | Row 08 comparison; incremental, tournament and bitset SCC owned by rows 70, 77, 40 |
| suisen low_link | https://suisen-cp.github.io/cp-library-cpp/library/graph/low_link.hpp | 2026-10-07 | `connected_component_num_if_removed` (`LowlinkResult::cut_components`) |
| Library Checker two_edge_connected_components, biconnected_components | https://github.com/yosupo06/library-checker-problems/tree/master/graph | 2026-10-07 | Output formats already covered by `components` and `block_vertices` |
| OI Wiki 割点和桥, 双连通分量; KACTL BiconnectedComponents; Nyaan, maspypy, ei1333, hitonanode lowlink | https://oi-wiki.org/graph/cut/ | 2026-10-07 | Row 09 comparison |
| suisen functional_graph | https://suisen-cp.github.io/cp-library-cpp/library/graph/functional_graph.hpp | 2026-10-07 | `kth_iterate` for all vertices in O(n) (`FunctionalGraph::jumpAll`) |
| maspypy functional_graph and doubling | https://maspypy.github.io/library/graph/functional_graph.hpp | 2026-10-07 | `jump_all`, `max_jump`/`max_step` (`FunctionalGraphFold::maxStep`) |
| hitonanode doubling | https://hitonanode.github.io/cplib-cpp/other_algorithms/doubling.hpp | 2026-10-07 | First step at which a monotone condition holds, reduced to `maxStep` |
| CSES problem set | https://cses.fi/problemset/ | 2026-10-07 | Planets Queries/Cycles operations, covered by `jump`, `depth` and cycle lengths |

## Pages fetched on 2026-10-09 (P043)

Fetched by the completeness sweeps for rows 18, 21, 23 and 24. The judge site rendered only titles, so Library Checker statements came from the GitHub problem repository where available (`two_sat` task.md returned 404).

| Source | URL | Fetched | Used for |
|---|---|---|---|
| KACTL 2sat.h | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/2sat.h | 2026-10-09 | `addVar`, `setValue`, O(k) at-most-one with prefix variables |
| ACL twosat; cp-algorithms 2-SAT; OI Wiki 2-SAT | https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/twosat.hpp | 2026-10-09 | Row 18 comparison; `satisfiable`/`answer` |
| ei1333 two-satisfiability, Nyaan two-sat, maspypy twosat, suisen two_sat, Koosaga twosat, hitonanode 2sat test | https://ei1333.github.io/library/graph/others/two-satisfiability.hpp | 2026-10-09 | `set_true`/`set_val`/`set` (`setValue`), `add_nand` (an `addClause` form), Horn-SAT (not adopted) |
| maspypy bipartite_matching, bipartite_matching_dense, rank_maximal_bipartite_matching | https://maspypy.github.io/library/flow/bipartite_matching_dense.hpp | 2026-10-09 | Dense bitset Kuhn with cover (`bipartiteMatchingDense`); rank-maximal matching (omitted) |
| ei1333 bipartite-flow | https://ei1333.github.io/library/graph/flow/bipartite-flow.hpp | 2026-10-09 | Edge erase with re-augmentation (`eraseEdge` + `augment`); lexicographic matching and cover (omitted) |
| suisen bipartite_matching; hitonanode bipartite_matching; Nyaan flow-on-bipartite-graph | https://suisen-cp.github.io/cp-library-cpp/library/graph/bipartite_matching.hpp | 2026-10-09 | Incremental solve (`addEdge` + `augment`); automatic bipartition (omitted); capacity edges (`bMatching`) |
| cp-algorithms Kuhn; OI Wiki 二分图最大匹配; Koosaga matching_bipartite | https://cp-algorithms.com/graph/kuhn_maximum_bipartite_matching.html | 2026-10-09 | Row 21 comparison; greedy initialisation recorded as a heuristic |
| hitonanode linear_sum_assignment | https://hitonanode.github.io/cplib-cpp/combinatorial_opt/linear_sum_assignment.hpp | 2026-10-09 | k-best assignments by Murty partitioning with warm duals (`kBestAssignments`); row augmentation (omitted as public API) |
| OI Wiki 二分图最大权匹配 | https://oi-wiki.org/graph/graph-matching/bigraph-weight-match/ | 2026-10-09 | Non-perfect maximum-weight matching (`linearSumAssignment(..., maxCardinality = false)`); dynamic Hungarian (omitted) |
| cp-algorithms Hungarian; SciPy linear_sum_assignment; KACTL WeightedMatching; maspypy and ei1333 hungarian; Library Checker assignment | https://cp-algorithms.com/graph/hungarian-algorithm.html | 2026-10-09 | Row 23 comparison; rectangular and maximize semantics |
| OI Wiki 支配树 | https://oi-wiki.org/graph/dominator-tree/ | 2026-10-09 | Lengauer–Tarjan; DAG LCA method and dominated counts (omitted / documented) |
| Library Checker dominatortree task | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/dominatortree/task.md | 2026-10-09 | Root and unreachable output convention |
| NetworkX dominance, dominance_frontiers | https://networkx.org/documentation/stable/reference/algorithms/dominance.html | 2026-10-09 | `dominanceFrontier` definition |
| maspypy dominator_tree, ei1333 dominator-tree, Koosaga dominator | https://maspypy.github.io/library/graph/dominator_tree.hpp | 2026-10-09 | Row 24 comparison; all compression-only |

## Pages fetched on 2026-10-10 (P044 max flow)

Fetched by the row 19 completeness sweep. Code was written from the algorithms; no implementation was copied.

| Source | URL | Fetched | Used for |
|---|---|---|---|
| ACL maxflow document | https://github.com/atcoder/ac-library/blob/master/document_en/maxflow.md | 2026-10-10 | `MaxFlow` API shape: `addEdge`, `flow(s, t, limit)` resumable, `minCut`, `getEdge`, `edges`, `changeEdge`; multi-push blocking DFS from t |
| KACTL Dinic, PushRelabel, MinCut, EdmondsKarp | https://github.com/kth-competitive-programming/kactl/tree/master/content/graph | 2026-10-10 | Reverse capacity on `addEdge` (`rev_cap`), scaling inside Dinic, highest-label push-relabel with gap; unit-capacity bound |
| cp-algorithms Edmonds–Karp, Dinic, MPM, push-relabel (faster) | https://cp-algorithms.com/graph/dinic.html | 2026-10-10 | Dinic phase/blocking-flow bounds, highest-label O(V^2 sqrt(E)) |
| OI Wiki 最大流, 最小割 | https://oi-wiki.org/graph/flow/max-flow/ | 2026-10-10 | HLPP with BFS initial labels, gap and global relabel; fewest-edge min cut reduction (notes) |
| hitonanode maxflow, maxflow_pushrelabel | https://hitonanode.github.io/cplib-cpp/flow/maxflow_pushrelabel.hpp | 2026-10-10 | Global relabel frequency proportional to m; second phase returning excess so edge flows are a valid flow |
| maspypy maxflow, incremental_maxflow | https://maspypy.github.io/library/flow/maxflow.hpp | 2026-10-10 | `pathDecomposition`, `changeCapacity` with push-back on decrease, edge insertion after flow |
| ei1333 dinic, ford-fulkerson | https://ei1333.github.io/library/graph/flow/dinic.hpp | 2026-10-10 | Row 19 comparison; nothing else missing |
| CSES Police Chase | https://cses.fi/problemset/ | 2026-10-10 | Min cut edge list (`minCutEdges`) |

## Source keys

Keys used in inventory status cells. Earlier audits (2026-09-27) inspected the same catalogs at navigation level; the 2026-10-06 sweep above supersedes those read scopes for this folder.

- **YC**: Library Checker problem repository (graph and tree directories) — independently testable operations.
- **CPA**: cp-algorithms graph section — article-level operation names; implementation-time article review still required.
- **ACL**: AtCoder Library maxflow/mincostflow/scc/twosat documentation — public method names.
- **KACTL**: KACTL `content/graph` headers.
- **OI**: OI Wiki graph navigation.
- **MAS**: maspypy graph/flow/tree catalog paths.
- **NYA**: Nyaan graph/tree catalog paths.
- **EI**: ei1333 graph catalog paths.
- **SUI**: suisen graph/tree catalog paths.
- **HIT**: hitonanode graph/flow/tree/combinatorial_opt catalog paths.
- **TKO**: tko919 Graph catalog paths; weighted-matching source documentation previously inspected.
- **NOS**: noshi91 library index.
- **KOO**: Koosaga `Library/codes/graph` and `combinatorial_optimization` file lists; focused skew-symmetric-flow comments, extreme-set function and tree-packing code inspected in the 2026-09-27 audit.
- **NX**: NetworkX algorithm reference headings; planarity, cycles and eulerize API documentation inspected in the 2026-09-27 audit (eulerize is unweighted; the Edmonds–Johnson reference motivates weighted T-joins).
- **OGDF**: OGDF graph-decomposition class list.
- **CSES**: CSES problem set titles (functional graphs, Hamiltonian flights), 2026-09-27 audit.
- **HDT**: https://github.com/tomtseng/dynamic-connectivity-hdt — README scope and Holm/de Lichtenberg/Thorup 2001 citation, 2026-09-27 audit; connectivity only.
- **AUDIT**: candidate list supplied with the 2026-10-06 rewrite task (level ancestor, linear extensions, exact vertex cover/dominating set, feedback sets, minimum ratio cycle, widest path, shortest-path counting/DAG, planar dual cut, Tutte-matrix matching, convex-cost flow, Hao–Orlin, tree knapsack, Hall violator, graph-class recognition, pseudoforests, degree-constrained MST, Wilson sampling, line graphs, weighted girth); each verified against at least one catalog or standard reference before adoption.
- **STANFORD**: Stanford ACM 2015–16 notebook declarations, 2026-09-27 audit; no new operations adopted from it on 2026-10-06.

## Audit narrative (condensed from the 2026-09-27 index)

The 2026-09-27 completeness audit compared named families and public-operation catalogs, not source correctness; all entries it added were planned and extracted bodies were unchanged. Canonical ownership was fixed then: DSU and dynamic-tree adapters reuse Data Structures, polynomial/determinant/counting kernels reuse Mathematics, Graphs owns combinatorial graph APIs and certificates. The boundary with Miscellaneous small-to-large/DSU-on-tree was clarified using the Nyaan DSU-on-tree catalog entry, and directed degree-sequence realization was made explicit as a flow reduction (paired out-/in-degree copies, admissible-arc capacities, saturating flow, loop/parallel-edge restrictions in the contract). Per-family read scope and caveats are in `00-Guidelines/History/2026-09-27-inventory-audit.md`; monolith extraction evidence is in `00-Guidelines/History/2026-09-27-monolith-transfer.md` and `Ledgers/monolith-map.json`.

## Not adopted

- cp-algorithms MPM max flow: dominated by Dinic and push-relabel in row 19.
- OI Wiki modular shortest path (同余最短路): a modelling pattern over Dijkstra, not a separate operation.
- OI Wiki random walks on trees/graphs: expectation computations belong to Mathematics probability.
- maspypy annulus/tree walk generating functions, characteristic polynomial of tree adjacency, count labeled graph classes, count unlabeled trees: Mathematics generating-function and polynomial work.
- maspypy bracket_graph, decompose_complete, find_path_through_specified, maximum_matching_between_vertex_edge, grid_decremental_connectivity, incremental_centroid, all_cycle_common_vertices, vs_to_es, restore_euler_tour: problem-specific constructions without a reusable family.
- maspypy flow/longest_shortest_path, min_cost_matching_on_line, unbalanced_transportation: specialized LP/MCF reductions without contest-general contracts.
- maspypy graph/ds/tree_wavelet_matrix and mo_on_tree: Data Structures 17 composition and Miscellaneous 10 ownership.
- Nyaan inclusion-tree: nested-interval tree construction, Miscellaneous interval ownership.
- ei1333 Fibonacci-heap Prim/Dijkstra: heap choice is Data Structures; no asymptotic gain for contest sizes.
- hitonanode basepolyhedron and convex_sum: submodular-function machinery beyond graph contracts.
- NetworkX centrality, communities, link analysis, similarity, summarization, assortativity, rich club, small-world, s-metric, structural holes, swap, planar drawing, D-separation, moral graphs, node classification, asteroidal (AT-free) and perfect-graph tests: not contest algorithms or lack a reusable exact engine.
- NetworkX Voronoi cells: already expressed by multi-source Dijkstra parents in row 05.
- OGDF GraphReduction and planar SPQR variants: embedding-aware SPQR is beyond the row 61 contract.
- Koosaga LP simplex/bigint: Mathematics 28.
