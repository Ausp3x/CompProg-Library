# 013-knapsack.hpp — Evidence

`013-knapsack.hpp` (batch MI16, package P015) provides capacity- and value-indexed knapsack DP with exact and at-most queries and witness restoration, Boolean feasibility, multiplicity-vector counts, the fewest-coins table and ordered composition counts. Prerequisite: the C01 template (P002). It is independent contest-profile code.

## Contracts

### KnapsackItem, KnapsackStatus, KnapsackResult

`KnapsackItem{weight,value,count}` is an item type with nonnegative `lng weight` (widened from `int` on 2026-10-23 so value-indexed tables accept huge weights; a weight above a capacity axis never fits), full signed `lng value`, and `count=-1` for unlimited copies or `count>=0` for a finite multiplicity. `count=1` is ordinary 0/1 knapsack, and item policies may be mixed. Repeated identical item types stay distinct coordinates of a selection vector. Axis bounds are nonnegative and below `INT_MAX`, and the item count fits `int`. Inputs are copied as needed; no borrowed state or global cache survives construction.

### KnapsackDP, ValueKnapsackDP, knapsackFeasible, KnapsackCounts

| API | Result semantics |
|---|---|
| `KnapsackDP(capacity,items,trace=false)` | Maximum value at each **exact** weight from zero through capacity; signed values supported. |
| `exact(weight)` / `atMost(capacity)` | `KnapsackResult{status,value,target}` reports unreachable, finite or unbounded. A finite result's `value` is the exact signed 128-bit optimum and `target` the selected exact weight. At-most queries use prefix optima. |
| `ValueKnapsackDP(max_value,items,trace=false)` | Minimum weight at each exact value through `max_value`; item values must be nonnegative. Items whose value exceeds the axis cannot contribute. |
| `minWeight(value)` | A finite result's `value` field is the minimum total **weight**, and `target` the requested exact **value**. Reachability has its own status. |
| `bestWithin(weight_budget)` | Highest reachable value **within the constructed value table** whose minimum weight is at most the nonnegative `lng` budget; the global optimum only if the caller's value bound covers it. |
| `restore(target)` | With tracing enabled and a finite reachable exact target, returns one quantity per input item (a weight target for capacity DP, a value target for value DP). It does not expand multiplicities into repeated item ids. |
| `knapsackFeasible(capacity,items)` | Boolean exact-weight feasibility, independent of values. |
| `KnapsackCounts<T>(capacity,items)` | Counts multiplicity vectors by exact weight in the caller's exact or modular type; `exact` and `atMost` return `KnapsackCount{status,count}` and distinguish unreachable, finite and unbounded. These count all feasible selections, not only optimal ones and not ordered sequences of picks. |

The empty choice is reachable with zero value and weight, even for an empty item list or axis zero. An unlimited zero-weight positive-value item makes every reachable capacity-DP state unbounded without making an unreachable exact weight reachable. Unlimited zero-weight nonpositive values are skipped for maximization but still make selection counts infinite. A finite zero-weight item contributes all copies exactly when its value is positive, and a factor `count+1` to counting regardless of value. Negative-valued positive-weight items can be needed for an exact-weight optimum and are retained.

At-most queries break ties toward the smaller weight; an unreachable or unbounded result carries `value=0`, and `target` is the requested axis point. Value-indexed DP differs on purpose: zero-value items with nonnegative weights cannot lower a minimum weight, and although unlimited positive-value zero-weight items can make many table values reachable at zero cost, `bestWithin` still queries a **truncated finite value table**, not a finite unrestricted maximum.

Finite capacity scores fit signed 128 bits: at most `INT_MAX` item types each have at most `INT_MAX` selected copies with signed 64-bit values, and positive-weight unlimited copies are capped by the finite axis. Value-DP total weights also fit 128 bits. Count arithmetic is the caller's domain: arbitrary precision for exact counts, a modular type for residues; a fixed-width exact type requires every intermediate sum and product to fit. Modular zero never establishes infeasibility.

