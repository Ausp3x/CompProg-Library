# 07-suffixarray.hpp — evidence

`07-suffixarray.hpp` implements the Basic suffix-array row. Construction uses stable counting-sort doubling and inverse ranks; the LCP array comes from `lcpArray` (Kasai) in `12-sais.hpp` ([12-sais.md](12-sais.md)), which this header includes. Exact LCE and accelerated pattern search reuse the verified Data Structures `SparseTable`; callers may omit RMQ to retain linear memory. Two-input longest common substring, longest repeated substring (at least `k` overlapping occurrences) and longest disjoint repeat include deterministic witnesses.

## Contracts

### SuffixArray, longestCommonSubstring

- `SuffixArray<T>` owns a `vector<T>` for integral `T`; the input's ordinary signed/unsigned order is preserved without narrowing symbols. A brace list (`SuffixArray<int> x({0, 1})`, `x.patternRange({0})`) selects the `initializer_list<T>` overloads, so lists starting with a literal `0` are not ambiguous with `string_view`. `SuffixArray(string_view)` and deduction from `string`/C strings use `T=int` and unsigned byte order, including NUL and bytes 128–255. A C string follows its normal terminating-NUL convention; use an explicit `string_view` length for embedded NUL.
- Input length `n < INT_MAX`. `sa` contains exactly the `n` nonempty suffix starts, `rank[sa[k]]=k`, and `lcp[k]=LCP(sa[k],sa[k+1])` has `max(0,n-1)` entries. Construction is sentinel-free; no input value is reserved. Empty construction is valid.
- Byte initialization costs `O(n+256)`; sparse integer initialization sorts positions to compress symbols in `O(n * log(n+1))` time and `O(n)` workspace. Stable radix doubling takes `O(n * log(n+1))` time and `O(n)` workspace, stopping once all ranks differ. `lcpArray` (Kasai) takes `O(n)` time and `O(n)` workspace, rebuilding the inverse ranks it needs.
- Default construction builds RMQ in `O(n * log(n+1))` time and stored space. Pass `false` as the second constructor argument for `O(n)` storage, then call `buildRmq()` if LCE becomes necessary. Repeated `buildRmq()` calls are valid. No hidden lazy mutation occurs in queries.
- `lce(i,j)` is exact and requires RMQ plus `0 <= i,j <= n`. It returns `n-i` when `i==j` and zero if exactly one argument is the empty suffix `n`. `substringLce(l,r,a,b)` accepts half-open intervals, including empty intervals, and caps the suffix answer at both interval lengths. Both queries cost `O(1)`.
- `patternRange(pattern)` returns ranks `[l,r)` in `sa`. Results enumerate occurrences as `sa[l..r)` in suffix order; an absent pattern has `l==r`. An empty pattern returns `[0,n)`: boundary occurrence `n` is omitted because it has no `sa` entry. With RMQ the query costs `O(m+log(n+1))`; without RMQ it costs `O(m * log(n+1))`, where `m` is the pattern length. The byte overload uses `O(m)` conversion storage; the integer overload uses constant query workspace and requires the same symbol type as the index; the `initializer_list` overload copies the list (`O(m)`).
- `longestRepeated(k = 2)` returns `{start,length}` of the longest substring with at least `k >= 1` (asserted) occurrences, overlaps allowed. `k = 1` gives the whole text `{0,n}`; no answer (including `k > n`) gives `{0,0}`. Equal lengths select the lexicographically smallest such substring, then its earliest start. It takes the maximum over windows of `k-1` adjacent LCP entries of the window minimum with a monotonic deque, then scans the selected LCP group: `O(n)` time, `O(min(k, n))` deque workspace, no RMQ.
- `longestRepeatedDisjoint()` returns `{first,last,length}` of the longest substring with two non-overlapping occurrences (`last - first >= length`), choosing the lexicographically smallest such substring; `first`/`last` are its earliest and latest starts. No answer gives `{0,0,0}`. Binary search on the length, each probe scanning LCP groups for the start spread: `O(n * log(n+1))` time, `O(1)` workspace, no RMQ.
- `longestCommonSubstring(a,b)` returns `{start_in_a,start_in_b,length}`. No common symbol returns `{0,0,0}`. Ties choose the lexicographically smallest common substring, then its earliest start independently in each input. Both vectors must use the same integer type; byte overloads use unsigned byte order. It requires `n+m+1 < INT_MAX`, compresses the union alphabet to positive ranks and uses a fresh separator zero, independent of all original symbol values. Time is `O((n+m) * log(n+m+1))`, storage/workspace `O(n+m)`; this reduction omits RMQ. Rank initialization of the joined sequence uses the same tested constructor.
- `rmq` is a `SparseTable<Min<int>>` over `lcp` (`00-monoids.hpp` preset; the nested `Minimum` functor was replaced on 2026-10-17, and `SubstringOrder` in `22-lexicographic_queries.hpp` uses the same preset). Fields are read-only after construction; replace the object to change its input. Copy and move destinations preserve their indexes. Assign a valid object to a moved-from object before querying it. The exposed `build(bytes,with_rmq)` construction helper operates on the owned text; its byte path requires all values in `[0,256)`, checked through a 128-bit comparison so every integral `T` (including `bool` and wide unsigned values) is exact and warning-free.

