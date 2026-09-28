# 04 Graphs — inventory and extraction status

The following names describe target header families. **Existing-unverified** rows now contain unchanged extracted code, with only the symbols listed in their source cell available; the full feature checklist remains future audit/build work. Other rows are **planned** or **legacy-reference**. **Verified** rows link their scoped implementation and test evidence. Basic/Advanced/Esoteric are editorial sections only. General families use underscore names, single algorithms use lower-case abbreviations. Prefixes prioritize common utility then dependencies; `98`/`99` are reserved aggregates.

## Basic

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `01-graph.hpp` | Directed/undirected full-range signed-integer weights; adjacency-list/CSR/dense views, stable edge/arc IDs, reverse arcs, loops/multiedges, zero-based vertices, input and label adapters. | Verified P010/GR01; [contracts, sources and tests](90-foundations.md). |
| `02-traversal.hpp` | Iterative DFS/multisource BFS, parent/depth/root and discovery/finish order, undirected components, bipartite coloring/odd-cycle witness, directed/undirected cycle witness. | Verified P010/GR01; [contracts, sources and tests](90-foundations.md). |
| `03-toposort.hpp` | Kahn and iterative DFS topological order, explicit cyclic failure and arc/vertex witness, lexicographically smallest heap option. | Verified P010/GR01; [contracts, sources and tests](90-foundations.md). |
| `04-dsu.hpp` | Canonical Data Structures DSU by inclusion; graph/CSR adapter for undirected or directed weak components. | Verified P010/GR01; [contracts, sources and tests](90-foundations.md). |
| `05-shortest_path.hpp` | BFS/multisource BFS, 0-1 BFS, dense/sparse Dijkstra, signed DAG shortest/longest and Bellman–Ford; exact 128-bit totals, negative reachability, path/cycle recovery, global negative-cycle detection and legacy adapter. | Verified P010/GR02; [contracts, sources and tests](90-foundations.md). |
| `06-mst.hpp` | Exact integer Kruskal, dense/sparse Prim and Borůvka forests with edge IDs; disconnected/negative/multiedge support; Kruskal reconstruction, minimax/maximin bottlenecks, LCA and threshold components. | Verified P010/GR02; [contracts, sources and tests](90-foundations.md). |
| `07-lca.hpp` | Iterative forest binary lifting, linear-preprocessing Euler RMQ and offline Tarjan LCA; ancestor/path jumps, edge/exact signed-weight distances, rerooted LCA, empty paths and legacy adapters. | Verified P011/GR03; [contracts, sources and tests](91-lca.md). |
| `08-scc.hpp` | Iterative Tarjan/Kosaraju on directed Graph/CSR multigraphs; source-to-sink component IDs and duplicate-free condensation DAG. | Verified P011/GR03; [contracts, sources and tests](92-scc_euler.md). |
| `09-bridges_articulation.hpp` | Iterative edge-ID lowlink bridges/articulation, edge/vertex-biconnected components, loops/multiedges and minimum-SCC strong orientation with Robbins obstruction witnesses; legacy adapters. | Verified P011/GR03; [contracts, sources and tests](93-lowlink.md). |
| `10-euleriantrail.hpp` | Directed/undirected Hierholzer with selectable start, original arc/edge witnesses, degree/connectivity conditions and explicit empty/failure semantics. | Verified P011/GR03; [contracts, sources and tests](92-scc_euler.md). |
| `11-functionalgraph.hpp` | Partial successor cycle/tail decomposition, full-64-bit jumps, reachability/distance/synchronous first meeting, ordered cycle/walk monoid folds and constant-memory single-orbit Floyd; missing successors terminate. | Verified P011/GR26; [contracts, sources and tests](94-functionalgraph.md). |

