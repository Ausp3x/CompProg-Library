# 14-tree_algorithms.hpp — evidence

Owned by package P042 / GR05 (tree algorithms and HLD, with `15-hld.hpp`); see the [common graph contract](00-notes.md#p010-common-contract) and [tree ownership](00-notes.md#tree-ownership).

## Contracts

### Domain

Every function takes `Graph` or `CsrGraph`. "Tree" means undirected, connected, n − 1 edges and n ≥ 1, checked by assertion at entry (`tree_detail::rooted`, a BFS from the root). With `NDEBUG`, a non-tree input with n ≥ 1 still terminates in O(n + m) with a meaningless result: every loop runs over the BFS order, which never exceeds n, `pruferEncode` keeps its leaf pointer below n, and `lca_detail::Forest` skips a vertex it has already visited. The optimized test builds never receive such inputs (`05-testing.md`), so this rests on that argument and on the 2026-11-04 audit's scratch probe of 3000 multigraphs with loops and cycles under `-DNDEBUG` and ASan/UBSan. "Forest" means an undirected simple forest, as `lca_detail::Forest` asserts (no loops, no parallel edges); `root = -1` roots every component at its minimum vertex, and an explicit root roots its own component first and the rest at their minimum. Weights span the full `lng` range, signed, and every weighted total is exact in `lll`: a path weighs less than n · 2^63 and a distance sum less than n^2 · 2^63 < 2^125.

### treeDiameter

Returns `TreeDiameter {length, vertices, arcs}`: the heaviest simple path, with `arcs[i]` running from `vertices[i]` to `vertices[i + 1]` and summing to `length`. With nonpositive edges only, the answer is the single vertex 0 with length 0. Algorithm: one bottom-up pass keeps `down[v]`, the heaviest downward path from v (at least 0, the empty path), and the arc to its best child. Each child arc offers `down[u] + down[v] + w`, which pairs the best earlier branch with this one. Every simple path has a unique top vertex where its two branches are each the best downward continuation, so the maximum over these offers is the diameter (signed weights included, since empty branches count 0). The witness follows the stored best-child arcs from the recorded top. O(n).

### treeEccentricities

Returns `TreeEccentricities {radius, ecc, center}`: `ecc[v]` is the maximum of dist(v, u) over all u including v, so it is never negative. `radius` is the minimum of `ecc`, and `center` lists every vertex that attains it, ascending. `treeRadius` and `treeCenter` are these fields (duplicates ledger keep). Algorithm: the two best downward values per vertex, then `up[v] = max(up[u], best downward value of u avoiding v) + w(u, v)`. Signed weights are exact. A center can lie inside an edge; that point is not reported (see [notes](00-notes.md#p042-gr05-omissions)). O(n).

### treeCentroid

Returns the one or two vertices v with max(largest child size, n − size(v)) ≤ n / 2, ascending. Two centroids occur exactly when an edge splits the tree into halves of n / 2. O(n).

### sumOfDistances

`res[v] = Σ_u dist(v, u)` in `lll` for a tree; an empty graph returns empty. The root's value is Σ size(v) · w(parent(v), v). Moving the root across edge (p, v) changes the sum by (n − 2 · size(v)) · w. O(n).

### EulerTour

Forest preorder in adjacency order, built on `lca_detail::Forest`'s iterative DFS. Fields `tin`, `tout`, `order`, `parent`, `dep`; v's subtree is `order[tin[v], tout[v])`, half-open. `subtreeRange(v)` returns `{tin[v], tout[v]}`. O(n).

### BfsNumbering

Fields `tin` and `tout` are the forest's DFS preorder, `tout` exclusive as in `EulerTour`. `order` is a multi-source BFS over the forest's roots, taken in root order, with children in adjacency order; `id[order[i]] = i`. BFS within depth d is sorted by DFS preorder (`tin`), because both visit children in the same order. The proof is by induction on d: siblings keep adjacency order in both, and children of earlier parents come earlier in both. Hence v's descendants at depth d are one contiguous range of `order`, found by two binary searches for `tin[v]` and `tout[v]` on `key[i] = tin[order[i]]` inside depth d's block `[start[d], start[d + 1])`.
- `levelRange(v, d)`: that range, or `{0, 0}` when it is empty, d < dep[v] or d is beyond the deepest level. O(log n).
- `diskRanges(v, r)`, r ≥ 0: the vertices within r edges of v, returned as disjoint nonempty ranges by increasing depth. Fix a depth d. If x at depth d has LCA a_j = the j-th ancestor of v, then dist(v, x) = (d − dep v + j) + j. So x is within r iff j ≤ k = min(dep v, ⌊(r − d + dep v) / 2⌋), and k ≥ dep v − d always holds for d ≥ dep v − r. The descendants of a_k at depth d contain those of every a_j with j ≤ k, and each of them is within distance d − dep v + 2k ≤ r. So depth d contributes exactly `levelRange(a_k, d)`. At most min(2r + 1, h + 1) depths exist for tree height h, so the cost is O((min(r, h) + 1) · log n), including the O(min(r, dep v)) ancestor walk.

### Rerooting

Tree-only inputs for `Rerooting`, `subtreeDp` and `subtreeMergeDp`, the commutative op of `Rerooting` and vertex-only centers in `treeEccentricities` are accepted contracts (user decision 2026-11-09).


`Rerooting<R>(g, mon)` takes a tree. R supplies `S`, `op` and `e`, a commutative monoid over sibling results, plus `addEdge(x, e)` and `addVertex(x, v)`. `addEdge(x, e)` lifts the result x of the part rooted at `e.to` across arc `e`, where `e.from` is the merging parent and `e.w`, `e.id` are available. `addVertex(x, v)` closes v over the product x of its neighbours' lifted results.
- `get(v)`: the DP of the tree rooted at v.
- `get(v, p)`, for a neighbour p: the DP of the component containing v after deleting edge (v, p), rooted at v. It is O(1) and asserts adjacency.

Construction rooting at 0:
1. `down[v]` for every subtree, bottom-up, caching `lift[v] = addEdge(down[v], arc parent → v)`.
2. Top-down, each vertex u takes the products of P and its children's lifts. P is the lift of `up[u]` across the arc u → parent; `up[c]` is the DP of the component of u after deleting (u, c), rooted at u. Then `up[c_j] = addVertex(prefix_j · suffix_{j+1}, u)` and `all[u] = addVertex(P · all lifts, u)`.

Calls to R: n `addVertex` for `down`, n − 1 `addEdge` for lifts, n − 1 for the parent contributions, then n − 1 + n `addVertex` and about 3(n − 1) `op` calls. Total O(n). Memory: four `S` arrays of size n plus O(max degree) workspace.

### subtreeDp

`subtreeDp(g, root, mon)` takes the same R interface. `res[v]` is v's subtree DP with the tree rooted at `root`. Children are folded left to right in adjacency order (`op(op(e, c1), c2)…`), so op need not be commutative. It is the single-root special case of `Rerooting` without the second pass. O(n) calls.

### subtreeMergeDp

`subtreeMergeDp(g, root, k, mon)` returns the root's vector truncated to k + 1 entries; k ≥ 0. R supplies `S`, `add`, `zero`, `mul` and `one` (a semiring), plus `closeVertex(v, dp)`. The hook receives `dp[j]`, the `add` over all ways of choosing child entries whose indices total j, of the `mul` of those entries. It rewrites dp into v's own vector. The library then truncates the vector to k + 1 entries.
- The empty product of children is `{one}`.
- Each child merge is the truncated convolution `res[x + y] = add(res[x + y], mul(a[x], b[y]))` for x + y ≤ k.

Bound: if every hook leaves at most min(size(v), k) + 1 entries, the total merge cost is O(n · (k + 1)) (OI Wiki 树上背包; the + 1 counts the n hook calls and unit merges at k = 0). Sketch: a merge of two parts with sizes a and b costs at most min(a, k) · min(b, k). Charge it to pairs of vertices, one from each part, both among the k nearest of their parts in preorder; summed over the tree this gives O(n · k). Memory O(n): merged child vectors are freed, and live vectors belong to disjoint unmerged subtrees. An empty hook output (the zero polynomial) empties its parent's product without a merge, and an overlong one is cut to k + 1 entries, so the cost stays bounded. In terms of n alone the bound is O(n · (min(n, k) + 1)).

Examples in the tester:
- Max-plus "pick j vertices": `res[j] = max(dp[j], dp[j − 1] + a_v)`.
- Connected subtrees containing the root: `res = {one} ++ dp · a_v`, shifted by one.

### VirtualTree

`VirtualTree(g, root = -1)` wraps an `EulerLCA` (public member `lca`) over a forest. `build(vs)` sorts vs by `t_in` and removes duplicates. It adds the LCA of each preorder-adjacent pair in the same component, then sorts and deduplicates again. The result is closed under LCA: in a preorder-sorted set, the LCA of any two members equals the LCA of some adjacent pair. The parent of the i-th vertex is the LCA of it and vertex i − 1; when they lie in different components, it is the root (−1). This holds because the LCA of the predecessor and the current vertex is the deepest proper ancestor of the current vertex in the closed set (KACTL CompressTree). It returns `VirtualTreeResult {vertices (preorder), parent (indices, −1 at each component root)}`. Edge lengths are `lca.dep` or `lca.weight` differences. Q: O(k log k) with O(1) LCA.

### pruferEncode, pruferDecode

- `pruferEncode(g)`: tree with n ≥ 2. It removes the smallest leaf n − 2 times and records the leaf's neighbour. The tree is rooted at n − 1, which is never removed, so the neighbour is the parent. Linear time comes from the pointer scheme: a pointer scans increasing labels for leaves. A neighbour that becomes a leaf with a smaller label than the pointer is removed next.
- `pruferDecode(code)`: values in [0, n), n = |code| + 2. It returns the unique labelled tree as a `Graph` with unit weights, edges in removal order. The last edge joins the remaining leaf and n − 1. Under `NDEBUG`, an out-of-range code value is clamped so the loops stay bounded.

Both run in O(n), and the two are mutually inverse over the n^(n − 2) labelled trees.

### Legacy mapping

- `EulerTourTree` (`OLD/[1] algorithms.cpp:473-541`, excerpt `97-Legacy/03-subtree_queries.cpp`, commented out in the source): `EulerTour` gives `t_in` → `tin`, `t_out` (inclusive) → `tout` (exclusive), and `time_to_orig` → `order`.
  - Subtree add, set, max, min and sum queries become a segment tree over `order` (`02-Data Structures/03-segmenttree.hpp`, `12-lazysegmenttree.hpp`) on `subtreeRange(v)`; `u_only` is the single slot `tin[v]`.
  - For values that follow HLD order, `TreeMonoid` `subtreeFold` and `subtreeApply` (row 15) do the same.
- `TreeEdgePainting` (`OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp`): row 15, see [15-hld.md](15-hld.md#legacy-mapping).
- `bfsNumbering` (planned name): the `BfsNumbering` struct.

## Feature-to-test map

Tester: [`14-tree_algorithms_tester.cpp`](../../96-Local%20Testing/04-Graphs/14-tree_algorithms_tester.cpp), driven by its `.py`. Every operation also runs on `CsrGraph` in part of the cases (`levelRange`, `diskRanges` and the height `Rerooting` only on `Graph`); inputs use shuffled labels, edge order and orientation and weights unit, ±5, nonnegative, nonpositive, full range, `LLONG_MIN`/`LLONG_MAX` and ±2^62.

| Operation | Oracle and cases |
|---|---|
| `treeDiameter` | All-pairs DFS distances (u = v included); the witness is checked as a path of g whose weight equals `length`, a single vertex at length 0; max-subarray on deep paths, double sweep on large nonnegative trees |
| `treeEccentricities` (`radius`, `center`) | All-pairs distances; path closed form and sampled DFS at large n |
| `treeCentroid` | Component sizes by BFS after deleting each vertex; path middle at large n |
| `sumOfDistances` | All-pairs distances; path closed form, sampled DFS; n = 0 |
| `EulerTour`, `subtreeRange` | Independent iterative preorder with the documented root order; parent walks confirm each range is exactly the subtree; forests of one to four trees, root −1 and explicit |
| `BfsNumbering`: `levelRange`, `diskRanges` | Independent BFS rooting; descendants at depth d for d from −1 past the deepest level, `INT_MAX − 1` and `INT_MAX`; BFS balls for every r up to twice the depth and `INT_MAX` (set equality, nonempty, strictly increasing depth) |
| `Rerooting`: `get(v)`, `get(v, p)` | Naive DP from every root and for every directed edge with three R: unit height (also against hop eccentricity), exact canonical text, labelled weighted orientation-sensitive hash |
| `subtreeDp` | Naive DP with an ordered (noncommutative) text concatenation and a polynomial hash |
| `subtreeMergeDp` | Subset enumeration for n ≤ 12 (max-plus "pick j", connected sets containing the root), Pascal rows mod P, closed forms; k ∈ {0, 1, 2, 3, n − 1, n, n + 4, `INT_MAX − 1`, `INT_MAX`}; empty hook outputs on random vertex sets at k ∈ {0, 1000, `INT_MAX`} (`testEmptyMerge`, audit regression) |
| `VirtualTree::build` | Naive pairwise-LCA closure and nearest ancestor in the set; duplicates, several components, empty and full sets |
| `pruferEncode`, `pruferDecode` | Naive smallest-leaf encode and decode (edges in removal order), both round trips, exhaustive codes for n ≤ 6 (quick), 7 (full), 8 (stress) with injectivity, random codes including n = 2 |
| Preconditions | 31 checked-build probes: directed, cycle, forest, empty inputs, root and vertex ranges, negative r and k, non-adjacent `get(v, p)`, Prüfer n = 1, forest and out-of-range code values, and values past each range: `subtreeRange` at n and n + 1, `levelRange` at −1 and n + 1, `diskRanges` and `get(v)` at n, `get(v, p)` with p = n + 1, and `pruferDecode` code values n and n + 1 |

## Commands and results

2026-11-05 (after the `/finish GR70` tests; the stress row is from 2026-11-03 and the integration row from 2026-11-04, before them), GCC 16.0.1 20260322 (`g++`) and GCC 14.3.0 (`g++-14`). Each run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes); full and stress add ASan+UBSan, all with `-Wall -Wextra -Wconversion -Werror`.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/04-Graphs/14-tree_algorithms_tester.py' --mode quick --seed 5` | PASS, 2 configurations, 607 cases, 6,666,691 checks, peak 563 MB |
| `… --mode full --seed 1` | PASS, 3 configurations, 35,616,569 checks, 30 probes, peak 1183 MB |
| `CXX=g++-14 … --mode full --seed 1` | PASS, same counts, peak 1030 MB |
| `… --mode stress --seed 2` | PASS, 3 configurations, peak 1177 MB |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 187 standalone and aggregate headers, multi-TU scalar and AVX2, workspace; peak 2669 MB |

2026-11-09: `python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/14-tree_algorithms.hpp` passed (23 operations, 0 failures on `g++` and `g++-14`), and `testers --filter 14-tree_algorithms` passed (1 quick run on an extracted build, 0 failures).

## Mutation testing

Record: mewt 4.0.0 on blob `42bbd8e51487d60819019e19fb329ac972d7e1b5` (2026-11-09, seed 1, 17 lines changed since blob `26214fbf238e`): 32 mutants, 23 caught (0 by timeout), 9 survived, 0 untested.

Stage seconds of that campaign: quick 99, full-optimized 220, full-all 1119.

Earlier campaign: mewt 4.0.0 on blob `26214fbf238e2dc7e0ce8b5eb709dc2261fc56b8` (2026-11-05, seed 1, whole file, uniform sample of 1099): 500 mutants, 449 caught (1 by timeout), 51 survived, 0 untested.

Stage seconds, over the campaign and its survivor retest together: quick 1025, full-optimized 1184, full-all 7995. The first pass, run 2026-11-04 to 05 by `/finish GR70` because this header had no record, left 58 survivors. Tests were added for the real gaps, and the retest with the same workdir caught 7 of them. The new tests are the range probes `subtree-range-far`, `level-range-far`, `disk-range`, `rerooting-get-range`, `rerooting-parent-far` and `prufer-value-far`.

Closed 2026-11-09 by the checked probe `centroid-empty`, verified by rebuilding the mutant: `CR 160`.

Each remaining survivor was checked for type context, reachability, downstream use, semantic comparison and a distinguishing input. Some arguments rest on user decisions of 2026-11-09 (`/merge-pr 4`): a changed choice among ties or of state no caller reads is accepted as within the contract, and guards reachable only under `NDEBUG` or beyond every memory cap are untestable and kept. Runtime-only mutants are recorded with their argument but stay open, since no ruling accepts that class: each needs a test or an equivalence argument.

| Mutant | Line | Argument |
|---|---|---|
| `COS 736`, `COS 806`, `COS 886`, `COS 956`, `COS 961`, `COS 1006` | 78, 136, 187, 225, 226, 267 | equivalent: the counter starts at 0, steps by 1 and meets its bound, which is at least 0 and fixed in the loop |
| `COS 682` `==` → `<=` | 13 | equivalent: with all n vertices reached, `edges + 1 >= n`, so `<=` holds only at equality |
| `COS 704` `>` → `>=` | 44 | equivalent under the contract (user decision 2026-11-09): ties pick another equally heavy branch, possibly through zero-weight arcs; the length is unchanged |
| `COS 708`, `COS 762`, `COS 787` | 46, 98, 132 | equivalent: the value is -1 or a nonnegative index, so the two tests agree |
| `COS 729` `>` → `>=` | 71 | equivalent: it assigns an equal value |
| `COS 742` `==` → `<=` | 78 | equivalent: `radius` is the minimum eccentricity |
| `AS 313`, `AS 320`, `AS 321`, `AS 323`, `AS 365`, `AS 377` | 92, 148, 152, 155, 251, 274 | equivalent: `min`, `max` and the called function are symmetric in value |
| `MR 1087`, `MR 1090`, `MR 1097`, `MR 1098`, `MR 1099` | 116, 117, 227, 231, 232 | equivalent: a copy instead of a move; the source is not read again |
| `LOS 647`, `COS 797`, `COS 799` | 135 | equivalent under the simple-forest contract: one arc joins u to each child, and each condition implies the other |
| `COS 836` `<` → `!=` | 148 | equivalent: `tin[v] < tout[v]` makes x <= y |
| `AOS 489` `+` → `/` | 154 | equivalent: the extra level d = start.size() - 1 makes `levelRange` return {0, 0}, which is dropped |
| `COS 876` `>=` → `!=` | 175 | equivalent under the contract (user decision 2026-11-09): only `down[root]` is skipped, which no `get` reads (`get(root)` uses `all`, and the root has no parent) |
| `AS 341` `pre[j], lift[kids[j]]` → `lift[kids[j]], pre[j]` | 187 | equivalent: `op` is commutative over sibling results by contract (user decision 2026-11-09) |
| `COS 894` `>=` → `>` | 188 | equivalent: `suf[0]` is never read |
| `AOS 556` `+` → `*` | 230 | equivalent: at size k + 1 the resize to k + 1 changes nothing |
| `CR 223` `vs.erase(unique(vs.begin(), vs.end()), vs.end());` → `/* vs.erase(unique(vs.begin(), vs.end()), vs.end()); */` | 249 | equivalent: a duplicate adds LCA(v, v) = v, removed by the second sort and unique; vs stays O(k) |
| `AOS 592`, `AOS 593`, `COS 1012`, `COS 1018`, `AOS 599`, `AOS 601`, `AOS 605`, `COS 1063`, `AOS 624`, `COS 1076`, `COS 1083` | 269, 273, 274, 284, 289 | equivalent: the remaining tree always has two leaves above `ptr`, at most one of them n - 1, so the n - 1 bounds never bind; degrees stay at least 1 |
| `CR 238`, `COS 1047` | 279 | untestable (user decision 2026-11-09): only codes of at least INT_MAX - 2 values, 8 GiB, reach it; the assert stays |
| `CR 135`, `IF 258`, `COS 708`, `COS 709`, `CR 161`, `IF 264`, `COS 773`, `COS 774` | 34, 85 | untestable (user decision 2026-11-09): these `n = 0` guards run only under `NDEBUG`, outside the `n >= 1` contract |
| `AOS 615`, `AOS 616`, `AOS 619`, `AOS 621` | 281, 287 | untestable (user decision 2026-11-09): the clamp changes only out-of-range values, which the checked build rejects and NDEBUG tests never give; the guard stays |

## Sources

See [the 2026-11-03 ledger](00-sources.md#pages-fetched-on-2026-11-03-p042-gr05): maspypy (rerooting, BFS numbering, compress tree, diameter, centroid), Nyaan and hitonanode (rerooting, auxiliary tree), ei1333 (weighted diameter), KACTL (CompressTree), OI Wiki (tree knapsack bound). No catalog code was copied.

## Limits and handoffs

- `Rerooting` and `subtreeMergeDp` take trees only; for a forest, run them per component (`connectedComponents` and `inducedSubgraph`).
- Invertible-merge rerooting is a recorded omission ([notes](00-notes.md#p042-gr05-omissions)).
- `meet(a, b, c)` is a follow-up for row 07.

## History

- 2026-11-05: `/finish GR70` mutation record (whole file, 500 of 1099 mutants): six range probes closed 7 survivors; 51 recorded with arguments.
- 2026-11-04: `/finish GR05` audit: `subtreeMergeDp` empty hook output, `pruferEncode` pointer bound under `NDEBUG`, dead `diskRanges` branch, complexity lines and evidence wording fixed; full passed on g++ and g++-14.
- 2026-11-03: P042 GR05 first implementation (planned row); full suite passed on g++ and g++-14, stress seed 2; testing fixed two `INT_MAX` overflows (`levelRange`, `subtreeMergeDp`), review fixed loops bounded by the BFS order under `NDEBUG`, an exclusive `BfsNumbering::tout` and the `closeVertex` hook name; the row 07 `Forest` now skips revisits under `NDEBUG` (user decision 2026-11-09).
- 2026-11-08: `/merge-pr 4` rebase on master: row set to `partial`, since the P254 omission sweep of 2026-11-08 added operations after this work (`missing:` in the inventory row); no code or test change.
- 2026-11-08: `/merge-pr 4` review fixes: `G = Graph` defaults, `n = 0` guards in `treeDiameter` and `treeCentroid`, `rootTree` and `pathVertices` back as planned.
