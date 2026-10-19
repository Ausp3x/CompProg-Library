# 07 Strings — sources

Source keys used in `00-index.md` status cells are defined under "Prior source keys and audit narrative" below. A key is a research starting point, never correctness evidence.

## Catalog pages fetched 2026-10-06

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker string directory | https://github.com/yosupo06/library-checker-problems/tree/master/string | 2026-10-06 | Twelve string problems (aho_corasick, eertree, enumerate_palindromes, longest_common_substring, lyndon_factorization, number_of_substrings, palindromes_in_deque, prefix_substring_lcs, runenumerate, suffixarray, wildcard_pattern_matching, zalgorithm) mapped to rows `06`, `13`, `05`, `07`/`28`, `14`, `10`, `45`, `48`, `26`, `12`, `21`, `02`; the main judge page is JavaScript-rendered and returned no list |
| cp-algorithms index | https://cp-algorithms.com/ | 2026-10-06 | String Processing section: hashing, Rabin–Karp, prefix function, Z, suffix array, Aho–Corasick, suffix tree, suffix automaton, Lyndon, Manacher, finding repetitions (Main–Lorentz, row `26`); expression parsing noted as Miscellaneous-owned |
| AtCoder Library string | https://atcoder.github.io/ac-library/master/document_en/string.html | 2026-10-06 | `suffix_array` (string, vector<T>, bounded-int upper), `lcp_array`, `z_algorithm` overloads; bounded-alphabet SA-IS entry point added to row `12` |
| KACTL strings chapter | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/chapter.tex | 2026-10-06 | KMP, Zfunc, Manacher, MinRotation, SuffixArray, SuffixTree (Ukkonen, row `33`), Hashing, AhoCorasick; no missing family |
| OI Wiki string section | https://oi-wiki.org/string/ | 2026-10-06 | Navigation: matching, hashing, trie, KMP, Boyer–Moore, Z, Aho–Corasick, suffix array, optimal in-place suffix sorting (row `43`), suffix automaton, suffix balanced tree (row `35`), generalized SAM (row `28`), suffix tree, Manacher, palindromic tree, sequence automaton (row `20`), minimal representation, Lyndon, Main–Lorentz |
| Nyaan library index | https://nyaannyaan.github.io/library/ | 2026-10-06 | string/: aho-corasick, manacher, number-of-subsequences (row `20`), rolling-hash-2d (row `32`), rolling-hash-on-segment-tree (row `23`), rolling-hash, run-enumerate (row `26`), run-length-encoding, string-search (row `16`), suffix-array, suffix-automaton, trie, wildcard-pattern-matching, z-algorithm |
| maspypy string directory | https://github.com/maspypy/library/tree/main/string | 2026-10-06 | 48 files; adopted: aho_corasick_for_general_trie (row `06` fromTrie), all_pairs_lcp, basic_substring_structure, count_subsequence, count_unbordered_string (row `27`), deque_rolling_hash (row `23`), double_ended_palindromic_tree, edit_distance, enumerate_occurrences, find_runs, generalized_suffix_automaton, inverse_manacher/inverse_suffix_array/inverse_z_algorithm (row `31`), is_subsequence/is_substring, kmp, lex_min/max_suffix_for_all_prefix, longest_common_subsequence, longest_common_substring, lyndon, manacher, many_string_compare (row `22`), minimum_cyclic_shift, online_z_algorithm, palindrome_decomposition_dp (row `13`), palindromic_tree, periods, prefix_substring_LCS, rolling_hash, rolling_hash_2d, rolling_hash_field (row `03` StringHash61), run_length, sort_substrings, substring_shortest_border (row `27`), suffix_array, suffix_automaton, suffix_tree, trie, trie_map, wildcard_pattern_matching, z_algorithm; the generated index page truncated before `string/` |
| ei1333 library index | https://ei1333.github.io/library/ | 2026-10-06 | string/: aho-corasick, lcp-array, longest-common-substring, manacher, palindromic-tree, rolling-hash, suffix-array, wildcard-pattern-matching, z-algorithm; no missing family |
| suisen library index | https://suisen-cp.github.io/cp-library-cpp/ | 2026-10-06 | library/string/: aho_corasick, aho_corasick_array, compare_substring (row `22`), dynamic_rolling_hash, manacher, morris_pratt, palindromic_tree, rolling_hash, rolling_hash_field, run_enumerate, substring_set (row `10`), suffix_automaton, trie_array (row `04` TrieDense), trie_map |
| hitonanode library index | https://hitonanode.github.io/cplib-cpp/ | 2026-10-06 | string/: aho_corasick, aho_corasick_online and incremental_matching (row `34`), longest_common_prefix, lyndon, manacher, mp_algorithm, palindromic_tree, rolling_hash_1d, rolling_hash_2d, suffix_array, suffix_array_doubling, trie_light, z_algorithm |
| tko919 library index | https://tko919.github.io/library/ | 2026-10-06 | String/: ahocorasick, manacher, palindromictree, prefixsubstrlcs (row `48`), rollinghash, suffixarray, suffixautomaton, trie, zalgo |
| noshi91 library index | https://noshi91.github.io/Library/ | 2026-10-06 | No string directory; nothing adopted |
| Lecroq exact string matching | https://www-igm.univ-eiffel.fr/~lecroq/string/ | 2026-10-06 | 35 named algorithms: Brute Force, DFA and Karp–Rabin, Shift-Or, MP/KMP, Boyer–Moore, Horspool, Quick Search (Sunday), Two-Way and Galil–Seiferas map to rows `16`, `17`, `01`, `43`; the remaining 24 are the operation list of row `49` |
| Runs Theorem (arXiv 1406.0263) | https://arxiv.org/abs/1406.0263 | 2026-10-06 | Abstract: Lyndon-root characterization, `rho(n) < n`, sum of exponents `< 3n`, linear-time runs algorithm without LZ factorization; basis for `runs` in row `26` and Fibonacci/Thue–Morse worst-case generators in row `50` |
| r-index (arXiv 1705.10382) | https://arxiv.org/abs/1705.10382 | 2026-10-06 | Abstract: count and locate in `O(r)` space, optimal `O(m + occ)` locate in `O(r log(n/r))` space, extraction bounds; basis for row `46` operations and the `36`/`46` ownership split |

