# 05-combinatorics.hpp — evidence

`05-combinatorics.hpp` implements the Basic combinatorics scope of P012. The implementation uses ordinary contest arithmetic and the existing full Core modular integer types. It adds no Mathematics-specific reduction backend or ISA path.

## Contracts

### PrimeCombinatorics and ModFac

`PrimeCombinatorics<M>` accepts a full Core modular integer type whose modulus is prime and whose prime flag is set. `ModFac` remains an alias for `PrimeCombinatorics<mint>`; construction, `n`, `fac`, `inv_fac` and all four legacy query names are still available. Construction and `reset(n)` require `0 <= n < min(M::mod(), INT_MAX)`, so every factorial in the table is a unit. Factorial queries require an index `<= n`. Negative selection counts give 0. Counts of choices, parts and objects must be nonnegative (asserted).

| API | Meaning and bounds |
|---|---|
| `reset(n)` | Builds factorial, inverse-factorial and derangement tables through `n` in `O(n + log(p))` time. |
| `check()` | Asserts that the current modulus equals the saved one and still has the prime flag. Every query calls it. |
| `factorial(k)`, `inverseFactorial(k)`, `derangement(k)` | `O(1)` queries for `0 <= k <= n`; `D_0 = 1`, `D_1 = 0`. |
| `doubleFactorial(k)` | `k!! = k (k - 2) ...` for `0 <= k <= n`, with `0!! = 1!! = 1`: `2^j * j!` for `k = 2j` and `k! / (2^j * j!)` for `k = 2j + 1`. `O(log(2 + k))` for the power: of 2 for even `k`, of `(p + 1) / 2 = 1/2` for odd `k`, so no query inverts (modulus 2 only reaches `j = 0`, where `(p + 1) / 2 = 1` is the empty power). |
| `combiNR(a,b)`, `permuNR(a,b)` | Choose or order `b` distinct objects from `a >= 0`. Negative or excessive selections return 0; otherwise they require `a <= n`. |
| `circularPermutation(a,b)` | Arrangements of `b` of `a >= 0` distinct objects around a circle, rotations identified: `P(a, b) / b`, computed as `permuNR(a, b) * fac[b-1] * inv_fac[b]` for `1 <= b <= a`. It has `permuNR`'s zero rules and bound: negative or excessive `b` gives 0, and `b = 0` gives the one empty arrangement but, like `permuNR(a, 0)`, asserts `a <= n` (user decision 2026-11-14; before, `b = 0` returned 1 for any `a`). |
| `combiNonAdjacent(a,b)` | Subsets of `b` pairwise non-adjacent positions among `a >= 0` in a row (OI Wiki, 不相邻的排列): `C(a - b + 1, b)` by `combiNR`. Negative `b`, or `b > a - b + 1`, gives 0; otherwise `combiNR` asserts `a - b + 1 <= n` (so `b = 0` needs `a + 1 <= n`; at `a = INT_MAX`, `b = 0` the converted index wraps negative and `combiNR` asserts `a >= 0` instead). The bijection removes one gap after each chosen position but the last. |
| `combiNegative(a,b)` | The generalized binomial `C(-a, b) = (-1)^b * C(a + b - 1, b)` (GMP `mpz_bin_ui` convention) for `a >= 0`: `combiWR(a, b)` with the sign of `b`, so its zero rules and bound are those of `combiWR`. |
| `combiInverse(a,b)` | `1 / C(a, b) = inv_fac[a] * fac[b] * fac[a-b]` for `0 <= b <= a <= n` (asserted; outside that range `C` is 0 and has no inverse). |
| `combiWR(a,b)` | Unordered multisets of size `b`. `b = 0` gives 1 even when `a = 0`, and positive selections from zero choices give 0. Nontrivial queries require `a + b - 1 <= n`. |
| `permuWR(a,b)` | Ordered selections with replacement, `a^b`, for `a >= 0`. Negative `b` returns 0 and `0^0 = 1`. It does not depend on the table size and costs `O(log(2 + b))`. |
| `starsBars(total,parts,positive)` | Ordered parts summing to `total`: nonnegative parts by default, positive parts when `positive` is set. Zero parts give one empty solution exactly when `total = 0`. Uses the matching combination-table bound. |
| `catalan(k)` | Catalan number for `k >= 0` and `2 * k <= n`. |
| `ballot(a,b,strict)` | A/B sequences where every prefix has A >= B, or A > B for every nonempty prefix when `strict`. Both conventions count the empty sequence once. Strict mode removes the forced first A. Weak queries require `a + b <= n` when a solution is possible; strict queries require `a + b - 1 <= n`. |
| `multinomial(parts)` | Arrangements of the given nonnegative multiplicities, with sum at most `n`. The empty list gives 1. Costs `O(parts.size())`. |
| `combiLarge(a,b)` | `C(a, b) mod p` for any `ulng a` and `b <= n` (asserted). `b < 0` or `b > a` gives 0. Costs `O(b)` and uses only `inv_fac[b]`. |

The three tables use about `3 * (n + 1) * sizeof(M)` bytes. `fac` and `inv_fac` keep their capacity after a smaller reset, so stored memory is `O(peak n)`; `der` is rebuilt by `derangementTable` and holds exactly `n + 1` entries. All fields are read-only for callers; change them only through the documented APIs.

Every dynamic `setMod` invalidates all earlier table values, even one that sets the same modulus again. Call `reset` before any further query or direct field read. `check()` catches a changed modulus by assertion. Core has no generation tag, so it cannot detect a same-modulus reset or a change away and back. Rebuilding is the caller's precondition, not a promised runtime check. Copy and move keep the live table and its modulus requirement. A moved-from table may be discarded or reset before reuse.

Correctness: Factorials below a prime modulus are units. Inverting `n!` once and walking backward gives every inverse factorial; factorial ratios then count ordered and unordered selections. Pascal's identity and elementary enumeration independently check those identities. Stars-and-bars places separators between or around objects. The reflection argument gives weak ballot counts `C(a+b,b) - C(a+b,b-1)`; removing the forced first A gives strict counts. Catalan is the case `a=b`. Derangements use `D_n = (n-1) * (D_(n-1) + D_(n-2))`, independently tested by inclusion-exclusion and direct permutation enumeration.

