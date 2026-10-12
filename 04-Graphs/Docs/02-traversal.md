# 02-traversal.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` and `03-toposort.hpp` through `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### CycleWitness and TraversalResult

A present cycle satisfies `vertices.size() == arcs.size() + 1`, `vertices.front() == vertices.back()`, and repeats no other vertex or logical edge; an empty arc list means no cycle. A loop is a one-arc cycle; two parallel undirected edges form a two-arc cycle. `TraversalResult(n)` asserts `n >= 0`. Unreached `parent`, `parent_arc`, `depth` and `root` entries are `-1`; a root has depth 0, no parent and its own vertex as root. `order` is discovery order. DFS also fills `postorder` and its first cycle; BFS leaves both empty. The internal `traversal_detail::treeCycle` closes the tree paths between the ends of a non-tree arc; it is covered through `dfs`, `dfsForest` and `bipartiteCheck`.

### bfs

`bfs(g, sources)` seeds every source, in list order with duplicates ignored, before traversal, so `depth` is the unweighted shortest distance from the source set. Ties follow source order and then arc order: `order`, `parent`, `parent_arc` and `root` equal those of a plain FIFO queue. Every source must be in `[0, n)`; one source is the list `{s}`. An empty source list reaches nothing.

### dfs and dfsForest

`dfs(g, sources)` is an iterative DFS with exactly the recursive discovery order, finishing order and parents. Sources are considered in turn, skipping already reached ones, so `depth` is tree depth. Directed cycles are detected by gray ancestors; undirected DFS skips only the exact reverse of the parent arc, so loops and parallel edges are detected. `dfsForest(g)` seeds every vertex in order; its `cycle` is the first cycle found, directed or undirected, or an empty witness, so it finds a cycle anywhere in `g`.

Correctness: `bfs` seeds all sources before traversal, yielding unweighted shortest distances; `dfs` considers seeds sequentially and returns recursive-order discovery/finishing order without recursion. Duplicate sources are ignored, and empty source lists reach nothing. DFS detects a gray ancestor and skips only the reverse parent arc in undirected graphs, so loops and parallel-edge cycles are handled correctly. The iterative DFS fetches the adjacency once per frame resume and continues the scan until a child is pushed; the skipped reverse parent arc is computed once per resume.

### ComponentsResult, connectedComponents, BipartiteResult and bipartiteCheck

Both require an undirected graph. Components are numbered by increasing minimum vertex; members follow traversal order. `bipartiteCheck` colors BFS layers; on success `color` is a complete 0/1 coloring, on failure it is partial (`-1` unvisited) and `cycle` is an odd cycle.

Correctness: the bipartite algorithm returns an odd cycle when an edge joins equal colors. Parent-tree paths meet at their common ancestor, so the resulting cycle is contiguous and simple.

### Legacy mapping

- `findCycle(g)` → `dfsForest(g).cycle`.
- `bfs(g, s)`, `dfs(g, s)` → `bfs(g, {s})`, `dfs(g, {s})`.

## Feature-to-test map

`96-Local Testing/04-Graphs/02-traversal_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. The corpus is 4,160 exhaustive/random graph cases; quick uses smaller exhaustive domains, 100 random cases and 20,000-vertex chains; full uses the stated corpus, and stress extends random counts to 12,000 and chains to 500,000. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `bfs` (multi-source, FIFO tie order), `TraversalResult` | Duplicate and repeated source lists, multiedges/loops, 200,000-vertex chains | Floyd reachability/hop distances; a plain FIFO `std::queue` reference comparing `order`, `parent`, `parent_arc` and `root` exactly |
| `dfs` (multi-source, iterative), `dfsForest` | Discovery and finishing order, parents, 200,000-vertex chains | Recursive traversal order |
| `CycleWitness` (`dfs`, `dfsForest(g).cycle`) | Directed and undirected arc witnesses, loops and parallel edges, closed 200,000-vertex chains | Independently checked simple-cycle witnesses; existence against Floyd reachability or the forest edge count |
| `ComponentsResult`, `connectedComponents` | Numbering and membership | Floyd reachability |
| `BipartiteResult`, `bipartiteCheck` | Colorings and odd-cycle witnesses | Enumerated colorings; independently checked odd-cycle witnesses |
| Preconditions | Eight assertion probes | Expected assertion failure |

Mutation checks (planted in a temporary copy, full mode) killed reversed BFS adjacency and duplicate BFS sources (keep-duplicate-sources). A surviving mutant (`treeCycle` `>=` vs `>`) is equivalent or changes only runtime.

## Commands and results

2026-10-12, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261012, g++ | PASS, all three builds, 4,160 cases and 6,478,995 checks per build, 8 probes, peak 641 MB |
| Full, seed 20261012, `CXX=g++-14` | PASS, all three builds, same counts, peak 640 MB |
| Stress, seed 7 | PASS, all three builds, 79,896 cases and 54,979,351 checks per build, peak 943 MB |
| Quick runner, all sixteen graph entries, from `/tmp` | PASS |
| `02-integration.py` (136 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 647 MB |
| `03-consistency.py`, with `--braces` on the touched headers, testers and benchmark | No errors |

```bash
python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode stress --seed 7
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [cp-algorithms, Breadth-first search](https://cp-algorithms.com/graph/breadth-first-search.html) | Queue layers, parent recovery and unweighted shortest distances. |
| [cp-algorithms, Bipartite check](https://cp-algorithms.com/graph/bipartite-check.html) (updated 2026-04-02) | BFS coloring and parity criterion. |
| [cp-algorithms, Finding a cycle](https://cp-algorithms.com/graph/finding-cycle.html) | Gray-state cycle recovery. Its simple-graph limitation is extended here using reverse-arc identities. |

Legacy: the old Team Notebook `old_cycle-find-dfs.cpp` was inspected; its recursive gray-state recovery is replaced by the iterative implementation. The broken exploratory destructive cycle-deletion snippet remains archival; it is not an active API or a claimed reusable cycle-enumeration engine. General cycle enumeration has its separate inventory owner. The legacy bipartite snippet remains in `97-Legacy`; its coloring feature is covered by the new result API with a witness.

## Limits and handoffs

SCCs belong to `08-scc.hpp`; cycle enumeration and odd-cycle extensions belong to `68-cycle_enumeration.hpp` (see [00-notes.md](00-notes.md#p010-re-audit-omissions)). GCC 14.2 itself (the judge floor) was not run; 14.4.1 is the closest available.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 5 findings fixed (internal `treeCycle` removed from the row, BFS tie-order oracle, brace rule, complexity lines, one adjacency fetch per DFS resume); full suite passed on g++ and g++-14, stress passed.
- 2026-10-12: prune, removed `findCycle` and the single-source `bfs`/`dfs` overloads (cases moved to `dfsForest(g).cycle` and `{s}`); full suite passed on g++ and g++-14, stress passed.
