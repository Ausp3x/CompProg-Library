# 23-assignment.hpp — Evidence

Owned by package P043 / GR10 (with `21-matching_bipartite.hpp`); omissions are in [00-notes.md](00-notes.md#p043-omissions). Contest profile: dense shortest-augmenting-path Hungarian, Murty partitioning with warm-started single-row re-augmentation, and sparse successive shortest paths with a binary heap. It replaces the `OLD/algorithms.cpp:4797-4862` excerpt (1-based e-maxx with `INF64` sentinels, `n ≤ m` only); its `Hungarian` interface maps to `hungarian` (Legacy Mapping).

## Contracts

The value type `T` is a signed integral type (`int`, `lng`, `lll`; `static_assert`). Floating costs are not supported: scale to integers. Costs are exact, and every bound below uses `K = max |a|`, `n = min(rows, cols)` and `m = max(rows, cols)`.

### assignment_detail

`State` holds row potentials `u` (index `n` is a virtual row owning every free column), column potentials `v`, `pc[j]` (row of column `j`, `n` if free, −1 while it is a search target) and `pr[i]`. Invariant: for allowed pairs `a_ij − u_i − v_j ≥ 0`, with equality on matched pairs; the virtual row has cost 0 to every column and is tight on its own columns.

`search` is a dense Dijkstra over columns from row `r`. It ends at the first free column (`target = −1`, one Hungarian phase) or at column `target`; in the latter case the virtual row may be traversed once, giving away one free column and taking another. After settling, potentials move only for settled nodes (rows up by `F − d`, columns down by `F − d`, with `F` the end distance), which keeps reduced costs nonnegative and makes the path tight; then the path is flipped. Free columns are never expanded after the virtual row, so a search settles at most `n + 2` columns: O(n·m). `internal` builds the `n × m` cost matrix, transposed when rows exceed columns and negated for maximization, with `numeric_limits<T>::max()` for forbidden pairs (outside the domain, so it never collides).

### AssignmentResult and Hungarian

A default-constructed `AssignmentResult` has `feasible = false`, `cost = 0` and empty vectors, so it copies safely. `hungarian(a, maximize = false, allowed = {})` takes a rectangular matrix (ragged input asserted) and an optional mask of the same shape (nonzero = allowed). It assigns exactly `min(rows, cols)` pairs, every vertex of the smaller side, with minimum (or maximum) total cost.

- `feasible` is false iff forbidden pairs leave no such assignment; then `cost = 0`, all partners are −1 and the potentials are 0.
- `row[i]` and `col[j]` are partners or −1, and `cost` is the exact sum of original entries.
- The potentials certify optimality when minimizing: `u_i + v_j ≤ a_ij` on allowed pairs with equality on assigned pairs, potentials of the larger side ≤ 0 and exactly 0 on its unassigned vertices (columns when rows ≤ cols), and `Σu + Σv = cost`. Maximizing reverses every inequality. This is LP duality for the bipartite assignment polytope, so a caller can verify the result in O(rows · cols).

Domain: `(4n + 4)·K ≤ max(T)`. Proof sketch: the per-row phases are the e-maxx phases; within a phase only the first step can be negative (≥ −K), and the phase total equals the optimal-cost increase. Column potentials therefore stay in `[−2nK, 0]`, row potentials in `[−K, (2n+1)K]`, and every tentative distance below `(4n + 3)K`. Time O(n²·m), memory O(n·m) for the internal matrix.

### kBestAssignments

`kBestAssignments(a, k, maximize = false, allowed = {})` returns up to `k ≥ 0` pairs `(cost, row → col)` in best-first order (nondecreasing cost, nonincreasing when maximizing). They are pairwise distinct as assignments, and each assigns `min(rows, cols)` pairs. Ties come in an unspecified but deterministic order. Fewer than `k` are returned when fewer exist, and none when there is no assignment.

The algorithm is Murty partitioning over the rows of the smaller side in index order. A node fixes rows `[0, f)` and bans a set of columns for row `f`. Child `t ≥ f` additionally fixes rows `f..t−1` and bans the node's column for row `t`, then frees that column and re-augments row `t` alone (`search` with the freed column as target). Fixed rows' columns are blocked for every row, the virtual one included. One search suffices: the parent state minus row `t` is optimal for the sub-problem without row `t` and its column, since its potentials stay feasible and tight; removing edges keeps feasibility, so every alternative assignment differs from it by the shortest alternating path plus nonnegative alternating cycles. The children of a node partition its solution space minus its own solution. Heap entries store only `(cost, parent, row)`, and a popped child is recomputed in O(n·m).

Time O(k·n²·m + k·n·log(k·n)), memory O(k·(n + m) + n·m). Domain: `(16n + 16)·K ≤ max(T)`. Each child search's `F` equals the child's cost increase (the virtual row stays tight on its columns because it relaxes them to their own distance), so along any root-to-node chain the potentials drift by at most `Σ F ≤ 2nK` on top of the Hungarian bounds.

### SparseAssignment and linearSumAssignment

`linearSumAssignment(nl, nr, edges, maximize = false, maxCardinality = true)` takes edges `(u, v, w)` with `u ∈ [0, nl)` and `v ∈ [0, nr)` (asserted) and allows multiedges. It runs successive shortest paths from a virtual source to all free left vertices with Johnson potentials. The initial potentials are 0 on the left and `min(0, min w)` on the right, so all free right vertices always share one potential and the first free right vertex popped closes the shortest augmenting path. Each phase costs O((V + E)·log(V + E)), since the lazy heap holds up to V + E entries and multiedges are unbounded; there are at most `k + 1` phases for result size `k`.

- `curve[j]` is the optimal cost of a matching with exactly `j` edges, for `0 ≤ j ≤ edges.size()`. It is convex: nondecreasing increments when minimizing.
- With `maxCardinality` the result has maximum cardinality and optimal cost among those.
- Without it the search stops before the first augmentation that does not improve, giving the optimal cost over all matchings, with the fewest edges among optimal ones; for nonnegative costs when minimizing that is the empty matching.
- `edges` lists matched edge IDs ascending, and `cost` is their exact sum.

Domain: `16·(min(nl, nr) + 1)·K ≤ max(T)`. Reached potentials are shortest distances, bounded by `(2n+1)K`; unreached potentials telescope through the last augmenting-path cost, so `|h| ≤ 3(2n+1)K` and heap keys stay below `(16n + 9)K`.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `Hungarian<T> h(n, m)`, `h.addEdge(i, j, w)` | `vector<vector<T>> a(n, vector<T>(m))` (unset pairs cost 0, as in the adapter), `a[i][j] = w` |
| `h.solve()` | `auto res = hungarian(a);` then `res.cost` |
| `h.getAssignment()` | `res.row` |

## Feature-to-Test Map

`96-Local Testing/04-Graphs/23-assignment_tester.py` runs on the shared contest runner with `-Werror` warnings: optimized, checked with probes, and ASan/UBSan in full and stress, where UBSan traps any signed overflow at the domain bounds. The oracle enumerates every injective assignment of the smaller side (exact `lll` sums) and every matching of a sparse edge list. Under `_GLIBCXX_DEBUG` only, the large dense and sparse section is capped at 30×40 and 200 vertices, because debug `priority_queue` re-validates the whole heap on each operation; the other sections and all domains are unchanged.

| Operation | Test | Oracle |
|---|---|---|
| `hungarian`, `AssignmentResult` (rectangular, min/max, forbidden, infeasible, default-constructed) | Every forbidden pattern up to 3×3, each in both senses; hand-checked 2×3 before and after an entry update, zero-filled 2×3 and 3×2, each also in both senses; a result default-initialized over storage poisoned with volatile `0xff` stores, then copied; random matrices with random masks (full): 3,000 `int` in [−9, 9] and 3,000 `lng` with ±`max/100` extremes up to 5×5, 750 `lll` with ±`max/100` extremes up to 4×4, 750 `int` in [−2, 2] (many ties) up to 6×6; 150×200 dense | Optimal cost and feasibility by enumeration; hand-computed optima; dual certificate (feasibility, slackness, sign and zero rule, objective) checked exactly; transpose invariance; equals sparse SSP on large inputs; default result is infeasible with cost 0 |
| `kBestAssignments` | Same small corpus with k up to 30 or 100, and k = 0; 8×8 with k = 400 (of 40,320) | Sorted enumeration: rank-by-rank costs, distinctness, validity and cost of each assignment, count when exhausted |
| `linearSumAssignment`, `SparseAssignment` (both modes, min/max, multiedges) | 3,000 random sparse cases per type at the domain bound (full); 150×200 dense as edges; 2,000-vertex sparse | Best cost per cardinality by enumeration; curve equality; matching validity; convexity on large inputs |
| Preconditions | 4 probes: ragged matrix, mask shape, negative k, sparse edge out of range | Expected assertion failure |

Seven planted mutants, quick suite, all killed: row potential off by one, missing ban of the parent column, fixed rows not blocked, missing virtual-row potential update, uncapped Johnson update, maximize potentials not negated, virtual row not relaxed. On 2026-10-13, removing the `feasible` and `cost` member initializers of `AssignmentResult` failed the `-Werror` build (`maybe-uninitialized`); without `-Werror` it fails the default-result check in the optimized and checked builds, and UBSan reports the invalid `bool` load.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, Python 3.14.7, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). The 2026-11-10 library-wide runs used seed 42, beside the other suites, with the build cache emptied before each cold run. The 2026-10-13 prune runs used the optimized, checked (`-O0` with probes) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all`, full and stress) builds, one suite after another, so their times are indicative.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 992 MB; optimized 0.4 s compile + 1.3 s run, checked 33.1 + 5.3 s, ASan-UBSan 38.8 + 13.6 s; MEMORY peak 1355 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 1002 MB; optimized 0.4 + 1.6 s, checked 54.9 + 8.8 s, ASan-UBSan 63.2 + 21.3 s; MEMORY peak 1321 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 37.3 + 0.2 s, checked 32.4 + 16.8 s; MEMORY peak 991 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 57 MB; optimized 0.1 + 6.4 s, checked 15.4 + 6.4 s, ASan-UBSan 17.7 + 66.2 s; MEMORY peak 779 MB |
| 2026-10-13 | `23-assignment_tester.py --mode quick` | PASS 2 configurations, 2,731 cases, 73,692 checks, 4 probes |
| 2026-10-13 | `23-assignment_tester.py --mode full --seed 20261013` | PASS 3 configurations, 14,881 cases, 539,865 checks (optimized and ASan/UBSan; 480,309 checked), 4 probes; 55 s; MEMORY peak 671 MB |
| 2026-10-13 | `CXX=g++-14 23-assignment_tester.py --mode full --seed 20261013` | PASS 3 configurations, 14,881 cases, 539,865 checks (optimized and ASan/UBSan; 480,309 checked), 4 probes; 55 s; MEMORY peak 633 MB |
| 2026-10-13 | `23-assignment_tester.py --mode stress --seed 3` | PASS 3 configurations, 91,381 cases, 3,467,840 checks (optimized and ASan/UBSan; 3,035,615 checked), 4 probes; 113 s; MEMORY peak 842 MB |
| 2026-10-13 | `02-integration.py` | PASS 135 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace; MEMORY peak 650 MB |
| 2026-10-13 | `03-consistency.py` | No errors |
| 2026-10-13 | `03-consistency.py --braces` on the three P043 headers and testers | No closing-brace, comment-cap or free-function violations |

```bash
python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: Contest profile without dispatch thresholds or Barrett or Montgomery reduction. The large cases are correctness checks. Dense Hungarian is O(n²·m) and sparse SSP O(k·(V + E)·log(V)); callers choose by density.

