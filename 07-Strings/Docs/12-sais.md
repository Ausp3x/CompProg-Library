# 12-sais.hpp — evidence

`12-sais.hpp` implements linear-time suffix sorting by induced sorting (SA-IS, Nong–Zhang–Chan) for bounded integer alphabets, byte strings and arbitrary ordered symbols (after `compressAlphabet`), plus Kasai's LCP array.

## Contracts

### sais (integer alphabet with upper bound)

- `template<int NAIVE = 32> vector<int> sais(const vector<int> &s, int upper)`: symbols in `[0, upper]` (`upper >= 0`, every symbol checked by an `O(n)` entry assertion), `n < INT_MAX`. Returns the `n` suffix starts in lexicographic order, sentinel-free (no caller sentinel, no reserved symbol, the empty suffix omitted; a shorter suffix that is a prefix of a longer one sorts first). `O(n + upper)` time and `O(n + upper)` workspace (bucket arrays of size `upper + 2`), `O(n)` per recursion level with halving sizes. Empty input returns `{}`.
- `NAIVE`: inputs (and reduced recursive inputs) shorter than `NAIVE` are sorted by direct suffix comparison. The default 32 is measured (Benchmarks); `NAIVE = 1` runs pure SA-IS on every level. The threshold is a compile-time parameter, so no global state exists.
- The original `saisSentinelFree` row item is this function: classical SA-IS appends a unique smallest sentinel; this implementation treats the position after the end as a virtual sentinel (last symbol L-type, `n - 1` induced first), so no sentinel is stored or returned. A caller needing the empty suffix prepends `n`.

### sais (string_view)

- `sais(string_view s)`: bytes compared as `unsigned char` (NUL and 128–255 valid), `upper = 255`, `O(n)` time and space. `string` arguments bind to this overload.

### compressAlphabet, sais (vector<T>)

- `compressAlphabet(const vector<T> &s)` returns `{codes, k}`: `codes[i]` in `[0, k)` is the rank of `s[i]` among the `k` distinct values, preserving `<` and equality (equality is `!(a < b) && !(b < a)`; `T` needs a strict weak order only). One index sort: `O(n log(n+1))` time, `O(n)` space.
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
| `lcpArray` | Direct character-by-character LCP for every overload; large texts against `SuffixArray::lcp`. |
| Large/adversarial | Unary, period 3, Fibonacci, Thue–Morse, random 4-letter and sparse random (values below 10^6) texts of 5,000/200,000/700,000 symbols against the verified prefix-doubling `SuffixArray` (independent algorithm); generic overload agreement. |
| Preconditions | Four checked subprocesses require assertion `SIGABRT`: negative `upper`, symbol above `upper`, negative symbol, `lcpArray` size mismatch. |

## Commands and results

Run 2026-10-09 on an 11th Gen Intel Core i9-11900H with GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`). Configurations (shared string runner): optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` (with the assertion probes), and in full/stress `-O1 -g -fsanitize=address,undefined`; all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress runs passed the quick pre-flight under the 2048 MB cap; `MEMORY peak` is the run's capped peak (4096 MB cap, 0 OOM kills).

```bash
python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode quick --seed 20261009  # PASS, 2 configurations, 128,767 cases and 1,040,018 checks each, 4 assertion probes
python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, 283,735 cases and 10,143,918 checks each, MEMORY peak=843MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, same counts, MEMORY peak=809MB
python3 '96-Local Testing/07-Strings/12-sais_tester.py' --mode stress --seed 20261010  # PASS, 3 configurations, 520,882 cases and 47,071,298 checks each, MEMORY peak=936MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 114 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=2737MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261009 --no-integration  # PASS, 12 suites
python3 '96-Local Testing/03-consistency.py'  # only the pre-existing, out-of-scope error 'Renamed/new verified suites missing from quick discovery' (16-poly_tester.py, introduced by P231)
```

## Benchmarks