## P082 sweep (2026-10-18)

Research for `11`; pages fetched 2026-10-18 by the completeness researcher. Adopted operations are cited in [11-suffixtree.md](11-suffixtree.md); rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `11` | https://maspypy.github.io/library/string/suffix_tree.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/suffix_tree.hpp, https://maspypy.github.io/library/string/basic_substring_structure.hpp, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/SuffixTree.h, https://oi-wiki.org/string/suffix-tree/, https://cp-algorithms.com/string/suffix-tree-ukkonen.html, https://en.wikipedia.org/wiki/Suffix_tree, https://github.com/yosupo06/library-checker-problems/tree/master/string; ei1333, hitonanode and suisen indexes (no suffix tree); Nyaan by search only; Gusfield's textbook not accessed | `step` (maspypy `next`), `suffixLink`, `children`, pre-order ids with `Node::end` (maspypy numbering) adopted; KACTL and cp-algorithms are Ukkonen (`33`); Library Checker has no suffix-tree problem |

## P082 re-sweep (2026-10-19)

Research for `11` after the omission sweep; pages fetched 2026-10-19 by the completeness researcher.

| Row | Pages | Outcome |
|---|---|---|
| `11` | https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/seq/suffix_array.hpp, https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/ds/rmq.hpp, https://api.github.com/repos/ecnerwala/cp-book/contents/src/seq, https://api.github.com/repos/maspypy/library/contents/string, https://raw.githubusercontent.com/maspypy/library/main/string/suffix_tree.hpp, https://en.wikipedia.org/wiki/Suffix_tree, https://oi-wiki.org/string/suffix-tree/, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/SuffixTree.h, https://nyaannyaan.github.io/library/ | `get_split` (LCP and split index of an SA interval over `{lcp, index}` pairs) informs `lca`; its `RangeMinQuery` layout (32-element blocks, in-block bitmasks, sparse table over block minima) is the `split` RMQ; Wikipedia's first occurrence is left out ([00-notes.md](00-notes.md)); nothing else missing |

