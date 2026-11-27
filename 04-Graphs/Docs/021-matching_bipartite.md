# 021-matching_bipartite.hpp — Evidence

Owned by package P043 / GR10 (with `023-assignment.hpp`); omissions are in [00-notes.md](00-notes.md#p043-omissions). Contest profile: iterative augmenting searches, a word-parallel dense Kuhn on plain 64-bit words (no ISA code), SCCs through the verified `tarjanScc` of `008-scc.hpp`, and b-matching flow through the verified `MaxFlow` of `019-max_flow.hpp`. The planned row had no implementation; the legacy reference `OLD/Team Notebook/src/graph/old_kuhn.cpp` is defective and served only to identify features.

## Contracts

### BipartiteMatching

`BipartiteMatching(nl, nr, edges = {})` has left vertices `[0, nl)` and right vertices `[0, nr)`, with `nl, nr >= 0` asserted before allocation. Each edge `(u, v)` gets its insertion index as ID; multiedges are allowed. Fields: `edges` (endpoints by ID, kept after erasure); `adj[u]` (pairs `(right vertex, edge ID)` of the alive edges at `u`, order unspecified after erasures); `left[u]` and `right[v]` (matched edge ID or −1); `size`. The constructor reserves each adjacency list once. Any engine may grow the matching, and it is kept across updates. Inputs are copied; results never alias caller storage.

- `addEdge(u, v)` appends an edge and returns its ID in O(1) amortized time without changing the matching. `eraseEdge(id)` asserts that `id` is alive (present in `adj[u]`), removes it in O(deg u) and unmatches it if matched. IDs are never reused. Under `NDEBUG` the structure stays in bounds, as `TwoSat` does: `addEdge` drops an out-of-range edge and returns −1, so later IDs no longer equal insertion indices, and `eraseEdge` ignores an out-of-range or already erased ID.
- `hopcroftKarp()` and `kuhn()` grow the current matching to a maximum one, starting from whatever matching is present. Both run on a CSR snapshot of `adj`, store mates as adjacency slots and write edge IDs back at the end (O(V + E) extra memory).
  - `hopcroftKarp()` alternates a full BFS, which layers left vertices from all free left vertices, with an iterative layered DFS with current-arc pointers, which accepts a free right vertex at any depth and retires exhausted vertices and vertices on found paths.
  - The relaxed acceptance keeps the O(√V) phase bound. Every augmented arc goes up one layer, so residual distances never decrease and created arcs point down. A remaining augmenting path of the old shortest length would lie wholly in the old layer graph; but its free root's DFS exhausted every vertex it could reach there, and path vertices from other roots are entered only through new downward arcs. So the shortest length strictly increases each phase, and after √V phases at most √V augmentations remain. Total O((V + E)·√V).
  - `kuhn()` repeats passes of single-root augmenting DFS, keeping marks for the whole pass, until a pass finds nothing; O(V·(V + E)).
- `augment()` runs one BFS over alternating paths from all free left vertices, flips the first augmenting path found and returns whether `size` grew; O(V + E). One edge insertion or deletion changes the maximum by at most one, so a single `augment()` after `addEdge`, or after erasing a matched edge, restores maximality.
- `match()` lists matched edge IDs in increasing left-vertex order; `mate(u)` is the right partner or −1.
- The queries below require a maximum matching. Each asserts, inside its own O(V + E) work, that no free right vertex is reachable from a free left vertex by an alternating path. `grow(layered)` (the shared Hopcroft–Karp and Kuhn engine), `alternating(fromLeft)`, `reachLeft()` and `pairGraph()` are public helpers (the library has no `private`) with no further contract.
- `konigVertexCover()`: with Z the vertices alternating-reachable from free left vertices, it returns `(L \ Z) ∪ (R ∩ Z)` as ascending `BipartiteVertices`. Its size equals the maximum matching (König), which certifies optimality. `maximumIndependentSet()` is the complement, of size V − |M|.
- `minimumEdgeCover(res)` returns false and clears `res` if some vertex has no alive edge; otherwise `res` is the matching plus one alive edge per exposed vertex, of size V − |M| (Gallai), in unspecified order.
- `hallViolator()` returns `S = L ∩ Z` ascending. Then `N(S) = R ∩ Z` and `|N(S)| = |S| − (free left vertices) < |S|`. It is empty iff the matching saturates the left side; the empty set is never a violator, so it is a safe sentinel. For a right-side violator, swap the sides.
- `essentialEdges()` returns per edge ID: 0 = in no maximum matching (also every erased edge), 1 = in some but not all, 2 = in every maximum matching. Let Z_L be the vertices reachable from free left vertices and Z_R those reachable from free right vertices along reversed alternating paths. In the pair graph H on left vertices, an unmatched edge `(x, v)` with `v` matched gives the arc `x → mate(v)`. An unmatched edge `(u, v)` is allowed iff `u ∈ Z_L`, or `v ∈ Z_R`, or `u` and `mate(v)` share an SCC of H (an even alternating path from a free vertex, or an even alternating cycle). A matched edge is avoidable iff `u ∈ Z_L`, `v ∈ Z_R`, or `u` lies on a cycle of H (an SCC of size > 1, or a self-loop from a parallel unmatched edge).
- `dulmageMendelsohn()` returns `k ≥ 2` blocks and a block per vertex. Block 0 is the right-surplus part Z_R, holding the free right vertices and their alternating closure; block k − 1 is the left-surplus part Z_L; blocks 1..k−2 are the SCCs of H on the perfectly matched middle part, in source-to-sink order, each nonempty with equal sides. Blocks 0 and k − 1 may be empty. Every edge `(u, v)` satisfies `left[u] ≤ right[v]`, and matched pairs share a block. The middle blocks are exactly the connected components of allowed edges among middle vertices, the fine (elementary) decomposition. Neither Z_L nor Z_R can enter the middle, because both are closed under the alternating moves.

### bipartiteMatchingDense

Same input; it returns a `BipartiteMatching` whose matching is maximum, so every query works on it. Each left vertex's neighbourhood is a row of `⌈nr/64⌉` words, and one pass of single-root Kuhn runs over the left vertices. The unvisited-right mask persists across failed roots, since a failed root's reachable set cannot lead to a free vertex until the matching changes, and resets only after an augmentation. Berge's lemma (no augmenting path from u stays true after other augmentations) makes one pass sufficient. Matched pairs map back to edge IDs through one scan of the edge list. Under `NDEBUG`, if the constructor dropped an invalid edge, the structure is returned unmatched instead of filling the bitset. Time O(min(nl, nr) · nl · nr / 64 + E); memory nl·⌈nr/64⌉ words plus O(V + E).

### bMatching

`bMatching(nl, nr, edges, bl, br)` requires `bl.size() == nl`, `br.size() == nr` and nonnegative capacities. It returns, ascending, the IDs of a maximum-size edge set that uses each edge at most once, left vertex `u` at most `bl[u]` times and right vertex `v` at most `br[v]` times. It builds a `MaxFlow<int>` network with the middle arcs `u → v (1)` first, so arc ID `e` is edge `e`, then `s → u (bl[u])` and `v → t (br[v])`, runs `flow(s, t)` (Dinic) and returns the edges with flow 1. Per-edge capacities above one are expressed as parallel edges. Under `NDEBUG`, mismatched capacity sizes or an out-of-range endpoint return an empty list, and a negative capacity acts as 0 (`MaxFlow` never uses an arc whose residual is below 1). The flow value is at most `edges.size()` and each residual pair sums to its `int` capacity, so nothing overflows.

Complexity is O((V + E)·√E) time and O(V + E) memory for the Dinic of `019-max_flow.hpp` on this network:

- Phase cost O(V + E). The blocking-flow search runs backwards from `t` with current-arc pointers. Only arcs `v → t` enter `t` and only unit middle arcs enter a right vertex, so every frame below a right vertex carries at most one unit. A unit that reaches `s` saturates every middle arc on its path, and a saturated arc cannot reopen within the phase, its reverse pointing against the levels. Successful walks therefore cost O(1) plus the arcs they saturate, and failed frames retire their vertex; with the BFS that is O(V + E).
- Each phase is a blocking flow. The BFS stops once `t` is labelled, by which time every vertex below `t`'s level is labelled. With `limit = MAX` the root frame at `t` ends only when its arcs are exhausted (the flow stays below `MAX`), so after the phase no path of the level graph remains and the residual distance to `t` grows.
- O(√E) phases. A simple residual `s`–`t` path has exactly two arcs at `s` or `t`; the rest are middle arcs, and each middle edge has total residual capacity 1 over its two directions (1 − f forward plus f backward), so it carries at most one unit of a residual flow. After `d` phases every augmenting path is longer than `d`, so the remaining flow is at most `E / (d − 1)`. After √E phases at most about √E units remain, and every later phase adds at least one.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/021-matching_bipartite_tester.py` runs on the shared contest runner with `-Werror` warnings: optimized, checked with probes, and ASan/UBSan in full and stress. The oracle enumerates every matching of the alive edges (recursion over left vertices) and keeps all maximum ones. Edge covers and small b-matchings use edge-subset enumeration; larger b-matchings are checked against the minimum cut, enumerated over left sets, and against `hopcroftKarp` on unit or doubled graphs. The DM oracle is rebuilt from Gallai–Edmonds sets (vertices exposed by some maximum matching, and their neighbours) and DSU components of allowed edges.

| Operation | Test | Oracle |
|---|---|---|
| `hopcroftKarp`, `kuhn`, `augment` loop, `bipartiteMatchingDense`; `match`, `mate`, `left`, `right`, `size` | Every simple bipartite graph up to 3×3; 4,000 random multigraphs nl, nr ≤ 6 (full); 200,000-vertex reversed path; dense 1,500×1,500; sparse 200,000 | Maximum size by enumeration; consistency of both sides and of `mate`; no augmenting path afterwards; engines agree on large inputs |
| `addEdge`, `eraseEdge`, `augment`; warm-started `hopcroftKarp`, `kuhn` | 600 sequences of 20 random insertions and erasures; after each update, in rotation, one `augment`, a warm `hopcroftKarp` or a warm `kuhn` | Maximum size of the alive edge set by enumeration after every step; queries every fifth step |
| `konigVertexCover`, `maximumIndependentSet` | All exhaustive and random cases; large dense and sparse | Cover size equals maximum matching and covers every edge; independent set of size V − ν and independent |
| `minimumEdgeCover` | Same corpus, isolated vertices included | Exists iff no isolated vertex; minimum size by subset enumeration (≤ 14 edges) |
| `hallViolator` | Same corpus | Empty iff left saturated; neighbourhood of S counted directly and smaller than S |
| `essentialEdges` | Same corpus; unique perfect matching on the 200,000 path | Allowed and forced status over the set of all maximum matchings |
| `dulmageMendelsohn`, `DulmageMendelsohn`, `BipartiteVertices` | Same corpus; the path gives n singleton middle blocks | Surplus blocks equal Gallai–Edmonds sets; middle blocks equal allowed-edge components; order on every edge; block count |
| `bMatching` | Graphs with ≤ 12 edges, capacities 0..3; unit capacities; 2,000 random multigraphs with nl ≤ 8, nr ≤ 11, up to 60 edges, capacities 0..5 (full); 200,000-vertex sparse with unit capacities and with left capacity 2 | Maximum by subset enumeration; size equals the minimum `s`–`t` cut enumerated over left sets (max-flow min-cut); equals matching size for unit capacities; on the large graph equals `hopcroftKarp`, and with left capacity 2 equals `hopcroftKarp` on the graph with every left vertex doubled (right capacity 1 forbids reusing an edge); ascending distinct IDs and capacities |
| Preconditions | 7 probes: edge out of range, negative size, double erase, erase out of range, cover on a non-maximum matching, capacity vector size, negative capacity | Expected assertion failure |

Planted mutants, quick suite, all killed: 8 on the matching code (wrong Z_R term in edge status, broken augment path walk, missing dense mask reset, swapped DM surplus blocks, inverted cover, dropped pair-graph self-loop, Hopcroft–Karp path flip without the right mate, skipped BFS layer), and 4 on the `MaxFlow` body of `bMatching` on 2026-10-13 (right capacity plus one, middle capacity 2, source arcs added before the middle arcs, a flow limit of `max(1, m / 2)`). The first, second and fourth of those four are also killed with only the min-cut and large sections enabled.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, Python 3.14.7, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). The 2026-11-10 library-wide runs used seed 42, beside the other suites, with the build cache emptied before each cold run. The 2026-10-13 prune runs used the optimized, checked (`-O0` with probes) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all`, full and stress) builds, one suite after another, so their times are indicative.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 874 MB; optimized 0.2 s compile + 6.3 s run, checked 20.4 + 27.6 s, ASan-UBSan 22.0 + 50.6 s; MEMORY peak 1073 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 918 MB; optimized 0.2 + 9.1 s, checked 26.7 + 29.9 s, ASan-UBSan 31.3 + 53.1 s; MEMORY peak 1086 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 32.4 + 1.2 s, checked 23.5 + 11.5 s; MEMORY peak 850 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 873 MB; optimized 0.1 + 5.8 s, checked 9.2 + 27.4 s, ASan-UBSan 10.6 + 51.5 s; MEMORY peak 1051 MB |
| 2026-11-10 | P276: tester full on g++ and g++-14, stress, `05-extraction.py`; `14-search_tester.py --mode quick` | PASS (counts not recorded) |
| 2026-10-13 | `021-matching_bipartite_tester.py --mode quick` | PASS 2 configurations, 1,877 cases, 342,589 checks, 7 probes |
| 2026-10-13 | `021-matching_bipartite_tester.py --mode full --seed 20261013` | PASS 3 configurations, 18,689 cases, 3,885,922 checks each, 7 probes; 73 s; MEMORY peak 818 MB |
| 2026-10-13 | `CXX=g++-14 021-matching_bipartite_tester.py --mode full --seed 20261013` | PASS 3 configurations, 18,689 cases, 3,885,922 checks each, 7 probes; 81 s; MEMORY peak 870 MB |
| 2026-10-13 | `021-matching_bipartite_tester.py --mode stress --seed 3` | PASS 3 configurations, 120,689 cases, 9,790,445 checks each, 7 probes; 128 s; MEMORY peak 1,016 MB |
| 2026-10-13 | `02-integration.py` | PASS 135 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace; MEMORY peak 649 MB |
| 2026-10-13 | `03-consistency.py` | No errors |
| 2026-10-13 | `03-consistency.py --braces` on the three P043 headers and testers | No closing-brace, comment-cap or free-function violations |
| 2026-10-13 | Ad-hoc `-DNDEBUG` probe, `-O1` with ASan, UBSan and `_GLIBCXX_ASSERTIONS` (not kept), five processes: invalid constructor edges followed by every engine and query; out-of-range `addEdge`; out-of-range and repeated `eraseEdge`; `bipartiteMatchingDense` with invalid edges; seven invalid `bMatching` calls | PASS, all in bounds and terminating |
| 2026-10-09 | `021-matching_bipartite_benchmark.py` (the engines it times are unchanged since) | Tables below |

