# C03 modular integers — contracts and verification

P005/C03 owns `05-modint.hpp`, which declares all four full public types alongside their shared `modint_detail` implementation. Family-specific helpers and their users stay together, preserving identical full-width corner cases and failure semantics. P005/C16 separately owns the four independently copyable structs in `06-modintmini.hpp`; their [reduced contract and verification](25-modintmini.md) are separate from the full-family evidence below. C01 and C02 are the verified prerequisites. Original sources remain unchanged in `OLD/1-Core/03-modint.hpp` and `OLD/1-Core/04-dynmodint.hpp`.

## Types and domains

| Type | Modulus | State and representation |
|---|---|---|
| `ModInt<MOD>` | Compile-time `uint`, `1 <= MOD < 2^32` | One canonical `uint n`; `mint` remains `ModInt<998244353>` |
| `ModInt64<MOD>` | Compile-time `ulng`, `1 <= MOD < 2^64` | One canonical `ulng n` |
| `DynModInt<ID>` | Runtime `uint`, same full range | One canonical `uint n`, shared context per ID |
| `DynModInt64<ID>` | Runtime `ulng`, same full range | One canonical `ulng n`, shared context per ID and independent of 32-bit IDs |

Static zero-modulus instantiations are rejected. Dynamic types default to modulus 998244353; `setMod(m, prm=-1)` requires a nonzero representable modulus. `prm=-1` detects primality, `prm=0` disables the prime flag even if the modulus is prime, and `prm=1` promises an actual prime (checked by assertions). `isPrime()` computes the actual primality; `is_prime` reports the enabled flag, a compile-time constant for static types. Primality uses deterministic Miller–Rabin with the seven witnesses valid below 2^64.

Every `setMod` invalidates all previous values, references to their meaning, and caller caches, even for an identical modulus. Discard, copy without arithmetic, or overwrite stale storage; copies remain stale. Never read a stale residue as a live value or perform operations on it. Reconstruct input and caches after setup. No generation tags are stored: this preserves one-word, trivially copyable values and makes freshness a caller precondition. Distinct IDs and widths do not invalidate each other. A function-local reducer initializes before its first use, including global initializers and setup in another translation unit; this avoids unordered initialization of templated static objects. There are no owned allocations or growing caches in the context. Single-threaded semantics apply; concurrent mutation is outside the contract. Shared context fields are inspection-only; use `setMod` to change them.

## Operations and failure contracts

| Feature | Contract |
|---|---|
| Construction and assignment | All built-in integral types, including signed/unsigned 128-bit and signed minima, normalize modulo `mod()`; default is zero. Floating-point construction is excluded. |
| `val()`, `n`, `mod()` | Canonical unsigned representative and modulus. Public `n` may only be changed to a canonical residue. |
| `init(x)`, `raw(x)` | Same operation, requiring an already canonical word `x < mod()`; checked assertions. These do not normalize. |
| Integral conversion | Explicit conversion to a representable integer type; narrowed values require a fit. Use `val()` or the word-width unsigned cast for full-width results. `bool` means nonzero. |
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

## Correctness arguments and costs

For canonical `a,b`, addition compares `a` with `m-b` before adding; the selected sum is below `m`, and subtraction uses `m-(b-a)` only when `b>a`. Neither operation requires a wider carry. Products use exact double-width unsigned intermediates and the selected reduction backend. Signed normalization first converts the positive modulus to signed 128 bits, so negative extrema can take a remainder without negation or signed/unsigned usual-conversion surprises.

Euclid maintains remainder/coefficient pairs with `r = a*x (mod m)`. Success ends at gcd one. Standard Euclidean coefficient bounds keep the coefficients and their next recurrence values bounded by the input modulus; signed 128-bit intermediates safely cover the full unsigned 64-bit modulus domain. The final inverse coefficient lies strictly between `-m` and `m`, so one conditional addition normalizes it. The failure path never writes output.

