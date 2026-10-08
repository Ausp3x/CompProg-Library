# 07-suffixarray.hpp — evidence

`07-suffixarray.hpp` implements the Basic suffix-array row. Construction uses stable counting-sort doubling, inverse ranks and Kasai LCP. Exact LCE and accelerated pattern search reuse the verified Data Structures `SparseTable`; callers may omit RMQ to retain linear memory. Two-input longest common substring, longest repeated substring (at least `k` overlapping occurrences) and longest disjoint repeat include deterministic witnesses.

## Contracts

### SuffixArray, longestCommonSubstring

- `SuffixArray<T>` owns a `vector<T>` for integral `T`; the input's ordinary signed/unsigned order is preserved without narrowing symbols. A brace list (`SuffixArray<int> x({0, 1})`, `x.patternRange({0})`) selects the `initializer_list<T>` overloads, so lists starting with a literal `0` are not ambiguous with `string_view`. `SuffixArray(string_view)` and deduction from `string`/C strings use `T=int` and unsigned byte order, including NUL and bytes 128–255. A C string follows its normal terminating-NUL convention; use an explicit `string_view` length for embedded NUL.
- Input length `n < INT_MAX`. `sa` contains exactly the `n` nonempty suffix starts, `rank[sa[k]]=k`, and `lcp[k]=LCP(sa[k],sa[k+1])` has `max(0,n-1)` entries. Construction is sentinel-free; no input value is reserved. Empty construction is valid.
- Byte initialization costs `O(n+256)`; sparse integer initialization sorts positions to compress symbols in `O(n * log(n+1))` time and `O(n)` workspace. Stable radix doubling takes `O(n * log(n+1))` time and `O(n)` workspace, stopping once all ranks differ. Kasai takes `O(n)` time.
- Default construction builds RMQ in `O(n * log(n+1))` time and stored space. Pass `false` as the second constructor argument for `O(n)` storage, then call `buildRmq()` if LCE becomes necessary. Repeated `buildRmq()` calls are valid. No hidden lazy mutation occurs in queries.
- `lce(i,j)` is exact and requires RMQ plus `0 <= i,j <= n`. It returns `n-i` when `i==j` and zero if exactly one argument is the empty suffix `n`. `substringLce(l,r,a,b)` accepts half-open intervals, including empty intervals, and caps the suffix answer at both interval lengths. Both queries cost `O(1)`.
- `patternRange(pattern)` returns ranks `[l,r)` in `sa`. Results enumerate occurrences as `sa[l..r)` in suffix order; an absent pattern has `l==r`. An empty pattern returns `[0,n)`: boundary occurrence `n` is omitted because it has no `sa` entry. With RMQ the query costs `O(m+log(n+1))`; without RMQ it costs `O(m * log(n+1))`, where `m` is the pattern length. The byte overload uses `O(m)` conversion storage; the integer overload uses constant query workspace and requires the same symbol type as the index; the `initializer_list` overload copies the list (`O(m)`).
- `longestRepeated(k = 2)` returns `{start,length}` of the longest substring with at least `k >= 1` (asserted) occurrences, overlaps allowed. `k = 1` gives the whole text `{0,n}`; no answer (including `k > n`) gives `{0,0}`. Equal lengths select the lexicographically smallest such substring, then its earliest start. It takes the maximum over windows of `k-1` adjacent LCP entries of the window minimum with a monotonic deque, then scans the selected LCP group: `O(n)` time, `O(min(k, n))` deque workspace, no RMQ.
- `longestRepeatedDisjoint()` returns `{first,last,length}` of the longest substring with two non-overlapping occurrences (`last - first >= length`), choosing the lexicographically smallest such substring; `first`/`last` are its earliest and latest starts. No answer gives `{0,0,0}`. Binary search on the length, each probe scanning LCP groups for the start spread: `O(n * log(n+1))` time, `O(1)` workspace, no RMQ.
- `longestCommonSubstring(a,b)` returns `{start_in_a,start_in_b,length}`. No common symbol returns `{0,0,0}`. Ties choose the lexicographically smallest common substring, then its earliest start independently in each input. Both vectors must use the same integer type; byte overloads use unsigned byte order. It requires `n+m+1 < INT_MAX`, compresses the union alphabet to positive ranks and uses a fresh separator zero, independent of all original symbol values. Time is `O((n+m) * log(n+m+1))`, storage/workspace `O(n+m)`; this reduction omits RMQ. Rank initialization of the joined sequence uses the same tested constructor.
- Fields are read-only after construction; replace the object to change its input. Copy and move destinations preserve their indexes. Assign a valid object to a moved-from object before querying it. The exposed `build(bytes,with_rmq)` construction helper operates on the owned text; its byte path requires all values in `[0,256)`, checked through a 128-bit comparison so every integral `T` (including `bool` and wide unsigned values) is exact and warning-free.

