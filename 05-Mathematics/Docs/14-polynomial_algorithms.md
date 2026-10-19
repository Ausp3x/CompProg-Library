# 14-polynomial_algorithms.hpp — evidence

`14-polynomial_algorithms.hpp` is the contest polynomial and formal-power-series family. Its functions are free and generic over a Core modint `M`, and polynomials are plain `vector<M>` with coefficient `i` of `x^i`. It covers four groups:

- Arithmetic, Euclidean algorithms (half-GCD, resultant, discriminant) and modular arithmetic.
- Dense and sparse power-series elementary functions.
- Compositions.
- Classic constructions: Taylor shift, product trees, the EGF product, cyclotomic polynomials, power sums and prefix sums.

It depends on Core `01-template.hpp` and `05-modint.hpp` and on `13-convolution.hpp`. It uses no Barrett/Montgomery and no ISA-specific code. Core `16-poly.hpp` owns the accelerated `Poly<T>` type. To avoid that name, this row's polynomial operations are `poly*` free functions rather than a struct, and every operation is free (03-cpp.md: Mathematics keeps free functions).

## Contracts

Common rules:

- **Polynomial vs FPS results.** A *polynomial* result is trimmed: no trailing zeros, and the zero polynomial is the empty vector. Inputs may carry trailing zeros. An *FPS* result (`fps*`, `polyTruncate`, `taylorShift`) has exactly the requested length `n`, zero-padded.
- **Domains.** "Any modint" means every Core `ModInt`/`ModInt64`/`DynModInt`/`DynModInt64`, including composite moduli. Products go through `convolution` (NTT for static NTT primes, CRT over one to six primes otherwise). "Prime field" means the modulus is prime, so every nonzero element is invertible. Operations that divide by `i` (log, exp, integral, power, factorials) need those `i` to be below the modulus; each contract states the bound.
- **Cutoffs.** `polynomial_detail::cut<M>({ntt, other})` selects the O(n^2) path up to a measured size. The pair holds one value for NTT primes and one for the other moduli (see Benchmarks). Both paths compute the same function, so the cutoff changes speed only.
- **Purity.** Every function returns a new value. Inputs are never mutated and never aliased with outputs. There are no caches.
- **Violated preconditions** are asserted. Under `NDEBUG`, `fpsPow`, `fpsSparsePow`, `polyDivmod`, `polyHalfGcd` and `fpsCompositionalInverse` return early with an unspecified result instead of looping or reading out of bounds.

### polyAdd, polySub, polyMul, polyDerivative, polyIntegral, polyEval, polyShift, polyTruncate, polyNormalize

- `polyAdd`, `polySub`, `polyMul`, `polyDerivative`: any modint; trimmed results.
- `polyIntegral(a)`: zero constant of integration; needs `|a| < mod` (asserted) and a prime modulus for the inverses.
- `polyEval(a, x)`: Horner.
- `polyShift(a, k)`: multiplies by `x^k` for `k >= 0`; for `k < 0` it drops the `-k` lowest coefficients (floor division by `x^-k`). This is the `x^k` shift; the Taylor shift is `taylorShift`.
- `polyTruncate(a, n)`: exactly `n >= 0` coefficients of `a mod x^n`.
- `polyNormalize(a)`: trims trailing zeros.

### polyDivmod, polyDivLinear

- `polyDivmod(a, b)` returns `(q, r)` with `a = b q + r` and `deg r < deg b`, both trimmed.
  - `b` must be nonzero after trimming (asserted), with a unit leading coefficient. Any modint satisfies this when `lc(b)` is a unit.
  - Long division is used when `min(deg a - deg b + 1, |b|) <= cut(DIV)`. Otherwise Newton: the reversed quotient is `rev(a) / rev(b) mod x^(deg a - deg b + 1)`, and the remainder comes from the low `|b| - 1` coefficients.
  - When `deg a < deg b` it returns `({}, a)`.
- `polyDivLinear(a, c)` is synthetic division by `x - c`. It returns `(q, a(c))` with `q` trimmed, and `({}, 0)` for the zero polynomial. Any modint, `O(n)`.

### polyGcd, PolyXgcd, polyXgcd, polyHalfGcd

Prime field.