`combiLarge(a, b)` computes `inv_fac[b] * a(a-1)...(a-b+1)` with each factor reduced mod `p`. The falling product divided by `b!` is the exact integer `C(a, b)`. Since `b <= n < p`, `b!` is a unit mod `p`, so reducing that identity mod `p` is exact for every `a`, including `a >= p`. This agrees with Lucas's theorem, which gives `C(a mod p, b)` for `b < p`. Pascal's rule `C(i, j) = C(i-1, j-1) + C(i-1, j)` and the derangement recurrence are polynomial identities over the integers, so they hold in every quotient ring.

### LogCombinatorics

`LogCombinatorics(n)` and `reset(n)` build `lf[k] = log(k!)` for `0 <= k <= n` as `long double`, asserting `0 <= n < INT_MAX`, in `O(n)` time and `16 * (n + 1)` bytes on x86-64 (capacity kept across a smaller reset). The table is a Kahan-compensated running sum of `std::log(i + 0.0L)` (user decision 2026-11-14, replacing one `lgammal` call per entry); it relies on IEEE evaluation without reassociation, which `-ffast-math` would break, and the library never sets that flag. `logFactorial(k)` asserts `0 <= k <= n`. `logBinomial(a, b)` and `logPermutation(a, b)` assert `a >= 0`; a negative or excessive `b` gives `-infinity` (the log of the count 0), and otherwise they assert `a <= n` and return `lf[a] - lf[b] - lf[a-b]` and `lf[a] - lf[a-b]`. The logs are natural; `std::exp` recovers counts up to about `1e4932`. Copies own independent tables; there is no modulus and no cache.

