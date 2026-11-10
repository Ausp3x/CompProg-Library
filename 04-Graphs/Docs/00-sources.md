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
| maspypy graph base | https://maspypy.github.io/library/graph/base.hpp | 2026-10-07 | Degree arrays, endpoint edge lookup and `rearrange` with kept edge IDs (row 01; `inducedSubgraph` keeps an edge map, the rest omitted in [00-notes.md](00-notes.md#p010-re-audit-omissions)) |
| maspypy find_cycle, find_odd_cycle | https://maspypy.github.io/library/graph/find_cycle.hpp | 2026-10-07 | Minimal cycle and directed odd cycle candidates, omitted for row 68 |
| Nyaan graph template and utility | https://nyaannyaan.github.io/library/graph/graph-template.hpp | 2026-10-07 | Row 01 storage comparison; tree utilities belong to row 14 |
| cp-algorithms, Depth-first search | https://cp-algorithms.com/graph/depth-first-search.html | 2026-10-07 | Entry/exit times and edge classification, derivable from `order`/`postorder` |
| cp-algorithms, Breadth-first search | https://cp-algorithms.com/graph/breadth-first-search.html | 2026-10-07 | Shortest-path DAG vertices/edges (row 13) and BFS tie order |
| OI Wiki, 拓扑排序 | https://oi-wiki.org/graph/topo/ | 2026-10-07 | Unique-order check (`TopologicalResult::unique`), lexicographic orders |
| OI Wiki, 最小生成树 | https://oi-wiki.org/graph/mst/ | 2026-10-07 | Kruskal reconstruction tree DFS-order leaf intervals (`leafRange`), maximum spanning forests |
| OI Wiki, 同余最短路 | https://oi-wiki.org/graph/mod-shortest-path/ | 2026-10-07 | Congruence shortest path (not adopted) |
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

## Pages fetched on 2026-10-13 (P045 min cost flow)

