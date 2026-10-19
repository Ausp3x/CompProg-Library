# 08-palindrome_queries.hpp — evidence

`08-palindrome_queries.hpp` (batch ST03, package P017) provides five small adapters over the verified P016 `Manacher` and `StringHash` types. They retain no input reference or extra state, and take existing preprocessing by const reference. Callers preserve the underlying objects' invariants; assign a new valid value before querying a moved-from object.

## Contracts

### isPalindrome, maybePalindrome, longestPalindrome, countPalindromes, countPalindromesByEndpoint

- `isPalindrome(m,l,r)` is an exact constant-time predicate for `[l,r)`, with `0 <= l <= r <= m.size()`. Empty intervals, including the final gap, are palindromes. With `len=r-l`, the center is `l+len/2`. Odd intervals need `m.odd[center] >= len/2+1`, because the radius includes the center; even intervals need `m.even[center] >= len/2`. The existing `oddInterval`/`evenInterval` methods perform the inverse conversion, including smaller valid radii.
- `maybePalindrome(h,l,r)` compares the forward and reverse fingerprints of `[l,r)`. False proves the interval is not a palindrome; true is a **Monte Carlo** answer. All three kinds are supported (`StringHash<>` double prime, `StringHash64` unsigned word, `StringHash61` Mersenne field), with their existing byte/integer encoding, size and base constraints. Empty intervals return true. No additional collision guarantee is introduced: the [hash contract](03-stringhash.md) applies, including fixed-base limitations and adversarial unsigned-word collisions.
- `countPalindromes(m)` returns the number of nonempty palindromic substrings counted by position (pairs `l < r` with `[l,r)` a palindrome), the sum of all `odd` and `even` radii, as `lng` (at most `n * (n+1) / 2`). `O(n)` time, `O(1)` space; empty input gives 0.
- `countPalindromesByEndpoint(m)` returns `{starting, ending}`, two arrays of `n` counts: `starting[i]` is the number of palindromic substrings `[i, r)` and `ending[i]` the number of palindromic substrings `[l, i + 1)` (indexed by last character, as `Manacher::longestEnding`). Each entry is at most `n`, so `int` suffices, and both arrays sum to `countPalindromes(m)`. `O(n)` time and `O(n)` returned storage; empty input gives two empty arrays. The standard use is counting pairs of crossing or disjoint palindromes (Codeforces 17E Palisection, 159D Palindrome pairs) from prefix sums of `ending` against `starting`.
- `longestPalindrome(m)` returns the half-open interval of the longest palindrome in the whole preprocessed sequence, breaking ties by smallest start index. Empty input returns `{0,0}`. It scans maximum odd/even intervals in `O(n)` time and constant auxiliary space. Extract byte contents with `auto [l,r] = longestPalindrome(m); auto result = s.substr(l,r-l);`, or use `[s.begin()+l,s.begin()+r)` for a generic sequence. The original sequence is needed only to extract contents.

Exact queries (`isPalindrome`, `longestPalindrome`, `countPalindromes`, `countPalindromesByEndpoint`) require radii built from an equality relation (not the involution form of `Manacher(n, match)`) and accept every byte or equality-comparable integer alphabet already supported by Manacher, with length below `INT_MAX`. Hash queries inherit `StringHash`'s integer encoding requirements. Manacher preprocessing is `O(n)` time/space; hash preprocessing is `O(n)` time/space. Every adapter uses `O(1)` additional storage. Longest extraction is a witness interval, so it avoids an unnecessary allocation or dependence on the source container type. Repeated longest queries can cache that returned interval at the call site.

Correctness: palindromes with a fixed center and parity form a chain under deletion of their two outer symbols. Therefore membership in that chain is precisely the corresponding radius inequality. Every nonempty palindrome has one character center or one interior gap, and cannot exceed its center's maximum interval. Scanning these maxima therefore contains a global optimum; a strict length comparison during the left-to-right scan enforces the leftmost tie rule, because equal-length palindromes have the same parity and their starts increase with the center. Each palindrome is counted once by its center and radius, so the radius sum is the palindromic substring count. The odd chain at center `i` with radius `k` holds the palindromes `[i - j, i + j + 1)` for `j < k`, whose starts fill `[i - k + 1, i]` and whose last indices fill `[i, i + k - 1]`; the even chain at gap `i` holds `[i - j, i + j)` for `1 <= j <= k`, with starts `[i - k, i - 1]` and last indices `[i, i + k - 1]`. Adding one over each of these contiguous ranges in a difference array and taking prefix sums therefore counts every palindrome once at its start and once at its end; a radius never passes either end of the sequence, so every index stays within the `n + 1` entries. The final gap can only contribute an empty interval and need not be scanned for the maximum. Differences and `l+len/2` avoid overflowing absolute endpoint sums, and existing interval reconstruction avoids forming an overflowing `2*odd_radius`.

## Feature-to-test map

