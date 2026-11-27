# 06-mst.hpp — Evidence

Owned by package P010 / GR02 (graph foundations, with `01-graph.hpp` through `05-shortest_path.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Scope

The row covers undirected minimum and maximum spanning forests over the integer weight domain below and the Kruskal reconstruction tree with its bottleneck queries. Directed MSTs, sensitivity and second-best MSTs, spanning-tree counting and fully dynamic MSTs belong to their own inventory rows; general LCA and dynamic-tree APIs keep their own owners.

### Weight Domain

Every engine uses exact signed-integer weights from the full `lng` range and `lll` accumulated costs. Status fields distinguish absent and present answers without reserving a numeric weight. Fractional and custom-weight instantiations of the archived notebook templates are outside this integer API.

### SpanningForest, kruskal, primSparse, primDense, boruvka and Their Maximum Mirrors

Every operation requires an undirected graph, ignores loops and accepts parallel and negative edges. `kruskalMaximum`, `primSparseMaximum` and `boruvkaMaximum` (and `primDense` on a maximum `DenseGraph` view) build a maximum-weight spanning forest. Each pair shares one engine in `mst_detail` that takes the order as a flag, so a mirror differs from its minimum form only in that flag; the comparator reverses the weight order, keeping edge-ID ties ascending, instead of negating weights, since `-w` overflows for the `lng` minimum. `primSparseMaximum` keys its set by `~w = -w - 1`, an order-reversing bijection on all of `lng`. `primDense` takes the order from its `DenseGraph` view, which already chose the minimum or maximum parallel arc of each pair, so it keeps that view rather than a mirror (user decision 2026-11-15).

`SpanningForest` holds the exact `lll` weight, the component count including isolated vertices, and the original logical edge IDs; an empty graph has zero components, weight zero and no edges. Ties compare weights, then edge IDs. Returned edge order follows each algorithm and is otherwise unspecified (user decision 2026-11-15).

Correctness and cost: a forest has exactly n − components edges, so its full-range signed sum fits in `lll`. Kruskal adds sorted edges only across distinct components. Prim repeatedly chooses the minimum crossing edge; its sparse set keeps one candidate per unused vertex, and its dense form scans the minimum-edge matrix. Both restart at isolated or disconnected components. Borůvka caches phase-start representatives and chooses each active component's minimum outgoing edge; union guards suppress duplicates and cycles, and active components halve per phase until each connected component is merged. The cut property, with the inequalities reversed for maximum forests, justifies every accepted edge. Bounds: O(n + m log(m+1)) for Kruskal, O((n+m) log(n+1)) for sparse Prim and Borůvka, and O(n²) for dense Prim, with the O(n²+m) dense-view setup charged separately.

### BottleneckResult and KruskalReconstruction

`BottleneckResult` distinguishes a disconnected pair, a valid empty self-path, and a nonempty path whose `value` and `edge` are its bottleneck weight and merge edge. `KruskalReconstruction(g, maximum)` requires an undirected graph with `n <= (INT_MAX + 1) / 2`, so the at most 2n−1 nodes fit `int`. Leaves `[0, n)` are the original vertices; each merge node has two children, its joining edge ID and weight, and a larger ID than its children. `parent` is `-1` at roots, `root[u]` is the tree root, `leaf_count[u]` counts original leaves, and leaf `value` 0 is unused. Ascending construction answers minimax paths, `maximum = true` maximin. `lca(u, v)` accepts all nodes and returns `-1` across trees. `bottleneck` and `componentAt` accept original vertices. `componentAt(u, t)` is the highest ancestor whose merges all have weight `<= t` (`>= t` for maximum), so its leaves form u's component, of size `leaf_count`. `leaves` lists the original vertices so that every node's leaves are contiguous, and `leafRange(node)` returns that half-open interval `[start[node], start[node] + leaf_count[node])` into `leaves`. Construction workspace is O(n + m).

Correctness: the LCA's merge weight is the minimum possible maximum edge on a path, and with `maximum = true` the maximum possible minimum edge. Merge weights are monotone along every root path, which makes binary lifting valid for `lca` and for `componentAt`, which climbs every threshold-eligible ancestor including equal-weight merges. One pass over descending node IDs builds depth, root and lifting tables without recursion, and assigns `start`: a root starts after the previous roots' leaves, a first child at its parent's start, a second child after the first child's leaves, so every subtree is one contiguous interval of `leaves`, in O(n) extra time and memory. Preprocessing and storage are O(n log(n+1)) beyond edge sorting, with O(log(n+1)) queries.

### Legacy Mapping

- `Kruskal<T>(n, E).solve()` (`OLD/Team Notebook/src/algs.cpp:1603`, `algsbetter.cpp:1742`, `test.cpp:481`) → `kruskal(g)` on a `Graph` with one `addEdge(u, v, w)` per entry of `E`: `weight` is its cost and `edges` the chosen edge IDs.
- `PrimDense<T>(n, adj).solve()` (`algs.cpp:1632`, `algsbetter.cpp:1798`, `test.cpp:505`) → `primDense` on a minimum `DenseGraph`. A disconnected graph gives a spanning forest (`components > 1`) instead of the `-1` sentinel, and the empty-graph bug is not inherited.
- `kruskal(g, true)`, `primSparse(g, true)` and `boruvka(g, true)` (flags removed by GR02, user decision 2026-11-14) → `kruskalMaximum(g)`, `primSparseMaximum(g)` and `boruvkaMaximum(g)`; `KruskalReconstruction(g, maximum)` keeps its constructor parameter.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/06-mst_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Full runs 1,099 exhaustive weighted graphs and 400 random multigraphs; quick uses n ≤ 3, 40 random cases and 1,000-vertex chains, and stress 2,500 random cases and 300,000-vertex chains.

| Operation | Test | Oracle |
|---|---|---|
| `SpanningForest`, `kruskal`, `primSparse`, `primDense`, `boruvka` (minimum) | Graph and CSR, minimum `DenseGraph` view (`primDense` has no CSR form); explicit tie IDs; full-range weights and totals; copy, move and snapshots; 100,000-vertex chains for the sparse engines (`primDense` is quadratic and runs on the small and random graphs only) | Independent forest-subset minimum |
| `kruskalMaximum`, `primSparseMaximum`, `primDense` (maximum view), `boruvkaMaximum` | Graph and CSR, maximum `DenseGraph` view; explicit tie IDs for both orders; a triangle chain with closed-form minimum and maximum totals | Independent forest-subset maximum (`oracle.maximum`) |
| `BottleneckResult`, `KruskalReconstruction: bottleneck` (minimax and maximin) | Merge-edge certificates | Floyd minimax and maximin closures |
| `KruskalReconstruction: componentAt` (threshold) | Every threshold | Floyd threshold closures |
| `KruskalReconstruction: lca` | Every node | Ancestor sets |
| `KruskalReconstruction: leafRange` | Every node | Explicit subtree leaf sets |
| Preconditions | 21 assertion probes: each index bound of `bottleneck`, `lca` and `leafRange` at the end and past it in either argument, on a path whose 3 vertices and 5 nodes separate the vertex and node bounds; `too-many-nodes` at n = 2^30 + 1 | Expected assertion failure |
| Size bound | `KruskalReconstruction` at n = 2^30, the largest accepted size, on a graph type holding only `n`, `directed` and `edges`, in a child process limited to 1 GiB of address space (optimized and checked builds; not under ASan, whose shadow memory needs more) | The constructor passes its assertion and ends in `bad_alloc` |

## Mutation Testing

Record: mewt 4.0.0 on blob `4db8d7d8bd2d3aa65192e2e583f9878cc4c41f36` (2026-11-15, seed 1, 1 lines changed since blob `0d06a53bb94c`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The changed line is /finish's direct `DSU` include; the review's comment split and the removal of the unused `edgeLess`/`edgeOrder` defaults before it generated no mutant either. The survivors below come from the whole-file campaign of 2026-11-15 (seed 1, uniform sample of 500 of 681; 470 caught, 1 by timeout, 30 survived after the retest with the extended tester), on lines no later record covered; the two on the removed defaults are gone. The 2026-11-27 size-bound check closed `AOS 285`, `AOS 286`, `AOS 287` and `COS 498`, each applied by hand to a scratch copy and caught by the checked build. Line numbers are the current header's. All are equivalent.

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 385`, `COS 388`, `>`/`<` → `>=`/`<=` | 15 | Reached only when the two weights differ. |
| `AS 211`, `uniteSets(e.u, e.v)` → `(e.v, e.u)` | 31 | Kruskal reads only whether the sets merged. |
| `AS 229`, the same in Borůvka | 72 | Borůvka also indexes `best` by representative, so the order of `edges` can change (422 of 40,000 random graphs in the /finish review), never the set or the weight. Equivalent by user decision 2026-11-15: the order is unspecified. |
| `COS 404`, `COS 409`, `COS 414`, `COS 464`, `!= -1` → `> -1` | 47, 50, 51, 100 | Edge IDs are `-1` or nonnegative. |
| `COS 433`, `COS 443`, `COS 483`, `COS 513`, `COS 518`, `COS 623`, `== -1` → `<= -1` | 66, 70, 107, 151, 172 | Edge IDs, arc IDs, parents and `lca` results are `-1` or nonnegative. |
| `COS 422`, `COS 447`, `COS 477`, `COS 542`, `<` → `!=` | 63, 97, 105, 156 | The counter steps by 1 from a value not above the bound (`levels >= 1`). |
| `AS 251`, `max(1, b)` → `max(b, 1)` | 146 | `max` of two ints is symmetric. |
| `IT 193`, `COS 538`, `u < n` → `true` / `u <= n` | 155 | Descending IDs visit every inner node before any leaf; an inner node's write to `leaves[start[u]]` (inside [0, n)) is overwritten by the leftmost leaf of its subtree, which shares that start. |
| `AS 253`, `swap(u, v)` → `swap(v, u)` | 161 | `swap` is symmetric. |
| `COS 573`, `depth[u] < depth[v]` → `<=` | 161 | Swapping equal depths leaves `delta = 0`, and the lifting and the returned common parent are symmetric in u and v. |
| `AOS 319`, `int(up.size()) - 1` → `/ 1` | 163 | The extra first iteration tests bit `levels` of `delta`, which is 0 since `delta < count < 2^levels`, so `up[levels]` is never read. |

## Commands and Results

2026-11-27 (P010 limits recheck, experiment none) for the tester runs, extraction of the tester and integration; 2026-11-15 (P010 GR02) for the row so marked, the header being unchanged since. Linux x86-64, Intel Core i9-11900H shared with other sessions, GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20, `-Wall -Wextra -Wconversion -Werror`.

| Run | Result |
|---|---|
| Full, seed 20261127, g++ | PASS, all three builds, 871,970 checks (optimized, checked) and 871,969 (ASan-UBSan, which skips the size bound), 21 probes; optimized 0.4 s, checked 1.8 s, ASan-UBSan 4.5 s; pre-flight quick peak 909 MB; peak 1,141 MB |
| Full, seed 20261127, `CXX=g++-14` | PASS, same counts; optimized 0.2 s, checked 2.7 s, ASan-UBSan 4.6 s; peak 1,131 MB |
| Stress, seed 7 | PASS, all three builds, 2,052,050 checks (2,052,049 under ASan-UBSan), 21 probes; optimized 0.7 s, checked 3.8 s, ASan-UBSan 10.6 s; peak 847 MB |
| Hand-applied size-bound mutants (scratch copy, checked build) | 4 of 4 caught: `AOS 285`, `AOS 286`, `AOS 287`, `COS 498` |
| `05-extraction.py ops --filter 06-mst.hpp` (2026-11-15) | PASS on g++ and g++-14 (14 operations in 13 extractions) |
| `05-extraction.py testers --filter 06-mst_tester` | PASS |
| `02-integration.py` | PASS, 192 standalone and aggregate headers, scalar and AVX2 multi-TU, workspace; peak 3,438 MB |

```bash
python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode full --seed 20261127
CXX=g++-14 python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode full --seed 20261127
python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter 06-mst.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 06-mst_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/06-mst.hpp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [driver](<../../96-Local Testing/04-Graphs/00-path_mst_benchmark.py>) compares sparse and dense Prim (and Dijkstra, recorded in `05-shortest_path.md`) on the workloads described there: chains, random sparse multigraphs, complete graphs, an adversarial complete graph (Θ(n^2) heap pushes for the sparse form) and disconnected blocks of at most eight vertices, at sizes 24, 200 and 800, seed 20260927. Five samples follow checked warmups, with repeated runs for smaller cases. Every forest total and component count is compared against Kruskal. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately. Conditions, from the git-ignored `00-path_mst_benchmark.json` log that also holds memory accounting and every sample: 11th Gen Intel Core i9-11900H, GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`; run 2026-11-15 beside a mutation campaign, so absolute times are inflated while the ratios stand.

| 800-vertex workload | Prim sparse/dense median ms | Dense setup ms |
|---|---|---|
| Chain | 0.062 / 1.954 | 0.499 |
| Random sparse | 0.801 / 4.284 | 0.626 |
| Complete | 28.824 / 23.111 | 9.896 |
| Adversarial complete | 59.470 / 8.690 | 9.553 |
| Disconnected blocks | 0.044 / 1.828 | 0.385 |

The adversarial row is the case the dense form exists for (`01-principles.md`, Priority: faster below a measured crossover). Dense setup can outweigh its traversal saving for a single call, so the choice stays explicit: no automatic density threshold, general superiority or timing gate is claimed.

```bash
python3 '96-Local Testing/04-Graphs/00-path_mst_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 最小生成树](https://oi-wiki.org/graph/mst/) | Kruskal, Prim and Borůvka cut choices, forest behaviour, reconstruction trees and bottleneck LCA interpretation. |
| [cp-algorithms, Prim](https://cp-algorithms.com/graph/mst_prim.html) | Dense quadratic and sparse priority-queue alternatives. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit) and, for the GR02 sweep (2026-11-14), [00-sources.md](00-sources.md#fetched-2026-11-14-p010-gr02); omitted candidates and the copied-code reasons are in [00-notes.md](00-notes.md#p010-re-audit-omissions) and [00-notes.md](00-notes.md#p010-gr02-omissions).

## Limits and Handoffs

- Prim on a cost oracle (maspypy `mst_prim(N, cost)`, hitonanode `mst_edges`) and the XOR minimum spanning tree (CF888G, an `implicitBoruvka` instance) belong to row 43, `43-implicit_graph.hpp` ([00-notes.md](00-notes.md#p010-gr02-omissions); follow-up on P051).
- The Held–Karp 1-tree bound (hitonanode `minimum_one_tree`) belongs to row 73, `73-hamiltonian.hpp` ([00-notes.md](00-notes.md#p010-gr02-omissions); follow-up on P166).

## Aliases

Search aliases (P263, grab --find): MST (minimum spanning tree), minimum spanning forest; Library Checker `minimum_spanning_tree`; 最小生成树, 克鲁斯卡尔重构树, 最小瓶颈路, 克鲁斯卡尔, 普里姆, 最大生成树, 最小生成森林, 普林算法.

## History

- 2026-09-27: original P010, full suite (seed 20260927) and benchmark passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-06: inventory rewrite reopened the row as partial (maximum-weight forests).
- 2026-10-07: re-audit, implemented maximum-weight `kruskal`, `primSparse`, `primDense`, `boruvka` and `KruskalReconstruction::leafRange`; 6 findings fixed (missing maximum engines, evidence claim, brace rule, complexity line placement and coverage); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune rerun, header and tests unchanged; full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 added the search aliases; no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR02, the `maximum` flags of `kruskal`, `primSparse` and `boruvka` became the named mirrors `kruskalMaximum`, `primSparseMaximum` and `boruvkaMaximum` over shared `mst_detail` engines (flag ruling, user decision 2026-11-14); completeness sweep added nothing; full suite passed on g++ and g++-14, stress passed, extraction passed.
- 2026-11-15: P010 GR02 whole-file mutation campaign and review: seven index-bound probes, the default union-tree check and the 2^30 + 1 size probe closed ten survivors; unused `edgeLess`/`edgeOrder` defaults removed.
- 2026-11-15: /finish GR02: the header includes `02-Data Structures/01-dsu.hpp` directly (it uses only `DSU`), batch prerequisite DS01 declared; Borůvka edge order and the `primDense` view recorded as user decisions; benchmark gained an adversarial complete graph.
- 2026-11-27: P010 limits recheck (experiment none): the ownership boundaries moved from Limits and Handoffs to Contracts (Scope); the GR02 handoffs to rows 43 and 73 filed as follow-ups on P051 and P166; the review found the size-bound survivors reachable, so an accepted-bound check at n = 2^30 closed `AOS 285`–`287` and `COS 498`; header unchanged; full suite passed on g++ and g++-14, stress passed, extraction of the tester and integration passed.
