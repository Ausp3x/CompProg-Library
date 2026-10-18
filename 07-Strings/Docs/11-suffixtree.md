# 11-suffixtree.hpp — evidence

`11-suffixtree.hpp` implements the offline, sentinel-free suffix tree of one integral sequence (bytes as `T = int` for strings), built in `O(n)` from a `07` `SuffixArray` by a bottom-up LCP-interval scan. Node ids follow the lexicographic DFS pre-order; it supports children by first symbol, explicit and implicit loci (`step`, `locate`), weighted ancestors (the locus of `s[i, j)`), suffix links, occurrences, maximal and supermaximal repeats and the distinct-substring count.

## Contracts

### SuffixTree

- `SuffixTree<T>` for integral `T` owns `index`, a `SuffixArray<T>` taken by value (`SuffixTree(sa)` copies or moves it, RMQ kept if present). The text constructors build `SuffixArray<T>(s, false)`, without RMQ: `vector<T>` keeps the symbols' own order, and `string_view` (deduced from `string`, `string_view` and C strings) uses `T = int` with unsigned byte order, embedded NUL included. The default constructor is the tree of the empty text. Length `n < 2^30`, asserted in `build`, so the `V <= 2n` node ids fit `int`.
- The tree is the compacted trie of the `n` nonempty suffixes, with no sentinel. Its explicit nodes are the root, every suffix and every right-branching substring (two distinct right extensions). This is the suffix tree of `s$` with its `$`-leaves contracted, so a suffix that is a proper prefix of another ends on an explicit internal node and has no leaf of its own. There are `V <= max(1, 2n)` nodes.
- `Node {l, r, depth, parent, end}`: `[l, r)` is the range of `index.sa` holding the suffixes that start with the node's string; `depth` is the string depth; `parent` is `-1` at the root; `end` is one past the last pre-order id of the subtree. The node's string is `text[sa[l], sa[l] + depth)`. The root is `{0, n, 0, -1, V}`; when every suffix starts with the same symbol, the root and its only child share `[0, n)`.
- Ids: node ids are the lexicographic DFS pre-order. Root `0` comes first, a parent precedes its children, and children are ordered by first edge symbol, so ids sort nodes by `(l, depth)` and by their strings. The subtree of `v` is the id range `[v, end)`, and decreasing ids give a children-before-parent order for bottom-up passes.
- `children(v)` is a `std::span` over the CSR arrays `start`/`kids`, in ascending ids, which is also ascending first symbol. `child(v, c)` binary-searches it and returns the child whose edge starts with symbol `c`, or `-1`. On byte trees, symbols are `0..255`: pass `uint8_t(c)`, since a negative `char` matches nothing. Cost `O(log(deg + 1))`.
- `leaf[i]` is the node of suffix `i` (its locus, always explicit). `leafSuffix(v)` returns the `i` with `leaf[i] == v`, or `-1`; the root gives `-1`, and at most one suffix ends on a node.
- `edgeLabel(v)` (`v > 0`, asserted) returns the text range `[a, b)` of the label on the edge into `v`, taken from the occurrence `sa[l]`: `a = sa[l] + depth(parent)`, `b = sa[l] + depth(v)`. `root()` is `0` and `depth(v)` is `nodes[v].depth`.
- Locus `{v, k}`: the position of a substring of length `k`, canonical when `depth(parent(v)) < k <= depth(v)` (the root only with `k = 0`). It is explicit when `k == depth(v)` and otherwise lies on the edge into `v`. `step(locus, c)` extends the locus by one symbol: `O(1)` inside an edge, `child` at a node. It returns `{-1, 0}` when the extension does not occur and asserts a canonical locus.
- `locate(p)` returns the locus of the longest prefix of `p` that occurs in the text, with its length. It never fails (`{0, 0}` when even the first symbol is absent) and costs `O(m * log(S + 1))`. `isSubstring(p)` is `locate(p).second == |p|`, so the empty pattern is a substring. Patterns are `vector<T>`, or `string_view` read as unsigned bytes when `T = int`, which copies `O(m)`. A braced list is ambiguous between the two overloads for `T = int` (a compile error); pass a `vector`.
- `occurrences(v)` returns the ascending starts of the node's string, `sa[l, r)` sorted, in `O(out * log(out + 1))`. The root gives `0..n-1`: the empty string's boundary occurrence `n` is omitted, as in `07` `patternRange` (unlike `10`, whose empty pattern includes `n`). A pattern's occurrences are `occurrences(locate(p).first)` when `isSubstring(p)`; the count is `r - l` of that node.
- `weightedAncestor(v, len)` (`0 <= len <= depth(v)`, asserted) returns the shallowest ancestor of `v`, `v` included, of depth at least `len`. The locus of `s[i, j)` is `{weightedAncestor(leaf[i], j - i), j - i}` for `i < n`, and the root when `i == j`. It walks Myers' skew-binary jump pointers (one `int` per node) in `O(log(n + 1))` worst case.
- `suffixLink(v)` (`v > 0`, asserted) returns the node of `v`'s string without its first symbol, the root at depth `1`, in `O(log(n + 1))` through `weightedAncestor(leaf[sa[l] + 1], depth - 1)`. The target is always explicit: removing the first symbol preserves right-branching and being a suffix.
- `maximalRepeats()` returns the maximal repeats in ascending ids (lexicographic order), in `O(n)` time and space. Following Gusfield, `w` is one when it occurs at two starts whose left symbols differ (start of text counting as distinct) and whose right symbols differ (end of text counting as distinct). These are exactly the non-root nodes with `r - l >= 2` whose suffixes do not all share one left symbol; a prefix count of left-symbol changes between adjacent suffix ranks tests each node in `O(1)`.
- `supermaximalRepeats()` returns, in ascending ids and `O(n)`, the maximal repeats that are no proper substring of another maximal repeat. Equivalently, `w` occurs at least twice and no one-symbol extension `bw` or `wb` occurs twice. These are the nodes with `r - l >= 2` whose children are all leaves (each right extension unique) and whose left extensions are unique. A left extension `bw` repeats exactly when the suffix starting with it shares more than `depth` symbols with an SA neighbour. Leaf-only nodes have disjoint intervals, so the scan is linear.
- `distinctSubstrings()` returns the exact number of distinct nonempty substrings as `lng` (at most `n(n+1)/2`), the sum of edge lengths, in `O(V)`.
- `build(sa)` replaces the whole tree, which invalidates ids and spans. Fields are read-only to callers. Copy and move are value semantics.
- Memory: `nodes`, `start`, `kids` and `jump` take eight `int`s per node and `leaf` one per symbol, on top of the owned `SuffixArray` (`text`, `sa`, `rank`, `lcp`). The text constructors build no RMQ, so storage is `O(n)`; a supplied `SuffixArray` keeps its RMQ, which stores `O(n * log(n + 1))` and costs as much to copy (move it in to avoid the copy). Construction needs `O(n)` workspace.

