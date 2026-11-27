# 017-numerical_methods.hpp — Evidence

`017-numerical_methods.hpp` holds scalar one-dimensional numerical methods in binary64: stable quadratic and cubic roots; polynomial value and derivative; compensated and pairwise summation; bracketing and open root finders with a status result; fixed-rule, adaptive, Gaussian, Romberg and double-exponential quadrature; Richardson extrapolation and finite differences; and forward-mode dual numbers.

Every operation is approximate; its contract states its stopping rule, tolerance and conditioning. Callers must not compile with `-ffast-math` or `-fassociative-math`, which reorder the compensated sums and the step-rounding trick; the header adds no floating-point flags. Every function is stateless, and caller functions must be deterministic.

## Contracts

### RootStatus

The result type of the root finders:

- `value`: the returned point.
- `converged`: true iff the method's stopping test was met: an exact zero `f(value) = 0`, a step or bracket within tolerance, or adjacent bracket endpoints.
- `iterations`: the number of method steps after the initial evaluations.
- `residual`: `|f(value)|`; for `fixedPointIterate` the last step `|x_k - x_(k-1)|` instead, infinity if no step was taken.

`converged = false` means the iteration cap was reached, or a derivative or difference quotient vanished or became non-finite. `value` is then the last valid iterate.

### numerical_detail

- `validate` asserts `itr >= 0` and finite nonnegative `abs_tol` and `rel_tol`.
- `tol(x)` is the step tolerance of the open methods: `max(abs_tol + rel_tol |x|, 4 eps |x|)`, with `eps = DBL_EPSILON`. The 4-ulp floor makes the default zero tolerances mean "to machine precision", so Newton cannot cycle between adjacent doubles until the cap.
- `opposite(a, b)` means a sign change or a zero; signed zeros count as zero.
- `pairwise` is the recursion behind `pairwiseSum`.

### solveQuadratic

`solveQuadratic(a, b, c)` returns the real roots of `a x^2 + b x + c`, ascending and with multiplicity.

- Preconditions (asserted): finite coefficients that are not all zero.
- `a = 0` gives the linear root `-c / b`, or no root when `b = 0` too.
- Otherwise all three coefficients are first multiplied by `2^-e`, `e = ilogb(max |coefficient|)`. That is exact and leaves the roots unchanged, and it puts the largest coefficient in `[1, 2)`, so `b^2` and `4 a c` cannot overflow, and underflow only where they are negligible against the largest coefficient squared. A coefficient more than `2^1074` below the largest one is lost, a limit of the double range.
- The discriminant uses Kahan's two-`fma` form: `w = 4 a c`, `d = fma(b, b, -w) + fma(-4 a, c, w)`. This form is not in Kahan (2004), whose FMA scheme splits both products exactly and adds four terms (§5); the argument here is this document's own, and the tester's exact-discriminant oracle checks it. The second `fma` is the exact rounding error of `w`, because `4 a` is exact. So `d` is accurate to a few ulps of `max(b^2, |4 a c|)` and keeps its sign near a double root, where `b * b - 4 * a * c` cancels.
- The roots are `q / a` and `c / q` with `q = -(b + sign(b) sqrt(d)) / 2`, which never subtracts nearly equal numbers. Each root therefore has a relative error of a few ulps, plus the discriminant's absolute error, which is amplified only near a double root (conditioning `sqrt(eps)`).
- `q = 0` happens only for `b = 0, d = 0`. That means `c = 0`, so the double root is 0.
- `std::fma` is a correctly rounded fused multiply-add by the C standard, also in software without hardware FMA, only slower. The judges' libm implementations were not run here.

### solveCubic

`solveCubic(a, b, c, d)` returns real roots of `a x^3 + b x^2 + c x + d`, ascending. `a = 0` returns `solveQuadratic(b, c, d)`. Otherwise all coefficients must be finite (asserted).

The polynomial is first made monic and rewritten in `y` with `x = 2^e y`. `e` is the largest `floor((ilogb(coef_k) - ilogb(a)) / k)` over the nonzero coefficients `b, c, d` (k = 1, 2, 3). Each monic coefficient is formed as a mantissa quotient in `(1/2, 2)` times an exact power of two, so neither the division by `a` nor the scaling overflows, and the scaled coefficients are below 2, 4 and 8. A coefficient underflows only when it is negligible at the largest root's scale: roots smaller than about `2^-537` of the largest are lost (`(1, -1e81, -1, 2e-81)` with roots `-2e-81, 1e-81, 1e81` returns its small roots as `-5e-82, 0`). The roots are scaled back by `2^e`. Zero `b, c, d` give the triple root 0.