Correctness: at doubling width `k`, each current rank describes the first `k` symbols of its suffix. Suffixes shorter than or equal to `k` have a missing second rank, smaller than every real rank. Listing those starts first, then subtracting `k` from the current suffix order where possible, sorts all starts by second rank. Stable counting sort on first ranks therefore sorts the rank pairs. Consecutive unequal pairs receive consecutive ranks, preserving equality and lexicographic order for width `2k`. Eventually the width covers every suffix; unique ranks permit early termination. Missing ranks use the internal value `-1`, never a text symbol. Width and index calculations avoid overflowing signed sums.

Kasai scans starts in text order. Removing a common leading symbol lowers the known adjacent-suffix LCP by at most one; the lexicographic neighbors of the shortened suffix share at least that remaining prefix. Consequently the carried lower bound is valid and the total extensions are linear. The LCP of any two suffixes equals the minimum adjacent LCP between their ranks: all suffixes sharing a prefix form a contiguous lexicographic interval. The sparse-table minimum therefore answers LCE exactly.

Pattern binary search retains an anchor suffix having the largest prefix match obtained so far. If the next suffix diverges from the anchor before that matched prefix ends, suffix-rank order determines its comparison with the pattern. Otherwise matching resumes at the known prefix length and every successful new comparison increases the best known match. Each bound search uses `O(log(n+1))` RMQs and at most `O(m)` successful new symbol comparisons, plus one terminating comparison per binary-search step.

A substring with at least `k` occurrences is a common prefix of `k` consecutive suffixes in `sa`, whose LCP is the minimum of the `k-1` adjacent LCP entries between them, so the best window minimum is the maximum length; the first best window is the lexicographically smallest. Non-overlapping repetition of length `L` is monotone in `L` (shorten both occurrences) and holds exactly when some group of suffixes with pairwise LCP `>= L` has start spread `>= L`. For common substrings, all suffixes with a given prefix form a contiguous group; a group containing both inputs has an adjacent ownership change. The fresh separator occurs once and prevents a matching prefix between distinct starts from crossing the first input's boundary. Thus the largest adjacent LCP across an ownership change is the common-substring maximum. Scanning in suffix order selects the smallest maximal substring; scanning its complete LCP-connected group selects earliest witness positions.

## Feature-to-test map

Runnable entry: [07-suffixarray_tester.py](<../../96-Local Testing/07-Strings/07-suffixarray_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/07-suffixarray_tester.cpp>). All test oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| SA, inverse ranks and Kasai | Direct lexicographic suffix sorting and direct character-by-character LCP; all ternary texts through lengths 5/7/8 in quick/full/stress. |
| Exact LCE and substring caps | Every pair of suffix starts including `n`; every pair of half-open substrings for lengths through 5; seeded random interval pairs at larger sizes. |
| Byte and sparse integer alphabets | Embedded NUL, full ascending/descending 256-byte alphabets, random bytes, signed and unsigned 64-bit extremes, signed char and `vector<bool>`; constructors deduced from strings, views and literals. |
| Pattern interval and acceleration | Direct suffix/pattern scans for every substring of short inputs, empty/absent/extreme patterns, random patterns, with and without RMQ; unary patterns half the text length and longer than the entire text; brace-list texts and patterns starting with `0` for `int` and `lng` (compile-time ambiguity regression). |
| Longest repeated (k), disjoint repeat | Direct enumeration of every distinct substring with its sorted occurrence starts, longest first and lexicographic within a length: `k = 2` for every case, every `k` in `1..n+1` and the disjoint variant for `n <= 24` (all exhaustive ternary texts, binary random texts); unary and period-three closed forms at 5,000/200,000/700,000 for `k` in `{1, 3, n/2, n, n+1}`, `k = 4` and the disjoint witness. |
| Longest common substring | Direct pair-of-starts LCP oracle, all binary input pairs through lengths 3/5/6, random byte/signed/unsigned inputs, empty/disjoint inputs and canonical ties; a large periodic pair. |
| Lifecycle and query configuration | Deferred and repeated `buildRmq`, default empty queries, copy, move destination, reassignment of moved-from object, assignment of a different index. |
| Preconditions | Twelve checked subprocesses require assertion `SIGABRT`: negative/past-end LCE positions, absent RMQ, invalid half-open intervals, nonbyte input (`-1` and `2^32` as `ulng`) to the byte construction helper and `longestRepeated(0)`. |
| Warnings | Every configuration compiles with `-Wall -Wextra -Wshadow -Wconversion -Werror`, including the `SuffixArray<bool>` and `signed char` instantiations. |
| Large/adversarial shape | Unary and period-three texts of length 5,000/200,000/700,000, with closed-form suffix/LCP/LCE and witness oracles. |

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode quick --seed 20261008  # PASS, 2 configurations, 592 cases and 517,992 checks each, 12 assertion probes
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261008  # PASS, 3 configurations, 5,534 cases and 3,609,024 checks each
CXX=g++-14 python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261008  # PASS, 3 configurations, same counts
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode stress --seed 20261009  # PASS, 3 configurations, 20,345 cases and 14,721,498 checks each
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261008 --no-integration  # PASS, 9 suites
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The pre-change suite passed in full mode (seed 1, three configurations) as the re-audit baseline; the integration, folder and consistency lines cover all three P017 headers.

