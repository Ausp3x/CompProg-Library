# 11-suffixtree.hpp — Evidence

`11-suffixtree.hpp` implements the offline, sentinel-free suffix tree of one integral sequence (strings as `T = int` bytes), built in `O(n)` from a `07` `SuffixArray` by a bottom-up LCP-interval scan. Node ids are the lexicographic DFS pre-order. It provides children by first symbol, explicit and implicit loci (`step`, `locate`), `O(1)` lowest common ancestors and suffix links, weighted ancestors (the locus of `s[i, j)`) online and as an offline batch, occurrences, maximal and supermaximal repeats and the distinct-substring count.

## Contracts

### SuffixTree

- `SuffixTree<T>` for integral `T` owns `index`, a `SuffixArray<T>` taken by value: `SuffixTree(sa)` copies or moves it and keeps its RMQ if present. The text constructors build `SuffixArray<T>(s, false)` (SA-IS, `O(n)` for bytes and dense integer ranges, see [07-suffixarray.md](07-suffixarray.md)) without RMQ: `vector<T>` keeps the symbols' own order, and `string_view` (deduced from `string`, `string_view` and C strings) uses `T = int` in unsigned byte order, embedded NUL included. The default constructor gives the tree of the empty text. Length `n < 2^30`, asserted in `build`, so the `V <= 2n` node ids fit `int`.
- The tree is the compacted trie of the `n` nonempty suffixes, with no sentinel. Its explicit nodes are the root, every suffix and every right-branching substring (two distinct right extensions). It is the suffix tree of `s$` with the `$`-leaves contracted, so a suffix that is a proper prefix of another ends on an explicit internal node and has no leaf of its own. There are `V <= max(1, 2n)` nodes.
- `Node {l, r, depth, parent, end}`: `[l, r)` is the range of `index.sa` holding the suffixes that start with the node's string, `depth` the string depth, `parent` `-1` at the root, `end` one past the last pre-order id of the subtree. The node's string is `text[sa[l], sa[l] + depth)`. The root is `{0, n, 0, -1, V}`; when every suffix starts with the same symbol, the root and its only child share `[0, n)`.
- Ids are the lexicographic DFS pre-order: root `0` first, a parent before its children, children by first edge symbol, so ids sort nodes by `(l, depth)` and by their strings. The subtree of `v` is the id range `[v, end)`, and decreasing ids give a children-before-parent order.
- `children(v)` is a `std::span` over the CSR arrays `start`/`kids`, in ascending ids, which is also ascending first symbol. `child(v, c)` binary-searches it in `O(log(deg + 1))` and returns the child whose edge starts with `c`, or `-1`. On byte trees symbols are `0..255`: pass `uint8_t(c)`, since a negative `char` matches nothing.
- `leaf[i]` is the node of suffix `i` (its locus, always explicit). `leafSuffix(v)` returns the `i` with `leaf[i] == v`, or `-1` (also for the root); at most one suffix ends on a node.
- `edgeLabel(v)` (`v > 0`, asserted) returns the text range `[a, b)` of the edge label into `v`, taken from the occurrence `sa[l]`: `a = sa[l] + depth(parent)`, `b = sa[l] + depth(v)`. `root()` is `0`; `depth(v)` is `nodes[v].depth`.
- A locus `{v, k}` is the position of a substring of length `k`, canonical when `depth(parent(v)) < k <= depth(v)` (the root only with `k = 0`); it is explicit when `k == depth(v)` and otherwise lies on the edge into `v`. `step(locus, c)` extends it by one symbol, `O(1)` inside an edge and `child` at a node; it returns `{-1, 0}` when the extension does not occur and asserts a canonical locus.
- `locate(p)` returns the locus of the longest prefix of `p` occurring in the text, with its length. It never fails (`{0, 0}` when even the first symbol is absent) and costs `O(m * log(S + 1))`. `isSubstring(p)` is `locate(p).second == |p|`, so the empty pattern is a substring. Patterns are `vector<T>`, or `string_view` read as unsigned bytes when `T = int` (an `O(m)` copy). For `T = int` a braced list is ambiguous between the two overloads (a compile error); pass a `vector`.
- `occurrences(v)` returns the ascending starts of the node's string (`sa[l, r)` sorted) in `O(out * log(out + 1))`. The root gives `0..n-1`: the empty string's boundary occurrence `n` is omitted, as in `07` `patternRange` and unlike `10`. A pattern's occurrences are `occurrences(locate(p).first)` when `isSubstring(p)`, and its count is `r - l` of that node.
- `lca(u, v)` (both in `[0, V)`, asserted) returns the lowest common ancestor in `O(1)`; `lca(v, v) = v`. Its string is the longest common prefix of the two node strings, so `depth(lca(leaf[i], leaf[j])) = lce(i, j)` for `i != j`, and `nodes[lca(u, v)].depth` is the LCP ecnerwala's `get_split` returns. The answer is `u` when `v` is in `u`'s subtree range (or symmetrically); otherwise, with `u < v`, it is the minimum of `split` over the boundaries `[l(u), l(v))`.
- `split[k]` (`0 <= k < n - 1`) is the deepest node holding ranks `k` and `k + 1`, of depth `lcp[k]`. `window[k]` and `blocks` answer its range minima in `O(1)`: bit `t` of `window[k]` marks `k - t` as a strict suffix minimum of `split` over the last 32 boundaries, so a range of at most 32 boundaries ending at `k` has its minimum at the highest marked bit inside it; longer ranges add the `SparseTable<Min<int>>` over 32-boundary block minima (the layout of ecnerwala's `RangeMinQuery`).
- `weightedAncestor(v, len)` (`0 <= len <= depth(v)`, asserted) returns the shallowest ancestor of `v`, `v` included, of depth at least `len`. The locus of `s[i, j)` is `{weightedAncestor(leaf[i], j - i), j - i}` for `i < n`, and the root when `i == j`. It walks Myers' skew-binary jump pointers (one `int` per node), `O(log(n + 1))` worst case.
- `weightedAncestors(queries)` answers a batch of `{v, len}` queries in query order, each under `weightedAncestor`'s precondition (asserted per query); duplicates and an empty batch are allowed. It runs offline in `O((n + q) * alpha(n))` time and `O(n + q)` memory with one DSU over the nodes, and equals `weightedAncestor` query by query. It pays off on deep trees, where jump-pointer walks are long (12 times faster on unary text); on shallow trees the online form is faster even at `q = n` (Benchmarks). Under `-DNDEBUG` a length outside `[0, n]` is clamped for bucketing, so it stays in bounds.
- `suffixLink(v)` (`v > 0`, asserted) returns the node of `v`'s string without its first symbol in `O(1)` (the root at depth `1`). The target is always explicit, since removing the first symbol preserves right-branching and being a suffix. `link[v]` is precomputed at build (`link[0] = -1`) as `lca(leaf[sa[l] + 1], leaf[sa[r - 1] + 1])` for depth at least 2, i.e. `V - 1` `O(1)` LCA queries.
- `maximalRepeats()` returns the maximal repeats in ascending ids (lexicographic order) in `O(n)` time and space. Following Gusfield, `w` is one when it occurs at two starts whose left symbols differ (the text start counting as distinct) and whose right symbols differ (the text end counting as distinct). These are exactly the non-root nodes with `r - l >= 2` whose suffixes do not all share one left symbol; a prefix count of left-symbol changes between adjacent ranks tests each node in `O(1)`.
- `supermaximalRepeats()` returns, in ascending ids and `O(n)`, the maximal repeats that are no proper substring of another maximal repeat: `w` occurs at least twice and no one-symbol extension `bw` or `wb` occurs twice. These are the nodes with `r - l >= 2` whose children are all leaves (each right extension unique) and whose left extensions are unique; a left extension `bw` repeats exactly when the suffix starting with it shares more than `depth` symbols with an SA neighbour. Leaf-only nodes have disjoint intervals, so the scan is linear.
- `distinctSubstrings()` returns the exact number of distinct nonempty substrings as `lng` (at most `n(n+1)/2`), the sum of edge lengths, in `O(V)`.
- `build(sa)` replaces the whole tree, invalidating ids and spans. Fields are read-only to callers; copy and move are value semantics.
- Memory: `nodes`, `start`, `kids`, `jump` and `link` take nine `int`s per node; `leaf`, `split` and `window` one word per symbol each; `blocks` `(n / 32) * log(n / 32 + 1)` `int`s; all on top of the owned `SuffixArray` (`text`, `sa`, `rank`, `lcp`). The text constructors build no RMQ, so storage is `O(n)`; a supplied `SuffixArray` keeps its RMQ, which stores `O(n * log(n + 1))` and costs as much to copy (move it in to avoid the copy). Construction needs `O(n)` workspace.

Correctness:

- Construction: the internal nodes of the suffix tree of `s$` are the LCP intervals (Abouelhoda, Kurtz, Ohlebusch 2004). The scan keeps a stack of open intervals with strictly increasing depths; at the start of position `k` the top has depth `lcp[k - 1]`. Suffix `sa[k]` has length `d > lcp[k - 1]`, since a proper prefix sorts first. If `d > h = lcp[k]` it is a leaf; otherwise (`d == h`) it ends on the interval of depth `h` starting at `k`, pushed at this step. Intervals deeper than `h` close at `r = k + 1`; the last closed interval (or the leaf) becomes a child of the open interval of depth `h`, which exists or is pushed with the closed interval's `l`. Each step pushes at most one node and each node is popped once, so the scan is `O(n)`.
- Pre-order: ancestors have a smaller or equal `l` and smaller depth, and nodes to the left a smaller `l`, so `(l, depth)` order is the pre-order. Nodes sharing `l` form an ancestor chain created deepest first: each push is the parent of the node just closed, and a node attached to an existing open interval has a different `l` unless that interval is the root, created first. A reversed counting sort by `l` with the root fixed at `0` therefore yields the pre-order in `O(n)`.
- Jump pointers (Myers 1983): `jump(v)` is `jump(jump(p))` when the two previous jump lengths from the parent `p` are equal, else `p`; finding the topmost ancestor satisfying the monotone predicate `depth >= len` takes `O(log V)` steps.
- `split` and `lca`: at the end of scan step `k < n - 1` the stack top has depth `lcp[k]` and holds ranks `k` and `k + 1`, every deeper open interval having closed at `k + 1`, so the top is `split[k]`. Ancestors are the id ranges `[v, end)`. If neither node contains the other and `u < v`, then `l(u) < l(v)`, since nodes sharing `l` form an ancestor chain. Every boundary in `[l(u), l(v))` lies inside the LCA `w`'s interval, so its split node is `w` or a descendant, and the boundary after the last rank of `w`'s child containing `u` splits at `w` itself; pre-order puts ancestors first, so the minimum id is `w`. The window masks keep a monotone stack restricted to 32 positions, each pushed and popped once, so the RMQ builds in `O(n)` and answers in `O(1)`. The block table has `(n / 32) * log(n / 32 + 1) <= n` entries, because `n < 2^30` keeps the logarithm below 32.
- Suffix links: for `v` at depth `d >= 2`, the suffixes `i = sa[l]` and `j = sa[r - 1]` have LCE exactly `d` (a leaf has `i = j`; an internal node has at least two children or a suffix ending on it). Then `i + 1` and `j + 1` have LCE `d - 1`, and their LCA is the explicit node of depth `d - 1`.
- `weightedAncestors` sweeps thresholds `d` from `n` down to `0`. The edge into each node opens at its parent's depth; within a bucket edges precede queries, and a DSU label keeps each component's top. At threshold `len` a component is a subtree cut at edges whose parent is shallower than `len`, so its top is the shallowest ancestor of depth at least `len`.
- Repeats: the characterizations are Gusfield's (as summarized by Wikipedia), restated for the contracted tree, whose nodes with `r - l >= 2` are the internal nodes of the tree of `s$`. If a proper superstring of a repeat `w` repeats, so does a one-symbol extension of `w`, which gives the supermaximal test.
- Distinct substrings: each distinct nonempty substring has exactly one locus, and the edge into `v` carries `depth(v) - depth(parent(v))` of them.

### Name Map

| Catalog name | Here |
|---|---|
| maspypy `suffix_tree` node `(L, R, lo, hi)` | `Node{l, r, depth}` with `lo = depth(parent)`, `hi = depth` |
| maspypy `next` | `step` |
| maspypy `get_suffix_positions` | `leaf` |
| planned `dfsOrder` | the node ids themselves (lexicographic pre-order) with subtree `[v, Node::end)` |
| Gusfield string depth, locus, extended locus | `depth`, `{node, length}`, `locate` |
| ecnerwala `get_split(l, r)` (LCP and split index of an SA interval) | `lca(leaf[sa[l]], leaf[sa[r - 1]])`; the LCP is its `depth` |

## Feature-to-Test Map

Runner: [11-suffixtree_tester.py](<../../96-Local Testing/07-Strings/11-suffixtree_tester.py>), with the [C++ suite](<../../96-Local Testing/07-Strings/11-suffixtree_tester.cpp>) on the real header. The small oracle enumerates every substring with its sorted starts, keeps the root, the suffixes and the right-branching substrings as explicit nodes in lexicographic order, takes each node's parent as its longest explicit proper prefix, and takes SA intervals from directly sorted suffixes. Large texts use the verified `07` `SuffixArray` with RMQ. A node interval `[l, r)` is accepted as the range of `s[i, i + len)` when it holds rank `i`, both ends share `len` symbols with suffix `i` by `lce`, and both outer neighbours do not; such ranks are contiguous, so each check is `O(1)`.

| Feature/domain | Independent verification |
|---|---|
| `Node`, ids (lexicographic pre-order), `root`, `depth`, `children`, `child` | Exact node set and count; `l`, `r`, `depth`, `parent`, `end` per node from the brute-force node list; children lists; `child` for every alphabet symbol plus absent ones. Large: `(l, depth)` order, parent nesting, internal depth equal to the interval LCE, parent depth equal to the larger boundary LCE, children partitioning the interval, `V <= 2n`. |
| `build`, constructors | Vector, `SuffixArray` with RMQ, rebuild of a used object, `string`, `string_view` and unsigned byte vector give identical trees (`link`, `split`, `window`, `blocks` included); the default (empty) tree. |
| `edgeLabel`, `leafSuffix`, `leaf` | Label length and text against each node's string; the suffix ending at each node; `leaf[i]` against the node of suffix `i`. Large: every suffix ends on exactly one node, `leaf[i].l == rank[i]`. |
| `locate`, `isSubstring`, `step` | The locus of every substring from the ancestor oracle, cross-checked on texts of at most 30 symbols against an extension oracle (`O(n^3 * log(n))`, skipped on medium texts); `step` from every substring's locus by every symbol, present or absent; random probes against the longest occurring prefix; byte strings against byte vectors. Large: loci against `07` `patternRange`, `step` along the text and by an absent symbol. |
| `occurrences` | Brute-force sorted starts of every node string (root `0..n-1`). Large: count, order and `lce` with the node string. |
| `lca` | Every ordered node pair against a walk up the brute parent chain, the walk itself checked against the longest common prefix of the two node strings; medium texts (60–200 symbols) reach ranges over 32 and 64 boundaries. Large: random node pairs and leaf pairs at rank gap `0..99` (inside one window, two windows, and with blocks) against depth `min(depth u, depth v, lce)` and the SA range of that prefix, plus symmetry. |
| `weightedAncestor` | Every node and length `0..depth` against the brute-force ancestor chain; every `s[i, j)`. Large: random `(i, len)` against the SA occurrence range of `s[i, i + len)` (the `O(1)` interval check). |
| `weightedAncestors` | One batch per text holding every `(node, length)` and every `s[i, j)`, duplicates included, against the brute-force ancestor chain; an empty batch; the empty tree. Large: a batch of `min(n, 200000)` random queries (half with length below 40) against the SA range, depth at least `len` and parent depth below it. |
| `suffixLink` | Brute-force node of the string without its first symbol (checked explicit). Large: every node's link, by depth and interval, against the SA range of the shortened string. |
| `maximalRepeats`, `supermaximalRepeats` | Gusfield's definitions directly: a maximal pair over all occurrence pairs, then exclusion of maximal repeats inside another. Hand-derived regressions in every mode: `banana` (`a`, `ana`; `ana`), `mississippi` (`i`, `issi`, `p`, `s`; `issi`, `p`), `abab`, `xabxac`, `aaaa` and empty, with their distinct counts. Large: equal counts on the reversed text, sampled membership of the reversed repeat, supermaximal a subset of maximal, the unary closed form (`a^1..a^(n-1)`; `a^(n-1)`). |
| `distinctSubstrings` | Size of the substring set. Large: `n(n+1)/2 - sum lcp`. |
| Alphabets | `int` (negative symbols), `bool`, `lng` extremes, `signed char` extremes, bytes with NUL, 0x7f, 0x80, 0xff. |
| Preconditions | Eighteen checked subprocesses require an assertion `SIGABRT`: `edgeLabel(0)`, `suffixLink(0)`; `weightedAncestor` beyond the depth, with a negative length or a node out of range; `step` from a non-canonical locus (root with length 1, length equal to the parent depth, length beyond the depth); `leafSuffix(-1)`, `occurrences(V)`, `depth(V)`, `children(-1)`, `child(V, c)`, `lca(V, 0)`, `lca(0, -1)`; `weightedAncestors` with a query beyond the depth (after a valid one), a negative length, and node `V`. |
| Large and adversarial | Unary, random binary, random 26-letter, Fibonacci, period-3 and Thue–Morse texts of 3,000/200,000/700,000 bytes (quick/full/stress). |

Oracle validation (2026-10-19, by hand): 27 mutants of the header ran in quick mode (seed 7, `-O2`) and 25 were caught, covering `lca` (ancestor test, both block bounds, stack pop condition, window mask width, short-range length, long-range left window, `split` set to the root, maximum block minima), suffix links (suffix pair, root link), `weightedAncestors` (dropped queries, queries ahead of edges in a bucket, wrong component label) and the earlier operations (maximal and supermaximal conditions, ancestor stop, `leafSuffix`, `child`, in-edge `step`, subtree `end`, pre-order sort direction, unsorted occurrences, edge label, distinct count). Both survivors are equivalent: a strict jump test in `weightedAncestor` changes only the step count, and ending the `weightedAncestors` sweep at 1 leaves length-0 answers at their initial value `0`, the root.

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.1 + 3.1 s, checked 33.6 + 20.3 s, ASan-UBSan 52.1 + 40.1 s. Peak 1611 MB, pre-flight quick 1180 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.2 + 8.1 s, checked 84.0 + 46.5 s, ASan-UBSan 98.8 + 99.4 s. Peak 1696 MB, pre-flight quick 1219 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 58.3 + 0.8 s, checked 38.8 + 16.8 s. Peak 1155 MB. |
| `python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode stress --seed 42` (three suites at a time) | PASS. Optimized 0.1 + 5.2 s, checked 19.9 + 31.3 s, ASan-UBSan 23.8 + 54.2 s. Peak 1063 MB, pre-flight quick 59 MB. |

Tester runs 2026-10-19 (GCC 16.2.1 and the floor GCC 14.4.1), the latest with check counts. The full lines are the P017 rerun (recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results)), the others the P082 reopening; the checked stress binary took about 136 s under load average about 3.