| Feature | Independent coverage |
|---|---|
| isPalindrome: exact predicate, empty and odd/even intervals, index/radius conversion | Every interval of every ternary string through length 7; direct symmetric scans independent of Manacher's recurrence; endpoints and both parity branches. |
| longestPalindrome: witness and leftmost tie rule | Brute-force interval enumeration picks the expected optimum; explicit `babad` and `abbacddc` ties, default empty and singleton. |
| countPalindromes | Brute-force count of palindromic intervals in every exhaustive and random case; `babad` = 7, empty = 0, unary `n * (n+1) / 2` and alternating closed form at 5000/200000/1000000. |
| countPalindromesByEndpoint: counts by start and last index | Brute-force interval enumeration tallies both arrays in every exhaustive and random case; `babad` `{2,2,1,1,1}`/`{1,1,2,2,1}`, empty input, unary `n - i`/`i + 1` and alternating `(n - i + 1) / 2`/`i / 2 + 1` at 5000/200000/1000000. A mutation shortening the last-index ranges fails the quick suite (2026-10-19). |
| maybePalindrome: all three hash kinds | Exhaustive and seeded interval comparisons with direct scans; empty/default objects, all bytes including NUL/255, prime maximum symbol and full-width unsigned symbols. Finite agreement is never interpreted as collision freedom. |
| Collision limitation | A nonpalindromic length-2048 Thue–Morse word is accepted by unsigned-word hashing for ten seeded odd bases; direct scanning and the exact predicate reject it. With eleven construction levels, reversal complements every bit and the difference polynomial is divisible by `2^64`. |
| Generic alphabet and value semantics | 500 seeded byte cases and corresponding signed 64-bit extreme alphabets; source mutation, copied/moved destinations and reassignment of moved-from values. |
| Scale and bounds | 200000-symbol unary/alternating inputs, independently known longest answers and every suffix query; six checked negative/reversed/out-of-bounds range probes. |

The runnable entry is [08-palindrome_queries_tester.py](<../../96-Local Testing/07-Strings/08-palindrome_queries_tester.py>). Quick/full/stress exhaust ternary lengths through 5/7/9, add 80/500/3000 random cases, and use large lengths 5000/200000/1000000.

## Commands and results

Latest run 2026-10-19 (P017 re-sweep, `countPalindromesByEndpoint`). Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>).

```bash
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, 4,281 cases and 5,346,852 checks each, 6 assertion probes, MEMORY peak=552MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, same counts, MEMORY peak=555MB
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode stress --seed 7  # PASS, 3 configurations, 35,525 cases and 32,858,544 checks each, MEMORY peak=865MB
```

Integration and consistency are recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results).

## Benchmarks

No benchmark: these are direct adapters with no competing specialization or dispatch threshold. The hash benchmark in [03-stringhash.md](03-stringhash.md) covers the underlying representation, not a claim that hashing beats exact Manacher queries.

## Sources

Inspected on 2026-09-28 using the cached source copies retrieved for P016; these adapters are independently written, with no external code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html), page update August 7, 2026: statement, centered palindrome chains, direct odd/even radius definitions and parity conversions. The two existing parity arrays avoid transformed-string sentinels. `countPalindromes` is derived there from the radii; the planned Python row `_10_strings.py` lists it and no C++ row owned it.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: inspected the two parity arrays and radius convention. Its odd radius excludes the center; the repository's canonical Manacher contract includes it.
- [Luogu CF17E](https://www.luogu.com.cn/problem/CF17E) (statement), [ouuan, CF17E Palisection](https://ouuan.github.io/post/cf17e-palisectionmanacher/) and [jkloverdcoi, CF17E Palisection](https://jkloverdcoi.github.io/2019/08/29/CF17E-Palisection/) (read 2026-10-19): Manacher radii, range additions per center by difference arrays, then prefix sums for the palindromes starting and ending at each position; adopted as `countPalindromesByEndpoint`, since no other row owned per-position counts.
- The existing [P016 hash proof and collision contract](03-stringhash.md) supplies fingerprint encoding, reversal and error assumptions. This package reuses that implementation and does not claim a new external hash-source review.

Targeted searches of preserved `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` found no palindrome-query or Manacher implementation to migrate.

## Limits and handoffs

Hash palindrome answers remain Monte Carlo: the suite demonstrates a known unsigned-word false positive while exact Manacher rejects it. Eertree/partitioning (ST07), arbitrary-subrange longest/count palindrome queries and other advanced static queries (ST16) and dynamic edits (ST28) are separate planned owners, not functionality of these adapters.

## History

- 2026-09-28: verified under the previous system (P017).
- 2026-10-08: re-audit baseline, full suite seed 1 passed before any change.
- 2026-10-08: re-audit, `countPalindromes` added, all three hash kinds tested, 1 finding fixed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-19: P017 re-sweep, `countPalindromesByEndpoint` added; full passed on g++ and g++-14, stress passed.
