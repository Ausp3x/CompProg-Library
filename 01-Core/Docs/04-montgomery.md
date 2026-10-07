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

`MontgomeryBackend<T>` is a public class template; `Montgomery32`, `Montgomery64` and `Montgomery` are its aliases (with `Wide` and `BITS`). `mod`, `inv = -mod^-1 mod R` and `rsq = R^2 mod mod` are immutable after construction. `add` and `sub` use the same formulas for ordinary residues. Scalar bulk loops run the unchecked `redc` shared by every scalar operation; the AVX2 loops (Montgomery32 only) run `red4`/`red8`, with `red8` factoring the even/odd lane packing shared by `mul8` and the bulk `get`, and `load`/`store` helpers. Even moduli belong to Barrett32/Barrett64. Compare lazy values only after normalization.

### Guard

`note` keeps the running maximum of words or 32-bit lanes branch-free; `ok`, read by one assertion after the loop, is true iff every noted value was below `top >= 1`. `ok` evaluates the lane test only for `BITS == 32` (no narrowing of a 64-bit `top`, no spare `vptest`). Checked builds keep the kernels' cost, and `NDEBUG` drops the check entirely (no Guard instructions in the `-O2 -mavx2` assembly of both widths). O(1) time per `note` and `ok`, O(1) memory.

### Arithmetic justification

**Montgomery.** For odd `m`, Newton lifting computes `inv = -m^-1 mod R`: each update doubles the number of correct low bits. With `q = low(x)*inv mod R`, `x+q*m` is divisible by `R`. The bound `x<m*R` gives `u=(x+q*m)/R<2m`. Full-width moduli require an extra carry beyond the double-width sum; the implementation retains it when deciding whether to subtract `m`. `R^2 mod m` permits conversion through one REDC.

**Sums (2026-10-07).** For canonical `a, b < m` the true sum is `< 2m`; it exceeds the word exactly when the wrapped `s = a + b` satisfies `s < a`, and then `s - m` (mod `R`) equals the true sum minus `m`, which lies in `[0, m)`; without wrap one conditional subtraction suffices. For `a < b`, `a - b + m` wraps back into `[0, m)`. For `m < R/4` and operands `< 2m`, `addLazy` forms `s < 4m < R` and `subLazy` forms `d = a - b + 2m` in `(0, 4m)`; one conditional subtraction of `2m` returns both to `[0, 2m)`.

**Guard (2026-10-07).** The running maximum of words is below `top` iff every word is. For 32-bit lanes with `t = top - 1 >= 0`, `max_epu32(acc, t) xor t` is zero in a lane iff that lane is `<= t`, so `testz` of the vector is true iff every noted lane was below `top`; `top` is `m >= 1` or `2m >= 2`.

**Lazy ranges.** Omitting the canonical subtraction requires `2m<R` for a word result. Multiplying arbitrary representatives `<2m` requires `4m^2<m*R`, hence `m<R/4`. These narrower lazy contracts do not reduce the full-width canonical domain. These bounds also govern SIMD; signed vector comparisons require a separate proven nonnegative range or unsigned comparison emulation.

## Feature-to-test map

The Python entry resolves paths independently of the working directory, uses only Python's standard library and GCC, reports seeds/configurations and first failing inputs, retains its oracles under `NDEBUG`, and executes precondition failures in subprocesses.

