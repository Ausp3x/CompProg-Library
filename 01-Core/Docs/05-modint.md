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
| `modint_detail::powerMontgomery` (REDC loop and per-call setup) | `modint_detail::power(C::montgomery, a, e)`: the same `noinline` boundary, on the context's cached `Montgomery64` |

### ModularInt, StaticModularInt

Public concepts for generic Poly/Matrix dispatch: `ModularInt<T>` accepts any full or mini residue type (`Word`, `mod()`, `val()`, `n`); `StaticModularInt<T>` additionally requires a constant `mod()`.

### Context (mul, red, canon, montgomery, reduction), norm, power

`Value::operator*=` calls `Context::mul`, the canonical-factor product hook; `Context::red` keeps accepting any double-width dividend. Static types use the native double-width `%` for canonical multiplication (a constant-modulus multiply/shift for 32-bit words, a `__umodti3` call for 64-bit ones), except the Mersenne modulus `2^61-1` (`Barrett64::MERSENNE61`, exposed as `ModInt61`): its product is one 64×64 multiply, one `Barrett64::fold61` and the canonical step `Context::canon` (a static-context member, so `ModInt61::canon` and `mint::canon` exist and the dynamic types have none: the residue of a value below `2 * MOD`, which one fold of every canonical product leaves, taken branch-free as `min(y, y - MOD)` on the wrapped difference because a data-dependent branch there mispredicts half the time in a power chain; the comparison runs in the word when `2 * MOD` fits it and in the double word otherwise, so the whole domain `y < 2 * MOD` holds for every modulus), and `red` takes an arbitrary 128-bit dividend with two folds followed by `canon`. Dynamic 32-bit values use Barrett32; dynamic 64-bit values use native double-width `%` for general moduli, retaining Barrett64's mask and two-fold Mersenne61 paths for `red`, while `mul` delegates to the static Mersenne context's single-fold product when the runtime modulus is `2^61-1`. A generic full-width Barrett product can improve a dependent chain but costs more than native remainder on the measured independent-product workloads; the default favors ordinary canonical arithmetic. Canonical `n` storage also avoids a representation change for every access/order/stream operation.

`norm`: `lll`/`ulll` keep the 128-bit remainder; every other integral type takes its magnitude in a 32- or 64-bit word (`U = uint` for inputs of at most four bytes), reduces it with `Context::red` for `uint` words (constant-modulus multiply/shift when static, Barrett32 when dynamic) or `u % mod()` for `ulng` words, and negates the residue when the input was negative. Signed minima are safe because `U(0) - U(a)` wraps to the exact magnitude. No native-width construction emits `__modti3` (`g++ -O2 -S`). `pow` starts from `raw(mod() != 1)`.

Fold correctness: for canonical `a, b < M = 2^61-1`, `x = a * b < M^2`, so `x >> 61 <= M - 1` and `fold61(x) = (x & M) + (x >> 61) <= 2M - 1`; `canon` returns `min(y, y - M)` in unsigned arithmetic, which is `y - M` exactly when `y >= M` (otherwise the difference wraps above `y`), and `x ≡ fold61(x) (mod M)` because `2^61 ≡ 1`. For an arbitrary 128-bit dividend the first fold leaves `y < 2^67 + 2^61` and the second `(y & M) + (y >> 61) <= M + 64 < 2M`, again `canon`'s domain; both bounds are stated and tested for `Barrett64::fold61` in [03-barrett.md](03-barrett.md).

At runtime, 64-bit powers with an odd modulus below `2^62`, other than Mersenne61, run on the context's cached `Montgomery64 montgomery` from exponent magnitude 5 upward, through `modint_detail::power(c, a, e)`, a `noinline`/`noipa` one-liner over `c.pow`. Every context holds that member, the `Montgomery64` of `mod | 1`: a `constexpr` member of the static contexts (so even moduli stay constructible and simply never dispatch) and a `constinit` member of the dynamic contexts that `setMod` rebuilds beside the Barrett reducer for 64-bit words, with the same lifetime rules; 32-bit contexts keep the default context unread. No setup is paid per call. The function boundary stays on purpose: when GCC inlines `Montgomery64::pow` under a constant modulus it compiles REDC's correction into a branch-free chain on the critical path, which measured 1.3–1.9x slower at `-O3` than the generic loop with its well-predicted branch (independent review, 4096 distinct bases). The modulus bound is a measured property of REDC, not of the cache: the canonical correction `r - (r < h || r >= mod ? mod : 0)` is taken with probability about `mod / 2^64`, so the branch is predictable for small moduli and a coin flip near `2^64`, and on this CPU's fast 64-bit division the mispredictions cost more than the division saves (measured with 4096 distinct bases, below). Mersenne61 keeps its fold product, which beats even the cached context at every exponent length; even moduli, moduli from `2^62` upward, exponents below 5 and constant evaluation use canonical multiplication.

The scalar value interface and batch inversion's dependent prefix chain offer no direct independent vector lanes. Canonical-to-Montgomery conversion costs were measured, including C02's AVX2 bulk routines; no additional type-level SIMD API is needed to expose the specified operations. C02 retains its explicit bulk/reusable-domain APIs for workloads that can stay in that representation. These are Core measurements, not evidence for the outside-Core 20% adoption rule or a universal fastest-modular-arithmetic claim.

### Correctness and cost

For canonical `a,b`, addition compares `a` with `m-b` before adding; the selected sum is below `m`, and subtraction uses `m-(b-a)` only when `b>a`. Neither operation requires a wider carry. Products use exact double-width unsigned intermediates and the selected reduction backend. Signed normalization first converts the positive modulus to signed 128 bits, so negative extrema can take a remainder without negation or signed/unsigned usual-conversion surprises.

Euclid maintains remainder/coefficient pairs with `r = a*x (mod m)`. Success ends at gcd one. Standard Euclidean coefficient bounds keep the coefficients and their next recurrence values bounded by the input modulus; signed 128-bit intermediates safely cover the full unsigned 64-bit modulus domain. The final inverse coefficient lies strictly between `-m` and `m`, so one conditional addition normalizes it. The failure path never writes output.

Negative exponents obtain their magnitude through unsigned 128-bit negation, which remains defined for a signed minimum. Binary exponentiation maintains the accumulated product and successive squared base. Static operations, including inversion and roots, remain usable in constant expressions.

Square roots use the direct exponent followed by a square check when `p % 4 == 3`; other odd primes use a Legendre test and deterministic Tonelli–Shanks. The loop maintains `x^2 = a*t`, with `t` in a decreasing-order power-of-two subgroup. Searching the first nonresidue from two terminates for each odd prime. With `z` the first nonresidue, its stated worst-case arithmetic cost is `O((z + log(p)) * log(p))`, constant auxiliary memory; no unjustified expected-time claim is made. The returned minimum accounts for the two roots.

