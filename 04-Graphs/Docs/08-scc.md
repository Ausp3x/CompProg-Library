# 08-scc.hpp — evidence

Owned by package P011 / GR03 (with `07-lca.hpp`, `09-bridges_articulation.hpp` and `10-euleriantrail.hpp`; GR26 `11-functionalgraph.hpp` completes P011); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions) and [00-notes.md](00-notes.md#p011-gr03-omissions-2026-11-15); GR03 sources are in [00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03). It uses the verified P010 `Graph`/`CsrGraph` representation, accepts loops and parallel edges, ignores weights across the full `lng` domain, leaves inputs unchanged and uses iterative traversals.

## Contracts

### SccResult, tarjanScc and kosarajuScc

`tarjanScc(g)` and `kosarajuScc(g)` require directed input and return an owning `SccResult`. `component[u]` indexes the nonempty `groups` partition. IDs run from zero in source-to-sink order: each original arc between different components goes from a smaller ID to a larger ID. The relative order of components that cannot reach each other is unspecified for each algorithm and may differ between them (user decision 2026-11-16), and group member order is unspecified. An empty graph has no components. The returned vectors do not borrow the graph.

Tarjan stores separate DFS and active-component stacks. A discovered vertex remains active until its component root closes. A DFS child propagates its lowlink on completion; a previously discovered active neighbor contributes its discovery index. Edges to completed components contribute nothing. Thus a vertex whose lowlink equals its discovery index is the earliest active ancestor reachable from its subtree, and the active-stack suffix through that vertex is exactly its SCC. Components close in sink-to-source order; reversing their IDs and group vector gives the stated order. Each vertex and arc is processed a constant number of times: O(n+m) time, O(n) auxiliary and returned storage.

Kosaraju computes complete DFS finishing order and traverses the transpose in decreasing finish order. For every arc between SCCs, the source component has the larger maximum finishing time. Consequently each transpose traversal reaches precisely the next remaining source component. Explicit DFS frames replace recursion, and all transpose searches use a vector stack. The two passes take O(n+m) time and O(n+m) storage including transpose adjacency and results. Tarjan avoids this transpose allocation; Kosaraju remains an independent alternative with the same public semantics.

### condensationDag

`condensationDag(g,scc)` requires an unchanged valid SCC result for the graph. It returns a directed `Graph` with one vertex per component and exactly one unit-weight edge per distinct ordered intercomponent pair. Original weights, multiplicities and edge IDs are not retained. Group-wise traversal and a destination stamp indexed by component eliminate duplicates in O(n+m) time; no sorting/hash-table assumption is needed. Returned storage and workspace total O(k+d) for k components and d condensation edges. The strict ID increase proves acyclicity, and replacing maximal same-component subpaths by component vertices proves preservation of reachability.

### attractingComponents and isSemiconnected

Both take a directed graph and a valid SCC result for it (from `tarjanScc` or `kosarajuScc`; the sizes are asserted) and cost O(n + m) time and O(k) memory for `k` components. `attractingComponents(g, scc)` returns the increasing IDs of the components no arc leaves (NetworkX `attracting_components`; the members are `scc.groups[c]`). `isSemiconnected(g, scc)` is true exactly when every vertex pair has a path in at least one direction, and true on the empty graph by convention. Proof: the graph is semiconnected exactly when its condensation has a Hamiltonian path; a DAG with a Hamiltonian path has exactly one topological order, and the component IDs are a topological order, so the test is whether every pair `c, c + 1` is joined by an arc.

### strongConnectivityAugmentation

`strongConnectivityAugmentation(g, scc)` returns a minimum set of arcs whose addition makes `g` strongly connected (Eswaran and Tarjan 1976, CSES 1685). Each arc joins two component representatives `scc.groups[c][0]`. It is empty for at most one component, and otherwise has exactly `max(s, t)` arcs for `s` source and `t` sink components (an isolated component counts as both), never a loop. That count is a lower bound because every source needs a new incoming arc and every sink a new outgoing one. Construction on the condensation: a DFS from each source in increasing ID order, over vertices no earlier search entered, stops at the first sink it enters and pairs it with that source. Every entered vertex then reaches a paired sink: it either lay on the path when a search stopped, or was finished, so all its successors were entered. Every unpaired sink is reached from a paired source, through the vertex where its source's search was blocked. Arcs `tail_i → head_(i+1)` join the `p` pairs into one cycle `C`, where `p >= 2` or the single pair has distinct ends whenever there are at least two components. Then `min(s, t) − p` arcs run from unpaired sinks to unpaired sources, and the rest run from the first tail to the remaining unpaired sources, or from the remaining unpaired sinks to the first head. Every sink reaches `C` and `C` reaches every source, so the result is strongly connected, with `p + max(s − p, t − p)` arcs. O(n + m) time, O(k + d) memory. The first arc also answers CSES 1682 Flight Routes Check: it runs from a sink component to a different source component, so its tail cannot reach its head.

### maximumWeightWalk

`maximumWeightWalk(g, scc, w)` asserts `w.size() == n` and `w[u] >= 0` once at entry. It returns `{value, components}`, where `value` (`lll`) is the largest total weight of the distinct vertices of a directed walk and `components` lists the components the walk passes in order (CSES 1686 Coin Collector, OI Wiki's condensation example). A walk entering a component can visit all of it, and nonnegative weights make that optimal, so this is a longest vertex-weighted path in the condensation. Component IDs are topological, so one pass in ID order over `condensationDag` relaxes `best[d] = max(best[d], best[c])`, keeping the first strict improvement as the predecessor. The witness is fixed: it ends at the smallest component ID whose heaviest walk ending there has the largest value, and steps back from each component `c` to the smallest-ID component with an arc into `c` among those of largest positive ending value, stopping when every such value is 0; the arcs' order in `g` never matters. The result is `{0, {}}` exactly when `n == 0`; otherwise the list is nonempty, consecutive components are joined by an arc, and their weights sum to `value`, which stays below `n · 2^63`. O(n + m) time, O(k + d) memory.

### reachabilityRealization

`reachabilityRealization(g)` treats the arcs of a directed `g` as requests `(a, b)` (loops need nothing) and returns a minimum set of arcs, none a loop, giving a path `a → b` for every request (CSES 1699 Flight Route Requests). For each weak component of the requests with `s` vertices it adds `s − 1` arcs along its vertices in the order of `tarjanScc(g).groups` concatenated (a topological order), and closes them into a cycle with one more arc when the component holds a nontrivial SCC. The order is fixed: first the chain arcs, each weak component's chain interleaved in that vertex order (each arc listed when its head is reached), then the closing arcs `(last, first)` in increasing order of `first`, independent of the DSU's representatives. Every request then follows the chain, or the cycle. Minimality: the requests of one weak component must lie in one weak component of the answer, which needs at least `s − 1` arcs, plus one more when the requests contain a directed cycle, since a tree has no directed cycle. Merging several components never saves arcs. O((n + m) · α(n)) time, O(n) memory.

## Feature-to-test map

`96-Local Testing/04-Graphs/08-scc_tester.py` uses the shared contest runner, which compiles GNU++20 optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1 -g` ASan/UBSan with leak detection. Quick/full/stress coverage is documented in the runnable entry; quick omits sanitizers, and stress expands random corpora to 10,000 rounds and chain/cycle sizes to 500,000, and also enumerates all looped four-vertex digraphs.

| Operation | Test | Oracle |
|---|---|---|
| `SccResult`, `tarjanScc`, `kosarajuScc`: component/group contract and ID order | All loopless digraphs through n=4, looped through n=3, all two-vertex arc multiplicities 0..2; 1,800 random multigraphs plus reversal/relabeling variants | Floyd transitive closure gives independent mutual-reachability equivalence |
| `condensationDag` (duplicate-free) | Same corpus | Exact intercomponent edge sets and condensed reachability |
| Representation, lifetime and boundaries | Every small case on Graph and CSR with both algorithms; 200,000-vertex chain and cycle (stack safety/order/condensation); cross-edge fixture, empty/singleton/isolate cases, ignored extreme signed weights, result move/repeated calls and CSR snapshot after graph mutation | Same oracles |
| `attractingComponents`, `isSemiconnected` | Every case of the corpus (Graph and CSR, Tarjan and Kosaraju results) | Floyd closure: a component is a sink iff nothing its members reach lies outside it; every pair reachable one way |
| `strongConnectivityAugmentation` | Same corpus; 200,000-vertex chain (`{(n − 1, 0)}`) and cycle | Count `max(sources, sinks)` from the closure (0 for at most one component), representatives `groups[c][0]`, no loops, closure of `g` plus the arcs all true; with at least two components the first arc's tail cannot reach its head in `g` (closure; the CSES 1682 witness) |
| `maximumWeightWalk` | Same corpus with per-case weights (zeros, 0..3 for ties, exactly 2^62, `INT64_MAX`, uniform up to 2^62); the chain with weights near 2^62 | Heaviest chain of components ordered by closure reachability (memoized); empty iff n == 0; distinct consecutive components joined by an arc of `g`, member weights summing to the value; the exact documented witness from closure-memoized ending values |
| `reachabilityRealization` | Every graph case; the chain (exactly the chain) and the cycle (one Hamiltonian cycle); two 2-cycles whose closing arcs sort differently by first and by last vertex | Closure of the result covers every request, no loops; count equals the weak-component formula, which an exhaustive search over all loopless arc subsets confirms through n = 4; the exact documented order from label-propagation weak components |
| Preconditions | 17 checked assertion probes: the original four and `attracting-undirected`, `attracting-size`, `semi-undirected`, `semi-size`, `augment-undirected`, `augment-size`, `walk-negative`, `walk-size`, `realize-undirected`, and `condensation-size-large`, `attracting-size-large`, `semi-size-large`, `walk-size-large` (one entry too many) | Expected assertion failure |

## Commands and results

Latest run 2026-11-16 (P011 /finish GR03; experiment: none). Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, the runner's optimized, checked and ASan/UBSan builds; GCC 16.2.1 unless stated, beside other suites and a running mutation campaign:

| Command | Result |
|---|---|
| `08-scc_tester.py --mode quick` | PASS, 361 cases, 607,948 checks, 17 probes; optimized run 0.07 s, checked 1.08 s; MEMORY peak 54 MB (build cache warm) |
| `08-scc_tester.py --mode full --seed 20261116` | PASS, 3 configurations, 7,031 cases and 7,654,168 checks per build, 17 probes; runs 0.92 s optimized, 4.01 s checked, 12.56 s ASan-UBSan; MEMORY peak 1083 MB |
| `CXX=g++-14 08-scc_tester.py --mode full --seed 20261116` (GCC 14.4.1, floor) | PASS, 3 configurations, same counts; MEMORY peak 1040 MB |
| `08-scc_tester.py --mode stress --seed 20261117` | PASS, 3 configurations, 82,817 cases and 46,009,356 checks per build; runs 4.02 s optimized, 19.51 s checked, 74.05 s ASan-UBSan; MEMORY peak 1049 MB |
| `05-extraction.py ops --filter 04-Graphs/08-scc.hpp` | PASS, 9 operations in 9 distinct extractions on g++ and g++-14 |
| `05-extraction.py testers --filter 08-scc_tester` | PASS, 1 quick run on the extracted build |
| `00-decomposition_benchmark.py` | PASS, every result checked; medians under Benchmarks |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar and available AVX2 multi-TU, workspace; MEMORY peak 3129 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode full --seed 20261116
CXX=g++-14 python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode full --seed 20261116
python3 '96-Local Testing/04-Graphs/08-scc_tester.py' --mode stress --seed 20261117
python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/08-scc.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 08-scc_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/08-scc.hpp
python3 '96-Local Testing/04-Graphs/00-decomposition_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

[Benchmark driver](<../../96-Local Testing/04-Graphs/00-decomposition_benchmark.py>) and its git-ignored `00-decomposition_benchmark.json` log (shared with `07-lca.hpp`) hold workload generation, seed, all five samples and medians. The C++ driver warms up every alternative, includes allocation in construction time, excludes input generation and checks every result outside the timed region. Small/common/large cases use n=32, 2,000 and 100,000; SCC uses chains, cycles, random graphs and SCC blocks. Memory is O(n+m) for Kosaraju's transpose and results.

Recorded 2026-11-15 medians in milliseconds (from the log: GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`, i9-11900H, seed 20260927; shared machine, beside a mutation campaign; 100,000 vertices):

| Workload | Tarjan SCC | Kosaraju SCC |
|---|---|---|
| Chain | 4.351 | 8.089 |
| Cycle | 1.984 | 6.169 |
| Chain plus 400,000 random arcs | 21.944 | 45.817 |
| SCC blocks of 32 | 2.413 | 7.076 |

Tarjan avoids transpose construction and was faster in these samples; both SCC algorithms remain available. These are shared-machine observations, especially noisy for microsecond-sized cases. There is no automatic dispatch threshold or universal speed claim. No Barrett/Montgomery or handwritten ISA specialization was introduced.

## Sources

Retrieved and inspected on 2026-09-27. The code was independently written from the algorithms and compared with these references; it does not copy their implementations.

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Strongly connected components and the condensation graph](https://cp-algorithms.com/graph/strongly-connected-components.html), last update 2025-12-26 | Kosaraju finishing-order theorem, Tarjan active-stack argument, SCC partition and condensation acyclicity. The new condensation explicitly deduplicates arcs. |
| [AtCoder Library, internal_scc.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_scc.hpp), current source retrieved 2026-09-27 | Tarjan lowlink handling and reversing sink-first IDs to source-first IDs. Its reference cites Tarjan, *Depth-First Search and Linear Graph Algorithms*; that paper itself was not separately inspected. The new implementation uses explicit stacks. |

Legacy: `OLD/Team Notebook/src/graph/old_kosaraju.cpp` was inspected; its two DFS passes and component listing are covered by the new Kosaraju implementation. Its recursive stack use, unnecessary parent exclusion and repeated-run append behavior are not preserved. The `TarjanSCC` sections of `OLD/Team Notebook/src/algsbetter.cpp` (line 1429), `algs.cpp` (1449) and `test.cpp` (212) were inspected. They provide SCC members, representative-root labels and a condensation adjacency list with duplicates; `component`, `groups` and `condensationDag` cover those features with compact ordered IDs and deduplication. Archived names/signatures are not active compatibility APIs, and original archive bytes remain untouched. The unrelated `Tarjan` in `OLD/algorithms.cpp:4865` is articulation/bridge lowlink; its name is not evidence of SCC coverage.

## Limits and handoffs

Incremental SCC belongs to `70-incrementalscc.hpp`; SCC-based transitive closure/reduction belongs to `40-reachability.hpp`; fully dynamic SCC has no row. These are not claimed here.

Eswaran–Tarjan (1976), the source of the augmentation bound, is named but not fetched ([00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03)); the proof under `strongConnectivityAugmentation` stands alone and the closure oracle checks it. `isSemiconnected` returns true on the empty graph, where NetworkX raises.

## Aliases

Search aliases (P263, grab --find): SCC (strongly connected components), condensation graph; 强连通分量, 缩点, 强连通分支, 强连通组件.

Search aliases (P011 GR03, grab --find): strong connectivity augmentation, fewest arcs to make strongly connected, Eswaran–Tarjan, Flight Routes Check, attracting components, sink components, semiconnected, unilaterally connected, Coin Collector, maximum weight walk, Flight Route Requests, minimum reachability realization; 最少加边强连通, 单向连通.

## Mutation testing

Record: mewt 4.0.0 on blob `2f7daec9ba908c2067898a2b0d1b4290cb00db29` (2026-11-16, seed 1, 2 lines changed since blob `a0e208580202`): 4 mutants, 4 caught (0 by timeout), 0 survived, 0 untested. Stage seconds: quick 5.

Earlier changed-line campaign: mewt 4.0.0 on blob `a0e2085802023ce004be4399773ee29d5531ea14` (2026-11-15, seed 1, 7 lines changed since blob `0a5da3e28125`): 36 mutants, 35 caught (0 by timeout), 1 survived, 0 untested. Stage seconds: quick 54, full on the optimized build 10, full on the checked and ASan-UBSan builds 117. The survivor `COS 470` (line 174, the closing-arc loop's `u < g.n` → `!=`) is equivalent: the counter steps by 1 from 0.

Earlier whole-file campaign: mewt 4.0.0 on blob `0a5da3e28125d3c66ca513e740f7d7c0c149725a` (2026-11-15, seed 1, whole file): 481 mutants, 444 caught (3 by timeout), 37 survived, 0 untested. Stage seconds of its first pass: quick 859, full on the optimized build 23, full on the checked and ASan-UBSan builds 769. It left 41 survivors. The rerun with the extended tester (same `--workdir`) caught four of them: `COS 334`, `354`, `369` and `434` (lines 76, 89, 102, 145), size assertions loosened from `==` to `>=`, which the probes `condensation-size-large`, `attracting-size-large`, `semi-size-large` and `walk-size-large` now pass one entry too many.

Two survivors showed outputs the contract left open, so the contract now fixes them (header changed after this blob):
- `COS 449` (line 152), `best[c] > best[d]` → `>=`: a tie changed the walk's witness. The contract now names the witness (smallest IDs on ties, no zero-value predecessor), and the tester checks it exactly; hand-applied to the new header (2026-11-15), the mutant fails `walk exact documented witness`.
- `AS 219` (line 164), the union's arguments swapped: the DSU's representatives ordered the closing arcs. The closing arcs now follow their head vertex, so the output no longer depends on the representatives, and the tester checks the exact order, with a fixed pair of 2-cycles that sorts differently by first and by last vertex; hand-applied to the new header, `AS 219` passes as an equivalent, and ordering by the last vertex instead fails `realization exact documented order`.

The other 35 are equivalent (line numbers of that blob):

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 271`, `281`, `306`, `311`, `336`, `361`, `386`, `396`, `406`, `411`, `416`, `436`, `466` `<` → `!=` | 20, 25, 49, 54, 78, 95, 119, 126, 132, 135, 136, 148, 174 | Each counter steps by 1 from at most its bound. |
| `COS 421`, `426` `<` → `!=` | 137, 138 | `i` starts at `both`, the smaller of the two sizes, and steps by 1. |
| `COS 316`, `it != order.rend()` → `<` | 61 | Random-access reverse iterators stepping by 1 from `rbegin()`. |
| `COS 278`, `323`, `453` `!= -1` → `> -1`; `COS 287`, `292`, `327`, `392`, `402`, `457` `== -1` → `<= -1` | 21, 63, 155, 27, 29, 69, 123, 130, 171 | The operand is `-1` or an index, never below `-1`. |
| `AS 205`, `206`, `211`, swapped `min` arguments | 29, 32, 133 | `min` is symmetric. |
| `COS 299`, `low[u] == ord[u]` → `>=` | 33 | `low[u] <= ord[u]` always holds. |
| `BL 227`, `seen[root] = true` → `false` | 51 | Equivalent under the unspecified order of mutually unreachable components (user decision 2026-11-16). A descendant's arc back to `root` opens a second frame of `root`, which continues `root`'s own arc list there, so `root`'s later children are found inside that descendant's frame and `order` changes; let a vertex's finish be its last position in `order`. A frame enters only vertices its own vertex reaches (the second frame of `root` opens from a vertex with an arc to `root`), every vertex finishes after the frames nested in its first frame, and `root`'s last occurrence follows its whole tree. Kosaraju's second pass needs, for components `C` and `D` with `C` reaching `D`, the latest finish in `C` after the latest in `D`. If `C` is entered first, at `x`, all of `D` is still unvisited and is entered inside `x`'s frame, so it finishes before `x` (before `root`'s last occurrence when `x` is `root`). If `D` is entered first, every vertex entered while its first vertex's frame is open is reachable from `D`, so none is in `C`, and `C` finishes later. The partition and the topological IDs therefore stay valid, and the second pass skips the earlier occurrence of `root` as already assigned. Only components that cannot reach each other reorder: the review's arcs 0→1, 0→3, 1→0, 1→2 give `component` `[0,0,2,1]`, the mutant `[0,0,1,2]`; the second review's brute force on a mutated copy found valid partitions with topological IDs on all 66,067 looped digraphs up to 4 vertices and 400,000 random multigraphs. |
| `COS 343`, `d != c` → `d > c`; `COS 358`, `component[a.to] != c` → `>` | 82, 93 | Component IDs are topological: an arc between components always goes to a larger ID. |
| `COS 346`, `seen[d] != c` → `seen[d] < c` | 82 | `seen[d]` is `-1` or an earlier, smaller component. |
| `COS 380`, `k <= 1` → `k == 1` | 115 | For `k == 0` every loop is empty and the result is the same empty set. |
| `COS 461`, `group.size() > 1` → `!= 1` | 173 | A component is never empty. |

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,772,582 checks), integration and benchmark passed on g++ 16; stress not run.
- 2026-10-07: re-audit, no findings for this header; brace rule and comment cap applied; full suite, stress, integration and benchmark passed on g++ 16; independent review: all eight package findings confirmed fixed.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-15: P011 GR03, added `attractingComponents`, `isSemiconnected`, `strongConnectivityAugmentation`, `maximumWeightWalk` (user decision) and `reachabilityRealization`; completeness sweeps (three passes and a gap recheck); full (seed 20261115) on g++ and g++-14, stress, extraction, integration and the benchmark passed; independent review: a backwards sentence in the augmentation proof and the merge wording fixed, the weight assertion moved to entry; whole-file mutation campaign, four size-assertion probes added, the walk witness and the closing-arc order fixed in the contract with exact tests, then a campaign on the changed lines (below).
- 2026-11-16: P011 /finish GR03: `reachabilityRealization` calls `graphComponents`; tests added for the CSES 1682 witness (the first augmentation arc's head is unreachable from its tail), weights up to `INT64_MAX` in `maximumWeightWalk` and the exact witness and realization order; the order of mutually unreachable components stays unspecified (user decision, `BL 227` above); probe count corrected to 17; full (seed 20261116) on g++ and g++-14, stress (seed 20261117), extraction and integration passed; changed-line mutation campaign: 4 mutants, all caught.
