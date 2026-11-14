# P225 / SUP03 judge sources

P225 adds judge solutions under `src/` and exercises the expander ([06-online.md](../../00-Guidelines/06-online.md)). This document records the 2026-11-15 slice, which covers the verified Core full headers `16-poly.hpp`, `18-bitset.hpp`, `09-rational.hpp`, and `03-barrett.hpp` / `04-montgomery.hpp` used directly as the reducer of a Mathematics operation. It lists every Library Checker problem whose bottleneck is one of those headers' operations, the source written for each, and the problems left out because no header in scope is the bottleneck. Every source here is **locally compiled/run**; none is accepted online yet.

## Selection

Candidates came from `07-judge.py problems` (256 problems, Library Checker commit `73876f1cbcf0`) matched against the operations in the inventory rows and evidence documents of the headers in scope ([16-poly.md](../../01-Core/Docs/16-poly.md), [18-bitset.md](../../01-Core/Docs/18-bitset.md), [09-rational.md](../../01-Core/Docs/09-rational.md), [03-barrett.md](../../01-Core/Docs/03-barrett.md), [04-montgomery.md](../../01-Core/Docs/04-montgomery.md)). A source was written when the library operation it calls is the work the problem measures, apart from a few lines of driver (input, output and a series identity such as `B_n = n! [x^n] exp(e^x - 1)`).

- `16-poly.hpp`: 46 problems, all under `src/01-Yosupo/03-Polynomials/`.
- `04-montgomery.hpp`: `primality_test` and `factorize`, through `isPrime` and `factorize` of [`07-primality_factorization.hpp`](../../05-Mathematics/07-primality_factorization.hpp), whose Miller–Rabin and Pollard–Brent rho run on `Montgomery64::redc` and `init`. Both are under `src/01-Yosupo/04-Number Theory/`.
- `03-barrett.hpp`: no problem. No Mathematics operation takes a Barrett reducer. Inside Poly, Barrett64 only reduces the Garner output of `PolyCompanion::convolutionArbitraryMod`, and inside modint it backs the runtime modulus of `DynModInt`. Neither is a direct use.
- `18-bitset.hpp`: no problem. The mod-2 matrix problems belong to `12-bitmatrix.hpp`, and no other Library Checker task is bound by bitset word operations.
- `09-rational.hpp`: no problem. `rational_approximation`, `stern_brocot_tree`, `sum_of_floor_of_linear` and `min_of_mod_of_linear` are integer continued-fraction tasks: `bestRationalApproximation` and the floor-sum family use `lng` pairs, not `Rational`.

Problems left out because no header in scope is the bottleneck:

| Problem | Why there is no source |
|---|---|
| `sqrt_mod`, `kth_root_mod`, `discrete_logarithm_mod`, `primitive_root`, `tetration_mod`, `binomial_coefficient` | Their Mathematics headers (`11-discrete_log_root.hpp`, `33-modular_square_roots.hpp`, `30-modular_power_towers.hpp`, `12-combinatorics_advanced.hpp`) multiply with 128-bit `%`, so no Core reducer is involved. |
| `montmort_number_mod` | Barrett32 work only through `DynModInt` in `derangementTable`, and output-bound; not a direct reducer use. |
| `factorial`, `many_factorials` | Poly-bound (`shiftSamplingPoints` doubling), but the sqrt(p) factorial algorithm belongs to the planned `05-Mathematics/37-fastfactorial.hpp`. |
| `mul_modp_convolution` | One Poly convolution after primitive-root reindexing, which belongs to the planned `05-Mathematics/65-convolution_specialized.hpp`. |
| `number_of_increasing_sequences_between_two_sequences` | Divide-and-conquer lattice-path algorithm with no library header yet. |
| `wildcard_pattern_matching` | `07-Strings/21-wildcardmatching.hpp` runs its own correlator rather than Core Poly. |
| `exp_of_set_power_series`, `log_of_set_power_series`, `polynomial_composite_set_power_series`, `power_projection_of_set_power_series`, `subset_convolution`, the bitwise, gcd and lcm convolutions | Set power series and transforms, owned by Mathematics (`66-set_power_series.hpp` planned, `24-transform_algorithms.hpp`), not Poly. |
| `matrix_*_mod_2`, `inverse_matrix_mod_2`, `system_of_linear_equations_mod_2` | `12-bitmatrix.hpp`, not Bitset (work order A25). |

