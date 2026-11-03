# 12-all_pairs_shortest_path.hpp — evidence

Owned by package P042 / GR04 (all-pairs and advanced shortest paths, with `13-shortest_path_advanced.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Domain and sentinels

Every function takes `Graph` or `CsrGraph`, directed or undirected, with loops and parallel edges; an undirected edge is two opposite arcs, so a negative undirected edge is a negative 2-arc cycle. Weights span the full `lng` range. Matrices hold `lll` and use `PATH_INF = 2^120` (from `13-shortest_path_advanced.hpp`). Every finite entry of `AllPairsResult` and `minimaxFloydWarshall` has magnitude below n * 2^63 < 2^95, and every `fixedLengthShortestPath` entry stays below 2^100 by its asserted bound, so the sentinels never collide with a value. Results are n × n and need O(n^2) memory: 20 bytes per pair for `AllPairsResult`, 16 for the bare matrices.

### AllPairsResult

`dist[u][v]` is `PATH_INF` when v is unreachable from u, and `-PATH_INF` when some u→v walk passes through a negative cycle (the distance is unbounded below). Otherwise it is the exact minimum walk weight. The diagonal is 0 unless u lies on or reaches a negative cycle that returns to u. `parent_arc[u][v]` is the last arc of a shortest u→v path for finite off-diagonal pairs and −1 elsewhere. `finite(u, v)` excludes both sentinels. `path(g, u, v)` needs the graph that produced the result and returns an oriented `PathWitness` from u to v whose arc weights sum to `dist[u][v]`, with one vertex when u == v and `exists = false` for a pair that is not finite. It replaces the removed `apspPath` (user decision 2026-11-03).

Correctness of `path`: every engine fills `parent_arc` from strict improvements, so for finite pairs, following `parent_arc[u][·]` back from v gives a chain of tight arcs (`dist[u][from] + w = dist[u][to]`). Without negative cycles on u's finite part, such a chain cannot close a cycle: a cycle of tight arcs has weight 0, and a strict improvement never closes one. The loop is also capped at n steps.

### floydWarshall

O(n^3 + m). The initial matrix takes the lightest parallel arc and keeps 0 on the diagonal unless a loop is negative. The update `d[i][j] = max(d[i][k] + d[k][j], -PATH_INF)` copies `parent_arc[k][j]`. The clamp keeps the values bounded: with negative cycles, Floyd values can otherwise shrink exponentially. Every value is the weight of some walk, or the clamp. A walk lighter than −(n−1)·2^63 contains a negative cycle, so only pairs that pass through a negative cycle can be clamped. Finite pairs keep the classical invariant: d[i][j] is at most the lightest path whose intermediate vertices are below k. After the loop, d[k][k] < 0 exactly for vertices on a closed negative walk. A pair is set to `-PATH_INF` iff some such k is reachable from i and reaches j, which is equivalent to some negative cycle lying on an i→j walk. The marking costs O(n^2 · |K|) for the |K| negative-diagonal vertices.

### johnson

O(n·m + n·(n + m)·log(n + m + 1)); the same matrix as `floydWarshall` on every input, including negative cycles. Steps:

1. Run Tarjan SCC (on a directed copy with the same arc numbering when g is undirected).
2. Run Bellman–Ford over arcs inside components only, from zero labels. A change in pass n marks the component of the changed vertex: every negative cycle has a relaxable arc in every pass, and with arcs restricted to components the change stays inside the cycle's component. The marked vertices form the set `cyc`.
3. Run a second zero-label Bellman–Ford over the arcs that avoid `cyc` to get feasible potentials; that subgraph has no negative cycle.
4. For each source u, mark as negative every vertex reachable from a `cyc` vertex that u reaches (two BFS passes). Fill the remaining vertices from Dijkstra over reduced weights w + h[from] − h[to] ≥ 0 on the graph without `cyc`. A walk from u that avoids `cyc` is exactly a walk to a vertex that is not marked, so these distances are exact. A source in `cyc` has only negative or unreachable entries.

With every weight ≥ 0, both Bellman–Ford loops stop after one pass. `johnson` is then n binary-heap Dijkstras at O(n·(n + m)·log(n + m + 1)), which is why it replaces the removed `apspDijkstra`.

### minimaxFloydWarshall

O(n^3 / w + n^2 + m·log(m + 1)) with w = 64. It returns the n × n `lll` matrix of the least maximum arc weight over u→v walks; with `maximum = true`, the greatest minimum. An unreachable pair holds the worst sentinel (`PATH_INF`, or `-PATH_INF` with `maximum`). The diagonal holds the best sentinel (`-PATH_INF`, or `PATH_INF`), meaning the empty walk; loops never matter. Signed weights.

The algorithm takes arcs in weight order (stable for equal weights). Each arc adds its pair to a bitset reachability relation, then closes the relation with an explicit stack. A new pair (u, v) contributes u → `to[v]` and `from[u]` → v, and each pair is processed once, at O(n / w) per pair.

Correctness: after each arc, the relation is the transitive closure of the arcs taken so far. For pairs (a, b) and (b, c), whichever is processed later finds the other already set and adds (a, c). A pair first appears while the arc of weight w is processed, so its bottleneck is exactly w.

There is no witness. A walk of value x is any walk over arcs no heavier than x; per source it is `minimaxPath` / `widestPath` of `13-shortest_path_advanced.hpp`, whose results carry parents.

### fixedLengthShortestPath

O(n^3·log(k + 1) + m). It returns the least weight of a walk with exactly k arcs, or `PATH_INF`; k = 0 gives the min-plus identity. Signed weights. It asserts k ≥ 0 and k · max |w| < 2^100, so every total stays below 2^100. In the inner loop, a sum with a `PATH_INF` operand stays above `PATH_INF / 2` and is normalized after each row, which keeps the loop branch-free. No witness; the walk can have up to k arcs.

### eccentricities

O(n^2). It takes any square distance matrix with n ≥ 1 and no `-PATH_INF` entry, for example `johnson(g).dist`; one entry pass asserts both. Under `NDEBUG` a ragged matrix still gives a bounded, meaningless result. `ecc[u]` is the maximum entry of row u, the diagonal included: the out-eccentricity, and `PATH_INF` when some vertex is unreachable. `radius` and `diameter` are the minimum and maximum of `ecc`; `center` and `periphery` list the vertices that attain them, in ascending order. On a directed graph, apply it to the transposed matrix for in-eccentricities.

### Legacy mapping

Legacy matrices use `INF64` for an absent arc, so a 0 entry is a real zero-weight arc. `Graph::fromMatrix` would drop such entries; build the graph with `g.addEdge(u, v, d[u][v])` for every entry below `INF64` instead.

- `FloydWarshall(n, dis).runFloydWarshall()` (`OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`; vertices `[1, n]`, the 0 index unused): `floydWarshall(g)` on that graph over vertices `[0, n)`. `PATH_INF` / `-PATH_INF` replace `INF64` / `-INF64`.
- `floydWarshall(d)` (in place, 0-based; `OLD/algorithms.cpp:4702-4722` and `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp`, `test.cpp`): `floydWarshall(g).dist`. The legacy updates were unclamped and could overflow on negative cycles; the clamp removes that defect.
- `floyd_warshall()` (`OLD/Team Notebook/src/graph/old_floyd-warshall.cpp`; globals, no negative-cycle handling): `floydWarshall(g)`.
- `minPlusMatrixExp(a, k)` (`algs.cpp`, `algsbetter.cpp`; `INF64` entries absent): `fixedLengthShortestPath(g, k)`.
- `apspPath` (planned name): `AllPairsResult::path(g, u, v)`.
- `apspDijkstra` (removed 2026-11-03, user decision: a narrowing of `johnson` with identical `dist` and `parent_arc` and the same cost): `johnson(g)`. Its nonnegative test cases run through `johnson` on every graph.

## Feature-to-test map

Tester: [`12-all_pairs_shortest_path_tester.cpp`](../../96-Local%20Testing/04-Graphs/12-all_pairs_shortest_path_tester.cpp), driven by its `.py`. Every graph is checked as `Graph` and, on alternating cases, as `CsrGraph`.

| Operation | Oracle and cases |
|---|---|
| `floydWarshall`, `johnson` | Per-source `lll` Bellman–Ford with negative-cycle marking (`bellmanOracle`). It is self-checked on exhaustive and random tiny graphs against walk enumeration up to a length that exposes any negative cycle. Cases: n = 0 and 1, loops with every extreme weight, an undirected negative edge, full-range chains, negative-cycle reachability, zero cycles, parallel arcs, a Johnson reweighting fixture, exhaustive n = 2 and 3, random weights (small signed, nonnegative, full range, wide, potential-feasible negative), word-boundary n = 63–200 |
| `AllPairsResult`: `finite`, `path` | Every pair of every result: sentinel ↔ `finite`, `parent_arc` −1 exactly off finite or diagonal pairs, the path is simple and oriented, it ends with `parent_arc`, and its weight equals `dist`. Constructor, repeated, copied and moved results; probes `path-size`, `finite-range` |
| `minimaxFloydWarshall` (both directions) | Threshold reachability (`thresholdOracle`), self-checked against a bottleneck Floyd; word-boundary sizes 63/64/65/127/128/129 |
| `fixedLengthShortestPath` | Walk-length DP (`walkOracle`) for every k up to n + 2. Fixtures: k up to 10^12 on 3-cycles, k = INT64_MAX with zero weights and with no arcs, k at the 2^100 bound with INT64_MIN/MAX, k = 0 identity, k = ⌊2^100 / w⌋ with k·w = 2^100 − 2^36. Probes `fixed-negative-k`, `fixed-overflow` |
| `eccentricities` | Direct recomputation on every oracle matrix without −PATH_INF. Fixtures: path, n = 1, disconnected, directed, full range, negative entries, a negative diagonal. Probes `ecc-empty`, `ecc-negative-infinite`, `ecc-non-square` |

## Commands and results

2026-11-03, GCC 16.0.1 20260322 (`g++`) and GCC 14.3.0 (`g++-14`). Each run builds optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the seven assertion probes) and, in full and stress, ASan+UBSan, all with `-Wall -Wextra -Wconversion -Werror`.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/04-Graphs/12-all_pairs_shortest_path_tester.py' --mode quick` | PASS, 516 cases, 4.79·10^6 checks, peak 537 MB |
| `… --mode full` | PASS, 30302 cases, 1.17·10^8 checks, 3 configurations, peak 1038 MB |
| `CXX=g++-14 … --mode full` | PASS, the same counts, peak 1017 MB |
| `… --mode stress --seed 7` | PASS, 280958 cases, 3.01·10^8 checks, peak 984 MB, 237 s wall; the checked build alone takes about 130 s against the runner's 180 s limit, which is why stress runs two boundary rounds |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 186 standalone and aggregate headers, multi-TU scalar and AVX2, workspace; peak 2744 MB |

## Sources

See [the 2026-11-03 ledger](00-sources.md#pages-fetched-on-2026-11-03-p042-gr04): cp-algorithms (Floyd–Warshall negative-cycle marking, fixed-length paths), suisen (bitset minimax closure), NetworkX (Johnson, distance measures). No catalog code was copied.

## Limits and handoffs

- Every result is Θ(n^2) memory; n ≈ 5000 already needs 500 MB for `AllPairsResult`.
- The min-plus product should move to Core Matrix `mulSemiring` once that planned operation is verified (follow-up for the Core Matrix package).
- Bitset transitive closure and the minimum-weight cycle belong to rows 40 and 68.

## History

- 2026-11-03: P042 GR04 rewrite from the pinned legacy excerpt; full suite passed on g++ and g++-14, stress seed 7; review: apspDijkstra removed (user decision), 11 findings fixed.
