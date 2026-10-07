# 02-customhash.hpp — evidence

`02-customhash.hpp` (batch MI01, package P013) provides seeded SplitMix64 hashing for scalars, 128-bit integers, pointers, strings, pairs, tuples and ordered ranges, with the `safe_unordered_map`/`safe_unordered_set` and `safe_gp_hash_table` aliases. First verified 2026-09-27 for P013. Re-audited 2026-10-07: `safe_gp_hash_table` was added, the `/reaudit-review` style findings (unqualified `uintptr_t`, string-length symbol) were fixed, and the header contracts moved here from code comments. Universal/tabulation/adversarial-resistant hash families remain owned by MI05 (`19`); they are not claims of this header. Dependency: P002 (template).

## Contracts

### CustomHash, safe_unordered_map, safe_unordered_set, safe_gp_hash_table

- Seed: `CustomHash()` copies the inline process seed `CustomHash::rnd`, formed from clock ticks xor the address of `rnd`; this is variation, not secret entropy. `CustomHash(ulng)` gives a reproducible seed. Hash values replay on x86-64 Linux; string chunks use the platform's little-endian representation. Keep the seed fixed while a container holds keys; copies and rehashes retain it.
- Scalars: `splitMix64` (Vigna's increment and finalizer) is a bijection of 64-bit words, so distinct 64-bit values never collide for a fixed seed. 128-bit keys mix both words. Enums and values convertible to `ulng` hash their conversion; the conversion must be defined and equal values must convert equally (no NaN or out-of-range floating conversions; fractional values collide after truncation).
- Text: `string`, `string_view` and NUL-terminated character arrays hash their bytes, including embedded NUL in explicit-length views; chunks are read with `memcpy`, so unaligned buffers are valid, and the tail is zero-padded before the length is mixed. Pointer variables (including `char *` and function pointers) hash their address without dereferencing; `nullptr` is supported.
- Composites: `pair`, `tuple` and const-iterable input ranges mix child hashes in order; tuples and ranges also mix their length. Equality must respect the visited order, so never use an unordered container as a key.
- Cost: O(1) scalars, O(L) strings, O(k) composites for `k` recursively visited elements, O(nesting depth) stack, O(1) stored state.
- Guarantee: no universal or cryptographic collision bound; a known seed permits collision construction. The containers keep exact key equality, so collisions cost time, never correctness: expected O(1) table operations can degrade to O(n) per operation.
- `safe_gp_hash_table<K, V>` is `__gnu_pbds::gp_hash_table<K, V, CustomHash>` (open addressing, linear probing, power-of-two sizes). Construct with `safe_gp_hash_table<K, V>(CustomHash(seed))` for a reproducible seed. Under `-D_GLIBCXX_DEBUG`, PB-DS enables its own debug validation, which revalidates every key after each operation (roughly quartic total work); do not use it in debug-mode stress runs.

## Correctness arguments

SplitMix64 is a composition of invertible steps (addition modulo 2^64, xorshift-right, multiplication by odd constants). The former pair shift/xor combiner lost bits and had a deterministic collision, kept as a regression test.

## Re-audit findings (P013, 2026-10-07)

| Header | Gap or finding | Resolution |
|---|---|---|
| `02` | Finding 5: unqualified `uintptr_t` | Now `std::uintptr_t` |
| `02` | Finding 6: string-hash comment used `n`, prose bounds | Struct line now reads `O(L) string, O(k) composite with k recursively visited elements` |
| all | Contracts in multi-line header comments over the 8% cap | Moved to `## Contracts` sections; headers pass the cap |
| all | `; }` closing braces in headers, testers, benchmark (finding 7) | Normalized; `03-consistency.py --braces` reports none |

The completeness sweep added `safe_gp_hash_table`.

## Feature-to-test map

| Feature | Verification |
|---|---|
| SplitMix/scalar/128-bit/combine | Arbitrary-precision Python fixtures, boundary/random values, inverse-mixer round trip, 128-bit high-word regression, scalar injectivity sample |
| Conversions, enums, copies/seeds | Signed/unsigned extrema, scoped enum, bool, custom conversion, valid floats/signed zero, copy/move and fixed process seed |
| Pointers and strings | Pointee mutation, distinct addresses with equal values, typed/void/null/function pointers, C-string pointer/array distinction; lengths across every 8-byte boundary, empty/binary/high-byte/NUL/unaligned buffers, Python byte-order oracle |
| Pair/tuple/ranges and container aliases | Pair-collision regression; empty/nested tuples; vector/list/forward-list/array/span/vector-bool; seeded `safe_unordered_map` against `std::map`; binary-string set and rehash seed lifetime |
| safe_gp_hash_table | 100,000 seeded insert/erase operations (40 under `_GLIBCXX_DEBUG`) on keys including `INT64_MIN + [0, 2000]` compared with `std::map`, erase results, final contents, stored seed; default-seed pair keys |
| Header/linkage | Each tester includes only its header; optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns aggregates. |

Modes: quick runs optimized and checked builds, 5,000 random draws, smaller hash dictionaries and reduced words through 8 bits. Full adds ASan/UBSan, 100,000 C++ draws and hash operations, 750 exact Random oracle cases, 200 `randDouble` cases and 25,682 hash oracle cases per configuration, and reduced words through 10 bits. Stress raises C++ workloads to 1,000,000 draws and 500,000 hash operations. Oracle checks survive `-DNDEBUG`. Fixed-seed frequency checks are regressions, not uniformity proofs.

## Commands and results

GCC 16.2.1, GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H, 2026-10-07:

```sh
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode full --seed 20260927    # PASS, 3 configurations
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode stress --seed 20260928 --configuration optimized  # PASS
```

Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The first full customhash run after the re-audit failed under UBSan on a signed overflow in the new test's key formula (test code, not the header); the formula was corrected and the full run passed. No benchmark is required: no reduction backend or tuned threshold exists. No online submission was made. The customhash tester has no assertion probes (0 of the package's 68). Two-translation-unit link checks run in this tester (both link orders) and in `02-integration.py`.

P013 package runs, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H, 2026-10-07. Baseline before the re-audit changes: all seven P013 suites (`01`–`07`) passed full mode with seed 20260927. After the changes:

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

`02-integration.py` also builds every header alone and the Basic/All aggregates.

## Benchmarks

No benchmark is required: no reduction backend or tuned threshold exists.

## Sources

- Sebastiano Vigna, [*splitmix64.c*](https://prng.di.unimi.it/splitmix64.c), public domain: constants and finalizer match the header (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h) motivated the `gp_hash_table` alias. The sweep's random sources are listed in [01-random.md](01-random.md).
- Local `97-Legacy/02-customhash.cpp`: read in full for legacy accounting; every retained public name is present in the active header.

## Limits and handoffs

- Hashing is noncryptographic and has no universal collision bound.
- Universal/tabulation/adversarial-resistant hash families belong to `19`.
- No online submission was made and no judge acceptance is claimed.
