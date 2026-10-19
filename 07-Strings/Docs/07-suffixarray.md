# 07-suffixarray.hpp — evidence

`07-suffixarray.hpp` implements the Basic suffix-array row. Construction runs SA-IS (`sais` from `12-sais.hpp`, [12-sais.md](12-sais.md)) on order-preserving symbol codes, then derives inverse ranks and the LCP array (`lcpArray`, Kasai, same header); the static builder `SuffixArray<T>::doubling` keeps stable counting-sort prefix doubling as a second, independent algorithm. Exact LCE and accelerated pattern search reuse the verified Data Structures `SparseTable`; callers may omit RMQ to retain linear memory. Two-input longest common substring, longest repeated substring (at least `k` overlapping occurrences) and longest disjoint repeat include deterministic witnesses.

## Contracts

### SuffixArray, longestCommonSubstring

- `SuffixArray<T>` owns a `vector<T>` for integral `T`; the input's ordinary signed/unsigned order is preserved without narrowing symbols. A brace list (`SuffixArray<int> x({0, 1})`, `x.patternRange({0})`) selects the `initializer_list<T>` overloads, so lists starting with a literal `0` are not ambiguous with `string_view`. `SuffixArray(string_view)` and deduction from `string`/C strings use `T=int` and unsigned byte order, including NUL and bytes 128–255. A C string follows its normal terminating-NUL convention; use an explicit `string_view` length for embedded NUL.
- Input length `n < INT_MAX`. `sa` contains exactly the `n` nonempty suffix starts, `rank[sa[k]]=k`, and `lcp[k]=LCP(sa[k],sa[k+1])` has `max(0,n-1)` entries. Construction is sentinel-free; no input value is reserved. Empty construction is valid.
- Construction codes the symbols (`suffixarray_detail::alphabetCodes`): when `max - min < max(4n, 256)` (every byte text; the bound is also capped at `INT_MAX - 1`) it subtracts the minimum in `O(n)`, giving codes in `[0, max - min]`; otherwise `compressAlphabet` sorts positions in `O(n * log(n+1))` time. The user confirmed this refinement of the omission decision (which named `compressAlphabet` for generic `T`) on 2026-10-19 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). The cutoff is measured (Benchmarks): shifting stays faster up to about `8n` at `n = 10^6`, and `4n` keeps the three SA-IS bucket arrays at about `12n` ints. SA-IS then takes `O(n + upper) = O(n)` time and workspace, inverse ranks `O(n)`, and `lcpArray` (Kasai) `O(n)` time and workspace, rebuilding the inverse ranks it needs. Without RMQ, byte and dense-range construction is therefore `O(n)`.
- `SuffixArray<T>::doubling(s, with_rmq = true)` returns the same object (`text`, `sa`, `rank`, `lcp`, RMQ configuration) built by one comparison sort of the symbols, then stable radix doubling until all ranks differ: `O(n * log(n+1))` time, `O(n)` workspace. It is an independent algorithm: the oracle of `12` and `14`, and a choice for callers with many short random texts (see Benchmarks). Brace lists work through its `vector<T>` parameter; byte strings are converted by the caller (unsigned order).
- Default construction builds RMQ in `O(n * log(n+1))` time and stored space. Pass `false` as the second constructor argument for `O(n)` storage, then call `buildRmq()` if LCE becomes necessary. Repeated `buildRmq()` calls are valid. No hidden lazy mutation occurs in queries.
- `lce(i,j)` is exact and requires RMQ plus `0 <= i,j <= n`. It returns `n-i` when `i==j` and zero if exactly one argument is the empty suffix `n`. `substringLce(l,r,a,b)` accepts half-open intervals, including empty intervals, and caps the suffix answer at both interval lengths. Both queries cost `O(1)`.
- `patternRange(pattern)` returns ranks `[l,r)` in `sa`. Results enumerate occurrences as `sa[l..r)` in suffix order; an absent pattern has `l==r`. An empty pattern returns `[0,n)`: boundary occurrence `n` is omitted because it has no `sa` entry. With RMQ the query costs `O(m+log(n+1))`; without RMQ it costs `O(m * log(n+1))`, where `m` is the pattern length. The byte overload uses `O(m)` conversion storage; the integer overload uses constant query workspace and requires the same symbol type as the index; the `initializer_list` overload copies the list (`O(m)`).
- `longestRepeated(k = 2)` returns `{start,length}` of the longest substring with at least `k >= 1` (asserted) occurrences, overlaps allowed. `k = 1` gives the whole text `{0,n}`; no answer (including `k > n`) gives `{0,0}`. Equal lengths select the lexicographically smallest such substring, then its earliest start. It takes the maximum over windows of `k-1` adjacent LCP entries of the window minimum with a monotonic deque, then scans the selected LCP group: `O(n)` time, `O(min(k, n))` deque workspace, no RMQ.
- `longestRepeatedDisjoint()` returns `{first,last,length}` of the longest substring with two non-overlapping occurrences (`last - first >= length`), choosing the lexicographically smallest such substring; `first`/`last` are its earliest and latest starts. No answer gives `{0,0,0}`. Binary search on the length, each probe scanning LCP groups for the start spread: `O(n * log(n+1))` time, `O(1)` workspace, no RMQ.
- `longestCommonSubstring(a,b)` returns `{start_in_a,start_in_b,length}`. No common symbol returns `{0,0,0}`. Ties choose the lexicographically smallest common substring, then its earliest start independently in each input. Both vectors must use the same integer type; byte overloads use unsigned byte order. It requires `n+m+1 < INT_MAX`, codes the concatenation with `alphabetCodes` (shifted by the union minimum, or sorted when the union range is at least `max(4(n + m), 256)`), adds one so that a fresh separator zero is smaller than every symbol whatever the original values, and builds the joined index with the tested constructor without RMQ. Time is `O(n+m)` for bytes and dense ranges, `O((n+m) * log(n+m+1))` otherwise; storage/workspace `O(n+m)`.
- `rmq` is a `SparseTable<Min<int>>` over `lcp` (`00-monoids.hpp` preset, also used by `SubstringOrder` in `22-lexicographic_queries.hpp`). Fields are read-only after construction; replace the object to change its input. Copy and move destinations preserve their indexes. Assign a valid object to a moved-from object before querying it. The construction helpers operate on owned fields: `build(with_rmq)` (codes and SA-IS over the owned text) requires only `n < INT_MAX`; `buildIndex(with_rmq)` (inverse ranks, LCP and RMQ) requires `sa` to be the suffix array of `text`. Codes compare through `ulll` differences: for every integral `T` up to 128 bits, including `bool`, signed extremes and `__int128`, `ulll(x) - ulll(min)` is the exact difference, and the dense codes stay below `INT_MAX - 1`.