Correctness: the internal nodes of the suffix tree of `s$` are the LCP intervals (Abouelhoda, Kurtz, Ohlebusch 2004). The scan keeps a stack of open intervals with strictly increasing depths; at the start of position `k` the top has depth `lcp[k - 1]`. Suffix `sa[k]` has length `d > lcp[k - 1]`, since a proper prefix sorts first. If `d > h = lcp[k]` it is a leaf; otherwise (`d == h`) it ends on the interval of depth `h` starting at `k`, which is pushed at this step. Intervals deeper than `h` close at `r = k + 1`; the last closed interval (or the leaf) becomes a child of the open interval of depth `h`, which either exists or is pushed with the closed interval's `l`. Each step pushes at most one node and each node is popped once, so the scan is `O(n)`. Ancestors have a smaller or equal `l` and a smaller depth, and nodes to the left have a smaller `l`, so `(l, depth)` order is the pre-order. Nodes sharing `l` form an ancestor chain created deepest first: each push is the parent of the node just closed, and a node attached to an existing open interval has a different `l` unless that interval is the root, which is created first. A reversed counting sort by `l` with the root fixed at `0` therefore yields the pre-order in `O(n)`. Jump pointers follow Myers (1983): `jump(v)` is `jump(jump(p))` when the two previous jump lengths from the parent `p` are equal, else `p`. Searching the topmost ancestor that satisfies the monotone predicate `depth >= len` takes `O(log V)` steps. The repeat characterizations are Gusfield's (as summarized by Wikipedia), restated for the contracted tree, where the internal nodes of the tree of `s$` are the nodes with `r - l >= 2`. If a proper superstring of a repeat `w` repeats, so does one of its one-symbol extensions of `w`, which gives the supermaximal test. Each distinct nonempty substring has exactly one locus, and the edge into `v` carries `depth(v) - depth(parent(v))` of them.