The method follows Numerical Recipes 5.6. With `Q = (b^2 - 3 c) / 9` and `R = (2 b^3 - 9 b c + 27 d) / 54` of the scaled polynomial:

- The cubic is taken to have three real roots when `R^2 - Q^3 <= 2 |R| e_R + 3 Q^2 e_Q + 4 eps max(R^2, |Q^3|)`. Here `e_Q = 4 eps (b^2 + 3 |c|) / 9` and `e_R = 4 eps (2 |b|^3 + 9 |b c| + 27 |d|) / 54` bound the rounding errors of `Q` and `R`, the monic division included, and the right side bounds the error that rounding propagates into `R^2 - Q^3`.
  - So exact double roots, whose true `R^2 - Q^3` is 0, stay in this branch.
  - A genuine complex pair is lost only when its discriminant is at rounding level. For example, `(1, 0, -3 + 2.5e-13, 2 + 5e-13)` with roots `-2` and `1 ± 5e-7 i` returns one root.
- In that branch the trigonometric roots are `-2 sqrt(max(Q, 0)) cos((theta + 2 pi k) / 3) - b / 3` with `theta = acos(clamp(R / sqrt(Q^3), -1, 1))` (0 when `Q^3 <= 0`).
  - The largest-magnitude root `z` is accurate, but the smaller ones cancel against it.
  - So the other two are recomputed by deflation: `solveQuadratic(1, -(c + d / z) / z, -d / z)`, from Vieta's `y1 y2 = -d / z` and `y1 + y2 = (c - y1 y2) / z`. Neither formula subtracts `z` from `-b`, so roots `1, 2, 1e10` come out exact.
  - If rounding leaves the deflated quadratic without two real roots (a near-double pair), its vertex `(c + d / z) / (2 z)` is returned twice, which keeps the pair's error at its conditioning even beside a dominant root. Roots are returned with multiplicity, and `Q = R = 0` gives `-b / 3` three times.
- Otherwise there is one real root, `A + Q / A - b / 3` with `A = -sign(R) cbrt(|R| + sqrt(R^2 - Q^3))` (Cardano, free of cancellation).

The sorted roots then get at most two Newton steps on the scaled polynomial. A step is accepted only when it strictly lowers `|p|` and moves the root by less than `max(gap / 2, 1e-6 max |root|)`, where `gap` is the distance to the nearest other root.

- The `1e-6` term lets both copies of a near-double root converge, since their closed-form error is about `sqrt(eps)` of the root scale.
- The gap term keeps a root from jumping onto a distinct root.
- No step is taken when `p'` vanishes, and the roots are sorted again afterwards.
- A double root is ill-conditioned: its error is about `sqrt(eps)` of the root scale (a rounded near-double pair at 221 beside a root at `1e14` is resolved to about `1e-4`), and this is the accuracy the tests allow.

### hornerDerivative

`hornerDerivative(a, x)` returns `{p(x), p'(x)}` for `p = a[0] + a[1] x + ...` in one Horner pass, over any ring `T` with `0`, `+` and `*`: `double`, `long double`, integers, modints or `DualNumber`. Empty `a` gives `{0, 0}`. Plain evaluation is row 14 `polyEval`.

### kahanSum, neumaierSum, pairwiseSum

All three take a `vector<double>` and return its sum; an empty vector gives 0.

- `kahanSum`: compensated summation. The error is at most `2 eps sum |a_i| + O(n eps^2) sum |a_i|`, independent of `n` to first order. Higham (4.9) gives the sharper `(eps + O(n eps^2)) sum |a_i|`, since `eps = 2u`.
- `neumaierSum`: Kahan–Babuška–Neumaier. Its bound has the same first-order term, but the second-order term is `O(n^2 eps^2) sum |a_i|` for `n eps <= 0.2` (Higham (4.10)); it stays below `eps sum |a_i|` while `n <= 10^8`. It also keeps the error when a term is larger than the running sum (`[1, 1e100, 1, -1e100]` gives 2, where Kahan gives 0). The correction is added once at the end.
- `pairwiseSum`: recursive halving down to blocks of at most 128 summed in order. The error is at most `(128 + ceil(log2(n / 128))) eps sum |a_i|`, with recursion depth `log2(n / 128)`: each term takes part in at most `127 + ceil(log2(n / 128))` additions (Higham (4.4), (4.6)). It is faster than the compensated sums and has no data dependence on a correction term.

### bisectionRoot

`bisectionRoot(l, r, f, itr = 100, abs_tol = 0, rel_tol = 0)` needs finite `l`, `r` in either order with `f(l)` and `f(r)` of opposite sign or zero (asserted, using the two initial evaluations).