Negative exponents obtain their magnitude through unsigned 128-bit negation, which remains defined for a signed minimum. Binary exponentiation maintains the accumulated product and successive squared base. Static operations, including inversion and roots, remain usable in constant expressions.

Square roots use the direct exponent followed by a square check when `p % 4 == 3`; other odd primes use a Legendre test and deterministic Tonelli–Shanks. The loop maintains `x^2 = a*t`, with `t` in a decreasing-order power-of-two subgroup. Searching the first nonresidue from two terminates for each odd prime. With `z` the first nonresidue, its stated worst-case arithmetic cost is `O((z + log(p)) * log(p))`, constant auxiliary memory; no unjustified expected-time claim is made. The returned minimum accounts for the two roots.

A product in the finite commutative ring `Z/mZ` is a unit iff each factor is a unit. Batch inversion stores prefix products, checks the inverse of the total, and walks backward to obtain each inverse. The original vector stays intact until the final move, including when input and output alias. Cost is `O(k + log(m))` time and `O(k)` temporary/output memory. Scalar arithmetic/construction costs constant bounded-word work; inverse and primality cost `O(log(m))` arithmetic steps, powers `O(1 + log(|e| + 1))` plus `O(log(m))` for a negative exponent, and stream parsing `O(d)` time and token memory for d characters.

## Migration accounting

All legacy user operations remain: normalized construction/assignment, canonical raw initialization, canonical public `n`, `mod`, arithmetic, truthiness, ordering, signed powers, unit inverse/division, prime-root lower-root/sentinel behavior, streams, constexpr static arithmetic/primality and independent dynamic IDs. `ModInt`/`DynModInt` names and `mint` remain available. Public types are now alias templates for the common value implementation; no code in the active repository specializes or forward-declares the old structs.

Intentional changes: the 32-bit stored residue/modulus is unsigned to support the full word; narrowed signed casts require representability. Raw initialization now checks its precondition. Zero-ring inversion is defined by the unit rule. Stream input extends beyond the old signed-64-bit extraction limit and rejects a malformed whole token. The old dynamic `IMOD` implementation field is removed: C02's floor reciprocal differs from the old ceiling reciprocal, and callers must not treat it as a stable API. `red`, `norm`, `MOD` and primality inspection remain; reduction uses the appropriate double-width input. Old tests that inspected `IMOD` or expected large valid decimal tokens to fail are superseded by the new contracts.

## Dependent-owner handoff

The existing Matrix implementation is still **existing-unverified**, owned by P020/C07. Its optimized modular kernels derive the modulus through an `int` cast, assume particular 32-bit ranges, and its field classification includes a compile-time use of `is_prime`. C03's wider domains and dynamic prime flags do not establish Matrix support for those domains. C07 must review full-width/64-bit modular matrices and dynamic field classification before advertising them; reconstruct matrices and cached factors after dynamic modulus changes. Start at `FastGaussian::getMod`, `Matrix::isField/is_field`, and the `is_mint32` accelerated paths: derive moduli through `T::mod()` with correct word widths, enforce each kernel's domain, and replace static field constraints with an appropriate runtime contract for dynamic types. A direct reproducer for the classification issue is `using D = DynModInt<>; static_assert(Matrix<D>::is_field);`, which requests a constant value from mutable `D::is_prime`. Ordinary `mint` compatibility is checked separately. This is a dependent-family gap, not missing modular-integer arithmetic.

## Backend selection

Static types use compiler-optimized double-width `%` for canonical multiplication. Dynamic 32-bit values use Barrett32; dynamic 64-bit values use native double-width `%` for general moduli, retaining Barrett64's mask and Mersenne61 paths. A generic full-width Barrett product can improve a dependent chain but costs more than native remainder on the measured independent-product workloads; the default favors ordinary canonical arithmetic. Canonical `n` storage also avoids a representation change for every access/order/stream operation.

