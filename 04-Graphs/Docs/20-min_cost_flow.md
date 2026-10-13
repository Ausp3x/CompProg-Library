# 20-min_cost_flow.hpp — evidence

Owned by package P045 / GR09. The header implements `MinCostFlow<Cap, Cost>`, successive shortest paths with Dijkstra on reduced costs. Potentials are kept across calls, so `flow` and `slope` resume from any residual state and accept any source and sink. Negative costs are handled by an automatic DAG-order or Bellman–Ford initialisation. Convex piecewise-linear edges cost one arc pair each. `potentials` returns the dual certificate. Contest profile: no SIMD, no Barrett/Montgomery, so no `Compact` twins.

## Contracts

### MinCostFlow<Cap, Cost>

- **Types and graph.** `Cap` and `Cost` are signed integer types (`int`, `lng`, `lll`; `static_assert`). Vertices are `0..n-1`; edges are directed. Loops, parallel and antiparallel edges are allowed, and a loop never carries flow. `LIM = max(Cost) / 16`.
- **`addEdge(u, v, cap, cost)`.** Returns sequential edge IDs from 0. It asserts `cap >= 0` and `|cost| <= LIM`, and stores one arc pair: a forward arc with residual `cap` and a reverse arc with residual 0 and cost `-cost`. Edges may be added at any time, including after a flow. A positive-capacity arc whose reduced cost `cost + p[u] - p[v]` is negative marks the potentials stale, and the next `flow`, `slope` or `potentials` recomputes them.
- **`addConvexEdge(u, v, pieces)`.** `pieces` is a list of `(length, unit cost)` with every length positive, unit costs nondecreasing and in `[-LIM, LIM]`, and the lengths summing to at most `MAX` (asserted).
  - Sending `x` units costs the sum of the cheapest `x` unit costs, which is the convex piecewise-linear function with those slopes.
  - The edge takes one ID and one arc pair. The forward arc exposes the remaining length of the first piece that is not full, at that piece's cost. The reverse arc exposes the flow in the piece holding the last unit, at minus its cost.
  - After every augmentation, `refresh` re-reads the edge's flow `x` from its forward arc and re-exposes the pieces around `x`. One push stays inside one exposed piece, so `x` moves by at most one piece boundary per augmentation.
  - An empty list adds a zero-capacity edge of cost 0.
