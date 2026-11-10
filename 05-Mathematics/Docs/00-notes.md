# 05 Mathematics — notes

The inventory lists operations; this file holds the folder-wide contracts every row must satisfy, ownership boundaries, the notes of rows not yet verified and the omissions each package recorded.

## Domains

- Domains are part of the contract: finite fields, composite residue rings, integers/rationals and approximate reals are not interchangeable. Every operation states integer widths and overflow, modulus prime/unit assumptions, characteristic, coefficient bounds, empty/zero/singular cases and legitimate failure.
- Signed 64-bit inputs include the minimum; wide (128-bit) results prevent narrowing the magnitude 2^63.
- Factorial tables in `05` require `n < mod` and a prime modulus; large-index and composite-modulus binomials belong to `12` and `37`, except row 05 `combiLarge` for small k.
- Randomized operations take retry budgets beside the seed (`03-cpp.md`, API contracts).
- Approximate methods also state conditioning (`03-cpp.md`, API contracts); exact and approximate pivoting APIs are separately named in `16`.
- Prime-field roots and logs are Advanced (`11`); prime-power/composite roots (`33`) and large-order discrete-log techniques (`54`) are separate Esoteric families.
- Fast composition, factorization, multivariate and polynomial-matrix families stay in their own Esoteric headers rather than mixing tiers into `14`.
- Integer roots are canonical in `21` (`55` calls them); inverse tables live in `09`; divisor and multiple zeta/Möbius over [1, n] in `24`, over the divisors of one n in `08` `DivisorArray`.

## Ownership boundaries

- Core `16-poly.hpp` owns full composition and compositional inverse (`14` keeps scalar duplicates, `40` Lagrange inversion as coefficient extraction); Core `10-matrix.hpp` owns characteristic/minimal polynomial and Frobenius/Jordan forms (`44` keeps the scalar Hessenberg/Berkowitz/black-box algorithms); Core `14-sparsematrix.hpp` keeps its fixed-iteration `steadyState`, `26` the stationary distributions that test irreducibility.
- XOR linear basis is Data Structures `62-xorbasis.hpp`; Core `12-bitmatrix.hpp` owns packed GF(2) subspaces.
- Lattice counts inside polygons belong to Geometry `19-lattice_geometry.hpp`; `34` and `55` own only the arithmetic circle and curve counts.
- `27` owns Monge/Knuth/Aliens DP optimizations; Miscellaneous `34-slope_trick.hpp` owns slope trick and Data Structures `21-lichao.hpp` and `22-convexhulltrick.hpp` own the line containers it bridges to.
- Graph-facing counting (LGV, Tutte) is Graphs `53-graph_counting.hpp`; `23` owns the matrix-tree counts, BEST included (user decision 2026-11-08), and the kernels stay in `23`, `48` and `67`.
- Strings owns distinct-subsequence counting (`19-lcs.hpp`, `20-subsequence_automaton.hpp`) and de Bruijn sequences (`41-debruijn.hpp`); Miscellaneous owns Gray codes (`06-bit_operations.hpp`), random sampling (`18-randomized_algorithms.hpp`), knapsack (`13`, `29`), digit sums and base conversion (`31-digitdp.hpp`) and parallel binary search (`20`).

## Legacy obligations

- `OLD/5-Mathematics/01-exgcdandmodpow.hpp`–`04-sieves.hpp` and `12-combinatorics.hpp` are accounted for by rows `01`–`06` (P012). `05-primesandfactors.hpp`, `06-phiandinverse.hpp` and `07-chiremthmandgarner.hpp` are fully accounted for by the verified headers `07`–`10` (P052; per-legacy-name mapping in each evidence document). `08-modlogandmodroot.hpp` is accounted for by rows `11` (P053) and `33` (P178, `modRoot` modulo prime powers and composites). `14-miscellaneous.hpp` is accounted for by rows `18` and `19` (P060: `getFloorSum` by `floorSum`, the recursive `josephus` by `josephus`); `13-numericalmethods.hpp` and `OLD/Team Notebook/src/math/simpsoninteg.hpp`, `issquare.hpp` are accounted for by rows `17` and `21` (P059; mapping in their evidence documents). All originals stay in `OLD`.
- `OLD/Team Notebook/src/math/old_gaussjordan.cpp`, `old_matrix.cpp` and the `Matrix` of `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` are accounted for by row `16` (P058, mapping in its evidence document); `00-matrix.hpp` is shared with Core Matrix (P020), which owns its arithmetic and `FastMatMul`.
- `OLD/5-Mathematics/09-montgomery.hpp` is a Core reference; `10-poly.hpp` and `11-moly.hpp` are references for Core Poly.
- Former `97-Legacy` excerpts `ModFac` (`OLD/[1] algorithms.cpp`) and `FastConv` (`OLD/algorithms.cpp`) are accounted for by rows 05 and 24; provenance in the [transfer record](../../00-Guidelines/History/2026-09-27-monolith-transfer.md).

## Evidence rules

- Check small signed domains by exhaustive references; factor products and primality, equation substitution, CRT residuals, convolution versus O(n m), interpolation reconstruction, normal-form identities, root intervals and algebraic invariants.

## Row notes

Explanations moved out of the operation cells of rows not yet verified, one bullet per row in inventory order; verified rows keep theirs in their evidence documents.