At runtime, odd 64-bit powers use Montgomery with per-call setup when the exponent magnitude is at least 512; Mersenne61 keeps its cheap folding path below 65536. Even moduli and shorter powers use canonical multiplication. Constant evaluation always uses the canonical path. The runtime helper remains generic with GCC `noinline`/`noipa`: measured constant-modulus specialization of this particular full-width REDC loop regressed sparse static powers. This function boundary restored the intended improvement without changing representation or constexpr behavior. The thresholds include setup/conversions and were checked on both sides with sparse and dense exponents, not inferred solely from a multiplication microbenchmark.

The scalar value interface and batch inversion's dependent prefix chain offer no direct independent vector lanes. Canonical-to-Montgomery conversion costs were measured, including C02's AVX2 bulk routines; no additional type-level SIMD API is needed to expose the specified operations. C02 retains its explicit bulk/reusable-domain APIs for workloads that can stay in that representation. These are Core measurements, not evidence for the outside-Core 20% adoption rule or a universal fastest-modular-arithmetic claim.

## Verification and performance evidence

The single mirrored Python entry, `05-modint_tester.py`, runs all four type suites through `_05_modint_test_runner.py` and `05-modint_test_support.hpp`. The four `05-*_cases.cpp` fixtures each include the actual consolidated `05-modint.hpp`. Optional `--type modint32`, `--type modint64`, `--type dynmodint32`, or `--type dynmodint64` restrict a diagnostic rerun; the default `--type all` retains every suite. Assertions are never the test oracle, so checks remain live under `-DNDEBUG`. Test-only Python arbitrary integers and built-in modular `pow` provide independent exact answers, including for signed/unsigned 128-bit normalization and exponents.

| Feature → all four full types | Verification |
|---|---|
| Constructors, assignment, raw, access, casts, type/layout properties | All native widths and signed minima, uint/ulng/128 extrema; canonical raw endpoints and invalid raw/init subprocesses; one-word standard-layout/trivially-copyable assertions; narrowed-cast death case |
| Arithmetic, unary, mixed integer operands, comparison, bool, increments and aliasing | Exhaustive small rings, exact double-width boundary oracle, seeded random pairs, Python arbitrary-integer differential results; compound/self aliases and copy/move |
| Inverse, division, negative powers, zero ring | gcd unit classification, exact Python inverse, failures preserving output, input/output aliases, composite zero divisors, modulus-one identity, precondition subprocesses |
| Powers and selected backends | Zero/one, sparse/dense signed/unsigned 128-bit extrema, 511/512/513 and 65535/65536/65537 neighbors, even/power-of-two/Mersenne61 and full-word odd moduli; constexpr powers through the same thresholds |
| Prime roots | Exhaustive enumeration of least roots in small prime fields, independently formed random squares in large primes including Goldilocks' large power-of-two subgroup, Python nonresidues, preserved outputs/sentinel, aliases and nonprime/disabled-flag deaths |
| Batch inversion | Empty, singleton, varied lengths, arbitrary units and composites, product/inverse identities, nonunits at varied positions, nonzero zero divisors, exact vector aliasing and unchanged failure output |
| Streams | Whitespace, signs, signed minima, thousands of digits, invalid whole tokens, EOF and output; independent exact decimal reductions |
| Static primality and constexpr | Prime/composite and strong-pseudoprime regressions through full width; compile-time arithmetic/inverse/root/power; zero modulus compile rejection |
| Dynamic IDs, reset, primality and reduction | Independent contexts and widths, repeated/same-modulus setup, prime hints, reconstructing overwritten old storage, brute-force small primality oracle, full double-width dividends, global default/custom setup and two-TU startup in both link orders |
| Header and consumer integration | All standalone headers, Basic/All and two-TU scalar/AVX2 builds; ordinary `mint` Matrix product/determinant and ModFac counting smoke; this does not verify those dependent families |