## Advanced

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `12-all_pairs_shortest_path.hpp` | Floyd–Warshall, Johnson, repeated Dijkstra, path reconstruction and negative cycles. | Existing-unverified; unchanged `FloydWarshall` excerpts. Remaining listed features await audit/build. |
| `13-shortest_path_advanced.hpp` | Dial and DS radix-heap Dijkstra, dense/bitset optimizations, A* and bidirectional search with admissibility/termination rules; SPFA and D’Esopo–Pape are heuristic alternatives with explicit worst-case bounds, never advertised as generally faster guarantees. Ranked walks/simple paths are separate. | Existing-unverified; unchanged `SPFA` excerpts. Remaining listed features await audit/build. |
| `14-tree_algorithms.hpp` | Diameter/radius/center, eccentricities, generic reroot/subtree DP, virtual tree and Euler indices. Supply subtree layouts for the separately owned Miscellaneous `15-smalltolarge.hpp` DSU-on-tree/sack engine; do not duplicate its container merging or callback machinery. Weighted/signed-weight assumptions, Prüfer encode/decode and rooted/unrooted reconstruction. | Planned; `[1] algorithms.cpp` TODOs. |
| `15-hld.hpp` | Heavy-light path/subtree folds/actions and painting/counting, direction-aware noncommutative folds, edge/vertex mode and rerooted subtree intervals. | Existing-unverified; unchanged `HLD` excerpts. Remaining listed features await audit/build. |
| `16-centroiddecomposition.hpp` | Build, nearest marked vertex, distance/range-contour updates/aggregates and all-distance histogram via scalar convolution; static/online updates distinguished, centroid versus tree center distinct. | Existing-unverified; unchanged `CentroidDecomposition` excerpts. Remaining listed features await audit/build. |
| `17-treeisomorphism.hpp` | AHU rooted/unrooted, canonical encoding or randomized hashes, automorphism count. | Planned; `[1] algorithms.cpp` TODO. |
| `18-twosat.hpp` | Implication graph, assignment, unsat witness, forced literals/backbone queries when requested, at-most-one/exactly-one-for-two/XOR encodings; large exactly-one is not generally 2-CNF. | Existing-unverified; unchanged `TwoSat` excerpts. Remaining listed features await audit/build. |
| `19-max_flow.hpp` | Dinic, push–relabel, capacity scaling and compact Edmonds–Karp baseline; min-cut/residual recovery, repeated augmentation and cut certificates. Bounds distinguish unit networks/unit capacities/general capacities. | Existing-unverified; unchanged `EdmondsKarp` excerpts. Remaining listed features await audit/build. |
| `20-min_cost_flow.hpp` | Successive shortest augmenting path with potentials/Dijkstra, Bellman–Ford initialization, cost-flow slope/breakpoints and path/edge recovery; negative-cycle policy explicit. Bounded circulation and scaling/simplex engines are separate targets. | Legacy-reference; Team Notebook `old_mcmf.cpp`. |
| `21-matching_bipartite.hpp` | Kuhn/Hopcroft–Karp with reconstruction, minimum vertex/edge cover, maximum independent set, allowed/essential matching edges and Dulmage–Mendelsohn decomposition; b-matching/capacities via flow. | Legacy-reference; `old_kuhn.cpp`. |
| `22-matching_general.hpp` | Unweighted Edmonds blossom/cardinality matching with edge witnesses, perfect-match existence and Tutte/Berge certificate research; weighted blossom is explicitly separate in Esoteric. | Planned |
| `23-assignment.hpp` | Hungarian O(n³), rectangular cost matrices, min/max and forbidden edges. | Existing-unverified; unchanged `Hungarian` excerpts. Remaining listed features await audit/build. |
| `24-dominatortree.hpp` | Lengauer–Tarjan and simple O(VE) fallback, directed flow graph dominance. | Planned |
| `25-directedmst.hpp` | Chu–Liu/Edmonds, reconstruction, unreachable root. | Planned |
| `26-dynamic_connectivity.hpp` | Offline edge-interval time decomposition with rollback DSU, component sums/bipartiteness/consistency, deletion-only reverse processing; insertion-only online bridge count/2-edge connectivity. Fully dynamic online connectivity is separate. | Existing-unverified; unchanged `OnlineBridges` excerpts. Remaining listed features await audit/build. |
| `27-blockcuttree.hpp` | Block-cut forest and bridge tree, vertex/edge biconnectivity, articulation query. | Legacy-reference; Team Notebook `old_block-cut-tree.cpp`. |
| `28-cycle_basis.hpp` | Fundamental cycle basis with edge-ID witnesses, F2 independence and cycle-space dimension for multigraphs; weighted minimum cycle basis is separate Esoteric work. | Planned |
| `29-planar_graph.hpp` | Given combinatorial half-edge embedding: face walks, Euler characteristic/components, dual graph and primal/dual traversal. Recognition/nonplanarity certificates are separate; geometric arrangements stay Geometry. | Planned |
| `30-flow_with_demands.hpp` | Feasible circulation and s–t flow with lower/upper bounds and vertex supplies/demands, minimum/maximum feasible flow value, reconstruction and infeasibility witness; finite capacities and signed-cost circulation contracts. | Planned; sources: OI, MAS, HIT. |
| `31-path_cover.hpp` | Minimum vertex-disjoint path cover of a DAG and reconstruction via bipartite matching; reachability-poset chain cover/Dilworth maximum antichain distinguished from covers using original edges. | Planned; sources: MAS. |
| `32-kshortestwalks.hpp` | Ranked shortest walks with repeated vertices/edges, Eppstein sidetracks/persistent heaps and simpler k-pop Dijkstra alternative; ties, duplicate edge sequences, unreachable targets and nonnegative/reweighted domains. | Planned; sources: EI, MAS, YC. |
| `33-chordalgraph.hpp` | Maximum cardinality search, chordal recognition, perfect elimination order or induced-cycle witness, optimal coloring and clique on chordal graphs. | Planned; sources: YC, KOO, OI. |
| `34-edge_coloring.hpp` | Optimal bipartite edge coloring with multiedges, matching/regularization alternatives; general simple-graph Vizing Δ+1 coloring is a separate operation and never a claim of optimal general coloring. | Planned; sources: KACTL, MAS, KOO. |
| `35-subgraph_enumeration.hpp` | Enumerate/count triangles and 4-cycles with degree orientation, weighted aggregates and duplicate-free witnesses; output-sensitive cost and simple-graph reduction explicit. | Planned; sources: YC, MAS. |
| `36-steiner_tree.hpp` | Exact terminal-subset Steiner tree DP for few terminals, reconstruction and disconnected failure; directed versus undirected domain explicit, metric-closure approximations separately labeled. | Planned; sources: YC, HIT, EI. |
| `37-graph_closure.hpp` | Maximum-weight closure, project selection, binary submodular energy and reducible ordered multi-label energy/min-cut reductions with reconstruction; hard implications, constant offsets, capacities and reducibility conditions. | Planned; sources: HIT, MAS. |
| `38-stablematching.hpp` | Gale–Shapley stable marriage and many-to-one hospital/residents with strict preferences and incomplete lists, proposer-optimal guarantee and unmatched agents; ties need a separate stated stability notion. | Planned; sources: MAS, KOO, OI. |
| `39-graphicalsequence.hpp` | Erdős–Gallai/Havel–Hakimi undirected simple-graph degree-sequence validation and construction; bipartite Gale–Ryser realization and directed paired in/out-degree realization via capacitated bipartite flow. State simple/multigraph and self-loop rules, enforce diagonal exclusions for loopless digraphs, and return a realization or failed inequality/cut certificate. | Planned; sources: MAS. |
| `40-reachability.hpp` | DAG/general directed transitive closure via SCC condensation and scalar word bitsets, batched reachability, DAG transitive reduction with preservation contract; use Core bitset acceleration only through Core APIs. | Planned; sources: MAS. |
| `41-cactusgraph.hpp` | Recognize/decompose cactus graphs into bridge/cycle blocks, block-tree path queries and reconstruction; vertex-disjoint versus edge-disjoint-cycle definitions explicitly distinguished. | Planned; sources: MAS. |
| `42-graph_decomposition.hpp` | Degeneracy/k-core ordering, core numbers and arboricity-oriented decomposition; returned orientation/forest certificates and graph-domain restrictions. | Planned; sources: NX. |
| `43-implicit_graph.hpp` | Implicit-neighbor BFS/Dijkstra and multi-state product/automaton graphs, complement-graph components/distances, range-to-point/point-to-range/range-to-range edge graph construction and reconstruction; preserve expansion size and avoid materializing dense complements. | Planned; sources: MAS, YC, HIT. |
| `44-differenceconstraints.hpp` | Feasible difference inequalities through shortest/longest paths, negative/positive-cycle infeasibility witness and variable reconstruction; equality and bound anchoring, disconnected components and overflow. | Planned; sources: OI. |
| `45-graph_connectivity.hpp` | Pair/global vertex and edge connectivity, vertex splitting, internally vertex-disjoint/edge-disjoint path witnesses and Menger cuts; distinguish directed/undirected and adjacent-terminal definitions, reuse canonical flow engines. | Planned; sources: OI, CPA. |

