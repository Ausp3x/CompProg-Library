# 01-random.hpp — Evidence

`01-random.hpp` (batch MI01, package P013) provides `Random`: an MT19937-64 engine with explicit or clock seeds, a URBG interface, unbiased inclusive integer draws over the full 64-bit domains, `randDouble` on `[0,1)` and `[l,r)`, Fisher–Yates `shuffle`, and the global `rng`. Advanced random sampling belongs to MI05 (`18`). Dependency: P002 (template).

## Contracts

### Random

- Engine: public `std::mt19937_64 rng`. `Random()` seeds from `steady_clock` ticks, which varies but guarantees neither entropy nor uniqueness. `Random(ulng)` accepts every 64-bit seed; record an explicit seed to replay, and reseed by assignment, `g = Random(x)`. Copy and move preserve the engine state. Callers synchronize shared objects.
- URBG: `result_type = ulng`, `min() = 0`, `max() = 2^64 - 1`, and `operator()()` returns the next engine word, so `Random` satisfies `std::uniform_random_bit_generator` and works with `std::shuffle`, `std::sample` and `<random>` distributions. How those standard algorithms consume words is implementation-defined; only the library's own methods replay across standard libraries.
- `randLng(l, r)`, `randUlng(l, r)`: inclusive bounds over the whole representable domain, singleton and full-width intervals included; precondition `l <= r` (asserted). The helper `randBelow(n)` behind them and `shuffle` needs `n > 0` and returns a value in `[0, n)`.
- `randDouble()`: `(word >> 11) * 2^-53`, uniform over the 2^53 multiples of 2^-53 in `[0, 1)`; one word per call.
- `randDouble(l, r)`: finite `l < r` with finite `r - l` (asserted). It returns `l + (r - l) * u` with `u` from `randDouble()` and redraws while the rounded value reaches `r`, so the result lies in `[l, r)`. The result is the rounded affine image of the 2^-53 grid, not exactly uniform over the doubles of the interval. Expected draws are below two when the interval spans at least two doubles; the one-ulp interval `[l, nextafter(l))` always returns `l` after an expected two draws. Bit-exact replay assumes the compiler does not contract `l + (r - l) * u` into an FMA (true for the tested x86-64 `-O2` builds without `-march`); the `[l, r)` guarantee holds either way.
- `shuffle(first, last)`: a valid mutable random-access range, not reversed (asserted); proxy and move-only elements are allowed. Fisher–Yates with `randBelow`, expected O(n) time, O(1) workspace; sizes zero and one consume no words.
- `inline Random rng`: the legacy global convenience object, one instance across translation units.
- Failure model: bounded draws are Las Vegas (a rejection loop with no finite worst case under the ideal-word model); MT19937-64 is not cryptographic.

Correctness: Lemire's mapping sets `M = 2^64`, `t = M mod n`, `p = x * n`, accepts when `p mod M >= t` and returns `floor(p / M)`. Every output in `[0, n)` has exactly `floor(M / n)` accepted preimages, so acceptance adds no bias to uniform independent words; at least half the words are accepted, so the expected cost is below two draws. Unsigned subtraction computes the width modulo 2^64, width zero denoting the full domain; signed offsets are added in `lll` and converted once the result fits. `randDouble()` uses the top 53 bits: each grid point has exactly 2^11 preimages and the product with 2^-53 is exact. For `randDouble(l, r)`, `u >= 0` and monotone rounding give `>= l`, the explicit test gives `< r`, and a redraw needs `u` within a few ulps of one unless the interval spans a single double. Fisher–Yates choice sequences are in bijection with the `n!` permutations.

### Legacy Mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `seed(x)` | `g = Random(x)` |
| `randInt(l, r)` | `int(randLng(l, r))`, the same value from the same words |

## Feature-to-Test Map

| Feature | Verification |
|---|---|
| MT engine, explicit and default seeds, reseed by assignment, copy/move, global `rng` | MT19937-64 10000th-word fixture, replay and state comparisons, assignment reseed against a fresh `std::mt19937_64`, default-constructor smoke, global identity across translation units |
| URBG interface | `static_assert(std::uniform_random_bit_generator<Random>)`, `min`/`max` values, `operator()` equals the engine sequence, `std::shuffle` and `std::uniform_int_distribution` accept `Random` |
| Bounded draws, full signed and unsigned width | Arbitrary-precision Python mapping on the raw engine words; minimum, maximum, singleton and full-width intervals, powers of two and their neighbours, and the former `randInt` intervals `[-2^31, 2^31)`, `[-7, 9]`, `[0, 0]` through `randLng`; the full run exercises 22,335 rejected words |
| `randDouble` (both overloads) | Exact Python oracle replaying raw words: `(w >> 11) * 2^-53` and the affine map with its rejection loop over `[0,1)`, `[-2.5,7.5)`, one-ulp `[1, nextafter(1))`, `[-8e307, 8e307)`, subnormal-scale `[1e-300, 3e-300)`, `[-5, -4.999)`, `[0, 5e-324)`; the full run has 200 cases with 15,054 redraws; C++ range and fixed-seed mean regression checks |
| Rejection uniformity argument | Exhaustive reduced words of 1–10 bits for every nonzero width |
| Fisher–Yates | Exact Python shuffle oracle, exhaustive abstract choices through n = 7, empty, singleton, duplicate, deque, proxy and move-only cases, replay |
| Invalid `Random` preconditions | Six checked assertion probes: zero bound, reversed `lng` and `ulng` intervals, reversed shuffle range, empty and infinite-width `randDouble` intervals |
| Header and linkage | Each tester includes only its header; an optimized two-TU test in both link orders checks early hash values, the shared seed address and the global `Random` address. Package integration owns the aggregates. |