- **Euclidean core.** `polynomial_detail::euclid` runs plain Euclid while the larger degree is at most `cut(GCD)`. Above that it alternates a half-GCD step and one division step, which gives `O(n log^2 n)`. The half-GCD recursion (`hgcd`) follows the cp-algorithms / Thull–Yap scheme with naive steps below `cut(HGCD)`. The naive tail of the extended form builds its own small transform matrix and composes it once, so it never multiplies small quotients into large matrices.
- **`polyGcd(a, b)`** returns the monic gcd, or empty when both inputs are zero.
- **`polyXgcd(a, b)`** returns `PolyXgcd{g, x, y}` with `x a + y b = g`, where `g` is monic.
  - The cofactors are those of the Euclidean remainder sequence, which is the sequence of the textbook extended Euclid. So `deg x < deg b - deg g` and `deg y < deg a - deg g` whenever those bounds are positive.
  - `b = 0` gives `(a / lc, 1 / lc, 0)`, and `a = 0` the mirror image.
  - Both zero gives all three empty.
- **`polyHalfGcd(a, b)`** requires `deg a > deg b` (asserted). It returns `{t00, t01, t10, t11}` with `(c, d) = (t00 a + t01 b, t10 a + t11 b)`.
  - `(c, d)` are the consecutive Euclidean remainders with `deg c >= ceil(deg a / 2) > deg d`.
  - The matrix is the product of the quotient steps `[[0, 1], [1, -q]]`, so it is unimodular.

### polyResultant, polyDiscriminant

Prime field.

- **`polyResultant(a, b)`** uses the convention `Res(a, b) = lc(a)^deg(b) * prod over the roots r of a of b(r)`, the Sylvester determinant. It is 0 when `a` or `b` is the zero polynomial; for constants, `Res(c, b) = c^deg(b)`.
  - The Euclidean core records `(deg q_i, lc q_i)` for every quotient, in order, through the half-GCD recursion as well. The remainder degrees and leading coefficients are recovered from them: `deg r_i = deg r_{i-1} - deg q_i` and `lc r_i = lc r_{i-1} / lc q_i`.
  - The formula is `Res(r0, r1) = (-1)^(d0 d1) lc(r1)^(d0 - d2) Res(r1, r2)`. The result is 0 when the last nonzero remainder has positive degree.
- **`polyDiscriminant(a)`** requires `deg a = n >= 1` (asserted). It returns `(-1)^(n (n - 1) / 2) / lc(a) * Res_{n, n-1}(a, a')`, with `a'` taken at formal degree `n - 1`.
  - When the characteristic divides `n`, `a'` loses degree and `Res_{n, n-1} = lc(a)^(n - 1 - deg a') Res(a, a')`.
  - It is 0 when `a' = 0`. For `n = 1` it is 1.

### polyPowMod, polyInverseMod, polyComposeMod

`m` (respectively `h`) must be nonzero with a unit leading coefficient, asserted after trimming. Above `cut(DIV)`, reductions use one precomputed inverse of `rev(m)` (detail `Reducer`).

- `polyPowMod(a, e, m)` returns `a^e mod m` with `e` an unsigned 64-bit exponent and `0^0 = 1`. A constant modulus gives the zero polynomial. Any modint. Cost `O(M(d) log e)`.
- `polyInverseMod(a, m)` requires `deg m >= 1` (asserted) and a prime field. It returns the inverse of `a` modulo `m`, of degree below `deg m`, or empty when `gcd(a, m) != 1`. Empty never collides with an answer, because an inverse is never zero when `deg m >= 1`.
- `polyComposeMod(f, g, h)` returns `f(g) mod h` by baby-step giant-step block Horner. With `k = |f|` and `d = deg h`, it costs `O(k d + sqrt(k) M(d))` scalar plus product operations. Any modint.

### fpsInverse, fpsDiv, fpsLog, fpsExp, fpsPow, fpsSqrt

Each returns exactly `n` coefficients, with `n >= 0` asserted.

- **`fpsInverse(a, n)`** requires `a[0]` to be a unit. Any modint.
  - It uses the O(n^2) recurrence up to `cut(INV)`.
  - Above it, Newton doubling: each step forms `a g = 1 + x^m t` with a length-`2m` cyclic product, whose wrap stays below `x^m`, then sets `g_hi = -(g t) mod x^m`.
