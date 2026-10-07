# 02-customhash.hpp — evidence

`02-customhash.hpp` (batch MI01, package P013) provides seeded SplitMix64 hashing for scalars, 128-bit integers, pointers, strings, pairs, tuples and ordered ranges, with the `safe_unordered_map`/`safe_unordered_set` and `safe_gp_hash_table` aliases. Universal/tabulation/adversarial-resistant hash families are owned by MI05 (`19`). Dependency: P002 (template).

## Contracts

### CustomHash, safe_unordered_map, safe_unordered_set, safe_gp_hash_table

- Seed: `CustomHash()` copies the inline process seed `CustomHash::rnd`, formed from clock ticks xor the address of `rnd`; this is variation, not secret entropy. `CustomHash(ulng)` gives a reproducible seed. Hash values replay on x86-64 Linux; string chunks use the platform's little-endian representation. Keep the seed fixed while a container holds keys; copies and rehashes retain it.
- Scalars: `splitMix64` (Vigna's increment and finalizer) is a bijection of 64-bit words, so distinct 64-bit values never collide for a fixed seed. 128-bit keys mix both words. Enums and values convertible to `ulng` hash their conversion; the conversion must be defined and equal values must convert equally (no NaN or out-of-range floating conversions; fractional values collide after truncation).
- Text: `string`, `string_view` and NUL-terminated character arrays hash their bytes, including embedded NUL in explicit-length views; chunks are read with `memcpy`, so unaligned buffers are valid, and the tail is zero-padded before the length is mixed. Pointer variables (including `char *` and function pointers) hash their address without dereferencing; `nullptr` is supported.
- Composites: `pair`, `tuple` and const-iterable input ranges mix child hashes in order; tuples and ranges also mix their length. Equality must respect the visited order, so never use an unordered container as a key.
- Cost: O(1) scalars, O(L) strings, O(k) composites for `k` recursively visited elements, O(nesting depth) stack, O(1) stored state.
- Guarantee: no universal or cryptographic collision bound; a known seed permits collision construction. The containers keep exact key equality, so collisions cost time, never correctness: expected O(1) table operations can degrade to O(n) per operation.
- `safe_gp_hash_table<K, V>` is `__gnu_pbds::gp_hash_table<K, V, CustomHash>` (open addressing, linear probing, power-of-two sizes). Construct with `safe_gp_hash_table<K, V>(CustomHash(seed))` for a reproducible seed. Under `-D_GLIBCXX_DEBUG`, PB-DS enables its own debug validation, which revalidates every key after each operation (roughly quartic total work); do not use it in debug-mode stress runs.

Correctness: SplitMix64 is a composition of invertible steps (addition modulo 2^64, xorshift-right, multiplication by odd constants). The former pair shift/xor combiner lost bits and had a deterministic collision, kept as a regression test.

## Feature-to-test map

| Feature | Verification |
|---|---|
| SplitMix/scalar/128-bit/combine | Arbitrary-precision Python fixtures, boundary/random values, inverse-mixer round trip, 128-bit high-word regression, scalar injectivity sample |
| Conversions, enums, copies/seeds | Signed/unsigned extrema, scoped enum, bool, custom conversion, valid floats/signed zero, copy/move and fixed process seed |
| Pointers and strings | Pointee mutation, distinct addresses with equal values, typed/void/null/function pointers, C-string pointer/array distinction; lengths across every 8-byte boundary, empty/binary/high-byte/NUL/unaligned buffers, Python byte-order oracle |
| Pair/tuple/ranges and container aliases | Pair-collision regression; empty/nested tuples; vector/list/forward-list/array/span/vector-bool; seeded `safe_unordered_map` against `std::map`; binary-string set and rehash seed lifetime |
| safe_gp_hash_table | 100,000 seeded insert/erase operations (40 under `_GLIBCXX_DEBUG`) on keys including `INT64_MIN + [0, 2000]` compared with `std::map`, erase results, final contents, stored seed; default-seed pair keys |
| Header/linkage | Each tester includes only its header; optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns aggregates. |

Modes: quick runs optimized and checked builds with smaller hash dictionaries. Full adds ASan/UBSan, 100,000 C++ hash operations and 25,682 hash oracle cases per configuration. Stress raises C++ workloads to 500,000 hash operations. Oracle checks survive `-DNDEBUG`. The tester has no assertion probes.

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates, two-TU links)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Sources

- Sebastiano Vigna, [*splitmix64.c*](https://prng.di.unimi.it/splitmix64.c), public domain: constants and finalizer match the header (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h) motivated the `gp_hash_table` alias. The sweep's random sources are listed in [01-random.md](01-random.md).
- Local `97-Legacy/02-customhash.cpp`: read in full for legacy accounting; every retained public name is present in the active header.

## Limits and handoffs

- Hashing is noncryptographic and has no universal collision bound.
- Universal/tabulation/adversarial-resistant hash families belong to `19`.
- No benchmark: no reduction backend or tuned threshold exists. No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-27: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, `safe_gp_hash_table` added, findings 5–7 (`std::uintptr_t`, string-length symbol, braces) fixed, contracts moved out of code comments; full, stress and integration passed.
