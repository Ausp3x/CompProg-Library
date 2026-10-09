# 21-matching_bipartite.hpp — evidence

Owned by package P043 / GR10 (with `23-assignment.hpp`); see [00-notes.md](00-notes.md#p043-package-record). Contest profile: iterative augmenting searches, word-parallel dense Kuhn using plain 64-bit words (no ISA code), and SCCs through the verified `tarjanScc` of `08-scc.hpp`. The planned row had no implementation; the legacy reference `OLD/Team Notebook/src/graph/old_kuhn.cpp` is defective and was used only to identify features.

## Contracts

### BipartiteMatching

`BipartiteMatching(nl, nr, edges = {})`: left vertices `[0, nl)`, right vertices `[0, nr)`, `nl, nr >= 0` (asserted before allocation). Each edge `(u, v)` gets ID equal to its insertion index; multiedges are allowed. Fields: `edges` (endpoints by ID, kept after erasure), `adj[u]` (pairs `(right vertex, edge ID)` of the alive edges at `u`, order unspecified after erasures), `left[u]` / `right[v]` (matched edge ID or −1), `size`. The constructor reserves each adjacency list once. The matching may be grown by any engine and kept across updates. Inputs are copied; results never alias caller storage.

- `addEdge(u, v)` appends an edge, returns its ID; O(1) amortized; does not change the matching. `eraseEdge(id)` asserts that `id` is alive (present in `adj[u]`), removes it in O(deg u) and unmatches it if matched. IDs are never reused.
- `hopcroftKarp()` and `kuhn()` grow the current matching to a maximum one, starting from whatever matching is present. Both run on a CSR snapshot of `adj`, with mates stored as adjacency slots, and write edge IDs back at the end (O(V + E) extra memory).
  - `hopcroftKarp()` alternates two steps. A full BFS layers left vertices from all free left vertices. An iterative layered DFS with current-arc pointers then accepts a free right vertex at any depth, and retires exhausted vertices and vertices on found paths.
  - The relaxed acceptance keeps the O(√V) phase bound. Every augmented arc goes up one layer, so residual distances never decrease and created arcs point down. A remaining augmenting path of the old shortest length would therefore lie wholly in the old layer graph. But its free root's DFS exhausted every vertex it could reach there, and path vertices from other roots are only entered through new downward arcs. So the shortest length strictly increases each phase, and after √V phases at most √V augmentations remain. Total O((V + E)·√V).
  - Development measurement (n = 100,000, m = 300,000 sparse random, construction included): truncated phases on edge-ID adjacency took about 380 ms over 45 phases. Relaxed phases cut that to about 140 ms over 12 augmenting phases plus a final empty one, and the CSR snapshot to about 70 ms.
  - `kuhn()` repeats passes of single-root augmenting DFS, with marks kept for the whole pass, until a pass finds nothing; O(V·(V + E)).
- `augment()` runs one BFS over alternating paths from all free left vertices and flips the first augmenting path found; returns whether `size` grew; O(V + E). Because one edge insertion or deletion changes the maximum by at most one, a single `augment()` after `addEdge` or after erasing a matched edge restores maximality.
- `match()` lists matched edge IDs in increasing left-vertex order; `mate(u)` is the right partner or −1.
- The queries below require a maximum matching. Each asserts that no free right vertex is reachable from a free left vertex by an alternating path, inside its own O(V + E) work. `grow(layered)` (the shared Hopcroft–Karp/Kuhn engine), `alternating(fromLeft)`, `reachLeft()` and `pairGraph()` are public helpers (the library has no `private`) and carry no further contract.
- `konigVertexCover()`: with Z = vertices alternating-reachable from free left vertices, returns `(L \ Z) ∪ (R ∩ Z)` as ascending `BipartiteVertices`. Its size equals the maximum matching (König), which certifies optimality. `maximumIndependentSet()` is the complement, of size V − |M|.
- `minimumEdgeCover(res)` returns false and clears `res` if some vertex has no alive edge; otherwise `res` is the matching plus one alive edge per exposed vertex, of size V − |M| (Gallai), order unspecified.
- `hallViolator()` returns `S = L ∩ Z` ascending. Then `N(S) = R ∩ Z` and `|N(S)| = |S| − (free left vertices) < |S|`. It is empty iff the matching saturates the left side; the empty set is never a violator, so it is a safe sentinel. For a right-side violator, swap the sides.
- `essentialEdges()` returns per edge ID: 0 = in no maximum matching (also every erased edge), 1 = in some but not all, 2 = in every maximum matching. Let Z_L be reachable from free left vertices and Z_R be reachable from free right vertices along reversed alternating paths. In the pair graph H on left vertices, unmatched edge `(x, v)` with `v` matched gives arc `x → mate(v)`. An unmatched edge `(u, v)` is allowed iff `u ∈ Z_L`, or `v ∈ Z_R`, or `u` and `mate(v)` share an SCC of H (an even alternating path from a free vertex, or an even alternating cycle). A matched edge is avoidable iff `u ∈ Z_L`, `v ∈ Z_R`, or `u` lies on a cycle of H (an SCC of size > 1, or a self-loop from a parallel unmatched edge).
- `dulmageMendelsohn()` returns `k ≥ 2` blocks and a block per vertex:
  - Block 0 is the right-surplus part Z_R, holding the free right vertices and their alternating closure.
  - Block k − 1 is the left-surplus part Z_L.
  - Blocks 1..k−2 are the SCCs of H on the perfectly matched middle part, in source-to-sink order, and are nonempty with equal sides.

  Blocks 0 and k − 1 may be empty. Every edge `(u, v)` satisfies `left[u] ≤ right[v]`, and matched pairs share a block. The middle blocks are exactly the connected components of allowed edges among middle vertices, i.e. the fine (elementary) decomposition. Neither Z_L nor Z_R can enter the middle, because both are closed under the alternating moves.

### bipartiteMatchingDense

Same input; returns a `BipartiteMatching` whose matching is maximum, so every query works on it. Each left vertex's neighbourhood is a row of `⌈nr/64⌉` words, and one pass of single-root Kuhn runs over left vertices. The unvisited-right mask persists across failed roots (a failed root's reachable set cannot lead to a free vertex until the matching changes) and resets only after an augmentation. Berge's lemma (no augmenting path from u stays true after other augmentations) makes one pass sufficient. Matched pairs are mapped back to edge IDs through one scan of the edge list. Time O(min(nl, nr) · nl · nr / 64 + E), memory nl·⌈nr/64⌉ words plus O(V + E).

