# 03-barrett.hpp — evidence

`03-barrett.hpp` (C02, package P004, together with `04-montgomery.hpp`; see [04-montgomery.md](04-montgomery.md)) provides machine-word Barrett reduction: exact quotient and remainder of double-width dividends, products, powers and Shoup fixed multipliers. Backend choice and the research/ownership boundary for the reduction family are in [00-notes.md](00-notes.md#reduction-family); the benchmark method shared with Montgomery is under [Benchmarks](#benchmarks).

## Contracts

### Common machine-word rules

Let `w` be 32 or 64 and `R = 2^w`. A context's fields are public for contest use but must not be modified. Copying or moving contexts creates independent values; there are no caches, allocation, global modulus, reset operation, or CPU runtime dispatch. Scalar kernels use unsigned arithmetic and GNU double-width integers. Context storage and per-operation scratch are O(1); general construction includes native division for precomputation, while the power-of-two and Mersenne61 contexts skip it. Exponentiation takes O(1+log(e+1)) word operations for exponent e and bulk operations take O(n).

All bulk APIs use pointer/count overloads with an `int n >= 0`; pointers address `n` words, with no alignment requirement. Count and pointer preconditions are asserted at entry. Empty calls permit null pointers. Buffers must be disjoint or have exactly the same start; partial overlap is outside the contract. Products support output aliasing either/both inputs. Fixed-multiplier arrays support exact in-place operation. Modulus zero and other domain violations are asserted in checked builds and remain preconditions under `NDEBUG`. Modulus one returns zero throughout, including exponent zero. No primality assumption or residue inverse belongs to this reducer. `LOCAL` does not affect the header.

| API | Domain and result |
|---|---|
| `Barrett32(m)`, `Barrett64(m)`; `Barrett` aliases the latter | Every nonzero unsigned word modulus, including one and even moduli; the default modulus is one. |
| `divMod(x)`, `reduce(x)` | Every unsigned double-width dividend, including its maximum; `divMod` returns the exact quotient (double width) and `x % m` in `[0,m)`, `reduce` the remainder alone. The mask, Mersenne61 and reciprocal paths all derive the quotient from the same folds. |
| `mul(a,b)`, `pow(a,e)` | Arbitrary unsigned word inputs and unsigned 64-bit exponent. Ordinary residues in/out; `0^0 = 1 % m`. |
| `Multiplier(r, b)` | Precomputes the fixed factor `b` (any word) of reducer `r` with a Shoup reciprocal; owns its modulus and canonicalized factor independently of `r`. Its `mul(a)` accepts any word. |
| `Barrett64::fold61(x)` | `constexpr`, any 128-bit `x`: returns `(x & M) + (x >> 61)` for `M = 2^61-1`, congruent to `x` modulo `M`, below `2^67 + 2^61` for every `x` and below `2M` for `x < 2^122 - 2^61` (every product of two words below `2^61`), so one conditional subtraction then canonicalizes. |

### Barrett32, Barrett64

The fields `mod`, `mu` and `mask` are immutable state. `mu = floor(2^(2w) / mod)` on the reciprocal path and zero on the power-of-two and Mersenne61 paths (`MERSENNE61 = 2305843009213693951`); `mask` flags a power-of-two modulus. `divMod` returns a quotient as wide as the dividend. With `mod = 1` every result is zero, even for exponent zero; for every other modulus `a^0 = 1`, including `a = 0`. Copies and moves are independent; there is no allocation, cache or global state. `Barrett64::highProduct` is the exact high half of a 128 x 128-bit product. `Barrett64::fold61` is the one Mersenne-61 fold: `divMod` applies it twice to an arbitrary dividend, and the modular types' Mersenne paths call it since the 2026-10-12 P005 prune.

### Barrett32::Multiplier, Barrett64::Multiplier

Constructed as `Multiplier(r, b)` from a reducer `r` and any factor word `b`: `value = b % mod` and `mu = floor(value * R / mod)`. The multiplier has the same input, output, state and pointer contracts as `r` and does not depend on `r`'s lifetime. Barrett64's 128-bit intermediates keep the 65th residual bit, which full-width moduli need.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `div(x)` | `divMod(x).first` |
| `multiplier(b)` | `Barrett32::Multiplier(r, b)`, `Barrett64::Multiplier(r, b)` for a reducer `r` |

### Arithmetic justification

**Barrett.** Set `B = R^2` and `mu = floor(B/m)`. Non-power-of-two moduli do not divide `B`, so the representable expression `floor((B-1)/m)` computes the same value. For `x < B`, `q = floor(x*mu/B)` is the exact quotient or one less: the approximation error is nonnegative and strictly below one. Thus `x-q*m < 2m`; one subtraction makes it canonical. Keep the extra residual bit until correction. Power-of-two moduli, including one, use masking and need no reciprocal. The 128-bit high product splits into four 64-bit products; summing the three middle limbs at 128-bit width preserves both carries. For `m=2^61-1`, use `2^61 = 1 (mod m)`: two folds of the entire 128-bit dividend leave a value at most `m+64 < 2m`, followed by one subtraction. One fold (`fold61`) of `x < 2^122 - 2^61` already leaves at most `2m - 1`, because then `x >> 61 <= m - 1` while `x & m <= m`.

**Fixed multiplier.** With `b < m`, precompute `c = floor(b*R/m)`. For any word `a`, `floor(a*c/R)` underestimates `floor(a*b/m)` by at most one. The widened residual is `<2m`, so one subtraction suffices, including full-width moduli. This replaces the general double-width reciprocal product for repeated scalar factors.

**Quotients.** On the mask path `x >> countr_zero(m)` is the exact quotient (shift zero for `m=1`). On the reciprocal path `q` is the true quotient or one less, so `q*m <= x` and `q + [r >= m]` is exact. For `m = 2^61-1` write `x = (x>>61)*2^61 + (x & m) = (x>>61)*m + y` with `y = fold61(x) = (x>>61) + (x & m) <= 2^67 + 2^61 - 2`, then `y = (y>>61)*m + z` with `z = fold61(y) = (y>>61) + (y & m) <= m + 64 < 2m`; hence `x = ((x>>61) + (y>>61))*m + z` and the quotient is `(x>>61) + (y>>61) + [z >= m]`, the remainder `z - [z >= m]*m`. `divMod` is the Barrett core: `reduce` reads its remainder, so the remainder path is unchanged after inlining (`Barrett32::reduce` 16 and `Barrett64::reduce` 104 instructions at `-O2` on GCC 16.2, identical instruction streams before and after `fold61`; 103 on GCC 14.4).

## Feature-to-test map

The Python entry resolves paths independently of the working directory, uses only Python's standard library and GCC, reports seeds/configurations and first failing inputs, retains its oracles under `NDEBUG`, and executes precondition failures in subprocesses.

| Operation | Test | Oracle |
|---|---|---|
| Construction, `reduce`, `mul` | `03-barrett_tester.cpp` | Native double-width `%`: all powers of two and neighbors, modulus one, primes/composites/even moduli, full unsigned maxima, Mersenne boundaries, dividend maxima, seeded random and bounded exhaustive small domains |
| `highProduct` | `03-barrett_tester.cpp` | Independent bit-by-bit 256-bit shift/add multiplication; carry boundaries and random full-width pairs |
| `pow`, `Multiplier` construction, value semantics | `03-barrett_tester.cpp` | Independent MSB-first modular exponentiation, zero/max exponents, `0^0`, arbitrary input words, all small fixed-factor pairs, copies/moves and a `Multiplier` outliving its temporary reducer |
| Bulk `mul` and preconditions | `03-barrett_tester.cpp` | Lengths 0–65, 127/128/129, 255/256/257 and 4095/4096/4097, eight offsets, guard words, alias either/both inputs, fixed in-place products, null empty ranges; 16 assertion death cases (in both checked builds) |
| `divMod` (both widths) | `03-barrett_tester.cpp` | Native `/` and `%` on the boundary dividend list (0, 1, m-1, m, m+1, 2m-1, MAX, MAX+1, MAX², TOP, TOP-1, m·MAX, m·MAX-1) for ~250 moduli per width including every power of two and its neighbours, `2^61-1`, `2^64-1`; 60,000 (full) / 600,000 (stress) random full-width dividends per width; exhaustive `m <= 128, x < 16384` (full) / `m <= 256, x < 65536` (stress); default constructor; the quotient is checked from the pair and as `divMod(x).first` (the former `div` cases) |
| `mod`, `mu`, `mask`, `Multiplier::{mod, value, mu, mask}`, `MERSENNE61` | `03-barrett_tester.cpp` | Bitwise restoring division `floor((2^(2w)-1)/m)` and `floor((b % m)·2^w / m)`; zero on the mask and Mersenne paths; constant equals `2305843009213693951` |
| `fold61` | `03-barrett_tester.cpp` `folds()` | Native 128-bit `%`: congruence modulo `M = 2^61-1` on 17 boundary inputs (0, 1, `M-1`, `M`, `M+1`, `2^61`, `2^61+1`, `(M-1)^2`, `M^2`, `M·2^61`, `M·2^61 + M`, `2^122 - 2^61 - 1`, `2^122 - 2^61`, `2^122`, `2^127`, `2^128-2`, `2^128-1`) and 10,000 (full) / 100,000 (stress) seeded random full-width and product-shaped inputs each, with the `< 2^67 + 2^61` bound everywhere and the `< 2M` bound below `2^122 - 2^61`; `static_assert` evaluates it at compile time at 0, `M`, `2^61`, `2^122 - 2^61 - 1` and `2^128-1` |
| `Barrett` alias, `load`, `store` | `03-barrett_tester.cpp` | `Barrett(17).mul(123, 456)` against `123 * 456 % 17`, and the multiple-TU build compares `Barrett` with `Barrett64`; the AVX2 helpers run inside every AVX2 bulk kernel of the batch tests in the optimized, checked and sanitized AVX2 builds |
| Bulk fixed products | `03-barrett_tester.cpp` `batches()` | Factors `MAX`, `m-1` and a seeded random factor for 16 (32-bit) / 17 (64-bit) batch moduli, checked against `Wide(a)*f % m`, sizes 0–65, 127–129, 255–257, 4095–4097, eight offsets, separate and in-place, leading/trailing guards; Barrett32 moduli include `2^32-5` and `3*2^30+1`, so the AVX2 Shoup kernel runs with residuals near `2^33` through its signed correction |

Quick mode covers every public operation with smaller exhaustive/random corpora. Full uses 60,000 random cases per width and exhaustive `m<=128, x<16384`; stress uses 600,000 and `m<=256, x<65536`, with exhaustive small fixed-factor pairs. Full/stress execute optimized `-O3 -DNDEBUG`, checked, and ASan/UBSan scalar/AVX2 paths (including a checked AVX2 build). Unsupported AVX2 hardware is reported as a skip.

## Commands and results

Prune, 2026-10-12 (`div` and `multiplier` removed, `fold61` added; the Montgomery exponent widened in the same change):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/03-barrett.hpp 01-Core/04-montgomery.hpp
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261012 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
```

All passed on GCC 16.2.1 and GCC 14.4.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every tester build with `-Wall -Wextra -Wshadow -Wconversion -Werror` (the benchmark builds use the `-Wall -Wextra` flags recorded in the JSONL; its source also compiles clean under the strict flags on both compilers). Full (seed 20261012): **53,670,566 non-removable checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 16 precondition deaths per checked build; standalone/multiple-TU build), 36.47 s on g++ and 37.55 s on g++-14, memory peak 616 MB and 619 MB under the 4096 MB cap. `02-integration.py` passed 136 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage and the Workspace snapshot; `03-consistency.py` reports no errors and no brace, comment-cap or free-function violations. The benchmark run verified all 9,116 measurements against native wide `%`.

## Benchmarks

The [benchmark entry](<../../96-Local Testing/01-Core/03-reduction_benchmark.py>) serves both reduction headers and writes `96-Local Testing/01-Core/03-reduction_benchmark.jsonl` with compiler commands, hardware, flags, seed, adaptive warmup and medians of five repetitions. Builds use GNU++20: baseline `-O3 -DNDEBUG` without AVX2/BMI2, accelerated `-O3 -DNDEBUG -mavx2 -mbmi2 -mno-avx512f`, and `scalar-asserts`/`avx2-asserts` at `-O2` with assertions enabled (the Codeforces build). Inputs are shared between implementations and every measured output is verified. Compiler memory barriers and volatile result sinks prevent elimination/hoisting. Kernels use precomputed contexts and O(n) input/output storage with O(1) scratch. Setup-and-power includes context construction and conversion for each of eight full-word bases/exponents.

The corpus covers modulus one, powers of two, typical contest primes, `2^31-1`, `2^61-1`, `2^64-59` and `2^64-1`; uniform and near-modulus inputs; bulk lengths 1, 7, 8, 9, 16, 32, 256 and 4096; arbitrary full-width dividends and fixed factors; canonical/lazy products, conversions, dependent product chains and dependent power chains with 64- and 128-bit exponents on one context. Unit-factor chains avoid products collapsing to zero, while the separate uniform distribution includes that legitimate case. Timings are observations on this host (Intel Core i9-11900H, GCC 16.2.1), not universal claims or timing gates.

Latest run, 2026-10-12 after the prune (seed 20261012, 2 ms warmup, five repetitions, four configurations): 9,116 verified measurements, held in the JSONL. Nanoseconds per complete workload, accelerated (AVX2) `-O3 -DNDEBUG` build:

| Workload | Reference | Barrett |
|---|---|---|
| 256 ordinary products, `m=998244353` | Native `%`: 576 | 221 |
| 256 fixed-factor products, `m=998244353` | Scalar Shoup: 225 | AVX2 Shoup: 69 |
| 256 full-width reductions, `m=2^61-1` | Generic reciprocal: 708; native: 1,058 | Two-fold reduction: 421 |
| 256 fixed-factor products, `m=2^64-59` | General Barrett: 1,293 | Shoup: 276 |
| 256 quotient+remainder, 64-bit dividends, `m=998244353` | Native `/` and `%`: 547 | `divMod`: 250 |
| 256 quotient+remainder, 128-bit dividends, `m=2^61-1` | Native: 1,074 | `divMod`: 558 |
| 256 quotient+remainder, 128-bit dividends, `m=2^63` | Native: 888 | `divMod`: 254 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,489 | 1,444 |

Montgomery's figures for the shared power workloads are in [04-montgomery.md](04-montgomery.md#benchmarks).

General 64-bit Barrett is **not universally faster than native remainder**: modulo `2^64-59` and `2^64-1`, 737 and 746 vs 571 and 574 ns for 256 reductions, and `divMod` 923 and 923 vs 585 and 597. `divMod` wins for every other measured modulus (`2^32+15`: 914 vs 1,133). Its value is a predictable exact precomputed backend over every modulus/dividend domain; use Montgomery for suitable long odd-modulus chains or Shoup for repeated factors, and ordinary `%` when setup/reuse does not justify a context.

Rejected candidates ([measurements](<../../96-Local Testing/01-Core/03-reduction_candidates.jsonl>), snapshots that predate the final headers and are labeled accordingly): a general AVX2 Barrett32 kernel with a synthesized high-64 vector reciprocal product showed no consistent improvement (bulk/scalar about 1.07 at 4096 elements), so ordinary Barrett32 uses a scalar loop except for power-of-two masks (eight packed lanes). Mersenne31 folding lost to ordinary reciprocal reduction (about 286 vs 225 ns) and was removed; Mersenne61 folding is retained. Manual 64-bit unrolling added overhead on short arrays without a consistent general benefit, so 64-bit bulk loops remain direct. Ordinary 64-bit multiplication uses the compiler's native wide products.

Shoup threshold (median AVX2 bulk/scalar ratio over `998244353`, `2^31-1`, `2^32-5`): 1.27 at 7, 0.45 at 8, 0.57 at 9, 0.31 at 256 elements; 0.54 at 8 with assertions enabled. The eight-element dispatch stands.

## Sources

| Source inspected (2026-09-27) | Relevant result and limits |
|---|---|
| Brent and Zimmermann, [Modern Computer Arithmetic, v0.5.9 (2010-10-07)](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf), §§2.4.1–2.4.4 and §2.5 ([saved copy](../../95-Resources/04-modern_computer_arithmetic.pdf); reading copy CC BY-NC-ND, no text copied) | Barrett approximation and special moduli. Its truncated normalized Barrett bound differs from the full-precision one-correction formula above. |
| [AtCoder internal_math.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `internal::barrett` | Ceiling reciprocal and correction proof for canonical 32-bit products. CC0 1.0. Does not establish this header's arbitrary-dividend/full-64-bit domain. |
| [NTL single-precision API](https://libntl.org/doc/ZZ.txt), `PrepMulModPrecon`/`MulModPrecon`; [selected sp_arith.h implementation](https://raw.githubusercontent.com/libntl/ntl/main/include/NTL/sp_arith.h) | Fixed-multiplier reciprocal and vector products. NTL's `NTL_SP_BOUND` restriction differs from this implementation's full-word domain; no source copied. |

The 2026-10-07 catalog sweep is in [00-sources.md](00-sources.md) ("Fetched 2026-10-07 (P004 re-audit sweep)"). Independently written; no external code copied.

## Limits and handoffs

GCC 14.4.1 (`CXX=g++-14`, the floor check) passed. The `Multiplier` has no quotient. Signed-dividend reduction and residue inverses stay with `05-modint.hpp`. Lazy Barrett outputs, a divisibility test and the other left-out candidates are listed with reasons in [00-notes.md](00-notes.md#reduction-and-modular-types).

## History

- 2026-09-27: original P004 completion, full and stress (91,538,696 checks per configuration) and integration passed; first benchmark (seed 20260927, 4,412 verified measurements).
- 2026-09-27: maintenance review, full suite, integration and consistency passed.
- 2026-10-07: re-audit, 3 findings fixed, `divMod`/`div`, `MERSENNE61` and default modulus added, full and stress passed on g++.
- 2026-10-07 (run): full (seed 20261007, 53,657,153 checks per configuration) and stress (seed 1, 146,158,229) passed, `02-integration.py --sanitizers` passed, independent `@reviewer` differential (8.13 million cases) and mutants found no defect, benchmark (seed 20261007, 8,912 verified measurements).
- 2026-10-08: restyle (comments only), full suite passed on g++ and g++-14, integration and consistency passed.
- 2026-10-12: prune (div and multiplier removed, fold61 added), full suite (seed 20261012) passed on g++ and g++-14, benchmark verified 9,116 measurements.
