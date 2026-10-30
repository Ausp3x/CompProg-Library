# 01-random.hpp — evidence

`01-random.hpp` (batch MI01, package P013) provides `Random`, an MT19937-64 engine with explicit/clock seeds, a URBG interface, unbiased inclusive integer draws over full 64-bit domains, `randDouble` on `[0,1)` and `[l,r)`, Fisher–Yates `shuffle`, and the global `rng`. Advanced random sampling is owned by MI05 (`18`). Dependency: P002 (template).

## Contracts

### Random

- Engine: public `std::mt19937_64 rng`. `Random()` seeds from `steady_clock` ticks, a convenient varying seed with no entropy or uniqueness guarantee. `Random(ulng)` accepts every 64-bit seed; record an explicit seed to replay, and reseed by assignment, `g = Random(x)`. Copy and move preserve engine state. Callers synchronize shared objects.
- URBG: `result_type = ulng`, `min() = 0`, `max() = 2^64 - 1`, `operator()()` returns the next engine word. `Random` therefore satisfies `std::uniform_random_bit_generator` and works with `std::shuffle`, `std::sample` and `<random>` distributions. Those standard algorithms are implementation-defined in how they consume words; only the library's own methods replay across standard libraries.
- `randLng(l, r)`, `randUlng(l, r)`: inclusive bounds over the entire representable domains, including singleton and full-width intervals; precondition `l <= r` (asserted). The helper `randBelow(n)` behind them and `shuffle`: `n > 0`, result in `[0, n)`.
- `randDouble()`: `(word >> 11) * 2^-53`, uniform over the 2^53 multiples of 2^-53 in `[0, 1)`; one word per call.
- `randDouble(l, r)`: finite `l < r` with `r - l` finite (asserted). Returns `l + (r - l) * u` from `randDouble()`, redrawing while the rounded value equals or exceeds `r`, so the result is in `[l, r)`. It is the rounded affine image of the 2^-53 grid, not exactly uniform over all doubles in the interval. Expected draws are below two whenever the interval spans at least two doubles; the degenerate one-ulp interval `[l, nextafter(l))` always returns `l` after expected two draws. Bit-exact replay of this overload assumes the compiler does not contract `l + (r - l) * u` into an FMA (true for the tested `-O2` x86-64 builds without `-march`); the `[l, r)` guarantee holds either way.
- `shuffle(first, last)`: valid mutable random-access range (proxy and move-only elements allowed), not reversed (asserted); Fisher–Yates with `randBelow`, expected O(n) time and O(1) workspace; consumes no words for sizes zero and one.
- `inline Random rng`: the legacy global convenience object, one instance across translation units.
- Failure model: bounded draws are Las Vegas (rejection loop with no finite worst-case bound under the ideal-word model); MT19937-64 is not cryptographic.

Correctness: Lemire's mapping puts `M = 2^64`, `t = M mod n`, `p = x * n`, accepts when `p mod M >= t` and returns `floor(p / M)`. Every output in `[0, n)` has exactly `floor(M / n)` accepted preimages, so acceptance adds no bias to uniform independent words; at least half the words are accepted, so expected work is below two draws. Unsigned subtraction computes the width modulo 2^64, width zero denoting the full domain; signed offsets are added in `lll` and converted only once the result fits. `randDouble()` uses the top 53 bits; each grid point has exactly 2^11 preimages and the product with 2^-53 is exact. For `randDouble(l, r)`, `u >= 0` and monotone rounding give `>= l`, the explicit `< r` test gives the upper bound, and a redraw needs `u` within a few ulps of one unless the interval spans a single double. Fisher–Yates choice sequences are in bijection with the `n!` permutations.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `seed(x)` | `g = Random(x)` |
| `randInt(l, r)` | `int(randLng(l, r))`, the same value from the same words |

## Feature-to-test map