- **`fpsDiv(f, g, n)`** is `f / g`.
- **`fpsLog(a, n)`** requires `a[0] = 1` and `n <= mod` (asserted), prime field. It computes `integral(a' / a)`, with the sparse recurrence up to `cut(LOG)`.
- **`fpsExp(a, n)`** requires `a[0] = 0` and `n <= mod`, prime field.
  - Up to `cut(EXP)` it uses the recurrence `i f_i = sum j a_j f_{i-j}`, which is `fpsSparseExp`.
  - Above it, Newton keeps `g = 1 / f` alongside `f`. A step from precision `m` to `2m`:
    1. Lift `g` to precision `m`.
    2. Form `r = f' - f a' mod x^(2m - 1)`, which vanishes below `x^(m - 1)`, from one length-`m` cyclic product.
    3. Correct `log f` by `x^(m-1) (g r / x^(m-1)) mod x^m`.
    4. Extend `f` by `f (a - log f) mod x^(2m)`.
  - That is five cyclic products per doubling.
- **`fpsPow(a, k, n)`** takes `k` in the signed 64-bit range, including `LLONG_MIN`.
  - Valuation rules: with valuation `v`, the result is 0 when `v k >= n`, and `x^(vk) c^k exp(k log(u))` otherwise. Here `a = x^v c u` with `u[0] = 1`, and `k` enters the exponent as a field element.
  - `0^0 = 1`, and the zero series to a positive power is zero.
  - `k < 0` requires `a[0] != 0` (asserted).
  - The remaining length `n - v k` must be at most `mod` (asserted).
  - For lengths up to `cut(POW)` it uses the recurrence `u f' = k u' f` (detail `powUnit`).
  - Without a modulus, `a^k` as a polynomial is `fpsPow(a, k, k deg a + 1)`.
- **`fpsPow(a, p, q, n)`** is the rational exponent `p / q`, with `q > 0` a unit (asserted).
  - It returns the empty vector, meaning no power-series answer, in these cases: the valuation times `p` is not divisible by `q` (no Puiseux exponent), or a negative exponent is applied to a positive valuation or to the zero series.
  - The lowest nonzero coefficient must be 1 (asserted). A `q`-th root of a general constant belongs to row 11, `kthRootModPrime`, and the caller can divide that constant out first.
  - The zero series gives zero for `p > 0` and 1 for `p = 0`.
  - The result is the branch with constant 1, the unique one whose constant is 1.
- **`fpsSqrt(a, n)`** requires an odd prime modulus (asserted).
  - Branch choice: of the two roots it returns the one whose lowest coefficient has the smaller `val()`. The other root is its negation.
  - It returns empty when the valuation is odd or the lowest coefficient is a nonresidue (no root exists); the zero series gives zero.
  - Up to `cut(SQRT)` (and `n <= mod`) it uses the sparse recurrence with `k = 1/2`.
  - Above it, Newton keeps `g` and `h = 1 / g` together, with two cyclic products for `g` and two for `h` per doubling. This path needs no `n <= mod`.
- For `n = 0` the result is empty. The sqrt and rational-power "no answer" result is empty too, so callers with `n = 0` know the answer is empty anyway.

### fpsSparseInverse, fpsSparseLog, fpsSparseExp, fpsSparsePow, fpsSparseSqrt

These run the O(n k) recurrences over the `k` nonzero coefficients below `x^n`, with the dense contracts.

- `fpsSparsePow` takes `k >= 0` (asserted).
- `fpsSparseSqrt` asserts `len <= mod`.
- They are also the dense functions' small-size paths, so for those sizes the identity tests below are the oracle.

### fpsComposition, fpsCompositionalInverse

- **`fpsComposition(f, g, n)`** returns exactly `n` coefficients of `f(g)`, for any `f` and `g`.
  - When `g[0] = 0`, only `f mod x^n` matters, so `f` is truncated first.
  - Block Horner with blocks of `floor(sqrt(k))` coefficients costs `O(k n + sqrt(k) M(n))`.
  - This is the scalar duplicate. Core Poly and row 52 own the `O(n log^2 n)` composition.
- **`fpsCompositionalInverse(f, n)`** requires `f[0] = 0`, `f[1] != 0` and a prime field.
  - It returns the unique `g` with `g[0] = 0` and `f(g) = x mod x^n`.
  - Newton `g <- g - (f(g) - x) / f'(g)` doubles the precision, at `O(n^2)` total.

### taylorShift, productTree, polyFromRoots, egfConvolution

