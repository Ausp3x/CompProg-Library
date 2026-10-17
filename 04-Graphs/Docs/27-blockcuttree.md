# 27-blockcuttree.hpp — evidence

Owned by package P047 / GR12; see [00-notes.md](00-notes.md#p047-package-record). Contest profile: extended block-cut and bridge forests built from the verified `lowlink` (row 09), with `EulerLCA` (row 07) for O(1) path tests, plus an incremental block forest over Data Structures 01 `DSU`. The row was a legacy reference to a non-compiling Team Notebook sketch.

## Contracts

### BlockCutTree

`BlockCutTree(n, tree)` wraps an extended block forest over `n` vertices. Node `v < n` is a vertex and node `n + b` is block `b` (`k = tree.n − n` blocks). Tree edges `(v, n + b)` weigh `|b|`. `lca` is an `EulerLCA` over `tree` with each component rooted at its minimum node, always a vertex, so every block's parent is a vertex. `size[x]` counts the vertex nodes in `x`'s subtree, and `child[start[x]..start[x+1])` lists `x`'s children by increasing `t_in`. Construction is O(n + k) time and memory.

Removal queries assert `x ≠ u, v` (or `v ≠ x`); vertex arguments are checked in `[0, n)`.

- `blockOf(v)`: `v`'s unique block, or −1 when `v` lies in two or more blocks. Loop blocks are excluded, so −1 marks exactly the articulation points. O(1).
- `sameBlock(u, v)`: true for `u = v`, or when both are children of one block, or one is the parent vertex of the other's parent block. O(1).
- `connectedAfterRemovingVertex(u, v, x)`: true when `u` and `v` are connected in `G − x`. That holds iff they share a component and `x` is not on the forest path between them, tested as `lca(u, v)` an ancestor of `x` and `x` an ancestor of `u` or `v` (inclusive Euler intervals). The vertices on every `u–v` path are exactly the interior vertex nodes of that path, since consecutive blocks on it meet only in those cut vertices. O(1).
- `componentSizeAfterRemovingVertex(v, x)`: the size of `v`'s component in `G − x`. It is the whole tree when `x` lies elsewhere, the tree minus `x`'s subtree when `x` is not an ancestor of `v`, and otherwise the subtree of `x`'s child block that contains `v`, found by binary search over `x`'s children by `t_in`. Each block minus one vertex stays connected, so these pieces are the components. O(log deg x).
- `mandatoryVertexCount(u, v)`: the number of vertices other than `u` and `v` that lie on every `u–v` path. A forest distance of `2k` passes `k` blocks and `k + 1` vertex nodes, so it returns `k − 1`; it returns 0 for `u = v` and −1 when they are disconnected. O(1).
- `simplePathVertexCount(u, v)`: the number of vertices on some simple `u–v` path. These are the vertices of the blocks on the path (OI Wiki, block forest), and consecutive blocks share one vertex, giving `Σ|B| − (k − 1)`. The weighted distance counts every path block twice, so the value is `wd/2 − dist/2 + 1`. It returns 1 for `u = v` and 0 when disconnected. O(1).

### blockCutTree

`blockCutTree(g)` takes an undirected `Graph` or `CsrGraph` (asserted by `lowlink`); loops and multiedges are allowed. Blocks are `lowlink`'s vertex-biconnected blocks (bridge dyads and 2-connected blocks, a pair joined by parallel edges included) in `lowlink` order. Isolated vertices keep `lowlink`'s singleton blocks in that order, blocks that consist of a single loop are dropped, and each loop-only vertex gets a singleton block at the end. Time and memory O(n + m).

### BridgeTree and bridgeTree

`bridgeTree(g)` (undirected, loops and multiedges allowed) builds a forest on 2-edge-connected components (`lowlink` component IDs, numbered by increasing minimum vertex). `node[v]` is `v`'s component. Tree edge `i` is the bridge `edge[i]`; bridges are in ascending ID order and keep their input weights. `tree_edge[e]` maps back, −1 off bridges. This equals `simplify(contract(g, low.component, k))`; the user kept it as a dedicated builder. `lca` roots each component at its minimum node and `size[x]` counts the vertices in `x`'s subtree. Construction is O(n + m).

- `connectedAfterRemovingEdge(u, v, e)`: if `e` is not a bridge, this is plain connectivity. Otherwise `u` and `v` are separated exactly when one of them lies in the lower endpoint's subtree. O(1).
- `componentSizeAfterRemovingEdge(v, e)`: the lower endpoint's subtree size or its complement within the tree. O(1).

### twoEdgeConnectivityAugmentation

`twoEdgeConnectivityAugmentation(g)` (undirected) returns vertex pairs whose addition makes `G` connected and bridgeless. It returns no pairs when the bridge forest has at most one node (`n ≤ 1`, or `G` is already 2-edge-connected). Otherwise it returns `⌈p/2⌉ + q` pairs, where `p` is the number of leaves (degree 1) and `q` the number of isolated nodes of the bridge forest. That is optimal: each leaf needs one new incident edge, each isolated node two, and each edge serves two (Eswaran–Tarjan 1976).

The construction first chains the components into one tree: the last leaf slot of each component connects to the first slot of the next, where an isolated node counts as two slots. It then pairs leaf `i` with leaf `i + ⌊p'/2⌋` in DFS preorder, for `i < p' − ⌊p'/2⌋`. This covers every tree edge. The leaves below an edge form a contiguous interval `[a, b]` of `1..p'−1` leaves, and no such interval is closed under the pairing, where `h = ⌊p'/2⌋`:
- `a ≥ h`: then `a − h` lies outside the interval.
- `a < h`, `p'` even: then `a + h ∈ [a, b]` puts `h` inside, and `h`'s partner `0` forces `a = 0`; every `j < h` then pulls in `j + h`, so `b = p' − 1`.
- `a < h`, `p'` odd: leaf `h` pairs with both `0` and `2h = p' − 1`, which forces the whole range. Chaining spends `c − 1` edges and removes `2(c − 1)` slots, so the total is `⌈(p + 2q)/2⌉`. Time O(n + m).

### IncrementalBlockCutTree

`IncrementalBlockCutTree(n)` maintains the extended block forest under online `addEdge(u, v)`; loops are ignored. Vertex `v`'s parent is a raw block ID resolved through the `merged` DSU, a representative block's parent is a vertex, and roots are vertices. At most `n − 1` blocks are created.

- An edge between trees (the `comp` DSU) reroots the smaller tree at its endpoint and creates block `{u, v}` with `u` below it and the block below `v`.
- An edge inside a tree walks both endpoints up alternately with stamps to the LCA. It merges every block on the path, and the LCA too if it is a block, into one block whose parent becomes the LCA's parent (block LCA) or the LCA itself (vertex LCA). Interior path vertices then hang below the merged block through their resolved parent IDs. These blocks form a cycle with the new edge and are exactly the ones that merge (incremental biconnectivity, Westbrook–Tarjan).
- `sameBlock(u, v)` uses the same parent test as the static structure.

Rerooting the smaller tree costs O(n log n) in total. A within-tree insertion that passes `k` blocks costs O(k) and removes `k − 1` of them, and only cross-tree insertions create blocks. So `addEdge` is O(log n) amortized, `sameBlock` is O(α(n)) amortized, and memory is O(n). Both methods are non-const because the DSUs compress paths.

## Legacy mapping

| Legacy (`OLD/Team Notebook/src/graph/old_block-cut-tree.cpp`, `tree`) | Replacement |
|---|---|
| `tree(_n, cutpoints, comps)` (non-compiling: `int (int u : …)`, undeclared `l`, missing `;`) | `blockCutTree(g)`; components come from `lowlink` |
| `id[u]`: compact node of `u` | Extended numbering: every vertex is node `u`; `blockOf(u)` for non-cut vertices |
| `is_cut` | `lowlink(g).is_art` (`isArticulation` left the row as a projected field) |
| `add_edge`, `adj` | `tree` (a `Graph`) |
| `l = ceil(log2(n))` (lifting depth) | `lca` (`EulerLCA`) |

Row names: `biconnectedQuery` left the row (`sameBlock`, decided 2026-10-18). `connectedAfterRemovingVertex`/`Edge` became the pair queries above.

## Aliases

- maspypy `block_cut` and hitonanode extended block-cut trees use this numbering (vertices then blocks). Nyaan's cut-vertex numbering and ei1333's blocks-first numbering are relabellings of `tree`.
- maspypy `remove_one_vertex_connectivity` and `remove_one_edge_connectivity` are the pair and size queries; their component counts are `lowlink`'s `cut_components[x]` and `roots.size() + is_bridge[e]`.
- maspypy `edge_to_block` and OGDF `bcproper` are `lowlink`'s `edge_block`; OGDF `bComponent` is `sameBlock`.
- NetworkX `k_edge_augmentation` with k = 2 is `twoEdgeConnectivityAugmentation`. OI Wiki 必经点 counts and APIO 2018 Duathlon path vertices are `mandatoryVertexCount` and `simplePathVertexCount`.

## Feature-to-test map

`96-Local Testing/04-Graphs/27-blockcuttree_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with probes, ASan/UBSan in full/stress). Oracles: BFS labels after deleting a vertex or an edge; blocks as the union classes of the edges of every simple cycle (2-cycles of parallel edges included) plus singletons; simple-path enumeration; and a direct bridgelessness check for augmentation, with exhaustive multisets for minimality.