| Feature | Verification |
|---|---|
| MT engine, explicit/default seeds, reseed by assignment, copy/move, global `rng` | MT19937-64 10000th-word fixture, replay/state comparisons, assignment reseed against a fresh `std::mt19937_64`, default constructor smoke, global identity across translation units |
| URBG interface | `static_assert(std::uniform_random_bit_generator<Random>)`, `min`/`max` values, `operator()` equals the engine sequence, `std::shuffle` and `std::uniform_int_distribution` accept `Random` |
| Bounded draws, full signed/unsigned width | Arbitrary-precision Python mapping against raw engine words; minimum/maximum/singleton/full-width intervals, powers of two and neighbours, and the former `randInt` intervals `[-2^31, 2^31)`, `[-7, 9]`, `[0, 0]` through `randLng`; full run exercises 22,335 rejected words |
| randDouble (both overloads) | Exact Python oracle replaying raw words: `(w >> 11) * 2^-53` and the affine map with the rejection loop over `[0,1)`, `[-2.5,7.5)`, one-ulp `[1, nextafter(1))`, `[-8e307, 8e307)`, subnormal-scale `[1e-300, 3e-300)`, `[-5, -4.999)`, `[0, 5e-324)`; full run 200 cases with 15,054 redraws; C++ range and fixed-seed mean regression checks |
| Rejection uniformity argument | Exhaustive reduced words 1–10 bits for every nonzero width |
| Fisher–Yates | Exact Python shuffle oracle, exhaustive abstract choices through n=7, empty/singleton/duplicates/deque/proxy/move-only cases, replay |
| Invalid Random preconditions | Six checked assertion probes: zero bound, reversed lng/ulng intervals, reversed shuffle range, empty and infinite-width `randDouble` intervals |
| Header/linkage | Each tester includes only its header; optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns aggregates. |

Modes: quick runs optimized and checked builds, 5,000 random draws and reduced words through 8 bits. Full adds ASan/UBSan, 100,000 C++ draws, 725 exact Random oracle cases and 200 `randDouble` cases per configuration, and reduced words through 10 bits. Stress raises C++ workloads to 1,000,000 draws. Fixed-seed frequency checks are regressions, not uniformity proofs.

## Commands and results

P013 prune run, 2026-10-12, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized, checked and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/06-Miscellaneous/_00_runner.py>).

| Command | Result |
|---|---|
| `01-random_tester.py --mode full --seed 20260927` | PASS, 3 configurations, 725 exact Random oracle cases (22,335 rejected words), 200 `randDouble` cases (15,054 redraws), 6 assertion probes, `MEMORY peak=429MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=446MB` |
| Other P013 suites, the call-site suites (`07-ordered_set`, `08-sequence_algorithms`, `12-enumeration`), `02-integration.py` and `03-consistency.py` | PASS; details and the package command loop in [04-compression.md](04-compression.md) |

## Sources

- Daniel Lemire, [*Fast Random Integer Generation in an Interval*, arXiv:1805.10941v4](https://arxiv.org/html/1805.10941v4): Algorithm 5, Lemma 4.1 and the Fisher–Yates description (read 2026-09-27).
- [cppreference `std::mersenne_twister_engine`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine.html): `mt19937_64` parameters and the 10000th-value fixture (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [Nyaan rng](https://nyaannyaan.github.io/library/misc/rng.hpp), [maspypy random_real](https://raw.githubusercontent.com/maspypy/library/main/random/random_real.hpp) and [base](https://raw.githubusercontent.com/maspypy/library/main/random/base.hpp), [hitonanode rand_nondeterministic](https://hitonanode.github.io/cplib-cpp/random/rand_nondeterministic.hpp), [OI Wiki random](https://oi-wiki.org/misc/random/), [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h). They motivated the URBG interface, the `[l, r)` real overload (Nyaan's `rng() * 2^-64` can round up to 1.0, which the 53-bit form avoids) and the `gp_hash_table` alias.
- `OLD/algorithms.cpp` lines 856–870: read in full for legacy accounting; every legacy feature maps to an active operation (`randInt` through the Legacy mapping above).

## Limits and handoffs

- Randomness is noncryptographic (MT19937-64). `randDouble(l, r)` replays bit-exactly only without FMA contraction.
- Advanced random sampling belongs to `18`; `randomPermutation(n)` is `iota` plus `shuffle` (see [00-notes.md](00-notes.md)).
- No benchmark: no reduction backend or tuned threshold exists.

## History

- 2026-09-27: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, URBG interface and `randDouble` added, contracts moved out of code comments, brace style normalized; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: prune, removed `seed(x)` and `randInt` (cases moved to assignment reseeding and `randLng`); full suite passed on g++ and g++-14.
- 2026-10-30: SUP18 arm64 portability (user decision 2026-10-30, fixed in P250): the tester builds with `-ffp-contract=off` (a new per-tester flag of the `06-Miscellaneous` runner), so the exact `randDouble(l, r)` oracle holds where g++ would fuse the multiply-add (aarch64); x86-64 output unchanged; quick passed on x86-64 and on aarch64 under qemu-user, full passed on g++ and g++-14.
