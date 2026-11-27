# 07 Strings — Notes

The inventory lists operations; this file holds the rules every row follows. Each header's evidence document records what was verified and is authoritative for that header.

## Representation

- Bytes are the default alphabet, read as `unsigned char` (0..255). Every header states whether it accepts bytes, code points or integer alphabets, whether it compresses the alphabet and whether it uses a sentinel. Empty inputs, embedded NUL and the full byte alphabet are valid inside the declared domain.
- Headers are sentinel-free by default (`01`, `02`, `05`, `07`, `11`, `12`): the caller adds no sentinel, `sa` omits the empty suffix while `lce` accepts it, `z[0] = n`, and Manacher `even` has `n + 1` entries. A header whose definition needs a sentinel (`29` BWT, `36`, `46`) names its sentinel or primary-index convention in its row.
- Time and memory bounds include transition storage and alphabet size (`O((m + 1) * sigma)` automata, sparse versus dense Aho tables). Map transitions are the default; dense tables are an explicit opt-in.
- Sizes are below `INT_MAX` symbols. Counts are exact `lng` unless the row says modular or saturated, and every counting API states which of the three it uses.

## Problem Classes

- Substring, subsequence, edit-distance and Hamming-distance problems are separate families and never share a function name.
- Longest common substring belongs to the suffix indexes (`07`, `28`). Its Monte Carlo hash and binary-search form belongs to `03` for two strings (adopted 2026-11-08) and to `28` for k strings (user decision 2026-11-14): `03` stays two-string like `07`, and the `28` form takes a `vector` of `StringHash` objects, so the shared name takes other argument types (`02-structure.md`, Operation Names).
- Longest common subsequence and whole-text counting DPs belong to `19`; subsequence membership and per-state counting belong to `20`.
- Witnesses and ties follow one convention unless a row states otherwise: ascending starts, overlaps included, leftmost, then lexicographically smallest.
- Transposition distances state which definition they implement (optimal string alignment or unrestricted Damerau–Levenshtein). Alignment scores (Needleman–Wunsch, Smith–Waterman, Gotoh) are maximizations with explicit scoring matrices and gap penalties, separate from edit-distance minimization.

## Hashes and Randomness

- Hashes are Monte Carlo fingerprints, never proof of equality. A hash-based API says so in its name or documentation (`maybePalindrome`, `StringHash::lcp`). Exact equality or LCE uses deterministic verification or an exact index.
- High-probability bounds (`47`, Optimal Dynamic Strings) are named separately from the Monte Carlo and Las Vegas classification (`03-cpp.md`, API Contracts).
- A static fingerprint context owns its bases; digests from different contexts compare only under identical base and encoding. Double prime is the default; unsigned-word wrap has demonstrated structured collisions; the Mersenne `2^61 - 1` field is a speed option with one large modulus. Collision bounds are in [03-stringhash.md](03-stringhash.md).
- Dynamic hashes (`23`) inherit the static collision model, and each structure documents the lifecycle of its random base.

## Ownership inside the Folder

- Distinct-substring counting (`10`, `11`, `22`) and kth-substring queries (`10`, `22`) belong to the suffix indexes, not to quadratic prefix-function or Z reductions. The exception is `02` `DistinctSubstringsDeque` (adopted 2026-11-08), since no suffix index supports edits and pops at both ends.
- Substring shortest-border and shortest-period queries belong to `27`, which states preprocessing, query and output costs separately. Shortest absent subsequence belongs to `20`; minimal absent words and minimal unique substrings belong to `22`.
- Palindromes: static radii `05`, static queries `08`, eertree `13`, range and characteristic queries and palindrome pairs `39`, double-ended and rollback structures `45`. Longest palindromic subsequence and palindromic-subsequence counting are subsequence DPs in `19`.
- Dynamic pattern sets (`34`) and dynamic text indexes (`35`, `44`, `45`, `47`) state their exact edit model (append, prepend, pop, insert, erase, split, join, rollback, persistence) before promising bounds; append-only analyses do not survive deletions.
- Generalized indexes over several strings (`28`) need separators that cannot collide with the input alphabet. `07` `longestCommonSubstring` codes the union alphabet to positive values (shift or ranks, [07-suffixarray.md](07-suffixarray.md)) and uses zero as a fresh separator.
- Each callable implementation has one owner: offline suffix tree `11`, Ukkonen `33`, two-way `16`, rare linear variants `43`, LZ `42`, grammars `37`, BWT `29`, FM-index and XBW `36`, run-length BWT and r-index `46`.

