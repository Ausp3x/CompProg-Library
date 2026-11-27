# 05 Mathematics — Notes

This file holds the folder-wide contracts, the ownership boundaries, the notes of rows not yet verified and the omissions each package recorded, each with its reason. The inventory lists the operations.

## Domains

- Domains are part of the contract: finite fields, composite residue rings, integers or rationals and approximate reals are not interchangeable. Every operation states its integer widths and overflow, its modulus assumptions (prime, unit), characteristic, coefficient bounds, empty, zero and singular cases, and its legitimate failures.
- Signed 64-bit inputs include the minimum; wide (128-bit) results keep the magnitude 2^63 from narrowing.
- Factorial tables in `05` require a prime modulus and `n < mod`. Large-index and composite-modulus binomials belong to `12` and `37`, except row 05 `combiLarge` for small k.
- Randomized operations take retry budgets beside the seed, and approximate methods state their conditioning (`03-cpp.md`, API Contracts). Row `16` names its exact and approximate pivoting operations separately.
- Prime-field roots and logs are Advanced (`11`); prime-power and composite roots (`33`) and large-order discrete-log techniques (`54`) are separate Esoteric families.
- Fast composition, factorization, multivariate and polynomial-matrix families stay in their own Esoteric headers instead of mixing tiers into `14`.
- Integer roots are canonical in `21` (`55` calls them); inverse tables live in `09`; divisor and multiple zeta and Möbius transforms over [1, n] live in `24`, and over the divisors of one n in `08` `DivisorArray`.

## Ownership Boundaries

- Core `16-poly.hpp` owns full composition and compositional inverse; `14` keeps scalar duplicates and `40` Lagrange inversion as coefficient extraction.
- Core `10-matrix.hpp` owns the characteristic and minimal polynomials and the Frobenius and Jordan forms; `44` keeps the scalar Hessenberg, Berkowitz and black-box algorithms.
- Core `14-sparsematrix.hpp` keeps its fixed-iteration `steadyState`; `26` owns the stationary distributions that test irreducibility.
- The XOR linear basis is Data Structures `062-xorbasis.hpp`; Core `12-bitmatrix.hpp` owns packed GF(2) subspaces.
- Lattice counts inside polygons belong to Geometry `019-lattice_geometry.hpp`; `34` and `55` own only the arithmetic circle and curve counts.
- `27` owns the Monge, Knuth and Aliens DP optimizations. Miscellaneous `034-slope_trick.hpp` owns slope trick, and Data Structures `021-lichao.hpp` and `022-convexhulltrick.hpp` own the line containers `27` bridges to.
- Graph-facing counting (LGV, Tutte) is Graphs `053-graph_counting.hpp`. `23` owns the matrix-tree counts, BEST included (user decision 2026-11-08); the kernels stay in `23`, `48` and `67`.
- Strings owns distinct-subsequence counting (`019-lcs.hpp`, `020-subsequence_automaton.hpp`) and de Bruijn sequences (`041-debruijn.hpp`). Miscellaneous owns Gray codes (`006-bit_operations.hpp`), random sampling (`018-randomized_algorithms.hpp`), knapsack (`13`, `29`), digit sums and base conversion (`031-digitdp.hpp`) and parallel binary search (`20`).

## Legacy Obligations

All originals stay in `OLD`; each evidence document maps its legacy names.

- `OLD/5-Mathematics/01-exgcdandmodpow.hpp` to `04-sieves.hpp` and `12-combinatorics.hpp`: rows `01`–`06` (P012).
- `05-primesandfactors.hpp`, `06-phiandinverse.hpp`, `07-chiremthmandgarner.hpp`: the verified rows `07`–`10` (P052).
- `08-modlogandmodroot.hpp`: rows `11` (P053) and `33` (P178, `modRoot` modulo prime powers and composites).
- `14-miscellaneous.hpp`: rows `18` and `19` (P060; `getFloorSum` is `floorSum`, the recursive `josephus` is `josephus`).
- `13-numericalmethods.hpp` and `OLD/Team Notebook/src/math/simpsoninteg.hpp`, `issquare.hpp`: rows `17` and `21` (P059).
- `OLD/Team Notebook/src/math/old_gaussjordan.cpp`, `old_matrix.cpp` and the `Matrix` of `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp`: row `16` (P058). `00-matrix.hpp` is shared with Core Matrix (P020), which owns its arithmetic and `FastMatMul`.
- `OLD/5-Mathematics/09-montgomery.hpp` is a Core reference; `10-poly.hpp` and `11-moly.hpp` are references for Core Poly.
- The former `97-Legacy` excerpts `ModFac` (`OLD/[1] algorithms.cpp`) and `FastConv` (`OLD/algorithms.cpp`): rows 05 and 24; provenance in the [transfer record](../../00-Guidelines/History/2026-09-27-monolith-transfer.md).

## Evidence Rules

