# 07 Strings — notes

Contracts moved out of the inventory on 2026-10-06. The inventory lists operations; this file holds the rules every row must respect. Per-header evidence documents (`01`–`09`) record what was verified and are authoritative for the nine Basic headers.

## Representation

- Bytes are the default alphabet; byte symbols are read as `unsigned char` (0..255). Every header states whether it accepts bytes, code points or integer alphabets, whether it compresses the alphabet and whether it uses a sentinel. Empty inputs, embedded NUL and the full byte alphabet are valid inside the declared domain.
- Sentinel-free is the house convention for existing Basic headers (`01`, `02`, `05`, `07`): no caller sentinel, the empty suffix is omitted from `sa` but valid for `lce`, `z[0] = n`, Manacher `even` has `n + 1` entries. New headers follow the same convention unless a sentinel is part of the definition (`29` BWT, `36`, `46`), in which case the sentinel or primary-index convention is named in the row.
- Transition storage and alphabet size appear in time and memory bounds (`O((m + 1) * sigma)` automata, sparse versus dense Aho tables). Map-based transitions are the default; dense tables are an explicit opt-in.
- Sizes are `< INT_MAX` symbols; counts are exact `lng` unless the row says modular or saturated. Counting APIs state exact, modular or saturated arithmetic.

## Problem classes

- Substring, subsequence, edit-distance and Hamming-distance problems are distinct families and never share a function name. Longest common substring belongs to suffix indexes (`07`, `28`); longest common subsequence belongs to `19`; subsequence membership and counting belong to `20`.
- Witnesses and ties use a documented convention: ascending starts, overlaps included, leftmost then lexicographically smallest unless a row states otherwise. All-solution and enumeration APIs are charged by output size and either return compact parameterizations or explicitly bounded enumerations.
- Transposition distances state which definition they implement (optimal string alignment versus unrestricted Damerau–Levenshtein). Alignment scores (Needleman–Wunsch, Smith–Waterman, Gotoh) are maximization problems with explicit scoring matrices and gap penalties, separate from edit-distance minimization.

## Hashes and randomness

- Hashes are Monte Carlo fingerprints and never proof of equality. A hash-based API says so in its name or documentation (`maybePalindrome`, `StringHash::lcp`). Exact equality or LCE uses deterministic verification or an exact index. Fingerprints are never reported as proof.
- Randomized runtime and randomized correctness are separate classifications: Las Vegas (exact answer, probabilistic time), Monte Carlo (probabilistic answer), high-probability bounds (`47` Optimal Dynamic Strings) are each named explicitly.
- Static fingerprint contexts own their bases; digests from different contexts are comparable only under identical base and encoding. Double-prime is the default; unsigned-word wrap has demonstrated structured collisions; the Mersenne `2^61 - 1` field variant is a speed option with a single large modulus. Collision bounds are in [03-stringhash.md](03-stringhash.md).
- Dynamic hashes (`23`) inherit the static collision model; randomized base lifecycle must be documented per structure.

## Ownership inside the folder

- Each callable implementation has exactly one owner; adapters may reuse it. Offline suffix-tree topology from suffix array and LCP is `11`; Ukkonen append-only construction is `33` and `11` may cross-reference it. Two-way matching is `16`; critical factorization proofs, Galil–Seiferas, Crochemore constant-workspace matching, in-place suffix sorting and sparse suffix arrays are `43`. LZ parsing is `42`; grammar/SLP compression is `37` and may call `42`. BWT transform and inverse are `29`; FM-index and XBW are `36`; run-length BWT and r-index are `46`.
- Distinct-substring counting and kth-substring queries belong to `10` and `22`, not to the quadratic prefix/Z reductions. Substring shortest-border and shortest-period queries belong to `27` and state preprocessing, query and output costs separately. Shortest absent subsequence belongs to `20`; minimal absent words and minimal unique substrings to `22`.
- Palindromes: static radii `05`, static queries `08`, eertree `13`, range and characteristic queries plus palindrome pairs `39`, double-ended and rollback structures `45`. Longest palindromic subsequence and counting palindromic subsequences are subsequence DP in `19`.
- Dynamic pattern sets (`34`) and dynamic text indexes (`35`, `44`, `45`, `47`) state the exact supported edit model (append, prepend, pop, insert, erase, split, join, rollback, persistence) before promising bounds; append-only analyses do not survive deletions.
- Generalized indexes over several strings (`28`) need separators that cannot collide with the input alphabet; the existing `longestCommonSubstring` in `07` compresses the union alphabet to positive ranks and uses zero as a fresh separator.

