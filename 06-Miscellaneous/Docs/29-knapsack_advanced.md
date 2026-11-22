# 29-knapsack_advanced.hpp — Evidence

`29-knapsack_advanced.hpp` (batch MI18, package P072) extends the Basic knapsack `13` with monotone-queue and concave (SMAWK) bounded tables, group, two-resource and tree (dependency) knapsacks, removable and FPS counting, k-best and optimal-count tables, branch and bound, Pisinger's balanced subset sum, and the huge-capacity unbounded and small-value bounded knapsacks. Prerequisites: the C01 template (P002), Mathematics `09` `inverseTable` and `14` `fpsExp` (P052, P057) and `13-knapsack.hpp` (P015). Contest profile; independent implementation with no legacy source.

## Contracts

Items are `13`'s `KnapsackItem{weight, value, count}`, with an `lng` weight (since 2026-10-23) and `count = -1` for unlimited, otherwise `0 <= count <= INT_MAX`. Capacities are `0 <= W < INT_MAX` unless stated. Optimization tables hold **exact-weight** maxima in `lll`, with `KNAPSACK_UNREACHABLE = -2^126` where no selection reaches a weight. Finite entries stay above `KNAPSACK_UNREACHABLE / 2`: at most `INT_MAX` copies of each of `n < 2^31` signed 64-bit values give `|sum| <= n * 2^94 < 2^125`. An unlimited zero-weight item with a positive value is rejected, and one with a nonpositive value is never worth taking. A finite zero-weight item with a positive value adds `value * count` to every reachable entry. `kthBestKnapsack` and `countOptimalSelections` instead use **at-most** semantics: `res[w]` ranges over selections of weight at most `w`.

### KNAPSACK_UNREACHABLE, KnapsackItem2D

`KNAPSACK_UNREACHABLE` is the shared sentinel above. `KnapsackItem2D{weight1, weight2, value, count}` has nonnegative weights and the same count rule; an unlimited `(0, 0)`-weight item needs a nonpositive value.

### boundedKnapsackMonotoneQueue

Exact-weight table. For an item of weight `w`, value `v` and `c = min(count, W / w)` usable copies, each residue class `r + t * w` satisfies `dp'[t] = t * v + max over s in [t - c, t] of (dp[s] - s * v)`, a sliding-window maximum over the last `c + 1` positions kept in a monotone deque; unreachable states never enter it. `O((n + 1) * (W + 1))` time, `O(W + 1)` memory.

### smallWeightKnapsack

The same table in `O((n + 1) * log(n + 2) + (D + 1) * (W + 1))` for `D` distinct positive weights. Within one weight class the best `c` copies are the `c` largest values, so their prefix sums `g` are concave, and each residue class needs one max-plus product `h[t] = max over 0 <= t - s <= L of a[s] + g[t - s]`, solved by SMAWK in `O(m)`.

