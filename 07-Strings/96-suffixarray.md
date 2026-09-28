# Suffix arrays — ST03 / P017

`07-suffixarray.hpp` implements the Basic suffix-array row. Construction uses stable counting-sort doubling, inverse ranks and Kasai LCP. Exact LCE and accelerated pattern search reuse the verified Data Structures `SparseTable`; callers may omit RMQ to retain linear memory. Two-input longest common substring and longest repeated substring include deterministic witnesses.

## Contracts and costs

- `SuffixArray<T>` owns a `vector<T>` for integral `T`; the input's ordinary signed/unsigned order is preserved without narrowing symbols. `SuffixArray(string_view)` and deduction from `string`/C strings use `T=int` and unsigned byte order, including NUL and bytes 128–255. A C string follows its normal terminating-NUL convention; use an explicit `string_view` length for embedded NUL.
- Input length `n < INT_MAX`. `sa` contains exactly the `n` nonempty suffix starts, `rank[sa[k]]=k`, and `lcp[k]=LCP(sa[k],sa[k+1])` has `max(0,n-1)` entries. Construction is sentinel-free; no input value is reserved. Empty construction is valid.
- Byte initialization costs `O(n+256)`; sparse integer initialization sorts positions to compress symbols in `O(n * log(n+1))` time and `O(n)` workspace. Stable radix doubling takes `O(n * log(n+1))` time and `O(n)` workspace, stopping once all ranks differ. Kasai takes `O(n)` time.
- Default construction builds RMQ in `O(n * log(n+1))` time and stored space. Pass `false` as the second constructor argument for `O(n)` storage, then call `buildRmq()` if LCE becomes necessary. Repeated `buildRmq()` calls are valid. No hidden lazy mutation occurs in queries.
- `lce(i,j)` is exact and requires RMQ plus `0 <= i,j <= n`. It returns `n-i` when `i==j` and zero if exactly one argument is the empty suffix `n`. `substringLce(l,r,a,b)` accepts half-open intervals, including empty intervals, and caps the suffix answer at both interval lengths. Both queries cost `O(1)`.
- `patternRange(pattern)` returns ranks `[l,r)` in `sa`. Results enumerate occurrences as `sa[l..r)` in suffix order; an absent pattern has `l==r`. An empty pattern returns `[0,n)`: boundary occurrence `n` is omitted because it has no `sa` entry. With RMQ the query costs `O(m+log(n+1))`; without RMQ it costs `O(m * log(n+1))`, where `m` is the pattern length. The byte overload uses `O(m)` conversion storage; the integer overload uses constant query workspace and requires the same symbol type as the index.
- `longestRepeated()` returns `{start,length}` and permits overlapping occurrences. No nonempty repeat returns `{0,0}`. Equal lengths select the lexicographically smallest repeated substring, then its earliest start. It scans the LCP array and selected group in `O(n)` time, without requiring RMQ.
- `longestCommonSubstring(a,b)` returns `{start_in_a,start_in_b,length}`. No common symbol returns `{0,0,0}`. Ties choose the lexicographically smallest common substring, then its earliest start independently in each input. Both vectors must use the same integer type; byte overloads use unsigned byte order. It requires `n+m+1 < INT_MAX`, compresses the union alphabet to positive ranks and uses a fresh separator zero, independent of all original symbol values. Time is `O((n+m) * log(n+m+1))`, storage/workspace `O(n+m)`; this reduction omits RMQ. Rank initialization of the joined sequence uses the same tested constructor.
- Fields are read-only after construction; replace the object to change its input. Copy and move destinations preserve their indexes. Assign a valid object to a moved-from object before querying it. The exposed `build(bytes,with_rmq)` construction helper operates on the owned text; its byte path requires all values in `[0,256)`.

SA-IS belongs to ST06/`12-sais.hpp`; generalized indexes and common-substring queries across arbitrary numbers of inputs belong to ST11/`28-multiple_string.hpp`. Distinct-substring counting and kth/frequency applications retain ST04/`10-suffixautomaton.hpp` and ST09/`22-lexicographic_queries.hpp` ownership. Suffix trees and dynamic indexing also retain their separate inventory owners. No missing Basic suffix-array feature is deferred to those packages.