## Esoteric

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `46-gomoryhutree.hpp` | All-pairs undirected min-cut represented by n−1 flow calls; weighted multigraph and disconnected contracts. | Planned |
| `47-stoerwagner.hpp` | Global undirected min-cut as alternative to flows; zero/negative capacity policy. | Planned |
| `48-matroidintersection.hpp` | Graphic/transversal/partition/linear matroid oracles, unweighted/weighted intersection, matroid union/partition, edge-disjoint spanning-tree/forest packing and basis exchange; graph clients reuse DS oracle and Math linear algebra. | Planned |
| `49-maximumclique.hpp` | Bron–Kerbosch maximal-clique enumeration and maximum-clique branch-and-bound, weighted clique/independent set variants and clique covers as separate contracts; exponential behavior explicit. | Planned |
| `50-treedecomposition.hpp` | Treewidth recognition/decomposition and DP, explicit width-2 construction, nice bags and certificates, with bounded-width premise and failure distinguished from heuristic inability. | Planned |
| `51-lct.hpp` | Graph-facing dynamic tree path aggregates, sharing the Data Structures implementation. | Planned |
| `52-sensitivity_analysis.hpp` | Replacement paths/edges, second-best MST, forced/forbidden MST edges, MST uniqueness and shortest-path edge sensitivity; bridges/articulation are reused, not rediscovered. | Planned |
| `53-graph_counting.hpp` | Graph-facing matrix-tree/all-minors spanning forest/tree counts, BEST Euler tours, rooted arborescences, LGV nonintersecting paths, small-graph connected/biconnected/bridgeless/DAG/independent-set counting and matching counts; reuse Math determinants/permanent/Pfaffian/hafnian. Tutte/reliability polynomial research has explicit small-graph/exponential domains. | Planned |
| `54-graph_isomorphism.hpp` | Color refinement, backtracking, planar/bounded-degree specialized methods; no universal polynomial claim. | Planned |
| `55-temporal_graph.hpp` | Time-dependent shortest path and reachability, FIFO assumption, offline interval edges. | Planned |
| `56-minimummeancycle.hpp` | Karp and Howard heuristic, rational exactness where needed. | Planned |
| `57-maximumdensitysubgraph.hpp` | Parametric flow/closure and fractional programming. | Planned |
| `58-gabowmatching.hpp` | Advanced cardinality/weighted matching scaling and faster exact algorithms (Gabow/Micali–Vazirani), each with precise published domain/bounds after baseline blossoms are reliable. | Planned |
| `59-kshortestsimplepaths.hpp` | Yen/Lawler-style ranked loopless simple paths, replacement/deviation subproblems, tie/duplicate policy and explicit expensive worst-case bounds; distinct from shortest walks. | Planned; sources: EI. |
| `60-stnumbering.hpp` | St-numbering/bipolar orientation of a biconnected graph with specified st edge, open-ear decomposition and witness; reject unsupported disconnected/articulation cases. | Planned; sources: YC, MAS, KOO. |
| `61-triconnectivity.hpp` | Separation pairs, triconnected components and SPQR-tree decomposition of biconnected multigraphs; virtual-edge ownership and reconstruction. | Planned; sources: OGDF. |
| `62-planarity.hpp` | Planarity recognition with combinatorial embedding or nonplanarity certificate; disconnected graphs, multiedges and conversion to the canonical planar half-edge representation. | Planned; sources: NX, OI. |
| `63-fully_dynamic_connectivity.hpp` | Online arbitrary edge insert/delete connectivity using dynamic forests and replacement-edge levels; HDLT/HDT amortized bounds, parallel-edge identities and a simpler offline fallback. | Planned; sources: HDT, KOO. |
| `64-weightedblossom.hpp` | Maximum-weight general matching and minimum-weight perfect matching via primal-dual blossoms, matching reconstruction, odd cycles, negative/zero weights and infeasible perfect matchings. | Planned; sources: TKO, YC, OI. |
| `65-costscalingflow.hpp` | Cost/capacity-scaling min-cost circulation with supplies/demands, negative cycles and residual optimality certificate; distinguish exact integral bounds from floating approximations. | Planned; sources: HIT, YC. |
| `66-networksimplex.hpp` | Network simplex for minimum-cost circulation, basis/pivot and degeneracy rules, artificial feasibility and unboundedness/infeasibility contracts; performance is not a universal polynomial bound. | Planned; sources: HIT, YC. |
| `67-coloring_exact.hpp` | Exact chromatic number/coloring and chromatic polynomial for small graphs by subset DP/inclusion–exclusion/deletion–contraction, reconstruction and exponential domain; polynomial arithmetic stays Mathematics. | Planned; sources: YC, TKO. |
| `68-cycle_enumeration.hpp` | Output-sensitive enumeration of all elementary directed cycles (Johnson), undirected cycles with canonical duplicates, and shortest/girth cycles with witnesses; chordless enumeration needs separate bounds. | Planned; sources: NX. |
| `69-tjoin.hpp` | Minimum-weight T-join, undirected Chinese postman and route reconstruction through shortest-path metric closure and perfect matching; directed postman uses imbalance min-cost flow. | Planned; sources: NX. |
| `70-incrementalscc.hpp` | Insertion-only SCC/condensation maintenance and incremental topological order, reporting merges/cycle creation with stated amortized bound; unrelated to fully dynamic undirected connectivity. | Planned; sources: MAS, YC. |
| `71-dynamicmst.hpp` | Incremental/decremental/offline fully dynamic spanning forest, replacement edges and weight-change sensitivity; no unsupported online worst-case guarantee. | Planned; sources: YC, KOO, HDT. |
| `72-minimumcyclebasis.hpp` | Weighted minimum cycle basis with edge-vector witnesses, independence checks and domain-specific algorithms; fundamental cycle basis remains the smaller Advanced family. | Planned; sources: NX. |
| `73-hamiltonian.hpp` | Exact Hamiltonian path/cycle and traveling-salesman subset DP with reconstruction, directed/undirected weights and infeasible states; exponential size limit explicit, counting reuses graph-counting kernels. | Planned; sources: CSES, OI, MAS. |
| `74-threeedgecomponents.hpp` | Three-edge-connected components, cut-pair witnesses and multigraph edge identity; distinguish edge connectivity from triconnected vertex/SPQR decomposition. | Planned; sources: YC, EI. |
| `75-minimumdiameterspanningtree.hpp` | Minimum-diameter spanning tree with reconstruction for the documented weighted/nonnegative domain, graph absolute-center relation and disconnected failure. | Planned; sources: YC, OI. |
| `76-dynamicstarmincut.hpp` | Global minimum cut in a dynamic star-augmented graph with fixed base graph and evolving star-edge capacities; scope is this restricted update model, not arbitrary fully dynamic cuts. | Planned; sources: YC, EI. |
| `77-structured_graph.hpp` | Series-parallel and outerplanar recognition/decomposition, tournament SCC/Hamiltonian ordering and class-specific certificates; explicit membership preconditions and forbidden subgraph witnesses. | Planned; sources: MAS, OI. |
| `78-grouplabeledshortestpath.hpp` | Shortest nonzero-group-product paths with explicit group/orientation semantics and nonnegative weights; distinguish simple paths from walks and document the specialized algorithm domain. | Planned; sources: EI, HIT, MAS. |
| `79-tree_ordering.hpp` | Rooted-tree precedence orders with minimum inversions, optimal product/order on trees and mergeable-component scheduling; exact objective/domain and reconstruction, not generic arbitrary-DAG scheduling. | Planned; sources: YC, MAS. |
| `80-rankedmatching.hpp` | Rank-maximal bipartite matching and preference-profile objectives; distinguish rank-maximal from minimum sum of ranks and stable matching, return matching plus rank signature. | Planned; sources: MAS. |
| `81-stableroommates.hpp` | Irving stable-roommates algorithm for strict complete preferences, possible nonexistence, rotation elimination and stable matching witness; incomplete lists/ties require explicitly separate domains. | Planned; sources: MAS, KOO. |
| `82-skew_symmetric_flow.hpp` | Integral skew-symmetric/bidirected flow and capacitated undirected f-factor/b-matching variants, exact vertex-demand and edge-capacity constraints, infeasibility certificate and reconstruction; the inspected code is only an f-factor special case, generalization needs primary research. | Planned; sources: KOO. |
| `83-extremevertexsets.hpp` | Laminar extreme sets of the undirected nonnegative weighted cut function, tree representation and global-min-cut relation; candidate-tree pruning/validation and zero-capacity/tie contracts need proof. | Planned; sources: KOO. |
| `84-dynamictreedp.hpp` | Fixed-tree point updates to reroot/subtree DP through HLD matrix products/static top trees, including path-composition-sum queries; fully dynamic link/cut DP uses the canonical DS top-tree engine under a separate cluster algebra. | Planned; sources: OI, MAS, YC. |

