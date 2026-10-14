# 12-sais.hpp — evidence

`12-sais.hpp` implements linear-time suffix sorting by induced sorting (SA-IS, Nong–Zhang–Chan) for bounded integer alphabets, byte strings and arbitrary ordered symbols (after `compressAlphabet`), plus Kasai's LCP array.

## Contracts

### sais (integer alphabet with upper bound)

- `template<int NAIVE = 32> vector<int> sais(const vector<int> &s, int upper)`: symbols in `[0, upper]` (`upper >= 0`, every symbol checked by an `O(n)` entry assertion), `n < INT_MAX`. Returns the `n` suffix starts in lexicographic order, sentinel-free (no caller sentinel, no reserved symbol, the empty suffix omitted; a shorter suffix that is a prefix of a longer one sorts first). `O(n + upper)` time and `O(n + upper)` workspace (bucket arrays of size `upper + 2`), `O(n)` per recursion level with halving sizes. Empty input returns `{}`.
- `NAIVE >= 1` (`static_assert`): inputs (and reduced recursive inputs) shorter than `NAIVE` are sorted by direct suffix comparison. The default 32 is measured (Benchmarks); `NAIVE = 1` runs pure SA-IS on every level. The threshold is a compile-time parameter, so no global state exists.
- The original `saisSentinelFree` row item is this function: classical SA-IS appends a unique smallest sentinel; this implementation treats the position after the end as a virtual sentinel (last symbol L-type, `n - 1` induced first), so no sentinel is stored or returned. A caller needing the empty suffix prepends `n`.

### sais (string_view)

- `sais(string_view s)`: bytes compared as `unsigned char` (NUL and 128–255 valid), `upper = 255`, `O(n)` time and space. `string` arguments bind to this overload.

### compressAlphabet, sais (vector<T>)

- `compressAlphabet(const vector<T> &s)` returns `{codes, k}`: `codes[i]` in `[0, k)` is the rank of `s[i]` among the `k` distinct values, preserving `<` and equality (equality is `!(a < b) && !(b < a)`; `T` needs a strict weak order only). One index sort: `O(n log(n+1))` time, `O(n)` space (ranking by `lower_bound` would be quadratic under `_GLIBCXX_DEBUG`, which checks each searched range).
- `template<class T> sais(const vector<T> &s)` compresses, then runs `sais(codes, max(k - 1, 0))`: `O(n log(n+1))`. For `T = int` this is the overload without an upper bound.

### lcpArray

- `lcpArray(s, sa)` for any indexable `s` (`vector<T>`, `string`, `string_view`) and its suffix array `sa` (size asserted; the order itself is a precondition, not checked): returns `max(n - 1, 0)` entries, `lcp[i] = LCP(sa[i], sa[i+1])` (the `07`/ACL convention). Kasai: `O(n)` time, `O(n)` space (inverse ranks).

Correctness: positions are typed S (suffix smaller than its successor) or L, with the last position L because the virtual sentinel is smaller than every symbol. LMS positions (S preceded by L) seeded at the tails of their first-symbol buckets induce, by one left-to-right pass placing L-type predecessors at bucket heads and one right-to-left pass placing S-type predecessors at bucket tails, an order in which LMS substrings are sorted; equal LMS substrings (same length, same symbols up to and including the next LMS position; the last LMS substring, ending at the virtual sentinel, is unique) receive the same name. The reduced string of names, in text order, has at most `n/2` symbols; its suffix array orders the LMS suffixes, and a final induction from them in that order sorts all suffixes. When all names differ the reduced order is read directly. Each level costs `O(n + alphabet)` and sizes halve, so the total is linear. Kasai's lower bound `k - 1` for the next text position is the standard argument also recorded in [07-suffixarray.md](07-suffixarray.md).

## Feature-to-test map

