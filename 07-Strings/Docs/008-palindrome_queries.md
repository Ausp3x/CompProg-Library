# 008-palindrome_queries.hpp — Evidence

`008-palindrome_queries.hpp` (batch ST03, package P017) provides five small adapters over the verified P016 `Manacher` and `StringHash` types. They keep no input reference or extra state and take existing preprocessing by const reference. Callers preserve the underlying objects' invariants; assign a new valid value before querying a moved-from object.

## Contracts

### isPalindrome, maybePalindrome, longestPalindrome, countPalindromes, countPalindromesByEndpoint

- `isPalindrome(m,l,r)` is an exact O(1) predicate for `[l,r)`, with `0 <= l <= r <= m.size()`. Empty intervals, the final gap included, are palindromes. With `len=r-l` the center is `l+len/2`; odd intervals need `m.odd[center] >= len/2+1`, since the radius includes the center, and even intervals need `m.even[center] >= len/2`. `oddInterval` and `evenInterval` perform the inverse conversion, smaller valid radii included.
- `maybePalindrome(h,l,r)` compares the forward and reverse fingerprints of `[l,r)`. False proves the interval is not a palindrome; true is a **Monte Carlo** answer. All three kinds are supported (`StringHash<>` double prime, `StringHash64` unsigned word, `StringHash61` Mersenne field), with their existing byte and integer encoding, size and base constraints. Empty intervals return true. No collision guarantee is added: the [hash contract](003-stringhash.md) applies, fixed-base limitations and adversarial unsigned-word collisions included.
- `countPalindromes(m)` returns the number of nonempty palindromic substrings counted by position (pairs `l < r` with `[l,r)` a palindrome), the sum of all `odd` and `even` radii, as `lng` (at most `n * (n+1) / 2`), in O(n) time and O(1) space; empty input gives 0.
- `countPalindromesByEndpoint(m)` returns `{starting, ending}`, two arrays of `n` counts: `starting[i]` is the number of palindromic substrings `[i, r)`, and `ending[i]` the number of palindromic substrings `[l, i + 1)` (indexed by last character, as `Manacher::longestEnding`). Each entry is at most `n`, so `int` suffices, and both arrays sum to `countPalindromes(m)`. O(n) time and returned storage; empty input gives two empty arrays. The standard use counts pairs of crossing or disjoint palindromes (Codeforces 17E Palisection, 159D Palindrome pairs) from prefix sums of `ending` against `starting`.
- `longestPalindrome(m)` returns the half-open interval of the longest palindrome in the whole preprocessed sequence, ties broken by smallest start; empty input returns `{0,0}`. It scans the maximal odd and even intervals in O(n) time and constant auxiliary space. Extract byte contents with `auto [l,r] = longestPalindrome(m); auto result = s.substr(l,r-l);`, or `[s.begin()+l,s.begin()+r)` for a generic sequence; the original sequence is needed only for that. Returning a witness interval avoids an allocation and any dependence on the source container; repeated callers can cache it.

The exact queries (`isPalindrome`, `longestPalindrome`, `countPalindromes`, `countPalindromesByEndpoint`) require radii built from an equality relation, not the involution form of `Manacher(n, match)`, and accept every byte or equality-comparable integer alphabet Manacher supports, with length below `INT_MAX`. Hash queries inherit `StringHash`'s integer encoding requirements. Manacher and hash preprocessing are each O(n) time and space; every adapter uses O(1) additional storage.