## Correctness arguments

At doubling width `k`, each current rank describes the first `k` symbols of its suffix. Suffixes shorter than or equal to `k` have a missing second rank, smaller than every real rank. Listing those starts first, then subtracting `k` from the current suffix order where possible, sorts all starts by second rank. Stable counting sort on first ranks therefore sorts the rank pairs. Consecutive unequal pairs receive consecutive ranks, preserving equality and lexicographic order for width `2k`. Eventually the width covers every suffix; unique ranks permit early termination. Missing ranks use the internal value `-1`, never a text symbol. Width and index calculations avoid overflowing signed sums.

Kasai scans starts in text order. Removing a common leading symbol lowers the known adjacent-suffix LCP by at most one; the lexicographic neighbors of the shortened suffix share at least that remaining prefix. Consequently the carried lower bound is valid and the total extensions are linear. The LCP of any two suffixes equals the minimum adjacent LCP between their ranks: all suffixes sharing a prefix form a contiguous lexicographic interval. The sparse-table minimum therefore answers LCE exactly.

Pattern binary search retains an anchor suffix having the largest prefix match obtained so far. If the next suffix diverges from the anchor before that matched prefix ends, suffix-rank order determines its comparison with the pattern. Otherwise matching resumes at the known prefix length and every successful new comparison increases the best known match. Each bound search uses `O(log(n+1))` RMQs and at most `O(m)` successful new symbol comparisons, plus one terminating comparison per binary-search step.

A repeated substring appears as a prefix of two adjacent suffixes, so the largest adjacent LCP is its maximum length. For common substrings, all suffixes with a given prefix form a contiguous group; a group containing both inputs has an adjacent ownership change. The fresh separator occurs once and prevents a matching prefix between distinct starts from crossing the first input's boundary. Thus the largest adjacent LCP across an ownership change is the common-substring maximum. Scanning in suffix order selects the smallest maximal substring; scanning its complete LCP-connected group selects earliest witness positions.

## Inspected sources and legacy accounting

Inspected live on 2026-09-28; implementations were independently written from the algorithms and the explicit contracts above, with no source code copied.

