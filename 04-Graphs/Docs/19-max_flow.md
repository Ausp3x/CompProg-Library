# 19-max_flow.hpp — evidence

Owned by package P044 / GR08. The header replaces the byte-for-byte legacy `EdmondsKarp` extraction with one residual network, `MaxFlow<Cap>`, carrying four engines (Dinic, Edmonds–Karp, capacity-scaling Dinic, highest-label push-relabel), single augmenting paths, dynamic capacity changes, cut witnesses and flow decomposition. Contest profile: no SIMD, no Barrett/Montgomery, so no `Compact` twins.

## Contracts

### MaxFlow<Cap>

- `Cap` is a signed integer type (`int`, `lng`, `lll`; checked by `static_assert`). Vertices are `0..n-1`. Loops and parallel edges are allowed. A loop never carries flow from an engine.
- `addEdge(u, v, cap, rev_cap = 0)` returns sequential edge IDs from 0 and asserts `0 <= cap`, `0 <= rev_cap` and `cap + rev_cap <= MAX`. It stores one arc pair: the forward arc starts with `cap`, the reverse arc with `rev_cap`. With `rev_cap = cap` this is an undirected edge using two arcs, not four. Edges may be added at any time, including after a flow; the next engine call resumes from the current residual network (incremental insertion).
- `MaxFlow(const G &)` accepts `Graph` or `CsrGraph`. Edge `i` of the graph becomes edge `i` with capacity `w`, and `rev_cap = w` when the graph is undirected. Weights must lie in `[0, MAX]` (asserted before the cast, so a narrower `Cap` never truncates silently), and `2w <= MAX` for undirected graphs.
- `getEdge(i)` returns `{from, to, cap, flow}`. Here `flow` is the net flow from `from` to `to`, in `[-rev_cap, cap]`; it is negative only when `rev_cap > 0`. `edges()` returns every `getEdge(i)` in ID order.
- `changeEdge(i, cap, flow)` is a raw set (ACL semantics): it asserts `0 <= cap`, `cap + rev_cap <= MAX` and `-rev_cap <= flow <= cap`, keeps `rev_cap`, and does not repair conservation; the caller owns the consequence.
- `flow(s, t, limit = MAX)` requires `s != t` and `limit >= 0`. It adds `min(remaining max flow, limit)` to the current flow and returns that amount. Repeated calls continue the same flow, so the sum of returned values is the flow value. The result never overflows: augmentation stops at `limit`, so with the default limit it returns `min(max flow, MAX)`. Use `Cap = lll` when the value can exceed 2^63 - 1.
- `edmondsKarp(s, t, limit = MAX)` has the same contract and result value as `flow`. It repeatedly pushes `min(bottleneck, limit - added)` along a shortest residual s→t path (fewest arcs) until `t` is unreachable or the limit is reached.
- `augmentPath(s, t)` requires `s != t`. It pushes one shortest residual s→t path at its full bottleneck and returns that amount, or returns 0 and leaves the network unchanged when `t` is unreachable. The amount is one arc's residual, so it fits `Cap`, but repeated calls can raise the flow value above `MAX`; sum the results in a wider type when that can happen.
- Under `NDEBUG`, `flow`, `edmondsKarp`, `capacityScalingDinic` and `augmentPath` with `s == t` return `limit` (`MAX` for `augmentPath`) without changing the network.
- `capacityScalingDinic(s, t, limit = MAX)` has the same contract and result value as `flow`. It runs Dinic phases restricted to residual arcs of capacity `>= delta`, with `delta` going from the largest power of two `<=` the largest residual capacity down to 1.
- `pushRelabel(s, t)` adds a maximum flow to the current flow and returns the added value. Precondition (asserted with `__builtin_add_overflow`): the residual capacities of the non-loop arcs leaving `s` sum to at most `MAX`, which bounds every excess. Under `NDEBUG`, a violated supply bound or `s == t` returns 0 before touching the network, so no signed overflow and no out-of-range label occur. Phase two returns stranded excess to `s`, so afterwards `getEdge`, `minCut` and `pathDecomposition` see a valid maximum flow.
- `changeCapacity(i, cap, s, t)` requires the current flow to come from the zero flow through this struct's `flow`, `edmondsKarp`, `capacityScalingDinic`, `pushRelabel` (with or without `limit`), `augmentPath`, `addEdge` and earlier `changeCapacity` calls with the same `s`, `t`, or more generally to be a valid flow that decomposes into s→t paths and cycles only. An arbitrary conserving flow set by `changeEdge` that carries a t→s path is outside the domain: the cancel step assumes the excess at `u` came from `s` (reviewer counterexample: `3->1->2->0` with `s = 0`, `t = 3`, value −1). It sets edge `i`'s forward capacity to `cap` (with `rev_cap` unchanged), leaves a maximum flow and returns the signed change of the flow value. A decrease below the edge's flow first reroutes the excess `u -> v` through the residual network, then cancels the remainder along `u -> s` and `t -> v`, then re-augments `s -> t`.
- `minCut(s)` returns the vertices reachable from `s` in the residual network. After a maximum flow (not a `limit`-truncated one) this is the source side of a minimum cut. `minCutEdges(s)` lists the IDs of edges whose forward arc (`cap > 0`) crosses from that side to the other, or whose reverse capacity (`rev_cap > 0`) crosses the same way, in ID order. Their capacities sum to the cut value.
- `pathDecomposition(s, t)` requires a valid flow (conservation outside `s` and `t`; any value, any flow through `t` or into `s`) and does not mutate the network. It returns `(amount, edge IDs)` pairs, each a simple s→t path following each edge in its flow direction, with each edge used at most its |flow|. The amounts sum to `max(value, 0)`. Flow cycles, including cycles through `s` or `t`, are cancelled and dropped. `s == t` returns no paths under `NDEBUG`.
- Workspace vectors `level`, `it`, `que` are members, reused by every engine call. The struct is single-threaded.