Correctness: palindromes with a fixed center and parity form a chain under deletion of their two outer symbols, so chain membership is exactly the radius inequality. Every nonempty palindrome has one character center or one interior gap and cannot exceed its center's maximal interval, so scanning those maxima finds a global optimum. A strict length comparison in the left-to-right scan enforces the leftmost tie rule, because equal-length palindromes share parity and their starts increase with the center. Each palindrome is counted once by its center and radius, so the radius sum is the palindromic substring count. The odd chain at center `i` with radius `k` holds `[i - j, i + j + 1)` for `j < k`, whose starts fill `[i - k + 1, i]` and last indices `[i, i + k - 1]`; the even chain at gap `i` holds `[i - j, i + j)` for `1 <= j <= k`, with starts `[i - k, i - 1]` and last indices `[i, i + k - 1]`. Adding one over each contiguous range in a difference array and taking prefix sums counts every palindrome once at its start and once at its end; a radius never passes either end of the sequence, so every index stays within the `n + 1` entries. The final gap contributes only an empty interval and is not scanned for the maximum. Differences and `l+len/2` avoid overflowing endpoint sums, and the existing interval reconstruction avoids forming an overflowing `2*odd_radius`.

## Feature-to-Test Map

Runnable entry: [008-palindrome_queries_tester.py](<../../96-Local Testing/07-Strings/008-palindrome_queries_tester.py>). Quick, full and stress exhaust ternary lengths through 5/7/9, add 80/500/3000 random cases and use large lengths 5000/200000/1000000.

| Feature | Independent coverage |
|---|---|
| isPalindrome: exact predicate, empty and odd/even intervals, index/radius conversion | Every interval of every ternary string through length 7 against direct symmetric scans independent of Manacher's recurrence; endpoints and both parity branches. |
| longestPalindrome: witness and leftmost tie rule | Brute-force interval enumeration picks the expected optimum; `babad` and `abbacddc` ties, default empty and singleton. |
| countPalindromes | Brute-force count of palindromic intervals in every exhaustive and random case; `babad` = 7, empty = 0, unary `n * (n+1) / 2` and the alternating closed form at 5000/200000/1000000. |
| countPalindromesByEndpoint: counts by start and last index | Brute-force interval enumeration tallies both arrays in every exhaustive and random case; `babad` `{2,2,1,1,1}`/`{1,1,2,2,1}`, empty input, unary `n - i`/`i + 1` and alternating `(n - i + 1) / 2`/`i / 2 + 1` at 5000/200000/1000000. A mutant shortening the last-index ranges fails the quick suite (2026-10-19). |
| maybePalindrome: all three hash kinds | Exhaustive and seeded interval comparisons with direct scans; empty and default objects, all bytes including NUL and 255, the prime maximum symbol and full-width unsigned symbols. The sampled oracle requires true on every palindrome (no false negative) and exact agreement from the double-prime and Mersenne kinds; a `StringHash64` true on a non-palindrome is a Monte Carlo false positive, counted in the PASS line and never a failure (user decision 2026-11-10, after seed 42 stress hit 2 on that word hash). Finite agreement is never read as collision freedom. |
| Collision limitation | A nonpalindromic length-2048 Thue–Morse word is accepted by unsigned-word hashing for ten seeded odd bases, while direct scanning and the exact predicate reject it: with eleven construction levels, reversal complements every bit and the difference polynomial is divisible by `2^64`. |
| Generic alphabet and value semantics | 500 seeded byte cases and matching signed 64-bit extreme alphabets; source mutation, copied and moved destinations, reassignment of moved-from values. |
| Scale and bounds | 200000-symbol unary and alternating inputs with independently known longest answers and every suffix query; six checked negative, reversed and out-of-bounds range probes. |

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites), this suite's results. The full and stress rows ran after the `StringHash64` false-positive fix (user decision 2026-11-10), the full rows with a warm cache; the quick row ran cold before it.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.2 + 1.3 s, checked 0.2 + 1.7 s, ASan-UBSan 0.5 + 52.9 s. Peak 559 MB; pre-flight quick peak 49 MB, runs optimized 0.3 s, checked 4.0 s. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.1 + 0.8 s, checked 13.7 + 1.7 s, ASan-UBSan 15.1 + 19.3 s. Peak 927 MB; pre-flight quick peak 787 MB, runs optimized 0.1 s, checked 2.1 s. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 26.4 + 0.3 s, checked 18.2 + 4.4 s. Peak 722 MB. |
| `python3 '96-Local Testing/07-Strings/008-palindrome_queries_tester.py' --mode stress --seed 42`, alone | PASS with 2 false positives counted. Optimized 0.0 + 1.6 s, checked 3.7 + 2.9 s, ASan-UBSan 4.6 + 68.0 s. Peak 928 MB, pre-flight quick 737 MB. |