```bash
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode quick --seed 20261019  # PASS, 2 configurations, 2,397 cases and 2,229,237 checks each, 18 assertion probes, MEMORY peak=682MB
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode full  # PASS, 30,379,721 checks per configuration, MEMORY peak=801MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode full  # PASS, same counts, MEMORY peak=813MB
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode stress --seed 20261020  # PASS, 3 configurations, 25,557 cases and 91,097,853 checks each, 18 assertion probes, MEMORY peak=953MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 163 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=1836MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261019 --no-integration  # PASS, 24 suites
```

## Benchmarks

Driver: [11-suffixtree_benchmark.py](<../../96-Local Testing/07-Strings/11-suffixtree_benchmark.py>) with [11-suffixtree_benchmark.cpp](<../../96-Local Testing/07-Strings/11-suffixtree_benchmark.cpp>), run as `python3 '96-Local Testing/07-Strings/11-suffixtree_benchmark.py' --reps 5` on 2026-10-19: seed 20261019, `-std=gnu++20 -O2 -DNDEBUG`, GCC 16.2.1, i9-11900H, load average 1.2 to 1.3 from other sessions, MEMORY peak=428MB; one warmup plus five repetitions, medians in ms; every method's checksum agrees.

Texts have 1,000,000 symbols; each query set has 1,000,000 queries: random node pairs, leaf pairs at rank gap below 64, and random `(node, length)` pairs. `sparse` is a `SparseTable<Min<int>>` over the same `split` map; `composed` is `weightedAncestor(u, min(depth u, depth v, lce))` through the `SuffixArray` RMQ.

