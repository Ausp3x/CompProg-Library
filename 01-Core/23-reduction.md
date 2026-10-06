# C02 — machine-word Barrett and Montgomery reduction

P004 owns `03-barrett.hpp`, `04-montgomery.hpp` and their verification. C01/P002 supplies the verified width aliases. C03 owns modular integer types; C04 and C11 own arbitrary-precision and polynomial backends. This document specifies machine-word arithmetic, not a claim that every modular arithmetic research variant is implemented.

## Contracts and backend choice

Let `w` be 32 or 64 and `R = 2^w`. A context's fields are public for contest use but must not be modified. Copying or moving contexts creates independent values; there are no caches, allocation, global modulus, reset operation, or CPU runtime dispatch. Scalar kernels use unsigned arithmetic and GNU double-width integers. Context storage and per-operation scratch are O(1); general construction includes native division for precomputation, while Barrett's power-of-two and Mersenne61 contexts skip it. Exponentiation takes O(1+log(e+1)) word operations and bulk operations take O(n).

| API | Domain and result |
|---|---|
| `Barrett32(m)`, `Barrett64(m)`; `Barrett` aliases the latter | Every nonzero unsigned word modulus, including one and even moduli; the default modulus is one. |
| `divMod(x)`, `div(x)`, `reduce(x)` | Every unsigned double-width dividend, including its maximum; `divMod` returns the exact quotient (double width) and `x % m` in `[0,m)`, `div`/`reduce` return one of them. The mask, Mersenne61 and reciprocal paths all derive the quotient from the same folds. |
| `mul(a,b)`, `pow(a,e)` | Arbitrary unsigned word inputs and unsigned 64-bit exponent. Ordinary residues in/out; `0^0 = 1 % m`. |
| `multiplier(b)` | Precompute a fixed multiplier using a Shoup reciprocal; owns its modulus and canonicalized multiplier independently of the original context. Its `mul(a)` accepts any word. |
| `Montgomery32(m)`, `Montgomery64(m)`; `Montgomery` aliases the latter | Every **odd** nonzero unsigned word modulus, including one. Existing default construction remains modulus one. |
| `red(x)` | `0 <= x < m * R`; returns `x * R^-1 mod m` canonically. This is a bounded REDC operation, distinct from Barrett's arbitrary-dividend reduction. |
| `init(a)`, `get(a)` | `init` accepts any ordinary word and returns its canonical Montgomery representation `a * R mod m`; `get` accepts a canonical Montgomery residue and returns its ordinary value. |
| `redc(x, lazy)` | Unchecked REDC core shared by every scalar operation and the bulk loops; same domain as `red` (`lazy=false`) or `redLazy` (`lazy=true`). |
| `mul(a,b)`, `add(a,b)`, `sub(a,b)`, `powMont(a,e)` | Canonical Montgomery operands `<m`; results remain canonical Montgomery residues. `add`/`sub` handle the carry and borrow past the word for `m > R/2`. |
| `pow(a,e)` | Ordinary word base and ordinary canonical result, with conversion included; preserves the old API. |
| `redLazy(x)`, `normalize(a)` | `m < R/2`; REDC retains `x < m * R` and returns `<2m`. Normalization accepts `<2m` and subtracts `m` if needed. |
| `mulLazy(a,b)`, `addLazy(a,b)`, `subLazy(a,b)` | `m < R/4`, operands `<2m`; results `<2m` can be chained without canonicalization. |
| `Guard` (`note`, `ok`) | Deferred operand check used by the bulk products and `get`: notes the running maximum of words or 32-bit lanes inside the kernel loop; one assertion after the loop reads `ok`. Dead code under `NDEBUG`. |

All bulk APIs use pointer/count overloads with an `int n >= 0`; pointers address `n` words, with no alignment requirement. Bulk operand ranges are asserted once after the loop through `Guard`; count and pointer preconditions are asserted at entry. Empty calls permit null pointers. Buffers must be disjoint or have exactly the same start; partial overlap is outside the contract. Products support output aliasing either/both inputs. Conversion and fixed-multiplier arrays support exact in-place operation. Modulus zero and other domain violations are asserted in checked builds and remain preconditions under `NDEBUG`.

For even moduli use Barrett directly. For an odd modulus with many chained products, convert once and use Montgomery, then convert the result back. For ordinary independent products or arbitrary wide dividends, Barrett avoids representation conversion. For a repeated multiplier use `multiplier(b)`. Modulus one returns zero throughout, including exponent zero. No primality assumption or inverse-of-an-arbitrary-residue operation belongs to these reducers.

## Arithmetic justification

**Barrett.** Set `B = R^2` and `mu = floor(B/m)`. Non-power-of-two moduli do not divide `B`, so the representable expression `floor((B-1)/m)` computes the same value. For `x < B`, `q = floor(x*mu/B)` is the exact quotient or one less: the approximation error is nonnegative and strictly below one. Thus `x-q*m < 2m`; one subtraction makes it canonical. Keep the extra residual bit until correction. Power-of-two moduli, including one, use masking and need no reciprocal. The 128-bit high product splits into four 64-bit products; summing the three middle limbs at 128-bit width preserves both carries. For `m=2^61-1`, use `2^61 = 1 (mod m)`: two folds of the entire 128-bit dividend leave a value at most `m+64 < 2m`, followed by one subtraction.

