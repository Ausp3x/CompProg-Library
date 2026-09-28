# P011 / GR26 — functional graphs

## Contracts and scope

`11-functionalgraph.hpp` owns static partial successor functions. The canonical input is `vector<int>` with size at most `INT_MAX`; each entry is a vertex in `[0,n)` or `-1`. The `Graph`/`CsrGraph` adapter requires directed input with at most one outgoing arc per vertex. Two parallel arcs are invalid even when their targets agree. Edge weights are ignored; use a separate vertex-indexed label vector for aggregates. The construction owns its data, so later input edits do not invalidate queries. Public preprocessing arrays are read-only. Reassignment rebuilds the snapshot.

Missing successors terminate the walk. They do not create a shared absorbing vertex or an implicit self-loop. A terminal vertex is real: its zero-step jump and self-distance succeed. Taking one further step gives `-1`; two terminated walks never meet at `-1`. Empty graphs can be constructed and have no cycles; any vertex query still requires a valid vertex.

`FunctionalGraph` exposes `successor`, connected `component`, `cycle` ID (`-1` when terminating), `entry` (first cycle vertex or terminal), `depth` (edges to entry), and `position` (`-1` off the cycle). Each `cycles[c]` follows successors starting at that cycle's smallest vertex. Cycle components are numbered by increasing smallest cycle vertex, followed by terminating components in increasing terminal-vertex order. `tin/tout` are half-open subtree intervals of the reverse forest obtained by deleting cycle arcs. Each cycle vertex and each terminal is a separate reverse-tree root. `up` stores actual successor powers, retaining `-1`; it has `max(1,bit_width(n))` levels.

`jump(u,k)` accepts the entire unsigned 64-bit count range. `distance(u,v)` returns the shortest directed walk length, or `-1`; `reachable` is its Boolean form. Self-distance is zero. `firstMeeting(u,v)` returns the first integer time at which walkers moving one edge per tick occupy the same real vertex, or `-1`. This synchronous contract differs from arbitrary intersection of the two reachable sets.

`cycleAggregates(values,identity,op)` folds each canonical stored cycle in successor order. `cycleAggregate(u,values,identity,op)` rotates the fold to `entry[u]` and excludes all tail labels. Its result is `{false,identity}` for terminating components. Label count must equal n. These direct variants require no aggregate preprocessing and work with ordered, noncommutative monoids.

`FunctionalGraphFold<T,Op>` owns a graph and label snapshot and preprocesses ordered aggregate blocks. `op` must be a const-callable associative binary operation with the supplied two-sided identity. The caller must ensure that its arithmetic and storage support all intermediate aggregates. `fold(u,k)` folds up to k **visited vertices**, starting with u, and returns `{next,count,value}`. The terminal is included once, then `next=-1`; `count<k` explicitly reports early termination. A zero count returns u and the identity. Cyclic walks always consume the full requested count, including `UINT64_MAX`. Labels can also represent outgoing edges, with the terminal label chosen as identity if there is no edge to count. `cycleAggregate(u)` provides the rotated full-cycle result using the precomputed blocks. The helper `segment(u,k)` requires `0<=k<=n` and at least k real visited vertices; its result uses the same convention.

`functionalOrbit(next,start)` is an independent Floyd alternative for one starting vertex. It uses constant auxiliary memory and returns `{entry,tail,cycle_length}`, with zero cycle length for termination. Only reached endpoint preconditions are checked; the full vector's valid successor domain remains the caller's contract. It deliberately avoids preprocessing the whole graph when only one orbit is needed.

## Algorithms, bounds and correctness

Indegree-zero peeling removes every vertex outside cycles. In a terminating component every vertex eventually peels; in a cyclic component the cycle vertices retain one incoming cycle edge. Scanning the survivors discovers exactly the directed cycles. Reversing the peel order propagates the already known component, cycle entry and distance through each tail. A separate iterative traversal of the reverse trees supplies the subtree intervals. These steps take O(n) time and memory. Successor doubling raises the complete constructor and stored memory to O(n log(n+1)). All traversals are iterative.

