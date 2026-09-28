# P011 / GR03 — SCC and Euler trails

These two headers are complete within GR03's static SCC and Euler-trail scope. Both use the verified P010 `Graph`/`CsrGraph` representation, accept loops and parallel edges, ignore weights across the full `lng` domain, leave inputs unchanged and use iterative traversals. This record covers their contracts and independent verification; package integration and comparative benchmarks are recorded by the P011 integration owner.

## SCC contracts and correctness

`tarjanScc(g)` and `kosarajuScc(g)` require directed input and return an owning `SccResult`. `component[u]` indexes the nonempty `groups` partition. IDs run from zero in source-to-sink order: each original arc between different components goes from a smaller ID to a larger ID. Incomparable components need not receive equal IDs between algorithms, and group member order is unspecified. An empty graph has no components. The returned vectors do not borrow the graph.

Tarjan stores separate DFS and active-component stacks. A discovered vertex remains active until its component root closes. A DFS child propagates its lowlink on completion; a previously discovered active neighbor contributes its discovery index. Edges to completed components contribute nothing. Thus a vertex whose lowlink equals its discovery index is the earliest active ancestor reachable from its subtree, and the active-stack suffix through that vertex is exactly its SCC. Components close in sink-to-source order; reversing their IDs and group vector gives the stated order. Each vertex and arc is processed a constant number of times: O(n+m) time, O(n) auxiliary and returned storage.

Kosaraju computes complete DFS finishing order and traverses the transpose in decreasing finish order. For every arc between SCCs, the source component has the larger maximum finishing time. Consequently each transpose traversal reaches precisely the next remaining source component. Explicit DFS frames replace recursion, and all transpose searches use a vector stack. The two passes take O(n+m) time and O(n+m) storage including transpose adjacency and results. Tarjan avoids this transpose allocation; Kosaraju remains an independent alternative with the same public semantics.

`condensationDag(g,scc)` requires an unchanged valid SCC result for the graph. It returns a directed `Graph` with one vertex per component and exactly one unit-weight edge per distinct ordered intercomponent pair. Original weights, multiplicities and edge IDs are not retained. Group-wise traversal and a destination stamp indexed by component eliminate duplicates in O(n+m) time; no sorting/hash-table assumption is needed. Returned storage and workspace total O(k+d) for k components and d condensation edges. The strict ID increase proves acyclicity, and replacing maximal same-component subpaths by component vertices proves preservation of reachability.

## Euler-trail contracts and correctness

`eulerianTrail(g,start=-1)` accepts directed or undirected input. `start=-1` chooses a valid start; any other start must be a valid vertex. A valid vertex that cannot start an Euler trail is an ordinary no-answer case. The result's `exists` flag distinguishes failure from an empty successful trail. On success, `vertices[i]` to `vertices[i+1]` follows original oriented `arcs[i]`, whose logical ID is `edges[i]`; every original logical edge appears once. On failure all three vectors are empty.

For a graph without edges, a nonempty vertex set returns the specified start or vertex zero, and a zero-vertex graph returns successful empty vectors. Isolated vertices do not obstruct a trail among the edge-bearing vertices. Loops count twice in undirected degrees and once each toward directed indegree/outdegree. Parallel edges retain independent logical IDs. The witness order is deterministic for fixed input adjacency/start but has no lexicographic guarantee.

The undirected degree condition is zero or two odd vertices; in the latter case the requested start must be odd. The directed condition is either all balanced, or one vertex with outdegree−indegree=1, one with −1, and every other vertex balanced; the +1 vertex must start an open trail. A start in an edgeless component cannot cover nonempty input. These conditions are necessary. With the valid start, Hierholzer repeatedly follows an unused edge and emits a vertex/entering arc when no unused outgoing edge remains. Reversing the emitted lists splices the closed excursions into one oriented walk. Logical edge marking consumes both reciprocal undirected arcs together, including loops. Every adjacency cursor advances once, so construction is O(n+m) time and O(n+m) storage including the result. Consuming exactly m edges verifies the remaining connectivity condition; otherwise it returns failure. Degree balance plus connectivity of the edge-bearing part is sufficient by the standard cycle-splicing argument, also after adding the closing edge for an open trail.

## Source review and archive accounting