Fetched by the row 20 completeness sweep. Code was written from the algorithms; no implementation was copied. Not fetched: the Library Checker `min_cost_b_flow` page (rendered by script; its `task.md` was read), the last 15k characters of the Nyaan index, and search-only hits (ferin's convex-edge blog, the ABC393 editorial, beet-aizu capacity scaling).

| Source | URL | Fetched | Used for |
|---|---|---|---|
| ACL mincostflow document and source | https://github.com/atcoder/ac-library/blob/master/document_en/mincostflow.md | 2026-10-13 | `addEdge`, `flow(s, t, limit)`, `slope`, `getEdge`, `edges`; dual bound `-(n-1)C <= dual <= 0`; the documented ban on repeated `flow`/`slope` calls, which a member potential removes |
| KACTL MinCostMaxFlow | https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/MinCostMaxFlow.h | 2026-10-13 | `setpi` Bellman–Ford initial potentials for negative costs, no negative cycles |
| maspypy min_cost_flow, min_cost_b_flow, dual_mcf, cyclic_b_flow | https://github.com/maspypy/library/tree/main/flow | 2026-10-13 | `get_potentials` (`potentials`), `DAG=true` initial potentials, `path_decomposition` (row 19's `pathDecomposition`); b-flow and dual variants belong to rows 65 and 44 |
| hitonanode mincostflow, mincostflow_nonegativeloop, b-flow | https://hitonanode.github.io/cplib-cpp/flow/mincostflow.hpp | 2026-10-13 | Linear-time DAG initial potentials; nonnegative-cost contract; pre-saturating negative arcs (b-flow, row 65) |
| ei1333 primal-dual | https://ei1333.github.io/library/graph/flow/primal-dual.hpp | 2026-10-13 | Target-flow form returning -1 (the `flow` limit with a size check) |
| suisen min_cost_flow | https://suisen-cp.github.io/cp-library-cpp/library/graph/min_cost_flow.hpp | 2026-10-13 | `min_cost_arbitrary_flow` (the `below` bound), member potentials kept across calls, DAG/Bellman–Ford/Dijkstra initialisers |
| Koosaga flow_cost_dijkstra, flow_cost_cubic | https://github.com/koosaga/olympiad/tree/master/Library/codes/combinatorial_optimization | 2026-10-13 | SPFA then Dijkstra; cost scaling belongs to row 65 |
| cp-algorithms Minimum-cost flow | https://cp-algorithms.com/graph/min_cost_flow.html | 2026-10-13 | SSP bound and target-flow form |
| OI Wiki 费用流 | https://oi-wiki.org/graph/flow/min-cost/ | 2026-10-13 | Primal–dual with potentials; dense O(n^2) Dijkstra footnote (measured, not adopted); negative cycles need cycle cancelling |
| Library Checker min_cost_b_flow task | https://github.com/yosupo06/library-checker-problems/blob/master/graph/min_cost_b_flow/task.md | 2026-10-13 | b-flow with potentials and infeasibility (row 65) |
| misawa flow library design | https://misawa.github.io/others/flow/library_design.html | 2026-10-13 | Max-gain flow by cost negation (needs negative-cost support) |

## Pages fetched on 2026-10-16 (P056)

Fetched by the row 16 and 17 completeness sweeps. Code was written from the algorithms; no implementation was copied. Not fetched: judge.yosupo.jp problem pages (rendered by script; the `task.md` files were read, and their numeric limits are unrendered placeholders), the suisen `point_add_range_contour_sum` pages, the tko919 file pages, the maspypy unlabeled-tree enumeration pages, the CSES task pages and any cp-algorithms tree-isomorphism article (none found).

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker tree tasks | https://github.com/yosupo06/library-checker-problems/tree/master/tree | 2026-10-16 | `vertex_add_range_contour_sum_on_tree` and `vertex_get_range_contour_add_on_tree` ([l, r) with distance 0 included), `frequency_table_of_tree_distance` (unordered pairs, distances 1..n-1), `rooted_tree_isomorphism_classification` (exact classes, K and ids) |
| maspypy centroid_decomposition, contour_query_range, distance_sum, incremental_centroid, subtree_hash | https://maspypy.github.io/library/graph/centroid_decomposition.hpp | 2026-10-16 | Per-centroid hook (preorder blocks here), binarized subtraction-free contours (`ContourFold`), `DistanceSum`, unweighted contour contract; incremental centroid (not adopted); rerooted subtree hash |
| suisen point_set_range_contour_sum, point_get_range_contour_operate, contour_sum, tree_isomorphism_classification | https://suisen-cp.github.io/cp-library-cpp/library/tree/point_get_range_contour_operate.hpp | 2026-10-16 | Monoid point-set contour fold, commutative-operator contour apply, static contour sum; per-instance classification ids and `classify_rerooting` (`allRootsClassification`) |
| hitonanode centroid_decomposition, tree_isomorphism | https://hitonanode.github.io/cplib-cpp/tree/centroid_decomposition.hpp | 2026-10-16 | Callback over centroid components; all-roots hash |
| Nyaan centroid-decomposition, tree-hash, rooted-tree-hash | https://nyaannyaan.github.io/library/tree/tree-hash.hpp | 2026-10-16 | Centroid tree views; hash prod(x[height] + child) mod 2^61 - 1 (`rootedTreeHash`) |
| ei1333 centroid-decomposition, tree-isomorphism | https://ei1333.github.io/library/graph/tree/tree-isomorphism.hpp | 2026-10-16 | Centroid tree views; unrooted isomorphism test |
| OI Wiki 点分治, 树哈希, AHU | https://oi-wiki.org/graph/tree-divide/ | 2026-10-16 | Weighted path counting with dist <= k (`countPathsWithin`, P4178), edge divide-and-conquer (omitted); AHU canonical names, rerooting hash, automorphism counts mod 998244353 (https://oi-wiki.org/graph/tree-hash/, https://oi-wiki.org/graph/tree-ahu/) |
| cp-algorithms centroid decomposition | https://cp-algorithms.com/graph/centroid_decomposition.html | 2026-10-16 | Paths of length k (a `treeDistanceFrequency` entry) |
| Koosaga tree_isomorphism | https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/tree_isomorphism.cpp | 2026-10-16 | Recursive sort-based classification, per-call ids |
| NetworkX isomorphism | https://networkx.org/documentation/stable/reference/algorithms/isomorphism.html | 2026-10-16 | `rooted_tree_isomorphism` and `tree_isomorphism` return a mapping (`rootedTreeIsomorphism`, `unrootedTreeIsomorphism`) |

## Pages fetched on 2026-10-18 (P047)

Fetched by the row 25, 26, 27 and 31 completeness sweeps. Code was written from the algorithms; no implementation was copied. Not fetched: judge.yosupo.jp problem pages (rendered by script; the `task.md` files were read), upstream KACTL Dilworth notes, the Library Checker problem list for path covers, and the Tarjan (1977) and Gabow–Galil–Spencer–Tarjan papers.

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker directedmst | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/directedmst/task.md | 2026-10-18 | Nonnegative weights, reachable root, output `p_S = S` (`parentEdge[root] = -1` here) |
| KACTL DirectedMST | https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/DirectedMST.h | 2026-10-18 | Skew-heap contraction with rollback reconstruction, O(E log V), -1 when unreachable (`ok`) |
| maspypy directed_mst, dag_path_cover, maximum_antichain, maximum_weighted_antichain | https://maspypy.github.io/library/graph/directed_mst.hpp | 2026-10-18 | Edge-ID output; flow-based chain cover without closure (not adopted); antichain routines on a closed relation (here the closure is internal or avoided) |
| maspypy block_cut, remove_one_vertex_connectivity, remove_one_edge_connectivity, incremental_bridge_connectivity, range_edge_connected_component_query, grid_decremental_connectivity | https://maspypy.github.io/library/graph/block_cut.hpp | 2026-10-18 | Extended numbering (vertices then blocks), pair and size queries after one removal (O(log deg) vertex, O(1) edge), range edge components by link-cut forest plus Fenwick (replaced by bottleneck-forest divide and conquer), grid decremental connectivity (not adopted) |
| ei1333 directed-mst, block-cut-tree, incremental-bridge-connectivity, offline-dynamic-connectivity | https://ei1333.github.io/library/graph/connected-components/incremental-bridge-connectivity.hpp | 2026-10-18 | `find` of the 2-edge-connected representative, component count member, super-vertex trick for a free root |
| hitonanode directed_mst, extended_block_cut_trees, incremental_bridge_connectivity, offline_dynamic_connectivity | https://hitonanode.github.io/cplib-cpp/graph/extended_block_cut_trees.hpp | 2026-10-18 | O(NM) Chu–Liu/Edmonds (adopted on 2026-11-08 into row 25 as `chuLiuEdmonds`), singleton blocks for isolated vertices, `find`, Begins/Ends schedule (DS row 63) |
| Koosaga directed_mst | https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/directed_mst.cpp | 2026-10-18 | Single-root contraction without an unreachable check |
| Nyaan block-cut-tree, offline-dynamic-connectivity | https://nyaannyaan.github.io/library/graph/offline-dynamic-connectivity.hpp | 2026-10-18 | Cut-vertex numbering, `run(add, del, query)` callback form (`run`) |
| tko919 blockcut | https://tko919.github.io/library/Graph/blockcut.hpp | 2026-10-18 | Block-cut tree catalog comparison |
| OGDF BCTree, DynamicBCTree | https://ogdf.github.io/doc/ogdf/classogdf_1_1_dynamic_b_c_tree.html | 2026-10-18 | Incremental `updateInsertedEdge`, `bComponent` (`sameBlock`), `insertNode` (not adopted) |
| cp-algorithms online bridges, deleting in O(log n) | https://cp-algorithms.com/graph/bridge-searching-online.html | 2026-10-18 | Forest over 2-edge-connected components, small-to-large rerooting, `find_2ecc`; offline segment tree over time (https://cp-algorithms.com/data_structures/deleting_in_log_n.html) |
| OI Wiki 最小树形图, 圆方树, 偏序理论 | https://oi-wiki.org/graph/dmst/ | 2026-10-18 | Tarjan contraction tree with O(n) expansion per root (`DirectedMst::arborescence`); block-forest mandatory vertices and simple-path vertex sums (https://oi-wiki.org/graph/block-forest/); Dilworth and Mirsky (https://oi-wiki.org/math/order-theory/) |
| NetworkX branchings, connectivity, dag | https://networkx.org/documentation/stable/reference/algorithms/tree.html | 2026-10-18 | `minimum_spanning_arborescence`, `maximum_branching`, `ArborescenceIterator` (not adopted); `k_edge_augmentation` (two-edge-connectivity augmentation); `antichains`, `antichain_width` |

## Pages fetched on 2026-10-28 (P048 GR13)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| NetworkX cycles | https://networkx.org/documentation/stable/reference/algorithms/cycles.html | 2026-10-28 | `cycle_basis` (Paton; node lists) as the form of `fundamentalCycleBasis`; no fundamental-cut API |
| NetworkX cycle_basis | https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.cycles.cycle_basis.html | 2026-10-28 | Undirected-only contract of row 28 |
| NetworkX cuts | https://networkx.org/documentation/stable/reference/algorithms/cuts.html | 2026-10-28 | No fundamental cut-set function |
| maspypy fundamental_cut | https://maspypy.github.io/library/graph/ds/fundamental_cut.hpp | 2026-10-28 | Dynamic pair switching over tree-edge cuts: planned row 90 |
| OI Wiki linear basis | https://oi-wiki.org/math/linear-algebra/basis/ | 2026-10-28 | Maximum XOR path exercise (Luogu P4151): `XorWalk` |
| AtCoder ABC451 G | https://atcoder.jp/contests/abc451/tasks/abc451_g | 2026-10-28 | Minimum XOR walk and walk-value counting: `XorWalk::minXorWalk`, `count` |
| AtCoder ABC419 editorial | https://atcoder.jp/contests/abc419/editorial/13662 | 2026-10-28 | Cycle-space membership by even degrees and enumeration as path XOR cycles (compositions, not adopted) |
| Gloid on BZOJ 3569 | https://www.cnblogs.com/Gloid/p/9382450.html | 2026-10-28 | Random cut-space labels and GF(2) dependence: `CutSpaceHash::disconnects` |
| maspypy planar_graph | https://maspypy.github.io/library/graph/planar_graph.hpp | 2026-10-28 | Faces from a drawing, outer face 0, left faces and twins: `fromDrawing`, `outer`, `face`, `next` |
| cp-algorithms planar faces | https://cp-algorithms.com/geometry/planar.html | 2026-10-28 | Angle-sorted rotations, face walks, outer face by signed area: `fromDrawing` |
| OI Wiki planar graphs | https://oi-wiki.org/graph/planar/ | 2026-10-28 | Euler formula, dual graph, bridges as dual loops, min cut as dual shortest path (BZOJ 1001): `planarMinCutDual` |
| NetworkX PlanarEmbedding | https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.planarity.PlanarEmbedding.html | 2026-10-28 | Half-edge accessors, `traverse_face`, `check_structure`, `connect_components` |
| Boost planar graphs | https://www.boost.org/doc/libs/1_86_0/libs/graph/doc/planar_graphs.html | 2026-10-28 | Face traversal, make_biconnected/maximal_planar, canonical ordering (not adopted) |
| OGDF ConstCombinatorialEmbedding | https://ogdf.github.io/doc/ogdf/classogdf_1_1_const_combinatorial_embedding.html | 2026-10-28 | `findCommonFace`, left/right face, `isBridge`, `externalFace`: `commonFace`, `outer` |
| OGDF CombinatorialEmbedding | https://ogdf.github.io/doc/ogdf/classogdf_1_1_combinatorial_embedding.html | 2026-10-28 | Embedding edits (not adopted) |
| OGDF DualGraphBase | https://ogdf.github.io/doc/ogdf/classogdf_1_1_dual_graph_base.html | 2026-10-28 | Primal-dual ID correspondence, kept as dual vertex f = face f and dual edge e = edge e |
| OGDF flow group | https://ogdf.github.io/doc/ogdf/group__ga-flow.html | 2026-10-28 | `MaxFlowSTPlanarDigraph` (flow from dual distances, Hassin) and Itai–Shiloach: the flow witness of `planarMinCutDual` |
| OGDF SeparatorLiptonTarjan | https://ogdf.github.io/doc/ogdf/classogdf_1_1_separator_lipton_tarjan.html | 2026-10-28 | Planar separators: planned row 89 |
| maspypy, Nyaan, ei1333, hitonanode, suisen, tko919, KACTL, Library Checker (v3 API problem list) | catalog indexes as in the 2026-10-06 table; https://v3.api.judge.yosupo.jp/problems | 2026-10-28 | No planar-embedding, face, cycle-basis or XOR-path entry beyond those listed above. Planar sweep read maspypy to 300k of 392k characters and Nyaan, ei1333, hitonanode and suisen to 200k; the cycle-basis sweep read maspypy fully and the others to 100k, plus tko919, noshi91, PyRival and the first Koosaga listing page |

Both headers were written independently from these descriptions; no catalog code was copied. Not inspected: the Codeforces 845G statement (HTTP 403), Luogu P4151, BZOJ 1001, NOI2010 海拔, CF 1889E and Luogu P7916 (search snippets only), and the original papers of Hassin (1981), Eppstein (tree–cotree) and Reif.

## Pages fetched on 2026-10-28 (P048 GR27)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| cp-algorithms flows with demands | https://cp-algorithms.com/graph/flow_with_demands.html | 2026-10-28 | Super source/sink reduction of lower bounds (`feasibleCirculation`); minimum flow by binary search on the t→s capacity (not adopted) |
| OI Wiki 上下界网络流 | https://oi-wiki.org/graph/flow/bound/ | 2026-10-28 | Feasible circulation, s–t feasible flow through a t→s arc, maximum and minimum flow by a second residual max flow (`maxFlowWithLowerBounds`, `minFlowWithLowerBounds`) |
| ei1333 maxflow-lower-bound | https://ei1333.github.io/library/graph/flow/maxflow-lower-bound.hpp | 2026-10-28 | `can_flow`, `max_flow`, `min_flow` (negative minimum allowed) |
| maspypy maxflow_with_lower_bound, min_cost_b_flow, cyclic_b_flow | https://maspypy.github.io/library/flow/maxflow_with_lower_bound.hpp | 2026-10-28 | Per-edge flow result in insertion order; `add_source`/`add_sink` supplies; costed b-flow is row 65 (https://maspypy.github.io/library/flow/min_cost_b_flow.hpp); `cyclic_b_flow` not adopted |
| hitonanode maxflow_lowerbound, b-flow | https://hitonanode.github.io/cplib-cpp/flow/maxflow_lowerbound.hpp | 2026-10-28 | Lower-bounded max flow; `B_Flow` accepts signed lower bounds and supplies (https://hitonanode.github.io/cplib-cpp/flow/b-flow.hpp): the signed-bound domain of row 30 |
| Nyaan, suisen, KACTL, NetworkX flow, Library Checker (v3 API problem list) | catalog indexes; https://networkx.org/documentation/stable/reference/algorithms/flow.html; https://v3.api.judge.yosupo.jp/problems | 2026-10-28 | No lower-bound or demand flow entry; `min_cost_b_flow` is row 65. Nyaan and suisen indexes read to 100k characters |

The header was written independently from these descriptions; no catalog code was copied. Not inspected: LEMON `Circulation` (connection refused) and ACL (no bounded-flow API).

## Pages fetched on 2026-10-29 (P048 GR32)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker enumerate_triangles | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/enumerate_triangles/task.md | 2026-10-29 | `triangleProductSum` contract: simple graph, sum of x_a x_b x_c mod 998244353, N, M <= 10^5 (`info.toml`) |
| Library Checker counting_c4 | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/counting_c4/task.md | 2026-10-29 | `countC4PerEdge`: 4-cycles through each edge, parallel edges allowed, N, M <= 3 * 10^5 (`info.toml`) |
| maspypy count_C3_C4, count_P3_P4_P5, count_K4, find_C4 | https://raw.githubusercontent.com/maspypy/library/main/graph/count/count_C3_C4.hpp | 2026-10-29 | Totals and pointwise C3/C4 (`countC3C4PerVertex`); per-vertex paths with 3, 4, 5 vertices (`count_P3_P4_P5.hpp`, `countPathsP3P4P5`); K4 by bitsets on each out-neighbourhood, O(m^1.5 + m^2 / w) (`count_K4.hpp`); one 4-cycle witness (`find_C4.hpp`, CF 1468M) |
| Koosaga count_4_cycles | https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/count_4_cycles.cpp | 2026-10-29 | Per-edge 4-cycle counts with parallel edges by multiplicity |
| OI Wiki 环计数 | https://oi-wiki.org/graph/rings-count/ | 2026-10-29 | Degree-ordered triangle and 4-cycle counting, O(m sqrt(m)); per-edge triangle counts (HDU 6184) |
| ei1333, suisen, hitonanode enumerate_triangles | https://ei1333.github.io/library/graph/others/enumerate-triangles.hpp | 2026-10-29 | Aliases of `enumerateTriangles` (suisen https://suisen-cp.github.io/cp-library-cpp/library/graph/enumerate_triangles.hpp, hitonanode https://hitonanode.github.io/cplib-cpp/other_algorithms/enumerate_triangles.hpp); simple graphs only |
| NetworkX triads | https://networkx.org/documentation/stable/reference/algorithms/triads.html | 2026-10-29 | `triadicCensus`: the 16 triad names and their order |
| NetworkX triads.py | https://raw.githubusercontent.com/networkx/networkx/main/networkx/algorithms/triads.py | 2026-10-29 | `TRICODES`, `TRIAD_NAMES` and the `_tricode` bit order, embedded verbatim in the tester as the census oracle |
| tko919 enumtriangle | https://raw.githubusercontent.com/tko919/library/main/Graph/enumtriangle.hpp | 2026-10-29 | `EnumTriangle(n, es, query)`: degree-ordered orientation, `query(u, v, w, i, j, k)` with the three edge indices; alias of `enumerateTriangleEdges` |
| NetworkX cluster.py | https://raw.githubusercontent.com/networkx/networkx/main/networkx/algorithms/cluster.py | 2026-10-29 | `clustering` (undirected and unweighted directed), `transitivity`, `square_clustering` (the `potential` formula of the current code), `generalized_degree`: contracts and oracles of `localClustering`, `directedClustering`, `transitivity`, `squareClustering`, `generalizedDegree`; weighted clustering (undirected and directed) and `average_clustering`: contracts and oracles of `weightedClustering`, `weightedDirectedClustering`, `averageClustering` |
| Nyaan, KACTL, cp-algorithms | https://nyaannyaan.github.io/library/; https://github.com/kth-competitive-programming/kactl/tree/main/content/graph; https://cp-algorithms.com/ | 2026-10-29 | Nothing in scope. Nyaan index read to 100k characters |

The header was written independently from these descriptions; no catalog code was copied.

## Fetched 2026-11-08 (omission sweep)

Scope `02`–`07` (P254); decisions are in the rows, the folder notes (Omission sweep 2026-11-08) and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded decisions).

