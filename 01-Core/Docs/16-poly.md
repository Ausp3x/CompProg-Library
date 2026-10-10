# 16-poly.hpp — evidence

`16-poly.hpp` provides `Poly<T>` (alias `FPS<T>`), the accelerated univariate polynomial and formal-power-series engine of C11 (package P025). It depends on `01-template.hpp` and `05-modint.hpp` only; big-integer coefficients are accepted through a structural `BigInt` concept (`07-infint.hpp` satisfies it without being included). Everything is a template or `inline`, so the header compiles alone, inside `99-all.hpp` and across translation units; the NTT and FFT root caches are function-local statics shared by all translation units. The header follows the type-header rule of `03-cpp.md`: every operation that takes a `Poly` is a `friend` defined inside `Poly<T>` and is found only through ADL (`exp(f, n)`, `gcd(a, b)`, `evalMulti(f, xs)`), operations that build a `Poly` or work on plain vectors are `static` members (`Poly<T>::interpolate(xs, ys)`, `Poly<T>::ntt(v)`), the seven type-independent entry points are `static` members of `PolyCompanion`, and kernels, caches and helpers live in `poly_detail`. The header has no public free function.

## Contracts

Conventions for every function: `v[i]` is the coefficient of `x^i`; `size()` counts stored terms (trailing zeros allowed); `deg()` ignores trailing zeros and is `-1` for zero; a precision argument `n >= 0` means "mod x^n" and the result has exactly `n` terms (empty for `n = 0`). Indices are zero-based and ranges half-open. Preconditions are `assert`s at entry; valid no-answer results are `bool` plus out-parameter or an empty polynomial as stated per function. Inputs are never mutated and results never alias inputs. `T` is a ring (`std::is_integral_v<T>` or `BigInt`: `int`, `lng`, `ulng`, `InfInt`) or a field (every other type: `ModInt`, `ModInt64`, `DynModInt`, the minis via `ModularInt`, `double`, `std::complex<double>`, a future `Rational`); field code uses `T(1) / c`, ring code exact `/`. Series operations marked "field" `static_assert` a field. Field operations that divide by `1..n` need characteristic `> n`; the finite-field routines need `T::is_prime`. `M(n)` is the product cost (`O(n log n)` for NTT, FFT and multi-prime paths, `O(n^1.585)` for Karatsuba). Caches: the NTT root table of `poly_detail::Ntt<T>` is keyed by `T::mod()` and rebuilt when a dynamic modulus changes (every earlier transform output is then stale); the FFT root table only grows; neither is thread-safe.

### API shape

Static members of `Poly<T>` (called as `Poly<T>::name`): `nttMaxLength`, `ntt`/`intt`/`transposedNtt`/`transposedIntt`/`nttDoubling` on `vector<T>`, `mixedRadix`, `rader`, `bluestein`, `multivariateCyclic`, `convolution2d`, `multivariate`, `interpolate`, `interpolateGeometric`, `interpolateIota`, `hermiteInterpolate`, `productOfGeometric`, `productOfArithmeticProgression`, `newtonToMonomial`, `factorialToMonomial`, `iotaPoints`, `shiftSamplingPoints`, `transposedEvalMulti`, `transposedInterpolate`, `cyclotomic`, `inv2d`, `linearRecurrenceKth`, `consecutiveTerms`, `findLinearRecurrence`, `bernoulliNumbers`, `partitionNumbers`, `stirling1Row`, `stirling2Row`, `eulerianRow`. `PolyCompanion` (non-template): `fft`, `ifft`, `convolutionFftMod`, `convolutionArbitraryMod` (vectors), `convolutionLng`, `convolution2p64`, `convolutionGF2k`; the three the dispatcher needs are implemented in `poly_detail` and wrapped. Every other operation, operators included, is a friend taking at least one `Poly` (also `productOfSequence` and `sumOfRationals`, whose `vector<Poly>` arguments make ADL find them, and `pade`/`rationalInterpolate`, whose `Poly &` out-parameters do). `poly_detail::powSparseUnit` is the shared recurrence of `powSparse` and `sqrtSparse`, not public API. `PolyModulus`, `SemiRelaxedMul` and `RelaxedMul` are separate structs declared after the companion. Members are grouped by family (convolution, division, series, transforms, evaluation and interpolation, algebra, factorization, composition, rational series, online products, famous series) with each family's static builders next to its friends, because a flat construction group of 35 builders would separate `ntt(vector)` from `ntt(Poly)` and `interpolate` from `evalMulti`.

### Poly<T>

Default (zero, no terms), `Poly(vector<T>)`, `Poly{...}`, `explicit Poly(n, x = 0)`. `size`, `deg`, `lead` (0 for zero), `coef(i)` (0 outside `[0, size)`), `operator[]` (asserts `0 <= i < size`), `isNil`. Mutators return `*this`: `trim`, `resize(n >= 0)`, `truncate(n >= 0)` (keeps at most `n` terms), `normalize` (monic; asserts nonzero; rings divide exactly), `reverse(n = -1)` (resizes to `n` first when `n >= 0`). `+=`, `-=`, `+`, `-`, unary `-` extend to the longer operand; `*= c`, `* c`, `c *`, `/= c`, `/ c`; `*=`/`*` call `convolution`; `/=`, `%=`, `/`, `%` call `divMod` (divisor nonzero); `<<= k`, `>>= k`, `<<`, `>>` shift by `x^k` with `k >= 0` (right shifts past the end give zero); `==` ignores trailing zeros; `<<` on a stream prints stored terms separated by spaces. `eval(a, x)` Horner; `deriv(a)`; `integ(a, c = 0)` (fields through one inversion of `n!`, rings exact coefficient division; the caller guarantees divisibility for rings).

### convolution

The dispatcher behind `*`: empty input gives empty output; `min(|a|, |b|) <= SCHOOLBOOK` (16 under `__AVX2__`, 32 otherwise) uses `schoolbook`; modular types use the NTT when `ceilPow2(|a| + |b| - 1) <= nttMaxLength` and the block product `convolutionLarge` while that padded length is at most `LARGE_RATIO` times the limit (16 under `__AVX2__`, 64 otherwise), otherwise (32-bit words, `mod < 2^30`, `|a| + |b| - 1 <= 2^20`, scalar build only) `convolutionFftMod`, otherwise `convolutionArbitraryMod`; `double`/`complex<double>` use the FFT; `int`/`lng` the exact three-prime reconstruction (true coefficients in `[-2^63, 2^63)`), `ulng` the exact mod-2^64 reconstruction, `BigInt` Kronecker substitution, every other ring `karatsuba`.

### schoolbook, karatsuba