## P085 sweeps (2026-10-16)

Per-row research for `21`, `22`, `23`; pages fetched 2026-10-16 by the completeness researcher. Adopted operations are cited in each evidence document; rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `21` | https://ics.uci.edu/~goodrich/teach/cs262P/notes/StringMatchingWithFFT.pdf (pp. 1–3), https://github.com/yosupo06/library-checker-problems/blob/master/string/wildcard_pattern_matching/task.md, https://nyaannyaan.github.io/library/string/wildcard-pattern-matching.hpp, https://ei1333.github.io/library/string/wildcard-pattern-matching.hpp, https://maspypy.github.io/library/string/wildcard_pattern_matching.hpp, https://en.wikipedia.org/wiki/Matching_wildcards, https://research.swtch.com/glob, https://cp-algorithms.com/algebra/fft.html; OI Wiki `/string/wildcard/` returned 404 and `/string/match/`, `/math/poly/fft/` do not cover it (the row's `OI-STR` key was dropped) | Clifford–Clifford (random symbol values) and the Fischer–Paterson bitwise decomposition adopted; glob by greedy leftmost segments plus a bit-parallel NFA; no catalog has a glob routine |
| `22` | https://cp-algorithms.com/string/suffix-array.html, https://oi-wiki.org/string/sa/, https://suisen-cp.github.io/cp-library-cpp/library/string/compare_substring.hpp, https://suisen-cp.github.io/cp-library-cpp/library/string/substring_set.hpp, maspypy `string/{all_pairs_lcp, many_string_compare, sort_substrings, lex_max_suffix_for_all_prefix, lex_min_suffix_for_all_prefix, non_dominated_suffix, suffix_lcp_change, substring_abundant_string, basic_substring_structure, enumerate_occurrences, suffix_tree}.hpp` (raw GitHub), Nyaan `string-search.hpp`, ei1333 `suffix-array.hpp`, hitonanode `longest_common_prefix.hpp`, https://arxiv.org/abs/1312.2738, https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.CPM.2017.24; Barton et al. 2014 and CMR 1998 seen in search results only | KMR `lcp`, many-string `lce`/`comparePieces`, both substring ranks adopted; `allPairsLcp` defined as per-suffix LCP sums (`lcpSums`), `frequencyStatistics` as the histogram by occurrence count; the rest in the notes |
| `23` | https://raw.githubusercontent.com/maspypy/library/main/alg/monoid/rolling_hash.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/deque_rolling_hash.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/rolling_hash.hpp, https://suisen-cp.github.io/cp-library-cpp/library/string/dynamic_rolling_hash.hpp, https://nyaannyaan.github.io/library/string/rolling-hash-on-segment-tree.hpp, https://hitonanode.github.io/cplib-cpp/string/rolling_hash_1d.hpp, https://oi-wiki.org/string/hash/, https://cp-algorithms.com/string/string-hashing.html; CF 580E and BZOJ 1014 write-ups; CSES Palindrome Queries from search results only | `reverseGet`, `lcs`, `compare` on the point-update and sequence hashes and range assignment in `HashMonoid` adopted; `mutableLcp` became `SequenceHash::lcp` |

## P085 re-sweep (2026-10-19)

Research for `21`, `22`, `23` after the omission sweep; pages fetched 2026-10-19 by the completeness researcher.

| Row | Pages | Outcome |
|---|---|---|
| `21`, `22`, `23` | https://cses.fi/problemset/task/2110, https://cses.fi/problemset/, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/number_of_substrings/task.md, https://raw.githubusercontent.com/maspypy/library/main/string/suffix_automaton.hpp, https://raw.githubusercontent.com/hitonanode/cplib-cpp/master/string/incremental_matching.hpp, https://oi-wiki.org/string/sa/, https://cp-algorithms.com/string/suffix-array.html, GitHub API string listings of maspypy, Nyaan, suisen, ei1333, hitonanode, tko919, koosaga, ecnerwala, PyRival and Library Checker | Nothing new; CSES 2110 (lengths `1..n`) is the only by-length source, so `distinctSubstringsByLength` adds entry 0 for the empty string as maspypy's automaton root range `[0, 1)` does; hitonanode `incremental_matching` is row `34` |