| Source | Inspected scope and use |
|---|---|
| [cp-algorithms, “Suffix Array”](https://cp-algorithms.com/string/suffix-array.html), [source](https://raw.githubusercontent.com/cp-algorithms/cp-algorithms/master/src/string/suffix-array.md) | Complete doubling/counting-sort argument, sentinel conversion, binary-search occurrences, rank comparison, Kasai/RMQ proof and distinct-substring reduction. This header directly sorts finite suffixes and avoids the article's literal `$` sentinel restriction. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Read comparator doubling, sparse integer rank compression, byte wrapper and Kasai source/primary reference. The SA-IS body was outside this package's adopted scope. Comparator doubling is `O(n * log(n)^2)` in general; this implementation uses counting-sort passes for the owned `O(n * log(n))` contract. |
| [KACTL `SuffixArray.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/SuffixArray.h), authors 罗穗骞/chilli, dated 2019-04-11 | Read source and comments: `n+1` suffix convention, preceding-neighbor LCP, no-NUL restriction and doubling approach. Compared domain/conventions only; its source reports unknown license. This header includes full bytes, omits the empty suffix and uses succeeding-neighbor LCP. |
| [Preserved `old_lcssubstr.py`](../OLD/Team%20Notebook/src/misc/old_lcssubstr.py) | Read all 30 lines: two-input longest common substring DP and printed witness, plus undefined trailing demo arguments. The new two-input API accounts for the length/witness behavior with explicit deterministic ties and no printing/import side effects. Its multi-document/Python archive ownership remains unchanged. |

No local suffix-array implementation was found in the inspected original notebook monoliths or focused archive search, and `07-Strings/97-Legacy` does not exist. No legacy bytes were changed. No online submission or acceptance is claimed. Source hashes from this inspection are `a6bdc7d560e1c41cfe74351e78b2cd73564eef2e0fae784c6a64ecdfc730b034` (cp-algorithms), `e7ffe101a268208d63c17374e970f781a1a828feb6cc6010ead78c2a709cda86` (ACL), and `b265cb80ce7de98d6dca3c549f6e1b567d75dd32f7e66b29c143087c60878fe9` (KACTL).

## Feature-to-test map

Runnable entry: [07-suffixarray_tester.py](../96-Local%20Testing/07-Strings/07-suffixarray_tester.py), with the actual-header [C++ suite](../96-Local%20Testing/07-Strings/07-suffixarray_tester.cpp). All test oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| SA, inverse ranks and Kasai | Direct lexicographic suffix sorting and direct character-by-character LCP; all ternary texts through lengths 5/7/8 in quick/full/stress. |
| Exact LCE and substring caps | Every pair of suffix starts including `n`; every pair of half-open substrings for lengths through 5; seeded random interval pairs at larger sizes. |
| Byte and sparse integer alphabets | Embedded NUL, full ascending/descending 256-byte alphabets, random bytes, signed and unsigned 64-bit extremes, signed char and `vector<bool>`; constructors deduced from strings, views and literals. |
| Pattern interval and acceleration | Direct suffix/pattern scans for every substring of short inputs, empty/absent/extreme patterns, random patterns, with and without RMQ; unary patterns half the text length and longer than the entire text. |
| Longest repeated substring | Direct all-start-pairs LCP maximum with independent lexical/earliest tie selection; overlap, no repeat, unary and periodic cases. |
| Longest common substring | Direct pair-of-starts LCP oracle, all binary input pairs through lengths 3/5/6, random byte/signed/unsigned inputs, empty/disjoint inputs and canonical ties; a large periodic pair. |
| Lifecycle and query configuration | Deferred and repeated `buildRmq`, default empty queries, copy, move destination, reassignment of moved-from object, assignment of a different index. |
| Preconditions | Ten checked subprocesses require assertion `SIGABRT`: negative/past-end LCE positions, absent RMQ, invalid half-open intervals and nonbyte input to the byte construction helper. |
| Large/adversarial shape | Unary and period-three texts of length 5,000/200,000/700,000, with closed-form suffix/LCP/LCE and witness oracles. |

## Verification record

Environment: GNU g++ 16.2.1 20260810, GNU++20, Python 3.14.7, Linux x86-64. Seeds are explicit; binaries use temporary directories. Quick/full/stress use optimized `-O2 -DNDEBUG` and checked `-O0 -g -D_GLIBCXX_DEBUG`; full/stress also use `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`, leak checking enabled.

Commands used:

```text
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode quick --seed 20260928
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20260928
python3 '96-Local Testing/07-Strings/07-suffixarray_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

Quick passed optimized and checked: 592 indexed cases and 539,166 checks per configuration, plus ten asserted-precondition probes. Full passed optimized, checked and ASan/UBSan: 5,534 indexed cases and 3,915,321 checks per configuration, plus ten asserted-precondition probes in the checked build. The first full sanitizer execution encountered LeakSanitizer's explicit `ptrace` environment restriction; the approved run outside the sandbox passed with leak checking still enabled. Logs were `/tmp/p017-suffixarray-full.log` and `/tmp/p017-suffixarray-asan.log` during the session.

Package integration passed 102 standalone/aggregate compile checks and scalar/available AVX2 multiple-translation-unit links; the shared runner discovered and passed all nine Strings quick suites from `/tmp`. See the package evidence for the integration commands and shared consistency result. An independent read-only review of the header and test oracles found no defects.

No timing optimization, dispatch threshold or superiority claim was introduced; the counting-sort doubling algorithm meets the requested asymptotic bound directly. Consequently there is no new benchmark gate. Sparse-table memory is explicit and optional. Stress mode is available; an unexecuted stress run, unavailable GCC14 runtime and all possible integral template instantiations are not claimed as tested. Finite checks supplement the correctness arguments rather than proving all inputs.