Modes: quick runs the optimized and checked builds with 5,000 random draws and reduced words through 8 bits. Full adds ASan/UBSan, 100,000 C++ draws, 725 exact `Random` oracle cases and 200 `randDouble` cases per configuration, and reduced words through 10 bits. Stress raises the C++ workloads to 1,000,000 draws. Fixed-seed frequency checks are regressions, not uniformity proofs.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14.7, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 748 MB; optimized 0.3 s compile + 1.7 s run, checked 11.9 s + 1.4 s, ASan-UBSan 12.9 s + 2.5 s; peak 871 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 790 MB; optimized 0.2 s + 1.9 s, checked 19.4 s + 3.3 s, ASan-UBSan 19.0 s + 4.3 s; peak 938 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 20.8 s + 0.4 s, checked 18.9 s + 0.6 s; peak 738 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 746 MB; optimized 0.1 s + 1.2 s, checked 5.2 s + 1.3 s, ASan-UBSan 5.2 s + 4.1 s; peak 503 MB |
| 2026-10-30 (SUP18) | `python3 '96-Local Testing/06-Miscellaneous/01-random_tester.py' --mode full --seed 20260927`, on `g++` and `g++-14` | passed (counts not recorded) |
| 2026-10-12 (P013 prune) | the same command on `g++` | PASS, 3 configurations, 725 exact `Random` oracle cases (22,335 rejected words), 200 `randDouble` cases (15,054 redraws), 6 assertion probes, `MEMORY peak=429MB` |
| 2026-10-12 (P013 prune) | the same command with `CXX=g++-14` | PASS, the same counts, `MEMORY peak=446MB` |
| 2026-10-12 (P013 prune) | the other P013 suites, the call-site suites (`07-ordered_set`, `08-sequence_algorithms`, `12-enumeration`), `02-integration.py` and `03-consistency.py` | PASS (2026-10-12) |

## Sources

- Daniel Lemire, [*Fast Random Integer Generation in an Interval*, arXiv:1805.10941v4](https://arxiv.org/html/1805.10941v4): Algorithm 5, Lemma 4.1 and the Fisher–Yates description (read 2026-09-27).
- [cppreference `std::mersenne_twister_engine`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine.html): `mt19937_64` parameters and the 10000th-value fixture (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [Nyaan rng](https://nyaannyaan.github.io/library/misc/rng.hpp), [maspypy random_real](https://raw.githubusercontent.com/maspypy/library/main/random/random_real.hpp) and [base](https://raw.githubusercontent.com/maspypy/library/main/random/base.hpp), [hitonanode rand_nondeterministic](https://hitonanode.github.io/cplib-cpp/random/rand_nondeterministic.hpp), [OI Wiki random](https://oi-wiki.org/misc/random/), [KACTL HashMap.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h). They motivated the URBG interface, the `[l, r)` real overload (Nyaan's `rng() * 2^-64` can round up to 1.0, which the 53-bit form avoids) and the `gp_hash_table` alias.
- `OLD/algorithms.cpp` lines 856–870, read in full for legacy accounting: every legacy feature maps to an active operation (`randInt` through the Legacy Mapping above).

## Limits and Handoffs

- Randomness is noncryptographic (MT19937-64). `randDouble(l, r)` replays bit-exactly only without FMA contraction.
- Advanced random sampling belongs to `18`; `randomPermutation(n)` is `iota` plus `shuffle` ([00-notes.md](00-notes.md)).
- No benchmark: there is no reduction backend or tuned threshold.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 随机函数, 随机数, 随机数生成, 随机打乱, 洗牌算法, 梅森旋转, 乱数.

## History

- 2026-09-27: P013 first verification; full and stress passed.
- 2026-10-07: P013 re-audit added the URBG interface and `randDouble`, moved contracts out of code comments and normalized brace style; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: prune removed `seed(x)` and `randInt` (their cases moved to assignment reseeding and `randLng`); full passed on g++ and g++-14.
- 2026-10-30: SUP18 arm64 portability (user decision 2026-10-30, fixed in P250): the tester builds with `-ffp-contract=off`, a new per-tester flag of the `06-Miscellaneous` runner, so the exact `randDouble(l, r)` oracle holds where g++ would fuse the multiply-add (aarch64); x86-64 output unchanged; quick passed on x86-64 and on aarch64 under qemu-user, full passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese search aliases (OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