| URL | Used for |
|---|---|
| https://networkx.org/documentation/stable/reference/functions.html | 01: `edgeSubgraph` |
| https://networkx.org/documentation/stable/reference/algorithms/operators.html | 01: `complementGraph` |
| https://ogdf.github.io/doc/ogdf/group__graph-generators.html | 01: `complementGraph` |
| https://maspypy.github.io/library/graph/toposort.hpp | 03: `topologicalSort` |
| https://networkx.org/documentation/stable/reference/algorithms/dag.html | 03: `allTopologicalSorts`; 40: `warshallClosure` |
| https://ei1333.github.io/library/structure/others/plus-minus-one-rmq.hpp | 07: `PlusMinusOneRMQ` |
| https://cp-algorithms.com/graph/rmq_linear.html | 07: `PlusMinusOneRMQ` |
| https://oi-wiki.org/graph/lca/ | 07: `SparseTableLCA`, `getLCA` |
| https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/LCA.h | 07: `SparseTableLCA`, `getLCA` |
| https://maspypy.github.io/library/graph/fast_lca.hpp | 07: `SparseTableLCA`, `getLCA` |
| https://ei1333.github.io/library/graph/tree/rmq-lowest-common-ancestor.hpp | 07: `SparseTableLCA`, `getLCA` |
| https://cses.fi/problemset/task/1685 | 08: `strongConnectivityAugmentation` |
| https://cses.fi/problemset/ | 08: `strongConnectivityAugmentation`; 13: `nearestTwoSources`, `mandatoryOnShortestPaths`; 14: `LongPathDecomposition`, `build`, `treeMaximumMatching`; 53: `countDagPaths`; 73: `countHamiltonianPaths`, `knightsTour` |
| https://networkx.org/documentation/stable/reference/algorithms/shortest_paths.html | 12: `apspBfs`; 13: `bidirectionalBfs` |
| https://networkx.org/documentation/stable/reference/algorithms/distance_measures.html | 12: `barycenter` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.distance_measures.centroid.html | 12: `barycenter` |
| https://maspypy.github.io/library/graph/shortest_path/top2_dijkstra.hpp | 13: `nearestTwoSources` |
| https://maspypy.github.io/library/ | 13: `nearestTwoSources`; 14: `subtreeDiameter`, `subtreeMergeDp`; 39: `tournamentFromScores`; 43: `implicitTopologicalSort`; 49: `vertexCoverAtMost`; 53: `countSpanningTreesAllSubsets`; 68: `findOddCycle`; 77: `outerplanarHamiltonianCycle` |
| https://cses.fi/problemset/task/3303 | 13: `nearestTwoSources` |
| https://cses.fi/problemset/task/1203 | 13: `mandatoryOnShortestPaths` |
| https://oi-wiki.org/graph/tree-diameter/ | 14: `treeDiameter` |
| https://oi-wiki.org/graph/hld/ | 14: `LongPathDecomposition`, `build`; 85: `LevelAncestor`, `jump` |
| https://oi-wiki.org/graph/tree-centroid/ | 14: `subtreeCentroids` |
| https://maspypy.github.io/library/graph/tree_dp/subtree_diameter.hpp | 14: `subtreeDiameter` |
| https://maspypy.github.io/library/graph/tree_dp/O_NK_rerooting_tree_dp.hpp | 14: `subtreeMergeDp` |
| https://cses.fi/problemset/task/1130 | 14: `treeMaximumMatching` |
| https://maspypy.github.io/library/graph/tree_dp/rerooting_dp.hpp | 14: `Rerooting`, `dpParent` |
| https://maspypy.github.io/library/graph/tree_dp/subtree_depth_sum.hpp | 14: `Rerooting`, `dpParent` |
| https://nyaannyaan.github.io/library/tree/euler-tour.hpp | 14: `EulerTour`, `pathRanges`; 15: `TreeGroup`, `pathSum` |
| https://suisen-cp.github.io/cp-library-cpp/library/tree/euler_tour.hpp | 14: `EulerTour`, `pathRanges` |
| https://oi-wiki.org/ds/ | 15: `GlobalBalancedTree`, `getLCA` |
| https://oi-wiki.org/ds/global-bst/ | 15: `GlobalBalancedTree`, `getLCA` |
| https://maspypy.github.io/library/graph/tree.hpp | 15: `maxPath` |
| https://maspypy.github.io/library/graph/ds/tree_abeliangroup.hpp | 15: `TreeGroup`, `pathSum` |
| https://github.com/yosupo06/library-checker-problems/tree/master/tree | 15: `TreeGroup`, `pathSum`; 51: `DynamicForest`, `pointSet`, `DynamicForest`, `subtreeFold`; 84: `pathCompositeSum`, `DynamicRerootingDp`, `setEdge`; 85: `LevelAncestor`, `jump` |
| https://oi-wiki.org/basic/prefix-sum/ | 15: `pathAddOffline`, `pathSumStatic` |
| https://cses.fi/problemset/task/1136 | 15: `pathAddOffline`, `pathSumStatic` |
| https://oi-wiki.org/graph/dynamic-tree-divide/ | 16: `MarkedDiameter`, `toggle` |
| https://oi-wiki.org/math/game-theory/impartial-game/ | 21: `graphPathGame` |
| https://maspypy.github.io/library/game/graph_path_game.hpp | 21: `graphPathGame` |
| https://oi-wiki.org/graph/graph-matching/general-match/ | 22: `perfectMatchingAllowedEdges` |
| https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/GeneralMatching.h | 22: `perfectMatchingAllowedEdges` |
| https://hitonanode.github.io/cplib-cpp/graph/general_matching.hpp | 22: `perfectMatchingAllowedEdges` |
| https://oi-wiki.org/graph/dominator-tree/ | 24: `dominatorTreeDag` |
| https://oi-wiki.org/graph/dmst/ | 25: `chuLiuEdmonds` |
| https://hitonanode.github.io/cplib-cpp/graph/directed_mst.hpp | 25: `chuLiuEdmonds` |
| https://arxiv.org/abs/2208.02590 | 25: `directedMstDense` |
| https://www.cnblogs.com/xiao-ju-ruo-xjr/p/6699488.html | 26: `OfflineDynamicConnectivityLct` |
| https://oi-wiki.org/ds/lct/ | 26: `OfflineDynamicConnectivityLct`; 51: `DynamicForest`, `pointSet`, `DynamicForest`, `findRoot`, `DynamicForest`, `subtreeFold` |
| https://oi-wiki.org/graph/kth-path/ | 32: `kShortestWalksAll` |
| https://oi-wiki.org/graph/chord/ | 33: `chordalMaximalCliques`, `chordalMinimumCliqueCover` |
| https://networkx.org/documentation/stable/reference/algorithms/chordal.html | 33: `chordalMaximalCliques`, `completeToChordal` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.chordal.chordal_graph_cliques.html | 33: `chordalMaximalCliques` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.chordal.complete_to_chordal_graph.html | 33: `completeToChordal` |
| https://oi-wiki.org/graph/color/ | 34: `bipartiteEdgeColoring` |
| https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/EdgeColoring.h | 34: `bipartiteEdgeColoring` |
| https://ei1333.github.io/library/graph/others/bipartite-graph-edge-coloring.hpp | 34: `bipartiteEdgeColoring` |
| https://maspypy.github.io/library/graph/bipartite_edge_coloring.hpp | 34: `bipartiteEdgeColoring` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/edgecolor_bipartite.cpp | 34: `bipartiteEdgeColoring` |
| https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/bipartite_edge_coloring/task.md | 34: `bipartiteEdgeColoring` |
| https://hitonanode.github.io/cplib-cpp/graph/minimum_steiner_tree.hpp | 36: `steinerTreeDense` |
| https://maspypy.github.io/library/graph/steiner_tree.hpp | 36: `steinerTree` |
| https://oi-wiki.org/graph/steiner-tree/ | 36: `steinerTree` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.approximation.steinertree.steiner_tree.html | 36: `metricClosureApproximation` |
| https://networkx.org/documentation/stable/reference/algorithms/approximation.html | 36: `metricClosureApproximation`; 50: `treeDecompositionMinDegree`, `treeDecompositionMinFillIn`; 57: `densestSubgraphPeeling`; 73: `metricTspApproximation` |
| https://maspypy.github.io/library/flow/binary_optimization.hpp | 37: `ternarySubmodularTerm` |
| https://hitonanode.github.io/cplib-cpp/flow/submodular_optimization_via_graph_cut.hpp | 37: `ternarySubmodularTerm`, `allOrNothingPenalty`, `binarySubmodularMinimization` |
| https://oi-wiki.org/graph/graph-matching/stable-match/ | 38: `topTradingCycles` |
| https://maspypy.github.io/library/graph/degree_sequence.hpp | 39: `treeFromDegreeSequence` |
| https://networkx.org/documentation/stable/reference/generators.html | 39: `treeFromDegreeSequence` |
| https://maspypy.github.io/library/graph/tournament.hpp | 39: `tournamentFromScores` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.graphical.is_digraphical.html | 39: `kleitmanWang` |
| https://networkx.org/documentation/stable/reference/generated/networkx.generators.degree_seq.directed_havel_hakimi_graph.html | 39: `kleitmanWang` |
| https://networkx.org/documentation/stable/reference/algorithms/graphical.html | 39: `kleitmanWang`, `isMultigraphical`, `isPseudographical` |
| https://maspypy.github.io/library/graph/reachability.hpp | 40: `countReachable` |
| https://maspypy.github.io/library/graph/bitset/transitive_reduction.hpp | 40: `warshallClosure` |
| https://immortalco.blog.uoj.ac/blog/1955 | 41: `CactusDecomposition`, `distance`, `cactusDiameter` |
| https://oi-wiki.org/graph/block-forest/ | 41: `CactusDecomposition`, `distance` |
| https://github.com/maspypy/library/blob/main/graph/is_cactus.hpp | 41: `isCactus` |
| https://github.com/maspypy/library/blob/main/graph/unicyclic_graph.hpp | 41: `Unicyclic`, `distance` |
| https://networkx.org/documentation/stable/reference/algorithms/core.html | 42: `onionLayers` |
| https://github.com/maspypy/library/blob/main/graph/decompose_complete.hpp | 42: `completeGraphDecomposition` |
| https://github.com/maspypy/library/tree/main/graph | 42: `completeGraphDecomposition` |
| https://github.com/maspypy/library/blob/main/graph/blackbox/scc.hpp | 43: `implicitScc` |
| https://github.com/maspypy/library/tree/main/graph/blackbox | 43: `implicitScc` |
| https://github.com/maspypy/library/blob/main/graph/blackbox/toposort.hpp | 43: `implicitTopologicalSort` |
| https://maspypy.github.io/library/graph/blackbox/toposort.hpp | 43: `implicitTopologicalSort` |
| https://hitonanode.github.io/cplib-cpp/graph/grid_graph_template.hpp | 43: `implicitZeroOneBfs` |
| https://hitonanode.github.io/cplib-cpp/graph/segment_edge_shortest_path.hpp | 43: `implicitZeroOneBfs` |
| https://nyaannyaan.github.io/library/graph/dimension-expanded-graph.hpp | 43: `implicitZeroOneBfs` |
| https://github.com/maspypy/library/blob/main/graph/blackbox/mst.hpp | 43: `implicitBoruvka` |
| https://networkx.org/documentation/stable/reference/algorithms/connectivity.html | 45: `isKEdgeConnected`, `isKVertexConnected`, `allMinimumVertexCuts`, `minimumEdgeCut`, `minimumVertexCut`; 46: `GomoryHuTree`, `kEdgeComponents` |
| https://cp-algorithms.com/graph/edge_vertex_connectivity.html | 45: `minimumEdgeCut`, `minimumVertexCut` |
| https://networkx.org/documentation/stable/reference/algorithms/flow.html | 46: `GomoryHuTree`, `kEdgeComponents`; 65: `capacityScalingMinCostFlow`, `minCostBFlow`; 66: `NetworkSimplex`, `totalCost` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.connectivity.stoerwagner.stoer_wagner.html | 47: `stoerWagnerSparse`, `stoerWagnerDense` |
| https://oi-wiki.org/graph/stoer-wagner/ | 47: `stoerWagnerSparse`, `stoerWagnerDense` |
| https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/GlobalMinCut.h | 47: `stoerWagnerSparse`, `stoerWagnerDense` |
| https://hitonanode.github.io/cplib-cpp/combinatorial_opt/matroid_union.hpp | 48: `matroidUnion` |
| https://hitonanode.github.io/cplib-cpp/ | 48: `matroidUnion`; 68: `shortestCycleThroughVertex` |
| https://tko919.github.io/library/Algorithm/matroid.hpp | 48: `weightedMatroidIntersection` |
| https://oi-wiki.org/math/matroid/ | 48: `weightedMatroidIntersection` |
| https://maspypy.github.io/library/graph/find_vertex_cover.hpp | 49: `vertexCoverAtMost` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.approximation.treewidth.treewidth_min_fill_in.html | 50: `treeDecompositionMinDegree`, `treeDecompositionMinFillIn` |
| https://networkx.org/documentation/stable/reference/algorithms/index.html | 50: `treeDecompositionMinDegree`, `treeDecompositionMinFillIn`; 67: `cliqueCover` |
| https://cp-algorithms.com/graph/second_best_mst.html | 52: `secondBestMst` |
| https://oi-wiki.org/graph/mst/ | 52: `secondBestMst` |
| https://tko919.github.io/library/Graph/shortestpathremedge.hpp | 52: `replacementPaths` |
| https://tko919.github.io/library/ | 52: `replacementPaths`; 88: `TreeWaveletMatrix`, `countRange` |
| https://networkx.org/documentation/stable/reference/algorithms/tree.html | 52: `kBestSpanningTrees` |
| https://maspypy.github.io/library/graph/count/count_tree_subgraph.hpp | 53: `countSpanningTreesAllSubsets` |
| https://networkx.org/documentation/stable/reference/algorithms/isomorphism.html | 54: `subgraphMonomorphism`, `allIsomorphisms`, `isomorphism`, `subgraphIsomorphism`, `canonicalForm` |
| https://networkx.org/documentation/stable/reference/algorithms/graph_hashing.html | 54: `canonicalForm` |
| https://www.vldb.org/pvldb/vol7/p721-wu.pdf | 55: `fastestPath`, `shortestTemporalPath`, `earliestArrival` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.approximation.density.densest_subgraph.html | 57: `densestSubgraphPeeling` |
| https://sites.cs.ucsb.edu/~suri/psdir/alenex03.pdf | 59: `kShortestSimplePaths` |
| https://arxiv.org/html/1512.02157v2 | 59: `kShortestSimplePaths` |
| https://github.com/yosupo06/library-checker-problems/tree/master/graph | 60: `stNumbering`; 63: `FullyDynamicConnectivity`, `addValue` |
| https://github.com/yosupo06/library-checker-problems/blob/master/graph/st_numbering/task.md | 60: `stNumbering` |
| https://maspypy.github.io/library/graph/st_numbering.hpp | 60: `stNumbering` |
| https://ogdf.github.io/doc/ogdf/classogdf_1_1_s_p_q_r_tree.html | 61: `SpqrTree`, `typeOf`, `SpqrTree`, `nodeOfEdge` |
| https://en.wikipedia.org/wiki/SPQR_tree | 61: `SpqrTree`, `typeOf`, `separationPairCount`, `separationPairs` |
| https://ogdf.github.io/doc/ogdf/classogdf_1_1_dynamic_s_p_q_r_tree.html | 61: `SpqrTree`, `nodeOfEdge`, `IncrementalSpqrTree`, `addEdge` |
| https://ogdf.github.io/doc/ogdf/group__decomp.html | 61: `IncrementalSpqrTree`, `addEdge` |
| https://www.mathe2.uni-bayreuth.de/EWS/demoucron.pdf | 62: `planarEmbeddingDmp` |
| https://en.wikipedia.org/wiki/Planarity_testing | 62: `planarEmbeddingDmp`; 91: `IncrementalPlanarity`, `addEdge` |
| https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/dynamic_graph_vertex_add_component_sum/task.md | 63: `FullyDynamicConnectivity`, `addValue`, `FullyDynamicConnectivity`, `insertEdge` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/data_structures/HDLT.cpp | 63: `FullyDynamicConnectivity`, `insertEdge` |
| https://networkx.org/documentation/stable/reference/algorithms/matching.html | 64: `maximumWeightMatching`, `minimumWeightMatching` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/combinatorial_optimization/matching_weighted_sparse.cpp | 64: `weightedBlossomSparse` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/combinatorial_optimization/matching_weighted_dense.cpp | 64: `weightedBlossomSparse` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.flow.capacity_scaling.html | 65: `capacityScalingMinCostFlow`, `minCostBFlow` |
| https://oi-wiki.org/graph/flow/min-cost/ | 65: `capacityScalingMinCostFlow`, `minCostCirculation` |
| https://maspypy.github.io/library/flow/min_cost_b_flow.hpp | 65: `minCostBFlow` |
| https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/min_cost_b_flow/task.md | 65: `minCostBFlow`; 66: `NetworkSimplex`, `addEdge` |
| https://tko919.github.io/library/Graph/mincostflow.hpp | 65: `minCostBFlow` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/combinatorial_optimization/flow_cost_cubic.cpp | 65: `minCostBFlow` |
| https://hitonanode.github.io/cplib-cpp/flow/mcf_costscaling.hpp | 65: `minCostBFlow` |
| https://hitonanode.github.io/cplib-cpp/flow/networksimplex.hpp | 66: `NetworkSimplex`, `addEdge`, `NetworkSimplex`, `supplyType`, `NetworkSimplex`, `totalCost` |
| https://github.com/kth-competitive-programming/kactl/tree/main/content/graph | 67: `cliqueCover` |
| https://oi-wiki.org/graph/max-clique/ | 67: `cliqueCover` |
| https://maspypy.github.io/library/graph/find_odd_cycle.hpp | 68: `findOddCycle` |
| https://maspypy.github.io/library/graph/find_cycle.hpp | 68: `findChordlessCycle` |
| https://networkx.org/documentation/stable/reference/algorithms/cycles.html | 68: `findChordlessCycle`, `boundedSimpleCycles`; 72: `minimumCycleBasis` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.cycles.simple_cycles.html | 68: `boundedSimpleCycles` |
| https://oi-wiki.org/graph/min-cycle/ | 68: `weightedGirth` |
| https://hitonanode.github.io/cplib-cpp/graph/shortest_cycle01.hpp | 68: `shortestCycleThroughVertex` |
| https://hitonanode.github.io/cplib-cpp/graph/shortest_cycle.hpp | 68: `shortestCycleThroughVertex` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.cycles.minimum_cycle_basis.html | 72: `minimumCycleBasis` |
| https://cses.fi/problemset/task/1690 | 73: `countHamiltonianPaths` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.approximation.traveling_salesman.traveling_salesman_problem.html | 73: `minimumHamiltonianPath`, `metricTspApproximation` |
| https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.approximation.traveling_salesman.christofides.html | 73: `metricTspApproximation` |
| https://cses.fi/problemset/task/1689 | 73: `knightsTour` |
| https://arxiv.org/abs/2002.04727 | 74: `cutPairs`, `threeEdgeConnectivityCertificate` |
| https://oi-wiki.org/graph/hamilton/ | 77: `tournamentHamiltonianCycle` |
| https://maspypy.github.io/library/graph/outerplanar.hpp | 77: `outerplanarHamiltonianCycle` |
| https://maspypy.github.io/library/graph/shortest_path/nonzero_group_product_shortest_path.hpp | 78: `nonzeroGroupProductShortestPath` |
| https://hitonanode.github.io/cplib-cpp/graph/nonzero_path_of_group_labeled_graph.hpp | 78: `nonzeroGroupProductShortestPath` |
| https://ei1333.github.io/library/graph/shortest-path/shortest-nonzero-path.hpp | 78: `nonzeroGroupProductShortestPath` |
| https://tko919.github.io/library/Graph/opttoposort.hpp | 79: `optimalTopologicalOrder` |
| https://github.com/yosupo06/library-checker-problems/blob/master/tree/rooted_tree_topological_order_with_minimum_inversions/task.md | 79: `optimalTopologicalOrder` |
| https://maspypy.github.io/library/graph/optimal_product_on_tree.hpp | 79: `optimalTopologicalOrder` |
| https://networkx.org/documentation/stable/reference/algorithms/broadcasting.html | 79: `treeBroadcastTime`, `treeBroadcastCenter` |
| https://ei1333.github.io/library/graph/others/extreme-vertex-set.hpp | 83: `ExtremeVertexSets`, `parent` |
| https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/extreme_vertex_sets.cpp | 83: `ExtremeVertexSets`, `parent` |
| https://ei1333.github.io/library/graph/flow/global-minimum-cut-of-dynamic-star-augmented-graph.hpp | 83: `ExtremeVertexSets`, `parent` |
| https://maspypy.github.io/library/graph/ds/static_toptree.hpp | 84: `pathCompositeSum` |
| https://maspypy.github.io/library/graph/ds/dynamic_rerooting_tree_dp.hpp | 84: `pathCompositeSum`, `DynamicRerootingDp`, `setEdge` |
| https://nyaannyaan.github.io/library/tree/dynamic-rerooting.hpp | 84: `DynamicRerootingDp`, `setEdge` |
| https://ei1333.github.io/library/graph/tree/static-top-tree-dp.hpp | 84: `DynamicRerootingDp`, `setEdge` |
| https://oi-wiki.org/dp/dynamic/ | 84: `dynamicDp` |
| https://github.com/yosupo06/library-checker-problems/blob/master/tree/jump_on_tree/task.md | 85: `LevelAncestor`, `jump` |
| https://suisen-cp.github.io/cp-library-cpp/library/tree/level_ancestor.hpp | 85: `PreorderLevelAncestor`, `build` |
| https://ar5iv.labs.arxiv.org/html/1406.7716 | 85: `TreeWeightedAncestor`, `build` |
| https://arxiv.org/abs/2103.00462 | 85: `TreeWeightedAncestor`, `build` |
| https://maspypy.github.io/library/graph/ds/tree_wavelet_matrix.hpp | 88: `TreePathKth`, `sumLess`, `TreePathKth`, `TreeWaveletMatrix`, `countRange` |
| https://ogdf.github.io/doc/ogdf/classogdf_1_1_separator_har_peled.html | 89: `cycleSeparator` |
| https://ogdf.github.io/doc/ogdf/classogdf_1_1_planar_separator_module.html | 89: `cycleSeparator`, `fundamentalCycleSeparator`, `refineSeparator` |
| https://ogdf.github.io/doc/ogdf/classogdf_1_1_separator_dual_f_c.html | 89: `fundamentalCycleSeparator` |
| https://ogdf.github.io/doc/ogdf/namespaceogdf_1_1planar__separators.html | 89: `refineSeparator` |
| https://en.wikipedia.org/wiki/Modular_decomposition | 92: `ModularDecomposition`, `build` |
| https://networkx.org/documentation/stable/reference/algorithms/sparsifiers.html | 93: `greedySpanner`, `baswanaSenSpanner` |
| https://ogdf.github.io/doc/ogdf/group__ga-spanner.html | 93: `greedySpanner`, `baswanaSenSpanner` |