**Fixed multiplier.** With `b < m`, precompute `c = floor(b*R/m)`. For any word `a`, `floor(a*c/R)` underestimates `floor(a*b/m)` by at most one. The widened residual is `<2m`, so one subtraction suffices, including full-width moduli. This replaces the general double-width reciprocal product for repeated scalar factors.

**Montgomery.** For odd `m`, Newton lifting computes `inv = -m^-1 mod R`: each update doubles the number of correct low bits. With `q = low(x)*inv mod R`, `x+q*m` is divisible by `R`. The bound `x<m*R` gives `u=(x+q*m)/R<2m`. Full-width moduli require an extra carry beyond the double-width sum; the implementation retains it when deciding whether to subtract `m`. `R^2 mod m` permits conversion through one REDC. The inverse and carries intentionally use unsigned wraparound.

**Quotients (2026-10-07).** On the mask path `x >> countr_zero(m)` is the exact quotient (shift zero for `m=1`). On the reciprocal path `q` is the true quotient or one less, so `q*m <= x` and `q + [r >= m]` is exact. For `m = 2^61-1` write `x = (x>>61)*2^61 + (x & m) = (x>>61)*m + y` with `y = (x>>61) + (x & m) <= 2^67 + 2^61 - 2`, then `y = (y>>61)*m + z` with `z = (y>>61) + (y & m) <= m + 64 < 2m`; hence `x = ((x>>61) + (y>>61))*m + z` and the quotient is `(x>>61) + (y>>61) + [z >= m]`, the remainder `z - [z >= m]*m`.

**Sums (2026-10-07).** For canonical `a, b < m` the true sum is `< 2m`; it exceeds the word exactly when the wrapped `s = a + b` satisfies `s < a`, and then `s - m` (mod `R`) equals the true sum minus `m`, which lies in `[0, m)`; without wrap one conditional subtraction suffices. For `a < b`, `a - b + m` wraps back into `[0, m)`. For `m < R/4` and operands `< 2m`, `addLazy` forms `s < 4m < R` and `subLazy` forms `d = a - b + 2m` in `(0, 4m)`; one conditional subtraction of `2m` returns both to `[0, 2m)`.

**Guard (2026-10-07).** The running maximum of words is below `top` iff every word is. For 32-bit lanes with `t = top - 1 >= 0`, `max_epu32(acc, t) xor t` is zero in a lane iff that lane is `<= t`, so `testz` of the vector is true iff every noted lane was below `top`; `top` is `m >= 1` or `2m >= 2`.

**Lazy ranges.** Omitting the canonical subtraction requires `2m<R` for a word result. Multiplying arbitrary representatives `<2m` requires `4m^2<m*R`, hence `m<R/4`. These narrower lazy contracts do not reduce the full-width canonical domain. Compare lazy values only after normalization. These bounds also govern SIMD; signed vector comparisons require a separate proven nonnegative range or unsigned comparison emulation.

## Research scope and provenance

Inspected on 2026-09-27; sources inform independently derived algorithms and proofs. External reference timings and acceptance records are not evidence for this implementation.

