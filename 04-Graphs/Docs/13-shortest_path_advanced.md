# 13-shortest_path_advanced.hpp — evidence

Owned by package P042 / GR04 (all-pairs and advanced shortest paths, with `12-all_pairs_shortest_path.hpp`); see the [common graph contract](00-notes.md#p010-common-contract) and [05-shortest_path.md](05-shortest_path.md) for `ShortestPathResult`, `PathWitness` and `relax`.

## Contracts

### Common domain

Every function takes `Graph` or `CsrGraph`, directed or undirected, with loops and parallel edges, `lng` weights and `lll` totals. Sources are a vector: duplicates are ignored, and an empty vector reaches nothing. A function documented for nonnegative weights asserts that for every arc at entry. Under `NDEBUG`, a violation returns an empty or sources-only result at once, so it never loops or overruns. The same holds for the weight bounds of `dialDijkstra` and `radixHeapDijkstra`. `PATH_INF = 2^120` lies above every exact total and bottleneck of this header and of row 12. Engines that return `ShortestPathResult` give exact `dist` where `finite(v)`, parents that form a shortest-path forest, and, for the signed engines, the `negative` marks of `bellmanFord`.

### StPath and TopTwoPaths

`StPath` holds an s–t total `dist` and its `path`; when t is unreachable, `path.exists` is false and `dist` is 0.

`TopTwoPaths` stores 2n states; state `2 * v + r` is rank r ∈ {0, 1} at v. `source[x]` is the source of the state's walk, or −1 when the state is absent. `dist[x]` is that walk's weight. `parent[x]` is the previous state and `parent_arc[x]` the arc into v (both −1 at a source's zero walk). `exists(v, r)` asserts r ∈ {0, 1}. `walk(g, v, r)` returns the oriented walk, which may repeat vertices. Parents always point to states accepted earlier, so the chain ends.

### spfa

FIFO label correcting, signed weights, O(n·m + n + k). In FIFO order, a vertex enters the queue at most once per Bellman–Ford phase, and without a reachable negative cycle there are at most n phases (sources form phase 0). A vertex queued more than n times therefore proves a reachable negative cycle. In that case the function returns `bellmanFord(g, sources)`, so the `negative` marks and `negative_cycle` always match `bellmanFord`. Without a negative cycle, the result has the same distances and `negative_cycle` is empty. It is a heuristic and is not claimed to be faster than `bellmanFord` in general.

### desopoPape

The D'Esopo–Pape deque rule, signed weights, O(n·m + n + k). A vertex never queued goes to the back. A vertex already processed whose label improves goes to the front. A queued vertex stays where it is. The rule alone has an exponential worst case and never stops on a reachable negative cycle. Every pop is therefore charged its out-degree plus one against a budget of n·(m + 1). Once the budget runs out, the function returns `bellmanFord(g, sources)`. Integer labels strictly decrease and are bounded below when there is no negative cycle, so the pure rule terminates there; the budget only bounds the time.

### dialDijkstra

A bucket queue of C + 1 circular buckets, where C is the maximum weight, which must lie in [0, 2^31 − 1). Time O(k + m + n·C), memory O(n + m + C). Every finite distance is at most (n − 1)·C. A bucket can be refilled through a zero-weight arc while it is being drained, so it is drained until empty.

### radixHeapDijkstra

Data Structures 31 `RadixHeapArray<ulng>` with `decreaseKey`, O(k + m + n·b) with b = 64. Nonnegative weights with (n − 1) · max w < 2^64, asserted at entry, so every finite distance fits the `ulng` key. Popped labels are final, so a vertex is pushed at most once.

### aStar

Nonnegative weights and a heuristic `h(v)` (any integer type convertible to `lll`) with h(v) ≤ dist(v, t) for every v that reaches t; values elsewhere, and negative values, are allowed, with |h(v)| < 2^126 so that dist + h fits `lll`. `h` is evaluated once per reached vertex other than t, and h(t) is taken as 0. Vertices are reopened when their label improves, so an admissible but inconsistent heuristic stays correct: while t's label exceeds dist(s, t), the first vertex u of a shortest path whose label is not yet optimal has been relaxed to it and sits in the heap with key dist(s, u) + h(u) ≤ dist(s, t), below t's key. So when t is first popped, its label is optimal. A negative h(t) would break this, which is why it is not used. A consistent heuristic gives O((n + m)·log(n + m + 1)); an inconsistent one can take exponential time. Unreachable t returns an absent path.

### bidirectionalDijkstra

Nonnegative weights. Two lazy Dijkstras run, from s over g and from t over the reversed arcs; the reversed in-lists cost O(n + m) per call. The side with the smaller top advances. `best` is the minimum of `df[v] + db[v]` over vertices labelled by both sides, rechecked whenever either label changes. The search stops when the two tops sum to at least `best`, or when one side is exhausted. If s == t the path has one vertex.

### secondShortestPath and top2Dijkstra

Nonnegative weights, O(n + (k + m)·log(k + m + 1)). One engine pops heap entries (weight, vertex, parent state, arc, source) in nondecreasing order. The first entry at v becomes rank 0. A later entry becomes rank 1 when `accept` allows it. Pushes from v stop once v has rank 1, so there are at most 2·m + k entries.

- `secondShortestPath(g, sources, strict = true)`: rank 0 is the shortest walk from any source. With `strict`, rank 1 is the least walk weight strictly above rank 0. Otherwise it is the second element of the multiset of all walk weights (distinct arc sequences), so it equals rank 0 when two shortest walks exist. A source's zero walk is a walk, so its rank 1 is the lighter of its lightest closed walk and its lightest walk from another source, each of positive weight when strict. Correctness: the i-th lightest walk to v, for i ≤ 2, ends with an arc from u, and its prefix is among the two lightest walks to u, or the lightest walk strictly above u's rank 0 in the strict case. Replacing a heavier prefix would give a walk strictly between the first two at v.
- `top2Dijkstra(g, sources)` (the 2026-11-08 sweep's `nearestTwoSources`): rank 0 is the nearest source, and rank 1 the nearest source different from rank 0's source, each with its distance and a walk witness. Ties between sources pick any minimizer. A walk to v from source s uses only states labelled s, and each vertex keeps the two best distinct sources, which is enough because a best-two-distinct-sources prefix extends.

### shortestPathDag

O(n + m). `sp` must come from a sum-weight engine run on g (`dijkstra`, `bellmanFord`, `spfa` and so on; not a bottleneck engine). `arcs` lists every arc that joins finite vertices with `dist[from] + w == dist[to]`, in arc order. These are exactly the arcs that lie on some shortest walk from a source. `order` is a Kahn topological order of the finite vertices that no tight cycle reaches through tight arcs. `acyclic` says that `order` holds every finite vertex. Tight cycles have weight 0.

### countShortestPaths

O(k + (n + m)·log(n + m + 1)) when every weight is ≥ 0 (Dijkstra), and O(n·m + n + k) otherwise (Bellman–Ford). `mod` lies in [1, 2^63], so the sum of two residues fits `ulng`. `count[v]` is the number of shortest walks from any source, modulo `mod`. A source contributes its zero walk only when `dist` is 0 there. `infinite[v]` marks finite vertices with infinitely many shortest walks: every finite vertex has a tight walk from a source, so a reachable tight cycle repeats without bound. `count` is meaningful where `paths.finite(v)` and not `infinite[v]`; it is 0 at negative-infinite vertices.

### minimaxPath and widestPath

Bottleneck Dijkstra, signed weights, O(k + (n + m)·log(n + m + 1)). `dist` is the least maximum arc weight (`minimaxPath`), or the greatest minimum (`widestPath`, its mirror, ruling 3), over walks from any source. Sources hold the empty-walk value: `-PATH_INF` for `minimaxPath`, `PATH_INF` for `widestPath`. Combining a label with an arc weight by max or min is monotone, so labels pop in order and popped labels are final; the parents form a forest of witness paths. `negative` stays false.

### negativeSsspScaling

Goldberg-style bit scaling for integer weights. It costs O(sqrt(n)·(n + m)·log(N) + (n + m)·log(n + m + 1) + k), plus O(n·m) for the `bellmanFord` rerun when a negative cycle is reachable, with N = 2 + the largest |w| of a reachable negative arc. It works only on the part reachable from the sources. Let B be the bit length of that largest |w|: there is no phase when no reachable arc is negative, and one phase when the most negative weight is −1.

**Scaling.** For t = B − 1 down to 0, the potential p doubles and the phase weight of every arc is c_t = ceil(w / 2^t), so c_0 = w. Rounding up gives c_t ≥ w / 2^t, so a cycle of weight ≥ 0 keeps a scaled weight ≥ 0 and `refine` never reports a false negative cycle. Since c_{t−1} ≥ 2·c_t − 1 and the previous phase ended with reduced weights ≥ 0, every phase starts with reduced weights ≥ −1. At t = B − 1 they start at ≥ −1 because N < 2^B. `refine` then makes all reduced weights ≥ 0. A final Dijkstra over w + p[from] − p[to] ≥ 0 with start keys −p[s] gives exact distances and parents. `p` stays below about n^1.5 · 2^64 < 2^112, so every reduced weight fits `lll`.

**Refine.** Let A be the live arcs with reduced weight ≤ 0 (that is, 0 or −1). A −1 arc inside a strongly connected component of A proves a negative cycle. A vertex is improvable if a −1 arc enters it; let k be their number. Over the condensation, d(x) is the least A-weight of a walk ending in component x, which lies in [−n, 0]. Each round does one of two steps:

- **Cut**, when some layer L_j = {d = −j} holds c ≥ √k improvable vertices. Decrease p by 1 on S = {d ≤ −j}. An arc leaving S has reduced weight ≥ 1 (an A arc leaving u ∈ S would put its head in S), so it stays ≥ 0. Every −1 arc into a vertex of L_j starts outside S (d(u) ≥ d(v) + 1) and becomes 0. Arcs entering S never decrease. So k drops by c and no −1 arc is created.
- **Path**, otherwise. Every layer holds fewer than √k improvable vertices, so the deepest component has depth D > √k. Its predecessor chain enters improvable vertices y_1, …, y_D in walk order, and the walk from y_i to y_j weighs −(j − i). A Dial pass over the clipped weights max(reduced, 0) computes D(v) = min(0, min_i(−i + dist⁺(y_i, v))) ∈ [−D, 0], and p += D(v). Then D(v) ≤ D(u) + max(w', 0) for every arc, so the new reduced weight is at least min(w', 0) ≥ −1, and it equals −1 only on old −1 arcs with D(u) = D(v). Suppose an arc (u, y_i) of weight −1 kept D(u) = D(y_i). Then D(u) = −j + dist⁺(y_j, u) ≤ −i for some j ≥ i. If j = i, u is A-reachable from y_i, which closes an A cycle with a −1 arc. If j > i, the walk y_i → y_j → u → y_i weighs at most −(j − i) + (j − i) − 1 < 0. Either way there is a negative cycle. So without one, every y_i is fixed and k drops by at least D.

Every round reduces k (by at least √k, or the function detects a negative cycle when a path step fixes fewer than D vertices), so `refine` ends after O(√n) rounds of O(n + m). A negative cycle is always detected: a run that ended with k = 0 would yield a feasible potential. On detection the function returns `bellmanFord(g, sources)`, so its results always match `bellmanFord`.

### Legacy mapping

- `SPFA(n).solve(s, adj)` (`OLD/algorithms.cpp`; `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp`, `test.cpp`; `INF64` labels, `false` on a negative cycle): `spfa(Graph::fromAdjacency(adj, true), {s})`. `d[v]` is `dist[v]` where `reachable[v]`; the legacy `false` corresponds to a nonempty `negative_cycle`, or to any `negative` mark.
- `DEsopoPape(n).solve(s, adj)` (`algs.cpp`, `algsbetter.cpp`, `test.cpp`; never terminated on negative cycles): `desopoPape(Graph::fromAdjacency(adj, true), {s})`.
- `bellman_ford(src)` (`OLD/Team Notebook/src/graph/old_bellman-ford.cpp`): already mapped to `bellmanFord` in [05-shortest_path.md](05-shortest_path.md#legacy-mapping); `spfa` and `desopoPape` share its results.

## Feature-to-test map

Tester: [`13-shortest_path_advanced_tester.cpp`](../../96-Local%20Testing/04-Graphs/13-shortest_path_advanced_tester.cpp), driven by its `.py`. Oracles are written in the tester:

- **Distances:** an `lll` Bellman–Ford with negative-cycle closure, self-checked against walk-length DP on n ≤ 5.
- **Tight arcs:** the tight-arc set and the vertices reachable from a tight cycle, the latter cross-checked through tight walks of n to 2n − 1 arcs.
- **Counts:** tight walks of fewer than n arcs, counted modulo mod.
- **Second shortest:** walk counts per (weight, vertex), capped at 2, with weights bounded by (2n − 1)·C by the fewest-arcs argument.
- **Bottleneck:** threshold reachability, checked by a fixpoint relaxation.
- **Self-check:** walk enumeration up to 2n − 1 arcs checks the count, infinite and second-shortest oracles.

Every `ShortestPathResult` gets its parent chain checked (bounded, ends at a source, equals `path(g, v)`, weight or bottleneck equals `dist`); every reported negative cycle must be closed, negative and made of negative-infinite vertices.

| Operation | Cases |
|---|---|
| `PATH_INF`, `StPath` | Unreachable t (`exists` false, `dist` 0, empty storage), s == t one-vertex path |
| `TopTwoPaths`: `exists`, `walk` | Every state of every result: oriented, weight equals `dist`, starts at `source`, absent states empty; probes `exists-rank`, `exists-negative-rank`, `walk-size`, `walk-smaller` |
| `spfa`, `desopoPape`, `negativeSsspScaling` | Distance oracle and exact agreement with `bellmanFord` (reachable, negative marks, dist, negative-cycle report), parent forest. Fixtures: reachable and unreachable negative cycles, zero cycles, full-range loops, INT64_MIN chains, LMIN + LMAX cycles, unreachable negative parts. Random: extreme weights and potential-generated negative weights (\|h\| ≤ 50 or 2^61), n up to 300, empty, duplicate and multiple sources |
| `desopoPape` budget | A complete DAG with arcs i → j listed by decreasing j, w = 2^(j−i), plain and potential-shifted. A copy of the pure rule in the tester confirms more than n·(m + 1) scans at every size used (n = 32 … 62); the result must equal `bellmanFord` |
| `dialDijkstra` | Random graphs with (n + 1)·(C + 1) ≤ 4·10^6. Fixtures: C = 2^22 − 1 (2^20 − 1 in quick), C = 0 with zero cycles. Probes `dial-negative`, `dial-int-max`, `dial-weight-max` |
| `radixHeapDijkstra` | Random graphs with (n − 1)·C < 2^64. Fixtures: an LMAX loop, LMAX arcs at n = 2 and 3, and distance exactly 2^64 − 1 at n = 4. Probes `radix-negative`, `radix-bound` |
| `aStar` | Every (s, t) for n ≤ 6, otherwise 8 random pairs plus s == t, with h = 0, the exact h, and a random admissible h that goes negative. h is arbitrary (±2^100, LMIN, LMAX) at t and at vertices that cannot reach t. The tester checks that h(t) is never evaluated and that no h(v) is evaluated twice. Fixture: h(t) = −100 regression. Probes `astar-negative`, `astar-range`, `astar-target-far` |
| `bidirectionalDijkstra` | The same pairs, directed and undirected, `CsrGraph`; probes `bidirectional-negative`, `bidirectional-range` |
| `secondShortestPath` | Strict and non-strict against the second-shortest oracle; non-strict rank 1 is a different walk. Fixtures: zero loop, an undirected edge there and back, parallel arcs 5/5/7. Probes `second-negative`, `second-source-range` |
| `top2Dijkstra` | Per-source oracle: rank 0 is a nearest source, rank 1 a different source at the minimum over the others. Fixtures: ties, duplicate sources (no rank 1), empty sources. Probes `top2-negative`, `top2-source-range` |
| `shortestPathDag` | On `bellmanFord`, `negativeSsspScaling` and `dijkstra` results: `arcs` equals the oracle's tight arcs in order; `order` is exactly the vertices not reachable from a tight cycle, topological on tight arcs; `acyclic` exactly when it holds every finite vertex. Probe `dag-size` |
| `countShortestPaths` | `paths` equals `dijkstra` or `bellmanFord` field for field; `infinite` and `count` against the oracles; mod ∈ {1, 2, 3, 7, 998244353, 2^61 − 1, 2^63 − 1, 2^63}. Closed forms 2^i on a 100-diamond chain and 3^i on 40 triple parallel arcs. Probes `count-mod-zero`, `count-mod-big` |
| `minimaxPath`, `widestPath` | Bottleneck oracle, `negative` all false, sources at ∓`PATH_INF`, witness max/min equals `dist` |
| Detail engines (`detailFixtures`) | `dijkstraPotential` with a random blocked set against the distance oracle on the graph without arcs into it; `refine` with weights in {−1, …, 2} returns ok exactly when the live part has no negative cycle, and then leaves every live reduced weight nonnegative; 300 / 3000 / 15000 graphs each |
| Stale queue entries (`staleEntryFixtures`, optimized builds) | k = 300000 arcs into one vertex, each improving it, then k arcs out: closed-form results for `minimaxPath`, `widestPath`, `negativeSsspScaling`, `aStar` and `bidirectionalDijkstra`, which time out if they rescan stale entries |

Common cases are `Graph` and `CsrGraph`, directed and undirected, loops, parallel arcs, n = 0 and 1, and repeated and copied calls. Exhaustive graphs come in three tiers:

- quick: n = 2 with weights {−1, 0, 1};
- full adds n = 2 with {0, 1, 2}, n = 3 directed without loops ({−1, 0, 1} and {0, 1, 2}) and n = 3 undirected with loops;
- stress adds n = 3 directed with loops.

Random graphs (quick / full / stress): 150 / 2000 / 10000 with n ≤ 8; 8 / 80 / 400 with n in [10, 40]; 4 / 30 / 120 with n up to 300.

Results reveal the `bellmanFord` fallback only through parents on ties. The fixture "scaling without fallback" pins that: on its tie, the scaling picks parent 1 and `bellmanFord` picks 2. For broader evidence, a scratch copy of the header with counters was therefore run on 20000 random graphs: potential-generated directed graphs with n up to 400 and |h| up to 2^61, plus undirected graphs. All 996 graphs with a reachable negative cycle took the fallback. None of the others did. There were 529329 cut steps and 43227 path steps, and every result matched `bellmanFord`.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 1083 MB; optimized 0.1 s compile + 6.6 s run, checked 36.6 s compile + 17.3 s run, ASan-UBSan 51.9 s compile + 72.5 s run; MEMORY peak 1519 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 1085 MB; optimized 0.4 s compile + 11.1 s run, checked 58.7 s compile + 32.7 s run, ASan-UBSan 82.8 s compile + 74.7 s run; MEMORY peak 1547 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 50.7 s compile + 4.3 s run, checked 28.7 s compile + 22.2 s run; MEMORY peak 1049 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 525 MB; optimized 0.1 s compile + 7.0 s run, checked 20.5 s compile + 55.3 s run, ASan-UBSan 27.2 s compile + 125.9 s run; MEMORY peak 877 MB. Dated rows below stay those commands' latest results.

2026-11-05 (after the `/finish GR70` tests; the stress and integration rows are from 2026-11-03, before them), GCC 16.0.1 20260322 (`g++`) and GCC 14.3.0 (`g++-14`). Each run builds optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the 21 assertion probes) and, in full and stress, ASan+UBSan, all with `-Wall -Wextra -Wconversion -Werror`.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/04-Graphs/13-shortest_path_advanced_tester.py' --mode quick` | PASS, 1214 cases, 2.38·10^6 checks, peak 598 MB |
| `… --mode full --seed 1` | PASS, 21114 cases, 1.03·10^7 checks, 3 configurations, peak 768 MB |
| `CXX=g++-14 … --mode full --seed 1` | PASS, the same counts, peak 768 MB |
| `… --mode stress --seed 7` | PASS, 62897 cases, 3.58·10^7 checks, peak 796 MB |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 186 standalone and aggregate headers, multi-TU scalar and AVX2, workspace; peak 2744 MB |

2026-11-09: `python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/13-shortest_path_advanced.hpp` passed (19 operations, 0 failures on `g++` and `g++-14`), and `testers --filter 13-shortest_path_advanced` passed (1 quick run on an extracted build, 0 failures).

## Mutation testing

Record: mewt 4.0.0 on blob `368fecb1194b24b17ff188f3dd858e46b29b9688` (2026-11-09, seed 1, 2 lines changed since blob `29d01ccc70d9`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

Earlier campaign: mewt 4.0.0 on blob `29d01ccc70d999dcc426cec79b507726b477e188` (2026-11-09, seed 1, 26 lines changed since blob `9c878282421c`): 33 mutants, 33 caught (0 by timeout), 0 survived, 0 untested.

Stage seconds of that campaign: quick 130, full-optimized 42, full-all 252.

Earlier campaign: mewt 4.0.0 on blob `9c878282421c0947f7c33b87f2ee6e0e6622541b` (2026-11-05, seed 1, whole file, uniform sample of 1330): 500 mutants, 437 caught (26 by timeout), 63 survived, 0 untested.

Stage seconds, over the campaign and its two survivor retests together: quick 3303, full-optimized 702, full-all 6152. The first pass, run 2026-11-04 to 05 by `/finish GR70` because this header had no record, left 91 survivors. Tests were added for the real gaps, and the retest with the same workdir caught 28 of them. The new tests are `detailFixtures`, `staleEntryFixtures`, a fixed star case for `refine` from the review and four probes, `dial-weight-max`, `astar-target-far`, `exists-negative-rank` and `walk-smaller`. `detailFixtures` checks `dijkstraPotential` with a random blocked set against the oracle on the graph without arcs into it, and checks `refine`'s contract (ok exactly without a live negative cycle, then every live reduced weight nonnegative). `staleEntryFixtures` runs under `NDEBUG` only: k = 300000 arcs into one vertex, each improving it, then k arcs out of it, so an engine that rescans stale queue entries does k² work and times out, with closed-form results for `minimaxPath`, `widestPath`, `negativeSsspScaling`, `aStar` and `bidirectionalDijkstra`.

Closed 2026-11-08 by new fixtures, each checked by rebuilding the mutant: `IT 346`, `LOS 795`, `AOS 632`, `AOS 633` and `COS 1026` (`fallbackTieFixtures`, a white-box check: tie parents stay outside the contract); `IF 302`, `COS 1114` (`aStarCallFixture`); `BL 476`, `AOS 589`, the `+`, `<`, `<=` and `==` forms of the frequent-layer test at line 148 and `COS 1008` (`scalingScaleFixtures`, by timeout).

Each remaining survivor was checked for type context, reachability, downstream use, semantic comparison and a distinguishing input. Some arguments rest on user decisions of 2026-11-09 (`/merge-pr 4`): a changed choice among ties or of state no caller reads is accepted as within the contract, and guards reachable only under `NDEBUG` or beyond every memory cap are untestable and kept. Runtime-only mutants are recorded with their argument but stay open, since no ruling accepts that class: each needs a test or an equivalence argument.

| Mutant | Line | Argument |
|---|---|---|
| `BL 478` `false` → `true` | 137 | untestable (user decision 2026-11-09): no input reaches this round-budget exit. The frequent-layer step provably leaves k <= need, and the path step creates no new negative heads; 9.1·10^6 random graphs (review and `/finish` scratch runs, 2026-11-05) never reached it, negative cycles included. The guard stays |
| `COS 861`, `COS 921`, `COS 976`, `COS 1016`, `COS 1146`, `COS 1226`, `COS 1236`, `COS 1266` | 63, 126, 149, 168, 267, 314, 317, 331 | equivalent: the counter starts at 0, steps by 1 and meets its bound, which is at least 0 and fixed in the loop |
| `COS 838`, `COS 853`, `COS 988`, `COS 1187` | 25, 33, 152, 285 | equivalent: the value is -1 or a nonnegative index, so the two tests agree |
| `COS 868`, `COS 1173` | 66, 275 | equivalent: a vertex's queue keys only decrease, so a popped key is never below its current one |
| `IT 329`, `COS 897`, `COS 899` | 98 | equivalent: an entry pushed to a vertex with both ranks set is popped and dropped at line 93; each vertex settles at most twice, so pushes stay at most 2m |
| `AS 406`, `AS 412` | 113, 165 | equivalent: `min`, `max` and the called function are symmetric in value |
| `COS 939` `==` → `>=` | 134 | equivalent: `tarjanScc` numbers components topologically, so a cross arc has `comp[from] < comp[to]` |
| `COS 942`, `COS 1232` | 136, 316 | equivalent: the count is never negative |
| `IF 276`, `COS 938` | 134 | runtime only (contributor proposal 2026-11-05): without the early exit, the `need` guard still returns false on a negative cycle within O(sqrt(k)) rounds |
| `COS 962` `<` → `<=` | 144 | equivalent: a tie picks another predecessor on an equally negative path, which still has t = -min d negative arcs, all the path step needs |
| `COS 991` `<` → `!=` | 153 | equivalent: `h` holds only arcs of reduced weight -1 or 0 |
| `COS 1002` `<` → `<=` | 160 | equivalent: bucket t would hold label 0, which is never pushed (a push needs a label below 0) |
| `AOS 627`, `AOS 628` | 169 | runtime only (contributor proposal 2026-11-05): a looser `need` only delays the negative-cycle exit |
| `COS 968`, `COS 969`, `COS 971` | 146, 148 | runtime only (contributor proposal 2026-11-05): `refine` keeps its contract here, and any potential it leaves is corrected by the label-correcting final search or the `bellmanFord` fallback |
| `COS 1029`, `BL 481` | 182, 183 | equivalent: each change only queues more vertices or falls back to `bellmanFord` sooner; results are exact and stay within the O(n * m + n + k) bound, parents among ties being any (user decision 2026-11-09) |
| `AOS 636`, `COS 1037` | 190, 194 | equivalent: the budget only decides when to fall back to `bellmanFord`, exact inside the same O(n * m + n + k) bound, parents among ties being any (user decision 2026-11-09) |
| `COS 1042` `==` → `<=` | 198 | equivalent: `state` is 0, 1 or 2 |
| `CR 184` `if (!ok) { return ShortestPathResult(g.n, sources); }` → `/* if (!ok) { return ShortestPathResult(g.n, sources); } */` | 209 | untestable (user decision 2026-11-09): runs only on out-of-domain weights under NDEBUG, which no allowed test gives; the guard stays |
| `LOS 797`, `COS 1073` | 216 | equivalent: the bucket popped at `cur` holds only entries labelled `cur` (labels in [cur, cur + c] are distinct mod c + 1), so `dist != cur` means the vertex is done |
| `CR 213`, `COS 1141` | 264 | equivalent: `ShortestPathResult(g.n, {s})` and `({t})` assert the same ranges before any index |
| `AOS 679`, `COS 1162` | 272 | equivalent: the search only runs longer; `best` changes only on a strict improvement and is minimal at the original stop, within the same bound |
| `COS 1165`, `COS 1166`, `COS 1167`, `COS 1168` | 273 | equivalent under the contract (user decision 2026-11-09): any alternation keeps bidirectional Dijkstra exact; only the meeting vertex among equal paths changes |
| `BL 489` `true` → `false` | 311 | equivalent: `acyclic` is assigned at line 320 before it is read |
| `IT 379`, `LOS 814` | 355 | runtime only (contributor proposal 2026-11-05): `big` wraps to about 2^64, so the scaling runs up to 64 rounds instead of ceil(log2 N); every round keeps its invariant and results are exact. On an LMAX arc the mutant's `-(e.w + 1)` overflows; GCC folds it to `~e.w` (UBSan silent in a 2026-11-05 review scratch run), so this case rests on that code generation |
| `COS 1286` `>=` → `!=` | 362 | equivalent: for w = 0 the negative branch gives -(0 >> t) = 0; for w < 0 the positive branch gives (w + 2^t - 1) >> t, which is ceil(w / 2^t) = -(\|w\| >> t) under the C++20 arithmetic shift |

## Benchmarks

Driver: [`13-shortest_path_advanced_benchmark.py`](../../96-Local%20Testing/04-Graphs/13-shortest_path_advanced_benchmark.py) with its `.cpp`, run as `python3 '96-Local Testing/04-Graphs/13-shortest_path_advanced_benchmark.py'` (`--no-large` skips the 26 s case). It compares `negativeSsspScaling`, `spfa` and `bellmanFord` from source 0.

Conditions:
- **Run:** 2026-11-03 on an AMD Ryzen AI 9 HX 370 (WSL2), g++ 16.0.1 20260322, `-std=gnu++20 -O2 -DNDEBUG -Wall -Wextra -Wconversion -Werror`.
- **Seeds:** 20261103 + input index (`mt19937_64`).
- **Repetitions:** one warmup per engine, then 5 interleaved repetitions (3 for the large case); the table gives medians.
- **Checks:** every run must equal the warmup on `reachable` and `dist`, and the warmup is certified: every arc is feasible, every reachable vertex has a tight parent arc, and nothing is marked negative. Parent chains are not walked, so exactness rests on the match with `bellmanFord`.
- **Memory:** peak RSS per input process; 357 MB for the whole capped run, compile included.
- **Load:** the run overlapped the 13 full, 13 `g++-14` full and 12 quick and full tester runs on this 24-thread machine.

| Input | n | m | scaling | spfa | bellmanFord | peak RSS |
|---|---|---|---|---|---|---|
| potential, W = 2^61 | 10^4 | 3·10^4 | 0.625 s | 0.0008 s | 0.0012 s | 17.8 MB |
| potential, W = 50 | 10^5 | 3·10^5 | 1.107 s | 0.051 s | 0.050 s | 52.4 MB |
| potential, W = 2^61 | 10^5 | 3·10^5 | 26.47 s | 0.082 s | 0.064 s | 55.3 MB |
| chain | 3·10^4 | 9·10^4 | 0.307 s | 0.0032 s | 0.028 s | 22.9 MB |
| antififo, W = 2^30 | 10^4 | ≈ 3·10^4 | 0.039 s | 0.360 s | 0.371 s | 18.0 MB |
| antififo, W = 2^30 | 3·10^4 | ≈ 9·10^4 | 0.138 s | 5.165 s | 3.502 s | 18.8 MB |

The inputs:
- **potential:** weights h[u] − h[v] + [0, 2], with h uniform in [−W, W] and 3n random arcs.
- **chain:** a −1 chain listed in reverse arc order plus 2n forward arcs.
- **antififo:** a star-plus-chain construction, the known anti-SPFA pattern (the generator was written in-house):
  - arcs 0 → v of weight 0 are listed for v = n − 1 down to 1;
  - a chain v − 1 → v of weight −[1, W] is listed in reverse;
  - n filler arcs of weight about n·W never relax.

  The FIFO queue re-enqueues every vertex downstream once per chain step, about n^2 / 2 pops; the fallback never fires because each vertex is enqueued fewer than n times. `bellmanFord` advances one chain step per pass. Tripling n slowed `spfa` 14.3× and `bellmanFord` 9.4×, but the scaling only 3.6×.

On the label-correcting worst case, `negativeSsspScaling` therefore wins by 9 to 37 times, and the gap grows with n. On random and chain inputs it loses by 11 to 830 times, because those finish in a few passes.

## Sources

See [the 2026-11-03 ledger](00-sources.md#pages-fetched-on-2026-11-03-p042-gr04): cp-algorithms (D'Esopo–Pape), maspypy (`top2_dijkstra`), suisen, NetworkX (A*, bidirectional Dijkstra), Koosaga's file listing for Goldberg scaling. The Goldberg refine above is derived here, not taken from the paper or from code. No catalog code was copied.

## Limits and handoffs

- `negativeSsspScaling` pays off only on inputs adversarial to label correcting: it is 9 to 37 times faster there, and 11 to 830 times slower on random and chain inputs (see Benchmarks). It costs a full O(n + m) round per refine step for each of up to 63 phases.
- `bidirectionalDijkstra` and `aStar` allocate O(n + m) per call; a reusable query structure with time-stamped labels is outside this row.
- k-th shortest walks and paths for k > 2 belong to rows 32 and 59.
- `secondShortestPath` is checked only where its oracle fits: n ≤ 5 by walk enumeration, or (2n − 1)·C·(n + m) ≤ 4·10^6 by the capped walk-count DP. The large graphs (n = 50 to 300) and full-range weights above n = 5 do not reach it.
- The `k > need` exit of `refine` is never taken in the tests: a counting copy of the header recorded 0 hits in 27160 full-mode rounds. The exit is only a safeguard. A `false` result always reruns `bellmanFord`, and `true` requires k = 0.
- The NDEBUG bounded returns follow from the code: each nonnegative or bounded-weight engine checks its weights once at entry and returns before its main loop. Optimized builds receive only in-domain inputs (`05-testing.md`, Memory safety), so no test exercises these returns.
- `dialDijkstra` allocates C + 1 bucket vectors (24 bytes each), so C near 2^31 needs about 48 GiB and Θ(n·C) bucket steps; it pays off only for small C. The tester covers C up to the largest value that fits its memory cap, and the domain bound by the INT_MAX probe.

## History

- 2026-11-05: `/finish GR70` mutation record (whole file, 500 of 1330 mutants): detail-engine and stale-entry fixtures and four probes closed 27 survivors, and the review's `refine` star case one more; 63 recorded with arguments.
- 2026-11-03: P042 GR04 rewrite from the pinned legacy excerpt; `top2Dijkstra` added; tester found `aStar` with negative h(t) and the scaling's false negative cycles, both fixed; review: 11 findings fixed (NDEBUG bounded returns, complexity lines, refine reuses its round graph); full suite passed on g++ and g++-14, stress seed 7.
- 2026-11-08: `/merge-pr 4` rebase on master: row set to `partial`, since the P254 omission sweep of 2026-11-08 added operations after this work (`missing:` in the inventory row); no code or test change.
- 2026-11-08: `/merge-pr 4` review fixes: `G = Graph` defaults, correctly sized `NDEBUG` results for `secondShortestPath` and `top2Dijkstra`, survivors closed by fixtures.
- 2026-11-09: `/merge-pr 4` second review round: comments of `noTopTwo` and `topTwo` only in the header, tie fixture labelled white-box, mutation campaign recorded above (0 mutants on the changed lines); `/merge-pr` gates passed on d4c47fc.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
