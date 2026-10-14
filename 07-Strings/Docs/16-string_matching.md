# 16-string_matching.hpp — evidence

`16-string_matching.hpp` (batch ST08, package P084) collects the exact single-pattern matchers: the naive scan, Rabin–Karp with verified hits, Boyer–Moore (strong good suffix plus the Galil rule) with its two tables, Horspool, Sunday (Quick Search), Crochemore–Perrin two-way, the prefix-automaton DFA adapter, and the dispatching queries `firstOccurrence`, `isSubstring` and `countOccurrences`. Contest profile, GNU C++20, no ISA code. KMP, Z, hashing and multi-pattern matching stay in `01`, `02`, `03` and `06`.

## Contracts

Every search takes `(pattern, text)` as random-access sequences with `.size() < INT_MAX` (asserted) and returns the starts of all occurrences ascending, overlaps included; the empty pattern occurs at every position `0..n`, and a pattern longer than the text has no occurrence. Pattern and text may be different sequence types (`string`, `string_view`, `vector`); symbols are compared with `==`. Inputs are not retained or mutated.

Coded algorithms (`badCharacterTable`, `boyerMoore`, `horspool`, `sunday`, `dfaSearch`) index a table by symbol: `char` symbols are read as unsigned bytes (`S = 256` default) and integral symbols must lie in `[0, S)`, checked once at entry for pattern and text (`O(n + m)` assertion, outside the loops). With `NDEBUG` an out-of-range symbol reads outside the table (undefined, but no loop or allocation depends on it).

### naiveSearch

Window-by-window comparison, `O(n * m)` time, `O(out)` memory. Equality-comparable symbols.

### rabinKarp(pattern, text, seed = 20261010)

Rolling polynomial hash modulo `2^61 - 1` (the `StringHash61` arithmetic of `03-stringhash.hpp`) with base drawn from `std::mt19937_64(seed)` in `[256, 2^61 - 257)`; symbols are integral or `char` (bytes unsigned), encoded as their value reduced modulo `2^61 - 1`. Every hash hit is verified symbol by symbol, so the output is exact (Las Vegas: only the running time is random). Time `O(n + m)` plus `O(m)` per hit; with a fixed seed an adversary can force collisions, and true occurrences always cost `O(m)` each, so the worst case is `O(n * m)` (for example `a^m` in `a^n`). Callers facing hacks pass a random seed. The verification path is tested with symbols that are equal modulo `2^61 - 1` (every window collides).

### badCharacterTable(pattern, S = 256, len = -1)

`res[c]` is the largest `i < len` with `pattern[i] = c`, or `-1`; `len = -1` means `m`, otherwise `0 <= len <= m` (asserted). `O(m + S)`. Boyer–Moore and Sunday use `len = m`, Horspool `len = m - 1`.

### goodSuffixTable(pattern)

Strong good-suffix shifts: for `j` in `[0, m)`, `res[j]` is the smallest `s` in `[1, m]` such that `pattern[k - s] = pattern[k]` for all `k` in `(j, m)` with `k >= s`, and `pattern[j - s] != pattern[j]` when `j >= s` (the shift after a mismatch at `j` with `pattern[j + 1, m)` matched). `res[0]` is the smallest period of the pattern (the full-match shift). Built in `O(m)` from the Z-function of the reversed pattern (`suff[i] = z[m - 1 - i]` is the longest common suffix of `pattern[0, i]` and `pattern`), following Lecroq's two-pass construction. Empty pattern gives an empty table.

### boyerMoore(pattern, text, S = 256)

Right-to-left comparison with shift `max(goodSuffix[j], j - last[text[i + j]])` after a mismatch at `j`. After a full match the window moves by the period `p = goodSuffix[0]` and the comparison stops at `m - p` (Galil rule: the first `m - p` symbols of the new window equal `pattern[p, m)`, which equals `pattern[0, m - p)`). With the strong good-suffix rule and Galil's rule the number of comparisons is `O(n + m)` for all occurrences (Galil 1979); tables cost `O(m + S)`. The unary text with a unary pattern (`n = 10^6`, `m = 20000`) is in the test suite; without the Galil rule it needs about `n * m = 2 * 10^10` comparisons and times out.