## Ownership across Folders

- Data Structures engines (DSU, Fenwick tree, sparse table, treap and splay tree, wavelet matrix, succinct rank/select) serve `07`, `11`, `22`, `23`, `35`, `36`, `46` and `48`.
- Mathematics `13-convolution.hpp` (batch MA06) owns convolution, which `21` and `38` require. Mathematics generating functions own necklace and Lyndon counting (Pólya–Burnside).
- Miscellaneous owns Huffman and optimal prefix coding (`40-optimal_merge.hpp`), digit DP (`31-digitdp.hpp`; `24` only supplies automata to it) and expression parsing (`37-expression_parser.hpp`).
- Graphs `07-lca.hpp` and `10-euleriantrail.hpp` serve `27` `BorderTree` and `41` as engines or oracles.
- [08-Python](../../08-Python/00-index.md) owns the Python subsets of this folder (`_10_strings.py`, `_12_dp.py`, `_23_trie.py`, `_24_aho.py`, `_25_suffix.py`, `_53_eertree.py`, `_54_lyndon.py`).

## Verification Policy

- Test differentially: matching against naive scans, suffix order against sorted suffixes, LCP against direct scans, substring and palindrome counts against sets, reconstruction by recomputing its input arrays, compression by decode roundtrip, and automata by brute-force language enumeration on small alphabets.
- Include all-equal, periodic and Fibonacci or Thue–Morse texts (worst cases for runs and palindromes; generators in `50`), adversarial alphabets, repeated patterns, empty patterns and edits that invalidate old states.

## Legacy Obligations

- `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` contain a legacy trie, accounted for in `04` ([04-trie.md](04-trie.md)). `OLD/Team Notebook/src/misc/old_zalgo.cpp` is accounted for in `02` ([02-z.md](02-z.md)) and `old_lcssubstr.py` in `07` ([07-suffixarray.md](07-suffixarray.md)).
- This folder has no `97-Legacy/` directory and no `legacy-reference` row.

## P016 Package Record (ST01, ST02, ST19)

Operations left out by the re-audit research (sources in [00-sources.md](00-sources.md)):

- `01`: Distinct-substring counting by prefix function is quadratic and owned by `10` and `22`; the double-ended Z form is row `02` `DistinctSubstringsDeque` (adopted 2026-11-08). suisen `min_period` returning the root string is `27` `wordRoot`. The naive O(n^3) and O(n^2) prefix functions are dominated: no lighter, wider, faster or more capable form (ST01 re-sweep 2026-11-12). Online pattern growth (emthrm `MorrisPratt::add`) is `KmpMatcher::pushBack`, which resets the stream (user decision 2026-11-13). emthrm `period(idx)` stays an alias for `idx - pi[idx - 1]`, so `prefixPeriod` takes no length parameter (user decision 2026-11-13).
- `02`: Incremental Z is `30`; `is_substring` is `16` (or `!zOccurrences(...).empty()`); inverse Z is `31`. Naive O(n^2) Z and Z by hash with binary search are dominated (ST01 re-sweep 2026-11-12).
- `03`: Appending to a built hash breaks O(1) reversed prefixes and is owned by `23` (`DequeHash`, `PointUpdateHash`). `StringHashField<F>` would add a policy layer for modint types without a new guarantee. Several Mersenne bases are unnecessary at `(L-1)/2^61` per comparison. `std::hash` for digests is left to callers, since ordering covers `set` and `map`. Lexicographic substring comparison is `22` `compareSubstrings` and needs the text; Rabin–Karp windows are `16`. ST02 re-sweep 2026-11-14: the tree-path rolling hash (maspypy `Rolling_Hash_on_Tree`) needs Graphs LCA or HLD and stays the unowned handoff of `omissions.md`. Single 32-bit-prime hashing and KACTL's modulo `2^64 - 1` ring are dominated by `KIND = 0` and `StringHash61` (same cost, a root bound). Distinct-substring counting by hash is quadratic and owned by `10` and `22`; longest palindrome by hash is dominated by `05` and `08`. k-mismatch matching by hash LCE is a lighter form of `38` `kMismatch` and is handed to row `38`. The k-string longest common substring by hash is row `28` (user decision 2026-11-14).
- `04`: Kth key and rank by multiplicity have no catalog source and belong to the Data Structures binary trie and `24` ranking. Per-key id lists belong to `06` pattern ids. Parent links are unnecessary for the cursor; `fromTrie` in `06` can rebuild them by BFS. ST02 re-sweep 2026-11-14: nothing is missing and no catalog has a slower string-trie algorithm. Binary-trie applications and the 01-trie merge belong to Data Structures `26` and `72`; persistent tries belong to Data Structures `26` (`PersistentBinaryTrie`) and to row `25` (`PersistentRadixTrie`).
- `05`: The Library Checker `2n - 1` combined length array is a radius conversion; `enumeratePalindromes` is a loop over `oddInterval` and `evenInterval`; an odd-only switch saves only one linear pass. ST02 re-sweep 2026-11-14: naive O(n^2) center expansion is dominated (not half the lines of the predicate loop), and so are the O(n log n) hash and suffix-index radius methods (heavier, slower, and the hash form Monte Carlo).
- `09`: The container constructor of suisen's `RunLengthEncoder` is `ctor (sequence)` (user decision 2026-11-14). Its other members stay out as other forms: `front`, `back` and iterators read the public `runs`, and `swap` and `clear` are assignment. `popBack` asserts `count <= length()` instead of clamping (user decision 2026-11-14). The ST19 re-sweep found no front-side variant and no slower alternative.