Direct `O(n m)` product and Karatsuba (`O(n^1.585)`, any ring, direct below 40 terms); both return the full product. Signed integer coefficients (`int`, `lng`) accumulate in the unsigned twin (`poly_detail::Wrap`), so partial sums wrap and the result is exact whenever every final coefficient fits the type; the `lng` dispatcher path (`convolutionLng`) has the same `[-2^63, 2^63)` envelope.

### nttMaxLength, ntt, intt, transposedNtt, transposedIntt

`Poly<T>::nttMaxLength()` is `min(2^k, 2^30)` for a prime modulus with `2^k | mod - 1`, 0 for composite moduli. `ntt`/`intt` (vector or Poly) need a power-of-two length `<= nttMaxLength`; output is bit-reversed and `intt` consumes that order. `transposedNtt`/`transposedIntt` are the transposed linear maps. 32-bit odd moduli below `2^30` use lazy Montgomery (AVX2 lanes under `__AVX2__`), 64-bit moduli below `2^62` scalar Montgomery, others generic butterflies; all paths give identical values.

### nttDoubling

`a` holds the NTT of a polynomial with fewer than `|a|` terms and becomes its `2|a|`-point NTT; asserts `|a| >= 1` and `2|a| <= nttMaxLength`.

### fft, ifft, convolutionFft

`vector<complex<double>>` of power-of-two length, same bit-reversed convention; `convolutionFft` for `double`/`complex<double>` with relative error about `log(n) * 2^-53` of the magnitude sums.

### convolutionFftMod

`uint` residues below `mod`, `1 <= mod < 2^30` (both asserted, inputs checked at entry), `|a| + |b| - 1 <= 2^20` asserted; exact.

### convolutionArbitraryMod

`vector<ulng>` inputs of any values with `mod` in `[1, 2^64)` (three primes below `2^32`, six above), or `Poly<T>` for modular types; exact for `min(|a|, |b|) * mod^2` below the prime product, which holds for every modulus below `2^32` with `min(|a|, |b|) <= 2^25` and every 64-bit modulus with `min(|a|, |b|) <= 2^49`; each prime transform is itself block-split beyond `2^23`.

### convolutionLng, convolution2p64

`convolutionLng`: exact when every true coefficient lies in `[-2^63, 2^63)`. `convolution2p64`: the product modulo `2^64`, exact for `min(|a|, |b|) < 2^24` (the cross terms of two 32-bit halves need `min * 2^64 < 2^89`).

### convolutionLarge

Modular NTT types; blocks of `nttMaxLength / 2` terms share transforms of the full length; needs `nttMaxLength >= 2`.

### convolutionGF2k

`GF(2^k)` elements as `k`-bit words, `1 <= k <= 64`, field polynomial `x^k = red`; Karatsuba over carry-less word products (PCLMUL under `__PCLMUL__`), empty input gives empty output.

### cyclic, negacyclic

`|a|, |b| <= n`, `n >= 1`; product modulo `x^n - 1` (one transform of length `n` when `n` is an admissible power of two) or `x^n + 1` (twisted by a `2n`-th root of unity when available); other `n` fold the full product.

### square, truncatedMul, mulHigh, middleProduct, sparseMul

`square(a)`; `truncatedMul(a, b, n)` is `(a b) mod x^n` with `n` terms; `mulHigh(a, b, n)` the coefficients from `x^n` on; `middleProduct(a, b)` needs `|a| >= |b| >= 1` and returns coefficients `[|b| - 1, |a|)`; `sparseMul(a, terms)` takes `(exponent >= 0, coefficient)` pairs.

### convolution2d, multivariate, multivariateCyclic

Row-major full 2-D product; `multivariate(a, b, shape)` is the product truncated to the shape (entries `>= 1`, both inputs of the product size); `multivariateCyclic` is the product in `T[x_1..x_k] / (x_i^{n_i} - 1)` for prime 32-bit moduli with every `n_i | mod - 1` (asserted).

### mixedRadix, rader, bluestein

`mixedRadix(a, w, inverse)`: DFT of any length with `w` a primitive `n`-th root of unity (prime factors below 32 by direct sums, larger primes by Rader through a cyclic product; `inverse` uses `w^-1` and divides by `n`); `rader` is the same entry for prime lengths; `bluestein(a, w, m)` evaluates `a(w^k)` for `k < m` for any `w`.

### divMod, monicDiv, pseudoDiv, pseudoRem, tryDivide, divRoot, divSeries

`divMod(a, b)` needs `b` nonzero; fields use Newton division above `SCHOOLBOOK` terms, rings divide coefficients exactly (correct when the division is exact, for example a monic divisor). `monicDiv` needs `b` monic. `pseudoDiv`: `lead(b)^(deg a - deg b + 1) a = q b + r`, `deg r < deg b`. `tryDivide(a, b, q)` is true with the exact quotient; `b` zero or an inexact division gives false and leaves `q`. `divRoot(a, c)` returns the quotient by `x - c` and `a(c)`. `divSeries(a, b, n)` is `a / b mod x^n`, `b[0]` invertible (empty for `n = 0`).

### gcd

Fields: the monic gcd (plain Euclid below degree `GCD_FAST` = 1024 under `__AVX2__`, 4096 otherwise, then the fast Euclidean algorithm with direct Euclid below degree `HGCD_BASE` = 128); rings: the primitive gcd with positive leading coefficient through the subresultant sequence; `gcd(0, 0) = 0`.

### exGcd, halfGcd, tryInvMod, invMod, lcm

`exGcd(a, b, x, y)` (fields): monic `g = x a + y b` with the cofactors of the extended Euclidean algorithm (`deg x < deg b - deg g` and `deg y < deg a - deg g` except when one input divides the other). `halfGcd(a, b)` (`deg a > deg b`): `[m00, m01, m10, m11]` with `(a', b') = M (a, b)` a consecutive remainder pair and `deg a' >= ceil(deg a / 2) > deg b'`. `tryInvMod(a, m, out)` (`deg m >= 1`) is false when `gcd(a, m)` is not constant; `invMod` asserts. `lcm` is normalized like `gcd` and zero when either input is zero.

### resultant, subresultants, discriminant

`resultant`: the Sylvester resultant; zero when either input is zero; constants give `c^deg` of the other; fields use the quotient chain of the fast Euclidean algorithm, rings Cohen's subresultant algorithm (exact; `lng` intermediates overflow beyond tiny degrees, use `InfInt`). `subresultants(a, b, seq)` returns (resultant as a constant polynomial, last nonzero subresultant) and optionally the sequence; both inputs nonzero. `discriminant(a)` (`deg a >= 1`) is `(-1)^(n(n-1)/2) res(a, a') / lc(a)` with formal degree `n - 1` for `a'`.

### content, primitivePart, cyclotomic, trySqrtExact

