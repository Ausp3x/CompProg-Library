# 12-all_pairs_shortest_path.hpp — Evidence

Owned by package P042 / GR04 (all-pairs and advanced shortest paths, with `13-shortest_path_advanced.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Domain and Sentinels

Every function takes `Graph` or `CsrGraph`, directed or undirected, with loops and parallel edges; an undirected edge is two opposite arcs, so a negative undirected edge is a negative 2-arc cycle. Weights span the full `lng` range. Matrices hold `lll` and use `PATH_INF = 2^120` (from `13-shortest_path_advanced.hpp`). Every finite entry of `AllPairsResult` and `minimaxFloydWarshall` has magnitude below n * 2^63 < 2^95, and every `fixedLengthShortestPath` entry stays below 2^100 by its asserted bound, so the sentinels never collide with a value. Results are n × n and need O(n^2) memory: 20 bytes per pair for `AllPairsResult`, 16 for the bare matrices.

### AllPairsResult

`dist[u][v]` is `PATH_INF` when v is unreachable from u, and `-PATH_INF` when some u→v walk passes through a negative cycle (the distance is unbounded below). Otherwise it is the exact minimum walk weight. The diagonal is 0 unless u lies on or reaches a negative cycle that returns to u. `parent_arc[u][v]` is the last arc of a shortest u→v path for finite off-diagonal pairs and −1 elsewhere. `finite(u, v)` excludes both sentinels. `path(g, u, v)` needs the graph that produced the result and returns an oriented `PathWitness` from u to v whose arc weights sum to `dist[u][v]`, with one vertex when u == v and `exists = false` for a pair that is not finite. It replaces the removed `apspPath` (user decision 2026-11-09).

Correctness of `path`: every engine fills `parent_arc` from strict improvements, so for a finite pair, following `parent_arc[u][·]` back from v gives a chain of tight arcs (`dist[u][from] + w = dist[u][to]`). Without negative cycles on u's finite part, such a chain cannot close a cycle: a cycle of tight arcs has weight 0, and a strict improvement never closes one. The loop is also capped at n steps.

### floydWarshall

O(n^3 + m). The initial matrix takes the lightest parallel arc and keeps 0 on the diagonal unless a loop is negative. The update `d[i][j] = max(d[i][k] + d[k][j], -PATH_INF)` copies `parent_arc[k][j]`. The clamp keeps values bounded, since with negative cycles Floyd values can otherwise shrink exponentially. Every value is the weight of some walk, or the clamp. A walk lighter than −(n−1)·2^63 contains a negative cycle, so only pairs that pass through a negative cycle can be clamped. Finite pairs keep the classical invariant: d[i][j] is at most the lightest path whose intermediate vertices are below k. After the loop, d[k][k] < 0 exactly for vertices on a closed negative walk. A pair is set to `-PATH_INF` iff some such k is reachable from i and reaches j, which is equivalent to a negative cycle lying on an i→j walk. The marking costs O(n^2 · |K|) for the |K| negative-diagonal vertices.

### johnson

O(n·m + n·(n + m)·log(n + m + 1)); the same matrix as `floydWarshall` on every input, negative cycles included. Steps:

1. Run Tarjan SCC (on a directed copy with the same arc numbering when g is undirected).
2. Run Bellman–Ford over arcs inside components only, from zero labels. A change in pass n marks the component of the changed vertex: every negative cycle has a relaxable arc in every pass, and with arcs restricted to components the change stays inside the cycle's component. The marked vertices form the set `cyc`.
3. Run a second zero-label Bellman–Ford over the arcs that avoid `cyc` to get feasible potentials; that subgraph has no negative cycle.
4. For each source u, mark as negative every vertex reachable from a `cyc` vertex that u reaches (two BFS passes). Fill the remaining vertices from Dijkstra over reduced weights w + h[from] − h[to] ≥ 0 on the graph without `cyc`. A walk from u that avoids `cyc` is exactly a walk to an unmarked vertex, so these distances are exact. A source in `cyc` has only negative or unreachable entries.

With every weight ≥ 0, both Bellman–Ford loops stop after one pass, and `johnson` is n binary-heap Dijkstras at O(n·(n + m)·log(n + m + 1)), which is why it replaced `apspDijkstra`.

### minimaxFloydWarshall

O(n^3 / w + n^2 + m·log(m + 1)) with w = 64. It returns the n × n `lll` matrix of the least maximum arc weight over u→v walks; with `maximum = true`, the greatest minimum. Weights are signed. An unreachable pair holds the worst sentinel (`PATH_INF`, or `-PATH_INF` with `maximum`). The diagonal holds the best sentinel (`-PATH_INF`, or `PATH_INF`), meaning the empty walk; loops never matter.

The algorithm takes arcs in weight order (stable for equal weights). Each arc adds its pair to a bitset reachability relation, then closes the relation with an explicit stack: a new pair (u, v) contributes u → `to[v]` and `from[u]` → v, and each pair is processed once, at O(n / w) per pair.

Correctness: after each arc, the relation is the transitive closure of the arcs taken so far, since for pairs (a, b) and (b, c), whichever is processed later finds the other already set and adds (a, c). A pair first appears while the arc of weight w is processed, so its bottleneck is exactly w.

There is no witness. A walk of value x is any walk over arcs no heavier than x; per source it is `minimaxPath` or `widestPath` of `13-shortest_path_advanced.hpp`, whose results carry parents.

### fixedLengthShortestPath

O(n^3·log(k + 1) + m). It returns the least weight of a walk with exactly k arcs, or `PATH_INF`; k = 0 gives the min-plus identity. Weights are signed. It asserts k ≥ 0 and k · max |w| < 2^100, so every total stays below 2^100. In the inner loop, a sum with a `PATH_INF` operand stays above `PATH_INF / 2` and is normalized after each row, which keeps the loop branch-free. No witness; the walk can have up to k arcs.

### Eccentricities and eccentricities

O(n^2). `eccentricities` takes any square distance matrix with n ≥ 1 and no `-PATH_INF` entry, for example `johnson(g).dist`; one entry pass asserts both. Under `NDEBUG` a ragged matrix still gives a bounded, meaningless result. The returned `Eccentricities` holds `ecc[u]`, the maximum entry of row u, the diagonal included: the out-eccentricity, and `PATH_INF` when some vertex is unreachable. `radius` and `diameter` are the minimum and maximum of `ecc`; `center` and `periphery` list the vertices that attain them, in ascending order. On a directed graph, apply it to the transposed matrix for in-eccentricities.

### Legacy Mapping

Legacy matrices use `INF64` for an absent arc, so a 0 entry is a real zero-weight arc. `Graph::fromMatrix` would drop such entries; build the graph with `g.addEdge(u, v, d[u][v])` for every entry below `INF64` instead.

- `FloydWarshall(n, dis).runFloydWarshall()` (`OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`; vertices `[1, n]`, the 0 index unused): `floydWarshall(g)` on that graph over vertices `[0, n)`. `PATH_INF` / `-PATH_INF` replace `INF64` / `-INF64`.
- `floydWarshall(d)` (in place, 0-based; `OLD/algorithms.cpp:4702-4722` and `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp`, `test.cpp`): `floydWarshall(g).dist`. The legacy updates were unclamped and could overflow on negative cycles; the clamp removes that defect.
- `floyd_warshall()` (`OLD/Team Notebook/src/graph/old_floyd-warshall.cpp`; globals, no negative-cycle handling): `floydWarshall(g)`.
- `minPlusMatrixExp(a, k)` (`algs.cpp`, `algsbetter.cpp`; `INF64` entries absent): `fixedLengthShortestPath(g, k)`.
- `apspPath` (planned name): `AllPairsResult::path(g, u, v)`.
- `apspDijkstra` (removed 2026-11-03, user decision 2026-11-09: a narrowing of `johnson` with identical `dist` and `parent_arc` and the same cost): `johnson(g)`. Its nonnegative test cases run through `johnson` on every graph.

## Feature-to-Test Map

Tester: [`12-all_pairs_shortest_path_tester.cpp`](../../96-Local%20Testing/04-Graphs/12-all_pairs_shortest_path_tester.cpp), driven by its `.py`. Every graph is checked as `Graph` and, on alternating cases, as `CsrGraph`.

| Operation | Oracle and cases |
|---|---|
| `floydWarshall`, `johnson` | Per-source `lll` Bellman–Ford with negative-cycle marking (`bellmanOracle`), self-checked on exhaustive and random tiny graphs against walk enumeration up to a length that exposes any negative cycle. Cases: n = 0 and 1, loops with every extreme weight, an undirected negative edge, full-range chains, negative-cycle reachability, zero cycles, parallel arcs, a Johnson reweighting fixture, a Johnson exponential ladder (k = 40) on which Dijkstra without valid potentials improves vertex k 2^k times, exhaustive n = 2 and 3, random weights (small signed, nonnegative, full range, wide, potential-feasible negative), word-boundary n = 63–200 |
| `AllPairsResult`: `finite`, `path` | Every pair of every result: sentinel ↔ `finite`, `parent_arc` −1 exactly off finite or diagonal pairs, the path is simple and oriented, ends with `parent_arc`, and weighs `dist`. Constructor, repeated, copied and moved results; probes `path-size`, `finite-range`, `finite-row`, `finite-row-far`, `finite-column-far` |
| `minimaxFloydWarshall` (both directions) | Threshold reachability (`thresholdOracle`), self-checked against a bottleneck Floyd; word-boundary sizes 63/64/65/127/128/129 |
| `fixedLengthShortestPath` | Walk-length DP (`walkOracle`) for every k up to n + 2. Fixtures: k up to 10^12 on 3-cycles, k = INT64_MAX with zero weights and with no arcs, k at the 2^100 bound with INT64_MIN/MAX, the k = 0 identity, k = ⌊2^100 / w⌋ with k·w = 2^100 − 2^36. Probes `fixed-negative-k`, `fixed-overflow`, `fixed-overflow-far` |
| `Eccentricities`, `eccentricities` | Direct recomputation of every field on every oracle matrix without −PATH_INF. Fixtures: path, n = 1, disconnected, directed, full range, negative entries, a negative diagonal. Probes `ecc-empty`, `ecc-negative-infinite`, `ecc-non-square` |

## Commands and Results

Each run builds optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the 11 assertion probes) and, in full and stress, ASan+UBSan, all with `-Wall -Wextra -Wconversion -Werror`. The 2026-11-10 rows are library-wide runs (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run) with this tester's stage times. The 2026-11-05 and 2026-11-03 rows ran on GCC 16.0.1 20260322 (`g++`) and GCC 14.3.0 (`g++-14`); the 2026-11-05 rows follow the `/finish GR70` tests.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (GCC 16.2.1) | PASS, counts not recorded: optimized 0.1 s compile + 7.5 s run, checked 22.9 s + 34.2 s, ASan-UBSan 24.9 s + 62.0 s; peak 1212 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS, counts not recorded: 0.3 s + 11.4 s, 37.1 s + 53.2 s, 43.0 s + 91.8 s; peak 1174 MB |
| 2026-11-10 | `01-run.py --mode quick` | PASS: optimized 41.4 s + 0.2 s, checked 29.8 s + 12.7 s; peak 928 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` | PASS, counts not recorded: 0.1 s + 10.8 s, 14.1 s + 72.7 s, 15.8 s + 152.8 s; peak 917 MB |
| 2026-11-09 | `05-extraction.py ops --filter 04-Graphs/12-all_pairs_shortest_path.hpp` | PASS, 8 operations, 0 failures on `g++` and `g++-14` |
| 2026-11-09 | `05-extraction.py testers --filter 12-all_pairs_shortest_path` | PASS, 1 quick run on an extracted build, 0 failures |
| 2026-11-05 | `12-all_pairs_shortest_path_tester.py --mode quick` | PASS, 517 cases, 5.45·10^6 checks; peak 537 MB |
| 2026-11-05 | `… --mode full --seed 1` | PASS, 30303 cases, 1.18·10^8 checks, 3 configurations; peak 1085 MB |
| 2026-11-05 | `CXX=g++-14 … --mode full --seed 1` | PASS, same counts; peak 1089 MB |
| 2026-11-03 | `… --mode stress --seed 7` | PASS, 280958 cases, 3.01·10^8 checks; peak 984 MB, 237 s wall. The checked build alone took about 130 s against the runner's former 180 s limit, which is why stress runs two boundary rounds |
| 2026-11-03 | `02-integration.py` | PASS: 186 standalone and aggregate headers, multi-TU scalar and AVX2, workspace; peak 2744 MB |

```bash
python3 '96-Local Testing/04-Graphs/12-all_pairs_shortest_path_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/12-all_pairs_shortest_path_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/12-all_pairs_shortest_path_tester.py' --mode full --seed 1
python3 '96-Local Testing/04-Graphs/12-all_pairs_shortest_path_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/12-all_pairs_shortest_path.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 12-all_pairs_shortest_path
python3 '96-Local Testing/01-run.py' --mode full
python3 '96-Local Testing/02-integration.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `35a1ae0ab936391e9fba96f03ff35ec938797a1a` (2026-11-09, seed 1, 5 lines changed since blob `6deb990c6048`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The record covers only the `G = Graph` template defaults. The survivors below come from the whole-file campaign of 2026-11-05 by `/finish GR70` (seed 1, uniform sample of 500 of 674; 440 caught, 25 by timeout, 60 survived after the retest with the Johnson exponential ladder and four range probes), on lines the newer record did not cover; `AOS 262` was closed later by `minimaxScaleFixture`. Line numbers are the current header's.

Some arguments rest on user decisions of 2026-11-09 (`/merge-pr 4`): a changed choice among ties or of state no caller reads is within the contract, and guards reachable only under `NDEBUG` or beyond every memory cap are untestable and kept. The runtime-only mutants stay open, since no ruling accepts that class without a measured ratio: each needs a test or an equivalence argument.

| Mutant | Line | Argument |
|---|---|---|
| `COS 386`, `COS 431`, `COS 436`, `COS 451`, `COS 466`, `COS 481`, `COS 491`, `COS 501`, `COS 531`, `COS 546`, `COS 566`, `COS 586`, `COS 591`, `COS 601`, `COS 606`, `COS 641` | 16, 26, 43, 55, 61, 67, 68, 71, 90, 97, 130, 147, 148, 151, 152, 165 | equivalent: the counter starts at 0, steps by 1 and meets its bound, which is at least 0 and fixed in the loop |
| `COS 418` `!=` → `>` | 20 | equivalent: no entry is below -PATH_INF (Floyd-Warshall clamps there; Johnson stores exact distances) |
| `COS 432` `<` → `<=` | 26 | equivalent: the parent arcs of a finite pair form a path of at most n - 1 arcs, so `v == u` ends the walk first |
| `COS 464`, `COS 474`, `COS 499` | 59, 63, 70 | equivalent: no entry exceeds PATH_INF |
| `AS 189`, `AS 215`, `AS 219`, `AS 225`, `AS 226` | 64, 140, 151, 166, 167 | equivalent: `min`, `max` and the called function are symmetric in value |
| `COS 486` `<` → `!=` | 67 | equivalent: `d[k][k]` starts at 0 and only decreases |
| `AS 190` `e.from, e.to` → `e.to, e.from` | 80 | equivalent: an undirected `Graph` stores both orientations of every edge, so `dg` gets the same arcs and the same components |
| `BL 234`, `IT 172`, `IT 174` | 83, 98, 102 | equivalent: without a negative cycle `negative` stays zero; the extra BFS per source costs O(n + m) inside the bound, and a BFS from no seed visits nothing |
| `IF 150` `(any && cyc[u])` → `(false)` | 103 | equivalent: every vertex a search from a cycle vertex reaches is reachable from that cycle, so `negative` already marks it and `fillRow` writes -PATH_INF first |
| `COS 512`, `COS 529` | 84, 89 | equivalent: marking needs `pass == n - 1`, and the extra pass `n` marks nothing |
| `COS 519` `==` → `>=` | 87 | equivalent: `tarjanScc` numbers components topologically, so a cross arc has `comp[from] < comp[to]` |
| `LOS 380`, `COS 536`, `COS 537` | 92 | equivalent: the second Bellman-Ford skips cycle components, so it converges within n - 1 passes; extra passes change nothing and stay O(n * m) |
| `CR 100`, `IT 171`, `AOS 254`, `COS 541`, `COS 543` | 91, 95 | runtime only, open (contributor proposal 2026-11-05): `dijkstraPotential` corrects labels, so distances stay exact; only the potential, hence the running time, changes |
| `BAOS 361` `\|=` → `^=` | 115 | equivalent: `to[u]` is zero there, so `^=` sets the same bit |
| `BAOS 362`, `SOS 661` | 115 | equivalent: the diagonal bit of `from[u]` only makes `add` revisit the pair being closed, which it rejects, so losing it changes nothing |
| `BAOS 367` `\|=` → `^=` | 124 | equivalent: `add` runs once per pair (the `to` bit guards it), so the `from` bit is still clear and `^=` sets it |
| `AOS 338` `*` → `+` | 146 | equivalent: the start value only has to exceed PATH_INF / 2, the normalization threshold; finite sums stay below 2^101 |
| `IF 152`, `COS 598`, `COS 599` | 149 | equivalent: a PATH_INF row entry gives sums of at least PATH_INF - 2^101 > PATH_INF / 2, normalized to PATH_INF (2 * PATH_INF < 2^127); no entry exceeds PATH_INF |
| `COS 614` `>` → `>=` | 152 | equivalent: no sum equals PATH_INF / 2: finite totals are below 2^101, the others at least PATH_INF - 2^101 |
| `COS 616` `>` → `!=` | 154 | equivalent: `k >= 0` (asserted) halves down to 0 |
| `AS 220` `res, base` → `base, res` | 155 | equivalent: `res` and `base` are min-plus powers of one matrix, which commute |
| `COS 621` `>` → `!=` | 156 | equivalent: `k >= 1` inside the loop |
| `COS 624` `>` → `>=` | 156 | equivalent: one extra squaring after the last bit, never read; its entries stay below 2^102 and it costs one more O(n^3) product inside the bound |
| `AS 221` `base, base` → `base, base` | 156 | equivalent: the mutation leaves the text unchanged |
| `COS 639` `==` → `>=` | 163 | equivalent: `find` returns an iterator in [begin, end] |
| `COS 659` `==` → `>=` | 170 | equivalent: `diameter` is the maximum eccentricity |

## Sources

See [the 2026-11-03 ledger](00-sources.md#pages-fetched-on-2026-11-03-p042-gr04): cp-algorithms (Floyd–Warshall negative-cycle marking, fixed-length paths), suisen (bitset minimax closure), NetworkX (Johnson, distance measures). No catalog code was copied.

## Limits and Handoffs

- Every result is Θ(n^2) memory; n ≈ 5000 already needs 500 MB for `AllPairsResult`.
- The min-plus product should move to Core Matrix `mulSemiring` once that planned operation is verified (follow-up for the Core Matrix package).
- Bitset transitive closure and the minimum-weight cycle belong to rows 40 and 68.

## History

- 2026-11-03: P042 GR04 rewrite from the pinned legacy excerpt; full suite passed on g++ and g++-14, stress seed 7; review: apspDijkstra removed (user decision 2026-11-09), 11 findings fixed.
- 2026-11-05: `/finish GR70` mutation record (whole file, 500 of 674 mutants): the Johnson exponential ladder and four range probes closed 13 survivors; 60 recorded with arguments.
- 2026-11-08: `/merge-pr 4` rebase on master: row set to `partial`, since the P254 omission sweep of 2026-11-08 added operations after this work (`missing:` in the inventory row); no code or test change.
- 2026-11-08: `/merge-pr 4` review fixes: `G = Graph` defaults, `AOS 262` closed by a fixture.
- 2026-11-09: /merge-pr 4 second review round: changed-line mutation campaign recorded above; extraction ops and testers passed.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; library-wide quick, full (g++ and g++-14) and stress runs passed.
