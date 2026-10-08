# 04-trie.hpp — evidence

`04-trie.hpp` (batch ST02, package P016) provides a multiset string trie in two transition layouts from one template: `Trie` (ordered sparse map over all 256 bytes) and `TrieDense<S, BASE>` (fixed-alphabet array rows). Both support counts, prefix counts, all-or-nothing erase with node reclamation, lexicographic enumeration, a node cursor, and stored-prefix queries. Contest profile, GNU C++20, no ISA code.

## Contracts

### BasicTrie, Trie, TrieDense

`BasicTrie<S = 0, BASE = 0>` is the implementation; `Trie = BasicTrie<>` keeps sparse `map<unsigned char, int>` transitions; `TrieDense<S = 26, BASE = 'a'> = BasicTrie<S, BASE>` keeps `array<int, S>` rows for keys whose bytes lie in `[BASE, BASE + S)`; the template static-asserts `BASE + S <= 256` and `BASE = 0` when `S = 0`, and `S = 0` selects the map (so `TrieDense<0, 0>` is `Trie`). Keys are byte strings (`string_view`) of length below `INT_MAX`, including the empty key, embedded NUL and, for `Trie` or `TrieDense<256, 0>`, every byte. Multiplicities are positive `lng`; the total is at most `INT64_MAX` (asserted before mutation); peak live nodes including the root fit `int`. Order is unsigned-byte lexicographic, a key before its extensions.

Complexity with key or query length `L`, map fanout `A <= 256` and dense row width `S`: map lookups and updates are O(L * log(A + 1)), dense ones O(L); insertion and the free list use amortized allocation bounds. Storage is O(P) for the map and O(P * S) for dense rows, P being peak live nodes. `clear()` is O(P), releases edge allocations and keeps vector capacities.

Operations:

- `insert(s, k = 1)` adds k copies. For `TrieDense`, every byte of `s` must be in range (asserted once at entry).
- `count(s)`, `countPrefix(s)`, `search(s)`, `size()`, `empty()`: terminal multiplicity, multiplicity of keys starting with `s` (the empty prefix counts everything), membership, total multiplicity, emptiness. Queries never assert the dense alphabet: a byte outside it simply has no edge.
- `erase(s, k = 1)` removes exactly k copies and returns true, or returns false without mutation when the key is absent or has fewer copies; O(L) workspace. Nodes whose pass count reaches zero are unlinked and pushed on the free list.
- Node cursor: `Node` has `next`, `terminal` (multiplicity of its key) and `pass` (multiplicity of keys through it); the root is node 0. `step(u, c)` returns the child of `u` along byte `c` or `-1`; `findNode(s)` walks from the root and returns the node of `s` or `-1`; the root exists even when the trie is empty. A node exists exactly when its pass count is positive (or it is the root). `newNode()` returns the most recently freed slot, otherwise appends a slot; a node obtained directly is detached and cleared. Node ids and references are invalidated by `insert`, `erase` and `clear`.
- `forEach(prefix, visit)` and `forEach(visit)` call `visit(string_view word, lng multiplicity)` once per distinct key with the prefix, in lexicographic order; returning false cancels and makes the result false (even at the last key); a missing prefix returns true with no calls. Cost O(L * log(A + 1) + V) for the map and O(L + V * S) dense, V visited nodes, workspace O(L + h) for their depth h. The word view lives only during its call. Callbacks may read (including nested enumeration) and may throw; they must not mutate the trie. An explicit stack avoids recursion on long keys.
- `forEachPrefixOf(q, visit)` calls `visit(int length, lng multiplicity)` for every stored key that is a prefix of `q`, shortest first, with the same cancellation rule; `longestPrefix(q)` returns the length of the longest stored key that is a prefix of `q`, or `-1` when none is (the stored empty key gives 0). Both O(L * log(A + 1)) map, O(L) dense.

Copies are independent; moves transfer state; clear or assign a moved-from trie before other use.

Correctness: every live edge appends its byte to the root-to-node key; `terminal` is that key's multiplicity and `pass` equals `terminal` plus the children's `pass`. Insert changes exactly one root path, and the root-total check before incrementing prevents every subordinate overflow. Erase checks the whole terminal multiplicity first, then subtracts along the same path; a zero-pass suffix of the path has no live branch (a child's pass never exceeds its parent's), so unlinking it bottom-up and recycling its emptied nodes preserves all other keys, and recycled slots have no children and zero counters, which `newNode` relies on. Depth-first traversal emits a node's key before its children in increasing byte order, which is lexicographic order. `forEachPrefixOf` visits exactly the nodes on the root path of `q`, so every reported length is a stored prefix and none is skipped.

## Feature-to-test map