### Engines and correctness

- **Dinic (`flow`, internal `dinic`).** Each phase runs a BFS from `s` over arcs with residual `>= delta`, stopping once `t` is labelled; every vertex at a level below `level[t]` has been labelled by then. The blocking flow is ACL's multi-push DFS from `t` back to `s` over arcs to level − 1, simulated with an explicit frame stack `(v, up, got)`. Each frame pushes into one child at a time and adds the child's result to its arc pair. It returns once `got == up` (keeping its current arc) or when its arcs are exhausted (the vertex is then marked dead with level `n`). There is no recursion, so a 500,000-vertex path is stack-safe. Bound: O(n^2 * m) in general, O(m * sqrt(m)) for unit capacities.
- **Edmonds–Karp (`edmondsKarp`, `augmentPath`, internal `augment`).** A BFS from `s` over arcs with positive residual stops once `t` is labelled. `it[v]` holds the index of `v`'s reverse arc to its BFS parent, so a walk from `t` reads the bottleneck, and a second walk pushes `min(bottleneck, limit)`. A BFS-tree path is a shortest residual path. `edmondsKarp` stops when `t` is unreachable, so no residual s–t path remains and the flow is maximum, or when the added flow reaches `limit`, because the last push is capped at the remaining limit. Each push is at most one arc's residual and keeps every arc pair's sum, so no arithmetic overflows. Bound: shortest-path distances never decrease, and each arc is saturated as the bottleneck at most n/2 times, so a run makes O(n * m) augmentations from any starting flow, each costing O(n + m). With integer capacities each augmentation also adds at least 1, so the cost is O(min(n * m, F + 1) * (n + m)) for an added flow F.
- **Push-relabel.** It saturates every residual arc leaving `s`, then assigns exact labels by a global relabel: the distance to `t` over reverse residual arcs, avoiding `s`; otherwise `n` plus the distance to `s`; otherwise `2n - 1`. Each of these is at least any valid labelling, so labels never decrease and the standard bounds hold.
  - It discharges the highest active label first, keeping a current arc per vertex.
  - The gap heuristic uses per-label doubly linked lists of vertices with labels below `n`. When the last vertex leaves label `old`, every vertex with a label in `(old, n)` moves to `n + 1`. The relabelled vertex is the highest active one, so none of the lifted vertices is active, and the cost is proportional to the vertices lifted.
  - A global relabel reruns after every `GLOBAL * (n + m)` units of relabel work (`GLOBAL = 4`, measured below).
  - The run ends when no vertex other than `s` and `t` has excess. Every vertex then conserves flow, and validity of `h[s] = n` rules out an s–t residual path, so the flow is maximum. Bound: O(n^2 * sqrt(m)).
  - Every vertex with positive excess has a residual path to `s`, so a relabel always finds an arc; the `h[u] >= 2n` break is a defensive bound only.