- Each step evaluates the midpoint and keeps the half whose endpoint values differ in sign, by `signbit`. It stops at an exact zero endpoint, at width `|r - l| <= abs_tol + rel_tol max(|l|, |r|)`, at adjacent endpoints, or after `itr` steps.
- `value` is the endpoint with the smaller `|f|`, so `residual` is a value that was actually evaluated. `iterations` counts midpoint evaluations.
- 100 steps shrink the width by `2^-100`. With zero tolerances, a bracket spanning many binades (a root near 0, subnormals) may need more steps; it then reports `converged = false` with the current bracket.
- `bisectionRoot` is kept beside `binSearchRealBracket` as root finding on f (`duplicates.md`, Kept by the Sweep).

### newtonRoot

`newtonRoot(x, f, df, itr = 100, abs_tol = 0, rel_tol = 0)` takes a finite start `x` and iterates `x - f(x) / df(x)`.

- It stops converged at `f(x) = 0`, or after a step with `|step| <= tol(new x)`, returning the new point.
- It stops not converged when a step is non-finite (zero, infinite or NaN derivative) or after `itr` steps.
- Convergence is quadratic near a simple root and linear near a multiple root, with no global guarantee; use `newtonSafeRoot` with a bracket.

### newtonSafeRoot

`newtonSafeRoot(l, r, f, df, itr, abs_tol, rel_tol)` follows Numerical Recipes `rtsafe`. It needs a bracket as for `bisectionRoot` and starts at the midpoint.

- Each step first shrinks the bracket to the current point by the sign of `f`. It then takes the Newton step if the step stays strictly inside the bracket and halves the step from two iterations ago (`|2 f| <= |prev df|`). Otherwise it bisects, which also covers a zero or non-finite `df`.
- It stops at an exact zero, after a step with `|step| <= tol`, at a step that does not move `x`, or after `itr` steps.
- The bracket never grows, so convergence is guaranteed: at worst linear as bisection, and quadratic once Newton steps are accepted.

### secantRoot

`secantRoot(x0, x1, f, itr, abs_tol, rel_tol)` takes finite starts `x0 != x1` (asserted) and iterates the secant through the last two points. Stopping is as in `newtonRoot`, plus equal values `f(x0) = f(x1)`, which give a non-finite step (not converged). The order is about 1.618 near a simple root. There is no guarantee.

### brentRoot

`brentRoot(l, r, f, itr, abs_tol, rel_tol)` is Brent's zeroin as Numerical Recipes `zbrent` gives it. It combines inverse quadratic interpolation and the secant with bisection, and needs a bracket as for `bisectionRoot`.

- The midpoint offset is `c / 2 - b / 2`, so brackets as wide as `[-DBL_MAX, DBL_MAX]` never overflow it.
- The convergence tolerance at each step is `t = 2 eps |b| + (abs_tol + rel_tol |b|) / 2 + DBL_MIN`. The `DBL_MIN` floor guarantees progress for a root at 0 with zero tolerances, where Brent's original requires a positive tolerance.
- It returns `b`, the best point, once the half-bracket `|c - b| / 2 <= t` or `f(b) = 0`.
- At the default `itr = 100`, a high-multiplicity root converges slowly: on `(x - 1)^9` over `[0, 3]`, `brentRoot` reports `converged = false` after 100 steps and `newtonSafeRoot` needs 97 (measured 2026-11-03); pass a larger `itr` for such roots.
- The defaults are a user decision of 2026-11-03: 100 iterations, zero tolerances meaning machine precision through the 4-ulp floor, the `DBL_MIN` Brent floor, `simpsonAdaptive` depth 50, tanh-sinh range `|t| < 7`, and a Romberg minimum of 5 levels.
- Each step either interpolates inside the bracket or bisects. The bracket keeps a sign change. Brent (1973, Chapter 4, p. 53) bounds the function evaluations by `(k + 1)^2 - 2`, where `k = ceil(log2(width / t_min))` and `t_min` is the least value of the step tolerance `t` over the bracket; bisection needs `k + 1`. `iterations` leaves out the two endpoint evaluations, so it is at most `(k + 1)^2 - 4`. The bound holds because a bisection is forced whenever an interpolation step is not under half the step before last, as here.

### fixedPointIterate

`fixedPointIterate(x, g, itr, abs_tol, rel_tol)` iterates `x = g(x)` from a finite `x`.

- It stops converged once `|g(x) - x| <= tol(g(x))`, returning `g(x)`.
- It stops not converged at a non-finite iterate (keeping the last finite `x`) or after `itr` steps.
- `residual` is the last step. For a contraction with constant `L < 1` the true error is at most `L / (1 - L)` times that step.

### simpsonFixed