Runner: [`04-trie_tester.py`](<../../96-Local Testing/07-Strings/04-trie_tester.py>). One generic suite runs on `Trie`, `TrieDense<26, 'a'>`, `TrieDense<2, 'a'>`, `TrieDense<256, 0>` and `TrieDense<4, 'a'>` against an ordered-map multiset oracle; all checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| insert, count, countPrefix, search, size, empty, erase | Every multiset of binary keys of length at most 2 with multiplicities 0..2 (2187 states) per alphabet variant, with absent/insufficient erase and partial/final removals; 1000/7000/40000 seeded operations per variant. |
| Node, findNode, step | Every audit: `findNode` is `-1` exactly when no key has the prefix (root excepted), node `terminal`/`pass` equal the oracle's count and prefix count, a `step` walk reaches the same node; structural invariant walk (subtree sums, unique live children, cleared free slots, every slot accounted for). |
| newNode | Returns the last freed slot after a deep erase, appends `nodes.size()` on a fresh trie. |
| forEachPrefixOf, longestPrefix | Every audit compares against all stored prefixes of the query, including cancellation after the first call; deep-key longest prefix. |
| forEach (prefix, all) | Ordered enumeration against the oracle on every audit; all 256 one-byte keys plus two-byte complements; cancellation at first/second/middle/last; nested reads; propagated exception. |
| TrieDense alphabet | Queries and erases with out-of-alphabet bytes on `TrieDense<4, 'a'>` return absence; three checked probes reject out-of-range inserts below and above the alphabet and dense total overflow. |
| clear, copy, move, counts | Copy independence, move destination, moved-from clear, `INT64_MAX` totals and terminal counts, 100,000-byte keys (10,000 for 256-way dense rows) with erase and slot reuse. |

Checked builds run eight assertion probes (nonpositive insert/erase counts, total overflow for both layouts, dense symbols outside the alphabet).

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode quick --seed 1  # PASS, 2 configurations
python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode full --seed 1  # PASS, 3 configurations, 6,839,561 checks each, 8 assertion probes
CXX=g++-14 python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode full --seed 2  # PASS, 3 configurations, 7,168,862 checks each (`CXX=g++-14`)
python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 27,456,095 checks each
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizers
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 9 suites
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The suite was also run in full mode before any change (seed 1, all three configurations PASS) as the re-audit baseline.

## Benchmarks

[`04-trie_benchmark.py`](<../../96-Local Testing/07-Strings/04-trie_benchmark.py>) times a full insert, query (`count + 3 * countPrefix + longestPrefix` over stored keys and stored prefixes) and erase pipeline for `Trie` and `TrieDense<26, 'a'>`; checksums must agree. Run 2026-10-08, i9-11900H, GCC 16.2.1, `-O2 -DNDEBUG`, seed 20261008, one warmup and five rotating repetitions; local log `04-trie_benchmark.json` (git-ignored).

| Keys | Map build / query / erase / total ms | Dense build / query / erase / total ms |
|---|---|---|
| 1000, length 1..8, 26 letters | 0.437 / 0.453 / 0.607 / 1.518 | 0.276 / 0.041 / 0.165 / 0.496 |
| 200,000, length 1..8, 26 letters | 152.7 / 254.7 / 251.8 / 655.8 | 44.2 / 32.3 / 53.3 / 130.5 |
| 200,000, length 1..20, 4 letters | 230.8 / 345.6 / 333.5 / 910.7 | 106.2 / 90.4 / 119.9 / 318.7 |
| 20,000, length 1..200, 2 letters | 171.9 / 102.2 / 166.6 / 444.1 | 157.9 / 83.8 / 102.5 / 347.2 |

Dense rows are 1.3–5 times faster here at the cost of `4 * S` bytes per node; the map remains the default for byte keys and sparse alphabets. These are shared-host observations, not gates.

## Sources

Code is independently implemented.

- [OI Wiki, 字典树 (Trie)](https://oi-wiki.org/string/trie/): definition and transition model.
- [cp-algorithms, Aho–Corasick, construction of the trie](https://cp-algorithms.com/string/aho_corasick.html#construction-of-the-trie): dense-array versus map transition trade-off.
- [suisen `trie_array` / `trie_map`](https://suisen-cp.github.io/cp-library-cpp/) and [Nyaan `string/trie.hpp`](https://nyaannyaan.github.io/library/string/trie.hpp) (fetched 2026-10-08): fixed-alphabet arrays and a node cursor (`move`/`find`).
- [ei1333 `structure/trie/trie.hpp`](https://ei1333.github.io/library/structure/trie/trie.hpp) (fetched 2026-10-08): `query(str, f)` visiting stored prefixes of a query, the basis of `forEachPrefixOf`; parity with the planned Python `longestPrefix`.
- Preserved `OLD/Team Notebook/src/algs.cpp` lines 2374–2397 and `algsbetter.cpp` lines 3332–3402 (lowercase-26 trie with terminal/pass counters and erase-one) are accounted for by `TrieDense<26, 'a'>`; original bytes unchanged.
- Catalog sweep 2026-10-08 in [00-sources.md](00-sources.md).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): kth key and rank by multiplicity (no catalog source; Data Structures binary trie and row `24` own ranking), per-key id lists (Aho–Corasick `06` owns pattern ids), parent links.
- Radix/compressed and persistent tries remain row `25`; binary integer/XOR tries remain Data Structures; `06` `fromTrie` may consume this node cursor.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-09-28: map `Trie` verified under the previous system (P016). 2026-10-08 re-audit: one `BasicTrie` template now provides `Trie` and `TrieDense`; `step`, `forEachPrefixOf` and `longestPrefix` added; `/reaudit-review` findings 5 (Node/newNode/findNode tests), 11 (`L`, `S` and fanout `A` symbols, clarified after review) and 12 (closing braces) fixed; contracts moved out of the header.