- **`taylorShift(a, c)`** returns `a(x + c)` with the same length. Any modint.
  - For `n <= cut(SHIFT)`, `n > mod` or a composite modulus, it uses the `O(n^2)` synthetic scheme.
  - Otherwise (prime modulus, `n <= mod`) it uses one correlation with the factorials up to `(n - 1)!`.
- **`productTree(ps)`** returns the product of all polynomials by balanced halving over the index, trimmed. It returns `{1}` for an empty list and the zero polynomial if any factor is zero. Cost `O(N log N log k)` for total length `N`.
- **`polyFromRoots(rs)`** returns the monic `prod (x - r_i)`.
- **`egfConvolution(a, b)`** returns `c_k = sum C(k, i) a_i b_{k-i}`, computed by scaling with inverse factorials. It needs `|a| + |b| - 1 <= mod` (asserted) and returns empty when either input is empty.

### cyclotomicPolynomial, cyclotomicPolynomialMod

`n >= 1` (asserted).

- Method: compute `Phi_rad(n)` as the series `prod over d | rad of (1 - x^d)^mu(rad / d) mod x^(phi(rad) + 1)`, with `Phi_1 = x - 1` special-cased. Then substitute `x -> x^(n / rad)`.
- Cost `O(sqrt(n) + 2^w phi(n))`.
- `cyclotomicPolynomial` works in wrapping `ulng` arithmetic, which is a ring homomorphism from `Z`. It is exact whenever every true coefficient fits in `lng`. Overflow is not detected.
- `cyclotomicPolynomialMod<M>` reduces the coefficients modulo `M::mod()` for any modint, including composite moduli.

### powerSumsOfRoots, polyFromPowerSums, prefixSumOfPolynomial

- **`powerSumsOfRoots(a, n)`** requires `a` nonzero (asserted) with a unit leading coefficient.
  - It returns `p_k`, the sum of the `k`-th powers of the roots counted with multiplicity, for `k < n`, with `p_0 = deg a`.
  - Method: `sum_{k >= 1} p_k x^k = -x R' / R` for `R = rev(a)`.
- **`polyFromPowerSums(p)`** returns the monic polynomial of degree `d = |p| - 1` whose power sums are `p_1 .. p_d`; `p_0` is ignored. It requires `d < mod` (asserted), prime field.
  - Method: `rev = exp(-sum p_k x^k / k)`.
  - It inverts `powerSumsOfRoots` for degrees below the characteristic.
- **`prefixSumOfPolynomial(f)`** returns `g` with `g(x) = sum_{i < x} f(i)` for every `x`, `deg g = deg f + 1` and `g(0) = 0`. It requires `|f| < mod`, prime field.
  - Method: `D g = B(D) f`, with `B(z) = z / (e^z - 1)` from one series inverse, then one correlation with factorials and one integration.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `polyMod(a, b)` | `polyDivmod(a, b).second` |

## Feature-to-test map

Entry: [`14-polynomial_algorithms_tester.py`](<../../96-Local Testing/05-Mathematics/14-polynomial_algorithms_tester.py>), with C++ cases in `14-polynomial_algorithms_tester.cpp`.

Oracles:

- Schoolbook loops, truncated products and long division written in the tester.
- Uniqueness identities: `a g = 1`, `a b' = a'`, `f' = f a'`, `g^2 = a`, `r^q = a^p`, `f(g) = x`.
- Sylvester determinants by Gaussian elimination.
- Root-product formulas.
- The textbook extended Euclid.
- Newton's identities as a recurrence.
- The exact 128-bit identities `Phi_pm = Phi_m(x^p)` (when `p | m`) and `Phi_m(x^p) / Phi_m(x)` (otherwise).

Moduli: 998244353, 1e9+7, dynamic 1000003, the 64-bit NTT prime `29 * 2^57 + 1`, the small primes 3, 5, 7, 97 and 113, and the composite dynamic modulus `10^9` (`compositeSuite`: division with a unit leading coefficient, inverse across `cut(INV)`, `polyPowMod`, `polyComposeMod`, `taylorShift` and `cyclotomicPolynomialMod` against the same brute oracles).

