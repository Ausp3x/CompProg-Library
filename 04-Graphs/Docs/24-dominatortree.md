# 24-dominatortree.hpp — evidence

Owned by package P043 / GR11; see [00-notes.md](00-notes.md#p043-package-record). Contest profile: iterative Lengauer–Tarjan with balanced linking over the verified `Graph`/`CsrGraph` representation, plus an independent vertex-deletion algorithm. The row was planned; there is no legacy code.

## Contracts

### DominatorTree

`DominatorTree(r, idom)` builds the query structure from an immediate-dominator array over `n` vertices. `idom[r]` must be −1 (asserted), `idom[v]` is −1 for vertices unreachable from `r`, and the non-−1 entries must form a tree rooted at `r`; the two algorithms below guarantee this. Fields:

- `root`;
- `idom`;
- `tin[v]`, `tout[v]`: a half-open Euler interval in the dominator tree, −1 for unreachable `v`. `tout[v] − tin[v]` is the number of vertices dominated by `v`, itself included.

Construction is iterative, O(n); it asserts the root and that every entry lies in `[-1, n)`, and under `NDEBUG` it ignores out-of-range entries and returns empty intervals for an out-of-range root.

- `immediateDominator(v)` returns `idom[v]`: −1 for the root and unreachable vertices. Library Checker `dominatortree` prints the root as its own dominator; map −1 at `root` accordingly.
- `dominates(u, v)` is reflexive dominance: `tin[u] ≤ tin[v] < tout[u]`. It is false when either vertex is unreachable. O(1).
- `dominanceFrontier(g)` requires the graph the tree was built from (directed, same `n`; asserted). It returns `DF(v) = { w : v dominates a reachable predecessor of w and v does not strictly dominate w }`, which includes `w = v` for a back arc or loop into `v`. Lists are duplicate-free and in no particular order; unreachable vertices get empty lists. Predecessors are grouped by target, and each walks up the dominator tree from the predecessor to `idom[w]`. `idom[w]` dominates every reachable predecessor of `w`, so each walk terminates there. Walks for the same `w` run consecutively, so meeting a vertex already stamped with `w` means the rest of its chain is recorded. That gives O(V + E + out) time and O(V + out) memory.

### dominatorTree

`dominatorTree(g, root)` takes a directed `Graph` or `CsrGraph` (asserted) and a root in range; loops and multiedges are allowed and weights ignored. It numbers vertices in iterative DFS preorder from 1, with number 0 as the forest sentinel (semi 0, size 0). It builds predecessor lists over reachable vertices, then for numbers N..2:

1. `semi[w] = min semi(eval(p))` over predecessors `p`;
2. put `w` in the bucket of `semi[w]`;
3. `link(parent, w)`;
4. for every `v` in the parent's bucket, set `dom[v] = eval(v)` if its semi is smaller, else the parent.

A final pass sets `dom[w] = dom[dom[w]]` where `dom[w] ≠ semi[w]`. `eval` and `link` are Tarjan's balanced versions with child chains and subtree sizes. `eval` returns `label[v]` even at a virtual-tree root, which is where the code differs from the simple compression-only variant, and path compression is iterative. Time O((V + E)·α(E, V)), memory O(V + E). The semidominator theorem and the idom corollary are those of Lengauer and Tarjan (1979).

### dominatorTreeSimple

`dominatorTreeSimple(g, root)` has the same input domain and returns the same tree. For each reachable `d ≠ root`, a BFS from the root that skips `d` marks the vertices that `d` strictly dominates: the reachable vertices left unreached. The strict dominators of `w` form a chain whose dominated sets strictly shrink, so `idom(w)` is the one dominating the fewest vertices; `w` defaults to the root when nothing else strictly dominates it. Time O(V·(V + E)), memory O(V + E). The row's former label "iterative O(VE)" is replaced by this provable vertex-deletion bound. Cooper–Harvey–Kennedy iteration is simpler to state but has no O(V·E) worst case.

## Feature-to-test map

`96-Local Testing/04-Graphs/24-dominatortree_tester.py` (shared graph runner with `-Werror` warnings, optimized, checked with probes, ASan/UBSan in full/stress). Oracle: the dataflow fixpoint `Dom(v) = {v} ∪ ⋂ Dom(p)` over reachable predecessors with bitmask sets, independent of both algorithms; `idom(v)` is the strict dominator whose set has one element fewer, and the frontier is evaluated from its definition.

| Operation | Test | Oracle |
|---|---|---|
| `dominatorTree` (Graph and CSR), `dominatorTreeSimple` | Every looped digraph on n ≤ 4 vertices with every root (full); 4,000 random multigraphs n ≤ 14 in three shapes; 200,000-vertex chain, skip ladder and star; 3,000-vertex random graph | Dataflow sets; the two algorithms agree on the large random graph |
| `DominatorTree`: `immediateDominator`, `dominates`, `tin`/`tout` | All small cases, all vertex pairs, unreachable vertices | Membership in `Dom(v)`; subtree size equals dominated count |
| `dominanceFrontier` | All small cases; back-arc frontier on the 200,000 chain | Definition evaluated over all predecessors; exact set and no duplicates |
| Preconditions | 7 probes: undirected graph, root out of range (both algorithms), frontier on a different graph, query out of range, constructor entry out of range, constructor root with a dominator | Expected assertion failure |

The suite caught two of my own defects during development: frontier stamps that were not grouped by target (duplicate entries), and `eval` returning `v` instead of `label[v]` at virtual roots (wrong idom on a 12-vertex graph). Both are fixed. Planted mutants (quick suite): the `eval` regression, a missing final idom pass, unstamped frontier walks, an inverted vertex-deletion comparison, and an inclusive Euler bound, all killed. Removing the balancing swap in `link` survived; it changes only the balance, and so the α bound, not results.

## Commands and results

2026-10-09, GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Linux x86-64, GNU++20.

| Command | Result |
|---|---|
| `24-dominatortree_tester.py --mode quick` | PASS 2 configurations, 1,870 cases, 232,084 checks, 7 probes |
| `24-dominatortree_tester.py --mode full --seed 1` | PASS 3 configurations, 267,714 cases, 24,270,210 checks, 7 probes; MEMORY peak 725 MB |
| `CXX=g++-14 24-dominatortree_tester.py --mode full --seed 2` | PASS 3 configurations, 267,714 cases, 24,257,760 checks, 7 probes; MEMORY peak 756 MB |
| `24-dominatortree_tester.py --mode stress --seed 3` | PASS 3 configurations, 293,714 cases, 33,805,505 checks, 7 probes; MEMORY peak 1,049 MB |
| `02-integration.py --sanitizers` | PASS 113 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 2,755 MB |
| `03-consistency.py` | No P043 errors; one error predates P043 at `HEAD` (`16-poly_tester.py` missing from `QUICK` in the committed `01-run.py`) |

```bash
python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode stress --seed 3
```

## Benchmarks

None required: contest profile, no threshold. The 1,000,000-vertex stress shapes check stack safety and near-linear behaviour, not timings.

## Sources

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, dominator tree](https://oi-wiki.org/graph/dominator-tree/) | Lengauer–Tarjan steps, semidominator theorem, DAG special case (omitted), dominated-count use |
| [Library Checker, dominatortree](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/dominatortree/task.md) | Output convention (root maps to itself, −1 unreachable) |
| [NetworkX, dominance](https://networkx.org/documentation/stable/reference/algorithms/dominance.html) and [dominance_frontiers](https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.dominance.dominance_frontiers.html) | `immediate_dominators`, `dominance_frontiers` definitions |
| [maspypy, dominator_tree](https://maspypy.github.io/library/graph/dominator_tree.hpp), [ei1333, dominator-tree](https://ei1333.github.io/library/graph/others/dominator-tree.hpp), [Koosaga, dominator](https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/dominator.cpp) | Catalog comparison; all use compression-only Lengauer–Tarjan |

Not inspected: the Lengauer–Tarjan paper itself and the Cooper–Harvey–Kennedy report (fetch returned 403). The balanced `link`/`eval` follow the published procedure as reproduced from memory and were validated only by the tests above.

## Limits and handoffs

Post-dominators: run on `g.reverse()` from the exit. Multiple roots: add a super-source. Edge dominators: subdivide each edge. Iterated dominance frontiers and the DAG-only LCA method are omitted ([00-notes.md](00-notes.md#p043-omissions)).
