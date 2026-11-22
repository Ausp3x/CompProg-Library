# 13-knapsack.hpp — Evidence

`13-knapsack.hpp` (batch MI16, package P015) provides capacity- and value-indexed knapsack DP with exact and at-most queries and witness restoration, Boolean feasibility and multiplicity-vector counts. Prerequisite: the C01 template (P002). It is independent contest-profile code. The row's `missing:` operations are not implemented.

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

Correctness: exact DP starts with only state zero reachable, and each item layer considers its legal multiplicities. Descending 0/1 updates read the previous virtual-item layer; ascending unlimited updates permit reuse. Finite multiplicities are clamped by the axis and split into logarithmically many binary bundles representing every quantity from zero through the limit, which preserves optima and feasibility but not counts (duplicate bundle representations must not be counted). The value table uses the same exact-axis engine with objective minus weight; finite zero-axis contributions and positive unlimited zero-axis unboundedness are handled separately. Prefix argmax tables answer at-most queries without changing exact reachability. Trace rows record each original item's selected quantity after all its bundles, so walking them backwards reconstructs an actual layered choice; a one-dimensional parent pointer is insufficient (0/1 items `(1,1),(1,2)` at capacity two could reconstruct the second item twice). Feasibility tracks remaining copies of the current type along each residue chain. Counts use direct item-layer recurrences and bounded sliding sums, one representation per multiplicity vector, with reachability kept apart from numeric residues; unlimited zero-weight items set an infinity flag.

Costs: optimization takes `O((axis+1) * (n+1+sum(log(1+bounded_limit))))` time with bounded items and `O((n+1) * (axis+1))` for 0/1 and unlimited inputs; a finite bound covering every axis-reachable quantity uses the unlimited recurrence. Finite positive zero-axis contributions scan `O(axis+1)` states; oversized or irrelevant items skip in constant time. Storage is `O(axis+1)`, trace `O(n * (axis+1))` integers, one restore `O(n)`. Feasibility and counts use `O((n+1) * (axis+1))` arithmetic operations and linear storage (arbitrary-precision bit costs extra). Count prefix sums and capacity prefix optima answer queries in constant time; value `bestWithin` scans its table.

## Feature-to-Test Map

| Public feature | Independent coverage |
|---|---|
| `KnapsackDP.exact/atMost`, `KnapsackItem`, `KnapsackStatus`, `KnapsackResult` | Recursive multiplicity-vector enumeration; exhaustive mixed item alphabets, singleton domains, seeded random cases, finite, unreachable and unbounded status, negative and full-width values, smallest-weight ties. |
| `ValueKnapsackDP.minWeight/bestWithin` | Independent exact-value enumeration and budget scans; nonnegative and zero values, oversized values, explicit finite truncation under unlimited zero-weight positive-value items. |
| Both `restore` methods | Multiplicity bounds, exact total weight and value and optimum checks for every traced small target; stale-parent counterexample, copies, repeated construction. |
| `knapsackFeasible` | Independent reachability of recursively enumerated multiplicity vectors, including zero-capacity and zero-count items. |
| `KnapsackCounts<T>.exact/atMost`, `KnapsackCount` | Exact and prefix multiplicity counts, bounded count-five duplicate-bundle regression, finite zero-weight factors and unlimited zero-weight infinity; modular types for moduli 2 and 1 verify that zero residues stay reachable. |
| Width, size, generic arithmetic | `INT_MAX` finite counts, `lng` extremes and 128-bit objectives; value-axis tables with weights `2^31` to `INT64_MAX` and random 63-bit weights; capacity tables with weights above the axis; 100,000-item and 100,000-axis fixtures; exact `2^1000` counts and independent Python arbitrary-integer convolution outputs. |
| Preconditions | 37 checked probes covering axes, weights, counts, values, queries, negative budgets and unavailable, unreachable or unbounded reconstruction. |

The C++ oracle enumerates original item multiplicities recursively instead of repeating DP transitions. Boost was unavailable, so a compact test-only nonnegative exact counter supplies the generic arithmetic instantiation; its decimal outputs are checked against Python integers. The Python oracle convolves permitted multiplicities directly, without sliding windows or binary grouping.