### Name map

| Catalog name | Here |
|---|---|
| maspypy `suffix_tree` node `(L, R, lo, hi)` | `Node{l, r, depth}` with `lo = depth(parent)`, `hi = depth` |
| maspypy `next` | `step` |
| maspypy `get_suffix_positions` | `leaf` |
| planned `dfsOrder` | the node ids themselves (lexicographic pre-order) with subtree `[v, Node::end)` |
| Gusfield string depth, locus, extended locus | `depth`, `{node, length}`, `locate` |

## Feature-to-test map

Runnable entry: [11-suffixtree_tester.py](<../../96-Local Testing/07-Strings/11-suffixtree_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/11-suffixtree_tester.cpp>). All oracles survive `-DNDEBUG`. The small oracle enumerates every substring with its sorted starts, keeps the root, the suffixes and the right-branching substrings as explicit nodes in lexicographic order, takes each node's parent as its longest explicit proper prefix and takes SA intervals from directly sorted suffixes.

| Feature/domain | Independent verification |
|---|---|
| `Node`, ids (`dfsOrder`), `root`, `depth`, `children`, `child` | Exact node set and count; `l`, `r`, `depth`, `parent`, `end` per node from the brute-force node list; children lists; `child` for every alphabet symbol plus absent ones. Large texts: `(l, depth)` order, parent nesting, internal depth equal to the interval LCE, parent depth equal to the larger boundary LCE, children partitioning the interval, `V <= 2n`. |
| `build`, constructors | Vector, `SuffixArray` with RMQ, rebuild of a used object, `string`, `string_view` and unsigned byte vector give identical trees; default (empty) tree. |
| `edgeLabel`, `leafSuffix`, `leaf` | Label length and text equality against each node's string; the suffix ending at each node; `leaf[i]` against the node of suffix `i`; large: every suffix ends on exactly one node, `leaf[i].l == rank[i]`. |
| `locate`, `isSubstring`, `step` | The locus of every substring (an extension oracle and an ancestor oracle cross-checked); `step` from every substring's locus by every symbol, present or absent; random probes against the longest occurring prefix; byte strings against byte vectors. Large: loci against `07` `patternRange`, `step` along the text and by an absent symbol. |
| `occurrences` | Brute-force sorted starts of every node string (root `0..n-1`); large: count, order and `lce` with the node string. |
| `weightedAncestor` | Every node and every length `0..depth`, against the brute-force ancestor chain; every `s[i, j)`; large: random `(i, len)` against the SA occurrence range of `s[i, i + len)` found by binary search with `lce`. |
| `suffixLink` | Brute-force node of the string without its first symbol (checked explicit); large: depth and interval against the SA range. |
| `maximalRepeats`, `supermaximalRepeats` | Gusfield's definitions directly: existence of a maximal pair over all occurrence pairs, then exclusion of maximal repeats contained in another. Fixed hand-derived regressions in every mode: `banana` (`a`, `ana`; `ana`), `mississippi` (`i`, `issi`, `p`, `s`; `issi`, `p`), `abab`, `xabxac`, `aaaa`, empty, with their distinct counts. Large: equal counts on the reversed text, sampled membership of the reversed repeat, supermaximal subset of maximal, unary closed form (`a^1..a^(n-1)`; `a^(n-1)`). |
| `distinctSubstrings` | Size of the substring set; large: `n(n+1)/2 - sum lcp`. |
| Alphabets | `int` (negative symbols), `bool`, `lng` extremes, `signed char` extremes, bytes with NUL, 0x7f, 0x80, 0xff. |
| Preconditions | Thirteen checked subprocesses require assertion `SIGABRT`: `edgeLabel(0)`, `suffixLink(0)`, `weightedAncestor` beyond the depth, negative length or node out of range, `step` from a non-canonical locus (root with length 1, length equal to the parent depth, length beyond the depth), `leafSuffix(-1)`, `occurrences(V)`, `depth(V)`, `children(-1)`, `child(V, c)`. |
| Large/adversarial | Unary, random binary, random 26-letter, Fibonacci, period-3 and Thue–Morse texts of 3,000/200,000/700,000 bytes (quick/full/stress). |

