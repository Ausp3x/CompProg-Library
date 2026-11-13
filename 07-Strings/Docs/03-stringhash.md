# 03-stringhash.hpp — evidence

`03-stringhash.hpp` (batch ST02, package P016) provides static polynomial fingerprints in three fields — two primes, the word ring modulo `2^64` and the Mersenne field modulo `2^61 - 1` — with length- and base-tagged digests, forward and reversed interval extraction, digests of outside sequences, concatenation, Monte Carlo substring LCP/LCS, and a Monte Carlo longest common substring of two hashed sequences. Contest profile, GNU C++20, no ISA code; Mersenne folding is a fixed-modulus reduction, not Barrett or Montgomery.

## Contracts

### StringHash, StringHash64, StringHash61

`StringHash<KIND>` selects the field: `KIND = 0` (default) uses primes 1,000,000,007 and 1,000,000,009 with independent bases; `KIND = 1` (`StringHash64`) uses unsigned wraparound modulo `2^64` with an odd base; `KIND = 2` (`StringHash61`) uses the prime `2^61 - 1`. `StringHash<true>` still names the word variant because `true` converts to `1`; any other `KIND` fails a `static_assert`. Each object owns its base, power table and forward/reversed prefix fingerprints; there is no global state, cache or retained input reference.

Inputs: byte strings (embedded NUL and high bytes included) or `vector<uint>`, length below `INT_MAX`. Bytes encode as unsigned value plus one; `vector<uint>` symbols encode as value plus one, with `x < 1000000006` for `KIND = 0` (checked once at entry) and any `uint` otherwise. Larger or signed alphabets can be compressed to ranks first. `build(n, code)` accepts any `ulng` code: `encode` reduces it modulo each field modulus (identity modulo `2^64`), so congruent codes collide deterministically and callers wanting injective encoding keep codes below the modulus. `encode`, `add`, `subtract` and `multiply` are the field operations on normalized words (`add`/`subtract`/`multiply` require normalized inputs, which `encode` and all stored values satisfy); `build` asserts `0 <= n < INT_MAX` and the base range once. Each `build` evaluates `code(i)` twice per index.

Bases: `defaultBase()` is fixed (`{911382323, 972663749}`, `11400714819323198485`, `2054820241022868036`, the last a primitive root modulo `2^61 - 1`). `randomBase(seed)` is reproducible: it draws from `std::mt19937_64(seed)` with `std::uniform_int_distribution<ulng>`, uniform in `[257, p - 1]` per prime for `KIND = 0` (first prime first), the first odd output of at least 257 for `KIND = 1`, and uniform in `[257, 2^61 - 3]` for `KIND = 2` (excluding `p - 1`, of order 2). `build` asserts `257 <= base < p` (and oddness for `KIND = 1`).

`Digest` holds value, length and base; it has defaulted three-way comparison, so equality is exact on all three members and ordering (value, then length, then base) makes digests usable as `set`/`map` keys. Unequal lengths or bases never compare equal; equal digests are fingerprints, not proof. `get(l, r)` and `reverseGet(l, r)` take half-open ranges, the latter hashing the reversal of exactly that interval; empty intervals have value zero. `hashOf(s)` (bytes or `vector<uint>`) returns the digest of an outside sequence under this object's base in O(L) time and O(1) space, equal to `get` of the same content. `power(k)` uses the table when `k <= size()` and binary powering otherwise, without growing hidden state. `concat(a, b)` requires this object's base on both inputs and a total length that fits `int`.

`lcp` and `lcs`, through the helper `common(other, l, r, a, b, suffix)`, binary-search the common prefix or suffix length of two ranges under equal bases in O(log(k + 1)) for the shorter length k; answers are Monte Carlo. No lexicographic comparison is offered (row `22` owns `compareSubstrings`).

Costs: construction O(n) time and three arrays of n + 1 eight-byte words; `get`, `reverseGet` O(1). Copies are independent; assign a moved-from object before querying it.