A product in the finite commutative ring `Z/mZ` is a unit iff each factor is a unit. Batch inversion stores prefix products, checks the inverse of the total, and walks backward to obtain each inverse. The original vector stays intact until the final move, including when input and output alias. Cost is `O(k + log(m))` time and `O(k)` temporary/output memory. Scalar arithmetic/construction costs constant bounded-word work; inverse and primality cost `O(log(m))` arithmetic steps, powers `O(1 + log(|e| + 1))` plus `O(log(m))` for a negative exponent, and stream parsing `O(d)` time and token memory for d characters.

## Feature-to-test map

The single mirrored Python entry, `05-modint_tester.py`, runs all four type suites through `_05_modint_test_runner.py` and `05-modint_test_support.hpp`. The four `05-*_cases.cpp` fixtures each include the actual consolidated `05-modint.hpp`. Optional `--type modint32`, `--type modint64`, `--type dynmodint32`, or `--type dynmodint64` restrict a diagnostic rerun; the default `--type all` retains every suite. Test-only Python arbitrary integers and built-in modular `pow` provide independent exact answers, including for signed/unsigned 128-bit normalization and exponents.

| Feature → all four full types | Verification |
|---|---|
| Constructors, assignment, raw, access, casts, type/layout properties | All native widths and signed minima, uint/ulng/128 extrema, assignment from every integral type through the constructor; canonical raw endpoints (0 and `mod() - 1`, constexpr `raw(6)`) and the invalid raw subprocess; one-word standard-layout/trivially-copyable assertions; narrowed-cast death case |
| Arithmetic, unary, mixed integer operands, comparison, bool and `!`, increments and aliasing | Exhaustive small rings, exact double-width boundary oracle, seeded random pairs, Python arbitrary-integer differential results; compound/self aliases and copy/move |
| Inverse, division, negative powers, zero ring | gcd unit classification, exact Python inverse, failures preserving output, input/output aliases, composite zero divisors, modulus-one identity, precondition subprocesses |
| Powers and selected backends (the cached `montgomery` context at `e >= 5` on odd non-Mersenne 64-bit moduli below `2^62`) | Zero/one, 2–8 around the threshold, sparse/dense signed/unsigned 128-bit extrema, 511/512/513 and 65535/65536/65537 neighbors, even/power-of-two/Mersenne61 and full-word odd moduli against Python `pow`; constexpr powers through the same exponents on the canonical path |
| Prime roots | Exhaustive enumeration of least roots in small prime fields, independently formed random squares in large primes including Goldilocks' large power-of-two subgroup, Python nonresidues, preserved outputs/sentinel, aliases and nonprime/disabled-flag deaths |
| Batch inversion | Empty, singleton, varied lengths, arbitrary units and composites, product/inverse identities, nonunits at varied positions, nonzero zero divisors, exact vector aliasing and unchanged failure output |
| Streams | Whitespace, signs, signed minima, thousands of digits, invalid whole tokens, EOF and output; independent exact decimal reductions |
| Static primality and constexpr | Prime/composite and strong-pseudoprime regressions through full width; compile-time arithmetic/inverse/root/power; the constexpr `montgomery` member (`mod`, `mod * inv == -1`, `M \| 1` for an even modulus); zero modulus compile rejection |
| Dynamic IDs, reset, primality and reduction | Independent contexts and widths, repeated/same-modulus setup, prime hints, reconstructing overwritten old storage, brute-force small primality oracle, full double-width dividends, `montgomery.mod` and `mod * inv == -1` after the default and the `setMod` calls to 97 and 12 (`m \| 1`), every other modulus through the power oracle, global default/custom setup and two-TU startup in both link orders |
| Header and consumer integration | All standalone headers, Basic/All and two-TU scalar/AVX2 builds; ordinary `mint` Matrix product/determinant and ModFac counting smoke; this does not verify those dependent families |
| `norm` (native widths and 128-bit) | `construction<M>` compares `M::norm` with an independent 128-bit signed/unsigned formula for every integral type and extreme; the Python `N` oracle builds through the narrowest native width (`int`/`lng`/`lll`, `uint`/`ulng`/`ulll`) at `±2^31 ± 1`, `2^32 ± 1`, `±2^63`, `2^64` and random 128-bit values |
| `Context::mul`, `red`, `reduction` | `pairCase` checks `M::mul` against the exact double-width product on every exhaustive, boundary and random pair; `verifyMod` checks `red` on six dividends including `2^128-1`; `contexts` checks `reduction().mod`, a double-width `reduce`, `mul` and `red` at `(m-1)^2` after `setMod`, including the Mersenne modulus |
| `ModInt61`, `Context::canon` (Mersenne paths through `Barrett64::fold61`) | Compile-time: alias identity with `ModInt64<2^61-1>`, modulus value and primality, `ModInt61::red` at 0, `M-1`, `M`, `2M`, `2^61 * M - 1` and `2^128-1`, `ModInt61::mul` at `(M-1)^2` and `2(M-1)`, `canon` at 0, `M-1`, `M` and `2M-1` for Mersenne61 and, above `2^(w-1)`, for 4294967291, 2147483648 and p64 at `2^w` and `2M-1`; runtime: the 64-bit suites run every fixture and the Python oracle for modulus 2305843009213693951 as a static and as a dynamic modulus |
| `ModularInt`, `StaticModularInt` | Compile-time on all four full types, `int` and `ulng` (`compileTime`), and on all four minis in the mixed full/mini consumer unit (`05-modint_consumers.cpp`) |

Quick runs all API fixtures with smaller random corpora in optimized/checked scalar and available AVX2 builds. Full runs 3,000 random C++ pairs per large modulus, dynamic exhaustive rings through 60, 1,000 Python arithmetic/normalization cases per modulus plus roots/powers/boundaries, and six configurations: optimized `-O3 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS`, and ASan/UBSan `-O1`, each scalar and AVX2. Stress expands to 30,000 random pairs, dynamic rings through 120 and 10,000 Python cases per modulus with the same configurations.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 1057 MB; 24 builds, 240.9 s compiling, 7.8 s running, longest run 1.0 s; MEMORY peak 2702 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 1122 MB; 24 builds, 868.7 s compiling, 28.4 s running, longest run 3.0 s; MEMORY peak 2756 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: 16 builds, 1039.4 s compiling, 1.3 s running, longest run 0.2 s; MEMORY peak 2475 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 344 MB; 24 builds, 2.3 s compiling, 53.6 s running, longest run 7.0 s; MEMORY peak 528 MB. Command limit 600 s instead of 300 s: g++-14 sanitized compiles took up to 207 s on a cold full run beside the other suites. Dated rows below stay those commands' latest results.

