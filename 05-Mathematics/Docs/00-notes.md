# 05 Mathematics — notes

The inventory lists operations and cross-folder ownership; this file holds the folder-wide contracts every row must satisfy, the row notes and the omissions each package recorded.

## Status semantics

- `partial` rows keep their evidence link; the `missing:` list is planned work, not a defect in the verified part.
- `legacy-reference` rows have an archived implementation in `OLD/5-Mathematics` and no active header; names listed in the row are the intended active API, and the legacy `modLog` multiplier form remains available under the new header.
- `planned` rows name the API that would be implemented; nothing in them establishes compliance.

## Domains

- Domains are part of the contract: finite fields, composite residue rings, integers/rationals and approximate reals are not interchangeable. Every operation states integer widths and overflow, modulus prime/unit assumptions, characteristic, coefficient bounds, empty/zero/singular cases and legitimate failure.
- Signed 64-bit inputs include the minimum; wide (128-bit) results prevent narrowing the magnitude 2^63.
- Factorial tables in `05` require `n < mod` and a prime modulus; large-index and composite-modulus binomials belong to `12` and `37`.
- Randomized operations distinguish expected time, Monte Carlo error and verifiable Las Vegas retry, with seeds and budgets as parameters.
- Approximate methods state tolerance, error and conditioning and never silently replace exact variants; exact and approximate pivoting APIs are separately named in `16`.
- Prime-field roots and logs are Advanced (`11`); prime-power/composite roots (`33`) and large-order discrete-log techniques (`54`) are separate Esoteric families.
- Fast composition, factorization, multivariate and polynomial-matrix families stay in their own Esoteric headers rather than mixing tiers into `14`.

## Legacy obligations

- `OLD/5-Mathematics/05-primesandfactors.hpp`, `06-phiandinverse.hpp` and `07-chiremthmandgarner.hpp` are fully accounted for by the verified headers `07`–`10` (P052; per-legacy-name mapping in each evidence document). `08-modlogandmodroot.hpp`, `13-numericalmethods.hpp` and `14-miscellaneous.hpp` stay pending until `11`, `17`–`19` account for every feature, including the `modLog` multiplier parameter. All originals stay in `OLD`.
- `OLD/5-Mathematics/09-montgomery.hpp` is a Core reference; `10-poly.hpp` and `11-moly.hpp` are references for Core Poly.
- The [transfer record](../../00-Guidelines/History/2026-09-27-monolith-transfer.md) preserves source-range/hash provenance for the former local excerpts: `ModFac` (`OLD/[1] algorithms.cpp`) is accounted for by row 05, and `FastConv` (`OLD/algorithms.cpp`), the last one, was removed when P055 verified row 24.
- The archived closed segmented-sieve interval migrated to half-open `[l, r)`; closed callers map as in the Legacy mapping of [06-segmentedsieve.md](06-segmentedsieve.md).

## Evidence rules

- Check small signed domains by exhaustive references; factor products and primality, equation substitution, CRT residuals, convolution versus O(n m), interpolation reconstruction, normal-form identities, root intervals and algebraic invariants.

## Row notes

Explanations and legacy names moved out of the inventory operation cells, one bullet per row in inventory order.