## P084 sweeps (2026-10-10)

Per-row research for `16`, `17`, `20`, `29`; pages fetched 2026-10-10 by the completeness researcher. Adopted operations are cited in each evidence document; rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `16` | https://www-igm.univ-eiffel.fr/~lecroq/string/ (index, node4, node5, node14, node19, node26), https://cp-algorithms.com/string/rabin-karp.html, https://oi-wiki.org/string/match/, https://oi-wiki.org/string/bm/, https://nyaannyaan.github.io/library/string/string-search.hpp (a suffix-array wrapper, `07`), https://raw.githubusercontent.com/maspypy/library/main/string/is_substring.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/enumerate_occurrences.hpp (multi-pattern, `06`), KACTL `KMP.h`, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/kmp.py | Galil rule inside `boyerMoore`, `isSubstring`, disjoint mode of `countOccurrences` adopted; Rabin–Karp named as verified hits; `sunday` named Quick Search |
| `17` | https://www-igm.univ-eiffel.fr/~lecroq/string/node6.html, https://en.wikipedia.org/wiki/Bitap_algorithm, https://oi-wiki.org/string/match/, https://users.dcc.uchile.cl/~gnavarro/ps/jstat04.pdf (pp. 1–6, 8–13); Codeforces 914F/963D returned 403 (technique from search results) | Character-class `patternMasks`, masks overloads for every search, `BitsetText` adopted; multiword support for the k-error searches |
| `20` | https://oi-wiki.org/string/seq-automaton/, https://github.com/maspypy/library/tree/main/string, https://raw.githubusercontent.com/maspypy/library/main/string/count_subsequence.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/is_subsequence.hpp, https://nyaannyaan.github.io/library/string/number-of-subsequences.hpp, https://suisen-cp.github.io/cp-library-cpp/library/dp/number_of_subsequences.hpp, https://atcoder.jp/contests/arc081/tasks/arc081_c, https://atcoder.jp/contests/abc214/tasks/abc214_f, https://atcoder.jp/contests/typical90/tasks/typical90_f, https://www.cnblogs.com/BlankAo/p/15826647.html, https://www.luogu.com.cn/article/v381e762; Library Checker `number_of_subsequences` page body empty | Sparse (position-list) variant, per-state counts, range `isSubsequence`, lexicographically smallest absent witness adopted |
| `29` | https://en.wikipedia.org/wiki/Burrows%E2%80%93Wheeler_transform, https://www.cs.princeton.edu/courses/archive/fall20/cos226/assignments/burrows/specification.php, https://rosalind.info/problems/list-view/?location=bioinformatics-textbook-track, https://arxiv.org/abs/0908.0239 and https://arxiv.org/abs/2106.11191 (abstracts); https://oi-wiki.org/string/bwt/ returned 404 and cp-algorithms has no BWT page, so the row's `OI-STR` key has no BWT page behind it; SPOJ BWHEELER returned 403 | `bijectiveBwt`/`inverseBijectiveBwt` and `inverseBwtPrimary` (named inverse per representation) adopted |

## P083 sweeps (2026-10-10)

