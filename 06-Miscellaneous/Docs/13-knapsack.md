# 13-knapsack.hpp — evidence

`13-knapsack.hpp` (batch MI16) provides capacity- and value-indexed knapsack DP with exact/at-most queries and witness restoration, Boolean feasibility and multiplicity-vector counts. Package P015 (order MI15, MI16, MI17). The verified C01 template prerequisite is supplied by P002. Implementations are independent contest-profile code. Archived originals remain unchanged. No online submissions are part of this work.

## Contracts

### KnapsackItem, KnapsackStatus, KnapsackResult

`KnapsackItem{weight,value,count}` represents an item type with nonnegative `int weight`, full signed `lng value`, and `count=-1` for unlimited copies or `count>=0` for a finite multiplicity. `count=1` is ordinary 0/1 knapsack; mixed item policies may coexist. Repeated identical item types remain distinct coordinates of a selection vector. Axis bounds are nonnegative and less than `INT_MAX`, and the item count fits `int`. Input values and multiplicities are copied as needed; no borrowed state or global cache survives construction.

### KnapsackDP, ValueKnapsackDP, knapsackFeasible, KnapsackCounts

| API | Result semantics |
|---|---|
| `KnapsackDP(capacity,items,trace=false)` | Maximum value at each **exact** weight from zero through capacity. Signed values are supported. |
| `exact(weight)` / `atMost(capacity)` | `KnapsackResult{status,value,target}` reports unreachable, finite or unbounded. For finite results, `value` is the exact signed 128-bit optimum and `target` the selected exact weight. At-most queries use prefix optima. |
| `ValueKnapsackDP(max_value,items,trace=false)` | Minimum weight at each exact value through `max_value`; item values must be nonnegative. Items whose value exceeds the axis cannot contribute to this table. |
| `minWeight(value)` | A finite result's `value` field is minimum total **weight**, and `target` is the requested exact **value**. Reachability has a separate status. |
| `bestWithin(weight_budget)` | Highest reachable value **within the constructed value table** with minimum weight at most the nonnegative `lng` budget. This is the global optimum only if the caller's value bound covers it. |
| `restore(target)` | With tracing enabled and a finite reachable exact target, returns one quantity per input item. Capacity DP takes a weight target; value DP takes a value target. It does not expand large multiplicities into repeated item IDs. |
| `knapsackFeasible(capacity,items)` | Boolean exact-weight feasibility independent of values. |
| `KnapsackCounts<T>(capacity,items)` | Counts multiplicity vectors by exact weight, using the caller's exact or modular arithmetic type; `exact` and `atMost` queries distinguish unreachable, finite and unbounded results. These are all feasible selections, not just optimal selections or ordered sequences of picks. |

The empty choice is reachable with zero value/weight, even for an empty item list or axis zero. An unlimited zero-weight positive-value item makes every reachable capacity-DP state unbounded, without making an unreachable exact weight reachable. Unlimited zero-weight nonpositive values can be skipped for maximization; they still make selection counts infinite. A finite zero-weight item contributes all copies exactly when its value is positive, and contributes a factor `count+1` to counting regardless of value. Negative-valued positive-weight items can be necessary for an exact-weight optimum and are retained.

Value-indexed DP deliberately has different semantics: zero-value items with nonnegative weights cannot lower a minimum weight. Unlimited positive-value zero-weight items can make many table values reachable at zero cost, but `bestWithin` still queries a **truncated finite value table**, not an assertion of a finite unrestricted maximum.

Finite capacity scores fit signed 128 bits: at most `INT_MAX` item types each have at most `INT_MAX` selected copies, with signed 64-bit values; positive-weight unlimited copies are capped by the finite axis. Value-DP total weights also fit 128 bits. Count arithmetic is a caller-supplied domain: choose arbitrary precision for unbounded finite integer counts or a modular type for residues; a fixed-width exact type requires all intermediate sums/products to fit. Modular zero never establishes infeasibility.

## Correctness, memory and optimization

Exact DP initializes only state zero as reachable. Each item layer considers its legal multiplicities. Descending 0/1 updates read the previous virtual-item layer; ascending unlimited updates permit repeated use. Finite multiplicities are clamped by the axis and split into logarithmically many binary bundles, which represent every quantity from zero through the limit. This transformation preserves optima and feasibility but not counting multiplicities: duplicate bundle representations must not be counted as different selections.

The value table uses the same exact-axis engine with objective equal to negative weight. Finite zero-axis contributions and positive unlimited zero-axis unboundedness are handled separately. Prefix argmax tables answer capacity at-most queries without changing exact reachability.