| Source inspected | Relevant result and limits |
|---|---|
| Brent and Zimmermann, [Modern Computer Arithmetic, v0.5.9 (2010-10-07)](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf), §§2.4.1–2.4.4 and §2.5; [saved copy](../95-Resources/04-modern_computer_arithmetic.pdf) | Barrett approximation, REDC, inverse lifting, special moduli, Montgomery–Svoboda/FastREDC/McLaughlin. Its truncated normalized Barrett bound differs from the full-precision one-correction formula above. Reading copy CC BY-NC-ND; no text/code copied. |
| [AtCoder internal_math.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `internal::barrett` | Ceiling reciprocal and correction proof for canonical 32-bit products. CC0 1.0. Does not establish our arbitrary-dividend/full-64-bit domain. |
| [cp-algorithms: Montgomery Multiplication](https://cp-algorithms.com/algebra/montgomery_multiplication.html), June 8, 2022 revision | REDC representation, inverse doubling, wide carry handling, R² conversion; independently derived here. Page text CC BY-SA 4.0; no prose/code adaptation. |
| [Algorithmica: Montgomery Multiplication](https://en.algorithmica.org/hpc/number-theory/montgomery/) | Split-word reduction, lazy residues and optimization rationale. Its range assumptions and measurements cannot be transferred to full-word moduli. |
| [Nyaan SIMD Montgomery](https://raw.githubusercontent.com/NyaanNyaan/library/master/modint/simd-montgomery.hpp) | Even/odd 32-bit lane widening and packing; narrower lazy signed-comparison domain. CC0 1.0. |
| [NTL single-precision API](https://libntl.org/doc/ZZ.txt), `PrepMulModPrecon`/`MulModPrecon`; [selected sp_arith.h implementation](https://raw.githubusercontent.com/libntl/ntl/main/include/NTL/sp_arith.h) | Fixed-multiplier reciprocal and vector products. NTL's `NTL_SP_BOUND` restriction differs from this implementation's full-word domain; no source copied. |

Power-of-two masking, a dedicated `2^61-1` Mersenne reduction, exact reciprocal multiplication, fixed-multiplier precomputation, canonical/lazy REDC, scalar fallbacks and AVX2 32-bit bulk kernels cover the reusable machine-word backends. The tested `2^31-1` folding candidate lost to ordinary reciprocal reduction and was removed. Additional special-prime, floating-point quotient, IFMA/AVX-512, GPU and residue-number-system variants require separate domain proofs and measured workloads; their presence in research does not make an unconditional hardware-specific implementation appropriate. No floating-point exactness or constant-time cryptographic behavior is promised.

Multiword truncated/folded Barrett, Montgomery–Svoboda, subquadratic FastREDC and FFT-based McLaughlin multiplication require big-integer or polynomial multiplication infrastructure. They are research alternatives for C04/C11, outside the word-sized C02 contracts. Their implementation remains with those owners; P004 does not claim them verified.

## Legacy accounting

The unchanged original is [OLD/5-Mathematics/09-montgomery.hpp](../OLD/5-Mathematics/09-montgomery.hpp). Its public `Montgomery`, `mod/inv/rsq`, default constructor, `red/init/mul/pow` remain available. Previously the constructor required an odd modulus below `2^63`; canonical support now extends to every odd 64-bit modulus. The valid REDC dividend domain is explicit. The old code did not provide 32-bit, lazy, conversion-out, array, or SIMD interfaces. No legacy material is deleted and no dependent modular integer API is changed.

## Verification and performance evidence

The per-header Python entries resolve paths independently of the working directory, use only Python's standard library and GCC, report seeds/configurations and first failing inputs, retain their oracles under `NDEBUG`, and execute precondition failures in subprocesses. There is no external test dependency. No online submissions were made.

| File/features | Independent coverage |
|---|---|
| Barrett construction, reduction and multiplication | Native double-width `%`; all powers of two and neighbors, modulus one, primes/composites/even moduli, full unsigned maxima, Mersenne boundaries, dividend maxima, seeded random and bounded exhaustive small domains. |
| Barrett exact high-128 product | Independent bit-by-bit 256-bit shift/add multiplication; carry boundaries and random full-width pairs. |
| Barrett power, fixed multipliers and value semantics | Independent MSB-first modular exponentiation, zero/max exponents, `0^0`, arbitrary input words, all small fixed-factor pairs, copies/moves and detached multiplier lifetime. |
| Barrett bulk and preconditions | Lengths 0–65, 127/128/129, 255/256/257 and 4095/4096/4097, eight offsets, guard words, alias either/both inputs, fixed in-place products, null empty ranges; 16 assertion death cases. |
| Montgomery construction, REDC, conversion, multiplication and powers | Independent native-wide `%` oracle computes `R^-1` using repeated exact modular halving, independent of Newton/REDC. Full odd word range, power-of-two neighbors, modulus one, zero/max exponent, copies/moves/assignment and repeated calls. |
| Montgomery carries and lazy forms | Deterministic `m=R-1, a=b=R-2` regression forces a carry past the double word; `x=mR-1`; exhaustive canonical/lazy operands, output bounds and chained lazy products, both lazy modulus boundaries. |
| Montgomery arrays and preconditions | Empty/null ranges, vector threshold/tail sizes and eight offsets, guard words, all exact product aliases, conversion in/out and in-place, lazy array chains; 68 assertion death cases in each checked scalar/AVX2 configuration. |

Quick modes cover every public operation with smaller exhaustive/random corpora. Barrett full uses 60,000 random cases per width and exhaustive `m<=128, x<16384`; stress uses 600,000 and `m<=256, x<65536`, with exhaustive small fixed-factor pairs. Montgomery full covers odd moduli through 65 and 10,000 random cases per width; stress covers through 129 and 100,000 random cases per width. Full/stress execute optimized `-O3 -DNDEBUG`, checked, and ASan/UBSan scalar/AVX2 paths. Unsupported AVX2 hardware is reported as a skip. `LOCAL` does not affect either header.

Original completion validation on 2026-09-27, GCC 16.2.1 (20260810), Linux x86-64, Intel i9-11900H. Fresh maintenance checks are recorded separately below:

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode stress --seed 42
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode quick --seed 20260928
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode stress --seed 42
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --repetitions 5 --milliseconds 1 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
```

Montgomery passed 774,252 quick, 1,788,164 full and 8,889,119 stress checks **per configuration**, plus its checked preconditions. Barrett full passed 29,477,748 checks per configuration before the final measured simplifications; final stress passed **91,538,696 checks in each of five configurations**, plus 16 assertion cases and standalone/two-translation-unit execution. Montgomery full/stress passed all six configurations. Integration passed **59 standalone/aggregate headers**, scalar/AVX2 multiple-TU linkage and the Workspace snapshot. Sanitizer runs were repeated outside the sandbox with approved execution because LeakSanitizer cannot operate under its ptrace restriction; both scalar and AVX2 ASan/UBSan executions then passed. No sanitizer finding was suppressed.

### Performance method and selection

The [benchmark entry](<../96-Local Testing/01-Core/03-reduction_benchmark.py>) and [raw final measurements](<../96-Local Testing/01-Core/03-reduction_benchmark.jsonl>) record compiler commands, hardware, flags, seed 20260927, adaptive warmup to 1 ms and medians of five repetitions. Builds use GNU++20 `-O3 -DNDEBUG`; baseline disables AVX2/BMI2, accelerated uses `-mavx2 -mbmi2 -mno-avx512f`. Inputs are shared between implementations and every measured output is verified. Compiler memory barriers and volatile result sinks prevent elimination/hoisting. Kernels use precomputed contexts and O(n) input/output storage with O(1) scratch. Setup-and-power includes context construction and conversion for each of eight full-word bases/exponents.

The corpus covers modulus one, powers of two, typical contest primes, `2^31-1`, `2^61-1`, `2^64-59` and `2^64-1`; uniform and near-modulus inputs; bulk lengths 1, 7, 8, 9, 16, 32, 256 and 4096; arbitrary full-width dividends and fixed factors; canonical/lazy products, conversions and dependent chains. Unit-factor chains avoid products collapsing to zero, while the separate uniform distribution includes that legitimate case. Timings are observations on this host, not universal claims or timing gates.

The [rejected-candidate measurements](<../96-Local Testing/01-Core/03-reduction_candidates.jsonl>) preserve the initial general AVX2 Barrett32 kernel, manual 64-bit unrolling and Mersenne31 comparisons. The synthesized high-64 vector reciprocal product showed no consistent improvement (bulk/scalar about 1.07 at 4096 uniform elements), so ordinary Barrett32 uses a scalar loop except for power-of-two masks. Manual unrolling added overhead on short arrays without a consistent general benefit, so 64-bit bulk loops remain direct. Mersenne31 folding took about 286 ns versus 225 ns for a 256-product reciprocal loop in the same AVX2 run; it was removed. Mersenne61 folding is retained with its full-dividend proof and measured benefit. These candidate snapshots predate the final header and are labeled accordingly.

The final run passed all output checks and recorded 4,412 measurements. Representative accelerated-build medians (nanoseconds per complete workload):

| Workload | Reference | Selected backend |
|---|---|---|
| 256 ordinary products, `m=998244353` | Native `%`: 672 ns | Barrett: 249 ns |
| 256 encoded products, `m=998244353` | Scalar Montgomery: 282 ns | AVX2 Montgomery: 114 ns; lazy: 90 ns |
| 256 fixed-factor products, `m=998244353` | Scalar Shoup: 249 ns | AVX2 Shoup: 79 ns |
| 256 full-width reductions, `m=2^61-1` | Generic reciprocal: 725 ns | Two-fold reduction: 413 ns |
| 256 fixed-factor products, `m=2^64-59` | General Barrett: 822 ns | Shoup: 278 ns |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,740 ns | Barrett: 1,724 ns; Montgomery: 1,377 ns |
| Eight powers including setup/conversions, `m=2^64-59` | Native `%`: 2,783 ns | Montgomery: 1,396 ns |

At eight elements, median AVX2 bulk/scalar ratios across tested non-power-of-two 32-bit moduli were 0.40 for canonical Montgomery, 0.45 for lazy multiplication and 0.47 for Shoup. At nine elements they were 0.50, 0.56 and 0.55. Both conversion kernels also improved at eight elements. These measurements support the eight-element dispatch threshold and scalar tails. The power-of-two kernels use eight packed lanes. Ordinary 64-bit multiplication uses the compiler's native wide products; synthesizing large high products from AVX2's 32-bit multipliers has no established benefit here.

General 64-bit Barrett is **not universally faster than native remainder**: for 256 arbitrary wide dividends modulo `2^64-59`, it took 708 ns versus 574 ns for native `%`. Its value is a predictable exact precomputed backend over every modulus/dividend domain; use Montgomery for suitable long odd-modulus chains or Shoup for repeated factors, and ordinary `%` when setup/reuse does not justify a context. No non-Core consumer was changed, and these Core benchmarks do not waive the separate 20% end-to-end evidence rule for such changes.

## Completion and integration boundary

C02 is complete for the stated machine-word domains, with no unresolved owned implementation or verification gaps. C03 can consume these stable contexts without changing representation contracts: Barrett accepts ordinary words; Montgomery multiplication accepts encoded residues, and lazy chaining has the stricter bound above. The general research variants assigned to C04/C11 remain visible in the scope section; no work in those packages is marked complete by P004. The original legacy header remains unchanged in OLD.

## Maintenance review — 2026-09-27

Reviewed the current integer, namespace/cohesive-header and compressed-style rules, the C02 batch/inventory and the applied Core migration. Barrett and Montgomery remain separate reusable headers. Public names, member order, construction order, modulus/dividend/operand domains, lazy bounds, scalar paths and eight-element AVX2 dispatch are preserved.

The headers now separate state/construction, scalar operations and array operations consistently. Barrett's multiline endings follow the compressed brace convention; assertions and loop setup are separate logical steps. Montgomery's SIMD pointer conversions use `reinterpret_cast`, and `int(rsq)` makes the intrinsic's existing 32-bit lane conversion explicit without changing its bit pattern. The benchmark source receives matching layout and explicit named-lambda return types; its workloads, inputs and timing logic are unchanged.

Both C++ testers use `int` for their bounded check counters and retain `ulng` seeds. The largest supported stress corpora remain below `INT_MAX` (under 100 million Barrett checks and 32 million Montgomery checks, including seed-dependent boundary lists). Array indices/counts already use `int`, and the few size conversions address fixed bounded test/benchmark buffers. Moduli, residues, reciprocals, exponents, wide intermediates, masks, random seeds and benchmark XOR checksums remain unsigned: their full-word and wraparound semantics require it. Local naming/grouping was aligned without changing cases, oracle operations or diagnostic strings.

No namespace migration is appropriate here. `MontgomeryBackend<T>` is an existing public class template; `Montgomery32`, `Montgomery64` and `Montgomery` are its aliases. Moving that template behind a detail alias would change template identity and possible specialization/CTAD behavior. Its SIMD helpers remain methods, as do `Barrett64::highProduct` and each nested fixed `Multiplier`. There are no loose shared family helpers or alias-only wrapper headers to consolidate. The existing dependency on `01-template.hpp` and the applied modular-family layout are preserved.

Fresh verification used GCC 16.2.1 (20260810) and CPython 3.14.7 on Linux x86-64:

| Check | Fresh result |
|---|---|
| Barrett full, seed 20260927 | 29,477,748 checks in each of five configurations: optimized NDEBUG scalar/AVX2, checked scalar, ASan/UBSan scalar/AVX2; 16 assertion cases; standalone/multiple-TU executable passed |
| Montgomery full, seed 20260927 | 1,788,164 checks in each of six configurations: optimized NDEBUG, checked and ASan/UBSan scalar/AVX2; 68 assertion cases in each checked configuration |
| Benchmark source | Scalar and AVX2/BMI2 GNU++20 optimized syntax checks passed; no new timing run |
| Core integration | 59 standalone/aggregate headers, scalar/AVX2 multiple-TU linkage and Workspace checks passed |
| Repository consistency | Zero errors across current ownership, paths, Markdown links and preserved archives |
| Historical measurements | Both JSONL files match their pre-maintenance SHA256 hashes below |

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20260927
g++ -std=gnu++20 -O3 -DNDEBUG -Wall -Wextra -mno-avx2 -mno-bmi2 -fsyntax-only '96-Local Testing/01-Core/03-reduction_benchmark.cpp'
g++ -std=gnu++20 -O3 -DNDEBUG -Wall -Wextra -mavx2 -mbmi2 -mno-avx512f -fsyntax-only '96-Local Testing/01-Core/03-reduction_benchmark.cpp'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Full suites used already-approved execution outside the sandbox to retain LeakSanitizer despite the previously established ptrace limitation. Leak checking was not disabled. Historical stress runs and performance medians above remain historical evidence; this maintenance introduces no new performance claim and does not relabel old measurements as measurements of the edited source.

| Historical artifact, unchanged at the 2026-09-27 maintenance (the benchmark ledger was re-recorded on 2026-10-07; see the re-audit) | SHA256 |
|---|---|
| `96-Local Testing/01-Core/03-reduction_benchmark.jsonl` | `39fef7f0b713d4971258ad24fd2598d1961a4e0065d0e26ad743348b8ef212c1` |
| `96-Local Testing/01-Core/03-reduction_candidates.jsonl` | `6e522c4051383062a6a871e10e375ab6e9632f72d08aea1b910b276688d07959` |

| Maintained source at the 2026-09-27 maintenance (superseded by the re-audit hashes below) | SHA256 |
|---|---|
| `01-Core/03-barrett.hpp` | `3368c40cbb74242616a964dee0a1f8a3841838f07cbf5b34627ced26e9bcee85` |
| `01-Core/04-montgomery.hpp` | `52bc741783aa6309f6ed16ec7700c048e20dbd46c5148ea03fd8bc2b91f1d075` |
| `96-Local Testing/01-Core/03-barrett_tester.cpp` | `17697706970c33c5b7fba98c30395f33ef0ecd8536257d406179bea0b38dc8d6` |
| `96-Local Testing/01-Core/04-montgomery_tester.cpp` | `3cb237f3555df3cc5e61eb259793059677bf9b4b27480b585bf32033db7269a4` |
| `96-Local Testing/01-Core/03-reduction_benchmark.cpp` | `9ccd866740eb2e4f80befd4d77a78035e4f086ef80a637076711b7e27bf42437` |

No owned maintenance gaps remain. P004 stays complete; no package ownership, scheduling or legacy source was changed.

## Re-audit — 2026-10-07

Package P004 re-audit under the current rules; the previous verification was treated as existing-unverified. Every operation in both inventory rows was compared with the code and the testers before any edit (the only row gap was `Barrett64::highProduct`), the unchanged full suites were rerun (Barrett 5 configurations in 28.91 s, Montgomery 6 configurations in 36.11 s, seed 20260927, both passing), and the headers were then brought to the current style, extended from the catalog sweep and re-tested.

### Confirmed findings and their disposition

| # | Finding | Disposition |
|---|---|---|
| 1 | `Barrett64::highProduct` missing from the inventory row | Fixed: the row names it, the new `MERSENNE61` constant, the `Multiplier` fields and the AVX2 `load`/`store` helpers; the brief reads the row. |
| 2 | Bulk fixed-multiplier tests used only `multiplier(MAX)`, which is 0 or 1 for most batch moduli | Fixed: `batches()` builds multipliers for `MAX`, `m-1` and a seeded random factor for every modulus, checks the separate and in-place bulk products against `Wide(a)*f % m` with leading/trailing guards, and the Barrett32 modulus list gains `2^32-5` and `3*2^30+1`, so the AVX2 Shoup kernel now runs with residuals near `2^33` through its signed correction. |
| 3 | Complexity comment used the undefined symbol `e` | Fixed in both headers (`powers O(log(e+1)) for exponent e`; Barrett's four comments likewise). |
| 4 | Per-element assertions inside the bulk SIMD and scalar loops | Fixed: the loops call the unchecked core `redc`, and operand ranges are checked by `Guard`, a running maximum of words (`cmp`/`cmov`) or 32-bit lanes (one `vpmaxud`) that one assertion reads after the loop. Under `-DNDEBUG` the generated code contains none of the Guard instructions (checked on the `-O2 -mavx2` assembly of both widths). The benchmark gained `-O2` assertion-enabled configurations (the Codeforces build); results below. |

### Changes beyond the findings

- Catalog sweep ([81-sources.md](81-sources.md), left-out items in [80-notes.md](80-notes.md)): Barrett `divMod`/`div` (maspypy `floor`/`divmod`, Nyaan and suisen `quo`/`quorem`, Lemire `fastdiv`) and a default modulus of one (maspypy, suisen); Montgomery `add`/`sub` on canonical residues (suisen, cp-algorithms) and `addLazy`/`subLazy` on `[0, 2m)` (Nyaan vectorize-modint/simd-montgomery). `divMod` is now the Barrett core: the quotient comes from the same folds on every path (`x >> countr_zero(m)` for powers of two, `(x>>61) + (y>>61) + [z >= m]` for `2^61-1`, `q + [r >= m]` for the reciprocal), and `reduce` reads its remainder, so the remainder path is unchanged after inlining (benchmark unchanged within noise).
- Montgomery: `redc(x, lazy)` replaces the duplicated `red`/`redLazy` bodies; `red8` factors the even/odd lane packing shared by `mul8` and the bulk `get`; the constructor parameter is `m` (vocabulary); `rsq` narrowing is explicit (`-Wconversion` clean, previously untested for this header).
- Both headers, both testers and the benchmark follow the closing-brace rule (`03-consistency.py --braces` reports nothing); the headers compile with `-Wall -Wextra -Wshadow -Wconversion -Werror` on scalar and AVX2 builds.
- Testers: Barrett adds a checked AVX2 build (precondition deaths run in both checked builds), field checks for `mod`/`mu`/`mask` and the `Multiplier` fields against an independent bitwise restoring division, `divMod`/`div` against native `/` and `%` on every boundary, random and exhaustive corpus, and the default constructor. Montgomery adds `-Wconversion -Werror`, a standalone/multiple-TU build in full and stress, direct `redc`/`Guard` coverage, exhaustive `add`/`sub`/`addLazy`/`subLazy`, full-width carry/borrow regressions for `add`/`sub` at `m = R-1`, and 16 new precondition deaths (50 per width: canonical and lazy sums/differences, a bad lane at position 0 of 16 for the vector path, bad tail positions for every bulk operation).

### Feature-to-test additions

| Feature | Independent coverage |
|---|---|
| `divMod`, `div` (both widths) | Native `/` and `%` on the boundary dividend list (0, 1, m-1, m, m+1, 2m-1, MAX, MAX+1, MAX², TOP, TOP-1, m·MAX, m·MAX-1) for ~250 moduli per width including every power of two and its neighbours, `2^61-1`, `2^64-1`; 60,000 (full) / 600,000 (stress) random full-width dividends per width; exhaustive `m <= 128, x < 16384` (full) / `m <= 256, x < 65536` (stress); default constructor. |
| `mod`, `mu`, `mask`, `Multiplier::{mod, value, mu, mask}`, `MERSENNE61` | Bitwise restoring division `floor((2^(2w)-1)/m)` and `floor((b % m)·2^w / m)`; zero on the mask and Mersenne paths; constant equals `2305843009213693951`. |
| Bulk fixed products (finding 2) | Factors `MAX`, `m-1`, random for 16 (32-bit) / 17 (64-bit) batch moduli, sizes 0–65, 127–129, 255–257, 4095–4097, eight offsets, separate and in-place, guards. |
| `redc` | Canonical and lazy core against the oracle on every boundary dividend of every boundary modulus; every other operation routes through it. |
| `add`, `sub` | Oracle `(a+b) % m`, `(a-b) mod m` on all boundary pairs and random pairs per width, decoded through `get`; exhaustive odd `m <= 65/129`; carry past the word (`m = R-1`, `a = R-2, b = R-3`) and borrow (`0 - (R-2)`). |
| `addLazy`, `subLazy` | Output bound `< 2m` and normalized value against the oracle on boundary pairs shifted by `m`, exhaustive `a, b < 2m` for odd `m <= 65/129`, chained `subLazy(addLazy(..), subLazy(..))`. |
| `Guard` | Direct: all-canonical and all-lazy arrays accepted; every single position raised to `m` or to the word maximum rejected, for `n` in {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 65, 257}, with 8-lane notes under AVX2 for Montgomery32; indirect: 32 bulk precondition deaths per width across vector, tail and `n = 8` positions. |
| `red8`, `load`, `store` | Through every bulk operation in the optimized and checked AVX2 builds (sizes/offsets/aliases above). |

### Commands and results

```bash
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/01-Core/03-barrett_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/04-montgomery_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/03-reduction_benchmark.py' --seed 20261007 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/03-reduction_benchmark.jsonl'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261007): Barrett **53,657,153 non-removable checks in each of six configurations** (optimized and checked scalar, optimized and checked AVX2, scalar and AVX2 ASan/UBSan with leak detection; 16 precondition deaths in both checked builds; standalone/multiple-TU build) in 41.72 s; Montgomery **3,699,028 checks in each of six configurations** (same set; 100 precondition deaths in each checked build; standalone/multiple-TU build) in 47.28 s. Stress (seed 1, one round): Barrett **146,158,229 checks in each of six configurations** in 61.53 s; Montgomery **23,203,772 checks in each of six configurations** in 50.75 s, with the same deaths and multiple-TU builds. `python3 '96-Local Testing/02-integration.py' --sanitizers` passed: 102 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, the Workspace snapshot and the sanitizer self-tests. `python3 '96-Local Testing/03-consistency.py'` reports no errors (eight brace-checked files).

### Benchmark — 2026-10-07

The [ledger](<../96-Local Testing/01-Core/03-reduction_benchmark.jsonl>) now holds this run (8,912 verified measurements; SHA256 in the table below; the 2026-09-27 figures above remain the historical record). Same host (Intel i9-11900H), GCC 16.2.1, GNU++20, seed 20261007, five repetitions, 2 ms adaptive warmup, four configurations: `scalar`/`avx2` with `-O3 -DNDEBUG` as before, and `scalar-asserts`/`avx2-asserts` with `-O2` and assertions enabled (Codeforces). Nanoseconds per complete workload, medians.

| Workload (`-O3 -DNDEBUG`, AVX2 build) | Reference | Selected backend |
|---|---|---|
| 256 ordinary products, `m=998244353` | Native `%`: 584 | Barrett: 247 |
| 256 encoded products, `m=998244353` | Scalar Montgomery: 281 | AVX2 Montgomery: 111; lazy: 92 |
| 256 fixed-factor products, `m=998244353` | Scalar Shoup: 246 | AVX2 Shoup: 75 |
| 256 full-width reductions, `m=2^61-1` | Generic reciprocal: 770; native: 1,214 | Two-fold reduction: 417 |
| 256 fixed-factor products, `m=2^64-59` | General Barrett: 871 | Shoup: 291 |
| 256 quotient+remainder, 64-bit dividends, `m=998244353` | Native `/` and `%`: 608 | `divMod`: 269 |
| 256 quotient+remainder, 128-bit dividends, `m=2^61-1` | Native: 1,311 | `divMod`: 684 |
| 256 quotient+remainder, 128-bit dividends, `m=2^63` | Native: 968 | `divMod`: 261 |
| Eight powers including setup/conversions, `m=998244353` | Native `%`: 2,721 | Barrett: 1,572; Montgomery: 1,374 |
| Eight powers including setup/conversions, `m=2^64-59` | Native `%`: 3,388 | Montgomery: 1,420 |

Finding 4 re-measured, `Montgomery32`, `m=998244353`, 256 products: AVX2 bulk 110.5 ns (`-O3 -DNDEBUG`) versus 112.3 ns with assertions (`-O2`), so the deferred Guard costs about 2% where the per-element assertions cost 49%; a scalar loop of checked `mul` calls in the same assertion build takes 446 ns. Scalar builds with assertions: bulk 451.6 ns against 432.0 ns for the per-element-assert loop (+4.5%); `Montgomery64`, `m=2^64-59`: 443.5 against 421.7 (+5%). An earlier candidate, a separate O(n) entry pass, cost +27% (AVX2) and +91% (scalar) in the assertion builds and was replaced by the in-loop maximum; a branch-free `bad |= x >= top` accumulator cost +10–20% in scalar builds against the maximum's +5%.

Threshold neighbours (median AVX2 bulk/scalar ratios over `998244353`, `2^31-1`, `2^32-5`): at 7 elements 1.24 (canonical), 1.28 (lazy), 1.30 (Shoup); at 8 elements 0.53, 0.57, 0.47; at 9 elements 0.62, 0.66, 0.55; at 256 elements 0.39, 0.37, 0.31. With assertions enabled the ratios at 8 elements are 0.57, 0.48, 0.53. Both conversion kernels improve from 8 elements in both configurations (0.39/0.51 and 0.52/0.74). The eight-element dispatch stands. General 64-bit Barrett remains slower than native remainder for full-width moduli (`m=2^64-59`: 1,120 vs 860 ns for 256 reductions; `divMod` 1,107 vs 717), as documented above; `divMod` wins for every other measured modulus.

Source hashes after the re-audit (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/03-barrett.hpp` | `ce620ceca3aa0ed76625c82dc2a60f4a9edb9b744e82b69951dad8e0474f9650` |
| `01-Core/04-montgomery.hpp` | `6f5c204baf5d256483aabb0320a4a289769f4d841fc334ba66c20bc8a0acf62f` |
| `96-Local Testing/01-Core/03-barrett_tester.cpp` | `7dca45c36bcdb3bf2c604c68be95681b926494fc1c226571fb5d78f6b1f82314` |
| `96-Local Testing/01-Core/03-barrett_tester.py` | `9290f5fdfc4f6306c0026ef913be5ca91dab871808dcccaca2545a1be78f3b44` |
| `96-Local Testing/01-Core/04-montgomery_tester.cpp` | `a7e4a8c1e6b868af61a1837dcdcb097c33ca8178ca6542903e54998b87de39f1` |
| `96-Local Testing/01-Core/04-montgomery_tester.py` | `1aa0b46eaae9e7e906d9337d2e356e63520e65f7fe899ad0857fdada6c95e56d` |
| `96-Local Testing/01-Core/03-reduction_benchmark.cpp` | `ff478e83a113dabd82ef198a31cd31e2949f38f97aabd0242a5b197cc48b06f7` |
| `96-Local Testing/01-Core/03-reduction_benchmark.py` | `f976b06c13d0ac0989fb83e9a1e851b656dc7b6ee6d62e95c6b0fb7b8e70d9e5` |
| `96-Local Testing/01-Core/03-reduction_benchmark.jsonl` | `dfbccd03d8d90ebd3eefc271e53e9828d58b1a2ef883689206985c627765f9ad` |
| `96-Local Testing/01-Core/03-reduction_candidates.jsonl` (unchanged) | `6e522c4051383062a6a871e10e375ab6e9632f72d08aea1b910b276688d07959` |

Independent review (`@reviewer`, 2026-10-07) found no correctness defect: it re-derived every argument above, ran its own 8.13-million-case differential against native `%`, `/` and `ulll` arithmetic under ASan/UBSan on scalar and AVX2 builds (quotients at maximal values near `k*m +- 1`, `2^61-1` and neighbours, `2^63 +- 1`, `2^64-59`; bulk Shoup with factors `m-1` and `MAX` at `2^32-1`, `2^32-5`, `3*2^30+1`; sums, differences, lazy forms at `2m-1`/`2m-2`; every 8-lane Guard verdict for seven `top` values), and 15 header mutants (dropped quotient corrections, single Mersenne fold, off-by-one Shoup correction, missing `add` wrap, wrong `sub` borrow, unreduced lazy forms, Guard bound/sign/term deletions) that the testers all caught in `-DNDEBUG` builds or by death tests. It confirmed the four findings fixed, instruction counts unchanged for the remainder paths (`Barrett32::reduce` 16/16, `Barrett64::reduce` 104/104 at `-O2`), and raised three low findings, all fixed: `Wide`, `BITS` and the Guard fields added to the row; a complexity line on `Guard`; the quotient, sum and Guard arguments added to the justification section. Its suggestions were also applied: `Guard::ok` evaluates the lane test only for `BITS == 32` (no narrowing of a 64-bit `top`, no spare `vptest`), the header comment names `red4`/`red8` for the AVX2 loops, and the Montgomery Python entry no longer uses a backslash inside an f-string replacement field (a SyntaxError before Python 3.12; the floor is 3.10).

Known limits and handoff: GCC 14 and Windows were not executed; the AVX2 kernels are 32-bit only (no 64x64 vector multiply in AVX2); the Barrett `Multiplier` has no quotient; signed-dividend reduction and residue inverses stay with `05-modint.hpp`, whose `powerMontgomery` repeats `powMont` for 128-bit exponents (a P005 decision whether to widen the backend exponent). No P004-owned gap remains.