Total monotonicity: give entry `(t, s)` the lexicographic key `(-dist(s, [t - L, t]), a[s] + g(clamp(t - s)))`. The distance is a convex function of `t - s` and `g` is concave on the band. For `s < s'` and `t < t'`, the first component's difference `key(t, s') - key(t, s)` does not decrease from row `t` to row `t'`, by concavity in `t - s`. When the first components tie in both rows, both columns lie in the band in both rows, where `a[s'] - a[s] + g(t - s') - g(t - s)` does not decrease either. So "column `s'` at least as good" carries from row `t` to row `t'`, as SMAWK's reduce and interpolate steps need. Column `s = t` is always in the band, so every row's maximum is a true band value. Unreachable `a[s]` are huge negative numbers whose sums stay below `UNREACHABLE / 2` and are reset to the sentinel.

### groupKnapsack

Exact-weight table taking at most one item from each group; items need `0 <= count <= 1`. Each group reads the previous layer, so two items of one group never combine. `O((G + k + 1) * (W + 1))` time, `O(W + 1)` memory.

### twoDimensionalKnapsack

Exact two-resource table `res[i][j]`. An item of weights `(w1, w2) != (0, 0)` splits the table into chains `(i0 + t * w1, j0 + t * w2)`, one per cell with `i0 < w1` or `j0 < w2`. Along a chain, `res - t * value` takes a monotone-queue sliding-window maximum over the last `c + 1` positions, `c` being the count capped at the chain length (unlimited: the chain length), as in `boundedKnapsackMonotoneQueue`. `(0, 0)` items add their positive value to every reachable entry. `O((W1 + 1) * (W2 + 1) * (n + 1))` time, `O((W1 + 1) * (W2 + 1))` memory plus a queue of `max(W1, W2) + 1` entries.

### treeKnapsack

Exact-weight maxima over vertex sets closed under `parent` (a forest, `-1` for roots, cycles asserted) with 0/1 items; `count = 0` makes a vertex and its subtree unavailable.

Recurrence: in preorder, `T_i[j] = max(T_end(v)[j], T_{i+1}[j - w_v] + value_v)`, skipping or taking the vertex at position `i` with its subtree `[i, end(v))`; `T_N` is the empty selection. Before position `i` is processed, the tables still needed are `T_{i+1}` and `T_end(u)` for the ancestors `u` of the vertex at `i`, since every earlier vertex is such an ancestor or ends at or before `i`. So after vertex `v` the tables inside its subtree are dropped, and `T_end(v)` too unless `v` is its parent's last child (a root's parent is a virtual root); otherwise `end(parent(v)) > end(v)` and no earlier vertex reads it. With the heaviest child placed last (one `max_element` per vertex), `end(u)` differs from `end(parent(u))` only for a light child `u`; at most `log2(n)` light edges lie on any path, so at most `log2(n) + 2` tables coexist. `O((n + 1) * (W + 1))` time, `O(n + (W + 1) * log(n + 2))` memory. Graphs `14` `subtreeMergeDp` is a different `O(n * W^2)` algorithm kept beside it ([duplicates.md](<../../00-Guidelines/23-Sweep Ledgers/duplicates.md>), Kept by the Sweep).

### RemovableKnapsackCounts

`ways[w]` counts multiplicity vectors of exact weight `w` in a caller ring `T` with `+`, `-` and construction from `lng` (for example `lng`, `ModInt`, `ModInt<2>`); `lng` is exact only while every count stays below `2^63`, and a modint gives counts modulo its modulus. The generating function is `prod (1 - x^((c_i + 1) w_i)) / (1 - x^(w_i))` truncated at `x^W`, with numerator 1 for unlimited items. `add` multiplies by the numerator and divides by `1 - x^w` (a prefix sum along stride `w`); `remove` applies the inverse operations. Series with constant term 1 are invertible in any ring, so removals are exact in any order. A removed item must have been added before, and `weight >= 1`. `O(W + 1)` per update.

### kthBestKnapsack

`res[w]` holds the `k >= 1` largest values, decreasing, over selections of weight at most `w`. Per selection, equal values from different multiplicity vectors repeat; with `distinct`, each value appears once. Items take any `count >= -1` (bounded counts: user decision 2026-10-23), and unlimited items need `weight >= 1` or `value <= 0` (unlimited zero-weight items: user decision 2026-10-23). The top `k` of a union depends only on the top `k` of each part, so every update merges sorted lists truncated to `k`:
- 0/1 items update in descending order, unlimited items (and bounded ones with `c >= W / w`) in ascending order, counting every multiplicity vector once.
- A bounded item with `2 <= c < W / w` copies of weight `w` and value `v`: along each residue class, position `s` takes the union over `s - c <= s' <= s` of the lists at `s'` shifted by `(s - s') * v`. Shifting every list by `-s' * v` makes this a sliding-window fold of one associative, commutative operation (truncated merge). `knapsack_advanced_detail::windowFold` keeps the window as a two-stack queue (suffix folds of the older part, one running fold of the newer part, a rebuild when the older part empties), so each position costs `O(1)` merges amortized, `O(k)` each.
- A zero-weight item with `c >= 2` copies, or unlimited with `v <= 0`: the `k` best of copies `0..c` come from `k` consecutive copy counts (any later one has `k` strictly larger or, for value 0, equal predecessors), so each list becomes the union of `n = min(c + 1, k)` (unlimited: `k`) copies of itself shifted by `0, -|v|, ..., -(n - 1)|v|` (plus `c * v` when `v > 0`), built by doubling in `O(k * log(n + 1))`.

