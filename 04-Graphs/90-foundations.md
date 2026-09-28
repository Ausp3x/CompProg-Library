# P010 graph foundations — GR01 then GR02

This record covers the six Basic graph headers owned by P010. The prerequisite P002 template and P006 canonical DSU are already verified. Implementations are independent contest-profile code; archived snippets remain unchanged. GR01 was completed and verified before GR02 implementation began. Both batches are complete as of 2026-09-27; the package checkbox is backed by the verification below.

## GR01 contracts and correctness

`Graph` owns zero-based vertices, insertion-ordered logical edges and oriented arcs. All counts fit `int`; weights use the full `lng` domain. An undirected edge has two reciprocal arcs even for a self-loop. A directed arc has `rev == -1`; its reverse graph is a separate object. Reversal preserves logical edge IDs. Algorithms return arc indices so orientation and parallel edges remain distinguishable; `g.arcs[a].id` recovers the logical ID. Public storage is inspectable but must not be edited directly.

`CsrGraph` is an owning immutable snapshot with contiguous adjacency indices. It preserves arc IDs and adjacency order. `DenseGraph` keeps the full graph and an O(n²) matrix selecting the minimum-weight arc per ordered pair, with earliest-arc ties and `-1` for absence. There are no implicit diagonal edges. Thus a dense projection does not lose parallel-edge provenance. `Graph::read` accepts exactly the requested number of weighted/unweighted records and base 0 or 1; malformed input is a precondition violation. `GraphLabels<T>` provides sorted coordinate compression with ordering-equivalent labels merged.

`bfs` seeds all sources before traversal, yielding unweighted shortest distances; `dfs` considers seeds sequentially and returns recursive-order discovery/finishing order without recursion. Duplicate sources are ignored, and empty source lists reach nothing. Unvisited fields are `-1`; roots have depth zero, no parent and their own root ID. DFS depth is tree depth. `dfsForest` covers all vertices. DFS detects a gray ancestor and skips only the reverse parent arc in undirected graphs, so loops and parallel-edge cycles are handled correctly. `CycleWitness` has one more vertex than arcs, repeated first/last vertex, and no other repeated vertex; an empty arc list means no cycle.

`connectedComponents` and `bipartiteCheck` require undirected input. Components are numbered by increasing minimum vertex; member order follows traversal. The bipartite algorithm colors BFS layers and returns an odd cycle when an edge joins equal colors. Parent-tree paths meet at their common ancestor, so the resulting cycle is contiguous and simple. Coloring on failure may be partial. The graph-facing `graphComponents` adapter uses the canonical P006 `DSU`, treating a directed graph as weak connectivity; it neither duplicates nor modifies the DSU implementation.

Kahn topological sorting counts every arc, including duplicates. A FIFO queue gives linear time; a minimum heap yields the lexicographically smallest valid order. DFS reverse postorder is a second implementation. Both require directed input and return an explicit acyclic flag; cyclic inputs return an empty order plus an oriented cycle. Empty directed graphs succeed with empty orders.

## GR02 contracts and correctness

All new weighted algorithms use exact signed-integer weights from the full `lng` range and `lll` accumulated distances/costs. Status fields distinguish absent, finite and unbounded answers without reserving a numeric weight. Fractional/custom-weight instantiations of archived notebook templates remain outside this specified integer API.

`ShortestPathResult` stores `dist`, `reachable`, `negative`, `parent`, `parent_arc` and one reachable `negative_cycle`. A distance is meaningful only when `finite(v)` is true; reachable negative-infinite vertices have both flags set. `path(g,v)` returns an explicit existence flag and oriented vertex/arc sequence; a valid zero-edge source path has one vertex. The input graph's arc IDs/content must remain unchanged. Sources are zero-initialized, duplicate sources are ignored, and an empty source set reaches nothing. Signed DAG/Bellman–Ford paths may improve a source through another source.

