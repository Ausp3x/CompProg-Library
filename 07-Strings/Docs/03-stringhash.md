# 03-stringhash.hpp — evidence

`03-stringhash.hpp` (batch ST02, package P016) provides static polynomial fingerprints in three fields — two primes, the word ring modulo `2^64` and the Mersenne field modulo `2^61 - 1` — with length- and base-tagged digests, forward and reversed interval extraction, digests of outside sequences, concatenation, and Monte Carlo substring LCP/LCS. Contest profile, GNU C++20, no ISA code; Mersenne folding is a fixed-modulus reduction, not Barrett or Montgomery.

## Contracts

### StringHash, StringHash64, StringHash61

`StringHash<KIND>` selects the field: `KIND = 0` (default) uses primes 1,000,000,007 and 1,000,000,009 with independent bases; `KIND = 1` (`StringHash64`) uses unsigned wraparound modulo `2^64` with an odd base; `KIND = 2` (`StringHash61`) uses the prime `2^61 - 1`. `StringHash<true>` still names the word variant because `true` converts to `1`. Each object owns its base, power table and forward/reversed prefix fingerprints; there is no global state, cache or retained input reference.

Inputs: byte strings (embedded NUL and high bytes included) or `vector<uint>`, length below `INT_MAX`. Bytes encode as unsigned value plus one; `vector<uint>` symbols encode as value plus one, with `x < 1000000006` for `KIND = 0` (checked once at entry) and any `uint` otherwise. Larger or signed alphabets can be compressed to ranks first. `build(n, code)` accepts any `ulng` code: `encode` reduces it modulo each field modulus (identity modulo `2^64`), so congruent codes collide deterministically and callers wanting injective encoding keep codes below the modulus. `encode`, `add`, `subtract` and `multiply` are the field operations on normalized words (`add`/`subtract`/`multiply` require normalized inputs, which `encode` and all stored values satisfy); `build` asserts `0 <= n < INT_MAX` and the base range once. Each `build` evaluates `code(i)` twice per index.

Bases: `defaultBase()` is fixed (`{911382323, 972663749}`, `11400714819323198485`, `2054820241022868036`, the last a primitive root modulo `2^61 - 1`). `randomBase(seed)` is reproducible: uniform in `[257, p - 1]` per prime for `KIND = 0`, odd and at least 257 for `KIND = 1`, uniform in `[257, 2^61 - 3]` for `KIND = 2` (excluding `p - 1`, of order 2). `build` asserts `257 <= base < p` (and oddness for `KIND = 1`).

`Digest` holds value, length and base; it has defaulted three-way comparison, so equality is exact on all three members and ordering (value, then length, then base) makes digests usable as `set`/`map` keys. Unequal lengths or bases never compare equal; equal digests are fingerprints, not proof. `get(l, r)` and `reverseGet(l, r)` take half-open ranges, the latter hashing the reversal of exactly that interval; empty intervals have value zero. `hashOf(s)` (bytes or `vector<uint>`) returns the digest of an outside sequence under this object's base in O(L) time and O(1) space, equal to `get` of the same content. `power(k)` uses the table when `k <= size()` and binary powering otherwise, without growing hidden state. `concat(a, b)` requires this object's base on both inputs and a total length that fits `int`.

`common(other, l, r, a, b, suffix)`, `lcp` and `lcs` binary-search the common prefix or suffix length of two ranges under equal bases in O(log(k + 1)) for the shorter length k; answers are Monte Carlo. No lexicographic comparison is offered (row `22` owns `compareSubstrings`).

Costs: construction O(n) time and three arrays of n + 1 eight-byte words; `get`, `reverseGet` O(1). Copies are independent; assign a moved-from object before querying it.

Collision model: for two fixed distinct equal-length sequences of length L with injective codes, the difference is a nonzero polynomial of degree at most L - 1. With bases uniform in the stated ranges, the root bound gives a collision probability at most `(L - 1) / (p - 257)` per prime for `KIND = 0` (the product for both), and at most `(L - 1) / (2^61 - 259)` for `KIND = 2`. Batches use a union bound; an LCP/LCS error needs a false positive on its binary-search path. Inputs must be independent of the base; fixed defaults and seeded generators are reproducibility conveniences, not adaptive-adversary protection. The `2^64` ring has no root bound: the length-1024 Thue–Morse word and its complement collide for every odd base (their difference is `±prod(1 - b^(2^i), i < 10)`, which contains at least 64 factors of two).

Correctness: Horner prefixes satisfy `H[i+1] = H[i] * base + code(i)`; `H[r] - H[l] * base^(r-l)` is the normalized interval polynomial, reversed prefixes apply the same identity to `[n - r, n - l)`, and concatenation shifts the left polynomial by the right length. Prime residues stay normalized: sums stay below twice the modulus, 32-bit products fit `ulng`. For `2^61 - 1`, a product `t < 2^122` folds as `(t mod 2^61) + floor(t / 2^61) < 2p`, so one conditional subtraction normalizes it; `a + b < 2^62` likewise.

## Feature-to-test map

Runner: [`03-stringhash_tester.py`](<../../96-Local Testing/07-Strings/03-stringhash_tester.py>). Every check runs for all three fields; oracles are independent 128-bit Horner evaluations with codes reduced modulo each field, direct character scans and exact substring sets; the Python entry adds arbitrary-precision weighted power sums.

