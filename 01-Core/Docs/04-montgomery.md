# 04-montgomery.hpp — evidence

`04-montgomery.hpp` (C02, package P004, together with `03-barrett.hpp`; see [03-barrett.md](03-barrett.md)) provides machine-word Montgomery REDC for odd moduli: canonical and lazy residues, conversion in and out, scalar operations and AVX2 32-bit bulk kernels. Backend choice, the shared benchmark method and the research/ownership boundary for the reduction family are in [00-notes.md](00-notes.md#reduction-family).

## Contracts

### Common machine-word rules

Let `w` be 32 or 64 and `R = 2^w`. A context's fields are public for contest use but must not be modified. Copying or moving contexts creates independent values; there are no caches, allocation, global modulus, reset operation, or CPU runtime dispatch. Scalar kernels use unsigned arithmetic and GNU double-width integers; the inverse and carries intentionally use unsigned wraparound. Context storage and per-operation scratch are O(1); construction includes native division for precomputation. Exponentiation takes O(1+log(e+1)) word operations for exponent e and bulk operations take O(n).

All bulk APIs use pointer/count overloads with an `int n >= 0`; pointers address `n` words, with no alignment requirement. Bulk operand ranges are asserted once after the loop through `Guard`; count and pointer preconditions are asserted at entry. Empty calls permit null pointers. Buffers must be disjoint or have exactly the same start; partial overlap is outside the contract. Products support output aliasing either/both inputs. Conversion arrays support exact in-place operation. Modulus zero and other domain violations are asserted in checked builds and remain preconditions under `NDEBUG`. Modulus one returns zero throughout, including exponent zero. No primality assumption or residue inverse belongs to this reducer. `LOCAL` does not affect the header.

| API | Domain and result |
|---|---|
| `Montgomery32(m)`, `Montgomery64(m)`; `Montgomery` aliases the latter | Every **odd** nonzero unsigned word modulus, including one. Default construction remains modulus one. |
| `red(x)` | `0 <= x < m * R`; returns `x * R^-1 mod m` canonically. This is a bounded REDC operation, distinct from Barrett's arbitrary-dividend reduction. |
| `init(a)`, `get(a)` | `init` accepts any ordinary word and returns its canonical Montgomery representation `a * R mod m`; `get` accepts a canonical Montgomery residue and returns its ordinary value. |
| `redc(x, lazy)` | Unchecked REDC core shared by every scalar operation and the bulk loops; same domain as `red` (`lazy=false`) or `redLazy` (`lazy=true`). |
| `mul(a,b)`, `add(a,b)`, `sub(a,b)`, `powMont(a,e)` | Canonical Montgomery operands `<m`; results remain canonical Montgomery residues. `add`/`sub` handle the carry and borrow past the word for `m > R/2`. |
| `pow(a,e)` | Ordinary word base and ordinary canonical result, with conversion included; preserves the old API. |
| `redLazy(x)`, `normalize(a)` | `m < R/2`; REDC retains `x < m * R` and returns `<2m`. Normalization accepts `<2m` and subtracts `m` if needed. |
| `mulLazy(a,b)`, `addLazy(a,b)`, `subLazy(a,b)` | `m < R/4`, operands `<2m`; results `<2m` can be chained without canonicalization. |
| `Guard` (`note`, `ok`) | Deferred operand check used by the bulk products and `get`: notes the running maximum of words or 32-bit lanes inside the kernel loop; one assertion after the loop reads `ok`. Dead code under `NDEBUG`. |

### MontgomeryBackend

`MontgomeryBackend<T>` is a public class template (a detail alias would change template identity and specialization/CTAD behavior); `Montgomery32`, `Montgomery64` and `Montgomery` are its aliases (with `Wide` and `BITS`). `mod`, `inv = -mod^-1 mod R` and `rsq = R^2 mod mod` are immutable after construction. `add` and `sub` use the same formulas for ordinary residues. Scalar bulk loops run the unchecked `redc` shared by every scalar operation; the AVX2 loops (Montgomery32 only) run `red4`/`red8`, with `red8` factoring the even/odd lane packing shared by `mul8` and the bulk `get`, and `load`/`store` helpers. Even moduli belong to Barrett32/Barrett64. Compare lazy values only after normalization.

### Guard

`note` keeps the running maximum of words or 32-bit lanes branch-free; `ok`, read by one assertion after the loop, is true iff every noted value was below `top >= 1`. `ok` evaluates the lane test only for `BITS == 32` (no narrowing of a 64-bit `top`, no spare `vptest`). Checked builds keep the kernels' cost, and `NDEBUG` drops the check entirely (no Guard instructions in the `-O2 -mavx2` assembly of both widths). O(1) time per `note` and `ok`, O(1) memory.

### Arithmetic justification

**Montgomery.** For odd `m`, Newton lifting computes `inv = -m^-1 mod R`: each update doubles the number of correct low bits. With `q = low(x)*inv mod R`, `x+q*m` is divisible by `R`. The bound `x<m*R` gives `u=(x+q*m)/R<2m`. Full-width moduli require an extra carry beyond the double-width sum; the implementation retains it when deciding whether to subtract `m`. `R^2 mod m` permits conversion through one REDC.

**Sums.** For canonical `a, b < m` the true sum is `< 2m`; it exceeds the word exactly when the wrapped `s = a + b` satisfies `s < a`, and then `s - m` (mod `R`) equals the true sum minus `m`, which lies in `[0, m)`; without wrap one conditional subtraction suffices. For `a < b`, `a - b + m` wraps back into `[0, m)`. For `m < R/4` and operands `< 2m`, `addLazy` forms `s < 4m < R` and `subLazy` forms `d = a - b + 2m` in `(0, 4m)`; one conditional subtraction of `2m` returns both to `[0, 2m)`.

**Guard.** The running maximum of words is below `top` iff every word is. For 32-bit lanes with `t = top - 1 >= 0`, `max_epu32(acc, t) xor t` is zero in a lane iff that lane is `<= t`, so `testz` of the vector is true iff every noted lane was below `top`; `top` is `m >= 1` or `2m >= 2`.

**Lazy ranges.** Omitting the canonical subtraction requires `2m<R` for a word result. Multiplying arbitrary representatives `<2m` requires `4m^2<m*R`, hence `m<R/4`. These narrower lazy contracts do not reduce the full-width canonical domain. These bounds also govern SIMD; signed vector comparisons require a separate proven nonnegative range or unsigned comparison emulation.

## Feature-to-test map

The Python entry resolves paths independently of the working directory, uses only Python's standard library and GCC, reports seeds/configurations and first failing inputs, retains its oracles under `NDEBUG`, and executes precondition failures in subprocesses.

| Operation | Test | Oracle |
|---|---|---|
| Construction, `red`, `init`, `get`, `mul`, `powMont`, `pow` | `04-montgomery_tester.cpp` | Independent native-wide `%` oracle computing `R^-1` by repeated exact modular halving, independent of Newton/REDC; full odd word range, power-of-two neighbors, modulus one, zero/max exponent, copies/moves/assignment and repeated calls |
| Carries and lazy forms (`redLazy`, `normalize`, `mulLazy`) | `04-montgomery_tester.cpp` | Deterministic `m=R-1, a=b=R-2` regression forces a carry past the double word; `x=mR-1`; exhaustive canonical/lazy operands, output bounds and chained lazy products, both lazy modulus boundaries |
| Bulk arrays and preconditions | `04-montgomery_tester.cpp` | Empty/null ranges, vector threshold/tail sizes and eight offsets, guard words, all exact product aliases, conversion in/out and in-place, lazy array chains; 100 assertion death cases per checked build (50 per width) |
| `redc` | `04-montgomery_tester.cpp` | Canonical and lazy core against the oracle on every boundary dividend of every boundary modulus; every other operation routes through it |
| `add`, `sub` | `04-montgomery_tester.cpp` | Oracle `(a+b) % m`, `(a-b) mod m` on all boundary pairs and random pairs per width, decoded through `get`; exhaustive odd `m <= 65/129`; carry past the word (`m = R-1`, `a = R-2, b = R-3`) and borrow (`0 - (R-2)`) |
| `addLazy`, `subLazy` | `04-montgomery_tester.cpp` | Output bound `< 2m` and normalized value against the oracle on boundary pairs shifted by `m`, exhaustive `a, b < 2m` for odd `m <= 65/129`, chained `subLazy(addLazy(..), subLazy(..))` |
| `Guard` | `04-montgomery_tester.cpp` | Direct: all-canonical and all-lazy arrays accepted; every single position raised to `m` or to the word maximum rejected, for `n` in {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 65, 257}, with 8-lane notes under AVX2 for Montgomery32; indirect: 32 bulk precondition deaths per width across vector, tail and `n = 8` positions |
| `red8`, `load`, `store` | `04-montgomery_tester.cpp` | Through every bulk operation in the optimized and checked AVX2 builds (sizes/offsets/aliases above) |

Quick mode covers every public operation with smaller exhaustive/random corpora. Full covers odd moduli through 65 and 10,000 random cases per width; stress covers through 129 and 100,000 random cases per width. Full/stress execute optimized `-O3 -DNDEBUG`, checked, and ASan/UBSan scalar/AVX2 paths. Unsupported AVX2 hardware is reported as a skip.

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261007 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261007): **3,699,028 checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 100 precondition deaths in each checked build; standalone/multiple-TU build) in 47.28 s. Stress (seed 1, one round): **23,203,772 checks in each of six configurations** in 50.75 s, with the same deaths and multiple-TU builds. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, the Workspace snapshot and the sanitizer self-tests. `03-consistency.py` reports no errors. Independent review (`@reviewer`, 2026-10-07) found no correctness defect: an 8.13-million-case differential against native arithmetic under ASan/UBSan (sums, differences, lazy forms at `2m-1`/`2m-2`, every 8-lane Guard verdict for seven `top` values) passed, and every header mutant was caught.

