# 07 Strings — sources

Source keys used in `00-index.md` status cells are defined in the second section. A key is a research starting point, never correctness evidence.

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

## Prior source keys and audit narrative (2026-09-27 and 2026-09-28, condensed)

Keys used in status cells: KACTL, CPALG (cp-algorithms), OI-STR (OI Wiki string section), MASPYPY, NYAAN, EI1333, SUISEN, HITONANODE, TKO919, ACL, YOSUPO (Library Checker), CSES, LECROQ, RUNS (arXiv 1406.0263), RINDEX (arXiv 1705.10382), DYNAMICSTRINGS (arXiv 1511.02612, Gawrychowski et al., Optimal Dynamic Strings), SEMILOCAL (arXiv 0707.3619, Tiskin, Semi-local string comparison), DS17 (Data Structures succinct engines batch).

- The 2026-09-27 cross-library audit (`00-Guidelines/16-inventory-audit.md`, records in `17-research-sources.json`) compared catalog navigation and selected API comments, not source correctness; it grew this folder from 35 to 49 rows (14 added). Paper abstracts identified advanced variants (dynamic strings, semi-local LCS, runs, r-index); their proofs remain implementation-time reading.
- Targeted pages read then: maspypy `inverse_manacher.hpp` (odd-only radii), `online_z_algorithm.hpp`, `double_ended_palindromic_tree.hpp`, `prefix_substring_LCS.hpp` (wavelet-matrix dependency); OI Wiki navigation including suffix balanced tree, in-place suffix sorting, subsequence automaton and Main–Lorentz; Lecroq's complete table of contents (the univ-mlv hostname had a certificate mismatch; the univ-eiffel successor was used).
- Library Checker coverage (`00-Guidelines/20-library-checker-coverage.json`) maps string problems to rows; status there is "inventoried", not accepted.
- P016/P017 (2026-09-28) inspected for the nine Basic headers: cp-algorithms prefix-function, Z, string hashing, Aho–Corasick, suffix array and Manacher articles; OI Wiki Z and trie pages; Nyaan `z-algorithm.hpp`, `rolling-hash.hpp`; ACL `string.hpp` Z and suffix-array overloads; KACTL `Hashing.h`, `SuffixArray.h`, `AhoCorasick.h`, `Manacher.h`; maspypy `run_length.hpp`. All code was written independently from the invariants; details and legacy accounting are in `90`–`97`.
- Legacy: `OLD/Team Notebook/src/misc/old_zalgo.cpp` (static Z, accounted for in `02`); Team Notebook `algs.cpp`/`algsbetter.cpp` trie/KMP routines (accounted for in `01`/`04`); no other owned string implementation was found in `OLD/algorithms.cpp` or `OLD/[1] algorithms.cpp`.

## Not adopted

- cp-algorithms "Expression parsing": owned by Miscellaneous `37-expression_parser.hpp`.
- Nyaan `misc/compress.hpp` and `math/enumerate-quotient.hpp`: coordinate compression and quotient enumeration are not string operations.
- maspypy `suffix_lcp_change.hpp`, `non_dominated_suffix.hpp`, `substring_abundant_string.hpp`: single-problem constructions whose API was not inspected beyond the file name; no named family in any other catalog.
- maspypy `split.hpp`, `binary_lyndon_list.txt`, `suffix_tree.png`: utility, data and image files.
- suisen `morris_pratt.hpp`: the MP border table is `prefixFunction`; no separate operation.
- hitonanode `suffix_array_doubling.hpp`, `longest_common_prefix.hpp`: covered by `07` radix doubling and `lce`.
- Lecroq Brute Force and DFA pages: naive scan and `prefixAutomaton`/`dfaSearch` already appear in `16`; no historical row entry.
- OI Wiki "Standard Library" page: C++ `std::string` usage, not an algorithm.
- KACTL/ACL: no item excluded; every entry maps to an existing row.
- Library Checker `enumerative_combinatorics/number_of_subsequences`: already mapped to row `20`.