- **Capacity scaling.** Phase `delta` leaves no s–t path of residual `>= delta`, so the remaining flow is below `m * delta`, and each phase does O(m) augmentations. Bound: O(n * m * log(U)).
- **changeCapacity.** With excess `d` at `u` and deficit `d` at `v`, a flow decomposition of the pseudo-flow shows that whatever does not reroute from `u` to `v` reaches `s` from `u` and reaches `v` from `t`. The cancellations therefore always complete. The final `s -> t` augmentation restores maximality, because a cancelled path can unblock another route. When the previous flow was maximum, all flow moved is at most `d = |cap - old cap|`, so the cost is O((n + m) * (d + 1)), capped by Dinic's O(n^2 * m).
- **pathDecomposition.** `need` starts as the net inflow of `t` (computed in `lll`). A walk from `s` follows edges with remaining flow. Reaching `t` while `need > 0` records a path of amount `min(bottleneck, need)` and lowers `need`; with `need = 0`, `t` is an ordinary vertex, so flow passing through `t` is cancelled as part of cycles. Revisiting a vertex on the stack cancels that cycle. Conservation guarantees that every vertex reached with positive inflow (other than `t` while `need > 0`) has positive outflow, and when `s` runs out of outflow the remaining net value is at most 0, so the recorded amounts total `max(value, 0)`. Each record zeroes an edge or `need`, so the cost is O(n * m), plus the output.

### Legacy mapping

The removed adapter `EdmondsKarp` (`OLD/[1] algorithms.cpp:348-469`) maps to `MaxFlow<lng>`:
- `EdmondsKarp(n, adj, cap)` → `MaxFlow<lng>(Graph::fromMatrix(c))`, where `c` is the `(n + 1) × (n + 1)` matrix holding `cap[u][v]` at the pairs listed in `adj[u]` and 0 elsewhere (a repeated listing already counted once). Edge IDs follow the row-major order of the nonzero entries. `ocap[u][v]` is `getEdge(i).cap` of the edge `i` from `u` to `v`. The residual `ncap[u][v]` was `ocap[u][v] - f(u→v) + f(v→u)`: the edge's `cap - flow`, plus the `flow` of the opposite edge when `c[v][u]` is nonzero too. The adapter's pair-overflow assertion has no counterpart, because each direction becomes its own edge.
- `getMaxFlow(s, t)` and `max_flow` → `flow(s, t)` (the Dinic run that the adapter made) or `edmondsKarp(s, t)` on a fresh `MaxFlow` per call. The adapter restarted from the original capacities on every call, while `flow` continues the current flow, so another pair `(s, t)` needs a new network. The adapter returned 0 for `s == t`; `MaxFlow` asserts `s != t`, so the caller handles that case.
- `augmentFlow(s, t)`, which returned the bottleneck of one shortest path without pushing it → `augmentPath(s, t)`, which pushes it. `add_flow` (0 after a maximum flow) → `augmentPath` returning 0. Its `s == t` result 0 is an asserted precondition too. Its BFS parent tree `par` is not exposed: `augmentPath` keeps the tree in the internal workspace `it`, and `getEdge` shows the pushed path.
- `getMinCut(s, t)` → `flow(s, t)` on a fresh network, then `minCut(s)` for `in_S` (the residual reachable set, which `par` also encoded after a maximum flow), and `minCutEdges(s)` with `getEdge(i).from`/`.to` for `cut_set`, in the same `(u, v)` order. `cut_set` also listed zero-capacity pairs; those have no edge now and do not appear.

## Feature-to-test map

`96-Local Testing/04-Graphs/19-max_flow_tester.py` uses the shared graph driver: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan `-O1 -g` with leak detection, all with `-Wall -Wextra -Wconversion -Werror`. Failures are non-removable and print the network, the operation, and the expected and actual values. The oracles:
- **Brute force:** the minimum over every s-side vertex subset (max-flow min-cut theorem), validated against the exhaustive three-vertex corpus.
- **Certificate:** a feasible, conserving flow of the returned value plus a residual cut of equal capacity. This is independent of the engines.

