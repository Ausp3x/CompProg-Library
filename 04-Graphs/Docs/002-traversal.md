# 002-traversal.hpp — Evidence

Owned by package P010 / GR01 (graph foundations, with `001-graph.hpp` and `003-toposort.hpp` through `006-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Scope

The row covers unweighted traversal, components, bipartiteness and arc classification. SCCs belong to `008-scc.hpp`; cycle enumeration and odd-cycle extensions belong to `068-cycle_enumeration.hpp` ([00-notes.md](00-notes.md#p010-re-audit-omissions)).

### CycleWitness and TraversalResult

A present cycle satisfies `vertices.size() == arcs.size() + 1` and `vertices.front() == vertices.back()`, and repeats no other vertex or logical edge; an empty arc list means no cycle. A loop is a one-arc cycle; two parallel undirected edges form a two-arc cycle. `TraversalResult(n)` asserts `n >= 0`. Unreached `parent`, `parent_arc`, `depth` and `root` entries are `-1`; a root has depth 0, no parent and itself as root. `order` is discovery order. DFS also fills `postorder` and its first cycle; BFS leaves both empty. The internal `traversal_detail::treeCycle` closes the tree paths between the ends of a non-tree arc; it is tested through `dfs`, `dfsForest` and `bipartiteCheck`.

### bfs

`bfs(g, sources)` seeds every source, in list order with duplicates ignored, before traversal, so `depth` is the unweighted shortest distance from the source set. Ties follow source order and then arc order: `order`, `parent`, `parent_arc` and `root` equal those of a plain FIFO queue. Every source must be in `[0, n)`; one source is the list `{s}`. An empty source list reaches nothing.

### dfs and dfsForest

`dfs(g, sources)` is an iterative DFS with exactly the recursive discovery order, finishing order and parents. Sources are taken in turn, skipping reached ones, so `depth` is tree depth. Duplicate sources are ignored and an empty list reaches nothing. Directed cycles are detected by gray ancestors; undirected DFS skips only the exact reverse of the parent arc, so loops and parallel edges are detected. `dfsForest(g)` seeds every vertex in order; its `cycle` is the first cycle found anywhere in `g`, or an empty witness.

Correctness: the iterative DFS fetches the adjacency once per frame resume and continues the scan until a child is pushed; the skipped reverse parent arc is computed once per resume.

### ComponentsResult, connectedComponents, BipartiteResult and bipartiteCheck

Both require an undirected graph. Components are numbered by increasing minimum vertex; members follow traversal order. `bipartiteCheck` colors BFS layers; on success `color` is a complete 0/1 coloring, on failure it is partial (`-1` unvisited) and `cycle` is an odd cycle.

Correctness: an edge joining equal colors yields the odd cycle. The two parent-tree paths meet at their common ancestor, so the cycle is contiguous and simple.

### ArcKind and classifyArcs

`classifyArcs(g, t)` takes `t` from `dfs(g, sources)` or `dfsForest(g)` of the same graph (asserted by sizes; a BFS result has no postorder) and labels every arc. `None` marks an arc whose tail the DFS did not reach. Otherwise the arc is `Tree` when it is the parent arc of its head or, undirected, the reverse of its tail's parent arc (both arcs of a tree edge); `Back` when its head is an ancestor of its tail or the tail itself, so every loop is `Back`; `Forward` when its head is a proper descendant; `Cross` otherwise. An undirected non-tree edge has one `Back` and one `Forward` arc, and no `Cross` arc appears there; a parallel copy of a tree edge is such a non-tree edge.

Correctness: `order` and `postorder` give entry and exit ranks, and in a DFS forest `u` is an ancestor of `v` exactly when `u`'s interval contains `v`'s. Every arc out of a reached vertex is scanned before that vertex finishes, so its head is reached too. By the DFS edge-classification theorem (Cormen et al., *Introduction to Algorithms*, §20.3) the head is then gray (ancestor: back), or black and discovered later (descendant: forward) or earlier (neither: cross), which the interval test reproduces afterwards. O(n + m).

### BfsArcKind and classifyBfsArcs

`classifyBfsArcs(g, t)` takes `t` from `bfs(g, sources)` of the same graph. It asserts that `depth` has `n` entries and `postorder` is empty, so a `dfs` result with a source is rejected; `dfs(g, {})` passes and, like `bfs(g, {})`, labels every arc `None`. Labels: `None` for an unreached tail, `Tree` as in `classifyArcs`, then `Level` when both ends have the same depth (every loop), `Forward` when the head is deeper and `Reverse` when it is shallower (NetworkX `bfs_labeled_edges`). A non-tree head is never more than one level deeper, since BFS depth is the hop distance from the source set. O(m).

### egoGraph

`egoGraph(g, center, radius)` with `center` in range and `radius >= 0` (asserted) is `inducedSubgraph(g, vs)` for the vertices within `radius` hops of `center` along out-arcs, in BFS order with `center` first (ties as in `bfs`). `vertex` of the returned `Subgraph` lists them, and the edges follow `inducedSubgraph`'s order. Weights are ignored; a weighted radius is a `005-shortest_path.hpp` composition. The BFS expands no vertex at depth `radius`, so `vs` is a prefix of `bfs(g, {center}).order` and the cost is O(n + d) with `d` the degree sum of the returned vertices (the O(n) term is the depth and position arrays).

### Legacy Mapping

- `findCycle(g)` → `dfsForest(g).cycle`.
- `bfs(g, s)`, `dfs(g, s)` → `bfs(g, {s})`, `dfs(g, {s})`.
- `isBipartite` (`OLD/algorithms.cpp:4768-4795`) → `bipartiteCheck` (`color` replaces `side`; a failure also returns an odd cycle).

## Feature-to-Test Map

`96-Local Testing/04-Graphs/002-traversal_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Full runs 4,160 exhaustive and random graph cases (7,767,589 checks per build). Quick uses smaller exhaustive domains, 100 random cases and 20,000-vertex chains (707 cases, 847,393 checks); stress raises random counts to 12,000 and chains to 500,000.

| Operation | Test | Oracle |
|---|---|---|
| `bfs` (multi-source, FIFO tie order), `TraversalResult` | Duplicate and repeated source lists, a repeated source before a new one (`{0, 0, 2, 1}`, also for `dfs`), multiedges and loops, 200,000-vertex chains | Floyd reachability and hop distances; a plain FIFO `std::queue` reference comparing `order`, `parent`, `parent_arc` and `root` exactly |
| `dfs` (multi-source, iterative), `dfsForest` | Discovery and finishing order, parents, 200,000-vertex chains | Recursive traversal order |
| `CycleWitness` (`dfs`, `dfsForest(g).cycle`) | Directed and undirected arc witnesses, loops and parallel edges, closed 200,000-vertex chains | Independently checked simple-cycle witnesses; existence against Floyd reachability or the forest edge count |
| `ComponentsResult`, `connectedComponents` | Numbering and membership | Floyd reachability |
| `BipartiteResult`, `bipartiteCheck` | Colorings and odd-cycle witnesses | Enumerated colorings; independently checked odd-cycle witnesses |
| `ArcKind`, `classifyArcs` | Every `dfs` result (sources none, all, `{0}`, `{n-1}`, repeated) and `dfsForest`, `Graph` and `CsrGraph`; chains (all Tree), the closed chain (Back, and Forward undirected); hand-worked parallel-edge and directed cases (Forward, Cross, Back loop, None) | Independent recursive white/gray/black DFS labelling each arc when examined |
| `BfsArcKind`, `classifyBfsArcs` | Every `bfs` result, chains, hand-worked cases | Tree arcs of a plain-queue BFS; Floyd hop distances from the source set |
| `egoGraph` | Every center at radius 0, 1, 2 and `INT_MAX`, `Graph` and `CsrGraph`; the chain prefix at radius n/2; a directed graph with a loop | Plain-queue BFS order filtered by Floyd distance; the exact edge sequence (source edges with both ends inside, grouped by the position of `u`, IDs increasing) with endpoints, orientation and weight |
| Preconditions | 21 assertion probes, including `bfs-past-end` and `dfs-past-end` (source n + 1), `classify-size`, `classify-small` (a result of a smaller graph), `classify-order-short`, `classify-bfs-result`, `classify-bfs-size`, `classify-bfs-small`, `classify-bfs-dfs-result`, `ego-negative-radius`, `ego-center-large`, `ego-center-past-end`, `ego-center-negative` | Expected assertion failure |

## Commands and Results

2026-11-14 (P010 GR01), Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`.

| Run | Result |
|---|---|
| Full, seed 20261114, g++ | PASS, all three builds, 4,160 cases and 7,767,589 checks per build, 21 probes; optimized 1.4 s, checked 3.8 s, ASan-UBSan 17.0 s; peak 1,057 MB |
| Full, seed 20261114, `CXX=g++-14` | PASS, all three builds, same counts; optimized 1.5 s, checked 3.6 s, ASan-UBSan 13.6 s; peak 1,028 MB |
| Stress, seed 7 | PASS, all three builds, 79,896 cases and 77,844,561 checks per build; optimized 6.6 s, checked 19.5 s, ASan-UBSan 125.1 s; peak 913 MB |
| `01-run.py --mode quick --filter 04-Graphs --seed 42 --no-integration` | PASS, all 32 graph suites, before /finish; its rerun was stopped for low system memory, and the suites /finish changed passed quick as the preflight of their full runs |
| `05-extraction.py ops --filter 04-Graphs/002-traversal.hpp`; `testers --filter 002-traversal_tester` | PASS on g++ and g++-14; PASS |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 3,077 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/002-traversal_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/04-Graphs/002-traversal_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/04-Graphs/002-traversal_tester.py' --mode stress --seed 7
python3 '96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter '04-Graphs/002-traversal.hpp'
python3 '96-Local Testing/05-extraction.py' testers --filter 002-traversal_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Mutation Testing

Record: mewt 4.0.0 on blob `d1a891aab48f9ac616c10f0850463c19765482bb` (2026-11-14, seed 1, 11 lines changed since blob `eaa6520ca59a`): 57 mutants, 53 caught (0 by timeout), 4 survived, 0 untested. Stage seconds: quick 107, full on the optimized build 17, full on the checked and ASan-UBSan builds 187.

The record covers the `classifyBfsArcs` assertion and the bounded `egoGraph` search that /finish GR01 changed. The other survivors come from the whole-file campaign of 2026-11-14 (seed 1; 445 mutants, 415 caught, 4 by timeout, 30 survived after the retest with the extended tester), on lines the newer record did not cover. All are equivalent; line numbers are the current header's.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 227`, `230`, `depth[u] >= depth[v]` → `!=`, `>` | 29 | Both callers pass an arc whose ends satisfy `depth[v] <= depth[u]` (DFS: `v` is an ancestor on the stack) or `depth[u] == depth[v]` (an odd BFS edge joins equal levels). Climbing either end first meets the same LCA and collects the same arcs on each side. |
| `MR 445`, `std::move(left)` → `left` | 35 | Copy instead of move of a local that is not read again. |
| `COS 247`, `277`, `297`, `307`, `317`, `327`, `352`, `367`, `397` `<` → `!=` | 49, 73, 104, 108, 127, 130, 149, 152, 168 | Each counter steps by 1 from 0 (or from a position at most its bound) to a bound that is at least it. |
| `COS 244`, `254`, `304`, `324`, `384`, `414` `!= -1` → `> -1` | 47, 53, 105, 128, 155, 171 | The value is `-1` or an index at least 0. |
| `COS 268`, `273`, `293`, `313`, `333`, `373`, `403` `== -1` → `<= -1` | 71, 73, 82, 111, 134, 154, 170 | Nothing is below `-1`. |
| `COS 288`, `state[v] == 1` → `<= 1` | 80 | The branch runs only when `state[v]` is nonzero, so the value is 1 or 2. |
| `COS 430`, `dv > du` → `>=` | 173 | Equal depths were already labelled `Level` on the line before. |
| `COS 455`, `i < vs.size()` → `!=` (newer record) | 183 | `i` steps by 1 from 0 while `vs` only grows. |
| `LC 189`, `continue` → `break` (newer record) | 185 | The queue is nondecreasing in depth and nothing deeper than `radius` is queued, so every vertex after the first at depth `radius` is also at `radius` and expands nothing. |
| `COS 463`, `depth[u] == radius` → `>=` (newer record) | 185 | Only vertices of depth below `radius` push, so no queued depth exceeds it. |
| `COS 466`, `depth[v] == -1` → `<= -1` (newer record) | 188 | Depths are `-1` or at least 0. |

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied. The 2026-11-14 additions follow the pages in [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr01) (definitions only, no code copied).

| Source | Inspected claims and use |
|---|---|
| [cp-algorithms, Breadth-first search](https://cp-algorithms.com/graph/breadth-first-search.html) | Queue layers, parent recovery and unweighted shortest distances. |
| [cp-algorithms, Bipartite check](https://cp-algorithms.com/graph/bipartite-check.html) (updated 2026-04-02) | BFS coloring and parity criterion. |
| [cp-algorithms, Finding a cycle](https://cp-algorithms.com/graph/finding-cycle.html) | Gray-state cycle recovery; its simple-graph limitation is lifted here with reverse-arc identities. |

Legacy: the old Team Notebook `old_cycle-find-dfs.cpp` was inspected; its recursive gray-state recovery is replaced by the iterative implementation. Its broken exploratory destructive cycle-deletion snippet stays archival and is not an API.

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): BFS (breadth-first search), DFS (depth-first search), bipartite graph check (OI Wiki: 二分图), cycle detection; Library Checker `cycle_detection`, `cycle_detection_undirected`; 深度优先搜索, 广度优先搜索, 深搜, 广搜, 连通分量, 连通块, 二分图判定, 二分图染色, 判环, 连通组件, 树边, 返祖边, 前向边, 横叉边, DFS 边分类, BFS 边分类; edge classification, tree/back/forward/cross arcs, ego graph.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 5 findings fixed; full suite passed on g++ and g++-14, stress passed; hand-planted mutants killed but one equivalent; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, removed `findCycle` and the single-source `bfs`/`dfs` overloads (cases moved to `dfsForest(g).cycle` and `{s}`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the search aliases; no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR01, added `classifyArcs`, `classifyBfsArcs` and `egoGraph`; full suite passed on g++ and g++-14, stress passed; independent review: 8 findings (allTopologicalSorts unbounded under NDEBUG for a negative limit, comment cap, missing complexity symbols and output terms, addCycle on an empty list under NDEBUG, duplicated in-degree and layer loops, evidence wording, an unqualified std name) fixed; whole-file mutation campaign (4 survivors closed with tests, 30 equivalent).
- 2026-11-14: /finish GR01, independent audit: 12 findings (1 medium: two bfs/dfs range-check mutants, `COS 237` and `COS 262`, wrongly recorded equivalent; 11 low) fixed; `egoGraph` stops its BFS at the radius (O(n + d)) and asserts its center, `classifyBfsArcs` rejects a `dfs` result; five new probes (closing COS 237 and COS 262) and an exact `egoGraph` edge-sequence oracle; full suite passed on g++ and g++-14, stress passed; mutation campaign on the changed lines (4 survivors equivalent). Review of the fix diff: 8 low findings (evidence wording, probe text match, stderr restore, a notes line for the bounded BFS) fixed.
- 2026-11-27: P010 limits recheck (experiment none): the ownership boundaries moved from Limits and Handoffs to Contracts (Scope), leaving no open limit; no code or test change.
