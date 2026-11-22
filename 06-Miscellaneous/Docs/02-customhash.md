# 02-customhash.hpp — Evidence

`02-customhash.hpp` (batch MI01, package P013) provides seeded SplitMix64 hashing for scalars, 128-bit integers, pointers, strings, pairs, tuples and ordered ranges, with the `safe_unordered_map`, `safe_unordered_set` and `safe_gp_hash_table` aliases. Universal, tabulation and adversarial-resistant hash families belong to MI05 (`19`). Dependency: P002 (template).

## Contracts

### CustomHash, safe_unordered_map, safe_unordered_set, safe_gp_hash_table

- Seed: `CustomHash()` copies the inline process seed `CustomHash::rnd`, formed from clock ticks xor the address of a function-local static in its initializing lambda (GCC 14 rejects taking `&rnd` inside its own initializer); this gives variation, not secret entropy. `CustomHash(ulng)` gives a reproducible seed. Hash values replay on x86-64 Linux; string chunks use the platform's little-endian representation. Keep the seed fixed while a container holds keys; copies and rehashes retain it.
- Scalars: `splitMix64` (Vigna's increment and finalizer) is a bijection of 64-bit words, so distinct 64-bit values never collide for a fixed seed. 128-bit keys mix both words. Enums and values convertible to `ulng` hash their conversion, which must be defined and must map equal values equally (no NaN or out-of-range floating conversions; fractional values collide after truncation).
- Text: `string`, `string_view` and NUL-terminated character arrays hash their bytes, including embedded NUL in explicit-length views. Chunks are read with `memcpy`, so unaligned buffers are valid, and the tail is zero-padded before the length is mixed. Pointer variables (`char *` and function pointers included) hash their address without dereferencing; `nullptr` is supported.
- Composites: `pair`, `tuple` and const-iterable input ranges mix child hashes in order; tuples and ranges also mix their length. Equality must respect the visited order, so an unordered container is never a key.
- Cost: O(1) for scalars, O(L) for strings, O(k) for composites with `k` recursively visited elements; O(nesting depth) stack and O(1) stored state.
- Guarantee: no universal or cryptographic collision bound; a known seed permits collision construction. The containers keep exact key equality, so collisions cost time, never correctness: expected O(1) table operations can degrade to O(n) each.
- `safe_gp_hash_table<K, V>` is `__gnu_pbds::gp_hash_table<K, V, CustomHash>` (open addressing, linear probing, power-of-two sizes). `safe_gp_hash_table<K, V>(CustomHash(seed))` gives a reproducible seed. Under `-D_GLIBCXX_DEBUG`, PB-DS revalidates every key after each operation (roughly quartic total work), so debug-mode stress runs must not use it.

Correctness: SplitMix64 composes invertible steps (addition modulo 2^64, xorshift-right, multiplication by odd constants).

## Feature-to-Test Map

| Feature | Verification |
|---|---|
| SplitMix, scalars, 128-bit, combine | Arbitrary-precision Python fixtures, boundary and random values, inverse-mixer round trip, 128-bit high-word regression, scalar injectivity sample |
| Conversions, enums, copies and seeds | Signed and unsigned extrema, scoped enum, bool, custom conversion, valid floats and signed zero, copy/move and the fixed process seed |
| Pointers and strings | Pointee mutation, distinct addresses with equal values, typed, void, null and function pointers, C-string pointer versus array; lengths across every 8-byte boundary, empty, binary, high-byte, NUL and unaligned buffers, Python byte-order oracle |
| Pairs, tuples, ranges and container aliases | Pair-collision regression; empty and nested tuples; vector, list, forward-list, array, span and vector-bool; seeded `safe_unordered_map` against `std::map`; binary-string set and seed lifetime across rehash |
| `safe_gp_hash_table` | 100,000 seeded insert/erase operations (40 under `_GLIBCXX_DEBUG`) on keys including `INT64_MIN + [0, 2000]`, compared with `std::map` on erase results, final contents and the stored seed; default-seed pair keys |
| Header and linkage | Each tester includes only its header; an optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns the aggregates. |

Modes: quick runs the optimized and checked builds with smaller hash dictionaries. Full adds ASan/UBSan, 100,000 C++ hash operations and 25,682 hash oracle cases per configuration. Stress raises the C++ workloads to 500,000 hash operations. The tester has no assertion probes.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14.7, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 823 MB; optimized 0.3 s compile + 1.2 s run, checked 14.2 s + 1.2 s, ASan-UBSan 18.1 s + 2.3 s; peak 969 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 886 MB; optimized 0.3 s + 2.0 s, checked 23.6 s + 1.2 s, ASan-UBSan 25.5 s + 2.0 s; peak 1093 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 24.5 s + 16.1 s, checked 19.4 s + 0.3 s; peak 822 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 833 MB; optimized 0.1 s + 1.7 s, checked 5.6 s + 1.4 s, ASan-UBSan 6.7 s + 3.1 s; peak 550 MB |
| 2026-10-12 (P013 prune) | `python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode full --seed 20260927` | PASS, 3 configurations, 25,682 exact Python hash cases, 100,000 map/set cases, both two-TU link orders, `MEMORY peak=517MB` |
| 2026-10-12 (P013 prune) | the same command with `CXX=g++-14` | PASS, the same counts, `MEMORY peak=545MB` |
| 2026-10-12 (P013 prune) | the other P013 suites, the call-site suites (`07-ordered_set`, `08-sequence_algorithms`, `12-enumeration`), `02-integration.py` and `03-consistency.py` | PASS (2026-10-12) |

## Sources

- Sebastiano Vigna, [*splitmix64.c*](https://prng.di.unimi.it/splitmix64.c), public domain: the constants and finalizer match the header (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h) motivated the `gp_hash_table` alias; the sweep's random sources are listed in [01-random.md](01-random.md).
- `OLD/algorithms.cpp` lines 872–933, read in full for legacy accounting: every retained public name is in the active header.

## Limits and Handoffs

- Hashing is noncryptographic and has no universal collision bound.
- Universal, tabulation and adversarial-resistant hash families belong to `19`.
- No benchmark: there is no reduction backend or tuned threshold.

## Aliases

Search aliases (P263, grab --find): 哈希表, 无序关联式容器, 自定义哈希, 防卡哈希, 哈希函数, 防哈希攻击.

## History

- 2026-09-27: P013 first verification; full and stress passed.
- 2026-10-07: P013 re-audit added `safe_gp_hash_table`, fixed 3 findings and moved contracts out of code comments; full, stress and integration passed.
- 2026-10-09: P227 fixed the process seed initializer for GCC 14; full passed on g++ 16.2.1 and g++-14 14.4.1.
- 2026-10-12: P013 prune run (header unchanged); full passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese search aliases (OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
