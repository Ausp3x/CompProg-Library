# Static forest LCA verification

P011 / GR03, 2026-09-27. Owned implementation: [07-lca.hpp](07-lca.hpp).

## Contracts

### lca_detail::Forest, LCA and EulerLCA

`LCA(g, root=-1)`, `EulerLCA(g, root=-1)` and `offlineLCA(g, queries, root=-1)` accept the canonical `Graph` or `CsrGraph`. Input must be an undirected forest: loops, parallel edges and cycles violate preconditions. All vertices, including isolated vertices, participate. Explicit roots orient their own component first; remaining components use their smallest vertex as root. The default roots every component at its smallest vertex. Empty forests are valid with `root=-1` and no queries.

Preprocessing owns its state and remains valid after the input graph changes or dies. Rebuild through construction/assignment; ordinary copies and moves are independent values. Exposed metadata and implementation helpers must not be mutated. Roots have `parent=-1`; `component[u]` is the component root; `parent_edge[u]` retains the original graph's logical edge identity. `dep` counts edges. `t_in`/`t_out` retain the historical inclusive DFS subtree interval convention. Canonical vertex labels are `[0,n)`.

| Operation | Result / domain | Binary LCA | Euler LCA |
|---|---|---|---|
| `getLCA(u,v)` | Vertex ID, or `-1` for different components | O(log(n)) | O(1) |
| `isAncestor(u,v)` | Reflexive ancestor relation; false across components | O(1) | O(1) |
| `distance(u,v)` | Number of edges, or `-1` across components | O(log(n)) | O(1) |
| `weightedDistance(u,v,out)` | Connected status; exact signed `lll` sum written only if connected | O(log(n)) | O(1) |
| `rerootedLCA(u,v,r)` | LCA when rooted at `r`; `-1` unless all three are connected | O(log(n)) | O(1) |
| `kthAncestor(u,k)` | `k >= 0`; ID or `-1` above the root; signed 64-bit `k` accepted | O(log(n)) | Use binary variant |
| `getKthAncestor(u,k)` | Legacy signed-int wrapper; `k <= 0` returns `u` | O(log(n)) | Use binary variant |
| `jump(u,v,k)` | Vertex at zero-based edge offset `k >= 0` along `u` to `v`; `-1` if disconnected or beyond endpoint | O(log(n)) | Use binary variant |

Self paths are legitimate zero-edge, zero-weight paths: LCA and zero-offset jump return the vertex. Graph weights span all `lng` values, including negatives. Distances are sums along the unique simple forest path, with no optimization over walks. At most `INT_MAX-1` edges of signed 64-bit magnitude fit in `lll`, including the temporary expressions in weighted queries.

Binary lifting takes O(n log(n)) preprocessing/storage. Its vertex-major `up[u][j]` layout and familiar `LCA` public method names remain compatible. Root jump pointers saturate at the root internally, while `kthAncestor` rejects jumps beyond the root with `-1`.

Euler LCA implements Farach-Colton–Bender ±1 RMQ in O(n) preprocessing/storage and O(1) query time. Its tour includes a virtual super-root, represented internally by `-1`, to preserve ±1 adjacent depths between disconnected components. Same-component queries cannot return this virtual root. Its signed-int Euler positions require `2*n+1 <= INT_MAX`; the binary/offline variants retain the Graph vertex-count domain. Component checks distinguish no answer before the RMQ.

