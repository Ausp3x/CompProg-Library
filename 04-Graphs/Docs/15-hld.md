# 15-hld.hpp — evidence

Owned by package P042 / GR05 (tree algorithms and HLD, with `14-tree_algorithms.hpp`); see the [common graph contract](00-notes.md#p010-common-contract) and [tree ownership](00-notes.md#tree-ownership).

## Contracts

### HLD

`HLD(g, root = -1)` takes `Graph` or `CsrGraph`: an undirected simple forest. One assertion after the build checks m + components = n, which rejects loops, parallel edges and cycles; under `NDEBUG` the build skips an arc to a visited vertex and stays O(n + m). `root = -1` roots every component at its minimum vertex; an explicit root roots its own component first and the rest at their minimum.

The build is iterative and recursion-free:
- a BFS gives `parent`, `parent_edge`, `component` (the component's root vertex), `dep` and `edge_vertex[id]` (the child endpoint of tree edge id);
- sizes come bottom-up, and each vertex's heavy child is its first child of maximum size in adjacency order;
- an explicit-stack preorder pushes the light children before the heavy child, so the heavy child gets the next `tin` and each heavy path is a contiguous block headed by `head`;
- a preorder taken from a stack keeps every subtree contiguous, so v's subtree is `[tin[v], tout[v])`, `order[tin[v]] = v`, and each component is the block of its root.

Every light edge at least halves the subtree size, so a root path crosses at most ⌊log2 n⌋ light edges and at most ⌊log2 n⌋ + 1 heavy paths.

- `getLCA(u, v)`, `distance(u, v)` (edge count) and `kthAncestor(u, k)` (k a `lng`, as in row 07) run in O(log n) and return −1 across components or when k > dep[u]. These are the O(n)-memory HLD algorithm beside row 07's binary lifting and Euler RMQ (a different algorithm; same names as row 07).
- `edgeVertex(id)` returns the child endpoint of tree edge id.
- `processPath(u, v, edges, f)` makes O(log n) calls f(l, r, up). Read in call order, `order[l..r)` (taken from r − 1 down to l when `up`) spells the path u → v. With `edges` the LCA is omitted, so each slot stands for the edge to its parent. It returns false, making no calls, across components. Ranges on u's side are emitted while climbing; v's side is buffered in a 64-entry array, which holds the ≤ 32 heavy paths of n < 2^31, and emitted in reverse.
- `subtreeRange(v, edges)` = `{tin[v] + edges, tout[v]}`.
- `rerootedSubtreeIntervals(v, r, edges)` asserts v and r share a component and returns the nonempty ranges of v's subtree under root r:
  - the whole component when v = r;
  - when r lies strictly inside v's subtree, the component minus the subtree of x, the child of v toward r (`kthAncestor(r, dep r − dep v − 1)`);
  - otherwise v's own subtree.

  `edges` drops the component root's slot or v's own slot.
- `reorderArray(a)` gives `res[tin[v]] = a[v]`, the base array for a caller's own segment tree (duplicates ledger keep).

### TreeMonoid

`TreeMonoid<A, COMMUTATIVE = false>(g, values, edges = false, root = -1, mon = {})` derives from `HLD`.
- **Values:** `values` holds one value per vertex, or one per edge id when `edges` (stored at the child slot; component roots hold `e()`).
- **Monoid:** A is a monoid or an acted monoid from row 00 of `02-Data Structures`. The engine is a `SegmentTree`, or a `LazySegmentTree` when A has `F`.
- **Noncommutative storage:** when `COMMUTATIVE` is false, each node stores `Reversible<A>`, the forward and backward product (twice the `op` calls). An upward range contributes its backward product, so `pathFold` is the exact ordered product for any associative A. An action reaches both components through `Reversible::mapping`, which requires the action to distribute over `op` in either order, as every acted preset does. With `COMMUTATIVE = true` the engine stores A directly, and the result is correct only for commutative A.
- **get, set:** `get(i)` and `set(i, x)` address a vertex, or an edge id when `edges`.
- **pathFold:** `pathFold(u, v)` returns `{true, product along u → v}` or `{false, e()}` across components.
- **pathApply:** `pathApply(u, v, f)` (acted A only) applies f to every slot on the path and returns false across components.
- **subtree operations:** `subtreeFold(v)` and `subtreeApply(v, f)` cover v's subtree, without v's own edge slot when `edges`. The fold multiplies in `tin` order, a heavy-first preorder, so it is meaningful for commutative A (accepted contract, contributor proposal 2026-11-04).
- **maxPath:** `maxPath(u, v, pred)` requires `pred(e())` and pred monotone along the path, true then false. It returns the last vertex x on u → v with `pred(pathFold(u, x))`:
  - It returns −1 across components, or in vertex mode when u alone fails; in edge mode x = u always qualifies.
  - Each range of `processPath` is first tested whole with one `seg.prod`. The first range that fails is searched by binary search on its accepted length, with `seg.prod` of the prefix (downward) or suffix (upward) inside the range, and the walk stops there. Searching the whole array with `minLeft`/`maxRight` would be wrong: pred is monotone only along the path, and the tree's nodes mix in off-path slots.
  - An accepted upward slot y reaches y in vertex mode and `parent[y]` in edge mode; a downward slot reaches y in both modes.
  - Cost: O(log^2 n).
- **Engine access:** the public `seg` exposes the engine: `seg.prod`, `seg.allProd` and the complement ranges of `rerootedSubtreeIntervals` cover outtree and rerooted subtree queries.

Costs: set, get, subtreeFold and subtreeApply are O(log n); pathFold and pathApply O(log^2 n); maxPath O(log^2 n). Memory O(n).

### Legacy mapping

- `HLD(N)` + `addEdge(u, v)` + `build(root)` (`OLD/algorithms.cpp:4118-4230`, `OLD/Team Notebook/src/graph/hld.cpp`, `algs.cpp`, `algsbetter.cpp`): `HLD(g, root)` on a `Graph`, which also accepts forests. The legacy recursive DFS overflowed the stack on deep paths; the build is now iterative.
- Renamed operations:
  - `getDis` → `distance`;
  - `kthAnc` → `kthAncestor`;
  - `processSubt(u, is_edge, f)` → `subtreeRange(u, edges)`, half-open; the legacy form was inclusive.
- `processPath(u, v, is_edge, f)` with inclusive f(l, r) in arbitrary direction → `processPath(u, v, edges, f)` with half-open ranges in path order and an `up` flag. It returns false across components.
- `in`, `out`, `rev`, `par` → `tin`, `tout` (exclusive), `order`, `parent`.
- `reorderArray`: kept.
- `TreeEdgePainting` (`algs.cpp`, `algsbetter.cpp`; planned as `treePainting`, until then): `TreeMonoid<Sum<int>, true> t(g, vector<int>(m), true)`.
  - Map the legacy edge ids onto `Graph` edge ids.
  - `paintEdge(id, paint)` becomes `t.set(id, paint)`.
  - `queryAncPath(anc, dec, …)` becomes `t.pathFold(dec, anc).second`.

  The tester's `painting` case reproduces it against a naive array.

## Feature-to-test map

Tester: [`15-hld_tester.cpp`](../../96-Local%20Testing/04-Graphs/15-hld_tester.cpp), driven by its `.py`. The oracle is an independent DFS rooting (parent, edge, component, depth, size) with explicit parent walks for paths; forests of one to six trees with isolated vertices, root −1 and explicit, `Graph` and `CsrGraph`, all pairs for n ≤ 12.

| Operation | Oracle and cases |
|---|---|
| `HLD` construction | Every field against the oracle; `order[tin[v]] = v`, `tout = tin + size`, nesting, the heavy child is exactly the first child of maximum size in adjacency order, head chains, contiguous heavy paths, `edge_vertex`; an isolated forest of 10^6 vertices (component, head, `tin`, `tout`, parent) |
| `getLCA`, `distance`, `kthAncestor`, `edgeVertex` | Parent-walk LCA; k = dep + 1, `INT_MAX` and `INT64_MAX`; pairs across components |
| `processPath` | Both modes: concatenated ranges equal the brute path, every call nonempty and in range, at most 2 · bit_width(n) + 2 calls, false with no calls across components |
| `subtreeRange`, `reorderArray` | Direct checks against the oracle |
| `rerootedSubtreeIntervals` | BFS reroot; ranges nonempty, disjoint and covering exactly the subtree, both modes |
| `TreeMonoid`: `get`, `set`, `pathFold`, `subtreeFold`, `pathApply`, `subtreeApply` | Naive arrays with ordered path walks under random update/query mixes, vertex and edge mode: `AffineComposite<mint>` and a stateful string-hash `Monoid` (noncommutative, both directions), `Sum`/`Min`/`Max` with COMMUTATIVE true and false, `RangeAssignRangeComposite<mint>`, `RangeAddRangeSum` (both), `RangeAffineRangeSum<mint>`; tin-order brute for `subtreeFold` |
| `maxPath` | Prefix thresholds on nonnegative and signed `Sum`, `Min`, `Max` and `MaxGain` (noncommutative), both modes, single and isolated vertices; the oracle checks only paths whose prefix predicate is monotone (the contract's domain); fixed regressions `maxPathRegressions` for a search leaving the path range (path 0-1-2-3 with `{-100, 0, 0, 5}` upward, `{0, 5, -100, 0}` downward, edge mode `{-100, 0, 5}`, and n = 2 without edges) |
| `treePainting` (removed) | `treeEdgePainting`: edge-mode `Sum<int>` paint/unpaint by id and counts from a vertex up to an ancestor, against a naive array |
| Preconditions | 22 checked-build probes: directed, self-loop, cycle, multi-edge, root and vertex ranges, negative k, edge id, path vertex, `rerootedSubtreeIntervals` across components, values size, and out-of-range ids for the root, either `getLCA` vertex, `edgeVertex`, the second `pathFold` end, `subtreeRange` and both `rerootedSubtreeIntervals` arguments, plus `reorderArray` arrays shorter and longer than n and `TreeMonoid` values shorter than n |

Large shapes: deep and shuffled paths, star, caterpillar, binary, random and a 40-tree broom forest at 2·10^4 (quick), 2·10^5 (full) and 10^6 (stress) vertices.

## Commands and results

2026-11-05 (after the `/finish GR70` tests; the stress row is from 2026-11-03 and the integration row from 2026-11-04, before them), GCC 16.0.1 20260322 (`g++`) and GCC 14.3.0 (`g++-14`). Each run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes); full and stress add ASan+UBSan, all with `-Wall -Wextra -Wconversion -Werror`.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/04-Graphs/15-hld_tester.py' --mode quick --seed 4` | PASS, 2 configurations, 4691 cases, 2,257,341 checks, peak 713 MB |
| `… --mode full --seed 1` | PASS, 3 configurations, 23,150,548 checks, 22 probes, peak 939 MB |
| `CXX=g++-14 … --mode full --seed 1` | PASS, same counts, peak 943 MB |
| `… --mode stress --seed 2` | PASS, 3 configurations, peak 956 MB |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 187 standalone and aggregate headers, multi-TU scalar and AVX2, workspace; peak 2669 MB |

2026-11-09: `python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/15-hld.hpp` passed (19 operations, 0 failures on `g++` and `g++-14`), and `testers --filter 15-hld` passed (1 quick run on an extracted build, 0 failures).

## Mutation testing

Record: mewt 4.0.0 on blob `d96064c0e508cbf107ebdc00644b591840ff7db5` (2026-11-09, seed 1, 2 lines changed since blob `cdf7102e7f43`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

Earlier campaign: mewt 4.0.0 on blob `cdf7102e7f43e4a6b5241aee22befcd7e19c3446` (2026-11-05, seed 1, whole file, uniform sample of 669): 500 mutants, 474 caught (14 by timeout), 26 survived, 0 untested.

Stage seconds, over the campaign and its survivor retest together: quick 2237, full-optimized 490, full-all 3287. The first pass, run 2026-11-04 to 05 by `/finish GR70` because this header had no record, left 39 survivors. Tests were added for the real gaps, and the retest with the same workdir caught 13 of them. The new tests are an isolated forest of 10^6 vertices in the large tests (component, head, `tin`, `tout`, parent) and 11 range probes: `root-far`, `vertex-far`, `vertex-first`, `edge-far`, `path-second`, `subtree-range`, `reroot-vertex`, `reroot-target`, `reorder-short`, `reorder-long` and `values-short`.

Each remaining survivor was checked for type context, reachability, downstream use, semantic comparison and a distinguishing input. Some arguments rest on user decisions of 2026-11-09 (`/merge-pr 4`): a changed choice among ties or of state no caller reads is accepted as within the contract, and guards reachable only under `NDEBUG` or beyond every memory cap are untestable and kept. Runtime-only mutants are recorded with their argument but stay open, since no ruling accepts that class: each needs a test or an equivalence argument.

| Mutant | Line | Argument |
|---|---|---|
| `COS 435`, `COS 450`, `COS 635`, `COS 645` | 21, 29, 96, 133 | equivalent: the counter starts at 0, steps by 1 and meets its bound, which is at least 0 and fixed in the loop |
| `COS 442`, `COS 447`, `COS 471`, `COS 476`, `COS 487`, `COS 512`, `COS 556` | 25, 28, 33, 35, 41, 45, 57 | equivalent: the value is -1 or a nonnegative index, so the two tests agree |
| `COS 461` `==` → `<=` | 30 | equivalent: a spanning forest gives edges + roots >= n, so `<=` holds only at equality |
| `COS 468` `>=` → `>` | 31 | equivalent: `bfs[0]` is a root, skipped anyway |
| `COS 496`, `COS 506`, `COS 508` | 44 | equivalent under the simple-forest contract (asserted at line 30): one edge joins u to each child, so the two tests agree |
| `AS 207`, `COS 546` | 53 | equivalent: on equal head depths neither head lies on the LCA's chain, so either may climb; `swap` is symmetric |
| `COS 593` `>=` → `>` | 72 | equivalent: on equal head depths v climbing first only stores its segment in `down`; f still gets the same sequence |
| `COS 610` `<` → `!=` | 84 | equivalent: every call has x <= y |
| `COS 621` `<` → `<=` | 86 | equivalent: `v == r` is handled first, so `tin[v] != tin[r]` |
| `COS 643` `==` → `>=` | 131 | equivalent: an extra value reaches `slot`, whose range assertion aborts the same way |
| `AS 223` `0, edgeVertex(i)` → `edgeVertex(i), 0` | 129 | equivalent: `min`, `max` and the called function are symmetric in value |
| `AS 225` `u, v` → `v, u` | 139 | equivalent: the same path slots get the action, and actions on disjoint ranges commute |
| `COS 650` `>` → `!=` | 160 | equivalent: hi - lo starts at r - l >= 1 and bisection keeps it at least 1 |
| `AS 250` `lo, hi` → `hi, lo` | 161 | equivalent: with hi - lo >= 2 both roundings give lo < c < hi, so the bisection finds the same last c |

## Sources

See [the 2026-11-03 ledger](00-sources.md#pages-fetched-on-2026-11-03-p042-gr05): maspypy (`tree_monoid`, `lazy_tree_monoid`, `max_path`, path decomposition), Nyaan, ei1333, suisen and hitonanode HLD, cp-algorithms (HLD, tree painting). No catalog code was copied.

## Limits and handoffs

- `subtreeFold` with a noncommutative A folds in heavy-first preorder, not in any order the caller chooses.
- HLD `jump` and `meet` stay in row 07 (`LCA::jump`); `meet` is a follow-up there.
- Point update tree DP and path composite under link and cut belong to rows 84 and Data Structures 37–39.

## History

- 2026-11-05: `/finish GR70` mutation record (whole file, 500 of 669 mutants): an isolated 10^6-vertex forest and 11 range probes closed 13 survivors; 26 recorded with arguments.
- 2026-11-04: `/finish GR05` audit: single build assertion, `kthAncestor` takes `lng` (ratified by the maintainer at `/merge-pr 4`, 2026-11-08), heavy-child tie tested; full passed on g++ and g++-14.
- 2026-11-03: P042 GR05 rewrite of the pinned legacy excerpt; full suite passed on g++ and g++-14, stress seed 2; testing fixed an unclamped `maxPath` search, review fixed `maxPath` (search confined to the path's ranges), the heavy-child tie rule, the `TreeMonoid` argument order and the `reorderArray` complexity line.
- 2026-11-08: `/merge-pr 4` rebase on master: row set to `partial`, since the P254 omission sweep of 2026-11-08 added operations after this work (`missing:` in the inventory row); no code or test change.
- 2026-11-08: `/merge-pr 4` review fixes: `G = Graph` defaults, `treePainting` back as planned.