For jumps confined to the tail, binary decomposition of k selects successor powers. After the tail, a cyclic walk uses its cycle position and the remainder modulo the cycle length; a terminating walk returns absence. Hence full 64-bit counts do not require 64 table levels. Query time is O(log(n+1)), with O(1) work after reaching a cycle.

For a target outside a cycle, it is reachable exactly when it is an ancestor in the source's reverse tree; the answer is the depth difference. A cycle target in the same component is reached after the source's tail plus the forward cycle offset. Thus distance/reachability are O(1). The distance is at most n−1. Component, depth and timestamp counters fit `int`; cycle-position subtraction and phase comparisons use `lng` before potentially large intermediate differences.

If two walkers have different depths, they cannot meet while either is in a tail: occupying a common tail vertex would give them equal remaining depths. Terminating walks therefore never meet in this case. On a cycle their phase is `position[entry]-depth` modulo cycle length. Equal phases first coincide at the larger tail depth; unequal phases never coincide. Walkers with equal depth can meet only when their entries agree, and binary lifting finds the last unequal pair before their paths merge. Beyond the merge both powers agree, so this lifting never selects a missing vertex. The first-meeting query uses O(log(n+1)) time and returns at most n−1 for a present meeting.

Aggregate table entries join two consecutive blocks in order, so induction gives the correct fold of every power-of-two segment. Selecting those blocks while advancing the starting vertex preserves order for arbitrary segment lengths. A long cyclic fold consists of the tail, whole laps of one fixed rotated cycle, and a final prefix. Repeated squaring computes powers of that cycle aggregate; associativity suffices because it is always the same ordered cycle product. It does not require commutativity or inverses. Missing successors truncate before a nonexistent label is accessed. Construction uses O(n log(n+1)) monoid operations/storage; a walk fold uses O(log(n+1)+log(k+1)) operations, a preprocessed cycle fold O(log(n+1)), direct one-cycle aggregation O(cycle length), and direct all-cycle aggregation O(n). These bounds count constant-size monoid values; variable-sized labels add their operation and payload costs.

Floyd's two-speed pointers either encounter termination or meet on a cycle. Resetting one pointer to the start identifies the entry after exactly the tail length, then one lap measures cycle length. A terminating orbit is retraced to identify its last real vertex. The time is O(tail+cycle_length+1) and auxiliary memory is O(1).

## Research and ownership

The following material was inspected on 2026-09-27. The header was independently implemented; no external implementation was copied.