Correctness: exact DP starts with only state zero reachable, and each item layer considers its legal multiplicities. Descending 0/1 updates read the previous virtual-item layer; ascending unlimited updates permit reuse. Finite multiplicities are clamped by the axis and split into logarithmically many binary bundles representing every quantity from zero through the limit, which preserves optima and feasibility but not counts (duplicate bundle representations must not be counted). The value table uses the same exact-axis engine with objective minus weight; finite zero-axis contributions and positive unlimited zero-axis unboundedness are handled separately. Prefix argmax tables answer at-most queries without changing exact reachability. Trace rows record each original item's selected quantity after all its bundles, so walking them backwards reconstructs an actual layered choice; a one-dimensional parent pointer is insufficient (0/1 items `(1,1),(1,2)` at capacity two could reconstruct the second item twice). Feasibility tracks remaining copies of the current type along each residue chain. Counts use direct item-layer recurrences, one representation per multiplicity vector: an in-place descending pass for 0/1 items, an ascending pass for unlimited ones (and for finite counts covering the axis), and for other bounded counts `c` a sliding window of `c+1` terms along each residue chain, `new[w] = new[w-step] - old[w-(c+1)*step] + old[w]`, computed ascending over the whole axis without divisions, subtracting before adding so no intermediate exceeds a window sum, with reachability kept apart from numeric residues; unlimited zero-weight items set an infinity flag.

### equalPartitionCount, signedSumCount, minimumCoins, compositionCountTable

