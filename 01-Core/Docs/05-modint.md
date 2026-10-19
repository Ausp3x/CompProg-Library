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

Catalog items for other owners, reported, not changed: double factorial and inverse binomial caches (`05-Mathematics/05-combinatorics.hpp`), fast-IO `rd`/`wt` overloads for residues (`06-Miscellaneous/03-fastio.hpp`), `randgen` hash base on `ModInt61` (Strings `randomBase`).

## History

- 2026-09-27: original P005 verification, full suites (35,811/55,323 oracle cases per configuration) and integration passed; first benchmark record.
- 2026-09-27: consolidation of the four full types into one header and one test entry, full suite passed.
- 2026-09-27: maintenance review, compile-time alias/width checks added, full suite and integration passed.
- 2026-10-07: re-audit, 6 findings fixed, `ModInt61` and the two concepts added, quick, full and stress passed on g++; benchmark record replaced.
- 2026-10-07 (run): quick, full (seed 20260927) and one stress round (288,532/446,488 oracle cases per 32/64-bit configuration) passed, benchmark (7,760 measurements) recorded, independent review found zero mismatches.
- 2026-10-08: restyle (one trailing comment removed), full suites passed on g++ and g++-14, integration passed.
- 2026-10-12: prune (`/prune P005`): `init`, the integral `operator=`, `operator!`, `modint_detail::MERSENNE61`/`fold61` and the `powerMontgomery` loop removed, `Context::canon` added, the 512 Montgomery threshold re-measured and retained; full suites on g++ and g++-14, the mini and poly suites, integration and the benchmark (8,192 measurements) passed.
- 2026-10-12 (cached contexts): `Context::montgomery` replaced `powerMontgomery`, power workloads moved to 4096 distinct bases, dispatch set to odd moduli below `2^62` from exponent 5, `canon` made branch-free; two independent reviews, 3 findings fixed; full suites on g++ and g++-14, the mini suites, poly quick, integration and the benchmark (11,808 measurements) passed.
