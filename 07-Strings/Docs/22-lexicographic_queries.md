# 22-lexicographic_queries.hpp — evidence

`22-lexicographic_queries.hpp` (batch ST09, package P085) holds the lexicographic queries over the substrings of one text or a string collection. Most are free functions on a caller-built `SuffixArray<T>` (`07-suffixarray.hpp`), and four structs own extra precomputation:

- `BidirectionalSuffixArray`: forward and reversed suffix arrays.
- `StringCollection`: one joined index.
- `DictionaryOfBasicFactors`: KMR names.
- `SubstringOrder`: prefix sums for kth and rank queries.

The per-prefix minimal suffix uses `IncrementalLyndon` (`14-lyndon.hpp`). The per-prefix maximal suffix is an online border-chain scan. Contest profile, GNU C++20, no ISA code.

## Contracts

Texts are integral sequences (`vector<T>`); a string is read as unsigned bytes into `T = int` (`SuffixArray(string_view)` and the string constructors), so byte order is unsigned. Sizes are below `INT_MAX`. Ranges are half-open; comparisons return -1, 0, 1, a proper prefix being smaller. Free functions taking a `SuffixArray` need it built with RMQ (the default) wherever `lce` is used (asserted through `indexed`); `frequencyStatistics`, `lcpSums`, `minimalAbsentWords` and the MUS/SUS functions read only `sa`, `rank` and `lcp`. Results never alias inputs.

### compareSubstrings(sa, l, r, a, b)

`O(1)`: `k = sa.substringLce(l, r, a, b)`; when `k` reaches either length the shorter range is smaller, otherwise the symbols at `l + k` and `a + k` decide.

### BidirectionalSuffixArray