## Shared contracts and verification

- All graph APIs specify n, 0-based vertices, directedness, edge identity, self-loops, parallel edges, disconnected cases, weight/capacity type and overflow policy. Return witnesses (paths, cuts, matchings, cycles), not just objective values where cheap.
- Distinguish nonnegative, arbitrary signed and time-dependent edge weights; Dijkstra must reject/forbid negatives. Distinguish max-flow capacity from cost and define residual mutations.
- Cross-check small graphs by brute force: shortest paths against Floyd–Warshall, MST against subset enumeration, lowlink against edge/vertex deletion, flow against cuts, matching against enumeration. Include disconnected and multigraph cases.
- Existing snippets may use template globals or ambiguous directed conventions. Migration requires standalone compile and contract tests before moving from legacy reference to verified.

## Unchanged monolith extraction

See [local legacy references](97-Legacy/00-index.md) for older, incomplete or integration-blocked variants and [the transfer record](../00-Guidelines/14-monolith-transfer.md) for scope and build evidence. Original bodies and old comments were preserved; compilation does not verify semantics, completeness, performance, indexing or complexity claims. The source-range/hash map remains authoritative for provenance.

## Completeness audit and ownership (2026-09-27)

The audit compares named families and public-operation catalogs, not source-code correctness. All new entries remain **planned**; extracted algorithm bodies and existing statuses are unchanged. This is a broad, extensible inventory, not a claim that all algorithms in every publication have been enumerated. Exact source paths and read scope are recorded in the audit record.

