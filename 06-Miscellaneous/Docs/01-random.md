# 01-random.hpp — evidence

`01-random.hpp` (batch MI01, package P013) provides `Random`, an MT19937-64 engine with explicit/clock seeds, a URBG interface, unbiased inclusive integer draws over full 64-bit domains, `randDouble` on `[0,1)` and `[l,r)`, Fisher–Yates `shuffle`, and the global `rng`. Advanced random sampling is owned by MI05 (`18`). Dependency: P002 (template).

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

Correctness: Lemire's mapping puts `M = 2^64`, `t = M mod n`, `p = x * n`, accepts when `p mod M >= t` and returns `floor(p / M)`. Every output in `[0, n)` has exactly `floor(M / n)` accepted preimages, so acceptance adds no bias to uniform independent words; at least half the words are accepted, so expected work is below two draws. Unsigned subtraction computes the width modulo 2^64, width zero denoting the full domain; signed offsets are added in `lll` and converted only once the result fits. `randDouble()` uses the top 53 bits; each grid point has exactly 2^11 preimages and the product with 2^-53 is exact. For `randDouble(l, r)`, `u >= 0` and monotone rounding give `>= l`, the explicit `< r` test gives the upper bound, and a redraw needs `u` within a few ulps of one unless the interval spans a single double. Fisher–Yates choice sequences are in bijection with the `n!` permutations.

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
| Header/linkage | Each tester includes only its header; optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns aggregates. |

Modes: quick runs optimized and checked builds, 5,000 random draws and reduced words through 8 bits. Full adds ASan/UBSan, 100,000 C++ draws, 750 exact Random oracle cases and 200 `randDouble` cases per configuration, and reduced words through 10 bits. Stress raises C++ workloads to 1,000,000 draws. Oracle checks survive `-DNDEBUG`. Fixed-seed frequency checks are regressions, not uniformity proofs.

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Sources

- Daniel Lemire, [*Fast Random Integer Generation in an Interval*, arXiv:1805.10941v4](https://arxiv.org/html/1805.10941v4): Algorithm 5, Lemma 4.1 and the Fisher–Yates description (read 2026-09-27).
- [cppreference `std::mersenne_twister_engine`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine.html): `mt19937_64` parameters and the 10000th-value fixture (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [Nyaan rng](https://nyaannyaan.github.io/library/misc/rng.hpp), [maspypy random_real](https://raw.githubusercontent.com/maspypy/library/main/random/random_real.hpp) and [base](https://raw.githubusercontent.com/maspypy/library/main/random/base.hpp), [hitonanode rand_nondeterministic](https://hitonanode.github.io/cplib-cpp/random/rand_nondeterministic.hpp), [OI Wiki random](https://oi-wiki.org/misc/random/), [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h). They motivated the URBG interface, the `[l, r)` real overload (Nyaan's `rng() * 2^-64` can round up to 1.0, which the 53-bit form avoids) and the `gp_hash_table` alias.
- Local `97-Legacy/01-random.cpp`: read in full for legacy accounting; every retained public name is present in the active header.

## Limits and handoffs

- Randomness is noncryptographic (MT19937-64). `randDouble(l, r)` replays bit-exactly only without FMA contraction.
- Advanced random sampling belongs to `18`; `randomPermutation(n)` is `iota` plus `shuffle` (see [00-notes.md](00-notes.md)).
- No benchmark: no reduction backend or tuned threshold exists. No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-27: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, URBG interface and `randDouble` added, contracts moved out of code comments, brace style normalized; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
