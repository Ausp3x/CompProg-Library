# 03-toposort.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp`, `02-traversal.hpp` and `04-dsu.hpp` through `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract). First verified on 2026-09-27. The 2026-10-07 re-audit added `TopologicalResult::unique` from the completeness sweep.

## Contracts

### TopologicalResult, topologicalSort and topologicalSortDfs

All require a directed graph and ignore weights. `topologicalSort(g)` is Kahn's algorithm with a FIFO queue; `topologicalSort(g, true)` uses a min-heap and returns the lexicographically smallest order. Parallel arcs count separately in in-degrees. `topologicalSortDfs(g)` returns the reverse DFS finishing order. On a cycle, `acyclic` is false, `order` is empty and `cycle` is a directed witness. `unique` is true exactly when the order is the only topological order; an empty or one-vertex graph has a unique order, and a cyclic graph has `unique = false`.

Correctness: Kahn topological sorting counts every arc, including duplicates. A FIFO queue gives linear time; a minimum heap yields the lexicographically smallest valid order. DFS reverse postorder is a second implementation. Both require directed input and return an explicit acyclic flag; cyclic inputs return an empty order plus an oriented cycle. Empty directed graphs succeed with empty orders.

A topological order is unique exactly when every consecutive pair is joined by an arc: if some consecutive pair `x, y` has no arc `x -> y`, no path joins them either (any path from `x` to `y` would pass through vertices strictly between them in the order), so swapping them gives a second order; conversely, a Hamiltonian path fixes the order. `toposort_detail::ordered` checks this in O(n + m) for both Kahn and DFS orders.

## Feature-to-test map

`96-Local Testing/04-Graphs/03-toposort_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. The full corpus is 6,951 cases: exhaustive loopless graphs through four vertices and looped graphs through three, and 1,200 random graphs/parallel-edge variants. Quick uses smaller exhaustive domains, 100 random cases and 20,000-vertex chains; stress extends random counts to 10,000 and chains to 500,000. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `topologicalSort` (Kahn), `TopologicalResult: acyclic, order, cycle` | Exhaustive and random graphs, parallel arcs; 200,000-vertex chains and closed chains; oriented witnesses | Independent permutation enumeration through seven vertices gives existence |
| `topologicalSort` (lexicographic heap) | Same corpus; 2,000 simultaneously ready vertices | The first valid permutation in the enumeration |
| `topologicalSortDfs` | Same corpus | Permutation enumeration |
| `TopologicalResult: unique` | Same corpus; a chain with skip arcs with and without one missing link | Fewer than two valid permutations |
| Preconditions | Three assertion probes | Expected assertion failure |

The initial 200,000-vertex all-ready topological fixture exceeded the checked-build timeout because libstdc++ validates the entire heap on each operation; the final fixture uses 2,000 all-ready vertices while retaining the 200,000-vertex deep cases, and passed all three builds. A planted `unique` off-by-one mutation (temporary copy, full mode) was killed.

## Commands and results

### 2026-09-27 (original P010)

Fresh runs used GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. Full mode with seed 20260927 passed all three builds. The sanitizer run encountered the environment's ptrace/LeakSanitizer restriction and passed after an approved execution outside the sandbox with leak detection enabled; this is a completed retry, not a skipped configuration. The shared quick runner passed all six P010 entries with seed 42 from `/tmp`. Integration passed 79 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage/execution, and LOCAL/non-LOCAL workspace compilation. Repository consistency passed with no errors. All 61 checked precondition probes of the package passed. Extended stress modes were implemented but not run in that pass. Independent review found no remaining correctness, domain, complexity or meaningful coverage gap.

### 2026-10-07 re-audit

Before any edit, the unchanged suite passed full mode with seed 20260927 in all three builds. After the changes every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20260927, before any edit | PASS, all three builds |
| Stress, seed 7 | PASS, all three builds, 89,130 cases and 39,310,158 checks per build |
| Full, seed 20261007, after the review fixes, GCC 16.2.1 | PASS, all three builds, 13,478,426 checks per build, 3 probes |
| Full, seed 20261007, GCC 14.4.1 20260915 (`CXX=g++-14`) | PASS, all three builds, same counts |

The current feature map's full result with seed 20260927 is 13,478,134 checks per build and three assertion probes. The shared quick runner passed all eleven graph entries from `/tmp`. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage, the workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors, and `--braces` reports nothing for the package. All runs used CPython 3.14 on Linux x86-64. GCC 14.2 itself was not run; 14.4.1 is the closest available. No online submission was made.

```bash
python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode full --seed 20260927        # before any edit
python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode full --seed 20261007        # after the review fixes
CXX=g++-14 python3 '96-Local Testing/04-Graphs/03-toposort_tester.py' --mode full --seed 20261007
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

The findings are recorded in `00-Guidelines/23-Reaudit Findings/p010.md`.

| # | Finding | Disposition |
|---|---|---|
| 7, 12, 14 | Closing-brace rule (`}} }`, `; }}}`, `; }`) | Fixed in all six P010 headers, testers and both benchmarks. |
| 8 | T/M lines not directly above | Every declaration now has its complexity line directly above it; contract prose moved to Contracts. |

Change beyond the findings: the completeness sweep added `TopologicalResult::unique` (Hamiltonian-path test in `toposort_detail::ordered`). Independent review (`@reviewer`, 2026-10-07) found no correctness defect.

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 拓扑排序](https://oi-wiki.org/graph/topo/) (updated 2026-04-23) | Kahn, DFS and heap-based lexicographic ordering. |

The 2026-10-07 completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates (including the lexicographically largest order) are in [00-notes.md](00-notes.md#p010-re-audit-omissions). Legacy: the old Team Notebook `old_topo.cpp` was inspected; its recursive reverse postorder is replaced by the iterative implementation.

## Limits and handoffs

No owned implementation or verification gap remains.