Collision model: for two fixed distinct equal-length sequences of length L with injective codes, the difference is a nonzero polynomial of degree at most L - 1. With bases uniform in the stated ranges, the root bound gives a collision probability at most `(L - 1) / (p - 257)` per prime for `KIND = 0` (the product for both), and at most `(L - 1) / (2^61 - 259)` for `KIND = 2`. Batches use a union bound; an LCP/LCS error needs a false positive on its binary-search path. Inputs must be independent of the base; fixed defaults and seeded generators are reproducibility conveniences, not adaptive-adversary protection. The `2^64` ring has no root bound: the length-1024 Thue–Morse word and its complement collide for every odd base (their difference is `±prod(1 - b^(2^i), i < 10)`, which contains at least 64 factors of two).

Correctness: Horner prefixes satisfy `H[i+1] = H[i] * base + code(i)`; `H[r] - H[l] * base^(r-l)` is the normalized interval polynomial, reversed prefixes apply the same identity to `[n - r, n - l)`, and concatenation shifts the left polynomial by the right length. Prime residues stay normalized: sums stay below twice the modulus, 32-bit products fit `ulng`. For `2^61 - 1`, a product `t < 2^122` folds as `(t mod 2^61) + floor(t / 2^61) < 2p`, so one conditional subtraction normalizes it; `a + b < 2^62` likewise.

### longestCommonSubstringHash