Cached Montgomery contexts, 2026-10-12 (`Context::montgomery` added and reached through the out-of-line `modint_detail::power`, `powerMontgomery` removed, dispatch for odd moduli below `2^62` from exponent 5, branch-free `canon` on its whole domain, warning-free fixture; GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7, Intel Core i9-11900H):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/04-montgomery.hpp 01-Core/05-modint.hpp '96-Local Testing/01-Core/05-modint_test_support.hpp' '96-Local Testing/01-Core/05-modint_benchmark.cpp'   # no violations
python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20261012                     # PASS, 4 types x 6 configurations, 722 s, peak 1449 MB
CXX=g++-14 python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20261012          # PASS, 4 types x 6 configurations, 687 s, peak 1504 MB
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20261012                 # PASS, 3 configurations, 78 s, peak 723 MB
CXX=g++-14 python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20261012      # PASS, 3 configurations, 85 s, peak 756 MB
python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode quick --seed 1                             # PASS (consumer smoke), 3 configurations, 51,278 checks each, 299 s, peak 1303 MB
python3 '96-Local Testing/02-integration.py'                                                           # PASS, 136 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                           # no errors
python3 '96-Local Testing/01-Core/05-modint_benchmark.py' --seed 20261012 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/05-modint_benchmark.jsonl'   # 11,808 measurements, all verified
```

Each type runs optimized, checked and ASan/UBSan builds, scalar and AVX2, against the exact Python big-integer oracle (39,404 cases per 32-bit and 60,883 per 64-bit configuration at this seed, exponents 0–8 included), with 7 (static) or 11 (dynamic) precondition deaths per checked build, the Matrix/ModFac consumer smoke, compile-time zero-modulus rejection and both cross-translation-unit link orders; the durations include the quick pre-flight under the 2048 MB cap and ran beside the other suites, so they are upper bounds. The fixture builds add `-Wconversion -Werror` to the runner's `-Wall -Wextra -Wshadow`, so they compile without any warning on both compilers (`EQUAL` compares integers through `std::cmp_not_equal`). The mini suite verifies 176,644 exact-oracle and full/mini-agreement cases per configuration together with the minis' compile-time reduced contract. The poly quick run is the consumer smoke for the unchanged `pow` results; its full run passed earlier the same day on the pruned header.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/01-Core/05-modint_benchmark.py>) produced `96-Local Testing/01-Core/05-modint_benchmark.jsonl` on 2026-10-12 (seed 20261012, GCC 16.2.1, Intel Core i9-11900H, a shared desktop host that is not frequency-isolated; the JSONL's source hashes are those of the committed headers): 11,808 checked measurements across scalar `-mno-avx2 -mno-bmi2` and accelerated `-mavx2 -mbmi2 -mno-avx512f` builds (`-std=gnu++20 -O3 -DNDEBUG`), with commands and exact output checks embedded. Each workload warms up by doubling its iteration count to at least 2 ms, then records the median of five repetitions. Ordinary products include uniform and near-modulus distributions at lengths 1, 7, 8, 9, 32, 256 and 4096; dependent chains use units. Moduli cover one, powers of two, common primes, Mersenne61, `2^62-57`, `2^63-25`, Goldilocks `2^64-2^32+1`, `2^64-59` and `2^64-1`; every power workload runs **4096 distinct random bases per pass** with exponents 0–7, sparse/dense values through all 128 bits and the threshold neighbors, against a `montgomery-cached` context, a per-call `montgomery-setup` and native `%` (distinct bases keep the branch predictor from learning REDC's data-dependent correction); inverse workloads compare individual EEA, prime Fermat and public batch inversion including its allocation. Timings support these choices, not portable pass/fail gates.

Scalar medians; constructions and products in microseconds per workload, powers in nanoseconds per power; `p64 = 2^64-59`, `M61 = 2^61-1`; the AVX2/BMI2 build agrees within noise except where noted.

| Workload | Reference | Now | Observation |
|---|---|---|---|
| Construct 4096 `mint` from `int` | 128-bit remainder 18.84 | 4.37 | 4.3x; `ModInt<4294967295>` 18.55 → 1.95 |
| Construct 4096 `DynModInt<>` (998244353) from `int` / `lng` | 18.06 / 14.14 | 10.06 / 9.44 | Barrett32 on the magnitude |
| Construct 4096 `DynModInt64<>` (p64) from `lng` | 19.55 | 11.14 | one 64-bit division |
| Construct 4096 `ModInt61` / static p64 from `lng` | 18.70 / 23.65 | 5.60 / 2.15 | constant multiply/shift |
| M61 4096 independent products | native `%` 8.19, Barrett64 8.65, Montgomery convert 16.64 | fold 3.74 (`ModInt61` loop 3.98 vs 9.31 native) | 2.2x below native with the branch-free `canon` |
| M61 256-factor chain | native 1.301, Barrett64 0.802, Montgomery domain 0.725 | fold 0.569 (type 0.622 vs 1.227) | fold beats the lazy Montgomery chain; the branch-free `canon` costs this dependent chain about 15% against the branchy form while it halves independent products and powers |
| `ModInt61` powers 7 / 512 / 65536 / 2^128-1 | native 11.5 / 37.4 / 75.0 / 825; cached Montgomery 10.3 / 23.4 / 46.2 / 492 | type 5.3 / 15.2 / 29.0 / 356 | 2.5x below native and 1.5x below Montgomery at 512: Mersenne61 keeps its fold |
| Dynamic M61 powers 512 / 2^128-1 | native 34.2 / 833 | type 16.4 / 382 | dynamic `mul` through the static Mersenne context |
| `ModInt64<998244353>` powers 5 / 7 / 31 / 512 / 65536 / 2^128-1 | native 10.2 / 12.9 / 24.1 / 32.0 / 55.1 / 740 | type 9.4 / 10.4 / 14.2 / 19.7 / 36.6 / 372 | cached Montgomery below `2^62`: 8% at 5, 19% at 7, 41% at 31, 38% at 512, 50% at 2^128-1 |
| `ModInt64<2^62-57>` powers 7 / 31 / 512 / 2^128-1 | native 14.7 / 27.4 / 37.6 / 832 | type 13.8 / 21.9 / 32.0 / 568 | 6% / 20% / 15% / 32%: the last modulus class that profits, within ±10% of native below exponent 7 |
| `ModInt64<2^64-59>` powers 7 / 512 / 2^128-1 | native 13.7 / 32.7 / 836; cached Montgomery 15.1 / 56.7 / 1026 | type (canonical) 14.0 / 34.6 / 789 | Montgomery would cost 1.1–1.7x here: moduli from `2^62` stay canonical |
| Dynamic 998244353 / 2^62-57 / p64 powers 512 | native 35.8 / 31.9 / 35.5 | type 22.3 / 27.6 / 39.2 | 38% / 13% below native; the dynamic canonical path pays 5–30% for its per-multiply modulus checks and the out-of-line `__umodti3` |
| 256 static-prime inverses, modulus 998244353 | Fermat 20.38 | EEA 14.53; batch 1.56 | uniform EEA and the one-inverse batch API |
| 256 static p64 inverses | Fermat 103.13 | EEA 48.74; batch 3.61 | same choice for full width |

Dispatch measurement, 2026-10-12, within one workload (`montgomery-cached` against `native-percent`, both over 4096 distinct bases), static / dynamic contexts, scalar build, exponent 512: 998244353 0.64 / 0.59, 4294967311 0.64 / 0.60, `2^62-57` 0.93 / 0.84, `2^63-25` 1.24 / 1.11, Goldilocks 1.87 / 1.83, `2^64-59` 1.81 / 1.99; the AVX2 build is within 0.1 of each cell except `2^62-57` static (1.03) and `2^64-59` dynamic (1.67). REDC's correction `r - (r < h || r >= mod ? mod : 0)` is taken with probability about `mod / 2^64`: predictable for small moduli, a coin flip near `2^64`, and on this CPU's fast 64-bit division the mispredictions cost more than the division saves, so the types use Montgomery only below `2^62`; `2^63-25` is a wash at long exponents (0.96–0.97 at 2^128-1) and a loss elsewhere. Along the exponent axis for 998244353 (scalar / AVX2): 1.04 / 1.00 at exponent 2, 0.85 / 0.88 at 3, 0.88 / 0.85 at 4, 0.75 / 0.77 at 5; for `2^62-57`: 0.99 / 1.06 at 3, 1.13 / 1.10 at 4, 0.89 / 0.94 at 5, 0.85 / 0.85 at 7. The threshold is 5, the first exponent at which every modulus below the bound gains in the candidates; in the type itself the 62-bit class still measures 1.08 of its canonical loop at exponents 5–6 and 0.94 at 7, the 30–33-bit moduli 0.89–0.92 at 5. Exponents 0–4 cost the same as the canonical loop in the static types (0.92–1.05 of the raw loop); the dynamic types' canonical path runs 1.1–1.3x the raw loop at exponents 1–4 and 1.0–1.3x for moduli from `2^63` upward, the cost of their per-multiply modulus checks. Transient cells of this run, not used: AVX2 static `2^62-57` at 65535 (1.54), `2^63-25` at 512 (4.26) and p64 at 65537 (2.52).

The 32-bit `DynModInt<>` construction from `int` (2.5 ns each) is bounded by the function-local reducer's initialization guard plus one Barrett step; a `constinit` reducer would need a constexpr Barrett constructor, which belongs to P004, and the guard is the same cost `*=` already pays.

### Optimality gap

Speed sweep of 2026-11-16 (`/speed-sweep P005`, scout) over the eleven Library Checker problems whose judge sources include this header; all eleven are linear-algebra solutions on `10-matrix.hpp` (P020, planned), so each cause below names its owner. Judge side: our submissions #388710–#388721 of 2026-07-27 (and the listed best of each problem), the judge's `g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`; the standing is the rank among the 30 fastest distinct users (`J fastest`, similar sources merged). Local side: i9-11900H, GCC 16.2.1 with the same flags, header commit 0c57e21, tests regenerated from `library-checker-problems` 73876f1 (2026-10-25), the minimum of three wall times per case (`J reproduce … --label ours`, every case AC). **The host was not idle**: other sessions ran stress tests and compiles (load average 8–9 of 16 threads, CPU near 96 °C), and the leaders' local times came out 1.1–2.9x their judge times where an idle host gives about 0.8. The local columns are therefore upper bounds whose ratios hold only within about 2x; the judge columns and the interleaved measurement below are sound, and the first `verify` run replaces the local columns on an idle host.

Two baselines differ from the tree. The judge columns of 2026-07-27 were made with a matrix header of about 1,350 lines (`lazyGauss`, `binaryRank`, `naiveMont`, a nonzero-counting product, a `MOD == 2` shortcut, a one-pass `solveLin` with a kernel basis) that exists in no commit and no archive: its only copies are our cached submissions `96-Local Testing/.judge-cache/sources/388710.cpp`–`388721.cpp`. The tree's `10-matrix.hpp` equals `OLD/1-Core/07-matrix.hpp` up to `inline` keywords and matches our 2026-07-01 submissions #383376–#383386, whose judge maxima are the third column. And the modint types those submissions used are the archived `OLD/1-Core/03-modint.hpp` and `04-dynmodint.hpp`, not this header.

| Problem | Ours judge 07-27 (listed rank of 30) | Tree-equivalent matrix header, judge 07-01 | #1 judge | max: ours local / judge, #1 local / judge | sum: ours local / judge, #1 local / judge | Cases with a judge gap above 1.5 |
|---|---|---|---|---|---|---|
| matrix_product | 218 (#388710; listed 214, #20) | 256 | 49 (408926) | 539 / 218, 135 / 49 | 2578 / 1121, 591 / 256 | random 5.8–7.3, max_random 4.3–4.5, small 2–12 on 2–12 ms |
| pow_of_matrix | 115 (#388711, #14) | 229 | 11 (401296) | 384 / 115, 26 / 11 | 7035 / 2368, 402 / 230 | every dense case 10.7–23 (104–115 ms against 7–11); perm_max_random 2.3–2.7; nontrivial_frobenius_form 2.8–16.4 |
| matrix_det | 32 (#388712; listed 24 of 07-01, #6) | 24 | 9 (405307) | 59 / 32, 21 / 9 | 692 / 304, 254 / 129 | max_random 2.4–3.1, lowrank_max_random 2.6–4.0, perm_max_random 4.0–5.5, random 2–3 |
| matrix_det_arbitrary_mod | 84 (#388713; listed 82, #8) | 132 | 49 (361230) | 474 / 84, 75 / 49 | 2495 / 495, 402 / 260 | max_random 1.7–1.8, perm_max_random 5–10 (10–11 ms against 1–2), random 1.7–2.0 |
| matrix_rank | 28 (#388714; listed 26 of 07-01, #5) | 26 | 10 (405300) | 61 / 28, 21 / 10 | 754 / 354, 210 / 119 | max 2.9–14, random 2.0–4.7, lowrank 2–7, zero 4, hack_of_system_of_linear 2–3 |
| system_of_linear_equations | 25 (#388715, #5) | 51 | 11 (406222) | 126 / 25, 21 / 11 | 1009 / 276, 223 / 140 | max 2.2–2.5, lowrank 2.8–6, random 2–5, hack 2–4, fullrank 2–3 |
| inverse_matrix | 58 (#388716, #5) | 72 | 16 (408951) | 204 / 58, 36 / 16 | 1662 / 553, 337 / 165 | max_random 3.6–3.9, lowrank_max_random 2.1–3.8, perm_max_random 3.8–4.8, random 2.5–6.3 |
| adjugate_matrix | 67 (#388721; listed 57, #3) | 125 | 34 (401318) | 317 / 67, 80 / 34 | 4415 / 1389, 1539 / 940 | max_random 1.8, lowrank_max_random 1.5–2.0, perm and almost_perm 2.0–3.3, almost_identity 2.6, nontrivial_frobenius_form 1.7–3.2 |
| characteristic_polynomial | 110 (#388718, #5) | 187 | 32 (404541) | 309 / 110, 92 / 32 | 3733 / 1621, 1337 / 556 | max_random 4.2–4.4, max_det_zero_random 4.1–4.3, nontrivial_frobenius_form 2.7–4.2, max_zero 4.0 |
| hafnian_of_matrix | 616 (#388719; listed 605, #2) | 970 | 540 (118101, Rust, not built) | 1534 / 616, best built 279353 741 / 678 | 6217 / 2494, 2981 / 2787 | none (max 1.12–1.14, large 1.2–1.25) |
| intersection_of_f2_vector_spaces | 1223 (#388720; listed 1219, #29) | 4818 | 152 (407091) | 6742 / 1223, 198 / 152 | 70039 / 14491, 2009 / 1560 | max_random 13.6–14.4, max_minus_1_random 7.6–7.8, random 6.9–7.1, nmk_all 7.4 |

The `example` rows at 11–12 ms against 0–1 are judge jitter on our side (ours 1–2 ms locally).

Leaders (`96-Local Testing/.judge-cache/sources/<id>.<ext>`; 35 of the 41 C++ sources among the top fives were rerun under bwrap with the user's yes of 2026-11-16; 353173, 109538, 186162, 179498, 262363 and 406229 do not build with GCC 16 (`assert` undeclared), and the Rust and Nim entries were read only). adamant's cp-algorithms-aux holds #1 on eight problems, directly or through Rohan_Kapri's copies.

| Leader (judge ms) | Kernel | Exact |
|---|---|---|
| 408926 product 49; 406211 product 69; 400787 product 82; 407076 product 50 | Winograd-form Strassen on packed `uint32` Montgomery data with 64×64 AVX2 leaves, an mmap arena and an mmap'd SIMD token parser (407076 and 400787 are independent Strassen codes with 4×8 and 64×64 leaves) | yes; odd modulus below 2^30 |
| 401296 pow 11; 353173 pow 18; 404541 charpoly 32 | Frobenius form from Krylov blocks, then x^k modulo each block polynomial, or the product of the block polynomials | random start vectors; the result is exact as read, its independence of the random choice not proven here |
| 346349 pow 56 | binary powering over a tiled AVX2 product, 64-bit lanes shrunk against 8·mod² | yes |
| 405307 det 9; 405300 rank 10; 406222 system 11 | 32-row blocked elimination, eight pivots applied to two rows per pass in `u64x4` lanes with one reduction per eight products; rank transposes when n > m; the system is one elimination of the augmented matrix | yes: eight products stay below 16·mod² < 2^64 for a modulus below 2^30 |
| 412918 rank 12; 412934 system 12; 412923 pow 17; 412902 adjugate 50; 412905 charpoly 46 | sounansya's library: eight-pivot batches with a Montgomery reduction of the lazy sum on a `uint32` work array; batch size fixed at compile time from the modulus (the power, adjugate and charpoly routines were not read) | yes for rank and system (bound in the source) |
| 411369 det 15; 411399 charpoly 36 | lazy `u64` rows shrunk below 8·P² every eight updates; a deterministic Hessenberg reduction whose column update is one lazy dot product per row | yes; P odd and below 2^30 |
| 361510 det 15 | plain Gaussian elimination with `%` by a constant, compiler-vectorised under `Ofast` | yes |
| 361230 det arbitrary 49; 109538 49; 215686 77 | Euclid on row pairs with one fused Barrett reduction of `t + q·s` per entry, rows swapped by pointer, a zero quotient skipped; mmap and an 8-digit SWAR parse (361230) | yes, on the whole input range |
| 408951 inverse 16; 406224 inverse 18 | Schur-complement block inversion on Strassen products from n ≥ 112 over an LU base with Shoup multiplication; fixed-width output | yes |
| 314161 inverse 52 | Gauss–Jordan with AVX2 Montgomery rows, `cin` | yes |
| 401318 adjugate 34 | borders the matrix with a random row and column, inverts the (n+1)×(n+1) matrix and reads the adjugate from it | no: Monte Carlo without a check |
| 118101 hafnian 540; 216351 608; 279353 678; 284818 683 | cycle DP over pair subsets with subset convolutions (Rust, scalar `uint32`); subset DP with `%`; 128-bit accumulation | yes |
| 406228 hafnian 606 | a copy of our own 2026-07-27 source | yes |
| 407091 F2 152; 412915 176; 406229 230; 205128 318; 204633 391 | an XOR basis on 64-bit words, the vector in the high half and its combination in the low half, reduced by `min(z, z ^ b)`, an early exit at dimension 30; packed XOR-basis libraries | yes |
| 404133, 404064, 404138, 404140, 404134, 404070, 404135 (toomer, Rust, 9–72) and 401306, 401308, 401312, 401309, 401313 (kemuniku, Nim, 16–59) | Strassen with AVX2 and AVX-512 leaves, a Frobenius power, AVX2 elimination; read by names and doc strings only | not read |

Measured here (2026-11-16, interleaved, three runs each, `perf stat`, user mode): the `matrix_det_arbitrary_mod` driver built with the judge's flags against the archived `OLD/1-Core/04-dynmodint.hpp` and `07-matrix.hpp`, and against this header and `10-matrix.hpp` (the same matrix code), on `max_random_00` (n = 494, modulus 156648748), identical output:

| Build | Task clock ms | Instructions | Branches | Branch misses |
|---|---|---|---|---|
| archived `DynModInt` | 125–146 | 1.729 G | 171.8 M | 0.38–0.45 M |
| this header's `DynModInt<0>` | 359–390 | 1.960 G | 254.2 M | 21.50–21.51 M |

97.7% of the misses fall in `FastGaussian::elimRow<DynModInt<0>>`, whose loop is `t[j] -= s[j] * a` (`10-matrix.hpp`, the scalar branch every large test takes, since all five large moduli are even). GCC 16 compiles `operator-=` (`n = n < a.n ? mod() - (a.n - n) : n - a.n`) to a compare, a conditional jump and an add of `MOD` from memory, and the borrow is a coin flip on random residues: about 21 M misses over about 40 M entries. Each product also tests the reducer's initialization guard and its mask flag (predictable, and the Barrett correction itself is a conditional move). The archived type's subtraction and addition were branch-free.

Causes, ranked by owner and by the time they explain:

Modint header (P005):

1. The dynamic types' `+=` and `-=` compile to data-dependent branches (measured above): 2.7x on `matrix_det_arbitrary_mod` `max_random` against the archived type under the same matrix code, the whole difference between the tree and its 07-01 judge time of 127–132 ms per case (tree 450–474 ms locally under load). The static types show no such loss on the same problems (`matrix_det`, `matrix_rank` and `characteristic_polynomial` run at the leaders' load factor), where the modulus is a constant; whether their scalar `+=`/`-=` also branch was not measured.
2. The dynamic reducer path: every product goes through the function-local `reduction()` with its guard and `Barrett32`'s mask test. The benchmark log already holds the cost: 4096 products with a runtime modulus take 9194 ns through `DynModInt<>` against 4307 ns through a raw `Barrett32` (scalar, 998244353, different operand distributions), about 2.1x. The paragraph above defers the `constinit` reducer to a constexpr Barrett constructor in P004; it was never rejected on a measurement.
3. `isPrime` at `setMod`: seven witnesses on 128-bit remainders even for a 32-bit modulus (the archived type used three on 64-bit products), and `setMod(m, 1)` runs it again inside an assertion. Once per case here, so negligible; once per query on `sqrt_mod` (candidate below). Unmeasured.
4. Not causes: the kernels need no new hook (`T::mod()`, the public `n`, `T::red` on a double-width dividend and a copyable `T::reduction()` let a fused update be written with the reducer held outside the loop); the asserting integral cast reached from `10-matrix.hpp` (`getMod`, the hafnian inner loops) is within noise and is the matrix header's to replace with `T::mod()` and `.n`; stream input shows no loss against the 07-01 runs; a cached 32-bit Montgomery context for odd dynamic moduli would save the per-row constants `10-matrix.hpp` recomputes, on a path no judge test reaches.

Matrix header (handed to P020, which gets its own sweep):

1. The 2026-07-27 header is missing from the tree: it explains pow 229 → 115, system 51 → 25, adjugate 125 → 57–68, charpoly 187 → 110, hafnian 970 → 616, F2 4818 → 1223, the arbitrary-modulus determinant 132 → 84, product 256 → 218 and inverse 72 → 58 on the judge. It reads `x.n` as `int&`, so it needs porting to the unsigned residue.
2. Power through the Frobenius form (104–115 → 8–11 ms), conditional on a proof or a deterministic vector choice.
3. Blocked elimination with lazy 64-bit accumulation, and LU with Strassen block inversion: determinant 22–24 → 8–9, rank 23–24 → 8–10, system 24–25 → 10–11, inverse 56–58 → 15–16, adjugate 56–57 → 32–34.
4. A Hessenberg reduction with contiguous lazy dot products (107–110 → 33–36); pointer row swaps, a fused reduction and an even-modulus vector path through `Barrett32::Multiplier` for the arbitrary modulus (82–84 → 47–49); packed words for F2 (1207–1223 → 84–152); Montgomery `uint32` Strassen leaves and one allocation for the product.

Judge sources (drivers, `97-Online Testing/src/01-Yosupo/01-Linear Algebra`): iostream token I/O costs about 9 ms per 250,000 input tokens and 6–8 ms per 250,000 output tokens (the perm cases: 10–12 ms against 1–3), by extrapolation roughly 100 of matrix_product's 218 ms; `11-intersectionf2.cpp` flushes with `endl` per test and fills bit by bit; `06-linearsystem.cpp` runs two eliminations (`solveLin`, then `ker`); every source stages the input in a `vector<vector<mint>>` before copying it into `Matrix`.

Targets. For this header, the interleaved build above is the measure: `DynModInt<0>` at or below the archived type's task clock on `matrix_det_arbitrary_mod` `max_random_00` with under 1 M branch misses, which is 127–132 ms per `max_random` case on the judge with the tree's matrix header; no modint-only change reaches a leader's time on any of the eleven problems. Per-case judge times of the best exact leader, for P020:

| Problem | Reference | Target per case (ours judge 07-27) |
|---|---|---|
| matrix_product | 408926 | max_random 48–49 (213–218); random 20–26 (132–146) |
| pow_of_matrix | 401296 (exactness to be proven), else 346349 | 6–11 on every n ≈ 200 case (104–115); 50–56 |
| matrix_det | 405307 | max_random 8–9 (22–24); lowrank_max_random 7–9 (21–32); perm_max_random 5 (11–12) |
| matrix_det_arbitrary_mod | 361230 | max_random 47–49 (80–84); perm_max_random 1–2 (10–11); random 3–4 (5–6) |
| matrix_rank | 405300 | max_03–05 10 (23–24); max_00–02 and max_06–08 4 (13–27); random 6 (20–28) |
| system_of_linear_equations | 406222 | max 11 (24–25); lowrank 5–11 (12–22) |
| inverse_matrix | 408951 | max_random 15–16 (56–58); lowrank_max_random 8–12 (25–30); perm_max_random 5–6 (17–19) |
| adjugate_matrix | 412902 and 401313 (exactness not read); not 401318 | max_random 32 (56–57); lowrank_max_random 23–30 (37–50); perm 6–7 (18–20); highrank stays ahead at 40–41 |
| characteristic_polynomial | 411399 | max_random 35 (108–110); max_det_zero_random 33–36 (107–108); nontrivial_frobenius_form 18–34 (45–109); max_zero 5 (12) |
| hafnian_of_matrix | 118101 | max 538–540 (604–616) |
| intersection_of_f2_vector_spaces | 407091 | max_random 84–90 (1207–1223); max_minus_1_random 149–152 (1156–1173); random 61 (427–432) |

Candidate problem without a judge source: `sqrt_mod` (`trySqrt` on `DynModInt`). #1 is 405759 at 13 ms (the code of 169592: eight queries per AVX2 vector with a 32-bit Montgomery modulus per lane, mmap input), then 33–35 ms (415767, 215966, 207878, 331256). `primality_test` has a source, on Mathematics' `isPrime`.

Verify runs (`/speed-sweep P005 verify` appends one row per problem):

| Date | Commit | Problem | Local max ms | Judge max ms | Rank | Best leader ms |
|---|---|---|---|---|---|---|

### Optimization check

Optimization check of 2026-11-16 (header commit 0c57e21, i9-11900H, GCC 16.2.1, idle host): speed the judge leaders do not show. **Nothing below is in the header**: each candidate is a prototype on a scratch copy, measured and awaiting the user's decision; none changes a signature, a domain, a result or a complexity, and all are scalar code.

Profile (`perf stat` and `perf record` on the eleven judge sources through `J reproduce … --label ours`, all 299 cases AC; idle local max ms: matrix_product 227, pow_of_matrix 170, matrix_det 24, matrix_det_arbitrary_mod 307, matrix_rank 27, system_of_linear_equations 51, inverse_matrix 89, adjugate_matrix 144, characteristic_polynomial 158, hafnian_of_matrix 991, intersection_of_f2_vector_spaces 4306). This header's share: the digit loop of `operator>>` takes 27.2% of matrix_product (`max_random_02`) and 11.2% of system_of_linear_equations; `elimRow<DynModInt<0>>` takes 80.4% of matrix_det_arbitrary_mod with 21.9 M branch misses (cause 1 above); 41% of hafnian_of_matrix is `__umodti3`, called by `10-matrix.hpp`'s `T(sum % mod)` on a runtime 128-bit modulus before this header reduces the value again. The static types' `+=` and `-=` compile to conditional moves, which answers cause 1's open question.

Method (`05-testing.md`, A/B rule): user-mode cycles, instructions and branch misses from perf_event_open, per operation over pools of 4096 random operands regenerated every repetition, the minimum of 15 repetitions in each of three interleaved passes, pinned to one core. Three builds: `-O2`, `-O2 -mavx2 -mbmi2 -mfma` and `-O2 -falign-functions=64 -falign-loops=32`. Two builds of the same header differ by at most 2%. The table gives `-O2` cycles per operation, baseline → candidate (ratio); the other two builds agree within 3% unless noted, and every candidate's output checksums equal the baseline's. With every candidate of the table and both P004 items enabled, this header's suite passed against a scratch copy of the tree (`05-modint_tester.py --mode full --seed 20261116`: six configurations per type with the sanitized builds, 39,164 and 60,658 oracle cases per 32-bit and 64-bit configuration). The two branch-free multiplier selects (`pow` below, `powMont` under P004) first failed that oracle on exponents of `2^64` and above, because they read the identity after the 128-bit loop had multiplied into it; corrected, they pass (the select in `pow` in quick mode), and the timings use smaller exponents. `mint` is `ModInt<998244353>`, `D32` and `D64` are `DynModInt<0>` and `DynModInt64<0>`, `p62 = 2^62-57`, `p64 = 2^64-59`.

| Kept candidate | Gain | Cost found |
|---|---|---|
| `operator>>`: 18 decimal digits per modular step (`chunk = 10 * chunk + digit` in a `ulng`, then `r = r * Value(p) + Value(chunk)`) instead of one product per digit | 9, 18 and 30 digits: `mint` 144.5 → 117.9 (0.82), 317 → 199 (0.63), 491 → 269 (0.55); `D32` 213.7 → 123.7 (0.58), 408 → 200 (0.49); `ModInt64<p64>` 233 → 119 (0.51), 516 → 199 (0.39); `ModInt61` 0.80; `D64` 287 → 115 (0.40), 30 digits 0.30 | none |
| Dynamic `-=` without a branch: `n = n - a.n + (mod() & -Word(n < a.n))`; `+=` and the static types unchanged | `D32` independent subtractions 13.85 → 2.30 (0.17, branch misses 0.49 → 0 per element), `x -= a * y` 22.6 → 6.08 (0.27), dependent chain 3.87 → 3.06 (0.79); `D64` 0.17 and 28.95 → 10.48 (0.36) | a dependent chain alternating `+=` and `-=` 2.77 → 3.19 (1.15), `D64` 1.16 |
| Static construction from 128-bit sources: one out-of-line `fold(hi, lo)` on the two halves (`lo % mod()` when `hi == 0`, else `((hi % mod()) * (2^64 mod mod()) + lo % mod()) % mod()`; 64-bit words reduce `(hi % mod()) << 64 \| lo` through `Context::red`), sign by a mask | `mint` unsigned 20.04 → 7.10 (0.35), values below `2^30` 10.04 → 4.07 (0.40), signed 26.47 → 8.85 (0.33); `ModInt<2^32-5>` 0.40; `ModInt64<998244353>` 0.50; `ModInt64<p62>` 0.57, signed 35.08 → 10.10 (0.29); `ModInt64<p64>` signed 0.45, unsigned 1.00; `ModInt61` 0.86–0.88 | modulus 7, whose 128-bit remainder GCC already expands inline: 5.90 → 10.45 (1.77) |
| `isPrime` at run time through `Montgomery64` (`powMont`), bases 2, 7, 61 below `2^32` and the seven bases above; constant evaluation unchanged | 30-bit primes 5496 → 1454 (0.26), 998244353 4467 → 1043 (0.23), 61-bit primes 11321 → 6980 (0.62), primes above `2^63` 1.01, random odd 30-bit 0.43; `setMod` on a 30-bit prime 5165 → 1469 (0.28); `setMod` plus one `sqrt` 6377 → 2748 (0.43) | a second code path beside the constant-evaluated one |
| `trySqrt` for `p ≡ 1 (mod 4)`: one power `w = a^((q-1)/2)` gives `x = a * w` and `t = x * w`, the order of `t` replaces the Euler test, and the non-residue search keeps `c = z^q` | `mint` (`s = 23`) 1598 → 1371 (0.86), `D32` 0.85–0.87, `ModInt64<p64>` 4354 → 1880 (0.43), `D64` 0.36–0.48; `p ≡ 3 (mod 4)` and the Montgomery-power moduli 1.00–1.04 | none |
| `tryInv` with unsigned alternating-sign cofactors in the word type instead of `lll` | `mint` 294 → 273 (0.93), `D32` 0.93, 64-bit words 0.92–0.95 (0.95–0.98 in the AVX2 build); instructions −32% to −36% | characteristic_polynomial's judge binary +7% under the host's `-march=native` (below) |
| `pow`: the exponent loop runs on a `ulng` once `b < 2^64` | random 64-bit exponents `mint` 1032 → 817 (0.79), 20-bit 0.79, exponents 0–4 0.90, `mod() - 2` 0.99; `ModInt61` 0.78; `ModInt64<p64>` 0.90; `D32` random 0.86 | `D32` with a fixed exponent 1.03–1.05 alone and 1.10–1.14 beside the other candidates (1.03 with the reducer change below) |
| Static 64-bit product: Möller–Granlund division by the compile-time reciprocal of the normalized modulus, and `a * b % M` in 64 bits when `M < 2^32` | `ModInt64<p62>` product 10.04 → 7.56 (0.75), chain 22.90 → 20.43 (0.89), `x += a * y` 0.82; `p64` 0.64, 0.81, 0.77–0.79, random `pow` 0.87; `2^63+29` 0.66 and 0.85; `ModInt64<998244353>` 10.04 → 3.05 (0.30), chain 0.52, `sqrt` 0.58–0.66 | the Montgomery `pow` dispatch (exponent 5, `2^62`) must be re-measured against it; fixed-exponent `pow` on large 64-bit moduli was bimodal (0.81–1.13) and is not counted |
| Static construction from signed sources of at most 32 bits: add a compile-time multiple of the modulus that is at least `2^31`, then one unsigned remainder | `mint` from mixed-sign `int` 15.03 → 3.03 (0.20, branch misses 0.48 → 0), non-negative 3.23 → 3.02 (0.93); `ModInt64` from `int` 0.68–0.80 | modulus 7, non-negative 1.13 |

Judge binaries (the judge's flags, slowest case, minimum of five runs, user-mode cycles in millions, identical outputs), baseline → all candidates above:

| Problem | Cycles | Source of the change |
|---|---|---|
| matrix_det_arbitrary_mod | 1172.5 → 432.6 (0.37); 452.6 with the `-=` alone | dynamic `-=`: task clock 260 → 101–106 ms, branch misses 21.90 M → 0.34 M, which meets cause 1's target |
| matrix_product | 770.8 → 716.8 (0.93) | `operator>>` (721.0 alone) |
| matrix_det, matrix_rank | 95.7 → 88.4, 95.9 → 88.8 (0.93) | `operator>>` |
| hafnian_of_matrix | 3743.9 → 3480.9 (0.93) | 128-bit `fold`; −3%, −7% and −6% under plain `-O2`, `-mavx2` and `-march=znver3` |
| system_of_linear_equations, inverse_matrix, adjugate_matrix, intersection_of_f2_vector_spaces | 0.97, 0.98, 0.99, 0.99 | `operator>>` |
| pow_of_matrix | 710.6 → 708.6 | none |
| characteristic_polynomial | 565.9 → 559.5 with `operator>>` alone, 607.9 with all | the `tryInv` change flips GCC's code for the matrix column loop under the host's `-march=native` (instructions −62 M, cycles +48 M); under `-march=znver3` and plain `-O2` baseline and candidate are equal (616.2 and 617.4, 833.5 and 833.3) |

These are instruction-path gains; none is an allocation, alignment or page-size change. The characteristic_polynomial row and hafnian's dependence on the flag set are code-layout effects that only the user's submissions settle.

Trade-offs, measured and left to the user:

- `pow` with a branch-free select of the multiplier: random exponents `mint` 0.58–0.72, `ModInt61` 0.63, `D32` 0.71–0.77; fixed exponents `ModInt<1000000007>` 1.05–1.08, `D32` 1.02–1.10, an even dynamic modulus 1.12–1.21, `ModInt61` `sqrt` 1.19–1.25.
- Construction from `lng` by a signed remainder and a mask: mixed signs 0.26–0.29, non-negative 1.21–1.44 (`mint` 3.27 → 3.95); the dynamic types with an xor fold: mixed 0.41 (`D32`) and 0.67 (`D64`), non-negative 1.00–1.18.
- Dynamic 64-bit product by Möller–Granlund division with three more statics set in `setMod`: product 0.83–0.85, chain 0.92–0.97, `pow` 0.85–0.91; by an inline `divq`: product 1.00, chain 0.92–1.00, small-exponent `pow` 0.72 (x86-64 assembly, so it needs a portable twin).
- Static `-=` as `min(d, d + M)`: independent 0.81, `x -= a * y` 0.77, chain 0.78, alternating chain 1.11–1.19, no judge binary changed.

Other owners' files, measured for their packages:

- P004: a `constexpr` `Barrett` constructor lets the dynamic reducer be a `constinit` static without the function-local guard (cause 2 above): `D32` product 4.64 → 4.11 (0.88), chain 0.88, random `pow` 0.93, small-exponent `pow` 0.85, `sqrt` 0.80, non-negative construction 0.65–0.80; matrix_det_arbitrary_mod 432.6 → 422.9 M cycles (instructions 2007 → 1760 M). A branch-free multiplier select in `Montgomery64::powMont`: random exponents 0.72–0.78, fixed 0.96–1.01.
- P020: `10-matrix.hpp`'s hafnian reduces its 128-bit sum with `% mod` before constructing `T`; dropping it removes `__umodti3`, but the loop's code is unstable under the host's `-march=native` (one scratch variant +31%), so it stays unmeasured until the matrix header is rebuilt.

Rejected: branch-free `+=` (dependent sum chain +9–12%, alternating +16–23%); three other spellings of the dynamic `-=` that GCC still compiles to a branch; an inline `divq` for the static 64-bit product (independent 1.00–1.09); the 128-bit fold inlined at the call site (hafnian +4–29%: GCC spills the caller's accumulator); a 32-bit Barrett chain for dynamic 128-bit sources (+39% on values with a small high half); an xor sign fold on static construction (non-negative +48%). Not tried: Cipolla for large `s`, a compile-time non-residue for static primes, reading the token straight from the stream buffer (string extraction and the sentry are 9.5% of matrix_product), a SWAR digit parse, the biased construction for dynamic moduli.

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

Python 3.10 execution (scripts parse under 3.10 grammar) was not run.

### Dependent-owner handoff

Matrix (P020/C07, existing-unverified): its optimized modular kernels derive the modulus through an `int` cast, assume particular 32-bit ranges, and its field classification includes a compile-time use of `is_prime`. C03's wider domains and dynamic prime flags do not establish Matrix support for those domains. C07 must review full-width/64-bit modular matrices and dynamic field classification before advertising them; reconstruct matrices and cached factors after dynamic modulus changes. Start at `FastGaussian::getMod`, `Matrix::isField/is_field`, and the `is_mint32` accelerated paths: derive moduli through `T::mod()` with correct word widths, enforce each kernel's domain, and replace static field constraints with a runtime contract for dynamic types. Reproducer: `using D = DynModInt<>; static_assert(Matrix<D>::is_field);`. Generic Matrix/Poly code may constrain on `ModularInt`/`StaticModularInt`.

Catalog items for other owners, reported, not changed: double factorial and inverse binomial caches (`05-Mathematics/05-combinatorics.hpp`), fast-IO `rd`/`wt` overloads for residues (`01-Core/19-fastio.hpp`, not adopted there: [19-fastio.md](19-fastio.md#limits-and-handoffs)), `randgen` hash base on `ModInt61` (Strings `randomBase`).

## Aliases

Search aliases (P263, grab --find): 模算术简介, 取模类, 自动取模, 乘法逆元, 模反元素, 快速幂, 二次剩余, 动态模数, 线性求逆元.

## History

- 2026-09-27: original P005 verification, full suites (35,811/55,323 oracle cases per configuration) and integration passed; first benchmark record.
- 2026-09-27: consolidation of the four full types into one header and one test entry, full suite passed.
- 2026-09-27: maintenance review, compile-time alias/width checks added, full suite and integration passed.
- 2026-10-07: re-audit, 6 findings fixed, `ModInt61` and the two concepts added, quick, full and stress passed on g++; benchmark record replaced.
- 2026-10-07 (run): quick, full (seed 20260927) and one stress round (288,532/446,488 oracle cases per 32/64-bit configuration) passed, benchmark (7,760 measurements) recorded, independent review found zero mismatches.
- 2026-10-08: restyle (one trailing comment removed), full suites passed on g++ and g++-14, integration passed.
- 2026-10-12: prune (`/prune P005`): `init`, the integral `operator=`, `operator!`, `modint_detail::MERSENNE61`/`fold61` and the `powerMontgomery` loop removed, `Context::canon` added, the 512 Montgomery threshold re-measured and retained; full suites on g++ and g++-14, the mini and poly suites, integration and the benchmark (8,192 measurements) passed.
- 2026-10-12 (cached contexts): `Context::montgomery` replaced `powerMontgomery`, power workloads moved to 4096 distinct bases, dispatch set to odd moduli below `2^62` from exponent 5, `canon` made branch-free; two independent reviews, 3 findings fixed; full suites on g++ and g++-14, the mini suites, poly quick, integration and the benchmark (11,808 measurements) passed.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, the 10 s quick bound, a raised limit (above); full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-16: speed sweep (scout), judge standing of the 2026-07-27 submissions #2–#8 on eight of the eleven linear-algebra problems and #20, #14 and #29 on matrix_product, pow_of_matrix and intersection_of_f2_vector_spaces (#1 at 49, 11 and 152 ms against 218, 115 and 1223); one modint cause, measured: the dynamic types' branching `+=`/`-=` cost 2.7x on matrix_det_arbitrary_mod against the archived type; every other gap is the matrix header's (the 2026-07-27 matrix header is not in the tree) or the drivers'; local columns taken on a loaded host; 55 leader sources fetched, 35 rerun (Optimality gap).
- 2026-11-16: optimization check (measure only; no header, tester or driver changed), eleven judge sources profiled and nine candidates kept on A/B counters in three builds, none adopted yet (Benchmarks, Optimization check): `operator>>` in 18-digit steps (0.30–0.82), a branch-free dynamic `-=` (matrix_det_arbitrary_mod 0.37), an out-of-line 128-bit fold (0.29–0.57 on typical moduli, hafnian 0.93), `isPrime` through `Montgomery64` (0.23–0.62), a one-power `trySqrt` (0.36–0.87), unsigned `tryInv` (0.92–0.95), a 64-bit `pow` loop (0.78–0.90), a static 64-bit product by precomputed reciprocal (0.30–0.89) and biased signed construction (0.20); four trade-offs left to the user, two items handed to P004 and one to P020, six forms rejected.