| Operation | Test | Oracle |
|---|---|---|
| `addEdge` (IDs, reverse capacity, loops, multiedges) | Every network in every corpus; `edges`/`getEdge` checked after every engine | Exact endpoints and capacities; flow within `[-rev_cap, cap]` |
| `flow` (limit, resume) | Random networks n ≤ 8, m ≤ 17, capacities up to 1, 6, 2^40, 2^61 (4,000 full / 20,000 stress); all 19,683 three-vertex looped multiplicity networks; limit in `[0, best + 2]` then resume; second call adds 0 | Brute min cut; certificate |
| `capacityScalingDinic` | Same corpus with and without limit; medium and large corpora | Brute min cut; certificate |
| `pushRelabel` | Same corpus (when the supply fits `Cap`); resumed after a limited Dinic; called twice; 300 medium networks n ∈ [20, 200); large chain, random, unit bipartite and dead-end excess-return networks | Brute min cut; certificate (valid flow after phase two) |
| `Cap` = `int`, `lng`, `lll` | Small corpus in all three; 2^98–2^101 capacities in `lll`; `lng` max flow above 2^63 saturates at `MAX` with a valid partial flow | Brute min cut computed in `lll` |
| `changeCapacity` | 1–6 random increases and decreases per network, starting from `flow`, `pushRelabel`, limited `flow`, limited scaling, limited `edmondsKarp` or one `augmentPath` (optionally followed by `pushRelabel`), mixed with `addEdge` after flow; large chain cut to 0 and restored | Brute min cut of the modified network; returned delta equals the change of value; certificate |
| `changeEdge` | Raw set with negative flow on reverse-capacity edges | `getEdge` round trip |
| `minCut`, `minCutEdges` | After every maximum flow | Side contains `s` and not `t`; cut capacity equals the flow value; exact edge-ID list |
| `pathDecomposition` | After every maximum flow, including `lll` and reverse capacities; one arbitrary valid flow per small round, set by `changeEdge` from random s→t paths, t→s paths and cycles through any vertex (negative values, flow through `t`, loops) | Simple s→t paths along flow directions; per-edge usage within absolute flow; amounts positive and summing to `max(value, 0)` |
| `MaxFlow(Graph)`, `MaxFlow(CsrGraph)` | Random directed and undirected multigraphs (`lng` and `int`) | Brute min cut with `rev_cap = w` for undirected |
| `edmondsKarp` (limit, resume) | Same small corpus as `flow` in `int`, `lng`, `lll`, with and without limit, resumed by itself or `pushRelabel`; 300 medium networks; large chain, random and dead-end networks (not the unit bipartite one, whose O(k * m) run is too slow for the checked build) | Brute min cut; certificate |
| `augmentPath` | Every small network in `lng` (and `int` when capacities are at most 6), called until it returns 0, from the zero flow or after a limited `flow`, including totals above 2^63 - 1 in `lng`; the first call on every matrix-built network; once more after each maximum there | Independent BFS over the residual network rebuilt from the edge flows: the changed edges form a simple s→t path of the BFS distance, each moved by the returned amount, which equals their minimum prior residual; 0 and no change when `t` is unreachable; the total equals the brute min cut; certificate after every call |
| `MaxFlow(Graph::fromMatrix(cap))` (the removed `EdmondsKarp` input) | Every third random network as a dense matrix (repeated pairs collapse to the last capacity, loops kept), then `flow` or `edmondsKarp` | Brute min cut of the collapsed network; certificate, including `minCutEdges` IDs in row-major order |
| Preconditions | 23 checked assertion probes (sizes, ranges, negative or overflowing capacities including an undirected `Graph` weight of 2^63 - 1, `s == t` for every engine and `augmentPath`, negative limits, flow bounds, supply overflow) | Expected assertion abort |