`simpsonFixed(l, r, f, n = 1000)` is composite Simpson on `n` equal intervals (`n >= 1` asserted; odd `n` is rounded up). It makes `n + 1` evaluations, at `l + i h`, and `f(r)` exactly at `r`. The error is at most `(r - l) h^4 max |f''''| / 180`, and the rule is exact for cubics. The orientation `l > r` integrates with the sign of `r - l`. Counters are `lng`, so `n = INT_MAX` does not overflow.

### simpsonAdaptive

`simpsonAdaptive(l, r, f, eps = 1e-9, depth = 50)` is adaptive Simpson with Richardson correction (Lyness; KACTL `IntegrateAdaptive.h`, the archived `adaptiveIntegS`).

- An interval is accepted when its two halves change the estimate by at most `15 eps`, returning `S2 + (S2 - S) / 15`. Otherwise it splits, with `eps / 2` per half.
- `depth` bounds the recursion: a branch at depth 0, or one whose difference is NaN, is accepted as is. That bounds the work by `2^(depth + 2) + 1` evaluations (3 initial, 2 per tree node) and the stack by `depth` frames.
- An interval is also accepted when its difference is at rounding level: `|S2 - S| <= 15 * 16 eps M`, with `M` the Simpson estimate of the integral of `|f|` over the interval. So an absolute `eps` below the rounding noise of a large integrand stops at rounding level instead of filling the tree: `1e8 exp(x)` on `[0, 1]` takes 2049 evaluations at the default `eps`.
- Along a jump discontinuity, recursion reaches `depth` on one path only (about `2 depth` extra evaluations). The exponential worst case needs an integrand whose noise exceeds the floor everywhere.
- `eps > 0` and `depth >= 0` are asserted. The error estimate is heuristic, as for every adaptive rule.

### gaussLegendreNodes, gaussLegendreIntegrate

- `gaussLegendreNodes(n)` (`n >= 1` asserted; `NDEBUG` clamps to 1) returns `{nodes ascending, weights}` of the n-point Gauss–Legendre rule on `[-1, 1]`, exact for polynomials of degree `2 n - 1`. Each of the `ceil(n / 2)` nonnegative nodes is found by Newton on `P_n`, which the three-term recurrence evaluates in O(n), starting from `cos(pi (i + 3/4) / (n + 1/2))` (Numerical Recipes `gauleg`). Newton stops once a step is at most `4 eps`, then `P_n'` is recomputed at the final node for the weight `2 / ((1 - z^2) P_n'(z)^2)`. Nodes are mirrored, so the rule is exactly symmetric. Cost: O(n^2) time, O(n) memory.
- `gaussLegendreIntegrate(l, r, f, nw, panels = 1)` applies a rule `nw` from `gaussLegendreNodes` on each of `panels` equal parts of `[l, r]`, with `panels * n` evaluations. Computing the rule once is the caller's precomputation. Asserted: `panels >= 1` and a non-empty rule of matching sizes. Under `NDEBUG` a mismatched rule uses the shorter length, so no read leaves either vector.

### rombergIntegrate

`rombergIntegrate(l, r, f, eps = 1e-10, levels = 20)` builds the Romberg tableau on trapezoids halved from one interval, reusing every previous evaluation.

- It stops after level `k >= 4` (at least 16 intervals) once two consecutive diagonal entries differ by at most `max(eps, 16 eps_mach M)`, or at `levels`. `M` is the trapezoid estimate of the integral of `|f|`, so an `eps` below the rounding noise of a large integrand stops at rounding level: `1e6 exp(x)` on `[0, 1]` takes at most 257 evaluations.
- `levels` is asserted in `[1, 30]` and clamped to 30, so it makes at most `2^29 + 1` evaluations.
- The minimum depth stops false convergence on integrands that are periodic on the coarse grid. Smooth integrands converge as in Richardson extrapolation of the Euler–Maclaurin expansion.

### richardsonExtrapolate

`richardsonExtrapolate(h, f, levels = 4, t = 2, p = 2)` estimates `f(0)` from `f(h), f(h / t), ..., f(h / t^(levels - 1))`. It assumes the error expansion `f(h) - f(0) = c1 h^p + c2 h^(2 p) + ...` and returns the deepest entry of the Neville tableau `R(i, j) = R(i, j-1) + (R(i, j-1) - R(i-1, j-1)) / (t^(j p) - 1)`.

- `p = 2` fits central differences and the trapezoid rule; forward differences, whose error has every power of `h`, use `p = 1`.
- Asserted: `levels >= 1`, `t > 1` and `p >= 1`. Under `NDEBUG`, `levels <= 0` runs one level.
- Ridders' derivative is `richardsonExtrapolate(h, [&](double s) { return finiteDifference(x, f, 1, s); })`.

### finiteDifference