Per-row research for `13`, `14`, `15`, `19`; pages fetched 2026-10-10. Adopted operations are cited in each evidence document; rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `13` | https://oi-wiki.org/string/pam/, https://maspypy.github.io/library/string/palindromic_tree.hpp, https://maspypy.github.io/library/string/palindrome_decomposition_dp.hpp, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/eertree/task.md, https://ei1333.github.io/library/string/palindromic-tree.hpp, https://hitonanode.github.io/cplib-cpp/string/palindromic_tree.hpp, https://tko919.github.io/library/String/palindromictree.hpp, https://arxiv.org/abs/1506.04862 (pages 1–2); Nyaan and cp-algorithms have no eertree; adamant's blog returned 403/404 | `parent`, `pos`/`palindrome`, `newString` (joint tree), `substringSuffixPalindrome`, generic `palindromicFactorization` adopted |
| `14` | https://cp-algorithms.com/string/lyndon_factorization.html, https://oi-wiki.org/string/lyndon/, https://maspypy.github.io/library/string/lyndon.hpp, https://maspypy.github.io/library/string/lex_min_suffix_for_all_prefix.hpp, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/lyndon_factorization/task.md, https://hitonanode.github.io/cplib-cpp/string/lyndon.hpp, https://arxiv.org/abs/1510.02637 (abstract); Nyaan, suisen, ei1333, KACTL have no Lyndon file | `IncrementalLyndon`, `suffixFactorization`, `nextLyndonWord`, exact-length enumeration adopted |
| `15` | https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/MinRotation.h, https://oi-wiki.org/string/minimal-string/, https://cp-algorithms.com/string/lyndon_factorization.html, https://cp-algorithms.com/string/suffix-array.html, https://maspypy.github.io/library/string/minimum_cyclic_shift.hpp, PyRival `strings/min_rotation.py`; Nyaan, ei1333, hitonanode have no rotation files | `minRotationIndices`, `rotationOffset` adopted; `minCyclicShift` merged into `minRotationIndices` |
| `19` | https://maspypy.github.io/library/string/longest_common_subsequence.hpp, https://maspypy.github.io/library/string/count_subsequence.hpp, https://suisen-cp.github.io/cp-library-cpp/library/dp/number_of_subsequences.hpp, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/lcs.py, https://oi-wiki.org/dp/basic/, https://atcoder.jp/contests/dp/tasks/dp_f, https://cses.fi/problemset/; Library Checker, KACTL, cp-algorithms, Nyaan, ei1333, hitonanode have no LCS page | `shortestCommonSupersequence`, `lcs3` adopted |

## P017 re-sweep (2026-10-19)

Research for `06`–`08` after the omission sweep; pages fetched 2026-10-19 by the completeness researcher. Adopted operations are cited in each evidence document; rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `06` | https://cp-algorithms.com/string/aho_corasick.html, https://oi-wiki.org/string/ac-automaton/, https://ei1333.github.io/library/string/aho-corasick.hpp, https://nyaannyaan.github.io/library/string/aho-corasick.hpp, https://hitonanode.github.io/cplib-cpp/string/aho_corasick.hpp, https://suisen-cp.github.io/cp-library-cpp/library/string/aho_corasick.hpp, KACTL `AhoCorasick.h`, https://github.com/maspypy/library/tree/main/string, https://cses.fi/problemset/task/2104 | `firstOccurrences` adopted (CSES Pattern Positions); `lastOccurrences` left out |
| `07` | https://cp-algorithms.com/string/suffix-array.html, https://oi-wiki.org/string/sa/, ACL `string.hpp`, KACTL `SuffixArray.h`, https://raw.githubusercontent.com/maspypy/library/main/string/suffix_array.hpp, https://nyaannyaan.github.io/library/string/suffix-array.hpp, https://ei1333.github.io/library/string/suffix-array.hpp, https://ei1333.github.io/library/string/longest-common-substring.hpp, https://hitonanode.github.io/cplib-cpp/string/suffix_array.hpp | No new operation; DC3 left out; byte `longestCommonSubstring` restated as `O(n + m)` (ei1333) |
| `08` | https://oi-wiki.org/string/manacher/, maspypy/Nyaan/ei1333/hitonanode/suisen `manacher.hpp`, KACTL `strings/`, PyRival `strings/`, https://www.luogu.com.cn/problem/CF17E, https://ouuan.github.io/post/cf17e-palisectionmanacher/, https://jkloverdcoi.github.io/2019/08/29/CF17E-Palisection/ | `countPalindromesByEndpoint` adopted (CF 17E Palisection: starts/ends per position by difference arrays over the radii) |

## P016 and P017 re-audit sweeps (2026-10-08)

Per-row research for `01`–`09`; pages fetched 2026-10-08. Adopted operations are cited in each evidence document; rejections are in [00-notes.md](00-notes.md).