| API | Result semantics |
|---|---|
| `equalPartitionCount<T>(values)` | The number of unordered splits {A, B} of the item positions into two halves of equal sum (CSES 1093 Two Sets II); items stay distinct when values repeat, zero values double the count, and an odd sum gives 0, no items 1. Values are nonnegative `int`s with `H = S/2 - max(values) < INT_MAX`. A named composition over `KnapsackCounts`: each unordered split has exactly one ordering with the largest item in A, so it counts subsets of the other items with sum `H`, which needs no division by two, works for every `T`, and builds the smallest such table whatever the input order. |
| `signedSumCount<T>(values,target)` | The number of sign vectors `s` with `sum s_i * values[i] = target` (the target-sum task of cp-algorithms' knapsack practice list); a zero value contributes both signs. It is 0 when `abs(target) > S` or `S + target` is odd; no items and target 0 give 1. Values are nonnegative `int`s with `min(P, S - P) < INT_MAX` for `P = (S + target) / 2`. A named composition over `KnapsackCounts`: the positive items form a subset of sum `P`, and complementing counts sums `S - P` the same way, so the smaller table is built. |
| `minimumCoins(total,coins)` | `vector<int>` of size `total+1`: the fewest coins, each coin value usable any number of times, summing exactly to each total, `-1` when none does; entry 0 is 0. Coins are nonnegative `int`s; zero coins and coins above `total` never help, duplicates are harmless. A named composition (CSES 1634, CP-algorithms complete knapsack) over `KnapsackDP` with items `{c,-1,-1}`, so its preconditions are `KnapsackDP`'s (`0 <= total < INT_MAX`, coins `>= 0`); for a witness, build that `KnapsackDP` with tracing and call `restore`. |
| `compositionCountTable<T>(total,parts)` | `vector<T>` of size `total+1`: the number of **ordered** sequences of parts summing to each total (CSES 1635), entry 0 is 1. Each listed part is a distinct type, so a repeated value counts once per listing (parts `{1,1}` give 4 sequences of total 2). Parts are `>= 1` (a zero part makes counts infinite) and `0 <= total < INT_MAX`; `T` as in `KnapsackCounts` (exact type wide enough for every partial sum, or a modular type). Unordered selections are `KnapsackCounts` with unlimited items. |

Correctness: `minimumCoins` scores each coin `-1`, so the maximum exact-weight value is minus the fewest coins, and a finite capacity status is exactly reachability. `compositionCountTable` classifies each nonempty sequence of total `w` by its last part `p`, leaving a sequence of total `w-p`, so `ways[w] = sum ways[w-p]` over parts `p <= w`; the parts are sorted once so the inner loop stops at the first part above `w`.

Under `NDEBUG`, `knapsack_detail::validate` clamps every axis into `[0, INT_MAX)` and every table is sized from the clamped axis, `knapsack_detail::idle` skips an item with negative weight (negative value on the value axis) or `count < -1`, `compositionCountTable` drops parts below 1, and the two count compositions read the last entry of their clamped table, so violated preconditions never index outside a table; the results are then meaningless but computed within the normal bounds.

Costs: optimization takes `O((axis+1) * (n+1+sum(log(1+bounded_limit))))` time with bounded items and `O((n+1) * (axis+1))` for 0/1 and unlimited inputs; a finite bound covering every axis-reachable quantity uses the unlimited recurrence. Finite positive zero-axis contributions scan `O(axis+1)` states; oversized or irrelevant items skip in constant time. Storage is `O(axis+1)`, trace `O(n * (axis+1))` integers, one restore `O(n)`. Feasibility and counts use `O((n+1) * (axis+1))` arithmetic operations and linear storage (arbitrary-precision bit costs extra). Count prefix sums and capacity prefix optima answer queries in constant time; value `bestWithin` scans its table.

## Feature-to-Test Map

| Public feature | Independent coverage |
|---|---|
| `KnapsackDP.exact/atMost`, `KnapsackItem`, `KnapsackStatus`, `KnapsackResult` | Recursive multiplicity-vector enumeration; exhaustive mixed item alphabets, singleton domains, seeded random cases, finite, unreachable and unbounded status, negative and full-width values, smallest-weight ties. |
| `ValueKnapsackDP.minWeight/bestWithin` | Independent exact-value enumeration and budget scans; nonnegative and zero values, oversized values, explicit finite truncation under unlimited zero-weight positive-value items. |
| Both `restore` methods | Multiplicity bounds, exact total weight and value and optimum checks for every traced small target; stale-parent counterexample, copies, repeated construction. |
| `knapsackFeasible` | Independent reachability of recursively enumerated multiplicity vectors, including zero-capacity and zero-count items. |
| `KnapsackCounts<T>.exact/atMost`, `KnapsackCount` | Exact and prefix multiplicity counts, bounded count-five duplicate-bundle regression, finite zero-weight factors and unlimited zero-weight infinity; modular types for moduli 2 and 1 verify that zero residues stay reachable; every 0/1, bounded and unlimited layer at capacities 0 to 10, 16 and 24 (quick, full, stress) and every step from 1 to capacity + 1, with counts 1, 2, 3, 5, bound - 1, bound and bound + 1, alone and between 0/1, bounded, unlimited and zero-weight items, plus 100, 1,000 and 5,000 seeded mixed sequences; each count also as full `ModInt<998244353>`, mini `ModIntMini<998244353>` and `ModIntMini<2>`, and wrapping `ulng`, which the Python oracle also checks. |
| Width, size, generic arithmetic | `INT_MAX` finite counts, `lng` extremes and 128-bit objectives; value-axis tables with weights `2^31` to `INT64_MAX` and random 63-bit weights; capacity tables with weights above the axis; 100,000-item and 100,000-axis fixtures; exact `2^1000` counts and independent Python arbitrary-integer convolution outputs. |
| `minimumCoins` | Breadth-first search over totals, itself checked against coin-multiset enumeration; exhaustive coin lists through 2, 3 and 4 values from {0, 1, 2, 3, 5, axis+1} at every total through 6, 8 and 10; empty lists, total 0, zero coins, duplicates, coins above the total and `INT_MAX`, the greedy-failing set {4, 3, 1}; seeded random cases; totals 1,000, 100,000 and 500,000 against the search and a closed form. |
| `compositionCountTable` | Layers by number of parts, checked against enumeration of ordered sequences, in exact `Big`, full `ModInt` and mini `ModIntMini` modulo 998244353, modulo 2 and `ulng` (modulo 2^64); duplicate parts ({1, 1} gives 4 at total 2), the CSES 1635 sample; Fibonacci by fast doubling for {1, 2}, 2^w and 3^w for repeated ones; the Python oracle (`--oracle-compositions`) checks exact, prime and 2^64 tables by a multinomial sum over part counts. |
| `equalPartitionCount` | Brute force over all 2^n assignments of positions, halved except for no items, cross-checked against sign-vector enumeration; exhaustive lists through 4, 5 and 6 values from {0, 1, 2, 3, 7}; the CSES 1093 sample, {0, 0}, odd sums, `INT_MAX`-wide values valid only because the largest item is fixed; every ordering of lists whose largest value is not last (all 5,040 orderings of 1..7) and of every fourth random list; full and mini modular types; ones against binomials and zeros against powers of two, modular and wrapping; the Python oracle (`--oracle-splits`) by a signed-sum walk over 1..60, 150 and 250. |
| `signedSumCount` | The sign-vector brute force at every target in [-S-2, S+2] on exhaustive lists, and at 0, ±S, their neighbours, `INT64_MIN`, `INT64_MAX` and random targets on seeded cases; ones against binomials with parity mismatches and `abs(t) > S`; zeros against 2^(k+1) and exactly 2^200; the same Python oracle. |
| Preconditions | 48 checked probes covering axes, weights, counts, values, queries, negative budgets, unavailable, unreachable or unbounded reconstruction, negative or `INT_MAX` totals, negative coins, zero or negative parts, negative values and targets reaching `INT_MAX`. |

The C++ oracle enumerates original item multiplicities recursively instead of repeating DP transitions. Boost was unavailable, so a compact test-only nonnegative exact counter supplies the generic arithmetic instantiation; its decimal outputs are checked against Python integers. The Python oracle convolves permitted multiplicities directly, without sliding windows or binary grouping.

Modes: quick, full and stress use exhaustive item-alphabet sequences through 2, 3 and 4 items, axes 4, 6 and 8, 100, 2,000 and 10,000 seeded cases, and large fixtures of 1,000, 100,000 and 500,000; every mode also exhausts singleton weight 0–4, value -3–3, counts -1/0/1/2/5 and axes 0–7. Coin, part and split lists exhaust their alphabets in every mode with 100, 2,000 and 10,000 seeded cases. A full run at seed 1 makes 30,348 cases and 5,745,159 C++ checks; the Python oracles check 207 count fixtures, 210 composition fixtures (6,300 rows) and 213 split fixtures.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14; optimized, checked and ASan/UBSan builds by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>); memory peaks include the compiler. Latest run 2026-11-26 (MI16), experiment: none. The benchmark row is from 2026-10-23; the MI16 changes add a constant-time check per item outside every DP loop.