`finiteDifference(x, f, d = 1, h = 0, central = true)` returns the `d`-th derivative (`0 <= d <= 60` asserted; `NDEBUG` clamps `d` to that range) at a finite `x` from binomial differences.

- Forward: `sum (-1)^(d-i) C(d, i) f(x + i h) / h^d`, with error O(h).
- Central: the same weights at offsets `i - d / 2`, so odd `d` uses half steps. The error is O(h^2).
- `h = 0` chooses `eps^(1 / (d + 1 + central)) max(1, |x|)`, which balances truncation against rounding.
- The step is rounded to `(x + h) - x`, so that every offset `x + i h` is the intended point. The rounded step must stay positive and finite (asserted): `finiteDifference(1e20, f, 1, 1e-10)` loses the step entirely.
- The rounding error grows like `eps |f| 2^d / h^d`, so high orders need a coarse `h` or Richardson extrapolation for higher accuracy.

### doubleExponentialIntegrate

`doubleExponentialIntegrate(l, r, f, eps = 1e-12, levels = 12)` is tanh-sinh quadrature (Takahasi–Mori) on a finite `[l, r]`, in either orientation.

- It uses `x = (l + r) / 2 + (r - l) / 2 tanh(pi / 2 sinh t)`, with step `h = 1` halved per level and nodes reused, over `|t| < 7`.
- Endpoint distances are computed directly as `(r - l) e / (1 + e)` with `e = exp(-pi sinh t)`. So `f` is never evaluated exactly at an endpoint, and a node whose distance rounds away is skipped.
- That allows integrable endpoint singularities such as `1 / sqrt(x)` or `log(x)`. The skipped tail contributes O(weight · f(near endpoint)), which is negligible for integrable singularities.
- It stops after level `k >= 2` once two levels differ by at most `eps`, or at `levels` (asserted in `[1, 20]`, clamped). Level `k` adds about `6 * 2^k` evaluations, fewer once nodes collapse onto the endpoints.
- `l = r` returns 0 without evaluating `f`. Infinite ranges are the missing operation `doubleExponentialIntegrateInfinite`.
- At a nonzero endpoint the abscissa `l + dist` is rounded to the ulp of `l`, so `f` sees the singularity at a slightly wrong distance. For `1 / sqrt(1 - x^2)` on `[-1, 1]` the error is about `2e-8` (the tests allow `1e-7`). Translate a singular endpoint to 0 for full accuracy, as in `x = 1 - u`.

### DualNumber

`DualNumber<T = double>` is `x + dx e` with `e^2 = 0`, over a field `T`.

- Operators: `+`, `-`, unary `-`, `*`, `/` and their compound forms, all hidden friends. The implicit constructor from `T` lets scalars mix, as in `2.0 * u`.
- Division computes `x / o.x` first and then `(dx - x' o.dx) / o.x`, and follows `T` for a zero divisor (IEEE infinities and NaN for `double`).
- `DualNumber<T>::derivative(f, x)` evaluates `f` at `DualNumber(x, 1)` and returns `dx = f'(x)` exactly in the arithmetic of `T`, with no truncation error.
- Nesting `DualNumber<DualNumber<double>>` gives second derivatives. Elementary functions are written by the chain rule at the call site.

### Legacy Mapping

| Archived | Replacement |
|---|---|
| `fixedIntegS(a, b, f, n = 1000000)` (`OLD/5-Mathematics/13-numericalmethods.hpp`, `OLD/Team Notebook/src/math/simpsoninteg.hpp`) | `simpsonFixed(a, b, f, n)`. The legacy default argument of 10^6 intervals becomes 1000 (user decision 2026-11-03). |
| `adaptiveIntegS(a, b, f, eps = 1e-9)` (same files) | `simpsonAdaptive(a, b, f, eps)`. It adds the depth cap and stops a NaN branch, where the archived version could recurse without bound. |
| `isSquare` (same file) | Row 21 `isPerfectSquare` ([021-integer_roots.md](021-integer_roots.md)) |

## Feature-to-Test Map

Entry: [`017-numerical_methods_tester.py`](<../../96-Local Testing/05-Mathematics/017-numerical_methods_tester.py>), which drives `017-numerical_methods_tester.cpp` in the shared runner's optimized `-O2 -DNDEBUG` and checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress) builds, plus ASan/UBSan outside quick mode. A Python oracle on exact rationals (`fractions.Fraction`) and 80-digit `decimal` checks the discriminant sign and the quadratic roots against a derived error bound, and the three sums against exact sums. The tester subagent reported that a naive discriminant, naive summation and a dropped Neumaier correction, each planted in a scratch copy, were caught; that check has no recorded command.

