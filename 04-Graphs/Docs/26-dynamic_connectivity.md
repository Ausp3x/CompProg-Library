# 26-dynamic_connectivity.hpp — evidence

Owned by package P047 / GR12; omissions are in [00-notes.md](00-notes.md#p047-omissions). Contest profile: offline time-segment connectivity over Data Structures 63 (`OfflineDeletion`) and 09 (`RollbackDSU`, `RollbackDSUAggregate`), online bridge maintenance after cp-algorithms, and an offline range edge component count. The pinned `OnlineBridges` excerpt (`OLD/algorithms.cpp:4906-5003`) was replaced; the monolith map records it as a P047 audited replacement.

## Contracts

### OfflineDynamicConnectivity

`OfflineDynamicConnectivity<M>(n, T, mon)` covers vertices `[0, n)` and times `[0, T)` with `0 ≤ T ≤ 2^29` (asserted by `OfflineDeletion`). `M` is a commutative monoid (`S`, `op`, `e`), stored as `m`, default `Sum<int>`; every vertex value starts at `e()`. Loops and parallel edges are allowed and count as separate copies.

- `addEdge(l, r, u, v)`: the edge is present during `[l, r)`, with `0 ≤ l ≤ r ≤ T`.
- `insertEdge(t, u, v)`, `eraseEdge(t, u, v)` with `0 ≤ t ≤ T`: event forms keyed by the unordered pair. An erase closes the latest open copy of the same pair; that copy must exist and must have opened at time ≤ t (asserted). Copies still open at the first `run` or `bipartiteness` call close at `T`, so every erase must come before that call.
- `addValue(l, r, u, x)`: during `[l, r)`, `x` is folded into `u`'s value.
- `query(t)`, `t ∈ [0, T)`, marks a time.
- `run(answer)` calls `answer(t, dsu)` once per marked time, in increasing order. `dsu` is a `const RollbackDSUAggregate<M>&` that then holds exactly the edges present at `t`. It offers `isSameSet`, `count` (components), `getSize`, `componentValue` (the `op` of the active values of the component's vertices) and `edgeCount` (present edge copies inside the component, loops included). The reference is valid only during the call.
- `bipartiteness(answer)` calls `answer(t, bipartite)` the same way. A loop makes the graph non-bipartite. Values are ignored.

Each interval covers O(log T) segment-tree nodes. A run is a DFS that applies a node's updates on entry and undoes them on exit (LIFO), so at leaf `t` exactly the intervals containing `t` are applied. A `RollbackDSU` find is O(log n) (union by size, no compression) and an undo is O(1). With `k` intervals (edges, values, paired events), a run costs O(n + T + k·log T·log n) time and O(n + T + k·log T) memory.

`componentValue` folds `x` into the current root's value; unions combine values with `op` in an arbitrary order, which is why `M` must be commutative, and undo restores them. For bipartiteness, `dsu` is a 2n-vertex double cover: an edge `(u, v)` unites `u` with `v + n` and `u + n` with `v`. It closes an odd cycle exactly when `u` and `v` already share a set, because an even walk joins them. A counter of such edges, kept on the undo stack, is zero exactly when the graph is bipartite. The first edge, in application order, that makes the graph non-bipartite is counted, and every counted edge certifies an odd closed walk.

Both calls can be repeated, and intervals added between calls accumulate.

### IncrementalBridgeConnectivity

`IncrementalBridgeConnectivity(n)` covers vertices `[0, n)`. `addEdge(u, v)` inserts online; loops are ignored and a parallel copy of a bridge turns it into a non-bridge.

- `find(v)` returns a vertex that represents `v`'s 2-edge-connected component; it may change after merges.
- `root(v)` returns the representative of `v`'s connected component.
- `twoEdgeConnected(u, v)` and `connected(u, v)` compare those representatives.
- `bridgeCount()` is the current number of bridges.

Bridges form a forest over 2-edge-connected representatives (`par`), and `cc` is a DSU of those trees rooted at a representative. An edge between trees counts a new bridge, reroots the smaller tree at its endpoint (reversing its parent path) and hangs it below the other endpoint. An edge inside a tree walks both endpoints up alternately with visit stamps to the LCA, collapses the path into the LCA's component and uncounts one bridge per collapsed node. Rerooting the smaller tree costs O(n log n) in total, and finds use iterative path compression. Total time O((n + m)·log n), so updates and queries are O(log n) amortized; memory O(n). The queries are non-const because they compress paths.

### rangeEdgeComponentCount

`rangeEdgeComponentCount(g, queries)` takes an undirected `Graph` or `CsrGraph` (asserted) and queries `[l, r)` with `0 ≤ l ≤ r ≤ m` (asserted). For each query it returns the number of connected components of `(V, {edges l..r−1})`, counting all `n` vertices.

Let edge `i` join `a` and `b`, and let `b(i)` be the least `j > i` such that `a` and `b` are connected within edges `i+1..j` (`m` if never, `i` for a loop). The maximum-ID spanning forest `F_r` of edges `[0, r)` contains `i` exactly when `i < r ≤ b(i)`: greedy insertion by decreasing ID keeps `i` iff edges `i+1..r−1` do not connect its ends. The restriction of a greedy forest to IDs `≥ l` spans the edges with IDs in `[l, r)` (matroid restriction). So the answer is `n − #{i ∈ [l, r) : b(i) ≥ r}`, a dominance count that a Fenwick sweep over decreasing `r` answers in O((m + q)·log m).

`b(i)` is the bottleneck (the minimum over paths of the maximum edge ID) between `a` and `b` among edges `(i, m)`. A divide and conquer `solve(l, r, t)` receives `t`, a forest on the endpoints `K` of edges `[l, r)` whose bottlenecks among `K` equal those of edges `[r, m)`, with weights ascending. The left half receives `compress(edges[mid, r) ++ t, K_left)` and the right half `compress(t, K_right)`. `compress` runs Kruskal over the ascending list on a DSU whose components carry one terminal representative, and emits `(rep, rep', w)` whenever two terminal-carrying components merge. This preserves the partition of the terminals at every threshold, so it preserves their bottlenecks. Edges `[mid, r)` have IDs `< r ≤` every weight in `t`, so the concatenation is already ascending. At a leaf, `t` holds at most one edge between the two endpoints, and its weight is `b(i)`. Each level handles O(m) list entries (`|t| < 2(r − l)`), for O(n + m·log m·α(n) + q·log m) time and O(n + m + q) memory.

## Legacy mapping

| Legacy (`OLD/algorithms.cpp:4906-5003`, `OnlineBridges`) | Replacement |
|---|---|
| `addEdge(a, b)` | `IncrementalBridgeConnectivity::addEdge` |
| `find2ec(v)` (recursive) | `find` (iterative) |
| `findCC(v)` (recursive) | `root`; `connected` compares two roots |
| `bridges` field | `bridgeCount()` |
| `makeRoot`, `mergePath` (per-call path vectors) | internal to `addEdge`, with reused buffers |

## Aliases

- Library Checker `dynamic_graph_vertex_add_component_sum`: `insertEdge`/`eraseEdge` at the query index, `addValue(t, T, v, x)` and `componentValue` in `run` with `Sum<lng>`.
- Nyaan `run(add, del, query)`, ei1333 component count and cp-algorithms "deleting in O(log n)" are `run` callbacks (`count()`, `isSameSet`); hitonanode's Begins/Ends schedule is Data Structures 63.
- ei1333/hitonanode `find` and cp-algorithms `find_2ecc` are `find`; maspypy `range_edge_connected_component_query` is `rangeEdgeComponentCount` (link-cut forest there, bottleneck divide and conquer here).

## Feature-to-test map

`96-Local Testing/04-Graphs/26-dynamic_connectivity_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with probes, ASan/UBSan in full/stress).

| Operation | Test | Oracle |
|---|---|---|
| `OfflineDynamicConnectivity`: `addEdge`, `insertEdge`, `eraseEdge` (reversed orientation, multiplicities), `addValue`, `query`, `run` | 4,000 rounds n ≤ 6, T ≤ 10, with intervals including empty ones, loops, `Sum<lng>`, `Min<lng>` and `Sum<int>`; default monoid; insert-only and erase-only sequences, n = 50,000, T = 100,000 | Per queried time, a fresh union-find over the present edges: all-pairs `isSameSet`, `count`, `getSize`, `componentValue`, `edgeCount`; an incremental DSU with sums for the long sequences; answers once per time, increasing |
| `OfflineDynamicConnectivity::bipartiteness` | Same rounds | BFS 2-colouring of the present edges |
| `IncrementalBridgeConnectivity`: `addEdge`, `find`, `root`, `twoEdgeConnected`, `connected`, `bridgeCount` | 3,000 sequences n ≤ 8, up to 15 edges, loops and repeated pairs, all pairs after every edge; 100,000 vertices in four phases (near-path then random) | Edge-deletion bridges and labels without bridges; Tarjan `lowlink` (row 09) bridges and components, DSU connectivity |
| `rangeEdgeComponentCount` | 4,000 random multigraphs (n ≤ 7, m ≤ 14, loops, repeats) with every `[l, r)` in shuffled order; m = 250 with every range for random, sparse-vertex and cycle shapes; n = 100,000, m = 200,000 random, path and star with 125 sampled ranges | Union-find over the edges of each range |
| Preconditions | 8 probes: vertex out of range (`addEdge`, `addValue`), interval past `T`, erase without insert, query at `T`, incremental vertex out of range, query `l > r`, directed graph | Expected assertion failure |

Planted mutants (quick suite, optimized build), all killed: a wrong terminal representative after a `compress` union, an off-by-one leaf bottleneck, a prefix instead of a range Fenwick count, a path collapse that runs past the LCA, a missing odd-edge count, and dropped value events.

## Commands and results

2026-10-18, GCC 16.2.1 and GCC 14.4.1 (floor), CPython 3.14.8, Linux x86-64 (Intel Core i9-11900H), GNU++20, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds.

| Command | Result |
|---|---|
| `26-dynamic_connectivity_tester.py --mode quick` | PASS 2 configurations, 900 cases, 253,645 checks per configuration, 8 probes |
| `26-dynamic_connectivity_tester.py --mode full --seed 1` | PASS 3 configurations, 11,000 cases, 2,716,597 checks per configuration, 8 probes; MEMORY peak 979 MB |
| `CXX=g++-14 26-dynamic_connectivity_tester.py --mode full --seed 2` | PASS 3 configurations, 11,000 cases, 2,614,197 checks per configuration, 8 probes; MEMORY peak 970 MB |
| `26-dynamic_connectivity_tester.py --mode stress --seed 3` | PASS 3 configurations, 70,000 cases, 16,251,916 checks per configuration, 8 probes; MEMORY peak 1,569 MB |
| `02-integration.py --sanitizers` | PASS: 159 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 1,829 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/26-dynamic_connectivity_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/26-dynamic_connectivity_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/26-dynamic_connectivity_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/26-dynamic_connectivity_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: contest profile, no threshold. One design measurement chose the union-find inside `compress`. The `compress` calls of a level touch O(m) entries but are O(m) calls, so allocating a Data Structures 01 `DSU` per call and remapping vertex IDs dominates. The header instead restarts a union-find on vertex IDs by a per-call stamp (union by size, path halving).

The measurement was a one-off scratch driver run through `_00_memory_cap.py`, not kept. Conditions: Intel Core i9-11900H, GCC 16.2.1 `-O2`, `rangeEdgeComponentCount` wall time, median of 5 runs, machine not idle (the review agent was running). Inputs: n = 200,000 with m = 400,000 random edges, and a path of 200,000 vertices with 400,000 edges; both use the same q = 200,000 uniform random ranges (seed 1), and the answer hashes matched across variants.

| Variant | Random | Path |
|---|---|---|
| `DSU` per call, edges and forest concatenated | 938 ms | 268 ms |
| `DSU` per call, no concatenation | 910 ms | 255 ms |
| Stamped union-find (kept) | 652 ms | 161 ms |

## Sources

Fetched 2026-10-18; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-18-p047).

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, finding bridges online](https://cp-algorithms.com/graph/bridge-searching-online.html) | Forest of 2-edge-connected components, small-to-large rerooting, path collapse, O((n + m) log n) |
| [cp-algorithms, deleting from a data structure in O(T(n) log n)](https://cp-algorithms.com/data_structures/deleting_in_log_n.html) | Segment tree over time with rollback DSU |
| [Library Checker graph problems](https://github.com/yosupo06/library-checker-problems/tree/master/graph) | `dynamic_graph_vertex_add_component_sum` operations |
| [maspypy range_edge_connected_component_query](https://maspypy.github.io/library/graph/ds/range_edge_connected_component_query.hpp) | Max-ID forest characterisation with a Fenwick count (link-cut engine replaced) |
| [Nyaan](https://nyaannyaan.github.io/library/graph/offline-dynamic-connectivity.hpp), [ei1333](https://ei1333.github.io/library/graph/connected-components/incremental-bridge-connectivity.hpp), [hitonanode](https://hitonanode.github.io/cplib-cpp/graph/incremental_bridge_connectivity.hpp) | Catalog comparison (see Aliases) |

The bottleneck-forest compression is this package's derivation from Kruskal's invariant (terminal partitions agree at every threshold). It is validated only by the proof above and the tests.

## Limits and handoffs

- Online fully dynamic connectivity is row 63; incremental SCC is row 70; offline dynamic MST is row 71.
- `OfflineDynamicConnectivity` needs a commutative monoid; non-commutative or potential-valued components (XOR paths, parity with values) go through a weighted rollback DSU (Data Structures 29) in a custom `OfflineDeletion` run.
- Grid decremental connectivity and offline 2-edge-connectivity over time are omitted ([00-notes.md](00-notes.md#p047-omissions)).

## History

- 2026-10-18: verified (P047); pinned `OnlineBridges` excerpt replaced and removed from the row (recorded prune); `find` from the completeness sweep; LCT-free `rangeEdgeComponentCount` with a stamped union-find (measured 28% faster than a per-call `DSU`); independent review: monolith ledger entry completed, setup bound and `NDEBUG` vertex clamp corrected, CSR and empty-input tests added; 6 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
