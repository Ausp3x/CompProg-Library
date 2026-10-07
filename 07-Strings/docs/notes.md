# 07 Strings — notes

Contracts moved out of the inventory on 2026-10-06. The inventory lists operations; this file holds the rules every row must respect. Package evidence (`89`–`97`) records what was verified and remains authoritative for the nine Basic headers.

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
- Static fingerprint contexts own their bases; digests from different contexts are comparable only under identical base and encoding. Double-prime is the default; unsigned-word wrap has demonstrated structured collisions; the Mersenne `2^61 - 1` field variant is a speed option with a single large modulus. Collision bounds are in `91-stringhash.md`.
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

- `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `OLD/Team Notebook/src/misc/old_zalgo.cpp` contain legacy trie, KMP and Z routines. The Z routine is accounted for in `02`; the prefix/KMP and trie routines are accounted for in `01` and `04` (see `90-prefix_z.md`, `92-trie_manacher.md`). Use `00-Guidelines/19-archive-map.json` to find any remaining string excerpt before deleting from `OLD`.
- This folder has no `97-Legacy/` directory; no row carries `legacy-reference` status.