### bMatching

`bMatching(nl, nr, edges, bl, br)` requires `bl.size() == nl`, `br.size() == nr` and nonnegative capacities. It returns, in ascending order, the IDs of a maximum-size edge set using each edge at most once, left vertex `u` at most `bl[u]` times and right vertex `v` at most `br[v]` times. It runs Dinic on `s → u (bl[u])`, `u → v (1)` per edge and `v → t (br[v])`. Every augmenting path carries one unit and saturates its unit middle arcs, so path walks with restart from `s` cost O(V + E) per phase. Residual flow decomposes into middle-arc-disjoint paths of length ≥ phase index, so there are O(√E) phases and O((V + E)·√E) time, O(V + E) memory. Per-edge capacities greater than one are expressed as parallel edges.

## Feature-to-test map

`96-Local Testing/04-Graphs/21-matching_bipartite_tester.py` (shared graph runner with `-Werror` warnings, optimized, checked with probes, ASan/UBSan in full/stress). The oracle enumerates every matching of the alive edges (recursion over left vertices) and keeps all maximum ones. Edge covers and b-matchings use edge-subset enumeration. The DM oracle is rebuilt from Gallai–Edmonds sets (vertices exposed by some maximum matching, and their neighbours) and DSU components of allowed edges.

| Operation | Test | Oracle |
|---|---|---|
| `hopcroftKarp`, `kuhn`, `augment` loop, `bipartiteMatchingDense`; `match`, `mate`, `left`, `right`, `size` | Every simple bipartite graph up to 3×3; 4,000 random multigraphs nl, nr ≤ 6 (full); 200,000-vertex reversed path; dense 1,500×1,500; sparse 200,000 | Maximum size by enumeration; consistency of both sides and of `mate`; no augmenting path afterwards; engines agree on large inputs |
| `addEdge`, `eraseEdge`, `augment`; warm-started `hopcroftKarp`, `kuhn` | 600 sequences of 20 random insertions/erasures; after each update, in rotation, one `augment`, a warm `hopcroftKarp` or a warm `kuhn` | Maximum size of the alive edge set by enumeration after every step; queries every fifth step |
| `konigVertexCover`, `maximumIndependentSet` | All exhaustive and random cases; large dense and sparse | Cover size equals maximum matching and covers every edge; independent set size V − ν and independent |
| `minimumEdgeCover` | Same corpus, isolated vertices included | Existence iff no isolated vertex; minimum size by subset enumeration (≤ 14 edges) |
| `hallViolator` | Same corpus | Empty iff left saturated; neighbourhood of S counted directly and smaller than S |
| `essentialEdges` | Same corpus; unique perfect matching on the 200,000 path | Allowed/forced status over the set of all maximum matchings |
| `dulmageMendelsohn`, `DulmageMendelsohn`, `BipartiteVertices` | Same corpus; path gives n singleton middle blocks | Surplus blocks equal Gallai–Edmonds sets; middle blocks equal allowed-edge components; order on every edge; block count |
| `bMatching` | Graphs with ≤ 12 edges, capacities 0..3; unit capacities; 200,000-vertex sparse | Maximum by subset enumeration; equals matching size for unit capacities; capacity checks |
| Preconditions | 7 probes: edge out of range, negative size, double erase, erase out of range, cover on a non-maximum matching, capacity vector size, negative capacity | Expected assertion failure |