## Sources

Fetched 2026-10-09; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-09-p043).

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Hungarian algorithm](https://cp-algorithms.com/graph/hungarian-algorithm.html) | O(n²m) row-by-row potentials; rectangular use; forbidden-pair behaviour |
| [OI Wiki, bipartite maximum-weight matching](https://oi-wiki.org/graph/graph-matching/bigraph-weight-match/) | KM labels, non-perfect maximum-weight variant |
| [hitonanode, linear_sum_assignment](https://hitonanode.github.io/cplib-cpp/combinatorial_opt/linear_sum_assignment.hpp) | k-best assignments by Murty partitioning with warm duals (design compared; code written independently) |
| [SciPy, linear_sum_assignment](https://docs.scipy.org/doc/scipy/reference/generated/scipy.optimize.linear_sum_assignment.html) | Rectangular and maximize semantics |
| [KACTL, WeightedMatching](https://github.com/kth-competitive-programming/kactl/tree/main/content/graph), [maspypy, hungarian](https://maspypy.github.io/library/), [ei1333, hungarian](https://ei1333.github.io/library/) | Catalog comparison |
| [Library Checker, assignment](https://github.com/yosupo06/library-checker-problems/tree/master/graph) | Square minimum assignment with cost and permutation |

Legacy: `OLD/algorithms.cpp:4797-4862` and `OLD/Team Notebook/src/algsbetter.cpp:3199` were inspected. Both are 1-based e-maxx with `addEdge`, `solve` (returns `−v[0]`), `getAssignment`, an `INF64` sentinel, `n ≤ m` only and zero-initialized pairs. Their interface maps to `hungarian` on a zero-filled matrix (Legacy Mapping); `hungarian` lifts the `n ≤ m` restriction, and the `INF64` sentinel, which overflowed for large costs, is gone.

## Limits and Handoffs

- The row is `partial`: `bottleneckAssignment` (threshold search over `hopcroftKarp`) is missing.
- Min-cost flow is row 20 and general weighted matching row 64.
- Incremental row addition, dynamic cost updates and the auction algorithm stay out, with reasons in [00-notes.md](00-notes.md#p043-omissions).
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): Hungarian algorithm, KM (Kuhn-Munkres), bipartite weighted matching (OI Wiki: 二分图最大权匹配), assignment problem, KM算法, 带权二分图匹配, 指派问题, 二分图最大权完美匹配, 分配问题, 匈牙利算法.

## History

- 2026-10-09: verified (P043); quick, full and stress passed on g++, full on g++-14; independent review: no wrong answers (own Bellman–Ford min-cost-flow oracle, overflow probes at the stated domain bounds).
- 2026-10-13: prune removed the `Hungarian` adapter (Legacy Mapping); its hand-checked matrices moved to `hungarian` with the enumeration oracle and the `legacy-range` probe was deleted. The review found that the default-result check could not fail on value-initialized vector elements; it now default-initializes over volatile-poisoned storage. Full passed on g++ and g++-14, stress on g++.
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
