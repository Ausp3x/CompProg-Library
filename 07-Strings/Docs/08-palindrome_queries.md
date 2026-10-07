# 08-palindrome_queries.hpp — evidence

## Contracts

`08-palindrome_queries.hpp` provides three small adapters over the verified P016 `Manacher` and `StringHash` types. They retain no input reference or extra state, and take existing preprocessing by const reference. Callers preserve the underlying objects' invariants; assign a new valid value before querying a moved-from object.

- `isPalindrome(m,l,r)` is an exact constant-time predicate for `[l,r)`, with `0 <= l <= r <= m.size()`. Empty intervals, including the final gap, are palindromes. With `len=r-l`, the center is `l+len/2`. Odd intervals need `m.odd[center] >= len/2+1`, because the radius includes the center; even intervals need `m.even[center] >= len/2`. The existing `oddInterval`/`evenInterval` methods perform the inverse conversion, including smaller valid radii.
- `maybePalindrome(h,l,r)` compares the forward and reverse fingerprints of `[l,r)`. False proves the interval is not a palindrome; true is a **Monte Carlo** answer. Both double-prime `StringHash<>` and unsigned-word `StringHash64` are supported, with their existing byte/integer encoding, size and base constraints. Empty intervals return true. No additional collision guarantee is introduced: the [hash contract](03-stringhash.md#collision-model) applies, including fixed-base limitations and adversarial unsigned-word collisions.
- `longestPalindrome(m)` returns the half-open interval of the longest palindrome in the whole preprocessed sequence, breaking ties by smallest start index. Empty input returns `{0,0}`. It scans maximum odd/even intervals in `O(n)` time and constant auxiliary space. Extract byte contents with `auto [l,r] = longestPalindrome(m); auto result = s.substr(l,r-l);`, or use `[s.begin()+l,s.begin()+r)` for a generic sequence. The original sequence is needed only to extract contents.

Exact queries accept every byte or equality-comparable integer alphabet already supported by Manacher, with length at most `INT_MAX`. Hash queries inherit the stricter `n < INT_MAX` domain and integer encoding requirements. Manacher preprocessing is `O(n)` time/space; hash preprocessing is `O(n)` time/space. Every adapter uses `O(1)` additional storage. Longest extraction is a witness interval, so it avoids an unnecessary allocation or dependence on the source container type. Repeated longest queries can cache that returned interval at the call site.

For correctness, palindromes with a fixed center and parity form a chain under deletion of their two outer symbols. Therefore membership in that chain is precisely the corresponding radius inequality. Every nonempty palindrome has one character center or one interior gap, and cannot exceed its center's maximum interval. Scanning these maxima therefore contains a global optimum; explicit length/start comparison enforces the stated tie rule. The final gap can only contribute an empty interval and need not be scanned for the maximum. Differences and `l+len/2` avoid overflowing absolute endpoint sums, and existing interval reconstruction avoids forming an overflowing `2*odd_radius`.

These are direct contest-profile adapters with no competing specialization, dispatch threshold or reducer choice requiring a new comparative benchmark. Large structural correctness cases exercise linear extraction and constant-time queries. The existing hash benchmark remains evidence for that underlying representation, not a claim that hashing beats exact Manacher queries.

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

Full mode passed **3,405,831 checks on 4,281 independently checked small cases** in each of optimized NDEBUG, checked and ASan/UBSan configurations, seed `20260928`, GCC 16.2.1 (20260810), GNU++20. The large structural cases are additional to that small-case count. All six checked assertion probes passed. Commands:

```bash
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20260928
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

The initial sanitizer process encountered LeakSanitizer's fatal ptrace incompatibility under the sandbox; the approved retry outside that tracer passed with leak detection enabled. Optimized uses `-O2 -DNDEBUG`, checked uses `-O0 -g -D_GLIBCXX_DEBUG`, and sanitizer uses `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Stress is available but was not executed. Package-wide integration is recorded below and in the P017 package record in [00-notes.md](00-notes.md). No owned feature gap remains.

Batch ST03, package P017: exact Manacher interval queries, explicitly probabilistic hash queries, leftmost longest palindrome witness and radius/index conversions.

P017 package verification (sole batch ST03, completed 2026-09-28; the C01/P002 and ST01–ST02/P016 prerequisites are verified). Seed **20260928**, GCC **16.2.1 (20260810)**, GNU++20, CPython **3.14.7**, Linux x86-64. All three completed full optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan configurations with leak checking enabled.

| Header | Checks in each full configuration | Checked precondition probes |
|---|---|---|
| Palindrome queries | 3,405,831 | 6 |

All **23** precondition probes passed. The initial sandbox sanitizer processes encountered LeakSanitizer's fatal ptrace incompatibility; approved reruns outside that tracer passed without disabling leak detection. The linked per-header records distinguish those retries. Aho also passed an optimized stress run with seed `20260929`, 9,612,363 checks, before the final dense-table release regression was added; the final full run includes that regression. Other stress modes are available but unexecuted. No untested compiler/interpreter configuration is claimed.

Independent review covered all three algorithms, boundary arithmetic, witness ties, test oracles and source/legacy claims. It confirmed the explicit empty-pattern/empty-suffix/empty-interval conventions. Review also led to Aho's empty-pattern construction cost being documented and a nested const-callback regression. Final Aho rebuilding from dense to sparse releases the completed transition table, with a capacity regression in the full suite.

Integration passed:

- **102** current standalone/aggregate headers compiled.
- All aggregates linked and ran across multiple translation units in scalar and available AVX2 configurations.
- The standalone Workspace compiled in LOCAL and non-LOCAL configurations.
- All **nine** Strings suites passed shared quick discovery from `/tmp`.
- Repository consistency passed with no errors: 428 targets, 335 batches and 226 packages. The checker found one preexisting trailing-whitespace issue on the P034 checklist line; only that whitespace was removed.

Per-header full command:

```bash
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 20260928
```

Package integration commands:

```bash
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Shared discovery, run with working directory `/tmp`:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20260928 --no-integration
```


The nine-suite discovery and aggregate integration preceded Aho's final memory-release correction; final Aho full verification covers the correction, and its shared quick entry passed again afterward with `--filter 07-Strings/06-aho`. The change does not alter declarations or dependencies. Strings Basic includes all nine current implementations and All includes Basic transitively.

## Benchmarks

No benchmark: the Manacher adapters use their direct justified algorithm without a new specialization or dispatch threshold requiring timing comparisons.

## Sources

Inspected on 2026-09-28 using the cached source copies retrieved for P016; these adapters are independently written, with no external code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html), page update August 7, 2026: statement, centered palindrome chains, direct odd/even radius definitions and parity conversions. The two existing parity arrays avoid transformed-string sentinels.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: inspected the two parity arrays and radius convention. Its odd radius excludes the center; the repository's canonical Manacher contract includes it.
- The existing [P016 hash proof and collision contract](03-stringhash.md) supplies fingerprint encoding, reversal and error assumptions. This package reuses that implementation and does not claim a new external hash-source review.

Targeted searches of preserved `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` found no palindrome-query or Manacher implementation to migrate. Originals are unchanged. Eertree/partitioning belongs to ST07; longest/count palindrome queries constrained to arbitrary subranges and other advanced static queries belong to ST16; dynamic edits belong to ST28. They remain separate planned ownership, not functionality advertised by these static adapters.

## Limits and handoffs

Hash palindrome answers remain Monte Carlo: the suite demonstrates a known unsigned-word false positive while exact Manacher rejects it. Finite tests support the stated proofs and domains; they do not establish collision freedom or allocate impractical maximum-size objects. ST07/ST16/ST28 retain eertree/advanced static/dynamic palindrome ownership; longest-palindrome queries constrained to a substring are explicitly listed under ST16. These separate scheduled families are not advertised as implemented here. No online submission or acceptance is claimed.
