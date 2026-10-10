# 19-max_flow.hpp — evidence

Owned by package P044 / GR08. The header replaces the byte-for-byte legacy `EdmondsKarp` extraction with one residual network, `MaxFlow<Cap>`, carrying three engines (Dinic, capacity-scaling Dinic, highest-label push-relabel), dynamic capacity changes, cut witnesses and flow decomposition, plus `EdmondsKarp` as a legacy adapter with its original names and fields. Contest profile: no SIMD, no Barrett/Montgomery, so no `Compact` twins.

## Contracts

### MaxFlow<Cap>

- `Cap` is a signed integer type (`int`, `lng`, `lll`; checked by `static_assert`). Vertices are `0..n-1`. Loops and parallel edges are allowed. A loop never carries flow from an engine.
- `addEdge(u, v, cap, rev_cap = 0)` returns sequential edge IDs from 0 and asserts `0 <= cap`, `0 <= rev_cap` and `cap + rev_cap <= MAX`. It stores one arc pair: the forward arc starts with `cap`, the reverse arc with `rev_cap`. With `rev_cap = cap` this is an undirected edge using two arcs, not four. Edges may be added at any time, including after a flow; the next engine call resumes from the current residual network (incremental insertion).
- `MaxFlow(const G &)` accepts `Graph` or `CsrGraph`. Edge `i` of the graph becomes edge `i` with capacity `w`, and `rev_cap = w` when the graph is undirected. Weights must lie in `[0, MAX]` (asserted before the cast, so a narrower `Cap` never truncates silently), and `2w <= MAX` for undirected graphs.
- `getEdge(i)` returns `{from, to, cap, flow}`. Here `flow` is the net flow from `from` to `to`, in `[-rev_cap, cap]`; it is negative only when `rev_cap > 0`. `edges()` returns every `getEdge(i)` in ID order.
- `changeEdge(i, cap, flow)` is a raw set (ACL semantics): it asserts `0 <= cap`, `cap + rev_cap <= MAX` and `-rev_cap <= flow <= cap`, keeps `rev_cap`, and does not repair conservation; the caller owns the consequence.
- `flow(s, t, limit = MAX)` requires `s != t` and `limit >= 0`. It adds `min(remaining max flow, limit)` to the current flow and returns that amount. Repeated calls continue the same flow, so the sum of returned values is the flow value. The result never overflows: augmentation stops at `limit`, so with the default limit it returns `min(max flow, MAX)`. Use `Cap = lll` when the value can exceed 2^63 - 1.
- `capacityScalingDinic(s, t, limit = MAX)` has the same contract and result value as `flow`. It runs Dinic phases restricted to residual arcs of capacity `>= delta`, with `delta` going from the largest power of two `<=` the largest residual capacity down to 1.
- `pushRelabel(s, t)` adds a maximum flow to the current flow and returns the added value. Precondition (asserted with `__builtin_add_overflow`): the residual capacities of the non-loop arcs leaving `s` sum to at most `MAX`, which bounds every excess. Under `NDEBUG`, a violated supply bound or `s == t` returns 0 before touching the network, so no signed overflow and no out-of-range label occur. Phase two returns stranded excess to `s`, so afterwards `getEdge`, `minCut` and `pathDecomposition` see a valid maximum flow.
- `changeCapacity(i, cap, s, t)` requires the current flow to come from the zero flow through this struct's `flow`, `capacityScalingDinic`, `pushRelabel` (with or without `limit`), `addEdge` and earlier `changeCapacity` calls with the same `s`, `t`, or more generally to be a valid flow that decomposes into s→t paths and cycles only. An arbitrary conserving flow set by `changeEdge` that carries a t→s path is outside the domain: the cancel step assumes the excess at `u` came from `s` (reviewer counterexample: `3->1->2->0` with `s = 0`, `t = 3`, value −1). It sets edge `i`'s forward capacity to `cap` (with `rev_cap` unchanged), leaves a maximum flow and returns the signed change of the flow value. A decrease below the edge's flow first reroutes the excess `u -> v` through the residual network, then cancels the remainder along `u -> s` and `t -> v`, then re-augments `s -> t`.
- `minCut(s)` returns the vertices reachable from `s` in the residual network. After a maximum flow (not a `limit`-truncated one) this is the source side of a minimum cut. `minCutEdges(s)` lists the IDs of edges whose forward arc (`cap > 0`) crosses from that side to the other, or whose reverse capacity (`rev_cap > 0`) crosses the same way, in ID order. Their capacities sum to the cut value.
- `pathDecomposition(s, t)` requires a valid flow (conservation outside `s` and `t`; any value, any flow through `t` or into `s`) and does not mutate the network. It returns `(amount, edge IDs)` pairs, each a simple s→t path following each edge in its flow direction, with each edge used at most its |flow|. The amounts sum to `max(value, 0)`. Flow cycles, including cycles through `s` or `t`, are cancelled and dropped. `s == t` returns no paths under `NDEBUG`.
- Workspace vectors `level`, `it`, `que` are members, reused by every engine call. The struct is single-threaded.