| Operation | Test | Oracle |
|---|---|---|
| Construction, `red`, `init`, `get`, `mul`, `powMont`, `pow` | `04-montgomery_tester.cpp` | Independent native-wide `%` oracle computing `R^-1` by repeated exact modular halving, independent of Newton/REDC; full odd word range, power-of-two neighbors, modulus one, zero/max exponent, copies/moves/assignment and repeated calls |
| Carries and lazy forms (`redLazy`, `normalize`, `mulLazy`) | `04-montgomery_tester.cpp` | Deterministic `m=R-1, a=b=R-2` regression forces a carry past the double word; `x=mR-1`; exhaustive canonical/lazy operands, output bounds and chained lazy products, both lazy modulus boundaries |
| Bulk arrays and preconditions | `04-montgomery_tester.cpp` | Empty/null ranges, vector threshold/tail sizes and eight offsets, guard words, all exact product aliases, conversion in/out and in-place, lazy array chains; 68 assertion death cases in each checked scalar/AVX2 configuration (2026-09-27), 100 per checked build after the re-audit (50 per width) |
| `redc` | `04-montgomery_tester.cpp` | Canonical and lazy core against the oracle on every boundary dividend of every boundary modulus; every other operation routes through it |
| `add`, `sub` | `04-montgomery_tester.cpp` | Oracle `(a+b) % m`, `(a-b) mod m` on all boundary pairs and random pairs per width, decoded through `get`; exhaustive odd `m <= 65/129`; carry past the word (`m = R-1`, `a = R-2, b = R-3`) and borrow (`0 - (R-2)`) |
| `addLazy`, `subLazy` | `04-montgomery_tester.cpp` | Output bound `< 2m` and normalized value against the oracle on boundary pairs shifted by `m`, exhaustive `a, b < 2m` for odd `m <= 65/129`, chained `subLazy(addLazy(..), subLazy(..))` |
| `Guard` | `04-montgomery_tester.cpp` | Direct: all-canonical and all-lazy arrays accepted; every single position raised to `m` or to the word maximum rejected, for `n` in {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 65, 257}, with 8-lane notes under AVX2 for Montgomery32; indirect: 32 bulk precondition deaths per width across vector, tail and `n = 8` positions |
| `red8`, `load`, `store` | `04-montgomery_tester.cpp` | Through every bulk operation in the optimized and checked AVX2 builds (sizes/offsets/aliases above) |

Quick mode covers every public operation with smaller exhaustive/random corpora. Full covers odd moduli through 65 and 10,000 random cases per width; stress covers through 129 and 100,000 random cases per width. Full/stress execute optimized `-O3 -DNDEBUG`, checked, and ASan/UBSan scalar/AVX2 paths. Unsupported AVX2 hardware is reported as a skip.

## Commands and results

### Original P004 completion — 2026-09-27

GCC 16.2.1 (20260810), Linux x86-64, Intel i9-11900H.

```bash
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode quick --seed 20260928
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode stress --seed 42
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --repetitions 5 --milliseconds 1 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
```

Montgomery passed 774,252 quick, 1,788,164 full and 8,889,119 stress checks **per configuration**, plus its checked preconditions; full/stress passed all six configurations. Integration passed **59 standalone/aggregate headers**, scalar/AVX2 multiple-TU linkage and the Workspace snapshot. Sanitizer runs were repeated outside the sandbox with approved execution because LeakSanitizer cannot operate under its ptrace restriction; scalar and AVX2 ASan/UBSan executions then passed. No sanitizer finding was suppressed. No external test dependency; no online submissions.

### Maintenance review — 2026-09-27

The header now separates state/construction, scalar operations and array operations. SIMD pointer conversions use `reinterpret_cast`, and `int(rsq)` makes the intrinsic's existing 32-bit lane conversion explicit without changing its bit pattern. Public names, member order, construction order, operand domains, lazy bounds, scalar paths and eight-element AVX2 dispatch are preserved. The tester uses `int` for bounded check counters (the largest stress corpus stays under 32 million checks, below `INT_MAX`) and retains `ulng` seeds; array indices/counts use `int`; residues, wide intermediates and seeds remain unsigned. No namespace migration is appropriate: moving `MontgomeryBackend<T>` behind a detail alias would change template identity and possible specialization/CTAD behavior; its SIMD helpers remain methods. Local naming/grouping was aligned without changing cases, oracle operations or diagnostic strings; the few size conversions address fixed bounded test/benchmark buffers.