Runnable entry: [12-sais_tester.py](<../../96-Local Testing/07-Strings/12-sais_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/12-sais_tester.cpp>). All oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| `sais(vector<int>, upper)` | Comparison-sorted suffixes for every ternary text through lengths 7/10/11 (quick/full/stress, also `upper = 4` with unused symbols through length 6), every binary text of lengths 13–16 and every 7th of lengths 17–18 in full/stress, random texts up to 300 symbols (every seventh round of length 30–34, the default threshold neighbours) with `upper` up to about 2,000 and periodic overrides; at `NAIVE` = default, 1 (pure SA-IS at every recursion level) and 4. |
| `sais(string_view)` / `string` | Unsigned byte order oracle on random bytes, ternary bytes, the full ascending/descending byte alphabet, 50 NULs and the empty string. |
| `sais(vector<T>)`, `compressAlphabet` | Comparison-sorted suffixes and sorted-unique rank oracle for `int`, `lng` extremes, `ulng` extremes, `signed char` (negative symbols) and `string` symbols; pairwise order preservation. |
| `lcpArray` | Direct character-by-character LCP for every overload; large texts against binary lifting over exact names of power-of-two blocks (`blockLcp`: a hash map keyed by the pair of half-block names, so equal names mean equal blocks; itself checked against direct LCP on the exhaustive ternary and random integer texts). `SuffixArray::lcp` is no oracle: `07` computes it with `lcpArray` since the P017 prune. |
| Large/adversarial | Unary, period 3, Fibonacci, Thue–Morse, random 4-letter and sparse random (values below 10^6) texts of 5,000/200,000/700,000 symbols against the verified prefix-doubling `SuffixArray` (independent algorithm); generic overload agreement. |
| Preconditions | Four checked subprocesses require assertion `SIGABRT`: negative `upper`, symbol above `upper`, negative symbol, `lcpArray` size mismatch. |

## Commands and results

P017 prune run, 2026-10-13 (large `lcpArray` oracle replaced, since `07` now computes `SuffixArray::lcp` with `lcpArray`): 11th Gen Intel Core i9-11900H, GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`), Python 3.14.7. Configurations (shared string runner): optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` (with the assertion probes), and in full/stress `-O1 -g -fsanitize=address,undefined`; all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full runs passed the quick pre-flight under the 2048 MB cap; `MEMORY peak` is the run's capped peak (4096 MB cap, 0 OOM kills).

```bash
python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261013  # PASS, 3 configurations, 283,735 cases and 10,223,799 checks each, 4 assertion probes, 135 s, MEMORY peak=873MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261013  # PASS, 3 configurations, same counts, 142 s, MEMORY peak=858MB
CP_TEST_PREFLIGHT=0 python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode stress --seed 20261013 --configuration checked  # PASS, 520,882 cases and 47,710,170 checks, 4 assertion probes, MEMORY peak=523MB
python3 '96-Local Testing/07-Strings/12-sais_benchmark.py' --reps 5  # PASS, every checksum equal, MEMORY peak=349MB
```

The checked build is the slowest: at full size it ran in 70 s on g++-14 against 59 s with the previous large check, under the runner's 180 s limit. At stress size (700,000 symbols) it passes on g++ (157 s in the review) but exceeds the limit on g++-14 (187 s, and 184 s with the previous tester, so the overrun predates this prune); the floor compiler is required in full mode only. A mutation capping `lcpArray` at 1000 passes every small case and fails the large block-name check in quick mode. Integration, the folder run and consistency are recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results).

## Benchmarks