| Date | Command | Result |
|---|---|---|
| 2026-11-26 (MI16) | `python3 '96-Local Testing/06-Miscellaneous/013-knapsack_tester.py' --mode quick --seed 1` | PASS, 2 builds, 6,360 cases, 995,969 checks, 48 probes, at most 1.5 s per build; peak 998 MB (the full run's preflight) |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/06-Miscellaneous/013-knapsack_tester.py' --mode full --seed 1` | PASS, 3 builds, 30,348 cases, 5,745,159 checks; peak 1387 MB |
| 2026-11-26 (MI16) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/013-knapsack_tester.py' --mode full --seed 2` | PASS, 3 builds, 30,348 cases, 5,751,389 checks; peak 1419 MB |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/06-Miscellaneous/013-knapsack_tester.py' --mode stress --seed 3` | PASS, 3 builds, 159,470 cases, 30,152,328 checks, ASan-UBSan run 74.5 s; peak 942 MB |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/05-extraction.py' ops --filter 06-Miscellaneous/013-knapsack.hpp` | PASS, 17 operations on g++ and g++-14, 0 failures |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/05-extraction.py' testers --filter 013-knapsack` | PASS, 1 quick run on the extracted build |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/02-integration.py'` | PASS, 192 standalone and aggregate headers, multi-TU, workspace; peak 1891 MB |
| 2026-11-26 (MI16) | `python3 '96-Local Testing/03-consistency.py'` | no errors |
| 2026-10-23 (P072) | `python3 '96-Local Testing/06-Miscellaneous/013-knapsack_benchmark.py'` | PASS, 48 comparisons, peak 333 MB |

## Mutation Testing

Record: mewt 4.0.0 on blob `fe973c8f8dd2eceaec55514dc82ef1445a37c75f` (2026-11-26, seed 1, 22 lines changed since blob `2cb3593f055c`, sample of 180: 10 high, 21 medium, 19 low): 50 mutants, 49 caught (0 by timeout), 1 survived, 0 untested. Stage seconds: quick 104, full-optimized 6, full-rest 77. Earlier campaigns the same day: whole file on blob `e5c01898e3ea` (50 mutants, 46 caught) and 33 changed lines on blob `2cb3593f055c` (50 mutants, 48 caught); their open survivors below sit on lines the later diffs did not cover.

| Mutant | Line | Change | Argument |
|---|---|---|---|
| `COS 655` (low, blob `e5c01898e3ea`) | 70 | `dp[target] == NONE` → `<=` | Equivalent: no entry ever falls below `NONE`, since every update reads a reachable entry and finite scores stay above `-2^126`. |
| `COS 699` (low, blob `e5c01898e3ea`) | 109 | `v >= 0` → `v != 0` in `bestWithin` | Equivalent: entry 0 always holds weight 0 (only nonpositive scores reach it and they are skipped), which every valid budget covers, so stopping before it returns the same 0. |
| `ER 27` (high, blob `e5c01898e3ea`) | 111 | final `return 0;` → `throw` | Equivalent: reachable only with a negative budget, past a violated precondition under `NDEBUG`, where it still ends within the bound (user decision 2026-11-12). |
| `AS 251` (low) | 121 | `min(x.count, top / step)` → `min(top / step, x.count)` | Equivalent: `min` is symmetric. |
| `IF 192` (medium, blob `2cb3593f055c`) | 154 | `(x.count == 1)` → `(false)` | Equivalent: the 0/1 branch is a fast path; the bounded window with `count = 1` (`span = 2 * step <= capacity` there) gives the same counts in the same O(W + 1) per item. |

Closed survivors: `CR 137` (`equalPartitionCount`'s width assert) by wide probes whose target exceeds `INT_MAX`, and `CR 121` (the clamped-axis assignment in `KnapsackCounts`) by making every clamped axis a new local `top`, which cannot be removed without breaking the build.

## Benchmarks

The [driver](<../../96-Local Testing/06-Miscellaneous/013-knapsack_benchmark.py>) and its git-ignored `013-knapsack_benchmark.json` (2026-10-23): GCC 16.2.1 GNU++20 `-O2 -DNDEBUG`, i9-11900H, fixed seed, one warmup and five rotating-order repetitions. Capacities 16, 512 and 4096 with 12, 48 and 48 item types; weights 1–23 (1–3 in a small-weight corpus); values -50–150; 0/1, bounded 1–32 or unlimited multiplicities; both trace settings. Timing covers DP allocation, every exact query, one full-capacity at-most query, optional reconstruction and destruction; reference generation and full output and witness verification are untimed.

| Workload | Library time / direct reference time |
|---|---|
| 0/1 | 0.248–0.411 |
| Bounded | 0.146–0.382 |
| Unlimited | 0.0006–0.0039 at capacity 4096; the reference keeps its direct quantity loop |

The reference enumerates every legal quantity from the previous layer without binary splitting. These are warmed-cache shared-host observations against a simple recurrence, not a fastest-known result, a timing gate or a tuned threshold. Row `29` monotone queues, row `22` bitsets and fine-grained subset-sum algorithms were not benchmarked.

## Sources

Inspected on 2026-09-28; the implementations are written independently from the recurrences, with no copied external source:

- [CP-algorithms knapsack](https://cp-algorithms.com/dynamic_programming/knapsack.html) (page updated 2026-09-18): 0/1, complete, multiple, binary grouping, monotone-queue and mixed sections. Binary grouping is the Basic implementation here; the monotone-queue alternative is an Advanced extension.
- [OI Wiki knapsack](https://oi-wiki.org/dp/knapsack/): ordinary variants, witness reconstruction, feasible and optimal counting, specialized extensions. Its ordinary text overlaps CP-algorithms substantially, so the two are not fully independent evidence. Some dominance advice does not apply to 0/1 or bounded exact-target problems and is not adopted.
- Antti Laaksonen, [*Competitive Programmer's Handbook*](https://cses.fi/book/book.pdf), locally saved draft of July 3, 2018: §§7.1 and 7.4, pp.68–73, coin witness and counting and 0/1 reachability derivations; §27.2 p.254, small-distinct-weight subset sum as an Advanced lead.
- [AtCoder DP E task](https://atcoder.jp/contests/dp/tasks/dp_e) and [iastm's editorial](https://atcoder.jp/contests/dp/editorial/17284), posted 2026-03-16: min weight by exact value, descending one-dimensional update and greatest feasible value. Reading them is not an acceptance claim.
- [CSES problem set](https://cses.fi/problemset/) tasks 1093 (Two Sets II), 1633 (Dice Combinations), 1634 (Minimizing Coins) and 1635 (Coin Combinations I), fetched 2026-11-26: the semantics of `equalPartitionCount`, `minimumCoins` and `compositionCountTable`.
- CP-algorithms' knapsack practice list (same page, fetched 2026-11-26): the target-sum task behind `signedSumCount`.
- Karl Bringmann, [*A Near-Linear Pseudopolynomial Time Algorithm for Subset Sum*](https://arxiv.org/abs/1610.04712), v2 2017-01-08 (SODA 2017): abstract and metadata only. Randomized near-linear positive-integer decision subset sum is a specialized MI18 research lead; it supplies neither arbitrary-profit optimization nor multiplicity-vector counting, and no adaptation or execution is claimed.

## Limits and Handoffs

- Legacy `OLD/Team Notebook/src/algsbetter.cpp` lines 3567–3608 and `algs.cpp` lines 2486–2508 (`knapsack01`, `knapsackComplete`, binary-grouped `knapsackMultiple`) are covered; their `lng` overflow, unchecked domains and zero-weight unboundedness are corrected here. No active callers; archives preserved.
- Advanced knapsack variants belong to row `29` (MI18), Boolean bitset subset sum to `22` and meet-in-the-middle to `30` ([00-notes.md](00-notes.md), Advanced and Esoteric Contracts).
- Not adopted in the 2026-10-08 sweep: [00-notes.md](00-notes.md), Basic contracts (Knapsack); swept sources in [00-sources.md](00-sources.md).
- `compositionCountTable` is the direct O(n * W) recurrence for any `T`. With a prime NTT-friendly modulus, the series inverse of `1 - sum x^p` (Core `Poly`, Mathematics power series) gives the table in O(W * log(W)), and a single huge total with small parts is a linear-recurrence term (Mathematics); both are compositions over those families, not row `13` operations.
- `equalPartitionCount` and `signedSumCount` build the O(n * W) `KnapsackCounts` table for any `T`; with a prime modulus above the target, Mathematics row 12 `subsetSumCount` (power-series log and exp, O(t * log(t))) is the faster route for the same counts.
- `minimumCoins` composes `KnapsackDP`, so it stores a 128-bit score and a prefix index per total (24 bytes against 4 for a direct `int` table). The reviewer measured 0.22 s against 0.087 s for a direct `int` DP at W = 10^6 with 100 coins (2026-11-26, `-O2 -DNDEBUG`, i9-11900H); the composition rule keeps the call (`03-cpp.md`, One API per Operation).
- Tower knapsack (AtCoder EDPC X) is planned row `39` `lawlerMoore` (user decision 2026-11-26).
- DMOJ Knapsack 3 (several capacities, bounded counts) is answered by a bounded `KnapsackDP` and its `atMost` queries. DMOJ Knapsack 4 (the largest value of two teams of equal total weight, signed values) composes as two exact `KnapsackDP` tables in O((n + m) * S), S the smaller side's total; the faster random-order windowed DP is planned row `29` `zeroSumKnapsack` (user decision 2026-11-26).

## Aliases

Search aliases (P263, grab --find): 背包 DP 基础, 背包 DP 进阶, 01背包, 完全背包, 多重背包, 背包问题, 混合背包, 背包方案数, 背包输出方案.

Catalog aliases (MI16 sweep, 2026-11-26): coin change, minimizing coins, coin combinations, dice combinations, two sets, equal sum partition, target sum, money sums, book shop.

## History

- 2026-09-28: verified (P015, previous system).
- 2026-10-08: re-audit restyled the comment cap and closing braces, 3 findings fixed, no behavior change; full suites, g++-14 full and benchmarks passed.
- 2026-10-23: P072 widened `KnapsackItem::weight` to `lng` (replacing `largeWeightSmallValueKnapsack`) and added width tests; full suites, g++-14 full, stress and benchmarks passed.
- 2026-10-24: P072 `/finish` (user decision 2026-10-24) marked the assert-only parameter and loop variable of `knapsack_detail::validate` `[[maybe_unused]]`, so `-DNDEBUG -Wall -Wextra` builds are warning-free; quick and full passed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; every library-wide run passed.
- 2026-11-26: MI16 added `minimumCoins`, `compositionCountTable`, `equalPartitionCount` and `signedSumCount` (the last two by user decision); review fixed 9 findings (axis clamping and item skips under `NDEBUG`, the largest item fixed in `equalPartitionCount`, in-place 0/1 and division-free bounded count layers, mini instantiations); three mutation campaigns, quick, full, g++-14 full, stress, extraction and integration passed.