Handoffs: `31` owns minimum-alphabet reconstruction and `25` compressed dictionaries.

## P017 Package Record (ST03)

Operations left out by the re-audit research (sources in [00-sources.md](00-sources.md)):

- `06`: A parent field (Library Checker `aho_corasick` output) is recoverable in `O(V)` from `nodes[u].next`. Scans resuming from a given state (ei1333 `move`) are `step` plus `matchCount`. maspypy's persistent-array transitions for `fromTrie` are replaced by the dense build over at most 256 bytes. `lastOccurrences` is unsourced and is `firstOccurrences` on the reversed text and patterns (`n - first - length`).
- `07`: `22` `frequencyStatistics` must not duplicate `longestRepeated(k)` (at least `k` occurrences, in `07`). Substring comparison, kth substrings, all-pairs LCP and occurrence ranges by position stay in `22`; distinct-substring counting in `10`; cyclic shifts in `15`. Dense symbol ranges (`max - min < max(4n, 256)`) are shifted rather than compressed (user refinement 2026-10-19). DC3 (skew) construction is not adopted: it is also `O(n)` and slower than SA-IS in practice. There is no size dispatch to `doubling`: it wins by up to 35% on short random texts but loses by 2–3 times on repetitive texts of the same lengths ([benchmark](07-suffixarray.md#benchmarks)).
- `08`: The longest palindromic prefix and suffix are `05` `longestStarting()[0]` and `longestEnding()[n-1]`; inverse Manacher is `31`; range-restricted longest, count and distinct palindrome queries are `39`.

Handoffs: `34` can reuse the stable state, failure and output interfaces of `06`; `clear` invalidates state ids.

## P081 Package Record (ST04, ST06, ST33)

Operations left out by the research (sources in [00-sources.md](00-sources.md)):

- `10`: `locate(L, R)` and `prefixState` (maspypy) are substring loci, owned by `11` `weightedAncestor`. `lengthRange` is `{minimalLength(v), nodes[v].len}`. The automaton's smallest cyclic shift belongs to `15`, many-string LCS to `28` and longest repeated substring to `07`. `kthSubstringDistinct` and `kthSubstring` are the automaton variants; `22` keeps the suffix-array versions. `lexicographicWalk` is the ordered enumeration of distinct substrings with early stop; `cyclicShiftOccurrences` counts equal rotations once (Codeforces 235C convention).
- `12`: There is no separate `saisSentinelFree`: `sais` itself is sentinel-free (ACL convention, `n` entries), and callers wanting the empty suffix prepend `n`. The inverse suffix array is one loop over `sa` (`07` owns `rank`). Prefix doubling stays in `07`, so there is no doubling fallback threshold here; the naive threshold is measured.
- `18`: The Hyyrö banded and transposition bit-vectors were adopted on 2026-11-08 into row `18` (`bandedMyersBitVector`, `optimalStringAlignmentBitVector`). Masek–Paterson Four Russians is not adopted (niche or theoretical). Scalar Ukkonen band doubling is subsumed by `diagonalEditDistance`. LCE-accelerated Landau–Vishkin `O(n + d^2)` is `38`. Scored alignments other than `hirschberg` return scores only; `hirschberg(a, b, score, gap)` is the Needleman–Wunsch witness. Hamming distance is `38`; LCS and indel distance are `19`.

Handoffs: `28` generalizes `BasicSuffixAutomaton`.

## P082 Package Record (ST05)

Operations left out by the research (sources in [00-sources.md](00-sources.md)):

- `11`: An occurrence count is `r - l` of the locus node. The suffix tree built as the suffix-link tree of the reversed text's automaton (OI Wiki) is a second construction of the same topology, owned by `10` `suffixLinkTree`. `parent` is the `Node::parent` field, and maspypy `get_suffix_positions` is `leaf`. `O(1)` weighted-ancestor structures (GLN14, BKPR21) belong to `51`, so `weightedAncestor` uses `O(log n)` jump pointers. The first or last occurrence of a locus (Wikipedia) is `10` `firstOccurrence` and `lastOccurrence` or the ends of `occurrences(v)`; per-node extremes would store `2V` more `int`s for an operation `10` owns.

Handoffs: `28` `GeneralizedSuffixTree` can reuse the LCP-interval build over a separator-joined text. `33` Ukkonen should keep the `{node, length}` locus and `step` conventions. Data Structures `54` `LinearRMQ`, once verified, may replace the inline `split` window RMQ of `lca` if it is not slower.

## P083 Package Record (ST07, ST34)

Operations left out by the research (sources in [00-sources.md](00-sources.md)):

- `13`: `seriesLink` is the `Node::series` field. A raw per-node count field is recomputed by `occurrenceCounts` from `suffix`. k-factorization (Rubinchik–Shur) is rare and bounded by parity plus `minPalindromePartition`. Per-symbol DP callbacks are subsumed by `palindromicFactorization`. Rich-string enumeration is research-only, and persistence is `45`. Listing the end positions of one palindrome has no catalog source (walk the link tree).
- `14`: Lyndon-word rank, unrank and k-th word (arXiv 1510.02637) need big integers and appear in no competitive-programming catalog. De Bruijn decoding (prefix counting of Lyndon words) was adopted on 2026-11-08 into row `41` (`deBruijnDecode`).
- `15`: `minCyclicShift` was merged into `minRotationIndices`, since every catalog treats them as one operation. Minimal rotation of every prefix has no catalog source (`22` owns per-prefix minimal suffixes). The rotation period is `minRotationIndices().second` (or `27` `wordRoot`); cyclic comparison is `rotationEquivalent` or `canonicalRotation`. The row's "Booth" became the two-pointer minimum expression with the same bound.
- `19`: A full LCS table is internal (semi-local queries are `48`). Counting LCS strings and the lexicographically smallest LCS have no catalog source. Range distinct-subsequence counting is a segment-tree application.

Handoffs: `26` runs can consume `lyndonArray` and `lyndonTree`; `41` de Bruijn can consume `nextLyndonWord`.

## P084 Package Record (ST08, ST20, ST23)

Operations left out by the research (sources in [00-sources.md](00-sources.md)):

- `16`: Bad-character-only Boyer–Moore is subsumed by `horspool`. The b5s (BMHBNFS) hybrid of CPython combines the `horspool` and `sunday` shifts and keeps their `O(n * m)` worst case, while the dispatcher needs a linear worst case (`boyerMoore` with Galil). Turbo-BM stays in `49` and Breslauer's two-way in `43`. Last occurrence is the last entry of a search's result, and a range search is a search on `text.substr` or a `string_view`; no catalog lists either. Multi-pattern Rabin–Karp has no fetched catalog source, and multi-pattern matching is `06`.
- `17`: Multi-pattern Shift-And overlaps Aho–Corasick (`06`). Optional or repeatable symbols, bounded gaps and Glushkov regex simulation are regular-expression features (`24`). BNDM is owned by `49`.
- `20`: The shortest uncommon subsequence of two texts (OI Wiki, HEOI2015) was adopted on 2026-11-08 into row `20` (`shortestUncommonSubsequence`). Lexicographic rank of a subsequence has no catalog source; it is the inverse walk of `kthDistinctSubsequence`. A persistent-segment-tree transition table adds no query the sparse variant lacks.
- `29`: The extended BWT of a string collection and its inverse are research constructions with no contest catalog. The sort transform of order k is niche. The run count `r` is a one-line loop owned by `46`. Dynamic BWT updates (prepending a symbol) were adopted on 2026-11-08 into row `36` (`DynamicFmIndex`).

Handoffs: `36` FM-index builds on `bwt` and `lfMapping`; `46` counts runs of `bwt` and `bwtPrimary`; `38` Hamming matching can compare with `bitapHamming` for small `k`; `24` may reuse `PatternMasks` for character classes.

## P085 Package Record (ST09, ST21)

Operations left out by the research (sources in [00-sources.md](00-sources.md)):

- `21`: Kalai's one-convolution randomized matcher handles pattern-only wildcards, a domain `wildcardMatch` covers with the same bound and a constant factor (seen in search results only). The single-backtrack iterative glob (Krauss, Cantatore) is `O(n * m)` in the worst case and returns the same answers as the greedy `globMatch`. Dynamic wildcard matching (arXiv 2601.16182) and wildcard indexing (arXiv 1110.5236) are research structures with no catalog source. The row's "variable-length star DP" is `GlobAutomaton`'s bit-parallel simulation, and `globMatch` uses greedy leftmost segments, which are asymptotically better.
- `22`: maspypy's two-string `all_pairs_lcp` table is an `O(n * m)` DP, not a suffix-array query.
- `23`: `reverseGet`, `lcp` and `compare` on `DequeHash` are left to `45` (double-ended palindrome structures). Multi-base hashing is a template choice, not an operation. Period checks are digest equalities of `get`.

Handoffs: `24` may compile globs through `GlobAutomaton`'s `PatternMasks`; `28` generalizes `StringCollection` into a generalized suffix index; `45` and `47` own the double-ended and `O(log n)` dynamic LCE structures.

## Omission Sweep 2026-11-08

The 2026-11-08 sweep left out exact duplicates, second implementations and items another row owns (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions). Each item names where it is implemented instead.

- `GeneralizedSuffixAutomaton`: step, suffixLinkTree, distinctSubstrings (MAS, OI): row `10` `BasicSuffixAutomaton` owns them, and row `28` generalizes it.
- `palindromicLength` (per prefix) (OI): verified row `13` owns it; P209 drops the row `39` copy.

## P275 Re-Review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per Operation) with its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). The items it returned are now operations of rows `08`, `09`, `11`, `20`, `22`, `39`, `50`, `55` and `56` and of Miscellaneous rows `08` and `28`, and their omission bullets were removed; every other omission recorded here stands ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)).

## Omission Sweep 2026-11-27

Left out from the candidates of the 2026-11-27 catalog survey (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`; user decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions): second implementations and slower algorithms that Priority does not admit.

- McCreight's suffix-tree construction (ITMO, GUSFIELD): the same tree as row 11 `SuffixTree`, neither lighter nor more capable than row 33's dependency-free online `UkkonenSuffixTree` (`03-cpp.md`, One API per Operation).
- Weiner's suffix-tree construction (ITMO, GUSFIELD): the same tree offline as row 11; its online prepend is row 10 `BasicSuffixAutomaton::extend` on the reversed text and row 35 `SuffixBalancedTree::pushFront` (One API per Operation).
- Farach's suffix-tree construction (ITMO, GUSFIELD): linear for integer alphabets, the domain, bound and result of row 11 `SuffixTree` built through row 12's integer-alphabet `sais` (One API per Operation).

## Omission Sweep 2026-11-28

Left out from the candidates of the 2026-11-28 sample of the sources the user placed (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`, Sampled and Applied 2026-11-28).

- Fingerprint, triangle-inequality and inverted-index similarity search (MEHTASAHNI): Other indexes for the threshold search bitboundSearch (DS2-087) answers, with no better worst-case bound (One API per Operation); the XOR filter is Misc 45 XorFilter.
- The position heap (MEHTASAHNI): A second implementation of Strings 11 SuffixTree locate and occurrences (and the suffix array search) with no better bound (03-cpp.md, One API per Operation).
- Parallel text algorithms (JEWELS): Single-threaded contest execution is the baseline (01-principles.md, Profiles); run sequentially each PRAM algorithm is a second implementation of a kept operation (DictionaryOfBasicFactors, prefixFunction, huffmanCodes, levenshtein; One API per Operation).
