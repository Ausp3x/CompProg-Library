# 04-montgomery.hpp — Evidence

`04-montgomery.hpp` (C02, package P004 with `03-barrett.hpp`; see [03-barrett.md](03-barrett.md)) provides machine-word Montgomery REDC for odd moduli: canonical and lazy residues, conversion in and out, scalar operations and AVX2 32-bit bulk kernels. Backend choice and the ownership boundary of the reduction family are in [00-notes.md](00-notes.md#reduction-family); the shared benchmark method is in [03-barrett.md](03-barrett.md#benchmarks).

## Contracts

### Common Machine-Word Rules

Let `w` be 32 or 64 and `R = 2^w`. A context's fields are public for contest use but must not be modified. Copies and moves are independent values; there are no caches, allocation, global modulus, reset operation or run-time CPU dispatch. Scalar kernels use unsigned arithmetic and GNU double-width integers; the inverse and the carries rely on unsigned wraparound. Context storage and per-operation scratch are O(1); construction divides natively for its precomputation. A power takes O(1 + log(e + 1)) word operations for exponent e, and bulk operations take O(n).

Every bulk API takes a pointer and an `int n >= 0` count; pointers address `n` words, with no alignment requirement. Count and pointer preconditions are asserted at entry, and operand ranges once after the loop through `Guard`; an empty call may pass null pointers. Buffers must be disjoint or start at exactly the same address; partial overlap is outside the contract. Products may write over either or both inputs, and conversion arrays work exactly in place. Modulus zero and the other domain violations are asserted in checked builds and remain preconditions under `NDEBUG`. Modulus one returns zero throughout, exponent zero included. The reducer assumes no primality and offers no residue inverse. `LOCAL` does not affect the header.

| API | Domain and result |
|---|---|
| `Montgomery32(m)`, `Montgomery64(m)`; `Montgomery` aliases the latter | Every **odd** nonzero unsigned word modulus, including one. Default construction remains modulus one. The constructor is `constexpr`: a context of a compile-time modulus is a compile-time constant, and a `constinit` context can be rebuilt at run time (`05-modint.hpp` keeps one per modular context). |
| `red(x)` | `0 <= x < m * R`; returns `x * R^-1 mod m` canonically. This is a bounded REDC operation, distinct from Barrett's arbitrary-dividend reduction. |
| `init(a)`, `get(a)` | `init` accepts any ordinary word and returns its canonical Montgomery representation `a * R mod m`; `get` accepts a canonical Montgomery residue and returns its ordinary value. |
| `redc(x, lazy)` | Unchecked REDC core shared by every scalar operation and the bulk loops; same domain as `red` (`lazy=false`) or `redLazy` (`lazy=true`). |
| `mul(a,b)`, `add(a,b)`, `sub(a,b)`, `powMont(a,e)` | Canonical Montgomery operands `<m`; results remain canonical Montgomery residues. `add`/`sub` handle the carry and borrow past the word for `m > R/2`; `powMont` takes any exponent up to 128 bits (`ulll`), `e = 0` giving `init(1)`. |
| `pow(a,e)` | Ordinary word base, any exponent up to 128 bits (`ulll`) and ordinary canonical result (`1 % m` for `e = 0`), with conversion included. |
| `redLazy(x)`, `normalize(a)` | `m < R/2`; REDC retains `x < m * R` and returns `<2m`. Normalization accepts `<2m` and subtracts `m` if needed. |
| `mulLazy(a,b)`, `addLazy(a,b)`, `subLazy(a,b)` | `m < R/4`, operands `<2m`; results `<2m` can be chained without canonicalization. |
| `Guard` (`note`, `ok`) | Deferred operand check used by the bulk products and `get`: notes the running maximum of words or 32-bit lanes inside the kernel loop; one assertion after the loop reads `ok`. Dead code under `NDEBUG`. |

### MontgomeryBackend

`MontgomeryBackend<T>` is a public class template (a detail alias would change template identity and specialization and CTAD behavior); `Montgomery32`, `Montgomery64` and `Montgomery` are its aliases, with `Wide` and `BITS`. `mod`, `inv = -mod^-1 mod R` and `rsq = R^2 mod mod` are immutable after construction and constant-evaluated when the modulus is. `add` and `sub` use the same formulas for ordinary residues. Scalar bulk loops run the unchecked `redc` that every scalar operation shares. The AVX2 loops (Montgomery32 only) run `red4` and `red8` (`red8` holds the even/odd lane packing that `mul8` and the bulk `get` share) with the `load` and `store` helpers; these kernels are internal and reached only through the checked bulk APIs. Even moduli belong to Barrett32 and Barrett64. Compare lazy values only after normalization. `powMont` runs a 128-bit loop only while the exponent's high word is set and then the plain word loop of `Barrett::pow`, so exponents below `2^64` run only the word loop (Benchmarks); it asserts its base once at entry and runs the unchecked `redc` inside the loops.

### Guard

`note` keeps the running maximum of words or 32-bit lanes without a branch; `ok`, read by one assertion after the loop, is true iff every noted value was below `top >= 1`. `ok` evaluates the lane test only for `BITS == 32` (no narrowing of a 64-bit `top`, no spare `vptest`). Checked builds keep the kernels' cost, and `NDEBUG` drops the check entirely (no Guard instructions in the `-O2 -mavx2` assembly of either width). `note` and `ok` take O(1) time and memory.

### Arithmetic Justification

**Montgomery.** For odd `m`, Newton lifting computes `inv = -m^-1 mod R`, each update doubling the correct low bits. With `q = low(x)*inv mod R`, `x + q*m` is divisible by `R`, and `x < m*R` gives `u = (x + q*m)/R < 2m`. Full-width moduli need the carry beyond the double-width sum, which the implementation keeps when deciding whether to subtract `m`. `R^2 mod m` gives conversion in one REDC.

**Sums.** For canonical `a, b < m` the true sum is `< 2m`; it exceeds the word exactly when the wrapped `s = a + b` satisfies `s < a`, and then `s - m` (mod `R`) equals the true sum minus `m`, which lies in `[0, m)`; without wrap one conditional subtraction suffices. For `a < b`, `a - b + m` wraps back into `[0, m)`. For `m < R/4` and operands `< 2m`, `addLazy` forms `s < 4m < R` and `subLazy` forms `d = a - b + 2m` in `(0, 4m)`; one conditional subtraction of `2m` returns both to `[0, 2m)`.

**Guard.** The running maximum of words is below `top` iff every word is. For 32-bit lanes with `t = top - 1 >= 0`, `max_epu32(acc, t) xor t` is zero in a lane iff that lane is `<= t`, so `testz` of the vector is true iff every noted lane was below `top`; `top` is `m >= 1` or `2m >= 2`.

**Lazy ranges.** Omitting the canonical subtraction needs `2m < R` for a word result. Multiplying arbitrary representatives `< 2m` needs `4m^2 < m*R`, hence `m < R/4`. These narrower lazy contracts leave the full-width canonical domain intact. The same bounds govern SIMD; signed vector comparisons need a proven nonnegative range or emulated unsigned comparison.

## Feature-to-Test Map

Harness: [`05-testing.md`](../../00-Guidelines/05-testing.md); the entry uses only Python's standard library and GCC.

| Operation | Test | Oracle |
|---|---|---|
| Construction, `red`, `init`, `get`, `mul`, `powMont`, `pow` | `04-montgomery_tester.cpp` | Independent native-wide `%` oracle computing `R^-1` by repeated exact modular halving, independent of Newton/REDC; full odd word range, power-of-two neighbors, modulus one, exponents 0, 1, 2, 63, `2^64-1`, `2^64`, `2^64+1`, `(2^64-1)·2^64` and `2^128-1` plus a seeded random 128-bit exponent whose `powMont` is also split as `(a^(2^64))^hi · a^lo` over 64-bit powers, copies/moves/assignment and repeated calls |
| Carries and lazy forms (`redLazy`, `normalize`, `mulLazy`) | `04-montgomery_tester.cpp` | Deterministic `m=R-1, a=b=R-2` regression forces a carry past the double word; `x=mR-1`; exhaustive canonical/lazy operands, output bounds and chained lazy products, both lazy modulus boundaries |
| Bulk arrays and preconditions | `04-montgomery_tester.cpp` | Empty/null ranges, vector threshold/tail sizes and eight offsets, guard words, all exact product aliases, conversion in/out and in-place, lazy array chains; 100 assertion death cases per checked build (50 per width) |
| `redc` | `04-montgomery_tester.cpp` | Canonical and lazy core against the oracle on every boundary dividend of every boundary modulus; every other operation routes through it |
| `add`, `sub` | `04-montgomery_tester.cpp` | Oracle `(a+b) % m`, `(a-b) mod m` on all boundary pairs and random pairs per width, decoded through `get`; exhaustive odd `m <= 65/129`; carry past the word (`m = R-1`, `a = R-2, b = R-3`) and borrow (`0 - (R-2)`) |
| `addLazy`, `subLazy` | `04-montgomery_tester.cpp` | Output bound `< 2m` and normalized value against the oracle on boundary pairs shifted by `m`, exhaustive `a, b < 2m` for odd `m <= 65/129`, chained `subLazy(addLazy(..), subLazy(..))` |
| `Guard` (`note`, `ok`, `top`, `mx`, `acc`) | `04-montgomery_tester.cpp` | Direct: all-canonical and all-lazy arrays accepted; every single position raised to `m` or to the word maximum rejected, for `n` in {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 65, 257}, with 8-lane notes under AVX2 for Montgomery32; indirect: 32 bulk precondition deaths per width across vector, tail and `n = 8` positions |
| `MontgomeryBackend`, `mod`, `inv`, `rsq`, `Wide`, `BITS`, `Montgomery` alias, constexpr construction | `04-montgomery_tester.cpp` | `mod * inv == R - 1` for every boundary modulus (`negative inverse`), `rsq` against the square of the oracle's independently computed `R mod m`, `static_assert` on the alias identity and on compile-time construction (`mod`, `inv` and `rsq` of `Montgomery32/64(998244353)` against `R - 1` and `(R mod m)^2 mod m`; `inv == R - 1`, `rsq == 0` for the default modulus); `Wide` and `BITS` drive the oracle's radix and every dividend bound |
| `red4`, `red8`, `mul8`, `load`, `store` | `04-montgomery_tester.cpp` | Through every bulk operation in the optimized and checked AVX2 builds (sizes/offsets/aliases above) |

Quick mode covers every public operation with smaller exhaustive and random corpora. Full covers odd moduli through 65 and 10,000 random cases per width; stress covers moduli through 129 and 100,000 random cases per width. Full and stress run the optimized `-O3 -DNDEBUG`, checked and ASan/UBSan builds on the scalar and AVX2 paths.

## Commands and Results

Latest header run, 2026-10-12 (cached contexts: the constructor became `constexpr`, so `05-modint.hpp` keeps one context per modulus, and `powMont` loops on the unchecked `redc`; the same day's prune widened the exponents and pruned `03-barrett.hpp`):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/04-montgomery.hpp '96-Local Testing/01-Core/04-montgomery_tester.cpp'
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261012 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'   # the prune run of the same day; the constructor change touches no measured path
```

All passed on GCC 16.2.1 and GCC 14.4.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every tester build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261012): **3,715,606 checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 100 precondition deaths per checked build; standalone and multiple-TU build; compile-time construction `static_assert`s), 62.23 s on g++ and 65.58 s on g++-14 beside the other suites, memory peak 636 MB and 634 MB under the 4096 MB cap. `02-integration.py` passed 136 standalone and aggregate headers, scalar and AVX2 multiple-translation-unit linkage and the Workspace snapshot; `03-consistency.py` reported no errors and no brace, comment-cap or free-function violations. The benchmark run verified all 9,116 measurements against native wide `%`.

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, run beside the other suites, build cache emptied before each cold run), all PASS:

| Command | Compiler | This suite | Memory peak |
|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` | GCC 16.2.1 | preflight quick peak 1109 MB; 6 builds, 39.0 s compiling, 3.9 s running, longest run 1.0 s | 1061 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | GCC 14.4.1 | preflight quick peak 1051 MB; 6 builds, 53.8 s compiling, 4.5 s running, longest run 1.5 s | 1045 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | GCC 16.2.1 | optimized-scalar 18.8 s compile + 0.1 s run, checked-scalar 15.4 s + 0.3 s, optimized-avx2 21.0 s + 0.1 s, checked-avx2 10.7 s + 0.3 s | 1218 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | GCC 16.2.1 | preflight quick peak 49 MB; 6 builds, 15.8 s compiling, 3.9 s running, longest run 1.3 s | 1073 MB |

## Benchmarks

Method and corpus: [03-barrett.md](03-barrett.md#benchmarks). Raw data: `96-Local Testing/01-Core/03-reduction_benchmark.jsonl`. Nanoseconds per complete workload, medians of five repetitions, accelerated (AVX2) `-O3 -DNDEBUG` build, 2026-10-12 (seed 20261012).

| Workload | Reference | Montgomery |
|---|---|---|
| 256 encoded products, `m=998244353` | Scalar Montgomery: 254 | AVX2: 102; lazy: 81 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,489 (Barrett: 1,444) | 1,260 |
| Eight powers including setup/conversions, `m=2^64-59` | Native `%`: 2,681 | 1,361 |
| Eight chained powers on one context, 64-bit exponents, `m=998244353` | Native `%`: 2,817; Barrett: 1,634 | 1,289 |
| Eight chained powers on one context, 64-bit exponents, `m=2^64-59` | Native `%`: 2,923; Barrett: 3,175 | 1,424 |
| Eight chained powers on one context, 128-bit exponents, `m=2^64-1` | Native `%`: 5,837 | 2,732 |

Exponent widening. `dependent-pow` chains eight `pow` calls on one context, each base the previous result, so the exponent loop's own latency shows. `powMont` runs the 128-bit loop only while the exponent's high word is set, because a single 128-bit loop cost 3–16% on `Montgomery32` and 8–10% on `Montgomery64` chains of 64-bit exponents (the independent review's isolated `pow` loop at `-O2`: +26% and +6%). Two-loop form, AVX2 build, nanoseconds per eight chained powers with 64-bit exponents (the `Barrett64` and native references are in the table above):

| Context and modulus | Nanoseconds |
|---|---|
| `Montgomery32`, `m=998244353` | 1,289 |
| `Montgomery32`, `m=2^31-1` | 1,266 |
| `Montgomery32`, `m=2^32-5` | 1,329 |
| `Montgomery64`, `m=2^61-1` | 1,298 |
| `Montgomery64`, `m=2^64-59` | 1,424 |

128-bit exponents cost 2,631 ns (`Montgomery32`, `m=998244353`) and 2,732–3,016 ns (`Montgomery64`) per eight chained powers, about twice the 64-bit chains, as the bit count predicts. The native 128-bit cell for `2^64-59` in this run (14,658 ns against 5,980–6,002 ns in the other three configurations) is a transient and is not used.

Deferred Guard, `Montgomery32`, `m=998244353`, 256 products: AVX2 bulk 102 ns (`-O3 -DNDEBUG`) versus 121 ns with assertions (`-O2`), while a scalar loop of per-call checked `mul` in the same assertion build takes 458 ns; scalar bulk with assertions 425 ns (`Montgomery64`, `m=2^64-59`: 401 ns). Per-element assertions, a separate O(n) entry pass and a branch-free `bad |= x >= top` accumulator measured slower on 2026-10-07 and were rejected.

Thresholds (median AVX2 bulk/scalar ratios, canonical / lazy multiplication, over `998244353`, `2^31-1`, `2^32-5`; the lazy kernel exists only for `998244353 < R/4`): 1.20 / 1.20 at 7, 0.44 / 0.43 at 8, 0.51 / 0.54 at 9, 0.40 / 0.36 at 256 elements; with assertions enabled 0.57 / 0.49 at 8. The lazy ratio at 8 is read from the near-modulus cell, since this run's uniform cell (14.1 ns against 3.1 ns near-modulus and 4.4 ns at 9) is a transient. Both conversion kernels gain from 8 elements in both configurations (`init` 0.37/0.50, `get` 0.49/0.69). The eight-element dispatch threshold and the scalar tails stand.

## Sources

| Source inspected (2026-09-27) | Relevant result and limits |
|---|---|
| Brent and Zimmermann, [Modern Computer Arithmetic, v0.5.9 (2010-10-07)](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf), §§2.4.1–2.4.4 and §2.5 ([saved copy](../../95-Resources/05-modern_computer_arithmetic.pdf); reading copy CC BY-NC-ND, no text copied) | REDC, inverse lifting, Montgomery–Svoboda/FastREDC/McLaughlin. |
| [cp-algorithms: Montgomery Multiplication](https://cp-algorithms.com/algebra/montgomery_multiplication.html), June 8, 2022 revision | REDC representation, inverse doubling, wide carry handling, R² conversion; independently derived here. Page text CC BY-SA 4.0; no prose/code adaptation. |
| [Algorithmica: Montgomery Multiplication](https://en.algorithmica.org/hpc/number-theory/montgomery/) | Split-word reduction, lazy residues and optimization rationale. Its range assumptions and measurements cannot be transferred to full-word moduli. |
| [Nyaan SIMD Montgomery](https://raw.githubusercontent.com/NyaanNyaan/library/master/modint/simd-montgomery.hpp) | Even/odd 32-bit lane widening and packing; narrower lazy signed-comparison domain. CC0 1.0. |

The 2026-10-07 catalog sweep is in [00-sources.md](00-sources.md) ("Fetched 2026-10-07 (P004 re-audit sweep)"). Independently written; no external code copied.

Legacy accounting: the unchanged original is [OLD/5-Mathematics/09-montgomery.hpp](../../OLD/5-Mathematics/09-montgomery.hpp). Its public `Montgomery`, `mod`/`inv`/`rsq`, default constructor and `red`/`init`/`mul`/`pow` remain. Its constructor required an odd modulus below `2^63`; this header accepts every odd word modulus and states the REDC dividend domain. The 32-bit, lazy, conversion-out, array and SIMD interfaces are new.

## Limits and Handoffs

- The AVX2 kernels are 32-bit only (AVX2 has no 64 × 64 vector multiply).
- Residue inverses stay with `05-modint.hpp`, whose power dispatch is in [05-modint.md](05-modint.md#pow).
- Left-out candidates (`neg`, `one`, lazy equality, `square`, 64-bit, AVX-512, IFMA and signed lanes) are listed with reasons in [00-notes.md](00-notes.md#reduction-and-modular-types).
- Verified before the mutation rule of 2026-11-03; no record yet.

## Aliases

Search aliases (P263, grab --find): 蒙哥马利模乘, 蒙哥马利约减, 蒙哥马利乘法, 快速取模, 取模优化.

## History

- 2026-09-27: original P004 completion, quick, full, stress (8,889,119 checks per configuration) and integration passed; first benchmark (seed 20260927, 4,412 verified measurements).
- 2026-09-27: maintenance review, full suite, integration and consistency passed.
- 2026-10-07: re-audit, 2 findings fixed, `Guard`, `add`/`sub`/`addLazy`/`subLazy` and shared `redc` added, full and stress passed on g++.
- 2026-10-07 (run): full (seed 20261007, 3,699,028 checks per configuration) and stress (seed 1, 23,203,772) passed, `02-integration.py --sanitizers` passed, independent `@reviewer` differential (8.13 million cases, every 8-lane Guard verdict) and mutants found no defect, benchmark (seed 20261007, 8,912 verified measurements).
- 2026-10-08: restyle (comments only), full suite passed on g++ and g++-14, integration and consistency passed.
- 2026-10-12: prune (`powMont`/`pow` exponents widened to 128 bits; Barrett's `div` and `multiplier` removed, `fold61` added), full suites passed on g++ and g++-14, 9,116 benchmark measurements verified.
- 2026-10-12 (cached contexts): constructor `constexpr` with compile-time `static_assert`s, `powMont` loops on the unchecked `redc`; full suites passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (`grab --find`), Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