## Ownership across folders

- Strings owns prefix/radix string dictionaries (`04`, `25`); Data Structures owns binary integer/XOR tries, sparse tables, wavelet matrices, succinct bitvectors/trees and sequence trees (treap/splay) that `23`, `35` and `36` reuse.
- Mathematics owns convolution (`13-convolution.hpp`, batch MA06). Batch ST21 (`21-wildcardmatching.hpp`) depends on it, as do `32` hash-free 2D alternatives and `38` Hamming convolution; this prerequisite must be present in the batch manifest.
- Miscellaneous owns Huffman/optimal prefix coding (`40-optimal_merge.hpp`), digit DP (`31-digitdp.hpp`, consumes `24` automata) and expression parsing (`37-expression_parser.hpp`, listed by cp-algorithms under strings but owned there).
- Graphs owns LCA (`07-lca.hpp`) and Eulerian trails (`10-euleriantrail.hpp`); `27` BorderTree uses LCA, `41` uses Hierholzer as the Eulerian construction and as a differential oracle for FKM output. Mathematics generating functions own necklace/Lyndon counting (Pólya–Burnside).
- Python (`08-Python`) implements documented subsets of this folder (`_23_trie.py`, `_24_aho.py`, `_25_suffix.py`, `_12_dp.py` LCS/edit distance); reductions are stated there, never silent.

## Verification policy

- Differentially test matching against naive scans, suffix order against sorted suffixes, LCP against direct scans, substring and palindrome counts against sets, reconstruction by recomputing its input arrays, compression by decode roundtrip and automata by brute-force language enumeration on small alphabets.
- Include all-equal, periodic, Fibonacci/Thue–Morse (worst cases for runs and palindromes; generators in `50`), adversarial alphabets, repeated patterns, empty patterns and edits that invalidate old states.
- Finite tests are paired with a correctness argument in the package evidence; verified means every operation in the row has an independent oracle and a recorded passing command.

## Legacy obligations