### horspool, sunday

Horspool compares the last window symbol, then the prefix, and shifts by `m - 1 - last_{m-1}[text[i + m - 1]]` (in `[1, m]`). Sunday (Quick Search) compares the window left to right and shifts by `m - last_m[text[i + m]]` (in `[1, m + 1]`), stopping at the last window. Both are `O(n * m)` in the worst case (`a^m` in `a^n`, or the benchmark's period-2 text for Sunday) and sublinear when shifts are long; tables `O(S)`.

### twoWay

Crochemore–Perrin: the critical factorization is the later of the two maximal suffixes under `<` and its reverse (Lecroq's `maxSuf`/`maxSufTilde`), with local period `per`. If `pattern[0, ell] = pattern[per, per + ell]` the pattern is periodic and a memory of `m - per` matched symbols skips re-comparison after each match; otherwise the shift after a right-part match is `max(ell + 1, m - ell - 1) + 1`. `O(n + m)` time, `O(1)` workspace. Symbols must be totally ordered by `<` with `==` consistent (bytes unsigned); the order only affects the factorization, never the result.

### dfaSearch(aut, text), dfaSearch(pattern, text, S = 256)

The first overload runs `aut = prefixAutomaton(pattern, S)` (from `01`, `m + 1` rows of `S` transitions, asserted non-empty) over the text in `O(n)` and reports `i + 1 - m` when state `m` is reached; an empty pattern (one row) reports `0..n`. The second codes the pattern (bytes unsigned) and builds the automaton: `O((m + 1) * S + n)` time and memory.

### firstOccurrence, isSubstring, countOccurrences

The dispatcher uses `boyerMoore` when both pattern and text have `char` symbols and `twoWay` otherwise (which therefore requires ordered symbols); both are `O(n + m)` worst case. The choice is measured (Benchmarks). `firstOccurrence` returns the smallest start or `-1` and stops at the first hit; `isSubstring` is `firstOccurrence >= 0`. `countOccurrences(pattern, text, overlapping = true)` counts all occurrences as `lng`, or with `overlapping = false` the greedy leftmost set of pairwise disjoint occurrences (start at least the previous start plus `m`), which has maximum size; the empty pattern counts `n + 1` in both modes (Python `str.count` convention). Extra memory is the dispatched matcher's: `O(m + S)` for `char` (Boyer–Moore tables) and `O(1)` on the two-way path.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `allOccurrences(pattern, text)` | `boyerMoore(pattern, text)` when both have `char` symbols, `twoWay(pattern, text)` otherwise (the dispatcher's choice; same results) |

## Feature-to-test map

Runner: [`16-string_matching_tester.py`](<../../96-Local Testing/07-Strings/16-string_matching_tester.py>). The oracle compares every window with `std::equal`; tables use brute definitions. All checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| naiveSearch, rabinKarp, twoWay, firstOccurrence, isSubstring, countOccurrences (both modes) | Every case: exhaustive binary patterns through length 4/5/6 against texts through 7/9/10 and ternary patterns through 3/3/4 against texts through 5/7/6 (quick/full/stress); 2000/8000/20000 random cases (a fifth over 256 bytes, a third periodic, most patterns substrings, some with a foreign last byte) each also as `int`, extreme-`lng` and 5-symbol vectors; `rabinKarp` with the default and a per-case seed; disjoint count against greedy selection over the brute list. |
| rabinKarp verification | Symbols `5` and `5 + (2^61 - 1)` as `lng`: every window collides, only verification separates them. |
| boyerMoore, horspool, sunday, dfaSearch (both overloads) | Every coded case (bytes with `S = 100` exhaustive and `S = 256` random, ints, 5-symbol vectors with `S = 5`), and `boyerMoore` at the default `S = 256` on every byte case; large unary hit/miss, period-3 and random binary texts (`n = 10^5/10^6/10^6`, `m = 2000/20000/20000`) against `kmpOccurrences` (verified `rabinKarp` there only when `hits * m <= 10^8`, its documented worst case). |
| badCharacterTable | Every coded case at `len = m, m - 1, m / 2` and the default against a brute last-occurrence scan. |
| goodSuffixTable | Every coded case against the brute strong good-suffix definition; `res[0]` against `prefixPeriod(prefixFunction(p))`. |
| Linear worst case | Unary hit (`n - m + 1` occurrences) and unary miss with `m = 20000`, `n = 10^6` for boyerMoore, twoWay, dfaSearch and the dispatcher (`countOccurrences`). |

Checked builds run five precondition probes: a pattern symbol and a text symbol outside `[0, S)`, `badCharacterTable` with `len > m`, and a DFA text symbol and pattern symbol outside the alphabet.

## Commands and results

P084 prune run, 2026-10-13: GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H. Full runs build optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, precondition probes) and ASan/UBSan (`-O1`) binaries after the quick pre-flight under the 2048 MB cap; quick runs build optimized and checked; all use `-Wall -Wextra -Wshadow -Wconversion -Werror`.