## Benchmarks

No benchmark: radix doubling is the direct algorithm for the stated bound, with no
specialization or dispatch threshold. Sparse-table storage is optional and explicit.

## Sources

Inspected live on 2026-09-28; implementations were independently written from the algorithms and the explicit contracts above, with no source code copied.

| Source | Inspected scope and use |
|---|---|
| [cp-algorithms, “Suffix Array”](https://cp-algorithms.com/string/suffix-array.html), [source](https://raw.githubusercontent.com/cp-algorithms/cp-algorithms/master/src/string/suffix-array.md) | Complete doubling/counting-sort argument, sentinel conversion, binary-search occurrences, rank comparison, Kasai/RMQ proof and distinct-substring reduction. This header directly sorts finite suffixes and avoids the article's literal `$` sentinel restriction. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Read comparator doubling, sparse integer rank compression, byte wrapper and Kasai source/primary reference. The SA-IS body was outside this package's adopted scope. Comparator doubling is `O(n * log(n)^2)` in general; this implementation uses counting-sort passes for the owned `O(n * log(n))` contract. |
| [KACTL `SuffixArray.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/SuffixArray.h), authors 罗穗骞/chilli, dated 2019-04-11 | Read source and comments: `n+1` suffix convention, preceding-neighbor LCP, no-NUL restriction and doubling approach. Compared domain/conventions only; its source reports unknown license. This header includes full bytes, omits the empty suffix and uses succeeding-neighbor LCP. |
| [Preserved `old_lcssubstr.py`](<../../OLD/Team Notebook/src/misc/old_lcssubstr.py>) | Read all 30 lines: two-input longest common substring DP and printed witness, plus undefined trailing demo arguments. The new two-input API accounts for the length/witness behavior with explicit deterministic ties and no printing/import side effects. Its multi-document/Python archive ownership remains unchanged. |
| [OI Wiki, 后缀数组](https://oi-wiki.org/string/sa/) (read 2026-10-08) | Applications: longest substring occurring at least `k` times (window minimum over `k-1` adjacent heights) and non-overlapping repeats (binary search on the length, group spread). Adopted as `longestRepeated(k)` and `longestRepeatedDisjoint`; its RMQ per group is replaced by a linear group scan. |

No local suffix-array implementation was found in the inspected original notebook monoliths or focused archive search, and `07-Strings/97-Legacy` does not exist.

## Limits and handoffs

- `/reaudit-review` findings ([p017](<../../00-Guidelines/23-Reaudit Findings/>), deleted after this run): 2 `initializer_list<T>` constructor and `patternRange` overloads remove the brace-list ambiguity; 7 the byte-path check compares through `lll`, warning-free for `bool`; 8 closing braces normalized; 9 the struct line has `U: NA` and both `longestCommonSubstring` overloads have `T/M` lines.
- Added in this re-audit after the completeness sweep: `longestRepeated(k)` and `longestRepeatedDisjoint()` ([OI Wiki 后缀数组](https://oi-wiki.org/string/sa/), sections on substrings occurring at least `k` times and non-overlapping repeats).

SA-IS belongs to ST06/`12-sais.hpp`; generalized indexes and common-substring queries across arbitrary numbers of inputs belong to ST11/`28-multiple_string.hpp`. Distinct-substring counting and kth applications retain ST04/`10-suffixautomaton.hpp` and ST09/`22-lexicographic_queries.hpp` ownership. Suffix trees and dynamic indexing also retain their separate inventory owners. No missing Basic suffix-array feature is deferred to those packages.

Finite tests support the stated proofs and domains; they do not allocate
impractical maximum-size objects.

## History

- 2026-09-28: verified under the previous system (P017). 2026-10-08 re-audit: `/reaudit-review` findings 2 and 7–9 fixed; `longestRepeated(k)` and `longestRepeatedDisjoint` added.
