# 06-mst.hpp — evidence

Owned by package P010 / GR02 (graph foundations, with `01-graph.hpp` through `05-shortest_path.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### Weight domain

All engines use exact signed-integer weights from the full `lng` range and `lll` accumulated costs. Status fields distinguish absent and present answers without reserving a numeric weight. Fractional/custom-weight instantiations of archived notebook templates remain outside this specified integer API.

### SpanningForest, kruskal, primSparse, primDense and boruvka

All require an undirected graph, ignore loops and accept parallel and negative edges. `maximum = true` (or a maximum `DenseGraph` view for `primDense`) builds a maximum-weight spanning forest; the comparator reverses the weight order instead of negating weights, since `-w` overflows for `lng` minimum. `SpanningForest` holds the exact `lll` weight, the component count including isolated vertices, and the original logical edge IDs; an empty graph has zero components, weight zero and no edges. Ties compare weights, then edge IDs. Returned edge order follows each algorithm. `primSparse` keys its set by `~w` for maximum, which is an order-reversing bijection on `lng`.

Correctness and cost: a forest contains exactly n−components edges, so its full-range signed sum fits comfortably in `lll`. Kruskal adds sorted edges only across distinct components. Prim repeatedly chooses the minimum crossing edge; its sparse set keeps one candidate per unused vertex, and its dense form scans the already-built minimum-edge matrix. Both restart at isolated/disconnected components. Borůvka caches phase-start representatives and chooses each active component's minimum outgoing edge; union guards suppress duplicates/cycles. The cut property justifies every accepted edge. Active components halve per phase until each original connected component is merged. Bounds are O(n+m log(m+1)) for Kruskal, O((n+m) log(n+1)) for sparse Prim/Borůvka, and O(n²) for dense Prim with O(n²+m) dense-view setup charged separately.

Maximum-weight forests reuse the same engines with the comparator reversed on weight and edge-ID ties kept ascending. Negating weights is avoided because `-w` overflows at the `lng` minimum; `primSparse` keys its set by `~w = -w - 1`, a bijection that reverses the order on all of `lng`. The cut property holds for maximum forests with the inequalities reversed.

### BottleneckResult and KruskalReconstruction

`BottleneckResult` distinguishes a disconnected pair, a valid empty self-path, and a nonempty path whose `value` and `edge` are its bottleneck weight and merge edge. `KruskalReconstruction(g, maximum)` requires an undirected graph with `n <= (INT_MAX + 1) / 2`. Leaves `[0, n)` are the original vertices; each merge node has two children, its joining edge ID and weight. `parent` is `-1` at roots, `root[u]` is the tree root, `leaf_count[u]` counts original leaves, and leaf `value` 0 is unused. Ascending construction answers minimax paths, `maximum = true` maximin. `lca(u, v)` accepts all nodes and returns `-1` across trees. `bottleneck` and `componentAt` accept original vertices. `componentAt(u, t)` is the highest ancestor whose merges all have weight `<= t` (`>= t` for maximum), so its leaves form u's component. `leaves` lists the original vertices so that every node's leaves are contiguous, and `leafRange(node)` returns that half-open interval `[start[node], start[node] + leaf_count[node])` into `leaves`. Construction workspace is O(n + m).

Correctness: `KruskalReconstruction` stores a binary union forest. Leaves are original vertices, each internal node records the accepted merge edge/weight, and parents have larger IDs than children. Descending node IDs build depth/root/binary-lifting data without recursion. The LCA merge weight is the minimum possible maximum edge on a path; descending construction (`maximum=true`) gives the maximum possible minimum edge. `bottleneck` explicitly distinguishes disconnected pairs, a valid empty self-path and a nonempty path; the latter also returns the merge edge ID. `componentAt` climbs every threshold-eligible ancestor, including equal-weight merges, and `leaf_count` gives the component size. Increasing/decreasing merge weights make binary lifting valid. The same descending pass assigns `start`: a root starts after the previous roots' leaves, a first child at its parent's start, a second child after the first child's leaves. Every subtree is therefore one contiguous interval of `leaves`, which `leafRange` returns, in O(n) extra time and memory. Construction requires n≤(INT_MAX+1)/2 so at most 2n−1 nodes fit `int`; preprocessing/storage is O(n log(n+1)) beyond edge sorting, with O(log(n+1)) queries.

## Feature-to-test map

`96-Local Testing/04-Graphs/06-mst_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. The full corpus is 1,099 exhaustive weighted graphs and 400 random multigraphs; quick uses n≤3/40 random cases/1,000-vertex chains, and stress 2,500 random cases/300,000-vertex chains. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `SpanningForest`, `kruskal`, `primSparse`, `primDense`, `boruvka` (minimum) | Graph and CSR, minimum `DenseGraph` view; explicit tie IDs; full-range weights/totals; copy/move/snapshots; 100,000-vertex chains | Independent forest-subset minimum |
| `kruskal`, `primSparse`, `primDense`, `boruvka` (maximum) | Graph and CSR, maximum `DenseGraph` view; explicit tie IDs for both orders; a triangle chain with closed-form minimum and maximum totals | Independent forest-subset maximum (`oracle.maximum`) |
| `BottleneckResult`, `KruskalReconstruction: bottleneck` (minimax/maximin) | Merge-edge certificates | Floyd minimax/maximin closures |
| `KruskalReconstruction: componentAt` (threshold) | Every threshold | Floyd threshold closures |
| `KruskalReconstruction: lca` | Every node | Ancestor sets |
| `KruskalReconstruction: leafRange` | Every node | Explicit subtree leaf sets |
| Preconditions | 14 assertion probes, including `leaf-range-negative` and `leaf-range-end` | Expected assertion failure |

Mutation checks (planted in a temporary copy, full mode) killed: `-w` instead of `~w` keys, Kruskal/primDense/Borůvka ignoring `maximum`, and a wrong second-child leaf offset.

## Commands and results

2026-10-07, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261007, g++ | PASS, all three builds, 872,516 checks per build, 14 probes |
| Full, seed 20261007, `CXX=g++-14` | PASS, all three builds, same counts |
| Full, seed 20260927 | PASS, 869,242 checks per build, 14 probes |
| Stress, seed 7 | PASS, all three builds, 2,052,048 checks per build |
| Quick runner, all eleven graph entries, from `/tmp` | PASS |
| `02-integration.py --sanitizers` (102 headers, scalar and AVX2 multi-TU linkage, workspace, sanitizer self-tests) | PASS |
| `03-consistency.py`, with `--braces` on the package | No errors |

```bash
python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode full --seed 20261007
CXX=g++-14 python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/04-Graphs/06-mst_tester.py' --mode stress --seed 7
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [driver](<../../96-Local Testing/04-Graphs/00-path_mst_benchmark.py>) and `96-Local Testing/04-Graphs/00-path_mst_benchmark.json` compare sparse/dense Prim (and Dijkstra, recorded in `05-shortest_path.md`) on chains, random sparse multigraphs, complete graphs and disconnected blocks of at most eight vertices. Sizes are 24, 200 and 800; weights are uniformly drawn from [0,10^6], with seed 20260927. Five samples follow checked warmups, with repeated runs for smaller cases. All forest totals/component counts are compared, with Kruskal providing the forest baseline. Allocation and result checking are included; input construction is excluded and dense-view setup is measured separately.

| 800-vertex workload | Prim sparse/dense median ms | Dense setup ms |
|---|---|---|
| Chain | 0.021 / 1.216 | 0.216 |
| Random sparse | 0.464 / 2.660 | 0.319 |
| Complete | 9.410 / 8.044 | 4.537 |
| Disconnected blocks | 0.018 / 1.053 | 0.174 |

These shared-host observations support keeping workload-specific choices explicit. Dense setup can outweigh its traversal saving for a single call. No automatic density threshold, universal superiority or timing gate is claimed. The record includes compiler/CPU/flags, memory accounting and every sample.

```bash
python3 '96-Local Testing/04-Graphs/00-path_mst_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 最小生成树](https://oi-wiki.org/graph/mst/) | Kruskal/Prim/Borůvka cut choices, forest behavior, reconstruction trees and bottleneck LCA interpretation. |
| [cp-algorithms, Prim](https://cp-algorithms.com/graph/mst_prim.html) | Dense quadratic and sparse priority-queue alternatives. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions). Legacy: the old notebook's Kruskal forest behavior and dense Prim are covered by the spanning-forest family without adopting its disconnected `-1` sentinel or empty-graph bug.

## Limits and handoffs

Fractional/custom-weight domains are not claimed. Directed MSTs, sensitivity/second-best MSTs, spanning-tree counting and fully dynamic MSTs belong to their separate inventory rows. Reconstruction-tree bottleneck queries belong here; general LCA and dynamic-tree APIs retain their separate owners. GCC 14.2 itself (the judge floor) was not run; 14.4.1 is the closest available.

## History

- 2026-09-27: original P010, full suite (seed 20260927) and benchmark passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-06: inventory rewrite reopened the row as partial (maximum-weight forests).
- 2026-10-07: re-audit, implemented maximum-weight `kruskal`, `primSparse`, `primDense`, `boruvka` and `KruskalReconstruction::leafRange`; 6 findings fixed (missing maximum engines, evidence claim, brace rule, complexity line placement and coverage); full suite passed on g++ and g++-14, stress passed.
