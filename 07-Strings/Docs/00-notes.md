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

- `OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and `OLD/Team Notebook/src/misc/old_zalgo.cpp` contain legacy trie, KMP and Z routines. The Z routine is accounted for in `02`; the prefix/KMP and trie routines are accounted for in `01` and `04` (see `90-prefix_z.md`, `92-trie_manacher.md`). Use `00-Guidelines/Ledgers/archive-map.json` to find any remaining string excerpt before deleting from `OLD`.
- This folder has no `97-Legacy/` directory; no row carries `legacy-reference` status.

## P016 package record (ST01, ST02, ST19)

Package **complete**, in the requested order **ST01 → ST02 → ST19**, on 2026-09-28. The C01/P002 prerequisite was already verified. All six selected headers are implemented and verified; no owned feature or verification gaps remain. Archived originals are preserved. Implementations use the contest profile and GNU C++20.

| Batch | Headers and features | Contracts, proofs and evidence |
|---|---|---|
| ST01 | Prefix function, streaming/overlapping/empty-pattern KMP, prefix counts, borders/periods, dense alphabet automaton; Z and extended KMP, exact prefix/Z validation and conversion. | [01-prefixfunction.md](01-prefixfunction.md), [02-z.md](02-z.md) |
| ST02 | Length/base-tagged double-prime and unsigned-word fingerprints, extraction/reversal/concatenation, substring LCP/LCS; full-byte multiset trie with prefix/lexicographic traversal and reclaimed nodes; generic Manacher radii and maximal/nested interval reconstruction. | [03-stringhash.md](03-stringhash.md), [04-trie.md](04-trie.md), [05-manacher.md](05-manacher.md) |
| ST19 | Byte/integer run-length encoding, validated encoded-size queries, bounded decoding, half-open witness spans, malformed/overflow rejection and output alias rules. | [09-runlength.md](09-runlength.md) |

Each linked document records the exact domains, complexity, independent correctness argument, per-feature test map, inspected source scope and disposition of relevant legacy features. New routines are independent implementations. Independent review found and fixed mixed-alphabet narrowing in streaming KMP before completion. Original legacy bytes remain unchanged. Package commands and integration results are repeated in each per-header `## Commands and results`.

| Header | Executed feature mode | Checks per optimized / checked / ASan-UBSan configuration | Checked assertion probes |
|---|---|---|---|
| Prefix/KMP | Full, then final stress | 3,315,194 in stress | 7 |
| Z / conversions | Stress; final full checked probes | 1,410,995 in stress | 3 |
| String hash | Full | 833,559 C++ plus 5,504 Python exact interval checks | 13 |
| Trie | Full | 2,077,118 | 5 |
| Manacher | Full | 3,200,052 | 10 |
| Run length | Full | 363,502 | 5 |

Future-owner handoff: There are no P016 continuation tasks. The remaining 43 Strings headers retain planned status. Future owners can reuse the following stable foundations:

- ST03 owns Aho, suffix arrays and palindrome-query adapters. `KmpMatcher`, `prefixAutomaton`, `StringHash` and `Manacher` radii are available; hash-based answers must retain their Monte Carlo classification. Exact substring-palindrome predicates and longest-palindrome extraction remain ST03.
- ST09 owns dynamic hashing and lexicographic integration; ST11 owns broader periodicity. Static fingerprint contexts own their actual bases and may be shared only under matching base/encoding contracts.
- ST25 owns constrained/minimum-alphabet reconstruction beyond ST01's exact feasibility and prefix/Z representation conversion.
- ST22 owns compressed/persistent string dictionaries. Data Structures retains binary integer/XOR tries; Miscellaneous retains Huffman coding. Lempel–Ziv and run-length BWT remain their existing separate owners.

## P017 package record (ST03)

Package **complete** for the sole batch **ST03**, on 2026-09-28. All owned features and verification are complete. The C01/P002 and ST01–ST02/P016 prerequisites are verified. All implementations use the contest profile and GNU C++20; preserved originals remain unchanged.

| Header | Features | Contracts and evidence |
|---|---|---|
| `06-aho.hpp` | Byte Aho-Corasick, failure/output links, duplicates and empty patterns, sparse/dense transitions, streaming and callback enumeration, per-pattern/position counts, suffix aggregation and forbidden-pattern adapters. | [06-aho.md](06-aho.md) |
| `07-suffixarray.hpp` | Sentinel-free radix doubling, sparse integer compression, inverse ranks and Kasai LCP, optional RMQ/LCE, pattern ranges, longest repeated/common substring witnesses. | [07-suffixarray.md](07-suffixarray.md) |
| `08-palindrome_queries.hpp` | Exact Manacher interval queries, explicitly probabilistic hash queries, leftmost longest palindrome witness and radius/index conversions. | [08-palindrome_queries.md](08-palindrome_queries.md) |

Each companion document records domains, complexity, correctness arguments, inspected source scope, legacy accounting, a feature-to-test map and verification. Three mirrored Python entries expose quick/full/stress and use independent non-removable oracles. Package commands and integration results are repeated in each per-header `## Commands and results`.

| Header | Checks in each full configuration | Checked precondition probes |
|---|---|---|
| Aho | 2,267,528 | 7 |
| Suffix array | 3,915,321 | 10 |
| Palindrome queries | 3,405,831 | 6 |

Future-owner boundaries: No owned implementation or verification gap remains, and no P017 continuation handoff is required. The other **40** Strings headers retain planned status.

ST04/ST05/ST06 retain suffix automaton/tree/SA-IS ownership; ST11 owns generalized multiple-string indexes. ST09 owns broader lexicographic/frequency applications. ST13 owns dynamic Aho updates and can reuse stable state/failure/output interfaces, with state IDs invalidated by `clear`. ST07/ST16/ST28 retain eertree/advanced static/dynamic palindrome ownership; longest-palindrome queries constrained to a substring are explicitly listed under ST16. These separate scheduled families are not advertised as implemented here. No online submission or acceptance is claimed.