| Feature | Independent verification |
|---|---|
| `solveQuadratic` | Rational roots with exact expected values; Kahan's case `(1, 1 + 2^-29)`; `b^2 >> 4 a c` cancellation; linear, no-root and zero-root cases; `±2^500` magnitudes. Regressions: roots `{-2, 1}`, `{-1, 1}` and none for `x^2 + 1`, all with coefficients scaled by `2^e`, e from -1070 to 1020. Bitwise invariance under negation, power-of-two scaling and `b -> -b`. The Python exact-discriminant and 80-digit root oracle. |
| `solveCubic` | Integer, rational, double, triple and complex-pair cubics built from known factors. Random-root cubics. Each root is checked within its conditioning: `(64 eps M / A)^(1/m)` for multiplicity `m`, from a normwise backward-error model. `a = 0` equals `solveQuadratic`. Negation and `2^-200` scaling leave roots bitwise unchanged. Saved regressions: `x (x + 5)^2`, `(x + 2)^2 (x - 3)`, `(x - 11) (x - 19)^2`, `-10 (x + 55)^2 (x - 223)`, `5 (x + 147) (x - 52)^2`. Extreme monic coefficients: roots `2^e {1, 2, 3}` and `-cbrt(4) 2^e` for e = ±100, ±250, ±340, plus the review cases `x^3 ± 3e-110 x`, `1e-100 x^3 + 1e60`, and the near-double complex pair `(1, 0, -3 + 2.5e-13, 2 + 5e-13)` giving one root. Non-monic extremes `1e-300 x^3 + 1e300`, `1e300 x^3 - 1e-300` and `1e-170 x^3 - 1e170 x`. Spread roots `{u, v, B}` for `B` from 1e4 to 1e12, each root checked against its own magnitude. Near-double pairs: the rounded `(221, 221, 1e14)` cubic and random pairs `u, u (1 ± 1e-9)` beside a root of `1e6` to `1e13`, against `64 sqrt(eps abs(B) abs(u))`. |
| `hornerDerivative` | Over `lng`, `DualNumber<double>`, nested `DualNumber<DualNumber<double>>` (value, `p'` and `p''`), a prime field and exact dyadic `long double`, against power-sum evaluation. |
| `DualNumber` | Every operator, its compound and aliased forms, and `derivative`, exactly in a prime field against the product and quotient rules. Rational functions, and nested second derivatives. |
| `kahanSum`, `neumaierSum`, `pairwiseSum` | Exact Python sums against each documented error bound; Neumaier exact on `[1, 1e100, 1, -1e100]`. Empty and singleton inputs. |
| `bisectionRoot`, `newtonRoot`, `newtonSafeRoot`, `secantRoot`, `brentRoot`, `fixedPointIterate`, `RootStatus` | Random monotone functions with known roots. Evaluation counting proves that `f` is evaluated only inside the bracket and that `iterations` equals the evaluations after the start. `converged` semantics, tolerance bounds (bisection within `log2(width / tol) + 1` steps), exact double roots at zero tolerance. Zero-derivative, equal-value, two-cycle, divergence, overflow and `itr = 0` cases; `brentRoot` on `[-DBL_MAX, DBL_MAX]` and `[-1e308, 1e308]` with every evaluation finite. Saved regressions: a step landing exactly on `f = 0` reports 1 (Newton, secant) or 0 (safe Newton, at the midpoint) iterations. |
| `simpsonFixed`, `simpsonAdaptive` | Exact for cubics; the quartic error term of the fixed rule; odd `n` rounding, `n = INT_MAX` (full and stress modes); Boole's rule at depth 0; tolerance on smooth integrands; NaN and depth caps. Regressions: the rounding floor bounds `1e8 exp(x)` at the default `eps` to 8192 evaluations, and `100 sin(x)` at `eps = 1e-15` to 65536. |
| `gaussLegendreNodes`, `gaussLegendreIntegrate` | Published node and weight tables for n <= 5; weight sum 2; exact moments up to degree `2 n - 1`; the analytic error on `x^(2n)`; panels. |
| `rombergIntegrate`, `richardsonExtrapolate` | Level counts and the minimum depth; closed forms of smooth integrals; Richardson cancelling constructed error terms of each order. Regression: the rounding floor bounds Romberg on `1e6 exp(x)` at the default `eps` to 257 evaluations. |
| `finiteDifference` | Forward and central, d = 0..6, exact on polynomials of matching degree; auto-`h` accuracy on `exp` against the stated orders. |
| `doubleExponentialIntegrate` | `1 / sqrt(x)`, `log(x)` and `1 / sqrt(1 - x^2)` endpoint singularities against closed forms (the last within `1e-7`, see Contracts); a reversed interval; `l = r`, including a singular `1 / sqrt(x)` at `l = r = 0`. |
| Preconditions | 40 checked-build assertion probes: all-zero and non-finite coefficients, brackets without a sign change, negative iteration counts or tolerances, equal secant starts, `gaussLegendreNodes(0)`, `simpsonFixed` with `n = 0`, Romberg and DE level bounds, `finiteDifference` with `d < 0` or a step lost to rounding. |