| Workload | Tree build | `lca` random | `sparse` random | `composed` random | `lca` gap < 64 | `sparse` gap < 64 | `composed` gap < 64 |
|---|---:|---:|---:|---:|---:|---:|---:|
| random, 2 letters | 101.5 | 49.0 | 27.6 | 246.5 | 33.0 | 25.2 | 212.2 |
| random, 26 letters | 103.8 | 42.9 | 26.7 | 134.9 | 33.2 | 24.8 | 129.8 |
| Fibonacci | 73.8 | 42.8 | 27.0 | 255.7 | 31.5 | 24.2 | 215.0 |
| unary | 45.1 | 5.2 | 5.1 | 453.8 | 3.7 | 3.7 | 61.7 |

| Workload | `weightedAncestors` batch | `weightedAncestor` loop | Sparse-table build | `lca` structure MB | Sparse table MB |
|---|---:|---:|---:|---:|---:|
| random, 2 letters | 88.7 | 45.5 | 20.1 | 9.7 | 75.8 |
| random, 26 letters | 49.5 | 15.9 | 23.0 | 9.7 | 75.8 |
| Fibonacci | 145.2 | 43.3 | 22.4 | 9.7 | 75.8 |
| unary | 38.3 | 471.4 | 12.8 | 9.7 | 75.8 |