`O((n + z * log(k + 1) + 1) * (W + 1) * k)` time for `z` zero-weight items with `count` -1 or at least 2, `O((W + 1) * k)` memory.

### countOptimalSelections

`res[w] = {maximum value over selections of weight at most w, number of selections reaching it}` in a caller type `T` with `+`, `*` and construction from `lng`; `lng` is exact only while every count stays below `2^63`, and a modint gives counts modulo its modulus. Items are as in `kthBestKnapsack`, except that an unlimited zero-weight item needs `value < 0` (value 0 has infinitely many optima) and leaves the table unchanged. A selection reaching the maximum splits uniquely into `t` copies of the last item and an optimal selection of the earlier items at `w - t * weight`, so the count sums over the copy numbers `t` that reach the maximum. Bounded items fold `(value - s * v, count)` pairs (maximum, counts of ties added) with `windowFold`. A zero-weight item with `c` copies adds `c * v` when `v > 0`, nothing when `v < 0`, and multiplies the count by `c + 1` when `v = 0`. `O((n + 1) * (W + 1))` operations.

### knapsackCountsFps

Multiplicity-vector counts of every exact weight `0..t` modulo a prime `M::mod() > t`, the values of `13`'s `KnapsackCounts`. The log of the product is `sum over m of coef[m] * sum over j of x^(j m) / j`, with `coef[w] += 1` per item and `coef[(c + 1) w] -= 1` per bounded item, followed by `fpsExp`. A zero-weight item multiplies everything by `c + 1`; an unlimited zero-weight item is rejected. The 0/1 form (Library Checker `sharp_p_subset_sum`) is Mathematics `12` `subsetSumCount`. `O((t + 1) * log(t + 2) + n)` time, `O(t + 1)` memory.

### knapsackBranchAndBound

`{maximum value within capacity, one optimal 0/1 vector}` for 0/1 items with `lng` weights and any capacity `>= 0`. Zero-weight positive items are always taken; items with nonpositive values or oversized weights never are. The remaining items are sorted by ratio and a depth-first search tries "take" first, pruning with the Dantzig bound: whole items in ratio order, then the fractional break item, floored because values are integers. The bound is the LP relaxation optimum, so it never cuts an optimum. The search is iterative: a dead end resumes at the skip branch of the deepest taken item, so no recursion depth limits `n`. After an improvement the path only takes items (each improves again, values being positive) or skips them until the next dead end, so the witness is copied once there, not at every improving node. Worst case `O(n * 2^n)` (equal ratios), `O(n)` memory.

### pisingerSubsetSum

The largest subset sum at most `t` by Pisinger's balanced algorithm, plus one 0/1 vector when `trace` is set (otherwise the vector is empty). Weights are `>= 0` with maximum `m <= INT_MAX / 2`, since the state array holds `2m` entries, and `t >= 0`. The algorithm starts from the greedy prefix `[0, b)` and tracks sums in `(t - m, t + m]`: at or below `t` it adds the next item, above `t` it removes an item of the prefix. `v[x]` is the largest `j` such that items `[0, j)` of the prefix are still untouched, and each removal pass scans only `j` in `[u[x], v[x])`. Time `O((n + 1) * (m + 1))`; untraced memory `O(n + m + 1)` (item order and state arrays). The index shift keeps sum `t` at index `m - 1`, so a state at exactly `t` can still add (KACTL's layout cannot, harmlessly for the value). The traced layers store `v` after each item, `O(n * (m + 1))` ints.