Quick runs all API fixtures with smaller random corpora in optimized/checked scalar and available AVX2 builds. Full runs 3,000 random C++ pairs per large modulus, dynamic exhaustive rings through 60, 1,000 Python arithmetic/normalization cases per modulus plus roots/powers/boundaries, and six configurations: optimized `-O3 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS`, and ASan/UBSan `-O1`, each scalar and AVX2. Stress expands to 30,000 random pairs, dynamic rings through 120 and 10,000 Python cases per modulus with the same configurations. ISA execution is explicitly skipped if unavailable. Entry paths resolve independently of the caller's working directory.

Reproduction commands (seed 20260927):

```bash
python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/02-integration.py'
```

The original P005 verification runs on 2026-09-27 used GCC 16.2.1 (20260810), GNU++20, Linux x86-64 on Intel Core i9-11900H, and CPython 3.14. Both 32-bit entries passed all six configurations with 35,811 exact Python cases per configuration. Both 64-bit entries passed all six with 55,323 cases per configuration. Final integration passed all 61 standalone/aggregate headers, scalar/AVX2 multiple-translation-unit linkage and Workspace compilation. All owned C03 features and verification are complete; there is no C03 implementation handoff gap. The additional independent review passed 400,000 inverse cases, 6,000 roots and 8,664 exact power cases under UBSan; the committed suites carry the relevant regressions.

Sanitizers were run outside the process-traced sandbox after LeakSanitizer explicitly refused the sandbox environment; address/undefined checks and leak detection passed in the completed runs. Python scripts also parse under Python 3.10 grammar. Exact GCC14/Python3.10 execution and PyPy are unavailable here, so no result on those runtimes is claimed. No online submission or acceptance is claimed. The benchmark record includes source/header SHA256 values; the pre-consolidation common engine hash was `b843fa0e00db856113914842bab17c90ee560fdc9f06376a391e77a43f9273fb`. Historical results and hashes are not claims about freshly measured files.

## Consolidation verification — 2026-09-27

The four full types now share one public header and one runnable test entry. The common arithmetic/context implementation is unchanged after ignoring the added namespace indentation; all four public alias declarations are identical to their former declarations. The consolidation snapshot `05-modint.hpp` SHA256 was `255025fb4500f95cc6299d7f2a61a627a783e80298fa2149b912f23ec7d68576`; the later maintenance hash is recorded below. Former wrapper headers are preserved under `OLD/2026-09-27-modint-layout/`; their live include paths have been replaced by `05-modint.hpp`. No compatibility wrappers remain in active Core.

The consolidated command shown above passed in full mode with seed 20260927: all four types in optimized, checked, and ASan/UBSan configurations, each scalar and AVX2. Each 32-bit type passed 35,811 independent Python cases per configuration; each 64-bit type passed 55,323. Both static zero-modulus compile failures, the Matrix/ModFac scalar/AVX2 smoke, and both dynamic cross-translation-unit global-initialization fixtures in both link orders passed. The initial sandbox run reached LeakSanitizer's explicit ptrace incompatibility; the completed run outside that tracing sandbox passed address, undefined-behavior and leak checks. No test coverage was removed when the four entries were consolidated.

The four C++ variant helpers are `05-modint32_cases.cpp`, `05-modint64_cases.cpp`, `05-dynmodint32_cases.cpp` and `05-dynmodint64_cases.cpp`. Generated zero-modulus and cross-translation-unit sources name `05-modint.hpp` directly instead of deriving a header from a former test filename. The benchmark source compiles with both scalar and AVX2/BMI2 flags, and updated Python scripts parse under Python 3.10 grammar. This maintenance did not repeat performance measurements: the existing benchmark JSONL is byte-for-byte unchanged, and its hashes and old paths identify the historical measurements.

## Maintenance review — 2026-09-27

The P005/C03 review checked all four full types against the current C++ guide and [Core migration record](../00-Guidelines/22-core-migration.md). The shared implementation remains in the shallow, four-space-indented `modint_detail` namespace in `05-modint.hpp`. Five shared concept uses now explicitly name `modint_detail::Integer`; method grouping and construction/assignment spacing follow the current guide. The lifecycle comment now states the preserved opaque-copy rule precisely: copied stale storage remains stale and cannot be read as a live value or used in arithmetic. Arithmetic, backend selection, domains, member/state ordering, all four public alias declarations, default dynamic IDs and `mint` are unchanged. The mini review is recorded separately in [C16 evidence](25-modintmini.md).