- The `O(n)` `lca` structure takes 1.0 to 1.8 times the sparse table's time, with an eighth of its memory and no extra `O(n * log(n))` build, so the tree keeps `O(n)` storage.
- The rejected composition of `lce` and `weightedAncestor` is 3.1 to 88 times slower.
- The offline `weightedAncestors` batch pays a fixed DSU pass over all `V` nodes: 12 times faster than the online loop on the deep unary tree, while on shallow trees (random, Fibonacci) the online jump pointers stay 1.9 to 3.4 times faster even at `q = n`.

## Sources

| Source | Use |
|---|---|
| [ecnerwala `suffix_array.hpp`](https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/seq/suffix_array.hpp), [`rmq.hpp`](https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/ds/rmq.hpp) | `get_split` (LCP and split index of an SA interval) informs `lca`; `RangeMinQuery` (32-element blocks, in-block masks, sparse table over block minima) is the layout of the `split` RMQ. Fetched 2026-10-19 by the completeness researcher. |
| [maspypy `suffix_tree.hpp`](https://maspypy.github.io/library/string/suffix_tree.hpp) | API survey: sentinel-free rectangles `(L, R, lo, hi)`, pre-order numbering, `next` (adopted as `step`), `get_suffix_positions` (`leaf`). |
| [OI Wiki, 后缀树](https://oi-wiki.org/string/suffix-tree/) | Subtree as an SA interval; construction from the reversed suffix automaton (left out, see notes). |
| [Wikipedia, Suffix tree](https://en.wikipedia.org/wiki/Suffix_tree) | Gusfield-style operations: suffix links, maximal repeats and pairs, locus queries. |
| Abouelhoda, Kurtz, Ohlebusch, "Replacing suffix trees with enhanced suffix arrays", J. Discrete Algorithms 2 (2004) | LCP-interval tree and bottom-up traversal (known algorithm; the paper was not fetched in this session). |
| Gusfield, *Algorithms on Strings, Trees, and Sequences* (1997), §7.12 | Maximal and supermaximal repeat definitions and theorems (not accessed in this session; definitions as summarized by Wikipedia, and verified by brute force here). |
| Myers, "An applicative random-access stack", Inform. Process. Lett. 17 (1983) | Skew-binary jump pointers (not fetched; the construction is standard and verified by exhaustive locus tests). |

The code was written independently from these algorithms; no source code was copied. `OLD` holds no suffix-tree implementation (text search).

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- `weightedAncestor` is `O(log(n + 1))` online; `weightedAncestors` is the offline batch, and `O(1)`-query weighted-ancestor structures belong to `51` (GLN14, BKPR21).
- The `lca` range minimum is an inline `O(n)` structure because Data Structures `54` LinearRMQ is still planned; once `54` is verified, it may replace the inline structure if it is not slower.
- `child` binary-searches the children. Under `-D_GLIBCXX_DEBUG`, `std::lower_bound` also checks that the range is partitioned, which costs `O(deg)` per call in that checked build only.
- Ukkonen's online construction is row `33`, generalized (multi-document) trees row `28`, and suffix-array queries rows `07` and `22`.

## Aliases

Search aliases (P263, grab --find): 后缀树, 极大重复子串, 本质不同子串.

## History

- 2026-10-19: implemented and verified (P082); independent review found 0 defects and 8 low or nit findings (`n < 2^30` domain, RMQ memory note, local naming, range asserts, row `dfsOrder` mapping, provenance, fixed regressions), all fixed; full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: omission-sweep reopening (P082): `O(1)` `lca` (split map, window RMQ), `O(1)` precomputed `suffixLink` and offline `weightedAncestors` added, plus a benchmark; 25 of 27 mutants caught (both survivors equivalent); the independent review found 1 evidence defect, 3 low and 5 nit findings, all fixed (NDEBUG clamp, naming, evidence wording); quick, full (g++ and g++-14), stress, integration and the folder suite passed.
- 2026-10-19: P017, `SuffixArray` construction moved to SA-IS (text constructors `O(n)` for bytes and dense ranges, complexity comment tightened, no API change); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
