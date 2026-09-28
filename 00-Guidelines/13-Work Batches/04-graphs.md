# 04-Graphs implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../04-Graphs/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| GR01 | `01-graph.hpp`, `02-traversal.hpp`, `03-toposort.hpp`, `04-dsu.hpp` | M | C01, DS01 | Graph storage/traversal/toposort and graph-facing DSU adapter; DS01. |
| GR02 | `05-shortest_path.hpp`, `06-mst.hpp` | L | C01, GR01 | Single-source shortest paths and MST; GR01. |
| GR03 | `07-lca.hpp`, `08-scc.hpp`, `09-bridges_articulation.hpp`, `10-euleriantrail.hpp` | L | C01, GR01 | LCA, SCC, lowlink and Euler trails; GR01. |
| GR04 | `12-all_pairs_shortest_path.hpp`, `13-shortest_path_advanced.hpp` | L | C01, DS27, GR01, GR02 | All-pairs and advanced shortest paths; GR02. |
| GR05 | `14-tree_algorithms.hpp`, `15-hld.hpp` | L | C01, DS01, GR01, GR03 | Tree algorithms, subtree layouts and HLD; GR03, DS01. MI04 owns small-to-large merging and DSU-on-tree/sack callbacks using these layouts. |
| GR06 | `16-centroiddecomposition.hpp`, `17-treeisomorphism.hpp` | L | C01, GR01, GR03, MA06 | Centroid decomposition and tree isomorphism; GR03. MA06 is required for the histogram milestone, not the initial decomposition/nearest-marked baseline. |
| GR07 | `18-twosat.hpp` | M | C01, GR01, GR03 | Two-SAT and witnesses; GR03. |
| GR08 | `19-max_flow.hpp` | XL | C01, GR01 | Max flow engines, cuts and residual recovery. |
| GR09 | `20-min_cost_flow.hpp` | XL | C01, GR01, GR08 | Successive-shortest-path min-cost flow, potentials, cost-flow slope/breakpoints and negative-edge/cycle policy. GR27 owns lower/upper-bound demand reductions; GR46/GR47 own general circulation backends. |
| GR10 | `21-matching_bipartite.hpp`, `23-assignment.hpp` | L | C01, GR01 | Bipartite matching and Hungarian assignment. |
| GR11 | `24-dominatortree.hpp` | L | C01, GR01 | Dominator tree, reachability conventions and witness/certificate tests. GR69 owns directed MST. |
| GR12 | `26-dynamic_connectivity.hpp`, `27-blockcuttree.hpp` | L | C01, DS03, GR01, GR03 | Dynamic connectivity and block-cut trees; DS03, GR03. |
| GR13 | `28-cycle_basis.hpp`, `29-planar_graph.hpp` | L | C01, GR01 | Cycle basis and planar embeddings. |
| GR14 | `46-gomoryhutree.hpp`, `47-stoerwagner.hpp` | L | C01, GR01, GR08 | Gomory–Hu and Stoer–Wagner cuts; GR08. |
| GR15 | `48-matroidintersection.hpp` | XL | C01, DS21, GR01, MA08 | Matroid intersection, weighted exchange paths; DS21 oracle. Only the linear-matroid client requires MA08; generic exchange/oracle machinery can start earlier. |
| GR16 | `49-maximumclique.hpp` | L | C01, GR01 | Stage maximal-clique enumeration, exact maximum/weighted clique and independent-set interfaces, then cover variants with explicit exponential bounds and brute-force witnesses. |
| GR17 | `50-treedecomposition.hpp` | XL | C01, GR01 | Tree decomposition and bounded-width DP. |
| GR18 | `51-lct.hpp`, `52-sensitivity_analysis.hpp` | L | C01, DS14, GR01, GR02 | Graph-facing LCT adapter and sensitivity analysis; DS14/GR02. |
| GR19 | `53-graph_counting.hpp` | XL | C01, GR01, MA11, MA32 | Graph-facing matrix-tree/all-minors/BEST/LGV counts, matching-count adapters to MA32, then exponential small-graph/subset and Tutte/reliability research stages. Keep mathematical kernels canonical in Mathematics. |
| GR20 | `54-graph_isomorphism.hpp` | XL | C01, GR01 | Graph isomorphism with specialized domains. |
| GR21 | `55-temporal_graph.hpp` | L | C01, GR01 | Temporal shortest paths/reachability and FIFO policy. |
| GR22 | `58-gabowmatching.hpp` | XL | C01, GR01, GR25, GR45 | Advanced cardinality and weighted matching algorithms/scaling after GR25 and GR45 baselines; separate published-domain/proof/implementation checkpoints for each method. |
| GR23 | `56-minimummeancycle.hpp` | M | C01, GR01, GR04 | Minimum-mean cycle and exact rational value. |
| GR24 | `57-maximumdensitysubgraph.hpp` | M | C01, GR01, GR08 | Maximum-density subgraph by parametric optimization. |
| GR25 | `22-matching_general.hpp` | XL | C01, GR01, GR10 | Unweighted general cardinality matching/blossom, perfect-match existence and witnesses. Weighted general/perfect matching is owned by GR45; GR10 provides differential matching tests. |
| GR26 | `11-functionalgraph.hpp` | M | C01, GR01, GR03 | Functional-graph cycle/tail decomposition, successor jumps, reachability/distance/meeting and missing-successor rules. |
| GR27 | `30-flow_with_demands.hpp` | L | C01, GR01, GR08, GR09 | Lower/upper-bound and vertex-supply feasibility reductions, minimum/maximum feasible s–t flow and reconstruction. Expose costed-network adapters under a backend contract; GR09 handles its declared restricted domain, GR46/GR47 own general signed-cost circulation engines. Do not add GR27 -> GR46/GR47, since those depend on the bounded-flow model. Keep frontend/backend ownership explicit in inventory and batch notes. |
| GR28 | `31-path_cover.hpp` | M | C01, GR01, GR10 | DAG vertex-disjoint path covers and reachability-poset chain/antichain witnesses; distinguish original edges from transitive reachability. |
| GR29 | `32-kshortestwalks.hpp` | L | C01, DS34, GR01, GR02 | Ranked shortest walks via sidetracks/persistent heaps and k-pop Dijkstra; tie, repetition and reweighting contracts. |
| GR30 | `33-chordalgraph.hpp` | L | C01, GR01 | Chordal recognition, perfect elimination order or induced-cycle witness, then specialized coloring and clique. |
| GR31 | `34-edge_coloring.hpp` | L | C01, GR01, GR10 | Bipartite multigraph edge coloring and separate Vizing coloring for general simple graphs; matching/regularization engines. |
| GR32 | `35-subgraph_enumeration.hpp` | M | C01, GR01 | Duplicate-free triangle/4-cycle enumeration and weighted counts using degree ordering; output-sensitive bounds. |
| GR33 | `36-steiner_tree.hpp` | L | C01, GR01, GR02, MI21 | Few-terminal exact Steiner DP and reconstruction over shortest paths/subsets; label metric approximations separately. |
| GR34 | `37-graph_closure.hpp` | L | C01, GR01, GR08 | Maximum-weight closure and reducible binary/multilabel energy via min-cut, with offsets and reconstruction. |
| GR35 | `38-stablematching.hpp` | M | C01, GR01 | Stable marriage and hospital/residents with strict/incomplete preferences, unmatched agents and proposer-optimal guarantees. |
| GR36 | `39-graphicalsequence.hpp` | M | C01, GR01, GR08 | Undirected/bipartite degree-sequence validation and realization, then directed paired in/out-degree realization using GR08 flow; diagonal exclusions, parallel-edge policy and infeasibility certificates. |
| GR37 | `40-reachability.hpp` | L | C01, GR01, GR03 | SCC-condensed/batched reachability and DAG transitive reduction; scalar bitsets with optional Core acceleration. |
| GR38 | `41-cactusgraph.hpp` | M | C01, GR01, GR12 | Cactus recognition, bridge/cycle block decomposition and block-tree queries with explicit cycle-disjointness definitions. |
| GR39 | `42-graph_decomposition.hpp` | M | C01, GR01 | Degeneracy/k-core/arboricity decompositions, orientations/forests and verifiable graph-domain certificates. |
| GR40 | `59-kshortestsimplepaths.hpp` | XL | C01, GR01, GR02 | Ranked loopless paths with deviation subproblems, graph edge masking, duplicate policy and worst-case bounds. |
| GR41 | `60-stnumbering.hpp` | L | C01, GR01, GR12 | st-numbering, bipolar orientation and open-ear witnesses under biconnected-graph preconditions. |
| GR42 | `61-triconnectivity.hpp` | XL | C01, GR01, GR41 | Separation pairs and SPQR decomposition, virtual edges and reconstruction for biconnected multigraphs. |
| GR43 | `62-planarity.hpp` | XL | C01, GR01, GR03, GR13 | Planarity recognition with embedding or nonplanarity certificate; convert to the canonical half-edge representation. |
| GR44 | `63-fully_dynamic_connectivity.hpp` | XL | C01, DS15, GR01 | Fully dynamic online connectivity, replacement-edge levels and HDLT/HDT bounds over dynamic forests. |
| GR45 | `64-weightedblossom.hpp` | XL | C01, GR01, GR25 | Weighted general/perfect matching with primal-dual blossom invariants, reconstruction and infeasibility handling. |
| GR46 | `65-costscalingflow.hpp` | XL | C01, GR01, GR27 | Cost/capacity-scaling circulation with supplies, negative cycles and exact residual optimality certificates. |
| GR47 | `66-networksimplex.hpp` | XL | C01, GR01, GR09, GR27 | Network-simplex circulation, pivot/degeneracy rules and artificial feasibility; distinguish empirical speed from worst-case bounds. |
| GR48 | `67-coloring_exact.hpp` | L | C01, GR01, MA12 | Exact small-graph coloring/chromatic polynomial with reconstruction and explicit exponential domains; reuse subset transforms. |
| GR49 | `68-cycle_enumeration.hpp` | L | C01, GR01, GR02, GR03 | Elementary directed/undirected cycle enumeration, duplicate rules and shortest/girth witnesses; isolate chordless bounds. |
| GR50 | `69-tjoin.hpp` | L | C01, GR01, GR02, GR09, GR45 | Weighted T-joins and Chinese postman reconstruction using shortest paths, weighted perfect matching and directed imbalance flow. |
| GR51 | `70-incrementalscc.hpp` | XL | C01, GR01, GR03 | Insertion-only SCC/condensation and topological-order maintenance with merge/cycle reports and amortized bounds. |
| GR52 | `71-dynamicmst.hpp` | XL | C01, DS14, GR01, GR02 | Incremental/decremental/offline dynamic MST, replacement edges and weight changes; specify supported update models. |
| GR53 | `72-minimumcyclebasis.hpp` | L | C01, C09, GR01, GR02, GR13 | Weighted minimum cycle basis with edge-vector witnesses, shortest-path engines and F2 independence checks. |
| GR54 | `73-hamiltonian.hpp` | L | C01, GR01, MI21 | Exact Hamiltonian/TSP subset DP, reconstruction and infeasible states under explicit exponential limits. |
| GR55 | `74-threeedgecomponents.hpp` | L | C01, GR01, GR03 | Three-edge components and cut-pair witnesses, preserving multigraph edge IDs and separation from vertex triconnectivity. |
| GR56 | `75-minimumdiameterspanningtree.hpp` | L | C01, GR01, GR04 | Minimum-diameter spanning tree and absolute-center relation using all-pairs distances; weighted-domain and disconnected contracts. |
| GR57 | `76-dynamicstarmincut.hpp` | XL | C01, GR01, GR08 | Dynamic star-augmented global min-cut with a fixed base graph; prove only the declared restricted update model. |
| GR58 | `43-implicit_graph.hpp` | L | C01, DS01, GR01, GR02 | Implicit/complement/product graph search and range-edge expansion, with reconstruction and materialization-size bounds. |
| GR59 | `77-structured_graph.hpp` | XL | C01, GR01, GR03, GR13 | Series-parallel/outerplanar recognition and tournament ordering with certificates; use general planarity only if the chosen method requires it. |
| GR60 | `44-differenceconstraints.hpp` | M | C01, GR01, GR02 | Difference-constraint feasibility and assignments, anchored bounds and cycle infeasibility witnesses. |
| GR61 | `78-grouplabeledshortestpath.hpp` | XL | C01, GR01, GR02 | Shortest nonzero group-product paths, orientation/group algebra and nonnegative-weight domain; distinguish paths from walks. |
| GR62 | `79-tree_ordering.hpp` | L | C01, DS48, GR01, GR05 | Rooted-tree precedence objectives, minimum inversions and mergeable-component scheduling; reconstruct orders using DSU/heaps. |
| GR63 | `80-rankedmatching.hpp` | L | C01, GR01, GR10, GR27 | Rank-maximal bipartite matching with matching witnesses and rank signatures, distinct from rank-sum or stability objectives. |
| GR64 | `81-stableroommates.hpp` | L | C01, GR01, GR35 | Stable roommates with strict complete preferences, rotation elimination and possible nonexistence; isolate other preference domains. |
| GR65 | `45-graph_connectivity.hpp` | L | C01, GR01, GR08 | Pair/global vertex/edge connectivity and disjoint paths via flow, including adjacent-terminal and directedness conventions. |
| GR66 | `82-skew_symmetric_flow.hpp` | XL | C01, GR01, GR08, GR25 | Skew-symmetric/bidirected flow and capacitated f-factor/b-matching; separate inspected special cases from general research. |
| GR67 | `83-extremevertexsets.hpp` | L | C01, GR01, GR14 | Laminar extreme vertex sets of weighted cuts, tree representation and candidate pruning with tie/zero-capacity proofs. |
| GR68 | `84-dynamictreedp.hpp` | XL | C01, DS16, GR01, GR05 | Dynamic tree DP on fixed trees through HLD/static top trees; treat link/cut cluster algebra as a separate milestone. |
| GR69 | `25-directedmst.hpp` | L | C01, DS03, DS48, GR01 | Directed MST/arborescence by Chu–Liu/Edmonds with reconstruction and unreachable-root failure; require DS48 only for heap-based engines. |