- Check small signed domains exhaustively against references. Check factor products and primality, equation substitution, CRT residuals, convolution against the O(n m) product, interpolation reconstruction, normal-form identities, root intervals and algebraic invariants.

## Row Notes

Explanations moved out of the Operations cells of rows not yet verified, one bullet per row in inventory order; verified rows keep theirs in their evidence documents.

- `022-interpolation.hpp`: lagrangeInterpolate: O(n^2); lagrangeEvaluate: value at x from points; lagrangeEquidistant: points 0..n, O(n); hermiteInterpolate: repeated points, derivative jets; productOfGeometricShifts: product of f(r^k x) (named apart from row 37's `productOfLinearShifts`).
- `023-matrixtree.hpp`: eulerianCircuitCountBest: BEST bridge.
- `025-game_theory.hpp`: grundy: memoized game DAG; subtractionGame: period detection with proof bound; wythoff: cold positions; retrogradeAnalysis: win, lose, draw on a finite game graph.
- `027-optimization.hpp`: divideConquerDp: monotone opt; knuthDp: quadrangle inequality; smawk: totally monotone row minima; larsch: online Monge; aliensTrick: Lagrangian relaxation, integer penalty, count tie rule; mongeCheck: quadrangle inequality on small instances; mongeShortestPath: unconstrained O(n log n) with predecessors and edge count, exactly d edges or a count in [L, R] by penalty binary search; convexHullTrickDp: bridge to Data Structures `022-convexhulltrick.hpp`; liChaoDp: bridge.
- `029-polynomial_roots.hpp`: sturmCount: real roots in an interval; realRootIsolate: rational intervals, square-free; realRootsApprox, realRootsApproxSimple: tolerance; polyRootsApprox: residual.
- `031-summatory_functions.hpp`: FloorBlockTable: quotient-indexed memo; dujiaoSieve: prefix of f from a Dirichlet relation; mobiusSum: Mertens; min25Sieve: prime prefix sums then multiplicative prefix; lucyHedgehog: prime counting O(n^(3/4)); primeCount: Meissel–Lehmer, practical to about 1e13 (OI Wiki gives O~(n^(2/3)) only to LMO); primeCountTable: the whole pi(n / i) table, O(n^(2/3) log^(1/3) n); lmoPrimeCount: O(n^(2/3) log^(1/3) n) time, O~(n^(1/3)) space segmented; delegliseRivatPrimeCount: the LMO refinement with a smaller log factor; divisorSummatory: sum of d(i); countByFactorType: needs prime counting, so it sits here (P052).
- `032-dirichlet_series.hpp`: DirichletSeries: truncated prefix representation on quotient blocks; eulerProduct: from prime-power coefficients.
- `035-lattice_reduction.hpp`: gramDeterminant: the Gram determinant (squared covolume) that Gram–Schmidt yields; row 49 owns `latticeDeterminant`; lllReduce: delta parameter; bkz: block size parameter; pslq: integer relation, precision parameter.
- `036-integer_factor_advanced.hpp`: ecm: curve budget; bpsw: probable-prime screening; pocklington: certificate; ecpp: certificate or unknown status.
- `037-fastfactorial.hpp`: factorialModPrime: O(sqrt(p) log p); factorialLarge: many queries, sorted block products; consecutiveProducts: product of (x + i), multipoint.
- `038-bernoulli.hpp`: bernoulliTable: B1 sign convention option; faulhaber: sum of i^k as a polynomial; powerSum: sum of i^k for i < n mod p; sumOfExponentialTimesPolynomialLimit: infinite formal sum; mixedPolyExpSum: polynomial given by samples.
- `040-generating_functions.hpp`: eulerTransform: multiset construction (MSET; `unlabelledMultiset` removed 2026-11-08 as its alias); logarithmicTransform: connected structures; burnsideCount: orbits from fixed-point counts; polyaEnumeration: cycle index evaluation; lagrangeInversion: coefficient of the compositional inverse; coefficientExtraction: nth coefficient of a rational function, wrapping row 15 `bostanMori` (user decision 2026-11-08); rookPolynomial: board; fpsLinearOde: first-order formal differential equation; fpsComposeExp: f(e^(cx)), while the e^x - 1 case stays row 12 `stirlingTransform`; simplePermutationTable: its own online-convolution algorithm (maspypy `simple_permutations`).
- `041-padic_arithmetic.hpp`: lte: lifting the exponent; henselLiftRoot: polynomial root to precision.
- `042-finite_fields.hpp`: findIrreducible: random, degree d; conwayPolynomial: table bridge; fieldIsomorphism: embedding between representations; basisConvert: polynomial to normal basis; GF2k::carrylessMultiply wraps row 65 `carrylessMultiply` (user decision 2026-11-08).
- `043-rationalreconstruction.hpp`: rationalReconstruct: x mod m with bounded numerator and denominator; rationalReconstructVector: common denominator; crtReconstructRational: from modular images; errorTolerantReconstruct: k wrong residues.
- `044-determinant_advanced.hpp`: berkowitz: division-free characteristic polynomial; hessenbergReduce: similarity; charPolyHessenberg: O(n^3); determinantAPlusXB: polynomial determinant; wiedemannDeterminant: black box, Monte Carlo; vandermondeDeterminant: product of differences; determinantPolynomialMatrix wraps row 51 `PolyMatrix::det`, which owns the evaluation-and-interpolation algorithm (user decision 2026-11-08).
- `045-contourintegral.hpp`: contourIntegrate: exponential convergence; argumentPrincipleCount: roots inside a disc; poleExclusion: exclusion discs; cauchyCoefficient: nth Taylor coefficient by contour.
- `046-polynomial_factorization.hpp`: equalDegreeFactor: randomized; rootsModPrime: all roots, randomized splitting.
- `047-nimber.hpp`: nimMulTable: small width precomputed; nimSolveQuadratic: x^2 + a x + b = 0; nimToField: basis conversion to GF(2^(2^k)) polynomial basis.
- `048-combinatorial_linear_algebra.hpp`: permanentRyser: Gray-code inclusion-exclusion; hafnian: O(2^n n^2); cauchyBinet: determinant-sum identity; perfectMatchingCount: Monte Carlo.
- `049-integer_linear_algebra.hpp`: hermiteNormalForm: with unimodular transform; smithNormalForm: with left and right unimodular transforms; integerKernel: lattice basis of the Z-nullspace; integerSolve: one solution plus full solution lattice over Z; modularSolve: A x = b mod m, full solution set; solveDioEqMulti: its solution set is a rank `n - 1` lattice, and chained `extendedGcd` coefficients overflow 128 bits without HNF size reduction (P012 MA01).
- `050-multivariate_polynomial.hpp`: mpolyDivision: division with remainder.
- `051-polynomial_matrix.hpp`: approximantBasis: minimal approximant or order basis; pRecursiveNthTerm: product M(0)...M(n-1); findPRecursive: order r, degree d from samples; reedsSloane: recurrence synthesis modulo composite; matrixBerlekampMassey: vector sequence.
- `052-relaxed_convolution.hpp`: SemiRelaxedConvolution: one operand known; dcDifferentialEquation: divide and conquer f' = g(f); powerProjection: weighted coefficients of f^k for all k; powerEnumerate: coefficients of f^k.
- `053-semiring_convolution.hpp`: minPlusConvolutionConvexConvex: merge, O(n); minPlusConvolutionBoundedRange: small value range.
- `054-discrete_log_advanced.hpp`: pollardRhoLog: randomized, expected sqrt; compositeGroupDecompose: unit group of Z/m as product of cyclic groups.
- `055-floor_sum_polynomial.hpp`: floorSumPolynomial: sum of i^a floor((p i + q)/m)^b for all a, b up to a bound; floorProduct: Euclidean product sums; floorMonoidProduct: noncommutative monoid of U and R steps, generic Euclidean; latticePointGeneratingFunction: rational generating function of lattice points under a line; convexFloorSum: sum of floor of a convex function; convexLatticeHull: convex hull of lattice points under a curve; gaussCircleCountHull: O(r^(2/3)).
- `056-coding_theory.hpp`: listDecodeGuruswamiSudan: radius and list-size parameters.
- `058-partizan_games.hpp`: zeroSumMatrixGame: mixed strategy by LP, value; misereOutcome: small game graph search.
- `059-zero_sum.hpp`: egzWitness: n of 2n - 1 elements summing to 0 mod n, constructive; zeroSumSubsequence: from any n elements, prefix sums; SidonSequence: n terms below p(p - 1) by CRT, O(p) build; query maps a sum to its index pair in O(1).
- `060-tableau_algorithms.hpp`: hookLengthCount: standard Young tableaux; hookContentCount: semistandard with bound; rsk: permutation to tableau pair; rskWord: two-line arrays; standardTableauEnumerate: bounded.
- `061-algebraic_numbers.hpp`: AlgebraicNumber::approximate: to precision.
- `062-ball_arithmetic.hpp`: Ball: midpoint plus radius; Ball::rationalInInterval: simplest rational inside the ball, wrapping row 20 `rationalInInterval`; intervalNewton: existence and uniqueness certificate; intervalBisect: root isolation; precisionEscalate: retry at higher precision via `063-multiprecision_float.hpp`.
- `063-multiprecision_float.hpp`: fromDecimalString: correctly rounded; toDouble: correctly rounded; bigPi: stated ulp bound; XDouble: NTL `xdouble`, an extended-exponent double, not `DoubleDouble`.
- `064-group_algorithms.hpp`: PermutationGroup::schreierSims: base and strong generating set; PermutationGroup::enumerateElements: bounded; orbitEnumerate: group action on objects; abelianDecompose: generators and relations; kthRootPermutation: p with p^k = q.
- `065-convolution_specialized.hpp`: convolutionGf2_64: binary extension field, carryless; multiplicativeConvolutionModPrime: index product mod p via primitive root; multiplicativeConvolutionMod2k: index product mod 2^k; karyXorConvolution: digit-wise addition mod k, kth roots of unity or the cyclotomic ring Z[x]/Phi_k when the modulus lacks them; schonhageStrassen: large exact integer convolution, reference.
- `066-set_power_series.hpp`: spsPolynomialComposite: f(S) for polynomial f, O(2^n n^2); spsComposition: composition with an FPS; spsMultivariate: several set variables; connectedComponentsBySubset: log of the subset generating function.

## P012 Research Decisions (2026-10-07)

Left out:

- `gcd128` (maspypy): GNU `std::gcd` accepts `__int128` directly, so no wrapper is needed.
- Binary gcd (hitonanode, Nyaan): the installed libstdc++ `std::gcd` is already Stein's algorithm, and `gcd64` uses it.
- `firstModInRange` (maspypy `first_mod_range_of_linear`): row 18, beside `minOfModOfLinear`.
- Factorization above the sieve bound (hitonanode `Sieve::factorize`): row 07 `factorize`.
- `powTable`, the completely multiplicative i^K table (hitonanode `enumerate_kth_pows`): row 08 `multiplicativeTable`.

## P012 MA01 Research Decisions (2026-11-12)

Adopted into row 01: `FixedBasePower` (OI Wiki 光速幂, sqrt tables; maspypy `Power_Query`, base-B digit tables for any exponent), as one type with `k` digit levels, `k = 2` being the sqrt form.

Adopted into row 02 after `@reviewer`: `binSearchRealBits`, bisection on the order-preserving integer image of the doubles (the 2026-10-07 sweep had mapped it to `binSearch`). It takes any finite bracket, zero and mixed signs included, in at most 64 reductions. Geometric bisection needs one sign and takes from 51 fewer to 2 more reductions: up to 51 fewer on `[denorm_min, DBL_MAX]` and 38 fewer on `[denorm_min, 1e-310]`, where the bit pattern is linear in the subnormals, and at most 2 more elsewhere (a `@reviewer` scratch run of 20,000 thresholds per bracket, not kept as a command). Both stay: Bits is wider, Geometric faster on ranges reaching into the subnormals.

Left out:

- `modMulLongDouble` (KACTL `ModMulLL`): a floating-point Barrett reduction (`01-principles.md`, Profiles: the 20% benchmark rule outside Core), valid only for `m < 2^63` with an 80-bit `long double`; Core `03-barrett.hpp` and `04-montgomery.hpp` own fast fixed-modulus products.
- Cubic interpolation of values and derivatives (Davidon; NR 10.3 discusses and rejects it, with no routine): not lighter, wider or more capable than `brentSearchRealBracketDerivative`, and no measured crossover.
- maspypy `binary_search_real_abs_rel` (mixed absolute and relative tolerance): `binSearchRealGeometric` with a `denorm_min` endpoint and `abs_tol` covers it.

User decisions of 2026-11-12, made after the batch was first finished:

- Adopted into row 02: `brentSearchRealBracketDerivative` (NR `dbrent`), fewer evaluations when `f'` is cheap.
- Adopted into row 03: `countNonRepresentable` (Selmer's gap count over the round-robin table) and the two-coin closed forms `frobeniusNumberSylvester`, `countNonRepresentableSylvester`.
- Planned in row 18 (MA10): `countRepresentableSylvester(a, b, x)`, the number of representable `t` in `[0, x]` for two coins, in O(log). For coprime `a, b` and `x < a * b`, each `t` has at most one form `a * i + b * j` (arXiv 1910.03079, Corollary 12), so the count is `sum over i <= x / a of floor((x - a * i) / b) + 1`, one `floorSum`. For `x >= a * b - a - b` it is `x + 1 - (a - 1) * (b - 1) / 2`. A common `g = gcd(a, b)` divides out with `x / g`. Row 03 cannot include `018-floorsum.hpp`, because MA10 depends on MA01.
- Planned in row 69: `bracketMinimum` (NR `mnbrak`, NR3 `Bracketmethod`: `GOLD = 1.618034`, `GLIMIT = 100`, `TINY = 1e-20`), downhill bracketing on an unbounded domain for the closed-bracket minimizers.
- Planned in row 49: `solveDioEqMulti` (OI Wiki, n-variable Bézout); its reason is in Row Notes.

A supplementary `@researcher` sweep the same day read what the first had left unread (maspypy `mod/`, `nt/` and `other/` in full, Nyaan and ei1333 past their first 100k characters, Library Checker `number_theory`, NR3 `mins.h`) and found nothing missing for rows 01–03. Follow-ups for other rows' owners: row 54, a table of `pow(a, e)` through the discrete log (maspypy `ModFast`); row 20, confirm that `sternBrocotRange` gives maspypy `farey_lr`'s Farey neighbours; Miscellaneous row 44, Nyaan's branch-light `fast_lb` on a sorted array, if adopted at all.

User decision of 2026-11-13, in `/finish`: row 03 adopts `residueShortestPathDijkstra` (OI Wiki 同余最短路), Dijkstra on the residues in O(k * m * log(k * m)), slower and lighter than the round robin. At 14 lines against 18 it falls short of the half-the-lines test of `01-principles.md` (Priority), so it is a recorded exception (`23-Sweep Ledgers/omissions.md`).

## P052 Research Decisions (2026-10-09)

Left out:

- `factorizeSpf` and `divisorsSpf` hybrids (maspypy): `LinearSieve::factorize` below the table, `factorize` above; `isPrimeSpf` is the only hybrid worth a name.
- `findPrimeFactor` (maspypy): `factorize(n)[0].fi`; P275 confirmed it, since `pollardRhoBrent` with `isPrime` answers it, a form between two operations.
- `fermatFactor` (cp-algorithms): fast only for close factors; rho dominates.
- `fermatProbablePrime` (OI Wiki): Monte Carlo and strictly weaker than deterministic Miller–Rabin.
- `DivisorArray` with a user semigroup (product-form Möbius): `T` with `+` and `-` covers the transforms; the product form is `exp` and `log` of a modint table at the call site.
- `inverseEuler`: `a^(phi(m) - 1)` needs a factorization and is dominated by `inverseXgcd`.
- `crtFixedPrimes` (maspypy `crt3`): owned by the arbitrary-modulus convolution in Core and row `13`.

## P055 Research Decisions (2026-10-09)

Left out:

- `convolutionSquare` (Nyaan, maspypy): `convolutionNtt(a, a)` detects the aliasing and does one forward transform.
- Transposed `nttDoubling`, 64-bit-prime NTT products and `convolution_all`: owned by Core Poly, row 65 and row 14 `productTree`.
- Split-FFT arbitrary modulus (KACTL `convMod`) and the real two-for-one trick: implementation alternatives of `convolutionArbitraryMod` and `convolutionFft`; `convolutionFft` uses the latter inside.
- Many-operand pointwise product (suisen): an implementation detail.
- Zeta with a custom semigroup (max or min over subsets): a two-line loop at the call site, or `kroneckerPowerTransform` over a semiring type with `+` and `*` redefined.
- XNOR convolution: `xorConvolution` with complemented indices, which P275 confirmed; `xor_submask_lower_bound` is a bit trick for Miscellaneous.

## P057 Research Decisions (2026-10-10)

Left out:

- FPS trigonometric and hyperbolic functions (Nyaan `fps-circular`, OI Wiki): owned by Core Poly (`sin` … `atanh`).
- Pseudo-division, polynomial lcm, `tryDivide`, reversal, inflate and deflate: integer-ring or trivial wrappers. Core Poly owns `pseudoDiv`; row 46 owns content and primitive parts.
- Polynomial power without a modulus: `fpsPow(a, k, k deg a + 1)`, stated in the contract.
- Graeffe step and power projection: internal to `bostanMori`; `powerProjection` is row 52.
- Products of `(1 - x^{a_i})` and `(1 + x^{a_i})`: rows 12 and 40. Shift of sampling points: row 22 `samplePointShift`.
- Rational `fpsPow` with a non-unit lowest coefficient: it needs a `q`-th root from row 11 (`kthRootModPrime`); the contract asserts constant term 1.
- Recurrences with a polynomial inhomogeneous term, and weighted prefix sums `sum a_i w^i`: call-site reductions documented in `015-linear_recurrence.md` (multiply `Q` by `(1 - x)^(deg + 1)`; substitute `c_j w^j`, `a_i w^i`).
- Reeds–Sloane and P-recursive interpolation (maspypy `interpolate_p_recursive`): row 51. Wiedemann and sparse determinants: row 44. `interpolate_poly_exp_sum`: row 38.
- Multiple k-th term queries for one recurrence: no catalog interface beats one `bostanMori` per query.
- Overlaps recorded rather than moved: row 46 keeps the integer (subresultant) resultants and discriminants, row 14 the field Euclidean ones; row 22 `subproductTree` keeps the multipoint tree, row 14 `productTree` only the product.

## P053 Research Decisions (2026-10-16)

Left out:

- Least positive discrete log (KACTL `modLog`, PyRival): `discreteLogEx(a, b, m, k * a mod m) + 1` when that is not -1.
- Precomputed-modulus order object (suisen `OrderMod`): one factorization of `lambda(m)` per query costs O(m^(1/4)); many discrete-log queries for one prime and base are row 54 `DiscreteLogTable`.
- Lifting primitive roots to `p^e`, and random primitive roots (maspypy, suisen): internal choices of `primitiveRoot`.
- An element of maximal order `lambda(m)` (OI Wiki): row 54 `compositeGroupDecompose`.
- Square-root special cases (the p = 3 mod 4 power, Atkin, Legendre's algorithm): dominated by Cipolla and Core `trySqrt`, which already takes the p = 3 mod 4 power.
- Nonresidue search and the order-of-a-power formula: internal loops and one-line formulas.

## P178 Research Decisions (2026-10-17)

Adopted: `BinaryQuadraticForm::reducedForms` (class-group enumeration; `classNumber` is its size). `fourSquares` reduces to `threeSquares` (maspypy), replacing the planned randomized Rabin–Shallit four-square search: the same O(log(n)) expected primality tests, but deterministic.

Left out:

- `unitRootMod` (an element of exact order k modulo a composite, OI Wiki): the unit group is not cyclic; row 54 `compositeGroupDecompose` owns generators and orders.
- Factorization-taking overloads (FLINT `n_sqrtmod_primepow`, `n_sqrtmodn`): the prime-power operations already take (p, e).
- `pellFour` (x^2 - D y^2 = +-4, OI Wiki): `pellGeneralized(D, +-4)`.
- Stern–Brocot Pell search (maspypy `pell_equation`): the same continued-fraction path, found by comparisons.
- Brute-force and PQa generalized Pell (SymPy `diop_bf_DN`, OI Wiki): Lagrange–Matthews–Mollin is polynomial in the period and the root count, with no Nagell-bound scan.
- Indefinite forms (cycle reduction, NUCOMP): the principal indefinite form is the Pell family; general real quadratic class groups are research-level.
- Row 57 `gaussianPrimeFactor` and `fourSquaresFromHurwitz` overlap `twoSquares` and `fourSquares`; the row 57 package settles them (handoff in [034-quadratic_congruence.md](034-quadratic_congruence.md)).

## P060 Research Decisions (2026-10-18)

Adopted: `maxOfModOfLinear` (maspypy; a ruling 3 mirror), `forEachJointQuotientBlock` (OI Wiki multi-dimensional blocks, suisen), `josephusTime` (Concrete Mathematics 3.3 renumbering), `continuedFractionQuadratic` (cp-algorithms) and `fareyKth` (maspypy, OI Wiki).

Removed by user decision (2026-10-18): `floorSumUnsigned` (it is `ulng(floorSum(n, m, a, b))`), `josephusZeroBased` and `josephusOneBased` (index base), and `josephusLarge`, since `josephus` itself became the run-skipping survivor, no slower than the plain loop it replaced ([019-josephus.md](019-josephus.md)).

Left out:

- Minimum by binary search with floor sums (suisen, O(log(m)^2)): dominated by `minOfModOfLinear`.
- Josephus with k = 2 by halving (CSES 2164): `josephusKth` is already O(log(n)) at k = 2; a start position s adds s mod n.
- Josephus with a variable step, the k = 3 closed forms and the Halbeisen–Hungerbühler matrix method: no contest catalog, high-precision constants, or an unproven running time.
- Stern–Brocot auxiliary trees (suisen, maspypy, Nyaan): they combine `sternBrocotRange`, `sternBrocotLca` and `sternBrocotAncestor`.

## P054 Research Decisions (2026-10-19)

User decisions (2026-10-19):

- Row 12 keeps `stirling1Row`, `stirling2Row`, `eulerianRow` and `partitionCount` as intended duplicates of Core Poly.
- `factorialPowerConvert` left row 12 for row 22 `factorialBasisConvert`.
- `tetrationMod`, `dyckPaths`, `pathsBelowDiagonal` (general endpoints) and `inclusionExclusion` (pruned search) stay.
- `binomialSmallPrimeOffline` became `BinomialSmallPrimeTable`; `stirling1SmallPrimeLargeN`, `stirling2SmallPrimeLargeN` and `stirlingQueries` became `StirlingSmallPrime`.

Refined in the package: `qBinomial` and `qFactorial` are members of `QCombinatorics`; `motzkin (table and nth)` is `motzkinTable` plus `motzkinPaths` between heights; `increasingSequencesBetweenBounds` moved to row 67 beside the rectangle transfer it uses; `ballotGeneral` is the slope-mu ballot (cycle lemma) and `pathsBelowLine` the integer-slope count from any start (Korolyuk), because no floor-sum method counts paths; `zeroPowerZeroConvention` is a contract.

Adopted from the sweep: `binomialRowCompositeMod` (suisen), `BinomialPrefixSumTable` (maspypy), the prefix-sum weight r (suisen), `boundedStarsBars` (OI Wiki), `powerTowerCapped` (suisen, SPOJ POWTOW), `gridPathsBoundaryTransfer` (hitonanode) and `bizleyPaths` (Bizley, arXiv 1606.08412).

Left out:

- `binomialPrefixSumMultipoint` (ei1333, arbitrary field x): its general form is row 51 `multipointPrefixProductEval`; integer n is Mo's algorithm or the table, and one large x is `binomialPrefixSum`.
- `binomialSmallKOffline` (suisen): row 37 `binomialLargeIndex` and row 05 `combiLarge`.
- Modular subset-sum feasibility (maspypy, hitonanode): knapsack, owned by Miscellaneous.
- Pentation (a↑↑↑n mod m): no catalog implements it.
- Exact preperiod reduction (suisen) and order-based periods: other routes to `tetrationMod`; row 11 owns multiplicative orders.
- Comparing two towers exactly: no catalog implementation; `powerTowerCapped` compares with a bound.
- Sato's matrix formula and kernel-method step sets (Krattenthaler 10.5): rational boundaries go through `increasingSequencesBetweenBounds` with heights floor((a x + b) / c); bounded-height DP or row 15 covers step sets.
- Schroeder paths between heights: the same reflection as `motzkinPaths` with flat steps of length 2; no catalog.

## P058 Research Decisions (2026-10-28)

User decisions (2026-10-28): `rref` stays and `gaussJordan` left row 16; `nullspace` stays beside the basis of `solveLinear(A, 0)`.

Adopted from the sweep: `luSolve` (maspypy `solve_linear_lower`; reuses `luDecompose`), `FieldBasis` (maspypy `linalg/basis`, OI Wiki basis), and the named `Mat` operations `identityMatrix` and `matrixProduct` (Library Checker `matrix_product`). `determinantInteger` is Bareiss, because Euclidean reduction over Z has no polynomial bound on entry growth.

Left out:

- `triangularSolve` (maspypy): `luSolve` on a factorization is the reusable form; a triangular A is solved by `solveLinear`, or by `luSolve` after `luDecompose`.
- `maximumWeightBasis` (maspypy): a weighted linear matroid basis, a candidate for Data Structures row 46 `LinearMatroid`.
- `diagonalizeTriangular` (maspypy): a special case of Core Matrix `jordanForm`.
- `mcrt` (PyRival, a different modulus per row): integer linear algebra, a candidate for row 49 `modularSolve`.

## P059 Research Decisions (2026-11-03)

Scope accepted as a user decision (2026-11-03). Adopted from the sweep: `ceilKthRoot` (suisen `kth_root_round`), `newtonSafeRoot` (Numerical Recipes `rtsafe`) and `doubleExponentialIntegrate` (tanh-sinh, Numerical Recipes 4.5). `DualNumber` lists sub and neg beside add, mul and div, and `finiteDifference` takes any derivative order up to 60. `horner` left row 17 as `polyEval` of row 14 (another name, `/duplicate-sweep` 2026-10). `ikthRoot` returns the floor only (user decision 2026-11-03); the remainder of a k-th root is `n - r^k`, and `isqrtRemainder` keeps the k = 2 form (GMP `mpz_sqrtrem`).

Left out:

- `riddersRoot` (Numerical Recipes 9.2): a bracketing method dominated by `brentRoot`.
- Adaptive Gauss–Kronrod or Gauss–Lobatto (Numerical Recipes 4.7): `simpsonAdaptive` and panelled `gaussLegendreIntegrate` cover adaptive and high-order quadrature.
- `trapezoidIntegrate` and the open midpoint rule (Numerical Recipes 4.2, 4.4): the base level of `rombergIntegrate`; endpoint singularities go to `doubleExponentialIntegrate`.
- `kahanSum` stays beside the never-worse `neumaierSum`: a different recurrence, so a row operation and kept.

## Omission Sweep 2026-11-08

The 2026-11-08 sweep (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions) adopted every candidate except exact duplicates, second implementations and items another row owns. It adopted these items that the sections above had left out or handed off: row 05 `LogCombinatorics` (Nyaan `FloatBinomial`, hitonanode `lognCr`); row 09 `inverseO1` (Nyaan, maspypy), O(1) online queries on arbitrary values against O(log p) for `inverseXgcd` and `inverseFermat`; row 12 `partitionsExactPartsTable` and `distinctPartsExactPartsTable` (OI Wiki, recurrence over any ring, since the reduction through `partitionsBoundedParts` needs a prime modulus > n); row 17 `falsePositionRoot` (Illinois), lighter than `brentRoot`; row 40 `fpsComposeExp` (Nyaan, suisen, maspypy); row 44 `hessenbergSolve` (hitonanode); row 51 `multipointPrefixProductEval` (maspypy `multipoint_evaluation_for_prefix_product`); row 54 `DiscreteLogTable` and `indexCalculus` (suisen `IndexCalculus`).

Left out:

- `newtonSystem` in row 17 (NR): moved to row 69 `continuous_optimization`, which lists it; row 17 stays scalar (user decision 2026-11-03).
- `BinaryQuadraticForm: compose (NUCOMP), square (NUDUPL)` (FLINT): a second implementation of row 34 `compose` with the same results, which already covers the whole `lng` domain through `__int128`.
- `coefOfRationalFps2d` in row 50 (MAS): the same operation as row 40 `bivariateRationalCoefficient`, which owns it.

## P275 Re-Review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per Operation, 2026-11-08), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`). It returned 63 operations to 26 existing rows and created the Esoteric rows 72–75; their earlier omission bullets are deleted here, and every other omission stands ([evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)). The P275 name `divMod` is `divModFloor` again since 2026-11-12 (user decision), and `orConvolutionOnline` replaces the proposed `onlineOrConvolution`.

## P012 MA02 Research Decisions (2026-11-14)

Adopted into row 05 under the adopt-all policy of 2026-11-08: `doubleFactorial` and `doubleFactorialExact` (GMP `mpz_2fac_ui`; the Core modint evidence hands double factorial to this row) and `circularPermutation` (OI Wiki 圆排列). `combiNegative` follows GMP's signed `C(-n, k) = (-1)^k * C(n + k - 1, k)`; maspypy's unsigned `C_negative` is `combiWR`.

User decisions at `/finish` (2026-11-14), on omissions the package session had made without the user:

- Adopted into row 05: `multiFactorial` and `multiFactorialExact` (GMP `mpz_mfac_uiui`; `doubleFactorialExact` wraps the exact form), `combiNonAdjacent` (OI Wiki 不相邻的排列), the table-free `logFactorialLarge`, `logBinomialLarge` and `logPermutationLarge` (`lgammal`, the wider form beyond any table), and `FieldCombinatorics` (Nyaan `Binomial_rational`: factorial tables over any field where `1..n` are units, `Rational` included).
- Adopted into row 04: `SieveOfErathSpf` (maspypy `spf_table`, suisen `Sieve`), the SPF table by Eratosthenes, kept beside `LinearSieve` as faster below a measured crossover (23–35% fewer cycles from `n = 1000` to 5e4, a tie near 2e6, 6% more at 5e6) and again from 1e7 (24% fewer at 1e8). Also `OddBitSieve` (cp-algorithms, suisen, Nyaan, PyRival bit sieves), odd-only bit-packed primality with block prime counts beside the byte `SieveOfErath`, wider through its 16 times smaller table (`n` below `2^32`).
- `LogCombinatorics` builds its table by a Kahan-compensated sum of `logl(i)` instead of one `lgammal` call per entry: the same accuracy class in 45% of the cycles ([benchmarks](005-combinatorics.md#benchmarks)). Keeping both would be a second implementation of the same operation (`03-cpp.md`, One API per Operation).
- `circularPermutation(a, 0)` follows `permuNR(a, 0)` and asserts `a <= n`, so the operation composes `permuNR` instead of repeating its guards.

`FieldCombinatorics` repeats `PrimeCombinatorics`' selection guards and products instead of sharing them: `PrimeCombinatorics` checks its dynamic modulus before every early return and needs `n < mod`, so sharing the bodies would put a check hook into every operation of both structs.

Left out: wheel-30 bit storage (cp-algorithms, Nyaan). It has `OddBitSieve`'s operations and its domain below `2^32` with a smaller constant, so it would be a second implementation (`03-cpp.md`, One API per Operation).

## Omission Sweep 2026-11-27

Left out from the candidates of the 2026-11-27 catalog survey (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`; user decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions): second implementations and slower algorithms that Priority does not admit.

- Solovay–Strassen test (HAC): a Monte Carlo Euler-pseudoprime test, neither lighter, wider, faster nor more capable than the deterministic row 07 `isPrime` and row 36 `bpsw`, the ground of the recorded `fermatProbablePrime` omission (`01-principles.md`, Priority).
- Sieve of Sundaram (WIKI): O(n log n), slower than row 04 `SieveOfErath` and `OddBitSieve` on the same domain with the same results, and not half their lines (`01-principles.md`, Priority).

## Omission Sweep 2026-11-28

Left out from the candidates of the 2026-11-28 sample of the sources the user placed (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`, Sampled and Applied 2026-11-28).

- Gauss-sum and finite-field primality tests (CRANDALL): Under 03-cpp.md One API per Operation and 01-principles.md Priority they are slower forms of the cyclotomy proof that row 36 aprcl (Jacobi sums) and ecpp already give, with the same results and no wider domain.
- The Goldwasser–Kilian primality test (CRANDALL): Under 01-principles.md Priority it is a slower elliptic-curve primality proof with the same certificate output as row 36 ecpp (Atkin–Morain), and it is not lighter once row 71 schoofPointCount is counted, nor wider, faster or more capable.
- Willans's and Wormell's prime formulas (HACKERS): Under 01-principles.md Priority they are exponential-time forms of row 31 nthPrime with the same results, and they are not lighter, wider, faster or more capable.
