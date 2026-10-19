# 05-manacher.hpp — evidence

`05-manacher.hpp` (batch ST02, package P016) computes sentinel-free palindrome radii for every character center and every gap, reconstructs maximal and nested intervals, and derives the longest palindrome ending and starting at every position. Contest profile, GNU C++20, no ISA code.

## Contracts

### Manacher

`Manacher(s)` takes any sequence with random access, `.size()` and equality, of length below `INT_MAX` (`string`, `string_view`, integer vectors; pass literals as `string_view`). It stores no reference to the input. `Manacher(n, match)` computes the same radii from a predicate `match(i, j)`, called only with `0 <= i < j < n`, with O(n) calls. The radii are exact when `match(i, j)` equals `key(i) == key(j)` for some key (any custom equality). For an involution relation `match(i, j) = (s[j] == f(s[i]))` such as complement matching, only `even` is exact: an odd palindrome's center would have to satisfy `f(c) = c`, which the reflection argument needs and the algorithm never checks, so `odd`, `centerEnd` of character centers and the per-position arrays are then unspecified. The default constructor represents the empty sequence.

`odd[i]` includes its center and denotes `[i-odd[i]+1, i+odd[i])`; `even[g]` denotes `[g-even[g], g+even[g])` around gap `g`, with `n + 1` gap entries including zero radii at both ends. A singleton has `odd = {1}, even = {0, 0}`; empty input has `odd = {}, even = {0}`. Fields are read-only after construction; assign a moved-from object before querying it.

`oddInterval(i, k = -1)` and `evenInterval(g, k = -1)` return half-open ranges; `k = -1` selects the maximum, an explicit radius must lie in `[1, odd[i]]` or `[0, even[g]]` (asserted). Empty even intervals are `[g, g)`, including `[0, 0)` for empty input; this is the interval convention, not an absence sentinel. Each costs O(1).

`centerEnd(c)` for a doubled center `c` in `[0, 2n-2]` (asserted) (even `c` is character `c/2`, odd `c` is gap `(c+1)/2`) returns the inclusive right end of its maximal palindrome; an empty even palindrome gives `c/2`, below its own start. `longestEnding()[r]` and `longestStarting()[l]` are the lengths of the longest palindromes ending at `r` and starting at `l`, O(n) time and returned storage; empty input gives `[]`. These three methods compute doubled centers up to `2n` in `int`, so they require `n < 2^30` (asserted at entry); the constructor, the radii and the interval methods keep the full `n < INT_MAX` domain.

Construction takes O(n) time and stored space.

Correctness: for each parity the loop keeps the rightmost maximal palindrome found so far; a center inside it starts from its reflected radius clipped at the boundary, and every further successful comparison advances the boundary, so total extension work is O(n). Mirror indices `l + (r - i)` avoid overflowing sums of absolute indices. For `longestEnding`, an end `r` is covered by doubled center `c` iff `c <= 2r` and `centerEnd(c) >= r`, with start `c - r`; the smallest such `c` gives the longest palindrome. Sweeping `c` upward with a pointer at the first unassigned end assigns each end once: every end `e < c/2` was already covered by its own character center `2e`, so the pointer never lags behind the current center. `longestStarting` is the mirrored sweep with `c` downward. Both are O(n).

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `oddInclusive(i, k)` over inclusive `[l, r]` | `oddInterval(i, k)`, `r - 1` |
| `evenInclusive(g, k)` over inclusive `[l, r]` | `evenInterval(g, k)`, `r - 1` |

## Feature-to-test map

Runner: [`05-manacher_tester.py`](<../../96-Local Testing/07-Strings/05-manacher_tester.py>). The oracle enumerates every interval and tests it directly (no mirrored recurrence).

| Operation | Coverage |
|---|---|
| Manacher(s): odd, even, size | Exhaustive ternary strings through length 6/8/9, 300/2000/15000 random byte and extreme-`lng` cases (some forced palindromic), all 256 bytes, unary and alternating lengths 5000/300000/1000000, copy/move/reset/assignment. |
| Manacher(n, match) | Equality predicate compared array-for-array with the sequence constructor on every case; complement involution `s[j] == (s[i] ^ 1)` checked on even radii against the direct anti-palindrome enumeration. |
| oddInterval, evenInterval | Maximal and every nested radius on every case, empty and endpoint gaps, default empty object. |
| centerEnd, longestEnding, longestStarting | Every doubled center: `[c - e, e]` is a palindrome and one step wider is not or leaves the string (direct check, not the index formula); per-position maxima of the enumerated palindromes on every case; closed forms on the large unary and alternating strings; empty object. |

Checked builds run twelve precondition probes: ten center/radius probes on the interval methods and `centerEnd` below and above its range.

## Commands and results

P016 prune follow-up run, 2026-10-13 (after the `n < 2^30` assertions): GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H. Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>).

```bash
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode quick --seed 20261013
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '07-Strings/05-manacher.hpp' '96-Local Testing/07-Strings/05-manacher_tester.cpp'
```

Full, seed 20261013: PASS in all three configurations with all 12 assertion probes, 13,842 cases and 4,604,043 checks per configuration; 26 s and peak 391 MB on g++, 26 s and peak 414 MB on g++-14 (each overlapped an integration run, so times are indicative). The quick run of `08-palindrome_queries`, the only header that includes this one, passed both configurations with 719,553 checks each and 6 assertion probes. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations.

## Benchmarks

Linear algorithm with no backend choice; no benchmark is required and no speed claim is made.

## Sources

Code is independently implemented.

- [cp-algorithms, Manacher's Algorithm](https://cp-algorithms.com/string/manacher.html): odd/even definitions, reflection clipping, linear proof; palindromic prefix/suffix applications.
- [KACTL Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h): compact two-parity form; KACTL's odd half-length excludes the center, this API includes it.
- [maspypy `string/manacher.hpp`](https://github.com/maspypy/library/blob/main/string/manacher.hpp) (fetched 2026-10-08): predicate form `manacher(n, match)`.
- [Nyaan `string/manacher.hpp`](https://github.com/NyaanNyaan/library/blob/master/string/manacher.hpp) (fetched 2026-10-08): `enumerate_leftmost_palindromes`, the per-end longest palindrome.
- Catalog sweep 2026-10-08 in [00-sources.md](00-sources.md). No legacy Manacher implementation exists in `OLD`.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): the Library Checker `2n - 1` combined length array (a radius/index conversion), a bulk `enumeratePalindromes` (a loop over the interval methods), the odd-only switch.
- Exact substring palindrome checks and longest-palindrome extraction stay in `08`; eertree in `13`; inverse Manacher in `31`.
- The `n < 2^30` assertions of `centerEnd`, `longestEnding` and `longestStarting` have no probe: an object that violates them holds at least 8 GB of radii, above the 4 GB test cap. Their placement rests on the bound above: with `n < 2^30` every doubled center, loop bound and length (`2n - 2`, `2r - c + 1`, `c - 2l + 1`) stays below `2^31`.

## History

- 2026-09-28: verified under the previous system (P016).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, predicate constructor, `centerEnd`, `longestEnding` and `longestStarting` added, 2 findings and 1 review note fixed, contracts moved out of the header; quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `oddInclusive` and `evenInclusive` (cases covered by the half-open interval checks), review found 1 defect, fixed; full suite passed on g++ and g++-14.
