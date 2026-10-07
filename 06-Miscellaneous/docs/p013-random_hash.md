# Random and custom hash — MI01 contracts and verification

First verified 2026-09-27 for P013. Re-audited 2026-10-07: the URBG interface, `randDouble` and `safe_gp_hash_table` were added, the `/reaudit-review` style findings (unqualified `uintptr_t`, string-length symbol) were fixed, and the header contracts moved here from code comments. Advanced random sampling and universal/tabulation/adversarial-resistant hash families remain owned by MI05 (`18`, `19`); they are not claims of these headers.

## Contracts

### Random

- Engine: public `std::mt19937_64 rng`. `Random()` seeds from `steady_clock` ticks, a convenient varying seed with no entropy or uniqueness guarantee. `Random(ulng)` and `seed(ulng)` accept every 64-bit seed; record an explicit seed to replay. Copy and move preserve engine state. Single-threaded use is the baseline; callers synchronize shared objects.
- URBG: `result_type = ulng`, `min() = 0`, `max() = 2^64 - 1`, `operator()()` returns the next engine word. `Random` therefore satisfies `std::uniform_random_bit_generator` and works with `std::shuffle`, `std::sample` and `<random>` distributions. Those standard algorithms are implementation-defined in how they consume words; only the library's own methods replay across standard libraries.
- `randInt(l, r)`, `randLng(l, r)`, `randUlng(l, r)`: inclusive bounds over the entire representable domains, including singleton and full-width intervals; precondition `l <= r` (asserted). `randBelow(n)`: `n > 0`, result in `[0, n)`.
- `randDouble()`: `(word >> 11) * 2^-53`, uniform over the 2^53 multiples of 2^-53 in `[0, 1)`; one word per call.
- `randDouble(l, r)`: finite `l < r` with `r - l` finite (asserted). Returns `l + (r - l) * u` from `randDouble()`, redrawing while the rounded value equals or exceeds `r`, so the result is in `[l, r)`. It is the rounded affine image of the 2^-53 grid, not exactly uniform over all doubles in the interval. Expected draws are below two whenever the interval spans at least two doubles; the degenerate one-ulp interval `[l, nextafter(l))` always returns `l` after expected two draws. Bit-exact replay of this overload assumes the compiler does not contract `l + (r - l) * u` into an FMA (true for the tested `-O2` x86-64 builds without `-march`); the `[l, r)` guarantee holds either way.
- `shuffle(first, last)`: valid mutable random-access range (proxy and move-only elements allowed), not reversed (asserted); Fisher–Yates with `randBelow`, expected O(n) time and O(1) workspace; consumes no words for sizes zero and one.
- `inline Random rng`: the legacy global convenience object, one instance across translation units.
- Failure model: bounded draws are Las Vegas (rejection loop with no finite worst-case bound under the ideal-word model); MT19937-64 is not cryptographic.

### CustomHash, safe_unordered_map, safe_unordered_set, safe_gp_hash_table