Modes: quick, full and stress use exhaustive item-alphabet sequences through 2, 3 and 4 items, axes 4, 6 and 8, 100, 2,000 and 10,000 seeded cases, and large fixtures of 1,000, 100,000 and 500,000; every mode also exhausts singleton weight 0–4, value -3–3, counts -1/0/1/2/5 and axes 0–7. Each full build runs 11,605 cases (1,650,055 C++ checks) and 207 Python fixtures (2,286 exact-count rows).

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14; optimized, checked and ASan/UBSan builds by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>); memory peaks include the compiler. The 2026-11-10 runs are library-wide (seed 42, beside the other suites, build cache emptied before each cold run); their figures are this tester's. The 2026-10-23 counts stand: the 2026-10-24 edit only added `[[maybe_unused]]` to `knapsack_detail::validate`.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; quick preflight peak 829 MB; optimized 0.3 s compile + 0.7 s run, checked 22.2 s + 1.3 s, ASan-UBSan 23.3 s + 11.0 s; peak 1024 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 883 MB; optimized 0.2 s + 0.4 s, checked 27.5 s + 1.5 s, ASan-UBSan 26.9 s + 8.7 s; peak 1077 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 29.7 s + 0.1 s, checked 21.9 s + 1.4 s; peak 805 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 830 MB; optimized 0.1 s + 1.0 s, checked 7.8 s + 2.1 s, ASan-UBSan 8.0 s + 25.4 s; peak 840 MB |
| 2026-10-24 (P072) | `13-knapsack_tester.py` quick and full | passed (counts not recorded) |
| 2026-10-23 (P072) | `python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode quick --seed 1` | PASS, 2 builds, 37 probes, peak 444 MB |
| 2026-10-23 (P072) | `python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 1` | PASS, 3 builds, peak 500 MB |
| 2026-10-23 (P072) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 2` | PASS, 3 builds, peak 508 MB |
| 2026-10-23 (P072) | `python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode stress --seed 3` | PASS, 3 builds, peak 825 MB |
| 2026-10-23 (P072) | `python3 '96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py'` | PASS, 48 comparisons, peak 333 MB |
| 2026-10-23 (P072) | `python3 '96-Local Testing/02-integration.py'` | PASS, 174 standalone and aggregate headers, multi-TU, workspace |
| 2026-10-23 (P072) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

The [driver](<../../96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py>) and its git-ignored `13-knapsack_benchmark.json` (2026-10-23): GCC 16.2.1 GNU++20 `-O2 -DNDEBUG`, i9-11900H, fixed seed, one warmup and five rotating-order repetitions. Capacities 16, 512 and 4096 with 12, 48 and 48 item types; weights 1–23 (1–3 in a small-weight corpus); values -50–150; 0/1, bounded 1–32 or unlimited multiplicities; both trace settings. Timing covers DP allocation, every exact query, one full-capacity at-most query, optional reconstruction and destruction; reference generation and full output and witness verification are untimed.

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
- Karl Bringmann, [*A Near-Linear Pseudopolynomial Time Algorithm for Subset Sum*](https://arxiv.org/abs/1610.04712), v2 2017-01-08 (SODA 2017): abstract and metadata only. Randomized near-linear positive-integer decision subset sum is a specialized MI18 research lead; it supplies neither arbitrary-profit optimization nor multiplicity-vector counting, and no adaptation or execution is claimed.

## Limits and Handoffs

- Legacy `OLD/Team Notebook/src/algsbetter.cpp` lines 3567–3608 and `algs.cpp` lines 2486–2508 (`knapsack01`, `knapsackComplete`, binary-grouped `knapsackMultiple`) are covered; their `lng` overflow, unchecked domains and zero-weight unboundedness are corrected here. No active callers; archives preserved.
- Advanced knapsack variants belong to row `29` (MI18), Boolean bitset subset sum to `22` and meet-in-the-middle to `30` ([00-notes.md](00-notes.md), Advanced and Esoteric Contracts).
- Not adopted in the 2026-10-08 sweep: [00-notes.md](00-notes.md), Basic contracts (Knapsack); swept sources in [00-sources.md](00-sources.md).
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 背包 DP 基础, 背包 DP 进阶, 01背包, 完全背包, 多重背包, 背包问题, 混合背包, 背包方案数, 背包输出方案.

## History

- 2026-09-28: verified (P015, previous system).
- 2026-10-08: re-audit restyled the comment cap and closing braces, 3 findings fixed, no behavior change; full suites, g++-14 full and benchmarks passed.
- 2026-10-23: P072 widened `KnapsackItem::weight` to `lng` (replacing `largeWeightSmallValueKnapsack`) and added width tests; full suites, g++-14 full, stress and benchmarks passed.
- 2026-10-24: P072 `/finish` (user decision 2026-10-24) marked the assert-only parameter and loop variable of `knapsack_detail::validate` `[[maybe_unused]]`, so `-DNDEBUG -Wall -Wextra` builds are warning-free; quick and full passed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; every library-wide run passed.