Mutation checks (scratch copies under `/tmp`, quick mode, not part of the suite):
- 2026-10-12, 6 mutants of Edmonds–Karp, all killed: no reverse-arc update, a bottleneck ignoring `limit`, DFS instead of BFS, a bottleneck read from the last arc only, zero-residual arcs traversed, and `edmondsKarp` stopping after one path.
- 2026-10-10, 17 mutants of the other kept code, 16 killed: changeCapacity dropping the `t -> v` cancel or the final augmentation, a decomposition direction swap or cycle off-by-one, the scaling loop skipping `delta = 1`, `minCutEdges` or `getEdge` ignoring reverse capacity, `changeEdge` writing the wrong slot, the `Graph` adapter dropping reverse capacity, Dinic ignoring `limit` or overrequesting from a child, global relabel ignoring residual capacity, push overshooting, a relabel that keeps its current arc, and `pathDecomposition` ignoring or not capping the net inflow of `t` (killed by the arbitrary-flow corpus).
- The survivor sets gap-lifted labels to `old + 1`. That changes labels only for vertices that cannot reach `t`, so it affects performance, not the result. Removing the dead-end pruning or the reroute in `changeCapacity` also survives, as expected for optimizations.

Finite tests complement the arguments above; they do not prove all inputs.

## Commands and results

2026-10-12, i9-11900H, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `19-max_flow_tester.py --mode full --seed 20261012` | PASS 3 configurations, 27,983 cases, 31,488,369 checks, 23 probes; MEMORY peak 883 MB (pre-flight quick 527 MB) |
| `CXX=g++-14 19-max_flow_tester.py --mode full --seed 20261012` | PASS (GCC 14.4.1), same counts; MEMORY peak 909 MB (pre-flight quick 552 MB) |
| `19-max_flow_tester.py --mode stress --seed 7` | PASS 3 configurations, 61,183 cases, 95,354,695 checks, 23 probes; MEMORY peak 1135 MB |
| `19-max_flow_benchmark.py` | Last run 2026-10-10, before the prune (the engines it times are unchanged): PASS, every result checked; MEMORY peak 333 MB |
| `02-integration.py` | PASS: 136 standalone/aggregate headers (including `19-max_flow.hpp` alone and `99-all.hpp`), scalar/available-AVX2 multi-TU, workspace; MEMORY peak 648 MB |
| `03-consistency.py`, with `--braces` on the header and tester | No errors |
| `NDEBUG` boundedness probe (ad hoc, `-O1 -DNDEBUG -fsanitize=address,undefined`, 512 MB cap): `edmondsKarp(1, 1, 7)`, `edmondsKarp(1, 1)`, `augmentPath(1, 1)`, `flow(1, 1, 7)`, `edmondsKarp(0, 1, -1)`, `capacityScalingDinic(1, 1, 7)`, `pushRelabel(1, 1)`, `pathDecomposition(1, 1)`, `augmentPath` to an unreachable `t`, two `LLONG_MAX` source edges into `pushRelabel` | Return 7, `MAX`, `MAX`, 7, 0, 7, 0, no paths, 0 and 0, with every edge flow unchanged; no sanitizer report; peak 23 MB |
| `@reviewer` (independent; own exact-BFS-path oracle, 40,000 rounds each in `int`, `lng`, `lll` under ASan/UBSan, 3,065,970 checks; reran quick, full and integration) | No code defect; 4 evidence findings fixed. |

