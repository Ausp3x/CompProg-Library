# 07 Strings — sources

Status-cell keys are the library's (`00-Guidelines/09-sources.md`, Source keys).

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
| `21` | https://ics.uci.edu/~goodrich/teach/cs262P/notes/StringMatchingWithFFT.pdf (pp. 1–3), https://github.com/yosupo06/library-checker-problems/blob/master/string/wildcard_pattern_matching/task.md, https://nyaannyaan.github.io/library/string/wildcard-pattern-matching.hpp, https://ei1333.github.io/library/string/wildcard-pattern-matching.hpp, https://maspypy.github.io/library/string/wildcard_pattern_matching.hpp, https://en.wikipedia.org/wiki/Matching_wildcards, https://research.swtch.com/glob, https://cp-algorithms.com/algebra/fft.html; OI Wiki `/string/wildcard/` returned 404 and `/string/match/`, `/math/poly/fft/` do not cover it (the row's `OI` key was dropped) | Clifford–Clifford (random symbol values) and the Fischer–Paterson bitwise decomposition adopted; glob by greedy leftmost segments plus a bit-parallel NFA; no catalog has a glob routine |
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
| `29` | https://en.wikipedia.org/wiki/Burrows%E2%80%93Wheeler_transform, https://www.cs.princeton.edu/courses/archive/fall20/cos226/assignments/burrows/specification.php, https://rosalind.info/problems/list-view/?location=bioinformatics-textbook-track, https://arxiv.org/abs/0908.0239 and https://arxiv.org/abs/2106.11191 (abstracts); https://oi-wiki.org/string/bwt/ returned 404 and cp-algorithms has no BWT page, so the row's `OI` key has no BWT page behind it; SPOJ BWHEELER returned 403 | `bijectiveBwt`/`inverseBijectiveBwt` and `inverseBwtPrimary` (named inverse per representation) adopted |

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

Scope `07`, `11`; decisions are in the rows, the P017/P082/P085/P232 plan notes and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded decisions).

| URL | Used for |
|---|---|
| https://maspypy.github.io/library/string/suffix_tree.hpp | `11` `step` (`next`), per-node `(l, r)` |
| https://oi-wiki.org/string/suffix-tree/ | `11` subtree = SA interval, LCA depth = LCP, `suffixLink` |
| https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/seq/suffix_array.hpp | `11` `lca` (`get_split`) |
| https://cses.fi/problemset/task/2110 | `22` `distinctSubstringsByLength` |
| https://github.com/yosupo06/library-checker-problems/tree/master/string, https://maspypy.github.io/library/string/suffix_array.hpp, https://oi-wiki.org/string/sa/, https://cp-algorithms.com/string/suffix-array.html, https://cp-algorithms.com/string/suffix-tree-ukkonen.html, KACTL `SuffixTree.h`, https://nyaannyaan.github.io/library/string/string-search.hpp, https://ei1333.github.io/library/string/suffix-array.hpp, ecnerwala `src/seq`, koosaga `codes/string/suffix_array.cpp` | Checked; nothing else missing from `07` or `11` |
| https://api.github.com/repos/{maspypy/library/contents/string, NyaanNyaan/library/contents/string, ei1333/library/contents/string, suisen-cp/cp-library-cpp/contents/library/string, hitonanode/cplib-cpp/contents/string} (full listings; the generated index pages truncate) and the suffix-array/LCP files they list | No suffix-tree or weighted-ancestor file outside maspypy; maspypy `suffix_lcp_change` and `basic_substring_structure` stay as decided under P085 in [00-notes.md](00-notes.md); the rest are aliases of `07`, `12`, `22`, `31` |
| https://arxiv.org/abs/1406.7716, https://arxiv.org/abs/2103.00462, https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.CPM.2021.8 | `51`: GLN14 O(n) extra space, O(1) query; BKPR21 O(1) query with linear construction (irreducible LCP values) |

## Fetched 2026-11-08 (omission sweep)

Scope `02`–`07` (P254); decisions are in the rows, the folder notes (Omission sweep 2026-11-08) and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded decisions).