- `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `OLD/Team Notebook/src/misc/old_zalgo.cpp` contain legacy trie, KMP and Z routines. The Z routine is accounted for in `02`; the prefix/KMP and trie routines are accounted for in `01` and `04` (see [01-prefixfunction.md](01-prefixfunction.md), [04-trie.md](04-trie.md)). Use `00-Guidelines/Ledgers/archive-map.json` to find any remaining string excerpt before deleting from `OLD`.
- This folder has no `97-Legacy/` directory; no row carries `legacy-reference` status.

## P016 package record (ST01, ST02, ST19)

Verified 2026-09-28 under the previous system; re-audited 2026-10-08 (contest profile, GNU C++20). All six headers are verified with the operations in their inventory rows; evidence: [01-prefixfunction.md](01-prefixfunction.md), [02-z.md](02-z.md), [03-stringhash.md](03-stringhash.md), [04-trie.md](04-trie.md), [05-manacher.md](05-manacher.md), [09-runlength.md](09-runlength.md).

Re-audit research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `01`: Gray-string DP over `prefixAutomaton` is problem-specific; distinct-substring counting by prefix function is quadratic and owned by `10`/`22`; suisen `min_period` returning the root string is `27` `primitiveRoot`.
- `02`: incremental Z is `30`; `is_substring` is `16` (or `!zOccurrences(...).empty()`); inverse Z is `31`.
- `03`: appending to a built hash breaks O(1) reversed prefixes and is owned by `23` (DequeHash, PointUpdateHash); `StringHashField<F>` adds a policy layer for modint types without a new guarantee; several Mersenne bases are unnecessary at `(L-1)/2^61` per comparison; `std::hash` for digests is left to callers (ordering covers `set`/`map`); lexicographic substring comparison is `22` `compareSubstrings` and needs the text; Rabin–Karp windows are `16`.
- `04`: kth key and rank by multiplicity have no catalog source and belong to the Data Structures binary trie and `24` ranking; per-key id lists belong to `06` pattern ids; parent links are unnecessary for the cursor (`fromTrie` in `06` can rebuild them by BFS).
- `05`: the Library Checker `2n - 1` combined length array is a radius conversion owned by `08`; `enumeratePalindromes` is a loop over `oddInterval`/`evenInterval`; an odd-only switch saves one linear pass only.
- `09`: suisen `RunLengthEncoder` push/pop has one unverified source and is a two-line idiom on `vector<pair<T, lng>>`.

Future-owner handoff: ST03 reuses `KmpMatcher`, `prefixAutomaton`, `StringHash<KIND>` (now with the `2^61 - 1` field) and `Manacher`; `08` `maybePalindrome` takes `StringHash<KIND>`. `06` `fromTrie` can consume the `BasicTrie` node cursor (`step`, `findNode`, `Node::terminal`). ST09 owns dynamic hashing, ST25 minimum-alphabet reconstruction, ST22 compressed dictionaries.

## P017 package record (ST03)

Verified 2026-09-28 under the previous system; re-audited 2026-10-08 (contest profile, GNU C++20). All three headers are verified with the operations in their inventory rows; evidence: [06-aho.md](06-aho.md), [07-suffixarray.md](07-suffixarray.md), [08-palindrome_queries.md](08-palindrome_queries.md). All ten `/reaudit-review` findings were fixed.

Re-audit research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `06`: a parent field (Library Checker `aho_corasick` output) is recoverable in `O(V)` from `nodes[u].next`; scans resuming from a given state (ei1333 `move`) are `step` plus `matchCount`; persistent-array transitions for `fromTrie` (maspypy) are replaced by the dense build over at most 256 bytes.
- `07`: `22` `frequencyStatistics` must not duplicate `longestRepeated(k)` (at least `k` occurrences, now in `07`); substring comparison, kth substrings, all-pairs LCP and occurrence ranges by position stay in `22`; distinct-substring counting in `10`; cyclic shifts in `15`.
- `08`: longest palindromic prefix/suffix is `05` `longestStarting()[0]`/`longestEnding()[n-1]`; inverse Manacher is `31`; "extend to palindrome" and double palindromes are problem-specific uses of those arrays.

ST04/ST05/ST06 retain suffix automaton/tree/SA-IS ownership; ST11 owns generalized multiple-string indexes. ST13 owns dynamic Aho updates and can reuse stable state/failure/output interfaces, with state IDs invalidated by `clear`. ST07/ST16/ST28 retain eertree/advanced static/dynamic palindrome ownership; longest-palindrome queries constrained to a substring are listed under ST16.

## P081 package record (ST04, ST06, ST33)

Implemented and verified 2026-10-09 (contest profile, GNU C++20): [10-suffixautomaton.md](10-suffixautomaton.md), [12-sais.md](12-sais.md), [18-editdistance.md](18-editdistance.md).

Research outcomes (sources in [00-sources.md](00-sources.md)); operations left out, one line each:

- `10`: `locate(L, R)` and `prefixState` (maspypy) are substring loci, owned by `11` `weightedAncestor`; `lengthRange` is `{minimalLength(v), nodes[v].len}`; the SAM smallest cyclic shift belongs to `15`; many-string LCS is `28`; longest repeated substring is `07`. `kthSubstringDistinct`/`kthSubstring` are the automaton variants; `22` keeps the suffix-array versions. `lexicographicWalk` is defined here as the ordered enumeration of distinct substrings with early stop; `cyclicShiftOccurrences` counts equal rotations once (Codeforces 235C convention).
- `12`: `saisSentinelFree` is not a separate function: `sais` itself is sentinel-free (ACL convention, `n` entries); callers wanting the empty suffix prepend `n`. The inverse suffix array is one loop over `sa` (`07` owns `rank`); prefix doubling stays in `07`, so there is no doubling fallback threshold here (the naive threshold is measured).
- `18`: Hyyrö banded and transposition bit-vectors, Masek–Paterson Four Russians, concave gap penalties and Levenshtein automata (Schulz–Mihov) are not adopted (niche or theoretical; an automaton would belong to `17`/`24`); Ukkonen band doubling is subsumed by `diagonalEditDistance`; LCE-accelerated Landau–Vishkin `O(n + d^2)` is `38`. Scored alignments other than `hirschberg` return scores only; `hirschberg(a, b, score, gap)` is the Needleman–Wunsch witness. Hamming distance is `38`; LCS and indel distance are `19`.

Handoffs: `22` may reuse `distinctSubstringsOnline` for `distinctSubstringsPerPrefix`; `28` generalizes `BasicSuffixAutomaton`; `29` can build a BWT from `sais`.
