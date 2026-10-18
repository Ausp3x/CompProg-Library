# 07 Strings — notes

The inventory lists operations; this file holds the rules every row must respect. Per-header evidence documents record what was verified and are authoritative for their headers.

## Representation

- Bytes are the default alphabet; byte symbols are read as `unsigned char` (0..255). Every header states whether it accepts bytes, code points or integer alphabets, whether it compresses the alphabet and whether it uses a sentinel. Empty inputs, embedded NUL and the full byte alphabet are valid inside the declared domain.
- Sentinel-free is the house convention for existing Basic headers (`01`, `02`, `05`, `07`): no caller sentinel, the empty suffix is omitted from `sa` but valid for `lce`, `z[0] = n`, Manacher `even` has `n + 1` entries. New headers follow the same convention unless a sentinel is part of the definition (`29` BWT, `36`, `46`), in which case the sentinel or primary-index convention is named in the row.
- Transition storage and alphabet size appear in time and memory bounds (`O((m + 1) * sigma)` automata, sparse versus dense Aho tables). Map-based transitions are the default; dense tables are an explicit opt-in.
- Sizes are `< INT_MAX` symbols; counts are exact `lng` unless the row says modular or saturated. Counting APIs state exact, modular or saturated arithmetic.

## Problem classes

- Substring, subsequence, edit-distance and Hamming-distance problems are distinct families and never share a function name. Longest common substring belongs to suffix indexes (`07`, `28`); longest common subsequence belongs to `19`; subsequence membership and counting belong to `20`.
- Witnesses and ties use a documented convention: ascending starts, overlaps included, leftmost then lexicographically smallest unless a row states otherwise.
- Transposition distances state which definition they implement (optimal string alignment versus unrestricted Damerau–Levenshtein). Alignment scores (Needleman–Wunsch, Smith–Waterman, Gotoh) are maximization problems with explicit scoring matrices and gap penalties, separate from edit-distance minimization.

## Hashes and randomness

- Hashes are Monte Carlo fingerprints and never proof of equality. A hash-based API says so in its name or documentation (`maybePalindrome`, `StringHash::lcp`). Exact equality or LCE uses deterministic verification or an exact index. Fingerprints are never reported as proof.
- Randomized runtime and randomized correctness are separate classifications: Las Vegas (exact answer, probabilistic time), Monte Carlo (probabilistic answer), high-probability bounds (`47` Optimal Dynamic Strings) are each named explicitly.
- Static fingerprint contexts own their bases; digests from different contexts are comparable only under identical base and encoding. Double-prime is the default; unsigned-word wrap has demonstrated structured collisions; the Mersenne `2^61 - 1` field variant is a speed option with a single large modulus. Collision bounds are in [03-stringhash.md](03-stringhash.md).
- Dynamic hashes (`23`) inherit the static collision model; randomized base lifecycle must be documented per structure.

## Ownership inside the folder

- Distinct-substring counting and kth-substring queries belong to `10` and `22`, not to the quadratic prefix/Z reductions. Substring shortest-border and shortest-period queries belong to `27` and state preprocessing, query and output costs separately. Shortest absent subsequence belongs to `20`; minimal absent words and minimal unique substrings to `22`.
- Palindromes: static radii `05`, static queries `08`, eertree `13`, range and characteristic queries plus palindrome pairs `39`, double-ended and rollback structures `45`. Longest palindromic subsequence and counting palindromic subsequences are subsequence DP in `19`.
- Dynamic pattern sets (`34`) and dynamic text indexes (`35`, `44`, `45`, `47`) state the exact supported edit model (append, prepend, pop, insert, erase, split, join, rollback, persistence) before promising bounds; append-only analyses do not survive deletions.
- Generalized indexes over several strings (`28`) need separators that cannot collide with the input alphabet; the existing `longestCommonSubstring` in `07` compresses the union alphabet to positive ranks and uses zero as a fresh separator.

## Ownership across folders

