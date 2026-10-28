# 30-flow_with_demands.hpp — evidence

Owned by package P048 / GR27; omissions are in [00-notes.md](00-notes.md#p048-gr27-omissions). Contest profile: lower/upper-bound and vertex-supply reductions to the row 19 `MaxFlow` (Dinic) and, for costed networks, to a pluggable min-cost-flow backend (row 20 `MinCostFlow` by default). No SIMD and no Barrett/Montgomery reduction, so no `Compact` twins. It replaces the legacy `FlowWithDemands` struct.

## Contracts

### Common domain

- Every function takes `n >= 0` vertices and an edge list. Uncosted functions take `vector<tuple<int, int, Cap, Cap>>` entries `(u, v, lo, hi)`, and the costed adapter takes `(u, v, lo, hi, cost)`. Edge `i` must carry a flow `f_i` with `lo <= f_i <= hi`. Bounds are signed: a negative `lo` lets the flow run against the edge's direction. Loops, parallel and antiparallel edges, isolated vertices and `n = 0` are allowed. `Cap` is a signed integer type (`int`, `lng`, `lll`).
- `b` is a `vector<Cap>` of the edges' `Cap` (another element type does not deduce), empty (all zero) or with `n` entries summing to 0 (asserted). `b[v]` is the required outflow minus inflow at `v`: positive is a supply, negative a demand.
- Overflow bound (asserted with a 128-bit accumulator): `W = Σ_e (|lo_e| + |hi_e|) + Σ_v |b_v| <= numeric_limits<Cap>::max()`. Every internal capacity, excess, flow value and s–t value then lies in `[-W, W]`. An "infinite" bound must be a finite stand-in that never binds. For a single such non-loop edge, pick an endpoint `x` that is not `s` or `t` in the s–t functions. Then `|b[x]|` plus the larger bound magnitude of each other non-loop edge at `x` is enough, by conservation at `x`. When the edge joins `s` and `t`, conservation does not bound it, and the stand-in is the caller's bound on the value. Use `Cap = lll` when the sums exceed 2^63.
- Asserted preconditions: vertices in `[0, n)`, `lo <= hi`, the size and zero sum of `b`, `W <= MAX`, `s != t` with both in range, and for the costed adapter a cost other than `numeric_limits<Cost>::min()` and `Σ_i |cost_i| · max(|lo_i|, |hi_i|) <= 2^127 − 1` (checked up front with overflow builtins, so it does not depend on the edge order; it always holds when `Cap` and `Cost` have at most 64 bits). Out-of-range vertices are unchecked under `NDEBUG`, as in `MaxFlow` (user decision 2026-10-29). A valid input with no feasible flow is not a precondition failure: it returns `ok = false` with a certificate.
- The functions are pure: they copy nothing back into the input and keep no state between calls.

### FlowWithDemandsResult<Cap>

`{ok, value, cost, flow, infeasibleCut, stCut, potentials}`. Witnesses are fields filled whenever they cost no more than the objective; there is no flag (user decision 2026-10-29).
- When `ok`, `flow[i]` is edge `i`'s flow in input order and `infeasibleCut` is empty. `value` is the s–t value of the s–t functions and 0 otherwise. `cost` is the exact `Σ cost_i · f_i` from the costed adapter and 0 otherwise. `stCut` (n entries, from `maxFlowWithLowerBounds` and `minFlowWithLowerBounds` only) is the s side of a tight cut certifying the optimum. `potentials` (n entries, from the costed adapter only) is a dual certificate. Both are empty elsewhere.
- When `!ok`, `flow`, `stCut` and `potentials` are empty and `value = cost = 0`. `infeasibleCut` has `n` entries and marks a set `S` violating Hoffman's condition: `b(S) > Σ_{u∈S, v∉S} hi − Σ_{u∉S, v∈S} lo`. The supplies inside `S` exceed what can leave `S` once the forced inflow is counted, so no feasible flow exists. Since `Σ b = 0`, the complement is a demand-side violation.

### Reduction and correctness

Substitute `f_i = lo_i + x_i` with `x_i ∈ [0, hi_i − lo_i]`. Conservation becomes `out_x(v) − in_x(v) = d[v]`, where `d[v] = b[v] − Σ_{out} lo + Σ_{in} lo`. A `MaxFlow` on `n + 2` vertices gets one arc per edge with capacity `hi − lo`, an arc `S → v` of capacity `d[v]` for each `d[v] > 0` and an arc `v → T` of capacity `−d[v]` for each `d[v] < 0`. A feasible `f` exists iff the maximum `S`–`T` flow saturates the `S` arcs. Their total equals that of the `T` arcs because `Σ d = Σ b = 0`.

If the flow falls short, take `R`, the vertices reachable from `S` in the residual network. That is a minimum cut, so `need − d⁺(R) + d⁻(R) + Σ_{out R}(hi − lo) < need`. Here `d⁺(R)` sums `d[v]` over the vertices of `R` with `d[v] > 0`, and `d⁻(R)` sums `−d[v]` over those with `d[v] < 0`. Their difference is `d(R)`. Substituting `d(R) = b(R) − lo(out R) + lo(in R)` gives the Hoffman violation above. The shared engine `flow_with_demands_detail::solve<Net>` (Net is `MaxFlow<Cap>` or the backend) computes `R` from `edges()` (forward residual when `flow < cap`, backward when `flow > 0`). This holds for `MaxFlow`, whose arcs here have no reverse capacity, and for any backend.

Integrality: every capacity is an integer, so Dinic returns an integer flow, and the optimum among real flows is attained at an integer flow (total unimodularity).

### feasibleCirculation

`feasibleCirculation(n, es, b = {})` returns a feasible `f` or the certificate. Time O(n² · (n + m)) (Dinic on `n + 2` vertices and at most `m + n` arcs), memory O(n + m).

### feasibleFlow

`feasibleFlow(n, es, s, t, b = {})` frees the balance at `s` and `t`. Conservation holds at every other vertex, while `s` sends `b[s] + value` and `t` receives `value − b[t]` net. The function returns some feasible flow, with a value of any sign, using one maximum flow. `ok = false` means no flow of any value exists. Added on user decision 2026-10-29 (ei1333 `can_flow(s, t)`).

The value is carried by two arcs `t → s` and `s → t`, each of capacity `W`. `|value| ≤ Σ_{e at s} max(|lo|, |hi|) + |b[s]| ≤ W`, so these arcs never bind. After feasibility, `value = f(t→s) − f(s→t)`. Both arcs are then zeroed with `changeEdge`, which leaves an s–t flow of that value. `feasibleFlow` stops there. The certificate puts `s` and `t` on the same side: an arc of capacity `W ≥ need` cannot cross a cut smaller than `need`. Time O(n² · (n + m)), memory O(n + m).

### maxFlowWithLowerBounds

`maxFlowWithLowerBounds(n, es, s, t, b = {})` has the domain of `feasibleFlow` and returns the maximum value. After the `feasibleFlow` steps it adds `flow(s, t)` on the residual network.

`S` is a dead end in that residual network: every `S` arc is saturated, so `S` can only be entered. `T` cannot be entered at all. A residual maximum therefore changes no supply arc and keeps every other vertex balanced. The increment is at most `Σ_{e at s}(hi − lo) ≤ W`, so `MaxFlow::flow` never truncates it at `MAX`. Maximality follows because no residual s–t path is left on the original edges. `stCut` is `C`, the set reachable from `s` over residual original edges; `S` is reachable but a dead end, and `T` cannot be entered. Every edge leaving `C` carries `hi` and every edge entering it carries `lo`, so `value = Σ_{u∈C, v∉C} hi − Σ_{u∉C, v∈C} lo − b(C)`. Every feasible flow's value is at most this, which certifies the maximum. Time O(n² · (n + m)), memory O(n + m).

### minFlowWithLowerBounds

`minFlowWithLowerBounds(n, es, s, t, b = {})` has the domain of `feasibleFlow` and returns the minimum value. The minimum may be negative: forced flow from `t` to `s` makes the value negative (ei1333 `min_flow` convention). A single arc `t → s` of capacity ∞ (OI Wiki, legacy) would force `value ≥ 0`. After the `feasibleFlow` steps it subtracts `flow(t, s)`, with the same dead-end argument. `stCut` is the complement of the set reachable from `t`. Edges leaving it carry `lo` and edges entering it carry `hi`, so `value = Σ_{u∈C, v∉C} lo − Σ_{u∉C, v∈C} hi − b(C)` is a lower bound on every feasible value. Time O(n² · (n + m)), memory O(n + m).

### minCostCirculationAdapter

`minCostCirculationAdapter<Cap, Cost, Backend = MinCostFlow<Cap, Cost>>(n, es, b = {})` returns a minimum-cost feasible circulation for costs of any sign, including negative cycles.

- An edge `(u, v, lo, hi, c)` with `c < 0` is replaced by `(v, u, −hi, −lo, −c)`, carrying `g = −f` at the same cost `c · f`. Under the shift to `lo`, the reversed edge starts saturated at `hi`, and each backend unit on `v → u` lowers `f` by one at a cost of `−c > 0`. The `lo > numeric_limits<Cap>::min()` assertion, implied by `W <= MAX`, keeps the negation defined.
- The reduction then runs as above on the reversed list, with backend arcs `u → v` of capacity `hi − lo` and cost `c ≥ 0`, and supply arcs of cost 0. The residual network has no negative cycle at the start, so a min-cost maximum `S`–`T` flow saturating the supply arcs is a minimum-cost circulation. The objective differs from the original by the constant `Σ c · lo`. Reversing an edge leaves every Hoffman term unchanged: `hi` out of `S` becomes `−lo' = hi` into `S`. So the certificate of the reversed list is valid for the original.
- `cost` is recomputed from the returned flows as `Σ c_i · f_i` in wrapping 128-bit arithmetic. Every partial sum is bounded by the asserted `Σ |c| · max(|lo|, |hi|) <= 2^127 − 1`, so the result is exact. The backend's own total is not used.
- `potentials` is the backend's `potentials()` restricted to `[0, n)`. On the backend's residual arcs `c + p[u] − p[v] ≥ 0`, so for every original edge, `c + p[u] − p[v] > 0` implies `f = lo` and `< 0` implies `f = hi` (complementary slackness, unchanged by the reversal). This certifies the minimum.
- Infeasibility uses the same residual-reachability certificate, since a min-cost maximum flow is a maximum flow.

**Backend contract.** `Backend(int n)`; `int addEdge(int u, int v, Cap cap, Cost cost)` with sequential IDs from 0; `flow(s, t)` returning a pair whose `.first` is the value of a minimum-cost maximum s–t flow (costs given to it are `>= 0`); `getEdge(i).flow`; `edges()` with `from`, `to`, `cap`, `flow`; and `potentials()`, returning at least `n + 2` values with `cost + p[u] − p[v] ≥ 0` on every residual arc after `flow`. `MinCostFlow` (row 20) satisfies it, and so can the planned rows 65 and 66. The header does not depend on them, because they may use the bounded-flow model (ownership in [00-notes.md](00-notes.md#bounded-flow-ownership)).

**Backend domain.** The backend's preconditions apply to the reduced network. For `MinCostFlow`, in input terms: `|cost_i| <= LIM = numeric_limits<Cost>::max() / 16`; over non-loop edges with `lo_i < hi_i`, `Σ_i (hi_i − lo_i) · |cost_i| < 2^126`, and either `Σ_i |cost_i| <= LIM` or `max_i |cost_i| <= LIM / (n + 1)`. That is its `bounded()` check, since supply arcs cost 0. Time O(n + m) plus one backend run on `n + 2` vertices and `m + n` arcs. With `MinCostFlow` that is O((K + 1) · (n + m) · log(n + m)) for K augmentations, which never exceeds the total positive excess `need` after the reversal (no Bellman–Ford is needed, because the costs are nonnegative). The bound is pseudo-polynomial in that excess, not in the supplies: `n = 2`, `b = 0`, edges `(0, 1, 0, 10^9, −1)` and `(1, 0, 0, 5, 0)` give `need = 10^9`. A successive-shortest-paths augmentation still moves a whole bottleneck at once. Polynomial cost scaling is row 65's job.

## Aliases

- `feasibleFlow`: 有源汇上下界可行流 (OI Wiki), ei1333 `can_flow(s, t)`.
- `feasibleCirculation`: circulation with demands, 无源汇上下界可行流 (OI Wiki), ei1333 `can_flow()`, hitonanode `B_Flow` without costs.
- `maxFlowWithLowerBounds`: 有源汇上下界最大流 (OI Wiki), maspypy `MaxFlow_with_Lower_Bound::flow`, hitonanode `MaxFlowLowerBound::flow`, ei1333 `max_flow`.
- `minFlowWithLowerBounds`: 有源汇上下界最小流 (OI Wiki), ei1333 `min_flow`, cp-algorithms "minimal flow".
- `minCostCirculationAdapter`: 上下界最小费用可行流 (OI Wiki), hitonanode `B_Flow`, maspypy `Min_Cost_B_Flow`.
- `infeasibleCut`: the violated set of Hoffman's circulation theorem.

## Legacy mapping

`FlowWithDemands<T>` (`OLD/Team Notebook/src/algs.cpp:1754`, `algsbetter.cpp:2010`, `test.cpp:640`; the three copies are the same apart from formatting):
- `FlowWithDemands(n)` plus `addEdge(u, v, cap, demand)` → an edge list entry `(u, v, demand, cap)`.
- `hasValidFlow(s, t)` added an arc `t → s` of capacity `numeric_limits<T>::max()` and asked whether a flow of nonnegative s–t value exists → `maxFlowWithLowerBounds(n, es, s, t).ok && .value >= 0`. Equivalently, `feasibleCirculation` with an added edge `(t, s, 0, W)`. Its `numeric_limits<T>::max()` arc could overflow the Dinic sums; `W` replaces it. It mutated the network, so a second call added the arcs twice; the free functions have no state.
- The legacy `sum_demands` and `d_in`/`d_out` members → the internal excess `d`; no public counterpart.

## Feature-to-test map

`96-Local Testing/04-Graphs/30-flow_with_demands_tester.py` uses the shared graph runner: optimized `-O2 -DNDEBUG`, checked, and ASan/UBSan outside quick, all with `-Wall -Wextra -Wconversion -Werror`.
- It runs 21 assertion probes and the `Cap`/`Cost` type pairs int/int, lng/lng, lll/lll, lng/int and int/lng.
- Every result is certified independently of the header. A feasible result passes a witness check (bounds, conservation, value and cost recomputed in `lll`) plus an optimality certificate. An infeasible result must give a strict Hoffman cut.
- The integer brute force is validated on 13 hand-derived cases.

| Operation | Test | Oracle |
|---|---|---|
| `FlowWithDemandsResult` fields | `verify` checks the shape of every field when ok and when not, per function; `same` compares repeated calls field by field | Witness recomputation in `lll` |
| `feasibleCirculation` | `handCases`, `exhaustive` (every two-vertex network, m ≤ 2 in full), `randomSmall` (n ≤ 4, m ≤ 4, bounds in [−3, 3], supplies), `extremesFor`, `medium` (n ≤ 40), `largeFor` (`lng`, `int` up to n = 100,000; `lll` at n / 4) | Brute force over integer flows; witness; Hoffman cut |
| `feasibleFlow` | Every section, through `runCase` | Witness; ok equals the brute and `maxFlowWithLowerBounds` ok; value in [min, max]; Hoffman cut with s and t on one side |
| `maxFlowWithLowerBounds` | Every section | Brute maximum; no residual s→t path in the returned flow (BFS); Hoffman cut |
| `minFlowWithLowerBounds` (negative values) | Every section | Brute minimum; no residual t→s path; Hoffman cut |
| `stCut` | `verify` with `tight` in every section | `s ∈ C`, `t ∉ C`; exact `lll` equalities: value = hi(out C) − lo(in C) − b(C) (max) and value = lo(out C) − hi(in C) − b(C) (min) |
| Consistency identity | `runCase` | The circulation is feasible iff 0 lies in [min, max]; min ≤ max |
| `minCostCirculationAdapter` (default `MinCostFlow`) | All sections; costs in [−1000, 1000], negative cycles and negative loops; bounds up to 10^12 at n = 100, 300 and 600 | Brute minimum cost; no negative residual cycle (Bellman–Ford); exact cost in `lll` |
| `potentials` | Every costed section, with both backends | Complementary slackness: `c + p[u] − p[v] > 0 ⇒ f = lo`, `< 0 ⇒ f = hi` |
| Backend contract | Tester-side `BellmanBackend` (Bellman–Ford successive shortest paths, Bellman–Ford `potentials()`) as the third template argument, m ≤ 40 | Certified independently; ok and cost equal to the default backend's |
| Cost bound | `costBound`: `Cap = lll`, `Cost = lng`, c = 2^30, all 24 edge orders, bound sums 2^126 (fixed flows, cost 0) and 2^125 (free flows, cost −2^124) | Brute and hand-derived; order independence |
| Infeasibility certificate (all functions) | Every `!ok` result; coverage requires ok and `!ok` results from each function | Strict Hoffman inequality |
| Signed bounds, loops, parallel and antiparallel edges, n = 0 and 1, disconnected graphs | Hand cases and random generators | As above |
| Pure repeated calls | Each call is repeated, and also run with explicit zero supplies when `b` is empty | Identical results |
| Extremes, W = MAX for `int`, `lng`, `lll` and int/lng | 7 hand cases per `Cap`; tiny networks scaled by k = MAX / W | Brute result × k. Costed `lll` runs keep W ≤ MAX / 8, inside `MinCostFlow::bounded()` |
| Assertion probes (checked build) | `--invalid`: n < 0, lo > hi, size of `b`, Σ b ≠ 0, vertex out of range or negative, s = t, s or t out of range, W > MAX (edges, negative edges, supplies, `lll` min, `lng` costed), `Cap` min, `Cost` min (`int` and `lng`), cost bound (2^128 and exactly 2^127) | SIGABRT from the assertion |

Modes:
- quick: every two-vertex network with m ≤ 1; 400 brute-force rounds; 100 scaled-extreme rounds per `Cap`; 60 medium networks; large uncosted at n = 2,000; costed at n = 150 and n = 100 with bounds up to 10^12.
- full: m ≤ 2 exhaustive; 4,000, 1,000 and 600 rounds; n = 30,000; costed n = 1,500 and 300.
- stress: 20,000, 5,000 and 3,000 rounds; n = 100,000; costed n = 3,000 and 600.

The checked build runs the large sections at a tenth of the size. Inclusion through `99-all.hpp` and two translation units are covered by `02-integration.py`.

## Commands and results

Run 2026-10-29 on the local machine (GCC 16.2 `g++`, GCC 14.4.1 `g++-14`):

| Command | Result | Memory peak |
|---|---|---|
| `python3 '96-Local Testing/04-Graphs/30-flow_with_demands_tester.py' --mode quick` | PASS, 2 configurations, 21 probes | 845 MB |
| `python3 '96-Local Testing/04-Graphs/30-flow_with_demands_tester.py' --mode full --seed 31` | PASS, 3 configurations | 976 MB |
| `CXX=g++-14 python3 '96-Local Testing/04-Graphs/30-flow_with_demands_tester.py' --mode full --seed 32` | PASS, 3 configurations | 945 MB |
| `python3 '96-Local Testing/04-Graphs/30-flow_with_demands_tester.py' --mode stress --seed 33` | PASS, 3 configurations | 1210 MB |

The peaks are dominated by compilation. The optimized binary alone takes 0.24 s in quick and about 20 s in stress.

`python3 '96-Local Testing/02-integration.py'`: PASS, 182 standalone and aggregate headers, multi-TU, workspace (2026-10-29).

Benchmarks: none required (Contest profile, no Barrett or Montgomery reduction).

## Sources

[00-sources.md](00-sources.md#pages-fetched-on-2026-10-28-p048-gr27): cp-algorithms flows with demands, OI Wiki 上下界网络流, ei1333 `maxflow-lower-bound`, maspypy `maxflow_with_lower_bound`, hitonanode `maxflow_lowerbound` and `B_Flow`. Written independently from the algorithm descriptions; no catalog code was copied. Not inspected:
- LEMON `Circulation` (connection refused);
- the Nyaan and suisen indexes beyond their first 100k characters;
- Hoffman's 1960 paper, whose theorem is used as stated in textbooks.

## Limits and handoffs

- Design calls confirmed by the user (user decision 2026-10-29):
  - free functions over edge lists rather than a builder struct;
  - signed bounds, and supplies on every function;
  - the s–t value carried by a two-way pair of capacity-`W` arcs, so minima may be negative;
  - negative-cost edges reversed, so `MinCostFlow` serves every sign of cost at a pseudo-polynomial bound;
  - `cost` and `value` added to the result;
  - Dinic (`MaxFlow::flow`) as the only engine. For adversarial deep-phase inputs, build the reduction by hand on `MaxFlow` and call `pushRelabel`.
- The costed adapter is pseudo-polynomial in the total positive excess after the reversal. Rows 65 and 66 own polynomial min-cost b-flow with dual certificates (Library Checker `min_cost_b_flow`).
- No incremental re-solve after a bound change; rebuild instead.

## History

- 2026-10-28: new row (P048 / GR27); review: 5 findings fixed (infinite-bound rule at s and t, out-of-range supply write under `NDEBUG`, infeasible-shape test, engine complexity comment, unread LEMON aliases); full suite passed on g++ and g++-14, stress passed.
- 2026-10-29: `/finish` audit: 5 defects fixed (order-dependent cost assertion, cost-bound probe, excess wording, adapter memory term, split subsections) and test gaps closed (int/lng, large-capacity costed, large `lll`/`int`); user decisions: design calls confirmed, `feasibleFlow`, `stCut` and `potentials` added; full suite passed on g++ and g++-14, stress passed.
