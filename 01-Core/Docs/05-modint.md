# 05-modint.hpp — evidence

P005/C03 owns `05-modint.hpp`, which declares all four full public types alongside their shared `modint_detail` implementation. Family-specific helpers and their users stay together, preserving identical full-width corner cases and failure semantics. P005/C16 separately owns the four independently copyable structs in `06-modintmini.hpp`; see [06-modintmini.md](06-modintmini.md). C01 and C02 are the verified prerequisites.

## Contracts

### ModInt, ModInt64, DynModInt, DynModInt64, ModInt61

| Type | Modulus | State and representation |
|---|---|---|
| `ModInt<MOD>` | Compile-time `uint`, `1 <= MOD < 2^32` | One canonical `uint n`; `mint` remains `ModInt<998244353>` |
| `ModInt64<MOD>` | Compile-time `ulng`, `1 <= MOD < 2^64` | One canonical `ulng n` |
| `DynModInt<ID>` | Runtime `uint`, same full range | One canonical `uint n`, shared context per ID |
| `DynModInt64<ID>` | Runtime `ulng`, same full range | One canonical `ulng n`, shared context per ID and independent of 32-bit IDs |
| `ModInt61` | Alias of `ModInt64<2^61-1>` | Same type, API and storage; products fold instead of dividing (see Context below) |

Static zero-modulus instantiations are rejected. Dynamic types default to modulus 998244353; `setMod(m, prm=-1)` requires a nonzero representable modulus. `prm=-1` detects primality, `prm=0` disables the prime flag even if the modulus is prime, and `prm=1` promises an actual prime (checked by assertions). `isPrime()` computes the actual primality; `is_prime` reports the enabled flag, a compile-time constant for static types. Primality uses deterministic Miller–Rabin with the seven witnesses valid below 2^64.

### Dynamic lifetime (setMod)

Every `setMod` invalidates all previous values, references to their meaning, and caller caches, even for an identical modulus. Discard, copy without arithmetic, or overwrite stale storage; copies remain stale. Never read a stale residue as a live value or perform operations on it. Reconstruct input and caches after setup. No generation tags are stored: this preserves one-word, trivially copyable values and makes freshness a caller precondition. Distinct IDs and widths do not invalidate each other. A function-local reducer initializes before its first use, including global initializers and setup in another translation unit; this avoids unordered initialization of templated static objects. There are no owned allocations or growing caches in the context. Single-threaded semantics apply; concurrent mutation is outside the contract. Shared context fields are inspection-only; use `setMod` to change them.

### Value operations

| Feature | Contract |
|---|---|
| Construction and assignment | All built-in integral types, including signed/unsigned 128-bit and signed minima, normalize modulo `mod()`; default is zero; `a = x` converts through the constructor. Native widths reduce their magnitude in a 32- or 64-bit word (Barrett for dynamic 32-bit moduli); only 128-bit inputs take a 128-bit remainder. Floating-point construction is excluded. |
| `val()`, `n`, `mod()` | Canonical unsigned representative and modulus. Public `n` may only be changed to a canonical residue. |
| `raw(x)` | Requires an already canonical word `x < mod()` (checked assertion); does not normalize. |
| Integral conversion | Explicit conversion to a representable integer type; narrowed values require a fit. Use `val()` or the word-width unsigned cast for full-width results. `bool` means nonzero, so `!a` tests for zero. |
| Arithmetic | Unary/binary/compound `+ - * /`, prefix/postfix `++ --`, assignment and copy/move. Mixed integral operands normalize via the constructor. Division requires a unit denominator. All full-width addition/subtraction/products avoid signed overflow. |
| Comparison | Equality and all relational operations compare canonical representatives, not an order compatible with ring arithmetic. |
| `tryInv(a, out)` | True iff `gcd(a.n, mod()) == 1`. On success writes the canonical inverse; on failure leaves `out` unchanged. Input/output aliasing is allowed. |
| `inv(a)` and division | Unit precondition; assertions detect violations. Composite moduli are supported without assuming Fermat's theorem. |
| `pow(a, e)` | Every native signed/unsigned exponent width through 128 bits. Negative exponents require a unit, including the minimum signed exponent. Zero exponent returns the multiplicative identity. |
| `trySqrt(a, out)` | Requires the prime flag. True iff a square root exists, writes the smaller canonical root. False leaves `out` unchanged. Alias allowed. |
| `sqrt(a)` | Compatibility API: smaller canonical root, or the residue of `-1` for no root. This sentinel cannot equal a valid smaller root: for odd primes a smaller root is at most `(p-1)/2`, while for p=2 both inputs have roots. |
| `batchInv(a, out)` | Input vector length at most `INT_MAX`; succeeds iff every element is a unit. One inverse plus linear many multiplications. Empty input succeeds with empty output; failure preserves output. Exact input/output vector aliasing is supported. |
| Streams | Output canonical unsigned decimal under default decimal formatting. Input consumes one whitespace-delimited token of optional sign and one or more ASCII decimal digits, reducing arbitrary length. Invalid tokens set `failbit` and preserve the destination. Standard stream exception masks still apply. |