| Row | Pages | Outcome |
|---|---|---|
| `01` | https://cp-algorithms.com/string/prefix-function.html, https://oi-wiki.org/string/kmp/, https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/KMP.h, maspypy `string/kmp.hpp` and `periods.hpp`, https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/main/library/string/morris_pratt.hpp, hitonanode `string/mp_algorithm.hpp`, Nyaan and ei1333 indexes, https://www-igm.univ-eiffel.fr/~lecroq/string/node8.html | `kmpNext` (Knuth strong failure, Lecroq) adopted and used by `KmpMatcher` |
| `02` | https://cp-algorithms.com/string/z-function.html, KACTL `Zfunc.h`, ACL `string.md`, https://nyaannyaan.github.io/library/string/z-algorithm.hpp, https://ei1333.github.io/library/string/z-algorithm.hpp, https://hitonanode.github.io/cplib-cpp/string/z_algorithm.hpp, https://oi-wiki.org/string/z-func/, Library Checker `zalgorithm/task.md`, maspypy `z_algorithm.hpp`, `is_substring.hpp` | Nothing missing |
| `03` | https://cp-algorithms.com/string/string-hashing.html, KACTL `Hashing.h`, maspypy `rolling_hash.hpp` and `rolling_hash_field.hpp`, https://nyaannyaan.github.io/library/string/rolling-hash.hpp, Nyaan `internal/internal-hash.hpp`, https://ei1333.github.io/library/string/rolling-hash.hpp, suisen `rolling_hash.hpp` and `rolling_hash_field.hpp`, https://hitonanode.github.io/cplib-cpp/string/rolling_hash_1d.hpp, https://tko919.github.io/library/String/rollinghash.hpp, https://oi-wiki.org/string/hash/ | `StringHash61` (Mersenne folding, primitive-root base), `hashOf`, Digest ordering adopted |
| `04` | https://oi-wiki.org/string/trie/, maspypy `string/trie.hpp` and `trie_map.hpp`, https://nyaannyaan.github.io/library/string/trie.hpp, suisen `trie_array`/`trie_map`, https://hitonanode.github.io/cplib-cpp/string/trie_light.hpp, https://tko919.github.io/library/String/trie.hpp, https://ei1333.github.io/library/structure/trie/trie.hpp, https://cses.fi/problemset/task/1731, KACTL and PyRival string directories | `TrieDense`, `step`, `forEachPrefixOf`, `longestPrefix` adopted |
| `05` | https://cp-algorithms.com/string/manacher.html, KACTL `Manacher.h`, maspypy/Nyaan/ei1333/suisen/hitonanode `manacher.hpp`, https://oi-wiki.org/string/manacher/, Library Checker `enumerate_palindromes/task.md` | Predicate constructor, `longestEnding`/`longestStarting` (with `centerEnd`) adopted |
| `09` | https://nyaannyaan.github.io/library/string/run-length-encoding.hpp, https://maspypy.github.io/library/string/run_length.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/run_length_encoder.hpp, https://hitonanode.github.io/cplib-cpp/utilities/run_length_encoding.hpp, ei1333/tko919 indexes, KACTL, PyRival | Nothing adopted |
| `06` | https://github.com/maspypy/library/blob/main/string/aho_corasick_for_general_trie.hpp, https://cp-algorithms.com/string/aho_corasick.html, https://oi-wiki.org/string/ac-automaton/, KACTL `AhoCorasick.h`, ei1333/Nyaan/suisen (`aho_corasick`, `aho_corasick_array`)/hitonanode/tko919 Aho pages, Library Checker `aho_corasick/task.md` | `fromTrie` adopted (byte labels, IDs and output links added; dense build instead of persistent arrays) |
| `07` | https://oi-wiki.org/string/sa/, https://cp-algorithms.com/string/suffix-array.html, ACL `string.md`, KACTL `SuffixArray.h`, ei1333 `suffix-array.hpp`/`lcp-array.hpp`, Nyaan/maspypy/hitonanode/tko919 suffix-array pages, Library Checker `string/` problems | `longestRepeated(k)` and `longestRepeatedDisjoint` adopted |
| `08` | https://cp-algorithms.com/string/manacher.html, https://oi-wiki.org/string/manacher/, KACTL `Manacher.h`, Nyaan/maspypy/ei1333/suisen/hitonanode/tko919 `manacher.hpp`, PyRival `strings/` | `countPalindromes` adopted (listed by the Python row, derived from the radii) |