## Pages fetched on 2026-11-03 (P042 GR04)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| cp-algorithms Floyd–Warshall, fixed-length paths | https://cp-algorithms.com/graph/all-pair-shortest-path-floyd-warshall.html; https://cp-algorithms.com/graph/fixed_length_paths.html | 2026-11-03 | Negative-cycle pairs through a vertex k with d[k][k] < 0; exact-k min-plus power and the at-most-k loop reduction |
| cp-algorithms D'Esopo–Pape | https://cp-algorithms.com/graph/desopo_pape.html | 2026-11-03 | Deque rule (never queued: back; processed and improved: front); negative weights without negative cycles; exponential worst case (hence the bellmanFord fallback) |
| suisen minmax_floyd_warshall | https://suisen-cp.github.io/cp-library-cpp/library/graph/minmax_floyd_warshall.hpp | 2026-11-03 | Arcs in weight order with bitset closure of every new pair, O(n^3 / w); `minimaxFloydWarshall` (an explicit stack replaces its recursion) |
| maspypy top2_dijkstra | https://maspypy.github.io/library/graph/shortest_path/top2_dijkstra.hpp | 2026-11-03 | Nearest and second-nearest distinct source; `top2Dijkstra` |
| Koosaga graph listing | https://github.com/koosaga/olympiad/tree/master/Library/codes/graph | 2026-11-03 | File name `negative_cycle_msqrtnlogw.cpp` only (code not read): Goldberg O(m sqrt(n) log(N)) scaling in a contest catalog. `negativeSsspScaling` follows Goldberg's scheme (bit scaling, refine by layer cut or path elimination) without the paper; its proofs are in [13-shortest_path_advanced.md](13-shortest_path_advanced.md) |
| NetworkX shortest paths, distance measures | https://networkx.org/documentation/stable/reference/algorithms/shortest_paths.html; https://networkx.org/documentation/stable/reference/algorithms/distance_measures.html | 2026-11-03 | `johnson`, `floyd_warshall_predecessor_and_distance`, `astar_path`, `bidirectional_dijkstra`, `goldberg_radzik`; eccentricity, radius, diameter, center, periphery |
| OI Wiki shortest paths | https://oi-wiki.org/graph/shortest-path/ | 2026-11-03 | Johnson reweighting, SPFA variants (not adopted), Floyd-based minimum cycle and closure (rows 68, 40) |
| ei1333, Nyaan, hitonanode, KACTL, maspypy indexes | https://ei1333.github.io/library/; https://nyaannyaan.github.io/library/; https://hitonanode.github.io/cplib-cpp/; https://github.com/kth-competitive-programming/kactl/tree/main/content/graph; https://maspypy.github.io/library/ | 2026-11-03 | Aliases: warshall-floyd, radix-heap Dijkstra, SPFA, Dial; ei1333 and Nyaan indexes read to 100k and 200k characters |
| CSES problem set | https://cses.fi/problemset/ | 2026-11-03 | Investigation (count, minimum and maximum arcs over shortest paths) |