| URL | Used for |
|---|---|
| https://raw.githubusercontent.com/maspypy/library/main/string/periods.hpp | 01: `borders`, `periods`; 27: `wordRoot` |
| https://cp-algorithms.com/string/z-function.html | 02: `DistinctSubstringsDeque`, `distinctSubstrings` |
| https://cp-algorithms.com/string/prefix-function.html | 02: `DistinctSubstringsDeque`, `distinctSubstrings` |
| https://oi-wiki.org/string/kmp/ | 02: `DistinctSubstringsDeque`, `distinctSubstrings` |
| https://oi-wiki.org/string/hash/ | 03: `longestCommonSubstringHash`; 28: `longestCommonSubstringHash` (k strings, added 2026-11-14) |
| https://cp-algorithms.com/string/string-hashing.html | 03: `longestCommonSubstringHash` |
| https://oi-wiki.org/string/pam/ | 08: `minPalindromePartition`; 13: `palindromicLength` |
| https://curiouscoding.nl/posts/approximate-string-matching/ | 17: `myersSearch`; 18: `bandedMyersBitVector`, `optimalStringAlignmentBitVector` |
| https://en.wikipedia.org/wiki/Bitap_algorithm | 17: `myersSearch` |
| https://en.wikipedia.org/wiki/Approximate_string_matching | 17: `myersSearch`; 38: `hammingDistances`, `kMismatch`, `approximateOccurrences` |
| https://www.mi.fu-berlin.de/en/inf/groups/abi/teaching/theses/master_dipl/langner_bitvector/dipl_thesis_langner.pdf | 18: `bandedMyersBitVector`, `optimalStringAlignmentBitVector` |
| https://cses.fi/problemset/task/3404 | 19: `huntSzymanskiWitness` |
| https://cses.fi/problemset/ | 19: `huntSzymanskiWitness` |
| https://oi-wiki.org/string/seq-automaton/ | 20: `shortestUncommonSubsequence` |
| https://oi-wiki.org/dp/dp-of-dp/ | 24: `Dfa`, `fromDp` |
| https://oi-wiki.org/misc/ | 24: `Dfa`, `fromDp` |
| https://en.wikipedia.org/wiki/DFA_minimization | 24: `Dfa`, `minimize`, `Dfa`, `isEmpty` |
| https://en.wikipedia.org/wiki/Regular_language | 24: `Dfa`, `star`, `Dfa`, `isEmpty`, `Dfa`, `countAccepted` |
| https://en.wikipedia.org/wiki/Glushkov%27s_construction_algorithm | 24: `Nfa`, `fromRegex` |
| https://en.wikipedia.org/wiki/Thompson%27s_construction | 24: `Nfa`, `fromRegex`, `Nfa`, `search` |
| https://en.wikipedia.org/wiki/Radix_tree | 25: `RadixTrie`, `predecessor`, `RadixTrie`, `split` |
| https://oi-wiki.org/string/trie/ | 25: `RadixTrie`, `split` |
| https://cp-algorithms.com/string/main_lorentz.html | 26: `countSquares`, `longestSquare` |
| https://oi-wiki.org/string/main-lorentz/ | 26: `countSquares`, `longestSquare` |
| https://www.luogu.com/article/qfa1j1dh | 27: `borderProgressions` |
| https://arxiv.org/abs/1406.0263 | 27: `isSquareFree`, `isOverlapFree` |
| https://en.wikipedia.org/wiki/Square-free_word | 27: `isSquareFree`; 50: `squareFreeTernaryWord`, `morphismFixedPoint` |
| https://en.wikipedia.org/wiki/Combinatorics_on_words | 27: `isOverlapFree`; 50: `squareFreeTernaryWord`, `morphismFixedPoint` |
| https://raw.githubusercontent.com/maspypy/library/main/string/generalized_suffix_automaton.hpp | 28: `GeneralizedSuffixAutomaton`, `build`, `GeneralizedSuffixAutomaton`, `step` |
| https://oi-wiki.org/string/general-sam/ | 28: `GeneralizedSuffixAutomaton`, `build`, `GeneralizedSuffixAutomaton`, `step` |
| https://nyaannyaan.github.io/library/string/rolling-hash-2d.hpp | 32: `Hash2D`, `hashOf` |
| https://hitonanode.github.io/cplib-cpp/string/rolling_hash_2d.hpp | 32: `Hash2D`, `hashOf` |
| https://raw.githubusercontent.com/maspypy/library/main/string/rolling_hash_2d.hpp | 32: `Hash2D`, `hashOf` |
| https://arxiv.org/html/2409.19576 | 33: `popFront` |
| https://arxiv.org/abs/2409.19576 | 33: `popFront` |
| https://arxiv.org/abs/1801.07449 | 33: `popFront`; 44: `SlidingWindowSuffixAutomaton`, `isSubstring` |
| https://oi-wiki.org/string/suffix-tree/ | 33: `step`, `suffixLink` |
| https://topic.alibabacloud.com/a/codeforces-163ee-government-ac-automaton-dfs-sequence--tree-array_8_8_30244620.html | 34: `ToggleAhoCorasick`, `activate` |
| https://oi-wiki.org/string/ac-automaton/ | 34: `ToggleAhoCorasick`, `activate` |
| https://hitonanode.github.io/cplib-cpp/string/incremental_matching.hpp | 34: `OnlineAhoCorasick`, `countPatterns` |
| https://hitonanode.github.io/cplib-cpp/string/aho_corasick_online.hpp | 34: `OnlineAhoCorasick`, `countPatterns` |
| https://oi-wiki.org/string/suffix-bst/ | 35: `SuffixBalancedTree`, `occurrenceCount` |
| https://github.com/xxsds/DYNAMIC | 36: `DynamicFmIndex`, `pushFront`; 46: `RlBwt`, `access` |
| https://arxiv.org/abs/2310.13174 | 38: `hammingDistances` |
| https://arxiv.org/abs/2210.02292v1 | 39: `shortestAbsentPalindromeInRange` |
| https://arxiv.org/abs/2210.02292 | 39: `shortestAbsentPalindromeInRange` |
| https://arxiv.org/abs/1905.08974 | 40: `cartesianTreeMatch` |
| https://en.wikipedia.org/wiki/De_Bruijn_sequence | 41: `deBruijnDecode` |
| https://arxiv.org/abs/1510.02637 | 41: `deBruijnDecode` |
| https://en.wikipedia.org/wiki/LZ77_and_LZ78 | 42: `lzwDecode` |
| https://www-igm.univ-eiffel.fr/~lecroq/string/node26.html | 43: `breslauerTwoWay` |
| https://oi-wiki.org/string/sa-optimal-inplace/ | 43: `inPlaceSuffixSort` |
| https://maspypy.github.io/library/string/double_ended_palindromic_tree.hpp | 45: `DoubleEndedPalindromicTree`, `size` |
| https://gist.github.com/NachiaVivias/6ccf981deb58a53497bffd28b45acf88 | 45: `DoubleEndedPalindromicTree`, `size` |
| https://github.com/yosupo06/library-checker-problems/blob/master/string/palindromes_in_deque/task.md | 45: `DoubleEndedPalindromicTree`, `size` |
| https://arxiv.org/abs/1809.02792 | 46: `RIndex`, `saAccess` |
| https://arxiv.org/abs/2006.05104 | 46: `MoveStructure`, `build` |
| https://arxiv.org/abs/2112.04271 | 46: `MoveStructure`, `build` |
| https://arxiv.org/abs/1802.05906 | 46: `RIndex`, `matchingStatistics` |
| https://arxiv.org/abs/2205.01576 | 46: `RIndex`, `matchingStatistics`, `RIndex`, `maximalUniqueMatches` |
| https://arxiv.org/abs/1705.10382 | 46: `RlBwt`, `countLess` |
| https://arxiv.org/abs/1511.02612 | 47: `DynamicString`, `occurrences` |
| https://arxiv.org/abs/1605.09558 | 47: `DynamicString`, `occurrences` |
| https://arxiv.org/abs/0707.3619 | 48: `maxCliqueCircleGraph` |
| https://en.wikipedia.org/wiki/Fibonacci_word | 50: `fibonacciBit`, `morphismFixedPoint` |
| https://en.wikipedia.org/wiki/Sturmian_word | 50: `standardWord`, `isBalancedWord` |
| https://en.wikipedia.org/wiki/Multi-key_quicksort | 52: `multikeyQuicksort`, `TernarySearchTree` |
| https://en.wikipedia.org/wiki/Ternary_search_tree | 52: `multikeyQuicksort`, `TernarySearchTree` |
| https://sedgewick.io/ | 52: `multikeyQuicksort`, `TernarySearchTree` |
| https://en.wikipedia.org/wiki/CYK_algorithm | 53: `Cfg`, `addRule` |
| https://oi-wiki.org/misc/fsm/ | 53: `Cfg`, `addRule` |
| https://en.wikipedia.org/wiki/Suffix_automaton | 54: `Cdawg`, `build` |