Oracle validation: 13 mutants of the header were run in quick mode, and 12 were caught (maximal and supermaximal conditions, ancestor stop, `leafSuffix`, `child`, in-edge `step`, subtree `end`, pre-order sort direction, suffix-link depth, unsorted occurrences, edge label). The survivor uses a strict jump test in `weightedAncestor`, which changes only the number of steps, not results.

## Commands and results

Run 2026-10-19 on an 11th Gen Intel Core i9-11900H with GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`). The shared string runner builds three configurations: optimized `-O2 -DNDEBUG`; checked `-O0 -g -D_GLIBCXX_DEBUG`, which also runs the assertion probes; and, in full and stress, `-O1 -g -fsanitize=address,undefined`. All use `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress runs passed the quick pre-flight under the 2048 MB cap. `MEMORY peak` is the run's capped peak (4096 MB cap, 0 OOM kills). The checked stress binary takes about 120 s against the runner's 180 s per-binary timeout.

```bash
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode quick --seed 20261019  # PASS, 2 configurations, 2,397 cases and 1,505,473 checks each, 13 assertion probes, MEMORY peak=666MB
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, 11,142 cases and 22,596,537 checks each, 13 assertion probes, MEMORY peak=855MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, same counts, MEMORY peak=818MB
python3 '96-Local Testing/07-Strings/11-suffixtree_tester.py' --mode stress --seed 20261020  # PASS, 3 configurations, 25,557 cases and 71,050,480 checks each, 13 assertion probes, MEMORY peak=936MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 163 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=1829MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261019 --no-integration  # PASS, 24 suites
```

## Benchmarks

No benchmark: the construction is the direct linear LCP-interval scan with no dispatch threshold, and the contest profile requires no Barrett/Montgomery comparison. From a text, the `07` prefix-doubling `SuffixArray` dominates construction (`O(n * log(n + 1))`); a caller holding a `SuffixArray` pays `O(n)`.

## Sources

| Source | Use |
|---|---|
| [maspypy `suffix_tree.hpp`](https://maspypy.github.io/library/string/suffix_tree.hpp) | API survey: sentinel-free rectangles `(L, R, lo, hi)`, pre-order numbering, `next` (adopted as `step`), `get_suffix_positions` (`leaf`). |
| [OI Wiki, 后缀树](https://oi-wiki.org/string/suffix-tree/) | Subtree as an SA interval; construction from the reversed suffix automaton (left out, see notes). |
| [Wikipedia, Suffix tree](https://en.wikipedia.org/wiki/Suffix_tree) | Gusfield-style operations: suffix links, maximal repeats and pairs, locus queries. |
| Abouelhoda, Kurtz, Ohlebusch, "Replacing suffix trees with enhanced suffix arrays", J. Discrete Algorithms 2 (2004) | LCP-interval tree and bottom-up traversal (known algorithm; the paper was not fetched in this session). |
| Gusfield, *Algorithms on Strings, Trees, and Sequences* (1997), §7.12 | Maximal and supermaximal repeat definitions and theorems (not accessed in this session; definitions as summarized by Wikipedia, and verified by brute force here). |
| Myers, "An applicative random-access stack", Inform. Process. Lett. 17 (1983) | Skew-binary jump pointers (not fetched; the construction is standard and verified by exhaustive locus tests). |

The code was written independently from these algorithms; no source code was copied. No legacy suffix-tree implementation exists in `OLD` (text search).

## Limits and handoffs

- `weightedAncestor` and `suffixLink` are `O(log(n + 1))`. Offline `O(1)` weighted-ancestor structures are not provided.
- `child` binary-searches the children. Under `-D_GLIBCXX_DEBUG`, `std::lower_bound` also checks that the range is partitioned, which costs `O(deg)` per call in that checked build only.
- Ukkonen's online construction is `33`, generalized (multi-document) trees are `28`, and suffix-array queries are `07`/`22`.

## History

- 2026-10-19: implemented and verified (P082); independent review found 0 defects and 8 low or nit findings (`n < 2^30` domain, RMQ memory note, local naming, range asserts, row `dfsOrder` mapping, provenance, fixed regressions), all fixed; full suite passed on g++ and g++-14, stress passed.