Correctness: both codings are strictly increasing maps of the symbols (a shift, or ranks among the distinct values), so suffix order, LCPs and every derived answer are unchanged; SA-IS correctness is argued in [12-sais.md](12-sais.md). For `doubling`: at doubling width `k`, each current rank describes the first `k` symbols of its suffix. Suffixes shorter than or equal to `k` have a missing second rank, smaller than every real rank. Listing those starts first, then subtracting `k` from the current suffix order where possible, sorts all starts by second rank. Stable counting sort on first ranks therefore sorts the rank pairs. Consecutive unequal pairs receive consecutive ranks, preserving equality and lexicographic order for width `2k`. Eventually the width covers every suffix; unique ranks permit early termination. Missing ranks use the internal value `-1`, never a text symbol. Width and index calculations avoid overflowing signed sums.

`lcpArray` (Kasai) scans starts in text order. Removing a common leading symbol lowers the known adjacent-suffix LCP by at most one; the lexicographic neighbors of the shortened suffix share at least that remaining prefix. Consequently the carried lower bound is valid and the total extensions are linear. The LCP of any two suffixes equals the minimum adjacent LCP between their ranks: all suffixes sharing a prefix form a contiguous lexicographic interval. The sparse-table minimum therefore answers LCE exactly.

Pattern binary search retains an anchor suffix having the largest prefix match obtained so far. If the next suffix diverges from the anchor before that matched prefix ends, suffix-rank order determines its comparison with the pattern. Otherwise matching resumes at the known prefix length and every successful new comparison increases the best known match. Each bound search uses `O(log(n+1))` RMQs and at most `O(m)` successful new symbol comparisons, plus one terminating comparison per binary-search step.

