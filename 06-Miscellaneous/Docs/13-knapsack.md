# 13-knapsack.hpp — evidence

`13-knapsack.hpp` (batch MI16, package P015, status audit) provides capacity- and value-indexed knapsack DP with exact/at-most queries and witness restoration, Boolean feasibility and multiplicity-vector counts. Prerequisite: the C01 template (P002). Implementations are independent contest-profile code. No online submission was made.

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

Correctness: exact DP initializes only state zero as reachable, and each item layer considers its legal multiplicities. Descending 0/1 updates read the previous virtual-item layer; ascending unlimited updates permit repeated use. Finite multiplicities are clamped by the axis and split into logarithmically many binary bundles representing every quantity from zero through the limit, which preserves optima and feasibility but not counts (duplicate bundle representations must not be counted). The value table uses the same exact-axis engine with objective equal to negative weight; finite zero-axis contributions and positive unlimited zero-axis unboundedness are handled separately. Prefix argmax tables answer at-most queries without changing exact reachability. Trace rows record the selected quantity of each original item after all its bundles, so walking them backwards reconstructs an actual layered choice; a one-dimensional parent pointer is insufficient (0/1 items `(1,1),(1,2)` at capacity two could reconstruct the second item twice). Feasibility tracks remaining copies of the current type along each residue chain. Counts use direct item-layer recurrences and bounded sliding sums, one representation per multiplicity vector, with reachability kept independently of numeric residues; unlimited zero-weight items are recorded as an infinity flag.

Costs: optimization takes `O((axis+1) * (n+1+sum(log(1+bounded_limit))))` time in the bounded case and `O((n+1) * (axis+1))` for 0/1/unlimited inputs; a finite bound covering every axis-reachable quantity uses the unlimited recurrence. Finite positive zero-axis contributions scan `O(axis+1)` states; oversized or irrelevant items skip in constant time. Storage is `O(axis+1)`, trace `O(n * (axis+1))` integers, and one restore `O(n)`. Feasibility and counts use `O((n+1) * (axis+1))` arithmetic operations and linear storage (arbitrary-precision bit costs extra). Count prefix sums and capacity prefix optima give constant-time queries; value `bestWithin` scans its table.

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

Run 2026-09-28: Linux x86-64, i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie` with leak checking enabled.

```bash
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 20260928                               # PASS, optimized and checked
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan    # PASS
python3 '96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 13-knapsack --no-integration                    # PASS, also when run from /tmp
python3 '96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py'                                                       # PASS, 48 comparisons
python3 '96-Local Testing/02-integration.py'                                                                                # PASS, 93 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                                # no errors
```

Each configuration ran 9,593 cases / 1,480,882 C++ checks and 207 Python fixtures / 2,323 exact-count rows; the checked build passed 37 assertion probes. Quick/full/stress use exhaustive item-alphabet sequences through 2/3/4 items, axes 4/6/8, 100/2,000/10,000 seeded cases and large fixtures of 1,000/100,000/500,000; every mode also exhausts singleton weight 0–4, value -3–3, counts -1/0/1/2/5 and axes 0–7. Stress mode exists but was not run. No `CXX=g++-14` floor run is recorded.

## Benchmarks

The [driver](<../../96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.py>) and `96-Local Testing/06-Miscellaneous/13-knapsack_benchmark.json` (2026-09-28): GCC 16.2.1 GNU++20 `-O2 -DNDEBUG`, i9-11900H, seed 20260928, one warmup and five rotating-order repetitions. Capacities 16/512/4096 with 12/48/48 item types, weights 1–23 (1–3 in a small-weight corpus), values -50–150, and 0/1, bounded 1–32 or unlimited multiplicities, both trace settings. Timing covers DP allocation, every exact query, one full-capacity at-most query, optional reconstruction and destruction; reference generation and full output/witness verification are untimed.

| Workload | Library time / direct reference time |
|---|---|
| 0/1 | 0.270–0.439 |
| Bounded | 0.123–0.382 |
| Unlimited (large) | larger gains; the reference keeps its direct quantity loop |

The reference enumerates every legal quantity from the previous layer without binary splitting. These are warmed-cache shared-host observations against a simple recurrence, not a fastest-known result, a timing gate or a tuned threshold. MI18 monotone queues, bitsets and fine-grained subset-sum algorithms were not benchmarked.

## Sources

Inspected on 2026-09-28; implementations are independently written from mathematical recurrences, with no copied external source code:

- [CP-algorithms knapsack](https://cp-algorithms.com/dynamic_programming/knapsack.html): ordinary 0/1, complete, multiple, binary grouping, monotone-queue and mixed sections; retrieved page reports update 2026-09-18. Binary grouping belongs to the Basic implementation here; the monotone-queue alternative remains an Advanced extension.
- [OI Wiki knapsack](https://oi-wiki.org/dp/knapsack/): ordinary variants, witness reconstruction, feasible/optimal counting and specialized extensions. Its ordinary text substantially overlaps CP-algorithms, so those two pages are not treated as fully independent evidence. Some dominance advice is inapplicable to 0/1/bounded exact-target problems and is deliberately not adopted.
- Antti Laaksonen, [*Competitive Programmer's Handbook*](https://cses.fi/book/book.pdf), locally saved July 3, 2018 draft: §§7.1/7.4, pp.68–73, coin witness/counting and 0/1 reachability derivations; §27.2 p.254 for small-distinct-weight subset sum as an Advanced lead.
- [AtCoder DP E task](https://atcoder.jp/contests/dp/tasks/dp_e) and [iastm's editorial](https://atcoder.jp/contests/dp/editorial/17284), posted 2026-03-16: full min-weight-by-exact-value recurrence, descending one-dimensional update and greatest feasible value. Reading this material is not an online acceptance claim.

The abstract/metadata of Karl Bringmann, [*A Near-Linear Pseudopolynomial Time Algorithm for Subset Sum*](https://arxiv.org/abs/1610.04712), v2 2017-01-08 (SODA 2017), were also inspected: randomized near-linear pseudopolynomial positive-integer decision subset sum is a specialized MI18 research lead. No proof/code adaptation or execution is claimed; its result does not directly supply arbitrary-profit optimization or multiplicity-vector counting.

## Limits and handoffs

- Open `/reaudit-review` findings for P015 ([p015.md](<../../00-Guidelines/23-Reaudit Findings/p015.md>)), not yet resolved:
  - 5: detail helpers `validate` and `Optimizer` have no complexity comment.
  - 6: the `Optimizer` constructor body closes on its own line (closing-brace rule).
  - 7: the `KnapsackCounts` constructor body closes on its own line (closing-brace rule).
- Legacy `OLD/Team Notebook/src/algsbetter.cpp` lines 3567–3608 and `algs.cpp` lines 2486–2508 (`knapsack01`, `knapsackComplete`, binary-grouped `knapsackMultiple`) are covered; their `lng` overflow, unchecked domains and zero-weight unboundedness are corrected here. No active callers; archives preserved.
- MI18 owns monotone-queue bounded optimization, meet-in-the-middle and bitset subset sum, specialized value/small-weight domains, generating-function counts and their witnesses. Counting optimal selections, k-best scores, grouped/precedence/multiple-resource constraints and fine-grained algorithms are research leads for that owner.
- Stress mode and other compiler/interpreter versions (including the GCC 14 floor) were not run.