Canonical ownership: DSU and dynamic-tree adapters reuse Data Structures; polynomial/determinant/counting kernels reuse Mathematics; Graphs owns combinatorial graph APIs and certificates. Walks versus simple paths, cardinality versus weighted general matching, given planar embedding versus recognition, and offline versus online fully dynamic connectivity are separate targets.

For precise per-family read scope and caveats, see [the completeness audit](../00-Guidelines/16-inventory-audit.md).

The final scope review clarified the boundary with Miscellaneous's canonical DSU-on-tree/small-to-large engine and made directed degree-sequence realization explicit under graphical sequences. The saved Nyaan catalog lists [DSU on Tree](https://nyaannyaan.github.io/library/tree/dsu-on-tree.hpp); only its catalog scope was checked here. Directed realization is a planned flow reduction: paired out-/in-degree copies, admissible-arc capacities and a flow saturating the equal degree totals; loop/parallel-edge restrictions remain part of the public contract. These additions are specifications, not implementation verification.

Additional unchanged archive leads: `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `test.cpp` contain `LcaO1`, `LcaLog`, `DEsopoPape`, `TarjanSCC`, `StrongOrientation`, `EulerPath`, `Kruskal`, `PrimDense`, `TreeEdgePainting`, `Dinic`, `FlowWithDemands`, `PushRelabel` and `Kuhn` (symbols vary by file). They remain legacy references, not verified active implementations.

Source keys (catalog inspection does not establish implementation correctness):

- **YC**: [https://github.com/yosupo06/library-checker-problems](https://github.com/yosupo06/library-checker-problems) — Graph/data_structure/tree problem-directory catalog, used to identify independently testable operations.
- **CPA**: [https://cp-algorithms.com/](https://cp-algorithms.com/) — Data Structures/Graphs navigation entries for sqrt-tree, RMQ and bounded flow; implementation-time article review still required.
- **OI**: [https://oi-wiki.org/](https://oi-wiki.org/) — Data-structure and graph navigation/catalog; uncommon graph decomposition and optimization families.
- **MAS**: [https://maspypy.github.io/library/](https://maspypy.github.io/library/) — ds, graph, flow, tree catalog paths.
- **NYA**: [https://nyaannyaan.github.io/library/](https://nyaannyaan.github.io/library/) — data-structure, graph and tree catalog paths.
- **EI**: [https://ei1333.github.io/library/](https://ei1333.github.io/library/) — structure and graph catalog paths.
- **SUI**: [https://suisen-cp.github.io/cp-library-cpp/](https://suisen-cp.github.io/cp-library-cpp/) — data structure dynamic/persistent/sequence catalog paths.
- **HIT**: [https://hitonanode.github.io/cplib-cpp/](https://hitonanode.github.io/cplib-cpp/) — data_structure and graph catalog paths.
- **KACTL**: [https://github.com/kth-competitive-programming/kactl](https://github.com/kth-competitive-programming/kactl) — Saved notebook Data Structures/Graphs contents and declarations; repository graph catalog.
- **CSES**: [https://cses.fi/problemset/](https://cses.fi/problemset/) — Graph/tree problem titles including functional graphs and Hamiltonian flights.
- **ODS**: [https://opendatastructures.org/ods-cpp.pdf](https://opendatastructures.org/ods-cpp.pdf) — Saved PDF table of contents, chapters 4/8/9/10/13 (skip lists, balancing, heaps and integer predecessor choices).
- **TKO**: [https://tko919.github.io/library/](https://tko919.github.io/library/) — Range LIS and general weighted matching catalog; weighted-matching source documentation inspected.
- **DYNAMIC**: [https://github.com/xxsds/DYNAMIC](https://github.com/xxsds/DYNAMIC) — README data-structure features and TODO; dynamic bitvector/wavelet tree supported, dynamic wavelet matrix explicitly TODO.
- **HDT**: [https://github.com/tomtseng/dynamic-connectivity-hdt](https://github.com/tomtseng/dynamic-connectivity-hdt) — README algorithm scope and Holm/de Lichtenberg/Thorup 2001 citation; implementation covers connectivity only.
- **OGDF**: [https://ogdf.github.io/doc/ogdf/group__decomp.html](https://ogdf.github.io/doc/ogdf/group__decomp.html) — Graph-decomposition API catalog, static/dynamic SPQR classes.
- **KOO**: [https://github.com/koosaga/olympiad/tree/master/Library/codes](https://github.com/koosaga/olympiad/tree/master/Library/codes) — Current Library/codes catalog; focused skew-symmetric flow special-case comments, extreme-set function and tree-packing augmentation code. The former repository is archived.
- **NX**: [https://networkx.org/documentation/stable/reference/algorithms/](https://networkx.org/documentation/stable/reference/algorithms/) — Planarity/cycles/eulerize API documentation; Eulerize is unweighted, Edmonds–Johnson reference motivates weighted T-join research.
- **STANFORD**: [https://github.com/jaehyunp/stanfordacm](https://github.com/jaehyunp/stanfordacm) — Saved 2015–16 team notebook contents and graph/data-structure declarations.

Bounded-flow ownership: flow_with_demands owns lower/upper-bound and supply reductions/reconstruction. Its costed-network adapter states the backend domain; ordinary SSP min-cost flow may have a restricted negative-cycle contract. General signed-cost circulation belongs to costscalingflow/networksimplex. Backends may use the bounded-flow model, so the model must not depend back on those engines.
