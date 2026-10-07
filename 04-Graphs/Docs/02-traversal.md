# 02-traversal.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` and `03-toposort.hpp` through `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract). First verified on 2026-09-27 and re-audited on 2026-10-07.

## Contracts

### CycleWitness and TraversalResult

A present cycle satisfies `vertices.size() == arcs.size() + 1`, `vertices.front() == vertices.back()`, and repeats no other vertex or logical edge; an empty arc list means no cycle. A loop is a one-arc cycle; two parallel undirected edges form a two-arc cycle. `TraversalResult(n)` asserts `n >= 0`. Unreached `parent`, `parent_arc`, `depth` and `root` entries are `-1`; a root has depth 0, no parent and its own vertex as root. `order` is discovery order. DFS also fills `postorder` and its first cycle; BFS leaves both empty. The internal `traversal_detail::treeCycle` closes the tree paths between the ends of a non-tree arc; it is covered through `dfs`, `findCycle` and `bipartiteCheck`.

### bfs

`bfs(g, sources)` seeds every source, in list order with duplicates ignored, before traversal, so `depth` is the unweighted shortest distance from the source set. Ties follow source order and then arc order: `order`, `parent`, `parent_arc` and `root` equal those of a plain FIFO queue. Every source must be in `[0, n)`; the single-source overload is `bfs(g, s)`. An empty source list reaches nothing.

### dfs, dfsForest and findCycle

`dfs(g, sources)` is an iterative DFS with exactly the recursive discovery order, finishing order and parents. Sources are considered in turn, skipping already reached ones, so `depth` is tree depth. Directed cycles are detected by gray ancestors; undirected DFS skips only the exact reverse of the parent arc, so loops and parallel edges are detected. `dfsForest(g)` seeds every vertex in order; `findCycle(g)` returns its first cycle, directed or undirected, or an empty witness.

Correctness: `bfs` seeds all sources before traversal, yielding unweighted shortest distances; `dfs` considers seeds sequentially and returns recursive-order discovery/finishing order without recursion. Duplicate sources are ignored, and empty source lists reach nothing. DFS detects a gray ancestor and skips only the reverse parent arc in undirected graphs, so loops and parallel-edge cycles are handled correctly. The iterative DFS fetches the adjacency once per frame resume and continues the scan until a child is pushed; the skipped reverse parent arc is computed once per resume.

### ComponentsResult, connectedComponents, BipartiteResult and bipartiteCheck

Both require an undirected graph. Components are numbered by increasing minimum vertex; members follow traversal order. `bipartiteCheck` colors BFS layers; on success `color` is a complete 0/1 coloring, on failure it is partial (`-1` unvisited) and `cycle` is an odd cycle.

Correctness: the bipartite algorithm returns an odd cycle when an edge joins equal colors. Parent-tree paths meet at their common ancestor, so the resulting cycle is contiguous and simple.

## Feature-to-test map

