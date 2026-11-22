# 25-directedmst.hpp — Evidence

Owned by package P047 / GR69; omissions are in [00-notes.md](00-notes.md#p047-omissions). Contest profile: Tarjan's contraction with the Data Structures 25 `LeftistHeap` (lazy add) and the Data Structures 01 `DSU`, followed by an O(n) expansion for any root. The row was planned; there is no legacy code.

## Contracts

### DirectedMstResult

`ok`, `weight` (exact `lll` total of the chosen arcs) and `parentEdge[v]`, the ID of the arc entering `v` (−1 at roots). When `ok` is false, `weight` is 0 and `parentEdge` is empty.

### DirectedMst

`DirectedMst(g)` takes a directed `Graph` or `CsrGraph` (asserted); weights are full-range `lng`, loops are ignored and parallel arcs allowed. `DirectedMst(n, arcs)` is the underlying builder over `Arc{from, to, id, w}` with endpoints in `[0, n)`, `n ≤ INT_MAX / 2`, `lll` weights with `|w| < 2^64` (asserted) and any `id`, copied to `parentEdge`; `maximumBranching` uses it. Both cost O(n + m log m) time and O(n + m) memory.

Construction appends a ring of artificial arcs `v → v + 1 (mod n)` of weight `INF = 2^100` (for `n ≥ 2`), making the graph strongly connected, and runs the contraction phase independently of any root. A path grows from vertex 0 along the cheapest in-arc of its current end (the heap top of that super node, skipping arcs whose tail already lies in it). When the tail's super node is already on the path, the cycle closes: each member `x` gets `fa[x] = c` for a new node `c`, its heap is lazily reduced by the weight of its chosen arc (`cut[x]`) and melded into `c`, and `nxt` links the members in a ring. The phase ends when the current end's heap is exhausted, which happens only for the single node `top` that contains everything. Each arc is popped at most once, and the contractions meld at most `2n − 1` heaps in total (each node once into its parent), so the phase is O(m log m) with at most `n − 1` contractions and `2n − 1` nodes.

`arborescence(root)` (asserted in range) climbs from `root` through `fa` to `top`. At each node `x` on the way, every other member `u` of `x`'s contraction cycle keeps its chosen arc, and the arc's head becomes the root of a recursive expansion inside `u`. An explicit stack replaces the recursion. Every contraction-tree node is climbed through once, so a query is O(n). This climb is algorithm LEAF of Camerini, Fratta and Maffioli (1979), which replaces Tarjan's (1977) extraction. OI Wiki states that the result is a minimum arborescence of the augmented graph rooted at `root`; none of the three papers read for this row proves it (Sources). An arborescence with an artificial arc costs at least `INF − (n − 2)·2^64`, and every real total is below `(n − 1)·2^64`. Since `(2n − 3)·2^64 < 2^95 < INF` for `n ≤ 2^30`, the optimum uses an artificial arc exactly when some vertex is unreachable from `root`, and the query then returns `ok = false`. The builder therefore asserts `|w| < 2^64`: with `|w|` near `2^99` a ring arc can undercut a real arborescence, as the review's three-vertex reproducer showed. Reduced keys stay within `[−2^64, INF + 2^65]` and totals within ±2^94, both inside `lll`.

The structure is immutable after construction. Queries for different roots are independent, and repeated queries return identical results.

### maximumBranching

`maximumBranching(g)` (directed, asserted) returns a maximum-weight branching: a forest of in-trees with at most one chosen in-arc per vertex, any number of roots and weights of any sign. Only arcs with `w > 0` can improve a branching, so the others are dropped. A super-root `n` gets arcs of key 0 to every vertex, the real arcs take key `−w`, which is exact in `lll` even for `w = −2^63` (that arc is dropped anyway), and the result is `DirectedMst(n + 1, arcs).arborescence(n)` with the sign restored. Vertices entered by a super arc are roots (`parentEdge` −1). `ok` is always true. Time O(n + m log m), memory O(n + m).

## Legacy Mapping

No legacy code. The planned row name `directedMst(g, root)` became `DirectedMst(g).arborescence(root)`: one build serves every root, and a single-root free function would be a wrapper of it (`03-cpp.md`, One API per Operation).

## Aliases

- Library Checker `directedmst` prints `p_S = S`; map `parentEdge[root] = −1` and the other arcs' tails.
- KACTL `dmst` (−1 when unreachable), ei1333/maspypy edge-list or edge-ID outputs, Koosaga single-root contraction and hitonanode O(NM) Chu–Liu/Edmonds are `arborescence`; the O(nm) algorithm itself is the row's `chuLiuEdmonds` (missing).
- NetworkX `maximum_branching` is `maximumBranching`; `minimum_branching` is it on negated weights; `minimum_spanning_arborescence` without a root is the row's `DirectedMst::arborescenceAnyRoot` (missing), until then the minimum over `arborescence(r)`.

Search aliases (P263, grab --find): DMST (directed minimum spanning tree), arborescence, Chu-Liu/Edmonds (最小树形图), 朱刘算法, 有向图最小生成树, 不定根最小树形图, 最大分支.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/25-directedmst_tester.py` runs on the shared contest runner with `-Werror` warnings: optimized, checked with probes, and ASan/UBSan in full and stress. Oracles: exhaustive enumeration of one in-arc per non-root vertex (one or none for branchings) with acyclicity by parent walks, and an independent O(V·E) Chu–Liu/Edmonds with explicit contraction. Every returned witness is checked: arcs enter their vertex, parent walks reach the root or a branching root, and the weight is the arc sum.

| Operation | Test | Oracle |
|---|---|---|
| `DirectedMst(g)` (Graph and CSR), `arborescence` for every root | Every simple digraph n ≤ 4 in three weight domains (small signed, nonnegative, full-range extremes ±2^63); 5,000 random multigraphs n ≤ 6 with loops and parallel arcs; 200 medium graphs n ≤ 60; 100,000-vertex random graph (roots 0, n/2, n−1) and directed cycle; 3,000-vertex chain plus random arcs | Exhaustive minimum and existence; Chu–Liu/Edmonds on medium and chain graphs; witness checks at every size |
| `DirectedMstResult` (`ok`, `weight`, `parentEdge`) | All cases, including unreachable roots (`ok` false, weight 0, empty parentEdge) and n = 0, 1 | Exhaustive existence |
| `maximumBranching` | Same small cases; 100,000-vertex random graph | Exhaustive maximum over branchings; positive-arc and acyclicity witness checks |
| `DirectedMst(n, arcs)` builder | 5,000 random arc lists n ≤ 5 with `lll` weights from {0, ±1, ±(2^63 − 1), ±2^63, ±(2^64 − 1)} and small values, IDs `1000 + i`, every root; the review's `2^63` three-vertex instance | Exhaustive minimum over the arc list in `lll` |
| Preconditions | 7 probes: undirected graph (`DirectedMst`, `maximumBranching`), root out of range, arc endpoint out of range, arc weight `INF` and `2^64`, negative `n` | Expected assertion failure |

Four planted mutants, quick suite, optimized build, were all killed: an unlinked cycle ring (`nxt[p] = x`), a missing lazy reduction before melding, a missing `INF` failure check, and branchings admitting zero-weight arcs.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, CPython 3.14.8, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`, floor), the runner's optimized, checked and ASan/UBSan builds with `-Wall -Wextra -Wconversion -Werror`. The 2026-11-10 library-wide runs used seed 42, beside the other suites, with the build cache emptied before each cold run.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 781 MB; optimized 0.2 s compile + 5.5 s run, checked 18.1 + 11.7 s, ASan-UBSan 19.6 + 31.5 s; MEMORY peak 937 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 811 MB; optimized 0.2 + 8.1 s, checked 24.4 + 13.4 s, ASan-UBSan 26.2 + 35.0 s; MEMORY peak 1000 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 23.9 + 0.1 s, checked 17.3 + 2.1 s; MEMORY peak 776 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 784 MB; optimized 0.1 + 13.2 s, checked 6.8 + 21.2 s, ASan-UBSan 7.3 + 45.8 s; MEMORY peak 1104 MB |
| 2026-11-06 | P253: `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full; `02-integration.py` | PASS (counts not recorded) |
| 2026-10-18 | `25-directedmst_tester.py --mode quick` | PASS 2 configurations, 970 cases, 57,688 checks per configuration, 7 probes |
| 2026-10-18 | `25-directedmst_tester.py --mode full --seed 1` | PASS 3 configurations, 14,166 cases, 1,249,345 checks per configuration, 7 probes; MEMORY peak 601 MB |
| 2026-10-18 | `CXX=g++-14 25-directedmst_tester.py --mode full --seed 2` | PASS 3 configurations, 14,166 cases, 1,814,630 checks per configuration, 7 probes; MEMORY peak 620 MB |
| 2026-10-18 | `25-directedmst_tester.py --mode stress --seed 3` | PASS 3 configurations, 54,166 cases, 5,418,102 checks per configuration, 7 probes; MEMORY peak 858 MB |
| 2026-10-18 | `02-integration.py --sanitizers` | PASS: 159 standalone and aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 1,829 MB |
| 2026-10-18 | `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/25-directedmst_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: Contest profile, no threshold. The 100,000/300,000-vertex shapes check stack safety and near-linear behaviour, not timings.