- Seed: `CustomHash()` copies the inline process seed `CustomHash::rnd`, formed from clock ticks xor the address of `rnd`; this is variation, not secret entropy. `CustomHash(ulng)` gives a reproducible seed. Hash values replay on x86-64 Linux; string chunks use the platform's little-endian representation. Keep the seed fixed while a container holds keys; copies and rehashes retain it.
- Scalars: `splitMix64` (Vigna's increment and finalizer) is a bijection of 64-bit words, so distinct 64-bit values never collide for a fixed seed. 128-bit keys mix both words. Enums and values convertible to `ulng` hash their conversion; the conversion must be defined and equal values must convert equally (no NaN or out-of-range floating conversions; fractional values collide after truncation).
- Text: `string`, `string_view` and NUL-terminated character arrays hash their bytes, including embedded NUL in explicit-length views; chunks are read with `memcpy`, so unaligned buffers are valid, and the tail is zero-padded before the length is mixed. Pointer variables (including `char *` and function pointers) hash their address without dereferencing; `nullptr` is supported.
- Composites: `pair`, `tuple` and const-iterable input ranges mix child hashes in order; tuples and ranges also mix their length. Equality must respect the visited order, so never use an unordered container as a key.
- Cost: O(1) scalars, O(L) strings, O(k) composites for `k` recursively visited elements, O(nesting depth) stack, O(1) stored state.
- Guarantee: no universal or cryptographic collision bound; a known seed permits collision construction. The containers keep exact key equality, so collisions cost time, never correctness: expected O(1) table operations can degrade to O(n) per operation.
- `safe_gp_hash_table<K, V>` is `__gnu_pbds::gp_hash_table<K, V, CustomHash>` (open addressing, linear probing, power-of-two sizes). Construct with `safe_gp_hash_table<K, V>(CustomHash(seed))` for a reproducible seed. Under `-D_GLIBCXX_DEBUG`, PB-DS enables its own debug validation, which revalidates every key after each operation (roughly quartic total work); do not use it in debug-mode stress runs.

## Correctness arguments

Lemire's mapping: put `M = 2^64`, `t = M mod n`, `p = x * n`. Accept when `p mod M >= t` and return `floor(p / M)`. Every output in `[0, n)` has exactly `floor(M / n)` accepted preimages, so conditioning on acceptance adds no bias to uniform independent words. At least half the words are accepted, so expected work is below two draws. Unsigned subtraction computes the interval width modulo 2^64, and width zero denotes exactly the full domain. Signed offsets are added in `lll` and converted only after the result is known to fit; no signed overflow occurs.

`randDouble()` uses the top 53 bits, each multiple of 2^-53 in `[0, 1)` has exactly 2^11 preimages, and the product with 2^-53 is exact. For `randDouble(l, r)`, `u >= 0` and round-to-nearest is monotone, so `l + (r - l) * u >= l`; the explicit `< r` test enforces the upper bound. A redraw happens only when rounding reaches `r`, which needs `u` within a few ulps of one unless the interval spans a single double.

Fisher–Yates chooses each final position uniformly among the remaining elements; the choice sequences are in bijection with the `n!` labeled permutations.

SplitMix64 is a composition of invertible steps (addition modulo 2^64, xorshift-right, multiplication by odd constants). The former pair shift/xor combiner lost bits and had a deterministic collision, kept as a regression test.

## Feature-to-test map

| Feature | Verification |
|---|---|
| MT engine, explicit/default seeds, reset, copy/move, legacy global | MT19937-64 10000th-word fixture, replay/state comparisons, default constructor smoke, global identity across translation units |
| URBG interface | `static_assert(std::uniform_random_bit_generator<Random>)`, `min`/`max` values, `operator()` equals the engine sequence, `std::shuffle` and `std::uniform_int_distribution` accept `Random` |
| Bounded draws, full signed/unsigned width | Arbitrary-precision Python mapping against raw engine words; minimum/maximum/singleton/full-width intervals, powers of two and neighbours; full run exercises 22,335 rejected words |
| randDouble (both overloads) | Exact Python oracle replaying raw words: `(w >> 11) * 2^-53` and the affine map with the rejection loop over `[0,1)`, `[-2.5,7.5)`, one-ulp `[1, nextafter(1))`, `[-8e307, 8e307)`, subnormal-scale `[1e-300, 3e-300)`, `[-5, -4.999)`, `[0, 5e-324)`; full run 200 cases with 15,054 redraws; C++ range and fixed-seed mean regression checks |
| Rejection uniformity argument | Exhaustive reduced words 1–10 bits for every nonzero width |
| Fisher–Yates | Exact Python shuffle oracle, exhaustive abstract choices through n=7, empty/singleton/duplicates/deque/proxy/move-only cases, replay |
| Invalid Random preconditions | Seven checked assertion probes: zero bound, reversed int/lng/ulng intervals, reversed shuffle range, empty and infinite-width `randDouble` intervals |
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
python3 '96-Local Testing/06-Miscellaneous/01-random_tester.py' --mode full --seed 20260927        # PASS, 3 configurations
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode full --seed 20260927    # PASS, 3 configurations
python3 '96-Local Testing/06-Miscellaneous/01-random_tester.py' --mode stress --seed 20260928 --configuration optimized      # PASS
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode stress --seed 20260928 --configuration optimized  # PASS
```

Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The first full customhash run after the re-audit failed under UBSan on a signed overflow in the new test's key formula (test code, not the header); the formula was corrected and the full run passed. No benchmark is required: no reduction backend or tuned threshold exists. No online submission was made.

## Sources

- Daniel Lemire, [*Fast Random Integer Generation in an Interval*, arXiv:1805.10941v4](https://arxiv.org/html/1805.10941v4): Algorithm 5, Lemma 4.1 and the Fisher–Yates description (read 2026-09-27).
- [cppreference `std::mersenne_twister_engine`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine.html): `mt19937_64` parameters and the 10000th-value fixture (2026-09-27).
- Sebastiano Vigna, [*splitmix64.c*](https://prng.di.unimi.it/splitmix64.c), public domain: constants and finalizer match the header (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [Nyaan rng](https://nyaannyaan.github.io/library/misc/rng.hpp), [maspypy random_real](https://raw.githubusercontent.com/maspypy/library/main/random/random_real.hpp) and [base](https://raw.githubusercontent.com/maspypy/library/main/random/base.hpp), [hitonanode rand_nondeterministic](https://hitonanode.github.io/cplib-cpp/random/rand_nondeterministic.hpp), [OI Wiki random](https://oi-wiki.org/misc/random/), [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h). They motivated the URBG interface, the `[l, r)` real overload (Nyaan's `rng() * 2^-64` can round up to 1.0, which the 53-bit form avoids) and the `gp_hash_table` alias.
- Local `97-Legacy/01-random.cpp` and `02-customhash.cpp`: read in full for legacy accounting; every retained public name is present in the active headers.
