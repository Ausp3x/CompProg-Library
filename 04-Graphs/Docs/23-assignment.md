# 23-assignment.hpp — evidence

Owned by package P043 / GR10 (with `21-matching_bipartite.hpp`); see [00-notes.md](00-notes.md#p043-package-record). Contest profile: dense shortest-augmenting-path Hungarian, Murty partitioning with warm-started single-row re-augmentation, and sparse successive shortest paths with a binary heap. It replaces the unchanged `OLD/algorithms.cpp:4797-4862` excerpt (1-based e-maxx with `INF64` sentinels and `n ≤ m` only). `Hungarian` remains as a legacy adapter.

## Contracts

Value type `T` is a signed integral type (`int`, `lng`, `lll`; `static_assert`). Floating costs are not supported: scale to integers. Costs are exact, and every bound below is in terms of `K = max |a|` and `n = min(rows, cols)`, `m = max(rows, cols)`.

### assignment_detail

`State` holds row potentials `u` (index `n` is a virtual row owning every free column), column potentials `v`, `pc[j]` (row of column `j`, `n` if free, −1 while it is a search target) and `pr[i]`. Invariant: for allowed pairs, `a_ij − u_i − v_j ≥ 0`, with equality on matched pairs; the virtual row has cost 0 to every column and is tight on its own columns.

`search` is a dense Dijkstra over columns from row `r`. It ends at the first free column (`target = −1`, one Hungarian phase) or at column `target`; in the latter case the virtual row may be traversed once, giving away one free column and taking another. After settling, potentials move only for settled nodes (rows up by `F − d`, columns down by `F − d`, `F` the end distance), which keeps reduced costs nonnegative and makes the path tight; then the path is flipped. Free columns are never expanded after the virtual row, so a search settles at most `n + 2` columns: O(n·m). `internal` builds the `n × m` cost matrix, transposed when rows exceed columns, negated for maximization, and uses `numeric_limits<T>::max()` for forbidden pairs (outside the domain, so it never collides).

### AssignmentResult and hungarian

A default-constructed `AssignmentResult` has `feasible = false` and `cost = 0` (so an unsolved `Hungarian` copies safely). `hungarian(a, maximize = false, allowed = {})` takes a rectangular matrix (ragged input asserted) and an optional mask of the same shape (nonzero = allowed). It assigns exactly `min(rows, cols)` pairs, i.e. every vertex of the smaller side, with minimum (or maximum) total cost. The result:

- `feasible` is false iff forbidden pairs leave no such assignment; then `cost = 0`, all partners are −1 and the potentials are 0.
- `row[i]` and `col[j]` are partners or −1, and `cost` is the exact sum of original entries.
- The potentials certify optimality in the minimize case: `u_i + v_j ≤ a_ij` on allowed pairs, equality on assigned pairs, potentials of the larger side ≤ 0 and exactly 0 on its unassigned vertices (columns when rows ≤ cols), and `Σu + Σv = cost`. For maximize every inequality is reversed. This is LP duality for the bipartite assignment polytope, so a caller can verify the result in O(rows · cols).

Domain: `(4n + 4)·K ≤ max(T)`. Proof sketch: the per-row phases are the e-maxx phases. Within a phase only the first step can be negative (≥ −K), and the phase total equals the optimal-cost increase. Hence column potentials stay in `[−2nK, 0]`, row potentials in `[−K, (2n+1)K]`, and every tentative distance is below `(4n + 3)K`. Time O(n²·m), memory O(n·m) for the internal matrix.

### kBestAssignments

`kBestAssignments(a, k, maximize = false, allowed = {})` returns up to `k ≥ 0` pairs `(cost, row → col)`. They come in best-first order (nondecreasing cost, or nonincreasing for maximize), are pairwise distinct as assignments, and each assigns `min(rows, cols)` pairs. Ties come in an unspecified but deterministic order. Fewer than `k` are returned when fewer exist, and none when there is no assignment.

The algorithm is Murty partitioning over the rows of the smaller side in index order. A node fixes rows `[0, f)` and bans a set of columns for row `f`. Child `t ≥ f` additionally fixes rows `f..t−1` and bans the node's column for row `t`, then frees that column and re-augments row `t` alone (`search` with the freed column as target). Fixed rows' columns are blocked for every row, including the virtual one. One search suffices: the parent state minus row `t` is optimal for the sub-problem without row `t` and its column, since its potentials stay feasible and tight. Removing edges keeps feasibility, so every alternative assignment differs from it by the shortest alternating path plus nonnegative alternating cycles. The children of a node partition its solution space minus its own solution. Heap entries store only `(cost, parent, row)`; a popped child is recomputed in O(n·m).

Time O(k·n²·m + k·n·log(k·n)), memory O(k·(n + m) + n·m). Domain: `(16n + 16)·K ≤ max(T)`. Each child search's `F` equals the child's cost increase (the virtual row stays tight on its columns because it relaxes them to their own distance). So along any root-to-node chain the potentials drift by at most `Σ F ≤ 2nK`, on top of the Hungarian bounds.

### SparseAssignment and linearSumAssignment

`linearSumAssignment(nl, nr, edges, maximize = false, maxCardinality = true)` takes edges `(u, v, w)` with `u ∈ [0, nl)`, `v ∈ [0, nr)` (asserted) and allows multiedges. It runs successive shortest paths from a virtual source to all free left vertices with Johnson potentials. The initial potentials are 0 on the left and `min(0, min w)` on the right, so all free right vertices always share one potential and the first free right vertex popped closes the shortest augmenting path. Each phase costs O((V + E)·log(V + E)) (the lazy heap holds up to V + E entries, and multiedges are unbounded) and there are at most `k + 1` phases for result size `k`.

Result:
- `curve[j]` is the optimal cost of a matching with exactly `j` edges, for `0 ≤ j ≤ edges.size()`. It is convex: nondecreasing increments for minimize.
- With `maxCardinality`, the result has maximum cardinality and optimal cost among those.
- Without it, the search stops before the first augmentation that does not improve, giving the optimal cost over all matchings with the fewest edges among optimal ones. For nonnegative minimize costs that is the empty matching.
- `edges` lists matched edge IDs in ascending order, and `cost` is their exact sum.

Domain: `16·(min(nl, nr) + 1)·K ≤ max(T)`. Reached potentials are shortest distances, bounded by `(2n+1)K`. Unreached potentials telescope through the last augmenting-path cost, so `|h| ≤ 3(2n+1)K` and heap keys stay below `(16n + 9)K`.

### Hungarian (legacy adapter)

`Hungarian<T>(n, m)` stores an `n × m` matrix initialised to 0; `addEdge(i, j, w)` sets one entry (range asserted); `solve()` returns `hungarian(a).cost` (minimum, any shape) and stores the result in `res`; `getAssignment()` returns `res.row`. As in the legacy code, unset pairs cost 0, so it is the minimum over complete assignments of the smaller side.

## Feature-to-test map

`96-Local Testing/04-Graphs/23-assignment_tester.py` (shared graph runner with `-Werror` warnings, optimized, checked with probes, ASan/UBSan in full/stress; UBSan traps any signed overflow at the domain bounds). The oracle enumerates every injective assignment of the smaller side (exact `lll` sums) and every matching of a sparse edge list. Under `_GLIBCXX_DEBUG` only, the large dense/sparse section is capped at 30×40 and 200 vertices, because debug `priority_queue` re-validates the whole heap on each operation; the other sections and all domains are unchanged.

| Operation | Test | Oracle |
|---|---|---|
| `hungarian`, `AssignmentResult` (rectangular, min/max, forbidden, infeasible) | Every forbidden pattern up to 3×3, each in both senses; random matrices with random masks (full): 3,000 `int` in [−9, 9] and 3,000 `lng` with ±`max/100` extremes up to 5×5, 750 `lll` with ±`max/100` extremes up to 4×4, 750 `int` in [−2, 2] (many ties) up to 6×6; 150×200 dense | Optimal cost and feasibility by enumeration; dual certificate (feasibility, slackness, sign and zero rule, objective) checked exactly; transpose invariance; equals sparse SSP on large inputs |
| `kBestAssignments` | Same small corpus with k up to 30 or 100, and k = 0; 8×8 with k = 400 (of 40,320) | Sorted enumeration: rank-by-rank costs, distinctness, validity and cost of each assignment, count when exhausted |
| `linearSumAssignment`, `SparseAssignment` (both modes, min/max, multiedges) | 3,000 random sparse cases per type at the domain bound (full); 150×200 dense as edges; 2,000-vertex sparse | Best cost per cardinality by enumeration; curve equality; matching validity; convexity on large inputs |
| `Hungarian` legacy: `addEdge`, `solve`, `getAssignment` | 2×3 with re-solve after an update, unset pairs, 3×2; copies of unsolved adapters (vector growth) | Hand-checked optimum; default result is infeasible with cost 0 |
| Preconditions | 5 probes: ragged matrix, mask shape, negative k, sparse edge out of range, legacy index out of range | Expected assertion failure |

Planted mutants (quick suite): row potential off by one, missing ban of the parent column, fixed rows not blocked, missing virtual-row potential update, uncapped Johnson update, maximize potentials not negated, virtual row not relaxed — all 7 killed.

## Commands and results

2026-10-09, GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `23-assignment_tester.py --mode quick` | PASS 2 configurations, 2,723 cases, 80,834 checks, 5 probes |
| `23-assignment_tester.py --mode full --seed 1` | PASS 3 configurations, 14,873 cases, 543,083 checks (optimized), 5 probes; MEMORY peak 684 MB |
| `CXX=g++-14 23-assignment_tester.py --mode full --seed 2` | PASS 3 configurations, 14,873 cases, 536,487 checks (optimized), 5 probes; MEMORY peak 652 MB |
| `23-assignment_tester.py --mode stress --seed 3` | PASS 3 configurations, 91,373 cases, 3,467,440 checks (optimized), 5 probes; MEMORY peak 808 MB |
| `02-integration.py --sanitizers` | PASS 113 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 2,755 MB |
| `03-consistency.py` | No P043 errors; one error predates P043 at `HEAD` (`16-poly_tester.py` missing from `QUICK` in the committed `01-run.py`) |

```bash
python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/23-assignment_tester.py' --mode stress --seed 3
```

## Benchmarks

None required: contest profile without dispatch thresholds or Barrett/Montgomery. The large cases are correctness checks; dense Hungarian is O(n²·m) and sparse SSP O(k·(V + E)·log(V)), and callers choose by density.

## Sources

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Hungarian algorithm](https://cp-algorithms.com/graph/hungarian-algorithm.html) | O(n²m) row-by-row potentials; rectangular use; forbidden-pair behaviour |
| [OI Wiki, bipartite maximum-weight matching](https://oi-wiki.org/graph/graph-matching/bigraph-weight-match/) | KM labels, non-perfect maximum-weight variant |
| [hitonanode, linear_sum_assignment](https://hitonanode.github.io/cplib-cpp/combinatorial_opt/linear_sum_assignment.hpp) | k-best assignments by Murty partitioning with warm duals (design compared; code written independently) |
| [SciPy, linear_sum_assignment](https://docs.scipy.org/doc/scipy/reference/generated/scipy.optimize.linear_sum_assignment.html) | Rectangular and maximize semantics |
| [KACTL, WeightedMatching](https://github.com/kth-competitive-programming/kactl/tree/main/content/graph), [maspypy, hungarian](https://maspypy.github.io/library/), [ei1333, hungarian](https://ei1333.github.io/library/) | Catalog comparison |
| [Library Checker, assignment](https://github.com/yosupo06/library-checker-problems/tree/master/graph) | Square minimum assignment with cost and permutation |

Legacy: `OLD/algorithms.cpp:4797-4862` and `OLD/Team Notebook/src/algsbetter.cpp:3199` were inspected. Both are 1-based e-maxx with `addEdge`, `solve` (returns `−v[0]`), `getAssignment`, an `INF64` sentinel, `n ≤ m` only and zero-initialised pairs. The adapter keeps the interface and the zero default and lifts the `n ≤ m` restriction; the `INF64` sentinel (which overflowed for large costs) is gone.

## Limits and handoffs

Min-cost flow is row 20 and general weighted matching row 64. Incremental row addition, dynamic cost updates, bottleneck assignment and the auction algorithm are omitted with reasons in [00-notes.md](00-notes.md#p043-omissions).
