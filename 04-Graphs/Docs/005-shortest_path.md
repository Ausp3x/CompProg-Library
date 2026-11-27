# 005-shortest_path.hpp — Evidence

Owned by package P010 / GR02 (graph foundations, with `001-graph.hpp` through `004-dsu.hpp` and `006-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Scope

The row covers single- and multi-source shortest paths, negative cycles anywhere in the graph (`findNegativeCycle`), DAG shortest and longest paths (`dagLongestPath` over the whole DAG), nearest-center cells (`voronoiCells`), the weighted ego graph and the AOE critical path, over the integer weight domain below. All-pairs paths belong to `012-all_pairs_shortest_path.hpp`, advanced single-source variants (Dial, SPFA, path counting, widest paths) to `013-shortest_path_advanced.hpp`, k shortest walks and simple paths to `032-kshortestwalks.hpp` and `059-kshortestsimplepaths.hpp`, implicit graphs to `043-implicit_graph.hpp` and temporal paths to `055-temporal_graph.hpp`.

### Weight Domain

Every weighted algorithm uses exact signed-integer weights from the full `lng` range and `lll` accumulated distances. Status fields distinguish absent, finite and unbounded answers without reserving a numeric weight. Fractional and custom-weight instantiations of the archived notebook templates are outside this integer API.

### PathWitness and ShortestPathResult

`PathWitness.exists` distinguishes an absent path from a source's zero-edge path, whose `vertices` holds that one source. `ShortestPathResult(n, sources)` asserts in-range sources and marks them reachable at distance 0. `dist[v]` is meaningful only when `finite(v)`. `reachable` includes negative-infinite vertices, and `negative` marks exactly those. Parents of finite vertices lead to a source, whose parent is `-1`. `negative_cycle` is one reachable negative directed arc-cycle; an undirected negative edge can contribute both of its reciprocal arcs. `path(g, v)` returns an existence flag and the oriented vertex and arc sequence; it needs the unchanged graph that produced the result, which it uses only in an assert (`[[maybe_unused]]`). `relax(e, a, longest)` is the public relaxation step: it asserts in-range endpoints, a reachable `e.from` and `a >= 0`, and needs a representable candidate sum. The algorithms use the unchecked `shortest_path_detail::relax` after validating their input once at entry.

Sources start at distance zero, duplicate sources are ignored, and an empty source set reaches nothing. Signed DAG and Bellman–Ford paths may improve a source through another source. Among several shortest paths the parent kept is unspecified (it follows each algorithm's scan order), and so is the first vertex of `negative_cycle` (user decision 2026-11-15).

### bfsShortestPaths, zeroOneBfs, dijkstra and dijkstraDense

`bfsShortestPaths` ignores weights, counts arcs, and has the `bfs` tie order. `zeroOneBfs` requires every weight to be 0 or one common c > 0, inferred from the first nonzero arc (any positive `lng`); under `NDEBUG` any other weight returns the sources-only result at once. Its deque holds stale entries, so its workspace is O(n + m). `dijkstra` is a lazy binary heap and requires every arc weight nonnegative, including arcs outside the reached component; a negative weight fails the entry assert, and under `NDEBUG` returns the sources-only result at once rather than looping on a negative cycle. `dijkstraDense` is the O(n^2) array scan over a minimum `DenseGraph` view and requires every source-graph weight nonnegative, even for discarded parallel arcs; its cost excludes the O(n^2 + m) view construction. One source is the list `{s}`.

`dijkstra` runs `shortest_path_detail::dijkstra<false>` after its entry check, and `egoGraphWeighted` runs `dijkstra<true>`. The engine settles vertices in nondecreasing distance. Only the bounded form stops once the heap's minimum exceeds its limit, records the settled vertices and returns false at the first negative arc it scans, so `dijkstra` pays for none of that (counter A/B in Commands and Results).

Correctness and cost: with weights {0, c} every queued distance is d or d + c for the front distance d, exactly as {0, 1} scaled by c, so the deque processes nondecreasing distances, and stale entries avoid rescanning improved candidates. The dense view keeps the best parallel edge with its original arc ID. Sparse heap time is O(k + (n+m) log(n+m+1)) and workspace O(n+m), with m the arc count and k the source count. Dense time is O(n² + m + k) beyond view construction. BFS and 0-1 BFS are linear in n + m + k.

### DagPathResult, dagShortestPaths and dagLongestPathsFrom

Directed graphs with arbitrary signed weights. `dagLongestPathsFrom` maximizes where `dagShortestPaths` minimizes; both call `shortest_path_detail::dagPaths` with the order as a flag. A cyclic graph returns `acyclic = false`, a result that reaches nothing, and a directed cycle witness. Relaxation follows a topological order, and topological induction establishes the minimum or maximum over all source paths.

### DagLongestPathResult and dagLongestPath

`dagLongestPath(g)` is the maximum-weight path of a DAG over every start, a single vertex counting as a path of weight 0 (NetworkX `dag_longest_path`). It calls `dagLongestPathsFrom` seeded at every vertex and takes the first maximum distance, so the path ends at the smallest vertex reaching `length` (NetworkX breaks ties by its topological order instead); `path` is that vertex's parent chain, starting at a vertex no arc improved. A cyclic graph gives `acyclic = false`, a directed cycle witness, `length = 0` and an absent path; an empty graph gives `acyclic = true`, `length = 0` and an absent path.

### VoronoiResult and voronoiCells

`voronoiCells(g, centers)` requires every weight nonnegative (under `NDEBUG` another weight returns the centers-only result at once). `paths` holds the multi-source distances from the centers, and `center[v]` is the smallest center at minimum distance from v, or `-1` when no center reaches v; directed graphs give outward cells (distance from the center). Duplicate centers are ignored, and an empty list reaches nothing. A center at distance 0 from a smaller center joins that center's cell, which keeps every cell a subtree of the parent forest: `center[parent[v]] == center[v]` for every reached non-center vertex. NetworkX `voronoi_cells` keeps each center in its own cell; this library's rule is kept by user decision 2026-11-15.

Correctness: the heap orders (distance, center) pairs lexicographically. Adding a nonnegative weight keeps that order and never decreases a pair, so Dijkstra's argument applies to the pairs, and each vertex settles at its minimum distance, then the smallest center reaching it at that distance. A popped entry is skipped unless both fields match the vertex's current pair. O(k + (n + m) log(n + m + 1)) time, O(n + m) workspace.

### egoGraphWeighted

`egoGraphWeighted(g, center, radius)` returns `inducedSubgraph(g, vs)` for the vertices `vs` within weighted distance `radius` (inclusive) of `center` along out-arcs, in nondecreasing distance with `center` first (NetworkX `ego_graph` with `distance`). It asserts `radius >= 0` and an in-range center. Every weight must be nonnegative, but only the arcs it scans are checked, so the cost stays local: a negative arc leaving a vertex outside the ball goes unnoticed and can make the result differ from the true distances (0→1 of weight 5 and 1→2 of weight −1 with radius 4 return {0}, although vertex 2 is at distance 4). A negative scanned arc fails the assert, and under `NDEBUG` returns the subgraph on `center` alone. The bounded engine stops when the heap minimum exceeds the radius, so the cost is O(n + d * log(d + 1)) for the d arcs scanned from the ball, plus the O(n) result allocation, as for `egoGraph` in row 02.

### CriticalPathResult and criticalPath

`criticalPath(g)` treats a directed graph as an activity-on-arc (AOE) network with signed integer durations (OI Wiki 关键路径). Over one topological order, `earliest[v]` is the maximum weight of a path into v from an in-degree 0 vertex (0 at those vertices); `length` is the maximum `earliest` over out-degree 0 vertices (0 for an empty graph); `latest[v]` is `length` minus the maximum weight of a path from v to an out-degree 0 vertex (`length` at those vertices). Arc a from u to v is critical when its slack `latest[v] - w - earliest[u]` is 0; `critical` lists those arc IDs in increasing order. Several sources and sinks are allowed: every source starts at 0 and every sink must finish by `length`, the AOE rule with one virtual source and sink. It holds for negative durations too, where an inner vertex may have `earliest` above `length`. A cyclic graph gives `acyclic = false`, a directed cycle witness and empty vectors.

Correctness: forward relaxation in topological order computes the maxima into each vertex and backward minimization the latest times; `latest[v] >= earliest[v]` by induction from the sinks, so every slack is nonnegative. With nonnegative durations the critical arcs are exactly those on some source-to-sink path of weight `length`. O(n + m) time and memory.

### shortestPaths

`shortestPaths(g, sources)` scans the weights once and calls `bellmanFord` when any weight is negative, `zeroOneBfs` when every weight is 0 or one common c > 0, and `dijkstra` otherwise (hitonanode `shortest_path::solve` without its Dial and dense branches, [notes](00-notes.md#p010-gr02-omissions)). Its result is that call's, field for field, and its cost is that call's plus O(m).

### bellmanFord and findNegativeCycle

`bellmanFord(g, sources)` runs at most n in-place passes over all arcs. An improvement in pass n seeds negative-infinity propagation along every reachable arc. In-place distances sum at most n * m `lng` weights; with `int`-bounded n and m their magnitude is below 2^125, so `lll` is exact. `findNegativeCycle(g)` seeds every vertex and returns a negative directed arc-cycle anywhere in `g`, including outside any single source's reach, or an empty witness.

Correctness: an improvement in pass n is reachable from a negative cycle, and every vertex reachable from such a vertex is marked negative-infinite. Stepping n predecessors from the last changed vertex reaches a negative cycle, recovered with its original arc orientation; an undirected negative edge legitimately supplies both reciprocal arcs of one logical edge. Strict relaxations keep finite parent chains free of spurious zero-weight cycles.

### Legacy Mapping

- `Dijkstra(n).runDijkstra(sources, adj)` (vertices `[0, n]`) → `dijkstra(Graph::fromAdjacency(adj, true), sources)` and `dis[u] = reachable[u] ? dist[u] : INF64`. `adj` must hold exactly `n + 1` lists, because lists past `n + 1` become vertices: resize it first. `is_proc` is `reachable`; every call returns a fresh result, so nothing needs a reset; distances have no `INF64` bound. `runGraph(sources, g)` → `dijkstra(g, sources)`.
- `dagLongestPaths(g, sources)` and `dagShortestPaths(g, sources, true)` (flag removed by GR02, user decision 2026-11-14) → `dagLongestPathsFrom(g, sources)`; `dagLongestPaths(g, s)` → `dagLongestPathsFrom(g, {s})`.
- `bellman_ford(src)` and `has_negative_cycle()` (`OLD/Team Notebook/src/graph/old_bellman-ford.cpp`) → `bellmanFord(g, {src})` and its `negative_cycle` witness.
- The single-source overloads `bfsShortestPaths(g, s)`, `zeroOneBfs(g, s)`, `dijkstra(g, s)`, `dijkstraDense(g, s)`, `dagShortestPaths(g, s)` and `bellmanFord(g, s)` → the same call with `{s}`.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/005-shortest_path_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Full runs 4,096 signed three-vertex digraphs × all eight source subsets, 1,000 random multigraphs, 1,000 signed DAGs and 1,000 random {0, c} multigraphs. Quick uses two-vertex exhaustive graphs, 100 of each random family and 2,000-vertex chains; stress adds 19,683 looped signed digraphs, 5,000 of each random family and 300,000-vertex chains. Every check runs on `Graph` for every graph, and on `CsrGraph` (except `dijkstraDense`, which has no CSR form) for every random and fixture graph and for the exhaustive graphs whose code is a multiple of 97 (101 in stress), about 1 in 100.

| Operation | Test | Oracle |
|---|---|---|
| `PathWitness`, `ShortestPathResult: finite, path` | Oriented path witnesses on every case | Exact Floyd reachability and distances |
| `ShortestPathResult: relax` | Minimum by default, then maximum, an equal value refused in maximum mode, an arc into vertex 0 with a nonzero ID; its six probes | Expected values and assertion failures |
| `bfsShortestPaths`, `zeroOneBfs`, `dijkstra`, `dijkstraDense` | Every source subset; 100,000-vertex chains for `zeroOneBfs` and `dijkstra` (`bfsShortestPaths` and `dijkstraDense` run on the small and random graphs only) | Exact Floyd distances |
| `zeroOneBfs` ({0, c} weights) | Random directed and undirected {0, c} multigraphs, c in {1, 7, 10^18, 4·10^18, `LLONG_MAX`}; a {0, `LLONG_MAX`} chain past 2^63; two deque-order adversaries with K = 100,000 (2,000 in quick), on which a label-correcting order runs Θ(K^2) | Exact Floyd distances, path witnesses |
| `DagPathResult`, `dagShortestPaths`, `dagLongestPathsFrom` | 1,000 signed DAGs; cyclic inputs; a 100,000-vertex `lng` maximum chain | Floyd distances; independent simple-path maxima |
| `DagLongestPathResult`, `dagLongestPath` | Every DAG case, cyclic inputs, n = 0, a long chain | Enumerated path maxima from every start; smallest end vertex; witness weight |
| `bellmanFord` (negative reachability marks, exact `lll` totals) | Signed digraphs, full-range sums | Floyd reachability and distances, negative-cycle closure |
| `findNegativeCycle` | Oriented negative-cycle witnesses | Negative-cycle closure |
| `VoronoiResult`, `voronoiCells` | Every source list (empty, duplicates) and all vertices as centers; ties, a center 0 away from a smaller one; a long chain; two stale-label adversaries with k = 200,000 (2,000 in quick and in the checked build, whose debug heap checks every push) | Floyd multi-source distances; smallest nearest center; tight in-cell tree arcs; witnesses start at the cell's center |
| `egoGraphWeighted` | Every center with radius 0, each distance d and d − 1, 2^100 | Floyd membership; center first, nondecreasing distance; `inducedSubgraph` and brute-force induced edges |
| `CriticalPathResult`, `criticalPath` | Every directed case; a hand-checked textbook network, n = 0, isolated vertices, parallel arcs, negative durations, an n-arc chain | Path enumeration from in-degree 0 and to out-degree 0 vertices; cycle witnesses |
| `shortestPaths` (dispatch) | Signed, {0, c} and nonnegative graphs | Floyd with negative marks; field-for-field equality with the engine the weights select |
| `dijkstra` on `Graph::fromAdjacency` lists (the removed `Dijkstra` cases) | Weighted and unweighted lists, repeated and empty source lists, one vertex, distances beyond `INF64` | Floyd distances; expected values |
| Preconditions | 27 assertion probes, including `source-past`, `zero-one-smaller` ({3, 2}), `finite-past`, `relax-to-past`, `path-small-graph`, `path-large-graph`, `relax-from-end`, `relax-from-past`, `zero-one` ({2, 3}), `zero-one-negative`, `voronoi-negative`, `voronoi-center`, `ego-negative-radius`, `ego-center`, `ego-negative`, `critical-undirected`, `dag-longest-undirected` | Expected assertion failure |
| Default-constructed `DagPathResult`, `DagLongestPathResult`, `CriticalPathResult` | One fixture | `acyclic` false, `length` 0 |

## Mutation Testing

Record: mewt 4.0.0 on blob `4a1947e8a46de660b7fd59667c38aeaaf86bad30` (2026-11-15, seed 1, whole file, uniform sample of 799): 500 mutants, 468 caught (11 by timeout), 32 survived, 0 untested. Stage seconds of the retest: quick 1,642, full on the optimized build 381, full on the checked and ASan-UBSan builds 2,682.

/finish GR02 ran it whole-file (`--whole`) because the earlier record's workdir was gone. The 32 survivors are equivalent. A scratch differential checked the order-sensitive ones: a driver printing every output field (`dist`, `reachable`, `parent`, `parent_arc`, `negative`, the cycle witness, `center`) for 200,000 random digraphs (n 1–6, 0–12 arcs, one or two sources, weights in {0, 2}, [0, 4] or [−2, 3] by family, `std::mt19937_64` seed 12345), built with `-O2 -DNDEBUG` against the header and each mutant copy, on an earlier blob whose code at these lines is unchanged. The tie-parent and cycle-rotation mutants are equivalent by user decision 2026-11-15, because the contract leaves tie parents and the cycle's first vertex unspecified.

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 509`, `COS 663`, `COS 673`, `COS 684`, `COS 738`, `COS 744`, `!= -1` → `> -1`, `== -1` → `<= -1` | 36, 157, 158, 162, 257, 258 | Arc IDs, `u`, `a` and `last` are `-1` or nonnegative. |
| `MR 790`, `MR 791`, `MR 793`, `MR 795`, a copy for `std::move` | 60, 180, 225 | Same values; one O(n) copy within each bound. |
| `COS 560`, `c > 0` → `c >= 0` | 70 | `c` is 0 only while every arc so far weighs 0, and then `e.w == c` repeats `e.w == 0`. |
| `COS 562`, `COS 652`, `COS 657`, `COS 692`, `<` → `!=` | 73, 154, 156, 233 | The counter steps by 1 from 0 to a nonnegative bound. |
| `IF 239`, `COS 567`, `COS 569`, the `zeroOneBfs` stale check removed, `d < dist[u]` (never true), `d > dist[u]` | 76 | The deque stays nondecreasing, so a vertex's current entry is popped before its stale one, which then relaxes nothing; a vertex is pushed at most twice (d + c, then d), so the extra scans stay O(m). No output differed in the differential. |
| `COS 573`, `e.w == 0` → `e.w <= 0` | 80 | Weights are nonnegative past the entry check. |
| `IT 283`, `IT 284`, `if constexpr (bounded)` → `(true)` | 94, 97 | `dijkstra` would fill an `order` it never reads (O(n) within its bound) and test arcs its entry check already proved nonnegative. |
| `COS 615`, `cell[u] == u` → `>=` | 128 | `cell[u]` is u or `-1` there. |
| `COS 619`, `COS 624`, `!=` → `>` | 131 | Labels only decrease, so `d >= dist[u]`, and a stale entry at equal distance has a larger center. |
| `COS 637`, `center < g.n` → `!=` | 143 | A center above n still aborts, in `ShortestPathResult(g.n, {center})` on the next line. |
| `COS 653`, `i < g.n` → `<=` | 154 | Each round uses one vertex, so the extra round finds `u == -1` and stops. |
| `COS 668`, `dist[v] < dist[u]` → `<=` | 157 | Equal distances settle the last vertex first: the same distances, another tight parent (681 of 66,667 differential graphs). Equivalent by the user decision. |
| `COS 708`, `<` → `<=` | 239 | Assigns an equal value. |
| `COS 720`, `==` → `>=` in the critical test | 242 | Slack is nonnegative, `latest[to] >= earliest[from] + w`. |
| `COS 735`, `i == g.n - 1` → `>=` | 255 | `i < g.n`. |
| `BL 406`, the last pass leaves `negative[e.to]` false | 256 | A vertex relaxed in pass n is reached from a negative cycle (otherwise its distance is final after n − 1 passes), and every reachable negative cycle relaxes one of its vertices in every pass (else summing its arcs' inequalities gives w(C) >= 0), so the closure from the last pass's vertices marks it again. No output differed in the differential. |
| `COS 748`, n + 1 parent steps instead of n | 260 | Still on the cycle; the witness starts one vertex later (8,517 of 66,667 differential graphs). Equivalent by the user decision. |

## Commands and Results

2026-11-15 (P010 GR02), Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`.

| Run | Result |
|---|---|
| Full, seed 20261115, g++ | PASS, all three builds, 35,823 cases; 14,549,836 checks on the optimized and ASan-UBSan builds, 13,559,836 on the checked build (smaller Voronoi adversaries); 27 probes; optimized 1.6 s, checked 9.9 s, ASan-UBSan 31.7 s beside a mutation campaign; peak 1,225 MB |
| Full, seed 20261115, `CXX=g++-14` | PASS, all three builds, same counts; optimized 1.8 s, checked 10.1 s, ASan-UBSan 23.9 s; peak 1,221 MB |
| Stress, seed 7 | PASS, all three builds, 67,506 cases; 38,093,330 checks (37,103,330 checked); optimized 3.6 s, checked 21.8 s, ASan-UBSan 85.6 s beside a mutation campaign; peak 1,191 MB |
| `05-extraction.py ops --filter 005-shortest_path.hpp`; `testers --filter 005-shortest_path_tester` | PASS on g++ and g++-14 (21 operations in 21 extractions); PASS |
| `02-integration.py` | PASS, 190 standalone and aggregate headers, multi-TU, workspace |
| `dijkstra` counter A/B against the master header (`perf stat -e cycles:u,instructions:u -r 5`, `-O2 -DNDEBUG -falign-functions=64 -falign-loops=32`; 200,000 vertices, 10^6 random arcs, five sources, graph construction included) | Instructions 1.515 G before GR02, 1.549 G with the shared engine, 1.540 G with its bounded bookkeeping compiled out (adopted); cycles in three alternating rounds 4.78 / 4.62, 4.36 / 4.54, 4.20 / 4.25 G (master / adopted), within noise |

```bash
python3 '96-Local Testing/04-Graphs/005-shortest_path_tester.py' --mode full --seed 20261115
CXX=g++-14 python3 '96-Local Testing/04-Graphs/005-shortest_path_tester.py' --mode full --seed 20261115
python3 '96-Local Testing/04-Graphs/005-shortest_path_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter 005-shortest_path.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 005-shortest_path_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/005-shortest_path.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [driver](<../../96-Local Testing/04-Graphs/00-path_mst_benchmark.py>) compares sparse and dense Dijkstra (and Prim, recorded in `006-mst.md`) on chains, random sparse multigraphs, complete graphs, an adversarial complete graph and disconnected blocks of at most eight vertices, at sizes 24, 200 and 800. Weights are uniform in [0,10^6] with seed 20260927, except in the adversarial graph: chain arcs u to u + 1 weigh 1 and every other arc u-v (u < v) weighs 2n - 2u, so vertices settle in index order and each improves every later one (Θ(n^2) heap pushes for the sparse form). Five samples follow checked warmups, with repeated runs for smaller cases; all distances are compared. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately. Conditions, from the git-ignored `00-path_mst_benchmark.json` log that also holds memory accounting and every sample: 11th Gen Intel Core i9-11900H, GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`; run 2026-11-15 beside a mutation campaign, so absolute times are inflated while the ratios stand.

| 800-vertex workload | Dijkstra sparse/dense median ms | Dense setup ms |
|---|---|---|
| Chain | 0.047 / 1.480 | 0.499 |
| Random sparse | 0.429 / 3.400 | 0.626 |
| Complete | 23.539 / 23.898 | 9.896 |
| Adversarial complete | 107.936 / 21.554 | 9.553 |
| Disconnected blocks | 0.007 / 0.020 | 0.385 |

The adversarial row is the case the dense form exists for (`01-principles.md`, Priority: faster below a measured crossover). Dense setup can outweigh its traversal saving for a single call, so the choice stays explicit: no automatic density threshold, general superiority or timing gate is claimed.

```bash
python3 '96-Local Testing/04-Graphs/00-path_mst_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [cp-algorithms, Bellman–Ford](https://cp-algorithms.com/graph/bellman_ford.html) | The nth-pass negative-cycle criterion, predecessor recovery, downstream unbounded vertices and all-zero initialization. |
| [KACTL, BellmanFord.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/BellmanFord.h) (Simon Lindholm, 2015-02-23; CC0) | Independent comparison of finite, unreachable and unbounded contracts and downstream propagation. Its specialized pass ordering is not adopted. |
| [Princeton Algorithms, §4.4 Shortest Paths](https://algs4.cs.princeton.edu/44sp/) | Nonnegative Dijkstra and signed DAG shortest and longest topological relaxation. |
| [cp-algorithms, 0-1 BFS](https://cp-algorithms.com/graph/01_bfs.html) | Deque ordering and zero/one front/back insertion. |

Legacy: the original `Dijkstra` in `OLD/algorithms.cpp:4488–4526` and its duplicate in `OLD/[1] algorithms.cpp` are preserved. Its n+1 indexing, multi-source runs and reset behaviour map to `dijkstra` on `Graph::fromAdjacency` input (Legacy Mapping); the graph APIs are zero-based. The notebook's global negative-cycle detector is covered by `findNegativeCycle`.

The GR02 sweep (2026-11-14) sources are in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr02); omitted candidates and the copied-code reasons are in [00-notes.md](00-notes.md#p010-gr02-omissions).

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): 0-1 BFS, 0-c BFS, single-source shortest paths, DAG shortest path, DAG longest path, dag_longest_path, graph Voronoi diagram, voronoi_cells, nearest facility, weighted ego graph, ego_graph distance, critical path method, CPM, AOE network, activity on edge, shortest_path solve; DAG 上的 DP, 最短路, 有向无环图, 单源最短路, 负环, 迪杰斯特拉, 贝尔曼福特, 双端队列广搜, 最长路, 最短路径, 戴克斯特拉, 关键路径, AOE 网, 关键活动, 最早开始时间, 最迟开始时间.

## History

- 2026-09-27: original P010, full suite (seed 20260927) and benchmark passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 4 findings fixed (brace rule, complexity line placement, unchecked inner-loop `relax`, overload complexity lines) and `[[maybe_unused]]` on `path`; full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, removed the legacy `Dijkstra` adapter, `dagLongestPaths` and six single-source overloads (cases moved to `dijkstra` on `Graph::fromAdjacency` input, `dagShortestPaths(g, sources, true)` and `{s}`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-03: P042 GR04 cross-package fix (ratified by the maintainer at `/merge-pr 4`, 2026-11-08): `dijkstra` and `zeroOneBfs` return the sources-only result under `NDEBUG` on an out-of-domain weight instead of looping; 05 suite rerun passed.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the search aliases; no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR02, `zeroOneBfs` widened to {0, c} weights; added `voronoiCells`, `egoGraphWeighted`, `criticalPath`, `dagLongestPath` and the `shortestPaths` dispatcher (user decision 2026-11-14); `dagShortestPaths(g, s, true)` became `dagLongestPathsFrom` (flag ruling); `dijkstra` shares a bounded engine with `egoGraphWeighted`; full suite passed on g++ and g++-14, stress passed, extraction passed; whole-file mutation campaign (sample of 500 of 791, 456 caught after a retest).
- 2026-11-15: P010 GR02 review fixes: `criticalPath` takes `length` over out-degree 0 vertices (the virtual-sink rule, which differs from the maximum `earliest` once a duration is negative); `DagPathResult` and `DagLongestPathResult` fields initialised; the out-of-domain negative-arc `egoGraphWeighted` fixture removed; CSR sampling stated.
- 2026-11-15: /finish GR02: `dijkstra` runs the engine's unbounded form, compiling out the radius bookkeeping (counter A/B); `findNegativeCycle` runs on every graph; two 0-1 BFS deque-order and two Voronoi stale-label adversaries, the `finite-past`, `relax-to-past` and `zero-one-smaller` probes and an equal-value `relax` check closed the order-changing and four other survivors; a new whole-file campaign; the benchmark gained an adversarial complete graph; user decisions recorded (tie parents and cycle start unspecified, the `relax` flag kept, the smallest-center Voronoi rule).
- 2026-11-27: P010 limits recheck (experiment none): the ownership boundaries moved from Limits and Handoffs to Contracts (Scope), leaving no open limit; no code or test change.