Owns `forward` (the text) and `backward` (its reversal), both with RMQ; `O(n log n)` setup and memory. `longestCommonSuffix(l, r, a, b)` is the longest common suffix of `s[l, r)` and `s[a, b)`, `O(1)`, through `backward.substringLce(n - r, n - l, n - b, n - a)`; forward LCE queries use `forward.lce`/`substringLce` directly (kept by the user's decision 2026-10-16).

### StringCollection

`StringCollection(strings)` takes `vector<S>` for `S = string` (bytes) or `vector<T>`. Symbols are ranked over the union alphabet and shifted to `k + rank`. String `i` ends with the unique separator `i < k`, so no common prefix crosses a string end, and the joined `SuffixArray<int>` (with RMQ, `O(L log L)` for `L` = total length + `k`) answers in `O(1)`:

- `length(i)`: the length of `strings[i]`.
- `lce(i, l, j, a)`: the common prefix of `strings[i][l, ..)` and `strings[j][a, ..)`.
- `compare(i, l, r, j, a, b)`: `strings[i][l, r)` against `strings[j][a, b)`.

`comparePieces(x, y)` compares two strings given as piece lists `{i, l, r}`, each piece `strings[i][l, r)` in order. It walks both lists with one `lce` per step, `O(|x| + |y|)`; empty pieces are skipped and equal concatenations give 0. Comparing `a + b` with `b + a` is `comparePieces({{a, 0, |a|}, {b, 0, |b|}}, {{b, 0, |b|}, {a, 0, |a|}})`.

### DictionaryOfBasicFactors

Karp–Miller–Rosenberg naming. `names[k][i]` is the dense rank of `s[i, i + 2^k)` among the factors of length `2^k` (`i + 2^k <= n`). Level 0 ranks the sorted distinct symbols. Level `k + 1` ranks the pairs `(names[k][i], names[k][i + 2^k])` by two counting-sort passes, so the build (constructor) is `O(n log n)` in time and memory, with `bit_width(n)` levels. Queries:

- `name(k, i)`: asserted in range.
- `equal(l, r, a, b)`: `O(1)` with two overlapping blocks of length `2^floor(log len)`.
- `compare(l, r, a, b)`: `O(1)`. The two blocks are compared as pairs; the first difference of equal-length ranges always falls in the first block, or else in the second, which starts before the first block ends.
- `lcp(i, j)`: the common prefix of suffixes `i, j` in `[0, n]`, `O(log n)` by descending the levels; `lcp(i, i) = n - i`.

### occurrenceRange(sa, l, r), occurrenceCount(sa, l, r)

`occurrenceRange` returns the SA ranks `[lo, hi)` whose suffixes start with `s[l, r)`, by two binary searches on the LCP RMQ around `rank[l]` (`O(log n)`), and `[0, n)` for the empty range. `occurrenceCount` returns the number of occurrences with overlaps: `hi - lo`, and `n + 1` for the empty range (the empty string occurs at every position `0..n`).

### sortSubstrings(sa, substrings)

`res[i]` is the dense rank (0-based; equal strings share one) of `s[l_i, r_i)` among the given substrings. The key `(lo of occurrenceRange, length)` orders substrings exactly as their strings do:

- If one string is a proper prefix of the other, its range contains the longer one's range.
- Otherwise the two ranges are disjoint and ordered.

The empty range has key `(0, 0)` and is smallest. `O(q log(n + q))`.

### SubstringOrder

Owns `index` (a `SuffixArray<T>` with RMQ), a sparse table `leftmost` over `sa` for leftmost starts, prefix sums `distinct` and `total` over ranks, and `later[x]` (the sum of `lce(sa[x], sa[y])` over `y > x`). Setup and memory are `O(n log n)`. All `k` are 0-based. The kth queries return `{start, length}` with the leftmost start among the occurrences, or `{-1, 0}` when `k < 0` or `k` is at least the count; this is the convention of `10-suffixautomaton.hpp`.

- `kthDistinctSubstring(k)`: over the distinct nonempty substrings, `O(log n)`. Rank `r` introduces the prefixes of suffix `sa[r]` longer than `lcp[r - 1]`.
- `kthSubstring(k)`: over all `n(n+1)/2` occurrences, equal strings adjacent, `O(log^2 n)`. For `X` with occurrence ranks `[p, q)`, the number of occurrences at most `X` is `total[p] + (q - p)|X| + later[q - 1]`:
  - ranks below `p` count every prefix;
  - ranks in `[p, q)` count `|X|` prefixes;
  - ranks from `q` count their common prefix with rank `q - 1`.
  The search first finds the rank whose suffix first exceeds `k`, then binary searches the length.
- `distinctSubstringRank(l, r)` and `substringRank(l, r)`: the numbers of distinct, or of all, nonempty substrings smaller than `s[l, r)`, both `O(log n)` and 0 for the empty range. Each is the inverse of its kth query.

### frequencyStatistics(sa)

`res[k]` (`k` in `0..n`) is the number of distinct nonempty substrings occurring exactly `k` times, overlaps counted. Each LCP interval of size `w`, depth `d` and parent depth `pd` adds `d - pd` to `res[w]`. Each rank adds to `res[1]` its suffix length minus the larger adjacent LCP. `O(n)` with one stack.

### lcpSums(sa)

`res[i]` is the sum of `lce(i, j)` over all `j` in `[0, n)`, including `j = i` (which adds `n - i`); this is AtCoder ABC 213 F. It is `n - i` plus the sums of range minima of the LCP array to the left and right of `rank[i]`, each computed by a monotone stack (`O(n)`). The sum over all ordered pairs is `sum_k k^2 res_freq[k]` (tested).

### minimalAbsentWords(sa, sigma)

Symbols must lie in `[0, sigma)` (asserted). A word is a minimal absent word when it does not occur and both of its maximal proper factors do. Each one is reported once as `{b, start, length}`, meaning the word `b + s[start, start + length)`:

- An absent symbol `c` is `{c, 0, 0}`.
- Every longer one is `b P a` for a suffix-tree node `P` (an LCP interval, including suffixes that are prefixes of others), a child edge starting with `a`, and a left extension `b` of `P` that is not a left extension of that child. `start` is the leftmost occurrence of `P a` and `length = |P| + 1`.

The tree is traversed bottom-up over the LCP array. Each node's left-extension set (the symbols before its occurrences, plus `s[n-1]` before the empty suffix at the root) is the deduplicated union of its children's sets. The sizes of all these sets sum to the transition count of the suffix automaton of the reversed text, at most `3n`. So `T` and `M` are `O(n + sigma + out)`, with mark arrays of size `sigma`. Order: traversal order, deterministic, not sorted.

### minimalUniqueSubstrings(sa), shortestUniqueSubstrings(sa)

A minimal unique substring occurs exactly once while its proper substrings repeat. The one starting at `i` can only be `s[i, i + L_i)`, where `L_i` is one plus the larger adjacent LCP of `rank[i]`, provided it fits. It qualifies exactly when `s[i + 1, i + L_i)` repeats, that is when `L_{i+1}` is undefined or `L_{i+1} >= L_i`. The function returns `{start, length}` ascending by start, which is also ascending by end. `O(n)`.

`shortestUniqueSubstrings` returns, for every position `p`, the shortest substring that covers `p` and occurs once, with the leftmost start on ties. Every unique substring contains an MUS, so the answer is the shortest hull of `p` with an MUS. The candidates are the shortest MUS covering `p` (a sliding-window minimum), the last MUS ending before `p` and the first MUS starting after it. `O(n)`; the whole text is unique, so every position has an answer.

### lexMinSuffixPerPrefix(s), lexMaxSuffixPerPrefix(s)

Any random-access sequence (`string`, `string_view`, `vector<T>`; chars as unsigned bytes). Both return `n + 1` entries, `res[0] = 0`: `res[i]` is the start of the smallest (respectively largest) nonempty suffix of `s[0, i)`. Proper prefixes are smaller.

`lexMinSuffixPerPrefix` is `i - IncrementalLyndon::minSuffix(i)` (Duval online; kept as the batch form by the user's decision).

`lexMaxSuffixPerPrefix` keeps the maximal suffix `x` of the current prefix and its prefix function. Two facts drive it, both proved by comparing suffixes of `x` with `x`:

1. The maximal suffix of the extended prefix is `z c` for `x` itself or a border `z` of `x`.
2. The next symbols `d(z) = x[|z|]` are nondecreasing as the borders shorten.

So `z c` beats every longer candidate exactly when `d(z) < c`. The new maximal suffix is `z c` for the shortest border with `d(z) < c`, or `x c` when no border has one. One walk down the border chain finds it, and the new prefix-function entry needs one comparison. The chain depth is a potential that pays for the walk: `O(n)` total, online, no suffix array.

### distinctSubstringsPerPrefix(sa)

`n + 1` entries; `res[i]` is the number of distinct nonempty substrings of `s[0, i)`. Let `a(j)` be the maximum of `lce(j', j)` over `j' < j`, found from the nearest ranks on either side with a smaller start by two monotone stacks. A substring starting at `j` is new there when it is longer than `a(j)`. So `res[i]` is the sum of `max(0, i - j - a(j))`, accumulated by bucketing `j + a(j)` in `O(n)` after the suffix array. This is the integer-sequence form beside `distinctSubstringsOnline(string_view)` in `10` (kept by the user's decision 2026-10-16).

### Legacy mapping

- `lce` (row) → `SuffixArray::lce` (`07`), removed from the row as the same operation.
- `allPairsLcp` (row) → `lcpSums`, the per-suffix sum of LCPs.
- `shortestUniqueSubstring (position query)` (row) → `shortestUniqueSubstrings`, all positions at once in `O(n)`.
- `compareSubstringsMany` (row) → `StringCollection::compare`, `lce` and `comparePieces`.

### Aliases

maspypy `lcp_range` is `occurrenceRange`; maspypy `sort_substrings` (dense IDs) is `sortSubstrings`; maspypy `Many_String_Compare` `ST_TS_comp3` is `comparePieces` on two orders; suisen `compare_substring` is `compareSubstrings`; suisen `SubstringSet::kth_element` is `SubstringOrder::kthDistinctSubstring`; maspypy `lex_min_suffix_for_all_prefix` and `lex_max_suffix_for_all_prefix` are the per-prefix functions (lengths there, starts here).

## Feature-to-test map

Runner: [`22-lexicographic_queries_tester.py`](<../../96-Local Testing/07-Strings/22-lexicographic_queries_tester.py>). Oracle: brute enumeration of every substring (occurrence lists, sorted distinct and multiset orders, sorted suffixes), brute scans for prefixes and suffixes, a brute MAW construction from the substring set (every `a u b` absent with `a u` and `u b` occurring, plus the absent symbols of `[0, sigma)`), and brute per-prefix sets and extrema. Large texts: the suffix automaton (`10`, verified independently) and counting identities. Every check survives `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| compareSubstrings, longestCommonSuffix, DictionaryOfBasicFactors (equal, compare) | All range pairs for texts up to 6 symbols, 3000 sampled pairs otherwise. |
| DictionaryOfBasicFactors (name, lcp, size, build) | Every level and position against brute dense ranks; level count; `lcp` on all (or 1/8 of) pairs in `[0, n]`. |
| occurrenceRange, occurrenceCount | Every range (empty included) against the sorted suffixes and occurrence counts; large texts against `SuffixAutomaton::occurrenceCount`. |
| sortSubstrings | 40 sampled ranges with duplicates and empty ranges per text; the empty list. |
| SubstringOrder (kthDistinctSubstring, kthSubstring, distinctSubstringRank, substringRank) | Every `k` on small texts, sampled `k` otherwise, `-1` and one past the end, ranks on every (or 1/4 of) range. Large texts: `SuffixAutomaton::kthSubstringDistinct`/`kthSubstring` on 200 random `k`, and the rank inverses. |
| frequencyStatistics, lcpSums | Brute histograms and brute LCP sums. Large texts: occurrence and distinct totals, and `sum lcpSums = sum k^2 freq[k]`. |
| minimalAbsentWords | Set equality, no duplicates, leftmost starts, `sigma` = 2, 3, 26 (with absent symbols). Large texts: sampled words absent and their factors present (automaton). |
| minimalUniqueSubstrings, shortestUniqueSubstrings | Brute definitions. Large texts: each MUS unique with repeating parts; sampled SUS cover `p`, occur once, and are minimal when at most 300 long. |
| lexMinSuffixPerPrefix, lexMaxSuffixPerPrefix | Brute on every small text; medium texts (300/1000/3000: random binary, Fibonacci, 26 letters, Thue–Morse) against brute extrema. |
| distinctSubstringsPerPrefix | Brute per-prefix sets; large texts against `distinctSubstringsOnline`. |
| StringCollection (length, lce, compare, comparePieces) | Random collections of 0..4 strings up to 8 symbols, 300 queries each (equal piece lists included); string input with high bytes and the empty string. |
| Suffix arrays without RMQ | `frequencyStatistics`, `lcpSums`, `minimalAbsentWords`, `minimalUniqueSubstrings`, `shortestUniqueSubstrings` on `SuffixArray(s, false)` give the same results on every text. |
| Symbol types | `lng` symbols with negative values, high-byte strings (`SuffixArray(string)`, string constructors and deduction guides, `string_view`) against an equivalent `int` rank text. |
| Preconditions | Checked builds abort on eleven probes: compare and suffix ranges, collection range and index, KMR level and range, occurrence range, a suffix array without RMQ, MAW symbol and `sigma`, rank range. |

Inputs: exhaustive binary texts through 7/9/12 and ternary through 4/6/7 symbols (quick/full/stress); 30/200/800 random texts up to 25/45/60 symbols (Fibonacci, Thue–Morse, periodic, alphabets 1..4 and 26) and as many collections; 20/100/400 symbol-type cases; medium texts of 300/1000/3000 symbols; large texts of 20000/50000/300000 symbols (random over 3, Fibonacci, period 5). The checked and ASan/UBSan builds run stress at full-mode sizes: their brute oracles allocate heavily and stress sizes exceed the runner's 180 s per-command limit there; the optimized build runs the stress sizes.

## Commands and results

Run 2026-10-16 on the final code. Every run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, precondition probes) binaries, and full/stress add ASan/UBSan (`-O1`); all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress first pass the quick pre-flight under the 2048 MB cap. The checked and ASan/UBSan builds run stress at full-mode sizes.

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 23 suites (this one: 2 configurations, 11 assertion probes)
python3 '96-Local Testing/07-Strings/22-lexicographic_queries_tester.py' --mode full --seed 1  # PASS, 3 configurations, 17,248,002 checks each, 11 assertion probes, MEMORY peak 725 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/22-lexicographic_queries_tester.py' --mode full --seed 2  # PASS, 3 configurations, 17,234,545 checks each, MEMORY peak 721 MB
python3 '96-Local Testing/07-Strings/22-lexicographic_queries_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 141,927,003 checks (optimized), 17,244,372 (checked, ASan/UBSan), MEMORY peak 725 MB
python3 '96-Local Testing/02-integration.py'  # PASS, 146 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'  # PASS, errors: []
```

## Benchmarks

No dispatch threshold and no Barrett/Montgomery: every operation is one algorithm, so no benchmark is required.

## Sources

Code is independently implemented from the cited descriptions; catalog sweep in [00-sources.md](00-sources.md#p085-sweeps-2026-10-16).

- [cp-algorithms, Suffix array](https://cp-algorithms.com/string/suffix-array.html) and [OI Wiki, SA](https://oi-wiki.org/string/sa/) (fetched 2026-10-16): substring comparison, prefix doubling names, the sum of LCPs over all pairs.
- maspypy `string/{all_pairs_lcp, many_string_compare, sort_substrings, lex_max_suffix_for_all_prefix, lex_min_suffix_for_all_prefix, suffix_lcp_change}.hpp` and suisen `compare_substring`, `substring_set` (fetched 2026-10-16 by the researcher): conventions (half-open ranges, -1/0/1, per-prefix arrays of `n + 1`, 0-based `k`).
- [arXiv 1312.2738](https://arxiv.org/abs/1312.2738) and [Mieno et al., CPM 2017](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.CPM.2017.24) (fetched 2026-10-16): minimal unique substrings and point SUS queries. Barton et al. 2014 and Crochemore–Mignosi–Restivo 1998 on minimal absent words were seen in search results only. The node/left-extension formulation and its `O(n + sigma + out)` bound are argued above.
- The online maximal-suffix-per-prefix scan is derived here; no fetched source gives an online version (maspypy's is offline over the suffix array).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): maspypy `suffix_lcp_change`, `nonDominatedSuffixes`, the basic substring structure, ordered-set queries over arbitrary patterns, the two-string LCP table.
- `minimalAbsentWords` allocates `O(sigma)`; compress large alphabets first.
- `kthSubstring` is `O(log^2 n)` per query; the automaton in `10` answers in `O(answer length)` for byte strings.

## History

- 2026-10-16: implemented and verified (P085), independent review found 4 rule and test-gap findings, fixed; full suite passed on g++ and g++-14.