| Operation | Tests |
|---|---|
| polyAdd, polySub, polyMul, polyDerivative, polyIntegral, polyEval, polyShift, polyTruncate, polyNormalize | `basicSuite`: 400 random cases per modulus (trailing zeros, negative shifts, `p = 7` with `len(a) < 7`) against coefficient loops; `polyIntegral` at `len(a) = p - 1` |
| polyDivmod | `divSuite`: identity, degree bound and long-division equality across the cutoff (`cut(DIV)` and `+1` neighbours) and up to 4e5 / 1.5e5 coefficients (optimized); trailing-zero divisor |
| polyDivLinear | `basicSuite`: identity, `r = a(c)`, zero polynomial |
| polyGcd, PolyXgcd, polyXgcd | `gcdSuite`: brute extended Euclid on shaped inputs (common factor, trailing zeros, degree 0, zero), threshold neighbours of `cut(GCD)`, 1000 small inputs over `p = 3` patterns, and a 6e4-degree run checked by identity, divisibility and cofactor degrees |
| polyHalfGcd | `gcdSuite`: transform and remainders equal to the brute remainder sequence stopped at `ceil(deg a / 2)`; large run: degree bounds and unimodularity |
| polyResultant | `gcdSuite`: Sylvester determinant (both orders, total degree up to 600), small inputs over `p = 5`, `prod b(r_i)` for 1500 roots, and above `cut(GCD)` (half-GCD quotient log): `prod b(r_i)` with `cut + 400` roots in both orders and a common-root zero (998244353 in every build, 1e9+7 optimized) |
| polyDiscriminant | `gcdSuite`: formal-degree Sylvester determinant (catches characteristic-dividing degrees over `p = 5`) and `prod (r_i - r_j)^2` with and without a repeated root, below and above `cut(GCD)` |
| polyPowMod, polyInverseMod, polyComposeMod | `modSuite`: repeated long division over degrees 0 to 400 across the reducer cutoff, 64-bit exponents, no-inverse inputs (multiples of `m`, zero), `0^0` and constant-modulus edges |
| fpsInverse, fpsDiv, fpsLog, fpsExp | `fpsSuite`: uniqueness identities with schoolbook checks to `n = 2049` and the cutoff neighbours, log(exp) round trips, transform-checked identities to `2^18` (optimized) or `2^20` (stress); `n = p` over `p = 7` |
| fpsPow | `powSuite`: binary powering with truncated schoolbook products (valuations, zero coefficients, 62-bit `k`, negative `k`, `LLONG_MIN`, cutoff neighbours, a large cube); rational exponents by `r^q = a^p`, branch and no-answer cases; `p = 7` and `p = 97` length edges |
| fpsSqrt | `sqrtSuite`: squaring, Euler-criterion existence, smaller-`val` branch, odd valuations, cutoff neighbours, large inputs; `p = 7` residue/nonresidue, branch `3` over `4`, and the Newton path at `n = 300, 1000 > p` |
| fpsSparseInverse, fpsSparseLog, fpsSparseExp, fpsSparsePow, fpsSparseSqrt | same suites (equality with the dense results, which use Newton above the cutoffs) plus sparse inputs of 3000 coefficients |
| fpsComposition | `compositionSuite`: Horner with truncated products, `g[0] != 0` and `= 0`, `n` up to 200, `len(f)` up to 220, empty inputs |
| fpsCompositionalInverse | `compositionSuite`: `f(g) = x` by Horner, `n = 0, 1` edges |
| taylorShift | `miscSuite`: `sum a_i (x + c)^i` by Horner across `cut(SHIFT)`, large evaluation identity `a(x + c)`; `p = 7` with `n > p`; `p = 113` (cutoff 112) at `n = 112, 113, 114`; composite modulus |
| productTree, polyFromRoots | `miscSuite`: sequential products (empty list, empty factors), `(1 + x)^n` binomials to 20000 factors |
| egfConvolution | `miscSuite`: Pascal-triangle brute; `p = 7` at `n = p` |
| cyclotomicPolynomial, cyclotomicPolynomialMod | `cyclotomicSuite`: every `n <= 3000` (`<= 400` quick) against the exact 128-bit identities, residues mod 1e9+7 and 7, and `n = 15015, 30030, 90090, 65536, 65537` (and 255255 in stress) |
| powerSumsOfRoots, polyFromPowerSums | `miscSuite`: Newton's identities (zero roots via low zero coefficients, non-monic), round trips, direct power sums of 3000 explicit roots |
| prefixSumOfPolynomial | `miscSuite` and `p = 7`: `g(x)` equals the running sum of `f(i)` for `x <= deg f + 2` |
| assertion probes | 28 checked-build probes: zero constants, non-unit `q`, long lengths over `p = 7`, even modulus, zero divisor or modulus, degree preconditions, negative sizes |