Tester runs 2026-10-19 (P017 re-sweep, `countPalindromesByEndpoint`), the latest with check counts:

```bash
python3 '96-Local Testing/07-Strings/008-palindrome_queries_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, 4,281 cases and 5,346,852 checks each, 6 assertion probes, MEMORY peak=552MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/008-palindrome_queries_tester.py' --mode full --seed 20261019  # PASS, 3 configurations, same counts, MEMORY peak=555MB
python3 '96-Local Testing/07-Strings/008-palindrome_queries_tester.py' --mode stress --seed 7  # PASS, 3 configurations, 35,525 cases and 32,858,544 checks each, MEMORY peak=865MB
```

Integration and consistency for that run are recorded in [007-suffixarray.md](007-suffixarray.md#commands-and-results).

## Benchmarks

No benchmark: these are direct adapters with no competing specialization or dispatch threshold. The hash benchmark in [003-stringhash.md](003-stringhash.md) covers the underlying representation; it is not a claim that hashing beats exact Manacher queries.

## Sources

Inspected on 2026-09-28 from the cached source copies retrieved for P016; the adapters are independently written, with no external code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html), page update August 7, 2026: statement, centered palindrome chains, odd and even radius definitions and parity conversions; the two parity arrays avoid transformed-string sentinels. `countPalindromes` is derived there from the radii; the planned Python row `_010_strings.py` lists it, and no C++ row owned it.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: the two parity arrays and radius convention. Its odd radius excludes the center; the repository's Manacher contract includes it.
- [Luogu CF17E](https://www.luogu.com.cn/problem/CF17E) (statement), [ouuan, CF17E Palisection](https://ouuan.github.io/post/cf17e-palisectionmanacher/) and [jkloverdcoi, CF17E Palisection](https://jkloverdcoi.github.io/2019/08/29/CF17E-Palisection/) (read 2026-10-19): Manacher radii, range additions per center by difference arrays, then prefix sums for the palindromes starting and ending at each position; adopted as `countPalindromesByEndpoint`, since no other row owned per-position counts.
- The [P016 hash proof and collision contract](003-stringhash.md) supplies fingerprint encoding, reversal and error assumptions; this package reuses that implementation and claims no new external hash-source review.

Targeted searches of preserved `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` found no palindrome-query or Manacher implementation to migrate.

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- Hash palindrome answers stay Monte Carlo: the suite demonstrates a known unsigned-word false positive that exact Manacher rejects.
- The eertree (ST07), arbitrary-subrange longest and count palindrome queries and other advanced static queries (ST16) and dynamic edits (ST28) have their own planned owners.

## Aliases

Search aliases (P263, grab --find): Manacher; 回文判断, 回文串判定, 回文子串计数, 最长回文子串.

## History

- 2026-09-28: verified under the previous system (P017).
- 2026-10-08: re-audit baseline, full suite seed 1 passed before any change.
- 2026-10-08: re-audit, `countPalindromes` added, all three hash kinds tested, 1 finding fixed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-19: P017 re-sweep, `countPalindromesByEndpoint` added; full passed on g++ and g++-14, stress passed.
- 2026-11-07: P263 added Chinese search aliases: OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full inputs unchanged; the stress round failed at seed 42 on a `StringHash64` false positive (also on `2346cad`), then the sampled oracle counted them (user decision 2026-11-10) and every run above passed.
