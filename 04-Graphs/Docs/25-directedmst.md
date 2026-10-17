# 25-directedmst.hpp — evidence

Owned by package P047 / GR69; see [00-notes.md](00-notes.md#p047-package-record). Contest profile: Tarjan's contraction with the Data Structures 25 `LeftistHeap` (lazy add) and the Data Structures 01 `DSU`, followed by an O(n) expansion for any root. The row was planned; there is no legacy code.

## Contracts

### DirectedMstResult

`ok`, `weight` (exact `lll` total of the chosen arcs) and `parentEdge[v]`, the ID of the arc entering `v` (−1 at roots). When `ok` is false, `weight` is 0 and `parentEdge` is empty.

### DirectedMst

`DirectedMst(g)` takes a directed `Graph` or `CsrGraph` (asserted); weights are full-range `lng`, loops are ignored and parallel arcs allowed. `DirectedMst(n, arcs)` is the underlying builder over `Arc{from, to, id, w}` with endpoints in `[0, n)`, `n ≤ INT_MAX / 2`, `lll` weights with `|w| < 2^64` (asserted) and any `id`, copied to `parentEdge`; `maximumBranching` uses it. Both cost O(n + m log m) time and O(n + m) memory.

Construction appends a ring of artificial arcs `v → v + 1 (mod n)` of weight `INF = 2^100` (for `n ≥ 2`), which makes the graph strongly connected, then runs the contraction phase independently of any root. A path grows from vertex 0 along the cheapest in-arc of its current end (the heap top of that super node, skipping arcs whose tail already lies in it). When the tail's super node is already on the path, the cycle closes: each member `x` gets `fa[x] = c` for a new node `c`, its heap is lazily reduced by the weight of its chosen arc (`cut[x]`) and melded into `c`, and `nxt` links the members in a ring. The phase ends when the current end's heap is exhausted, which happens only for the single node `top` that contains everything. Each arc is popped at most once, and the contractions meld at most `2n − 1` heaps in total (each node once into its parent), so the phase is O(m log m) with at most `n − 1` contractions and `2n − 1` nodes.

`arborescence(root)` (asserted in range) climbs from `root` through `fa` to `top`. At each node `x` on the way, every other member `u` of `x`'s contraction cycle keeps its chosen arc, and the arc's head becomes the root of a recursive expansion inside `u`. An explicit stack replaces the recursion. Every contraction-tree node is climbed through once, so a query is O(n). The result is a minimum arborescence of the augmented graph rooted at `root`, which is the property of the contraction tree proved by Tarjan (1977), with Camerini, Fratta and Maffioli's correction, and stated in OI Wiki. An arborescence with an artificial arc costs at least `INF − (n − 2)·2^64`, and every real total is below `(n − 1)·2^64`. Since `(2n − 3)·2^64 < 2^95 < INF` for `n ≤ 2^30`, the optimum uses an artificial arc exactly when some vertex is unreachable from `root`, and the query then returns `ok = false`. This bound is why the builder asserts `|w| < 2^64`: with `|w|` near `2^99`, a ring arc can undercut a real arborescence, as the review's three-vertex reproducer showed. Reduced keys stay within `[−2^64, INF + 2^65]` and totals within ±2^94, both inside `lll`.

The structure is immutable after construction. Queries for different roots are independent, and repeated queries return identical results.

### maximumBranching

`maximumBranching(g)` (directed, asserted) returns a maximum-weight branching: a forest of in-trees with at most one chosen in-arc per vertex, any number of roots and weights of any sign. Only arcs with `w > 0` can improve a branching, so the others are dropped. A super-root `n` gets arcs of key 0 to every vertex, the real arcs take key `−w`, which is exact in `lll` even for `w = −2^63` (that arc is dropped anyway), and the result is `DirectedMst(n + 1, arcs).arborescence(n)` with the sign restored. Vertices entered by a super arc are roots (`parentEdge` −1). `ok` is always true. Time O(n + m log m), memory O(n + m).

## Legacy mapping

No legacy code. The planned row name `directedMst(g, root)` became `DirectedMst(g).arborescence(root)`: one build serves every root, and a single-root free function would be a wrapper of it (`03-cpp.md`, One API per operation).

## Aliases

- Library Checker `directedmst` prints `p_S = S`; map `parentEdge[root] = −1` and the other arcs' tails.
- KACTL `dmst` (−1 when unreachable), ei1333/maspypy edge-list or edge-ID outputs, Koosaga single-root contraction and hitonanode O(NM) Chu–Liu/Edmonds are `arborescence`.
- NetworkX `maximum_branching` is `maximumBranching`; `minimum_branching` is it on negated weights; `minimum_spanning_arborescence` without a root is the minimum over `arborescence(r)` (see [00-notes.md](00-notes.md#p047-omissions)).

## Feature-to-test map

`96-Local Testing/04-Graphs/25-directedmst_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with probes, ASan/UBSan in full/stress). Oracles: exhaustive enumeration of one in-arc per non-root vertex (one or none for branchings) with acyclicity by parent walks, and an independent O(V·E) Chu–Liu/Edmonds with explicit contraction. Every returned witness is checked: arcs enter their vertex, parent walks reach the root or a branching root, and the weight is the arc sum.

| Operation | Test | Oracle |
|---|---|---|
| `DirectedMst(g)` (Graph and CSR), `arborescence` for every root | Every simple digraph n ≤ 4 in three weight domains (small signed, nonnegative, full-range extremes ±2^63); 5,000 random multigraphs n ≤ 6 with loops and parallel arcs; 200 medium graphs n ≤ 60; 100,000-vertex random graph (roots 0, n/2, n−1) and directed cycle; 3,000-vertex chain plus random arcs | Exhaustive minimum and existence; Chu–Liu/Edmonds on medium and chain graphs; witness checks at every size |
| `DirectedMstResult` (`ok`, `weight`, `parentEdge`) | All cases, including unreachable roots (`ok` false, weight 0, empty parentEdge) and n = 0, 1 | Exhaustive existence |
| `maximumBranching` | Same small cases; 100,000-vertex random graph | Exhaustive maximum over branchings; positive-arc and acyclicity witness checks |
| `DirectedMst(n, arcs)` builder | 5,000 random arc lists n ≤ 5 with `lll` weights from {0, ±1, ±(2^63 − 1), ±2^63, ±(2^64 − 1)} and small values, IDs `1000 + i`, every root; the review's `2^63` three-vertex instance | Exhaustive minimum over the arc list in `lll` |
| Preconditions | 7 probes: undirected graph (`DirectedMst`, `maximumBranching`), root out of range, arc endpoint out of range, arc weight `INF` and `2^64`, negative `n` | Expected assertion failure |

Planted mutants (quick suite, optimized build): an unlinked cycle ring (`nxt[p] = x`), a missing lazy reduction before melding, a missing `INF` failure check, and branchings admitting zero-weight arcs. All four were killed.

## Commands and results

2026-10-18, GCC 16.2.1 and GCC 14.4.1 (floor), CPython 3.14.8, Linux x86-64 (Intel Core i9-11900H), GNU++20, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds.

| Command | Result |
|---|---|
| `25-directedmst_tester.py --mode quick` | PASS 2 configurations, 970 cases, 57,688 checks per configuration, 7 probes |
| `25-directedmst_tester.py --mode full --seed 1` | PASS 3 configurations, 14,166 cases, 1,249,345 checks per configuration, 7 probes; MEMORY peak 601 MB |
| `CXX=g++-14 25-directedmst_tester.py --mode full --seed 2` | PASS 3 configurations, 14,166 cases, 1,814,630 checks per configuration, 7 probes; MEMORY peak 620 MB |
| `25-directedmst_tester.py --mode stress --seed 3` | PASS 3 configurations, 54,166 cases, 5,418,102 checks per configuration, 7 probes; MEMORY peak 858 MB |
| `02-integration.py --sanitizers` | PASS: 159 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 1,829 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: contest profile, no threshold. The 100,000/300,000-vertex shapes check stack safety and near-linear behaviour, not timings.

## Sources

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, 最小树形图](https://oi-wiki.org/graph/dmst/) | Tarjan contraction with leftist heaps, the infinite ring, the per-root `expand`/`expand_iter` recursion |
| [KACTL DirectedMST](https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/DirectedMST.h) | Rooted contraction with lazy heaps; unreachable result |
| [Library Checker directedmst](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/directedmst/task.md) | Output convention |
| [maspypy](https://maspypy.github.io/library/graph/directed_mst.hpp), [ei1333](https://ei1333.github.io/library/graph/mst/directed-mst.hpp), [hitonanode](https://hitonanode.github.io/cplib-cpp/graph/directed_mst.hpp), [Koosaga](https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/directed_mst.cpp), [NetworkX branchings](https://networkx.org/documentation/stable/reference/algorithms/tree.html) | Catalog comparison (see Aliases) |

Not inspected: Tarjan (1977), Camerini–Fratta–Maffioli (1979) and Gabow–Galil–Spencer–Tarjan (1986). The expansion follows the OI Wiki description and was validated only by the tests above.

## Limits and handoffs

- Weights are `lng` through `Graph`; `DirectedMst(n, arcs)` accepts `lll` weights below `2^64` in magnitude for derived instances; negated `lng` weights fit.
- An unreachable root gives no partial witness; `bfs` from the root finds the unreachable vertices.
- k-best arborescences, a free-root minimum and the Gabow–Galil–Spencer–Tarjan bound are omitted ([00-notes.md](00-notes.md#p047-omissions)).

## History

- 2026-10-18: verified (P047); new row; all-roots contraction tree from the completeness sweep; independent review: 1 correctness finding fixed (the builder now asserts `|w| < 2^64`, where the `INF` ring is sound; builder tests with `lll` extremes and custom IDs added) and the meld-count wording corrected; 4 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