- **`getEdge(i)`, `edges()`.** `getEdge(i)` returns `{from, to, cap, flow, cost}` with `0 <= flow <= cap`. For a linear edge `cost` is its unit cost. For a convex edge `cap` is the total length and `cost` is the unit cost of the piece holding the last unit of flow (the first piece's at flow 0), so a linear edge reads as a one-piece convex edge. `edges()` returns every `getEdge(i)` in ID order.
- **`flow(s, t, limit = MAX, below = INF)`.** Requires `s != t` and `limit >= 0`. It augments along successive cheapest residual `s -> t` paths and returns `{added flow, added cost}`, the cost in `lll`. It stops when:
  - the added flow reaches `limit`,
  - `t` becomes unreachable, or
  - the next path's unit cost is `>= below`.

  Each call adds the minimum-cost increment of that amount to the current flow, and the units it adds cost strictly less than `below`. So `flow(s, t)` is the min-cost maximum flow, `flow(s, t, k)` the min-cost flow of value `min(k, max)` (check `.first == k` for a target), and `flow(s, t, MAX, 0)` the min-cost flow of any amount. `flow(s, t, MAX, P)` maximises `P * flow - cost` with the fewest units among maximisers.
- **Repeated calls.** Calls may repeat with the same or different `(s, t)` pairs and interleave with `addEdge`. The current flow is always a minimum-cost flow for its own divergence (inflow minus outflow per vertex), so a sequence of calls from the zero flow ends at a min-cost flow for the summed supplies. That is why repeated calls are allowed, which ACL's local duals forbid.
- **`slope(s, t, limit = MAX, below = INF)`.** Has the same contract and stopping rule as `flow`, which returns its last element. It returns the breakpoints `(flow, cost)` of the added-cost function: it starts at `(0, 0)`, and consecutive points delimit segments of strictly increasing unit cost, with collinear segments merged. Every integral amount between two breakpoints is attained at the interpolated cost. With `limit` reached inside a segment, the last point is `(limit, cost)`.
- **`potentials()`.** Returns `p` with `cost(a) + p[u] - p[v] >= 0` for every arc `u -> v` with positive residual capacity, recomputing it first when stale. This certifies that the current flow is optimal for its divergence (complementary slackness), and `p[t] - p[s]` bounds the cost of every residual `s -> t` path from below. It costs O(n), or O(n * m) after a recomputation; a recomputation asserts the magnitude domain below, as `flow` does.
- **Negative costs.** The residual network must never contain a negative-cost cycle of positive-capacity arcs. Before any flow this means the input has none; `flow`, `slope` and edges with nonnegative reduced cost preserve it; an edge added after a flow must not close one. A zero-capacity arc never counts. A violation is caught by the assertion in `initPotentials`.
- **Magnitude domain.** Let `B = min(sum of |cost|, (n - 1) * max |cost|)` over the non-loop edges of positive capacity, where a convex edge counts its largest absolute unit cost. Loops and zero-capacity edges lie on no residual path and never carry flow, so they are excluded. `B` bounds every simple residual path. `flow`, `slope` and a recomputing `potentials` assert `B <= LIM`. `flow` and `slope` also assert that the sum of capacity times largest absolute unit cost over the same edges is below `2^126`, so every running total and every difference of totals fits `lll`. Both checks are one O(m) scan per call, inside the stated bound. Use `Cost = lll` for larger costs.
- **Under `NDEBUG`.** A violated precondition never overflows or loops:
  - Capacities are clamped at 0, costs and convex unit costs into `[-LIM, LIM]` at insertion, and convex lengths into `[0, MAX - so far]`. Non-convex pieces become convex.
  - Bellman–Ford values are clamped at `-LIM`, which valid inputs never reach.
  - A remaining negative cycle leaves the potentials stale, and `flow`/`slope` then return `(0, 0)` without touching the network. The same happens when `bounded()` fails.
  - `s == t` returns `{limit, 0}`, or `{0, 0}` when `below <= 0`, without changing the network (it follows from the loop; the `NDEBUG` probe below checks the first case).
- **Workspace and threads.** The members `dist`, `vis`, `pe`, `low` and `heap` are reused workspace. `h` holds the potentials, `fresh` records that they are valid, and `sink` is the sink they are anchored to. The struct is single-threaded.

### Algorithm and correctness

- **Successive shortest paths.** The invariant is that `h` is a feasible potential for the whole residual network. Each phase runs Dijkstra on the reduced costs `cost + h[u] - h[v] >= 0` from `s` and stops once `t` is settled.
  - Every settled vertex `v` gets `h[v] += dist[v] - dist[t]`; unsettled vertices keep `h`. This is the pointwise `h + min(dist, dist[t]) - dist[t]`, which stays feasible on every arc, including arcs between settled and unsettled vertices, and leaves `h[t]` unchanged.
  - The path's true unit cost is then `h[t] - h[s]`. The path arcs become tight (reduced cost 0), so their reverse arcs are tight too and the invariant survives the push.
  - Each augmentation adds at least one unit, so `K` is at most the added flow. With `limit` and `below` stops, the units added are the cheapest ones (standard SSP optimality), which gives the `slope` breakpoints exactly.
- **Zero reduced-cost stack.** A head reached by a zero reduced-cost arc has the current minimum distance, so it goes on a stack popped before the heap (ACL's `que_min`). The skip of settled heads in relaxation keeps parent pointers on a tree even under invalid potentials (`NDEBUG`), so the augmenting walk always terminates.
- **Initial potentials (`initPotentials`).** Bellman–Ford from a zero-cost virtual source over positive-capacity arcs.
  - Vertices are processed in Kahn order: the topological prefix first, then the cyclic remainder by index. Gauss–Seidel updates in that order settle an acyclic residual network in one round, plus one round confirming it, so O(n + m). Otherwise it takes at most `n - 1` updating rounds.
  - A round still updating at round `n` means a negative cycle.
  - The result `p(v) = min_u dist(u, v)` lies in `[-B, 0]`.
- **Re-anchoring.** `h[t]` never changes while the sink stays `t`, and settled vertices are set to `h[t] + d(v) - d(t)` with true residual distances `|d| <= B`. Hence `h` stays within `[-3B, 2B]` as long as `h[t]` came from a canonical `p in [-B, 0]`.
  - A call with a different sink first re-canonicalises with one Dijkstra seeded at every `v` with `-h[v]`, after which `h += dist` gives `min_u dist(u, v)`. Arbitrary seeds are valid because the arc weights are nonnegative.
  - Without this step, alternating `(0, 2)` and `(1, 3)` calls on `0->3 (0), 0->2 (5), 1->2 (0), 1->3 (5)` lower both sinks' potentials by 10 per round. The drift test keeps that network within `[-3B, 2B]` for 300 calls.
- **Overflow argument.** With `B <= LIM = max(Cost)/16`:
  - `|h| <= 3B`;
  - reduced distances are at most `6B`;
  - every relaxed value `dist[u] + h[u] + cost - h[v]` is at most `12B`;
  - canonical seeds are at most `3B`;
  - unit costs are at most `5B`.

  All of these fit `Cost`. Costs accumulate in `lll` as `f * unit`; every partial sum is the cost of an actual flow, bounded by `sum cap * |cost| < 2^126`.
- **Convex edges.** By convexity the exposed forward piece is the cheapest unfilled one and the exposed reverse piece the most expensive filled one, so the residual arcs are exactly those of the parallel-arc expansion's cheapest available arcs.
  - A push along a tight arc that fills a piece exposes the next piece at a higher or equal cost, so the reduced cost stays `>= 0`. The new reverse arc stays tight.
  - Emptying a piece backwards exposes the previous piece, whose reverse cost is lower or equal, so it stays `>= 0`.
  - The potentials therefore stay feasible with only one arc pair per edge, and each phase costs O(n + m) arcs rather than O(n + total pieces). `refresh` runs once per augmentation over the convex edges (O(c) with c <= m).
- **Removed `pathReconstruction`.** Load `edges()` into a `MaxFlow<lll>` (`addEdge(from, to, cap)`, then `changeEdge(id, cap, flow)`) and call `pathDecomposition(s, t)`. Under the no-negative-cycle contract, every cycle of positive flow in a min-cost flow has cost `>= 0` (input arcs) and `<= 0` (optimality), so dropping cycles keeps the whole cost on the paths. This holds for linear edges and a single `(s, t)` call sequence from the zero flow; the tester checks it on every eligible network.

### Legacy mapping

- **`OLD/Team Notebook/src/graph/old_mcmf.cpp` (`flow_network`).** `long double` capacities and costs, SPFA on potential-reduced costs, optional `bellman_ford()`.
  - `flow_network(n, s, t)` → `MinCostFlow<lng, lng>(n)`, with `s` and `t` passed to each call. Capacities and costs are integers; scale fractional inputs.
  - `add_edge(u, v, cap, cost)` → `addEdge(u, v, cap, cost)`.
  - `max_flow(do_bellman_ford)`, which returned `{cost, flow}` → `flow(s, t)`, which returns `{flow, cost}`. Negative-cost initialisation is automatic.
  - `bellman_ford()` → automatic (`initPotentials`).
  - `get_flow(u, v)` → the net flow: the sum of `getEdge(i).flow` over the IDs `addEdge` returned for `u -> v`, minus the same sum for `v -> u`. The legacy index under the key `{u, v}` held the `u -> v` edges and the reverse arcs of the `v -> u` edges, whose flow is negated.
  - Its SPFA stopped on a vertex visited `n` times (a negative cycle) and returned the partial result; that is now an asserted precondition.
  - The trailing note about updating the back edge's cost has no counterpart: there is no cost update, so build a new network.
- **Matrix copies** (`MinCostMaxFlow<T, C>` in `OLD/Team Notebook/src/algsbetter.cpp:2040-2118` and `algs.cpp:1770-1807`, `MaxFlowMinCut<T, C>` in `test.cpp:670-748`).
  - `X(n, E).solve(s, t, k)` → one `addEdge` per entry of `E`, then `flow(s, t, k)`.
  - The legacy versions kept O(n^2) capacity and cost matrices, so parallel edges overwrote each other and antiparallel edges shared one cell. Each listed edge now stays separate, which changes the result exactly when `E` had such pairs.
  - Plain SPFA without potentials → Dijkstra with potentials.
- **Row names removed 2026-10-13** (`duplicates.md`, P045):
  - `minCostMaxFlow` → `flow(s, t)`;
  - `bellmanFordPotentials` → automatic inside `flow`, `slope` and `potentials` (`initPotentials` is the public helper);
  - `pathReconstruction` → the `MaxFlow::pathDecomposition` recipe above.
- **Aliases elsewhere:**
  - ACL `mcf_graph::add_edge`, `flow`, `slope`, `get_edge`, `edges`;
  - KACTL `MCMF::maxflow` with `setpi`;
  - maspypy `get_potentials` → `potentials`;
  - suisen `min_cost_arbitrary_flow` → `flow(s, t, MAX, 0)`;
  - ei1333 `min_cost_flow(s, t, f)` returning -1 → `flow(s, t, f).first < f`;
  - max-gain flow → negated costs.

## Feature-to-test map

`96-Local Testing/04-Graphs/20-min_cost_flow_tester.py` uses the shared graph driver with three builds, all with `-Wall -Wextra -Wconversion -Werror`:
- optimized `-O2 -DNDEBUG`;
- checked `-O0 -g -D_GLIBCXX_DEBUG`;
- ASan/UBSan `-O1 -g` in full and stress modes.

libstdc++ debug mode re-validates the whole heap on every `push_heap`/`pop_heap`, which makes each Dijkstra O(m^2) there. In full and stress modes the checked build therefore shrinks the heap-heavy sections:

| Section | Checked build | Other two builds (full / stress) |
|---|---|---|
| reverse-labelled DAG chain | n = 20,000 | 100,000 / 300,000 |
| cyclic network | n = 300 | 5,000 / 15,000 |
| convex comparison | n = 80 | 400 / 1,200 |
| assignment | k = 25 | 60 / 120 |

Quick mode uses its own sizes in every build. The optimized and ASan/UBSan builds run every section at its listed size. Failures are non-removable and print the seed, the network, the call, and the expected and actual values.

Every library call is compared with an oracle and then certified:
- **Edge reads:** `getEdge` and `edges` endpoints, capacity, `0 <= flow <= cap`, and `cost` equal to the unit cost of the last unit.
- **Conservation** to the tracked divergence.
- **Cost:** the sum of the edges' convex costs equals the sum of the returned costs.
- **Potentials:** `potentials()` within `[-3B, 2B]` and complementary slackness on every residual arc of the model, computed from the model's own pieces.
- **Legitimate stop:** limit reached, `t` unreachable in the model residual, or `p[t] - p[s] >= below`.

| Operation | Test | Oracle |
|---|---|---|
| `flow`, `slope` (limit, below, resumed, other `(s, t)`) | 3,000 full / 12,000 stress random networks, n ≤ 5, m ≤ 6 and up to 3 more edges added between 1–4 calls, caps ≤ 3, convex edges of 1–3 pieces, five `(Cap, Cost)` combinations | Exhaustive enumeration of every integral edge-flow vector: min cost per divergence vector, giving the exact cost curve from any current state; breakpoint vectors compared exactly |
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
| Preconditions | 24 probes in the checked build: size, ranges, negative capacity, cost above and below `LIM`, convex range, length, order, overflow, cost, `getEdge` range, `s == t` and negative limit for `flow` and `slope`, a negative cycle, a negative loop, a negative residual cycle after flow, `potentials` on a negative cycle and on a path above `LIM`, a path above `LIM` for `flow`, a single product above 2^126 and two products summing to 2^126 | Expected assertion abort |

Mutation check (2026-10-13, after the review fixes; scratch copies under `/tmp`; not part of the suite): 22 mutants, all killed. 19 were killed by quick mode:
- the potential update without the `-dist[t]` shift, or applied to unsettled vertices;
- the reduced instead of the true unit cost;
- the bottleneck read from the reverse arc (timeout);
- a convex piece that never advances or never steps back, the reverse arc always on the current piece, and every piece at the first unit cost;
- a negative reduced cost never invalidating the potentials;
- no re-anchoring, or canonical seeds at 0;
- `below` inclusive;
- collinear segments not merged;
- convex `getEdge` read as linear;
- one Bellman–Ford round, or the Bellman–Ford clamp above the domain;
- the `NDEBUG` early return taken on success;
- loops and zero-capacity edges counted in `B`;
- the zero-cost stack pushing the wrong vertex.

The other three were killed elsewhere:
- Bellman–Ford in index order instead of Kahn order: timeout in full mode.
- `bounded()` always true: the `path-bound`, `total-bound` and `total-sum-bound` probes.
- The `2^126` sum check dropped: the `total-sum-bound` probe.

Finite tests complement the arguments above; they do not prove all inputs.

## Commands and results

2026-10-14, i9-11900H, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14, Linux x86-64, GNU++20. The tester, integration and consistency runs are from 2026-10-14, after the rebase onto master. The benchmark, the review and the `NDEBUG` probe are from 2026-10-13, on the same header.

| Command | Result |
|---|---|
| `20-min_cost_flow_tester.py --mode full --seed 20261013` | PASS 3 configurations, 41,380 cases; 5,514,597 checks optimized and ASan/UBSan, 2,653,157 checked; 24 probes; MEMORY peak 769 MB (pre-flight quick 707 MB) |
| `CXX=g++-14 20-min_cost_flow_tester.py --mode full --seed 20261013` | PASS (GCC 14.4.1), same counts; MEMORY peak 782 MB (pre-flight quick 720 MB) |
| `20-min_cost_flow_tester.py --mode stress --seed 7` | PASS 3 configurations, 68,345 cases; 13,906,568 checks optimized and ASan/UBSan, 4,115,298 checked; 24 probes; MEMORY peak 888 MB |
| `20-min_cost_flow_benchmark.py` | PASS, every result checked; MEMORY peak 341 MB |
| `02-integration.py` | PASS: 138 standalone/aggregate headers (including `20-min_cost_flow.hpp` alone and `99-all.hpp`), scalar/available-AVX2 multi-TU, workspace; MEMORY peak 647 MB |
| `03-consistency.py`, with `--braces` on the header, tester and benchmark | No errors |
| `NDEBUG` boundedness probe (ad hoc, `-O1 -DNDEBUG -fsanitize=address,undefined`, 512 MB cap) | See below |
| `@reviewer` (independent: own unit-step Bellman–Ford SSP oracle, 3,000 rounds under ASan/UBSan and 30,000 at `-O2` with assertions, n ≤ 12 and up to 40 calls each; 20,000 × 300-call `NDEBUG` zero-length-piece fuzz; reran quick, full and consistency) | No wrong answer on valid inputs. Fixed: `potentials()` now asserts the magnitude domain before recomputing (it hit the negative-cycle assertion at `B = 2 LIM`); negative capacities are clamped under `NDEBUG` (an `lll` capacity of `-2^126` hid a large total from `bounded()`); the `get_flow` mapping is the net flow; the mutation count; a `total-sum-bound` probe for the `2^126` sum; the `NDEBUG` `s == t` wording. Adopted: loops and zero-capacity edges leave `B`, the `D`/`Convex::e` names, `flow`'s complexity line, the shorter row. Decided by the user: `flow` stays beside `slope` (Keeps in `duplicates.md`) |

The `NDEBUG` probe cases and their results:

| Case | Result |
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
| `lll` capacity `-2^126` beside two `2^120` edges (formerly hid the total from `bounded()`) | flow 0 |
| `potentials()` with `B = 2 LIM` | returns clamped values |
| valid: loop of cost `LIM` and zero-capacity edge of cost `-LIM` beside a `±LIM/2` path | `{2, 0}` |

There was no sanitizer report; peak 27 MB.

```bash
python3 '96-Local Testing/04-Graphs/20-min_cost_flow_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/04-Graphs/20-min_cost_flow_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/04-Graphs/20-min_cost_flow_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/20-min_cost_flow_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/20-min_cost_flow_benchmark.py>) writes `20-min_cost_flow_benchmark.json`, which is ignored by git. Conditions:
- **Machine and build:** i9-11900H, shared, with load average about 2 on 16 threads during the run. GCC 16.2.1, `-O2 -DNDEBUG`, seed 20261013.
- **Runs:** one checked warm-up run, then 5 samples per implementation, interleaved.
- **Timing:** network construction is included; input generation is excluded.
- **Checking:** every implementation returns the same `(flow, cost)`.
- **References:** written inside the driver. The ACL-style reference has the same algorithm: one path per Dijkstra stopping at `t`, duals moved for settled vertices, and the zero reduced-cost stack. Two variants drop that stack, or replace the heap by an O(n^2) scan per phase (OI Wiki's dense Dijkstra).
- **Memory:** O(n + m) arcs; the heap holds at most m entries.

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

Headlines:
- `MinCostFlow` tracks the ACL-style reference within +1% to +5% on every workload. A run under load average 6 earlier the same day gave -1% to +9%, with absolute times 1.6–2.5× higher.
- Two inner-loop choices were measured in six interleaved rounds on the dense, bipartite and sparse workloads:
  - Hoisting `dist[u] + h[u]` out of the arc loop.
  - Reading `dist`, `h`, `vis` and `pe` through raw pointers: member vectors otherwise reload their data pointers after every heap push.

  Without them the library was 5–11% slower than the reference on the bipartite workload; with them it was 5% faster there and within ±4% elsewhere. Moving the workspace into locals of `slope` was not faster.
- The zero reduced-cost stack saves 2–35% (34% sparse, 35% grid, 22% dense, 2% bipartite), so it is kept. The dense O(n^2) scan is 1.7–2.1× slower than the heap on every workload where it ran, including the half-dense graph and the 50-vertex network, so it is not adopted (notes).
- One convex edge of p pieces costs one arc pair, so the x^2 workload runs 4.4× faster than the parallel-arc expansion with identical results.
- Kahn order makes the negative-cost initialisation independent of labelling; index-order Bellman–Ford needs about n rounds on the reverse-labelled DAG (the mutation check timed out on it).
- These are shared-machine observations, not universal speed claims.

## Sources

See [00-sources.md](00-sources.md#pages-fetched-on-2026-10-13-p045-min-cost-flow) for the sweep and [00-notes.md](00-notes.md#p045-min-cost-flow-omissions) for the candidates left out with reasons. The algorithms follow:
- **ACL `mcf_graph`:** API shape, Dijkstra stopping at `t` with duals moved for settled vertices, the zero reduced-cost stack, and slope merging.
- **KACTL `MCMF::setpi`:** Bellman–Ford initial potentials.
- **maspypy (`DAG=true`), hitonanode `mincostflow_nonegativeloop` and suisen's DAG initialiser:** linear-time initialisation on a DAG.
- **suisen:** member potentials across calls and the any-amount flow.
- **maspypy `get_potentials`:** the dual export.

Convex edges follow the row's piecewise-linear wording; no catalog has a convex-edge API. The code was written independently; no implementation was copied.

## Limits and handoffs

- **Elsewhere:**
  - supplies and demands, lower bounds and circulations: row 30;
  - negative cycles and min-cost b-flow (cost scaling, network simplex): rows 65 and 66;
  - assignment: row 23;
  - min-cost-flow duals for difference constraints: row 44;
  - Chinese postman: row 69.
- **Not provided:** changing an edge's capacity or cost after a flow, convex costs given by a callable over large capacities, and floating-point capacities or costs. These are recorded in the notes.
- **Not run:** exact GCC 14.2 and the Windows build.