```bash
python3 '96-Local Testing/04-Graphs/021-matching_bipartite_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/021-matching_bipartite_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/04-Graphs/021-matching_bipartite_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/04-Graphs/021-matching_bipartite_tester.py' --mode stress --seed 3
python3 '96-Local Testing/04-Graphs/021-matching_bipartite_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/021-matching_bipartite_benchmark.py>) writes raw samples to the git-ignored `021-matching_bipartite_benchmark.json`. Conditions: 2026-10-09, i9-11900H (shared machine), GCC 16.2.1 `-O2 -DNDEBUG`, seed 20260927, `nl = nr = n` with uniformly random endpoints (multiedges kept). Construction, allocation and matching are timed; edge generation is not. The first of six runs is discarded and the medians of the other five are shown; matching sizes are compared outside the timed region. Memory is O(V + E) for both engines, plus `n·⌈n/64⌉` words for the dense variant.

| Workload | n | E | `hopcroftKarp` ms | `kuhn` ms | `bipartiteMatchingDense` ms |
|---|---|---|---|---|---|
| Sparse random, degree 3 | 100,000 | 300,000 | 62.2 | 901.1 | not run (bitset memory) |
| Sparse random, degree 3 | 5,000 | 15,000 | 1.6 | 3.0 | 24.9 |
| Dense random, 2,000,000 uniform draws (distinct-pair density about 0.39) | 2,000 | 2,000,000 | 22.9 | 25.9 | 51.7 |
| Dense random, 1,250,000 uniform draws (distinct-pair density about 0.049) | 5,000 | 1,250,000 | 12.0 | 9.5 | 477.2 |
| Reversed path (long augmenting chains in bit order) | 8,000 | 15,999 | 0.4 | 0.3 | 3,180.4 |

`hopcroftKarp` is the default. `kuhn` matches it on dense random graphs but is O(V·(V + E)) and 14× slower on large sparse ones. The dense variant lost on every measured workload: its O(n³/64) bound beats O(E·√V) only on near-complete graphs with n below about 4,096, and the reversed path shows its worst case. It stays because it is a different algorithm (word-parallel Kuhn) beside Hopcroft–Karp, which the keep clause of One API per Operation (`03-cpp.md`) covers. There is no automatic dispatch.

A development measurement chose the engine layout (n = 100,000, m = 300,000 sparse random, construction included): truncated phases on edge-ID adjacency took about 380 ms over 45 phases, relaxed phases about 140 ms over 12 augmenting phases plus a final empty one, and the CSR snapshot about 70 ms; hence relaxed acceptance on a CSR snapshot.

`bMatching` on `MaxFlow<int>` against the removed hand-written Dinic: 2026-10-13, same machine and flags, seed 20261013, `nl = nr = n`, uniform random endpoints and capacities in the stated range. An ad-hoc probe, not kept, held the old body copied from `HEAD`. The first of six runs is discarded and the medians of the other five are shown; result sizes agreed on every workload. Memory is O(V + E) for both, with two arcs per network edge; the memory-capped probe peaked at 108 MB.

| Workload | n | E | old ms | `MaxFlow` ms |
|---|---|---|---|---|
| Sparse, `bl = 2`, `br = 1` | 200,000 | 600,000 | 522.4 | 297.4 |
| Sparse, capacities in [1, 5] | 100,000 | 1,000,000 | 597.0 | 365.5 |
| Dense, capacities in [1, 3] | 2,000 | 1,000,000 | 25.1 | 36.6 |
| Sparse, unit capacities | 200,000 | 600,000 | 5,717.5 | 1,176.5 |

The shared Dinic, which pushes many units per search from `t`, is faster on the sparse workloads and 11.5 ms slower on the dense one. For unit capacities `hopcroftKarp` remains the faster choice.

## Sources

Fetched 2026-10-09; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-09-p043).

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Kuhn's algorithm](https://cp-algorithms.com/graph/kuhn_maximum_bipartite_matching.html) | Augmenting-path theorem and the one-pass argument (Berge) |
| [OI Wiki, bipartite maximum matching](https://oi-wiki.org/graph/graph-matching/bigraph-match/) | Hopcroft–Karp phase structure, incremental re-augmentation pattern |
| [hitonanode, bipartite_matching](https://hitonanode.github.io/cplib-cpp/graph/bipartite_matching.hpp) | Catalog comparison (incremental solve, bipartition helper) |
| [maspypy, bipartite_matching / bipartite_matching_dense](https://maspypy.github.io/library/flow/bipartite_matching_dense.hpp) | Dense bitset Kuhn and vertex cover; O(N1² N2 / w) bound |
| [ei1333, bipartite-flow](https://ei1333.github.io/library/graph/flow/bipartite-flow.hpp) | Edge erase with re-augmentation, residual-graph DM, lexicographic variants (now row operations, missing) |
| [suisen, bipartite_matching](https://suisen-cp.github.io/cp-library-cpp/library/graph/bipartite_matching.hpp) | Solve keeps earlier matches (incremental use) |
| [Nyaan, flow-on-bipartite-graph](https://nyaannyaan.github.io/library/flow/flow-on-bipartite-graph.hpp) | Capacity edges via flow (b-matching interface) |
| [Library Checker graph problems](https://github.com/yosupo06/library-checker-problems/tree/master/graph) | `bipartitematching` problem id |

Legacy: `OLD/Team Notebook/src/graph/old_kuhn.cpp` was inspected. It returns after the first neighbour and writes `mt[v] = v`, so it is not a valid matcher; its features (edge insertion, Kuhn DFS) are covered. The `Kuhn` structs at `OLD/Team Notebook/src/algs.cpp:1852`, `algsbetter.cpp:2201` and `test.cpp:831` (recursive DFS, greedy initial matching, `mt` right-to-left array, size result) are covered by `kuhn()`, `right[]` and `size`; the greedy initialization is a heuristic, not an operation.

## Limits and Handoffs

- The row is `partial`: `graphPathGame`, `maximumMatchingLexicographic` and `konigVertexCoverLexicographic` are missing.
- Weighted assignment is row 23; general matching is rows 22 and 64; maximum antichain and DAG path and chain covers are row 31; bipartite edge colouring is row 34.
- Rank-maximal matching and automatic bipartition stay out, with reasons in [00-notes.md](00-notes.md#p043-omissions).
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): matching, bipartite matching, Kuhn, Hopcroft-Karp, MIS (maximum independent set) of a bipartite graph, Konig; 二分图最大匹配, 图匹配, 二分图匹配, 匈牙利算法, 最小点覆盖, 最大独立集, 柯尼希定理, 霍尔定理, 最小边覆盖, 二部图匹配, 二分图多重匹配.

## History

- 2026-10-09: verified (P043); quick, full and stress passed on g++, full on g++-14; engine benchmark recorded; independent review: no wrong answers (own remove/contract matching-number oracle, overflow probes at the stated domain bounds).
- 2026-10-13: prune: `bMatching` runs on `MaxFlow<int>` instead of its own Dinic (complexity argument and timing above); min-cut and doubled-graph Hopcroft–Karp oracles added. Same-day follow-up: `addEdge`, `eraseEdge`, `bipartiteMatchingDense` and `bMatching` stay in bounds under `NDEBUG`, checked by the ad-hoc probe. Full passed on g++ and g++-14, stress on g++; independent review: `bMatching` correct, including against the reviewer's own Ford–Fulkerson probe, and the complexity argument confirmed against `MaxFlow::dinic`.
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-10: P276 rename (call sites of `edge`); full on g++ and g++-14, stress and extraction passed.
- 2026-11-10: /finish P276 (user decision 2026-11-10): the search alias `matching` restores the `simple` corpus rank that rows added by P254 and P275 had taken; `14-search_tester.py --mode quick` passed.
