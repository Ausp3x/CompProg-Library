# 54-kbest_enumeration.hpp — evidence

`54-kbest_enumeration.hpp` (batch MI34, package P072) enumerates the `k` best pair sums, subarray sums, subset sums, fixed-size subset sums and one-per-list sums with priority-queue frontiers, and the `k` best solutions of a 0/1 problem by Lawler partitioning over a caller oracle. Prerequisites: the C01 template (P002) and the Data Structures `00-monoids.hpp` `Monoid` and `04-sparsetable.hpp` `SparseTable` (verified). Contest profile; independent implementation, no legacy source.

## Contracts

Common rules. Values live in an ordered group `T` under the strict weak order `cmp` (default `std::less<>`; every operation uses the given object, so a comparator may carry state): `+`, `-` and `T(0)` exist and `cmp(x, y)` implies `cmp(x + z, y + z)`. Passing `std::greater<>()` enumerates the `k` largest instead (the row's `kLargest`, not a separate function; user decision 2026-10-23). `k >= 0`; fewer outputs appear when fewer candidates exist, and cost is charged by `k`. Sums must not overflow `T`. Outputs are in nondecreasing order under `cmp`. Pair and subarray sums break ties by index tuple; the others break ties by discovery order, which is deterministic for a given input.

### kSmallestPairSums

Index pairs `(i, j)` of the `k` smallest `a[i] + b[j]` for `a`, `b` sorted under `cmp`, ordered by `(sum, i, j)`. Row `i < min(n, k)` starts at `(i, 0)`, and popping `(i, j)` pushes `(i, j + 1)`. Rows are nondecreasing in `j`, so the frontier holds every active row's smallest unpopped pair, and the heap order `(sum, i, j)` pops the global minimum. A pair in row `i >= k` is preceded by the `k` pairs `(t, 0)` with `t < k`, so those rows never matter. `O((k + 1) * log(k + 2))` time, `O(k + 1)` memory.

### kSmallestSubarraySums

Ranges `[l, r)` with `min_len <= r - l <= max_len` (`1 <= min_len <= max_len`) of the `k` smallest sums, ordered by `(sum, l, r)`. For each `l` the admissible ends form an interval; its best end is the leftmost argmin of the prefix sums, taken from a `SparseTable` over prefix indices whose `Monoid` lambda compares with the caller's `cmp` object and keeps the left operand on ties, so overlapping blocks still return the leftmost argmin and stateful comparators work. Popping `(l, r)` from interval `[lo, hi]` pushes `[lo, r - 1]` and `[r + 1, hi]`. The left part has strictly larger sums and the right part sums at least as large with larger `r`, so no child precedes its parent in `(sum, l, r)` order, and the intervals partition the unpopped ranges. `O((n + k + 1) * log(n + k + 2))` time, `O((n + 1) * log(n + 2) + k)` memory. NOI 2010 超级钢琴 is the `std::greater` case.

### KSmallestSubsets, kSmallestSubsetSums

The `k` smallest of the `2^n` subset sums. The first output is the set of `cmp`-negative elements (the minimum); elements with `cmp(a_i, 0)` start taken, and choosing one in the frontier drops it at cost `-a_i`. Every subset is the symmetric difference of that base with a set `S` of positions in the costs `b` sorted ascending, at sum `base + sum_S b`. Frontier: a state is `S` with its largest position `i`; the children add `i + 1` or replace `i` by `i + 1`. Every nonempty `S` has one parent: drop its largest position `p` if `p - 1` is in `S`, otherwise move `p` to `p - 1`. Both children cost at least their parent because `b` is sorted and nonnegative. Each output stores one node `(position, next)` of a persistent list: the add child links to the parent's node, the replace child to the parent's tail. `subset(e)` returns the sorted original indices in `O(n)` and asserts `0 <= e < sums.size()`. `O((n + 1) * log(n + 2) + k * log(k + 1))` time (heap of at most `2k` entries), `O(n + k)` memory.

### kSmallestFixedSizeSubsetSums

The `k` smallest sums of `m`-element subsets (`m >= 0`; none when `m > n`; only the empty set when `m = 0`), sharing `KSmallestSubsets`. Elements are sorted ascending. A state `(j, p, lim, list)` keeps elements `0..j-1` at positions `0..j-1`, element `j` at position `p < lim`, and the elements above `j` fixed in `list`; the start is `(m - 1, m - 1, n)`. Children: element `j` moves to `p + 1` if `p + 1 < lim`; element `j - 1` starts moving to position `j`, freezing `j` at `p`, if `0 < j < p`. For a subset `q_0 < ... < q_{m-1}` other than the first `m` positions, take `j` as the smallest index with `q_j > j`. Its parent is `(j, q_j - 1)` if `q_j > j + 1` or `j = m - 1`, and otherwise the state with element `j + 1` at `q_{j+1}` that spawned it. So every subset has exactly one parent and appears once. Both moves raise one element's position, so sums never decrease. Same time and memory bounds as `kSmallestSubsetSums`.

### KSmallestChoices, kSmallestSortedListSums

The `k` smallest sums taking one element from every list, each list sorted under `cmp`. An empty list gives no outputs; no lists give the single empty choice. Lists with at least two elements are stably sorted by first gap `l[1] - l[0]`. A state `(p, t, rest)` has sorted list `p` at element `t >= 1`, later lists at element 0, and earlier choices in `rest`. Children: advance to `t + 1`, next (list `p + 1` takes element 1), and swap when `t = 1` (list `p` returns to 0, list `p + 1` takes 1).

The parent of `(p, t)` is `(p, t - 1)` for `t > 1`. For `t = 1` it is the next child of list `p - 1` when that list is nonzero, the swap child otherwise, and the root push when `p = 0`. Every move adds a nonnegative amount: a later gap, a next gap, or the difference of two gaps in sorted order. `choice(e)` returns the chosen element of every list in `O(m)` and asserts the range. `O((m + 1) * log(m + 2) + k * log(k + 1))` time (heap of at most `3k` entries), `O(m + k)` memory; only the elements reached are read.

### lawlerKBest

The `k` best solutions of a 0/1 problem with `n` variables, in order. `oracle(fixed)` gets `fixed[i]` in `{-1 free, 0, 1}` and returns `std::optional{value, x}` with `x` minimal under `cmp`, or `nullopt` when infeasible. For an output with constraints `f`, solution `x` and free variables `i_1 < ... < i_r`, child `c` fixes `i_1..i_{c-1}` to `x` and flips `i_c`. Together with `{x}` these `r` subproblems partition the solutions of `f`, so every solution appears once and the heap always pops the best remaining one.

A child is pushed with its optimum value and solved again when popped, so memory stays `O(k * n)` instead of storing a solution per child. The re-solve is the same subproblem, so its optimum value is the one pushed; any optimal `x` is accepted, and the re-solve is asserted feasible with a value equal to the pushed one under `cmp`. Ties are broken by discovery. `O((k * (n + 1) + 1) * oracle + k * n * log(k * n + 2))` time, `O(k * n)` memory besides the oracle's.

### Legacy mapping

| Removed (2026-10-23 sweep) | Replacement |
|---|---|
| `kSmallestSubsetSumsWitness` | `kSmallestSubsetSums`, whose witnesses cost `O(1)` per output |

## Feature-to-test map

| Public feature | Independent coverage |
|---|---|
| `kSmallestPairSums` | All `n * m` pairs sorted by `(sum, i, j)` under the same comparator; exact output order for every `k <= n * m + 1`; large `n = k` with a two-pointer certificate (every pair below the last sum is present, all distinct, sorted). |
| `kSmallestSubarraySums` | All ranges with length in `[min_len, max_len]` sorted by `(sum, l, r)`; exact order for every `k` and every `min_len <= max_len <= n + 1`; large `std::greater` order, validity and distinctness. |
| `kSmallestSubsetSums`, `KSmallestSubsets::subset` | All `2^n` subset sums sorted; sums equal to the sorted prefix; every witness valid, sorted, sum-consistent and pairwise distinct; first output equals the negative set; large order and sampled witnesses. |
| `kSmallestFixedSizeSubsetSums` | All `m`-subsets for every `m <= n + 1`; the same witness checks with exact size `m`; large minimum equals the `n / 2` smallest elements. |
| `kSmallestSortedListSums`, `KSmallestChoices::choice` | Every choice vector enumerated; empty lists, no lists, singletons, ties; choices valid, sum-consistent and distinct; large minimum, order and sampled witnesses. |
| `lawlerKBest` | Every feasible 0/1 vector of `sum c_i x_i + d * popcount(x)^2` under a weight limit (a nonlinear objective), sorted; exact values, feasible distinct solutions, oracle calls at most `1 + k * (n + 1)`. Half the cases use an oracle returning a uniformly random optimum, so a re-solve may return another vector of equal value (all-zero objectives included). |
| Comparators | Besides `std::less<>` and `std::greater<>`, every operation runs a quarter of the seeded rounds under a comparator carrying state (`s * x < s * y` with `s = 1` and `s = -1`; a default-constructed one ties everything, so ignoring the passed object fails). |
| `kLargest` | Every operation above also runs under `std::greater<>()`. |
| Element types | `int` subset sums, `double` pair and list sums. |
| Preconditions | 16 probes: negative `k` for each operation, `min_len = 0`, `min_len > max_len`, `m < 0`, out-of-range `subset` and `choice` indices on both sides, `n < 0`, a Lawler oracle that fails to re-solve its own output, and one whose re-solve reports another value. |

Exhaustive cases use the alphabet `{-2, 0, 1, 1, 3}` (duplicates and zero included) through `n = 3/4/5` (quick/full/stress) with every `k` (`n = 5`: `k <= 6` and `31..33`, since outputs for smaller `k` are prefixes of `k = 2^n`), every `m` and every `L <= R`. Seeded rounds run 150/2000/12000 per standard comparator and 37/500/3000 per stateful one (`n <= 10`, Lawler `n <= 6`, values to `1e12`); large fixtures use `n = k = 2000/100000/300000`. The `-O0` debug-container build caps large fixtures at `n = 1000` and runs stress as full, because its heaps validate the whole range on every push and pop.

## Commands and results

Run 2026-10-24: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations: optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/06-Miscellaneous/_00_runner.py>); MEMORY peaks include the compiler.