`96-Local Testing/04-Graphs/02-traversal_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. The corpus is 4,160 exhaustive/random graph cases; quick uses smaller exhaustive domains, 100 random cases and 20,000-vertex chains; full uses the stated corpus, and stress extends random counts to 12,000 and chains to 500,000. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `bfs` (single/multi-source, FIFO tie order), `TraversalResult` | Duplicate and repeated source lists, multiedges/loops, 200,000-vertex chains | Floyd reachability/hop distances; a plain FIFO `std::queue` reference comparing `order`, `parent`, `parent_arc` and `root` exactly |
| `dfs` (single/multi-source, iterative), `dfsForest` | Discovery and finishing order, parents, 200,000-vertex chains | Recursive traversal order |
| `findCycle`, `CycleWitness` | Directed and undirected arc witnesses, loops and parallel edges | Independently checked simple-cycle witnesses |
| `ComponentsResult`, `connectedComponents` | Numbering and membership | Floyd reachability |
| `BipartiteResult`, `bipartiteCheck` | Colorings and odd-cycle witnesses | Enumerated colorings; independently checked odd-cycle witnesses |
| Preconditions | Eight assertion probes | Expected assertion failure |

Mutation checks (planted in a temporary copy, full mode) killed reversed BFS adjacency and duplicate BFS sources (keep-duplicate-sources). The independent reviewer's surviving mutant here (`treeCycle` `>=` vs `>`) is equivalent or changes only runtime.

## Commands and results

### 2026-09-27 (original P010)

Fresh runs used GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. Full mode with seed 20260927 passed all three builds. The sanitizer run encountered the environment's ptrace/LeakSanitizer restriction and passed after an approved execution outside the sandbox with leak detection enabled; this is a completed retry, not a skipped configuration. The shared quick runner passed all six P010 entries with seed 42 from `/tmp`. Integration passed 79 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage/execution, and LOCAL/non-LOCAL workspace compilation. Repository consistency passed with no errors. All 61 checked precondition probes of the package passed. Extended stress modes were implemented but not run in that pass. Independent review found no remaining correctness, domain, complexity or meaningful coverage gap.

### 2026-10-07 re-audit

Before any edit, the unchanged suite passed full mode with seed 20260927 in all three builds. After the changes every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20260927, before any edit | PASS, all three builds |
| Stress, seed 7 | PASS, all three builds, 79,896 cases and 56,673,337 checks per build |
| Full, seed 20261007, after the review fixes, GCC 16.2.1 | PASS, all three builds, 6,541,575 checks per build, 8 probes |
| Full, seed 20261007, GCC 14.4.1 20260915 (`CXX=g++-14`) | PASS, all three builds, same counts |

The current feature map's full result with seed 20260927 is 6,549,903 checks per build and eight assertion probes. The shared quick runner passed all eleven graph entries from `/tmp`. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage, the workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors, and `--braces` reports nothing for the package. All runs used CPython 3.14 on Linux x86-64. GCC 14.2 itself was not run; 14.4.1 is the closest available. No online submission was made.

```bash
python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode full --seed 20260927        # before any edit
python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode full --seed 20261007        # after the review fixes
CXX=g++-14 python3 '96-Local Testing/04-Graphs/02-traversal_tester.py' --mode full --seed 20261007
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

Before any edit the row listed `TraversalResult: treeCycle`, an internal helper. The findings are recorded in `00-Guidelines/23-Reaudit Findings/p010.md`.

| # | Finding | Disposition |
|---|---|---|
| 1 | Row lists `TraversalResult: treeCycle`, an internal helper | Removed from the row; documented under CycleWitness and TraversalResult as covered through `dfs`, `findCycle` and `bipartiteCheck`. |
| 5 | BFS tie order had no oracle; reversed-adjacency mutant passed | A plain FIFO `std::queue` reference compares `order`, `parent`, `parent_arc` and `root` exactly; the reversed-adjacency and keep-duplicate-sources mutants now fail. |
| 7, 12, 14 | Closing-brace rule (`}} }`, `; }}}`, `; }`) | Fixed in all six P010 headers, testers and both benchmarks. |
| 8 | `treeCycle` without a complexity line; T/M lines not directly above | Every declaration now has its complexity line directly above it; contract prose moved to Contracts. |
| 9 | Iterative DFS called the asserting `g[u]` twice per arc | The adjacency is fetched once per frame resume and the scan continues until a child is pushed; the skipped reverse parent arc is computed once per resume. This change is constant-factor only and was not benchmarked. |

Independent review (`@reviewer`, 2026-10-07) found no correctness defect.

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [cp-algorithms, Breadth-first search](https://cp-algorithms.com/graph/breadth-first-search.html) | Queue layers, parent recovery and unweighted shortest distances. |
| [cp-algorithms, Bipartite check](https://cp-algorithms.com/graph/bipartite-check.html) (updated 2026-04-02) | BFS coloring and parity criterion. |
| [cp-algorithms, Finding a cycle](https://cp-algorithms.com/graph/finding-cycle.html) | Gray-state cycle recovery. Its simple-graph limitation is extended here using reverse-arc identities. |

Legacy: the old Team Notebook `old_cycle-find-dfs.cpp` was inspected; its recursive gray-state recovery is replaced by the iterative implementation. The broken exploratory destructive cycle-deletion snippet remains archival; it is not an active API or a claimed reusable cycle-enumeration engine. General cycle enumeration has its separate inventory owner. The legacy bipartite snippet remains in `97-Legacy`; its coloring feature is covered by the new result API with a witness.

## Limits and handoffs

No owned implementation or verification gap remains. SCCs belong to `08-scc.hpp`; cycle enumeration and odd-cycle extensions belong to `68-cycle_enumeration.hpp` (see [00-notes.md](00-notes.md#p010-re-audit-omissions)).