Persistent full-family tests now check public alias identity, exact modulus return widths, default IDs and distinct width/ID types at compile time. Test support received closing-brace formatting fixes. Benchmark support received the same formatting and named-lambda spacing fixes, and its runtime constant uses the lower-case name `exponents`. Both scalar and AVX2/BMI2 optimized benchmark builds passed. Historical JSONL measurements and their hashes remain byte-for-byte unchanged; this review did not repeat timings or change a measured algorithm.

Fresh verification used the consolidated full command above with seed 20260927 on GCC 16.2.1 and CPython 3.14.7. All four types passed all six optimized, checked and ASan/UBSan scalar/AVX2 configurations: **35,811 exact Python cases per configuration for each 32-bit type and 55,323 for each 64-bit type**, totaling 1,093,608 cases. Both static zero-modulus rejections, Matrix/ModFac scalar/AVX2 smoke tests and both dynamic cross-translation-unit default/custom global-initialization checks in both link orders passed. Address, undefined-behavior and leak checks passed outside the known process-tracing restriction. The Python test/benchmark scripts also parse under Python 3.10 grammar; exact older-runtime availability remains as documented above.

Current integration passed all **59 standalone/aggregate headers**, scalar/AVX2 multiple-translation-unit linkage and Workspace compilation. The repository consistency validator reported no errors. Existing numbering, inventory status, aggregate membership, package ownership and shared maps required no changes; one integration owner reviewed those records. The maintained full header SHA256 is `49135a97b58d627d82952177ed348e98aad93134c6f6cd5f228720ca9afde684`. No C03 verification gap remains. The dependent Matrix/P020/C07 handoff above remains open under its existing owner.

## Benchmark record

The committed [benchmark driver](<../96-Local Testing/01-Core/05-modint_benchmark.py>) reproduces the workloads in the historical [JSONL measurements](<../96-Local Testing/01-Core/05-modint_benchmark.jsonl>): 7,492 checked measurements across scalar and AVX2/BMI2 builds. The JSONL retains the original source hashes and the former wrapper paths `06-modint64.hpp`, `07-dynmodint.hpp` and `08-dynmodint64.hpp` as immutable historical evidence. Current includes and source hashing use only `05-modint.hpp` plus its reduction dependencies. No performance run is claimed for the consolidation: only namespace indentation, alias placement, include paths and test organization changed; arithmetic and backend selection did not. To make fresh measurements from the current source, run:

```bash
python3 '96-Local Testing/01-Core/05-modint_benchmark.py' --seed 20260927 --repetitions 5 --milliseconds 1 --output /tmp/p005-benchmark.jsonl
```

The original benchmark run used GCC 16.2.1, `-std=gnu++20 -O3 -DNDEBUG`, scalar `-mno-avx2 -mno-bmi2` and accelerated `-mavx2 -mbmi2 -mno-avx512f`, on the CPU above. Each workload warmed up by doubling its iteration count to at least 1 ms, then recorded the median of five repetitions. Seeds, compiler commands, configuration, source hashes, exact output checks and allocation/setup boundaries are embedded in the record. Ordinary products include uniform and near-modulus distributions at lengths 1, 7, 8, 9, 32, 256 and 4096; dependent chains use units to avoid accidental zero collapse. Fixed and dynamic moduli cover one, powers of two, common primes, Mersenne61, small 64-bit primes and full-word prime/composite boundaries. Powers test eight bases with sparse/dense exponents through all 128 bits and threshold neighbors. Inverse workloads compare individual EEA, prime Fermat and public batch inversion including its allocation.

Representative scalar medians below compare the same inputs; power times cover all eight bases and include per-power Montgomery setup/conversion. `p64 = 18446744073709551557`.