Rings: `content` is the gcd of the coefficients with the sign of the leading coefficient, `primitivePart` divides it out; fields: the leading coefficient and the monic part. `Poly<T>::cyclotomic(n)` (`n >= 1`) over any `T`. `trySqrtExact(a, out)` (fields) is the exact polynomial square root, false when `a` is not a square.

### PolyModulus, mulMod, powMod, powersMod, composeMod

`PolyModulus<T>(m)` (fields, `deg m >= 1`) stores the monic-normalized modulus and the reversed inverse: `reduce(a)`, `mul(a, b)`, `pow(a, e)`. `mulMod(a, b, m)`; `powMod(a, e, m)` with `e < 0` through `invMod` (asserted invertible); `powersMod(g, k, m)` for `k >= 0`; `composeMod(f, g, m)` by Brent–Kung baby steps and giant steps with naive block sums.

### squareFree, factor, roots, isIrreducible

Fields with `T::is_prime` (prime fields handle `p`-th powers by deflation; characteristic zero is accepted for `squareFree`). `squareFree(a)` returns `(monic square-free factor, multiplicity)` sorted by multiplicity with product `a / lc(a)`. `factor(a, seed = 1)` returns sorted `(monic irreducible, multiplicity)` pairs (distinct-degree splitting, Cantor–Zassenhaus with the `(p^d - 1)/2` exponent built from Frobenius images, trace map for `p = 2`); Monte Carlo only through the 256-trial cap of equal-degree splitting: with probability below 2^-217 a returned factor is not irreducible; otherwise the seed only changes running time. `roots(a, seed = 1)` are the distinct roots in `F_p` sorted by value. `isIrreducible(a)` is Rabin's test; constants are not irreducible.

### inv, log, exp, powUnit, pow, powRational, kthRoot

`inv(a, n)`: `a[0]` invertible (any `T`; rings need a unit). `log(a, n)` (field, `a[0] = 1`), `exp(a, n)` (field, `a[0] = 0` or empty), `powUnit(u, k, n)` (field, `u[0] = 1`, exponent as an element of `T`). `pow(a, k, n)`: any `lng` exponent for fields (zero series with `k > 0` gives zero, `k < 0` needs `a[0]` invertible, valuation `v` with `v k >= n` gives zero); rings need `k > 0` and use binary powering. `powRational(a, p, q, n)` (field, `q > 0`, any `lng` `p`; zero `a` needs `p > 0`; otherwise `q | v p` and `a[v] = 1`; `v p` is formed in 128 bits so an exponent near `2^63` with a positive valuation gives the zero series instead of overflowing); `kthRoot(a, k, n) = powRational(a, 1, k, n)`.

### trySqrt, sqrt, invSparse, logSparse, expSparse, powSparse, sqrtSparse

Field. `trySqrt(a, n, out)` is true with `n` terms when `n = 0`, `a` is zero, or the valuation is even and the leading constant has a square root (modular branch: the smaller residue; floating point: the principal root); `sqrt` returns empty on failure (indistinguishable from `n = 0`). The sparse variants have the dense contracts with `O(n * terms)` recurrences; `powSparse` needs `k >= 0`.

### circular, sin, cos, tan, asin, atan, sinh, cosh, tanh, asinh, atanh

Field, `a[0] = 0`; `circular(a, n)` returns `(cos, sin)`; all return `n` terms.

### ogfToEgf, egfToOgf, fromLogDerivative, linearOde

`ogfToEgf` divides coefficient `i` by `i!`, `egfToOgf` multiplies (field, characteristic `> n`). `fromLogDerivative(h, n)` is `f` with `f' / f = h`, `f(0) = 1`. `linearOde(a, b, c, n)` solves `f' = a f + b`, `f(0) = c`, through the semi-relaxed product (field, characteristic `> n`).

### SemiRelaxedMul, RelaxedMul, semiRelaxedMul, relaxedMul

`SemiRelaxedMul<T>(g)`: `partial()` is `sum_{j<i} f_j g_{i-j}` for the next index `i`; `push(f_i)` returns the final coefficient `i` of `f g`. `RelaxedMul<T>`: both sequences online; `partial()` sums the pairs with `0 < j, k < i`; `push(f_i, g_i)` returns the final coefficient `i`. The wrappers return `(a b) mod x^n`.

### evalMulti, interpolate

`evalMulti(a, xs)`: Horner for at most 32 points or terms and for approximate `T`, else the transposed subproduct tree. `interpolate(xs, ys)`: the polynomial of degree `< n` through distinct points (asserted); approximate `T` use `O(n^2)` Newton divided differences.

### evalMultiGeometric, interpolateGeometric, productOfGeometric

`evalMultiGeometric(a, c, r, m)`: `a(c r^i)` for `i < m`, any `c`, `r`. `interpolateGeometric(c, r, ys)`: `c`, `r` nonzero and `r^k != 1` for `0 < k < n` (asserted), field with characteristic `> n`. `productOfGeometric(c, r, n)`: `prod_{i<n} (x - c r^i)` (q-binomial closed form when `r^k != 1` for `0 < k < n`, product tree otherwise).

### interpolateIota, shiftSamplingPoints, prefixSumOfPolynomial

Field, characteristic above the sizes involved. `interpolateIota(ys)`: the polynomial of degree `< n` with `a(i) = ys[i]`. `shiftSamplingPoints(ys, c, m)`: its values at `c, ..., c + m - 1`. `prefixSumOfPolynomial(f)`: `g` with `g(k) = sum_{i<k} f(i)` for every integer `k`, `|f| + 1` terms.

### hermiteInterpolate, taylorShift

`hermiteInterpolate(xs, c)`: distinct points, `c[i]` the nonempty Taylor coefficient list at `x_i` (`f(x_i + t) = sum c[i][k] t^k mod t^{|c[i]|}`); field with characteristic above twice the largest multiplicity. `taylorShift(a, c)` is `a(x + c)` (fields by binomial convolution, rings by splitting with `(x + c)^(2^j)`).

### monomialToNewton, newtonToMonomial, monomialToFactorial, factorialToMonomial, iotaPoints

Newton basis `prod_{j<k} (x - x_j)` with `|xs| >= n - 1`; the factorial forms use the nodes `0, 1, ...` (`Poly<T>::iotaPoints(n)`), falling factorials.

### transposedEvalMulti, transposedInterpolate

`transposedEvalMulti(c, xs, d)`: `sum_i c_i x_i^j` for `j < d` (`|c| = |xs|`). `transposedInterpolate(b, xs)`: `y_i = sum_j b_j [x^j] L_i` for the Lagrange basis of distinct points (`|b| = |xs|`).

### productOfSequence, productOfArithmeticProgression, sumOfRationals