Modulus one is the zero ring: its single element zero is also the multiplicative identity and a unit (`gcd(0,1)=1`). Its inverse and every power, including zero and negative exponents, are zero. Prime-root APIs are unavailable at modulus one. Failure results are not conflated with this valid zero.

Integer division, remainder, bitwise operations, and shifts on residues are deliberately not invented. Composite-modulus square roots, all-root parameterizations, primitive roots, discrete logarithms, CRT and finite-field extensions belong to their Mathematics/finite-field batches; a residue value type does not claim those algorithms. No random root search or probabilistic primality result is exposed. Explicit runtime context objects are unnecessary for these ID-based single-threaded types; arbitrary numbers of simultaneous moduli can use distinct IDs at compile time.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `init(x)` | `raw(x)` |
| template `operator=(T)` | the converting constructor plus copy assignment: `a = x` |
| `operator!` | `!a` through `explicit operator bool` |
| `modint_detail::MERSENNE61` | `Barrett64::MERSENNE61` |
| `modint_detail::fold61(x)` (fold plus conditional subtraction) | `Context::canon(Barrett64::fold61(x))`, reachable as `ModInt61::canon` |
| `modint_detail::powerMontgomery` REDC loop | `Montgomery64(m).pow(a, e)` (128-bit exponent); the name stays as the `noinline` one-liner `pow` calls |

### ModularInt, StaticModularInt

Public concepts for generic Poly/Matrix dispatch: `ModularInt<T>` accepts any full or mini residue type (`Word`, `mod()`, `val()`, `n`); `StaticModularInt<T>` additionally requires a constant `mod()`.

### Context (mul, red, canon, reduction), norm, powerMontgomery

`Value::operator*=` calls `Context::mul`, the canonical-factor product hook; `Context::red` keeps accepting any double-width dividend. Static types use compiler-optimized double-width `%` for canonical multiplication, except the Mersenne modulus `2^61-1` (`Barrett64::MERSENNE61`, exposed as `ModInt61`): its product is one 64×64 multiply, one `Barrett64::fold61` and the conditional subtraction `Context::canon` (a static-context member, so `ModInt61::canon` and `mint::canon` exist and the dynamic types have none: the residue of a value below `2 * MOD`, which one fold of every canonical product leaves), and `red` takes an arbitrary 128-bit dividend with two folds followed by `canon`. Dynamic 32-bit values use Barrett32; dynamic 64-bit values use native double-width `%` for general moduli, retaining Barrett64's mask and two-fold Mersenne61 paths for `red`, while `mul` delegates to the static Mersenne context's single-fold product when the runtime modulus is `2^61-1`. A generic full-width Barrett product can improve a dependent chain but costs more than native remainder on the measured independent-product workloads; the default favors ordinary canonical arithmetic. Canonical `n` storage also avoids a representation change for every access/order/stream operation.