A substring with at least `k` occurrences is a common prefix of `k` consecutive suffixes in `sa`, whose LCP is the minimum of the `k-1` adjacent LCP entries between them, so the best window minimum is the maximum length; the first best window is the lexicographically smallest. Non-overlapping repetition of length `L` is monotone in `L` (shorten both occurrences) and holds exactly when some group of suffixes with pairwise LCP `>= L` has start spread `>= L`. For common substrings, all suffixes with a given prefix form a contiguous group; a group containing both inputs has an adjacent ownership change. The fresh separator occurs once and prevents a matching prefix between distinct starts from crossing the first input's boundary. Thus the largest adjacent LCP across an ownership change is the common-substring maximum. Scanning in suffix order selects the smallest maximal substring; scanning its complete LCP-connected group selects earliest witness positions.

### Legacy mapping

| Removed | Replacement |
|---|---|
| Kasai loop inside `SuffixArray::build` (second implementation; prune, 2026-10-13) | `lcpArray(text, sa)` from `12-sais.hpp`; same `lcp` values |
| Prefix doubling as the default construction (2026-10-19, omission sweep) | `SuffixArray<T>::doubling(s, with_rmq)`; the constructors use SA-IS |
| `build(bytes, with_rmq)` byte flag and its `[0, 256)` precondition (2026-10-19) | `build(with_rmq)`: bytes always take the dense shift |

## Feature-to-test map

Runnable entry: [07-suffixarray_tester.py](<../../96-Local Testing/07-Strings/07-suffixarray_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/07-suffixarray_tester.cpp>).

| Feature/domain | Independent verification |
|---|---|
| SA, inverse ranks and LCP (`lcpArray`), SA-IS constructors, direct `build` and `doubling` | Direct lexicographic suffix sorting and direct character-by-character LCP and LCE for both builders, with and without RMQ, and `build(false)` on a text assigned to a default object; all ternary texts through lengths 5/7/8 in quick/full/stress, every random case, brace-list `doubling`. |
| Exact LCE and substring caps | Every pair of suffix starts including `n`; every pair of half-open substrings for lengths through 5; seeded random interval pairs at larger sizes. |
| Byte, dense and sparse integer alphabets | Embedded NUL, full ascending/descending 256-byte alphabets, random bytes, signed and unsigned 64-bit extremes, `__int128` extremes, signed char and `vector<bool>`; both sides of the code boundary `max(4n, 256)`, each checked through the coding's `upper` as well as the full oracle (`lng` range 255 and 256 at `n = 5`, `ulng` at the top, `int` range 1199 and 1200 at `n = 300` with a permutation text and a single spike, `__int128` near its minimum and across its full range, empty input); a cutoff off by one fails the quick suite; constructors deduced from strings, views and literals. |
| Pattern interval and acceleration | Direct suffix/pattern scans for every substring of short inputs, empty/absent/extreme patterns, random patterns, with and without RMQ; unary patterns half the text length and longer than the entire text; brace-list texts and patterns starting with `0` for `int` and `lng` (compile-time ambiguity regression). |
| Longest repeated (k), disjoint repeat | Direct enumeration of every distinct substring with its sorted occurrence starts, longest first and lexicographic within a length: `k = 2` for every case, every `k` in `1..n+1` and the disjoint variant for `n <= 24` (all exhaustive ternary texts, binary random texts); unary and period-three closed forms at 5,000/200,000/700,000 for `k` in `{1, 3, n/2, n, n+1}`, `k = 4` and the disjoint witness. |
| Longest common substring | Direct pair-of-starts LCP oracle, all binary input pairs through lengths 3/5/6, random byte/signed/unsigned inputs, empty/disjoint inputs and canonical ties; a large periodic pair. |
| Lifecycle, `text`, `size` and query configuration | `text` equal to the input and `size()` to its length; deferred and repeated `buildRmq`, default empty queries, copy, move destination, reassignment of moved-from object, assignment of a different index. |
| Preconditions | Ten checked subprocesses require assertion `SIGABRT`: negative/past-end LCE positions, absent RMQ, invalid half-open intervals and `longestRepeated(0)`. |
| Warnings | Every configuration compiles with `-Wall -Wextra -Wshadow -Wconversion -Werror`, including the `SuffixArray<bool>` and `signed char` instantiations. |
| Large/adversarial shape | Unary and period-three texts of length 5,000/200,000/700,000, with closed-form suffix/LCP/LCE and witness oracles; `doubling` on both, and the period-three text over sparse `lng` symbols (sorted coding) against the same order. Dependent suites `10`, `11`, `12`, `14`, `15`, `22`, `29` rerun on the new construction. |

