# 03-toposort.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp`, `02-traversal.hpp` and `04-dsu.hpp` through `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### TopologicalResult, topologicalSort and topologicalSortDfs

All require a directed graph and ignore weights. `topologicalSort(g)` is Kahn's algorithm with a FIFO queue (O(n + m)). `topologicalSortSmallest(g)` and `topologicalSortLargest(g)` run Kahn with a min-heap and a max-heap (O(m + n log n)) and return the lexicographically smallest and largest orders. `topologicalSortEarliest(g)` returns the order whose position array is lexicographically smallest: vertex 0 as early as possible, then vertex 1, and so on (the "labeling" problem); it is not the smallest order in general (arcs `2 -> 0`, vertex 1 free: smallest `1 2 0`, earliest `2 0 1`). Parallel arcs count separately in in-degrees. `topologicalSortDfs(g)` returns the reverse finishing order of `dfsForest(g)`: roots in increasing vertex order, out-arcs in adjacency (ID) order. On a cycle, `acyclic` is false, `order` is empty and `cycle` is a directed witness from `dfsForest`. `unique` is true exactly when the order is the only topological order; an empty or one-vertex graph has a unique order, and a cyclic graph has `unique = false`.

Correctness of the heap sorts: Kahn may emit any ready vertex next, and every prefix it builds extends to a complete order, so taking the smallest (largest) ready vertex at each step is the greedy choice that is lexicographically optimal at every position. `topologicalSortEarliest` runs the max-heap sort on `g.reverse()` and reverses the result: the reversed order fills positions from the back, and putting the largest available sink last is the classic exchange argument that the position of vertex 0 is then minimal, then that of vertex 1 among such orders, and so on (the smallest-first greedy on `g` fails, as the example above shows). The reverse graph costs O(n + m) memory.

### GenerationsResult and topologicalGenerations