- Python (`08-Python`) implements documented subsets of this folder (`_23_trie.py`, `_24_aho.py`, `_25_suffix.py`, `_12_dp.py` LCS/edit distance); reductions are stated there, never silent.

## Verification policy

- Differentially test matching against naive scans, suffix order against sorted suffixes, LCP against direct scans, substring and palindrome counts against sets, reconstruction by recomputing its input arrays, compression by decode roundtrip and automata by brute-force language enumeration on small alphabets.
- Include all-equal, periodic, Fibonacci/Thue–Morse (worst cases for runs and palindromes; generators in `50`), adversarial alphabets, repeated patterns, empty patterns and edits that invalidate old states.

## Legacy obligations

- `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `OLD/Team Notebook/src/misc/old_zalgo.cpp` contain legacy trie, KMP and Z routines. The Z routine is accounted for in `02`; the prefix/KMP and trie routines are accounted for in `01` and `04` (see [01-prefixfunction.md](01-prefixfunction.md), [04-trie.md](04-trie.md)).
- This folder has no `97-Legacy/` directory; no row carries `legacy-reference` status.

## P016 package record (ST01, ST02, ST19)

Re-audit research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `01`: Gray-string DP over `prefixAutomaton` is problem-specific; distinct-substring counting by prefix function is quadratic and owned by `10`/`22`; suisen `min_period` returning the root string is `27` `primitiveRoot`.
- `02`: incremental Z is `30`; `is_substring` is `16` (or `!zOccurrences(...).empty()`); inverse Z is `31`.
- `03`: appending to a built hash breaks O(1) reversed prefixes and is owned by `23` (DequeHash, PointUpdateHash); `StringHashField<F>` adds a policy layer for modint types without a new guarantee; several Mersenne bases are unnecessary at `(L-1)/2^61` per comparison; `std::hash` for digests is left to callers (ordering covers `set`/`map`); lexicographic substring comparison is `22` `compareSubstrings` and needs the text; Rabin–Karp windows are `16`.
- `04`: kth key and rank by multiplicity have no catalog source and belong to the Data Structures binary trie and `24` ranking; per-key id lists belong to `06` pattern ids; parent links are unnecessary for the cursor (`fromTrie` in `06` can rebuild them by BFS).
- `05`: the Library Checker `2n - 1` combined length array is a radius conversion owned by `08`; `enumeratePalindromes` is a loop over `oddInterval`/`evenInterval`; an odd-only switch saves one linear pass only.
- `09`: suisen `RunLengthEncoder` push/pop has one unverified source and is a two-line idiom on `vector<pair<T, lng>>`.

Future-owner handoff: ST09 owns dynamic hashing, ST25 minimum-alphabet reconstruction, ST22 compressed dictionaries.

## P017 package record (ST03)

Re-audit research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `06`: a parent field (Library Checker `aho_corasick` output) is recoverable in `O(V)` from `nodes[u].next`; scans resuming from a given state (ei1333 `move`) are `step` plus `matchCount`; persistent-array transitions for `fromTrie` (maspypy) are replaced by the dense build over at most 256 bytes.
- `07`: `22` `frequencyStatistics` must not duplicate `longestRepeated(k)` (at least `k` occurrences, now in `07`); substring comparison, kth substrings, all-pairs LCP and occurrence ranges by position stay in `22`; distinct-substring counting in `10`; cyclic shifts in `15`.
- `08`: longest palindromic prefix/suffix is `05` `longestStarting()[0]`/`longestEnding()[n-1]`; inverse Manacher is `31`; "extend to palindrome" and double palindromes are problem-specific uses of those arrays.

ST04/ST05/ST06 retain suffix automaton/tree/SA-IS ownership; ST11 owns generalized multiple-string indexes. ST13 owns dynamic Aho updates and can reuse stable state/failure/output interfaces, with state IDs invalidated by `clear`. ST07/ST16/ST28 retain eertree/advanced static/dynamic palindrome ownership; longest-palindrome queries constrained to a substring are listed under ST16.

## P081 package record (ST04, ST06, ST33)

Research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `10`: `locate(L, R)` and `prefixState` (maspypy) are substring loci, owned by `11` `weightedAncestor`; `lengthRange` is `{minimalLength(v), nodes[v].len}`; the SAM smallest cyclic shift belongs to `15`; many-string LCS is `28`; longest repeated substring is `07`. `kthSubstringDistinct`/`kthSubstring` are the automaton variants; `22` keeps the suffix-array versions. `lexicographicWalk` is defined here as the ordered enumeration of distinct substrings with early stop; `cyclicShiftOccurrences` counts equal rotations once (Codeforces 235C convention).
- `12`: `saisSentinelFree` is not a separate function: `sais` itself is sentinel-free (ACL convention, `n` entries); callers wanting the empty suffix prepend `n`. The inverse suffix array is one loop over `sa` (`07` owns `rank`); prefix doubling stays in `07`, so there is no doubling fallback threshold here (the naive threshold is measured).
- `18`: Hyyrö banded and transposition bit-vectors, Masek–Paterson Four Russians, concave gap penalties and Levenshtein automata (Schulz–Mihov) are not adopted (niche or theoretical; an automaton would belong to `17`/`24`); Ukkonen band doubling is subsumed by `diagonalEditDistance`; LCE-accelerated Landau–Vishkin `O(n + d^2)` is `38`. Scored alignments other than `hirschberg` return scores only; `hirschberg(a, b, score, gap)` is the Needleman–Wunsch witness. Hamming distance is `38`; LCS and indel distance are `19`.

Handoffs: `22` may reuse `distinctSubstringsOnline` for `distinctSubstringsPerPrefix`; `28` generalizes `BasicSuffixAutomaton`.

## P082 package record (ST05)

Research outcomes (sources in [00-sources.md](00-sources.md)); left out, one line each:

- `11`: Gusfield maximal pairs (Wikipedia only, `Θ(n + z)` output) are niche and enumerable per node from `maximalRepeats` and `occurrences`; an occurrence count is `r - l` of the locus node; the suffix tree as the suffix-link tree of the reversed text's automaton (OI Wiki) is a second construction of the same topology, owned by `10` `suffixLinkTree`; `parent` is the `Node::parent` field and maspypy `get_suffix_positions` is `leaf`; `O(1)` weighted-ancestor structures (GLN14, BKPR21) belong to `51`, so `weightedAncestor` uses `O(log n)` jump pointers.

Handoffs: `28` GeneralizedSuffixTree can reuse the LCP-interval build over a separator-joined text; `33` Ukkonen should keep the `{node, length}` locus and `step` conventions.

## P083 package record (ST07, ST34)

Research outcomes (sources in [00-sources.md](00-sources.md)); left out, one line each:

- `13`: `seriesLink` is the `Node::series` field; a raw per-node count field is recomputed by `occurrenceCounts` from `suffix`; k-factorization (Rubinchik–Shur) is rare and bounded by parity plus `minPalindromePartition`; per-symbol DP callbacks are subsumed by `palindromicFactorization`; rich-string enumeration is research-only and persistence is `45`; listing the end positions of one palindrome has no catalog source (walk the link tree).
- `14`: Lyndon-word rank/unrank/k-th word (arXiv 1510.02637) need big integers and appear in no competitive-programming catalog.
- `15`: `minCyclicShift` was merged into `minRotationIndices` (every catalog treats them as one operation); minimal rotation of every prefix has no catalog source (`22` owns per-prefix minimal suffixes); the rotation period is `minRotationIndices().second` (or `27` `primitiveRoot`); cyclic comparison is `rotationEquivalent`/`canonicalRotation`; the row's "Booth" became the two-pointer minimum expression with the same bound.
- `19`: longest common increasing subsequence has no fetched catalog source and no standard linear-memory witness (LIS lives in Miscellaneous `08`); a full LCS table is internal (semi-local queries are `48`); counting LCS strings and the lexicographically smallest LCS have no catalog source; range distinct-subsequence counting is a segment-tree application.

Handoffs: `39` should reuse `13` `palindromicLength`; `22` `lexMinSuffixPerPrefix` is `IncrementalLyndon::minSuffix`; `26` runs can consume `lyndonArray`/`lyndonTree`; `41` de Bruijn can consume `nextLyndonWord`.

## P084 package record (ST08, ST20, ST23)

Research outcomes (sources in [00-sources.md](00-sources.md)); left out, one line each:

- `16`: bad-character-only Boyer–Moore is subsumed by `horspool`; the b5s/BMHBNFS hybrid (CPython) combines the `horspool` and `sunday` shifts and keeps their `O(n * m)` worst case, while the dispatcher needs a linear worst case (`boyerMoore` with Galil); Turbo-BM stays in `49` and Breslauer's two-way in `43`; last occurrence is the last entry of a search's result and a range search is a search on `text.substr`/`string_view`, neither listed by any catalog; multi-pattern Rabin–Karp has no fetched catalog source and multi-pattern matching is `06`.
- `17`: multi-pattern Shift-And overlaps Aho–Corasick (`06`); optional/repeatable symbols, bounded gaps and Glushkov regex simulation are regular-expression features (`24`); BNDM is owned by `49`.
- `20`: distinct common subsequences and the shortest uncommon subsequence of two texts are product-automaton DPs over `next` of two automata (one blog source, `O(n * m * S)` state pairs), left to problem code; the lexicographically smallest length-k subsequence is the `O(n)` monotone-stack greedy (Miscellaneous `08`); lexicographic rank of a subsequence has no catalog source (it is the inverse walk of `kthDistinctSubsequence`); a persistent-segment-tree transition table adds no query the sparse variant lacks.
- `29`: the extended BWT of a string collection and its inverse are research constructions with no contest catalog; the sort transform of order k is niche; the run count `r` is a one-line loop owned by `46`; dynamic BWT updates have no stated bound and no contest use; move-to-front is a separate pipeline stage (Wikipedia, Princeton COS226) and belongs with Huffman coding in Miscellaneous.

Handoffs: `36` FM-index builds on `bwt`/`lfMapping`; `46` counts runs of `bwt`/`bwtPrimary`; `38` Hamming matching can compare with `bitapHamming` for small `k`; `24` may reuse `PatternMasks` for character classes.

## P085 package record (ST09, ST21)

Research outcomes (sources in [00-sources.md](00-sources.md)); left out, one line each:

- `21`: Kalai's one-convolution randomized matcher handles pattern-only wildcards, a domain `wildcardMatch` covers with the same bound and a constant factor (seen in search results only); the single-backtrack iterative glob (Krauss, Cantatore) is `O(n * m)` worst case and returns the same answers as the greedy `globMatch`; dynamic wildcard matching (arXiv 2601.16182) and wildcard indexing (arXiv 1110.5236) are research structures with no catalog source; the row's "variable-length star DP" is `GlobAutomaton`'s bit-parallel simulation, and `globMatch` uses greedy leftmost segments, which are asymptotically better.
- `22`: maspypy `suffix_lcp_change` (one source, a compact form of rank-0 LCPs) and `non_dominated_suffix` (niche, would belong to `14`) are not adopted; the basic substring structure (arXiv 2312.11873) is esoteric; ordered-set queries over arbitrary patterns (suisen `SubstringSet` lower bounds) are `07` `patternRange` plus the ranks here; maspypy's two-string `all_pairs_lcp` table is an `O(n * m)` DP, not a suffix-array query.
- `23`: `reverseGet`, `lcp` and `compare` on `DequeHash` are left to `45` (double-ended palindrome structures); multi-base hashing is a template choice, not an operation; period checks are digest equalities of `get`.

Handoffs: `24` may compile globs through `GlobAutomaton`'s `PatternMasks`; `28` generalizes `StringCollection` into a generalized suffix index; `45` and `47` own double-ended and `O(log n)` dynamic LCE structures.

