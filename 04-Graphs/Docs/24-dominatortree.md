# 24-dominatortree.hpp — Evidence

Owned by package P043 / GR11; omissions are in [00-notes.md](00-notes.md#p043-omissions). Contest profile: iterative Lengauer–Tarjan with balanced linking over the verified `Graph`/`CsrGraph` representation, plus an independent vertex-deletion algorithm. The row was planned; there is no legacy code.

## Contracts

### DominatorTree

`DominatorTree(r, idom)` builds the query structure from an immediate-dominator array over `n` vertices. `idom[r]` must be −1 (asserted), `idom[v]` is −1 for vertices unreachable from `r`, and the other entries must form a tree rooted at `r`, which the two algorithms below guarantee. The fields are `root`, `idom`, and `tin[v]`, `tout[v]`: a half-open Euler interval in the dominator tree, −1 for unreachable `v`, where `tout[v] − tin[v]` is the number of vertices `v` dominates, itself included.

Construction is iterative and O(n). It asserts the root and that every entry lies in `[-1, n)`; under `NDEBUG` it ignores out-of-range entries and returns empty intervals for an out-of-range root.

- `immediateDominator(v)` returns `idom[v]`: −1 for the root and unreachable vertices. Library Checker `dominatortree` prints the root as its own dominator; map −1 at `root` accordingly.
- `dominates(u, v)` is reflexive dominance: `tin[u] ≤ tin[v] < tout[u]`. It is false when either vertex is unreachable. O(1).
- `dominanceFrontier(g)` requires the graph the tree was built from (directed, same `n`; asserted). It returns `DF(v) = { w : v dominates a reachable predecessor of w and v does not strictly dominate w }`, which includes `w = v` for a back arc or loop into `v`. Lists are duplicate-free and unordered; unreachable vertices get empty lists. Predecessors are grouped by target, and each walks up the dominator tree from the predecessor to `idom[w]`, which dominates every reachable predecessor of `w`, so each walk terminates there. Walks for the same `w` run consecutively, so meeting a vertex already stamped with `w` means the rest of its chain is recorded. Time O(V + E + out), memory O(V + out).

### dominatorTree

`dominatorTree(g, root)` takes a directed `Graph` or `CsrGraph` (asserted) and a root in range; loops and multiedges are allowed and weights ignored. It numbers vertices in iterative DFS preorder from 1, with number 0 as the forest sentinel (semi 0, size 0). It builds predecessor lists over reachable vertices, then for numbers N..2:

1. `semi[w] = min semi(eval(p))` over predecessors `p`;
2. put `w` in the bucket of `semi[w]`;
3. `link(parent, w)`;
4. for every `v` in the parent's bucket, set `dom[v] = eval(v)` if its semi is smaller, else the parent.

A final pass sets `dom[w] = dom[dom[w]]` where `dom[w] ≠ semi[w]`. `eval` and `link` are Tarjan's balanced versions with child chains and subtree sizes. `eval` returns `label[v]` even at a virtual-tree root, where the code differs from the simple compression-only variant; path compression is iterative. Time O((V + E)·α(E, V)), memory O(V + E). The semidominator theorem and the idom corollary are those of Lengauer and Tarjan (1979).

### dominatorTreeSimple

`dominatorTreeSimple(g, root)` has the same input domain and returns the same tree. For each reachable `d ≠ root`, a BFS from the root that skips `d` marks the vertices that `d` strictly dominates: the reachable vertices left unreached. The strict dominators of `w` form a chain whose dominated sets strictly shrink, so `idom(w)` is the one dominating the fewest vertices; `w` defaults to the root when nothing else strictly dominates it. Time O(V·(V + E)), memory O(V + E). Cooper–Harvey–Kennedy iteration is simpler to state but has no O(V·E) worst case.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/24-dominatortree_tester.py` runs on the shared contest runner with `-Werror` warnings: optimized, checked with probes, and ASan/UBSan in full and stress. The oracle is the dataflow fixpoint `Dom(v) = {v} ∪ ⋂ Dom(p)` over reachable predecessors with bitmask sets, independent of both algorithms; `idom(v)` is the strict dominator whose set has one element fewer, and the frontier is evaluated from its definition.

| Operation | Test | Oracle |
|---|---|---|
| `dominatorTree` (Graph and CSR), `dominatorTreeSimple` | Every looped digraph on n ≤ 4 vertices with every root (full); 4,000 random multigraphs n ≤ 14 in three shapes; 200,000-vertex chain, skip ladder and star; 3,000-vertex random graph | Dataflow sets; the two algorithms agree on the large random graph |
| `DominatorTree`: `immediateDominator`, `dominates`, `tin`/`tout` | All small cases, all vertex pairs, unreachable vertices | Membership in `Dom(v)`; subtree size equals dominated count |
| `dominanceFrontier` | All small cases; back-arc frontier on the 200,000 chain | Definition evaluated over all predecessors; exact set and no duplicates |
| Preconditions | 7 probes: undirected graph, root out of range (both algorithms), frontier on a different graph, query out of range, constructor entry out of range, constructor root with a dominator | Expected assertion failure |

Planted mutants, quick suite: `eval` returning `v` instead of `label[v]` at virtual roots, a missing final idom pass, unstamped frontier walks, an inverted vertex-deletion comparison and an inclusive Euler bound were all killed. Removing the balancing swap in `link` survived; it changes only the balance, and so the α bound, not results.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, CPython 3.14, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). The 2026-11-10 library-wide runs used seed 42, beside the other suites, with the build cache emptied before each cold run.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 766 MB; optimized 0.1 s compile + 7.7 s run, checked 13.1 + 28.0 s, ASan-UBSan 14.8 + 139.5 s; MEMORY peak 914 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 823 MB; optimized 0.2 + 8.4 s, checked 16.2 + 32.8 s, ASan-UBSan 17.6 + 151.1 s; MEMORY peak 985 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 22.2 + 0.2 s, checked 18.2 + 3.2 s; MEMORY peak 762 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 767 MB; optimized 0.1 + 5.5 s, checked 6.2 + 26.5 s, ASan-UBSan 6.9 + 104.7 s; MEMORY peak 979 MB |
| 2026-11-06 | P253: `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full; `02-integration.py` | PASS (counts not recorded) |
| 2026-10-09 | `24-dominatortree_tester.py --mode quick` | PASS 2 configurations, 1,870 cases, 232,084 checks, 7 probes |
| 2026-10-09 | `24-dominatortree_tester.py --mode full --seed 1` | PASS 3 configurations, 267,714 cases, 24,270,210 checks, 7 probes; MEMORY peak 725 MB |
| 2026-10-09 | `CXX=g++-14 24-dominatortree_tester.py --mode full --seed 2` | PASS 3 configurations, 267,714 cases, 24,257,760 checks, 7 probes; MEMORY peak 756 MB |
| 2026-10-09 | `24-dominatortree_tester.py --mode stress --seed 3` | PASS 3 configurations, 293,714 cases, 33,805,505 checks, 7 probes; MEMORY peak 1,049 MB |
| 2026-10-09 | `02-integration.py --sanitizers` | PASS 113 standalone and aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 2,755 MB |
| 2026-10-09 | `03-consistency.py` | No P043 errors |

```bash
python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/24-dominatortree_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: Contest profile, no threshold. The 1,000,000-vertex stress shapes check stack safety and near-linear behaviour, not timings.

## Sources

Fetched 2026-10-09; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-09-p043).

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, dominator tree](https://oi-wiki.org/graph/dominator-tree/) | Lengauer–Tarjan steps, semidominator theorem, DAG special case (row operation `dominatorTreeDag`, missing), dominated-count use (`DominatorTree::dominatedCount`, missing) |
| [Library Checker, dominatortree](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/dominatortree/task.md) | Output convention (root maps to itself, −1 unreachable) |
| [NetworkX, dominance](https://networkx.org/documentation/stable/reference/algorithms/dominance.html) and [dominance_frontiers](https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.dominance.dominance_frontiers.html) | `immediate_dominators`, `dominance_frontiers` definitions |
| [maspypy, dominator_tree](https://maspypy.github.io/library/graph/dominator_tree.hpp), [ei1333, dominator-tree](https://ei1333.github.io/library/graph/others/dominator-tree.hpp), [Koosaga, dominator](https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/dominator.cpp) | Catalog comparison; all use compression-only Lengauer–Tarjan |

Not fetched: the Lengauer–Tarjan paper (1979) and the Cooper–Harvey–Kennedy report (fetch returned 403). The balanced `link`/`eval` follow the published procedure as reproduced from memory and are validated only by the tests above.

## Limits and Handoffs

- The row is `partial`: `dominatorTreeDag`, `DominatorTree::dominatedCount` and `postDominatorTree` are missing. Until `postDominatorTree` lands, run `dominatorTree` on `g.reverse()` from the exit.
- Multiple roots: add a super-source. Edge dominators: subdivide each edge.
- Iterated dominance frontiers stay out ([00-notes.md](00-notes.md#p043-omissions)).
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 支配树, 必经点, 支配点, 最近支配点, 支配边界, 灭绝树.

## History

- 2026-10-09: verified (P043); quick, full and stress passed on g++, full on g++-14; independent review: no wrong answers (own Cooper–Harvey–Kennedy oracle, overflow probes at the stated domain bounds).
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; quick, full, `CXX=g++-14` full and integration passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