The headers were written independently from these descriptions; no catalog code was copied.

## Pages fetched on 2026-11-03 (P042 GR05)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| maspypy tree, tree_monoid, lazy_tree_monoid | https://maspypy.github.io/library/graph/tree.hpp; https://maspypy.github.io/library/graph/ds/tree_monoid.hpp; https://maspypy.github.io/library/graph/ds/lazy_tree_monoid.hpp | 2026-11-03 | Forward and reversed products for noncommutative path folds; edge values at child slots; `max_path` (row 15 `maxPath`); `get_path_decomposition` (ordered `processPath` ranges); rerooted subtree ranges |
| maspypy tree DP, numbering, compress, diameter, centroid, center | https://maspypy.github.io/library/graph/tree_dp/rerooting_dp.hpp; https://maspypy.github.io/library/graph/tree_dp/O_NK_rerooting_tree_dp.hpp; https://maspypy.github.io/library/graph/tree_dp/subtree_depth_sum.hpp; https://maspypy.github.io/library/graph/ds/bfs_numbering.hpp; https://maspypy.github.io/library/graph/compress_tree.hpp; https://maspypy.github.io/library/graph/tree_diameter.hpp; https://maspypy.github.io/library/graph/find_centroid.hpp; https://maspypy.github.io/library/graph/tree_center.hpp; https://maspypy.github.io/library/graph/tree_dp/subtree_diameter.hpp | 2026-11-03 | `subtree_diameter` as a rerooting instance (not adopted); rerooting with merge, add-vertex and add-edge callbacks; `BfsNumbering` level ranges and the radius-r query (`diskRanges`); virtual tree output shape; centroid pair |
| Nyaan HLD, rerooting, auxiliary tree | https://nyaannyaan.github.io/library/tree/heavy-light-decomposition.hpp; https://nyaannyaan.github.io/library/tree/rerooting.hpp; https://nyaannyaan.github.io/library/tree/auxiliary-tree.hpp | 2026-11-03 | Ranges in traversal order for noncommutative path queries; all-roots rerooting answers |
| ei1333 HLD, rerooting, diameter | https://ei1333.github.io/library/graph/tree/heavy-light-decomposition.hpp; https://ei1333.github.io/library/graph/tree/rerooting.hpp; https://ei1333.github.io/library/graph/tree/tree-diameter.hpp | 2026-11-03 | Edge mode excluding the LCA; weighted diameter with a path witness |
| suisen HLD, rerooting | https://suisen-cp.github.io/cp-library-cpp/library/tree/heavy_light_decomposition.hpp; https://suisen-cp.github.io/cp-library-cpp/library/tree/rerooting.hpp; https://suisen-cp.github.io/cp-library-cpp/library/tree/rerooting_invertible.hpp | 2026-11-03 | Forest roots; point get and set by vertex; invertible rerooting (not adopted) |
| hitonanode HLD, rerooting, auxiliary tree, Euler tour | https://hitonanode.github.io/cplib-cpp/tree/heavy_light_decomposition.hpp; https://hitonanode.github.io/cplib-cpp/tree/rerooting.hpp; https://hitonanode.github.io/cplib-cpp/tree/auxiliary_tree.hpp; https://hitonanode.github.io/cplib-cpp/tree/euler_tour.hpp | 2026-11-03 | `get(v, p)` for a directed edge in O(1); online auxiliary tree (not adopted) |
| tko919 HLD, rerooting | https://tko919.github.io/library/Graph/hld.hpp; https://tko919.github.io/library/Graph/rerooting.hpp | 2026-11-03 | Aliases only |
| KACTL CompressTree | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/CompressTree.h | 2026-11-03 | Virtual tree parent of the i-th preorder vertex is the LCA of it and its predecessor |
| cp-algorithms HLD, tree painting | https://cp-algorithms.com/graph/hld.html; https://cp-algorithms.com/graph/tree_painting.html | 2026-11-03 | Heavy-first layout; legacy tree painting as an edge-mode path sum |
| OI Wiki HLD, tree DP | https://oi-wiki.org/graph/hld/; https://oi-wiki.org/dp/tree/ | 2026-11-03 | O(n * k) bound of size-bounded tree knapsack merges; long-path decomposition (planned row 84 `longPathDp`) |
| Library Checker problem list | https://v3.api.judge.yosupo.jp/problems | 2026-11-03 | Tree problems: vertex_add_path_sum, vertex_set_path_composite, vertex_add_subtree_sum, tree_diameter, jump_on_tree |