- `22-interpolation.hpp`: lagrangeInterpolate: O(n^2); lagrangeEvaluate: value at x from points; lagrangeEquidistant: points 0..n, O(n); hermiteInterpolate: repeated points, derivative jets; productOfGeometricShifts: product of f(r^k x) (named apart from row 37's `productOfLinearShifts`).
- `23-matrixtree.hpp`: eulerianCircuitCountBest: BEST bridge.
- `25-game_theory.hpp`: grundy: memoized game DAG; subtractionGame: period detection with proof bound; wythoff: cold positions; retrogradeAnalysis: win, lose, draw on a finite game graph.
- `27-optimization.hpp`: divideConquerDp: monotone opt; knuthDp: quadrangle inequality; smawk: totally monotone row minima; larsch: online Monge; aliensTrick: Lagrangian relaxation, integer penalty, count tie rule; mongeCheck: quadrangle inequality on small instances; mongeShortestPath: unconstrained O(n log n) with predecessors and edge count, exactly d edges or a count in [L, R] by penalty binary search; convexHullTrickDp: bridge to Data Structures `22-convexhulltrick.hpp`; liChaoDp: bridge.
- `29-polynomial_roots.hpp`: sturmCount: real roots in an interval; realRootIsolate: rational intervals, square-free; realRootsApprox: tolerance; polyRootsComplex: residual.
- `31-summatory_functions.hpp`: FloorBlockTable: quotient-indexed memo; dujiaoSieve: prefix of f from a Dirichlet relation; mobiusSum: Mertens; min25Sieve: prime prefix sums then multiplicative prefix; lucyHedgehog: prime counting O(n^(3/4)); primeCount: Meissel–Lehmer, practical to about 1e13 (OI Wiki gives O~(n^(2/3)) only to LMO); primeCountTable: the whole pi(n / i) table, O(n^(2/3) log^(1/3) n); lmoPrimeCount: O(n^(2/3) log^(1/3) n) time, O~(n^(1/3)) space segmented; delegliseRivatPrimeCount: the LMO refinement with a smaller log factor; divisorSummatory: sum of d(i).
- `32-dirichlet_series.hpp`: DirichletSeries: truncated prefix representation on quotient blocks; eulerProduct: from prime-power coefficients.
- `35-lattice_reduction.hpp`: gramDeterminant: the Gram determinant (squared covolume) that Gram–Schmidt yields; row 49 owns `latticeDeterminant`; lllReduce: delta parameter; bkz: block size parameter; pslq: integer relation, precision parameter.
- `36-integer_factor_advanced.hpp`: ecm: curve budget; bpsw: probable-prime screening; pocklington: certificate; ecpp: certificate or unknown status.
- `37-fastfactorial.hpp`: factorialModPrime: O(sqrt(p) log p); factorialLarge: many queries, sorted block products; consecutiveProducts: product of (x + i), multipoint.
- `38-bernoulli.hpp`: bernoulliTable: B1 sign convention option; faulhaber: sum of i^k as a polynomial; powerSum: sum of i^k for i < n mod p; sumOfExponentialTimesPolynomialLimit: infinite formal sum; mixedPolyExpSum: polynomial given by samples.
- `40-generating_functions.hpp`: eulerTransform: multiset construction (MSET; `unlabelledMultiset` removed 2026-11-08 as its alias); logarithmicTransform: connected structures; burnsideCount: orbits from fixed-point counts; polyaEnumeration: cycle index evaluation; lagrangeInversion: coefficient of the compositional inverse; coefficientExtraction: nth coefficient of a rational function, wrapping row 15 `bostanMori` (user decision 2026-11-08); rookPolynomial: board; fpsLinearOde: first-order formal differential equation.
- `41-padic_arithmetic.hpp`: lte: lifting the exponent; henselLiftRoot: polynomial root to precision.
- `42-finite_fields.hpp`: findIrreducible: random, degree d; conwayPolynomial: table bridge; fieldIsomorphism: embedding between representations; basisConvert: polynomial to normal basis; GF2k::carrylessMultiply wraps row 65 `carrylessMultiply` (user decision 2026-11-08).
- `43-rationalreconstruction.hpp`: rationalReconstruct: x mod m with bounded numerator and denominator; rationalReconstructVector: common denominator; crtReconstructRational: from modular images; errorTolerantReconstruct: k wrong residues.
- `44-determinant_advanced.hpp`: berkowitz: division-free characteristic polynomial; hessenbergReduce: similarity; charPolyHessenberg: O(n^3); determinantAPlusXB: polynomial determinant; wiedemannDeterminant: black box, Monte Carlo; vandermondeDeterminant: product of differences; determinantPolynomialMatrix wraps row 51 `PolyMatrix::det`, which owns the evaluation-and-interpolation algorithm (user decision 2026-11-08).
- `45-contourintegral.hpp`: contourIntegrate: exponential convergence; argumentPrincipleCount: roots inside a disc; poleExclusion: exclusion discs; cauchyCoefficient: nth Taylor coefficient by contour.
- `46-polynomial_factorization.hpp`: equalDegreeFactor: randomized; rootsModPrime: all roots, randomized splitting.
- `47-nimber.hpp`: nimMulTable: small width precomputed; nimSolveQuadratic: x^2 + a x + b = 0; nimToField: basis conversion to GF(2^(2^k)) polynomial basis.
- `48-combinatorial_linear_algebra.hpp`: permanentRyser: Gray-code inclusion-exclusion; hafnian: O(2^n n^2); cauchyBinet: determinant-sum identity; perfectMatchingCount: Monte Carlo.
- `49-integer_linear_algebra.hpp`: hermiteNormalForm: with unimodular transform; smithNormalForm: with left and right unimodular transforms; integerKernel: lattice basis of the Z-nullspace; integerSolve: one solution plus full solution lattice over Z; modularSolve: A x = b mod m, full solution set.
- `50-multivariate_polynomial.hpp`: mpolyDivision: division with remainder.
- `51-polynomial_matrix.hpp`: approximantBasis: minimal approximant or order basis; pRecursiveNthTerm: product M(0)...M(n-1); findPRecursive: order r, degree d from samples; reedsSloane: recurrence synthesis modulo composite; matrixBerlekampMassey: vector sequence.
- `52-relaxed_convolution.hpp`: SemiRelaxedConvolution: one operand known; dcDifferentialEquation: divide and conquer f' = g(f); powerProjection: weighted coefficients of f^k for all k; powerEnumerate: coefficients of f^k.
- `53-semiring_convolution.hpp`: minPlusConvolutionConvexConvex: merge, O(n); minPlusConvolutionBoundedRange: small value range.
- `54-discrete_log_advanced.hpp`: pollardRhoLog: randomized, expected sqrt; compositeGroupDecompose: unit group of Z/m as product of cyclic groups.
- `55-floor_sum_polynomial.hpp`: floorSumPolynomial: sum of i^a floor((p i + q)/m)^b for all a, b up to a bound; floorProduct: Euclidean product sums; floorMonoidProduct: noncommutative monoid of U and R steps, generic Euclidean; latticePointGeneratingFunction: rational generating function of lattice points under a line; convexFloorSum: sum of floor of a convex function; convexLatticeHull: convex hull of lattice points under a curve; gaussCircleFast: O(r^(2/3)).
- `56-coding_theory.hpp`: listDecodeGuruswamiSudan: radius and list-size parameters.
- `58-partizan_games.hpp`: zeroSumMatrixGame: mixed strategy by LP, value; misereOutcome: small game graph search.
- `59-zero_sum.hpp`: egzWitness: n of 2n - 1 elements summing to 0 mod n, constructive; zeroSumSubsequence: from any n elements, prefix sums; SidonSequence: n terms below p(p - 1) by CRT, O(p) build; query maps a sum to its index pair in O(1).
- `60-tableau_algorithms.hpp`: hookLengthCount: standard Young tableaux; hookContentCount: semistandard with bound; rsk: permutation to tableau pair; rskWord: two-line arrays; standardTableauEnumerate: bounded.
- `61-algebraic_numbers.hpp`: AlgebraicNumber::approximate: to precision.
- `62-ball_arithmetic.hpp`: Ball: midpoint plus radius; Ball::rationalInInterval: simplest rational inside the ball, wrapping row 20 `rationalInInterval`; intervalNewton: existence and uniqueness certificate; intervalBisect: root isolation; precisionEscalate: retry at higher precision via `63-multiprecision_float.hpp`.
- `63-multiprecision_float.hpp`: fromDecimalString: correctly rounded; toDouble: correctly rounded; bigPi: stated ulp bound.
- `64-group_algorithms.hpp`: PermutationGroup::schreierSims: base and strong generating set; PermutationGroup::enumerateElements: bounded; orbitEnumerate: group action on objects; abelianDecompose: generators and relations; kthRootPermutation: p with p^k = q.
- `65-convolution_specialized.hpp`: convolutionGf2_64: binary extension field, carryless; multiplicativeConvolutionModPrime: index product mod p via primitive root; multiplicativeConvolutionMod2k: index product mod 2^k; karyXorConvolution: digit-wise addition mod k, kth roots of unity or the cyclotomic ring Z[x]/Phi_k when the modulus lacks them; schonhageStrassen: large exact integer convolution, reference.
- `66-set_power_series.hpp`: spsPolynomialComposite: f(S) for polynomial f, O(2^n n^2); spsComposition: composition with an FPS; spsMultivariate: several set variables; connectedComponentsBySubset: log of the subset generating function.

## P012 research decisions (2026-10-07)

Left out, one line each:

- `floorDivStrict`/`ceilDivStrict` (Nyaan `int_div`): the largest integer strictly below `a / b` is `ceilDiv(a, b) - 1` and the smallest strictly above is `floorDiv(a, b) + 1`; one-liners at the call site. (returned by P275 2026-11-10 into row 01)
- `divMod` (maspypy): `floorDiv` plus `a - b * q` at the call site; the compiler merges the two divisions. (returned by P275 2026-11-10 into row 01 `divMod`)
- `gcd128` (maspypy): GNU `std::gcd` accepts `__int128` directly; no wrapper needed.
- Binary gcd (hitonanode, Nyaan): the installed libstdc++ `std::gcd` is already Stein's algorithm, and `gcd64` uses it.
- `binSearchRealRelative` (maspypy): logarithmic-scale bisection is `binSearch` over the order-preserving bit pattern of nonnegative doubles (64 steps); not a separate API.
- `firstModInRange` (maspypy `first_mod_range_of_linear`): belongs with `18-floorsum.hpp` beside `minOfModOfLinear`, where that row now lists it.
- `dioMinLinear` (cp-algorithms "minimum x + y"): a linear objective over `DioBox` is evaluated at its two endpoint parameters `l` and `r - 1`; no extra API.
- `primePi` table (hitonanode `bs_sieve`): `upper_bound(prms, x)` on the existing prime list.
- Factorization above the sieve bound (hitonanode `Sieve::factorize`): owned by `07-primality_factorization.hpp` `factorize`.
- `powTable`, the completely multiplicative i^K table (hitonanode `enumerate_kth_pows`): handed off to `08-multiplicative_functions.hpp` `multiplicativeTable`.
- `lcmOfList` (maspypy `all_lcm`): caller-side maximum of `getPrimeFac` exponents.
- `combiNegative` and `combiInverse` (maspypy `C_negative`, `C_inv`): `(-1)^k * combiWR(n, k)` and the product `inverseFactorial(a) * factorial(b) * factorial(a - b)` from the existing tables. (returned by P275 2026-11-10 into row 05)

## P052 research decisions (2026-10-09)

Left out, one line each:

- `factorizeSpf`/`divisorsSpf` hybrids (maspypy): `LinearSieve::getPrimeFac` below the table, `factorize` above; `isPrimeSpf` is the only hybrid worth a name.
- `findPrimeFactor` (maspypy): `factorize(n)[0].fi`.
- `fermatFactor` (cp-algorithms): only fast for close factors; rho dominates.
- `fermatProbablePrime` (OI Wiki): Monte Carlo and strictly weaker than deterministic Miller–Rabin.
- `countByFactorType` (maspypy): needs prime counting; belongs with the Lucy/Meissel row.
- `DivisorArray` with a user semigroup (product-form Möbius): `T` with `+`/`-` covers the transforms; product form is `exp`/`log` of a modint table at the call site.
- `divisorProduct` (CSES): `n^(d(n)/2)` from `divisorCount` and `modPow` with the square-root case; problem-specific.
- `coprimePairCount` (OI Wiki Möbius): a floor-sum over `prefixMobiusTable`; belongs to the Möbius-sum row `31`. (returned by P275 2026-11-10 into row 31)
- `inverseO1` (Nyaan, maspypy): adopted on 2026-11-08 into row 09 (user decision): O(1) online queries on arbitrary values, against O(log p) for `inverseXgcd` and `inverseFermat`.
- `inverseEuler`: `a^(phi(m) - 1)` needs a factorization and is dominated by `inverseXgcd`.
- `crtFixedPrimes` (maspypy `crt3`): owned by the arbitrary-modulus convolution in Core and row `13`.
- `garnerBigint` (Nyaan): evaluate `garnerDigits` by Horner in Core `InfInt`.
- `coprimeBasis` exponents (maspypy): divide each input by the base elements at the call site.

## P055 research decisions (2026-10-09)

Left out, one line each:

- `convolutionSquare` (Nyaan, maspypy): `convolutionNtt(a, a)` detects the aliasing and does one forward transform.
- `convolutionLeq` (maspypy, sum over i <= j): problem-specific divide and conquer on top of `convolution`. (returned by P275 2026-11-10 into row 65)
- Transposed `nttDoubling`, 64-bit-prime NTT products, `convolution_all`: owned by Core Poly, row 65 and row 14 `productTree`.
- Split-FFT arbitrary modulus (KACTL `convMod`) and the real two-for-one trick: implementation alternatives of `convolutionArbitraryMod` and `convolutionFft`; the latter is used inside `convolutionFft`.
- Many-operand pointwise product (suisen) and 2-D stencil translation (hitonanode): implementation detail and problem-specific.
- Online subset zeta / online OR convolution (maspypy): an online set-function family; handed to row 66 (set power series) or row 52 (relaxed).
- `transformPolynomialEval` (suisen): transform, pointwise `f`, inverse at the call site. (returned by P275 2026-11-10 into row 24)
- Sparse divisor Mobius on a divisor-closed key set (hitonanode): row 08 `DivisorArray` covers the divisors of one n; a general sparse key set is niche.
- Zeta with a custom semigroup (max/min over subsets): a two-line loop at the call site or `kroneckerPowerTransform` over a semiring type with `+` and `*` redefined.
- XNOR convolution: `xorConvolution` with complemented indices; `xor_submask_lower_bound` is a bit trick for Miscellaneous.

## P057 research decisions (2026-10-10)

Left out, one line each:

- FPS trigonometric and hyperbolic functions (Nyaan `fps-circular`, OI Wiki): owned by Core Poly (`sin` … `atanh`).
- `fpsComposeExp` (f(e^{cx}), Nyaan/suisen/maspypy): adopted on 2026-11-08 into row 40 (the P057 handoff had not reached the row); the `e^x - 1` case stays row 12 `stirlingTransform`.
- Pseudo-division, polynomial lcm, `tryDivide`, reversal, inflate/deflate: integer-ring or trivial wrappers (Core Poly owns `pseudoDiv`; row 46 owns content and primitive parts).
- Polynomial power without a modulus: `fpsPow(a, k, k deg a + 1)`, stated in the contract.
- Graeffe step, power projection: internal to `bostanMori`; `powerProjection` is row 52.
- Products of `(1 - x^{a_i})` / `(1 + x^{a_i})`: rows 12 and 40; shift of sampling points: row 22 `samplePointShift`.
- Rational `fpsPow` with a non-unit lowest coefficient: needs a `q`-th root from row 11 (`kthRootModPrime`); the contract asserts constant 1.
- Recurrences with a polynomial inhomogeneous term and weighted prefix sums `sum a_i w^i`: call-site reductions documented in `15-linear_recurrence.md` (multiply `Q` by `(1 - x)^(deg + 1)`; substitute `c_j w^j`, `a_i w^i`).
- Reeds–Sloane and P-recursive interpolation (maspypy `interpolate_p_recursive`): row 51; Wiedemann and sparse determinants: row 44; `interpolate_poly_exp_sum`: row 38.
- Multiple k-th term queries for one recurrence: no catalog interface beats one `bostanMori` per query.
- Overlaps recorded rather than moved: row 46 keeps integer (subresultant) resultants and discriminants, row 14 the field Euclidean ones; row 22 `subproductTree` keeps the multipoint tree, row 14 `productTree` only the product; row 40 `coefficientExtraction` should call row 15 `bostanMori`.

## P053 research decisions (2026-10-16)

Left out, one line each:

- Least positive discrete log (KACTL `modLog`, PyRival): `discreteLogEx(a, b, m, k * a mod m) + 1` when that is not -1.
- All k-th roots modulo a prime (cp-algorithms discrete root): `kthRootModPrime` times the powers of `unitRootModPrime(gcd(k, p - 1), p)`; the count is row 33 `rootCount`.
- Precomputed-modulus order object (suisen `OrderMod`): one factorization of `lambda(m)` per query costs O(m^(1/4)); many discrete-log queries for one prime and base are row 54 `DiscreteLogTable`, adopted on 2026-11-08.
- Lifting primitive roots to `p^e` and random primitive roots (maspypy, suisen): internal choices of `primitiveRoot`.
- An element of maximal order `lambda(m)` (OI Wiki): row 54 `compositeGroupDecompose`.
- Index calculus (suisen `IndexCalculus`): adopted on 2026-11-08 into row 54 as `indexCalculus` (the P053 handoff had not reached the row).
- Square-root special cases (p = 3 mod 4 power, Atkin, Legendre's algorithm): dominated by Cipolla and Core `trySqrt`, which already takes the p = 3 mod 4 power.
- Nonresidue search and the order-of-a-power formula: internal loops and one-line formulas.

## P178 research decisions (2026-10-17)

Adopted: `BinaryQuadraticForm::reducedForms` (class-group enumeration; `classNumber` is its size). `fourSquares` reduces to `threeSquares` (maspypy), replacing the planned randomized Rabin–Shallit four-square search: same O(log(n)) expected primality tests, deterministic. Left out, one line each:

- `unitRootMod` (an element of exact order k modulo a composite, OI Wiki): the unit group is not cyclic; row 54 `compositeGroupDecompose` owns generators and orders.
- `kthPowerResidueCount` (OI Wiki): phi(p^e) / gcd(k, phi(p^e)) per odd prime power, a call-site formula; residuosity of one value is `rootCount > 0`. (returned by P275 2026-11-10 into row 33)
- All k-th roots modulo a composite (OI Wiki `kth_roots_mod_m`): a coset of the k-torsion, not a union of residue classes; `kthRootModComposite` and `rootCount`, and for a cyclic unit group the coset `x0 * (g^(phi / d))^i` with row 11 `primitiveRoot`.
- Factorization-taking overloads (FLINT `n_sqrtmod_primepow`, `n_sqrtmodn`): the prime-power operations already take (p, e).
- `quadraticDiophantine` (SymPy `diop_quadratic`): completing the square reduces it to `pellGeneralized` plus congruence filtering of the classes; a full conic solver is research-level. (returned by P275 2026-11-10 into row 73)
- `legendreTernary` (SymPy `diop_ternary_quadratic`): needs square roots modulo |abc| and lattice reduction (row 35); no contest catalog entry. (returned by P275 2026-11-10 into row 73)
- `pellFour` (x^2 - D y^2 = +-4, OI Wiki): `pellGeneralized(D, +-4)`.
- Stern–Brocot Pell search (maspypy `pell_equation`): the same continued-fraction path found by comparisons.
- Brute-force and PQa generalized Pell (SymPy `diop_bf_DN`, OI Wiki): Lagrange–Matthews–Mollin is polynomial in the period and the root count, with no Nagell-bound scan.
- `eisensteinRepresentationCount` (OI Wiki): `BinaryQuadraticForm{1, 1, 1}.represent(n).size()`, or 6 times the sum of (-3 / d) over d | n.
- `sumOfSquaresEnumerate` for k > 2 (SymPy): low value; k = 2 is `twoSquaresAll`.
- Indefinite forms (cycle reduction, NUCOMP): the principal indefinite form is the Pell family; general real quadratic class groups are research-level.
- Row 57 `gaussianPrimeFactor` and `fourSquaresFromHurwitz` overlap `twoSquares` and `fourSquares`; the row 57 package settles them (handoff in [34-quadratic_congruence.md](34-quadratic_congruence.md)).

## P060 research decisions (2026-10-18)

Adopted: `maxOfModOfLinear` (maspypy; a ruling 3 mirror), `forEachJointQuotientBlock` (OI Wiki multi-dimensional blocks, suisen), `josephusTime` (Concrete Mathematics 3.3 renumbering), `continuedFractionQuadratic` (cp-algorithms) and `fareyKth` (maspypy, OI Wiki). Removed by user decision: `floorSumUnsigned` (`ulng(floorSum(n, m, a, b))`), `josephusZeroBased`/`josephusOneBased` (index base) and `josephusLarge` (2026-10-18: `josephus` itself became the run-skipping survivor, no slower than the plain loop it replaced, [19-josephus.md](19-josephus.md)). Left out, one line each:

- `forEachCeilQuotientBlock` (maspypy `ceil_range`): ceil(N / x) = floor((N - 1) / x) + 1 for N >= 1, so its blocks are those of `forEachQuotientBlock(N - 1)` plus x >= N with quotient 1. (returned by P275 2026-11-10 into row 18)
- `quotientCount` (suisen): `enumerateQuotients(n).size()`, or 2 isqrt(n) - [isqrt(n)^2 + isqrt(n) > n].
- `minOfLinearSegments` (maspypy): the prefix-minimum structure behind its `min_of_linear`; `minOfModOfLinear` exposes the value and least argument.
- Minimum by binary search with floor sums (suisen, O(log(m)^2)): dominated by `minOfModOfLinear`.
- Range forms over [L, R) (maspypy `floor_sum_of_linear`, `min_of_linear`): shift b by a L at the call site.
- Josephus with k = 2 by halving (CSES 2164): `josephusKth` is already O(log(n)) at k = 2; a start position s adds s mod n.
- Josephus with a variable step, k = 3 closed forms, the Halbeisen–Hungerbühler matrix method: no contest catalog, high-precision constants, or an unproven running time.
- `convergentAt` (cp-algorithms, k-th convergent of a periodic expansion): the period's 2 x 2 matrices raised with Core Matrix; row 34 `pellSolutions` covers Pell.
- `compareContinuedFractions` (cp-algorithms): compare the convergents' rationals at the call site. (returned by P275 2026-11-10 into row 20)
- Linear fractional transformations and Gosper arithmetic (cp-algorithms): no contest library implements them.
- `approximateReal` (KACTL `approximate`): a decimal input is the exact rational x / 10^k; take the nearer side of `bestRationalApproximation`.
- Stern–Brocot depth, distance, isAncestor, moveTo, order comparisons and auxiliary trees (suisen, maspypy, Nyaan): depth is the sum of the encoded counts; the rest combine `sternBrocotRange`, `sternBrocotLca` and `sternBrocotAncestor`.
- Calkin–Wilf tree and Stern's diatomic sequence (OI Wiki, cp-algorithms): expository only, no catalog implementation.
- The length of F_n: row 31 `phiSum` plus one, or `countRationalsInRange(n, 0, 1, 1, 1) + 1`.

## P054 research decisions (2026-10-19)

User decisions (2026-10-19): row 12 keeps `stirling1Row`, `stirling2Row`, `eulerianRow` and `partitionCount` as intended duplicates of Core Poly; `factorialPowerConvert` left row 12 for row 22 `factorialBasisConvert`; `tetrationMod`, `dyckPaths`, `pathsBelowDiagonal` (general endpoints) and `inclusionExclusion` (pruned search) stay; `binomialSmallPrimeOffline` became `BinomialSmallPrimeTable`; `stirling1SmallPrimeLargeN`, `stirling2SmallPrimeLargeN` and `stirlingQueries` became `StirlingSmallPrime`. Refined in the package: `qBinomial` and `qFactorial` are members of `QCombinatorics`; `motzkin (table and nth)` is `motzkinTable` plus `motzkinPaths` between heights; `increasingSequencesBetweenBounds` moved to row 67 beside the rectangle transfer it uses; `ballotGeneral` is the slope-mu ballot (cycle lemma) and `pathsBelowLine` the integer-slope count from any start (Korolyuk), because no floor-sum method counts paths; `zeroPowerZeroConvention` is a contract. Adopted from the sweep: `binomialRowCompositeMod` (suisen), `BinomialPrefixSumTable` (maspypy), the prefix-sum weight r (suisen), `boundedStarsBars` (OI Wiki), `powerTowerCapped` (suisen, SPOJ POWTOW), `gridPathsBoundaryTransfer` (hitonanode) and `bizleyPaths` (Bizley, arXiv 1606.08412). Left out, one line each:

- `binomialPrefixSumMultipoint` (ei1333, arbitrary field x): its general form, prefix products of a polynomial sequence at many points (maspypy `multipoint_evaluation_for_prefix_product`), was adopted on 2026-11-08 into row 51 as `multipointPrefixProductEval`; integer n is Mo's or the table, one large x is `binomialPrefixSum`.
- `simplePermutations` (maspypy): -[x^n] f^(-1)(x) - 2 (-1)^n for f = sum n! x^n, a correction of Core Poly `compositionalInverse`.
- `partitionsExactParts`, `distinctPartsExactParts` (OI Wiki): adopted on 2026-11-08 into row 12 as `partitionsExactPartsTable` and `distinctPartsExactPartsTable` (recurrence, any ring); the reduction through `partitionsBoundedParts` needs a prime modulus > n.
- `binomialSmallKOffline` (suisen): row 37 `binomialLargeIndex` and row 05 `combiLarge`.
- Modular subset-sum feasibility and floating harmonic numbers (maspypy, hitonanode): knapsack and row 39. Log C(n, r) was adopted on 2026-11-08 into row 05 as `LogCombinatorics` (Nyaan `FloatBinomial`, hitonanode `lognCr`).
- Infinite towers and pentation (a↑↑↑n mod m): a↑↑b stops changing once b passes the chain length; no catalog implements them.
- Exact preperiod reduction (suisen) and order-based periods: other routes to `tetrationMod`; row 11 owns multiplicative orders.
- Comparing two towers exactly: no catalog implementation; `powerTowerCapped` compares with a bound.
- Sato's matrix formula and kernel-method step sets (Krattenthaler 10.5): rational boundaries go through `increasingSequencesBetweenBounds` with heights floor((a x + b) / c); bounded-height DP or row 15 covers step sets.
- Schroeder paths between heights: same reflection as `motzkinPaths` with flat steps of length 2; no catalog.
- Strict Bizley counts (touching only at the endpoints): 1 - 1 / F of `bizleyPaths`.

## P058 research decisions (2026-10-28)

User decisions (2026-10-28): `rref` stays and `gaussJordan` left row 16; `nullspace` stays beside the basis of `solveLinear(A, 0)`. Adopted from the sweep: `luSolve` (maspypy `solve_linear_lower`; reuses `luDecompose`), `FieldBasis` (maspypy `linalg/basis`, OI Wiki basis), and the named `Mat` operations `identityMatrix` and `matrixProduct` (Library Checker `matrix_product`). `determinantInteger` is Bareiss: Euclidean reduction over Z has no polynomial bound on entry growth. Left out, one line each:

- `triangularSolve` (maspypy): `luSolve` on a factorization is the reusable form; a triangular A is solved by `solveLinear` or by `luSolve` after `luDecompose`.
- `hessenbergSolve` (hitonanode): adopted on 2026-11-08 into row 44 beside `hessenbergReduce`.
- `maximumWeightBasis` (maspypy): a weighted linear matroid basis, a candidate for Data Structures row 46 `LinearMatroid`.
- `rankAllColumnSubsets` (maspypy): one catalog, no judge problem.
- `solveLinear2` (KACTL, determined variables): variable j is determined iff column j is zero in every `nullspace` vector.
- `diagonalizeTriangular` (maspypy): a special case of Core Matrix `jordanForm`.
- `mcrt` (PyRival, a different modulus per row): integer linear algebra, a candidate for row 49 `modularSolve`.

## P059 research decisions (2026-11-03)

Scope accepted as a user decision 2026-11-03. Adopted from the sweep: `ceilKthRoot` (suisen `kth_root_round`), `newtonSafeRoot` (Numerical Recipes `rtsafe`), `doubleExponentialIntegrate` (tanh-sinh, Numerical Recipes 4.5). `DualNumber` lists sub and neg beside add, mul and div, and `finiteDifference` takes any derivative order up to 60. `horner` left row 17 as `polyEval` of row 14 (another name, `/duplicate-sweep` 2026-10). `ikthRoot` returns the floor only (user decision 2026-11-03); the remainder of a k-th root is `n - r^k`, and `isqrtRemainder` keeps the k = 2 form (GMP `mpz_sqrtrem`). Left out, one line each:

- `riddersRoot` (Numerical Recipes 9.2): a bracketing method dominated by `brentRoot`. `falsePositionRoot` (Illinois) was adopted on 2026-11-08 into row 17 as lighter than `brentRoot`.
- Adaptive Gauss–Kronrod or Gauss–Lobatto (Numerical Recipes 4.7): `simpsonAdaptive` and panelled `gaussLegendreIntegrate` cover adaptive and high-order quadrature.
- `trapezoidIntegrate` and the open midpoint rule (Numerical Recipes 4.2, 4.4): the base level of `rombergIntegrate`; endpoint singularities go to `doubleExponentialIntegrate`.
- Infinite-range exp-sinh and sinh-sinh rules: substitute `x = t / (1 - t^2)` (or `x = a + t / (1 - t)`) into `doubleExponentialIntegrate`.
- `aitkenDelta2`, `wynnEpsilon`, Euler's series transform (Numerical Recipes 5.3): sequence acceleration with no contest use beside `richardsonExtrapolate`. (returned by P275 2026-11-10 into row 74)
- `hornerDerivatives(k)` (Numerical Recipes `ddpoly`): higher derivatives come from nested `DualNumber<DualNumber<double>>` or row 14 `polyDerivative`. (returned by P275 2026-11-10 into row 17 `hornerDerivativeTable`)
- Elementary functions of `DualNumber` (sin, exp, sqrt): the chain rule is one line at the call site, `{f(x), f'(x) * dx}`. (returned by P275 2026-11-10 into row 17)
- `kahanSum` stays beside the never-worse `neumaierSum`: a different recurrence (row operation, kept).

## Omission sweep 2026-11-08

Left out by the 2026-11-08 sweep (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded decisions): exact duplicates, second implementations and items another row owns. Handoff notes name where a candidate is implemented instead.

- `newtonSystem` in row 17 (NR): moved to row 69 `continuous_optimization`, which lists it; row 17 stays scalar (user decision 2026-11-03).
- `BinaryQuadraticForm: compose (NUCOMP), square (NUDUPL)` (FLINT): a second implementation of row 34 `compose` with the same results, which already covers the whole `lng` domain through `__int128`.
- `coefOfRationalFps2d` in row 50 (MAS): the same operation as row 40 `bivariateRationalCoefficient`, which owns it.

## P275 re-review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per operation, 2026-11-08), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). The items below returned to rows and supersede the earlier bullets that name them; every other omission recorded here stands ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)).

- `01-mod_arithmetic.hpp`: floorDivStrict, ceilDivStrict; divMod.
- `02-search_algorithms.hpp`: binSearchRealGeometric (geometric-mean bisection).
- `03-equation_solvers.hpp`: solveDioMinLinear (minimum x + y).
- `04-sieve_algorithms.hpp`: SieveOfErath::primeCount, LinearSieve::primeCount.
- `05-combinatorics.hpp`: PrimeCombinatorics::combiNegative, PrimeCombinatorics::combiInverse; primorial (caller-passed big integer).
- `07-primality_factorization.hpp`: lcmMod (values, m); divisorProductMod; coprimeFactorization (basis with exponents).
- `10-crt.hpp`: garnerExact (caller-passed big integer).
- `11-discrete_log_root.hpp`: kthRootModPrimeAll.
- `16-linear_algebra.hpp`: solveLinearDetermined (determined variables).
- `17-numerical_methods.hpp`: doubleExponentialIntegrateInfinite (exp-sinh, sinh-sinh); hornerDerivativeTable; DualNumber::sqrt, DualNumber::exp, DualNumber::log, DualNumber::sin, DualNumber::cos.
- `18-floorsum.hpp`: forEachCeilQuotientBlock; enumerateQuotientsCount; floorSumInRange, minOfModOfLinearInRange; minOfModOfLinearSegments.
- `20-continued_fraction.hpp`: PeriodicContinuedFraction::convergent (k); compareContinuedFractions; bestRationalApproximationReal; sternBrocotDepth, sternBrocotDistance, sternBrocotIsAncestor, sternBrocotMove, sternBrocotComparePreOrder, sternBrocotComparePostOrder; fareySequenceCount; calkinWilfParent, calkinWilfChildren, sternDiatomic.
- `24-transform_algorithms.hpp`: transformPolynomialEval.
- `30-modular_power_towers.hpp`: infiniteTetrationMod.
- `67-lattice_paths.hpp`: bizleyPrimitivePaths.
- `31-summatory_functions.hpp`: coprimePairCount (Möbius floor sum).
- `33-modular_square_roots.hpp`: kthPowerResidueCount; kthRootModCompositeAll.
- `34-quadratic_congruence.hpp`: eisensteinNormCount; sumOfSquaresAll (n, k).
- `36-integer_factor_advanced.hpp`: nextPrimeBig, prevPrimeBig (big integer).
- `39-special_functions.hpp`: harmonicNumberApprox.
- `40-generating_functions.hpp`: fpsComposeExpMinusOne, fpsComposeOneMinusExp, fpsComposeLogOneMinusX; simplePermutationTable.
- `48-combinatorial_linear_algebra.hpp`: matrixRankAllColumnSubsets.
- `52-relaxed_convolution.hpp`: orConvolutionOnline (online subset zeta).
- `62-ball_arithmetic.hpp`: partitionCountExact (Hardy–Ramanujan–Rademacher).
- `63-multiprecision_float.hpp`: XDouble: +, -, *, /, <, ==, sqrt, log, exp, power, mulAdd.
- `65-convolution_specialized.hpp`: convolutionLeq; divisorMobiusSparse; convolutiveTranslation2d.
- new Esoteric row `72-continued_fraction_arithmetic.hpp`: continuedFractionLinearFractional; continuedFractionBihomographic (Gosper) (OI).
- new Esoteric row `73-quadratic_diophantine.hpp`: quadraticDiophantine (binary quadratic); legendreTernary (SYMPY).
- new Esoteric row `74-sequence_acceleration.hpp`: aitkenDelta2; wynnEpsilon; eulerSeriesTransform (NR).
- new Esoteric row `75-number_theory_problems.hpp`: CoprimePairFinder: enumerate, remove; placingBishopsCount (MAS, CPA, CF).
- maspypy `all_lcm` and `find_coprime_pair` were not folded into rows 08 and 34: they return as row 07 `lcmMod` and row 75 `CoprimePairFinder`; NTL `xdouble` is an extended-exponent double, not `DoubleDouble` (row 63 `XDouble`); maspypy's relative real search bisects by the geometric mean (`binSearchRealGeometric`); `simplePermutations` is its own online-convolution algorithm.
- Not returned after the P275 review: `findPrimeFactor` (`pollardRhoBrent` with `isPrime` answers it, a form between two operations), `xnorConvolution` (the index complement of `xorConvolution`); `divMod` and `orConvolutionOnline` replace the proposed `divModFloor` and `onlineOrConvolution`.