`productOfSequence(fs)` (empty product 1); `productOfArithmeticProgression(a, d, n)` is `prod_{i<n} (x + a + i d)` (fields with `d != 0` by doubling with Taylor shifts, otherwise a product tree); `sumOfRationals(fractions)` returns numerator and unreduced denominator (empty sum `0 / 1`).

### compose, composeBrentKung, powerProjection, polynomialPowerEnumerate

`compose(f, g, n)`: `f(g) mod x^n` with `f` a polynomial of any length; Brent–Kung below `COMPOSE_BK` (512 under `__AVX2__`, 1024 otherwise) in `max(n, |f|)`, Kinoshita–Li above. `powerProjection(w, g, n)`: `s_k = sum_{i<n} w_i [x^i] g^k` for `k < n`. `polynomialPowerEnumerate(f, g, n)`: `[x^{n-1}] g f^k` for `k < n`.

### compositionalInverse, lagrangeInversionCoefficient, inv2d

`compositionalInverse(g, n)`: `h` with `h(g(x)) = x mod x^n` for `g[0] = 0`, `g[1]` invertible, field with characteristic `> n`. `lagrangeInversionCoefficient(g, k, n)`: `[x^n]` of the `k`-th power of the compositional inverse, `0 <= k`, `T(n)` invertible for `n >= 1`. `inv2d(f, n, m)`: `1 / f mod (x^n, y^m)` for a row-major `n x m` array with `f[0]` invertible.

### coefOfRationalFps, sliceRationalFps, linearRecurrenceKth, consecutiveTerms, findLinearRecurrence

`coefOfRationalFps(p, q, k)` (`q[0]` invertible, `k >= 0`); `sliceRationalFps(p, q, l, r)` the coefficients `[l, r)`; `linearRecurrenceKth(a, c, k)` for `a_i = sum_{j=1}^{d} c[j-1] a_{i-j}` with `|a| >= d` (indices below `|a|` are read directly); `consecutiveTerms(a, c, l, m)`; `findLinearRecurrence(a)` (Berlekamp–Massey) returns the shortest `c` valid for `|c| <= i < |a|`.

### pade, rationalInterpolate, partialFractions

`pade(f, m, n, p, q)`: `p / q` with `deg p <= m`, `deg q <= n`, `q(0) = 1`, `p = q f mod x^(m+n+1)`; false when no approximant with `q(0) != 0` exists. `rationalInterpolate(xs, ys, m, n, p, q)` with `|xs| = m + n + 1` points: false when the reduced solution does not interpolate every point. `partialFractions(p, roots)`: `(root, multiplicity >= 1)` pairs with distinct roots; returns the polynomial part and `c[i][j-1]` for `(x - r_i)^j`.

### bernoulliNumbers, partitionNumbers, stirling1Row, stirling2Row, eulerianRow

Fields with characteristic above the arguments: `B_0..B_{n-1}` (`B_1 = -1/2`), `p(0..n-1)`, signed `s(n, k)` (`n + 1` entries), `S(n, k)`, `A(n, k)` for `k < n` (`{1}` for `n = 0`).

### Complexity and correctness argument

Newton iterations (`inv`, `exp`, `sqrt`, `circular`, `inv2d`) double the precision per round for `O(M(n))`; `exp` lifts the inverse once per round, `sqrt` only while another round follows. The fast Euclidean algorithm follows von zur Gathen and Gerhard, Algorithm 11.4: a call with bound `k` returns the matrix of the maximal number of steps whose total degree drop is at most `k`; the first recursive call works on the top `2 ceil(k/2) - 1` coefficients, the second on the top `2 k*` coefficients after one explicit division, and the lemma that Euclidean quotients depend only on these top coefficients makes every recursive quotient the true one; the resultant is the product of `(-1)^{n_i n_{i+1}} lc(r_{i+1})^{n_i - n_{i+2}}` over the recorded quotient chain, whose degrees and leading coefficients follow from `deg q_{i+1} = n_i - n_{i+1}` and `lc(r_{i+1}) = lc(r_i) / lc(q_{i+1})`. Transposed evaluation descends the subproduct tree with correlations against the reversed sibling products (Bostan–Lecerf–Schost); `transposedInterpolate` transposes the weighted sum `sum_i y_i w_i P / (x - x_i)`. Kinoshita–Li composition is the transpose of the bivariate Bostan–Mori recursion on `Q = 1 - y g(x)`: each level halves the `x` precision and doubles the `y` degree, keeping every product `O(n)`; the transposed level spreads the previous result onto odd rows and correlates against `Q_i(-x, y)` reversed in both variables. The online products use the block decomposition in which, after coefficient `i`, the block with `len = 2^{v+1}` (`2^v` the lowest set bit of `i + 1`) and start `l = i + 1 - 2^v` contributes `f[l, i+1) * g[0, len)` (and symmetrically) to `h[i+1, i+1+2^v)`; every pair `(j, k)` with `j, k >= 1` is counted once by the deepest tree node separating `k` from `j + k`, and pairs with a zero index are added at the push. Finite-field routines use the standard distinct-degree, equal-degree and Rabin arguments.

### Boundedness without assertions

Every loop whose termination rests on a precondition has a hard bound so that a violated precondition under `NDEBUG` ends within the operation's normal cost: the NTT root search tries at most 1024 candidates for a quadratic non-residue (the least non-residue of every prime below 2^62 is far smaller; composite moduli otherwise could cycle through units of a Carmichael-like modulus), `poly_detail::primitiveRootMod` runs a Fermat check per candidate and returns 0 for a composite argument (the first witness or non-unit appears before the smallest prime factor), `poly_detail::dft` compares `p * p` in 64 bits, equal-degree splitting gives up after 256 random trials and appends the unsplit factor (a trial fails with probability at most 5/9, reached for p = 3 with two linear factors, so a valid input fails with probability below (5/9)^256 < 2^-217), `divMod`/`monicDiv`/`pseudoDiv` return `(0, a)` or `0` for a zero divisor instead of reading `b.v[-1]`, and the series entry points read the constant term through `coef(0)` so an empty input never indexes an empty vector. Negative sizes reach `vector` as values above `max_size()` and throw `std::length_error` immediately.

### Optimization scope

The Montgomery NTT keeps values in `[0, 2p)`, runs radix-4 DIF/DIT passes with the first level fused with the Montgomery conversion and the last with the scaling, and uses eight 32-bit lanes under `__AVX2__`; the FFT keeps split real/imaginary arrays with FMA butterflies under `__FMA__`; the GF(2^k) kernel uses `PCLMULQDQ` under `__PCLMUL__`. Scalar fallbacks produce identical results. Measured thresholds (benchmark below): `SCHOOLBOOK` 16 (AVX2) / 32 (scalar), Karatsuba base 40, `HGCD_BASE` 128, `GCD_FAST` 1024 (AVX2) / 4096 (scalar), `COMPOSE_BK` 512 (AVX2) / 1024 (scalar), `LARGE_RATIO` 16 (AVX2) / 64 (scalar) for the block product beyond the transform limit, and the scalar-build split-FFT dispatch for non-NTT 32-bit moduli up to `2^20` terms.

