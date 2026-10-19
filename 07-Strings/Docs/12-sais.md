# 12-sais.hpp — evidence

`12-sais.hpp` implements linear-time suffix sorting by induced sorting (SA-IS, Nong–Zhang–Chan) for bounded integer alphabets, byte strings and arbitrary ordered symbols (after `compressAlphabet`), plus Kasai's LCP array.

## Contracts

### sais (integer alphabet with upper bound)

- `template<int NAIVE = 32> vector<int> sais(const vector<int> &s, int upper)`: symbols in `[0, upper]` (`upper >= 0`, every symbol checked by an `O(n)` entry assertion), `n < INT_MAX`. Returns the `n` suffix starts in lexicographic order, sentinel-free (no caller sentinel, no reserved symbol, the empty suffix omitted; a shorter suffix that is a prefix of a longer one sorts first). `O(n + upper)` time and `O(n + upper)` workspace (bucket arrays of size `upper + 2`), `O(n)` per recursion level with halving sizes. Empty input returns `{}`.
- `NAIVE >= 1` (`static_assert`): inputs (and reduced recursive inputs) shorter than `NAIVE` are sorted by direct suffix comparison. The default 32 is measured (Benchmarks); `NAIVE = 1` runs pure SA-IS on every level. The threshold is a compile-time parameter, so no global state exists.
- Sentinel-free: classical SA-IS appends a unique smallest sentinel; this implementation treats the position after the end as a virtual sentinel (last symbol L-type, `n - 1` induced first), so no sentinel is stored or returned. A caller needing the empty suffix prepends `n`.

### sais (string_view)

- `sais(string_view s)`: bytes compared as `unsigned char` (NUL and 128–255 valid), `upper = 255`, `O(n)` time and space. `string` arguments bind to this overload.

### compressAlphabet, sais (vector<T>)

- `compressAlphabet(const vector<T> &s)` returns `{codes, k}`: `codes[i]` in `[0, k)` is the rank of `s[i]` among the `k` distinct values, preserving `<` and equality (equality is `!(a < b) && !(b < a)`; `T` needs a strict weak order only). One index sort: `O(n log(n+1))` time, `O(n)` space (ranking by `lower_bound` would be quadratic under `_GLIBCXX_DEBUG`, which checks each searched range).
- `template<class T> sais(const vector<T> &s)` compresses, then runs `sais(codes, max(k - 1, 0))`: `O(n log(n+1))`. For `T = int` this is the overload without an upper bound.

### lcpArray

- `lcpArray(s, sa)` for any indexable `s` (`vector<T>`, `string`, `string_view`) and its suffix array `sa` (size asserted; the order itself is a precondition, not checked): returns `max(n - 1, 0)` entries, `lcp[i] = LCP(sa[i], sa[i+1])` (the `07`/ACL convention). Kasai: `O(n)` time, `O(n)` space (inverse ranks).

Correctness: positions are typed S (suffix smaller than its successor) or L, with the last position L because the virtual sentinel is smaller than every symbol. LMS positions (S preceded by L) seeded at the tails of their first-symbol buckets induce, by one left-to-right pass placing L-type predecessors at bucket heads and one right-to-left pass placing S-type predecessors at bucket tails, an order in which LMS substrings are sorted; equal LMS substrings (same length, same symbols up to and including the next LMS position; the last LMS substring, ending at the virtual sentinel, is unique) receive the same name. The reduced string of names, in text order, has at most `n/2` symbols; its suffix array orders the LMS suffixes, and a final induction from them in that order sorts all suffixes. When all names differ the reduced order is read directly. Each level costs `O(n + alphabet)` and sizes halve, so the total is linear. Kasai's lower bound `k - 1` for the next text position is the standard argument also recorded in [07-suffixarray.md](07-suffixarray.md).

## Feature-to-test map

Runnable entry: [12-sais_tester.py](<../../96-Local Testing/07-Strings/12-sais_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/12-sais_tester.cpp>).