Modes: random counts are x1, x10 and x100 for quick, full and stress; Gauss–Legendre nodes are checked up to n = 15, 200 and 400; the Python oracle runs 400, 20,000 and 200,000 quadratics and 60, 600 and 4,000 sums. The runner has no `--rounds`; stress varies the seed and the counts. The 10 s quick bound (P274) moved the 20-level double-exponential cases to full and stress; quick keeps 12 levels.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14.8. Every command passed; times are compile + run.

- 2026-11-10, P274 library-wide runs on the shared contest runner's optimized, checked and ASan-UBSan builds, seed 42, beside the other suites, build cache emptied before each cold run:

  | Command | Optimized | Checked | ASan-UBSan | MEMORY peak | Pre-flight quick peak |
  |---|---|---|---|---|---|
  | `python3 '96-Local Testing/01-run.py' --mode full` | 0.1 + 34.8 s | 27.7 + 41.8 s | 32.8 + 117.4 s | 1195 MB | 939 MB |
  | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | 0.4 + 52.6 s | 43.5 + 52.0 s | 51.5 + 93.0 s | 1150 MB | 963 MB |
  | `python3 '96-Local Testing/01-run.py' --mode quick` | 40.0 + 1.8 s | 26.5 + 3.2 s | — | 926 MB | — |
  | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 0.1 + 45.7 s | 11.2 + 46.1 s | 13.9 + 105.4 s | 764 MB | 932 MB |