Driver: [12-sais_benchmark.py](<../../96-Local Testing/07-Strings/12-sais_benchmark.py>) with [12-sais_benchmark.cpp](<../../96-Local Testing/07-Strings/12-sais_benchmark.cpp>), `python3 '96-Local Testing/07-Strings/12-sais_benchmark.py' --reps 5` (seed 20261009, `-std=gnu++20 -O2 -DNDEBUG`, GCC 16.2.1, Intel Core i9-11900H, one warmup plus five rotating repetitions, medians in ms, every method's suffix-array checksum equal; MEMORY peak=350MB). Chunk workloads are 2,000,000 symbols split into independent texts of the given length; `doubling` is the verified `07` `SuffixArray<int>(s, false)`.

| Workload | `sais<1>` | `sais<8>` | `sais<16>` | `sais<32>` | `sais<64>` | `doubling` |
|---|---:|---:|---:|---:|---:|---:|
| chunks n=8 S=4 | 102.4 | 102.0 | 36.8 | 37.1 | 36.5 | 90.0 |
| chunks n=16 S=4 | 104.9 | 104.0 | 104.8 | 55.1 | 55.1 | 87.1 |
| chunks n=24 S=4 | 103.8 | 103.0 | 101.2 | 84.2 | 85.4 | 93.1 |
| chunks n=32 S=4 | 101.4 | 101.5 | 93.2 | 94.3 | 91.4 | 90.7 |
| chunks n=64 S=4 | 103.8 | 103.8 | 103.5 | 90.8 | 92.5 | 82.2 |
| chunks n=256 S=4 | 90.4 | 91.4 | 90.2 | 90.9 | 93.1 | 79.8 |
| unary chunks n=16 | 36.4 | 28.1 | 28.9 | 18.4 | 18.0 | 71.6 |
| unary chunks n=32 | 30.1 | 27.3 | 27.7 | 30.1 | 52.5 | 84.8 |
| unary chunks n=48 | 27.6 | 26.1 | 26.0 | 27.6 | 103.2 | 92.7 |
| unary chunks n=64 | 26.7 | 25.5 | 26.1 | 25.6 | 22.2 | 89.3 |
| random n=1000000 S=2 | 53.5 | 51.4 | 53.0 | 54.4 | 52.5 | 106.6 |
| random n=1000000 S=4 | 56.6 | 56.6 | 55.4 | 55.6 | 55.1 | 108.3 |
| random n=1000000 S=26 | 52.5 | 52.5 | 52.2 | 52.4 | 51.4 | 88.2 |
| fibonacci n=1000000 | 28.0 | 27.0 | 26.5 | 26.8 | 26.2 | 202.1 |
| unary n=1000000 | 12.4 | 12.1 | 12.1 | 11.4 | 11.6 | 128.2 |

Threshold choice: `NAIVE = 32`. Against 16 it halves random 16-symbol texts (104.8 to 55.1 ms), cuts 24-symbol texts by 17% and unary 16-symbol texts by 36%, and is within noise everywhere else, including every 10^6-symbol input (the reduced recursive strings are rarely short enough to matter). `NAIVE = 64` was rejected: the comparison sort is quadratic in compared symbols on periodic texts (unary 48-symbol texts: 103.2 ms against 27.6 ms). At 10^6 symbols SA-IS is 1.7–2.0 times faster than prefix doubling on random texts, 7.5 times on Fibonacci and 11 times on unary; for many tiny independent texts both are allocation-bound and doubling is 10–12% faster at 64–256 symbols, which is why `07` keeps its own construction.

## Sources

| Source | Use |
|---|---|
| G. Nong, S. Zhang, W. H. Chan, “Linear Suffix Array Construction by Almost Pure Induced-Sorting”, DCC 2009 (algorithm as described in the sources below; the paper itself was not re-read) | SA-IS structure: S/L types, LMS substrings, induced sorting, naming and recursion. |
| [AtCoder Library `string.hpp`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp) | Sentinel-free convention, overload set (`vector<int>` with upper, `vector<T>`, `string`), compile-time naive/doubling thresholds (10/40), `lcp_array` convention. Read for API and conventions; the implementation here was written independently. |
| [OI Wiki, 后缀数组](https://oi-wiki.org/string/sa/) | SA-IS outline and the LCP (`height`) lemma. |
| maspypy, ei1333, Nyaan suffix-array pages (P081 sweep in [00-sources.md](00-sources.md)) | API survey: inverse suffix array and `n + 1`-entry LCP conventions (not adopted, see notes). |

No legacy SA-IS implementation exists in `OLD` (archive map and text search).

## Limits and handoffs

- `upper` drives bucket memory; sparse integer alphabets should use the generic overload. Prefix doubling, RMQ, LCE and pattern search stay in `07`; `29` can derive a BWT from `sais`.

## History

- 2026-10-09: implemented and verified (P081). Independent review: `static_assert(NAIVE >= 1)` added. A debug-build quadratic slowdown (`_GLIBCXX_DEBUG` checks each `lower_bound` range) was removed by ranking with one index sort in `compressAlphabet`.
