# 17-treeisomorphism.hpp — evidence

Owned by package P056 / GR06; omissions are in [00-notes.md](00-notes.md#p056-omissions). Contest profile. The header has a deterministic linear-time AHU classification with canonical ids, built on per-height tries. On top of it sit a Monte Carlo rooted hash, automorphism orbits, rooted and center-based isomorphism mappings, canonical strings and automorphism counts. The row was planned; there is no legacy code.

## Contracts

### Common domain

Every function takes a tree: an undirected `Graph` or `CsrGraph`, connected, with `n >= 1` vertices and `n - 1` edges (all asserted through the edge count and a BFS that reaches every vertex). Weights are ignored.

- Rooted operations take `root` in `[0, n)` (asserted).
- `treeCanonicalString` and `automorphismCount` read `root = -1` as unrooted.
- Under `NDEBUG` a non-tree stays bounded: the BFS from the root (row 02's `bfs`) ignores non-tree arcs, every loop runs over the BFS order only, unreached vertices become isolated roots, and results are meaningless.

All operations take O(n) time and memory and are iterative.

**Centers.** A BFS from 0 finds the farthest vertex `a`, and a BFS from `a` finds the diameter path.

- An odd number of path vertices gives one center: the tree is rooted there.
- An even number gives two adjacent centers: both become children of a virtual vertex `n`.

Every automorphism and every isomorphism maps centers to centers, so this rooting is canonical.

### treeiso_detail::Classes

The engine takes a rooted forest (`par`, top-down `order`) and returns:

- `id`: classes ordered by height first, then by the sorted list of child ids compared lexicographically, with a proper prefix first;
- `kids[start[v], start[v + 1])`: the children of `v` by increasing id.

It assigns ids height by height, in increasing order. Emitting a vertex `v` with id `x` appends `x` to its parent's list and advances the parent's pointer in the trie of the parent's height:

- **Shared nodes.** `mark[node] == x` means the node already has a child for symbol `x`. Every emission of one id is consecutive, so all parents that share a prefix reach the same node.
- **Sorted lists.** Ids are emitted in increasing order, so every parent's list is sorted.
- **Sorted siblings.** Each trie node creates its children in increasing symbol order and prepends them. A stack preorder that pushes them in list order therefore visits the terminal nodes of a height-`h` trie lexicographically, prefix first.
- **Complete tries.** A parent is linked to its terminal node when its last child is emitted, which happens while a lower height is processed. Each height-`h` trie is therefore complete before its preorder runs.

Each emission creates at most one node, so time and memory are O(n + height).

The order between two classes depends only on the classes themselves, by induction on height. Ids are therefore ranks in one fixed total order of all rooted trees: canonical within a call. Across calls, ids are ranks among different class sets and are not comparable. Use canonical strings, the hash, or one call on trees joined under a new root.

### rootedTreeClassification

`rootedTreeClassification(g, root)` returns `id` in `[0, K)`, where `K = max + 1` is the number of distinct rooted subtrees. `id[u] == id[v]` iff the subtrees of `u` and `v` are isomorphic. Ids follow the canonical order above. This is Library Checker `rooted_tree_isomorphism_classification`: print `K`, then the ids.

### rootedTreeHash

`rootedTreeHash(g, root, seed = CustomHash::rnd)` returns, for every vertex, `H(v) = Π over children c of (x[height(v)] + H(c)) mod 2^61 - 1`, with `H(leaf) = 1` and `x[h] = splitMix64(seed + h) mod (2^61 - 1)`.

- **Distinct polynomials.** Distinct rooted trees give distinct polynomials:
  - As a polynomial in `x[h]` with `h` the root's height, `H` is monic with roots `-H(c)`, and the children's polynomials do not contain `x[h]`.
  - Unique factorization therefore recovers the children's multiset, and the claim follows by induction.
- **Degree.** The degree is at most `n - 1`.
- **Collision bound.** For uniformly random `x`, a fixed non-isomorphic pair collides with probability at most `(n - 1) / (2^61 - 1)` (Schwartz–Zippel); `splitMix64` stands in for uniform values.

Hashes are comparable across calls and trees with the same seed. The default seed is per process; pass a fixed one to reproduce a run. The result is Monte Carlo: a collision is a wrong equality.

### allRootsClassification

`allRootsClassification(g)` returns ids in `[0, K)`, where `id[u] == id[v]` iff `g` rooted at `u` is isomorphic to `g` rooted at `v`. These are the orbits of the automorphism group, since a rooted isomorphism between `(g, u)` and `(g, v)` is an automorphism mapping `u` to `v`.

With the canonical center rooting, an automorphism fixes the root, so `u` and `v` share an orbit iff their root paths carry the same class sequence. Orbits are therefore assigned top-down:

- The first vertex of an orbit (its representative) gives each run of equal child classes a new orbit.
- Every later vertex of the orbit has the same sorted child-class list, so it copies the representative's child orbits position by position.

The virtual root is dropped and ids are shifted to stay contiguous.

### rootedTreeIsomorphism and unrootedTreeIsomorphism

- `rootedTreeIsomorphism(g, r, h, s)` returns `res` with `res[v]` the image in `h` of every vertex `v` of `g`, `res[r] = s`, edges onto edges.
- `unrootedTreeIsomorphism(g, h)` returns the same without a root condition. It roots both trees at their centers (or virtual vertices) and returns empty when the sizes differ.

Both return an empty vector when no isomorphism exists. Trees have `n >= 1`, so empty never collides with a mapping.

Both run `Classes` once on the disjoint union, so ids are comparable, and compare the roots' classes. They then pair children in id order: equal classes have equal sorted child-class lists, so by induction this is a bijection that preserves edges.

### treeCanonicalString

`treeCanonicalString(g, root)` returns `(` and `)` only, with length `2n`.

- **Rooted.** The string is `(` + the children's strings in increasing id + `)`. It is equal iff the rooted trees are isomorphic.
- **Unrooted (`root = -1`).** With one center, the string is the rooted string at the center. With two centers, it is the two halves' rooted strings concatenated in increasing id, with no outer pair. Strings are equal iff the trees are isomorphic, and a single block never equals two blocks.

The string is canonical but not the lexicographically smallest encoding.

### automorphismCount

`automorphismCount<M>(g, root)` returns `|Aut(g)|` for `root = -1`, or the number of automorphisms fixing `root` otherwise. It is the product, over every vertex of the (center-)rooted tree, of `m!` for each run of `m` equal child classes. A virtual root contributes 2 exactly when the two halves are isomorphic.

`M` needs `M(int)` and `*=`. Use `lll` or a big integer for exact counts below its range, or a modint for counts modulo a prime (`mint`: mod 998244353).

## Aliases

- Library Checker `rooted_tree_isomorphism_classification` and "count distinct subtrees" (UOJ 763) are `rootedTreeClassification` and its `K`.
- suisen `is_isomorphic` and NetworkX `rooted_tree_isomorphism` are `rootedTreeIsomorphism`; NetworkX `tree_isomorphism`, ei1333 and Nyaan `tree_isomorphism` are `unrootedTreeIsomorphism`.
- suisen `classify_rerooting` is `allRootsClassification`.
- Nyaan `rooted-tree-hash` is `rootedTreeHash`.
- CSES Tree Isomorphism I and II are the rooted (at vertex 1) and unrooted mappings, or canonical strings.

## Feature-to-test map

`96-Local Testing/04-Graphs/17-treeisomorphism_tester.py` uses the shared graph runner with `-Werror` warnings, in optimized, checked (`_GLIBCXX_DEBUG`, with probes) and ASan/UBSan (full and stress) builds.

Oracles:

- naive AHU strings (each child string sorted and recomputed for every root), validated on paths and stars;
- brute-force permutations for automorphism counts;
- direct mapping validation (bijection, `res[r] = s`, every edge onto an edge of the other tree).

| Operation | Test | Oracle |
|---|---|---|
| `rootedTreeClassification` | Every labelled tree via Prüfer codes, n ≤ 6 (quick) or 7 (full) at every root (r = 0 above 6); symmetric families (copies of a random tree on a center or across an edge); large path (ids are heights) and complete binary tree | Partition equals naive subtree strings; ids contiguous; `id[u] < id[v]` iff `(height, sorted child ids)` is smaller |
| `rootedTreeHash` | Same cases, two seeds, Graph and CSR; root hashes compared across all labelled trees of each n; saved 7-vertex regression where child hash sums coincide; large relabelled roots | Partition equals naive strings; equal across trees iff naive strings are equal |
| `allRootsClassification` | Every labelled tree; symmetric families; large path; large relabelled copies | Partition equals naive strings at every root; orbits map onto orbits under the found isomorphism |
| `rootedTreeIsomorphism` | 300 (quick) or 3000 (full) random pairs per n, half relabelled, random roots; families against a tree one vertex larger; large relabelled copies | Mapping valid iff naive rooted strings are equal; empty when sizes differ |
| `unrootedTreeIsomorphism` | Same pairs (second graph as CSR); symmetric families relabelled and against a tree one vertex larger; large relabelled copies; large degree-changing non-isomorphic pairs (5 in full) | Mapping valid iff naive unrooted strings (minimum over roots) are equal; empty when sizes differ or the degree multiset changes |
| `treeCanonicalString` | Every labelled tree, rooted at every vertex and unrooted; families; large copies | Bijection with naive strings over all trees of each n; length `2n`; relabelling invariance |
| `automorphismCount` | Every labelled tree n ≤ 6, plus 1 in 16 trees at n = 7 (`lll`, rooted and unrooted); families n ≤ 9; large star `(n-1)!`, path 2 and 1, complete binary tree `2^(2^(d-1)-1)` (`mint`) | Brute-force permutations; closed forms |
| Preconditions | 9 probes: directed, root out of range, cycle, disconnected, empty, hash root, isomorphism root, automorphism root, unrooted cycle | Expected assertion failure |

Planted mutants were each compiled against the quick suite on 2026-10-16. All 8 were killed:

- trie nodes never shared;
- preorder reversed;
- the second center equal to the first;
- automorphism runs off by one;
- orbits never shared;
- children paired in reverse;
- additive hash (killed by the saved regression);
- virtual root wrapped in parentheses.

## Commands and results

2026-10-16, GCC 16.2.1 and GCC 14.4.1, Linux x86-64, GNU++20, 11th Gen Intel Core i9-11900H.

| Command | Result |
|---|---|
| `17-treeisomorphism_tester.py --mode quick` | PASS 2 configurations, 1,642 cases, 639,138 checks, 9 probes; MEMORY peak 528 MB |
| `17-treeisomorphism_tester.py --mode full --seed 1` | PASS 3 configurations, 20,249 cases, 4,664,734 checks (optimized; 3,366,430 in the capped checked build), 9 probes; 5 large non-isomorphic pairs; MEMORY peak 889 MB |
| `CXX=g++-14 17-treeisomorphism_tester.py --mode full --seed 2` | PASS 3 configurations, 20,249 cases, 4,805,639 checks, 9 probes; MEMORY peak 884 MB |
| `17-treeisomorphism_tester.py --mode stress --seed 3` | PASS 3 configurations, 290,393 cases, 39,646,926 checks (optimized: every labelled tree n ≤ 8, 10,000 families, 1,000,000-vertex shapes; ASan/UBSan 4,810,977 and checked 3,512,673 at their caps), 9 probes; MEMORY peak 896 MB |
| `02-integration.py --sanitizers` | PASS 144 standalone/aggregate headers (each header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 2,763 MB |
| `03-consistency.py` | No errors |

The checked build (`-O0 -D_GLIBCXX_DEBUG`) runs full and stress at n ≤ 7, 2,000 families and 50,000-vertex shapes; the ASan/UBSan build runs stress at n ≤ 7, 2,000 families and 200,000-vertex shapes. Both caps keep each run inside the runner's 180 s timeout.

```bash
python3 '96-Local Testing/04-Graphs/17-treeisomorphism_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/17-treeisomorphism_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/17-treeisomorphism_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/17-treeisomorphism_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: contest profile, no dispatch threshold. The 1,000,000-vertex stress shapes check stack safety and linear behaviour, not timings.

## Sources

Fetched 2026-10-16; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-16-p056).

| Source | Inspected claim and application |
|---|---|
| [Library Checker rooted_tree_isomorphism_classification](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/tree/rooted_tree_isomorphism_classification/task.md) | Exact classes, `K` and ids |
| [OI Wiki AHU](https://oi-wiki.org/graph/tree-ahu/) and [树哈希](https://oi-wiki.org/graph/tree-hash/) | AHU names with level radix sorting (linear); rerooting hash; automorphism counts mod 998244353 |
| [Nyaan rooted-tree-hash](https://nyaannyaan.github.io/library/tree/rooted-tree-hash.hpp), [tree-hash](https://nyaannyaan.github.io/library/tree/tree-hash.hpp) | `prod(x[height] + child)` mod 2^61 - 1; centroid-based unrooted hash |
| [suisen tree_isomorphism_classification](https://suisen-cp.github.io/cp-library-cpp/library/tree/tree_isomorphism_classification.hpp) | Per-instance ids, `classify_rerooting` |
| [ei1333 tree-isomorphism](https://ei1333.github.io/library/graph/tree/tree-isomorphism.hpp), [hitonanode tree_isomorphism](https://hitonanode.github.io/cplib-cpp/tree/tree_isomorphism.hpp), [Koosaga tree_isomorphism](https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/graph/tree_isomorphism.cpp), [maspypy subtree_hash](https://maspypy.github.io/library/graph/tree_dp/subtree_hash.hpp) | Catalog comparison: sort-based O(n log n) classification and randomized hashes |
| [NetworkX isomorphism](https://networkx.org/documentation/stable/reference/algorithms/isomorphism.html) | Rooted and unrooted tree isomorphism return a mapping |

The per-height trie with `mark` sharing is this library's formulation of the linear AHU bound. The catalogs fetched sort per level (O(n log n)) or hash. It was validated only by the tests above.

## Limits and handoffs

- Forests: join the roots (or the trees' centers) under a new vertex.
- Vertex colours: no API.
- Path rolling hashes on trees belong to Strings ([00-notes.md](00-notes.md#p056-omissions)).
- General graph isomorphism is row 54; centers and centroids as standalone operations are row 14.

## History

- 2026-10-16: verified (P056); new row; completeness sweep added `allRootsClassification` and `rootedTreeIsomorphism`; independent review: 1 finding fixed (`rootedTreeHash` read past the BFS order under `NDEBUG` on non-trees), BFS reused from row 02; 8 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