## Prior source keys and audit narrative (2026-09-27 and 2026-09-28, condensed)

- The 2026-09-27 cross-library audit (`00-Guidelines/History/2026-09-27-inventory-audit.md`, records in `Ledgers/research-sources.json`) compared catalog navigation and selected API comments, not source correctness; it grew this folder from 35 to 49 rows (14 added). Paper abstracts identified advanced variants (dynamic strings, semi-local LCS, runs, r-index); their proofs remain implementation-time reading.
- Targeted pages read then: maspypy `inverse_manacher.hpp` (odd-only radii), `online_z_algorithm.hpp`, `double_ended_palindromic_tree.hpp`, `prefix_substring_LCS.hpp` (wavelet-matrix dependency); OI Wiki navigation including suffix balanced tree, in-place suffix sorting, subsequence automaton and Main–Lorentz; Lecroq's complete table of contents (the univ-mlv hostname had a certificate mismatch; the univ-eiffel successor was used).
- Library Checker coverage (`00-Guidelines/Ledgers/library-checker-coverage.json`) maps string problems to rows; status there is "inventoried", not accepted.
- Legacy: no string implementation beyond those in [00-notes.md](00-notes.md) (Legacy obligations) was found in `OLD/algorithms.cpp` or `OLD/[1] algorithms.cpp`.

