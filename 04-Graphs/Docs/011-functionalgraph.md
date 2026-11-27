# 011-functionalgraph.hpp — Evidence

Owned by package P011 / GR26 (functional graphs); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

## Contracts

### Scope

The row covers static all-vertex queries, ordered cycle and walk aggregates and one-orbit constant-memory analysis. Successor updates and fully dynamic functional-graph maintenance need a dynamic-tree design and are outside this static row. Generic Floyd and Brent cycle finding on an arbitrary step function is Miscellaneous `014-cyclefinding.hpp` ([00-notes.md](00-notes.md#ownership-boundaries)).

### FunctionalGraph

The header owns static partial successor functions. The canonical input is a `vector<int>` of size at most `INT_MAX` whose entries are vertices in `[0,n)` or `-1`. The `Graph`/`CsrGraph` adapter requires directed input with at most one outgoing arc per vertex; two parallel arcs are invalid even when their targets agree. Edge weights are ignored; aggregates use a separate vertex-indexed label vector. The structure owns its data, so later input edits do not invalidate queries. Public preprocessing arrays are read-only; reassignment rebuilds the snapshot.

A missing successor ends the walk; it creates no shared absorbing vertex or implicit self-loop. A terminal vertex is real: its zero-step jump and self-distance succeed, one further step gives `-1`, and two terminated walks never meet at `-1`. An empty graph can be constructed and has no cycles; every vertex query still requires a valid vertex.

`FunctionalGraph` exposes `successor`, connected `component`, `cycle` ID (`-1` when terminating), `entry` (first cycle vertex or terminal), `depth` (edges to entry) and `position` (`-1` off the cycle). Each `cycles[c]` follows successors from that cycle's smallest vertex. Cycle components are numbered by increasing smallest cycle vertex, then terminating components by increasing terminal vertex. `tin/tout` are half-open subtree intervals of the reverse forest obtained by deleting cycle arcs; each cycle vertex and each terminal is a separate reverse-tree root. `up` stores successor powers, keeping `-1`, in `max(1,bit_width(n))` levels.

`jump(u,k)` accepts the whole unsigned 64-bit count range. `distance(u,v)` returns the shortest directed walk length, or `-1`; `reachable` is its Boolean form; self-distance is zero. `firstMeeting(u,v)` returns the first time at which walkers moving one edge per tick occupy the same real vertex, or `-1`; this synchronous contract differs from intersecting the two reachable sets.

`jumpAll(k)` returns `jump(u, k)` for every vertex in O(n) time and memory for any `ulng` k. It visits vertices in reverse-forest preorder (`tin` order), keeps the current root path indexed by depth, answers `path[depth[u] - k]` when `k <= depth[u]`, and otherwise uses the O(1) cycle branch of `jump`. In preorder the last vertex seen at each smaller depth is the current vertex's ancestor there, which is the k-th successor.

### cycleAggregates and cycleAggregate

`cycleAggregates(values, mon)` folds each canonical stored cycle in successor order under the monoid `mon` (`S`, `op`, `e`; [stored-object contract](<../../02-Data Structures/Docs/00-monoids.md#monoid-contract>); formerly `(values, identity, op)`). `cycleAggregate(u, values, mon)` rotates the fold to `entry[u]` and excludes all tail labels; it returns `{false, e()}` for a terminating component. `M` is deduced from `mon`, so callers pass a preset object or `Monoid{id, op}`. The label count must equal n. These direct variants need no aggregate preprocessing and work with ordered, noncommutative monoids.

### FunctionalFoldResult and FunctionalGraphFold

`FunctionalGraphFold<M>(g, values, mon = {})` (formerly `<T, Op>` with `(id, op)`) owns a graph, a label snapshot and the monoid, kept as `m`, and preprocesses ordered aggregate blocks. `M` follows the [stored-object contract](<../../02-Data Structures/Docs/00-monoids.md#monoid-contract>): `op` associative with two-sided identity `e()`, static or `const` member functions. The caller's arithmetic and storage must hold every intermediate aggregate. `fold(u,k)` folds up to k **visited vertices**, starting with u, and returns `{next,count,value}`. The terminal is included once, then `next=-1`, and `count<k` reports early termination. A zero count returns u and `e()`. Cyclic walks always consume the full count, `UINT64_MAX` included. Labels can also represent outgoing edges, with the terminal's label the identity when there is no edge to count. `cycleAggregate(u)` gives the rotated full-cycle result from the precomputed blocks. The helper `segment(u,k)` requires `0<=k<=n` and at least k real visited vertices, with the same result convention.

`maxStep(u, pred, limit = ULLONG_MAX)` returns the largest `k <= limit` such that `pred(fold(u, k).value)` holds, never more than the vertices available on a terminating walk. `pred` must hold for `e()` (asserted) and be monotone: once false for some `k`, false for every larger `k`. The smallest `k` at which a monotone predicate becomes true (`step_until` in other libraries) is `maxStep` of its negation plus one, when that is at most `limit`. The search descends the doubling table greedily through the tail (a level is taken only if the step count is available, within the bound and the predicate still holds), then counts whole laps by exponential search on powers of the rotated lap aggregate, squaring only while the predicate accepts the next power, and finally descends the table through the last partial lap. Because squaring stops at the first rejected power, no fold longer than about twice the answer is formed, so a summing monoid whose answer fits does not overflow during the search. Cost: O(log(n + 1) + log(k + 1)) operations and predicate calls for answer k; O(log(k + 1)) temporary label values.

### FunctionalOrbit and functionalOrbit

`functionalOrbit(next,start)` is an independent Floyd alternative for one starting vertex, avoiding whole-graph preprocessing when only one orbit is needed. It uses constant auxiliary memory and returns `{entry,tail,cycle_length}`, with zero cycle length for termination. Only `next.size()` and `start` are asserted, once at entry; the successor domain (`next[u]` is `-1` or a vertex) is the caller's contract and is not checked per step, since checking the whole vector would cost O(n).

### Correctness and Bounds

Indegree-zero peeling removes every vertex outside cycles: in a terminating component every vertex peels, and in a cyclic component the cycle vertices keep one incoming cycle edge, so scanning the survivors finds exactly the cycles. Reversing the peel order propagates component, cycle entry and distance through each tail, and an iterative traversal of the reverse trees supplies the subtree intervals, all in O(n) time and memory. Successor doubling raises construction and storage to O(n log(n+1)). All traversals are iterative.

A jump inside the tail decomposes k into successor powers. Past the tail, a cyclic walk uses its cycle position and the remainder modulo the cycle length, and a terminating walk returns absence, so full 64-bit counts need no 64 table levels. Query time is O(log(n+1)), with O(1) work after reaching a cycle.

A target outside a cycle is reachable exactly when it is an ancestor in the source's reverse tree, at the depth difference. A cycle target in the same component is reached after the source's tail plus the forward cycle offset. So distance and reachability are O(1), and a distance is at most n−1. Component, depth and timestamp counters fit `int`; cycle-position subtraction and phase comparisons use `lng` before large intermediate differences.

Walkers of different depths cannot meet while either is in a tail, since a common tail vertex would give them equal remaining depths; so terminating walks never meet then. On a cycle their phase is `position[entry]-depth` modulo the cycle length: equal phases first coincide at the larger tail depth, unequal phases never. Walkers of equal depth can meet only when their entries agree, and binary lifting finds the last unequal pair before their paths merge; past the merge both powers agree, so the lifting never selects a missing vertex. `firstMeeting` takes O(log(n+1)) time and returns at most n−1 for a present meeting.

Each aggregate table entry joins two consecutive blocks in order, so induction gives the fold of every power-of-two segment, and selecting blocks while advancing the start preserves order for any length. A long cyclic fold is the tail, whole laps of one fixed rotated cycle, and a final prefix; repeated squaring computes powers of that lap aggregate, which needs only associativity, since it is always the same ordered product, and neither commutativity nor inverses. Missing successors truncate before a nonexistent label is read. Construction uses O(n log(n+1)) monoid operations and storage; a walk fold O(log(n+1)+log(k+1)) operations, a preprocessed cycle fold O(log(n+1)), a direct one-cycle aggregate O(cycle length), and direct all-cycle aggregation O(n). These bounds count constant-size monoid values; variable-sized labels add their own costs.

`functionalOrbit` runs Floyd's two-speed pointers, which Laaksonen (16.4, Sources) describes for the cyclic case; the argument here is self-contained and covers termination too. Write `x_0 = start`, `x_(i+1) = next[x_i]`, and `x_(i+1) = -1` once `x_i = -1`, as the code's `step(-1) = -1` does. After `t` rounds the pointers hold `x_t` and `x_(2t)`. A terminating orbit repeats no real vertex, since a repeat would cycle forever, so the pointers cannot meet at a real vertex and one of them reaches `-1` within `μ + 1` rounds, `μ` here the depth of the last real vertex; retracing from `start` finds that vertex. A cyclic orbit with tail length `μ` and cycle length `λ` never reaches `-1`, and `x_i = x_j` with `i < j` holds exactly when `i >= μ` and `λ` divides `j − i` (each vertex has one successor, so the walk repeats only from its first repeated vertex on). So `x_t = x_(2t)` with `t >= 1` holds exactly when `t >= μ` and `λ` divides `t`, and the first meeting comes at the least such `t`, at most `μ + λ`. Then one pointer restarts at `x_0` while the other continues from `x_t`, both one step per round: after `i` rounds they hold `x_i` and `x_(t+i)`, which are equal for `i = μ`, since `λ` divides `t`, and differ for `i < μ`, since `x_i` lies on the tail and `x_(t+i)` on the cycle. So they meet at the entry after exactly `μ` rounds, and one lap from it measures `λ`. O(μ + λ + 1) time, O(1) auxiliary memory.

## Feature-to-Test Map

[011-functionalgraph_tester.py](<../../96-Local Testing/04-Graphs/011-functionalgraph_tester.py>) runs the header through the shared contest runner, with [independent C++ oracles](<../../96-Local Testing/04-Graphs/011-functionalgraph_tester.cpp>). Quick exhausts maps through n=3, adds 80 random cases and 20,000-vertex fixtures, and omits sanitizers. Full exhausts n=5, adds 700 random cases and 200,000-vertex fixtures. Stress exhausts n=6, adds 5,000 random cases and 500,000-vertex fixtures.

| Features | Independent verification |
|---|---|
| Cycle and tail decomposition, canonical ordering and components | All 8,477 partial successor maps through n=5; visited-vertex sequences find first repeats and termination independently, cycle rotations are normalized by their minimum vertex, and terminal components are checked separately. |
| Jump, distance and reachability | Explicit unique-orbit positions and membership on every vertex pair; counts include zero, n and n+1, 2^63−1, 2^63, `UINT64_MAX−1` and `UINT64_MAX`. |
| Synchronous first meeting | Explicit simulation in the product state space `(u,v)` until coincidence, termination or a repeated pair; the oracle uses no depth or phase formula. |
| Reverse-tree intervals, compact powers and adapters | Every Graph, CSR and vector construction on every small case; independent ancestor walks, exact table powers and component order; all ignored weight extremes and the direct `successors` helper. |
| Floyd single-orbit analysis | Returned entry, tail and cycle length against visited sequences for every small starting vertex; large terminating and cyclic orbit fixtures. |
| Direct and preprocessed cycle aggregates | Literal successor-order folds in canonical and entry-rotated order; terminal false/identity result; strings and noncommutative 2×2 matrices through `Monoid`; a capturing-lambda sum modulo 7 and 10^9+7 (runtime state) for `cycleAggregates`. |
| `FunctionalGraphFold::fold`, `FunctionalFoldResult`: walk folds and bounded segments | Literal walks for every count through 2n+3, including terminal consumption once, zero counts, lengths beyond termination and all available segment lengths; the same walk oracle for `FunctionalGraphFold` over the capturing-lambda modular sum. |
| Full-width ordered folds | A separate fixed-64-level reference stores truncated block lengths, without cycle decomposition or repeated-cycle powers. Exact 128-bit signed sums on every exhaustive map; noncommutative matrix products on 700 random maps and permutations. |
| Boundaries and ownership | Default and explicit empty constructors, singleton terminal and self-loop, disconnected components, copies and moves, input mutation, snapshot and reassignment, label and graph lifetime; 200,000-vertex terminal chains, cycles and long tails. |
| `jumpAll` | Every vertex against the explicit orbit position for every count in the jump list plus every count through 2n+3, on every enumerated and random map; full-range counts on the 200,000-vertex terminal chain and cycle. |
| `maxStep` | Every enumerated map through n=4 and every random map: step-by-step walk oracle for every vertex, limits {0, 1, 2, n, 2n+3, `UINT64_MAX`}, nonnegative-sum thresholds {0, 1, 2, 5, n, 3n} and ordered string-prefix predicates (targets are walk folds of length 0, 1, n, 2n+3), so order errors are visible. The oracle stops after a step cap past which a still-true predicate means every further lap adds nothing, and then answers the limit. Large chain and full-range cycle fixtures. A mutation dropping the terminal-availability test was caught. |
| Preconditions | 29 checked subprocess probes: invalid successors and vertices, empty queries, directed, outdegree and parallel-arc adapter requirements, label count, Floyd start and size, segment length and availability, and `maxStep` vertex and identity-predicate requirements. |

## Mutation Testing

Record: mewt 4.0.0 on blob `13a0db78c468ec5de3c4e74f04d077d52108bab9` (2026-11-27, seed 1, whole file, sample of 983: 10 high, 25 medium, 15 low): 50 mutants, 48 caught (0 by timeout), 2 survived, 0 untested.

Stage times: quick 97 s, full optimized 11 s, full checked and ASan/UBSan 158 s. This is the header's first campaign, owed since P274 (user decision 2026-11-10).

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `CR 125`, the `functionalOrbit` size assertion removed; `COS 563`, its `<=` → `!=` | 11 | Unreachable: it binds only for a successor vector of more than `INT_MAX` entries (8 GiB), which no test can allocate, and the assertion vanishes under `NDEBUG`. |

## Commands and Results

Intel Core i9-11900H, Linux x86-64, GNU++20. The 2026-11-10 rows are library-wide runs (P274, seed 42, beside the other suites, build cache emptied before each cold run) with this tester's stage times; their command limit is 1500 s instead of 720 s, since the ASan-UBSan stress run took 685 s beside the other suites.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (GCC 16.2.1) | PASS, counts not recorded: optimized 0.2 s compile + 4.8 s run, checked 28.1 s + 10.0 s, ASan-UBSan 34.8 s + 104.9 s; peak 1127 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS, counts not recorded: 0.2 s + 6.0 s, 34.9 s + 13.8 s, 43.4 s + 86.0 s; peak 1113 MB |
| 2026-11-10 | `01-run.py --mode quick` | PASS: optimized 33.8 s + 0.3 s, checked 25.6 s + 3.8 s; peak 909 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` | PASS, counts not recorded: 0.1 s + 27.8 s, 11.3 s + 58.3 s, 14.3 s + 685.2 s; peak 1281 MB |
| 2026-10-17 | `011-functionalgraph_tester.py --mode full --seed 20261017` (GCC 16.2.1, CPython 3.14.7) | PASS, 3 configurations, 12,806,455 checks per configuration, 29 assertion probes; peak 805 MB |
| 2026-10-17 | `CXX=g++-14 011-functionalgraph_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 12,806,455 checks per configuration; peak 835 MB |
| 2026-10-17 | `011-functionalgraph_tester.py --mode stress --seed 7 --configuration optimized` | PASS, 222,137,296 checks (optimized build); peak 407 MB |
| 2026-10-17 | `02-integration.py --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace, sanitizer self-tests; peak 1834 MB |
| 2026-10-07 | Ten hand mutations, quick suites | Every non-equivalent mutant fails |

```bash
python3 '96-Local Testing/04-Graphs/011-functionalgraph_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/04-Graphs/011-functionalgraph_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/04-Graphs/011-functionalgraph_tester.py' --mode stress --seed 7 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode full
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Inspected on 2026-09-27. The header was implemented independently; no implementation was copied.

| Source | Inspected scope and use |
|---|---|
| Antti Laaksonen, [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, filed as `95-Resources/01-competitive_programmers_handbook-laaksonen.pdf` (SHA-256 `3a04509c7cc2bde6717907d52821c7dd159f92c3a73199f5f6664bf0218a947e`), sections 16.3–16.4 (printed pages 154–156) | Successor components, doubling recurrence, Floyd entry and cycle recovery, and the constant-memory single-orbit trade-off. This jump also reduces post-tail counts modulo the cycle length and handles missing successors. |
| [USACO Guide, Functional Graphs](https://usaco.guide/silver/func-graphs), retrieved 2026-09-27 | Rooted-tree and cycle component model, single-orbit Floyd and all-vertex decomposition examples. Its single-orbit and whole-graph workloads are kept distinct. |
| [maspypy, graph/functional_graph.hpp](https://maspypy.github.io/library/graph/functional_graph.hpp), retrieved 2026-09-27 | `dist`, `jump`, `collect_cycle`, `meet_time` and aggregate APIs. It uses a cut-cycle tree and group inverses for some folds; this header uses explicit cycles, reverse-tree intervals and monoid blocks, supports noncommutative operations without inverses, and adds termination semantics. |

Sources for `jumpAll` and `maxStep` (fetched 2026-10-07) are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions). The header was planned with no active legacy implementation.

## Limits and Handoffs

- `jump` takes O(log(n + 1)) by binary lifting. An O(1) form through row 85's ladder `LevelAncestor` on the reverse forest, plus the existing cycle branch, is benchmarked against it once row 85 is verified (user decision 2026-11-27, follow-up on P011).

## Aliases

Search aliases (P263, grab --find): 倍增, 基环树, 内向基环树, 函数图, 基环森林, 内向树, 判圈算法, 龟兔赛跑算法.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 9,178 cases, 8,243,815 checks, 28 probes) and integration passed on g++ 16; stress not run.
- 2026-10-07: re-audit, added `jumpAll` and `maxStep`; 2 findings fixed; full suite, optimized stress and integration passed on g++ 16; ten hand mutations checked; independent review: all eight package findings confirmed fixed, its own brute-force checks of every addition passed under ASan/UBSan/`_GLIBCXX_DEBUG`.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, peak 805/835/407 MB.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound and a raised command limit of 1500 s; full and stress inputs and checks unchanged; library-wide quick, full (g++ and g++-14) and stress runs passed. Header edit for `-Werror` without demotions: `[[maybe_unused]]` on names only an `assert` uses; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
- 2026-11-27: P011 limits recheck: the ownership boundaries moved to Contracts (Scope); the `functionalOrbit` correctness argument completed (termination, meeting time, entry after exactly the tail length), so it rests on no unread Floyd source; the O(1) `jump` handed to a follow-up on P011 (user decision); first mutation campaign, 2 unreachable-assertion survivors.