Driver: [12-sais_benchmark.py](<../../96-Local Testing/07-Strings/12-sais_benchmark.py>) with [12-sais_benchmark.cpp](<../../96-Local Testing/07-Strings/12-sais_benchmark.cpp>), `python3 '96-Local Testing/07-Strings/12-sais_benchmark.py' --reps 5`, rerun 2026-10-13 after the P017 prune (seed 20261009, `-std=gnu++20 -O2 -DNDEBUG`, GCC 16.2.1, Intel Core i9-11900H, load average below 1, one warmup plus five rotating repetitions, medians in ms, every method's suffix-array checksum equal; MEMORY peak=349MB). Chunk workloads are 2,000,000 symbols split into independent texts of the given length; `doubling` is the verified `07` `SuffixArray<int>(s, false)`, whose LCP step is now `lcpArray`.

| Workload | `sais<1>` | `sais<8>` | `sais<16>` | `sais<32>` | `sais<64>` | `doubling` |
|---|---:|---:|---:|---:|---:|---:|
| chunks n=8 S=4 | 96.2 | 99.3 | 34.2 | 33.3 | 33.4 | 96.5 |
| chunks n=16 S=4 | 97.0 | 96.6 | 95.8 | 48.5 | 48.8 | 85.3 |
| chunks n=24 S=4 | 95.9 | 94.8 | 92.0 | 76.1 | 77.2 | 91.1 |
| chunks n=32 S=4 | 97.1 | 98.3 | 90.0 | 90.9 | 87.3 | 89.4 |
| chunks n=64 S=4 | 100.1 | 101.1 | 100.5 | 88.2 | 87.7 | 84.4 |
| chunks n=256 S=4 | 86.6 | 87.8 | 88.4 | 88.7 | 87.9 | 79.6 |
| unary chunks n=16 | 32.7 | 25.7 | 33.6 | 18.6 | 17.3 | 79.2 |
| unary chunks n=32 | 26.1 | 23.4 | 25.7 | 23.2 | 48.3 | 84.0 |
| unary chunks n=48 | 24.2 | 22.5 | 23.4 | 22.5 | 97.4 | 87.6 |
| unary chunks n=64 | 23.2 | 22.0 | 22.9 | 22.5 | 22.5 | 87.2 |
| random n=1000000 S=2 | 49.2 | 49.1 | 48.2 | 48.0 | 47.7 | 95.4 |
| random n=1000000 S=4 | 49.9 | 49.5 | 50.8 | 49.4 | 50.3 | 97.3 |
| random n=1000000 S=26 | 50.1 | 48.4 | 48.7 | 48.1 | 48.4 | 86.8 |
| fibonacci n=1000000 | 22.1 | 21.8 | 21.7 | 21.3 | 21.2 | 161.9 |
| unary n=1000000 | 11.5 | 10.7 | 10.5 | 10.6 | 10.7 | 123.4 |

Threshold choice: `NAIVE = 32`. Against 16 it halves random 16-symbol texts (95.8 to 48.5 ms), cuts 24-symbol texts by 17%, 64-symbol texts by 12% and unary 16-symbol texts by 45%, and is no worse beyond noise anywhere else, including every 10^6-symbol input (the reduced recursive strings are rarely short enough to matter). `NAIVE = 64` was rejected: the comparison sort is quadratic in compared symbols on periodic texts (unary 48-symbol texts: 97.4 ms against 22.5 ms). At 10^6 symbols SA-IS is 1.8–2.0 times faster than prefix doubling on random texts, 7.6 times on Fibonacci and 11.6 times on unary; for many tiny independent texts both are allocation-bound and doubling is 4–10% faster at 64–256 symbols, which is why `07` keeps its own construction.

## Sources

| Source | Use |
|---|---|
| G. Nong, S. Zhang, W. H. Chan, “Linear Suffix Array Construction by Almost Pure Induced-Sorting”, DCC 2009 (algorithm as described in the sources below; the paper itself was not re-read) | SA-IS structure: S/L types, LMS substrings, induced sorting, naming and recursion. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Sentinel-free convention, overload set (`vector<int>` with upper, `vector<T>`, `string`), compile-time naive/doubling thresholds (10/40), `lcp_array` convention. Read for API and conventions; the implementation here was written independently. |
| [OI Wiki, 后缀数组](https://oi-wiki.org/string/sa/) | SA-IS outline and the LCP (`height`) lemma. |
| maspypy, ei1333, Nyaan suffix-array pages (P081 sweep in [00-sources.md](00-sources.md)) | API survey: inverse suffix array and `n + 1`-entry LCP conventions (not adopted, see notes). |

No legacy SA-IS implementation exists in `OLD` (archive map and text search).

## Limits and handoffs

- `upper` drives bucket memory; sparse integer alphabets should use the generic overload. Prefix doubling, RMQ, LCE and pattern search stay in `07`, which includes this header for `lcpArray`; `29` can derive a BWT from `sais`.

## History

- 2026-10-09: implemented and verified (P081), independent review found 2 issues, fixed.
- 2026-10-13: P017 prune, `07` `SuffixArray` now builds its LCP with `lcpArray`, so the large `lcpArray` check moved from `SuffixArray::lcp` to an exact block-name lifting oracle; full suite passed on g++ and g++-14, checked stress passed, benchmark rerun.