## Benchmarks

Method, corpus and artifact hashes: [00-notes.md](00-notes.md#reduction-family). Raw data: `96-Local Testing/01-Core/03-reduction_benchmark.jsonl`. Nanoseconds per complete workload, medians, accelerated (AVX2) `-O3 -DNDEBUG` build, 2026-10-07.

| Workload | Reference | Montgomery |
|---|---|---|
| 256 encoded products, `m=998244353` | Scalar Montgomery: 281 | AVX2: 111; lazy: 92 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,721 (Barrett: 1,572) | 1,374 |
| Eight powers including setup/conversions, `m=2^64-59` | Native `%`: 3,388 | 1,420 |

Deferred Guard, `Montgomery32`, `m=998244353`, 256 products: AVX2 bulk 110.5 ns (`-O3 -DNDEBUG`) versus 112.3 ns with assertions (`-O2`), so the Guard costs about 2% where per-element assertions cost 49%; a scalar loop of checked `mul` calls in the same assertion build takes 446 ns. Scalar builds with assertions: bulk 451.6 ns against 432.0 ns for a per-element-assert loop (+4.5%); `Montgomery64`, `m=2^64-59`: 443.5 against 421.7 (+5%). Rejected: a separate O(n) entry pass (+27% AVX2, +91% scalar) and a branch-free `bad |= x >= top` accumulator (+10–20% scalar).

Thresholds (median AVX2 bulk/scalar ratios, canonical / lazy multiplication, over `998244353`, `2^31-1`, `2^32-5`): 1.24 / 1.28 at 7, 0.53 / 0.57 at 8, 0.62 / 0.66 at 9, 0.39 / 0.37 at 256 elements; with assertions enabled 0.57 / 0.48 at 8. Both conversion kernels improve from 8 elements in both configurations (0.39/0.51 and 0.52/0.74). The eight-element dispatch threshold and scalar tails stand.

## Sources

| Source inspected (2026-09-27) | Relevant result and limits |
|---|---|
| Brent and Zimmermann, Modern Computer Arithmetic v0.5.9 (see [00-notes.md](00-notes.md#reduction-family)) | REDC, inverse lifting, Montgomery–Svoboda/FastREDC/McLaughlin. |
| [cp-algorithms: Montgomery Multiplication](https://cp-algorithms.com/algebra/montgomery_multiplication.html), June 8, 2022 revision | REDC representation, inverse doubling, wide carry handling, R² conversion; independently derived here. Page text CC BY-SA 4.0; no prose/code adaptation. |
| [Algorithmica: Montgomery Multiplication](https://en.algorithmica.org/hpc/number-theory/montgomery/) | Split-word reduction, lazy residues and optimization rationale. Its range assumptions and measurements cannot be transferred to full-word moduli. |
| [Nyaan SIMD Montgomery](https://raw.githubusercontent.com/NyaanNyaan/library/master/modint/simd-montgomery.hpp) | Even/odd 32-bit lane widening and packing; narrower lazy signed-comparison domain. CC0 1.0. |

The 2026-10-07 catalog sweep is in [00-sources.md](00-sources.md) ("Fetched 2026-10-07 (P004 re-audit sweep)"). Independently written; no external code copied.

Legacy accounting: the unchanged original is [OLD/5-Mathematics/09-montgomery.hpp](../../OLD/5-Mathematics/09-montgomery.hpp). Its public `Montgomery`, `mod/inv/rsq`, default constructor, `red/init/mul/pow` remain available. Previously the constructor required an odd modulus below `2^63`; canonical support now extends to every odd 64-bit modulus. The valid REDC dividend domain is explicit. The old code did not provide 32-bit, lazy, conversion-out, array, or SIMD interfaces. No legacy material is deleted and no dependent modular integer API is changed.

## Limits and handoffs

GCC 14 and Windows were not executed. The AVX2 kernels are 32-bit only (no 64x64 vector multiply in AVX2). Residue inverses stay with `05-modint.hpp`, whose `powerMontgomery` repeats `powMont` for 128-bit exponents (a P005 decision whether to widen the backend exponent). Left-out candidates (`neg`, `one`, lazy equality, `square`, 64-bit/AVX-512/IFMA/signed lanes) are listed with reasons in [00-notes.md](00-notes.md#reduction-and-modular-types).

## History

- 2026-09-27: original P004 completion, quick, full, stress (8,889,119 checks per configuration) and integration passed; first benchmarks.
- 2026-09-27: maintenance review, full suite, integration and consistency passed.
- 2026-10-07: re-audit, 2 findings fixed (complexity comment, per-element bulk assertions replaced by `Guard`), `add`/`sub`/`addLazy`/`subLazy` and shared `redc` added, full and stress passed on g++.