Reconstruction from layer `i` at state `(x, j)`:
- if the previous layer has `(x, j)`, item `i` was skipped;
- if the previous layer has `(x - w_i, j)`, item `i` was added;
- otherwise prefix item `j` was removed from `x + w_j`, whose current value exceeds `j`, so the walk stays in the layer with a strictly larger `j`.

The walk ends at the initial state because layer 0 holds only it.

### unboundedKnapsackLargeCapacity

Maximum value of weight at most `C` (`0 <= C < 2^63`) for unlimited items with `weight >= 1`; useful weights (`<= C`, positive value) are below `INT_MAX / 2`. Lemma: for `c > m`, with `m` the largest useful weight, `f(c) = max f(a) + f(c - a)` over `|2a - c| <= m`. Proof: split an optimal multiset greedily into halves of weights `s1 <= s2 <= s1 + m`; some integer `a` satisfies `s1 <= a <= c - s2` and `|2a - c| <= m`, because the two intervals intersect and the band has length `m >= 1`.

The base table covers `[0, min(C, 2m)]`. Each level needs `[ceil((l - m) / 2), floor((r + m) / 2)]` of the next, so widths settle near `2m` over `O(log C)` levels. Values fit `lll` (`C * max value < 2^126`). Level bounds are computed in `lll` and positions are scanned by offset, so `C = INT64_MAX` is exact. `O(n + m^2 * log(C + 2))` time, `O(m + log(C + 2))` memory.

### boundedKnapsackSmallValue

Maximum value of weight at most `capacity` (`lng`, any size) for items with `weight >= 1`, `value >= 0` and counts up to `INT_MAX` or unlimited. With `V` the maximum value and `c_i = min(count_i, V - 1)`, the domain needs `V' = sum c_i * value_i < INT_MAX`. `13`'s `ValueKnapsackDP` gives the minimum weight of every value `u <= V'` using at most `c_i` copies; the remaining `count_i - c_i` copies are then filled greedily in ratio order, and the best total is returned.

Proof, with items sorted by ratio:
- **Exchange.** If `i` precedes `j`, `x_j >= v_i` and `count_i - x_i >= v_j`, replacing `v_i` copies of `j` by `v_j` copies of `i` keeps the value and adds no weight. The lexicographically largest optimum admits no such exchange.
- **Decomposition.** Let `p` be the last item used at least `V` times. Every earlier item leaves at most `V - 1` copies unused and every later item uses at most `V - 1`. That optimum is `y + z`: `z` takes all `count_i - c_i` residual copies before `p`, some of `p` and none after, and `y` stays within the `c_i`.
- **Greedy step.** The greedy fill of the capacity left by `y` reaches at least the value of `z`. Greedy value never decreases with capacity: at the first item where one more unit of capacity changes the fill, the extra copy is worth more than anything later items could add. So the minimum-weight `y'` of the same value is at least as good.
- **Feasibility.** Every candidate respects the counts, so the maximum is the optimum.

`V - 1` is tight: `{(1, 1) x 100, (6, 5) unlimited}` at capacity 120 has optimum 116 with 4 = `V - 1` unit copies unused, while `V - 2` copies give 115 (a regression fixture). `O((n + 1) * log(n + 2) + V' * (n + 1 + sum log(c_i + 1)))` time, `O(V' + n)` memory.

### Legacy Mapping

| Removed (user decision 2026-10-23) | Replacement |
|---|---|
| `largeWeightSmallValueKnapsack` | `13`'s `ValueKnapsackDP` with `lng` weights (`minWeight`, `bestWithin`) |

## Feature-to-Test Map