```bash
python3 '96-Local Testing/07-Strings/16-string_matching_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/07-Strings/16-string_matching_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261013 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '07-Strings/16-string_matching.hpp' '96-Local Testing/07-Strings/16-string_matching_tester.cpp'
```

Full, seed 20261013: PASS in all three configurations with all 5 assertion probes, 4,442,736 checks per configuration; 148 s and peak 642 MB on g++, 137 s and peak 677 MB on g++-14. The quick 07-Strings run passed all 20 suites. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations.

Benchmark: `python3 '96-Local Testing/07-Strings/16-string_matching_benchmark.py' > '96-Local Testing/07-Strings/16-string_matching_benchmark.jsonl'` (every repetition's count verified; table below).

## Benchmarks

Driver: [`16-string_matching_benchmark.py`](<../../96-Local Testing/07-Strings/16-string_matching_benchmark.py>) (raw JSONL beside it, ignored by git). Intel Core i9-11900H, g++ 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed 20261010, `n = 4 * 10^6`, one warmup and five timed repetitions, median in ms; every repetition's occurrence count is checked against `kmpOccurrences`. Random texts are uniform over `S` letters with the pattern copied from the text.

| Workload (bytes) | boyerMoore | horspool | sunday | twoWay | kmpOccurrences | zOccurrences | rabinKarp | string::find | dfaSearch | naiveSearch |
|---|---|---|---|---|---|---|---|---|---|---|
| random S=2, m=4 (250907 hits) | 19.7 | 18.9 | 16.5 | 32.4 | 32.0 | 39.1 | 37.3 | 22.5 | 16.3 | 26.6 |
| random S=2, m=256 | 5.4 | 34.8 | 38.8 | 28.4 | 27.2 | 34.9 | 24.8 | 13.3 | 14.0 | 27.6 |
| random S=4, m=32 | 6.2 | 7.1 | 6.9 | 15.5 | 19.1 | 25.9 | 26.2 | 6.4 | 13.0 | 14.9 |
| random S=26, m=4 | 6.6 | 6.0 | 5.5 | 7.9 | 10.1 | 14.0 | 27.9 | 2.2 | 15.8 | 6.3 |
| random S=26, m=32 | 1.4 | 1.3 | 1.3 | 8.7 | 11.3 | 13.8 | 22.5 | 2.2 | 16.1 | 6.0 |
| random S=256, m=256 | 0.3 | 0.4 | 0.3 | 6.5 | 9.0 | 11.0 | 27.6 | 0.4 | — | 4.7 |
| unary hit, m=256 | 12.3 | 533.7 | 485.3 | 14.5 | 12.2 | 27.1 | 609.0 | 43.1 | 23.1 | — |
| unary miss, m=256 | 24.9 | 25.0 | 276.2 | 6.7 | 9.5 | 16.1 | 25.8 | 45.0 | 13.2 | — |
| period 2, m=256 | 11.1 | 10.0 | 138.9 | 4.6 | 6.8 | 21.2 | 27.0 | 22.5 | 15.9 | — |

| Workload (`vector<int>`) | twoWay | kmpOccurrences | rabinKarp | naiveSearch |
|---|---|---|---|---|
| random S=4, m=4 | 18.7 | 23.0 | 34.7 | 17.7 |
| random S=4, m=64 | 18.0 | 28.9 | 35.9 | 19.0 |
| random S=10^9, m=64 | 6.6 | 9.8 | 32.8 | 4.6 |

The full grid (S in {2, 4, 26, 256}, m in {4, 32, 256}, adversarial at m in {4, 32, 256}) is in the JSONL. Dispatcher decision: among linear-worst-case matchers Boyer–Moore with Galil is fastest on every random byte workload (up to 20x faster than two-way at large alphabets) and stays within 25 ms on the adversarial ones, where Horspool, Sunday and verified Rabin–Karp degrade by 20–50x; two-way is faster on unary-miss and periodic texts but 2–20x slower on random text. For non-byte symbols Boyer–Moore needs a dense alphabet, so the dispatcher uses two-way, which beats KMP by 20–40%. `string::find` (memchr plus compare) wins at `m = 4` over large alphabets but is quadratic (43–45 ms on the unary workloads at `m = 256`, growing with `m`). No threshold is needed: Boyer–Moore is never more than about 1.3x slower than the best linear matcher on random byte text.

The table was measured 2026-10-11. `rabinKarp` was re-timed on 2026-10-13 after switching to the `StringHash61` arithmetic, in an ad-hoc A/B against the previous header (commit 6225705; the A/B driver is not committed): random S = 2/26/256 with m = 4/32/256, unary hit and period 2 at m = 256 (bytes), and random S = 4 and 10^9 at m = 64 (`vector<int>`), `n = 4 * 10^6`, seed 20261010, same machine and flags, each figure the median of six interleaved runs of seven timed repetitions after one warmup, every count checked against `kmpOccurrences`. Its times were 0.89–1.02 times those of the earlier arithmetic, inside run-to-run noise, so the table stands.

## Sources

Code is independently implemented from the cited descriptions.

- [Lecroq, Exact String Matching Algorithms](https://www-igm.univ-eiffel.fr/~lecroq/string/) (fetched 2026-10-10): Brute Force, Automaton (node4), Karp–Rabin with verified hits (node5), Boyer–Moore `suff`/`bmGs`/`bmBc` (node14), Quick Search (node19), Two Way `maxSuf`/`maxSufTilde` (node26).
- [OI Wiki, Boyer–Moore](https://oi-wiki.org/string/bm/) and [字符串匹配](https://oi-wiki.org/string/match/) (fetched 2026-10-10): Galil rule for linear worst case, Horspool/Sunday variants.
- [cp-algorithms, Rabin–Karp](https://cp-algorithms.com/string/rabin-karp.html) (fetched 2026-10-10): rolling hash windows.
- [maspypy `is_substring.hpp`](https://raw.githubusercontent.com/maspypy/library/main/string/is_substring.hpp) and [PyRival `kmp.py`](https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/kmp.py) (fetched 2026-10-10): boolean substring predicate.
- Z. Galil, "On improving the worst case running time of the Boyer–Moore string matching algorithm", CACM 22(9), 1979 (cited for the linear bound; not fetched).
- Catalog sweep in [00-sources.md](00-sources.md#p084-sweeps-2026-10-10).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): simplified bad-character-only Boyer–Moore, the b5s/BMHBNFS hybrid, Turbo-BM and Breslauer's two-way variant (`49`/`43`), last occurrence and range-restricted searches, multi-pattern Rabin–Karp.
- Coded algorithms need symbols in `[0, S)`; callers with sparse integer alphabets compress first or use `twoWay`.

## History

- 2026-10-11: implemented and verified (P084), independent review found 3 issues, fixed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `allOccurrences` (its checks moved to `boyerMoore` at the default `S` and to `twoWay`) and the Rabin–Karp copies of the `StringHash61` arithmetic; full suite passed on g++ and g++-14.