`norm`: `lll`/`ulll` keep the 128-bit remainder; every other integral type takes its magnitude in a 32- or 64-bit word (`U = uint` for inputs of at most four bytes), reduces it with `Context::red` for `uint` words (constant-modulus multiply/shift when static, Barrett32 when dynamic) or `u % mod()` for `ulng` words, and negates the residue when the input was negative. Signed minima are safe because `U(0) - U(a)` wraps to the exact magnitude. No native-width construction emits `__modti3` (`g++ -O2 -S`). `pow` starts from `raw(mod() != 1)`.

Fold correctness: for canonical `a, b < M = 2^61-1`, `x = a * b < M^2`, so `x >> 61 <= M - 1` and `fold61(x) = (x & M) + (x >> 61) <= 2M - 1`; `canon` subtracts `M` at most once for the canonical residue, and `x ≡ fold61(x) (mod M)` because `2^61 ≡ 1`. For an arbitrary 128-bit dividend the first fold leaves `y < 2^67 + 2^61` and the second `(y & M) + (y >> 61) <= M + 64 < 2M`, again `canon`'s domain; both bounds are stated and tested for `Barrett64::fold61` in [03-barrett.md](03-barrett.md).

At runtime, odd 64-bit powers use Montgomery with per-call setup when the exponent magnitude is at least 512; Mersenne61 never uses Montgomery, since its fold product is cheaper than a REDC at every exponent length (re-measured 2026-10-12 below). Even moduli and shorter powers use canonical multiplication. Constant evaluation always uses the canonical path. The runtime helper `modint_detail::powerMontgomery(a, m, e)` is a GCC `noinline`/`noipa` one-line call to `Montgomery64(m).pow(a, e)` (128-bit exponent since the P004 prune): the function boundary keeps one generic REDC loop for every modulus, because measured constant-modulus specialization of this full-width loop regressed sparse static powers; it changes neither representation nor constexpr behavior. The threshold includes setup/conversions and was checked on both sides with sparse and dense exponents, not inferred solely from a multiplication microbenchmark.

The scalar value interface and batch inversion's dependent prefix chain offer no direct independent vector lanes. Canonical-to-Montgomery conversion costs were measured, including C02's AVX2 bulk routines; no additional type-level SIMD API is needed to expose the specified operations. C02 retains its explicit bulk/reusable-domain APIs for workloads that can stay in that representation. These are Core measurements, not evidence for the outside-Core 20% adoption rule or a universal fastest-modular-arithmetic claim.

### Correctness and cost

For canonical `a,b`, addition compares `a` with `m-b` before adding; the selected sum is below `m`, and subtraction uses `m-(b-a)` only when `b>a`. Neither operation requires a wider carry. Products use exact double-width unsigned intermediates and the selected reduction backend. Signed normalization first converts the positive modulus to signed 128 bits, so negative extrema can take a remainder without negation or signed/unsigned usual-conversion surprises.

Euclid maintains remainder/coefficient pairs with `r = a*x (mod m)`. Success ends at gcd one. Standard Euclidean coefficient bounds keep the coefficients and their next recurrence values bounded by the input modulus; signed 128-bit intermediates safely cover the full unsigned 64-bit modulus domain. The final inverse coefficient lies strictly between `-m` and `m`, so one conditional addition normalizes it. The failure path never writes output.

Negative exponents obtain their magnitude through unsigned 128-bit negation, which remains defined for a signed minimum. Binary exponentiation maintains the accumulated product and successive squared base. Static operations, including inversion and roots, remain usable in constant expressions.

Square roots use the direct exponent followed by a square check when `p % 4 == 3`; other odd primes use a Legendre test and deterministic Tonelli–Shanks. The loop maintains `x^2 = a*t`, with `t` in a decreasing-order power-of-two subgroup. Searching the first nonresidue from two terminates for each odd prime. With `z` the first nonresidue, its stated worst-case arithmetic cost is `O((z + log(p)) * log(p))`, constant auxiliary memory; no unjustified expected-time claim is made. The returned minimum accounts for the two roots.