```bash
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

GCC 16.2.1 (20260810), CPython 3.14.7, Linux x86-64. Montgomery full: 1,788,164 checks in each of six configurations (optimized NDEBUG, checked and ASan/UBSan scalar/AVX2), 68 assertion cases in each checked configuration. Integration: 59 standalone/aggregate headers, scalar/AVX2 multiple-TU linkage and Workspace checks passed. Consistency: zero errors. Full suites used approved execution outside the sandbox to retain LeakSanitizer; leak checking was not disabled. The benchmark-source syntax checks of this review are recorded in [00-notes.md](00-notes.md#reduction-family).

| File | SHA256 at the 2026-09-27 maintenance (superseded) |
|---|---|
| `01-Core/04-montgomery.hpp` | `52bc741783aa6309f6ed16ec7700c048e20dbd46c5148ea03fd8bc2b91f1d075` |
| `96-Local Testing/01-Core/04-montgomery_tester.cpp` | `3cb237f3555df3cc5e61eb259793059677bf9b4b27480b585bf32033db7269a4` |

### Re-audit — 2026-10-07

Package P004 re-audit, treating the previous verification as existing-unverified. Every operation in the row was compared with the code and the tester before any edit; the unchanged full suite was rerun (6 configurations in 36.11 s, seed 20260927, passing); the header was then brought to the current style, extended from the catalog sweep and re-tested.

| # | Finding | Disposition |
|---|---|---|
| 3 | Complexity comment used the undefined symbol `e` | Fixed (`powers O(log(e+1)) for exponent e`). |
| 4 | Per-element assertions inside the bulk SIMD and scalar loops | Fixed: the loops call the unchecked core `redc`, and operand ranges are checked by `Guard`, a running maximum of words (`cmp`/`cmov`) or 32-bit lanes (one `vpmaxud`) that one assertion reads after the loop. Under `-DNDEBUG` the generated code contains none of the Guard instructions (checked on the `-O2 -mavx2` assembly of both widths). The benchmark gained `-O2` assertion-enabled configurations; results under Benchmarks. |

Changes beyond the findings: catalog sweep ([00-sources.md](00-sources.md), left-out items in [00-notes.md](00-notes.md#reduction-and-modular-types)) added `add`/`sub` on canonical residues (suisen, cp-algorithms) and `addLazy`/`subLazy` on `[0, 2m)` (Nyaan vectorize-modint/simd-montgomery). `redc(x, lazy)` replaces the duplicated `red`/`redLazy` bodies; `red8` factors the shared lane packing; the constructor parameter is `m`; `rsq` narrowing is explicit (`-Wconversion` clean, previously untested for this header). Header, tester and benchmark follow the closing-brace rule; the header compiles with `-Wall -Wextra -Wshadow -Wconversion -Werror` on scalar and AVX2 builds. The tester adds `-Wconversion -Werror`, a standalone/multiple-TU build in full and stress, direct `redc`/`Guard` coverage, exhaustive `add`/`sub`/`addLazy`/`subLazy`, full-width carry/borrow regressions at `m = R-1`, and 16 new precondition deaths (50 per width: canonical and lazy sums/differences, a bad lane at position 0 of 16 for the vector path, bad tail positions for every bulk operation).

```bash
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261007 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261007): **3,699,028 checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 100 precondition deaths in each checked build; standalone/multiple-TU build) in 47.28 s. Stress (seed 1, one round): **23,203,772 checks in each of six configurations** in 50.75 s, with the same deaths and multiple-TU builds. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, the Workspace snapshot and the sanitizer self-tests. `03-consistency.py` reports no errors (eight brace-checked files).

Independent review (`@reviewer`, 2026-10-07) found no correctness defect in either reduction header: it re-derived every argument, ran its own 8.13-million-case differential against native arithmetic under ASan/UBSan on scalar and AVX2 builds (sums, differences, lazy forms at `2m-1`/`2m-2`; every 8-lane Guard verdict for seven `top` values), and 15 header mutants across both headers (for Montgomery: missing `add` wrap, wrong `sub` borrow, unreduced lazy forms, Guard bound/sign/term deletions) that the testers all caught in `-DNDEBUG` builds or by death tests. It raised low findings, all fixed: `Wide`, `BITS` and the Guard fields added to the row; a complexity line on `Guard`; the sum and Guard arguments added to the justification. Its suggestions were applied: `Guard::ok` evaluates the lane test only for `BITS == 32`, the header comment names `red4`/`red8` for the AVX2 loops, and the Python entry no longer uses a backslash inside an f-string replacement field (a SyntaxError before Python 3.12; the floor is 3.10).