## Commands and results

P057 prune run, 2026-10-13: GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan+UBSan `-O1`.

```bash
python3 '96-Local Testing/05-Mathematics/14-polynomial_algorithms_tester.py' --mode full --seed 20261013             # PASS, 3 configurations, 28 probes: 2,810,627 / 2,809,236 / 2,809,236 checks; MEMORY peak=1084MB
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/14-polynomial_algorithms_tester.py' --mode full --seed 20261013  # PASS, 3 configurations, 28 probes: 2,810,627 / 2,809,236 / 2,809,236 checks; MEMORY peak=1108MB
python3 '96-Local Testing/02-integration.py'                                                                          # PASS: 135 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py' --braces '05-Mathematics/14-polynomial_algorithms.hpp' '96-Local Testing/05-Mathematics/14-polynomial_algorithms_tester.cpp'  # no violations
python3 '96-Local Testing/03-consistency.py'                                                                          # no errors
```

- Row 15, which includes this header, passed its full suite in the same run ([15-linear_recurrence.md](15-linear_recurrence.md#commands-and-results)).
- 2026-10-19 rerun after the row 13 change: full PASS ([13-convolution.md](13-convolution.md#commands-and-results)); counts not recorded.
- Pre-flight quick peaks were 949 MB (g++) and 876 MB (g++-14) under the 2048 MB cap, mostly the compiler on the `_GLIBCXX_DEBUG` build. Test programs peak below 100 MB.
- Mode coverage:
  - Quick uses small sizes and every suite.
  - Full adds:
    - schoolbook identity checks to `n = 2049`;
    - every cutoff neighbour (the large-modulus ones, `cut(GCD)` brute and `n = 4000` composites in the optimized build only);
    - the resultant above `cut(GCD)`;
    - transform-checked FPS identities at `2^18` (optimized) or `2^14` (checked builds);
    - cyclotomic polynomials for every `n <= 3000`.
  - Stress adds `2^20` FPS sizes and `Phi_255255`.
  - The checked builds use the smaller sizes so that each run stays below the runner's 180 s limit; it takes about 100 s unloaded.
- The headers build with `-Wall -Wextra -Wconversion` without warnings, alone and through `99-all.hpp`.

## Benchmarks

`python3 '96-Local Testing/05-Mathematics/14-polynomial_algorithms_benchmark.py' 5` (the driver covers both headers). Conditions:

- CPU: Intel i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2` (no `-march`), on an otherwise idle machine.
- One warmup and 5 timed repetitions; the tables show medians.
- The outputs of both compared paths are checked equal.
- Seed 20261010, uniform random inputs.
- Benchmark MEMORY peak was 478 MB.
- Threshold rows repeat each call `8192 / n` times (`4096 / m` for division) and report the total. Single runs vary by about ±15% between sessions.

Cutoffs (`polynomial_detail::cut`, `{NTT primes, other moduli}`). In the table, "naive" is the `O(n^2)` recurrence or long division and "fast" is the Newton or transform branch.

| Cutoff | Value | Evidence (998244353 naive / fast ms; 1e9+7 naive / fast ms) |
|---|---|---|
| `INV` | `{112, 1536}` | 96: 0.94 / 1.54, 128: 1.20 / 1.09; 1536: 13.8 / 14.3, 2048: 17.4 / 10.6 |
| `LOG` | `{112, 896}` | 96: 1.73 / 2.62, 128: 1.97 / 1.67; 768: 10.9 / 16.7, 1024: 16.7 / 14.0 |
| `EXP` | `{224, 2048}` | 192: 1.92 / 2.64, 256: 2.57 / 2.08; 1536: 12.9 / 22.3, 2048: 17.7 / 17.2 |
| `POW` | `{144, 256}` | 128: 3.17 / 3.38, 192: 5.15 / 4.44; for 1e9+7 log+exp beats the direct recurrence from 256 on (7.44 / 7.30) |
| `SQRT` | `{80, 768}` | 64: 1.85 / 1.95, 96: 2.96 / 2.58; 768: 21.4 / 21.2, 1024: 26.1 / 14.7 |
| `DIV` (`min(k, m)`) | `{256, 1536}` | `2m / m`: 256: 1.67 / 1.79, 320: 1.90 / 1.32; 1024: 7.28 / 8.01, 1536: 8.62 / 8.09 |
| `SHIFT` | `{40, 112}` | 32: 0.51 / 0.59, 48: 0.85 / 0.75; 96: 1.92 / 2.75, 128: 2.57 / 2.09 |
| `HGCD` (recursion base) | `{128, 512}` | gcd of degree 32768 (mint): base 32/64/128/256/512/1024 = 371/386/360/347/398/492 ms; degree 16384 (1e9+7): base 128 to 4096 = 464/435/409/364/434/571 ms. 128 and 512 sit in the flat optimum across sessions |
| `GCD` (Euclid below) | `{3072, 12288}` | Euclid against half-GCD from the top: mint 2048: 8.40 / 11.47, 3072: 17.96 / 19.29, 4096: 32.9 / 28.0; 1e9+7 8192: 108 / 146, 12288: 243 / 243, 16384: 482 / 392 |
| `BM` (row 15) | `{1280, 6144}` | Berlekamp–Massey against the half-GCD form: mint 1024: 2.40 / 2.74, 1536: 5.34 / 4.73; 1e9+7 4096: 31.1 / 37.0, 8192: 142 / 99.6 |

The Newton paths have power-of-two steps (for example `n = 160` costs the same as `n = 256`), so each cutoff is the measured crossover, rounded inside the first doubling where Newton wins.

Headline timings (998244353):

| Workload | ms |
|---|---|
| `fpsInverse` / `fpsLog` / `fpsExp` / `fpsSqrt`, n = 5e5 | 147 / 249 / 259 / 236 |
| `fpsPow` k = 1e18, n = 5e5 | 508 |
| `polyDivmod` 5e5 / 2.5e5 | 165 |
| `taylorShift` 5e5 | 92.7 |
| `polyFromRoots` 1e5 | 87.4 |
| `polyXgcd` / `polyResultant`, degree 5e4 | 680 / 497 |
| `polyPowMod` e = 1e18, modulus degree 1e5 | 3415 |
| `fpsComposition` / `fpsCompositionalInverse`, n = 8000 | 281 / 762 |

## Sources

[00-sources.md](00-sources.md), P057 sweep:

- cp-algorithms "Operations on polynomials and series" and the half-GCD article (adamant): divmod by reversal, Newton inverse/log/exp, half-GCD recursion and the resultant via Euclid.
- OI Wiki polynomial pages: Newton sqrt, power with valuation, composition.
- Nyaan `fps/` and maspypy `poly/` (sparse recurrences, Taylor shift, prefix sum of polynomial, composition and inverse), Library Checker `polynomial` problems, FLINT `nmod_poly` (power sums to polynomial, `compose_mod_brent_kung`).

Adopted by the P057 sweep: `fpsDiv`, `polyDivLinear`, `polyFromPowerSums`, `polyComposeMod` (Brent–Kung block Horner sharing the `fpsComposition` kernel) and `prefixSumOfPolynomial`.

The code was written independently.

## Limits and handoffs

- The dense Newton paths use `cyclicConvolution` from row 13, which takes three transforms per product and does not reuse transforms between products. Per doubling that is 6 length-`2m` transforms for the inverse and about 21 length-`m`-equivalent transforms for exp. Hand-tuned versions that reuse transforms need about 5 and 17. The constant-factor gap was not measured here; Core Poly owns the accelerated engine.
- `fpsComposition` and `fpsCompositionalInverse` are the scalar `O(n^2)`-type duplicates. The `O(n log^2 n)` Kinoshita–Li composition and power projection belong to Core Poly and row 52.
- `fpsPow` with a rational exponent requires lowest coefficient 1. General constants need a `q`-th root from row 11.
- `cyclotomicPolynomial` does not detect `lng` overflow (see the contract).
- The other-moduli cutoffs were measured before row 13's 2026-10-19 change (prime count chosen from the coefficient bound, aliased squares); re-measure them at the next P057 run.
- Rejected candidates are in [00-notes.md](00-notes.md), under P057 research decisions.

## History

- 2026-10-10: P057 initial implementation and verification, `@reviewer` found 4 defects and 2 minor issues, all fixed.
- 2026-10-11: verification run, quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `polyMod` (Legacy mapping; its cases are covered by the `divSuite` identity); full suite passed on g++ and g++-14.
- 2026-10-19: P054 follow-up rerun (included `13-convolution.hpp` changed), header unchanged; full passed.