| Public feature | Independent coverage |
|---|---|
| `boundedKnapsackMonotoneQueue`, `smallWeightKnapsack`, `KNAPSACK_UNREACHABLE` | Copy-expansion DP, itself checked against multiplicity enumeration: every pair from a 78-item alphabet (weights 0–3, values -2–3, counts -1/0/1/2/`INT_MAX`) at every capacity `<= 5/7`; seeded `n <= 6`, capacity `< 300`, values to `2^61`; capacity 600/5000/20000 with 3 distinct weights; a hand-checked fixture. |
| `groupKnapsack` | Group enumeration (seeded, at most one per group, values to `2^61`); singleton groups equal the 0/1 table. |
| `twoDimensionalKnapsack`, `KnapsackItem2D` | Multiplicity enumeration (seeded, zero weights, `(0, 0)` items, oversized weights); 1D tables embedded on the diagonal and flat. |
| `treeKnapsack` | Closed-set enumeration (`n < 12`) and an independent `O(n * W^2)` subtree-merge DP (`n < 60`); a forest of roots equals the 0/1 table; a long path against its prefix oracle; a large random forest against the merge DP; a caterpillar against a spine-prefix oracle and a star against a 0/1 table of its leaves. In full and stress, placing the heaviest child first or keeping every light child's skip table pushes the caterpillar or the star above the 4096 MB cap (mutation-checked). |
| `RemovableKnapsackCounts` | 30 random add/remove steps per round, removal in random order, recomputed from scratch by direct polynomial products in `lng`, `ModInt<998244353>` and `ModInt<2>`; zero and `INT_MAX` counts. |
| `knapsackCountsFps` | Direct products and running-sum products (validated against each other) modulo 998244353, 1e9+7, 7 (`t <= 6`) and 2 (`t <= 1`); zero weights and large counts; `t = 2000/30000/100000` with up to 2000 items. |
| `kthBestKnapsack`, `countOptimalSelections` | Every selection enumerated with at-most semantics (counts -1 to 7, zero-weight copies enumerated one by one); per-selection and distinct lists for `k <= 12`, and in a quarter of the rounds `k <= 60` with a first item of weight 0 and up to 69 copies; optimal counts in `lng` and `ModInt`. Unlimited zero-weight items of value `<= 0` (value `< 0` for the counts) enter the oracles as `k + 3` copies: a selection with more copies has `k` at least as good ones with fewer, and none is optimal when the value is negative. A direct recurrence over each item's copy number, validated against the enumeration, checks 31 items at capacity 150/1500/2000 with bounded counts to 29, zero-weight counts to 39 and one unlimited zero-weight item of negative value. Hand-checked `INT_MAX`-copy zero-weight fixtures (positive, negative and zero values, and mixed with a bounded item) and unlimited zero-weight fixtures of values -2, 0 and -3. |
| `knapsackBranchAndBound` | `2^n` enumeration (`n < 15`, weights and values to `2^61`, capacity to `2^62`, equal-ratio worst cases) and a DP for `n < 60`; a DP-checked `n = 2000/100000/200000` instance whose search path is `n` items deep (measured 2026-10-23 with a depth counter in a scratch copy); witness availability, weight and value. |
| `pisingerSubsetSum` | Reachability DP (zero weights, all-equal weights, `t` beyond the total); a bitset oracle for `n` up to 10000 at `m = 800` and for `n = 60` at `m = 1e5`; traced witnesses valid with the reported sum; the untraced result equal with an empty vector. |
| `unboundedKnapsackLargeCapacity` | Plain DP (capacity `< 400`) and the pigeonhole formula with the best-ratio item `s`: some optimum uses fewer than `w_s` other copies, so `f(C) = max over u <= (w_s - 1) * m of g(u) + floor((C - u) / w_s) * v_s`. Capacities to `INT64_MAX` (random capacities within 3 of it), values to `2^61`, oversized and nonpositive items; `m = 100/600/1000` at capacity `1e18`; hand-checked `1e18` and `INT64_MAX` fixtures. |
| `boundedKnapsackSmallValue` | Capacity DP with binary bundles (capacity `< 3000`, counts to `1e9`), near-tie pairs `(w, v)`/`(w + 1, v + 1)` and unit/`(u + 1, u)` pairs; a full-count `ValueKnapsackDP` (capacities to `2^62`); the pigeonhole formula with an unlimited best-ratio item and `INT_MAX` counts; all-fit sums; enumeration with weights log-uniform to `2^62`, capacities to `2^63` and counts 0–5; the tight `V - 1` and near-tie fixtures. |
| Weights above `INT_MAX` | 50/300/1000 rounds: adding items of weight `2^32 + 1` or more (every count kind) to the monotone-queue, small-weight, group, two-resource, tree, k-best, optimal-count, removable and FPS tables leaves them unchanged. |
| Preconditions | 59 probes: both sides of every capacity bound, every size bound, the Pisinger weight bound, negative or zero weights where an assert forbids them, counts below -1, unlimited zero-weight positive items, 0/1 counts, tree size/parent/cycle, the FPS modulus, the Pisinger and branch-and-bound domains, the unbounded width and the `V' < INT_MAX` axis. |