| Source | Inspected scope and use |
|---|---|
| Antti Laaksonen, [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, sections 16.3–16.4 (printed pages 154–156) | Successor components, doubling recurrence, Floyd entry/cycle recovery and constant-memory single-orbit tradeoff. The new jump method reduces post-tail counts modulo cycle length, and explicitly handles missing successors. |
| [USACO Guide, Functional Graphs](https://usaco.guide/silver/func-graphs), current article retrieved 2026-09-27 | Rooted-tree/cycle component model, single-orbit Floyd and all-vertex decomposition examples. Its single-orbit and whole-graph workloads are kept distinct. |
| [maspypy, graph/functional_graph.hpp](https://maspypy.github.io/library/graph/functional_graph.hpp), current source retrieved 2026-09-27 | `dist`, `jump`, `collect_cycle`, `meet_time` and aggregate APIs. The reference uses a cut-cycle tree and group inverses for some folds; this implementation uses explicit cycles/reverse-tree intervals and monoid blocks, supports noncommutative operations without inverses, and adds termination semantics. |

Static all-vertex queries, ordered cycle/walk aggregates and one-orbit constant-memory analysis are complete scope here. Successor updates and fully dynamic functional-graph maintenance require a separate dynamic-tree design; they are outside this static batch. Constant-query-time level-ancestor constructions and arbitrary-speed/collision schedules are distinct specialized extensions, not claimed by these APIs. No original Floyd paper was independently reviewed, and no judge acceptance or automatic submission is claimed. This was a planned header, so no active legacy implementation was removed.

## Feature-to-test map and completed verification

[11-functionalgraph_tester.py](<../96-Local Testing/04-Graphs/11-functionalgraph_tester.py>) runs the actual header through the shared graph driver, with [independent C++ oracles](<../96-Local Testing/04-Graphs/11-functionalgraph_tester.cpp>). Failure reporting retains seed, mode, configuration, successor map, operation and expected/actual success, and the driver reports command, crash and timeout details. Oracles remain active under `-DNDEBUG`.

| Features | Independent verification |
|---|---|
| Cycle/tail decomposition, canonical ordering and components | All 8,477 partial successor maps through n=5; visited-vertex sequences find first repeats/termination independently, cycle rotations are normalized by their minimum vertex, and terminal components are checked separately. |
| Jump, distance and reachability | Explicit unique-orbit positions and membership on every vertex pair; counts include zero, n/n+1, 2^63−1, 2^63, `UINT64_MAX−1` and `UINT64_MAX`. |
| Synchronous first meeting | Explicit simulation in the product state space `(u,v)` until coincidence, termination or a repeated pair; no depth/phase formula is used by the oracle. |
| Reverse-tree intervals, compact powers and adapters | Every Graph/CSR/vector construction on every small case; independent ancestor walks, exact table powers and component order; all ignored weight extremes and direct `successors` helper. |
| Floyd single-orbit analysis | Compare returned entry/tail/cycle length to visited sequences for every small starting vertex; large terminating/cyclic orbit fixtures. |
| Direct and preprocessed cycle aggregates | Literal successor-order folds, with canonical and entry-rotated order; terminal false/identity result; strings and noncommutative 2×2 matrices. |
| Walk folds and bounded segments | Literal walks for every count through 2n+3, including terminal consumption once, zero counts, lengths beyond termination and all available segment lengths. |
| Full-width ordered folds | A separate fixed-64-level reference stores actual truncated block lengths, without cycle decomposition or repeated-cycle powers. Exact 128-bit signed sums cover every exhaustive map; noncommutative matrix products cover 700 random maps/permutations. |
| Boundaries and ownership | Default/explicit empty constructors, singleton terminal/self-loop, disconnected components, copies/moves, input mutation, snapshot/reassignment, labels/graph lifetime; 200,000-vertex terminal chains, cycles and long tails. |
| Preconditions | 28 checked subprocess probes cover invalid successors/vertices, empty queries, directed/outdegree/parallel-arc adapter requirements, label count, Floyd reached endpoints, and segment length/availability. |

Fresh full verification on 2026-09-27 used GNU++20, GCC 16.2.1, and seed 20260927:

```text
python3 '96-Local Testing/04-Graphs/11-functionalgraph_tester.py' --mode full --seed 20260927
PASS optimized -O2 -DNDEBUG: 9,178 cases, 8,243,815 checks
PASS checked -O0 -g -D_GLIBCXX_DEBUG: same cases/checks, 28 assertion probes
PASS ASan/UBSan -O1 -g -D_GLIBCXX_ASSERTIONS: same cases/checks
```

The sandbox's LeakSanitizer/ptrace restriction prevented the initial sanitizer execution. An approved outside-sandbox retry using `--configuration ASan-UBSan` passed the same full corpus with leak detection retained. Quick exhausts maps through n=3, adds 80 random cases and 20,000-vertex fixtures, and omits sanitizers. Full exhausts n=5, adds 700 random cases and 200,000-vertex fixtures. Stress exhausts n=6, adds 5,000 random cases and 500,000-vertex fixtures; it was implemented but not run. Exact GCC14 execution is not claimed.

Independent implementation reviews of decomposition/jumps/distance/Floyd and of meeting/monoid-fold logic found no correctness or overflow issue; the empty-table-level comment was clarified. Final standalone/aggregate/multiple-translation-unit checks passed as recorded in [P011 integration](95-p011.md). No owned feature or verification gap remains.