| Operation | Test | Oracle |
|---|---|---|
| `blockCutTree`, `BlockCutTree` shape and block sets | Every simple graph n ≤ 5; 3,000 random multigraphs n ≤ 7 with loops, isolated vertices and parallel edges | Cycle-union blocks (vertex sets, tree edge weights) |
| `blockOf`, `sameBlock` | Same | Articulation by deletion; `sameBlock` iff no other vertex separates `u` and `v` (Menger) |
| `connectedAfterRemovingVertex`, `componentSizeAfterRemovingVertex`, `mandatoryVertexCount`, `simplePathVertexCount` | Same, all triples; 100,000-vertex random sparse, path and ladder-with-loop shapes with 6 sampled deleted vertices × 3,000 pairs and all sizes | Deletion BFS; simple-path enumeration; path values on the path shape |
| `bridgeTree`, `BridgeTree` fields, `connectedAfterRemovingEdge`, `componentSizeAfterRemovingEdge` | Same small cases, all edges; large shapes with 6 sampled edges × 3,000 pairs | Edge-deletion bridges and 2-edge-connected classes; deletion BFS |
| `twoEdgeConnectivityAugmentation` | Same small cases (minimality exhaustive for n ≤ 5); large shapes; cycle (none) and edgeless (n pairs) | Bridgeless after adding; count `⌈p/2⌉ + q`; no smaller multiset works; `lowlink` on the augmented large graphs |
| `IncrementalBlockCutTree`: `addEdge`, `sameBlock` | 3,000 random insertion sequences n ≤ 7, all pairs after each insertion; large shapes, 20,000 pairs | Brute `sameBlock`; the static structure on large graphs |
| Preconditions | 7 probes: directed graph (both builders), vertex out of range, removed endpoint, size query at the removed vertex, edge out of range, incremental vertex out of range | Expected assertion failure |