## Feature-to-test map

The independent oracles live in [16-poly_tester.cpp](<../../96-Local Testing/01-Core/16-poly_tester.cpp>) with the second translation unit [16-poly_tester_tu.cpp](<../../96-Local Testing/01-Core/16-poly_tester_tu.cpp>) and the runnable [Python entry](<../../96-Local Testing/01-Core/16-poly_tester.py>). The entry accepts `--mode quick|full|stress`, `--seed`, `--rounds` (stress repeats every configuration with consecutive seeds), `--scalar-only` and the shared `CP_TEST_MODE`/`CP_TEST_SEED`, calls `_00_memory_cap.ensure()` (4096 MB cap, 2048 MB quick pre-flight before full and stress), compiles the header alone and after `99-all.hpp`, builds every configuration from two translation units, keeps every check under `-DNDEBUG`, fails on subprocess or timeout errors and reports unavailable ISA execution as SKIP.

| Public feature | Independent coverage |
|---|---|
| Construction, access, mutation, operators, `eval`, `deriv`, `integ`, stream, `FPS` | Fixed small cases for every method (`testBasics`), ring `integ`, cross-translation-unit product and cache sharing. |
| `convolution`, `schoolbook`, `karatsuba`, NTT and FFT paths, `convolutionArbitraryMod`, `convolutionLng`, `convolution2p64`, `convolutionFft`, `convolutionFftMod`, `convolutionLarge`, `convolutionGF2k`, `square` | Schoolbook products for `mint`, `ModInt<1e9+7>`, `ModInt64`, composite and small-2-adic dynamic moduli, `lng`, `int`, `ulng`, `double`, `complex`, `InfInt`, sizes 1..8192 crossing 32/40/64; extreme `lng`/`int` magnitudes whose partial sums overflow the signed type (`2^31` coefficients, `LLONG_MAX`/`LLONG_MIN`) through the dispatcher, `schoolbook` and `karatsuba`; seven moduli for the `ulng` reconstruction; `GF(2^k)` for `k` in 1..64 against shift-and-xor; block products beyond the `2^18` limit of 786433 against the three-prime path. |
| `cyclic`, `negacyclic`, `truncatedMul`, `mulHigh`, `middleProduct`, `sparseMul`, `convolution2d`, `multivariate`, `multivariateCyclic` | Folded schoolbook products (power-of-two and odd lengths, ring and field), index-sum brute force for 1–3 variables and 2-D arrays, a 40x50 product through the NTT path. |
| `ntt`, `intt`, `transposedNtt`, `transposedIntt`, `nttDoubling`, `fft`, `ifft`, `mixedRadix`, `rader`, `bluestein` (and the internal `poly_detail::primitiveRootMod`) | Natural DFT by Horner in bit-reversed order, inner-product transposition identity, roundtrips (also for the 64-bit Montgomery modulus 4179340454199820289 and the generic path of 3·2^30+1, including `nttDoubling` from `n = 1`), direct `O(n^2)` DFT over 998244353 for the admissible lengths and over `DynModInt` primes `p = k n + 1` for 12, 37, 74, 97, 210, 1009, 1155, 3127 and 9973 (Rader for the primes) with inverses; known primitive roots. |
| `inv`, `log`, `exp`, `pow`, `powUnit`, `powRational`, `kthRoot`, `trySqrt`/`sqrt`, sparse variants, `circular` and the ten trigonometric series, `divSeries`, `fromLogDerivative`, `ogfToEgf`, `egfToOgf`, `linearOde`, `trySqrtExact` | Naive `O(n^2)` recurrences, exp/log/sqrt identities, derivative identities for the trigonometric series, valuation and no-answer cases including `n = 0` for every series, ring `pow` over `lng` and `InfInt`, `ModInt<1e9+7>` generic path and `double`. |
| `divMod`, `/`, `%`, `monicDiv`, `pseudoDiv`, `pseudoRem`, `tryDivide`, `divRoot` | Identity `q b + r = a` with degree bounds, ring exactness, pseudo-division identity, `InfInt`. |
| `evalMulti`, `interpolate`, `evalMultiGeometric`, `interpolateGeometric`, `productOfGeometric`, `interpolateIota`, `shiftSamplingPoints`, `hermiteInterpolate`, `taylorShift`, Newton/factorial bases, `transposedEvalMulti`, `transposedInterpolate`, `productOfSequence` | Horner at every point, interpolation roundtrips, Lagrange-basis sums, inner-product transposition, binomial expansion for Taylor shifts (field, `lng`, `InfInt`), Taylor coefficients at every Hermite node (up to 300 nodes with multiplicities to 5 in full mode, the same data for `partialFractions`), `double` evaluation and Newton interpolation. |
| `gcd`, `exGcd`, `halfGcd`, `tryInvMod`/`invMod`, `lcm`, `resultant`, `subresultants`, `discriminant`, `content`, `primitivePart`, `cyclotomic`, `PolyModulus`, `mulMod`, `powMod`, `powersMod`, `composeMod` | Euclid, remainder chains, Bezout identity and cofactor degrees, Sylvester determinants (degree `<= 30`) and Euclid chains (larger), `lng`/`InfInt` rings against the field reduction, `product of Phi_d over d dividing n equals x^n - 1`, repeated squaring and Horner modulo `m`. |
| `squareFree`, `factor`, `roots`, `isIrreducible` | Exhaustive monic polynomials over `F_2`, `F_3` (degree `<= 4`), `F_7` (`<= 3`), `F_101` (`<= 2`) against trial division and root enumeration; known factorizations with multiplicities over small fields and `998244353`; random degree-300 products. |
| `compose`, `composeBrentKung`, `powerProjection`, `polynomialPowerEnumerate`, `compositionalInverse`, `lagrangeInversionCoefficient`, `inv2d` | Horner composition, explicit power sums, two-sided inverse identity, powers of the inverse, bivariate product identity. |
| `coefOfRationalFps`, `sliceRationalFps`, `linearRecurrenceKth`, `consecutiveTerms`, `findLinearRecurrence`, `pade`, `rationalInterpolate`, `partialFractions` | Series division coefficients, iterated recurrences, reproduced sequences and known recurrences, Padé congruence and exact fractions, rational function values, reassembled fractions. |
| `RelaxedMul`, `SemiRelaxedMul`, `relaxedMul`, `semiRelaxedMul` | Truncated schoolbook products and partial sums at every index. |
| `productOfArithmeticProgression`, `sumOfRationals`, famous series, `prefixSumOfPolynomial` | Explicit products, cross-multiplied sums, known values and `O(n^2)` recurrences (Stirling, Eulerian, coin-change partitions), prefix sums by direct evaluation. |
| Preconditions | 47 `--invalid` probes on the checked scalar build (listed in the Python entry, each must abort through its assertion); `convolutionFftMod` at its `2^20` bound with worst-case residues for three moduli. |