`lca_detail::Forest` is the shared iterative DFS: one global preorder timer runs across all components, so the inclusive intervals `[t_in, t_out]` of different components are disjoint and `covers(u, v)` (the unchecked interval test used inside `getLCA`'s lifting loop) is exactly the ancestor relation without a component comparison. `isAncestor` keeps the vertex-range assertions; `getLCA` validates its two arguments once at entry. `Forest` costs O(n) time and memory; `isAncestor` is O(1).

`lca_detail::fromAdjacency(n, adj)` builds the legacy constructor's graph in O(n * log(n)) time (sorting and binary-searching the at most 2 * (n - 1) arcs of a valid forest) and O(n) memory. `n` is nonnegative, `adj` has `n` or `n + 1` lists, each listed neighbor is in range, no loops or duplicates, and the lists are symmetric; all of these are asserted. The `n + 1` form keeps one-based labels, with vertex 0 an isolated ordinary vertex.

### offlineLCA

Offline Tarjan returns one answer per input query, retaining order and duplicates. The query count fits `int`. Empty batches, self pairs and disconnected pairs are valid. It uses the canonical Data Structures `DSU`, with O((n+q) alpha(n)) amortized time, O(n+q) auxiliary storage and O(q) output. All forest traversals and union-find searches are iterative.

### LCAFold

`LCAFold<T, Op>(g, values, identity, op, root = -1)` is an `LCA` (same forest, root and query contracts, all `LCA` methods available) plus two vertex-major lifting tables: `rise[u][j]` folds the `2^j` vertices from `u` upward in walk order, `fall[u][j]` folds the same vertices in the reverse (downward) order. `op` is associative with two-sided `identity`; commutativity is not needed. `values.size() == n` is asserted. `pathFold(u, v)` returns `{true, fold of the vertex labels on the path u -> v in order, both endpoints included}`; `pathFold(u, v, true)` folds edge labels instead, where the caller stores each edge's label at its child vertex under the chosen rooting and the LCA's label is skipped. A self path folds `values[u]` (vertex mode) or nothing (edge mode). Disconnected pairs return `{false, identity}`. Setup O(n * log(n)) operations and storage; query O(log(n)) operations. Table entries above a root repeat the root's saturated jump and are never read by a query, which only climbs at most the depth difference. Caller-owned arithmetic must not overflow for any prefix the tables combine.

### pathIntersection

`pathIntersection(t, a, b, c, d)` accepts an `LCA`, `LCAFold` or `EulerLCA` and returns the endpoints `{x, y}` of the vertex intersection of the paths a–b and c–d, with `x` the endpoint nearest `c` and `y` nearest `d` (`x == y` for a single shared vertex), or `{-1, -1}` when the paths do not meet or any two of the four vertices lie in different components. All four vertices are asserted in range. With `meet(p, q, r) = lca(p, q) ^ lca(p, r) ^ lca(q, r)` (the median vertex of three: two of the three LCAs coincide), `x = meet(a, b, c)` and `y = meet(a, b, d)` are the projections of `c` and `d` onto path a–b. If they differ, the path c–d runs through both, so the intersection is the path x–y. If they coincide, the intersection is `{x}` exactly when `x` lies on path c–d, that is `meet(c, d, x) == x`, and empty otherwise. Cost: at most nine LCA queries, so O(log(n)) with `LCA` and O(1) with `EulerLCA`.

## Correctness arguments

The shared DFS skips exactly the parent logical edge. Every other edge must discover a new vertex; encountering a visited endpoint identifies an invalid cycle, including a loop or parallel edge. Preorder timestamps increase only on discovery, so a completed subtree occupies exactly its inclusive timestamp interval. Parent/depth/component metadata follows the chosen root orientation. Accumulated weight adds each parent edge once.

Binary lifting's table stores the 2^j-th saturated ancestor by induction on `j`. Decomposing a valid ancestor offset into set bits reaches the requested ancestor. For LCA, the interval checks handle ancestor pairs immediately; descending jump sizes preserve the invariant that the current vertex is below the answer and not an ancestor of the other endpoint. The parent after the final jump is their first common ancestor.

An Euler walk visits a parent after every child subtree. The minimum depth between first visits to two vertices in one component is exactly their LCA. Attaching component roots below a virtual root yields a single ±1 walk without altering any answer inside a component. The block size is `max(1,floor(log2(m))/2)` for tour length `m`. Every block is determined, up to an additive depth offset, by its ±1 signature; microtables store the exact minimum offset for every subinterval of each encountered signature. The final partial block uses the same valid signature prefix, and queries never access padding. A sparse table covers complete middle blocks; the two boundary microqueries and at most two sparse blocks cover a query interval. The macrotable has O((m/log(m))*log(m)) entries; all possible microtables take O(sqrt(m)*log(m)^2)=O(m) space/time.

Tarjan processes the real DFS postorder. When a vertex finishes, each completed child subtree has already been united into it, and `ancestor[findSet(vertex)]` names the highest ancestor whose completed portion contains that set. Queries are answered only after both endpoints finish. The set containing the other endpoint then names their LCA. Immediately afterward, uniting the completed vertex into its parent restores the invariant for the next exit event. Components are never united; cross-component queries explicitly retain `-1`.

A path from `u` to `v` climbs to `a=LCA(u,v)` and descends to `v`. Subtracting the two root-to-`a` prefixes gives its edge count and weight; a path jump chooses the appropriate upward segment. For rerooting, the deepest of `LCA(u,v)`, `LCA(u,r)` and `LCA(v,r)` is the unique intersection of the three pairwise paths, hence the common ancestor closest to `u` and `v` under root `r`.

## Provenance and legacy accounting

The algorithms were independently implemented from the invariants above; external source bodies were not copied. These pages were retrieved and inspected on 2026-09-27:

- [cp-algorithms, Lowest Common Ancestor — Binary Lifting](https://cp-algorithms.com/graph/lca_binary_lifting.html): powers-of-two parent table and ancestor interval query invariant.
- [cp-algorithms, Lowest Common Ancestor — Farach-Colton and Bender](https://cp-algorithms.com/graph/lca_farachcoltonbender.html): ±1 Euler reduction, half-logarithmic signature blocks, microtables, and macro sparse-table proof.
- [cp-algorithms, Lowest Common Ancestor — Tarjan's off-line algorithm](https://cp-algorithms.com/graph/lca_tarjan.html): union/completed-subtree ancestor invariant. Its simplified linear-time claim is not adopted for an ordinary DSU implementation.
- [OI Wiki, 最近公共祖先](https://oi-wiki.org/graph/lca/): independently reviewed binary lifting, Euler/RMQ and offline Tarjan sections. Its warning that ordinary union-find retains an inverse-Ackermann factor supports the stated offline bound. It identifies a separate specialized linear-time union-tree technique in Gabow–Tarjan (1983), [DOI 10.1145/800061.808753](https://doi.org/10.1145/800061.808753); the primary PDF request returned HTTP 403, so no primary-paper inspection is claimed. That specialized DSU variant is not claimed by this implementation.

The archived `LCA` in `OLD/algorithms.cpp:4563-4637` and `OLD/[1] algorithms.cpp:583-657` supplied the previous public constructor/query names, binary table orientation and inclusive timestamp convention. Both archives remain unchanged. `LCA(n,root,adj)` accepts adjacency sizes `n` (zero-based) or `n+1` (legacy one-based) and validates symmetric simple adjacency; vertex `0` remains an ordinary isolated vertex when unused. The maintained `.n` metadata is the actual adjacency size. Legacy nonpositive `getKthAncestor` remains identity. Historical recursive `init` was a construction helper and is superseded by constructing/assigning a fresh object; direct metadata mutation is outside the maintained contract.

`OLD/Team Notebook/src/algs.cpp:878` / `:931` and corresponding `algsbetter.cpp` sections contain `LcaO1` and `LcaLog`. Their linear-preprocessing constant-query Euler RMQ and logarithmic binary LCA capabilities are accounted for by `EulerLCA` and `LCA`; archived names/bodies remain preserved. The new versions add empty forests, stack safety, disconnected results, weighted sums, rerooting and offline queries.

## Feature-to-test map

Tester: [07-lca_tester.py](<../96-Local Testing/04-Graphs/07-lca_tester.py>), including the actual header from [07-lca_tester.cpp](<../96-Local Testing/04-Graphs/07-lca_tester.cpp>). Failure messages retain seed, graph edges/root, operation and expected/actual values. Test oracles use explicit runtime checks even with `-DNDEBUG`.

| Features | Independent evidence |
|---|---|
| Binary, Euler and offline LCA; CSR parity | Enumerated labeled simple forests through n=6, every preferred root/default, every vertex pair; independently rooted BFS parent-chain intersection |
| Ancestor/kth ancestor, inclusive timestamps | BFS depths/parents, explicit parent walks, all valid offsets and one-past-root, signed-64 maximum and legacy negative offsets |
| Edge distance, weighted sums, path jump | Independent BFS between each pair reconstructs the unique path and its exact `lll` sum; every path position plus one-past-end |
| Rerooted LCA | Independently BFS-root each small component at every possible vertex; compare every triple, including cross-component roots |
| Empty/singleton/isolated/disconnected forests | Exhaustive enumeration from n=0; empty batches, self paths, missing result/status semantics and unchanged weighted output |
| Full signed weight domain | 350 seeded random forests using `LNG_MIN`, `LNG_MAX`, negative/zero/positive edges; 200000-vertex all-maximum chain |
| FCB blocks, partial signatures and macrotable boundaries | Sizes 7/8 through 1023/1024, chains/stars/binary/random/disconnected forests, independent parent oracle on 1000 pairs per shape |
| Stack safety and large indices | 200000-vertex chain; binary/Euler queries, rerooted median identity and offline Tarjan endpoints |
| Query order, duplicates, repeated calls | Reverse/duplicate offline batches, empty batches and repeated queries on each fixture |
| Construction, lifetime, copy/move/reset | Graph/CSR constructors, copied/moved/assigned preprocessors, mutated input graph with unchanged snapshots, rebuild by assignment |
| Legacy API migration | Zero-/one-based constructors, actual adjacency size metadata, unused zero vertex, empty constructor and nonpositive historical ancestor rule |
| `LCAFold::pathFold` (vertex and edge modes) | Noncommutative string concatenation over every ordered pair of every enumerated forest through n=4 with every root, enumerated n=5..6 forests with roots -1, 0 and n-1, and every random forest, against the label sequence of the independent BFS path (edge labels keyed by endpoint pair at the BFS child); Graph and CSR builds; 2000 random sum folds on the 200000-vertex chain |
| `pathIntersection` | All n^4 quadruples of every enumerated forest through n=5 (default root); 100 random quadruples for other default-root cases and 10 per explicit root (the answer does not depend on the rooting); binary and Euler variants, against the common vertices of the two BFS paths in c-to-d order; interval-overlap identity on the 200000-vertex chain |
| Invalid preconditions | 23 assertion subprocesses: directed/cyclic/loop/parallel input, invalid roots/endpoints/offsets, malformed adjacency size/labels/symmetry/duplicates; binary/Euler/offline paths; `pathIntersection` endpoint, `LCAFold` label count and query endpoint |

## Recorded verification and performance

The 2026-10-07 re-audit commands, results and benchmark rerun are in [95-p011.md](95-p011.md); the paragraphs below are the original 2026-09-27 record.

`python3 '96-Local Testing/04-Graphs/07-lca_tester.py' --mode quick` passed optimized and checked builds with 500803 runtime checks per build and all 20 assertion probes.

Full command: `python3 '96-Local Testing/04-Graphs/07-lca_tester.py' --mode full --seed 20260927`. Optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`) builds each passed 25537081 runtime checks, and all 20 assertion probes passed. The initial ASan/UBSan executable encountered the sandbox LeakSanitizer ptrace restriction. Retrying the complete sanitizer configuration outside the sandbox with `--configuration ASan-UBSan` passed the same 25537081 runtime checks, with leak detection still enabled. Compiler: GNU C++ 16.2.1, GNU++20. Logs: `/tmp/p011-lca-full.log` and `/tmp/p011-lca-sanitizer.log`. Full mode includes 22497 exhaustive forest/root cases, 350 random weighted forests and the 200000-vertex chain; stress expands random cases to 2500 and the chain to 500000 without truncating full coverage.

The integration owner also compiles standalone/aggregate/multi-TU combinations. Shared benchmark artifacts are [92-decomposition_benchmark.py](<../96-Local Testing/04-Graphs/92-decomposition_benchmark.py>) and its neighboring C++/JSON files. They compare complete binary/Euler setup and query batches across chain/star/random/disconnected shapes, with output verification outside timing, one warmup, five samples, recorded machine/compiler/flags and explicit memory bounds. On the recorded 100000-vertex, 400000-query workloads, binary/Euler setup medians were 6.88/4.35 ms for chains and 24.06/18.17 ms for random trees; query medians were 3.43/9.61 ms for chains and 42.76/13.36 ms for random trees. Both variants remain explicit choices: ancestor-heavy chains can favor binary lifting's interval fast path; RMQ provides worst-case constant queries and linear storage. No hardware dispatch or universal speed claim is made.