Optional trace rows retain the selected quantity of the current original item after all its virtual bundles have been processed. Walking these rows backwards reconstructs an actual layered choice. A mutable one-dimensional parent pointer is insufficient: with 0/1 items `(1,1),(1,2)` and capacity two, updating state one after state two could otherwise reconstruct the second item twice.

Feasibility tracks how many copies of the current type remain along each residue chain. Counts use direct item-layer recurrences and bounded sliding sums, preserving one representation per multiplicity vector; their reachability is maintained independently of numeric residues. Unlimited zero-weight items are omitted from finite recurrence arithmetic and recorded as an infinity flag.

Capacity/value optimization takes `O((axis+1) * (n+1+sum(log(1+bounded_limit))))` time in the general bounded case, and `O((n+1) * (axis+1))` for 0/1/unlimited inputs. A finite bound covering every axis-reachable quantity safely uses the ascending unlimited recurrence. Finite positive zero-axis contributions scan `O(axis+1)` states; oversized or irrelevant items skip in constant time after validation. Normal storage is `O(axis+1)`; optional trace storage is `O(n * (axis+1))` integers and restoring one witness costs `O(n)`. Feasibility and counts use `O((n+1) * (axis+1))` arithmetic operations and linear-axis storage; arbitrary-precision count bit costs are additional. Count prefix sums and capacity prefix optima support constant-time queries. Value `bestWithin` scans its table.

## Feature-to-test map

| Public feature | Independent coverage |
|---|---|
| `KnapsackDP.exact/atMost` | Recursive multiplicity-vector enumeration; exhaustive mixed item alphabets, singleton domains, seeded random cases, finite/unreachable/unbounded status, negative/full-width values and smallest-weight ties. |
| `ValueKnapsackDP.minWeight/bestWithin` | Independent exact-value enumeration and budget scans; nonnegative/zero values, oversized values and explicit finite truncation under unlimited zero-weight positive-value items. |
| Both `restore` methods | Multiplicity bounds, exact total weight/value and optimum checks against every traced small target; stale-parent counterexample, copies and repeated construction. |
| `knapsackFeasible` | Independent reachability of recursively enumerated multiplicity vectors, including zero-capacity and zero-count items. |
| `KnapsackCounts<T>` | Exact and prefix multiplicity counts, bounded count-five duplicate-bundle regression, finite zero-weight factors and unlimited zero-weight infinity; modular types for moduli 2 and 1 verify zero residues remain reachable. |
| Width/size/generic arithmetic | `INT_MAX` finite counts, `lng` extremes and 128-bit objectives; 100,000-item and 100,000-axis fixtures; exact `2^1000` counts and independent Python arbitrary-integer convolution outputs. |
| Preconditions | 37 checked probes covering axes, weights/counts/values, queries, negative budgets and unavailable/unreachable/unbounded reconstruction. |

The C++ oracle recursively enumerates original item multiplicities rather than repeating DP transitions. Boost was unavailable; a compact test-only nonnegative exact counter supplies the generic arithmetic instantiation, and its actual decimal outputs are checked independently against Python integers. The Python oracle directly convolves permitted multiplicities without sliding windows or binary grouping.

## Commands and results

The mirrored `13-knapsack_tester.py` suite passed optimized NDEBUG, checked, and ASan/UBSan with leak checking. Each configuration ran 9,593 cases / 1,480,882 C++ checks and 207 Python fixtures / 2,323 exact-count output rows. The checked build passed 37 assertion probes.

Quick/full/stress modes use exhaustive item-alphabet sequences through 2/3/4 items, axes 4/6/8, 100/2,000/10,000 seeded cases, and large fixtures of 1,000/100,000/500,000 items/axis positions. Every mode also exhausts singleton weight 0–4, value -3–3, counts -1/0/1/2/5 and axes 0–7. Full/stress add ASan/UBSan. Commands:

```bash
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 20260928
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

The sanitizer configuration passed outside the sandbox after its process tracer blocked LeakSanitizer; leak checking remained enabled. Parent/worker approval timing caused one redundant successful run, not different coverage. Stress mode is implemented but was not executed.

Final environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Full configurations use GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Leak checking remained enabled. The initial sanitizer failures came from LeakSanitizer's documented inability to run beneath the sandbox process tracer; approved runs outside that tracer passed. No algorithm failures remained.

Package-wide P015 integration and consistency commands passed:

```bash
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Integration compiled all 93 present standalone/aggregate headers, linked combined scalar and available AVX2 aggregates across two translation units, and checked the standalone Workspace in LOCAL/non-LOCAL modes. Repository consistency reported no errors. Every saved benchmark source hash matched the final source bytes.