## Sources and local results

Each source reads with `FastInput<0>` and writes with `FastOutput` (`19-fastio.hpp`), uses the full type, and is built with the judge's flags, so the `__AVX2__`, `__FMA__` and `__PCLMUL__` paths are active. Times are the minimum of three runs per test on the local machine (below), taken while other sessions shared it, so they bound correctness and rough scale rather than serve as benchmarks. Every test of every problem is accepted by the problem's own checker.

| Source | Problem | Exercises | Tests AC | Local max (ms) |
|---|---|---|---|---|
| `03-Polynomials/01-convolutionmod.cpp` | [convolution_mod](https://judge.yosupo.jp/problem/convolution_mod) | `Poly<mint>` operator* (dispatcher: Montgomery NTT on AVX2 lanes) | 53/53 | 113 |
| `03-Polynomials/02-convolutionmod1000000007.cpp` | [convolution_mod_1000000007](https://judge.yosupo.jp/problem/convolution_mod_1000000007) | `Poly<ModInt<1000000007>>` operator* (dispatcher: three NTT primes with Garner, split FFT below 2^20) | 48/48 | 157 |
| `03-Polynomials/03-convolutionmod2p64.cpp` | [convolution_mod_2_64](https://judge.yosupo.jp/problem/convolution_mod_2_64) | PolyCompanion::convolution2p64 (32-bit halves through three NTT primes) | 44/44 | 289 |
| `03-Polynomials/04-convolutionf2p64.cpp` | [convolution_F_2_64](https://judge.yosupo.jp/problem/convolution_F_2_64) | PolyCompanion::convolutionGF2k with k = 64, x^64 = x^4 + x^3 + x + 1 (Karatsuba over PCLMUL carry-less products) | 51/51 | 7590 |
| `03-Polynomials/05-convolutionmodlarge.cpp` | [convolution_mod_large](https://judge.yosupo.jp/problem/convolution_mod_large) | `Poly<mint>` operator* beyond the NTT length limit (dispatcher: block product of length nttMaxLength / 2) | 54/54 | 4681 |
| `03-Polynomials/06-invfps.cpp` | [inv_of_formal_power_series](https://judge.yosupo.jp/problem/inv_of_formal_power_series) | inv (Newton, five transforms per doubling) | 25/25 | 152 |
| `03-Polynomials/07-logfps.cpp` | [log_of_formal_power_series](https://judge.yosupo.jp/problem/log_of_formal_power_series) | log (derivative times inv, integral) | 25/25 | 205 |
| `03-Polynomials/08-expfps.cpp` | [exp_of_formal_power_series](https://judge.yosupo.jp/problem/exp_of_formal_power_series) | exp (Newton with the inverse lifted once per doubling) | 26/26 | 263 |
| `03-Polynomials/09-powfps.cpp` | [pow_of_formal_power_series](https://judge.yosupo.jp/problem/pow_of_formal_power_series) | pow with a 64-bit exponent (valuation shift, powUnit through log and exp) | 37/37 | 354 |
| `03-Polynomials/10-sqrtfps.cpp` | [sqrt_of_formal_power_series](https://judge.yosupo.jp/problem/sqrt_of_formal_power_series) | trySqrt (even valuation, Tonelli constant, Newton) | 35/35 | 128 |
| `03-Polynomials/11-invfpssparse.cpp` | [inv_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/inv_of_formal_power_series_sparse) | invSparse (O(n * k) recurrence over the nonzero terms) | 24/24 | 47 |
| `03-Polynomials/12-logfpssparse.cpp` | [log_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/log_of_formal_power_series_sparse) | logSparse (O(n * k) recurrence) | 24/24 | 68 |
| `03-Polynomials/13-expfpssparse.cpp` | [exp_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/exp_of_formal_power_series_sparse) | expSparse (O(n * k) recurrence) | 25/25 | 68 |
| `03-Polynomials/14-powfpssparse.cpp` | [pow_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/pow_of_formal_power_series_sparse) | powSparse with a 64-bit exponent (valuation shift, O(n * k) recurrence) | 35/35 | 98 |
| `03-Polynomials/15-sqrtfpssparse.cpp` | [sqrt_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/sqrt_of_formal_power_series_sparse) | sqrtSparse (valuation, constant square root, O(n * k) recurrence) | 45/45 | 111 |
| `03-Polynomials/16-divisionpoly.cpp` | [division_of_polynomials](https://judge.yosupo.jp/problem/division_of_polynomials) | divMod (Newton division over a field) | 35/35 | 171 |
| `03-Polynomials/17-invpoly.cpp` | [inv_of_polynomials](https://judge.yosupo.jp/problem/inv_of_polynomials) | tryInvMod (half-GCD extended Euclid, -1 when gcd(f, g) is not constant; a constant g answered as 0 in the driver) | 18/18 | 359 |
| `03-Polynomials/18-multipointeval.cpp` | [multipoint_evaluation](https://judge.yosupo.jp/problem/multipoint_evaluation) | evalMulti (transposed subproduct tree) | 11/11 | 277 |
| `03-Polynomials/19-multipointevalgeometric.cpp` | [multipoint_evaluation_on_geometric_sequence](https://judge.yosupo.jp/problem/multipoint_evaluation_on_geometric_sequence) | evalMultiGeometric (chirp-z) | 25/25 | 119 |
| `03-Polynomials/20-interpolation.cpp` | [polynomial_interpolation](https://judge.yosupo.jp/problem/polynomial_interpolation) | Poly::interpolate (subproduct tree, transposed evaluation of the derivative) | 9/9 | 376 |
| `03-Polynomials/21-interpolationgeometric.cpp` | [polynomial_interpolation_on_geometric_sequence](https://judge.yosupo.jp/problem/polynomial_interpolation_on_geometric_sequence) | Poly::interpolateGeometric (chirp-z and q-binomial products) | 28/28 | 259 |
| `03-Polynomials/22-taylorshift.cpp` | [polynomial_taylor_shift](https://judge.yosupo.jp/problem/polynomial_taylor_shift) | taylorShift (binomial convolution) | 38/38 | 74 |
| `03-Polynomials/23-shiftsamplingpoints.cpp` | [shift_of_sampling_points_of_polynomial](https://judge.yosupo.jp/problem/shift_of_sampling_points_of_polynomial) | Poly::shiftSamplingPoints (one middle product) | 32/32 | 255 |
| `03-Polynomials/24-prefixsumpoly.cpp` | [prefix_sum_of_polynomial](https://judge.yosupo.jp/problem/prefix_sum_of_polynomial) | prefixSumOfPolynomial (Faulhaber through Bernoulli numbers) | 26/26 | 214 |
| `03-Polynomials/25-monomialtonewton.cpp` | [conversion_from_monomial_basis_to_newton_basis](https://judge.yosupo.jp/problem/conversion_from_monomial_basis_to_newton_basis) | monomialToNewton (subproduct tree remainders) | 15/15 | 190 |
| `03-Polynomials/26-composition.cpp` | [composition_of_formal_power_series](https://judge.yosupo.jp/problem/composition_of_formal_power_series) | compose (Kinoshita–Li; Brent–Kung below the measured threshold) | 27/27 | 44 |
| `03-Polynomials/27-compositionlarge.cpp` | [composition_of_formal_power_series_large](https://judge.yosupo.jp/problem/composition_of_formal_power_series_large) | compose (Kinoshita–Li bivariate Bostan–Mori) up to 2^17 terms | 32/32 | 887 |
| `03-Polynomials/28-compositionalinverse.cpp` | [compositional_inverse_of_formal_power_series](https://judge.yosupo.jp/problem/compositional_inverse_of_formal_power_series) | compositionalInverse (Lagrange inversion through power projection) | 23/23 | 37 |
| `03-Polynomials/29-compositionalinverselarge.cpp` | [compositional_inverse_of_formal_power_series_large](https://judge.yosupo.jp/problem/compositional_inverse_of_formal_power_series_large) | compositionalInverse (Lagrange inversion through power projection) up to 2^17 terms | 28/28 | 927 |
| `03-Polynomials/30-kthterm.cpp` | [kth_term_of_linearly_recurrent_sequence](https://judge.yosupo.jp/problem/kth_term_of_linearly_recurrent_sequence) | Poly::linearRecurrenceKth (Bostan–Mori through coefOfRationalFps) | 20/20 | 705 |
| `03-Polynomials/31-consecutiveterms.cpp` | [consecutive_terms_of_linear_recurrent_sequence](https://judge.yosupo.jp/problem/consecutive_terms_of_linear_recurrent_sequence) | Poly::consecutiveTerms (sliceRationalFps: Bostan–Mori to k, then one product) | 43/43 | 1541 |
| `03-Polynomials/32-findlinearrecurrence.cpp` | [find_linear_recurrence](https://judge.yosupo.jp/problem/find_linear_recurrence) | Poly::findLinearRecurrence (Berlekamp–Massey) | 18/18 | 125 |
| `03-Polynomials/33-polyfactorization.cpp` | [factorization_of_polynomials](https://judge.yosupo.jp/problem/factorization_of_polynomials) | factor over `Poly<DynModInt<>>` with a runtime prime (square-free, distinct-degree, Cantor–Zassenhaus) | 67/67 | 589 |
| `03-Polynomials/34-polyrootfinding.cpp` | [polynomial_root_finding](https://judge.yosupo.jp/problem/polynomial_root_finding) | roots (gcd with x^p - x, Cantor–Zassenhaus splitting) | 36/36 | 172 |
| `03-Polynomials/35-productpolysequence.cpp` | [product_of_polynomial_sequence](https://judge.yosupo.jp/problem/product_of_polynomial_sequence) | productOfSequence (shortest pair first) | 27/27 | 831 |
| `03-Polynomials/36-bernoulli.cpp` | [bernoulli_number](https://judge.yosupo.jp/problem/bernoulli_number) | Poly::bernoulliNumbers (inverse of (e^x - 1) / x) | 11/11 | 113 |
| `03-Polynomials/37-partition.cpp` | [partition_function](https://judge.yosupo.jp/problem/partition_function) | Poly::partitionNumbers (inverse of the pentagonal series) | 11/11 | 88 |
| `03-Polynomials/38-stirling1.cpp` | [stirling_number_of_the_first_kind](https://judge.yosupo.jp/problem/stirling_number_of_the_first_kind) | Poly::stirling1Row (productOfArithmeticProgression: doubling with Taylor shifts) | 10/10 | 119 |
| `03-Polynomials/39-stirling2.cpp` | [stirling_number_of_the_second_kind](https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind) | Poly::stirling2Row (one convolution) | 10/10 | 109 |
| `03-Polynomials/40-stirling1fixedk.cpp` | [stirling_number_of_the_first_kind_fixed_k](https://judge.yosupo.jp/problem/stirling_number_of_the_first_kind_fixed_k) | log, pow (valuation k) and egfToOgf: s(n, k) = n! [x^n] log(1 + x)^k / k! | 14/14 | 397 |
| `03-Polynomials/41-stirling2fixedk.cpp` | [stirling_number_of_the_second_kind_fixed_k](https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind_fixed_k) | ogfToEgf, pow (valuation k) and egfToOgf: S(n, k) = n! [x^n] (e^x - 1)^k / k! | 14/14 | 349 |
| `03-Polynomials/42-bellnumber.cpp` | [bell_number](https://judge.yosupo.jp/problem/bell_number) | ogfToEgf, exp and egfToOgf: B_n = n! [x^n] exp(e^x - 1) | 11/11 | 213 |
| `03-Polynomials/43-sharppsubsetsum.cpp` | [sharp_p_subset_sum](https://judge.yosupo.jp/problem/sharp_p_subset_sum) | exp of the harmonic log sum_s c_s log(1 + x^s) | 24/24 | 259 |
| `03-Polynomials/44-multivariatecyclic.cpp` | [multivariate_convolution_cyclic](https://judge.yosupo.jp/problem/multivariate_convolution_cyclic) | `Poly<DynModInt<>>::multivariateCyclic` (per-axis DFTs of lengths dividing p - 1) with a runtime prime | 24/24 | 557 |
| `03-Polynomials/45-multivariate.cpp` | [multivariate_convolution](https://judge.yosupo.jp/problem/multivariate_convolution) | Poly::multivariate (truncated product by the chi-graded Kronecker trick) | 17/17 | 937 |
| `03-Polynomials/46-inv2d.cpp` | [inv_of_formal_power_series_2d](https://judge.yosupo.jp/problem/inv_of_formal_power_series_2d) | Poly::inv2d (Newton in x over F_p[y] / y^m) | 28/28 | 763 |
| `04-Number Theory/01-primalitytest.cpp` | [primality_test](https://judge.yosupo.jp/problem/primality_test) | Montgomery64 (redc, init) as the reducer of isPrime (deterministic Miller-Rabin, 05-Mathematics/07-primality_factorization.hpp) | 13/13 | 335 |
| `04-Number Theory/02-factorize.cpp` | [factorize](https://judge.yosupo.jp/problem/factorize) | Montgomery64 (redc, init) as the reducer of factorize (Miller-Rabin and batched Pollard-Brent rho, 05-Mathematics/07-primality_factorization.hpp) | 31/31 | 137 |

## Commands and results (2026-11-15)

Environment: GCC 16.2.1 (`20260810`), CPython 3.14, i9-11900H, flags `-O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native` (the judge's x86 command, `07-judge.py`), tests and reference answers regenerated from Library Checker commit `73876f1cbcf0`.

```bash
python3 '96-Local Testing/00-Tools/07-judge.py' reproduce <problem> '97-Online Testing/src/01-Yosupo/<topic>/<source>.cpp' --label ours   # 48 problems: every test AC
python3 '97-Online Testing/01-expander.py'                      # run from /tmp: Expanded 68 C++ source(s); removed 0 owned orphan(s)
python3 '96-Local Testing/00-Tools/02-online_tester.py'         # 5 PASS
python3 '96-Local Testing/00-Tools/07-judge.py' reproduce <problem> '97-Online Testing/expanded/01-Yosupo/<topic>/<source>.expanded.cpp' --label expanded --runs 1   # 48 bundles, 1319 of 1327 tests AC; 8 convolution_F_2_64 tests over the limit under load (Expansion)
```

The first run of `inv_of_polynomials` failed `example_01`, where `g = 7` is a constant. `tryInvMod` requires `deg m >= 1` (its contract), and modulo a unit the answer is `h = 0`. The driver now prints `0` for a constant `g`, and the rerun accepts all 18 tests. Its local maximum moved between 266 ms and 808 ms across runs under machine load; the table records the latest run.

## Expansion

The grab bundler (default) expanded all 68 sources. No bundle keeps a quoted library include. The 48 new bundles range from 37 KB (`01-primalitytest`) to 134 KB (`33-polyfactorization`), since Poly's closure is large. Each new bundle was rerun on its problem's tests with the judge's flags, one run per test, while another session kept the machine loaded (load average 8 to 12). The checker accepted 1319 of the 1327 tests. The other 8 are `convolution_F_2_64` tests that ran past 10 s, which the tool reports as TLE without checking them. Run by hand, the checker accepts all 8 outputs. Run back to back on `random_01` under the same load, the source took 12.8 to 14.3 s and the bundle 11.7 to 12.2 s, with byte-identical outputs, so the load caused the excess, not the bundle. Bundle timings from this run are not benchmarks.

## Open limits

- `convolution_mod` also offers a function-style (grader) submission. `07-judge.py` used to refuse every problem with a grader; with the maintainer's yes (2026-11-15) it now refuses only a problem without `sol/correct.cpp`, since Library Checker accepts stdin solutions beside function submissions ([judge tool evidence](<../../96-Local Testing/Docs/00-judge.md>)). Source and bundle then passed all 53 tests through the tool itself.
- `convolution_F_2_64` takes 7.6 s locally against a 10 s limit (judge estimate 9.9 s at the tool's factor of 1.3). The other slow problems are `convolution_mod_large` (4.7 s, estimate 6.1 s, limit 10 s) and `consecutive_terms_of_linear_recurrent_sequence` (1.5 s, limit 10 s). Only a submission settles the GF(2^64) margin. Its Karatsuba over carry-less products is a target for the Poly speed sweep (work order A10).
- Online acceptance: none recorded. Each source's `Status` line changes to an acceptance record (submission id, compiler, date, source hash) only after a hand submission.
- Remaining P225 work outside this slice: judge sources for the other Core headers (matrix, bitmatrix and sparse matrix: work order A25) and for contest folders.

## History

- 2026-11-15: Poly, Bitset, Rational, Barrett and Montgomery slice: 48 sources written (46 Poly, 2 Montgomery64); every test of all 48 problems accepted locally; expander run on 68 sources, every new bundle rerun with every output accepted by its checker; one driver fix (`inv_of_polynomials`, constant modulus); `07-judge.py` now runs grader problems that ship `sol/correct.cpp` (user's yes), and `convolution_mod` passed through it.