`longestCommonSubstringHash(a, b)` takes two `StringHash<KIND>` objects with equal bases (asserted once), of sizes `n = a.size()` and `m = b.size()` (a moved-from object, of size `-1`, is asserted against; under `NDEBUG` the call returns `{0, 0, 0}` within `O(max(n, 1))` time and memory, since the table is sized from `max(n, 1)` (two slots when `a` is moved-from, `a`'s usual table when `b` is) and the search range `[0, min(n, m)]` is then empty), and returns `{i, j, L}` such that `a`'s window `[i, i + L)` and `b`'s window `[j, j + L)` have equal digests, with `L` the largest length found. An empty input or no common symbol returns `{0, 0, 0}`, the same triple as row `07` `longestCommonSubstring`. Tie rule, exact whenever no fingerprint collides: `j` is the earliest start in `b` of a length-`L` window occurring in `a`, and `i` is the earliest start in `a` of that window. It takes two strings only: the k-string form is row `28` (user decision 2026-11-14).

Algorithm: binary search on `L` over `[0, min(n, m)]`. Each probe inserts the keys of `a`'s length-`L` windows into an open-addressing table in increasing `i` order, keeping the first `i` per key, then scans `b` in increasing `j` order and stops at the first key present. Keys are the window values (the two residues packed into one `ulng` for `KIND = 0`, which is injective), and slots come from a multiplicative hash of the top `bits` bits. Window values are read straight from the prefix and power tables, so the inner loops run no range asserts.

Costs: the table has `2^bits >= 2 * max(n, 1)` slots (at most `4 * max(n, 1)`; the exact size is unspecified, user decision 2026-11-14), so its load stays at most one half and linear probing always reaches an empty slot. Each probe refills the table, at `O(n)`, and does `O(n + m)` expected table operations. Total time is `O(n + (n + m) * log(min(n, m) + 1))` expected (the `n` term is the table, allocated even when `m = 0`) and auxiliary memory is `12 * 2^bits <= 48 * max(n, 1)` bytes, built over `a`. To save memory, pass the shorter sequence as `a`; this changes which side the tie rule favours. The expected bound treats fingerprints as spread uniformly by the multiplicative slot hash. Clustered keys degrade the probing, not the answer.

Correctness: equal windows always have equal digests, so a probe never misses a true common length. Common lengths are downward closed, because a prefix of a common substring is common. So without a collision the binary search returns the exact maximum length, and `lo` changes only on a successful probe, which makes `best` the witness of the final `lo`. A collision can only make a probe report success wrongly. Since `hi` falls only after a failed probe, which is exact, the returned `L` is never below the true length. An error needs two distinct equal-length windows of `a` or `b` with equal digests in one of at most `ceil(log2(min(n, m) + 1))` probes: within `a` it gives a wrong witness `i`, across `a` and `b` a false match. By the collision model above and a union bound over the at most `C(n + m, 2)` window pairs per probe, the error probability is at most `ceil(log2(k + 1)) * C(n + m, 2)` times the per-pair bound at length `k = min(n, m)`, the longest probed length: `(k - 1)^2 / (p - 257)^2` for both primes of `KIND = 0` and `(k - 1) / (2^61 - 259)` for `KIND = 2`. `StringHash61` gives the smallest bound for long inputs; the `2^64` ring has none.

## Feature-to-test map

Runner: [`03-stringhash_tester.py`](<../../96-Local Testing/07-Strings/03-stringhash_tester.py>). Every check runs for all three fields except the `naiveCommon` validation and the row `07` cross-check, which are field-independent and run once, under the double-prime field; oracles are independent 128-bit Horner evaluations with codes reduced modulo each field, direct character scans and exact substring sets; the Python entry adds arbitrary-precision weighted power sums.

| Operation | Coverage |
|---|---|
| get, reverseGet, size, Digest tags | Exhaustive binary strings, random integer symbols and every interval, `reverseGet` compared as a whole digest (value, length, base) with `hashOf` of the reversed interval; Python exact weighted power sums (bases from `randomBase`) for all three fields. |
| encode, add, subtract, multiply | All pairs of boundary and random residues (0, 1, 2, 256, middle, `p - 2`, `p - 1`) against 128-bit modular references; `encode` on codes around 1e9+7, 1e9+9, `2^32`, `2^61 - 1` and `2^64 - 1`. |
| build (caller functor) | 30/300/2000 random code sequences drawn from those codes, every interval forward and reversed; the code-reduction regression (`2^32 + 1` versus `1`) differs in every field. |
| hashOf | Every substring of every tested sequence (`vector<uint>`), byte strings including all 256 bytes, substrings and empty input. |
| power, concat | Split/join identities, right fragments beyond the table, `power(3n + 1)` against one multiplication, a digest of length exactly `INT_MAX` built by 61 concatenations in every build, and a checked overflow probe past it. |
| lcp, lcs (through the helper `common`) | Direct scans on random ranges of independent objects, self ranges, 200,000-byte unary and middle-mismatch inputs. |
| longestCommonSubstringHash | O(n * m) suffix-length DP oracle with the same tie rule, itself validated against a direct comparison of every window pair on the exhaustive binary pairs: every pair of binary strings through length 4/6/7 (quick/full/stress, empty included), 200/2000/10000 random pairs over alphabets of 1–4 symbols and lengths below 60, each in both orders and against itself; 10,000/200,000/1,000,000-byte fixtures: a planted copy of `n/10` random letters (length and witness content), unary text against one middle mismatch (`{0, 0, n/2}`), and no common symbol (`{0, 0, 0}`); `2^16 - 2`/`2^18 - 2`/`2^20 - 2` distinct shuffled symbols against a disjoint set (`{0, 0, 0}`) and against themselves, which nearly fill a table of `n + 2` slots at every short length. Under the double-prime field, lengths are cross-checked against row `07` `longestCommonSubstring` (suffix array) on every `s`/`t` pair and on the planted, unary-cut and distinct-disjoint fixtures; the two forms break ties differently, so only lengths are compared. |
| defaultBase, randomBase, contexts | Full and stress compile `StringHash<0>` and `StringHash<2>` and check that `StringHash<-1>` and `StringHash<3>` fail the `static_assert` (`-fsyntax-only`, once per run); 2000 seeds per field inside the stated ranges, 64 seeds per field equal to the documented draw from `mt19937_64(seed)`, seed repeatability, `StringHash<true>` identity with `StringHash64`, copy/move/assignment, different-base and different-length rejection. |
| Digest ordering | `set<Digest>` size equals the exact distinct-substring count on random strings; `<` and `==` match the member-wise tuple order. |
| StringHash64, StringHash61 limits | Ten seeded odd bases reproduce the Thue–Morse word-ring collision; the double-prime and Mersenne fields separate the same pair. |

Checked builds run 23 assertion probes: base bounds and parity for all three fields (including a `2^61 - 1` field base above the modulus), the `KIND = 0` alphabet in the constructor and in `hashOf`, negative and `INT_MAX` `build` lengths, a byte view of length `INT_MAX` in `hashOf` (an `mmap` reservation of address space, never read before the assertion), a moved-from input to `longestCommonSubstringHash`, invalid ranges, mixed contexts (concatenation with a foreign left or right digest, `lcp`, `longestCommonSubstringHash` on different bases), negative power and concatenated-length overflow. Finite tests do not establish absence of collisions; the collision model is the guarantee.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 860 MB; optimized 0.3 s compile + 1.6 s run, checked 23.4 s compile + 3.6 s run, ASan-UBSan 31.5 s compile + 9.3 s run; MEMORY peak 1037 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 943 MB; optimized 0.2 s compile + 1.5 s run, checked 32.4 s compile + 3.0 s run, ASan-UBSan 35.1 s compile + 16.5 s run; MEMORY peak 1058 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 31.1 s compile + 0.4 s run, checked 24.5 s compile + 2.6 s run; MEMORY peak 875 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 875 MB; optimized 0.1 s compile + 1.2 s run, checked 9.3 s compile + 4.7 s run, ASan-UBSan 11.4 s compile + 18.5 s run; MEMORY peak 602 MB. Dated rows below stay those commands' latest results.

Run 2026-11-14 (ST02; experiment: none; Intel Core i9-11900H, GNU++20). Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). Every line passed:

```bash
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 1  # PASS, 3 configurations, 2,028,867 checks plus 7,053 Python exact intervals each, 23 assertion probes, KIND domain check; pre-flight quick peak 952 MB, optimized 0.22 s and checked 2.83 s quick runs; MEMORY peak 1356 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 2  # PASS, 3 configurations, 2,056,105 checks plus 6,155 Python intervals each, 23 probes, KIND domain check; pre-flight quick peak 980 MB; MEMORY peak 1413 MB
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 11,218,753 checks plus 7,189 Python intervals each, 23 probes, KIND domain check; runs 12.4 s / 31.5 s / 58.5 s; MEMORY peak 974 MB
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 1  # PASS, 3 configurations (dependent maybePalindrome); MEMORY peak 561 MB
python3 '96-Local Testing/05-extraction.py' ops --filter 07-Strings/03-stringhash.hpp  # PASS, 19 operations in 11 extractions on g++ and g++-14, 9 in their default form, 0 failures, 0 uninstantiated templates
python3 '96-Local Testing/05-extraction.py' testers --filter 03-stringhash  # PASS, 1 quick run on the extracted build (5 names), after the row 07 cross-check
python3 '96-Local Testing/02-integration.py'  # PASS, 190 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace; MEMORY peak 295 MB
python3 '96-Local Testing/03-consistency.py'  # no errors
```

## Mutation testing

Record: mewt 4.0.0 on blob `89a778c819607976e0cee332ca01e0d326ada071` (2026-11-14, seed 1, 1 lines changed since blob `3882fc5e4c46`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

That `/finish` campaign covers the complexity comment only (line 135), where mewt generates no mutant. The campaign before it is the record for the code: mewt 4.0.0 on blob `3882fc5e4c46cdb7cd8bb34e016ffc2e71237e33` (2026-11-14, seed 1, 5 lines changed since blob `5ea3d1c9b539`): 35 mutants, 31 caught (1 by timeout), 4 survived, 0 untested.

That campaign covers the review fixes: the `KIND` `static_assert`, the `n` and `m` definition in the complexity comment, the moved-from assert, table sizing from `max(n, 1)` in place of an early return, and the default `template<int KIND = 0>` on the function for extraction. Stage seconds: quick 52, full optimized build 153, full checked and ASan-UBSan builds 203. The first run of this campaign left a fifth survivor, `AOS 454` (`2 * uint(...)` → `2 + uint(...)`), which sizes the table to about `n + 2` slots at `n = 2^k - 2`: answers stay the same, but with every short window distinct the table is nearly full and each failed linear-probing lookup walks most of it. The distinct-symbol fixture (Coverage) closes it by timeout, confirmed alone, in the full optimized stage; measured on a scratch copy, its quick run takes 28 s against 0.22 s for the header. The four survivors are on line 142 (`int bits = std::bit_width(2 * uint(max(n, 1)) - 1);`):

| Mutant | Line | Why it survives |
|---|---|---|
| `AS 230` `max(n, 1)` → `max(1, n)` | 142 | equivalent: `max` commutes |
| `AOS 450`, `AOS 451`, `AOS 452` (`2x - 1` → `2x + 1`, `2x * 1`, `2x / 1`) | 142 | equivalent under the contract (user decision 2026-11-14: the exact table size stays unspecified within `4 * max(n, 1)` slots): the table at most doubles and stays within that bound, its load only falls and every answer is unchanged; only a test measuring the allocation could tell them apart |

An intermediate campaign on blob `4e13f16704059d8d901c03f6dc3db3b9bedc45db` (the same fixes with an early return `if (n <= 0 || m <= 0)`) left `LOS 535` (the `static_assert` with `||`), now caught by the `KIND` domain check, and `COS 838`, `COS 843`, `COS 845` on the early return. One of those, `n == 0`, let a moved-from input reach a 48 GiB table under `NDEBUG`. The rules give an optimized build no out-of-domain inputs, so no test could catch it, and sizing from `max(n, 1)` removed the early return and the mutants with it.

Earlier campaign, the header's first: mewt 4.0.0 on blob `5ea3d1c9b539c13baf7bd5e22e169bc57c003965` (2026-11-14, seed 1, whole file, uniform sample of 874): 500 mutants, 474 caught (34 by timeout), 26 survived, 0 untested. Line numbers below are that blob's; the `static_assert` moved the later lines down by one.

```bash
python3 '96-Local Testing/04-mutation.py' run 07-Strings/03-stringhash.hpp
python3 '96-Local Testing/04-mutation.py' run 07-Strings/03-stringhash.hpp --workdir /tmp/cp-mutation-03-stringhash-8ng9a6ko   # survivor retest; workdir deleted once recorded
python3 '96-Local Testing/04-mutation.py' run 07-Strings/03-stringhash.hpp   # after the review fixes, changed lines (three times: the intermediate blob, the final blob, and the final blob again with the distinct-symbol fixture, its record pointed back at blob 5ea3d1c for the same scope)
python3 '96-Local Testing/04-mutation.py' run 07-Strings/03-stringhash.hpp   # /finish: the changed complexity line, 0 mutants
```

Stage seconds of the survivor retest: quick 1450 (including a wait on another campaign's lock), full optimized build 168, full checked and ASan-UBSan builds 806.

Its first run left 38 survivors. Five tester additions closed 12 of them:

- **Exact `randomBase` draws.** 64 seeds per field are compared with the documented draw from `mt19937_64(seed)`. This closed `AOS 248`, `AOS 249`, `AOS 253` and `AOS 257` (lines 26 and 31), which change a range bound.
- **`reverseGet` digest tags.** `reverseGet(l, r)` is now compared as a whole digest, length included. This closed `AOS 399` and `AOS 400` (line 90).
- **`INT_MAX` concatenation.** Every build builds a digest of length exactly `INT_MAX`. This closed `COS 786` (line 114).
- **Probes.** Four new probes: `base61-above` closed `COS 720` (line 74), `build-max` closed `COS 681` (line 70), `hashof-length` closed `CR 97` and `COS 746` (line 93), and `concat-base-left` closed `COS 768` (line 113).

The other 26 stand:

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 551`, `COS 556`, `COS 561`, `COS 576`, `COS 606`, `COS 621`, `COS 631`, `COS 646`, `COS 666`, `COS 686`, `COS 816` (`KIND == c` → `KIND <= c` in `if constexpr` chains) | 20, 21, 26, 33, 47, 52, 55, 58, 64, 71, 146 | equivalent: `KIND` is 0, 1 or 2 (a `static_assert` since the review fixes), and each chain tests the smaller values first, so `KIND <= c` selects the same branch |
| `AS 195`, `AS 199`, `AS 205`, `AS 206`, `AS 211`, `AS 233` (operands of `multiply` or `add` swapped) | 78, 80, 90, 95, 109, 148 | equivalent: field addition and multiplication commute, and both operands are normalized words |
| `AS 239` `min(n, m)` → `min(m, n)` | 162 | equivalent: `min` commutes |
| `AAOS 510` `a[i] -= MOD[i]` → `a[i] %= MOD[i]` | 50 | equivalent: the branch runs only for a sum in `[MOD, 2 * MOD)` (two normalized residues, below `2^32`), where both give `a - MOD` |
| `COS 673` `x >= P61` → `x > P61` | 68 | equivalent: the fold gives `x ≡ a * b (mod p)` with `x < 2p`, so `x == p` needs `a * b ≡ 0`, which for normalized words means `a * b = 0` and so `x = 0` |
| `COS 680` `n < INT_MAX` → `n != INT_MAX`; `COS 690` `i < 2` → `i != 2` | 70, 72 | equivalent: an `int` never exceeds `INT_MAX`, and `i` runs through 0 and 1 |
| `COS 855` `lo < hi` → `lo != hi` | 164 (165 now) | recorded survivor (user decision 2026-11-12, NDEBUG-only mutants): inside the domain the binary search keeps `lo <= hi`, so results agree; a moved-from input under `NDEBUG` starts at `hi = -1`, where the mutant runs one probe at length 1 and returns the same `{0, 0, 0}`, but with `a` moved-from that probe scans all of `b`, `O(m)` beyond the stated `O(max(n, 1))`, so it leaves the bound there (bounded; the `/finish` reviewer's scratch copy at `-O2 -DNDEBUG` measured 144 ms against 0.0065 ms at `m = 2 * 10^7`) |
| `CR 107` (table lookup in `power` removed) | 106 | not equivalent, kept as a cost-only survivor (user decision 2026-11-14): binary powering returns the same `base^k` that `powers[k]` stores, but in `O(log(k))` instead of the documented `O(1)`; a timeout fixture would need seconds of extra `concat` calls in every full run to cross the harness's 60 s minimum |
| `COS 590` `s.size() < INT_MAX` → `!=` (`vector<uint>` constructor) | 38 | untestable (the user decisions of 2026-11-12 (Fenwick) and 2026-11-13 (Z) on `INT_MAX` size asserts, applied here on 2026-11-14 and confirmed by the user the same day): sizes in `(INT_MAX, 2^32)` still assert in `build`, since `int(s.size())` is negative, so only a vector of at least `2^32` symbols, 16 GiB, tells them apart, beyond the memory caps; the assert stays |
| `COS 751` `s.size() < INT_MAX` → `<=` (`hashOf(vector<uint>)`) | 98 | untestable (the same decisions, applied and confirmed as `COS 590`): only a vector of exactly `INT_MAX` symbols, 8 GiB, tells them apart, beyond the memory caps, and its digest is still correct because the length fits `int`; a `vector<uint>` cannot reserve address space without memory as the byte probe `hashof-length` does; the assert stays |

## Benchmarks

[`03-stringhash_benchmark.py`](<../../96-Local Testing/07-Strings/03-stringhash_benchmark.py>) builds each hash and answers LCP queries; every warmup and timed answer is verified against direct scans. Run 2026-10-08, i9-11900H, GCC 16.2.1, `-O2 -DNDEBUG`, seed 20261008, one warmup and five rotating repetitions; local log `03-stringhash_benchmark.json` (git-ignored).

```bash
python3 '96-Local Testing/07-Strings/03-stringhash_benchmark.py' --seed 20261008 --warmup 1 --reps 5
```

Median setup / query / pipeline milliseconds, 262,144 bytes and 256 queries:

| Input | Double prime | Word `2^64` | Mersenne `2^61 - 1` | Direct scans |
|---|---|---|---|---|
| Random bytes | 6.714 / 0.218 / 6.932 | 3.675 / 0.153 / 3.839 | 5.517 / 0.179 / 5.670 | 0 / 4.321 / 4.321 |
| Period 11 | 6.066 / 0.210 / 6.259 | 3.414 / 0.157 / 3.530 | 5.096 / 0.193 / 5.265 | 0 / 11.607 / 11.607 |
| Unary | 6.014 / 0.221 / 6.236 | 3.437 / 0.155 / 3.592 | 5.088 / 0.212 / 5.286 | 0 / 17.242 / 17.242 |

At 4096 bytes and 1024 queries the pipelines were 0.33, 0.12–0.13, 0.24–0.25 and 0.31–1.23 ms respectively. The Mersenne field is about 15–20% faster than the double-prime default with a single 61-bit field; the word ring is fastest but has the structured collision above. Auxiliary storage is `24 * (n + 1)` bytes plus the object for every field. No dispatch threshold or universal winner is claimed.

## Sources

Code is independently implemented.

- [cp-algorithms, String Hashing](https://cp-algorithms.com/string/string-hashing.html): polynomial encoding, substring extraction, multiple moduli, the warning about modulo-`2^64` collisions.
- [OI Wiki, 字符串哈希](https://oi-wiki.org/string/hash/) (fetched 2026-11-14): longest common substring by binary search on the length with a hash table of window fingerprints; its k-string form goes to row `28`.
- [KACTL Hashing.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Hashing.h): prefix powers, interval subtraction, `hashString`-style digest of an outside string, Thue–Morse warning.
- [maspypy `rolling_hash.hpp` and `rolling_hash_field.hpp`](https://github.com/maspypy/library/blob/main/string/rolling_hash.hpp): object-local base, concatenation, substring LCP, `eval`; the `2^61 - 1` field variant.
- [Nyaan `internal/internal-hash.hpp`](https://github.com/NyaanNyaan/library/blob/master/internal/internal-hash.hpp) (fetched 2026-10-08): Mersenne folding `(t & p) + (t >> 61)` and primitive-root base selection over the factors of `2^61 - 2`.
- [hitonanode `rolling_hash_1d.hpp`](https://hitonanode.github.io/cplib-cpp/string/rolling_hash_1d.hpp): digest ordering for container keys.
- Catalog sweep 2026-10-08 and ST02 re-sweep 2026-11-14 in [00-sources.md](00-sources.md#st02-re-sweep-2026-11-14). `OLD` contains container hashes only (Miscellaneous), no rolling hash.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): appending to a built hash (`push_back`; reversed prefixes cannot extend in O(1), row `23` owns appendable and deque hashes), a field-generic `StringHashField<F>`, several Mersenne bases, `std::hash` for digests, lexicographic substring comparison (row `22`), Rabin–Karp windows (row `16`).
- `longestCommonSubstringHash` takes two sequences. The k-string hash form is the row `28` operation `longestCommonSubstringHash (k strings)` (user decision 2026-11-14), scheduled in batch ST11, which now waits on ST02. No benchmark is recorded for it: the Contest profile requires none, and no dispatch or crossover against row `07`'s exact suffix-array version is claimed.
- Hash answers stay Monte Carlo; `08-palindrome_queries.hpp` `maybePalindrome` takes `StringHash<KIND>` (its template parameter changed from `bool` because `bool` cannot be deduced from an `int` parameter) and is tested on all three fields.

## Aliases

Search aliases (P263, grab --find): 字符串哈希, 滚动哈希, 多项式哈希, 自然溢出哈希, 双哈希, 二分哈希, 子串哈希, 最长公共子串, 哈希求最长公共子串, longest common substring hash.

## History

- 2026-09-28: double-prime and word variants verified under the previous system (P016).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, template parameter `bool WRAP64` became `int KIND`, `StringHash61`, `hashOf` and Digest ordering added, 5 findings fixed, contracts moved out of the header; quick, full and stress passed on g++, full on g++-14, integration with sanitizers passed.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: ST02, `longestCommonSubstringHash` (two strings; user decision 2026-11-14: k strings go to row `28`) implemented; the header's first mutation campaign closed 12 survivors with 5 test additions and recorded 26; full passed on g++ and g++-14, stress passed, extraction, integration and the dependent row `08` passed.
- 2026-11-14: ST02 review (2 medium, 8 low findings fixed): `KIND` `static_assert` with a compile-time check, moved-from inputs asserted and bounded, the collision bound at length `min(n, m)`, the `naiveCommon` oracle validated, an `mmap` byte-view probe, the sweep record in `00-sources.md` and `00-notes.md`; a changed-lines mutation campaign recorded 5 cost-only or equivalent survivors; full passed on g++ and g++-14, stress, extraction, integration and the dependent row `08` passed.
- 2026-11-14: ST02, user decision: `AOS 454` closed by a distinct-symbol fixture (caught by timeout) instead of being recorded as cost-only; the changed-lines campaign rerun records 4 survivors; full passed on g++ and g++-14, stress and the tester extraction passed.
- 2026-11-14: ST02 `/finish` (review: 2 medium, 9 low findings and limits): row `07` suffix-array lengths cross-checked under the double-prime field (each `s`/`t` pair, planted, unary-cut and distinct-disjoint fixtures); complexity line gains the `O(n)` table term; moved-from, `max(n, 1)`, probe-count and survivor arguments corrected; user decisions 2026-11-14 on `CR 107` (cost-only survivor) and `AOS 450`–`452` (table size unspecified); `COS 855` a recorded survivor under the 2026-11-12 decision; changed-line campaign 0 mutants; full passed on g++ and g++-14, stress, extraction, integration and the dependent row `08` passed.