| Operation | Coverage |
|---|---|
| get, reverseGet, size, Digest tags | Exhaustive binary strings, random integer symbols and every interval; Python exact weighted power sums (bases from `randomBase`) for all three fields. |
| encode, add, subtract, multiply | All pairs of boundary and random residues (0, 1, 2, 256, middle, `p - 2`, `p - 1`) against 128-bit modular references; `encode` on codes around 1e9+7, 1e9+9, `2^32`, `2^61 - 1` and `2^64 - 1` (finding 6). |
| build (caller functor) | 30/300/2000 random code sequences drawn from those codes, every interval forward and reversed; the finding-3 regression (`2^32 + 1` versus `1`) now differs in every field. |
| hashOf | Every substring of every tested sequence (`vector<uint>`), byte strings including all 256 bytes, substrings and empty input. |
| power, concat | Split/join identities, right fragments beyond the table, `power(3n + 1)` against one multiplication, constructed `INT_MAX` digest followed by a checked overflow probe. |
| common, lcp, lcs | Direct scans on random ranges of independent objects, self ranges, 200,000-byte unary and middle-mismatch inputs. |
| defaultBase, randomBase, contexts | 2000 seeds per field inside the stated ranges, seed repeatability, `StringHash<true>` identity with `StringHash64`, copy/move/assignment, different-base and different-length rejection. |
| Digest ordering | `set<Digest>` size equals the exact distinct-substring count on random strings; `<` and `==` match the member-wise tuple order. |
| StringHash64, StringHash61 limits | Ten seeded odd bases reproduce the Thue–Morse word-ring collision; the double-prime and Mersenne fields separate the same pair. |

Checked builds run 17 assertion probes: base bounds and parity for all three fields, the `KIND = 0` alphabet in the constructor and in `hashOf`, negative `build` length, invalid ranges, mixed contexts, negative power and concatenated-length overflow. Finite tests do not establish absence of collisions; the collision model is the guarantee.

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode quick --seed 1  # PASS, 2 configurations
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 1  # PASS, 3 configurations, 1,589,416 checks plus 7,053 Python exact intervals each, 17 assertion probes
CXX=g++-14 python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 2  # PASS, 3 configurations, 1,661,404 checks plus 6,155 Python intervals each (`CXX=g++-14`)
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 9,467,596 checks plus 7,189 Python intervals each
python3 '96-Local Testing/07-Strings/08-palindrome_queries_tester.py' --mode full --seed 1  # PASS (dependent maybePalindrome)
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizers
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 9 suites
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The suite was also run in full mode before any change (seed 1, all three configurations PASS) as the re-audit baseline.

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

At 4096 bytes and 1024 queries the pipelines were 0.33, 0.12–0.13, 0.24–0.25 and 0.31–1.23 ms respectively. The Mersenne field is about 15–20% faster than the double-prime default with a single 61-bit field; the word ring is fastest but has the structured collision above. A rerun of the previous header on the same host (6.09–6.44 ms double-prime setup) shows that the new `encode` reduction costs nothing measurable; absolute times are about 2.5–3 times the 2026-09-28 record on this shared host. Auxiliary storage is `24 * (n + 1)` bytes plus the object for every field. No dispatch threshold or universal winner is claimed.

## Sources

Code is independently implemented.

- [cp-algorithms, String Hashing](https://cp-algorithms.com/string/string-hashing.html): polynomial encoding, substring extraction, multiple moduli, the warning about modulo-`2^64` collisions.
- [KACTL Hashing.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Hashing.h): prefix powers, interval subtraction, `hashString`-style digest of an outside string, Thue–Morse warning.
- [maspypy `rolling_hash.hpp` and `rolling_hash_field.hpp`](https://github.com/maspypy/library/blob/main/string/rolling_hash.hpp): object-local base, concatenation, substring LCP, `eval`; the `2^61 - 1` field variant.
- [Nyaan `internal/internal-hash.hpp`](https://github.com/NyaanNyaan/library/blob/master/internal/internal-hash.hpp) (fetched 2026-10-08): Mersenne folding `(t & p) + (t >> 61)` and primitive-root base selection over the factors of `2^61 - 2`.
- [hitonanode `rolling_hash_1d.hpp`](https://hitonanode.github.io/cplib-cpp/string/rolling_hash_1d.hpp): digest ordering for container keys.
- Catalog sweep 2026-10-08 in [00-sources.md](00-sources.md). `OLD` contains container hashes only (Miscellaneous), no rolling hash.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): appending to a built hash (`push_back`; reversed prefixes cannot extend in O(1), row `23` owns appendable and deque hashes), a field-generic `StringHashField<F>`, several Mersenne bases, `std::hash` for digests, lexicographic substring comparison (row `22`), Rabin–Karp windows (row `16`).
- Hash answers stay Monte Carlo; `08-palindrome_queries.hpp` `maybePalindrome` now takes `StringHash<KIND>` (one-token change of its template parameter in this package, required because `bool` cannot be deduced from an `int` parameter); it compiles for all three fields, but its P017 tester exercises only `KIND` 0 and 1. That header's own brace and comment-cap restyle remains with P017.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-09-28: double-prime and word variants verified under the previous system (P016). 2026-10-08 re-audit: template parameter `bool WRAP64` became `int KIND`; `StringHash61`, `hashOf` and Digest ordering added; `/reaudit-review` findings 3 (codes reduced modulo the field), 4 (StringHash61), 6 (direct helper and `build` tests), 9 (`U: NA`) and 10 (alphabet checked once at entry) fixed; contracts moved out of the header.
