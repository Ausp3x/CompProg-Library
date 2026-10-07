# 08-palindrome_queries.hpp — evidence

`08-palindrome_queries.hpp` (batch ST03, package P017) provides three small adapters over the verified P016 `Manacher` and `StringHash` types. They retain no input reference or extra state, and take existing preprocessing by const reference. Callers preserve the underlying objects' invariants; assign a new valid value before querying a moved-from object.

## Contracts

### isPalindrome, maybePalindrome, longestPalindrome

- `isPalindrome(m,l,r)` is an exact constant-time predicate for `[l,r)`, with `0 <= l <= r <= m.size()`. Empty intervals, including the final gap, are palindromes. With `len=r-l`, the center is `l+len/2`. Odd intervals need `m.odd[center] >= len/2+1`, because the radius includes the center; even intervals need `m.even[center] >= len/2`. The existing `oddInterval`/`evenInterval` methods perform the inverse conversion, including smaller valid radii.
- `maybePalindrome(h,l,r)` compares the forward and reverse fingerprints of `[l,r)`. False proves the interval is not a palindrome; true is a **Monte Carlo** answer. Both double-prime `StringHash<>` and unsigned-word `StringHash64` are supported, with their existing byte/integer encoding, size and base constraints. Empty intervals return true. No additional collision guarantee is introduced: the [hash contract](03-stringhash.md) applies, including fixed-base limitations and adversarial unsigned-word collisions.
- `longestPalindrome(m)` returns the half-open interval of the longest palindrome in the whole preprocessed sequence, breaking ties by smallest start index. Empty input returns `{0,0}`. It scans maximum odd/even intervals in `O(n)` time and constant auxiliary space. Extract byte contents with `auto [l,r] = longestPalindrome(m); auto result = s.substr(l,r-l);`, or use `[s.begin()+l,s.begin()+r)` for a generic sequence. The original sequence is needed only to extract contents.

Exact queries accept every byte or equality-comparable integer alphabet already supported by Manacher, with length at most `INT_MAX`. Hash queries inherit the stricter `n < INT_MAX` domain and integer encoding requirements. Manacher preprocessing is `O(n)` time/space; hash preprocessing is `O(n)` time/space. Every adapter uses `O(1)` additional storage. Longest extraction is a witness interval, so it avoids an unnecessary allocation or dependence on the source container type. Repeated longest queries can cache that returned interval at the call site.

Correctness: palindromes with a fixed center and parity form a chain under deletion of their two outer symbols. Therefore membership in that chain is precisely the corresponding radius inequality. Every nonempty palindrome has one character center or one interior gap, and cannot exceed its center's maximum interval. Scanning these maxima therefore contains a global optimum; explicit length/start comparison enforces the stated tie rule. The final gap can only contribute an empty interval and need not be scanned for the maximum. Differences and `l+len/2` avoid overflowing absolute endpoint sums, and existing interval reconstruction avoids forming an overflowing `2*odd_radius`.

## Feature-to-test map

| Feature | Independent coverage |
|---|---|
| Exact predicate, empty and odd/even intervals, index/radius conversion | Every interval of every ternary string through length 7; direct symmetric scans independent of Manacher's recurrence; endpoints and both parity branches. |
| Longest witness and leftmost tie rule | Brute-force interval enumeration picks the expected optimum; explicit `babad` and `abbacddc` ties, default empty and singleton. |
| Both probabilistic predicates | Exhaustive and seeded interval comparisons with direct scans; empty/default objects, all bytes including NUL/255, prime maximum symbol and full-width unsigned symbols. Finite agreement is never interpreted as collision freedom. |
| Collision limitation | A nonpalindromic length-2048 Thue–Morse word is accepted by unsigned-word hashing for ten seeded odd bases; direct scanning and the exact predicate reject it. With eleven construction levels, reversal complements every bit and the difference polynomial is divisible by `2^64`. |
| Generic alphabet and value semantics | 500 seeded byte cases and corresponding signed 64-bit extreme alphabets; source mutation, copied/moved destinations and reassignment of moved-from values. |
| Scale and bounds | 200000-symbol unary/alternating inputs, independently known longest answers and every suffix query; six checked negative/reversed/out-of-bounds range probes. |

The runnable entry is [08-palindrome_queries_tester.py](<../../96-Local Testing/07-Strings/08-palindrome_queries_tester.py>). Quick/full/stress exhaust ternary lengths through 5/7/9, add 80/500/3000 random cases, and use large lengths 5000/200000/1000000. Test oracles remain active under `-DNDEBUG`. Tests do not allocate impractical `INT_MAX` objects; the index argument above establishes the arithmetic boundary behavior. The mirrored suite runs from any working directory.

## Commands and results

P017 verification run, 2026-09-28: Linux x86-64, GCC 16.2.1 (20260810), GNU++20,
CPython 3.14.7, seed 20260928. Optimized `-O2 -DNDEBUG`, checked
`-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan (leak checking on; run outside the sandbox
because LeakSanitizer cannot run under its process tracer).

| Command from repository root | Result |
|---|---|
| `python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20260928` | PASS, all three configurations, 3,405,831 checks on 4,281 small cases each, plus the large structural cases; 6 assertion probes |
| `python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20260928 --no-integration` (from `/tmp`) | PASS, all nine Strings suites |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 102 standalone/aggregate headers, multi-TU scalar/AVX2 aggregates, Workspace LOCAL and non-LOCAL |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

Stress mode exists but was not run; no other compiler, including the `g++-14`
floor, is recorded as tested.

## Benchmarks

No benchmark: these are direct adapters with no competing specialization or
dispatch threshold. The hash benchmark in [03-stringhash.md](03-stringhash.md)
covers the underlying representation, not a claim that hashing beats exact
Manacher queries.

## Sources

Inspected on 2026-09-28 using the cached source copies retrieved for P016; these adapters are independently written, with no external code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html), page update August 7, 2026: statement, centered palindrome chains, direct odd/even radius definitions and parity conversions. The two existing parity arrays avoid transformed-string sentinels.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: inspected the two parity arrays and radius convention. Its odd radius excludes the center; the repository's canonical Manacher contract includes it.
- The existing [P016 hash proof and collision contract](03-stringhash.md) supplies fingerprint encoding, reversal and error assumptions. This package reuses that implementation and does not claim a new external hash-source review.

Targeted searches of preserved `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` found no palindrome-query or Manacher implementation to migrate.

## Limits and handoffs

- Open `/reaudit-review` findings for P017 ([p017.md](<../../00-Guidelines/23-Reaudit Findings/p017.md>)), not yet resolved:
  - 10: multi-line function and lambda bodies close on their own line (closing-brace rule).

Hash palindrome answers remain Monte Carlo: the suite demonstrates a known unsigned-word false positive while exact Manacher rejects it. Finite tests support the stated proofs and domains; they do not establish collision freedom or allocate impractical maximum-size objects. Eertree/partitioning (ST07), arbitrary-subrange longest/count palindrome queries and other advanced static queries (ST16) and dynamic edits (ST28) are separate planned owners, not functionality of these adapters.