### Engines and correctness

- **Dinic (`flow`, internal `dinic`).** Each phase runs a BFS from `s` over arcs with residual `>= delta`, stopping once `t` is labelled; every vertex at a level below `level[t]` has been labelled by then. The blocking flow is ACL's multi-push DFS from `t` back to `s` over arcs to level − 1, simulated with an explicit frame stack `(v, up, got)`. Each frame pushes into one child at a time and adds the child's result to its arc pair. It returns once `got == up` (keeping its current arc) or when its arcs are exhausted (the vertex is then marked dead with level `n`). There is no recursion, so a 500,000-vertex path is stack-safe. Bound: O(n^2 * m) in general, O(m * sqrt(m)) for unit capacities.
- **Push-relabel.** It saturates every residual arc leaving `s`, then assigns exact labels by a global relabel: the distance to `t` over reverse residual arcs, avoiding `s`; otherwise `n` plus the distance to `s`; otherwise `2n - 1`. Each of these is at least any valid labelling, so labels never decrease and the standard bounds hold.
  - It discharges the highest active label first, keeping a current arc per vertex.
  - The gap heuristic uses per-label doubly linked lists of vertices with labels below `n`. When the last vertex leaves label `old`, every vertex with a label in `(old, n)` moves to `n + 1`. The relabelled vertex is the highest active one, so none of the lifted vertices is active, and the cost is proportional to the vertices lifted.
  - A global relabel reruns after every `GLOBAL * (n + m)` units of relabel work (`GLOBAL = 4`, measured below).
  - The run ends when no vertex other than `s` and `t` has excess. Every vertex then conserves flow, and validity of `h[s] = n` rules out an s–t residual path, so the flow is maximum. Bound: O(n^2 * sqrt(m)).
  - Every vertex with positive excess has a residual path to `s`, so a relabel always finds an arc; the `h[u] >= 2n` break is a defensive bound only.
- **Capacity scaling.** Phase `delta` leaves no s–t path of residual `>= delta`, so the remaining flow is below `m * delta`, and each phase does O(m) augmentations. Bound: O(n * m * log(U)).
- **changeCapacity.** With excess `d` at `u` and deficit `d` at `v`, a flow decomposition of the pseudo-flow shows that whatever does not reroute from `u` to `v` reaches `s` from `u` and reaches `v` from `t`. The cancellations therefore always complete. The final `s -> t` augmentation restores maximality, because a cancelled path can unblock another route. When the previous flow was maximum, all flow moved is at most `d = |cap - old cap|`, so the cost is O((n + m) * (d + 1)), capped by Dinic's O(n^2 * m).
- **pathDecomposition.** `need` starts as the net inflow of `t` (computed in `lll`). A walk from `s` follows edges with remaining flow. Reaching `t` while `need > 0` records a path of amount `min(bottleneck, need)` and lowers `need`; with `need = 0`, `t` is an ordinary vertex, so flow passing through `t` is cancelled as part of cycles. Revisiting a vertex on the stack cancels that cycle. Conservation guarantees that every vertex reached with positive inflow (other than `t` while `need > 0`) has positive outflow, and when `s` runs out of outflow the remaining net value is at most 0, so the recorded amounts total `max(value, 0)`. Each record zeroes an edge or `need`, so the cost is O(n * m), plus the output.

### EdmondsKarp (legacy adapter)