Planted mutants (quick suite): wrong Z_R term in edge status, broken augment path walk, missing dense mask reset, swapped DM surplus blocks, b-matching capacity ignored, inverted cover, dropped pair-graph self-loop, Hopcroft–Karp path flip without the right mate, and skipped BFS layer — all 9 killed.

## Commands and results

2026-10-09, GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `21-matching_bipartite_tester.py --mode quick` | PASS 2 configurations, 1,677 cases, 240,261 checks, 7 probes |
| `21-matching_bipartite_tester.py --mode full --seed 1` | PASS 3 configurations, 16,689 cases, 2,865,188 checks, 7 probes; MEMORY peak 805 MB |
| `CXX=g++-14 21-matching_bipartite_tester.py --mode full --seed 2` | PASS 3 configurations, 16,689 cases, 2,860,073 checks, 7 probes; MEMORY peak 811 MB |
| `21-matching_bipartite_tester.py --mode stress --seed 3` | PASS 3 configurations, 110,689 cases, 8,597,503 checks, 7 probes; MEMORY peak 978 MB |
| `02-integration.py --sanitizers` | PASS 113 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 2,755 MB |
| `03-consistency.py` | No P043 errors; one error predates P043 at `HEAD` (`16-poly_tester.py` missing from `QUICK` in the committed `01-run.py`) |
| `21-matching_bipartite_benchmark.py` | PASS: all sizes agree; medians below |

```bash
python3 '96-Local Testing/04-Graphs/21-matching_bipartite_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/21-matching_bipartite_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/21-matching_bipartite_tester.py' --mode stress --seed 3
python3 '96-Local Testing/04-Graphs/21-matching_bipartite_benchmark.py'
```

## Benchmarks

