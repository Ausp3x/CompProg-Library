# 07-lca.hpp — Evidence

Owned by package P011 / GR03 (static forest LCA); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions) and [00-notes.md](00-notes.md#p011-gr03-omissions-2026-11-15); GR03 sources are in [00-sources.md](00-sources.md#fetched-2026-11-15-p011-gr03). Implementation: [07-lca.hpp](../07-lca.hpp).

## Contracts

### Scope

The row covers static forest LCA, ancestor jumps and path folds. Ancestor and path binary search by predicate is row 15's `maxPath` (HLD), O(1) level ancestor is row 85's ([00-notes.md](00-notes.md#tree-ownership)), and tree and HLD aggregates and dynamic-tree APIs keep their own owners ([00-notes.md](00-notes.md#placement-decisions-recorded)).

### lca_detail::Forest, LCA and EulerLCA

`LCA(g, root=-1)`, `EulerLCA(g, root=-1)` and `lcaOffline(g, queries, root=-1)` accept the canonical `Graph` or `CsrGraph`. Input must be an undirected forest: loops, parallel edges and cycles violate preconditions. Every vertex participates, isolated ones included. An explicit root orients its own component first; the other components, and every component by default, are rooted at their smallest vertex. An empty forest is valid with `root=-1` and no queries.

Preprocessing owns its state and stays valid after the input graph changes or dies. Rebuild by construction or assignment; copies and moves are independent values. Exposed metadata and helpers must not be mutated. Roots have `parent=-1`; `component[u]` is the component root; `parent_edge[u]` keeps the original logical edge ID. `dep` counts edges. `t_in`/`t_out` keep the historical inclusive DFS subtree interval convention. Vertex labels are `[0,n)`.

| Operation | Result / domain | Binary LCA | Euler LCA |
|---|---|---|---|
| `lca(u,v)` | Vertex ID, or `-1` for different components | O(log(n)) | O(1) |
| `isAncestor(u,v)` | Reflexive ancestor relation; false across components | O(1) | O(1) |
| `distance(u,v)` | Number of edges, or `-1` across components | O(log(n)) | O(1) |
| `distanceWeighted(u,v,out)` | Connected status; exact signed `lll` sum written only if connected | O(log(n)) | O(1) |
| `rerootedLCA(u,v,r)` | LCA when rooted at `r`; `-1` unless all three are connected | O(log(n)) | O(1) |
| `kthAncestor(u,k)` | `k >= 0`; ID or `-1` above the root; signed 64-bit `k` accepted | O(log(n)) | Use binary variant |
| `jump(u,v,k)` | Vertex at zero-based edge offset `k >= 0` along `u` to `v`; `-1` if disconnected or beyond the endpoint | O(log(n)) | Use binary variant |

Self paths are legitimate zero-edge, zero-weight paths: LCA and a zero-offset jump return the vertex. Weights span all `lng` values, negatives included. Distances are sums along the unique simple forest path. At most `INT_MAX-1` edges of signed 64-bit magnitude fit in `lll`, including the temporaries of weighted queries.

Binary lifting takes O(n log(n)) preprocessing and storage, in the vertex-major `up[u][j]` layout. Jump pointers saturate at the root internally, while `kthAncestor` answers `-1` beyond the root.

Euler LCA is Farach-Colton–Bender ±1 RMQ with O(n) preprocessing and storage and O(1) queries. Its tour includes a virtual super-root, `-1` internally, that keeps adjacent depths ±1 across components; a same-component query cannot return it. Its signed-int Euler positions require `2*n+1 <= INT_MAX`; the binary and offline variants keep the Graph vertex-count domain. A component check answers cross-component queries before the RMQ.

`lca_detail::Forest` is the shared iterative DFS. One global preorder timer runs across all components, so the inclusive intervals `[t_in, t_out]` of different components are disjoint, and `covers(u, v)` (the unchecked interval test inside `lca`'s lifting loop) is exactly the ancestor relation without a component comparison. `isAncestor` keeps the vertex-range assertions; `lca` validates its two arguments once at entry. `Forest` costs O(n) time and memory.

### SparseTableLCA and lca_detail::Paths

`SparseTableLCA(g, root = -1)` has `LCA`'s forest, root and query contracts and the methods `lca`, `distance`, `distanceWeighted`, `rerootedLCA` and `isAncestor`, without `kthAncestor` or `jump`. Setup costs O(n log n) time and memory and each query O(1) (OI Wiki DFS-order LCA; maspypy `Fast_LCA`, hitonanode `lca_rmq`). It is kept beside `EulerLCA` for its faster queries, with a measured crossover below which `EulerLCA`'s cheaper O(n) setup wins (Benchmarks; `01-principles.md`, Priority). The table is the verified Data Structures `SparseTable` with a one-line min monoid `lca_detail::MinInt` (`00-notes.md`, P011 GR03 omissions).

Correctness: position `i` of the preorder stores `t_in` of the parent of `order[i]`, or `-1` at a root. For `u != v` in one component with `t_in[u] < t_in[v]`, the minimum over positions `(t_in[u], t_in[v]]` is `t_in` of their LCA. If `u` is an ancestor of `v`, the child of `u` towards `v` lies in that range and every vertex there is a proper descendant of `u`, so no parent precedes `u`. Otherwise the child of the LCA `a` towards `v` lies in the range and every vertex there lies below `a`, so no parent precedes `a`. Ranges inside one component never contain a root.

`lca_detail::Paths<D>` (CRTP over `Forest`) holds the one copy of `distance`, `distanceWeighted` and `rerootedLCA`, calling `D::lca`. `LCA` (and so `LCAFold`), `EulerLCA` and `SparseTableLCA` derive from it and share its contract.

### meet and lcaSet

`meet(t, a, b, c)` accepts any LCA struct here and returns the median of the three vertices, the unique vertex on all three pairwise paths, or `-1` unless all three share a component; it asserts the range. It returns exactly `t.rerootedLCA(a, b, c)` by calling it, kept as a named free function by user decision 2026-11-15 (duplicates.md, Keeps; maspypy `meet`). `pathIntersection` uses it.

`lcaSet(t, vs)` accepts any LCA struct and returns the LCA of the vertex set `vs` (duplicates allowed), or `-1` when `vs` is empty or spans several components. Every vertex is range-asserted, even after a cross-component one. Proof: the LCA of the vertices with the smallest and largest `t_in` has a subtree interval containing every `t_in` between them, so it is a common ancestor of all; any common ancestor of all is a common ancestor of those two, hence an ancestor of their LCA. O(|vs|) plus one LCA query (OI Wiki: the LCA of a union is the LCA of the parts' LCAs).

### lcaOffline

Offline Tarjan returns one answer per input query, keeping order and duplicates. The query count fits `int`. Empty batches, self pairs and disconnected pairs are valid. It uses the canonical Data Structures `DSU`: O((n+q) alpha(n)) amortized time, O(n+q) auxiliary storage and O(q) output. All traversals and union-find searches are iterative.

### LCAFold

`LCAFold<M>(g, values, mon = {}, root = -1)` (formerly `<T, Op>` with `(identity, op)`) takes a monoid `M` (`S`, `op`, `e`) under the [stored-object contract](<../../02-Data Structures/Docs/00-monoids.md#monoid-contract>), kept as `m`. It is an `LCA` (same contracts, every `LCA` method) plus two vertex-major lifting tables: `rise[u][j]` folds the `2^j` vertices from `u` upward in walk order, `fall[u][j]` the same vertices downward. `op` is associative with two-sided identity `e()`; commutativity is not needed. `values.size() == n` is asserted. `pathFold(u, v)` returns `{true, fold of the vertex labels on the path u -> v in order, both endpoints included}`; `pathFold(u, v, true)` folds edge labels instead, where the caller stores each edge's label at its child vertex under the chosen rooting and the LCA's label is skipped. A self path folds `values[u]` (vertex mode) or nothing (edge mode). Disconnected pairs return `{false, identity}`. Setup O(n * log(n)) operations and storage; query O(log(n)) operations. Table entries above a root repeat the root's saturated jump and are never read, since a query climbs at most the depth difference. Caller-owned arithmetic must not overflow for any prefix the tables combine.

### pathIntersection

`pathIntersection(t, a, b, c, d)` accepts an `LCA`, `LCAFold`, `EulerLCA` or `SparseTableLCA` and returns the endpoints `{x, y}` of the vertex intersection of the paths a–b and c–d, with `x` nearest `c` and `y` nearest `d` (`x == y` for a single shared vertex), or `{-1, -1}` when the paths do not meet or two of the four vertices lie in different components. All four vertices are asserted in range. With `meet(p, q, r)` the median of three (the deepest of the three pairwise LCAs, since two coincide), `x = meet(a, b, c)` and `y = meet(a, b, d)` are the projections of `c` and `d` onto path a–b. If they differ, path c–d runs through both, so the intersection is path x–y. If they coincide, the intersection is `{x}` exactly when `meet(c, d, x) == x`, and empty otherwise. At most nine LCA queries: O(log(n)) with `LCA`, O(1) with `EulerLCA`.

### Correctness

The shared DFS skips exactly the parent logical edge; any other edge reaching a visited endpoint is a cycle, loop or parallel edge, which the entry assertion rejects. Under `NDEBUG` that arc is skipped, so the DFS still visits each vertex once and ends in O(n + m) with a meaningless forest (P042 GR05 fix, ratified by the maintainer at `/merge-pr 4`, 2026-11-08). Preorder timestamps increase only on discovery, so a completed subtree occupies exactly its inclusive interval. Accumulated weight adds each parent edge once.

Binary lifting stores the 2^j-th saturated ancestor by induction on `j`, and decomposing an offset into set bits reaches the requested ancestor. For LCA, the interval checks settle ancestor pairs at once; descending jump sizes keep the current vertex below the answer and not an ancestor of the other endpoint, so the parent after the last jump is the first common ancestor.

An Euler walk visits a parent after every child subtree, so the minimum depth between the first visits of two vertices in one component is their LCA. Hanging the component roots below a virtual root gives one ±1 walk without changing any answer inside a component. The block size is `max(1,floor(log2(m))/2)` for tour length `m`. Each block is determined, up to a depth offset, by its ±1 signature; microtables store the exact minimum offset of every subinterval of each signature met. The final partial block uses a valid signature prefix, and queries never read padding. A sparse table covers complete middle blocks, so two boundary microqueries and at most two sparse lookups answer a query. The macrotable has O((m/log(m))*log(m)) entries; all microtables take O(sqrt(m)*log(m)^2)=O(m) space and time.

Tarjan processes the DFS postorder. When a vertex finishes, each completed child subtree is already united into it, and `ancestor[findSet(vertex)]` names the highest ancestor whose completed part contains that set. A query is answered once both endpoints finish: the set containing the other endpoint then names their LCA. Uniting the finished vertex into its parent restores the invariant. Components are never united; cross-component queries keep `-1`.

A path from `u` to `v` climbs to `a=LCA(u,v)` and descends to `v`; subtracting the two root-to-`a` prefixes gives its edge count and weight, and a path jump picks the right segment. For rerooting, the deepest of `LCA(u,v)`, `LCA(u,r)` and `LCA(v,r)` is the unique intersection of the three pairwise paths, hence the common ancestor closest to `u` and `v` under root `r`.

### Legacy Mapping

- `LCA(n, root, adj)` and `lca_detail::fromAdjacency(n, adj)` → `LCA(Graph::fromAdjacency(adj), root)`. The vertex count is `adj.size()`, so the one-based form (`n + 1` lists) keeps vertex 0 isolated. `Graph::fromAdjacency` adds the edges in the same increasing `(min, max)` order as the removed builder, so `t_in`, `order` and every answer are unchanged for any list order (compared with the removed builder on 20,000 random forests with shuffled lists before its removal). Out-of-range, unmirrored, duplicate and loop entries still fail a checked precondition: `fromAdjacency`'s range and symmetry checks, or the forest check on the parallel edge or loop they become.
- P276 rename, 2026-11-10: `getLCA` → `lca`; `getKthAncestor` → `kthAncestor` (asserts `k >= 0`; the clamp of negative `k` is gone); `offlineLCA` → `lcaOffline`; `weightedDistance` → `distanceWeighted`.

## Feature-to-Test Map

Tester: [07-lca_tester.py](<../../96-Local Testing/04-Graphs/07-lca_tester.py>), which includes the header from [07-lca_tester.cpp](<../../96-Local Testing/04-Graphs/07-lca_tester.cpp>). Full mode runs 22,497 exhaustive forest and root cases, 350 random weighted forests and the 200,000-vertex chain; stress raises random cases to 2,500 and the chain to 500,000.

| Features | Independent evidence |
|---|---|
| `LCA`, `EulerLCA`: `lca`; `lcaOffline`; CSR parity | Enumerated labelled simple forests through n=6, every preferred root and the default, every vertex pair; independently rooted BFS parent-chain intersection |
| `isAncestor`, `kthAncestor`, inclusive timestamps | BFS depths and parents, explicit parent walks, all valid offsets and one past the root, signed-64 maximum |
| `distance`, `distanceWeighted`, `jump` | Independent BFS between each pair reconstructs the unique path and its exact `lll` sum; every path position plus one past the end |
| `rerootedLCA` | Each small component BFS-rooted at every vertex; every triple, including cross-component roots |
| Empty, singleton, isolated and disconnected forests | Exhaustive enumeration from n=0; empty batches, self paths, missing-result semantics and unchanged weighted output |
| Full signed weight domain | 350 seeded random forests using `LNG_MIN`, `LNG_MAX`, negative, zero and positive edges; 200000-vertex all-maximum chain |
| FCB blocks, partial signatures and macrotable boundaries | Sizes 7/8 through 1023/1024, chains, stars, binary, random and disconnected forests, independent parent oracle on 1000 pairs per shape |
| Stack safety and large indices | 200000-vertex chain; binary and Euler queries, rerooted median identity and offline Tarjan endpoints |
| Query order, duplicates, repeated calls | Reversed and duplicate offline batches, empty batches and repeated queries on each fixture |
| Construction, lifetime, copy, move and reset | Graph and CSR constructors, copied, moved and assigned preprocessors, mutated input graph with unchanged snapshots, rebuild by assignment |
| Adjacency lists through `Graph::fromAdjacency` (the removed constructor's cases); `kthAncestor` on them | Unsorted zero- and one-based lists, vertex count equal to the list count, unused vertex 0 isolated, empty lists |
| `LCAFold::pathFold` (vertex and edge modes) | Noncommutative string concatenation over every ordered pair of every enumerated forest through n=4 with every root, enumerated n=5..6 forests with roots -1, 0 and n-1, and every random forest, against the label sequence of the independent BFS path (edge labels keyed by endpoint pair at the BFS child); Graph and CSR builds; 2000 random sum folds with `Sum<lng>` on the 200000-vertex chain and 500 per modulus with a capturing-lambda sum modulo 7 and 10^9+7 through `Monoid` (closed-form path sums) |
| `pathIntersection` (also on `SparseTableLCA`) | All n^4 quadruples of every enumerated forest through n=5 (default root); 100 random quadruples for other default-root cases and 10 per explicit root (the answer does not depend on the rooting); binary and Euler variants, against the common vertices of the two BFS paths in c-to-d order; interval-overlap identity on the 200000-vertex chain |
| `SparseTableLCA`: `lca`, `isAncestor`, `distance`, `distanceWeighted`, `rerootedLCA`, fields | Everywhere `EulerLCA` is checked: every pair and reroot vertex of every enumerated forest and root and of the random extreme-weight forests (Graph and CSR), 1000 queries per FCB boundary shape, the deep chain (expected `min(u, v)`, exact weights); fields against `LCA` and BFS; copy, move, snapshot and rebuild |
| `meet` (`LCA`, `LCAFold`, `EulerLCA`, `SparseTableLCA`) | Every triple of every small and random forest against the single common vertex of the three BFS paths; the chain's middle value |
| `lcaSet` (all four structs) | Every vertex subset through n=6 and 200 random multisets beyond, each also reversed with a duplicate; empty (also on the empty forest), singleton, isolated and cross-component sets; oracle: deepest vertex on every parent-walk ancestor chain; the chain's minimum |
| `lca_detail::Paths` (shared `distance`, `distanceWeighted`, `rerootedLCA`) | The rows above for `LCA`, `EulerLCA` and `SparseTableLCA`, which all inherit it |
| Invalid preconditions | 28 assertion subprocesses: `sparse-query-end`, `sparse-cycle`, `meet-end` (middle argument), `set-end` (an out-of-range vertex after a cross-component pair), `root-beyond` (root n + 1), `euler-query-first`, `intersection-first` and `intersection-second` (first and second operands past the end), and directed, cyclic, loop and parallel input, invalid roots, endpoints and offsets, an out-of-range entry in undirected lists through `Graph::fromAdjacency` (`adjacency-label`); binary, Euler and offline paths; `pathIntersection` endpoint, `LCAFold` label count and query endpoint. Unmirrored lists and a loop listed once are `01-graph_tester` probes |

## Mutation Testing

Record: mewt 4.0.0 on blob `b32d34a25944604d92425a2e6a76564a17094a5c` (2026-11-15, seed 1, whole file, uniform sample of 1099): 500 mutants, 449 caught (0 by timeout), 51 survived, 0 untested. Stage seconds of the first pass: quick 1186, full on the optimized build 154, full on the checked and ASan-UBSan builds 5533.

The 51 survivors are equivalent or runtime only; line numbers are the current header's.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 748`, `898`, `913`, `918`, `923`, `938`, `973`, `1003` `<` → `!=` | 37, 146, 154, 155, 157, 162, 189, 208 | Each counter steps by 1 from at most its bound. |
| `COS 720`, `745`, `895`, `1015` `!= -1` → `> -1`; `COS 784`, `814`, `944` `== -1` → `<= -1` | 23, 36, 141, 216, 56, 77, 165 | The operand is `-1` or a vertex index, never below `-1`. |
| `AS 318`, `320`, `321`, `334`, `366`, `370`, swapped `lca` or `meet` arguments | 53, 60, 62, 117, 243, 244 | `lca` is symmetric in its two vertices and `meet` in its three. |
| `AS 323`, `341`, swapped `max` and `min` arguments | 74, 147 | `max` and `min` are symmetric. |
| `MR 1073`, `std::move(a)` → `a` | 190 | Only a copy is added. |
| `COS 806`, `dep[a] >= dep[b]` → `>` | 63 | Of the three pairwise LCAs two coincide and the third is their descendant, so equal depths mean `a == b`, and the mutated branch then returns `b`. |
| `CR 150`, `IF 242`, the component check of `LCA::lca` removed | 87 | Across components neither interval covers the other, so the loop lifts `u` to its root, whose parent is `-1`; O(log(n)) as before. |
| `AS 325`, `covers(v, u)` → `covers(u, v)` | 89 | That test is then never true, and when `v` is an ancestor of `u` the lifting loop stops at `v`'s child on the path and returns `v`. |
| `CR 188`, the component check of `EulerLCA::lca` removed | 170 | Between the first visits of two components' vertices the tour passes the separator `-1`, the unique least height, so the answer is `tour[best] == -1`. |
| `AOS 497`, `498`, the `tour.reserve` size changed | 136 | Only capacity changes. |
| `AOS 505`, `506`, `509`, `522`, `523`, `IF 252`, block size changed, micro-table count enlarged, or every block's micro table rebuilt | 142, 145, 152 | Runtime only: the in-block tables are exact for any block size at least 1, so the answers stay identical. The block size becomes 1 or `ceil(log2(m) / 2)` instead of `floor(log2(m) / 2)`, or the micro tables grow to `2^block_size` or `2^(block_size + 1)` masks or are rebuilt per block. Measured `EulerLCA` setup at n = 500 000 (min of 5, `-O2`, 2026-11-15, scratch driver not kept; original 18.9 ms on a chain, 154.8 ms on a random recursive tree): block size 1 (`505`, `509`) 3.4× and 1.4×, the larger block (`506`) 1.04× and 1.03×, `2^(block_size + 1)` masks (`522`, the larger of `522` and `523`) 1.0× and 1.03×, per-block rebuild (`252`) 1.3× and 1.1×; block size 1 also raises the peak from 101 MB to 173 MB through its O(n * log(n)) sparse table. |
| `COS 911`, `>` → `>=` on adjacent heights | 149 | Adjacent tour heights differ by exactly 1, the separator `-1` included. |
| `BAOS 687`, `\|=` → `^=` | 149 | Each mask bit is set at most once, from zero. |
| `AOS 579`, `580`, sparse level size `count - (1 << j) + 1` → `count - (1 << j)` | 161 | Queries read level `j` only for blocks strictly between `a` and `b`, so index at most `count - 1 - (1 << j)`; the build reads level `j - 1` only below its mutated size too. The dropped last entry is never read. |
| `AS 343`, `347`, `352`, `353`, swapped `better` arguments | 163, 174, 177 | They change only which position wins a tie, and every least-height position of a tour range holds the same vertex: the range's LCA, or `-1` across components. |
| `COS 961`, `996`, `l > r` → `>=` | 171, 197 | Swapping equal bounds changes nothing. |
| `COS 1031`, `t_in[u] > t_in[hi]` → `>=` | 233 | Preorder times are distinct, so equality means `u == hi`. |
| `COS 968`, `a + 1 < b` → `!=` | 175 | `a < b` holds there, so `a + 1 <= b`. |
| `CR 206`, the query-count assertion removed | 205 | Unreachable: it binds only past `INT_MAX` queries (16 GiB), and the assertion vanishes under `NDEBUG`. |
| `COS 1048`, `x != y` → `x < y` | 244 | When `x != y` the projections of `c` and `d` onto `path(a, b)` differ, so `path(c, d)` runs through `x` and `meet(t, c, d, x) == x` returns the same pair. |

## Commands and Results

2026-11-15 (P011 GR03). Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, the runner's optimized, checked and ASan/UBSan builds; GCC 16.2.1 unless stated, beside other suites and a running mutation campaign.

| Command | Result |
|---|---|
| `07-lca_tester.py --mode quick` | PASS, 1,256,699 checks, 28 assertion probes, optimized run 0.27 s, checked 3.42 s; peak 938 MB |
| `07-lca_tester.py --mode full --seed 20261115` | PASS, 3 configurations, 71,563,399 checks per configuration, 28 probes; runs 8.65 s optimized, 23.87 s checked, 210.23 s ASan-UBSan (limit 1000 s); peak 1311 MB |
| `CXX=g++-14 07-lca_tester.py --mode full --seed 20261115` (GCC 14.4.1, floor) | PASS, 71,563,399 checks per configuration, 28 probes; peak 1329 MB |
| `07-lca_tester.py --mode stress --seed 20261116` | PASS, 3 configurations, 106,218,704 checks per configuration; runs 13.05 s optimized, 36.87 s checked, 402.91 s ASan-UBSan; peak 1769 MB |
| `05-extraction.py ops --filter 04-Graphs/07-lca.hpp` | PASS, 20 operations in 14 distinct extractions on g++ and g++-14 |
| `05-extraction.py testers --filter 07-lca_tester` | PASS, 1 quick run on the extracted build |
| `00-decomposition_benchmark.py` | PASS, every result checked; medians under Benchmarks |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace; peak 3129 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/07-lca_tester.py' --mode full --seed 20261115
CXX=g++-14 python3 '96-Local Testing/04-Graphs/07-lca_tester.py' --mode full --seed 20261115
python3 '96-Local Testing/04-Graphs/07-lca_tester.py' --mode stress --seed 20261116
python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/07-lca.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter 07-lca_tester
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/07-lca.hpp
python3 '96-Local Testing/04-Graphs/00-decomposition_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/00-decomposition_benchmark.py>) and its git-ignored `00-decomposition_benchmark.json` log (shared with `08-scc.hpp`) hold workload generation, seed, all five samples and medians. The C++ driver warms up every alternative, includes allocation in construction time, times queries separately, excludes input generation and checks every result against the others outside the timed region. Cases use n = 32, 2,000, 100,000 and 1,000,000 with q = max(1000, 4n) uniform queries on chains, stars, random recursive trees and disconnected chains of at most 32 vertices. Memory is O(n log(n)) for `LCA` and `SparseTableLCA`, O(n) for `EulerLCA`.

Medians recorded 2026-11-15 in milliseconds, setup / queries (GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`, i9-11900H, seed 20260927; shared machine, beside a mutation campaign):

| Workload | `LCA` | `EulerLCA` | `SparseTableLCA` |
|---|---|---|---|
| Chain, n = 10^5 | 10.66 / 5.58 | 5.17 / 17.66 | 8.69 / 8.96 |
| Star, n = 10^5 | 8.04 / 46.12 | 4.89 / 18.40 | 7.83 / 8.20 |
| Random tree, n = 10^5 | 31.44 / 57.80 | 24.07 / 22.93 | 21.39 / 8.38 |
| Chain, n = 10^6 | 151.6 / 116.8 | 61.7 / 415.8 | 100.0 / 201.6 |
| Star, n = 10^6 | 116.5 / 1400.0 | 68.7 / 357.5 | 80.2 / 135.5 |
| Random tree, n = 10^6 | 512.3 / 1494.9 | 415.3 / 397.3 | 453.5 / 130.2 |
| Disconnected chains, n = 10^6 | 89.9 / 22.8 | 52.1 / 26.4 | 80.9 / 24.2 |

`SparseTableLCA` answers queries fastest of the O(1) structures in every workload (2.2–3.1 times `EulerLCA` on stars and random trees, 2.0 times on chains, 1.1 times on disconnected short chains) and finishes setup plus 4n queries first on stars and random trees. Binary lifting finishes first on chains, whose ancestor shortcut makes queries cheap, and `EulerLCA` on disconnected short chains, whose queries mostly stop at the component check. `EulerLCA` builds faster, with O(n) memory, in every workload but the random tree at n = 10^5, so it wins when queries are few. The two totals meet at about 1.6n queries (chain) and 1.15n (star) at n = 10^5, where `SparseTableLCA` wins on the random tree at every count; at about 0.72n (chain), 0.21n (star) and 0.57n (random tree) at n = 10^6; and at about 52n on disconnected short chains. This measured crossover is why both are kept (`01-principles.md`, Priority); `LCA` stays for `kthAncestor`, `jump` and `LCAFold`. There is no automatic dispatch threshold, and no Barrett/Montgomery or ISA specialization.

## Sources

The algorithms were implemented independently from the invariants above; no source body was copied. These pages were retrieved and inspected on 2026-09-27:

- [cp-algorithms, Lowest Common Ancestor — Binary Lifting](https://cp-algorithms.com/graph/lca_binary_lifting.html): powers-of-two parent table and ancestor interval query invariant.
- [cp-algorithms, Lowest Common Ancestor — Farach-Colton and Bender](https://cp-algorithms.com/graph/lca_farachcoltonbender.html): ±1 Euler reduction, half-logarithmic signature blocks, microtables, and the macro sparse-table proof.
- [cp-algorithms, Lowest Common Ancestor — Tarjan's off-line algorithm](https://cp-algorithms.com/graph/lca_tarjan.html): union and completed-subtree ancestor invariant. Its simplified linear-time claim is not adopted for an ordinary DSU.
- [OI Wiki, 最近公共祖先](https://oi-wiki.org/graph/lca/): binary lifting, Euler/RMQ and offline Tarjan sections. Its warning that ordinary union-find keeps an inverse-Ackermann factor supports the stated offline bound. It names a specialized linear-time union-tree technique in Gabow–Tarjan (1983), [DOI 10.1145/800061.808753](https://doi.org/10.1145/800061.808753). The paper, read on 2026-11-22 ([00-sources.md](00-sources.md#fetched-2026-11-22-source-check)), runs static tree set union in O(m + n) on a random-access machine and applies it to off-line nearest common ancestors (application 6); that variant is not claimed.

The archived `LCA` in `OLD/algorithms.cpp:4563-4637` and `OLD/[1] algorithms.cpp:583-657` supplied the earlier constructor and query names, the binary table orientation and the inclusive timestamp convention. Its `LCA(n,root,adj)` constructor and `getKthAncestor` map as in Legacy Mapping (the nonpositive-offset identity is gone, P276). The recursive `init` helper is superseded by constructing or assigning a fresh object; direct metadata mutation is outside the contract.

`OLD/Team Notebook/src/algs.cpp:878` / `:931` and the matching `algsbetter.cpp` sections hold `LcaO1` and `LcaLog`. Their linear-preprocessing constant-query Euler RMQ and logarithmic binary LCA are accounted for by `EulerLCA` and `LCA`, which add empty forests, stack safety, disconnected results, weighted sums, rerooting and offline queries.

Sources for `LCAFold` and `pathIntersection` (fetched 2026-10-07) are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

## Limits and Handoffs

None.

## Aliases

Renamed aliases (P276, 2026-11-10): `getLCA`, `getKthAncestor`, `offlineLCA`, `weightedDistance`.

Search aliases (P263, grab --find): 倍增, 最近公共祖先, 最低共同祖先, 树上倍增, k级祖先, 树上距离, 欧拉序, 树上路径交.

Search aliases (P011 GR03, grab --find): DFS-order LCA, RMQ LCA, maspypy `Fast_LCA`, LCA of a vertex set, median of three vertices, tree median; DFS序求LCA, 点集LCA, 三点中位数.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 25,537,081 checks, 20 probes), integration and benchmark passed on g++ 16.
- 2026-10-07: re-audit, added `LCAFold::pathFold` and `pathIntersection`; 4 findings fixed (complexity lines, unchecked `getLCA` loop, `offlineLCA` naming and brace); full suite, stress, integration and benchmark passed on g++ 16; hand mutations of the additions failed the quick suites; independent review: all eight package findings confirmed fixed, its own brute-force checks of every addition passed under ASan/UBSan/`_GLIBCXX_DEBUG`.
- 2026-10-12: prune, removed the legacy `LCA(n, root, adj)` constructor and `lca_detail::fromAdjacency` (cases moved to `Graph::fromAdjacency` lists, four legacy probes replaced by `adjacency-label`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect, equivalence confirmed on its own 30,000 random forests and multigraphs.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, peak 1052/1201/1680 MB.
- 2026-11-04: P042 GR05 `NDEBUG` fixes (ratified by the maintainer at `/merge-pr 4`, 2026-11-08): `Forest` skips a visited endpoint (before, a cycle looped forever), and `kthAncestor` returns −1 for k < 0 instead of walking past the lifting table; full on g++ and g++-14 and stress (seed 7) passed with unchanged check counts.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>` and `template<class L = LCA>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound and a raised ASan-UBSan limit of 1000 s; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-10: P276 rename (`getLCA` → `lca`, `offlineLCA` → `lcaOffline`, `weightedDistance` → `distanceWeighted`, `getKthAncestor` merged into `kthAncestor` (user decision)); full on g++ and g++-14, stress and extraction passed.
- 2026-11-15: P011 GR03, added `SparseTableLCA`, `meet` (user decision: kept beside `rerootedLCA`) and `lcaSet`, and the shared `lca_detail::Paths` base; completeness sweeps (three passes and a gap recheck); full (seed 20261115) on g++ and g++-14, stress, extraction, integration and the benchmark with `SparseTableLCA` passed; independent review: no finding on this header, its question on keeping `SparseTableLCA` answered by the measured crossover; whole-file mutation campaign, four probes added for its survivors, 51 equivalent or runtime only.
- 2026-11-16: P011 /finish GR03: review fixes, no header change: the `EulerLCA` crossover stated per n and per workload, the `lcaSet` proof reworded, probe count corrected to 28, the GR03 sources and omissions linked, the benchmark driver recorded as scratch; the mutation record stays current (header unchanged).
- 2026-11-27: P011 limits recheck: the row's ownership boundaries moved from Limits and Handoffs to Contracts (Scope); the scratch-driver note dropped, since Mutation Testing records it; documents only, no run.
