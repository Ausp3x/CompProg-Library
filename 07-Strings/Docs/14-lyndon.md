# 14-lyndon.hpp — evidence

`14-lyndon.hpp` (batch ST07, package P083) computes the Lyndon (Chen–Fox–Lyndon) factorization by Duval's algorithm, Lyndon tests, the standard factorization, the Lyndon array and the standard Lyndon tree, factorizations of every suffix, an online factorization of every prefix, and lexicographic enumeration of Lyndon words (Fredricksen–Kessler–Maiorana / Duval successor). Contest profile, GNU C++20, no ISA code.

## Contracts

Sequence functions take a random-access sequence with `.size()` below `INT_MAX` and symbols totally ordered by `<` alone (`char` compares as unsigned bytes; integers; any ordered type). A Lyndon word is a nonempty word strictly smaller than each of its proper suffixes (equivalently, than each proper rotation). Factorizations are returned as cut vectors `0 = c[0] < c[1] < ... < c[k] = n` (Library Checker `lyndon_factorization` format); factors `s[c[t], c[t+1])` are Lyndon and lexicographically non-increasing, and the factorization is unique.

### duval, longestLyndonPrefix, isLyndon, standardFactorization

`duval(s)` returns the cuts in O(n) time and O(k) returned space; empty input gives `{0}`. `longestLyndonPrefix(s)` returns the length of the longest Lyndon prefix (0 for empty input) in O(n) time, O(1) space; it equals the first Duval factor. `isLyndon(s)` is `n >= 1 && longestLyndonPrefix(s) == n`. `standardFactorization(w)` asserts `w` Lyndon with `|w| >= 2` and returns the split `m` of the standard factorization `w = w[0, m) w[m, n)`, where `w[m, n)` is the longest proper Lyndon suffix (equivalently the smallest proper suffix, the last Duval factor of `w[1, n)`); both parts are Lyndon. O(n) time, O(k) space for the cuts of `w[1, n)`.

Correctness: Duval keeps `s[i, j) = u^t u'` with `u` Lyndon of length `j - k` and `u'` a proper prefix of `u`; `s[j] = s[k]` extends the period, `s[j] > s[k]` makes `s[i, j]` Lyndon, `s[j] < s[k]` finalizes the `t` copies of `u`. Each outer round advances `i` by at least the work it repeats, giving at most `4n` comparisons.

### lyndonArray

`lyndonArray(s)` returns `res[i]` = length of the longest Lyndon prefix of `s[i, n)`. Implementation: map symbols to codes, build the suffix array with `12` `sais(code, upper)`, invert it in place to ranks and take next-smaller-suffix by a stack: `res[i] = nss(i) - i`, where `nss(i)` is the first `j > i` with suffix `j` smaller than suffix `i` (the empty suffix `n` is smaller than all). One-byte integral symbols (`char` as unsigned through `lyndon_detail::at`, `unsigned char`, `signed char` as signed, `char8_t`, `bool`) map directly to `at(s, i) - numeric_limits<C>::min()` in `[0, 255]`, an order-preserving shift of exactly the order `at` compares, so `string`, `string_view` and `vector<char>` all sort bytes unsigned with embedded NUL allowed; this path is O(n) time. Other symbol types are ranked by a comparison sort on `at` (dense codes, O(n log n), unavoidable for comparison-only symbols) and then cost O(n). O(n) space. `12` `sais(const vector<T> &)` is not used because its `compressAlphabet` compares `char` signed. Proof of the identity (Hohlweg–Reutenauer): let `j = nss(i)`. For `i < t < j`, if `s[t, j)` were a prefix of `s[i, n)` then comparing `s[t, n)` with `s[i, n)` reduces to suffix `j` against suffix `i + j - t`, and `suffix j < suffix i < suffix i + j - t` would give `suffix t < suffix i`, a contradiction; so `s[t, j)` differs from `s[i, j)` at a position where it is larger, and `s[i, j)` is Lyndon. For `j' > j`, either `s[j, j')` differs from `s[i, n)` within its length, where it is smaller, or it is a proper prefix of `s[i, j')`; both make `s[j, j') < s[i, j')`, so `s[i, j')` is not Lyndon.

### LyndonTree, lyndonTree, suffixFactorization

`lyndonTree(lambda)` takes `lambda = lyndonArray(s)` (asserting `1 <= lambda[i] <= n - i`; other invalid arrays give a bounded but meaningless tree) and returns `LyndonTree{l, r, left, right, roots}`. Nodes `0..n-1` are the leaves `[i, i + 1)` with `left = right = -1`; internal nodes follow, each with interval `[l, r)`, children `left = [l, m)` and `right = [m, r)` where `[m, r)` is the longest proper Lyndon suffix of `s[l, r)` (standard bracketing). `roots` lists the trees of the Lyndon factors left to right, so a Lyndon word has one root. There are `2n - (number of factors)` nodes. O(n) time and space. Correctness: scanning `i` downward, the stack holds the factor trees of `s[i + 1, n)`, whose last element is the minimal suffix; merging while the top ends by `i + lambda[i]` builds `s[i, i + lambda[i])` with the last merged factor as right child, which is the minimal proper suffix of the merged word and hence its standard right factor.

