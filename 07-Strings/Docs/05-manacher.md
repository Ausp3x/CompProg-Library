# 05-manacher.hpp — evidence

`05-manacher.hpp` (batch ST02, package P016) computes sentinel-free palindrome radii for every character center and every gap, reconstructs maximal and nested intervals, and derives the longest palindrome ending and starting at every position. Contest profile, GNU C++20, no ISA code.

## Contracts

### Manacher

`Manacher(s)` takes any sequence with random access, `.size()` and equality, of length below `INT_MAX` (`string`, `string_view`, integer vectors; pass literals as `string_view`). It stores no reference to the input. `Manacher(n, match)` computes the same radii from a predicate `match(i, j)`, called only with `0 <= i < j < n`, with O(n) calls. The radii are exact when `match(i, j)` equals `key(i) == key(j)` for some key (any custom equality). For an involution relation `match(i, j) = (s[j] == f(s[i]))` such as complement matching, only `even` is exact: an odd palindrome's center would have to satisfy `f(c) = c`, which the reflection argument needs and the algorithm never checks, so `odd`, `centerEnd` of character centers and the per-position arrays are then unspecified. The default constructor represents the empty sequence.

`odd[i]` includes its center and denotes `[i-odd[i]+1, i+odd[i])`; `even[g]` denotes `[g-even[g], g+even[g])` around gap `g`, with `n + 1` gap entries including zero radii at both ends. A singleton has `odd = {1}, even = {0, 0}`; empty input has `odd = {}, even = {0}`. Fields are read-only after construction; assign a moved-from object before querying it.

`oddInterval(i, k = -1)` and `evenInterval(g, k = -1)` return half-open ranges and `oddInclusive`/`evenInclusive` inclusive endpoints; `k = -1` selects the maximum, an explicit radius must lie in `[1, odd[i]]` or `[0, even[g]]` (asserted). Empty even intervals are `[g, g)` or inclusive `[g, g-1]`, including `[0, -1]` for empty input; these are interval conventions, not absence sentinels. Each costs O(1).

`centerEnd(c)` for a doubled center `c` in `[0, 2n-2]` (asserted) (even `c` is character `c/2`, odd `c` is gap `(c+1)/2`) returns the inclusive right end of its maximal palindrome; an empty even palindrome gives `c/2`, below its own start. `longestEnding()[r]` and `longestStarting()[l]` are the lengths of the longest palindromes ending at `r` and starting at `l`, O(n) time and returned storage; empty input gives `[]`.

Construction takes O(n) time and stored space.

Correctness: for each parity the loop keeps the rightmost maximal palindrome found so far; a center inside it starts from its reflected radius clipped at the boundary, and every further successful comparison advances the boundary, so total extension work is O(n). Mirror indices `l + (r - i)` avoid overflowing sums of absolute indices. For `longestEnding`, an end `r` is covered by doubled center `c` iff `c <= 2r` and `centerEnd(c) >= r`, with start `c - r`; the smallest such `c` gives the longest palindrome. Sweeping `c` upward with a pointer at the first unassigned end assigns each end once: every end `e < c/2` was already covered by its own character center `2e`, so the pointer never lags behind the current center. `longestStarting` is the mirrored sweep with `c` downward. Both are O(n).

## Feature-to-test map

Runner: [`05-manacher_tester.py`](<../../96-Local Testing/07-Strings/05-manacher_tester.py>). The oracle enumerates every interval and tests it directly (no mirrored recurrence); all checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| Manacher(s): odd, even, size | Exhaustive ternary strings through length 6/8/9, 300/2000/15000 random byte and extreme-`lng` cases (some forced palindromic), all 256 bytes, unary and alternating lengths 5000/300000/1000000, copy/move/reset/assignment. |
| Manacher(n, match) | Equality predicate compared array-for-array with the sequence constructor on every case; complement involution `s[j] == (s[i] ^ 1)` checked on even radii against the direct anti-palindrome enumeration. |
| oddInterval, evenInterval, oddInclusive, evenInclusive | Maximal and every nested radius on every case, empty and endpoint gaps, default empty object. |
| centerEnd, longestEnding, longestStarting | Every doubled center: `[c - e, e]` is a palindrome and one step wider is not or leaves the string (direct check, not the index formula); per-position maxima of the enumerated palindromes on every case; closed forms on the large unary and alternating strings; empty object. |

Checked builds run twelve precondition probes: ten center/radius probes on the interval methods and `centerEnd` below and above its range.

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode quick --seed 1  # PASS, 2 configurations
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 1  # PASS, 3 configurations, 5,344,613 checks each, 12 assertion probes
CXX=g++-14 python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 2  # PASS, 3 configurations, 5,360,299 checks each (`CXX=g++-14`)
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 25,712,584 checks each
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizers
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 9 suites
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The suite was also run in full mode before any change (seed 1, all three configurations PASS) as the re-audit baseline.

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

- Not adopted (reasons in [00-notes.md](00-notes.md)): the Library Checker `2n - 1` combined length array (a radius/index conversion owned by `08-palindrome_queries.hpp`), a bulk `enumeratePalindromes` (a loop over the interval methods), the odd-only switch.
- Exact substring palindrome checks and longest-palindrome extraction stay in `08`; eertree in `13`; inverse Manacher in `31`.
- The size domain is now `n < INT_MAX` (previously `n <= INT_MAX`), matching the folder convention and the predicate constructor's `int n`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-09-28: verified under the previous system (P016). 2026-10-08 re-audit: `/reaudit-review` findings 13 (`U: NA`) and 14 (closing braces) fixed; predicate constructor, `centerEnd` (range asserted after review), `longestEnding` and `longestStarting` added; contracts moved out of the header.
