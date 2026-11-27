# 020-min_cost_flow.hpp — Evidence

Owned by package P045 / GR09. The header implements `MinCostFlow<Cap, Cost>`, successive shortest paths with Dijkstra on reduced costs. Potentials are kept across calls, so `flow` and `slope` resume from any residual state with any source and sink. Negative costs get an automatic DAG-order or Bellman–Ford initialization, convex piecewise-linear edges cost one arc pair each, and `potentials` returns the dual certificate. Contest profile: no SIMD and no Barrett or Montgomery reduction, so no `Compact` twins.

Row 20 is `partial`: `maxCostFlow` joined it under `missing:` (user decision 2026-11-22; source CF 884F), and P045 implements it. Everything below covers the operations the header has.

## Contracts

### MinCostFlow<Cap, Cost>

- **Types and graph.** `Cap` and `Cost` are signed integer types (`int`, `lng`, `lll`; `static_assert`). Vertices are `0..n-1` and edges are directed. Loops, parallel and antiparallel edges are allowed, and a loop never carries flow. `LIM = max(Cost) / 16`.
- **`addEdge(u, v, cap, cost)`.** Returns sequential edge IDs from 0, asserts `cap >= 0` and `|cost| <= LIM`, and stores one arc pair: a forward arc with residual `cap` and a reverse arc with residual 0 and cost `-cost`. Edges may be added at any time, including after a flow. A positive-capacity arc whose reduced cost `cost + p[u] - p[v]` is negative marks the potentials stale, and the next `flow`, `slope` or `potentials` recomputes them.
- **`addConvexEdge(u, v, pieces)`.** `pieces` lists `(length, unit cost)` pairs with every length positive, unit costs nondecreasing and in `[-LIM, LIM]`, and lengths summing to at most `MAX` (asserted). Sending `x` units costs the sum of the cheapest `x` unit costs, the convex piecewise-linear function with those slopes. The edge takes one ID and one arc pair: the forward arc exposes the remaining length of the first piece that is not full, at that piece's cost, and the reverse arc exposes the flow in the piece holding the last unit, at minus its cost. After every augmentation `refresh` re-reads the edge's flow `x` from its forward arc and re-exposes the pieces around `x`; one push stays inside one exposed piece, so `x` crosses at most one piece boundary per augmentation. An empty list adds a zero-capacity edge of cost 0.
- **`edge(i)`, `edges()`.** `edge(i)` returns `{from, to, cap, flow, cost}` with `0 <= flow <= cap`. For a linear edge `cost` is its unit cost. For a convex edge `cap` is the total length and `cost` the unit cost of the piece holding the last unit of flow (the first piece's at flow 0), so a linear edge reads as a one-piece convex edge. `edges()` returns every `edge(i)` in ID order.
- **`flow(s, t, limit = MAX, below = INF)`.** Requires `s != t` and `limit >= 0`. It augments along successive cheapest residual `s -> t` paths and returns `{added flow, added cost}`, the cost in `lll`. It stops when the added flow reaches `limit`, when `t` becomes unreachable, or when the next path's unit cost is `>= below`. Each call adds the minimum-cost increment of that amount to the current flow, and every unit it adds costs strictly less than `below`. So `flow(s, t)` is the min-cost maximum flow, `flow(s, t, k)` the min-cost flow of value `min(k, max)` (check `.first == k` for a target), and `flow(s, t, MAX, 0)` the min-cost flow of any amount. `flow(s, t, MAX, P)` maximizes `P * flow - cost` with the fewest units among maximizers.
- **Repeated calls.** Calls may repeat with the same or different `(s, t)` pairs and interleave with `addEdge`. The current flow is always a minimum-cost flow for its own divergence (inflow minus outflow per vertex), so a sequence of calls from the zero flow ends at a min-cost flow for the summed supplies. This is why repeated calls are allowed, which ACL's local duals forbid.
- **`slope(s, t, limit = MAX, below = INF)`.** Same contract and stopping rule as `flow`, which returns its last element. It returns the breakpoints `(flow, cost)` of the added-cost function, starting at `(0, 0)`; consecutive points delimit segments of strictly increasing unit cost, with collinear segments merged. Every integral amount between two breakpoints is attained at the interpolated cost. When `limit` is reached inside a segment, the last point is `(limit, cost)`.
- **`potentials()`.** Returns `p` with `cost(a) + p[u] - p[v] >= 0` for every arc `u -> v` with positive residual capacity, recomputing it first when stale. This certifies that the current flow is optimal for its divergence (complementary slackness), and `p[t] - p[s]` bounds the cost of every residual `s -> t` path from below. It costs O(n), or O(n * m) after a recomputation, which asserts the magnitude domain below as `flow` does.
- **Negative costs.** The residual network must never contain a negative-cost cycle of positive-capacity arcs. Before any flow this means the input has none; `flow`, `slope` and edges with nonnegative reduced cost preserve it; an edge added after a flow must not close one. A zero-capacity arc never counts. A violation is caught by the assertion in `initPotentials`.
- **Magnitude domain.** Let `B = min(sum of |cost|, (n - 1) * max |cost|)` over the non-loop edges of positive capacity, a convex edge counting its largest absolute unit cost; loops and zero-capacity edges lie on no residual path and are excluded. `B` bounds every simple residual path. `flow`, `slope` and a recomputing `potentials` assert `B <= LIM`. `flow` and `slope` also assert that the sum of capacity times largest absolute unit cost over the same edges is below `2^126`, so every running total and difference of totals fits `lll`. Both checks are one O(m) scan per call, inside the stated bound. Use `Cost = lll` for larger costs.
- **Under `NDEBUG`** a violated precondition never overflows or loops:
  - Capacities are clamped at 0, costs and convex unit costs into `[-LIM, LIM]` at insertion, and convex lengths into `[0, MAX - so far]`; non-convex pieces become convex.
  - Bellman–Ford values are clamped at `-LIM`, which valid inputs never reach.
  - A remaining negative cycle leaves the potentials stale, and `flow` and `slope` then return `(0, 0)` without touching the network; so does a failed `bounded()`.
  - `s == t` returns `{limit, 0}`, or `{0, 0}` when `below <= 0`, without changing the network; this follows from the loop, and the `NDEBUG` probe below checks the first case.
- **Workspace and threads.** The members `dist`, `vis`, `pe`, `low` and `heap` are reused workspace; `h` holds the potentials, `fresh` records that they are valid, and `sink` is the sink they are anchored to. The struct is single-threaded.

### Algorithm and Correctness

- **Successive shortest paths.** The invariant is that `h` is a feasible potential for the whole residual network. Each phase runs Dijkstra on the reduced costs `cost + h[u] - h[v] >= 0` from `s` and stops once `t` is settled.
  - Every settled vertex `v` gets `h[v] += dist[v] - dist[t]`; unsettled vertices keep `h`. This is the pointwise `h + min(dist, dist[t]) - dist[t]`, which stays feasible on every arc, including arcs between settled and unsettled vertices, and leaves `h[t]` unchanged.
  - The path's true unit cost is then `h[t] - h[s]`. The path arcs become tight (reduced cost 0), so their reverse arcs are tight too and the invariant survives the push.
  - Each augmentation adds at least one unit, so the number of phases is at most the added flow. With `limit` and `below` stops, the units added are the cheapest ones (standard SSP optimality), which gives the `slope` breakpoints exactly.
- **Zero reduced-cost stack.** A head reached by a zero reduced-cost arc has the current minimum distance, so it goes on a stack popped before the heap (ACL's `que_min`). Skipping settled heads in relaxation keeps parent pointers on a tree even under invalid potentials (`NDEBUG`), so the augmenting walk always terminates.
- **Initial potentials (`initPotentials`).** Bellman–Ford from a zero-cost virtual source over positive-capacity arcs, processing vertices in Kahn order: the topological prefix first, then the cyclic remainder by index. Gauss–Seidel updates in that order settle an acyclic residual network in one round plus one confirming round, O(n + m); otherwise at most `n - 1` updating rounds are needed. A round still updating at round `n` means a negative cycle. The result `p(v) = min_u dist(u, v)` lies in `[-B, 0]`.
- **Re-anchoring.** `h[t]` never changes while the sink stays `t`, and settled vertices are set to `h[t] + d(v) - d(t)` with true residual distances `|d| <= B`, so `h` stays within `[-3B, 2B]` as long as `h[t]` came from a canonical `p in [-B, 0]`. A call with a different sink first re-canonicalizes with one Dijkstra seeded at every `v` with `-h[v]`, after which `h += dist` gives `min_u dist(u, v)`; arbitrary seeds are valid because the arc weights are nonnegative. Without this step, alternating `(0, 2)` and `(1, 3)` calls on `0->3 (0), 0->2 (5), 1->2 (0), 1->3 (5)` lower both sinks' potentials by 10 per round; the drift test keeps that network within `[-3B, 2B]` for 300 calls.
- **Overflow argument.** With `B <= LIM = max(Cost)/16`: `|h| <= 3B`, reduced distances are at most `6B`, every relaxed value `dist[u] + h[u] + cost - h[v]` is at most `12B`, canonical seeds at most `3B` and unit costs at most `5B`, so all fit `Cost`. Costs accumulate in `lll` as `f * unit`; every partial sum is the cost of an actual flow, bounded by `sum cap * |cost| < 2^126`.
- **Convex edges.** By convexity the exposed forward piece is the cheapest unfilled one and the exposed reverse piece the most expensive filled one, so the residual arcs are exactly the cheapest available arcs of the parallel-arc expansion. A push along a tight arc that fills a piece exposes the next piece at a higher or equal cost, so the reduced cost stays `>= 0` and the new reverse arc stays tight; emptying a piece backwards exposes the previous piece, whose reverse cost is lower or equal. The potentials therefore stay feasible with one arc pair per edge, and each phase costs O(n + m) arcs rather than O(n + total pieces). `refresh` runs once per augmentation over the convex edges (O(c) with c <= m).
- **Removed `pathReconstruction`.** Load `edges()` into a `MaxFlow<lll>` (`addEdge(from, to, cap)`, then `changeEdge(id, cap, flow)`) and call `pathDecomposition(s, t)`. Under the no-negative-cycle contract every cycle of positive flow in a min-cost flow has cost `>= 0` (input arcs) and `<= 0` (optimality), so dropping cycles keeps the whole cost on the paths. This holds for linear edges and a single `(s, t)` call sequence from the zero flow; the tester checks it on every eligible network.

### Legacy Mapping

- **`OLD/Team Notebook/src/graph/old_mcmf.cpp` (`flow_network`)**: `long double` capacities and costs, SPFA on potential-reduced costs, optional `bellman_ford()`.
  - `flow_network(n, s, t)` → `MinCostFlow<lng, lng>(n)`, with `s` and `t` passed to each call. Capacities and costs are integers; scale fractional inputs.
  - `add_edge(u, v, cap, cost)` → `addEdge(u, v, cap, cost)`.
  - `max_flow(do_bellman_ford)`, which returned `{cost, flow}` → `flow(s, t)`, which returns `{flow, cost}`; negative-cost initialization is automatic.
  - `bellman_ford()` → automatic (`initPotentials`).
  - `get_flow(u, v)` → the net flow: the sum of `edge(i).flow` over the IDs `addEdge` returned for `u -> v`, minus the same sum for `v -> u`. The legacy index under the key `{u, v}` held the `u -> v` edges and the reverse arcs of the `v -> u` edges, whose flow is negated.
  - Its SPFA stopped on a vertex visited `n` times (a negative cycle) and returned the partial result; that is now an asserted precondition.
  - The trailing note about updating the back edge's cost has no counterpart: there is no cost update, so build a new network.
- **Matrix copies** (`MinCostMaxFlow<T, C>` in `OLD/Team Notebook/src/algsbetter.cpp:2040-2118` and `algs.cpp:1770-1807`, `MaxFlowMinCut<T, C>` in `test.cpp:670-748`):
  - `X(n, E).solve(s, t, k)` → one `addEdge` per entry of `E`, then `flow(s, t, k)`.
  - The legacy versions kept O(n^2) capacity and cost matrices, so parallel edges overwrote each other and antiparallel edges shared one cell. Each listed edge now stays separate, which changes the result exactly when `E` had such pairs.
  - Plain SPFA without potentials → Dijkstra with potentials.
- **Row names removed 2026-10-13:** `minCostMaxFlow` → `flow(s, t)`; `bellmanFordPotentials` → automatic inside `flow`, `slope` and `potentials` (`initPotentials` is the public helper); `pathReconstruction` → the `MaxFlow::pathDecomposition` recipe above.
- P276 rename, 2026-11-10: `getEdge` → `edge`.
- **Aliases elsewhere:** ACL `mcf_graph::add_edge`, `flow`, `slope`, `get_edge`, `edges`; KACTL `MCMF::maxflow` with `setpi`; maspypy `get_potentials` → `potentials`; suisen `min_cost_arbitrary_flow` → `flow(s, t, MAX, 0)`; ei1333 `min_cost_flow(s, t, f)` returning -1 → `flow(s, t, f).first < f`; max-gain flow → negated costs today, the row's `maxCostFlow` (missing) once P045 adds it.

Renamed aliases (P276, 2026-11-10): `getEdge`.

Search aliases (P263, grab --find): 费用流, 网络流简介, 最小费用最大流, 最小费用流, 凸费用流, 原始对偶, 最小花费最大流, 最小成本流, zkw费用流.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/020-min_cost_flow_tester.py` uses the shared contest runner with three builds, all with `-Wall -Wextra -Wconversion -Werror`: optimized `-O2 -DNDEBUG`; checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress); ASan/UBSan `-O1 -g` in full and stress.

libstdc++ debug mode re-validates the whole heap on every `push_heap`/`pop_heap`, which makes each Dijkstra O(m^2) there, so in full and stress the checked build shrinks the heap-heavy sections. Quick mode uses its own sizes in every build.

| Section | Checked build | Other two builds (full / stress) |
|---|---|---|
| reverse-labelled DAG chain | n = 20,000 | 100,000 / 300,000 |
| cyclic network | n = 300 | 5,000 / 15,000 |
| convex comparison | n = 80 | 400 / 1,200 |
| assignment | k = 25 | 60 / 120 |

Every library call is compared with an oracle and then certified: `edge` and `edges` endpoints, capacity, `0 <= flow <= cap` and `cost` equal to the unit cost of the last unit; conservation to the tracked divergence; the sum of the edges' convex costs equal to the sum of the returned costs; `potentials()` within `[-3B, 2B]` with complementary slackness on every residual arc of the model, computed from the model's own pieces; and a legitimate stop (limit reached, `t` unreachable in the model residual, or `p[t] - p[s] >= below`).

| Operation | Test | Oracle |
|---|---|---|
| `addEdge`, `flow`, `slope` (limit, below, resumed, other `(s, t)`) | 3,000 full / 12,000 stress random networks, n ≤ 5, m ≤ 6 and up to 3 more edges added between 1–4 calls, caps ≤ 3, convex edges of 1–3 pieces, five `(Cap, Cost)` combinations | Exhaustive enumeration of every integral edge-flow vector: min cost per divergence vector, giving the exact cost curve from any current state; breakpoint vectors compared exactly |
| same, medium | 300 / 1,000 networks, n ≤ 24, m ≤ 96, caps ≤ 8, convex up to 4 pieces of length ≤ 4, 1–5 calls | Unit-step Bellman–Ford successive shortest paths on the model residual (convex pieces as unit marginals) |
| `flow`, `slope` on every small shape | All 46,656 networks on 3 vertices (quick: 4,096) whose 6 ordered pairs take cap {0, 1, 2} and cost {-1, 2}; 31,744 without negative cycles; `flow(0, 2)` or `slope(0, 2)`, then a repeated call adding nothing | Exhaustive edge-flow enumeration conserving at vertex 1 (oracle validated: zero flow costs 0) |
| `Cap`, `Cost` = `int`, `lng`, `lll` | Mixed combinations in every random corpus; caps scaled by K ≤ 2^62 and costs by L ≤ 2^54 (600 / 3,000) | Scaled exhaustive curve: breakpoint `(F, C)` maps to `(K F, K L C)` (metamorphic) |
| Magnitude domain | Costs `LIM` and `-LIM` at cap `MAX` for `lng`/`lng`, `int`/`int`, `int`/`lng` (cap 2 for `lll`); 17-vertex chains whose costs sum to `LIM`, DAG with negative and cyclic with positive costs, plus a loop of cost `LIM` and a zero-capacity edge of cost `-LIM` that stay outside `B`, 8 calls with alternating sources and sinks, under UBSan | Exact product; certificate |
| Negative costs | All three generation families (uniform with rejection of negative cycles, potential-shifted nonnegative bases, DAG order); zero-capacity arcs closing negative cycles allowed; negative-cost edges added after flow; reverse-labelled DAG chain with negative skips, n = 2,000 / 100,000 / 300,000 (index-order Bellman–Ford would be quadratic: its mutant times out); cyclic potential-shifted network n = 100 / 5,000 / 15,000 | Oracles above; certificate on the large ones |
| `addConvexEdge` | Small and medium corpora; 30–1,200-vertex networks with 60% convex edges of up to 30 pieces against the same network expanded into parallel `addEdge` arcs | Exhaustive / unit oracle; identical breakpoints and totals plus certificate for the large comparison |
| `potentials` | After every call above | Complementary slackness computed independently from the model; magnitude bound |
| Re-anchoring | Drift network, 300 alternating calls, with and without pushing | `potentials` within `[-3B, 2B]` (the mutant without re-anchoring fails it) |
| `below` and the any-amount optimum | Random `below` in every corpus; `below` equal to the path cost adds nothing; `flow(s, t, MAX, 0)` on sparse bipartite networks | Oracles; `linearSumAssignment(..., maxCardinality = false).cost` |
| Assignment workloads | Dense k × (k + 0..5) bipartite networks, costs ±1e9, min and max; sparse quarter-density bipartite networks, `slope` | `hungarian(a).cost`, `-hungarian(a, true).cost`; `linearSumAssignment(...).curve[j]` equal to the slope interpolated at every j |
| `pathReconstruction` recipe | First call on every linear small network and every family network | `MaxFlow<lll>::pathDecomposition` paths' summed cost equals the returned cost |
| Degenerate | 0 vertices, no edges, empty piece list, limit 0, `below` at and above the path cost, resume to maximum then nothing, loop unused, reverse residual flow from `t` to `s` | Hand values |
| Preconditions | 24 probes in the checked build: size, ranges, negative capacity, cost above and below `LIM`, convex range, length, order, overflow, cost, `edge` range, `s == t` and negative limit for `flow` and `slope`, a negative cycle, a negative loop, a negative residual cycle after flow, `potentials` on a negative cycle and on a path above `LIM`, a path above `LIM` for `flow`, a single product above 2^126 and two products summing to 2^126 | Expected assertion abort |

Planted mutants (2026-10-13, after the review fixes, scratch copies, not part of the suite): 22 mutants, all killed. Quick mode killed 19: the potential update without the `-dist[t]` shift or applied to unsettled vertices; the reduced instead of the true unit cost; the bottleneck read from the reverse arc (timeout); a convex piece that never advances or never steps back, the reverse arc always on the current piece, and every piece at the first unit cost; a negative reduced cost never invalidating the potentials; no re-anchoring, or canonical seeds at 0; `below` inclusive; collinear segments not merged; convex `edge` read as linear; one Bellman–Ford round, or the Bellman–Ford clamp above the domain; the `NDEBUG` early return taken on success; loops and zero-capacity edges counted in `B`; the zero-cost stack pushing the wrong vertex. The other three: Bellman–Ford in index order instead of Kahn order (timeout in full mode); `bounded()` always true (the `path-bound`, `total-bound` and `total-sum-bound` probes); the `2^126` sum check dropped (the `total-sum-bound` probe).

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, CPython 3.14, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). The 2026-11-10 library-wide runs used seed 42, beside the other suites, with the build cache emptied before each cold run. The 2026-10-14 runs followed the rebase onto master; the benchmark, review and `NDEBUG` probe of 2026-10-13 ran on the same header.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 1181 MB; optimized 0.1 s compile + 1.4 s run, checked 44.2 + 10.3 s, ASan-UBSan 57.5 + 25.9 s; MEMORY peak 1643 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 1215 MB; optimized 0.2 + 2.3 s, checked 66.1 + 11.9 s, ASan-UBSan 78.7 + 32.1 s; MEMORY peak 1617 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 60.5 + 0.2 s, checked 41.8 + 3.9 s; MEMORY peak 1158 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 1197 MB; optimized 0.1 + 1.9 s, checked 20.6 + 6.3 s, ASan-UBSan 25.3 + 25.5 s; MEMORY peak 905 MB |
| 2026-11-10 | P276 rename: tester full on g++ and g++-14, stress, `05-extraction.py` | PASS (counts not recorded) |
| 2026-10-14 | `020-min_cost_flow_tester.py --mode full --seed 20261013` | PASS 3 configurations, 41,380 cases; 5,514,597 checks optimized and ASan/UBSan, 2,653,157 checked; 24 probes; MEMORY peak 769 MB (pre-flight quick 707 MB) |
| 2026-10-14 | `CXX=g++-14 020-min_cost_flow_tester.py --mode full --seed 20261013` | PASS, same counts; MEMORY peak 782 MB (pre-flight quick 720 MB) |
| 2026-10-14 | `020-min_cost_flow_tester.py --mode stress --seed 7` | PASS 3 configurations, 68,345 cases; 13,906,568 checks optimized and ASan/UBSan, 4,115,298 checked; 24 probes; MEMORY peak 888 MB |
| 2026-10-13 | `020-min_cost_flow_benchmark.py` | PASS, every result checked; MEMORY peak 341 MB |
| 2026-10-14 | `02-integration.py` | PASS: 138 standalone and aggregate headers (including `020-min_cost_flow.hpp` alone and `99-all.hpp`), scalar and available-AVX2 multi-TU, workspace; MEMORY peak 647 MB |
| 2026-10-14 | `03-consistency.py`, with `--braces` on the header, tester and benchmark | No errors |
| 2026-10-13 | `NDEBUG` boundedness probe (ad hoc, `-O1 -DNDEBUG -fsanitize=address,undefined`, 512 MB cap) | Cases below; no sanitizer report; peak 27 MB |
| 2026-10-13 | `@reviewer` (independent: own unit-step Bellman–Ford SSP oracle, 3,000 rounds under ASan/UBSan and 30,000 at `-O2` with assertions, n ≤ 12 and up to 40 calls each; 20,000 × 300-call `NDEBUG` zero-length-piece fuzz; reran quick, full and consistency) | No wrong answer on valid inputs; 6 findings fixed, 4 suggestions adopted; by the user's decision `flow` stays beside `slope` (Keeps in `duplicates.md`) |

| `NDEBUG` probe case | Result |
|---|---|
| `flow(1, 1, 7)` | `{7, 0}`, network unchanged |
| `flow(0, 1, -1)` | `{0, 0}` |
| 3-vertex negative cycle | `{0, 0}`; potentials returned |
| 2,000-vertex cycle of cost `-LIM` per arc and cap `MAX` | `{0, 0}` |
| costs `INT64_MIN`/`INT64_MAX` (clamped to ±LIM, zero-cost cycle) | flow 3 |
| path above `LIM` | `{0, 0}` |
| `lll` total above 2^126 | flow 0 |
| negative capacity | `{0, 0}` |
| convex pieces with lengths 0, -3, `MAX` overflow and decreasing costs, called twice | `{0, 0}` |
| negative residual cycle after flow | `{0, 0}` |
| negative loop | `{0, 0}` |
| `lll` capacity `-2^126` beside two `2^120` edges | flow 0 |
| `potentials()` with `B = 2 LIM` | returns clamped values |
| valid: loop of cost `LIM` and zero-capacity edge of cost `-LIM` beside a `±LIM/2` path | `{2, 0}` |

```bash
python3 '96-Local Testing/04-Graphs/020-min_cost_flow_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/04-Graphs/020-min_cost_flow_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/04-Graphs/020-min_cost_flow_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/020-min_cost_flow_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/020-min_cost_flow_benchmark.py>) writes `020-min_cost_flow_benchmark.json`, which git ignores. Conditions (2026-10-13): i9-11900H, shared, load average about 2 on 16 threads; GCC 16.2.1, `-O2 -DNDEBUG`, seed 20261013; one checked warm-up run, then 5 interleaved samples per implementation; network construction included, input generation excluded; every implementation returns the same `(flow, cost)`. The references are written in the driver: an ACL-style one with the same algorithm (one path per Dijkstra stopping at `t`, duals moved for settled vertices, the zero reduced-cost stack), and two variants that drop that stack or replace the heap by an O(n^2) scan per phase (OI Wiki's dense Dijkstra). Memory is O(n + m) arcs; the heap holds at most m entries.

Medians in milliseconds:

| Workload | n | m | `MinCostFlow` | ACL-style | no zero stack | dense scan |
|---|---|---|---|---|---|---|
| small random, caps [1,100], costs [0,1000] | 50 | 300 | 0.137 | 0.134 | 0.207 | 0.261 |
| dense random p = 1/2, caps [1,10] | 300 | 44,682 | 146 | 144 | 188 | 251 |
| bipartite assignment k = 300, costs [0,1e6] | 602 | 90,600 | 51.7 | 50.0 | 52.9 | 107 |
| 100×100 grid, both directions | 10,000 | 39,600 | 8.26 | 8.14 | 12.7 | — |
| sparse random | 5,000 | 50,000 | 55.6 | 53.1 | 84.9 | — |

| Workload | n | m | Result | Time | Compared with |
|---|---|---|---|---|---|
| x^2 costs: 1,000 edges of 50 unit pieces (1, 3, 5, ...) | 200 | 1,000 | flow 150, cost 16,930 | `addConvexEdge` 3.85 | 50,000 parallel `addEdge` arcs 16.9 |
| negative DAG: chain cap 10 plus 200,000 unit intervals of cost [-1e6, -1] | 200,000 | 399,999 | — | in-order labels 527 | reverse labels 517 |

- `MinCostFlow` tracks the ACL-style reference within +1% to +5% on every workload.
- Two inner-loop choices were measured in six interleaved rounds on the dense, bipartite and sparse workloads: hoisting `dist[u] + h[u]` out of the arc loop, and reading `dist`, `h`, `vis` and `pe` through raw pointers (member vectors otherwise reload their data pointers after every heap push). Without them the library was 5–11% slower than the reference on the bipartite workload; with them it was 5% faster there and within ±4% elsewhere. Moving the workspace into locals of `slope` was not faster.
- The zero reduced-cost stack saves 2–35% (34% sparse, 35% grid, 22% dense, 2% bipartite) and is kept. The dense O(n^2) scan is 1.7–2.1× slower than the heap on every workload where it ran, including the half-dense graph and the 50-vertex network, so it is not adopted (notes).
- One convex edge of p pieces costs one arc pair, so the x^2 workload runs 4.4× faster than the parallel-arc expansion, with identical results.
- Kahn order makes the negative-cost initialization independent of labelling; index-order Bellman–Ford needs about n rounds on the reverse-labelled DAG, where its mutant timed out.
- These are shared-machine observations, not universal speed claims.

## Sources

The sweep is in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-13-p045-min-cost-flow) and the candidates left out, with reasons, in [00-notes.md](00-notes.md#p045-min-cost-flow-omissions). The algorithms follow ACL `mcf_graph` (API shape, Dijkstra stopping at `t` with duals moved for settled vertices, the zero reduced-cost stack, slope merging); KACTL `MCMF::setpi` (Bellman–Ford initial potentials); maspypy (`DAG=true`), hitonanode `mincostflow_nonegativeloop` and suisen's DAG initializer (linear-time initialization on a DAG); suisen (member potentials across calls, the any-amount flow); and maspypy `get_potentials` (the dual export). Convex edges follow the row's piecewise-linear wording, since no catalog has a convex-edge API. `maxCostFlow` comes from CF 884F (user decision 2026-11-22). The code was written independently; no implementation was copied.

## Limits and Handoffs

- The row is `partial`: `maxCostFlow` is missing (user decision 2026-11-22; source CF 884F) and P045 implements it.
- Elsewhere: supplies and demands, lower bounds and circulations, row 30; negative cycles and min-cost b-flow (cost scaling, network simplex), rows 65 and 66; assignment, row 23; min-cost-flow duals for difference constraints, row 44; Chinese postman, row 69.
- Not provided, with reasons in the notes: changing an edge's capacity or cost after a flow, convex costs given by a callable over large capacities, and floating-point capacities or costs.
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## History

- 2026-10-13: P045 implementation; independent review, 6 findings fixed; planted mutants (22 of 22 killed), benchmark and `NDEBUG` probe.
- 2026-10-14: verified (P045) after the rebase onto master; full (seed 20261013) passed on g++ and g++-14, stress (seed 7), integration and consistency passed.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); `zkw费用流` names a related method for the same problem and stays a search synonym (user decision 2026-11-07); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-10: P276 rename (`getEdge` → `edge`); full on g++ and g++-14, stress and extraction passed.
- 2026-11-22: row set to `partial`: `maxCostFlow` added under `missing:` (user decision 2026-11-22; source CF 884F); P045 reopens to implement it. No code or test change.