- `01-mod_arithmetic.hpp`: intPow: overflow checked; modPow: signed exponent, unit inverse; invMod2p64: Newton inverse of odd word mod 2^64.
- `02-search_algorithms.hpp`: ternSearch: leftmost discrete minimum; fibSearch: Fibonacci-section leftmost minimum, fewer evaluations; expSearch: upward galloping, unbounded.
- `07-primality_factorization.hpp`: isPrimeSpf: table fallback; pollardRhoBrent: seeded, retry budget; factorize: sorted prime powers; isPrimeCompact, pollardRhoBrentCompact, factorizeCompact: Compact twins without Montgomery; factorizeTrial: deterministic O(sqrt(n)), legacy getPrimeFacSlow; coprimeBase: coarsest coprime factorization of a list; maxDivisorCount: highly composite search.
- `08-multiplicative_functions.hpp`: phi: from factorization; carmichaelLambda: lcm over prime powers, not multiplicative in the ordinary product sense; multiplicativeTable: linear-sieve hook from prime-power values, also completely multiplicative i^K.
- `09-modinverse.hpp`: inverseXgcd: nonunit failure; inverseTable: 1..n; inverseBatch: prefix-product trick, nonunit failure; inversePierce: legacy expansion; inverseList: zero entries; powerTable: i^k for i in [0, n].
- `10-crt.hpp`: crt: system, lcm overflow detection; crtScaled: a_i x = b_i mod m_i by linear-congruence reduction; crtMod: answer mod a target, via coprimeBase and Garner; garner: target modulus; garnerDigits: mixed-radix reconstruction.
- `11-discrete_log_root.hpp`: discreteLogEx: least nonnegative; modLog: legacy k a^x = b multiplier form; kthRootModPrime: zero and all-roots cases; unitRootModPrime: primitive nth root of unity.
- `12-combinatorics_advanced.hpp`: binomialPrefixSum: sum of C(n, i) for i <= k; binomialPrefixSumQueries: n and k moving by one; inclusionExclusion: sign-weighted subset sum helper; subsetSumCount: sharp-P.
- `13-convolution.hpp`: fft: iterative, bit-reversed spectrum; convolutionFft: error bound; ntt: bit-reversed spectrum; convolution: dispatcher; convolutionLarge: length beyond transform limit; convolutionTensor: multi-dimensional.
- `14-polynomial_algorithms.hpp`: polyDivLinear: synthetic division by x - c; polyShift: x^k shift; polyPowMod: modular polynomial power; polyComposeMod: f(g) mod h; fpsPow: valuation rules; fpsSqrt: branch choice; productTree: product of a polynomial sequence.
- `15-linear_recurrence.hpp`: findLinearRecurrence: status on composite modulus; kitamasa: O(d^2 log k); fiduccia: polynomial power modulo characteristic polynomial; bostanMori: nth coefficient of P/Q; consecutiveTerms: range of terms; rationalGeneratingFunction: P and Q from recurrence; interpolateLinearRecurrence: nth term from samples; interpolatePeriodicSequence: explicit period bound; interpolatePolyExp: polynomial times known exponential, degree bound from the sample count.
- `16-linear_algebra.hpp`: inverseMod: unit determinant; adjugate: singular safe; solveLinear: one solution plus nullspace basis; tridiagonalSolve: pivot breakdown status; bandedSolve: bandwidth bound; solveLinearReal: tolerance; freivaldsProductCheck: randomized; schwartzZippelIdentityCheck: randomized evaluation.
- `17-numerical_methods.hpp`: DualNumber: forward-mode.
- `18-floorsum.hpp`: floorSum: sum of floor((a i + b)/m); floorSumUnsigned: ACL contract; enumerateQuotients: distinct floor(n/i) blocks; modSum: sum of (a i + b) mod m; countModInRange: count of i with (a i + b) mod m in [l, r); floorSumMoments: sum of i floor and floor squared.
- `19-josephus.hpp`: josephus: survivor, O(n); josephusK2: closed form; josephusLarge: O(k log n) recurrence for k < n; josephusKth: kth eliminated; josephusOrder: full order.
- `20-continued_fraction.hpp`: continuedFractionSqrt: periodic, period detection; bestRationalApproximation: bounded denominator; rationalInInterval: smallest denominator; fractionBinarySearch: predicate on Stern–Brocot path.
- `21-integer_roots.hpp`: isqrt: exact floor; icbrt: exact floor; ikthRoot: floor, remainder; isPerfectPower: smallest base and largest exponent.
- `22-interpolation.hpp`: lagrangeInterpolate: O(n^2); lagrangeEvaluate: value at x from points; lagrangeEquidistant: points 0..n, O(n); hermiteInterpolate: repeated points, derivative jets; productOfGeometricShifts: product of f(r^k x) (named apart from row 37's `productOfLinearShifts`).
- `23-matrixtree.hpp`: eulerianCircuitCountBest: BEST bridge.
- `24-transform_algorithms.hpp`: walshHadamard: exact integer divisibility; subsetConvolution: O(2^n n^2); disjointUnionConvolution: q operands.
- `25-game_theory.hpp`: grundy: memoized game DAG; subtractionGame: period detection with proof bound; wythoff: cold positions; retrogradeAnalysis: win, lose, draw on a finite game graph.
- `27-optimization.hpp`: divideConquerDp: monotone opt; knuthDp: quadrangle inequality; smawk: totally monotone row minima; larsch: online Monge; aliensTrick: Lagrangian relaxation, integer penalty, count tie rule; mongeCheck: quadrangle inequality on small instances; mongeShortestPath: d-edge Monge; convexHullTrickDp: bridge to Data Structures `22-convexhulltrick.hpp`; liChaoDp: bridge.
- `29-polynomial_roots.hpp`: sturmCount: real roots in an interval; realRootIsolate: rational intervals, square-free; realRootsApprox: tolerance; polyRootsComplex: residual.
- `30-modular_power_towers.hpp`: tetrationMod: noncoprime threshold; powerTowerMod: finite exponent list; iteratedPowerMod: height, stabilization detection; towerThreshold: log2 m bound.
- `67-lattice_paths.hpp`: gridPathsAvoidingPoints: sorted obstacles, O(k^2); pathsBetweenLines: strip; pathsBelowLine: y <= a x + b boundary; catalanConvolution: k-fold; nonIntersectingPathsDeterminant: LGV bridge, Graphs owns path families.
- `31-summatory_functions.hpp`: FloorBlockTable: quotient-indexed memo; dujiaoSieve: prefix of f from a Dirichlet relation; mobiusSum: Mertens; min25Sieve: prime prefix sums then multiplicative prefix; lucyHedgehog: prime counting O(n^(3/4)); primeCount: O(n^(2/3)); divisorSummatory: sum of d(i).
- `32-dirichlet_series.hpp`: DirichletSeries: truncated prefix representation on quotient blocks; eulerProduct: from prime-power coefficients.
- `33-modular_square_roots.hpp`: sqrtModPrimePower: singular branch when p divides x; sqrtModAll: enumerated roots; polynomialCongruenceRoots: f(x) = 0 mod p^k; quadraticCongruenceMod: a x^2 + b x + c = 0 mod m.
- `34-quadratic_congruence.hpp`: pellSolutions: family generation; pellGeneralized: x^2 - D y^2 = N, fundamental set; cornacchia: x^2 + d y^2 = m; twoSquares: one representation; twoSquaresCount: r2; pythagoreanTriples: primitive, bounded hypotenuse; gaussCircleCount: lattice points with x^2 + y^2 <= r^2, O(r).
- `35-lattice_reduction.hpp`: gramDeterminant: the Gram determinant (squared covolume) that Gram–Schmidt yields; row 49 owns `latticeDeterminant`; lll: delta parameter; bkz: block size parameter; pslq: integer relation, precision parameter.
- `36-integer_factor_advanced.hpp`: ecm: curve budget; bpsw: probable-prime screening; pocklington: certificate; ecpp: certificate or unknown status.
- `37-fastfactorial.hpp`: factorialModPrime: O(sqrt(p) log p); factorialLarge: many queries, sorted block products; consecutiveProducts: product of (x + i), multipoint.
- `38-bernoulli.hpp`: bernoulliTable: B1 sign convention option; faulhaber: sum of i^k as a polynomial; powerSum: sum of i^k for i < n mod p; sumOfExponentialTimesPolynomialLimit: infinite formal sum; mixedPolyExpSum: polynomial given by samples.
- `40-generating_functions.hpp`: eulerTransform: multiset construction; logarithmicTransform: connected structures; burnsideCount: orbits from fixed-point counts; polyaEnumeration: cycle index evaluation; lagrangeInversion: coefficient of the compositional inverse; coefficientExtraction: nth coefficient of a rational function; rookPolynomial: board; fpsLinearOde: first-order formal differential equation.
- `41-padic_arithmetic.hpp`: lte: lifting the exponent; henselLiftRoot: polynomial root to precision.
- `42-finite_fields.hpp`: findIrreducible: random, degree d; conwayPolynomial: table bridge; fieldIsomorphism: embedding between representations; basisConvert: polynomial to normal basis.
- `43-rationalreconstruction.hpp`: rationalReconstruct: x mod m with bounded numerator and denominator; rationalReconstructVector: common denominator; crtReconstructRational: from modular images; errorTolerantReconstruct: k wrong residues.
- `44-determinant_advanced.hpp`: berkowitz: division-free characteristic polynomial; hessenbergReduce: similarity; charPolyHessenberg: O(n^3); determinantAPlusXB: polynomial determinant; wiedemannDeterminant: black box, Monte Carlo; vandermondeDeterminant: product of differences.
- `45-contourintegral.hpp`: contourIntegrate: exponential convergence; argumentPrincipleCount: roots inside a disc; poleExclusion: exclusion discs; cauchyCoefficient: nth Taylor coefficient by contour.
- `46-polynomial_factorization.hpp`: equalDegreeFactor: randomized; rootsModPrime: all roots, randomized splitting.
- `47-nimber.hpp`: nimMulTable: small width precomputed; nimSolveQuadratic: x^2 + a x + b = 0; nimToField: basis conversion to GF(2^(2^k)) polynomial basis.
- `48-combinatorial_linear_algebra.hpp`: permanentRyser: Gray-code inclusion-exclusion; hafnian: O(2^n n^2); cauchyBinet: determinant-sum identity; perfectMatchingCount: Monte Carlo; matrixTreeBridge: Graphs owns LGV and graph-facing counts.
- `49-integer_linear_algebra.hpp`: hermiteNormalForm: with unimodular transform; smithNormalForm: with left and right unimodular transforms; integerKernel: lattice basis of the Z-nullspace; integerSolve: one solution plus full solution lattice over Z; modularSolve: A x = b mod m, full solution set.
- `50-multivariate_polynomial.hpp`: mpolyDivision: division with remainder.
- `51-polynomial_matrix.hpp`: approximantBasis: minimal approximant or order basis; pRecursiveNthTerm: product M(0)...M(n-1); findPRecursive: order r, degree d from samples; reedsSloane: recurrence synthesis modulo composite; matrixBerlekampMassey: vector sequence.
- `52-relaxed_convolution.hpp`: SemiRelaxedConvolution: one operand known; dcDifferentialEquation: divide and conquer f' = g(f); powerProjection: weighted coefficients of f^k for all k; powerEnumerate: coefficients of f^k.
- `53-semiring_convolution.hpp`: minPlusConvolutionConvexConvex: merge, O(n); minPlusConvolutionBoundedRange: small value range.
- `54-discrete_log_advanced.hpp`: pollardRhoLog: randomized, expected sqrt; compositeGroupDecompose: unit group of Z/m as product of cyclic groups.
- `55-floor_sum_polynomial.hpp`: floorSumPolynomial: sum of i^a floor((p i + q)/m)^b for all a, b up to a bound; floorProduct: Euclidean product sums; floorMonoidProduct: noncommutative monoid of U and R steps, generic Euclidean; latticePointGeneratingFunction: rational generating function of lattice points under a line; convexFloorSum: sum of floor of a convex function; convexLatticeHull: convex hull of lattice points under a curve; gaussCircleFast: O(r^(2/3)).
- `56-coding_theory.hpp`: listDecodeGuruswamiSudan: radius and list-size parameters.
- `58-partizan_games.hpp`: zeroSumMatrixGame: mixed strategy by LP, value; misereOutcome: small game graph search.
- `59-zero_sum.hpp`: egzWitness: n of 2n - 1 elements summing to 0 mod n, constructive; zeroSumSubsequence: from any n elements, prefix sums; sidonSequence: construction.
- `60-tableau_algorithms.hpp`: hookLengthCount: standard Young tableaux; hookContentCount: semistandard with bound; rsk: permutation to tableau pair; rskWord: two-line arrays; standardTableauEnumerate: bounded.
- `61-algebraic_numbers.hpp`: AlgebraicNumber::approximate: to precision.
- `62-ball_arithmetic.hpp`: Ball: midpoint plus radius; intervalNewton: existence and uniqueness certificate; intervalBisect: root isolation; precisionEscalate: retry at higher precision via `63-multiprecision_float.hpp`.
- `63-multiprecision_float.hpp`: fromDecimalString: correctly rounded; toDouble: correctly rounded; bigPi: stated ulp bound.
- `64-group_algorithms.hpp`: PermutationGroup::schreierSims: base and strong generating set; PermutationGroup::enumerateElements: bounded; orbitEnumerate: group action on objects; abelianDecompose: generators and relations; kthRootPermutation: p with p^k = q.
- `65-convolution_specialized.hpp`: convolutionGf2_64: binary extension field, carryless; multiplicativeConvolutionModPrime: index product mod p via primitive root; multiplicativeConvolutionMod2k: index product mod 2^k; karyXorConvolution: digit-wise addition mod k, kth roots of unity; schonhageStrassen: large exact integer convolution, reference.
- `66-set_power_series.hpp`: spsPolynomialComposite: f(S) for polynomial f, O(2^n n^2); spsComposition: composition with an FPS; spsMultivariate: several set variables; connectedComponentsBySubset: log of the subset generating function.

## P012 research decisions (2026-10-07)

Left out, one line each:

- `floorDivStrict`/`ceilDivStrict` (Nyaan `int_div`): the largest integer strictly below `a / b` is `ceilDiv(a, b) - 1` and the smallest strictly above is `floorDiv(a, b) + 1`; one-liners at the call site.
- `divMod` (maspypy): `floorDiv` plus `a - b * q` at the call site; the compiler merges the two divisions.
- `gcd128` (maspypy): GNU `std::gcd` accepts `__int128` directly; no wrapper needed.
- Binary gcd (hitonanode, Nyaan): the installed libstdc++ `std::gcd` is already Stein's algorithm, and `gcd64` uses it.
- `binSearchRealRelative` (maspypy): logarithmic-scale bisection is `binSearch` over the order-preserving bit pattern of nonnegative doubles (64 steps); not a separate API.
- `firstModInRange` (maspypy `first_mod_range_of_linear`): belongs with `18-floorsum.hpp` beside `minOfModOfLinear`, where that row now lists it.
- `dioMinLinear` (cp-algorithms "minimum x + y"): a linear objective over `DioBox` is evaluated at its two endpoint parameters `l` and `r - 1`; no extra API.
- `primePi` table (hitonanode `bs_sieve`): `upper_bound(prms, x)` on the existing prime list.
- Factorization above the sieve bound (hitonanode `Sieve::factorize`): owned by `07-primality_factorization.hpp` `factorize`.
- `powTable`, the completely multiplicative i^K table (hitonanode `enumerate_kth_pows`): handed off to `08-multiplicative_functions.hpp` `multiplicativeTable`.
- `lcmOfList` (maspypy `all_lcm`): caller-side maximum of `getPrimeFac` exponents.
- `combiNegative` and `combiInverse` (maspypy `C_negative`, `C_inv`): `(-1)^k * combiWR(n, k)` and the product `inverseFactorial(a) * factorial(b) * factorial(a - b)` from the existing tables.

## P052 research decisions (2026-10-09)

Left out, one line each:

- `factorizeSpf`/`divisorsSpf` hybrids (maspypy): `LinearSieve::getPrimeFac` below the table, `factorize` above; `isPrimeSpf` is the only hybrid worth a name.
- `findPrimeFactor` (maspypy): `factorize(n)[0].fi`.
- `fermatFactor` (cp-algorithms): only fast for close factors; rho dominates.
- `fermatProbablePrime` (OI Wiki): Monte Carlo and strictly weaker than deterministic Miller–Rabin.
- `countByFactorType` (maspypy): needs prime counting; belongs with the Lucy/Meissel row.
- `DivisorArray` with a user semigroup (product-form Möbius): `T` with `+`/`-` covers the transforms; product form is `exp`/`log` of a modint table at the call site.
- `divisorProduct` (CSES): `n^(d(n)/2)` from `divisorCount` and `modPow` with the square-root case; problem-specific.
- `coprimePairCount` (OI Wiki Möbius): a floor-sum over `prefixMobiusTable`; belongs to the Möbius-sum row `31`.
- `inverseO1` (Nyaan, maspypy): `O(p^(2/3))` precomputation for `O(1)` queries with `p <= 2^30`; a niche constant-factor trick beside `inverseTable`/`inverseBatch`.
- `inverseEuler`: `a^(phi(m) - 1)` needs a factorization and is dominated by `inverseXgcd`.
- `crtFixedPrimes` (maspypy `crt3`): owned by the arbitrary-modulus convolution in Core and row `13`.
- `garnerBigint` (Nyaan): evaluate `garnerDigits` by Horner in Core `InfInt`.
- `coprimeBasis` exponents (maspypy): divide each input by the base elements at the call site.

## P055 research decisions (2026-10-09)

Left out, one line each:

- `convolutionSquare` (Nyaan, maspypy): `convolutionNtt(a, a)` detects the aliasing and does one forward transform.
- `convolutionLeq` (maspypy, sum over i <= j): problem-specific divide and conquer on top of `convolution`.
- Transposed `nttDoubling`, 64-bit-prime NTT products, `convolution_all`: owned by Core Poly, row 65 and row 14 `productTree`.
- Split-FFT arbitrary modulus (KACTL `convMod`) and the real two-for-one trick: implementation alternatives of `convolutionArbitraryMod` and `convolutionFft`; the latter is used inside `convolutionFft`.
- Many-operand pointwise product (suisen) and 2-D stencil translation (hitonanode): implementation detail and problem-specific.
- Online subset zeta / online OR convolution (maspypy): an online set-function family; handed to row 66 (set power series) or row 52 (relaxed).
- `transformPolynomialEval` (suisen): transform, pointwise `f`, inverse at the call site.
- Sparse divisor Mobius on a divisor-closed key set (hitonanode): row 08 `DivisorArray` covers the divisors of one n; a general sparse key set is niche.
- Zeta with a custom semigroup (max/min over subsets): a two-line loop at the call site or `kroneckerPowerTransform` over a semiring type with `+` and `*` redefined.
- XNOR convolution: `xorConvolution` with complemented indices; `xor_submask_lower_bound` is a bit trick for Miscellaneous.

## P057 research decisions (2026-10-10)

Left out, one line each:

- FPS trigonometric and hyperbolic functions (Nyaan `fps-circular`, OI Wiki): owned by Core Poly (`sin` … `atanh`).
- `fpsComposeExp` (f(e^{cx}), Nyaan/suisen/maspypy): generating-function transform for row 40; the `e^x - 1` case is row 12 `stirlingTransform`.
- Pseudo-division, polynomial lcm, `tryDivide`, reversal, inflate/deflate: integer-ring or trivial wrappers (Core Poly owns `pseudoDiv`; row 46 owns content and primitive parts).
- Polynomial power without a modulus: `fpsPow(a, k, k deg a + 1)`, stated in the contract.
- Graeffe step, power projection: internal to `bostanMori`; `powerProjection` is row 52.
- Products of `(1 - x^{a_i})` / `(1 + x^{a_i})`: rows 12 and 40; shift of sampling points: row 22 `samplePointShift`.
- Rational `fpsPow` with a non-unit lowest coefficient: needs a `q`-th root from row 11 (`kthRootModPrime`); the contract asserts constant 1.
- Recurrences with a polynomial inhomogeneous term and weighted prefix sums `sum a_i w^i`: call-site reductions documented in `15-linear_recurrence.md` (multiply `Q` by `(1 - x)^(deg + 1)`; substitute `c_j w^j`, `a_i w^i`).
- Reeds–Sloane and P-recursive interpolation (maspypy `interpolate_p_recursive`): row 51; Wiedemann and sparse determinants: row 44; `interpolate_poly_exp_sum`: row 38.
- Multiple k-th term queries for one recurrence: no catalog interface beats one `bostanMori` per query.
- Overlaps recorded rather than moved: row 46 keeps integer (subresultant) resultants and discriminants, row 14 the field Euclidean ones; row 22 `subproductTree` keeps the multipoint tree, row 14 `productTree` only the product; row 40 `coefficientExtraction` should call row 15 `bostanMori`.