`bfsShortestPaths` counts arcs while ignoring weights. `zeroOneBfs` requires all weights in {0,1}; deque ordering processes nondecreasing distances and stale entries avoid rescanning improved candidates. Sparse `dijkstra` uses a lazy binary heap; both it and `dijkstraDense` require every original edge weight to be nonnegative, including edges in unreachable components. Dense construction selects the best parallel edge while retaining its original arc ID. For general multigraphs, sparse heap time is O(k+(n+m) log(n+m+1)) and workspace O(n+m), where m is the arc count and k is the source count. Dense time is O(n²+m+k) beyond dense-view construction. BFS/0-1 BFS are linear in n+m+k.

`dagShortestPaths`/`dagLongestPaths` relax a directed graph in topological order with arbitrary signed weights. A cyclic graph is a valid failure result with an oriented witness and no reachable path result. Topological induction establishes the minimum/maximum over all source paths. `bellmanFord` performs at most n in-place passes; an improvement in pass n is reachable from a negative cycle. It marks all such destinations and every vertex reachable from them as negative-infinite. Stepping n predecessors from the final changed vertex reaches a negative cycle, which is recovered with original arc orientation. This witness is a directed arc-cycle; an undirected negative edge legitimately supplies both reciprocal arcs of the same logical edge. `findNegativeCycle` seeds every vertex, so it also detects cycles outside any single source's reachable component.

Every Bellman–Ford intermediate represents a walk with at most n·m edge additions. Since n,m≤INT_MAX and |w|≤2^63, its magnitude is less than 2^125; exact `lll` arithmetic needs no saturating sentinels. Finite optimal paths, DAG paths and forest totals have smaller bounds. Strict relaxations preserve finite parent chains and do not form spurious zero-weight parent cycles.

The historical `Dijkstra(n)` adapter retains vertices [0,n], `dis`, `is_proc`, `unproc` and repeated `runDijkstra` resets. Weighted and notebook-style unweighted adjacency overloads are supported. This compatibility interface requires n<INT_MAX, at least n+1 adjacency lists, nonnegative weights and each reachable shortest distance strictly below `INF64`; its queue is empty after a completed run. The new result API supports larger exact distances and explicit absence.

`kruskal`, `primSparse`, `primDense` and `boruvka` return a `SpanningForest` with its exact weight, component count and original logical edge IDs. They require undirected input, ignore loops, accept parallel/negative edges and include isolated vertices as components. The empty graph returns weight zero, zero components and no edges. Ties compare logical edge IDs; returned edge order follows each algorithm. A forest contains exactly n−components edges, so its full-range signed sum fits comfortably in `lll`.

Kruskal adds sorted edges only across distinct components. Prim repeatedly chooses the minimum crossing edge; its sparse set keeps one candidate per unused vertex, and its dense form scans the already-built minimum-edge matrix. Both restart at isolated/disconnected components. Borůvka caches phase-start representatives and chooses each active component's minimum outgoing edge; union guards suppress duplicates/cycles. The cut property justifies every accepted edge. Active components halve per phase until each original connected component is merged. Bounds are O(n+m log(m+1)) for Kruskal, O((n+m) log(n+1)) for sparse Prim/Borůvka, and O(n²) for dense Prim with O(n²+m) dense-view setup charged separately.

`KruskalReconstruction` stores a binary union forest. Leaves are original vertices, each internal node records the accepted merge edge/weight, and parents have larger IDs than children. Descending node IDs build depth/root/binary-lifting data without recursion. The LCA merge weight is the minimum possible maximum edge on a path; descending construction (`maximum=true`) gives the maximum possible minimum edge. `bottleneck` explicitly distinguishes disconnected pairs, a valid empty self-path and a nonempty path; the latter also returns the merge edge ID. `lca` accepts all reconstruction nodes, while `bottleneck` and `componentAt` accept original vertices. `componentAt` climbs every threshold-eligible ancestor, including equal-weight merges, and `leaf_count` gives the component size. Increasing/decreasing merge weights make binary lifting valid. Construction requires n≤(INT_MAX+1)/2 so at most 2n−1 nodes fit `int`; preprocessing/storage is O(n log(n+1)) beyond edge sorting, with O(log(n+1)) queries.