`suffixFactorization(lambda, i)` returns the cuts of the Lyndon factorization of `s[i, n)` for `i` in `[0, n]` (asserted) by following `i, i + lambda[i], ...`; every visited entry is asserted in `[1, n - i]` and the walk advances by at least one, so an invalid array stays bounded under `NDEBUG`; O(out). Correct because the first factor of a factorization is the longest Lyndon prefix.

### IncrementalLyndon<T>

Online Duval over symbols of type `T` (ordered by `<`; for bytes use `T = int` with unsigned values or `T = unsigned char`). `add(c)` appends a symbol in amortized O(1) (total at most the offline Duval work on the final string); fewer than `INT_MAX - 1` symbols. `size()` is the length. `minSuffix(m)` for `m` in `[0, size()]` (asserted) is the length of the last Lyndon factor of the prefix of length `m` (its minimal nonempty suffix; 0 for `m = 0`). `factorize(m)` returns the cuts of the factorization of that prefix in O(number of factors). Fields `s`, `mn`, `i`, `j`, `k` are the Duval state and must not be edited.

Correctness: with the state `s[i, j) = u^t u'`, the factorization of a prefix `P` ending in the current round is the finished factors, then `u^t`, then the factorization of `u'`, which equals the factorization of the earlier prefix ending at `i + |u'|` restricted to `[i, ...)`. Hence `mn[j] = mn[k]` when `|u'| > 0` (with `k = j - |u|`), `mn[j] = |u|` when `u'` is empty, and `mn[j] = j - i` when `s[i, j)` becomes Lyndon. Restarts reprocess positions whose prefix values do not change.

### nextLyndonWord, lyndonWords

`nextLyndonWord(w, n, k)` transforms a Lyndon word `w` over `[0, k)` with `1 <= |w| <= n` (asserted) into the next Lyndon word of length at most `n` in lexicographic order and returns `true`; after the last word (`{k - 1}`) it clears `w` and returns `false`. Under `NDEBUG` an empty `w` returns `false` and an over-long `w` is not extended, so a violated precondition stays bounded. Worst case O(n) per call, amortized O(1) over a full enumeration (Berstel–Pocchiola). `lyndonWords(n, k, f, exact = false)` calls `f(const vector<int> &)` for every Lyndon word of length at most `n` (only length `n` when `exact`) in lexicographic order; `n <= 0` or `k <= 0` produce nothing (negative values also assert). O(n + number of Lyndon words of length at most n) amortized, plus the cost of `f`.

## Feature-to-test map

Runner: [`14-lyndon_tester.py`](<../../96-Local Testing/07-Strings/14-lyndon_tester.py>). The oracle tests Lyndon-ness directly (strictly below every proper suffix, O(n^2)) and factorizes greedily by longest Lyndon prefix.

| Operation | Coverage |
|---|---|
| duval, longestLyndonPrefix, isLyndon | Exhaustive ternary strings through length 6/8/9 (quick/full/stress), 200/1500/10000 random byte strings (some periodic, a third over 256 bytes) also as extreme-`lng` vectors, 256 bytes ascending and descending; brute factors checked non-increasing. |
| standardFactorization | Every Lyndon case of length at least 2 against the longest proper Lyndon suffix; the left part checked Lyndon. |
| lyndonArray, suffixFactorization | Every position of every case against brute longest Lyndon prefixes and brute suffix factorizations; large unary closed form. Byte order: every string over `{00, 7f, 80, ff}` through length 5/7/7 (quick/full/stress) as `string`, through 5/6/6 as `vector<char>` and `vector<signed char>` (signed order), and through 5 as `vector<unsigned char>` and `u8string`, all against the brute force. Non-byte path: 200/1000/2000 random `vector<string>`, `deque<int16_t>`, `vector<uint16_t>` cases of length at most 20 against the brute force. Large: random 256-byte, random `{00, ff}` and `(7f^4 80)^*` strings of length 20000/1000000/1000000, every position checked against the independent `07` prefix-doubling suffix array (`SuffixArray<int>::doubling` over unsigned byte codes) (suffix `i + res[i]` smaller, every suffix strictly inside larger by sparse-table minimum) and `vector<char>` equal to `string`. A signed-byte mutation of the code map fails the byte-order cases. |
| LyndonTree, lyndonTree | Node count `2n - factors`, roots equal to the factors, every node Lyndon, leaves, child partition, right child equal to the brute longest proper Lyndon suffix; large trees checked for size and contiguous roots. |
| IncrementalLyndon: add, size, minSuffix, factorize | After every add, `factorize(p)` and `minSuffix(p)` for every prefix `p` against brute factorization and the brute minimal suffix. |
| nextLyndonWord, lyndonWords | All words of length up to `n` over `k <= 4` letters enumerated, filtered by the brute test and sorted, compared with both enumeration modes for `n <= 6/9/11`; successor of every word and clearing after the last. |
| cross-checks at scale | Duval, IncrementalLyndon and `suffixFactorization(lyndonArray(s), 0)` agree on random binary, `a^6 b` periodic and Thue–Morse strings of length 20000/300000/1000000. |