The headers were written independently from these descriptions; no catalog code was copied. The linear Prüfer encode and decode follow the standard smallest-leaf pointer scheme; no Prüfer page was fetched in this sweep (the 2026-10-06 cp-algorithms and Nyaan index rows list it).

## Pages fetched on 2026-11-04 (P042 GR70)

| Source | URL | Fetched | Used for |
|---|---|---|---|
| suisen level_ancestor | https://suisen-cp.github.io/cp-library-cpp/library/tree/level_ancestor.hpp | 2026-11-04 | Depth-bucket binary search, O(log n) query (not adopted); -1 for k outside [0, depth] |
| OI Wiki long-path decomposition, LCA | https://oi-wiki.org/graph/hld/; https://oi-wiki.org/graph/lca/ | 2026-11-04 | Ladders (long paths extended upward by their length) with 2^h jumps: O(1) k-th ancestor |
| Wikipedia level ancestor problem | https://en.wikipedia.org/wiki/Level_ancestor_problem | 2026-11-04 | Jump-pointer and ladder algorithm; O(n) macro-micro bound (not adopted); depth form LA(v, d) |
| Menghani–Matani | https://ar5iv.arxiv.org/html/1903.01387 | 2026-11-04 | Depth-bucket O(n) / O(log n) method (not adopted); online leaf addition (Dietz) cited (not adopted) |
| Library Checker jump_on_tree; Nyaan and maspypy jump | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/tree/jump_on_tree/task.md; https://nyaannyaan.github.io/library/verify/verify-yosupo-graph/yosupo-jump-on-tree.test.cpp; https://maspypy.github.io/library/graph/tree.hpp | 2026-11-04 | O(1) path jump as LCA plus level ancestor (row 85 `jump`, adopted by the 2026-11-08 sweep) |
| Luogu P5903 | https://www.luogu.com.cn/problem/P5903 | 2026-11-04 | Standard O(1) k-th ancestor workload (n 5e5, q 5e6) |
| ei1333, hitonanode, KACTL, cp-algorithms binary lifting | https://ei1333.github.io/library/; https://hitonanode.github.io/cplib-cpp/; https://github.com/kth-competitive-programming/kactl/tree/main/content/graph; https://cp-algorithms.com/graph/lca_binary_lifting.html | 2026-11-04 | No level-ancestor or offline batched form beyond binary lifting |