Seeded rounds for quick/full/stress are 300/3000/20000 per family, except group, two-resource and tree at 200/2000/12000 each, counting at 60/500/3000 and Pisinger at 500/5000/30000. The `-O0` debug-container build shrinks the large fixtures and runs stress as full.

Hand mutation check (2026-10-23 and 2026-10-24; one textual replacement per mutant in scratch copies, optimized build, quick mode unless noted; not part of the suite and not a mewt record): all 55 non-equivalent mutants of 59 fail, 53 in quick mode and 2 in full mode at the memory cap (the heavy-first tree order and keeping each light child's skip table). The 24 non-equivalent mutants on the `/finish` additions (window fold, zero-weight doubling and closed forms, residue loops, unlimited zero-weight copies, the `INT64_MAX` split levels, the two-resource chain queue) all fail in quick mode. The 4 survivors are equivalent: the SMAWK tie rule, a no-op group mutant, a base table of `m` instead of `2m` (the lemma holds for `c > m`), and adding `c * v` to the maximum for a zero-weight item when `v = 0`.

## Commands and Results

Conditions: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14; optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's. Memory peaks include the compiler.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; quick preflight peak 1335 MB; optimized 0.1 s compile + 15.9 s run, checked 47.5 s + 15.9 s, ASan-UBSan 53.7 s + 141.4 s; peak 1970 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS; quick preflight peak 1365 MB; optimized 0.2 s + 24.9 s, checked 75.3 s + 21.5 s, ASan-UBSan 91.1 s + 182.8 s; peak 1880 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 58.1 s + 2.3 s, checked 49.6 s + 29.4 s; peak 1359 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 83 MB; optimized 0.1 s + 17.8 s, checked 27.9 s + 7.0 s, ASan-UBSan 28.5 s + 60.9 s; peak 1211 MB |
| 2026-10-24 | `python3 '96-Local Testing/06-Miscellaneous/29-knapsack_advanced_tester.py' --mode quick --seed 1` | PASS, 2 configurations, 59 probes, peak 836 MB |
| 2026-10-24 | `python3 '96-Local Testing/06-Miscellaneous/29-knapsack_advanced_tester.py' --mode full --seed 1` | PASS, 3 configurations, 122,042 cases / 21.2 million checks per configuration (checked build 121,961 / 21.1 million), peak 1092 MB |
| 2026-10-24 | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/29-knapsack_advanced_tester.py' --mode full --seed 2` | PASS, 3 configurations, peak 1097 MB |
| 2026-10-24 | `python3 '96-Local Testing/06-Miscellaneous/29-knapsack_advanced_tester.py' --mode stress --seed 3` | PASS, 3 configurations, optimized 494,833 cases / 129.8 million checks (debug-container and ASan builds run stress as full), peak 1091 MB |
| 2026-10-24 | `python3 '96-Local Testing/02-integration.py'` | PASS, 174 standalone/aggregate headers, multi-TU, workspace, peak 704 MB |
| 2026-10-24 | `python3 '96-Local Testing/03-consistency.py'` | no errors |

On 2026-10-24, ASan stress alone took 119 s (ASan full about 87 s), which is why ASan runs stress as full. The full-mode caterpillar and star keep the peak far below the 4.8 GB that a heavy-first order or kept light-child tables would need.

## Sources

`@researcher` fetched the sources on 2026-10-23 ([00-sources.md](00-sources.md), P072 sweep); the implementations are written independently from the recurrences and proofs above.

- [OI Wiki 背包 DP 进阶](https://oi-wiki.org/dp/knapsack/advanced/) and [背包 DP](https://next.oi-wiki.org/dp/knapsack/): group, two-dimensional, dependency, removable counting, k-th best (HDU 2639) and optimal-count variants.
- [cp-algorithms knapsack](https://cp-algorithms.com/dynamic_programming/knapsack.html) and [ei1333 `dp/knapsack-limitations`](https://ei1333.github.io/library/): the monotone queue; [ei1333 `dp/knapsack-limitations-2`](https://ei1333.github.io/library/) (AOJ DPL_1_I): `boundedKnapsackSmallValue`.
- [KACTL FastKnapsack](https://github.com/kth-competitive-programming/kactl/blob/main/content/various/FastKnapsack.h) and [hitonanode `bounded_knapsack`](https://hitonanode.github.io/cplib-cpp/): Pisinger's balanced algorithm.
- [maspypy `knapsack/`](https://maspypy.github.io/library/) and [Nyaan `dp/branch-and-bound`](https://nyaannyaan.github.io/library/): branch and bound, the Pisinger witness, ancestor-closed (tree) knapsack.
- [noshi91 `axiotis_tzamos_knapsack`, `axiotis_tzamos_unbounded_knapsack`](https://noshi91.github.io/Library/): the concave max-plus per weight class and the doubling recurrence.

## Limits and Handoffs

- Row 29 is partial: `connectedSetKnapsack` and `modularSubsetSum` (Potępa, witness) are planned and not implemented.
- `smallWeightKnapsack` runs its own SMAWK; the package implementing Mathematics `53` concave max-plus convolution (P195) switches it to that (user decision 2026-10-23, recorded in P195's plan note).
- Under `NDEBUG`, a violated asserted precondition may read or write out of bounds; `03-cpp.md` requires only termination without a hang or unbounded allocation (user decisions 2026-10-23 and 2026-10-24).
- Meet in the middle and subset-sum counts by cardinality (maspypy `subset_sum_count_by_size`) belong to row `30`; Boolean bitset subset sum is row `22`.
- Items not adopted in the 2026-10-23 sweep and their reasons: [00-notes.md](00-notes.md), P072 Package Record (MI18, MI34).
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 背包 DP 基础, 背包 DP 进阶, 单调队列优化多重背包, 分组背包, 二维费用背包, 有依赖的背包, 树上背包, 退背包, 背包第k优解, 背包最优方案数, 子集和.

## History

- 2026-10-23: P072 implemented, independently reviewed (tree memory, branch-and-bound copies and depth, complexity symbols, probes, round counts; 5 findings fixed) and verified.
- 2026-10-24: `/finish` audit, five review rounds: bounded multiplicities and unlimited zero-weight items in `kthBestKnapsack` and `countOptimalSelections`, the chain monotone queue in `twoDimensionalKnapsack`, `NDEBUG` limits, renames, 14 probes, hand mutation check (user decisions 2026-10-23 and 2026-10-24); ASan stress runs as full; `[[maybe_unused]]` on the assert-only `check` parameters; full suite passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` with its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed.