Original bytes remain in `OLD/[1] algorithms.cpp:348-469`.
- The adapter keeps the constructor `(n, adj, cap)` over vertices `0..n` (`adj` and `cap` have size `n + 1`; `cap` is a dense matrix) and the public fields `n, adjm, adjl, ocap, ncap, max_flow, add_flow, par, in_S, cut_set`. A pair listed several times in `adj[u]` counts once with capacity `cap[u][v]`. Unlisted matrix entries are ignored.
- The constructor asserts sizes, endpoint ranges and nonnegative listed capacities, and `ocap[u][v] + ocap[v][u] <= 2^63 - 1` for every pair, so that residual sums are representable.
- `augmentFlow(s, t)` returns the bottleneck of one shortest augmenting path in `ncap` (0 if none) and leaves the BFS tree in `par`. `ncap` is not changed.
- `getMaxFlow(s, t)` runs Dinic on a fresh `MaxFlow<lng>` (better than Edmonds–Karp's O(n * m^2)) and writes the residual matrix back into `ncap`, then calls `augmentFlow`, which returns 0 and leaves `par` as the residual reachability tree. `s == t` gives 0. The result is exact when the max flow is at most 2^63 - 1.
- `getMinCut(s, t)` sets `in_S` from that tree and lists `cut_set` as the original pairs `(u, v)` with `u` in S and `v` not in S, ordered by `u` and then `v`. This includes zero-capacity listed pairs, as the legacy did.

Legacy defects repaired:
- `ncap` was empty before the first `getMaxFlow`, so an early `augmentFlow` read out of bounds.
- The initial bottleneck was `INF64` instead of the true maximum.
- There was no `s == t` handling and no overflow guard.

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
| `changeCapacity` | 1–6 random increases and decreases per network, starting from `flow`, `pushRelabel`, limited `flow` or limited scaling (optionally followed by `pushRelabel`), mixed with `addEdge` after flow; large chain cut to 0 and restored | Brute min cut of the modified network; returned delta equals the change of value; certificate |
| `changeEdge` | Raw set with negative flow on reverse-capacity edges | `getEdge` round trip |
| `minCut`, `minCutEdges` | After every maximum flow | Side contains `s` and not `t`; cut capacity equals the flow value; exact edge-ID list |
| `pathDecomposition` | After every maximum flow, including `lll` and reverse capacities; one arbitrary valid flow per small round, set by `changeEdge` from random s→t paths, t→s paths and cycles through any vertex (negative values, flow through `t`, loops) | Simple s→t paths along flow directions; per-edge usage within absolute flow; amounts positive and summing to `max(value, 0)` |
| `MaxFlow(Graph)`, `MaxFlow(CsrGraph)` | Random directed and undirected multigraphs (`lng` and `int`) | Brute min cut with `rev_cap = w` for undirected |
| `EdmondsKarp` | Every third random network (1,334 in full mode; those with max flow above 2^63 - 1 skipped) with duplicate listings and loops; `s == t` | `augmentFlow` path length equals an independent BFS distance, bottleneck exact; brute min cut; residual pair sums and conservation; exact ordered `cut_set` |
| Preconditions | 22 checked assertion probes (sizes, ranges, negative or overflowing capacities, `s == t`, flow bounds, supply overflow, legacy sizes, ranges and pair overflow) | Expected assertion abort |

Mutation check (scratch copies under `/tmp`, quick mode, not part of the suite): 19 mutants.
- 18 were killed: changeCapacity dropping the `t -> v` cancel or the final augmentation, a decomposition direction swap or cycle off-by-one, the scaling loop skipping `delta = 1`, `minCutEdges` or `getEdge` ignoring reverse capacity, `changeEdge` writing the wrong slot, the legacy cut ignoring direction, the `Graph` adapter dropping reverse capacity, `augmentFlow` returning a stale bottleneck, Dinic ignoring `limit` or overrequesting from a child, global relabel ignoring residual capacity, push overshooting, a relabel that keeps its current arc, and `pathDecomposition` ignoring or not capping the net inflow of `t` (killed by the arbitrary-flow corpus).
- The survivor sets gap-lifted labels to `old + 1`. That changes labels only for vertices that cannot reach `t`, so it affects performance, not the result. Removing the dead-end pruning or the reroute in `changeCapacity` (earlier rounds) also survives, as expected for optimizations.

Finite tests complement the arguments above; they do not prove all inputs.

## Commands and results

2026-10-10, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `19-max_flow_tester.py --mode full --seed 20260927` | PASS 3 configurations, 27,983 cases, 23,926,841 checks, 22 probes; MEMORY peak 780 MB (pre-flight quick 523 MB) |
| `CXX=g++-14 19-max_flow_tester.py --mode full --seed 20260927` | PASS (GCC 14.4.1) 3 configurations, 27,983 cases, 23,926,841 checks, 22 probes; MEMORY peak 852 MB |
| `19-max_flow_tester.py --mode stress --seed 1` | PASS 3 configurations, 61,183 cases, 71,421,108 checks, 22 probes; MEMORY peak 936 MB |
| `19-max_flow_benchmark.py` | PASS, every result checked; MEMORY peak 333 MB |
| `02-integration.py` and `02-integration.py --sanitizers` | PASS: 111 standalone/aggregate headers (including `19-max_flow.hpp` alone and `99-all.hpp`), scalar/available-AVX2 multi-TU, workspace, sanitizer self-tests; MEMORY peak 2694 MB |
| `03-consistency.py` (worktree wrapper `/tmp/p044mut/consistency_wt.py`: identical checks, tolerating only archive files that git reports ignored and absent from the worktree) | No P044 errors. Two remaining errors are outside P044: the absent ignored `96-Local Testing/01-Core/18-bitset_benchmark.jsonl`, and `QUICK` in `01-run.py` lacking `16-poly_tester.py` (introduced by commit 8f54370, P231) |
| `NDEBUG` boundedness probes (ad hoc, `-O1 -DNDEBUG -fsanitize=address,undefined`, 512 MB cap): `pathDecomposition(1, 1)`, `pushRelabel(1, 1)`, two `LLONG_MAX` source edges into `pushRelabel` | Return empty, 0 and 0; no sanitizer report; peak 25 MB |
| `@reviewer` (independent, 2M-round own `changeCapacity` stress from engine-produced flows) | 2 confirmed `NDEBUG` defects (`pathDecomposition`/`pushRelabel` with `s == t`), 2 contract mismatches (`changeCapacity`, `pathDecomposition` on flows carrying t→s paths), supply-overflow UB, `Graph` weight narrowing: all fixed above; the `pathDecomposition` algorithm now handles every valid flow |

```bash
python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/04-Graphs/19-max_flow_tester.py' --mode stress --seed 1
python3 '96-Local Testing/04-Graphs/19-max_flow_benchmark.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
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

## Sources

See [00-sources.md](00-sources.md#pages-fetched-on-2026-10-10-p044-max-flow) for the sweep and [00-notes.md](00-notes.md#p044-max-flow-omissions) for the candidates left out with reasons. The algorithms follow:
- ACL `mf_graph`: API and multi-push DFS from `t`.
- KACTL Dinic and PushRelabel: reverse capacity and scaling.
- cp-algorithms and OI Wiki: bounds, HLPP with gap and global relabel.
- hitonanode `maxflow_pushrelabel`: the excess-return phase and global-relabel frequency.
- maspypy `maxflow`/`incremental_maxflow`: decomposition and capacity decrease.

The code was written independently; no implementation was copied.

Legacy material inspected:
- `OLD/[1] algorithms.cpp:348-469` (`EdmondsKarp`), adapted as above.
- `OLD/Team Notebook/src/algsbetter.cpp:1940` and `algs.cpp:1712` (`Dinic`, recursive single path), and `test.cpp:568`. Their features are covered by `flow`.
- `algsbetter.cpp:2122` and `algs.cpp:1811` (`PushRelabel`, dense O(n^3) matrix, highest-label set rescanned each round). Covered by `pushRelabel`, which is sparse, has the gap and global-relabel heuristics, and returns a valid flow.
- `OLD/Team Notebook/src/graph/old_dinic.cpp`, `old_ford-fulkerson-d.cpp` and `old_ford-fulkerson-u.cpp`. Covered by `flow` and by `rev_cap` for undirected edges.

`FlowWithDemands` and `MaxFlowMinCut` (a min-cost SPFA) in the same files belong to rows 30 and 20. Archive bytes are untouched.

## Limits and handoffs

- Floating-point capacities, push-relabel with `limit`, residual export and the fewest-edge min cut are documented reductions or omissions in the notes.
- Lower bounds and circulations: row 30. Min-cost flow: row 20. Matching wrappers: rows 21 and 82. Gomory–Hu: row 46. Global min cut: row 47. Edge- and vertex-disjoint path extraction: row 45 (it can use `pathDecomposition` with unit capacities).
- Exact GCC 14.2 and the Windows build were not run.