The following sources were retrieved and inspected on 2026-09-27. The new code was independently written from the algorithms and compared with these references; it does not copy their implementations.

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Strongly connected components and the condensation graph](https://cp-algorithms.com/graph/strongly-connected-components.html), last update 2025-12-26 | Kosaraju finishing-order theorem, Tarjan active-stack argument, SCC partition and condensation acyclicity. The new condensation explicitly deduplicates arcs. |
| [AtCoder Library, internal_scc.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_scc.hpp), current source retrieved 2026-09-27 | Tarjan lowlink handling and reversing sink-first IDs to source-first IDs. Its reference cites Tarjan, *Depth-First Search and Linear Graph Algorithms*; that paper itself was not separately inspected. The new implementation uses explicit stacks. |
| [cp-algorithms, Finding the Eulerian path in O(M)](https://cp-algorithms.com/graph/euler_path.html), last update 2026-09-18 | Undirected multigraph/loop degree conditions, exclusion of isolates, iterative Hierholzer and unused-edge connectivity check. Its matrix example does not establish the new adjacency-list complexity. |
| [KACTL, EulerWalk.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/EulerWalk.h), Simon Lindholm, 2019-12-31, CC0 | Directed/undirected logical edge marking, iterative trail recovery and start-sensitive validity. The new API checks directed/undirected degree conditions explicitly and returns both arc and logical-edge certificates. |

`OLD/Team Notebook/src/graph/old_kosaraju.cpp` was inspected; its two DFS passes and component listing are covered by the new Kosaraju implementation. Its recursive stack use, unnecessary parent exclusion and repeated-run append behavior are not preserved.

The `TarjanSCC` and `EulerPath` sections of `OLD/Team Notebook/src/algsbetter.cpp` (starting at lines 1429 and 1590), `algs.cpp` (1449 and 1532), and `test.cpp` (212 and 394) were all inspected. The Tarjan copies provide SCC members, representative-root labels and a condensation adjacency list with duplicates; `component`, `groups`, and `condensationDag` cover those features with compact ordered IDs and deduplication. The Euler copies provide repeatable undirected Hierholzer with logical edge marks, odd-degree selection and a vertex witness; the new implementation also supports directed graphs, explicit starts and arc/edge witnesses. The old `{0}` result on a zero-vertex graph is replaced by the documented empty successful result. Archived names/signatures are not active compatibility APIs. Original archive bytes remain untouched. The unrelated `Tarjan` in `OLD/algorithms.cpp:4865` is articulation/bridge lowlink; its name is not evidence of SCC coverage. The archived `EulerTourTree` concerns subtree flattening, not Euler trails.

Incremental SCC belongs to `70-incrementalscc.hpp`; SCC-based transitive closure/reduction belongs to `40-reachability.hpp`. Euler-tour counting belongs to `53-graph_counting.hpp`, and Chinese postman/route augmentation to `69-tjoin.hpp`. These remain separately owned inventory work. This completion does not claim those extensions or automatic online submissions.

## Feature-to-test map and results

Both entries reuse the graph test driver with non-removable assertions, failing graph/operation output, reproducible seeds and subprocess timeouts. The driver compiles GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g` ASan/UBSan with leak detection. Fresh execution used GCC 16.2.1 (20260810), CPython 3.14.7, Linux x86-64. GCC14 itself was unavailable; exact-floor execution is not claimed.

| Entry / features | Independent verification | Full result, seed 20260927 |
|---|---|---|
| `08-scc_tester.py`: both algorithms, component/group contract, ID order, condensation | Floyd transitive closure gives independent mutual-reachability equivalence; exact intercomponent edge sets and condensed reachability; all loopless digraphs through n=4, looped through n=3, all two-vertex arc multiplicities 0..2; 1,800 random multigraphs plus reversal/relabeling variants. | 7,030 cases and 3,772,582 checks in each of all three configurations. Four checked assertion probes passed. |
| SCC representation, lifetime and boundaries | Every small case runs on Graph and CSR with both algorithms; 200,000-vertex chain and cycle check stack safety/order/condensation; cross-edge fixture, empty/singleton/isolate cases, ignored extreme signed weights, result move/repeated calls and CSR snapshot after graph mutation. | Included in the same full results. |
| `10-euleriantrail_tester.py`: existence, selectable start and all witnesses | Independent subset DP enumerates possible start vertices for trails using each logical edge once, without degree/connectivity tests; every start checked; all looped directed graphs through n=3 and undirected through n=4, two-vertex multiplicities 0..2; 1,500 random multigraph/generated-trail cases and reversal variants. | 3,390 cases and 14,683,272 checks in each of all three configurations. Three checked assertion probes passed. |
| Euler representations, lifecycle and boundaries | Graph/CSR in every small case, oriented arc/logical-ID validation, zero-edge and n=0 successes, wrong legal starts, degree failures, balanced disconnected graphs, loops and isolates, extreme ignored weights, result move/repeated calls, mutation/snapshot, 200,000-vertex directed/undirected chains and cycles. | Included in the same full results. |

Reproduction commands:

```bash
python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode full --seed 20260927
```

The first sandboxed sanitizer execution of each suite hit the environment's LeakSanitizer/ptrace restriction. Approved retries of the same full corpus using `--configuration ASan-UBSan` outside the sandbox passed with leak detection enabled. No sanitizer configuration was skipped. Quick/full/stress coverage is documented in each runnable entry; quick omits sanitizers, and stress expands random corpora to 10,000 rounds and chain/cycle sizes to 500,000, with SCC also enumerating all looped four-vertex digraphs. Stress mode was implemented but not run in this completion. Final standalone/aggregate/multiple-translation-unit integration and package bookkeeping belong to the P011 integration record.

No known owned feature or verification gap remains for these two headers. Finite tests complement the algorithm arguments; they do not prove all inputs.