Accuracy: each `logl(i)` is within half an ulp, at most `2^-64 * log(i)`, and these errors total at most `2^-64 * log(k!)`; Kahan summation adds about 2 ulp of the sum, so every entry is within about 3 ulp, `3 * 2^-63 * log(k!)`. A result combines at most three entries whose logs total at most `2 * log(a!)`, plus two roundings, so its absolute error stays below `8 * 2^-63 * (1 + log(a!))`, about `1.3e-10` at `a = 1e7`; that is the relative error of the count after `exp`. The tester checks every entry through 120 and the large-table samples (the bound, its neighbour, 3000 and 3001 and 40 random indices, each with 12 selections) against 45-digit references (exact `ln(k!)` through 3000, Stirling's series beyond, validated against the exact logs at 300, 1000 and 3000) within that tolerance, on tables up to 1e6 in full mode and 3e6 in stress. The [benchmark](#benchmarks) compares every entry through `1e7` against the previous `lgammal` table. An uncompensated running sum, the sources' construction, drifts past that tolerance (first at `a = 6824`, independent review 2026-11-14). Other C libraries (MinGW on Codeforces) were not measured.

### logFactorialLarge, logBinomialLarge, logPermutationLarge

`logFactorialLarge(k)` returns `std::lgamma(k + 1.0L)` for any `lng k >= 0` (asserted), `logBinomialLarge(a, b)` and `logPermutationLarge(a, b)` the same differences as `LogCombinatorics` for any `lng a >= 0` (asserted), with `-infinity` for a negative or excessive `b`, in `O(1)` time and memory without a table (user decision 2026-11-14). `k + 1.0L` is exact through `2^63`, since the `long double` significand has 64 bits. The absolute error is a few ulp of `log(a!)` (the tester's tolerance, `8 * 2^-63 * (1 + log(a!))`, holds on every case through `2^63 - 1`), so the cancellation in a binomial costs relative accuracy only as the count shrinks against `a!`: about `2e-11` absolute at `a = 1e8`, `1e-8` at `1e10` and `2.5e-6` at `1e12` (independent review's scratch run, 2026-11-14). `lgammal` writes the global `signgam`, so concurrent calls are not thread safe; the library is single-threaded.

### primorial

`primorial<T>(n)` returns `n# = product of the primes <= n` (GMP `mpz_primorial_ui`, FLINT `fmpz_primorial`), with `0# = 1# = 1`, for `0 <= n < INT_MAX` (asserted by `SieveOfErath`). `T` is caller-passed and needs only `T(ulng)` and `*` in a commutative ring: `InfInt` or `InfIntMini` for the exact value, a modular type for `n# mod m`, wrapping `ulng` for `n# mod 2^64`. Primes from `SieveOfErath(n)` are packed greedily into `ulng` words, which are multiplied pairwise in a balanced tree, so every level multiplies operands of balanced size (subquadratic `T` multiplication pays off) over `b = O(n)` total bits (`log2(n#) ~ 1.44 n`). Time `O(n * log(log(3 + n)))` for the sieve plus `O(log(n))` levels of `T` products; memory `O(n)` bytes for the sieve plus the factors; [benchmark](#benchmarks) timings. A signed builtin `T` overflows once `n#` leaves its range: `lng` holds `n#` exactly through `n = 52`, and `primorial<lng>(53)` is signed overflow, which UBSan reports (independent review 2026-11-14), so signed builtins need `n#` to fit, as `fibonacciPair`'s contract says of its intermediates.

### multiFactorial

`multiFactorial<T>(n, m)` returns `n (n - m) (n - 2m) ...` over the positive terms in a caller-passed ring `T` with `T(ulng)` and `*=`, for `m >= 1` (asserted) and any `ulng n`, in `O(1 + n / m)` ring operations and `O(1)` memory (user decision 2026-11-14, GMP `mpz_mfac_uiui`): a modular type for `n!^(m) mod p` at any `m`, where `PrimeCombinatorics::doubleFactorial` covers only `m = 2` below its table bound, `InfInt` for the exact value, wrapping `ulng` for the value mod `2^64`. Signed builtins need the product to fit. Correctness: the loop multiplies exactly the terms `n - j m > 1`, and the omitted terms equal to 1 do not change the product. With `m = 0` and the assert compiled out, the step sets `n` to 0, so `multiFactorial` and `multiFactorialExact` stop after one factor instead of looping (`03-cpp.md`, Toolchain and judges).

### FieldCombinatorics

`FieldCombinatorics<F>(n)` and `reset(n)` build `fac` and `inv_fac` through `n` over a caller-passed field `F` with `F(int)`, `*` and `/`, in `O(n)` field operations and one division, asserting `0 <= n < INT_MAX`; the caller guarantees that `1, ..., n` are units (characteristic 0, as `Rational<T>`, or a prime above `n`, as a Core modular type). It mirrors Nyaan `Binomial_rational` (user decision 2026-11-14): `factorial(k)` and `inverseFactorial(k)` assert `0 <= k <= n`; `combiNR`, `combiWR`, `permuNR` and `multinomial` have `PrimeCombinatorics`' conventions (negative or excessive selections give `F(0)`, `combiWR(a, 0) = 1`, positive selections from zero choices give 0, the same asserted table bounds). There is no modulus to check: a dynamic modular `F` needs `reset` after any `setMod`, which this struct cannot detect. With `Rational<InfInt>` or `Rational<InfIntMini>`, entry `k` holds `k!` exactly, `O(k log k)` bits; `Rational<lll>` overflows (asserted by `Rational`) beyond `33!`. Memory: `2 * (n + 1)` values of `F`.

Correctness: as `PrimeCombinatorics`; every `F(i)` with `1 <= i <= n` is a unit, so `inv_fac[n] = 1 / n!` and the backward walk give every inverse factorial, and the factorial ratios are the integer counts mapped into `F`.

### fibonacciPair, lucas

`fibonacciPair<T>(n)` returns `(F_n, F_(n+1))`; `lucas<T>(n)` returns `L_n`, with `L_0 = 2` and `L_1 = 1`. They use iterative fast doubling: `O(log(2 + n))` ring operations, `O(1)` storage, and the full unsigned 64-bit index range. `T` only has to be a commutative ring (no division), so composite modular rings and the zero ring work as well as prime fields. Unsigned builtins of width at least 32 compute in their natural wraparound ring. Narrower builtin types are rejected by `static_assert`, because integer promotion could cause signed overflow. Signed builtin types require every intermediate to fit. Use the exact APIs for checked unsigned 64-bit results.

Correctness: Fast doubling maintains `(F_k,F_(k+1))` and applies `F_(2k)=F_k*(2*F_(k+1)-F_k)` and `F_(2k+1)=F_k^2+F_(k+1)^2`. Lucas follows from `L_k=2*F_(k+1)-F_k`. These are polynomial identities and need no inverses. Exact wrappers use unsigned 128-bit intermediates within their explicitly checked fitting-index range; independent matrix exponentiation verifies the full-width modular index domain.

### binomialTable and derangementTable

`binomialTable<T>(n)` returns the Pascal rows `C(i, 0..i)` for `0 <= i <= n`. `derangementTable<T>(n)` returns `D_0..D_n` by `D_i = (i - 1) * (D_(i-1) + D_(i-2))`. Both assert `n >= 0` and use only addition and multiplication, so they work in any ring `T`, including composite moduli (`DynModInt64` with the prime flag off), modulus 1 and wrapping `ulng`. Signed builtin `T` needs every intermediate to fit. Costs are `O(n^2)` time and memory for the binomial table and `O(n)` for derangements. They were added because Library Checker `montmort_number_mod` allows any modulus up to `10^9`, while `PrimeCombinatorics` needs a prime.

### Exact APIs

Every exact API accepts all `ulng` inputs. It returns `true` and writes the exact count when the count fits in `ulng`. `false` means the count exceeds `UINT64_MAX`, and `out` is left unchanged. Impossible selections are successful zero results. Empty selections and arrangements count once, including `0^0`. Inputs may alias `out`, because scalar inputs are copied before evaluation. Every function finishes within a constant number of loop steps (at most 64) before it completes or detects overflow.

| Exact API | Coverage and cost |
|---|---|
| `combiExact(a,b,out)` | Binomial, 0 for an impossible selection. At most 34 product/division steps before completion or overflow. |
| `permuExact(a,b,out)`, `factorialExact(n,out)` | Falling product and factorial with checked multiplication. At most 21 steps; factorial fits through 20. |
| `multiFactorialExact(n,m,out)`, `doubleFactorialExact(n,out)` | `n (n - m) (n - 2m) ...` over the positive terms (GMP `mpz_mfac_uiui`; `0` and `1` give 1) for `m >= 1` (asserted), with checked multiplication: every factor above 1 is at least 2, so at most 64 steps before completion or overflow, and the step `n > m ? n - m : 0` never wraps. `doubleFactorialExact` is `multiFactorialExact(n, 2, out)` and fits through 33 (`33!! = 6332659870762850625`). |
| `combiRepExact(a,b,out)`, `permuRepExact(a,b,out)` | Unordered and ordered selections with replacement, with the same empty-selection rules as the modular APIs. Uses a checked index sum or binary exponentiation (at most 64 steps). |
| `starsBarsExact(total,parts,out,positive)` | Nonnegative or positive parts, including zero parts. |
| `ballotExact(a,b,out,strict)`, `catalanExact(n,out)` | Same prefix and empty conventions as `PrimeCombinatorics::ballot`. The direct ballot recurrence avoids overflowing a binomial when the final ballot count still fits. At most 37 recurrence steps, each with gcd cancellation, costing `O(min(b, 37) * log(2 + a + b))`. Catalan fits through 36. |
| `combinatorics_detail::ratioProduct` | Positive `r * a * b / (c * d)` with an integral result. Cancels each denominator against the numerators before multiplying; false above `UINT64_MAX`. `O(log(max(r, a, b, c, d)))`. |
| `multinomialExact(parts,out)` | Full-width multiplicities, `O(k)` combination calls (each at most 34 steps) for `k` parts; the empty list gives 1. |
| `derangementExact(n,out)` | Exact through `D_20`; larger indices report overflow. |
| `fibonacciPairExact(n,out)` | Both values fit through index 92; larger indices report overflow. |
| `fibonacciExact(n,out)`, `lucasExact(n,out)` | Exact Fibonacci through 93 and Lucas through 92; larger indices report overflow. |

Correctness: Exact binomial recurrence carries a fitting unsigned 64-bit partial value and multiplies it by a factor at most `UINT64_MAX`; its unsigned 128-bit intermediate therefore cannot overflow. After each exact division, exceeding the result limit is conclusive because subsequent coefficients increase. Symmetry ensures that the smallest overflowing central case occurs by step 34. Exact falling products are nondecreasing through their used factors and overflow by the 21st step when that many nontrivial factors are requested. Checked exponentiation avoids an unused final square, so a fitting power is never rejected due to unused work.

With-replacement counts can require a mathematical index `a+b-1` above the unsigned 64-bit limit. In this case both complementary binomial selections are nonzero and the binomial is at least that index, so reporting overflow is exact. Similarly, if the running multinomial total overflows, it combines at least two positive groups and its count is at least the new total. No fitting answer is rejected merely because an intermediate index is large.

For exact ballot counts fix `d=a-b >= 0` and build `B_i = (d+1)/(d+i+1) * C(d+2*i,i)`. Then `B_i / B_(i-1) = (d+2*i)*(d+2*i-1)/(i*(d+i+1))`. The implementation cancels both denominator factors against the previous value and the two numerator factors before multiplying. All cancellation operands fit unsigned 128 bits even for full-width inputs. Integrality guarantees complete cancellation; the remaining products are tested against the result limit before multiplication. `B_i` is nondecreasing in `i` and is at least Catalan `C_i`, so an overflow cannot later disappear and at most 37 steps are needed. This includes Catalan 36, whose intermediate central binomial exceeds 64 bits but final count `11959798385860453492` fits. Strict ballot uses the equivalent weak instance `(a-1,b)` after handling empty/impossible cases.

Negative population or part counts, invalid table bounds, a composite factorial modulus and stale tables are precondition errors. Negative selection counts in the four preserved modular APIs are legitimate zero results. Exact APIs take counts as unsigned parameters, so they cannot be negative.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `fibonacci<T>(n)` | `fibonacciPair<T>(n).first` |

## Feature-to-test map

Runner: [`05-combinatorics_tester.py`](<../../96-Local Testing/05-Mathematics/05-combinatorics_tester.py>) driving [`05-combinatorics_tester.cpp`](<../../96-Local Testing/05-Mathematics/05-combinatorics_tester.cpp>). Oracles: exact Python integers, enumeration, inclusion-exclusion and matrix powers.

| Feature | Independent coverage |
|---|---|
| `factorial`, `inverseFactorial`, `derangement`, `factorialExact`, `derangementExact` | Python exact factorials and inverses; Pascal table; derangements by inclusion-exclusion plus permutation enumeration through 8. |
| `combiNR`, `combiWR`, `permuNR`, `permuWR`, `combiExact`, `combiRepExact`, `permuExact`, `permuRepExact` | Exact Python combinations, permutations and powers; empty and impossible selections; zero choices; unsigned index-sum overflow; every binomial threshold around rows 60–80. |
| `starsBars`, `ballot`, `catalan`, `starsBarsExact`, `ballotExact`, `catalanExact` | Explicit small composition and path enumeration; independent exact closed forms; empty conventions; strict and weak branches; the Catalan 36/37 boundary and the overflowing-binomial regression. |
| `multinomial`, `multinomialExact` | Python factorial ratios, empty and zero groups, full-width groups, sum and product overflow. |
| `doubleFactorial`, `doubleFactorialExact` | Ops `md` (every `k <= n` per prime setup) and `ex` (0..100, `2^64 - 2`, `2^64 - 1`) against Python `prod(range(k, 0, -2))`; aliased output. Probes `double-factorial-negative`, `double-factorial-bound`. |
| `combiNegative`, `combiInverse`, `circularPermutation`, `combiNonAdjacent` | Ops `mn`, `mi`, `mo` for `a <= 30` and `b` from -1 to 31 (also past `a`), and `mj` for `b` from -1 to `a + 2`, per prime setup: the generalized-binomial falling product `(-a)...(-a-b+1) / b!`, a modular inverse of `math.comb`, `P(a, b) / b`, which the driver first checks against enumerated rotation classes for `a <= 6`, and `C(a - b + 1, b)`, which it first checks against enumerated gap-free subsets for `a <= 10`. In `checkTable` on all six Core types (the `mint` default included), together with `doubleFactorial` against a running product: `combiInverse * combiNR == 1`, `circularPermutation * b == permuNR`, `combiNonAdjacent(a, b) == combiNR(a - b + 1, b)`, and the Pascal rule `C(-a, b) = C(-a-1, b) + C(-a-1, b-1)` for `combiNegative`. In the smoke test, `circularPermutation(100, 101) == 0`, `combiNonAdjacent(100, 51)` and `(INT_MAX, INT_MAX)` give 0 past a table of 20, and `combiNonAdjacent(19, 0)` and `(39, 20)` reach its bound. Probes for negative `a`, `b` outside `[0, a]`, table bounds, `circular-zero-bound` (`b = 0` past the table) and `nonadjacent-bound`. |
| `multiFactorial`, `multiFactorialExact` | Op `mu` per prime setup for 17 `(n, m)` pairs (`m` from 1 to above `n`, `n` and `m` near `2^64`, a random word) printing the modular value, wrapping `ulng`, `InfIntMini` and `InfInt` hex against Python `prod(range(n, 0, -m))`; op `eu` for `n` in 0..79 and six values near `2^32`, `2^63`, `2^64` with 16 to 18 steps `m` each, against a running Python product stopped above 64 bits; `checkTable` checks `multiFactorial<M>(k, 2) == doubleFactorial(k)` on all six Core types. Probes `multifactorial-zero-step`, `multifactorial-exact-zero-step`. |
| `FieldCombinatorics` | Ops `fs`, `ff`, `fc`, `fr`, `fp`, `fm` for prime setups `(2, 1)`, `(7, 6)`, `(998244353, 60)` and a prime near `2^64` with `n` 60/150/300 (quick/full/stress), each printing `Rational<InfIntMini>` (mini), `Rational<InfInt>` (full) and the modular type: every factorial and inverse factorial against Python `Fraction`, every `a <= n` with 12 selections `b` (edges and random) against `math.comb`, `math.perm` and multisets, 30 random multinomials against factorial ratios. In `checkTable` on all six Core types against the Pascal-checked `PrimeCombinatorics` tables, with copy and reset. 14 `field-*` probes for the constructor, every query's negative and bound cases. |
| `LogCombinatorics` | Ops `ls`, `lf`, `lb`, `lp` for tables 0, 1, 300 and 1e5/1e6/3e6 (quick/full/stress): every `a <= 120` with `b` from -1 to `a + 1`, then random and edge `a` (3000 and 3001 included) with `b` in {-1, 0, 1, 2, 40, a/2, a-1, a, a+1} and three random values. The reference is 45-digit `Decimal` logs: exact `ln(a!)` through 3000 and Stirling's series beyond, which the driver first checks against the exact logs. The tolerance is `8 * 2^-63 * (1 + log(a!))` on output printed with 21 digits, and results outside the table give exactly `-inf`. The smoke test covers copy, move and grow/shrink reset, and `logBinomial(10, 11)` and `logPermutation(10, 11)` past a table of 5 giving `-inf`; 8 probes. |
| `logFactorialLarge`, `logBinomialLarge`, `logPermutationLarge` | Ops `LF`, `LB`, `LP` for `a` in 0..40, 2999..3001, `10^4`, `10^6`, `10^9`, `10^12`, `10^15`, `10^18`, `2^62`, `2^63 - 2`, `2^63 - 1` and 20/200/2000 random magnitudes (quick/full/stress), with `b` in {-1, 0, 1, 2, a/3, a/2, a-1, a, a+1}, against the same 45-digit references and tolerance; 3 probes for negative `a`. |
| `primorial` | Op `pr` (n in {0, 1, 2, 3, 4, 5, 10, 30, 100, 1000} per prime setup, and 200 under every ring modulus) prints `InfIntMini` and `InfInt` hex, the modular value and wrapping `ulng`; op `pb` the `InfInt` value at 20,000 and 1e5/1e6/3e6, against a Python bytearray sieve's product. `lng` and empty products in the smoke test; probe `primorial-negative`. |
| `combiLarge` | Op `ml` against `math.comb(a, b) % p` for `a` in {0, 1, 2, n, p-1, p, p+1, 2p+3, 2^63, 2^64-1, random words} and `b` in {-1, 0, 1, 2, n/2, n, random}, restricted to `b <= n`, for every prime setup (2, 3, 7, 97, 998244353 and a prime near `2^64`). Probe `large-bound`. |
| `derangementTable`, `binomialTable` | Ops `rd` (n = 30) and `rb` (n = 12) over `DynModInt64` ring moduli 1, 2, 8, 1000, `2^63` and `2^64-1`, and op `ud` (wrapping `ulng`, n = 40), against Python exact values reduced mod the ring. Probes `table-negative` and `derangement-table-negative`. |
| `fibonacciPair`, `lucas`, `fibonacciPairExact`, `fibonacciExact`, `lucasExact` | Modular matrix powers, arbitrary 64-bit indices, static and dynamic prime, composite and modulus-one rings, unsigned 32/64-bit rings, exact 92/93/94 boundaries; a safe signed 32-bit regression and narrow-type compile rejection. |
| `PrimeCombinatorics` (`reset`, `check`), `ModFac`, types, state, preconditions | Static and dynamic 32/64-bit Core types, modulus 2 and a prime near `2^64`, copy/move/reset, same-modulus rebuild, changed-modulus assertions (a smaller and a larger new prime, and a `stale-<operation>` probe for every operation that calls `check`, at its early returns), the `ballot`/`starsBars` and exact defaults, `combiWR(0, b)` past the table, preserved aliased outputs, and 88 assertion probes. |

A compile-fail fixture checks, on every run, that narrow unsigned Fibonacci types are rejected. This is separate from the assertion probes.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 840 MB; optimized 0.2 s compile + 7.2 s run, checked 20.5 s compile + 2.8 s run, ASan-UBSan 22.7 s compile + 3.4 s run; MEMORY peak 1024 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 897 MB; optimized 0.2 s compile + 12.4 s run, checked 31.4 s compile + 3.6 s run, ASan-UBSan 35.1 s compile + 3.7 s run; MEMORY peak 1076 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 28.4 s compile + 14.9 s run, checked 27.8 s compile + 13.9 s run; MEMORY peak 847 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 836 MB; optimized 0.1 s compile + 4.7 s run, checked 7.5 s compile + 3.4 s run, ASan-UBSan 8.8 s compile + 4.0 s run; MEMORY peak 655 MB. Dated rows below stay those commands' latest results.

P012 MA02 run, 2026-11-14 (experiment `none`): GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. Every suite runs the shared runner's optimized `-O2 -DNDEBUG`, checked and ASan/UBSan configurations.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/05-Mathematics/05-combinatorics_tester.py' --mode full --seed 1` | PASS, 3 configurations, 127,275 Python cases each plus the C++ smoke checks and the narrow-ring compile rejection, 88 assertion probes; pre-flight quick peak 539 MB, `MEMORY peak=1618MB` |
| same with `CXX=g++-14` | PASS, same counts; pre-flight quick peak 1185 MB, `MEMORY peak=1663MB` |
| `... --mode stress --seed 1` | PASS, 477,650 cases, 88 probes; pre-flight quick peak 539 MB, `MEMORY peak=598MB` |
| `python3 '96-Local Testing/05-extraction.py' ops --filter 05-Mathematics/05-combinatorics.hpp` | PASS, 55 operations in 55 extractions on g++ and g++-14 (32 instantiating their default form), 0 failures; 20 mini-first findings listed as PENDING (the `M = mint` default, P258/P260) |
| `python3 '96-Local Testing/05-extraction.py' testers --filter 05-combinatorics_tester` | PASS, 36 names |
| `python3 '96-Local Testing/02-integration.py'` | PASS, 190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace; `MEMORY peak=2718MB` |

## Benchmarks

Conditions (2026-11-14, P012 MA02 `/finish`): Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, Linux x86-64; the perf counts on the otherwise idle machine, the latest driver run beside another session (below); the driver run's peak was 470 MB (three `long double` tables of `10^7 + 1` entries, 160 MB each: the reference, the timed table and the timed `lgammal` copy). The factorial tables and doubling algorithms have their stated bounds and no other performance claim is made.

`LogCombinatorics` (user decision 2026-11-14) replaced one `lgammal` call per entry with a Kahan-compensated sum of `logl(i)`. Cycle and instruction counts (`perf stat -r 3`, user space, `-falign-functions=64 -falign-loops=32`, the table rebuilt 200,000, 2,000 and 20 times):

| n | `lgammal` table cycles per entry | compensated sum cycles per entry | `lgammal` instructions | compensated instructions |
|---|---|---|---|---|
| 1,000 | 212 | 96 | 29.9G | 11.3G |
| 100,000 | 216 | 98 | 29.8G | 11.2G |
| 10,000,000 | 216 | 99 | 29.8G | 11.4G |

The [benchmark driver](<../../96-Local Testing/05-Mathematics/05-combinatorics_benchmark.py>) with its [C++ workload](<../../96-Local Testing/05-Mathematics/05-combinatorics_benchmark.cpp>) writes `96-Local Testing/05-Mathematics/05-combinatorics_benchmark.json`: one warmup and five samples, the construction timed with its allocation, and every entry of the compensated table compared with the `lgammal` table outside the timed region.

| Workload | n | median | note |
|---|---|---|---|
| `LogCombinatorics(n)` | 100,000 | 2.90 ms | 29.0 ns per entry; worst gap to `lgammal` 2.00 ulp |
| `lgammal` table | 100,000 | 6.05 ms | 60.5 ns per entry |
| `LogCombinatorics(n)` | 1,000,000 | 28.8 ms | 28.8 ns per entry; worst gap 2.00 ulp |
| `lgammal` table | 1,000,000 | 60.3 ms | 60.3 ns per entry |
| `LogCombinatorics(n)` | 10,000,000 | 288 ms | 28.8 ns per entry; worst gap 2.00 ulp |
| `lgammal` table | 10,000,000 | 597 ms | 59.7 ns per entry |
| `primorial<InfInt>(n)` | 100,000 | 1.03 ms | 35,954 hex digits, the count of a Python product's (the tester's `pb` op checks the value) |
| `primorial<InfInt>(n)` | 1,000,000 | 17.2 ms | 360,128 hex digits, the count of a Python product's |

This latest run had another session's sanitizer tests on other cores; the earlier run on the idle machine, before the driver recorded memory, measured 2.07, 21.9 and 218 ms against 4.51, 46.0 and 461 ms for the log tables and 0.81 and 14.0 ms for `primorial`, the same ratios. The worst gap is measured against `lgammal`, itself within about an ulp, and stays inside the 3-ulp argument under [LogCombinatorics](#logcombinatorics).

## Mutation testing

Record: mewt 4.0.0 on blob `50b9c58852eb59b4392912206ad14c50732afbce` (2026-11-14, seed 1, 3 lines changed since blob `156876d9c192`): 68 mutants, 63 caught (7 by timeout), 5 survived, 0 untested.

The changed lines are the final review's fixes: the multifactorial step stops at `m = 0` under `NDEBUG` (lines 202 and 278), and `FieldCombinatorics::multinomial` multiplies with `*` only (line 122). Stage seconds: quick 426, full on the optimized build 21, full on the checked and ASan-UBSan builds 146. Its 5 survivors, `AS 559`, `COS 1848`, `COS 1853`, `COS 1958` and `COS 1963`, are the multifactorial rows of the table below, renumbered.

The campaign before it, mewt 4.0.0 on blob `156876d9c1927ebf33084f002276a1349acf524d` (2026-11-14, seed 1, 71 lines changed since blob `016ed07d5765`): 576 mutants, 560 caught (7 by timeout), 16 survived, 0 untested, covered the lines the MA02 `/finish` changed: `doubleFactorial`, `circularPermutation`, `combiNonAdjacent`, `FieldCombinatorics`, the compensated `LogCombinatorics` table, the large log forms and `multiFactorial`. Stage seconds: quick 1565, full on the optimized build 113, full on the checked and ASan-UBSan builds 766. A first campaign on blob `3a255e711b1a` left `CR 280`, the removal of a redundant `assert(t <= n)` in `combiNonAdjacent`, which `combiNR` already asserts; the assert was deleted. Of the 16 survivors, 11 are equivalent and 5 untestable until P288, whose death probes will name the expected assertion. They follow the precedent of Fenwick's `LOS 302` (user decision 2026-11-12), as `AOS 612` below does.

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 1444`, `1 <= b` → `1 < b` | 54 | equivalent: at `b = 1` the factor `fac[0] * inv_fac[1]` is 1, so both return `permuNR(a, 1)` |
| `COS 1578`, `COS 1693`, `N < INT_MAX` → `!=` | 100, 133 | equivalent: `N` is an `int` |
| `COS 1588`, `i > 0` → `!=` | 103 | equivalent: `i` steps down by 1 from `n >= 0` |
| `COS 1644`, `a == 0` → `<=` | 113 | equivalent: `a >= 0` is asserted on the line above |
| `CR 327` (assert removed), `AOS 877`, `AOS 878`, `AOS 880`, `AOS 881` (`lng(a) + b - 1 <= n` → `lng(a) + b % 1`, `lng(a) - b - 1`, `lng(a) / b - 1`, `lng(a) % b - 1`) | 114 | untestable until P288: as `AOS 612` in `PrimeCombinatorics::combiWR`, every pair the weakened assert admits calls `combiNR(a + b - 1, b)`, which asserts `a + b - 1 <= n`, or, when the sum overflows an `int` and wraps negative in practice, `a + b - 1 >= 0`; both forms die at a glibc assert, at different expressions. The probe `field-replacement-bound` (`combiWR(INT_MAX, INT_MAX)`) reaches it and kill it once probes name line 114's expression. |
| `AOS 914`, `i + 0.0L` → `i - 0.0L` | 137 | equivalent: the same value |
| `AS 559`, `__builtin_mul_overflow(r, n, &r)` → `(n, r, &r)` | 202 | equivalent: commutative, and the builtin detects overflow of the exact product |
| `COS 1848`, `COS 1958` (`COS 1846`, `COS 1956` before), `n > 1` → `n >= 1` | 202, 278 | equivalent: one more factor 1, after which the step sets `n` to 0 (`1 > m` fails for every `m >= 1`, and `m = 0` past the assert sets it to 0 too) |
| `COS 1853`, `COS 1963` (`COS 1851`, `COS 1961` before), `n > m` → `n >= m` | 202, 278 | equivalent: at `n == m` both set `n` to 0 |

The survivors below are those of the whole-file campaign on blob `b715ba46521e99fb977c8b56ebde035079ebbf6d` (2026-11-14, seed 1, uniform sample of 1582): 500 mutants, 483 caught (6 by timeout), 17 survived, 0 untested; stage seconds (both runs together) quick 1144, full on the optimized build 101, full on the checked and ASan-UBSan builds 656. Its first run left 25 survivors; the `stale-*` probes for the modulus check, the smoke check `combiWR(0, INT_MAX) == 0` and the default-argument checks of `ballot`, `starsBars`, `ballotExact` and `starsBarsExact` closed 8. Their lines are unchanged since and are numbered as in the current header. The reviewer agreed that today's probes cannot tell `COS 1045` and `AOS 612`/`AOS 613` apart from the original.

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 1040`, `N < INT_MAX` → `!=` | 22 | equivalent: `N` is an `int` |
| `COS 1045`, `ulng(N) < M::mod()` → `!=` | 22 | untestable until P288 (as Fenwick's `LOS 302`, user decision 2026-11-12): `N == mod` fails both. For `N > mod` the mutant's table has `fac[n] = 0`, and `inv` asserts a unit (`01-Core/05-modint.hpp`, `assert(ok)`), so both abort at a glibc assert, at different expressions, which today's death probe cannot tell apart. Probe `n-above-mod` (`setMod(7)`, `N = 8`) reaches it and kills it once probes name the expected expression. Under `NDEBUG` both asserts are gone and the code is the same. |
| `COS 1136`, `b == 0` → `<=` | 44 | equivalent: `b < 0` returned on the line above |
| `AOS 612`, `AOS 613`, `lng(a) + b - 1 <= n` → `lng(a) / b - 1` / `lng(a) % b - 1` | 46 | untestable until P288 (as Fenwick's `LOS 302`): the line runs with `a, b >= 1`. Each pair the weakened assert admits calls `combiNR(a + b - 1, b)`. Where `a + b - 1` fits in an `int`, `combiNR` asserts it is at most `n`. Where it overflows, it wraps to a negative value in practice, and `combiNR` asserts it is nonnegative. Both forms therefore die at a glibc assert, and only a UBSan probe build, which the runner does not have, would see the overflow. Probe `replacement-bound` (`combiWR(INT_MAX, INT_MAX)`) reaches it and kills it once probes name line 46's expression. |
| `COS 1261`, `a == 0 && b == 0` → `a <= 0 && ...` | 73 | equivalent: `a >= 0` is asserted |
| `COS 1276`, `strict && a == b` → `a <= b` | 74 | equivalent: `a < b` returned in the same test, so `a >= b` there |
| `AS 452`, `min(b, a - b)` operands swapped | 187 | equivalent: symmetric |
| `AS 455`, `permuExact(n, n, out)` operands swapped | 198 | equivalent: identical operands |
| `COS 1441`, `a == 0` → `<=` | 207 | equivalent: unsigned |
| `AS 469`, `gcd` operands swapped | 230 | equivalent: symmetric |
| `COS 1461`, `assert(denominator == 1)` → `<= 1` | 231 | equivalent: every operand is positive (`k >= 1`, `delta + k + 1 >= 2`, numerators at least 1), so a division by a common gcd leaves the denominator at least 1 |
| `AS 471`, `AS 473`, numerator or denominator arguments of `ratioProduct` swapped | 246, 247 | equivalent: the gcd cancellation removes from each prime the same total exponent in any order, so the cancelled factors multiply to the same exact value; every factor is at least 1, so the running product passes `UINT64_MAX` exactly when that value does |
| `AS 479`, `__builtin_mul_overflow(r, c, &r)` → `(c, r, &r)` | 256 | equivalent: commutative, and the builtin detects overflow of the exact product |
| `IT 435`, word-full test → `true` | 286 | equivalent: each prime becomes its own leaf after a leading `T(1)`, so the product is the same; π(n) + 1 leaves instead of about π(n) / 3 keeps the O(log(n)) levels, the O(n) total bits per level and O(n) memory of the stated bound |
| `MR 1553`, `std::move(f.back())` → `f.back()` | 292 | equivalent: a copy instead of a move, same value |

## Sources

All pages below were retrieved and their cited sections read on 2026-09-27. Code is independently implemented from the mathematical identities, with no external code copied.

- [cp-algorithms: Binomial coefficients](https://cp-algorithms.com/combinatorics/binomial-coefficients.html), factorial/inverse-factorial method for a prime `p > n`, Pascal recurrence, and separation of prime-power/arbitrary-modulus/large-index methods.
- [OI Wiki: 排列组合](https://oi-wiki.org/math/combinatorics/combination/), selection with repetition, multinomial counting, and Pascal/binomial identities; independent Chinese-language comparison of counting conventions.
- 2026-10-07 sweep, for the added operations: [Nyaan binomial.hpp](https://nyaannyaan.github.io/library/modulo/binomial.hpp) (large-index `C` with a small lower index), [Nyaan binomial-table.hpp](https://nyaannyaan.github.io/library/modulo/binomial-table.hpp) (Pascal table without inverses), [ei1333 montmort.hpp](https://ei1333.github.io/library/math/combinatorics/montmort.hpp) (derangement table for any modulus), and the cp-algorithms binomial page above (Pascal triangle for arbitrary moduli). These were read for the operation lists only; the code was written independently.
- [cp-algorithms: Stars and bars](https://cp-algorithms.com/combinatorics/stars_and_bars.html), nonnegative and positive ordered parts and their shifted binomial indices.
- [OI Wiki: 卡特兰数](https://oi-wiki.org/math/combinatorics/catalan/), path interpretation, recurrence, and reflection/closed forms; displayed page update 2026-09-06.
- [Bertrand's ballot theorem](https://en.wikipedia.org/wiki/Bertrand%27s_ballot_theorem), strict-leading formula and the ties-allowed variant; mathematical identities compared with explicit short path enumeration.
- [OI Wiki: 错位排列](https://oi-wiki.org/math/combinatorics/derangement/), two-term derangement recurrence and inclusion-exclusion.
- MA02, 2026-11-14 (`@researcher`, and `/finish` for Nyaan `rational-binomial.hpp`), URLs in [00-sources.md](00-sources.md): GMP Number Theoretic Functions (`mpz_bin_ui` with negative `n`, `mpz_2fac_ui`, `mpz_mfac_uiui`, `mpz_primorial_ui`), FLINT `fmpz_primorial`, maspypy `modint_common.hpp` (`C_negative`, `C_inv`), Nyaan `float-binomial.hpp` and `rational-binomial.hpp` (`Binomial_rational`), hitonanode `big_combination_logarithm.hpp` and OI Wiki 排列组合 (圆排列, 不相邻的排列). Read for contracts only; code written independently.
- [cp-algorithms: Fibonacci numbers](https://cp-algorithms.com/algebra/fibonacci-numbers.html), matrix representation and the two fast-doubling identities. Lucas's companion identity is verified from its initial values and recurrence.

## Limits and handoffs

- `OLD/5-Mathematics/12-combinatorics.hpp` and the `ModFac` block in `OLD/[1] algorithms.cpp` are preserved unchanged. The legacy factorial, inverse-factorial and four counting operations are accounted for; `ModFac` and its exposed `n`, `fac` and `inv_fac` remain available. The old `combiWR(0,0)=0` is corrected to one empty multiset, construction rejects invalid prime-factorial domains, and wide index checks avoid signed overflow before indexing. Legacy method-based modular inverse/power syntax remains only in its archive.
- Factorial inversion at `n >= p` is invalid and rejected. Lucas-theorem (small prime, large index), prime-power and CRT composite-modulus binomials belong to `12-combinatorics_advanced.hpp`, together with general rising/falling factorials, Stirling/Bell/partition numbers, bounded partitions, q-binomials and combinatorial transforms. Fibonacci's companion **Lucas sequence** here is distinct from **Lucas's binomial theorem**.
- `PrimeCombinatorics` defaults to the full `mint` and asserts the full types' `is_prime` flag; the mini-first default belongs to P258 and P260, and `05-extraction.py ops` lists the row as pending until then.
- Inverse tables, CRT and primality/factorization are rows 09, 10 and 07.

## Aliases

Search aliases (P263, grab --find): binomial coefficient, nCr, derangement (Montmort number); negative binomial, inverse binomial, `C_inv` (maspypy), `C_negative` and `H` (maspypy, Nyaan: `combiWR`), log binomial, `logC`, `logfac`, `logP` (Nyaan), `lognCr` (hitonanode), double factorial, `mpz_2fac_ui`, primorial, `mpz_primorial_ui`, circular permutation, 圆排列, 环排列, 双阶乘, 素数阶乘; multifactorial, `mpz_mfac_uiui`, 多重阶乘; non-adjacent selection, 不相邻的排列; `Binomial_rational` (Nyaan), rational binomial, field factorial table; `lgamma`, large log binomial; Library Checker `montmort_number_mod`; 卢卡斯定理, 排列组合, 斐波那契数列, 错位排列, 卡特兰数, 组合数, 错排, 杨辉三角, 插板法, 隔板法, 卡塔兰数, 巴斯卡三角形.

## History

- 2026-09-27: initial P012 implementation and evidence.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, added `combiLarge`, `binomialTable` and `derangementTable`, 1 finding and 3 reviewer notes fixed; after `@reviewer`, full, stress (seed 1) and integration passed.
- 2026-10-12: prune, removed `fibonacci<T>` (`fibonacciExact` and the tests use `fibonacciPair<T>(n).first`); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: P012 MA02 added `LogCombinatorics`, `combiNegative`, `combiInverse`, `primorial` and, from the sweep, `doubleFactorial`, `doubleFactorialExact`, `circularPermutation`; 35 probes, 17 of them from the mutation and review rounds; independent review: 10 findings fixed (log accuracy oracle tightened to 45-digit references, early returns past the table, identity checks on every Core type; changed-lines campaign, no mutants); full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (14 equivalent, 3 untestable survivors).
- 2026-11-14: P012 MA02 `/finish` (user decisions): added `combiNonAdjacent`, `multiFactorial`, `multiFactorialExact` (`doubleFactorialExact` now wraps it), `logFactorialLarge`, `logBinomialLarge`, `logPermutationLarge` and `FieldCombinatorics`; `LogCombinatorics` builds by a compensated sum; `circularPermutation` composes `permuNR` and asserts its bound at `b = 0`; review fixes (`doubleFactorial` without an inverse, the signed `primorial` contract, unrecorded timings replaced by a benchmark); 88 probes; full on g++ and g++-14, stress, extraction and integration passed; changed-lines mutation campaign (11 equivalent and 5 untestable-until-P288 survivors, a redundant assert removed); benchmarks recorded (compensated log table in 45% of the `lgammal` table's cycles); final review: the multifactorial step stops at `m = 0` under `NDEBUG`, `FieldCombinatorics::multinomial` uses `*` only, the benchmark records memory; full on g++ and g++-14, stress, extraction, integration and a changed-lines campaign (5 equivalent survivors) passed again.