## Sources

Fetched 2026-10-18; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-18-p047).

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, 最小树形图](https://oi-wiki.org/graph/dmst/) | Tarjan contraction with leftist heaps, the infinite ring, the per-root `expand`/`expand_iter` recursion |
| [KACTL DirectedMST](https://github.com/kth-competitive-programming/kactl/blob/main/content/graph/DirectedMST.h) | Rooted contraction with lazy heaps; unreachable result |
| [Library Checker directedmst](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/graph/directedmst/task.md) | Output convention |
| [maspypy](https://maspypy.github.io/library/graph/directed_mst.hpp), [ei1333](https://ei1333.github.io/library/graph/mst/directed-mst.hpp), [hitonanode](https://hitonanode.github.io/cplib-cpp/graph/directed_mst.hpp), [Koosaga](https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/directed_mst.cpp), [NetworkX branchings](https://networkx.org/documentation/stable/reference/algorithms/tree.html) | Catalog comparison (see Aliases) |

Read on 2026-11-22 ([00-sources.md](00-sources.md#fetched-2026-11-22-source-check)):

- Gabow, Galil, Spencer and Tarjan (1986, §3) describe the contraction phase used here. Dummy edges make the graph strongly connected, and a single growth path is extended by a cheapest entering edge. A closed cycle is contracted after each member's chosen-edge cost is subtracted from the edges entering it. They add, without proof, that a varied expansion serves a specified or an unspecified root.
- Tarjan (1977, §2) extracts the branching through his Lemma 2, which he does not prove. Camerini, Fratta and Maffioli (1979) refute that lemma with a counterexample. Their repair records the nesting of contracted cycles as a forest over the chosen edges and expands it with algorithm LEAF; `arborescence` climbs the same structure. They defer LEAF's proof to their paper on the k best spanning arborescences (Networks 10(2), 1980), which is not on file; it was requested from the user on 2026-11-22.
- So the expansion rests on that published description and the tests above, not on a proof read here. Tarjan (1977) does support the O(m log n) bound (§3) and the rule that only positive edges enter a branching (step G1), as in `maximumBranching`.

## Limits and Handoffs

- Weights are `lng` through `Graph`; `DirectedMst(n, arcs)` accepts `lll` weights below `2^64` in magnitude for derived instances; negated `lng` weights fit.
- An unreachable root gives no partial witness; `bfs` from the root finds the unreachable vertices.
- The row is `partial`: `chuLiuEdmonds`, `directedMstDense`, `kBestArborescences`, `maximumArborescence` and `DirectedMst::arborescenceAnyRoot` are missing.
- The Gabow–Galil–Spencer–Tarjan bound stays out ([00-notes.md](00-notes.md#p047-omissions)).
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## History

- 2026-10-18: verified (P047); new row; all-roots contraction tree from the completeness sweep; independent review: 1 correctness finding fixed (the builder now asserts `|w| < 2^64`, where the `INF` ring is sound; builder tests with `lll` extremes and custom IDs added) and the meld-count wording corrected; 4 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; quick, full, `CXX=g++-14` full and integration passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