## Not adopted

- cp-algorithms "Expression parsing": owned by Miscellaneous `37-expression_parser.hpp`.
- Nyaan `misc/compress.hpp` and `math/enumerate-quotient.hpp`: coordinate compression and quotient enumeration are not string operations.
- maspypy `substring_abundant_string.hpp`: a single-problem construction with no named family in any other catalog; `suffix_lcp_change` and `non_dominated_suffix` are decided under P085 in [00-notes.md](00-notes.md).
- maspypy `split.hpp`, `binary_lyndon_list.txt`, `suffix_tree.png`: utility, data and image files.
- suisen `morris_pratt.hpp`: the MP border table is `prefixFunction`; no separate operation.
- hitonanode `suffix_array_doubling.hpp`, `longest_common_prefix.hpp`: covered by `07` radix doubling and `lce`.
- OI Wiki "Standard Library" page: C++ `std::string` usage, not an algorithm.

## Covered by existing rows

- Lecroq Brute Force and DFA pages: naive scan and `dfaSearch` appear in `16`, `prefixAutomaton` in `01`; no historical row entry.
- KACTL/ACL: no item excluded; every entry maps to an existing row.
- Library Checker `enumerative_combinatorics/number_of_subsequences`: already mapped to row `20`.

## Fetched 2026-11-10 (P275 re-review)

Pages the P275 re-review agents read to confirm the returns in `Docs/00-notes.md` (P275 re-review 2026-11-10).

- <https://raw.githubusercontent.com/maspypy/library/main/string/basic_substring_structure.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/string/substring_abundant_string.hpp>
- <https://suisen-cp.github.io/cp-library-cpp/library/string/substring_set.hpp>
- <https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/main/library/util/run_length_encoder.hpp>
- <https://cp-algorithms.com/string/prefix-function.html>
- <https://en.wikipedia.org/wiki/Maximal_pair>
- <https://en.wikipedia.org/wiki/Gap_penalty>
- <https://en.wikipedia.org/wiki/Levenshtein_automaton>
- <https://en.wikipedia.org/wiki/Move-to-front_transform>
- <https://www.luogu.com.cn/problem/P4555>
- <https://www.luogu.com.cn/problem/P1819>
- <https://www.luogu.com.cn/problem/CF10D>

## ST01 re-sweep (2026-11-12)

Rows `01` and `02` (batch ST01): nothing missing beyond `DistinctSubstringsDeque`; no catalog defines a double-ended distinct-substring counter with more operations or a better bound than one Z pass per update (cp-algorithms and OI Wiki give the append formula and say the other ends work alike). Naive O(n^3)/O(n^2) prefix functions, naive Z and Z by hash with binary search are dominated slower forms.