- 2026-11-03, the latest run with check counts: the maintainer's runs in P059's `/merge-pr 3` after its fixes, under `merge_pr.py run 3` and its gates (bwrap, memory cap). Counts are per configuration and equal on both compilers.

  | Command | Result |
  |---|---|
  | `017-numerical_methods_tester.py --mode quick` | PASS, 2 configurations, 6,702,077 C++ checks, 402 Python quadratics and 62 sums, 40 assertion probes |
  | `017-numerical_methods_tester.py --mode full --seed 1` | PASS, 3 configurations, 7,094,311 C++ checks, 20,002 Python quadratics and 602 sums, `MEMORY peak=629MB` |
  | same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=630MB` |
  | `017-numerical_methods_tester.py --mode stress --seed 2` | PASS, 3 configurations, 10,602,856 C++ checks, `MEMORY peak=782MB` |
  | `02-integration.py --sanitizers` (gate) | PASS, `MEMORY peak=4886MB cap=8192MB` |
  | `03-consistency.py` (gate) | PASS |

## Benchmarks

No timing benchmark: the Contest profile asks for optimal complexity and a direct implementation, and each operation's cost is the evaluation count its contract states. No Barrett/Montgomery or ISA-specific code is used.

## Sources

The sources read on 2026-11-22 are recorded in [00-sources.md](00-sources.md#fetched-2026-11-22-source-check).

| Reference | Inspected claim and use |
|---|---|
| Numerical Recipes 3rd ed., §4.5, §4.6, §5.6, §9.3, §9.4, read 2026-11-22 | Match the code: the DE substitution, the `gauleg` start `cos(pi (i + 3/4) / (n + 1/2))`, the quadratic and cubic formulas (5.6.4)–(5.6.17), the `zbrent` tolerance `2 eps \|b\| + tol / 2` and the `rtsafe` midpoint start. §9.3 states only that the method is "guaranteed (by Brent) to converge"; the step bound is Brent's (next row). |
| Brent, *Algorithms for Minimization without Derivatives* (1973), Chapter 4, printed pp. 47–50 and 53–54, read 2026-11-22 ([ledger](00-sources.md#fetched-2026-11-22-source-check)) | `brentRoot` follows procedure `zero`. The step tolerance `δ(x) = 2 macheps \|x\| + t` (3.3) is the header's `t`, and the stop test `\|m\| <= δ`, the step of at least `δ` toward `m` and the rule that forces a bisection when `\|p/q\| >= \|e\| / 2` or `\|e\| < δ` (pp. 48–50) match the code. Section 3 (p. 53) gives `k = ceil(log2((b - a) / δ_m))`, with `δ_m` the minimum of `δ` over `[a, b]`, and at most `2 + (5 + 7 + ... + (2k + 1)) = (k + 1)^2 - 2` evaluations (3.4), against `k + 1` for bisection. Brent adds that the bound is reached only by contrived examples and that his tests never needed more than `3(k + 1)` evaluations. The introduction (p. 48) rounds the bound to "about `[log2((b - a)/δ)]^2`", which is the form the evidence used before. |
| [KACTL numerical](https://github.com/kth-competitive-programming/kactl/tree/main/content/numerical) | `Integrate.h` (fixed Simpson) and `IntegrateAdaptive.h` (adaptive Simpson with the 15-fold Richardson test). |
| [cp-algorithms, Simpson's rule](https://cp-algorithms.com/num_methods/simpson-integration.html); [Newton's method](https://cp-algorithms.com/num_methods/roots_newton.html) | Composite Simpson error, Newton convergence. |
| [OI Wiki, numerical integration](https://oi-wiki.org/math/numerical/integral/) | Adaptive Simpson recursion. |
| W. Kahan, "On the Cost of Floating-Point Computation Without Extra-Precise Arithmetic" (2004), read 2026-11-22 | The root formulas `q / a`, `c / q` match §1. The two-`fma` discriminant is not in the paper (see `solveQuadratic`), and §6 gives no proofs. The tester's exact-discriminant oracle checks the discriminant. |
| N. J. Higham, *Accuracy and Stability of Numerical Algorithms*, 2nd ed., §4.1–4.3, read 2026-11-22 | Error bounds (4.4), (4.6), (4.9) and (4.10) of the three sums; the exact-sum oracle checks them. |
| J. N. Lyness (1969), Modifications 1–3; H. Takahasi, M. Mori (1974), (3.2)–(3.3); read 2026-11-22 | The bisection tolerance `15 eps 2^-r`, the `(S2 - S) / 15` correction, and the substitution `x = tanh(pi / 2 sinh u)`; the tester checks both against closed forms. Lyness handles round-off in §5 (Modification 4) by raising the tolerance once the difference stops shrinking; the fixed rounding floor `15 * 16 eps M` of `simpsonAdaptive` is this library's. |
| [hitonanode, dual number](https://hitonanode.github.io/cplib-cpp/) | `DualNumber` interface. |

## Limits and Handoffs

- The row is `partial`: `falsePositionRoot`, `doubleExponentialIntegrateInfinite`, `hornerDerivativeTable` and the `DualNumber` elementary functions `sqrt`, `exp`, `log`, `sin`, `cos` are missing.
- Polynomial real-root isolation and complex roots belong to row 29, contour integrals to row 45, and real searches by predicate to row 02.
- Multidimensional integration nests `simpsonAdaptive` or `gaussLegendreIntegrate`.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 数值积分, 牛顿迭代法, Kahan 求和, 自适应辛普森, 辛普森积分, 牛顿法, 一元三次方程, 卡尔丹公式, 龙贝格积分, 弦截法, 高斯求积.

## History

- 2026-11-03: P059 implementation; the tester found 2 defects (cubic double roots, iteration counts on an exact zero) and the review 8 (cubic and quadratic range, cubic slack, adaptive Simpson noise floor, bounded misuse, DE at `l = r`, a parameter name, handoffs), all fixed; full passed on g++ and g++-14.
- 2026-11-03: `/finish` audit fixed 8 defects (non-monic cubic range, spread-root deflation, Romberg rounding floor, two assertions, source labels, wrong figures, duplicate keeps) and 6 from its diff review (near-double vertex fallback, finite step, documented limits) and recorded 5 user decisions; full passed on g++ and g++-14.
- 2026-11-03: the maintainer's review in `/merge-pr 3` found 4 defects, all fixed, three with regressions that fail on the old header: `simpsonFixed` overflowed `int` at `n = INT_MAX`, `brentRoot` overflowed `c - b` on brackets wider than `DBL_MAX`, `hornerDerivative` did not compile for nested `DualNumber` (now `T{}`), and `finiteDifference` was unbounded under `NDEBUG` for huge `d` (now clamped, checked by inspection since tests keep `NDEBUG` inputs in the domain). The user confirmed the decisions recorded as theirs; full passed on g++ and g++-14.
- 2026-11-06: P253 `/finish` (user decision 2026-11-06) wrote the inventory's `DualNumber` operations `add, sub, neg, mul, div` as the operators `+, - (binary, unary), *, /`, so `05-extraction.py ops` resolves them; no code change.
- 2026-11-07: P263 added Chinese search aliases (OI Wiki page titles and common names, Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 moved the builds to the shared contest runner with `-Werror` and the 10 s quick bound; full and stress inputs unchanged; every library-wide run passed. The header gained `[[maybe_unused]]` on names only an `assert` uses, no logic change; P274 ran no mutation campaign (user decision 2026-11-10), so the first one stays with this header's own package.