Planted mutants (quick suite, optimized build), all killed: an edge-removal size that ignores the side, `simplePathVertexCount` off by one, a missing parent update for merged incremental blocks, an augmentation pairing offset of `⌈p/2⌉`, a path test without the LCA condition, and `sameBlock` without the parent-vertex case.

## Commands and results

2026-10-18, GCC 16.2.1 and GCC 14.4.1 (floor), CPython 3.14.8, Linux x86-64 (Intel Core i9-11900H), GNU++20, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds.

| Command | Result |
|---|---|
| `27-blockcuttree_tester.py --mode quick` | PASS 2 configurations, 376 cases, 420,413 checks per configuration, 7 probes |
| `27-blockcuttree_tester.py --mode full --seed 1` | PASS 3 configurations, 4,100 cases, 3,760,807 checks per configuration, 7 probes; MEMORY peak 793 MB |
| `CXX=g++-14 27-blockcuttree_tester.py --mode full --seed 2` | PASS 3 configurations, 4,100 cases, 3,743,480 checks per configuration, 7 probes; MEMORY peak 810 MB |
| `27-blockcuttree_tester.py --mode stress --seed 3` | PASS 3 configurations, 16,100 cases, 12,653,109 checks per configuration, 7 probes; MEMORY peak 953 MB |
| `02-integration.py --sanitizers` | PASS: 159 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 1,829 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/27-blockcuttree_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/27-blockcuttree_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/27-blockcuttree_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: contest profile, no threshold.

## Sources

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, 圆方树](https://oi-wiki.org/graph/block-forest/) | Extended numbering, mandatory vertices and simple-path vertex sums on the block forest |
| [maspypy block_cut](https://maspypy.github.io/library/graph/block_cut.hpp), [remove_one_vertex_connectivity](https://maspypy.github.io/library/graph/ds/remove_one_vertex_connectivity.hpp), [remove_one_edge_connectivity](https://maspypy.github.io/library/graph/ds/remove_one_edge_connectivity.hpp) | Numbering, pair and size queries after one removal |
| [hitonanode extended block-cut trees](https://hitonanode.github.io/cplib-cpp/graph/extended_block_cut_trees.hpp) | Singleton blocks for isolated vertices |
| [OGDF DynamicBCTree](https://ogdf.github.io/doc/ogdf/classogdf_1_1_dynamic_b_c_tree.html) | Incremental edge insertion merging the blocks on the path |
| [NetworkX connectivity](https://networkx.org/documentation/stable/reference/algorithms/connectivity.html) | `k_edge_augmentation` |
| [Nyaan](https://nyaannyaan.github.io/library/tree/block-cut-tree.hpp), [ei1333](https://ei1333.github.io/library/graph/others/block-cut-tree.hpp), [tko919](https://tko919.github.io/library/Graph/blockcut.hpp) | Catalog comparison (see Aliases) |

Not inspected: the Eswaran–Tarjan and Westbrook–Tarjan papers. The augmentation bound and both proofs above are this package's arguments, validated by exhaustive minimality on small graphs.

## Limits and handoffs

- The incremental structure answers only `sameBlock`; its block IDs change under merges.
- Set-of-terminals queries are row 14 virtual trees over `tree`; three-edge-connectivity is row 74; cactus structure is row 41.
- Vertex-biconnectivity augmentation is omitted ([00-notes.md](00-notes.md#p047-omissions)).

## History

- 2026-10-18: verified (P047); Team Notebook reference replaced; user decisions applied (`bridgeTree` kept, `biconnectedQuery` removed, pair removal queries); size, mandatory, simple-path and augmentation queries from the completeness sweep; independent review: missing vertex checks in `connectedAfterRemovingVertex` added, singleton-block wording corrected, CSR tests added; 6 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