`topologicalGenerations(g)` returns `layers`, where `layers[i]` holds, in increasing vertex order, the vertices whose longest incoming path has exactly `i` arcs (NetworkX `topological_generations`). Each layer is an antichain and the layer count equals the vertex count of a longest path, so it is a minimum antichain partition (Mirsky's theorem). On a cycle `acyclic` is false, `layers` is empty and `cycle` is the witness of `topologicalSort`. A longest-path DP along the FIFO order computes the levels and a bucket pass emits them sorted, O(n + m).

### allTopologicalSorts

`allTopologicalSorts(g, limit)` asserts `limit >= 0` (a negative limit is a violated precondition; under `NDEBUG` the call ends in O(n + m)) and returns up to `limit` distinct topological orders, all of them when there are at most `limit`; the result is empty exactly when `limit == 0` or `g` has a cycle (checked first by `topologicalSort`, so a cyclic graph never starts an exponential search), and an empty graph has one empty order. The first order is `topologicalSort(g).order`; the order of the later outputs is unspecified.

Enumeration is Varol and Rotem's algorithm (Knuth, TAOCP 7.2.1.2, Algorithm V; user decision 2026-11-14, replacing Knuth–Szwarcfiter backtracking, which cost Θ(n + m) per order on a dense DAG). Vertices are relabelled by their position in the Kahn order, so the identity is a topological order. Element `k` (from `n - 1` down) moves one place left past its neighbour `l` unless `l -> k` is an arc. When it cannot move, it rotates back to its home position `k` and the next smaller element tries. Invariant: while `k` moves, every larger element is at home to its right, so its left neighbour `l` is smaller. The elements below `k` keep their relative order during its sweep, so the sweeps form a mixed-radix counter in which every linear extension appears exactly once. Two neighbours in a linear extension are comparable only through a direct arc, since a path between them would pass through a vertex placed between them. So one arc test per step suffices, a binary search in `k`'s sorted, deduplicated predecessor labels. Each output costs O(n) for its copy, O(n) for the failed tests of the larger elements, and the rotations, which are paid by earlier moves. Each test costs O(log n), so the bound is O(n + m + limit * n * log(n)) time and O(n + m + limit * n) memory with the output. A hashed arc set would make a test O(1) expected, but the library's hash (`06-Miscellaneous/02-customhash.hpp`, `02-Data Structures/61-hashmap.hpp`) belongs to packages the plan schedules after P010. The search is iterative.

A topological order is unique exactly when every consecutive pair is joined by an arc: if some consecutive pair `x, y` has no arc `x -> y`, no path joins them either (any path from `x` to `y` would pass through vertices strictly between them in the order), so swapping them gives a second order; conversely, a Hamiltonian path fixes the order. `toposort_detail::ordered` checks this in O(n + m) for both Kahn and DFS orders.

### Legacy mapping

- `topologicalSort(g, true)` → `topologicalSortSmallest(g)` (flags choosing between operations become named mirrors, user decision 2026-11-14); `topologicalSort(g, false)` → `topologicalSort(g)`.

## Feature-to-test map

`96-Local Testing/04-Graphs/03-toposort_tester.py` uses the shared contest runner, which builds optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and full/stress `-O1` ASan/UBSan variants. The full corpus is 4,697 exhaustive graphs (loopless through four vertices, looped through three) and 1,200 random graphs, each with a parallel-edge variant when it has an edge. Quick uses smaller exhaustive domains, 100 random cases and 10,000-vertex chains; stress extends random counts to 10,000 and chains to 500,000. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `topologicalSort` (Kahn FIFO), `TopologicalResult: acyclic, order, cycle` | Exhaustive and random graphs, parallel arcs; 200,000-vertex chains and closed chains; oriented witnesses | Independent permutation enumeration through seven vertices gives existence |
| `topologicalSortSmallest`, `topologicalSortLargest` | Same corpus; 2,000 simultaneously ready vertices; 200,000-vertex ascending and descending chains | The first and the last valid permutation |
| `topologicalSortEarliest` | Same corpus; the hand-solved `2 -> 0` case; a deep case (free odd vertices, descending even chain) where it differs from the smallest order | The valid permutation with the lexicographically smallest position array |
| `topologicalSortDfs` | Same corpus | Permutation enumeration for validity; through 2,000 vertices the exact reverse finishing order of an independent recursive DFS (roots increasing, arcs in adjacency order) |
| `TopologicalResult: unique` | Same corpus; a chain with skip arcs with and without one missing link | Fewer than two valid permutations |
| `GenerationsResult`, `topologicalGenerations` | Same corpus, deep chains, cyclic cases with witnesses | Longest incoming path per vertex by n rounds of arc relaxation |
| `allTopologicalSorts` | Limits 0, 1, count − 1, count and `INT_MAX`; cyclic graphs; n = 0; a 20-vertex antichain at limit 1,000; two disjoint chains (all C(2a, a) orders, and limit 1,000 at a = 10); a transitive tournament of `c` vertices with some doubled arcs beside `f` free vertices ((c + 1)⋯(c + f) orders: all of them at c = 8 (quick) or 30, f = 3, and limit 1,000 at c = 300, f = 8 outside quick) | The set of valid permutations; distinctness, validity, `min(limit, count)` outputs, first order equal to `topologicalSort` |
| Preconditions | Eight assertion probes: every sort, `topologicalGenerations` and `allTopologicalSorts` on undirected graphs, and `allTopologicalSorts` with limit −1 | Expected assertion failure |

The all-ready heap fixture uses 2,000 vertices, because checked libstdc++ validates the entire heap on each operation; the 200,000-vertex deep cases remain. Quick chains have 10,000 vertices to stay inside the 10 s quick bound. Each operation also runs on `CsrGraph`.

## Commands and results

2026-11-14 (P010 GR01; experiment: none), Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`. Each full run's pre-flight quick passed.

| Run | Result |
|---|---|
| Full, seed 20261114, g++ | PASS, all three builds, 6,965 cases; 9,018,414 checks (optimized, ASan-UBSan) and 9,018,416 (checked, whose debug-heap fixture adds two), 8 probes; optimized 4.1 s, checked 26.4 s, ASan-UBSan 39.4 s; peak 1,108 MB |
| Full, seed 20261114, `CXX=g++-14` | PASS, all three builds, same counts; optimized 3.9 s, checked 32.9 s, ASan-UBSan 45.4 s; peak 1,091 MB |
| Stress, seed 7 | PASS, all three builds, 89,130 cases and 34,937,011 checks per build; optimized 9.1 s, checked 74.4 s, ASan-UBSan 131.9 s; peak 1,484 MB |
| `01-run.py --mode quick --filter 04-Graphs --seed 42 --no-integration` | PASS, all 32 graph suites (before /finish; its rerun was stopped for low system memory, and the suites /finish changed passed quick as their full runs' pre-flight) |
| `05-extraction.py ops --filter 04-Graphs/03-toposort.hpp`; `testers --filter 03-toposort_tester` | PASS on g++ and g++-14; PASS |
| `02-integration.py` (190 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 3,077 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode stress --seed 7
python3 '96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter '04-Graphs/03-toposort.hpp'
python3 '96-Local Testing/05-extraction.py' testers --filter 03-toposort_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Mutation testing

Record: mewt 4.0.0 on blob `d2a41eee167f7803a49616f06f16b0dd3bef271e` (2026-11-14, seed 1, 26 lines changed since blob `bb1d0fc49b6c`): 116 mutants, 105 caught (0 by timeout), 11 survived, 0 untested.

/finish GR01 campaign on the Varol–Rotem rewrite and the added include. Stage seconds: quick 204, full on the optimized build 28, full on the checked and ASan-UBSan builds 303. Nine survivors are equivalent and two are runtime only:

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 266`, `limit <= 0` → `limit == 0` | 108 | `NDEBUG` only (the checked build asserts `limit >= 0` first): a negative limit then outputs one order and stops at line 121, O(n + m) (user decision 2026-11-12). |
| `COS 272`, `277`, `287` `<` → `!=` | 111, 113, 119 | Each counter steps by 1 from 0 to `n >= 0`. |
| `COS 282`, `p.back() != l` → `<` | 116 | `l` rises through the outer loop, so every stored label is at most `l`. |
| `COS 291`, `res.size() >= limit` → `==` | 121 | `limit >= 1` here (line 108), and `res.size()` grows by 1 from 1. |
| `COS 302`, `j > 0` → `j != 0` | 126 | `j` is a position, at least 0. |
| `COS 307`, `j < k` → `!=` | 129 | Every element above `k` is at home to its right, so `pos[k] <= k`. |
| `COS 313`, `k < 0` → `k <= 0` | 131 | The loop breaks only after a move, and element 0 never moves: when the countdown reaches it, everything above is at home, so it sits at position 0. |
| `IT 125` (condition forced true), `COS 283` (`!=` → `<=`) | 116 | Runtime only: parallel arcs then stay in the predecessor lists, which remain sorted, so every binary search answers the same; a search costs O(log(in-degree with multiplicity)) instead of O(log(n)). Measured (2026-11-14, `-O2 -DNDEBUG`, a 300-vertex transitive tournament with every arc 64 times beside 8 free vertices, 5 × 20,000 orders, 3 runs each): median 0.300 s original, 0.409 s mutant, ratio 1.36, identical checksums. |

Earlier whole-file campaign (GR01): mewt 4.0.0 on blob `433ab60d0357e399f3f539e61886966bb162d92a` (2026-11-14, seed 1, whole file): 295 mutants, 268 caught (0 by timeout), 27 survived, 0 untested. The survivors in code that the rewrite kept are equivalent (line numbers of that blob, one less than now); those in the replaced `allTopologicalSorts` body are gone:

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `BL 151`, `acyclic = true` → `false` | 7 | Every `TopologicalResult` is built by aggregate initialisation naming `acyclic`; the default is never read. |
| `COS 184`, `order.size() != n` → `<` | 24 | Kahn and DFS orders hold each vertex at most once, so the size never exceeds `n`. |
| `COS 189`, `204`, `214`, `219`, `229` `<` → `!=` | 26, 36, 52, 53, 98 | Each counter steps by 1 from 0 to a bound that is at least 0 (`order.size()` grows only ahead of `i`). |
| `COS 195`, `pos[to] == pos[from] + 1` → `<=` | 28 | The order is a complete topological order, so `pos[to] > pos[from]` for every arc and `<=` holds only at equality. |
| `COS 210`, `225` `--degree[v] == 0` → `<=` | 43, 56 | Each arc decrements its head once from its in-degree, so the count never goes below 0. |
| `MR 290`–`295`, `std::move(x)` → `x` | 30, 57, 77, 84, 86, 92 | Copy instead of move; the moved-from local is not read again. |
| `AS 149`, `150`, swapped `max` arguments | 95, 96 | `max` of two `int`s is symmetric. |

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied. The 2026-11-14 additions follow the pages listed in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr01) (definitions only, no code copied).

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 拓扑排序](https://oi-wiki.org/graph/topo/) (updated 2026-04-23) | Kahn, DFS and heap-based lexicographic ordering. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions). Legacy: the old Team Notebook `old_topo.cpp` was inspected; its recursive reverse postorder is replaced by the iterative implementation.

## Limits and handoffs

None.

## Aliases

Search aliases (P263, grab --find): 拓扑排序, 有向无环图, 拓扑序, 字典序最小拓扑序, 有向图判环, 字典序最大拓扑序, 反图最大堆, 拓扑分层, 所有拓扑序; lexicographically largest topological order, labeling balls (smallest inverse permutation), topological generations, all topological sorts.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, added `TopologicalResult::unique`; 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune, the cyclic result takes its witness from `dfsForest(g).cycle` (`findCycle` removed); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: P010 GR01, flag ruling (user decision 2026-11-14): `topologicalSort(g, true)` became `topologicalSortSmallest`; added `topologicalSortLargest`, `topologicalSortEarliest`, `topologicalGenerations` and `allTopologicalSorts`; full suite passed on g++ and g++-14, stress passed; independent review: 8 findings (allTopologicalSorts unbounded under NDEBUG for a negative limit, comment cap, missing complexity symbols and output terms, addCycle on an empty list under NDEBUG, duplicated in-degree and layer loops, evidence wording, an unqualified std name) fixed; mutation campaign (the enumeration then stopped at `res.size() >= limit`; survivors equivalent).
- 2026-11-14: /finish GR01, independent audit: 12 findings (1 medium: two bfs/dfs range-check mutants wrongly recorded equivalent; 11 low) fixed; `allTopologicalSorts` became Varol–Rotem enumeration with binary-searched predecessor lists (user decision 2026-11-14), O(n log n) per order instead of Θ(n + m); the direct `01-graph.hpp` include; an exact recursive oracle for `topologicalSortDfs` and dense tournament enumerations; full suite passed on g++ and g++-14, stress passed; mutation campaign on the changed lines (9 survivors equivalent, 2 runtime only). Review of the fix diff: 8 low findings (evidence wording, probe text match, stderr restore, a notes line for the bounded BFS) fixed.