Correctness: at doubling width `k`, each current rank describes the first `k` symbols of its suffix. Suffixes shorter than or equal to `k` have a missing second rank, smaller than every real rank. Listing those starts first, then subtracting `k` from the current suffix order where possible, sorts all starts by second rank. Stable counting sort on first ranks therefore sorts the rank pairs. Consecutive unequal pairs receive consecutive ranks, preserving equality and lexicographic order for width `2k`. Eventually the width covers every suffix; unique ranks permit early termination. Missing ranks use the internal value `-1`, never a text symbol. Width and index calculations avoid overflowing signed sums.

`lcpArray` (Kasai) scans starts in text order. Removing a common leading symbol lowers the known adjacent-suffix LCP by at most one; the lexicographic neighbors of the shortened suffix share at least that remaining prefix. Consequently the carried lower bound is valid and the total extensions are linear. The LCP of any two suffixes equals the minimum adjacent LCP between their ranks: all suffixes sharing a prefix form a contiguous lexicographic interval. The sparse-table minimum therefore answers LCE exactly.

Pattern binary search retains an anchor suffix having the largest prefix match obtained so far. If the next suffix diverges from the anchor before that matched prefix ends, suffix-rank order determines its comparison with the pattern. Otherwise matching resumes at the known prefix length and every successful new comparison increases the best known match. Each bound search uses `O(log(n+1))` RMQs and at most `O(m)` successful new symbol comparisons, plus one terminating comparison per binary-search step.

A substring with at least `k` occurrences is a common prefix of `k` consecutive suffixes in `sa`, whose LCP is the minimum of the `k-1` adjacent LCP entries between them, so the best window minimum is the maximum length; the first best window is the lexicographically smallest. Non-overlapping repetition of length `L` is monotone in `L` (shorten both occurrences) and holds exactly when some group of suffixes with pairwise LCP `>= L` has start spread `>= L`. For common substrings, all suffixes with a given prefix form a contiguous group; a group containing both inputs has an adjacent ownership change. The fresh separator occurs once and prevents a matching prefix between distinct starts from crossing the first input's boundary. Thus the largest adjacent LCP across an ownership change is the common-substring maximum. Scanning in suffix order selects the smallest maximal substring; scanning its complete LCP-connected group selects earliest witness positions.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| Kasai loop inside `SuffixArray::build` (second implementation) | `lcpArray(text, sa)` from `12-sais.hpp`; same `lcp` values |

## Feature-to-test map

Runnable entry: [07-suffixarray_tester.py](<../../96-Local Testing/07-Strings/07-suffixarray_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/07-suffixarray_tester.cpp>). All test oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| SA, inverse ranks and LCP (`lcpArray`) | Direct lexicographic suffix sorting and direct character-by-character LCP; all ternary texts through lengths 5/7/8 in quick/full/stress. |
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

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `07-suffixarray_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 3,744,494 checks per configuration, 12 assertion probes, MEMORY peak 694 MB |
| `CXX=g++-14 07-suffixarray_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 3,744,494 checks per configuration, MEMORY peak 686 MB |
| `07-suffixarray_tester.py --mode stress --seed 7` | PASS, 14,701,731 checks per configuration, MEMORY peak 895 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

No benchmark driver: radix doubling is the direct algorithm for the stated bound, with no
specialization or dispatch threshold. Sparse-table storage is optional and explicit.
Construction timings against SA-IS are in [12-sais.md](12-sais.md#benchmarks) (`doubling`).

Prune measurement, 2026-10-13 (one-off, against the deleted inline Kasai loop): `SuffixArray<int>(s, false)` on `n = 10^6`, GCC 16.2.1 `-std=gnu++20 -O2 -DNDEBUG`, Intel Core i9-11900H pinned to one core (`taskset -c 5`, shared host, load average 2–4), seed 20261013, memory-capped. Each run takes the median of seven constructions after one warmup; the table gives the median of six runs per build, alternating builds; LCP sums are equal.

| Text (`n = 10^6`) | Inline Kasai ms | `lcpArray` ms | Change |
|---|---:|---:|---:|
| random, 2 letters | 141.5 | 147.1 | +4% |
| random, 26 letters | 149.5 | 158.7 | +6% |
| Fibonacci prefix | 287.1 | 300.4 | +5% |
| unary | 174.9 | 186.7 | +7% |

`lcpArray` rebuilds the inverse ranks `build` already holds (peak 65 MB against 62 MB). The user confirmed the removal with this cost on 2026-10-13; queries are unaffected.

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

- Added in this re-audit after the completeness sweep: `longestRepeated(k)` and `longestRepeatedDisjoint()` ([OI Wiki 后缀数组](https://oi-wiki.org/string/sa/), sections on substrings occurring at least `k` times and non-overlapping repeats).

SA-IS belongs to ST06/`12-sais.hpp`; generalized indexes and common-substring queries across arbitrary numbers of inputs belong to ST11/`28-multiple_string.hpp`. Distinct-substring counting and kth applications retain ST04/`10-suffixautomaton.hpp` and ST09/`22-lexicographic_queries.hpp` ownership. Suffix trees and dynamic indexing also retain their separate inventory owners. No missing Basic suffix-array feature is deferred to those packages.

Finite tests support the stated proofs and domains; they do not allocate
impractical maximum-size objects.

## History

- 2026-09-28: verified under the previous system (P017).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, `longestRepeated(k)` and `longestRepeatedDisjoint` added, 4 findings fixed; quick, full and stress passed on g++, full on g++-14, integration with sanitizers passed.
- 2026-10-13: prune, the inline Kasai loop replaced by `lcpArray` from `12-sais.hpp` (construction 4–7% slower at `n = 10^6`, accepted); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (the `Min<int>` preset replaces the nested `Minimum`; `rmq.prod` calls; no API change); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 694/686/895 MB.