## Commands and results

Latest run 2026-10-19 (P017 omission-sweep apply), Linux x86-64 (Intel Core i9-11900H), GNU++20, GCC 16.2.1 and the floor GCC 14.4.1 (`CXX=g++-14`), CPython 3.14.7, the optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan configurations built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>):

| Command | Result |
|---|---|
| `07-suffixarray_tester.py --mode full --seed 20261019` (g++) | PASS, 3 configurations, 5,541 cases and 5,932,460 checks per configuration, 10 assertion probes, MEMORY peak 799 MB |
| `CXX=g++-14 07-suffixarray_tester.py --mode full --seed 20261019` (floor) | PASS, same counts, MEMORY peak 820 MB |
| `07-suffixarray_tester.py --mode stress --seed 7` | PASS, 20,352 cases and 23,917,558 checks per configuration, MEMORY peak 1179 MB |
| Dependents on the final code: `11`, `29` full (g++ and `CXX=g++-14`), `10`, `12`, `14`, `15`, `22` quick | PASS: 30,379,721 / 1,267,465 checks per configuration; MEMORY peaks 801 / 673 MB (813 / 698 MB on g++-14) |
| Dependents full before the final code-range cutoff (`max(n, 256)`, same API and outputs): `10`, `12`, `14`, `15`, `22` on g++; `11`, `12`, `14` on `CXX=g++-14` | PASS: 5,860,174 / 10,115,167 / 7,698,991 / 1,217,302 / 17,230,277 checks per configuration (MEMORY peaks 844 / 814 / 872 / 588 / 743 MB); floor same counts (813 / 849 / 767 MB) |
| `07-suffixarray_benchmark.py --reps 5` | PASS, every checksum equal, MEMORY peak 678 MB (tables below) |
| `02-integration.py --sanitizers` (final code) | PASS: 166 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1835 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261019
CXX=g++-14 python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20261019
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode stress --seed 7
python3 '96-Local Testing/07-Strings/07-suffixarray_benchmark.py' --reps 5
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