The shared runner discovered and passed this suite from `/tmp`, confirming that tests resolve paths independently of the caller's directory:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 13-knapsack --no-integration
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py>) and `96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.json` passed 48 workload/method comparisons. Command:

```bash
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py'
```

The run used GCC 16.2.1 GNU++20 `-O2 -DNDEBUG` on an Intel Core i9-11900H, seed 20260928, one warmup and five measured repetitions with rotating method order. Capacities were 16/512/4096, with 12/48/48 item types, positive weights in 1–23 (1–3 for a separate small-weight corpus), values in -50–150, and 0/1, bounded 1–32, or unlimited multiplicities. Both trace settings were measured.

Timing includes fresh DP allocation/setup, every exact query, one full-capacity at-most query, optional reconstruction, and destruction of internal state. Independent reference generation and complete output/witness verification occur outside timing. Raw samples, medians, environment, memory accounting and final source hashes are recorded. The direct reference enumerates every legal quantity from the previous item layer; it does not reuse binary splitting. The library used roughly 0.270–0.439 of that reference's time on 0/1 cases and 0.123–0.382 on bounded cases. Large unlimited cases show larger gains because the reference intentionally retains its direct quantity loop.

These are warmed-cache shared-host observations against a simple independent recurrence, not a fastest-known bounded-knapsack result, a portable timing gate or a tuned dispatch threshold. MI18 monotone queues, bitsets and fine-grained subset-sum algorithms were not benchmarked. Binary grouping supplies the compact Basic bounded variant; those Advanced alternatives remain explicitly planned.

## Sources

Inspected on 2026-09-28; implementations are independently written from mathematical recurrences, with no copied external source code:

- [CP-algorithms knapsack](https://cp-algorithms.com/dynamic_programming/knapsack.html): ordinary 0/1, complete, multiple, binary grouping, monotone-queue and mixed sections; retrieved page reports update 2026-09-18. Binary grouping belongs to the Basic implementation here; the monotone-queue alternative remains an Advanced extension.
- [OI Wiki knapsack](https://oi-wiki.org/dp/knapsack/): ordinary variants, witness reconstruction, feasible/optimal counting and specialized extensions. Its ordinary text substantially overlaps CP-algorithms, so those two pages are not treated as fully independent evidence. Some dominance advice is inapplicable to 0/1/bounded exact-target problems and is deliberately not adopted.
- Antti Laaksonen, [*Competitive Programmer's Handbook*](https://cses.fi/book/book.pdf), locally saved July 3, 2018 draft: §§7.1/7.4, pp.68–73, coin witness/counting and 0/1 reachability derivations; §27.2 p.254 for small-distinct-weight subset sum as an Advanced lead.
- [AtCoder DP E task](https://atcoder.jp/contests/dp/tasks/dp_e) and [iastm's editorial](https://atcoder.jp/contests/dp/editorial/17284), posted 2026-03-16: full min-weight-by-exact-value recurrence, descending one-dimensional update and greatest feasible value. Reading this material is not an online acceptance claim.

Legacy `OLD/Team Notebook/src/algsbetter.cpp` lines 3567–3608 and `algs.cpp` lines 2486–2508 contain `knapsack01`, `knapsackComplete` and binary-grouped `knapsackMultiple`. Their at-most nonnegative-empty-choice behavior and three multiplicity policies are covered. Their `lng` overflow, unchecked domains and zero-weight unboundedness limitations are corrected in the new APIs. There were no active callers to migrate; the archived names and bytes are preserved.

The abstract/metadata of Karl Bringmann, [*A Near-Linear Pseudopolynomial Time Algorithm for Subset Sum*](https://arxiv.org/abs/1610.04712), v2 2017-01-08 (SODA 2017), were also inspected: randomized near-linear pseudopolynomial positive-integer decision subset sum is a specialized MI18 research lead. No proof/code adaptation or execution is claimed; its result does not directly supply arbitrary-profit optimization or multiplicity-vector counting.

## Limits and handoffs

Advanced ownership remains MI18: monotone-queue bounded optimization, meet-in-the-middle and bitset subset sum, specialized value/small-weight domains, generating-function counts and their witness algorithms. Counting **optimal** selections, k-best scores, grouped/precedence/multiple-resource constraints and modern fine-grained algorithms are explicit research leads for that owner, not implemented or claimed complete here.

MI18 retains advanced knapsack algorithms and the research leads listed above; they are not claimed implemented by P015. Extended stress mode and execution on other compiler/interpreter versions were not performed. No external online acceptance is claimed, and no online submission was made.
