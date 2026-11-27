# 003-toposort.hpp — Evidence

Owned by package P010 / GR01 (graph foundations, with `001-graph.hpp`, `002-traversal.hpp` and `004-dsu.hpp` through `006-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### TopologicalResult, topologicalSort and topologicalSortDfs

Every operation requires a directed graph and ignores weights. `topologicalSort(g)` is Kahn's algorithm with a FIFO queue (O(n + m)). `topologicalSortSmallest(g)` and `topologicalSortLargest(g)` run Kahn with a min-heap and a max-heap (O(m + n log n)) and return the lexicographically smallest and largest orders. `topologicalSortEarliest(g)` returns the order whose position array is lexicographically smallest: vertex 0 as early as possible, then vertex 1, and so on (the "labeling" problem). It is not the smallest order in general: with arcs `2 -> 0` and vertex 1 free, the smallest order is `1 2 0` and the earliest `2 0 1`. Parallel arcs count separately in in-degrees. `topologicalSortDfs(g)` returns the reverse finishing order of `dfsForest(g)`: roots in increasing vertex order, out-arcs in adjacency (ID) order. On a cycle, `acyclic` is false, `order` is empty and `cycle` is a directed witness from `dfsForest`. `unique` is true exactly when the order is the only topological order; an empty or one-vertex graph has a unique order, and a cyclic graph has `unique = false`.

Correctness of the heap sorts: Kahn may emit any ready vertex next, and every prefix it builds extends to a complete order, so taking the smallest (largest) ready vertex at each step is lexicographically optimal at every position. `topologicalSortEarliest` runs the max-heap sort on `g.reverse()` and reverses the result. The reversed order fills positions from the back, and putting the largest available sink last minimizes the position of vertex 0, then that of vertex 1 among such orders, and so on (the classic exchange argument; the smallest-first greedy on `g` fails, as the example shows). The reverse graph costs O(n + m) memory.

A topological order is unique exactly when every consecutive pair is joined by an arc. If a consecutive pair `x, y` has no arc `x -> y`, no path joins them either (it would pass through vertices strictly between them), so swapping them gives a second order; conversely, a Hamiltonian path fixes the order. `toposort_detail::ordered` checks this in O(n + m) for Kahn and DFS orders.

### GenerationsResult and topologicalGenerations

`topologicalGenerations(g)` returns `layers`, where `layers[i]` holds, in increasing vertex order, the vertices whose longest incoming path has exactly `i` arcs (NetworkX `topological_generations`). Each layer is an antichain and the layer count equals the vertex count of a longest path, so the layers are a minimum antichain partition (Mirsky's theorem). On a cycle `acyclic` is false, `layers` is empty and `cycle` is the witness of `topologicalSort`. A longest-path DP along the FIFO order computes the levels and a bucket pass emits them sorted, O(n + m).

### allTopologicalSorts

`allTopologicalSorts(g, limit)` asserts `limit >= 0`; under `NDEBUG` a negative limit ends the call in O(n + m). It returns up to `limit` distinct topological orders, all of them when there are at most `limit`. The result is empty exactly when `limit == 0` or `g` has a cycle, which `topologicalSort` checks first, so a cyclic graph never starts an exponential search; an empty graph has one empty order. The first order is `topologicalSort(g).order`; the order of later outputs is unspecified.

Enumeration is Varol and Rotem's algorithm (Knuth, TAOCP 7.2.1.2, Algorithm V; user decision 2026-11-14, replacing Knuth–Szwarcfiter backtracking, which cost Θ(n + m) per order on a dense DAG). Vertices are relabelled by their position in the Kahn order, so the identity is a topological order. Element `k` (from `n - 1` down) moves one place left past its neighbour `l` unless `l -> k` is an arc; when it cannot move, it rotates back to its home position `k` and the next smaller element tries. Invariant: while `k` moves, every larger element is at home to its right, so its left neighbour `l` is smaller. The elements below `k` keep their relative order during its sweep, so the sweeps form a mixed-radix counter in which every linear extension appears exactly once. Two neighbours in a linear extension are comparable only through a direct arc, since a path between them would pass through a vertex placed between them, so one arc test per step suffices: a lookup of the label pair `l * n + k` in a `HashSet<lng>` (`02-Data Structures/061-hashmap.hpp`) of the relabelled arcs, parallel arcs stored once, O(1) expected. Each output costs O(n) for its copy, O(n) for the failed tests of the larger elements, and the rotations, which earlier moves pay for, so the bound is O(n + m + limit * n) expected time and O(n + m + limit * n) memory with the output. The hash set replaced binary search in sorted predecessor lists, O(log n) per test, on the 2026-11-27 counter A/B ([Benchmarks](#benchmarks)). The search is iterative.

### Legacy Mapping

- `topologicalSort(g, true)` → `topologicalSortSmallest(g)` (flags choosing between operations become named mirrors, user decision 2026-11-14); `topologicalSort(g, false)` → `topologicalSort(g)`.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/003-toposort_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Full runs 4,697 exhaustive graphs (loopless through four vertices, looped through three) and 1,200 random graphs, each with a parallel-edge variant when it has an edge. Quick uses smaller exhaustive domains, 100 random cases and 10,000-vertex chains, to stay inside the 10 s quick bound; stress raises random counts to 10,000 and chains to 500,000. Every operation also runs on `CsrGraph`. The all-ready heap fixture has 2,000 vertices because checked libstdc++ validates the whole heap on each operation; the 200,000-vertex deep cases remain.

| Operation | Test | Oracle |
|---|---|---|
| `topologicalSort` (Kahn FIFO), `TopologicalResult: acyclic, order, cycle` | Exhaustive and random graphs, parallel arcs; 200,000-vertex chains and closed chains; oriented witnesses | Independent permutation enumeration through seven vertices gives existence |
| `topologicalSortSmallest`, `topologicalSortLargest` | Same corpus; 2,000 simultaneously ready vertices; 200,000-vertex ascending and descending chains | The first and the last valid permutation |
| `topologicalSortEarliest` | Same corpus; the hand-solved `2 -> 0` case; a deep case (free odd vertices, descending even chain) where it differs from the smallest order | The valid permutation with the lexicographically smallest position array |
| `topologicalSortDfs` | Same corpus | Permutation enumeration for validity; through 2,000 vertices the exact reverse finishing order of an independent recursive DFS (roots increasing, arcs in adjacency order) |
| `TopologicalResult: unique` | Same corpus; a chain with skip arcs, with and without one missing link | Fewer than two valid permutations |
| `GenerationsResult`, `topologicalGenerations` | Same corpus, deep chains, cyclic cases with witnesses | Longest incoming path per vertex by n rounds of arc relaxation |
| `allTopologicalSorts` | Limits 0, 1, count − 1, count and `INT_MAX`; cyclic graphs; n = 0; a 20-vertex antichain at limit 1,000; two disjoint chains (all C(2a, a) orders, and limit 1,000 at a = 10); a transitive tournament of `c` vertices with some doubled arcs beside `f` free vertices ((c + 1)⋯(c + f) orders: all of them at c = 8 (quick) or 30, f = 3, and limit 1,000 at c = 300, f = 8 outside quick) | The set of valid permutations; distinctness, validity, `min(limit, count)` outputs, first order equal to `topologicalSort` |
| Preconditions | Eight assertion probes: every sort, `topologicalGenerations` and `allTopologicalSorts` on undirected graphs, and `allTopologicalSorts` with limit −1 | Expected assertion failure |

## Commands and Results

2026-11-27 (P010 limits recheck, hashed arc test, experiment none): Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`.

| Run | Result |
|---|---|
| Full, seed 20261127, g++ | PASS, all three builds, 6,959 cases; 8,938,410 checks (optimized, ASan-UBSan) and 8,938,412 (checked, whose debug-heap fixture adds two), 8 probes; optimized 5.2 s, checked 23.3 s, ASan-UBSan 36.8 s; pre-flight quick peak 855 MB; peak 1,224 MB |
| Full, seed 20261127, `CXX=g++-14` | PASS, same counts; optimized 3.5 s, checked 21.3 s, ASan-UBSan 29.6 s; peak 1,178 MB |
| Stress, seed 7 | PASS, all three builds, 89,130 cases and 34,937,011 checks per build; optimized 9.8 s, checked 67.0 s, ASan-UBSan 134.6 s; peak 1,616 MB |
| `05-extraction.py ops --filter 04-Graphs/003-toposort.hpp`; `testers --filter 003-toposort_tester` | PASS on g++ and g++-14 (12 operations in 9 extractions); PASS |
| `04-mutation.py run 04-Graphs/003-toposort.hpp` | See Mutation Testing |
| `02-integration.py` (192 standalone and aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 3,438 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/003-toposort_tester.py' --mode full --seed 20261127
CXX=g++-14 python3 '96-Local Testing/04-Graphs/003-toposort_tester.py' --mode full --seed 20261127
python3 '96-Local Testing/04-Graphs/003-toposort_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '04-Graphs/003-toposort.hpp'
python3 '96-Local Testing/05-extraction.py' testers --filter 003-toposort_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/003-toposort.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Mutation Testing

Record: mewt 4.0.0 on blob `ec5d4619b5f69e2608b6b4d03f5ae439720a6e19` (2026-11-27, seed 1, 5 lines changed since blob `d2a41eee167f`): 35 mutants, 34 caught (0 by timeout), 1 survived, 0 untested. Stage seconds: quick 54, full on the optimized build 7, full on the checked and ASan-UBSan builds 93.

The record covers the hashed arc test and its include: every mutant on those 5 lines, 35 of the 334 mewt generated for the file. The other survivors come from the 2026-11-14 campaigns: the Varol–Rotem rewrite (26 changed lines, 116 mutants, 11 survived) and, on lines 9–100, the whole-file campaign (295 mutants, 27 survived), on lines no newer record covered; those in the replaced predecessor lists are gone. Line numbers are the current header's.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `BL 151`, `acyclic = true` → `false` | 9 | Every `TopologicalResult` is built by aggregate initialisation naming `acyclic`; the default is never read. |
| `COS 184`, `order.size() != n` → `<` | 26 | Kahn and DFS orders hold each vertex at most once, so the size never exceeds `n`. |
| `COS 189`, `204`, `214`, `219`, `229` `<` → `!=` | 28, 38, 54, 55, 100 | Each counter steps by 1 from 0 to a bound that is at least 0 (`order.size()` grows only ahead of `i`). |
| `COS 195`, `pos[to] == pos[from] + 1` → `<=` | 30 | The order is a complete topological order, so `pos[to] > pos[from]` for every arc and `<=` holds only at equality. |
| `COS 210`, `225` `--degree[v] == 0` → `<=` | 45, 58 | Each arc decrements its head once from its in-degree, so the count never goes below 0. |
| `MR 290`–`295`, `std::move(x)` → `x` | 32, 59, 79, 86, 88, 94 | Copy instead of move; the moved-from local is not read again. |
| `AS 149`, `150`, swapped `max` arguments | 97, 98 | `max` of two `int`s is symmetric. |
| `COS 266`, `limit <= 0` → `limit == 0` | 109 | `NDEBUG` only (the checked build asserts `limit >= 0` first): a negative limit then outputs one order and stops at line 120, O(n + m) (user decision 2026-11-12). |
| `COS 272`, `277`, `287` `<` → `!=` | 112, 114, 118 | Each counter steps by 1 from 0 to `n >= 0`. |
| `COS 291`, `res.size() >= limit` → `==` | 120 | `limit >= 1` here (line 109), and `res.size()` grows by 1 from 1. |
| `COS 308` (newest record), `j > 0` → `j != 0` | 125 | `j` is a position, at least 0. |
| `COS 307`, `j < k` → `!=` | 128 | Every element above `k` is at home to its right, so `pos[k] <= k`. |
| `COS 313`, `k < 0` → `k <= 0` | 130 | The loop breaks only after a move, and element 0 never moves: when the countdown reaches it, everything above is at home, so it sits at position 0. |

## Benchmarks

`allTopologicalSorts` arc test, 2026-11-27 (experiment none): the hash set of the current header against the previous header's binary search in sorted predecessor lists, with the same enumeration. The scratch rig built each graph, ran one variant once and folded every order into a checksum; both forms gave identical checksums on every row. Counters: `perf stat -r 5 -e cycles:u,instructions:u`, minus a construction-only run of the same graph; GCC 15.3, `-std=gnu++20 -O2 -DNDEBUG -falign-functions=64 -falign-loops=32`, Intel Core i9-11900H, under the exclusive reservation of `_00_memory_cap.py --alone`.

| Workload (n, orders) | Cycles, binary search → hash (ratio) | Instructions (ratio) |
|---|---|---|
| Transitive tournament on 300 vertices, every arc 64 times, and 8 free vertices (308, 100,000) | 603M → 548M (0.91) | 1,602M → 1,213M (0.76) |
| The same tournament with single arcs (308, 100,000) | 510M → 423M (0.83) | 1,481M → 997M (0.67) |
| Two disjoint 10-vertex chains (20, all 184,756) | 51M → 49M (0.96) | 142M → 146M (1.03) |
| Antichain (20, 2,000,000) | 562M → 578M (1.03) | 1,445M → 1,431M (0.99) |
| Random DAG, each forward pair an arc with probability 1/8 (40, 1,000,000) | 521M → 558M (1.07) | 1,018M → 1,019M (1.00) |
| Random DAG, 4 forward arcs per vertex within 50 labels (2,000, 20,000) | 297M → 297M (1.00) | 493M → 492M (1.00) |
| Random DAG, probability 1/4 (200, 300,000) | 534M → 528M (0.99) | 926M → 910M (0.98) |

Three interleaved rounds of `perf stat -r 10` on the two dense tournaments and the two random DAGs of 40 and 200 vertices repeated the dense gains (cycle ratios 0.83 to 0.91 in every round). The 40-vertex row averaged 526M against 534M cycles (+1.5%) and the 200-vertex row 532M against 534M, both with equal instruction counts, so the 1.07 above is noise. No row is more than 3% slower. A hybrid that kept binary search for predecessor lists of at most 8, 16 or 32 labels matched the hash on every row and was not adopted, since it gains nothing for its extra code. The hash set stores each distinct arc once in slots of 16 bytes at a load of at most one half, which is more memory than the 4-byte predecessor labels it replaces, inside the same O(n + m) bound.

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied. The 2026-11-14 additions follow the pages in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr01) (definitions only, no code copied).

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 拓扑排序](https://oi-wiki.org/graph/topo/) (updated 2026-04-23) | Kahn, DFS and heap-based lexicographic ordering. |
| Varol and Rotem (1981) and Knuth, TAOCP 4A §7.2.1.2, Algorithm V, read on 2026-11-22 ([00-sources.md](00-sources.md#fetched-2026-11-22-source-check)) | `allTopologicalSorts` follows Algorithm V step by step. Varol and Rotem run their procedure in this direction in their Table 2 and allow at most N − 1 rotations between two outputs. The cost argument under Contracts is this package's own. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions). Legacy: the old Team Notebook `old_topo.cpp` was inspected; its recursive reverse postorder is replaced by the iterative implementation.

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): 拓扑排序, 有向无环图, 拓扑序, 字典序最小拓扑序, 有向图判环, 字典序最大拓扑序, 反图最大堆, 拓扑分层, 所有拓扑序; lexicographically largest topological order, labeling balls (smallest inverse permutation), topological generations, all topological sorts.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, added `TopologicalResult::unique`; 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune, the cyclic result takes its witness from `dfsForest(g).cycle` (`findCycle` removed); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR01, flag ruling (user decision 2026-11-14): `topologicalSort(g, true)` became `topologicalSortSmallest`; added `topologicalSortLargest`, `topologicalSortEarliest`, `topologicalGenerations` and `allTopologicalSorts`; full suite passed on g++ and g++-14, stress passed; independent review: 8 findings (allTopologicalSorts unbounded under NDEBUG for a negative limit, comment cap, missing complexity symbols and output terms, addCycle on an empty list under NDEBUG, duplicated in-degree and layer loops, evidence wording, an unqualified std name) fixed; whole-file mutation campaign (the enumeration then stopped at `res.size() >= limit`; survivors equivalent).
- 2026-11-14: /finish GR01, independent audit: 12 findings (1 medium: two bfs/dfs range-check mutants wrongly recorded equivalent; 11 low) fixed; `allTopologicalSorts` became Varol–Rotem enumeration with binary-searched predecessor lists (user decision 2026-11-14), O(n log n) per order instead of Θ(n + m); the direct `001-graph.hpp` include; an exact recursive oracle for `topologicalSortDfs` and dense tournament enumerations; full suite passed on g++ and g++-14, stress passed; mutation campaign on the changed lines (9 survivors equivalent, 2 runtime only). Review of the fix diff: 8 low findings (evidence wording, probe text match, stderr restore, a notes line for the bounded BFS) fixed.
- 2026-11-27: P010 limits recheck (experiment none): the review found the stale reason for keeping binary search in `allTopologicalSorts`; a counter A/B on GCC 15 adopted a `HashSet` arc test (dense DAGs 9–17% fewer cycles, no row over 3% slower), O(n + m + limit * n) expected; batch prerequisite DS49 added; full suite passed on g++ and g++-14, stress passed, extraction passed, changed-line mutation campaign (1 equivalent survivor), integration passed.