| URL | Used for |
|---|---|
| https://cp-algorithms.com/string/z-function.html, https://oi-wiki.org/string/z-func/ | 02: `DistinctSubstringsDeque` contract (non-empty substrings, length minus max z) |
| https://cp-algorithms.com/string/prefix-function.html, https://oi-wiki.org/string/kmp/ | 01: every application section; aliases |
| https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/KMP.h, https://github.com/kth-competitive-programming/kactl/tree/main/content/strings | 01, 02: aliases only |
| https://atcoder.github.io/ac-library/production/document_en/string.html | 01: ACL has no prefix function |
| https://raw.githubusercontent.com/maspypy/library/main/string/kmp.hpp, https://maspypy.github.io/library/string/z_algorithm.hpp | 01, 02: aliases only |
| https://hitonanode.github.io/cplib-cpp/string/mp_algorithm.hpp, https://hitonanode.github.io/cplib-cpp/string/z_algorithm.hpp | 01, 02: aliases only |
| https://nyaannyaan.github.io/library/, https://ei1333.github.io/library/, https://suisen-cp.github.io/cp-library-cpp/ | 01, 02: no further operation (index pages, first 100k characters) |
| https://github.com/cheran-senthil/PyRival/tree/master/pyrival/strings | 01, 02: aliases only; `string_find` is row `16` |
| https://github.com/yosupo06/library-checker-problems/tree/master/string | 02: `zalgorithm` covered; no deque distinct-substring problem |
| https://api.github.com/repos/ei1333/library/contents/string, https://api.github.com/repos/ei1333/library/git/trees/master?recursive=1, https://raw.githubusercontent.com/ei1333/library/master/string/z-algorithm.hpp | `/finish` follow-up 2026-11-13: ei1333 has no KMP/MP file; its `ZAlgorithm` is incremental Z (row `30`) |
| https://api.github.com/repos/NyaanNyaan/library/contents/string, https://raw.githubusercontent.com/NyaanNyaan/library/master/string/z-algorithm.hpp, https://raw.githubusercontent.com/NyaanNyaan/library/master/string/string-search.hpp | `/finish` follow-up: `z_algorithm` is `zFunction`; no KMP/MP file; `string-search` is suffix-array search |
| https://api.github.com/repos/suisen-cp/cp-library-cpp/contents/library/string, https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/main/library/string/morris_pratt.hpp | `/finish` follow-up: `morris_pratt` is `prefixFunction` (n + 1 entries, `-1` first), `all_periods` is `prefixPeriods` (with a divisor filter), `min_period` is `prefixPeriod`, `MatcherMP` is `kmpOccurrences` |
| https://raw.githubusercontent.com/emthrm/cp-library/master/include/emthrm/string/morris-pratt.hpp, https://raw.githubusercontent.com/emthrm/cp-library/master/include/emthrm/string/knuth-morris-pratt.hpp, https://raw.githubusercontent.com/emthrm/cp-library/master/include/emthrm/string/z_algorithm.hpp | `/finish` follow-up: `border`/`match` are `prefixFunction`/`kmpNext`/`kmpOccurrences`; `period(idx)` is `idx - pi[idx - 1]`; `MorrisPratt::add` (append to the pattern, amortized O(1)) is the one candidate absent from row `01` |

## ST02 re-sweep (2026-11-14)

Rows `03`, `04` and `05` (batch ST02): nothing missing beyond `longestCommonSubstringHash`. The omissions and dominated forms are listed in [00-notes.md](00-notes.md) (P016 package record).

| URL | Used for |
|---|---|
| https://oi-wiki.org/string/hash/, https://cp-algorithms.com/string/string-hashing.html | 03: `longestCommonSubstringHash` (two strings); k strings to `28`; dominated hash applications |
| https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/Hashing.h | 03: the modulo `2^64 - 1` ring, dominated |
| https://raw.githubusercontent.com/maspypy/library/main/string/rolling_hash.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/rolling_hash_field.hpp, https://raw.githubusercontent.com/maspypy/library/main/string/longest_common_substring.hpp | 03: aliases; the suffix-array LCS is row `07` |
| https://raw.githubusercontent.com/maspypy/library/main/graph/ds/rolling_hash_on_tree.hpp | 03: tree-path hash, the unowned handoff |
| https://nyaannyaan.github.io/library/string/rolling-hash.hpp, https://ei1333.github.io/library/string/rolling-hash.hpp, https://suisen-cp.github.io/cp-library-cpp/library/string/rolling_hash.hpp, https://hitonanode.github.io/cplib-cpp/string/rolling_hash_1d.hpp | 03: aliases only |
| https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/hashing.py | 03: single-prime two-base hashing, dominated |
| https://oi-wiki.org/string/trie/, https://raw.githubusercontent.com/maspypy/library/main/string/trie.hpp, https://nyaannyaan.github.io/library/string/trie.hpp, https://ei1333.github.io/library/structure/trie/trie.hpp, https://hitonanode.github.io/cplib-cpp/string/trie_light.hpp | 04: aliases only; binary-trie applications are Data Structures `26`/`72` |
| https://cp-algorithms.com/string/manacher.html, https://oi-wiki.org/string/manacher/, https://raw.githubusercontent.com/maspypy/library/main/string/manacher.hpp, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/LPSubstr.py | 05: aliases; dominated radius methods |
| https://github.com/yosupo06/library-checker-problems/tree/master/string | 03–05: no further problem |
