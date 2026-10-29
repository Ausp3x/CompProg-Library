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
| hitonanode directed_mst, extended_block_cut_trees, incremental_bridge_connectivity, offline_dynamic_connectivity | https://hitonanode.github.io/cplib-cpp/graph/extended_block_cut_trees.hpp | 2026-10-18 | O(NM) Chu–Liu/Edmonds (dominated), singleton blocks for isolated vertices, `find`, Begins/Ends schedule (DS row 63) |
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

## Source keys

Keys used in inventory status cells. Earlier audits (2026-09-27) inspected the same catalogs at navigation level; the 2026-10-06 sweep above supersedes those read scopes for this folder. Per-family read scope and caveats of that audit are in `00-Guidelines/History/2026-09-27-inventory-audit.md`.

- **YC**: Library Checker problem repository (graph and tree directories) — independently testable operations.
- **CPA**: cp-algorithms graph section — article-level operation names; implementation-time article review still required.
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
- **OGDF**: OGDF class and group documentation (graph decomposition, planar embedding, flow, separators).
- **CSES**: CSES problem set titles (functional graphs, Hamiltonian flights), 2026-09-27 audit.
- **HDT**: https://github.com/tomtseng/dynamic-connectivity-hdt — README scope and Holm/de Lichtenberg/Thorup 2001 citation, 2026-09-27 audit; connectivity only.
- **AUDIT**: candidate list supplied with the 2026-10-06 rewrite task (level ancestor, linear extensions, exact vertex cover/dominating set, feedback sets, minimum ratio cycle, widest path, shortest-path counting/DAG, planar dual cut, Tutte-matrix matching, convex-cost flow, Hao–Orlin, tree knapsack, Hall violator, graph-class recognition, pseudoforests, degree-constrained MST, Wilson sampling, line graphs, weighted girth); each verified against at least one catalog or standard reference before adoption.

## Not adopted

- OI Wiki random walks on trees/graphs: expectation computations belong to Mathematics probability.
- maspypy annulus/tree walk generating functions, characteristic polynomial of tree adjacency, count labeled graph classes, count unlabeled trees: Mathematics generating-function and polynomial work.
- maspypy bracket_graph, decompose_complete, find_path_through_specified, maximum_matching_between_vertex_edge, all_cycle_common_vertices, restore_euler_tour: problem-specific constructions without a reusable family.
- maspypy flow/longest_shortest_path, min_cost_matching_on_line, unbalanced_transportation: specialized LP/MCF reductions without contest-general contracts.
- maspypy graph/ds/tree_wavelet_matrix and mo_on_tree: Data Structures 17 composition and Data Structures 23 (`TreeMo`).
- Nyaan inclusion-tree: nested-interval tree construction, Miscellaneous interval ownership.
- hitonanode basepolyhedron and convex_sum: submodular-function machinery beyond graph contracts.
- NetworkX centrality, communities, link analysis, similarity, summarization, assortativity, rich club, small-world, s-metric, structural holes, swap, planar drawing, D-separation, moral graphs, node classification, asteroidal (AT-free) and perfect-graph tests: not contest algorithms or lack a reusable exact engine.
- NetworkX Voronoi cells: already expressed by multi-source Dijkstra parents in row 05.
- OGDF GraphReduction and planar SPQR variants: embedding-aware SPQR is beyond the row 61 contract.
- Koosaga LP simplex/bigint: Mathematics 28.