A product in the finite commutative ring `Z/mZ` is a unit iff each factor is a unit. Batch inversion stores prefix products, checks the inverse of the total, and walks backward to obtain each inverse. The original vector stays intact until the final move, including when input and output alias. Cost is `O(k + log(m))` time and `O(k)` temporary/output memory. Scalar arithmetic/construction costs constant bounded-word work; inverse and primality cost `O(log(m))` arithmetic steps, powers `O(1 + log(|e| + 1))` plus `O(log(m))` for a negative exponent, and stream parsing `O(d)` time and token memory for d characters.

## Feature-to-test map

The single mirrored Python entry, `05-modint_tester.py`, runs all four type suites through `_05_modint_test_runner.py` and `05-modint_test_support.hpp`. The four `05-*_cases.cpp` fixtures each include the actual consolidated `05-modint.hpp`. Optional `--type modint32`, `--type modint64`, `--type dynmodint32`, or `--type dynmodint64` restrict a diagnostic rerun; the default `--type all` retains every suite. Assertions are never the test oracle, so checks remain live under `-DNDEBUG`. Test-only Python arbitrary integers and built-in modular `pow` provide independent exact answers, including for signed/unsigned 128-bit normalization and exponents.

| Feature → all four full types | Verification |
|---|---|
| Constructors, assignment, raw, access, casts, type/layout properties | All native widths and signed minima, uint/ulng/128 extrema, assignment from every integral type through the constructor; canonical raw endpoints (0 and `mod() - 1`, constexpr `raw(6)`) and the invalid raw subprocess; one-word standard-layout/trivially-copyable assertions; narrowed-cast death case |
| Arithmetic, unary, mixed integer operands, comparison, bool and `!`, increments and aliasing | Exhaustive small rings, exact double-width boundary oracle, seeded random pairs, Python arbitrary-integer differential results; compound/self aliases and copy/move |
| Inverse, division, negative powers, zero ring | gcd unit classification, exact Python inverse, failures preserving output, input/output aliases, composite zero divisors, modulus-one identity, precondition subprocesses |
| Powers and selected backends (`powerMontgomery`, so `Montgomery64::pow`, at `e >= 512` on odd non-Mersenne 64-bit moduli) | Zero/one, sparse/dense signed/unsigned 128-bit extrema, 511/512/513 and 65535/65536/65537 neighbors, even/power-of-two/Mersenne61 and full-word odd moduli against Python `pow`; constexpr powers through the same thresholds |
| Prime roots | Exhaustive enumeration of least roots in small prime fields, independently formed random squares in large primes including Goldilocks' large power-of-two subgroup, Python nonresidues, preserved outputs/sentinel, aliases and nonprime/disabled-flag deaths |
| Batch inversion | Empty, singleton, varied lengths, arbitrary units and composites, product/inverse identities, nonunits at varied positions, nonzero zero divisors, exact vector aliasing and unchanged failure output |
| Streams | Whitespace, signs, signed minima, thousands of digits, invalid whole tokens, EOF and output; independent exact decimal reductions |
| Static primality and constexpr | Prime/composite and strong-pseudoprime regressions through full width; compile-time arithmetic/inverse/root/power; zero modulus compile rejection |
| Dynamic IDs, reset, primality and reduction | Independent contexts and widths, repeated/same-modulus setup, prime hints, reconstructing overwritten old storage, brute-force small primality oracle, full double-width dividends, global default/custom setup and two-TU startup in both link orders |
| Header and consumer integration | All standalone headers, Basic/All and two-TU scalar/AVX2 builds; ordinary `mint` Matrix product/determinant and ModFac counting smoke; this does not verify those dependent families |
| `norm` (native widths and 128-bit) | `construction<M>` compares `M::norm` with an independent 128-bit signed/unsigned formula for every integral type and extreme; the Python `N` oracle builds through the narrowest native width (`int`/`lng`/`lll`, `uint`/`ulng`/`ulll`) at `±2^31 ± 1`, `2^32 ± 1`, `±2^63`, `2^64` and random 128-bit values |
| `Context::mul`, `red`, `reduction` | `pairCase` checks `M::mul` against the exact double-width product on every exhaustive, boundary and random pair; `verifyMod` checks `red` on six dividends including `2^128-1`; `contexts` checks `reduction().mod`, a double-width `reduce`, `mul` and `red` at `(m-1)^2` after `setMod`, including the Mersenne modulus |
| `ModInt61`, `Context::canon` (Mersenne paths through `Barrett64::fold61`) | Compile-time: alias identity with `ModInt64<2^61-1>`, modulus value and primality, `ModInt61::red` at 0, `M-1`, `M`, `2M`, `2^61 * M - 1` and `2^128-1`, `ModInt61::mul` at `(M-1)^2` and `2(M-1)`, `canon` at 0, `M-1`, `M` and `2M-1`; runtime: the 64-bit suites run every fixture and the Python oracle for modulus 2305843009213693951 as a static and as a dynamic modulus |
| `ModularInt`, `StaticModularInt` | Compile-time on all four full types, `int` and `ulng` (`compileTime`), and on all four minis in the mixed full/mini consumer unit (`05-modint_consumers.cpp`) |