```bash
python3 '96-Local Testing/06-Miscellaneous/54-kbest_enumeration_tester.py' --mode quick --seed 1               # PASS, 2 configurations, 16 probes, peak 643 MB
python3 '96-Local Testing/06-Miscellaneous/54-kbest_enumeration_tester.py' --mode full --seed 1                # PASS, 3 configurations, peak 753 MB
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/54-kbest_enumeration_tester.py' --mode full --seed 2     # PASS, 3 configurations, peak 748 MB
python3 '96-Local Testing/06-Miscellaneous/54-kbest_enumeration_tester.py' --mode stress --seed 3              # PASS, 3 configurations, peak 755 MB
python3 '96-Local Testing/02-integration.py'    # PASS, 174 standalone/aggregate headers, multi-TU, workspace, peak 704 MB
python3 '96-Local Testing/03-consistency.py'    # no errors
```

Per configuration, full ran 638,930 cases / 11.4 million checks (the checked build 10.8 million checks) and optimized stress 3,145,180 cases / 63.8 million checks; the debug-container and ASan builds run stress as full: alone, ASan stress took 93 s of the runner's 180 s per-binary limit and 172 s under parallel load (ASan full: about 34 s).

## Sources

Fetched 2026-10-23 by `@researcher` ([00-sources.md](00-sources.md), P072 sweep); the implementations are independently written:

- [OI Wiki 堆](https://oi-wiki.org/ds/heap/): frontier-heap enumeration of pair and subset sums.
- [cnblogs autoint, NOI 2010 超级钢琴](https://www.cnblogs.com/autoint/p/10671934.html): range splitting at the argmin of prefix sums for length-bounded subarray sums.
- [Qiita tsukasa\_\_diary, Lawler k-best](https://qiita.com/tsukasa__diary/items/8bc2947d1f65d1451bb3): partitioning by fixing a prefix and flipping one variable.
- [algo.monster 2386](https://algo.monster/liteproblems/2386): k-th largest subsequence sum, the `std::greater` case of `kSmallestSubsetSums`.
- Codeforces blogs 61211, 146040 and 4190 (HTTP 403) and the Luogu CCO 2020 "Shopping Plans" article (HTTP 523) were not read. `kSmallestFixedSizeSubsetSums` and the list frontier are derived and proved above.

## Limits and handoffs

- k-th value selection without enumeration (sorted matrix, threshold DFS) is Mathematics `02` `binSearch` over a monotone count; persistent-heap k-best paths are Graphs `32`; XOR k-best runs the `kSmallestSubarraySums` range split with the Data Structures `26` trie as the per-range oracle ([00-notes.md](00-notes.md), P072 package record).
- Lawler's oracle must be exact; an approximate oracle breaks the order guarantee.
- Under `NDEBUG`, any violated asserted precondition may read or write out of bounds; `03-cpp.md` requires only termination without a hang or unbounded allocation (user decisions 2026-10-23 and 2026-10-24).

## History

- 2026-10-23: implemented, independent review (complexity comments, probes; 2 findings fixed) and verified (P072).
- 2026-10-24: `/finish` audit, five review rounds (`min_len`/`max_len`, Lawler equal-value assert and bound, per-name seeds, 1 probe, `NDEBUG` limit, `kLargest` kept by user decision); ASan stress runs as full; full suite passed on g++ and g++-14.