Checked builds run twelve precondition probes: `standardFactorization` on a single letter and on a non-Lyndon word, `lyndonTree` with a zero and an overflowing length, `suffixFactorization` below and above range and through a zero-length entry, `minSuffix` above range, `factorize` below range, `nextLyndonWord` on an empty and an over-long word, `lyndonWords` with negative `n`.

## Commands and results

Run 2026-10-12 on the final code (SA-IS `lyndonArray`, unused `07` include dropped; quick and full rerun after the drop). The full lines are the 2026-10-19 P017 rerun with the large oracle `SuffixArray<int>::doubling`, before `07`'s final code-range cutoff (recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results)). Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>).

```bash
python3 '96-Local Testing/07-Strings/14-lyndon_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 1,125,301 checks, 12 assertion probes
python3 '96-Local Testing/07-Strings/14-lyndon_tester.py' --mode full  # PASS, 7,698,991 checks per configuration, MEMORY peak 872 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/14-lyndon_tester.py' --mode full  # PASS, same counts, MEMORY peak 767 MB
python3 '96-Local Testing/07-Strings/14-lyndon_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 20,745,777 checks (optimized, ASan/UBSan), 7,725,613 (checked), MEMORY peak 960 MB
python3 '96-Local Testing/07-Strings/29-bwt_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 5 assertion probes
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 136 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 20 suites
python3 '96-Local Testing/03-consistency.py' --braces '07-Strings/14-lyndon.hpp' '96-Local Testing/07-Strings/14-lyndon_tester.cpp'  # no violations
python3 '96-Local Testing/03-consistency.py'  # errors: []
```

## Benchmarks

Duval is linear and needs no benchmark. `lyndonArray` on `07`'s prefix-doubling suffix array (`doubling`) against `12` SA-IS (`sais`), identical outputs checked, ad-hoc driver under `_00_memory_cap.py` (peak 180 MB), i9-11900H, `g++` 16.2 `-std=gnu++20 -O2`, seed 1, median of 7 runs after one warm-up check run:

| n | input | `doubling` ms | `sais` ms | speedup |
|---|---|---|---|---|
| 10^6 | random 256 bytes | 208.6 | 86.0 | 2.4x |
| 10^6 | unary | 176.0 | 19.4 | 9.1x |
| 10^6 | random binary | 154.6 | 79.7 | 1.9x |
| 10^6 | random `int` in [0, 10^9) | 286.0 | 216.1 | 1.3x |
| 4·10^6 | random 256 bytes | 1208.1 | 432.3 | 2.8x |
| 4·10^6 | unary | 925.0 | 87.2 | 10.6x |
| 4·10^6 | random binary | 1016.6 | 401.3 | 2.5x |
| 4·10^6 | random `int` in [0, 10^9) | 1604.9 | 1134.9 | 1.4x |

## Sources

Code is independently implemented from the invariants above.

- [cp-algorithms, Lyndon factorization](https://cp-algorithms.com/string/lyndon_factorization.html) (fetched 2026-10-10): Duval's algorithm and its linear bound.
- [OI Wiki, Lyndon 分解](https://oi-wiki.org/string/lyndon/) (fetched 2026-10-10): Duval, minimal rotation application.
- [maspypy `lyndon.hpp`](https://maspypy.github.io/library/string/lyndon.hpp) (fetched 2026-10-10): incremental Lyndon factorization (`add`, per-prefix minimal suffix, factorize); [`lex_min_suffix_for_all_prefix.hpp`](https://maspypy.github.io/library/string/lex_min_suffix_for_all_prefix.hpp) is its projection.
- [hitonanode `lyndon.hpp`](https://hitonanode.github.io/cplib-cpp/string/lyndon.hpp) (fetched 2026-10-10): generic Duval, longest Lyndon prefixes, Lyndon-word enumeration up to length `n`.
- [Library Checker `lyndon_factorization`](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/lyndon_factorization/task.md) (fetched 2026-10-10): cut-vector output format.
- Catalog sweep 2026-10-10 in [00-sources.md](00-sources.md); adopted: `IncrementalLyndon`, `suffixFactorization`, `nextLyndonWord`.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): Lyndon-word rank/unrank and k-th word (exponential counts need big integers; no competitive-programming catalog has them).
- Runs (`26`) can consume `lyndonArray`/`lyndonTree` under both orders; de Bruijn sequences (`41`) can consume `nextLyndonWord`; per-prefix lexicographically minimal suffixes (`22`) are `IncrementalLyndon::minSuffix`.

## History

- 2026-10-10: implemented and verified (P083), independent review found 4 issues, fixed.
- 2026-10-12: `lyndonArray` moved from prefix doubling to `12` SA-IS (O(n) on bytes), byte-order, non-byte and large suffix-array checks added, the unused `07-suffixarray.hpp` include dropped; full suite passed on g++ and g++-14.
- 2026-10-19: P017, the large oracle is now `SuffixArray<int>::doubling` (the `07` constructors call SA-IS); full suite passed on g++ and g++-14.
