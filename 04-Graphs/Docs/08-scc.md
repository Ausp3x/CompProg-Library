# 08-scc.hpp — evidence

Owned by package P011 / GR03 (with `07-lca.hpp`, `09-bridges_articulation.hpp` and `10-euleriantrail.hpp`; GR26 `11-functionalgraph.hpp` completes P011); see [00-notes.md](00-notes.md#p011-package-record) for the package record. The header is complete within GR03's static SCC scope. It uses the verified P010 `Graph`/`CsrGraph` representation, accepts loops and parallel edges, ignores weights across the full `lng` domain, leaves inputs unchanged and uses iterative traversals.

## Contracts

### SccResult, tarjanScc and kosarajuScc

`tarjanScc(g)` and `kosarajuScc(g)` require directed input and return an owning `SccResult`. `component[u]` indexes the nonempty `groups` partition. IDs run from zero in source-to-sink order: each original arc between different components goes from a smaller ID to a larger ID. Incomparable components need not receive equal IDs between algorithms, and group member order is unspecified. An empty graph has no components. The returned vectors do not borrow the graph.

Tarjan stores separate DFS and active-component stacks. A discovered vertex remains active until its component root closes. A DFS child propagates its lowlink on completion; a previously discovered active neighbor contributes its discovery index. Edges to completed components contribute nothing. Thus a vertex whose lowlink equals its discovery index is the earliest active ancestor reachable from its subtree, and the active-stack suffix through that vertex is exactly its SCC. Components close in sink-to-source order; reversing their IDs and group vector gives the stated order. Each vertex and arc is processed a constant number of times: O(n+m) time, O(n) auxiliary and returned storage.

Kosaraju computes complete DFS finishing order and traverses the transpose in decreasing finish order. For every arc between SCCs, the source component has the larger maximum finishing time. Consequently each transpose traversal reaches precisely the next remaining source component. Explicit DFS frames replace recursion, and all transpose searches use a vector stack. The two passes take O(n+m) time and O(n+m) storage including transpose adjacency and results. Tarjan avoids this transpose allocation; Kosaraju remains an independent alternative with the same public semantics.

### condensationDag

`condensationDag(g,scc)` requires an unchanged valid SCC result for the graph. It returns a directed `Graph` with one vertex per component and exactly one unit-weight edge per distinct ordered intercomponent pair. Original weights, multiplicities and edge IDs are not retained. Group-wise traversal and a destination stamp indexed by component eliminate duplicates in O(n+m) time; no sorting/hash-table assumption is needed. Returned storage and workspace total O(k+d) for k components and d condensation edges. The strict ID increase proves acyclicity, and replacing maximal same-component subpaths by component vertices proves preservation of reachability.

## Feature-to-test map

`96-Local Testing/04-Graphs/08-scc_tester.py` reuses the graph test driver with non-removable assertions, failing graph/operation output, reproducible seeds and subprocess timeouts. The driver compiles GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g` ASan/UBSan with leak detection. Quick/full/stress coverage is documented in the runnable entry; quick omits sanitizers, and stress expands random corpora to 10,000 rounds and chain/cycle sizes to 500,000, and also enumerates all looped four-vertex digraphs.

| Operation | Test | Oracle |
|---|---|---|
| `SccResult`, `tarjanScc`, `kosarajuScc`: component/group contract and ID order | All loopless digraphs through n=4, looped through n=3, all two-vertex arc multiplicities 0..2; 1,800 random multigraphs plus reversal/relabeling variants | Floyd transitive closure gives independent mutual-reachability equivalence |
| `condensationDag` (duplicate-free) | Same corpus | Exact intercomponent edge sets and condensed reachability |
| Representation, lifetime and boundaries | Every small case on Graph and CSR with both algorithms; 200,000-vertex chain and cycle (stack safety/order/condensation); cross-edge fixture, empty/singleton/isolate cases, ignored extreme signed weights, result move/repeated calls and CSR snapshot after graph mutation | Same oracles |
| Preconditions | Four checked assertion probes | Expected assertion failure |

Finite tests complement the algorithm arguments; they do not prove all inputs.

## Commands and results

2026-10-07, GCC 16.2.1, CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `08-scc_tester.py --mode full --seed 20260927` | PASS 3 configurations, 7,030 cases, 3,772,582 checks, 4 probes |
| `08-scc_tester.py --mode stress --seed 1` | PASS 3 configurations, 82,816 cases |
| `02-integration.py` and `02-integration.py --sanitizers` | PASS: 102 standalone/aggregate headers, scalar/available-AVX2 multi-TU, workspace, sanitizer self-tests |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode stress --seed 1
python3 '96-Local Testing/04-Graphs/00-decomposition_benchmark.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

[Benchmark driver](<../../96-Local Testing/04-Graphs/00-decomposition_benchmark.py>) and `96-Local Testing/04-Graphs/00-decomposition_benchmark.json` (shared with `07-lca.hpp`) contain CPU/compiler/flags, workload generation, seed, all five samples and medians. The C++ driver warms up both alternatives, includes allocation in construction time, excludes input generation and checks every result outside the timed region. Small/common/large cases use n=32, 2,000 and 100,000; SCC uses chains, cycles, random graphs and SCC blocks. Memory is O(n+m) for Kosaraju's transpose and results.

Recorded 2026-10-07 medians in milliseconds (flags `-O2 -DNDEBUG -mno-avx2`, i9-11900H, shared machine; 100,000 vertices):

| Workload | Tarjan SCC | Kosaraju SCC |
|---|---|---|
| Chain | 3.237 | 5.812 |
| Cycle | 1.462 | 4.271 |
| Chain plus 400,000 random arcs | 17.258 | 28.300 |

Tarjan avoids transpose construction and was faster in these samples; both SCC algorithms remain available. These are shared-machine observations, especially noisy for microsecond-sized cases. There is no automatic dispatch threshold or universal speed claim. No Barrett/Montgomery or handwritten ISA specialization was introduced.

## Sources

Retrieved and inspected on 2026-09-27. The code was independently written from the algorithms and compared with these references; it does not copy their implementations.

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Strongly connected components and the condensation graph](https://cp-algorithms.com/graph/strongly-connected-components.html), last update 2025-12-26 | Kosaraju finishing-order theorem, Tarjan active-stack argument, SCC partition and condensation acyclicity. The new condensation explicitly deduplicates arcs. |
| [AtCoder Library, internal_scc.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_scc.hpp), current source retrieved 2026-09-27 | Tarjan lowlink handling and reversing sink-first IDs to source-first IDs. Its reference cites Tarjan, *Depth-First Search and Linear Graph Algorithms*; that paper itself was not separately inspected. The new implementation uses explicit stacks. |

Legacy: `OLD/Team Notebook/src/graph/old_kosaraju.cpp` was inspected; its two DFS passes and component listing are covered by the new Kosaraju implementation. Its recursive stack use, unnecessary parent exclusion and repeated-run append behavior are not preserved. The `TarjanSCC` sections of `OLD/Team Notebook/src/algsbetter.cpp` (line 1429), `algs.cpp` (1449) and `test.cpp` (212) were inspected. They provide SCC members, representative-root labels and a condensation adjacency list with duplicates; `component`, `groups` and `condensationDag` cover those features with compact ordered IDs and deduplication. Archived names/signatures are not active compatibility APIs, and original archive bytes remain untouched. The unrelated `Tarjan` in `OLD/algorithms.cpp:4865` is articulation/bridge lowlink; its name is not evidence of SCC coverage.

## Limits and handoffs

Incremental SCC belongs to `70-incrementalscc.hpp`; SCC-based transitive closure/reduction belongs to `40-reachability.hpp`; dynamic SCC remains with its separate batch owner. These are not claimed here. No `CXX=g++-14` floor run is recorded for this header.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,772,582 checks), integration and benchmark passed on g++ 16; stress not run.
- 2026-10-07: re-audit, no findings for this header; brace rule and comment cap applied; full suite, stress, integration and benchmark passed on g++ 16.