| File | SHA256 after the re-audit |
|---|---|
| `01-Core/04-montgomery.hpp` | `6f5c204baf5d256483aabb0320a4a289769f4d841fc334ba66c20bc8a0acf62f` |
| `96-Local Testing/01-Core/04-montgomery_tester.cpp` | `a7e4a8c1e6b868af61a1837dcdcb097c33ca8178ca6542903e54998b87de39f1` |
| `96-Local Testing/01-Core/04-montgomery_tester.py` | `1aa0b46eaae9e7e906d9337d2e356e63520e65f7fe899ad0857fdada6c95e56d` |

## Benchmarks

Method, corpus and artifact hashes: [00-notes.md](00-notes.md#reduction-family). Raw data: `96-Local Testing/01-Core/03-reduction_benchmark.jsonl`. Nanoseconds per complete workload, medians, accelerated (AVX2) `-O3 -DNDEBUG` build.

| Workload | Reference | Montgomery, 2026-09-27 | Montgomery, 2026-10-07 |
|---|---|---|---|
| 256 encoded products, `m=998244353` | Scalar Montgomery: 282 / 281 | AVX2: 114; lazy: 90 | AVX2: 111; lazy: 92 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,740 / 2,721 (Barrett: 1,724 / 1,572) | 1,377 | 1,374 |
| Eight powers including setup/conversions, `m=2^64-59` | Native `%`: 2,783 / 3,388 | 1,396 | 1,420 |

Reference columns give 2026-09-27 / 2026-10-07 values.

Deferred Guard (finding 4), `Montgomery32`, `m=998244353`, 256 products: AVX2 bulk 110.5 ns (`-O3 -DNDEBUG`) versus 112.3 ns with assertions (`-O2`), so the deferred Guard costs about 2% where the per-element assertions cost 49%; a scalar loop of checked `mul` calls in the same assertion build takes 446 ns. Scalar builds with assertions: bulk 451.6 ns against 432.0 ns for the per-element-assert loop (+4.5%); `Montgomery64`, `m=2^64-59`: 443.5 against 421.7 (+5%). An earlier candidate, a separate O(n) entry pass, cost +27% (AVX2) and +91% (scalar) in the assertion builds and was replaced by the in-loop maximum; a branch-free `bad |= x >= top` accumulator cost +10–20% in scalar builds against the maximum's +5%.

Thresholds (median AVX2 bulk/scalar ratios, canonical / lazy multiplication): 2026-09-27, across tested non-power-of-two 32-bit moduli, 0.40 / 0.45 at eight and 0.50 / 0.56 at nine elements, and both conversion kernels improved at eight elements. 2026-10-07 over `998244353`, `2^31-1`, `2^32-5`: 1.24 / 1.28 at 7, 0.53 / 0.57 at 8, 0.62 / 0.66 at 9, 0.39 / 0.37 at 256 elements; with assertions enabled 0.57 / 0.48 at 8. Both conversion kernels improve from 8 elements in both configurations (0.39/0.51 and 0.52/0.74). These support the eight-element dispatch threshold and scalar tails; the dispatch stands.

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

GCC 14 and Windows were not executed. The AVX2 kernels are 32-bit only (no 64x64 vector multiply in AVX2). Residue inverses stay with `05-modint.hpp`, whose `powerMontgomery` repeats `powMont` for 128-bit exponents (a P005 decision whether to widen the backend exponent). Left-out candidates (`neg`, `one`, lazy equality, `square`, 64-bit/AVX-512/IFMA/signed lanes) are listed with reasons in [00-notes.md](00-notes.md#reduction-and-modular-types). No P004-owned gap remains.