## P081 sweeps (2026-10-09)

Pages fetched 2026-10-09 by the completeness researcher; all code was written independently from the cited algorithms.

| Row | Pages | Outcome |
|---|---|---|
| `10` | https://cp-algorithms.com/string/suffix-automaton.html, https://oi-wiki.org/string/sam/, https://maspypy.github.io/library/string/suffix_automaton.hpp, https://nyaannyaan.github.io/library/string/suffix-automaton.hpp, ei1333 and KACTL indexes (no SAM), https://github.com/yosupo06/library-checker-problems/tree/master/string (`number_of_substrings`, `longest_common_substring`) | `step`, `findNode` (Nyaan `next`/`find`), `matchingStatistics` (maspypy `next`, cp-algorithms two-string walk), `lastOccurrence` (maspypy `max_pos`), `shortestAbsentString` (cp-algorithms, OI Wiki) adopted |
| `12` | https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp, https://oi-wiki.org/string/sa/, https://maspypy.github.io/library/string/suffix_array.hpp, https://ei1333.github.io/library/string/suffix-array.hpp, https://ei1333.github.io/library/string/lcp-array.hpp, https://nyaannyaan.github.io/library/string/suffix-array.hpp, https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/suffixarray/task.md | Generic `sais(vector<T>)` (ACL `suffix_array(vector<T>)`) adopted; ACL's compile-time naive threshold mirrored as `template<int NAIVE>` with a measured default |
| `18` | https://en.wikipedia.org/wiki/Levenshtein_distance, https://en.wikipedia.org/wiki/Edit_distance, https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance, https://en.wikipedia.org/wiki/Sequence_alignment, https://en.wikipedia.org/wiki/Gap_penalty, https://en.wikipedia.org/wiki/Bitap_algorithm, https://maspypy.github.io/library/string/edit_distance.hpp.html, https://ei1333.github.io/library/ (`dp/edit-distance.hpp`), https://www-igm.univ-eiffel.fr/~lecroq/string/ (exact matching only), https://curiouscoding.nl/posts/approximate-string-matching/ | Multiword `myersBitVector` (Myers 1999 blocks, Hyyrö), `diagonalEditDistance` (Ukkonen/Myers furthest-reaching diagonals with Ukkonen's cutoff) and `semiGlobalAlignment` adopted; transposition variants renamed by definition |

## Fetched 2026-10-19 (omission sweep)

Scope `07`, `11`; decisions are in the rows, the P017/P082/P085/P232 notes and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded decisions).

| URL | Used for |
|---|---|
| https://maspypy.github.io/library/string/suffix_tree.hpp | `11` `step` (`next`), per-node `(l, r)` |
| https://oi-wiki.org/string/suffix-tree/ | `11` subtree = SA interval, LCA depth = LCP, `suffixLink` |
| https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/seq/suffix_array.hpp | `11` `lca` (`get_split`) |
| https://cses.fi/problemset/task/2110 | `22` `distinctSubstringsByLength` |
| https://github.com/yosupo06/library-checker-problems/tree/master/string, https://maspypy.github.io/library/string/suffix_array.hpp, https://oi-wiki.org/string/sa/, https://cp-algorithms.com/string/suffix-array.html, https://cp-algorithms.com/string/suffix-tree-ukkonen.html, KACTL `SuffixTree.h`, https://nyaannyaan.github.io/library/string/string-search.hpp, https://ei1333.github.io/library/string/suffix-array.hpp, ecnerwala `src/seq`, koosaga `codes/string/suffix_array.cpp` | Checked; nothing else missing from `07` or `11` |
| https://api.github.com/repos/{maspypy/library/contents/string, NyaanNyaan/library/contents/string, ei1333/library/contents/string, suisen-cp/cp-library-cpp/contents/library/string, hitonanode/cplib-cpp/contents/string} (full listings; the generated index pages truncate) and the suffix-array/LCP files they list | No suffix-tree or weighted-ancestor file outside maspypy; maspypy `suffix_lcp_change` and `basic_substring_structure` stay as decided under P085 in [00-notes.md](00-notes.md); the rest are aliases of `07`, `12`, `22`, `31` |
| https://arxiv.org/abs/1406.7716, https://arxiv.org/abs/2103.00462, https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.CPM.2021.8 | `51`: GLN14 O(n) extra space, O(1) query; BKPR21 O(1) query with linear construction (irreducible LCP values) |

