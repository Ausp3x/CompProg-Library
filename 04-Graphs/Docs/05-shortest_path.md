# 05-shortest_path.hpp — evidence

Owned by package P010 / GR02 (graph foundations, with `01-graph.hpp` through `04-dsu.hpp` and `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Weight domain

All weighted algorithms use exact signed-integer weights from the full `lng` range and `lll` accumulated distances. Status fields distinguish absent, finite and unbounded answers without reserving a numeric weight. Fractional/custom-weight instantiations of archived notebook templates remain outside this specified integer API.

### PathWitness and ShortestPathResult

`PathWitness.exists` distinguishes an absent path from a source's zero-edge path, whose `vertices` holds that one source. `ShortestPathResult(n, sources)` asserts in-range sources and marks them reachable at distance 0. `dist[v]` is meaningful only when `finite(v)`. `reachable` includes negative-infinite vertices, and `negative` marks exactly those. Parents of finite vertices lead to a source, whose parent is `-1`. `negative_cycle` is one reachable negative directed arc-cycle; an undirected negative edge can contribute both of its reciprocal arcs. `path(g, v)` needs the graph that produced the result. `relax(e, a, longest)` is the public relaxation step: it asserts in-range endpoints, a reachable `e.from` and `a >= 0`, and needs a representable candidate sum. The algorithms use the unchecked `shortest_path_detail::relax` after validating their input once at entry.

Correctness: `ShortestPathResult` stores `dist`, `reachable`, `negative`, `parent`, `parent_arc` and one reachable `negative_cycle`. Reachable negative-infinite vertices have both flags set. `path(g,v)` returns an explicit existence flag and oriented vertex/arc sequence; a valid zero-edge source path has one vertex. The input graph's arc IDs/content must remain unchanged. Sources are zero-initialized, duplicate sources are ignored, and an empty source set reaches nothing. Signed DAG/Bellman–Ford paths may improve a source through another source. `path`'s graph parameter is used only in an assert and is marked `[[maybe_unused]]`.

### bfsShortestPaths, zeroOneBfs, dijkstra and dijkstraDense

`bfsShortestPaths` ignores weights, counts arcs, and has the `bfs` tie order. `zeroOneBfs` requires every weight in {0, 1}; its deque holds stale entries, so workspace is O(n + m). `dijkstra` is a lazy binary heap and requires every arc weight nonnegative, including arcs outside the reached component. `dijkstraDense` is the O(n^2) array scan over a minimum `DenseGraph` view and requires every source-graph weight nonnegative, even for discarded parallel arcs; it excludes the O(n^2 + m) view construction. One source is the list `{s}`.

Correctness and cost: `zeroOneBfs` deque ordering processes nondecreasing distances and stale entries avoid rescanning improved candidates. Dense construction selects the best parallel edge while retaining its original arc ID. For general multigraphs, sparse heap time is O(k+(n+m) log(n+m+1)) and workspace O(n+m), where m is the arc count and k is the source count. Dense time is O(n²+m+k) beyond dense-view construction. BFS/0-1 BFS are linear in n+m+k.

### DagPathResult and dagShortestPaths

Directed graphs with arbitrary signed weights. `longest = true` maximizes instead of minimizing. A cyclic graph returns `acyclic = false`, a result that reaches nothing, and a directed cycle witness. Signed paths may improve a source through another source.

Correctness: relaxation follows topological order; a cyclic graph is a valid failure result with an oriented witness and no reachable path result. Topological induction establishes the minimum/maximum over all source paths.

### bellmanFord and findNegativeCycle

`bellmanFord(g, sources)` runs at most n in-place passes over all arcs. An improvement in pass n seeds negative-infinity propagation along every reachable arc. In-place distances sum at most n * m `lng` weights; with `int`-bounded n and m their magnitude is below 2^125, so `lll` is exact. `findNegativeCycle(g)` seeds every vertex and returns a negative directed arc-cycle anywhere in `g`, or an empty witness.

Correctness: an improvement in pass n is reachable from a negative cycle. The algorithm marks all such destinations and every vertex reachable from them as negative-infinite. Stepping n predecessors from the final changed vertex reaches a negative cycle, which is recovered with original arc orientation. This witness is a directed arc-cycle; an undirected negative edge legitimately supplies both reciprocal arcs of the same logical edge. `findNegativeCycle` seeds every vertex, so it also detects cycles outside any single source's reachable component.

Every Bellman–Ford intermediate represents a walk with at most n·m edge additions. Since n,m≤INT_MAX and |w|≤2^63, its magnitude is less than 2^125; exact `lll` arithmetic needs no saturating sentinels. Finite optimal paths and DAG paths have smaller bounds. Strict relaxations preserve finite parent chains and do not form spurious zero-weight parent cycles.

### Legacy mapping

- `Dijkstra(n).runDijkstra(sources, adj)` (vertices `[0, n]`) → `dijkstra(Graph::fromAdjacency(adj, true), sources)` and `dis[u] = reachable[u] ? dist[u] : INF64`. `adj` must hold exactly `n + 1` lists, because lists past `n + 1` become vertices: resize it first. `is_proc` is `reachable`; every call returns a fresh result, so nothing needs a reset; distances have no `INF64` bound. `runGraph(sources, g)` → `dijkstra(g, sources)`.
- `dagLongestPaths(g, sources)` → `dagShortestPaths(g, sources, true)`; `dagLongestPaths(g, s)` → `dagShortestPaths(g, {s}, true)`.
- The single-source overloads `bfsShortestPaths(g, s)`, `zeroOneBfs(g, s)`, `dijkstra(g, s)`, `dijkstraDense(g, s)`, `dagShortestPaths(g, s, longest)` and `bellmanFord(g, s)` → the same call with `{s}`.

## Feature-to-test map

`96-Local Testing/04-Graphs/05-shortest_path_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. The full corpus is 4,096 signed three-vertex digraphs × all eight source subsets, 1,000 random multigraphs and 1,000 signed DAGs. Quick uses two-vertex exhaustive graphs, 100 multigraphs/100 DAGs and 2,000-vertex chains; stress adds 19,683 looped signed digraphs, 5,000 multigraphs/5,000 DAGs and 300,000-vertex chains. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `PathWitness`, `ShortestPathResult: finite, path` | Oriented path witnesses on every case | Exact Floyd reachability/distances |
| `ShortestPathResult: relax` | Helper tests; its three probes | Expected values and assertion failures |
| `bfsShortestPaths`, `zeroOneBfs`, `dijkstra`, `dijkstraDense` | Every source subset, Graph and CSR; 100,000-vertex chains | Exact Floyd distances |
| `DagPathResult`, `dagShortestPaths` (shortest and `longest`) | 1,000 signed DAGs; cyclic inputs; a 100,000-vertex `lng` maximum chain | Floyd distances; independent simple-path maxima |
| `bellmanFord` (negative reachability marks, exact `lll` totals) | Signed digraphs, full-range sums | Floyd reachability/distances and negative-cycle closure |
| `findNegativeCycle` | Oriented negative-cycle witnesses | Negative-cycle closure |
| `dijkstra` on `Graph::fromAdjacency` lists (the removed `Dijkstra` cases) | Weighted and unweighted lists, repeated and empty source lists, one vertex, distances beyond `INF64` | Floyd distances; expected values |
| Preconditions | 11 assertion probes, including `dense-maximum` | Expected assertion failure |

Surviving mutants (an n−1-step Bellman–Ford walk, zero-one BFS `push_front` on weight 1, longest-path tie parent) are equivalent or change only runtime.

## Commands and results

2026-10-12, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261012, g++ | PASS, all three builds, 34,808 cases and 4,707,279 checks per build, 11 probes, peak 615 MB |
| Full, seed 20261012, `CXX=g++-14` | PASS, all three builds, same counts, peak 617 MB |
| Stress, seed 7 | PASS, all three builds, 62,491 cases and 9,698,614 checks per build, peak 749 MB |
| `00-path_mst_benchmark.cpp` after the call-site change | Compiles under `-Werror`; one capped run (seed 20260927) passed its internal result checks; the table below was not re-measured |
| Quick runner, all sixteen graph entries, from `/tmp` | PASS |
| `02-integration.py` (136 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 647 MB |
| `03-consistency.py`, with `--braces` on the touched headers, testers and benchmark | No errors |

```bash
python3 '96-Local Testing/04-Graphs/05-shortest_path_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/04-Graphs/05-shortest_path_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/04-Graphs/05-shortest_path_tester.py' --mode stress --seed 7
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [driver](<../../96-Local Testing/04-Graphs/00-path_mst_benchmark.py>) and `96-Local Testing/04-Graphs/00-path_mst_benchmark.json` compare sparse/dense Dijkstra (and Prim, recorded in `06-mst.md`) on chains, random sparse multigraphs, complete graphs and disconnected blocks of at most eight vertices. Sizes are 24, 200 and 800; weights are uniformly drawn from [0,10^6], with seed 20260927. Five samples follow checked warmups, with repeated runs for smaller cases. All distances are compared. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately.

| 800-vertex workload | Dijkstra sparse/dense median ms | Dense setup ms |
|---|---|---|
| Chain | 0.013 / 0.868 | 0.216 |
| Random sparse | 0.233 / 1.944 | 0.319 |
| Complete | 8.699 / 9.759 | 4.537 |
| Disconnected blocks | 0.002 / 0.009 | 0.174 |

These shared-host observations support keeping workload-specific choices explicit. Dense setup can outweigh its traversal saving for a single call. No automatic density threshold, universal superiority or timing gate is claimed. The record includes compiler/CPU/flags, memory accounting and every sample.

```bash
python3 '96-Local Testing/04-Graphs/00-path_mst_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [cp-algorithms, Bellman–Ford](https://cp-algorithms.com/graph/bellman_ford.html) | The nth-pass negative-cycle criterion, predecessor recovery, downstream unbounded vertices and all-zero initialization. |
| [KACTL, BellmanFord.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/BellmanFord.h) (Simon Lindholm, 2015-02-23; CC0) | Independent comparison of finite/unreachable/unbounded contracts and downstream propagation. Its specialized pass ordering is not adopted. |
| [Princeton Algorithms, §4.4 Shortest Paths](https://algs4.cs.princeton.edu/44sp/) | Nonnegative Dijkstra and signed DAG shortest/longest topological relaxation. |
| [cp-algorithms, 0-1 BFS](https://cp-algorithms.com/graph/01_bfs.html) | Deque ordering and zero/one front/back insertion. |

Legacy: the original `Dijkstra` in `OLD/algorithms.cpp:4488–4526` and its duplicate in `OLD/[1] algorithms.cpp` are preserved. Its n+1 indexing, multisource runs and reset behavior map to `dijkstra` on `Graph::fromAdjacency` input (Legacy mapping); the graph APIs are zero-based. The notebook's global negative-cycle detector is covered by all-vertex Bellman–Ford initialization (`findNegativeCycle`). The old Team Notebook `old_bellman-ford.cpp` is superseded by this header.

## Limits and handoffs

Fractional/custom-weight domains are not claimed. All-pairs, advanced, implicit, temporal and ranked shortest paths belong to their separate rows (`12-all_pairs_shortest_path.hpp`, `13-shortest_path_advanced.hpp` and others). GCC 14.2 itself (the judge floor) was not run; 14.4.1 is the closest available.

## History

- 2026-09-27: original P010, full suite (seed 20260927) and benchmark passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 4 findings fixed (brace rule, complexity line placement, unchecked inner-loop `relax`, overload complexity lines) and `[[maybe_unused]]` on `path`; full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, removed the legacy `Dijkstra` adapter, `dagLongestPaths` and six single-source overloads (cases moved to `dijkstra` on `Graph::fromAdjacency` input, `dagShortestPaths(g, sources, true)` and `{s}`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