The header was written independently from these descriptions; no catalog code was copied. Storing jumps only at long-path bottoms follows the jump-pointer argument of the ladder algorithm; the Bender–Farach-Colton paper itself was not readable (403).

## Source keys

Status-cell keys are the library's (`00-Guidelines/09-sources.md`, Source keys); this folder read under each as follows. Earlier audits (2026-09-27) inspected the same catalogs at navigation level; the 2026-10-06 sweep above supersedes those read scopes for this folder. Per-family read scope and caveats of that audit are in `00-Guidelines/History/2026-09-27-inventory-audit.md`.

- **LC**: Library Checker problem repository (graph and tree directories) — independently testable operations.
- **CPA**: cp-algorithms graph section — article-level operation names; implementation-time article review still required.
- **KACTL**: KACTL `content/graph` headers.
- **OI**: OI Wiki graph navigation.
- **MAS**: maspypy graph/flow/tree catalog paths.
- **NYA**: Nyaan graph/tree catalog paths.
- **EI**: ei1333 graph catalog paths.
- **SUI**: suisen graph/tree catalog paths.
- **WIKI**: Wikipedia article named in the row (2026-11-04: Level ancestor problem).
- **BFC04**: Bender and Farach-Colton, The Level Ancestor Problem simplified, Theoretical Computer Science 321 (2004); not readable on 2026-11-04 (HTTP 403).
- **HIT**: hitonanode graph/flow/tree/combinatorial_opt catalog paths.
- **TKO**: tko919 Graph catalog paths; weighted-matching source documentation previously inspected.
- **NOSHI**: noshi91 library index.
- **KOO**: Koosaga `Library/codes/graph` and `combinatorial_optimization` file lists; focused skew-symmetric-flow comments, extreme-set function and tree-packing code inspected in the 2026-09-27 audit.
- **NX**: NetworkX algorithm reference headings; planarity, cycles and eulerize API documentation inspected in the 2026-09-27 audit (eulerize is unweighted; the Edmonds–Johnson reference motivates weighted T-joins).
- **OGDF**: OGDF class and group documentation (graph decomposition, planar embedding, flow, separators).
- **CSES**: CSES problem set titles (functional graphs, Hamiltonian flights), 2026-09-27 audit.
- **SPOJ**: problem COT (k-th smallest on a tree path), named by the P030 sweep (`02-Data Structures/Docs/00-notes.md`, P030 omissions); not fetched, P248 reads it first.
- **HDT**: https://github.com/tomtseng/dynamic-connectivity-hdt — README scope and Holm/de Lichtenberg/Thorup 2001 citation, 2026-09-27 audit; connectivity only.
- **AUDIT**: candidate list supplied with the 2026-10-06 rewrite task (level ancestor, linear extensions, exact vertex cover/dominating set, feedback sets, minimum ratio cycle, widest path, shortest-path counting/DAG, planar dual cut, Tutte-matrix matching, convex-cost flow, Hao–Orlin, tree knapsack, Hall violator, graph-class recognition, pseudoforests, degree-constrained MST, Wilson sampling, line graphs, weighted girth); each verified against at least one catalog or standard reference before adoption.
- **WIKI**: English Wikipedia articles; URLs in [Fetched 2026-11-08](#fetched-2026-11-08-omission-sweep).

## Not adopted

- OI Wiki random walks on trees/graphs: expectation computations belong to Mathematics probability.
- maspypy annulus/tree walk generating functions, characteristic polynomial of tree adjacency, count labeled graph classes, count unlabeled trees: Mathematics generating-function and polynomial work.
- maspypy find_path_through_specified, maximum_matching_between_vertex_edge, all_cycle_common_vertices, restore_euler_tour: problem-specific constructions without a reusable family. maspypy decompose_complete was adopted on 2026-11-08 into row 42 as `completeGraphDecomposition`, and bracket_graph into Miscellaneous row 53 as `bracketForest`. (returned by P275 2026-11-10 into row 95)
- maspypy flow/longest_shortest_path, min_cost_matching_on_line, unbalanced_transportation: specialized LP/MCF reductions without contest-general contracts. (returned by P275 2026-11-10 into row 96)
- maspypy mo_on_tree: Data Structures 23 (`TreeMo`). (graph/ds/tree_wavelet_matrix was adopted on 2026-11-08 into row 88 as `TreeWaveletMatrix`.)
- Nyaan inclusion-tree: nested-interval tree construction, Miscellaneous interval ownership. (returned by P275 2026-11-10 into Miscellaneous row 09 `inclusionTree`)
- hitonanode basepolyhedron and convex_sum: submodular-function machinery beyond graph contracts. (an Open question of P275: `00-Guidelines/23-Sweep Ledgers/omissions.md`)
- NetworkX centrality, communities, link analysis, similarity, summarization, assortativity, rich club, small-world, s-metric, structural holes, swap, planar drawing, D-separation, moral graphs, node classification, asteroidal (AT-free) and perfect-graph tests: not contest algorithms or lack a reusable exact engine. (returned by P275 2026-11-10 into row 97 (user decision 2026-11-10))
- NetworkX Voronoi cells: already expressed by multi-source Dijkstra parents in row 05. (returned by P275 2026-11-10 into row 05 `voronoiCells`)
- OGDF GraphReduction and planar SPQR variants: embedding-aware SPQR is beyond the row 61 contract.
- Koosaga LP simplex/bigint: Mathematics 28.

## Pages fetched on 2026-11-08 (P042 tree-DP presets)

Read through a summarizing fetch, so code quotes are not byte-checked; catalog indexes were truncated at 100,000 to 200,000 characters.

| Source | URL | Fetched | Used for |
|---|---|---|---|
| maspypy presets | https://maspypy.github.io/library/graph/tree_dp/subtree_depth_sum.hpp; https://maspypy.github.io/library/graph/tree_dp/subtree_hash.hpp | 2026-11-08 | The only shipped presets on a rerooting contract: `subtreeDepthSum` (row 14), `subtreeHash` (row 17) |
| Generic rerooting templates | https://maspypy.github.io/library/graph/tree_dp/rerooting_dp.hpp; https://nyaannyaan.github.io/library/tree/rerooting.hpp; https://ei1333.github.io/library/graph/tree/rerooting.hpp; https://suisen-cp.github.io/cp-library-cpp/library/tree/rerooting.hpp; https://tko919.github.io/library/Graph/rerooting.hpp | 2026-11-08 | No preset in the header |
| suisen tests | https://suisen-cp.github.io/cp-library-cpp/test/src/tree/rerooting/edpc_v.test.cpp; https://suisen-cp.github.io/cp-library-cpp/test/src/tree/rerooting/abc160_f.test.cpp; https://suisen-cp.github.io/cp-library-cpp/test/src/tree/rerooting/abc222_f.test.cpp | 2026-11-08 | Instances in tests, not shipped |
| hitonanode, OI Wiki | https://hitonanode.github.io/cplib-cpp/tree/rerooting.hpp; https://oi-wiki.org/dp/tree/ | 2026-11-08 | Worked examples, not shipped |
| Library Checker | https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/tree/tree_path_composite_sum/task.md | 2026-11-08 | Row 84's `pathCompositeSum` |

## Fetched 2026-11-10 (P275 re-review)

Pages the P275 re-review agents read to confirm the returns in `Docs/00-notes.md` (P275 re-review 2026-11-10).

- <https://networkx.org/documentation/stable/reference/algorithms/voronoi.html>
- <https://networkx.org/documentation/stable/reference/algorithms/wiener.html>
- <https://networkx.org/documentation/stable/reference/algorithms/tree.html>
- <https://networkx.org/documentation/stable/reference/algorithms/dag.html>
- <https://networkx.org/documentation/stable/reference/algorithms/distance_measures.html>
- <https://networkx.org/documentation/stable/reference/algorithms/edge_connectivity.html>
- <https://raw.githubusercontent.com/maspypy/library/main/graph/base.hpp>
- <https://oi-wiki.org/graph/topo/>
- <https://oi-wiki.org/graph/block-forest/>
- <https://oi-wiki.org/graph/tree-divide/>
- <https://cp-algorithms.com/graph/depth-first-search.html>
- <https://cp-algorithms.com/graph/breadth-first-search.html>
- <https://cp-algorithms.com/graph/fixed_length_paths.html>
- <https://en.wikipedia.org/wiki/Linear_bottleneck_assignment_problem>
- <https://en.wikipedia.org/wiki/Dominator_(graph_theory)>
- <https://en.wikipedia.org/wiki/Arboricity>
- <https://www.boost.org/doc/libs/1_86_0/libs/graph/doc/table_of_contents.html>
- <https://arxiv.org/abs/1310.5037>