| Feature/domain | Independent verification |
|---|---|
| `sais(vector<int>, upper)` | Comparison-sorted suffixes for every ternary text through lengths 7/10/11 (quick/full/stress, also `upper = 4` with unused symbols through length 6), every binary text of lengths 13–16 and every 7th of lengths 17–18 in full/stress, random texts up to 300 symbols (every seventh round of length 30–34, the default threshold neighbours) with `upper` up to about 2,000 and periodic overrides; at `NAIVE` = default, 1 (pure SA-IS at every recursion level) and 4. |
| `sais(string_view)` / `string` | Unsigned byte order oracle on random bytes, ternary bytes, the full ascending/descending byte alphabet, 50 NULs and the empty string. |
| `sais(vector<T>)`, `compressAlphabet` | Comparison-sorted suffixes and sorted-unique rank oracle for `int`, `lng` extremes, `ulng` extremes, `signed char` (negative symbols) and `string` symbols; pairwise order preservation. |
| `lcpArray` | Direct character-by-character LCP for every overload; large texts against binary lifting over exact names of power-of-two blocks (`blockLcp`: a hash map keyed by the pair of half-block names, so equal names mean equal blocks; itself checked against direct LCP on the exhaustive ternary and random integer texts). `SuffixArray::lcp` is no oracle: `07` computes it with `lcpArray` since the P017 prune. |
| Large/adversarial | Unary, period 3, Fibonacci, Thue–Morse, random 4-letter and sparse random (values below 10^6) texts of 5,000/200,000/700,000 symbols against `SuffixArray<int>::doubling` (`07` prefix doubling, an independent algorithm; `07`'s constructors call `sais` since 2026-10-19); generic overload agreement. |
| Preconditions | Four checked subprocesses require assertion `SIGABRT`: negative `upper`, symbol above `upper`, negative symbol, `lcpArray` size mismatch. |

## Commands and results

P017 run, 2026-10-19 (the large suffix-order oracle is now `SuffixArray<int>::doubling`, since `07`'s constructors call `sais`; the stress line is from the 2026-10-13 prune): 11th Gen Intel Core i9-11900H, GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`), Python 3.14.7. Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>).

```bash
python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, 283,735 cases and 10,115,167 checks each, 4 assertion probes, MEMORY peak=814MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, same counts, MEMORY peak=849MB
CP_TEST_PREFLIGHT=0 python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode stress --seed 20261013 --configuration checked  # PASS, 520,882 cases and 47,710,170 checks, 4 assertion probes, MEMORY peak=523MB
python3 '96-Local Testing/07-Strings/12-sais_benchmark.py' --reps 5  # PASS, every checksum equal, MEMORY peak=358MB
```

The checked build is the slowest: at full size it runs in 70 s on g++-14, under the runner's 180 s limit. A mutation capping `lcpArray` at 1000 passes every small case and fails the large block-name check in quick mode. Integration and consistency are recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results).

## Benchmarks

Driver: [12-sais_benchmark.py](<../../96-Local Testing/07-Strings/12-sais_benchmark.py>) with [12-sais_benchmark.cpp](<../../96-Local Testing/07-Strings/12-sais_benchmark.cpp>), `python3 '96-Local Testing/07-Strings/12-sais_benchmark.py' --reps 5`, rerun 2026-10-19 for P017 (seed 20261009, `-std=gnu++20 -O2 -DNDEBUG`, GCC 16.2.1, Intel Core i9-11900H, shared host with load average 2.2–3.2, one warmup plus five rotating repetitions, medians in ms, every method's suffix-array checksum equal; MEMORY peak=358MB). Chunk workloads are 2,000,000 symbols split into independent texts of the given length; `doubling` is `07`'s `SuffixArray<int>::doubling(s, false)` (suffix array, inverse ranks and `lcpArray`), so it does more work than the `sais` columns.

| Workload | `sais<1>` | `sais<8>` | `sais<16>` | `sais<32>` | `sais<64>` | `doubling` |
|---|---:|---:|---:|---:|---:|---:|
| chunks n=8 S=4 | 102.5 | 103.3 | 35.6 | 35.7 | 35.1 | 100.1 |
| chunks n=16 S=4 | 103.8 | 103.2 | 102.6 | 52.9 | 52.1 | 92.8 |
| chunks n=24 S=4 | 101.1 | 100.3 | 98.3 | 80.6 | 80.9 | 98.0 |
| chunks n=32 S=4 | 103.2 | 104.0 | 96.3 | 99.1 | 92.2 | 96.7 |
| chunks n=64 S=4 | 106.8 | 108.1 | 106.5 | 94.4 | 92.9 | 88.7 |
| chunks n=256 S=4 | 91.7 | 92.9 | 93.2 | 93.8 | 93.1 | 85.0 |
| unary chunks n=16 | 27.4 | 27.1 | 26.7 | 18.1 | 18.4 | 80.1 |
| unary chunks n=32 | 25.8 | 25.7 | 25.4 | 25.7 | 53.1 | 91.0 |
| unary chunks n=48 | 24.7 | 24.3 | 24.4 | 24.8 | 105.6 | 95.6 |
| unary chunks n=64 | 25.0 | 25.9 | 25.7 | 25.3 | 25.3 | 95.4 |
| random n=1000000 S=2 | 54.5 | 53.4 | 54.0 | 54.3 | 54.0 | 116.6 |
| random n=1000000 S=4 | 56.1 | 57.0 | 55.7 | 55.6 | 55.7 | 115.9 |
| random n=1000000 S=26 | 53.8 | 54.1 | 54.9 | 54.5 | 54.5 | 102.3 |
| fibonacci n=1000000 | 28.6 | 28.4 | 28.2 | 27.4 | 27.3 | 223.4 |
| unary n=1000000 | 12.5 | 12.0 | 11.3 | 11.5 | 11.5 | 141.3 |

Threshold choice: `NAIVE = 32`. Against 16 it halves random 16-symbol texts (102.6 to 52.9 ms), cuts 24-symbol texts by 18%, 64-symbol texts by 11% and unary 16-symbol texts by 32%, and is no worse beyond noise anywhere else, including every 10^6-symbol input (the reduced recursive strings are rarely short enough to matter). `NAIVE = 64` was rejected: the comparison sort is quadratic in compared symbols on periodic texts (unary 48-symbol texts: 105.6 ms against 24.8 ms). At 10^6 symbols SA-IS is 1.9–2.1 times faster than prefix doubling on random texts, 8.2 times on Fibonacci and 12.3 times on unary; for many tiny independent texts both are allocation-bound and doubling is 2–9% faster at 32–256 symbols, which is why `07` keeps prefix doubling as `SuffixArray<T>::doubling` beside its SA-IS constructors ([07-suffixarray.md](07-suffixarray.md#benchmarks)).

## Sources

| Source | Use |
|---|---|
| G. Nong, S. Zhang, W. H. Chan, “Linear Suffix Array Construction by Almost Pure Induced-Sorting”, DCC 2009 (algorithm as described in the sources below; the paper itself was not re-read) | SA-IS structure: S/L types, LMS substrings, induced sorting, naming and recursion. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Sentinel-free convention, overload set (`vector<int>` with upper, `vector<T>`, `string`), compile-time naive/doubling thresholds (10/40), `lcp_array` convention. Read for API and conventions; the implementation here was written independently. |
| [OI Wiki, 后缀数组](https://oi-wiki.org/string/sa/) | SA-IS outline and the LCP (`height`) lemma. |
| maspypy, ei1333, Nyaan suffix-array pages (P081 sweep in [00-sources.md](00-sources.md)) | API survey: inverse suffix array and `n + 1`-entry LCP conventions (not adopted, see notes). |

No legacy SA-IS implementation exists in `OLD` (archive map and text search).

## Limits and handoffs

- `upper` drives bucket memory; sparse integer alphabets should use the generic overload. Prefix doubling, RMQ, LCE and pattern search stay in `07`, which builds through `sais` and `lcpArray`; `29` can derive a BWT from `sais`.
- The checked stress run (700,000 symbols) passes on g++ (157 s) but exceeds the runner's 180 s limit on g++-14 (187 s).

## History

- 2026-10-09: implemented and verified (P081), independent review found 2 issues, fixed.
- 2026-10-13: P017 prune, `07` `SuffixArray` now builds its LCP with `lcpArray`, so the large `lcpArray` check moved from `SuffixArray::lcp` to an exact block-name lifting oracle; full suite passed on g++ and g++-14, checked stress passed, benchmark rerun.
- 2026-10-19: P017, `07`'s constructors now call `sais`, so the large suffix-order oracle and the benchmark's `doubling` column use `SuffixArray<int>::doubling`; full suite passed on g++ and g++-14, benchmark rerun.