## Source review and legacy accounting

Sources below were retrieved and inspected on 2026-09-27. They support the stated algorithms and contracts; no source code was copied. A source review is distinct from verification or online acceptance.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 图的存储](https://oi-wiki.org/graph/save/) (page revision 2026-09-12) | Matrix/list/forward-star tradeoffs and paired reverse arcs; used to assess storage coverage. |
| [Boost 1.89, Compressed Sparse Row Graph](https://www.boost.org/doc/libs/1_89_0/libs/graph/doc/compressed_sparse_row.html) | Source-group offsets, O(n+m) storage and immutable CSR use. |
| [cp-algorithms, Breadth-first search](https://cp-algorithms.com/graph/breadth-first-search.html) | Queue layers, parent recovery and unweighted shortest distances. |
| [cp-algorithms, Bipartite check](https://cp-algorithms.com/graph/bipartite-check.html) (updated 2026-04-02) | BFS coloring and parity criterion. |
| [cp-algorithms, Finding a cycle](https://cp-algorithms.com/graph/finding-cycle.html) | Gray-state cycle recovery. Its simple-graph limitation is extended here using reverse-arc identities. |
| [OI Wiki, 拓扑排序](https://oi-wiki.org/graph/topo/) (updated 2026-04-23) | Kahn, DFS and heap-based lexicographic ordering. |
| [cp-algorithms, Bellman–Ford](https://cp-algorithms.com/graph/bellman_ford.html) | The nth-pass negative-cycle criterion, predecessor recovery, downstream unbounded vertices and all-zero initialization. |
| [KACTL, BellmanFord.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/BellmanFord.h) (Simon Lindholm, 2015-02-23; CC0) | Independent comparison of finite/unreachable/unbounded contracts and downstream propagation. Its specialized pass ordering is not adopted. |
| [Princeton Algorithms, §4.4 Shortest Paths](https://algs4.cs.princeton.edu/44sp/) | Nonnegative Dijkstra and signed DAG shortest/longest topological relaxation. |
| [cp-algorithms, 0-1 BFS](https://cp-algorithms.com/graph/01_bfs.html) | Deque ordering and zero/one front/back insertion. |
| [OI Wiki, 最小生成树](https://oi-wiki.org/graph/mst/) | Kruskal/Prim/Borůvka cut choices, forest behavior, reconstruction trees and bottleneck LCA interpretation. |
| [cp-algorithms, Prim](https://cp-algorithms.com/graph/mst_prim.html) | Dense quadratic and sparse priority-queue alternatives. |

The original `Dijkstra` in `OLD/algorithms.cpp:4488–4526` and its duplicate in `OLD/[1] algorithms.cpp` are preserved. Its n+1 indexing, multisource runs and reset behavior require an explicit compatibility adapter; the new graph APIs are zero-based. The old notebook's Kruskal forest behavior and dense Prim are covered by the new spanning-forest family without adopting its disconnected `-1` sentinel or empty-graph bug. The notebook's global negative-cycle detector is covered by all-vertex Bellman–Ford initialization.

The old Team Notebook `old_topo.cpp` and `old_cycle-find-dfs.cpp` were inspected: recursive reverse postorder and gray-state recovery are replaced by iterative implementations. The broken exploratory destructive cycle-deletion snippet remains archival; it is not an active API or a claimed reusable cycle-enumeration engine. General cycle enumeration has its separate inventory owner.

The legacy bipartite snippet remains in `97-Legacy`; its coloring feature is covered by the new result API with a witness. Canonical P006 DSU already assigns historical potential/parity and component edge-count metadata to their Advanced owners; the graph adapter does not claim those variants.

## Feature-to-test map and execution

The entries below live under `96-Local Testing/04-Graphs`. The shared driver builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. Fresh runs used GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. GCC14 is unavailable here, so execution on that exact compiler floor is not claimed.

| Header / tester stem | Independent coverage | Full result, seed 20260927 |
|---|---|---|
| `01-graph` | 606 exhaustive small graphs, 500 random multigraphs; edge/arc counts and identities, reverse involution, CSR equivalence, independent dense minima/ties, snapshots/copy/move, defaults, input/label adapters and full-range weights. | All three builds passed, 4,173,536 checks each; 20 assertion probes. |
| `02-traversal` | 4,160 exhaustive/random graph cases against Floyd reachability/hop distances, recursive traversal order and enumerated colorings; independently checked simple/odd-cycle witnesses, multiedges/loops, source variants and 200,000-vertex chains. | All three builds passed, 6,508,315 checks each; eight assertion probes. |
| `03-toposort` | 6,951 cases: exhaustive loopless graphs through four vertices and looped graphs through three; 1,200 random graphs/parallel-edge variants; independent first valid permutation oracle through seven vertices, oriented witnesses, 200,000-vertex chains/cycles and 2,000 simultaneously ready vertices. | All three builds passed, 8,622,500 checks each; three assertion probes. |
| `04-dsu` | 67,166 exhaustive directed/undirected graphs through four vertices and 500 random graphs against transitive closure; CSR/reversal, canonical DSU identity and 100,000-vertex chain. | All three builds passed, 6,355,359 checks each; three assertion probes. |
| `05-shortest_path` | 4,096 signed three-vertex digraphs × all eight source subsets; 1,000 random multigraphs and 1,000 signed DAGs; exact Floyd reachability/distances/negative-cycle closure, independent simple-path maxima, oriented path/negative-cycle witnesses, CSR/overloads, full-range sums, helper/legacy reset tests and 100,000-vertex chains. | All three builds passed, 34,808 cases and 4,790,366 checks each; 15 assertion probes. |
| `06-mst` | 1,099 exhaustive weighted graphs and 400 random multigraphs; independent forest-subset minimum/maximum, Floyd minimax/maximin and threshold closures, every-node LCA by ancestor sets, merge-edge certificates, explicit tie IDs, full-range weights/totals, copy/move/snapshots and 100,000-vertex chains. | All three builds passed, 707,913 checks each; 12 assertion probes. |

Graph quick uses exhaustive graphs through two vertices and 70 random cases; full extends to three vertices/500 random cases, stress to 4,000 random cases. DSU quick uses three vertices/50 random cases, full four vertices/500 cases and stress four vertices/4,000 cases. Traversal/toposort quick use smaller exhaustive domains, 100 random cases and 20,000-vertex chains; full uses the tabled corpora, and stress extends random counts to 12,000/10,000 and chains to 500,000. MST quick uses n≤3/40 random cases/1,000-vertex chains; full uses the tabled corpora and stress 2,500 random cases/300,000-vertex chains. Shortest-path quick uses two-vertex exhaustive graphs, 100 multigraphs/100 DAGs and 2,000-vertex chains; full uses the tabled corpus, while stress adds 19,683 looped signed digraphs, 5,000 multigraphs/5,000 DAGs and 300,000-vertex chains. Exact mode coverage is also stated in each runnable entry. Full includes sanitizer builds; quick omits them. Sanitizer runs for all six entries encountered the environment's ptrace/LeakSanitizer restriction and passed after an approved execution outside the sandbox with leak detection enabled. These are completed retries, not skipped configurations. The initial 200,000-vertex all-ready topological fixture exceeded the checked-build timeout because libstdc++ validates the entire heap on each operation; the final fixture uses 2,000 all-ready vertices while retaining the 200,000-vertex deep cases, and passed all three builds.

Reproduce each entry with `python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20260927`, replacing the stem with the corresponding row. `--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry. The shared quick runner passed all six entries with seed 42 from `/tmp`, validating discovery and caller-directory independence:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Integration passed 79 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage/execution, and LOCAL/non-LOCAL workspace compilation. Graph ASan/UBSan coverage comes from the six dedicated full suites; unrelated Core sanitizer suites were not rerun. Repository consistency passed with no errors after the final inventory, aggregate, provenance and checklist updates. All 61 checked precondition probes passed. Extended stress modes are implemented and documented but were not run in this completion pass. Independent review found no remaining correctness, domain, complexity or meaningful coverage gap.

## Representation benchmark

The [benchmark driver](<../96-Local Testing/04-Graphs/90-foundations_benchmark.py>) and [record](<../96-Local Testing/04-Graphs/90-foundations_benchmark.json>) compare the same BFS kernel on adjacency-list and CSR storage. It uses chain, star and connected random multigraphs at 32, 3,000 and 100,000 vertices, seed 20260927, one checked warmup and five samples with repetition for smaller graphs. CSR conversion is timed separately; input graph construction is excluded. All traversal checksums agree. Both representations store O(n+m), and conversion retains the input plus the output.

On this shared host, the 100,000-vertex random graph (499,999 logical edges) had median traversal times 14.34 ms for lists and 9.27 ms for CSR, plus 4.48 ms to construct CSR. Tiny cases slightly favored lists; chain/star results also varied. This supports an explicit reusable CSR option, not an automatic threshold or universal speed claim. Full compiler/CPU/flags and raw samples are in the record. Reproduce with `python3 '96-Local Testing/04-Graphs/90-foundations_benchmark.py'`.

## Dense/sparse algorithm benchmark

The [driver](<../96-Local Testing/04-Graphs/91-path_mst_benchmark.py>) and [record](<../96-Local Testing/04-Graphs/91-path_mst_benchmark.json>) compare sparse/dense Dijkstra and Prim on chains, random sparse multigraphs, complete graphs and disconnected blocks of at most eight vertices. Sizes are 24, 200 and 800; weights are uniformly drawn from [0,10^6], with seed 20260927. Five samples follow checked warmups, with repeated runs for smaller cases. All distances and forest totals/component counts are compared, with Kruskal providing the forest baseline. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately.

| 800-vertex workload | Dijkstra sparse/dense median ms | Prim sparse/dense median ms | Dense setup ms |
|---|---|---|---|
| Chain | 0.013 / 0.868 | 0.021 / 1.216 | 0.216 |
| Random sparse | 0.233 / 1.944 | 0.464 / 2.660 | 0.319 |
| Complete | 8.699 / 9.759 | 9.410 / 8.044 | 4.537 |
| Disconnected blocks | 0.002 / 0.009 | 0.018 / 1.053 | 0.174 |

These shared-host observations support keeping workload-specific choices explicit. Dense setup can outweigh its traversal saving for a single call. No automatic density threshold, universal superiority or timing gate is claimed. The record includes compiler/CPU/flags, memory accounting and every sample. Reproduce with `python3 '96-Local Testing/04-Graphs/91-path_mst_benchmark.py'`.

## Scope boundaries and remaining work

No owned P010 implementation or verification gap remains, so no continuation handoff is needed. The exact GCC14 runtime check and fractional/custom-weight domains are not claimed; the tested environment and supported integer contracts are stated above.

P010 does not own all-pairs/advanced/implicit/temporal/ranked shortest paths, SCCs, dynamic graphs, directed MSTs, sensitivity/second-best MSTs, spanning-tree counting, or fully dynamic MSTs. These remain explicitly planned or existing-unverified in their current inventory rows. Reconstruction-tree bottleneck queries belong here; general LCA and dynamic-tree APIs retain their separate owners. No automatic judge submissions are part of this package.