| Workload | Baseline | Selected implementation | Observation |
|---|---|---|---|
| Static p64, exponent 512 | native `%`: 0.284 µs | 0.262 µs | About 8% lower near the conservative threshold |
| Static p64, exponent `2^128-1` | native `%`: 6.619 µs | 3.583 µs | About 46% lower |
| Dynamic p64, exponent `2^128-1` | native `%`: 7.705 µs | 4.217 µs | About 45% lower |
| Static Mersenne61, exponent 65536 | native `%`: 0.633 µs | 0.418 µs | About 34% lower at its threshold |
| 256 static-prime inverses, modulus 998244353 | Fermat: 21.787 µs | EEA: 15.411 µs; batch: 1.769 µs | Supports uniform EEA and the one-inverse batch API |
| 256 static p64 inverses | Fermat: 147.069 µs | EEA: 65.464 µs; batch: 5.061 µs | Same choice for full width |

AVX2/BMI2 builds also preserved the power improvements: static p64 maximum-128-bit powers measured 5.590 versus 9.872 µs, and the Mersenne61 threshold 0.483 versus 0.731 µs. These figures do not imply vectorizing one scalar dependency chain; compiler flags and host noise affect the measurements. The shared machine was not frequency-isolated, and timings are evidence for these choices, not portable pass/fail gates. The native path remains the compact fallback; specialized conversion/setup does not pay universally on short powers or ordinary products.

## Research and provenance

Inspected 2026-09-27; the implementation is independently written from the described algorithms and preserved API. Source review does not constitute online acceptance or a proof supplied by the source's popularity.

| Reference and inspected scope | Use and boundaries |
|---|---|
| [AtCoder Library modint documentation](https://raw.githubusercontent.com/atcoder/ac-library/master/document_en/modint.md), construction, arithmetic, raw, inverse, dynamic IDs/setup; [internal_math.hpp](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `inv_gcd` cofactor invariant | Compared canonical-value and unit contracts. ACL's narrower dynamic modulus domain does not justify our full-word domain. ACL is CC0; no code copied. |
| [cp-algorithms, Modular Multiplicative Inverse](https://cp-algorithms.com/algebra/module-inverse.html), EEA, prime/composite distinctions and array inverses | Unit-only division and the single-inverse prefix/suffix algorithm. The prime-only reciprocal recurrence is deliberately not used on composite rings. |
| Brent and Zimmermann, [Modern Computer Arithmetic](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf), version 0.5.9, 2010-10-07, §§2.5–2.5.1, pp.65–68, Algorithms 2.10 and 2.11 | Reduced-cofactor bounds and multiple inversion. Newton/Hensel, product-tree parallel inversion and arbitrary-precision half-GCD were assessed as different backends/workloads; they add no missing machine-word operation here. No source text/code adapted. |
| [cp-algorithms, Primality tests](https://cp-algorithms.com/algebra/primality_tests.html), deterministic 64-bit Miller–Rabin witnesses and base reduction | Seven-witness deterministic guarantee; zero-reduced witnesses are skipped. This is not randomized probable-prime checking. |
| [OI Wiki, 二次剩余](https://oi-wiki.org/math/number-theory/quad-residue/), prime-root shortcut, Cipolla, Tonelli–Shanks and nonresidue search | Preserved prime-root semantics, selected deterministic Tonelli–Shanks and stated scan cost. Its mentions of Bostan–Mori/Cipolla multiplication-count improvements and Muller variants were comparison scope, not a reviewed-paper/proof claim. Composite/all-root algorithms remain separately owned. No translated text/code copied. |
| [C02 reduction evidence](23-reduction.md) and actual Barrett/Montgomery headers | Reused verified full-width scalar backends, including mask/Mersenne paths; C03 measures wrapper/conversion/setup costs separately. |

The older `OLD/Team Notebook/src/math/00-modint.hpp`, `00-dynmodint.hpp` and `old_modint.cpp` were also compared by operation search. They add no feature beyond the preserved Core originals, and contain narrower arithmetic contracts; all remain archived unchanged.