## Commands and results

Final header, 2026-10-10, Linux x86-64 (11th Gen Intel Core i9-11900H), GCC 16.2.1 unless stated; every entry point under `_00_memory_cap` (4096 MB cap, 2048 MB quick pre-flight), all three suites started concurrently on an otherwise idle 16-core machine.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode quick --seed 12` | PASS, 3 configurations (optimized-scalar, checked-scalar + 47 `--invalid` probes, optimized-avx2), two translation units, header-alone and `99-all` compiles, 201 s, `MEMORY peak=1305MB cap=4096MB oom_kills=0` |
| `python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode full --seed 20261009` | PASS, pre-flight quick (peak 1305 MB), 5 configurations (optimized-scalar 57 s, checked-scalar 67 s + 47 probes, optimized-avx2 42 s, sanitized-scalar 1079 s, sanitized-avx2 1052 s), 63,088 checks each, 2732 s, `MEMORY peak=1910MB cap=4096MB oom_kills=0` |
| `CXX=g++-14 python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode full --seed 20261009` (GCC 14.4.1 floor check) | PASS, same 5 configurations and 63,088 checks (optimized-scalar 58 s, checked-scalar 74 s + 47 probes, optimized-avx2 43 s, sanitized-scalar 846 s, sanitized-avx2 827 s), 2262 s, `MEMORY peak=2010MB cap=4096MB oom_kills=0` |
| `python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode stress --seed 7 --rounds 1` | PASS, pre-flight quick (peak 1297 MB), optimized-scalar 403 s, checked-scalar 612 s + 47 probes, optimized-avx2 376 s (63,868 checks each on the stress lists), sanitized-scalar 862 s and sanitized-avx2 822 s (62,897 checks each on the full lists, seed 7), 3563 s, `MEMORY peak=1903MB cap=4096MB oom_kills=0` |
| `python3 '96-Local Testing/01-Core/16-poly_benchmark.py' --seed 20261010 --repetitions 5 --milliseconds 3 --output '96-Local Testing/01-Core/16-poly_benchmark.jsonl'` (machine idle) | PASS, 460 verified measurements in both configurations, `MEMORY peak=648MB cap=4096MB oom_kills=0`; table below |
| `python3 '96-Local Testing/03-consistency.py' --braces 01-Core/16-poly.hpp '96-Local Testing/01-Core/16-poly_tester.cpp' '96-Local Testing/01-Core/16-poly_benchmark.cpp'` and `python3 '96-Local Testing/03-consistency.py'` | no closing-brace, comment-cap or free-function violations; 0 errors |
| ASan/UBSan reproducers of the review (`/tmp/p025/ubcheck.cpp`: `2^31` and `LLONG_MAX`/`LLONG_MIN` products through `*`, `schoolbook`, `karatsuba`; `powRational` with exponent `2^62`), `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all` | ok, `MEMORY peak=24MB` |

Coverage per mode: quick runs every test family with the small size lists (51,164 checks); full adds the large sizes (`convolutionFftMod` at 2^20, 300-node Hermite data, 4200-degree gcd, degree-300 factorizations, exhaustive small fields), the sanitized builds and the `--invalid` probes; stress extends the random size lists (5000/8192-term products, 3000-term series, 1500-degree algebra) in the optimized and checked builds and repeats per `--rounds` with consecutive seeds; its sanitized builds execute the full-mode lists with the stress seeds, because the stress lists under ASan/UBSan exceeded the two-hour limit (7200 s timeout on 2026-10-10). AVX2/FMA/BMI2/PCLMUL execution ran natively (no SKIP); PyPy, oj-bundle, the exact GCC 14.2 build and the Windows/MinGW build were not run.

## Benchmarks

Conditions: 11th Gen Intel(R) Core(TM) i9-11900H @ 2.50GHz, g++ (GCC) 16.2.1 20260810, Linux, `-std=gnu++20 -O2 -DNDEBUG`; `scalar` = `-march=x86-64 -mno-avx -mno-avx2 -mno-fma -mno-bmi2 -mno-pclmul`, `AVX2` = `-march=x86-64 -mavx2 -mfma -mbmi2 -mpclmul -mno-avx512f`; seed 20261010, uniform random residues modulo 998244353 unless named, doubling warmup to 3 ms then the median of 5 repetitions, every result folded into a verified checksum; peak memory 648 MB (`MEMORY peak=648MB cap=4096MB oom_kills=0`). Raw log: `96-Local Testing/01-Core/16-poly_benchmark.jsonl` (git-ignored). Entries are milliseconds, scalar / AVX2.

| Operation | 2^10 | 2^12 | 2^14 | 2^16 | 2^18 | 2^20 |
|---|---|---|---|---|---|---|
| `mint` product (`*`, NTT) | 0.088 / 0.0215 | 0.459 / 0.0906 | 2.38 / 0.525 | 10.8 / 2.03 | 48.8 / 10.1 | 215 / 52.8 |
| `ntt` + `intt` of `mint` | 0.0983 / 0.0221 | 0.455 / 0.0981 | 2.51 / 0.571 | 10.3 / 2.01 | 48.9 / 10.8 | 221 / 53.6 |
| `ModInt<1e9+7>` product (`*`, three primes; scalar split-FFT below 2^20) | 0.17 / 0.106 | 0.752 / 0.589 | 3.37 / 1.95 | 17 / 9.82 | 91.2 / 46.5 | — |
| three-prime `convolutionArbitraryMod` (1e9+7) | 0.346 / 0.109 | 1.58 / 0.559 | 7.84 / 2.24 | 34.2 / 9.52 | 161 / 44 | — |
| `PolyCompanion::convolutionFftMod` (1e9+7) | 0.163 / 0.0965 | 0.758 / 0.538 | 3.29 / 1.79 | 16.8 / 9.86 | 91.6 / 57.4 | — |
| `PolyCompanion::convolutionLng` | 0.269 / 0.0872 | 1.62 / 0.518 | 7.73 / 1.87 | 33.5 / 9.45 | 155 / 44.8 | — |
| `double` product (FFT) | 0.074 / 0.0341 | 0.357 / 0.213 | 1.49 / 0.704 | 7.39 / 3.8 | 52.1 / 23.5 | — |
| `inv` | 0.259 / 0.0599 | 0.788 / 0.242 | 4.25 / 1.06 | 18.3 / 4.47 | 87.9 / 19.8 | — |
| `log` | 0.315 / 0.0991 | 1.14 / 0.387 | 6.51 / 1.69 | 30.3 / 7.4 | 131 / 33.1 | — |
| `exp` (Newton) | 0.388 / 0.164 | 1.57 / 0.569 | 8.94 / 2.56 | 41.6 / 10.5 | 193 / 42.2 | — |
| `exp` through `linearOde` (semi-relaxed) | 0.373 / 0.281 | 1.85 / 0.988 | 10.8 / 5.18 | 49.4 / 22 | — | — |
| `pow` (exponent 1e9) | 0.786 / 0.255 | 3.16 / 0.916 | 15.9 / 4.35 | 68.9 / 18.4 | 335 / 80.3 | — |
| `sqrt` | 0.253 / 0.0963 | 1.14 / 0.321 | 5.85 / 1.61 | 22.5 / 6.16 | 107 / 27.6 | — |
| `taylorShift` | 0.113 / 0.0478 | 0.61 / 0.176 | 2.81 / 0.821 | 10.6 / 4.27 | — | — |
| `evalMulti` (n points) | 1.93 / 0.871 | 9.03 / 3.54 | 46.3 / 16.8 | 227 / 77.1 | — | — |
| `interpolate` | 2.69 / 1.38 | 13 / 5.32 | 69 / 25.6 | 347 / 118 | — | — |
| `gcd` (degree n) | 2.17 / 2.35 | 33.5 / 12.1 | 196 / 57.8 | 1081 / 276 | — | — |
| `resultant` | 5.58 / 2.5 | 34.3 / 12.4 | 196 / 55.8 | 1010 / 295 | — | — |
| `compose` (dispatch) | 8.03 / 1.61 | 40.8 / 7.62 | 217 / 43.9 | 1122 / 255 | — | — |
| `composeBrentKung` | 9.07 / 3.31 | 88 / 38.7 | 1103 / 585 | — | — | — |
| `compositionalInverse` | 9.41 / 1.98 | 41.6 / 8.95 | 240 / 48.7 | 1336 / 264 | — | — |
| `coefOfRationalFps` (k = 1e18) | 19.6 / 4.43 | 88.1 / 17.4 | 429 / 87.1 | 1965 / 404 | — | — |
| `relaxedMul` | 0.9 / 0.518 | 4.36 / 2.76 | 22.9 / 10.6 | 125 / 47 | — | — |
| `semiRelaxedMul` | 0.536 / 0.302 | 2.45 / 1.42 | 12.2 / 5.17 | 66.7 / 22.9 | — | — |
| `factor` (random degree n) | 3535 / 1145 | — | — | — | — | — |

Small sizes and dispatch thresholds (ms, scalar / AVX2):

| Operation | 8 | 16 | 24 | 32 | 40 | 48 | 64 | 128 | 256 |
|---|---|---|---|---|---|---|---|---|---|
| `schoolbook` (`mint`) | 0.000106 / 0.00012 | 0.000554 / 0.000442 | 0.000877 / 0.000999 | 0.00162 / 0.00188 | 0.00273 / 0.00291 | 0.00467 / 0.00507 | 0.00792 / 0.00672 | 0.0246 / 0.03 | 0.0951 / 0.123 |
| `karatsuba` (`mint`) | — | 0.00062 / 0.000542 | 0.00101 / 0.00102 | 0.00163 / 0.00184 | 0.00284 / 0.00299 | 0.00416 / 0.0041 | 0.00669 / 0.0058 | 0.0162 / 0.022 | 0.0496 / 0.0651 |
| NTT product (`mint`) | 0.000108 / 0.000128 | 0.000597 / 0.000455 | 0.00129 / 0.000571 | 0.00153 / 0.00057 | 0.0036 / 0.00113 | 0.00426 / 0.0012 | 0.00436 / 0.00115 | 0.00815 / 0.00267 | 0.0166 / 0.00601 |
| three-prime (1e9+7) | 0.00189 / 0.00136 | 0.00377 / 0.00175 | 0.00619 / 0.00262 | 0.00719 / 0.00333 | 0.0146 / 0.00487 | 0.013 / 0.00511 | 0.0119 / 0.00593 | 0.0295 / 0.0142 | 0.0733 / 0.0294 |
| split-FFT (1e9+7) | 0.00137 / 0.000652 | 0.00186 / 0.00116 | 0.00283 / 0.00194 | 0.00357 / 0.00249 | 0.00753 / 0.00365 | 0.0076 / 0.00417 | 0.00738 / 0.00484 | 0.0157 / 0.012 | 0.0371 / 0.022 |

| Threshold probe | 128 | 256 | 512 | 1024 | 2048 | 4096 | 8192 | 16384 |
|---|---|---|---|---|---|---|---|---|
| fast Euclidean `gcd` (forced) | 0.228 / 0.224 | 0.834 / 0.493 | 2.37 / 1.12 | 6.29 / 1.77 | 15.8 / 6.28 | 35.2 / 11.6 | 80.5 / 25.4 | 210 / 54.3 |
| plain Euclid `gcd` | 0.0719 / 0.0754 | 0.22 / 0.212 | 0.671 / 0.726 | 2.25 / 1.81 | 8.57 / 8.72 | 31 / 34 | 137 / 119 | 526 / 536 |
| `compose` dispatch | 0.264 / 0.0966 | 0.91 / 0.331 | 2.97 / 0.752 | 8.1 / 1.6 | 19.9 / 4.28 | — | — | — |
| `composeBrentKung` (forced) | 0.268 / 0.0981 | 0.904 / 0.301 | 3.07 / 0.779 | 9.5 / 3.78 | 31.2 / 13.2 | — | — | — |

`DynModInt` modulus 786433 (transform limit 2^18) at 2^19 terms: `convolutionLarge` 148 / 51.3 ms against three-prime `convolutionArbitraryMod` 355 / 113 ms. A scratch sweep of the same modulus with `n = m` from 2^17 to 2^23 (padded length 1 to 64 times the limit, run capped while the test suites were executing, best of 2–3) gave block / three-prime milliseconds scalar 23/75, 68/173, 155/375, 342/784, 832/1599, 2061/3495, 5963/8936 and AVX2 11/25, 24/46, 56/104, 143/232, 391/597, 1228/1200, 4291/3564: the block product shares one forward transform per block and stays ahead until the pointwise block products dominate, so the dispatcher uses it while the padded length is at most `LARGE_RATIO` = 16 (AVX2, parity at 32) or 64 (scalar, still 1.5x ahead at 64) times the limit and sends longer products to the three-prime path.

Thresholds set from this run: `SCHOOLBOOK` 16 under AVX2 (NTT and schoolbook tie at 16 and the NTT wins from 24) and 32 scalar (schoolbook still ahead at 32, NTT ahead from 40); `GCD_FAST` 1024 under AVX2 (tie at 1024, 1.4x at 2048) and 4096 scalar (Euclid ahead through 4096, the fast algorithm 1.7x ahead at 8192); `COMPOSE_BK` 512 under AVX2 (Brent–Kung 10% ahead at 256, Kinoshita–Li level at 512 and 2.4x ahead at 1024) and 1024 scalar (level at 512, Kinoshita–Li 1.2x ahead at 1024); `HGCD_BASE` 128 and Karatsuba base 40 from the 2026-10-08 sweep (unchanged). The scalar split-FFT beats the scalar three-prime path at every measured size (1.8x at 2^18), so the scalar dispatcher uses it up to its exactness bound `FFT_MOD_MAX` = 2^20; under AVX2 the Montgomery lanes make the three-prime path faster from 2^12 on (44 against 57 ms at 2^18), so the split-FFT dispatch is compiled out there.

## Sources

| Reference | Review and use |
|---|---|
| von zur Gathen, Gerhard, *Modern Computer Algebra*, 3rd ed., Chapter 11 (fast Euclidean algorithm) | Algorithm 11.4 structure and the truncation lemma; implemented independently without monic normalization, quotients recorded for resultants. |
| Cohen, *A Course in Computational Algebraic Number Theory*, Algorithm 3.3.7 (sub-resultant) | Ring resultant and subresultant sequence; implemented from the algorithm description. |
| Kinoshita, Li, *Power Series Composition in Near-Linear Time* (FOCS 2024, arXiv 2404.05177) | Bivariate Bostan–Mori power projection and its transposition; derived and implemented independently. |
| Bostan, Lecerf, Schost, *Tellegen's principle into practice* (ISSAC 2003) | Transposed multipoint evaluation and transposed interpolation. |
| Library Checker problem list (polynomial, convolution) | Operation names and domains (composition, compositional inverse, geometric sequences, Newton basis, shift of sampling points, multivariate cyclic, convolution_mod_large). |
| Nyaan, maspypy, suisen, hitonanode, ei1333, tko919 indexes; FLINT nmod_poly; NTL ZZ_pX | Completeness sweep of 2026-10-08 (see `00-sources.md`); no code copied. |
| `OLD/5-Mathematics/10-poly.hpp`, `11-moly.hpp` | Behavioural references: `divMod`, `chirpZ` → `evalMultiGeometric`, `inter` → `interpolate`, `comp` → `compose`, `invS` → `inv`, `powS` → `pow`, `solveKthTerm` → `coefOfRationalFps`, `calcMinLinRec` → `findLinearRecurrence`, `guessKthTerm` → `findLinearRecurrence` + `linearRecurrenceKth`, `shift` → `taylorShift`. |

## Limits and handoffs

- Ring resultants and subresultants over `lng` overflow from about degree 4 with single-digit coefficients (pseudo-remainders multiply by `lead^(delta + 1)`); `InfInt` is exact. The tester compares `lng` only through degree 3.
- `composeMod` uses naive block sums (`O(n * d)` after the `O(sqrt(n) M(d))` powers); a polynomial-matrix product would lower the exponent. `findLinearRecurrence` is the `O(n^2)` Berlekamp–Massey; a half-gcd based `O(M(n) log n)` variant is not provided. `convolutionGF2k` is Karatsuba over carry-less products; the additive (Lin–Chung–Han) `O(n log n)` transform is a known faster alternative that was not adopted. Distinct-degree factorization raises to the `p`-th power with `powMod` per degree (`O(n log p M(n))`) rather than baby-step giant-step with modular composition.
- Approximate coefficients (`double`, `complex<double>`) use Horner for every point and `O(n^2)` Newton divided differences for interpolation; the subproduct-tree paths are numerically unusable for them and are not selected. The Newton series operations (`inv`, `exp`, `sqrt`, `log` and their derivatives) carry an error proportional to the magnitude of the intermediate inverse series: for a random `a` with 60 coefficients in `[-1, 1]` and `a[0] = 1` the inverse has coefficients up to `1e14` and `sqrt(a * a)` deviates from `a` by up to `2.6e-2` (400 seeds), while the same series scaled by `0.1` beyond the constant term gives `1.8e-16`; the tester uses the scaled form and the contract promises accuracy only for inputs whose inverse series stays bounded. FFT error is about `log(n) * 2^-53` of the magnitude sums; `convolutionFftMod` asserts `n + m - 1 <= 2^20` for exactness with `mod < 2^30`.
- `Rational` coefficients are untested until `09-rational.hpp` exists; the field paths compile for any type with `T(1) / c`.
- `multivariateCyclic` needs a prime 32-bit modulus with every shape entry dividing `mod - 1`; `mixedRadix` needs a primitive root supplied by the caller.
- `17-polymini.hpp` is still planned; the mini subset (ntt, convolution, divMod, inv, log, exp, pow, evalMulti, interpolate, taylorShift, berlekampMassey, linearRecurrenceKth) can be copied from the scalar paths here. Mathematics keeps its scalar duplicates (`13`, `22`, `40`, `46`, `52`, `65`) under its own rows.
- GCC 14.4.1 (`CXX=g++-14`) is the floor check; exact GCC 14.2 and the Windows/MinGW build were not executed. AVX-512 is not used (`-mno-avx512f` in the benchmark).

## History

- 2026-10-07/08: P025 implementation in chunks (kernels and convolution family, division and series, DFT variants, evaluation/interpolation, algebra, advanced), scratch-tested before the formal suite.
- 2026-10-08/09: first formal suite (seed 20261008, 63,069 checks, 5 configurations on GCC 16 and GCC 14) and the first review (ring `pow`, `inv(n = 0)`, `nttDoubling(n = 1)`, `LLONG_MIN` exponents, measured thresholds) under the free-function layout; superseded by the type-header refactor and the runs above.
- 2026-10-09/10: type-header refactor (friends, static builders, `PolyCompanion`, one `poly_detail` block), `--invalid`/`--rounds`/memory-cap tester changes, NDEBUG boundedness audit, second review (signed accumulation for `int`/`lng` schoolbook and Karatsuba, 128-bit `powRational` shift, unbalanced Karatsuba and GF(2^k) blocking, direct `03-barrett.hpp` include, naming and comment fixes), threshold remeasurement (`GCD_FAST` scalar 4096, `COMPOSE_BK` AVX2 512, `LARGE_RATIO` 16/64) and the final runs recorded above.