```bash
python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/19-max_flow_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/19-max_flow_benchmark.py>) writes `19-max_flow_benchmark.json`, which is ignored by git. Conditions:
- **Machine and build:** i9-11900H on a shared machine, GCC 16.2.1, `-O2 -DNDEBUG`, seed 20260927.
- **Runs:** one checked warm-up run, then 5 samples per engine; small networks repeat 2,000 times per sample.
- **Timing:** network construction is included; input generation is excluded.
- **Checking:** every timed result equals the Dinic value, and that value is certified once by an equal-capacity residual cut.
- **Reference:** a minimal recursive ACL-style Dinic written inside the driver (BFS from `s`, multi-push DFS from `t`).
- **Memory:** O(n + m) arcs for every engine.

Medians in milliseconds:

| Workload | n | m | Dinic `flow` | `capacityScalingDinic` | `pushRelabel` | ACL-style reference |
|---|---|---|---|---|---|---|
| small random, capacities [1,100] | 50 | 300 | 0.015 | 0.017 | 0.016 | 0.014 |
| sparse random, capacities [1,1e9] | 20,000 | 100,000 | 16.6 | 86.4 | 20.6 | 15.5 |
| unit bipartite (k = 50,000) | 100,002 | 300,000 | 213 | 197 | 183 | 213 |
| dense random p = 1/2, [1,1e6] | 400 | 80,101 | 2.11 | 6.98 | 2.58 | 1.79 |
| undirected 300×300 grid, [1,1e6] | 90,000 | 179,400 | 15.3 | 56.6 | 25.9 | 14.7 |
| chain plus side arcs (adversarial, about n phases) | 20,000 | 26,666 | 3,612 | 768 | 1,199 | 3,546 |

Headlines:
- The iterative Dinic tracks the recursive reference within noise on bipartite and grid workloads, and is 7–18% slower on sparse and dense ones, in exchange for stack safety.
- An earlier single-path iterative blocking flow took 105 s on a 200,000-vertex version of the adversarial chain, because it re-walked the shared prefix for every augment. It was replaced by the multi-push frame stack.
- Push-relabel without global relabelling took 405 ms on the grid. Probing the global-relabel frequency (relabel work above `GLOBAL * (n + m)`, three runs each, push-relabel only) gave these medians in ms for GLOBAL = 1, 2, 4, 8 and never:
  - sparse: 17, 18, 22, 28, 29
  - bipartite: 168, 170, 138, 161, 177
  - grid: 25, 28, 27, 29, 405
  - chain: 1283, 1155, 968, 990, 2638

  `GLOBAL = 4` was kept.
- Capacity scaling and push-relabel are the engines to use on adversarial deep-phase inputs; Dinic is the default.
- These are shared-machine observations. The header has no automatic engine dispatch and makes no universal speed claim.
- `edmondsKarp` and `augmentPath` (added 2026-10-12) are not benchmarked: Edmonds–Karp's O(n * m * (n + m)) is a separate algorithm, not a candidate default.

## Sources

See [00-sources.md](00-sources.md#pages-fetched-on-2026-10-10-p044-max-flow) for the sweep and [00-notes.md](00-notes.md#p044-max-flow-omissions) for the candidates left out with reasons. The algorithms follow:
- ACL `mf_graph`: API and multi-push DFS from `t`.
- KACTL Dinic and PushRelabel: reverse capacity and scaling.
- cp-algorithms and OI Wiki: bounds (Edmonds–Karp, Dinic), HLPP with gap and global relabel.
- hitonanode `maxflow_pushrelabel`: the excess-return phase and global-relabel frequency.
- maspypy `maxflow`/`incremental_maxflow`: decomposition and capacity decrease.

The code was written independently; no implementation was copied.

Legacy material inspected:
- `OLD/[1] algorithms.cpp:348-469` (`EdmondsKarp`): its features map to `MaxFlow` (Legacy mapping); the adapter that kept its names was removed on 2026-10-12.
- `OLD/Team Notebook/src/algsbetter.cpp:1940` and `algs.cpp:1712` (`Dinic`, recursive single path), and `test.cpp:568`. Their features are covered by `flow`.
- `algsbetter.cpp:2122` and `algs.cpp:1811` (`PushRelabel`, dense O(n^3) matrix, highest-label set rescanned each round). Covered by `pushRelabel`, which is sparse, has the gap and global-relabel heuristics, and returns a valid flow.
- `OLD/Team Notebook/src/graph/old_dinic.cpp`, `old_ford-fulkerson-d.cpp` and `old_ford-fulkerson-u.cpp`. Covered by `flow` and by `rev_cap` for undirected edges.

`FlowWithDemands` and `MaxFlowMinCut` (a min-cost SPFA) in the same files belong to rows 30 and 20. Archive bytes are untouched.

## Limits and handoffs

- Floating-point capacities, push-relabel with `limit`, residual export and the fewest-edge min cut are documented reductions or omissions in the notes.
- Lower bounds and circulations: row 30. Min-cost flow: row 20. Matching wrappers: rows 21 and 82. Gomory–Hu: row 46. Global min cut: row 47. Edge- and vertex-disjoint path extraction: row 45 (it can use `pathDecomposition` with unit capacities).

## History

- 2026-10-10: original P044; full suites (seed 20260927, 23,926,841 checks, 22 probes) passed on g++ and g++-14, stress (seed 1), benchmark and integration with sanitizers passed; independent review, 6 findings fixed.
- 2026-10-12: prune, removed the `EdmondsKarp` legacy adapter (cases moved to `MaxFlow` on `Graph::fromMatrix` and to `augmentPath`, four legacy probes replaced) and added `edmondsKarp` and `augmentPath`; full suite passed on g++ and g++-14, stress passed.
