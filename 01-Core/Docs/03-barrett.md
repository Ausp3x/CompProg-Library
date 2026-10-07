# 03-barrett.hpp — evidence

`03-barrett.hpp` (C02, package P004, together with `04-montgomery.hpp`; see [04-montgomery.md](04-montgomery.md)) provides machine-word Barrett reduction: exact quotient and remainder of double-width dividends, products, powers and Shoup fixed multipliers. Backend choice, the shared benchmark method and the research/ownership boundary for the reduction family are in [00-notes.md](00-notes.md#reduction-family).

## Contracts

### Common machine-word rules

Let `w` be 32 or 64 and `R = 2^w`. A context's fields are public for contest use but must not be modified. Copying or moving contexts creates independent values; there are no caches, allocation, global modulus, reset operation, or CPU runtime dispatch. Scalar kernels use unsigned arithmetic and GNU double-width integers. Context storage and per-operation scratch are O(1); general construction includes native division for precomputation, while the power-of-two and Mersenne61 contexts skip it. Exponentiation takes O(1+log(e+1)) word operations for exponent e and bulk operations take O(n).

All bulk APIs use pointer/count overloads with an `int n >= 0`; pointers address `n` words, with no alignment requirement. Count and pointer preconditions are asserted at entry. Empty calls permit null pointers. Buffers must be disjoint or have exactly the same start; partial overlap is outside the contract. Products support output aliasing either/both inputs. Fixed-multiplier arrays support exact in-place operation. Modulus zero and other domain violations are asserted in checked builds and remain preconditions under `NDEBUG`. Modulus one returns zero throughout, including exponent zero. No primality assumption or residue inverse belongs to this reducer. `LOCAL` does not affect the header.

| API | Domain and result |
|---|---|
| `Barrett32(m)`, `Barrett64(m)`; `Barrett` aliases the latter | Every nonzero unsigned word modulus, including one and even moduli; the default modulus is one. |
| `divMod(x)`, `div(x)`, `reduce(x)` | Every unsigned double-width dividend, including its maximum; `divMod` returns the exact quotient (double width) and `x % m` in `[0,m)`, `div`/`reduce` return one of them. The mask, Mersenne61 and reciprocal paths all derive the quotient from the same folds. |
| `mul(a,b)`, `pow(a,e)` | Arbitrary unsigned word inputs and unsigned 64-bit exponent. Ordinary residues in/out; `0^0 = 1 % m`. |
| `multiplier(b)` | Precompute a fixed multiplier using a Shoup reciprocal; owns its modulus and canonicalized multiplier independently of the original context. Its `mul(a)` accepts any word. |

### Barrett32, Barrett64

The fields `mod`, `mu` and `mask` are immutable state. `mu = floor(2^(2w) / mod)` on the reciprocal path and zero on the power-of-two and Mersenne61 paths (`MERSENNE61 = 2305843009213693951`); `mask` flags a power-of-two modulus. `divMod` returns a quotient as wide as the dividend. With `mod = 1` every result is zero, even for exponent zero; for every other modulus `a^0 = 1`, including `a = 0`. Copies and moves are independent; there is no allocation, cache or global state. `Barrett64::highProduct` is the exact high half of a 128 x 128-bit product.

### Barrett32::Multiplier, Barrett64::Multiplier

`value = b % mod` and `mu = floor(value * R / mod)`. The multiplier has the same input, output, state and pointer contracts as its parent reducer and does not depend on the parent's lifetime. Barrett64's 128-bit intermediates keep the 65th residual bit, which full-width moduli need.

### Arithmetic justification

**Barrett.** Set `B = R^2` and `mu = floor(B/m)`. Non-power-of-two moduli do not divide `B`, so the representable expression `floor((B-1)/m)` computes the same value. For `x < B`, `q = floor(x*mu/B)` is the exact quotient or one less: the approximation error is nonnegative and strictly below one. Thus `x-q*m < 2m`; one subtraction makes it canonical. Keep the extra residual bit until correction. Power-of-two moduli, including one, use masking and need no reciprocal. The 128-bit high product splits into four 64-bit products; summing the three middle limbs at 128-bit width preserves both carries. For `m=2^61-1`, use `2^61 = 1 (mod m)`: two folds of the entire 128-bit dividend leave a value at most `m+64 < 2m`, followed by one subtraction.

**Fixed multiplier.** With `b < m`, precompute `c = floor(b*R/m)`. For any word `a`, `floor(a*c/R)` underestimates `floor(a*b/m)` by at most one. The widened residual is `<2m`, so one subtraction suffices, including full-width moduli. This replaces the general double-width reciprocal product for repeated scalar factors.

**Quotients (2026-10-07).** On the mask path `x >> countr_zero(m)` is the exact quotient (shift zero for `m=1`). On the reciprocal path `q` is the true quotient or one less, so `q*m <= x` and `q + [r >= m]` is exact. For `m = 2^61-1` write `x = (x>>61)*2^61 + (x & m) = (x>>61)*m + y` with `y = (x>>61) + (x & m) <= 2^67 + 2^61 - 2`, then `y = (y>>61)*m + z` with `z = (y>>61) + (y & m) <= m + 64 < 2m`; hence `x = ((x>>61) + (y>>61))*m + z` and the quotient is `(x>>61) + (y>>61) + [z >= m]`, the remainder `z - [z >= m]*m`. `divMod` is the Barrett core: `reduce` reads its remainder, so the remainder path is unchanged after inlining (benchmark unchanged within noise; `Barrett32::reduce` 16/16 and `Barrett64::reduce` 104/104 instructions at `-O2`, before/after).

## Feature-to-test map

The Python entry resolves paths independently of the working directory, uses only Python's standard library and GCC, reports seeds/configurations and first failing inputs, retains its oracles under `NDEBUG`, and executes precondition failures in subprocesses.

| Operation | Test | Oracle |
|---|---|---|
| Construction, `reduce`, `mul` | `03-barrett_tester.cpp` | Native double-width `%`: all powers of two and neighbors, modulus one, primes/composites/even moduli, full unsigned maxima, Mersenne boundaries, dividend maxima, seeded random and bounded exhaustive small domains |
| `highProduct` | `03-barrett_tester.cpp` | Independent bit-by-bit 256-bit shift/add multiplication; carry boundaries and random full-width pairs |
| `pow`, `multiplier`, value semantics | `03-barrett_tester.cpp` | Independent MSB-first modular exponentiation, zero/max exponents, `0^0`, arbitrary input words, all small fixed-factor pairs, copies/moves and detached multiplier lifetime |
| Bulk `mul` and preconditions | `03-barrett_tester.cpp` | Lengths 0–65, 127/128/129, 255/256/257 and 4095/4096/4097, eight offsets, guard words, alias either/both inputs, fixed in-place products, null empty ranges; 16 assertion death cases (in both checked builds since 2026-10-07) |
| `divMod`, `div` (both widths) | `03-barrett_tester.cpp` | Native `/` and `%` on the boundary dividend list (0, 1, m-1, m, m+1, 2m-1, MAX, MAX+1, MAX², TOP, TOP-1, m·MAX, m·MAX-1) for ~250 moduli per width including every power of two and its neighbours, `2^61-1`, `2^64-1`; 60,000 (full) / 600,000 (stress) random full-width dividends per width; exhaustive `m <= 128, x < 16384` (full) / `m <= 256, x < 65536` (stress); default constructor |
| `mod`, `mu`, `mask`, `Multiplier::{mod, value, mu, mask}`, `MERSENNE61` | `03-barrett_tester.cpp` | Bitwise restoring division `floor((2^(2w)-1)/m)` and `floor((b % m)·2^w / m)`; zero on the mask and Mersenne paths; constant equals `2305843009213693951` |
| Bulk fixed products | `03-barrett_tester.cpp` `batches()` | Factors `MAX`, `m-1` and a seeded random factor for 16 (32-bit) / 17 (64-bit) batch moduli, checked against `Wide(a)*f % m`, sizes 0–65, 127–129, 255–257, 4095–4097, eight offsets, separate and in-place, leading/trailing guards; Barrett32 moduli include `2^32-5` and `3*2^30+1`, so the AVX2 Shoup kernel runs with residuals near `2^33` through its signed correction |

Quick mode covers every public operation with smaller exhaustive/random corpora. Full uses 60,000 random cases per width and exhaustive `m<=128, x<16384`; stress uses 600,000 and `m<=256, x<65536`, with exhaustive small fixed-factor pairs. Full/stress execute optimized `-O3 -DNDEBUG`, checked, and ASan/UBSan scalar/AVX2 paths (a checked AVX2 build since 2026-10-07). Unsupported AVX2 hardware is reported as a skip.

## Commands and results

### Original P004 completion — 2026-09-27

GCC 16.2.1 (20260810), Linux x86-64, Intel i9-11900H.

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode stress --seed 42
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --repetitions 5 --milliseconds 1 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
```

Barrett full passed 29,477,748 checks per configuration before the final measured simplifications; final stress passed **91,538,696 checks in each of five configurations**, plus 16 assertion cases and standalone/two-translation-unit execution. Integration passed **59 standalone/aggregate headers**, scalar/AVX2 multiple-TU linkage and the Workspace snapshot. Sanitizer runs were repeated outside the sandbox with approved execution because LeakSanitizer cannot operate under its ptrace restriction; scalar and AVX2 ASan/UBSan executions then passed. No sanitizer finding was suppressed. No external test dependency; no online submissions.

### Maintenance review — 2026-09-27

The header now separates state/construction, scalar operations and array operations; multiline endings follow the compressed brace convention; assertions and loop setup are separate logical steps. Public names, member order, construction order, modulus/dividend domains, scalar paths and dispatch are preserved. The tester uses `int` for bounded check counters (the largest stress corpus stays under 100 million checks, below `INT_MAX`) and retains `ulng` seeds; moduli, residues, reciprocals, exponents, wide intermediates, masks and seeds remain unsigned. `Barrett64::highProduct` and each nested fixed `Multiplier` remain methods. Local naming/grouping was aligned without changing cases, oracle operations or diagnostic strings; the few size conversions address fixed bounded test/benchmark buffers.

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20260927
g++ -std=gnu++20 -O3 -DNDEBUG -Wall -Wextra -mno-avx2 -mno-bmi2 -fsyntax-only '96-Local Testing/01-Core/03-reduction_benchmark.cpp'
g++ -std=gnu++20 -O3 -DNDEBUG -Wall -Wextra -mavx2 -mbmi2 -mno-avx512f -fsyntax-only '96-Local Testing/01-Core/03-reduction_benchmark.cpp'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

GCC 16.2.1 (20260810), CPython 3.14.7, Linux x86-64. Barrett full: 29,477,748 checks in each of five configurations (optimized NDEBUG scalar/AVX2, checked scalar, ASan/UBSan scalar/AVX2), 16 assertion cases, standalone/multiple-TU executable passed. Benchmark syntax checks passed. Integration: 59 standalone/aggregate headers, scalar/AVX2 multiple-TU linkage and Workspace checks passed. Consistency: zero errors. Full suites used approved execution outside the sandbox to retain LeakSanitizer; leak checking was not disabled. Historical stress runs and performance medians remain historical evidence.

| File | SHA256 at the 2026-09-27 maintenance (superseded) |
|---|---|
| `01-Core/03-barrett.hpp` | `3368c40cbb74242616a964dee0a1f8a3841838f07cbf5b34627ced26e9bcee85` |
| `96-Local Testing/01-Core/03-barrett_tester.cpp` | `17697706970c33c5b7fba98c30395f33ef0ecd8536257d406179bea0b38dc8d6` |

### Re-audit — 2026-10-07

Package P004 re-audit, treating the previous verification as existing-unverified. Every operation in the row was compared with the code and the tester before any edit (the only row gap was `Barrett64::highProduct`); the unchanged full suite was rerun (5 configurations in 28.91 s, seed 20260927, passing); the header was then brought to the current style, extended from the catalog sweep and re-tested.

| # | Finding | Disposition |
|---|---|---|
| 1 | `Barrett64::highProduct` missing from the inventory row | Fixed: the row names it, the new `MERSENNE61` constant, the `Multiplier` fields and the AVX2 `load`/`store` helpers; the brief reads the row. |
| 2 | Bulk fixed-multiplier tests used only `multiplier(MAX)`, which is 0 or 1 for most batch moduli | Fixed: `batches()` builds multipliers for `MAX`, `m-1` and a seeded random factor for every modulus (see the feature map). |
| 3 | Complexity comment used the undefined symbol `e` | Fixed (`powers O(log(e+1)) for exponent e`; Barrett's four comments likewise). |

Changes beyond the findings: catalog sweep ([00-sources.md](00-sources.md), left-out items in [00-notes.md](00-notes.md#reduction-and-modular-types)) added `divMod`/`div` (maspypy `floor`/`divmod`, Nyaan and suisen `quo`/`quorem`, Lemire `fastdiv`) and a default modulus of one (maspypy, suisen). The header, tester and benchmark follow the closing-brace rule; the header compiles with `-Wall -Wextra -Wshadow -Wconversion -Werror` on scalar and AVX2 builds. The tester adds a checked AVX2 build, field checks, `divMod`/`div` coverage and the default constructor.

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261007 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261007): **53,657,153 non-removable checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 16 precondition deaths in both checked builds; standalone/multiple-TU build) in 41.72 s. Stress (seed 1, one round): **146,158,229 checks in each of six configurations** in 61.53 s. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, the Workspace snapshot and the sanitizer self-tests. `03-consistency.py` reports no errors (eight brace-checked files).

Independent review (`@reviewer`, 2026-10-07) found no correctness defect in either reduction header: it re-derived every argument, ran its own 8.13-million-case differential against native `%`, `/` and `ulll` arithmetic under ASan/UBSan on scalar and AVX2 builds (quotients at maximal values near `k*m +- 1`, `2^61-1` and neighbours, `2^63 +- 1`, `2^64-59`; bulk Shoup with factors `m-1` and `MAX` at `2^32-1`, `2^32-5`, `3*2^30+1`), and 15 header mutants across both headers (for Barrett: dropped quotient corrections, single Mersenne fold, off-by-one Shoup correction) that the testers all caught in `-DNDEBUG` builds or by death tests. It confirmed the findings fixed and instruction counts unchanged for the remainder paths; its low findings (row names, complexity lines, the quotient argument in the justification) were fixed.

| File | SHA256 after the re-audit |
|---|---|
| `01-Core/03-barrett.hpp` | `ce620ceca3aa0ed76625c82dc2a60f4a9edb9b744e82b69951dad8e0474f9650` |
| `96-Local Testing/01-Core/03-barrett_tester.cpp` | `7dca45c36bcdb3bf2c604c68be95681b926494fc1c226571fb5d78f6b1f82314` |
| `96-Local Testing/01-Core/03-barrett_tester.py` | `9290f5fdfc4f6306c0026ef913be5ca91dab871808dcccaca2545a1be78f3b44` |

## Benchmarks

Method, corpus and artifact hashes: [00-notes.md](00-notes.md#reduction-family). Raw data: `96-Local Testing/01-Core/03-reduction_benchmark.jsonl`, `96-Local Testing/01-Core/03-reduction_candidates.jsonl`. Nanoseconds per complete workload, medians, accelerated (AVX2) `-O3 -DNDEBUG` build.

| Workload | Reference | Barrett, 2026-09-27 | Barrett, 2026-10-07 |
|---|---|---|---|
| 256 ordinary products, `m=998244353` | Native `%`: 672 / 584 | 249 | 247 |
| 256 fixed-factor products, `m=998244353` | Scalar Shoup: 249 / 246 | AVX2 Shoup: 79 | AVX2 Shoup: 75 |
| 256 full-width reductions, `m=2^61-1` | Generic reciprocal: 725 / 770; native (2026-10-07): 1,214 | Two-fold reduction: 413 | Two-fold reduction: 417 |
| 256 fixed-factor products, `m=2^64-59` | General Barrett: 822 / 871 | Shoup: 278 | Shoup: 291 |
| 256 quotient+remainder, 64-bit dividends, `m=998244353` | Native `/` and `%`: 608 (2026-10-07) | — | `divMod`: 269 |
| 256 quotient+remainder, 128-bit dividends, `m=2^61-1` | Native: 1,311 (2026-10-07) | — | `divMod`: 684 |
| 256 quotient+remainder, 128-bit dividends, `m=2^63` | Native: 968 (2026-10-07) | — | `divMod`: 261 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,740 / 2,721 | 1,724 | 1,572 |

Reference columns give 2026-09-27 / 2026-10-07 values. Montgomery's figures for the shared power workloads are in [04-montgomery.md](04-montgomery.md#benchmarks).

General 64-bit Barrett is **not universally faster than native remainder**: on 2026-09-27, 256 arbitrary wide dividends modulo `2^64-59` took 708 ns versus 574 ns for native `%`; on 2026-10-07, 1,120 vs 860 ns for 256 reductions and `divMod` 1,107 vs 717. `divMod` wins for every other measured modulus. Its value is a predictable exact precomputed backend over every modulus/dividend domain; use Montgomery for suitable long odd-modulus chains or Shoup for repeated factors, and ordinary `%` when setup/reuse does not justify a context.

Rejected candidates: the synthesized high-64 vector reciprocal product of the initial general AVX2 Barrett32 kernel showed no consistent improvement (bulk/scalar about 1.07 at 4096 uniform elements), so ordinary Barrett32 uses a scalar loop except for power-of-two masks, whose kernels use eight packed lanes. Mersenne31 folding took about 286 ns versus 225 ns for a 256-product reciprocal loop in the same AVX2 run and was removed; the tested `2^31-1` folding candidate lost to ordinary reciprocal reduction. Mersenne61 folding is retained with its full-dividend proof and measured benefit. Ordinary 64-bit multiplication uses the compiler's native wide products; synthesizing large high products from AVX2's 32-bit multipliers has no established benefit here.

Shoup threshold (median AVX2 bulk/scalar ratio): 2026-09-27, 0.47 at eight and 0.55 at nine elements across tested non-power-of-two 32-bit moduli. 2026-10-07 over `998244353`, `2^31-1`, `2^32-5`: 1.30 at 7, 0.47 at 8, 0.55 at 9, 0.31 at 256 elements; 0.53 at 8 with assertions enabled. The eight-element dispatch stands.

## Sources

| Source inspected (2026-09-27) | Relevant result and limits |
|---|---|
| Brent and Zimmermann, Modern Computer Arithmetic v0.5.9 (see [00-notes.md](00-notes.md#reduction-family)) | Barrett approximation and special moduli. Its truncated normalized Barrett bound differs from the full-precision one-correction formula above. |
| [AtCoder internal_math.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `internal::barrett` | Ceiling reciprocal and correction proof for canonical 32-bit products. CC0 1.0. Does not establish this header's arbitrary-dividend/full-64-bit domain. |
| [NTL single-precision API](https://libntl.org/doc/ZZ.txt), `PrepMulModPrecon`/`MulModPrecon`; [selected sp_arith.h implementation](https://raw.githubusercontent.com/libntl/ntl/main/include/NTL/sp_arith.h) | Fixed-multiplier reciprocal and vector products. NTL's `NTL_SP_BOUND` restriction differs from this implementation's full-word domain; no source copied. |

The 2026-10-07 catalog sweep is in [00-sources.md](00-sources.md) ("Fetched 2026-10-07 (P004 re-audit sweep)"). Independently written; no external code copied.

## Limits and handoffs

GCC 14 and Windows were not executed. The `Multiplier` has no quotient. Signed-dividend reduction and residue inverses stay with `05-modint.hpp`. Lazy Barrett outputs, a divisibility test and the other left-out candidates are listed with reasons in [00-notes.md](00-notes.md#reduction-and-modular-types). No P004-owned gap remains.