Driver: [07-suffixarray_benchmark.py](<../../96-Local Testing/07-Strings/07-suffixarray_benchmark.py>) with [07-suffixarray_benchmark.cpp](<../../96-Local Testing/07-Strings/07-suffixarray_benchmark.cpp>), `python3 '96-Local Testing/07-Strings/07-suffixarray_benchmark.py' --reps 5`, run 2026-10-19 on the final code (seed 20261019, `-std=gnu++20 -O2 -DNDEBUG`, GCC 16.2.1, Intel Core i9-11900H, shared host with load average 2.1–6.4, not pinned, one warmup plus five rotating repetitions, medians in ms, every method's checksum equal; MEMORY peak=678MB). Construction methods build `sa`, `rank` and `lcp` without RMQ: `vector` is `SuffixArray<int>(s, false)`, `bytes` is `SuffixArray<int>(string_view, false)`, `doubling` is `SuffixArray<int>::doubling(s, false)`. Chunk workloads are 2,000,000 symbols split into independent texts of the given length; letters are `'a' + x`.

| Workload | `vector` | `bytes` | `doubling` |
|---|---:|---:|---:|
| chunks n=8 S=4 | 78.4 | 80.4 | 107.4 |
| chunks n=16 S=4 | 89.7 | 92.6 | 99.9 |
| chunks n=32 S=4 | 135.4 | 136.4 | 102.3 |
| chunks n=64 S=4 | 128.6 | 136.4 | 95.7 |
| chunks n=128 S=4 | 136.8 | 137.0 | 88.7 |
| chunks n=256 S=4 | 125.5 | 125.7 | 90.4 |
| chunks n=1024 S=4 | 119.3 | 115.8 | 94.8 |
| chunks n=8 S=26 | 67.0 | 68.0 | 79.0 |
| chunks n=16 S=26 | 66.7 | 69.4 | 75.5 |
| chunks n=32 S=26 | 125.2 | 126.7 | 95.3 |
| chunks n=64 S=26 | 108.8 | 110.4 | 100.5 |
| chunks n=128 S=26 | 97.4 | 98.6 | 99.2 |
| chunks n=256 S=26 | 91.4 | 90.3 | 95.5 |
| chunks n=1024 S=26 | 105.5 | 106.6 | 99.6 |
| unary chunks n=16 | 37.9 | 37.9 | 84.5 |
| unary chunks n=64 | 37.7 | 37.9 | 101.6 |
| unary chunks n=256 | 33.9 | 33.3 | 111.3 |
| random n=1000000 S=2 | 86.4 | 84.2 | 112.0 |
| random n=1000000 S=26 | 83.4 | 85.5 | 119.8 |
| sparse n=1000000 S=10^9 | 179.0 | — | 136.2 |
| fibonacci n=1000000 | 45.2 | 45.1 | 214.9 |
| unary n=1000000 | 24.5 | 24.3 | 143.7 |

At 10^6 symbols SA-IS is 1.3–5.9 times faster than doubling on every dense text. Doubling is 24% faster on random sparse values, where almost every symbol is distinct, so doubling stops after the comparison sort both builders share. On short random texts of 32–1024 symbols, doubling is faster by up to 35%, because SA-IS pays per-text allocation and recursion setup. On unary texts of 16–256 symbols, doubling is 2.2–3.3 times slower. Length alone does not separate the cases, so a size dispatch would trade a bounded gain on random short texts for a 2–3 times loss on repetitive ones. The constructors keep SA-IS, with its linear worst case; callers with many short random texts can call `doubling`.

Code-range cutoff: the same run times SA-IS over shifted codes (`shift`, buckets of `max - min + 1`) against SA-IS through `compressAlphabet` (`compress`). The texts are random values in `[0, range)` totalling 2,000,000 symbols; texts below 32 symbols use the naive sort, so only the coding differs there.

| n | range | `shift` | `compress` |
|---|---|---:|---:|
| 16 | 64 / 128 / 256 | 37.6 / 35.8 / 36.8 | 54.6 / 53.1 / 54.7 |
| 64 | 64 / 128 / 256 | 84.7 / 86.6 / 94.5 | 132.4 / 128.9 / 129.9 |
| 10^4 | n/2 / n / 2n / 4n / 8n / 16n | 68.4 / 69.8 / 72.6 / 78.6 / 92.2 / 137.7 | 170.8 / 169.9 / 171.7 / 172.4 / 171.8 / 171.8 |
| 10^6 | n/2 / n / 2n / 4n / 8n / 16n | 107.0 / 118.6 / 140.6 / 188.7 / 237.3 / 399.1 | 298.6 / 296.0 / 299.4 / 303.6 / 305.8 / 303.8 |

Shifting wins through `8n` and loses at `16n` for `n = 10^6`. Its cost is the three SA-IS bucket arrays, about `12 * (max - min)` bytes. The cutoff `max - min < max(4n, 256)` keeps a 1.4–2.8 times gain over sorting everywhere below it, while bounding those arrays at about `48n` bytes, comparable to the rest of an RMQ-free build.

## Sources

Inspected live on 2026-09-28; implementations were independently written from the algorithms and the explicit contracts above, with no source code copied.

| Source | Inspected scope and use |
|---|---|
| [cp-algorithms, “Suffix Array”](https://cp-algorithms.com/string/suffix-array.html), [source](https://raw.githubusercontent.com/cp-algorithms/cp-algorithms/master/src/string/suffix-array.md) | Complete doubling/counting-sort argument, sentinel conversion, binary-search occurrences, rank comparison, Kasai/RMQ proof and distinct-substring reduction. This header directly sorts finite suffixes and avoids the article's literal `$` sentinel restriction. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Read comparator doubling, sparse integer rank compression, byte wrapper and Kasai source/primary reference. Its SA-IS is the reference of `12`, which this header calls. Comparator doubling is `O(n * log(n)^2)` in general; this implementation uses counting-sort passes for the owned `O(n * log(n))` contract. |
| [KACTL `SuffixArray.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/SuffixArray.h), authors 罗穗骞/chilli, dated 2019-04-11 | Read source and comments: `n+1` suffix convention, preceding-neighbor LCP, no-NUL restriction and doubling approach. Compared domain/conventions only; its source reports unknown license. This header includes full bytes, omits the empty suffix and uses succeeding-neighbor LCP. |
| [Preserved `old_lcssubstr.py`](<../../OLD/Team Notebook/src/misc/old_lcssubstr.py>) | Read all 30 lines: two-input longest common substring DP and printed witness, plus undefined trailing demo arguments. The new two-input API accounts for the length/witness behavior with explicit deterministic ties and no printing/import side effects. Its multi-document/Python archive ownership remains unchanged. |
| [ei1333 `longest-common-substring.hpp`](https://ei1333.github.io/library/string/longest-common-substring.hpp) (read 2026-10-19) | Two-input common substring over a joined suffix array, `O(n)` uncompressed and `O(n * log(n))` compressed; matches the restated bounds of `longestCommonSubstring`. |
| [OI Wiki, 后缀数组](https://oi-wiki.org/string/sa/) (read 2026-10-08) | Applications: longest substring occurring at least `k` times (window minimum over `k-1` adjacent heights) and non-overlapping repeats (binary search on the length, group spread). Adopted as `longestRepeated(k)` and `longestRepeatedDisjoint`; its RMQ per group is replaced by a linear group scan. |

No local suffix-array implementation was found in the inspected original notebook monoliths or focused archive search, and `07-Strings/97-Legacy` does not exist.

## Limits and handoffs

SA-IS itself belongs to ST06/`12-sais.hpp`; DC3 and a small-input dispatch are not adopted ([00-notes.md](00-notes.md)); generalized indexes and common-substring queries across arbitrary numbers of inputs belong to ST11/`28-multiple_string.hpp`. Distinct-substring counting and kth applications retain ST04/`10-suffixautomaton.hpp` and ST09/`22-lexicographic_queries.hpp` ownership. Suffix trees and dynamic indexing also retain their separate inventory owners. No missing Basic suffix-array feature is deferred to those packages.

## History

- 2026-09-28: verified under the previous system (P017).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, `longestRepeated(k)` and `longestRepeatedDisjoint` added, 4 findings fixed; quick, full and stress passed on g++, full on g++-14, integration with sanitizers passed.
- 2026-10-13: prune, the inline Kasai loop replaced by `lcpArray` from `12-sais.hpp` (construction 4–7% slower at `n = 10^6`, accepted); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (the `Min<int>` preset replaces the nested `Minimum`; `rmq.prod` calls; no API change); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 694/686/895 MB.
- 2026-10-19: P017 omission-sweep apply: SA-IS default construction with dense-range coding, `doubling` static builder, `longestCommonSubstring` through the same coding (`O(n + m)` for bytes); construction benchmark added, no size dispatch; full passed on g++ and g++-14, stress passed, dependents rerun; independent review: 1 evidence defect and 6 low findings fixed (code-range cutoff measured and set to `4n`, confirmed by the user).