Quick runs all API fixtures with smaller random corpora in optimized/checked scalar and available AVX2 builds. Full runs 3,000 random C++ pairs per large modulus, dynamic exhaustive rings through 60, 1,000 Python arithmetic/normalization cases per modulus plus roots/powers/boundaries, and six configurations: optimized `-O3 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS`, and ASan/UBSan `-O1`, each scalar and AVX2. Stress expands to 30,000 random pairs, dynamic rings through 120 and 10,000 Python cases per modulus with the same configurations. ISA execution is explicitly skipped if unavailable. Entry paths resolve independently of the caller's working directory.

## Commands and results

Prune, 2026-10-12 (`init`, the integral `operator=`, `operator!`, `modint_detail::MERSENNE61`, `fold61` and the `powerMontgomery` loop removed, `Context::canon` added, the `16-poly.hpp` call sites moved to `raw`; GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7, Intel Core i9-11900H):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/05-modint.hpp 01-Core/16-poly.hpp '96-Local Testing/01-Core/05-modint_benchmark.cpp' '96-Local Testing/01-Core/06-modintmini_tester.cpp'   # no violations
python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20261012                     # PASS, 4 types x 6 configurations, 826 s, peak 1390 MB
CXX=g++-14 python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20261012          # PASS, 4 types x 6 configurations, 828 s, peak 1415 MB
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20261012                 # PASS, 3 configurations, 101 s, peak 727 MB
CXX=g++-14 python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20261012      # PASS, 3 configurations, 106 s, peak 765 MB
python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode full --seed 1                              # PASS (the five raw call sites), 5 configurations, 63,201 checks each, 3253 s, peak 1906 MB
python3 '96-Local Testing/02-integration.py'                                                           # PASS, 136 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                           # no errors
python3 '96-Local Testing/01-Core/05-modint_benchmark.py' --seed 20261012 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/05-modint_benchmark.jsonl'   # 8,192 measurements, all verified
```

Each type runs optimized, checked and ASan/UBSan builds, scalar and AVX2, against the exact Python big-integer oracle (36,054 cases per 32-bit and 55,721 per 64-bit configuration at this seed), with 7 (static) or 11 (dynamic) precondition deaths per checked build, the Matrix/ModFac consumer smoke, compile-time zero-modulus rejection and both cross-translation-unit link orders; the durations include the quick pre-flight under the 2048 MB cap. The `-Wall -Wextra -Wshadow` builds compile without new warnings (the fixture's two pre-existing `-Wsign-compare` comparisons `EQUAL(..., 0)` predate this change). The mini suite verifies 176,644 exact-oracle and full/mini-agreement cases per configuration together with the minis' compile-time reduced contract, which no longer probes `init`. The poly run exercises the five `raw` call sites in the NTT, FFT-mod convolution, negacyclic and doubling paths with their schoolbook oracles.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/01-Core/05-modint_benchmark.py>) produced `96-Local Testing/01-Core/05-modint_benchmark.jsonl` on 2026-10-12 (seed 20261012, GCC 16.2.1, Intel Core i9-11900H, not frequency-isolated): 8,192 checked measurements across scalar `-mno-avx2 -mno-bmi2` and accelerated `-mavx2 -mbmi2 -mno-avx512f` builds (`-std=gnu++20 -O3 -DNDEBUG`), with source hashes, commands and exact output checks embedded. Each workload warms up by doubling its iteration count to at least 2 ms, then records the median of five repetitions. Ordinary products include uniform and near-modulus distributions at lengths 1, 7, 8, 9, 32, 256 and 4096; dependent chains use units. Moduli cover one, powers of two, common primes, Mersenne61, small 64-bit primes and full-word prime/composite boundaries; powers test eight bases with sparse/dense exponents through all 128 bits and threshold neighbors, with a `montgomery-noinline` candidate that calls `modint_detail::powerMontgomery` itself, a `montgomery-setup` candidate that constructs and inlines a constant-modulus `Montgomery64::pow`, and a `montgomery-cached` context; inverse workloads compare individual EEA, prime Fermat and public batch inversion including its allocation. Timings support these choices, not portable pass/fail gates.

Scalar medians, microseconds; `p64 = 18446744073709551557`, `M61 = 2^61-1`; the AVX2/BMI2 build agrees within noise except where noted.

| Workload | Reference | Now | Observation |
|---|---|---|---|
| Construct 4096 `mint` from `int` | 128-bit remainder 18.98 | 4.38 | 4.3x; `ModInt<4294967295>` 19.83 → 1.99 |
| Construct 4096 `DynModInt<>` (998244353) from `int` / `lng` | 18.67 / 18.73 | 9.96 / 10.55 | Barrett32 on the magnitude |
| Construct 4096 `DynModInt64<>` (p64) from `lng` | 18.84 | 11.29 | one 64-bit division |
| Construct 4096 `ModInt61` / static p64 from `lng` | 19.45 / 22.79 | 5.79 / 2.32 | constant multiply/shift |
| M61 4096 independent products | native `%` 8.68 (AVX2 build; the scalar cell, 27.69, is a transient), Barrett64 8.96, Montgomery convert 18.30 | fold 6.26 (`ModInt61` loop 6.95 vs 8.86 native) | 28% below native and Barrett64 |
| M61 256-factor chain | native 1.435, Barrett64 0.866, Montgomery domain 0.798 | fold 0.564 (type 0.526 vs 1.260) | fold beats the lazy Montgomery chain |
| M61 powers, exponent 512 / 65536 | `powerMontgomery` 0.280 / 0.388, cached Montgomery 0.161 / 0.264 | fold 0.160 / 0.248 (type 0.131 / 0.229) | the fold matches a cached Montgomery context without any setup |
| M61 powers, exponent 2^128-1 | `powerMontgomery` 3.205, cached 3.139 | fold 2.866 (type 3.153) | within noise of Montgomery; no exponent threshold justifies a second backend |
| Dynamic M61 powers 65536 / 2^128-1 | native 0.547 / 6.096 | type 0.290 / 2.980 | dynamic `mul` through the static Mersenne context |
| Static p64 powers 511 / 512 / 513 | native 0.438 / 0.287 / 0.315 | type 0.447 / 0.270 / 0.263 | threshold 512 retained (6% at 512, 17% at 513, 47% at 2^128-1: 3.576 vs 6.753) |
| Dynamic p64, exponent 2^128-1 | native 6.906 | type 3.560 | 48% lower |
| 256 static-prime inverses, modulus 998244353 | Fermat 20.84 | EEA 15.15; batch 1.73 | uniform EEA and the one-inverse batch API |
| 256 static p64 inverses | Fermat 112.27 | EEA 49.37; batch 3.60 | same choice for full width |

Threshold re-measurement, 2026-10-12: `montgomery-noinline` (the exact `powerMontgomery` path, per-call setup included) against native `%` on static p64, eight powers, scalar / AVX2: exponent 255 0.279 / 0.290 vs 0.334 / 0.373; 256 0.226 / 0.247 vs 0.215 / 0.240; 511 0.309 / 0.334 vs 0.381 / 0.408; 512 0.279 / 0.267 vs 0.268 / 0.269; 513 0.260 / 0.268 vs 0.305 / 0.296; 1023 0.390 / 0.370 vs 0.484 / 0.463; 1024 0.307 / 0.298 vs 0.315 / 0.312; 2^128-1 3.875 / 3.834 vs 6.629 / 5.358. Dense exponents gain 15–22% from 255 upward; sparse exponents (powers of two) lose 3–5% at 256, are within the run-to-run spread at 512 (over the twelve odd non-Mersenne 64-bit cells, static and dynamic in both builds, the helper ranges from 19% faster to 10% slower and is slower by 2–10% in five of them) and gain 3–4% at 1024. The threshold therefore stays at 512, the exponent value from which no power is slower beyond noise; the actual type measures 0.270 against 0.287 there. The inlined constant-modulus `montgomery-setup` candidate is slower than the `noinline` helper at every dispatched exponent (512 and above) for every static modulus (512: 0.326 vs 0.279; 2^128-1: 6.125 vs 3.875), the specialization regression the out-of-line boundary avoids; for the dynamic types the same boundary costs 2–25% against an inlined `Montgomery64(mod()).pow` at those exponents (scalar dynamic p64 at 512: 0.282 vs 0.239), a trade-off the recorded `noinline` decision accepts and this document records.

The 32-bit `DynModInt<>` construction from `int` (2.4 ns each) is bounded by the function-local reducer's initialization guard plus one Barrett step; a `constinit` reducer would need a constexpr Barrett constructor, which belongs to P004, and the guard is the same cost `*=` already pays.

## Sources

Inspected 2026-09-27; the implementation is independently written from the described algorithms and preserved API.

| Reference and inspected scope | Use and boundaries |
|---|---|
| [AtCoder Library modint documentation](https://raw.githubusercontent.com/atcoder/ac-library/master/document_en/modint.md), construction, arithmetic, raw, inverse, dynamic IDs/setup; [internal_math.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `inv_gcd` cofactor invariant | Compared canonical-value and unit contracts. ACL's narrower dynamic modulus domain does not justify our full-word domain. ACL is CC0; no code copied. |
| [cp-algorithms, Modular Multiplicative Inverse](https://cp-algorithms.com/algebra/module-inverse.html), EEA, prime/composite distinctions and array inverses | Unit-only division and the single-inverse prefix/suffix algorithm. The prime-only reciprocal recurrence is deliberately not used on composite rings. |
| Brent and Zimmermann, [Modern Computer Arithmetic](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf), version 0.5.9, 2010-10-07, §§2.5–2.5.1, pp.65–68, Algorithms 2.10 and 2.11 | Reduced-cofactor bounds and multiple inversion. Newton/Hensel, product-tree parallel inversion and arbitrary-precision half-GCD were assessed as different backends/workloads; they add no missing machine-word operation here. No source text/code adapted. |
| [cp-algorithms, Primality tests](https://cp-algorithms.com/algebra/primality_tests.html), deterministic 64-bit Miller–Rabin witnesses and base reduction | Seven-witness deterministic guarantee; zero-reduced witnesses are skipped. This is not randomized probable-prime checking. |
| [OI Wiki, 二次剩余](https://oi-wiki.org/math/number-theory/quad-residue/), prime-root shortcut, Cipolla, Tonelli–Shanks and nonresidue search | Preserved prime-root semantics, selected deterministic Tonelli–Shanks and stated scan cost. Its mentions of Bostan–Mori/Cipolla multiplication-count improvements and Muller variants were comparison scope, not a reviewed-paper/proof claim. Composite/all-root algorithms remain separately owned. No translated text/code copied. |
| C02 reduction evidence ([03-barrett.md](03-barrett.md), [04-montgomery.md](04-montgomery.md)) and actual Barrett/Montgomery headers | Reused verified full-width scalar backends, including mask/Mersenne paths; C03 measures wrapper/conversion/setup costs separately. |

The 2026-10-07 catalog sweep (ACL, suisen, hitonanode, yosupo, maspypy, Nyaan, ecnerwala, Benq, KACTL, cp-algorithms, OI Wiki) is in [00-sources.md](00-sources.md); rejected items with reasons are in [00-notes.md](00-notes.md).

Legacy: originals remain unchanged in `OLD/1-Core/03-modint.hpp` and `OLD/1-Core/04-dynmodint.hpp`; former wrapper headers in `OLD/2026-09-27-modint-layout/`. All legacy user operations remain (normalized construction/assignment, canonical raw initialization, public `n`, `mod`, arithmetic, truthiness, ordering, signed powers, unit inverse/division, prime-root lower-root/sentinel behavior, streams, constexpr static arithmetic/primality, independent dynamic IDs, `ModInt`/`DynModInt`/`mint`). Intentional changes: the 32-bit residue/modulus is unsigned to support the full word; narrowed signed casts require representability; raw initialization checks its precondition; zero-ring inversion follows the unit rule; stream input extends beyond signed 64 bits and rejects a malformed whole token; the old dynamic `IMOD` field is removed (C02's floor reciprocal differs from the old ceiling reciprocal). `OLD/Team Notebook/src/math/00-modint.hpp`, `00-dynmodint.hpp` and `old_modint.cpp` add no feature beyond the Core originals.

## Limits and handoffs

GCC 14.4.1 (`CXX=g++-14`, the floor check) passed; exact GCC 14.2, a Windows/MinGW build, Python 3.10 execution (scripts parse under 3.10 grammar) and PyPy were not run; no online acceptance is claimed.

### Dependent-owner handoff

Matrix (P020/C07, existing-unverified): its optimized modular kernels derive the modulus through an `int` cast, assume particular 32-bit ranges, and its field classification includes a compile-time use of `is_prime`. C03's wider domains and dynamic prime flags do not establish Matrix support for those domains. C07 must review full-width/64-bit modular matrices and dynamic field classification before advertising them; reconstruct matrices and cached factors after dynamic modulus changes. Start at `FastGaussian::getMod`, `Matrix::isField/is_field`, and the `is_mint32` accelerated paths: derive moduli through `T::mod()` with correct word widths, enforce each kernel's domain, and replace static field constraints with a runtime contract for dynamic types. Reproducer: `using D = DynModInt<>; static_assert(Matrix<D>::is_field);`. Generic Matrix/Poly code may constrain on `ModularInt`/`StaticModularInt`.

Catalog items for other owners, reported, not changed: double factorial and inverse binomial caches (`05-Mathematics/05-combinatorics.hpp`), fast-IO `rd`/`wt` overloads for residues (`06-Miscellaneous/03-fastio.hpp`), `randgen` hash base on `ModInt61` (Strings `randomBase`).

## History

- 2026-09-27: original P005 verification, full suites (35,811/55,323 oracle cases per configuration) and integration passed; first benchmark record.
- 2026-09-27: consolidation of the four full types into one header and one test entry, full suite passed.
- 2026-09-27: maintenance review, compile-time alias/width checks added, full suite and integration passed.
- 2026-10-07: re-audit, 6 findings fixed (complexity line, closing braces, 128-bit construction division, three mini findings), `ModInt61` and the two concepts added, quick, full and stress passed on g++; benchmark record replaced.
- 2026-10-07 (run): quick, full (seed 20260927) and one stress round (288,532/446,488 oracle cases per 32/64-bit configuration) passed, benchmark (7,760 measurements) recorded, independent review found zero mismatches.
- 2026-10-08: restyle (one trailing comment removed), full suites passed on g++ and g++-14, integration passed.
- 2026-10-12: prune (`/prune P005`): `init`, the integral `operator=`, `operator!`, `modint_detail::MERSENNE61`/`fold61` and the `powerMontgomery` loop removed, `Context::canon` added, the 512 Montgomery threshold re-measured and retained; full suites on g++ and g++-14, the mini and poly suites, integration and the benchmark (8,192 measurements) passed.