[Benchmark driver](<../../96-Local Testing/04-Graphs/21-matching_bipartite_benchmark.py>); raw samples go to the git-ignored `21-matching_bipartite_benchmark.json`. Conditions: 2026-10-09, i9-11900H (shared machine), GCC 16.2.1 `-O2 -DNDEBUG`, seed 20260927, `nl = nr = n` with uniformly random endpoints (multiedges kept). Construction, allocation and matching are timed; edge generation is not. The first of six runs is discarded and the medians of the other five are shown. Matching sizes are compared outside the timed region. Memory is O(V + E) for both engines, plus `n·⌈n/64⌉` words for the dense variant.

| Workload | n | E | `hopcroftKarp` ms | `kuhn` ms | `bipartiteMatchingDense` ms |
|---|---|---|---|---|---|
| Sparse random, degree 3 | 100,000 | 300,000 | 62.2 | 901.1 | not run (bitset memory) |
| Sparse random, degree 3 | 5,000 | 15,000 | 1.6 | 3.0 | 24.9 |
| Dense random, 2,000,000 uniform draws (distinct-pair density about 0.39) | 2,000 | 2,000,000 | 22.9 | 25.9 | 51.7 |
| Dense random, 1,250,000 uniform draws (distinct-pair density about 0.049) | 5,000 | 1,250,000 | 12.0 | 9.5 | 477.2 |
| Reversed path (long augmenting chains in bit order) | 8,000 | 15,999 | 0.4 | 0.3 | 3,180.4 |

`hopcroftKarp` is the default. `kuhn` matches it on dense random graphs but is O(V·(V + E)) and 14× slower on large sparse ones. The dense variant lost on every measured workload. Its O(n³/64) bound beats O(E·√V) only on near-complete graphs with n below about 4,096, and the reversed path shows its worst case. It stays for catalog parity and for callers holding adjacency bitsets. No automatic dispatch.

## Sources

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Kuhn's algorithm](https://cp-algorithms.com/graph/kuhn_maximum_bipartite_matching.html) | Augmenting-path theorem and the one-pass argument (Berge) |
| [OI Wiki, bipartite maximum matching](https://oi-wiki.org/graph/graph-matching/bigraph-match/) | Hopcroft–Karp phase structure, incremental re-augmentation pattern |
| [hitonanode, bipartite_matching](https://hitonanode.github.io/cplib-cpp/graph/bipartite_matching.hpp) | Catalog comparison (incremental solve, bipartition helper) |
| [maspypy, bipartite_matching / bipartite_matching_dense](https://maspypy.github.io/library/flow/bipartite_matching_dense.hpp) | Dense bitset Kuhn and vertex cover; O(N1² N2 / w) bound |
| [ei1333, bipartite-flow](https://ei1333.github.io/library/graph/flow/bipartite-flow.hpp) | Edge erase with re-augmentation, residual-graph DM, lexicographic variants (omitted) |
| [suisen, bipartite_matching](https://suisen-cp.github.io/cp-library-cpp/library/graph/bipartite_matching.hpp) | Solve keeps earlier matches (incremental use) |
| [Nyaan, flow-on-bipartite-graph](https://nyaannyaan.github.io/library/flow/flow-on-bipartite-graph.hpp) | Capacity edges via flow (b-matching interface) |
| [Library Checker graph problems](https://github.com/yosupo06/library-checker-problems/tree/master/graph) | `bipartitematching` problem id |

Legacy: `OLD/Team Notebook/src/graph/old_kuhn.cpp` was inspected. It returns after the first neighbour and writes `mt[v] = v`, so it is not a valid matcher; its features (edge insertion, Kuhn DFS) are covered. The `Kuhn` structs at `OLD/Team Notebook/src/algsbetter.cpp:2201` and `test.cpp:831` (recursive DFS, greedy initial matching, `mt` right-to-left array, size result) are covered by `kuhn()`, `right[]` and `size`. The greedy initialisation is a heuristic, not an operation.

## Limits and handoffs

Weighted assignment is row 23; general matching is rows 22/64; maximum antichain, DAG path and chain covers are row 31; bipartite edge colouring is row 34. Lexicographically extremal matchings or covers, rank-maximal matching and automatic bipartition are omitted with reasons in [00-notes.md](00-notes.md#p043-omissions).