## Prior source keys and audit narrative (2026-09-27 and 2026-09-28, condensed)

Keys used in status cells: KACTL, CPALG (cp-algorithms), OI-STR (OI Wiki string section), MASPYPY, NYAAN, EI1333, SUISEN, HITONANODE, TKO919, ACL, YOSUPO (Library Checker), CSES, LECROQ, RUNS (arXiv 1406.0263), RINDEX (arXiv 1705.10382), DYNAMICSTRINGS (arXiv 1511.02612, Gawrychowski et al., Optimal Dynamic Strings), SEMILOCAL (arXiv 0707.3619, Tiskin, Semi-local string comparison), GLN14 (arXiv 1406.7716, Gawrychowski, Lewenstein, Nicholson, Weighted ancestors in suffix trees, ESA 2014), BKPR21 (LIPIcs CPM 2021.8, Belazzougui, Kosolobov, Puglisi, Raman, Weighted Ancestors in Suffix Trees Revisited), DS17 (Data Structures succinct engines batch).

- The 2026-09-27 cross-library audit (`00-Guidelines/History/2026-09-27-inventory-audit.md`, records in `Ledgers/research-sources.json`) compared catalog navigation and selected API comments, not source correctness; it grew this folder from 35 to 49 rows (14 added). Paper abstracts identified advanced variants (dynamic strings, semi-local LCS, runs, r-index); their proofs remain implementation-time reading.
- Targeted pages read then: maspypy `inverse_manacher.hpp` (odd-only radii), `online_z_algorithm.hpp`, `double_ended_palindromic_tree.hpp`, `prefix_substring_LCS.hpp` (wavelet-matrix dependency); OI Wiki navigation including suffix balanced tree, in-place suffix sorting, subsequence automaton and Main–Lorentz; Lecroq's complete table of contents (the univ-mlv hostname had a certificate mismatch; the univ-eiffel successor was used).
- Library Checker coverage (`00-Guidelines/Ledgers/library-checker-coverage.json`) maps string problems to rows; status there is "inventoried", not accepted.
- Legacy: `OLD/Team Notebook/src/misc/old_zalgo.cpp` (static Z, accounted for in `02`); Team Notebook `algs.cpp`/`algsbetter.cpp` trie/KMP routines (accounted for in `01`/`04`); no other owned string implementation was found in `OLD/algorithms.cpp` or `OLD/[1] algorithms.cpp`.

## Not adopted

- cp-algorithms "Expression parsing": owned by Miscellaneous `37-expression_parser.hpp`.
- Nyaan `misc/compress.hpp` and `math/enumerate-quotient.hpp`: coordinate compression and quotient enumeration are not string operations.
- maspypy `suffix_lcp_change.hpp`, `non_dominated_suffix.hpp`, `substring_abundant_string.hpp`: single-problem constructions whose API was not inspected beyond the file name; no named family in any other catalog.
- maspypy `split.hpp`, `binary_lyndon_list.txt`, `suffix_tree.png`: utility, data and image files.
- suisen `morris_pratt.hpp`: the MP border table is `prefixFunction`; no separate operation.
- hitonanode `suffix_array_doubling.hpp`, `longest_common_prefix.hpp`: covered by `07` radix doubling and `lce`.
- OI Wiki "Standard Library" page: C++ `std::string` usage, not an algorithm.

## Covered by existing rows

- Lecroq Brute Force and DFA pages: naive scan and `prefixAutomaton`/`dfaSearch` already appear in `16`; no historical row entry.
- KACTL/ACL: no item excluded; every entry maps to an existing row.
- Library Checker `enumerative_combinatorics/number_of_subsequences`: already mapped to row `20`.
