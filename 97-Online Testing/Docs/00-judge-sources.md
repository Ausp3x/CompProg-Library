# P225 / SUP03 Judge Sources

P225 adds judge solutions under `src/` and exercises the expander ([06-online.md](../../00-Guidelines/06-online.md)). This document records the 2026-11-15 slice. It covers the verified Core full headers `16-poly.hpp`, `18-bitset.hpp` and `09-rational.hpp`, and `03-barrett.hpp` and `04-montgomery.hpp` where a Mathematics operation uses them directly as its reducer. It lists every Library Checker problem whose bottleneck is one of those headers' operations, the source written for each, and the problems left out because no header in scope is the bottleneck.

Every source here is **locally compiled/run** and **accepted online**. The maintainer submitted the 48 bundles of commit `5765edd` by hand on 2026-11-14 (UTC) as `Ausp3x`, the maintainer's judge account. `factorize` and `primality_test` were resubmitted later with `04-submit.py`, and the table cites the latest submission. Each accepted submission is byte-identical to its bundle.

## Selection

Candidates came from `07-judge.py problems` (256 problems, Library Checker commit `73876f1cbcf0`). They were matched against the operations in the inventory rows and evidence documents of the headers in scope: [16-poly.md](../../01-Core/Docs/16-poly.md), [18-bitset.md](../../01-Core/Docs/18-bitset.md), [09-rational.md](../../01-Core/Docs/09-rational.md), [03-barrett.md](../../01-Core/Docs/03-barrett.md) and [04-montgomery.md](../../01-Core/Docs/04-montgomery.md). A source was written when the library operation it calls is the work the problem measures. The driver adds only a few lines: input, output and at most a series identity such as `B_n = n! [x^n] exp(e^x - 1)`.

- `16-poly.hpp`: 46 problems, all under `src/01-Yosupo/03-Polynomials/`.
- `04-montgomery.hpp`: `primality_test` and `factorize`, both under `src/01-Yosupo/04-Number Theory/`. They call `isPrime` and `factorize` of [`07-primality_factorization.hpp`](../../05-Mathematics/07-primality_factorization.hpp), whose Miller–Rabin and Pollard–Brent rho run on `Montgomery64::redc` and `init`.
- `03-barrett.hpp`: no problem. No Mathematics operation takes a Barrett reducer. Barrett64 only reduces the Garner output of `PolyCompanion::convolutionArbitraryMod` inside Poly and backs the runtime modulus of `DynModInt` inside modint; neither is a direct use.
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
| `matrix_*_mod_2`, `inverse_matrix_mod_2`, `system_of_linear_equations_mod_2` | `12-bitmatrix.hpp`, not Bitset (a later P225 slice). |

## Sources and Local Results

Each source reads with `FastInput<0>` and writes with `FastOutput` (`19-fastio.hpp`) and uses the full type. It is built with the judge's flags, so the `__AVX2__`, `__FMA__` and `__PCLMUL__` paths are active. Every test of every problem is accepted by the problem's own checker. Local times are the minimum of three runs per test on the machine described under Commands and Results. Other sessions shared the machine, so the times show correctness and rough scale; they are not benchmarks.

| Source | Problem | Exercises | Tests AC | Local max (ms) | Judge (AC, ms) |
|---|---|---|---|---|---|
| `03-Polynomials/01-convolutionmod.cpp` | [convolution_mod](https://judge.yosupo.jp/problem/convolution_mod) | `Poly<mint>` operator* (dispatcher: Montgomery NTT on AVX2 lanes) | 53/53 | 113 | [#417048](https://judge.yosupo.jp/submission/417048), 70 |
| `03-Polynomials/02-convolutionmod1000000007.cpp` | [convolution_mod_1000000007](https://judge.yosupo.jp/problem/convolution_mod_1000000007) | `Poly<ModInt<1000000007>>` operator* (dispatcher: three NTT primes with Garner, split FFT below 2^20) | 48/48 | 157 | [#417049](https://judge.yosupo.jp/submission/417049), 176 |
| `03-Polynomials/03-convolutionmod2p64.cpp` | [convolution_mod_2_64](https://judge.yosupo.jp/problem/convolution_mod_2_64) | PolyCompanion::convolution2p64 (32-bit halves through three NTT primes) | 44/44 | 289 | [#417050](https://judge.yosupo.jp/submission/417050), 324 |
| `03-Polynomials/04-convolutionf2p64.cpp` | [convolution_F_2_64](https://judge.yosupo.jp/problem/convolution_F_2_64) | PolyCompanion::convolutionGF2k with k = 64, x^64 = x^4 + x^3 + x + 1 (Karatsuba over PCLMUL carry-less products) | 51/51 | 7590 | [#417051](https://judge.yosupo.jp/submission/417051), 8942 |
| `03-Polynomials/05-convolutionmodlarge.cpp` | [convolution_mod_large](https://judge.yosupo.jp/problem/convolution_mod_large) | `Poly<mint>` operator* beyond the NTT length limit (dispatcher: block product of length nttMaxLength / 2) | 54/54 | 4681 | [#417075](https://judge.yosupo.jp/submission/417075), 2106 |
| `03-Polynomials/06-invfps.cpp` | [inv_of_formal_power_series](https://judge.yosupo.jp/problem/inv_of_formal_power_series) | inv (Newton, five transforms per doubling) | 25/25 | 152 | [#417053](https://judge.yosupo.jp/submission/417053), 67 |
| `03-Polynomials/07-logfps.cpp` | [log_of_formal_power_series](https://judge.yosupo.jp/problem/log_of_formal_power_series) | log (derivative times inv, integral) | 25/25 | 205 | [#417054](https://judge.yosupo.jp/submission/417054), 110 |
| `03-Polynomials/08-expfps.cpp` | [exp_of_formal_power_series](https://judge.yosupo.jp/problem/exp_of_formal_power_series) | exp (Newton with the inverse lifted once per doubling) | 26/26 | 263 | [#417055](https://judge.yosupo.jp/submission/417055), 110 |
| `03-Polynomials/09-powfps.cpp` | [pow_of_formal_power_series](https://judge.yosupo.jp/problem/pow_of_formal_power_series) | pow with a 64-bit exponent (valuation shift, powUnit through log and exp) | 37/37 | 354 | [#417056](https://judge.yosupo.jp/submission/417056), 197 |
| `03-Polynomials/10-sqrtfps.cpp` | [sqrt_of_formal_power_series](https://judge.yosupo.jp/problem/sqrt_of_formal_power_series) | trySqrt (even valuation, Tonelli constant, Newton) | 35/35 | 128 | [#417057](https://judge.yosupo.jp/submission/417057), 74 |
| `03-Polynomials/11-invfpssparse.cpp` | [inv_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/inv_of_formal_power_series_sparse) | invSparse (O(n * k) recurrence over the nonzero terms) | 24/24 | 47 | [#417058](https://judge.yosupo.jp/submission/417058), 29 |
| `03-Polynomials/12-logfpssparse.cpp` | [log_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/log_of_formal_power_series_sparse) | logSparse (O(n * k) recurrence) | 24/24 | 68 | [#417059](https://judge.yosupo.jp/submission/417059), 42 |
| `03-Polynomials/13-expfpssparse.cpp` | [exp_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/exp_of_formal_power_series_sparse) | expSparse (O(n * k) recurrence) | 25/25 | 68 | [#417060](https://judge.yosupo.jp/submission/417060), 40 |
| `03-Polynomials/14-powfpssparse.cpp` | [pow_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/pow_of_formal_power_series_sparse) | powSparse with a 64-bit exponent (valuation shift, O(n * k) recurrence) | 35/35 | 98 | [#417061](https://judge.yosupo.jp/submission/417061), 85 |
| `03-Polynomials/15-sqrtfpssparse.cpp` | [sqrt_of_formal_power_series_sparse](https://judge.yosupo.jp/problem/sqrt_of_formal_power_series_sparse) | sqrtSparse (valuation, constant square root, O(n * k) recurrence) | 45/45 | 111 | [#417062](https://judge.yosupo.jp/submission/417062), 89 |
| `03-Polynomials/16-divisionpoly.cpp` | [division_of_polynomials](https://judge.yosupo.jp/problem/division_of_polynomials) | divMod (Newton division over a field) | 35/35 | 171 | [#417063](https://judge.yosupo.jp/submission/417063), 97 |
| `03-Polynomials/17-invpoly.cpp` | [inv_of_polynomials](https://judge.yosupo.jp/problem/inv_of_polynomials) | tryInvMod (half-GCD extended Euclid, -1 when gcd(f, g) is not constant; a constant g answered as 0 in the driver) | 18/18 | 359 | [#417064](https://judge.yosupo.jp/submission/417064), 144 |
| `03-Polynomials/18-multipointeval.cpp` | [multipoint_evaluation](https://judge.yosupo.jp/problem/multipoint_evaluation) | evalMulti (transposed subproduct tree) | 11/11 | 277 | [#417065](https://judge.yosupo.jp/submission/417065), 170 |
| `03-Polynomials/19-multipointevalgeometric.cpp` | [multipoint_evaluation_on_geometric_sequence](https://judge.yosupo.jp/problem/multipoint_evaluation_on_geometric_sequence) | evalMultiGeometric (chirp-z) | 25/25 | 119 | [#417066](https://judge.yosupo.jp/submission/417066), 70 |
| `03-Polynomials/20-interpolation.cpp` | [polynomial_interpolation](https://judge.yosupo.jp/problem/polynomial_interpolation) | Poly::interpolate (subproduct tree, transposed evaluation of the derivative) | 9/9 | 376 | [#417068](https://judge.yosupo.jp/submission/417068), 245 |
| `03-Polynomials/21-interpolationgeometric.cpp` | [polynomial_interpolation_on_geometric_sequence](https://judge.yosupo.jp/problem/polynomial_interpolation_on_geometric_sequence) | Poly::interpolateGeometric (chirp-z and q-binomial products) | 28/28 | 259 | [#417069](https://judge.yosupo.jp/submission/417069), 186 |
| `03-Polynomials/22-taylorshift.cpp` | [polynomial_taylor_shift](https://judge.yosupo.jp/problem/polynomial_taylor_shift) | taylorShift (binomial convolution) | 38/38 | 74 | [#417070](https://judge.yosupo.jp/submission/417070), 67 |
| `03-Polynomials/23-shiftsamplingpoints.cpp` | [shift_of_sampling_points_of_polynomial](https://judge.yosupo.jp/problem/shift_of_sampling_points_of_polynomial) | Poly::shiftSamplingPoints (one middle product) | 32/32 | 255 | [#417071](https://judge.yosupo.jp/submission/417071), 107 |
| `03-Polynomials/24-prefixsumpoly.cpp` | [prefix_sum_of_polynomial](https://judge.yosupo.jp/problem/prefix_sum_of_polynomial) | prefixSumOfPolynomial (Faulhaber through Bernoulli numbers) | 26/26 | 214 | [#417072](https://judge.yosupo.jp/submission/417072), 152 |
| `03-Polynomials/25-monomialtonewton.cpp` | [conversion_from_monomial_basis_to_newton_basis](https://judge.yosupo.jp/problem/conversion_from_monomial_basis_to_newton_basis) | monomialToNewton (subproduct tree remainders) | 15/15 | 190 | [#417073](https://judge.yosupo.jp/submission/417073), 148 |
| `03-Polynomials/26-composition.cpp` | [composition_of_formal_power_series](https://judge.yosupo.jp/problem/composition_of_formal_power_series) | compose (Kinoshita–Li; Brent–Kung below the measured threshold) | 27/27 | 44 | [#417076](https://judge.yosupo.jp/submission/417076), 25 |
| `03-Polynomials/27-compositionlarge.cpp` | [composition_of_formal_power_series_large](https://judge.yosupo.jp/problem/composition_of_formal_power_series_large) | compose (Kinoshita–Li bivariate Bostan–Mori) up to 2^17 terms | 32/32 | 887 | [#417077](https://judge.yosupo.jp/submission/417077), 363 |
| `03-Polynomials/28-compositionalinverse.cpp` | [compositional_inverse_of_formal_power_series](https://judge.yosupo.jp/problem/compositional_inverse_of_formal_power_series) | compositionalInverse (Lagrange inversion through power projection) | 23/23 | 37 | [#417078](https://judge.yosupo.jp/submission/417078), 18 |
| `03-Polynomials/29-compositionalinverselarge.cpp` | [compositional_inverse_of_formal_power_series_large](https://judge.yosupo.jp/problem/compositional_inverse_of_formal_power_series_large) | compositionalInverse (Lagrange inversion through power projection) up to 2^17 terms | 28/28 | 927 | [#417079](https://judge.yosupo.jp/submission/417079), 397 |
| `03-Polynomials/30-kthterm.cpp` | [kth_term_of_linearly_recurrent_sequence](https://judge.yosupo.jp/problem/kth_term_of_linearly_recurrent_sequence) | Poly::linearRecurrenceKth (Bostan–Mori through coefOfRationalFps) | 20/20 | 705 | [#417080](https://judge.yosupo.jp/submission/417080), 332 |
| `03-Polynomials/31-consecutiveterms.cpp` | [consecutive_terms_of_linear_recurrent_sequence](https://judge.yosupo.jp/problem/consecutive_terms_of_linear_recurrent_sequence) | Poly::consecutiveTerms (sliceRationalFps: Bostan–Mori to k, then one product) | 43/43 | 1541 | [#417081](https://judge.yosupo.jp/submission/417081), 678 |
| `03-Polynomials/32-findlinearrecurrence.cpp` | [find_linear_recurrence](https://judge.yosupo.jp/problem/find_linear_recurrence) | Poly::findLinearRecurrence (Berlekamp–Massey) | 18/18 | 125 | [#417082](https://judge.yosupo.jp/submission/417082), 93 |
| `03-Polynomials/33-polyfactorization.cpp` | [factorization_of_polynomials](https://judge.yosupo.jp/problem/factorization_of_polynomials) | factor over `Poly<DynModInt<>>` with a runtime prime (square-free, distinct-degree, Cantor–Zassenhaus) | 67/67 | 589 | [#417083](https://judge.yosupo.jp/submission/417083), 404 |
| `03-Polynomials/34-polyrootfinding.cpp` | [polynomial_root_finding](https://judge.yosupo.jp/problem/polynomial_root_finding) | roots (gcd with x^p - x, Cantor–Zassenhaus splitting) | 36/36 | 172 | [#417084](https://judge.yosupo.jp/submission/417084), 125 |
| `03-Polynomials/35-productpolysequence.cpp` | [product_of_polynomial_sequence](https://judge.yosupo.jp/problem/product_of_polynomial_sequence) | productOfSequence (shortest pair first) | 27/27 | 831 | [#417086](https://judge.yosupo.jp/submission/417086), 315 |
| `03-Polynomials/36-bernoulli.cpp` | [bernoulli_number](https://judge.yosupo.jp/problem/bernoulli_number) | Poly::bernoulliNumbers (inverse of (e^x - 1) / x) | 11/11 | 113 | [#417087](https://judge.yosupo.jp/submission/417087), 63 |
| `03-Polynomials/37-partition.cpp` | [partition_function](https://judge.yosupo.jp/problem/partition_function) | Poly::partitionNumbers (inverse of the pentagonal series) | 11/11 | 88 | [#417088](https://judge.yosupo.jp/submission/417088), 62 |
| `03-Polynomials/38-stirling1.cpp` | [stirling_number_of_the_first_kind](https://judge.yosupo.jp/problem/stirling_number_of_the_first_kind) | Poly::stirling1Row (productOfArithmeticProgression: doubling with Taylor shifts) | 10/10 | 119 | [#417089](https://judge.yosupo.jp/submission/417089), 74 |
| `03-Polynomials/39-stirling2.cpp` | [stirling_number_of_the_second_kind](https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind) | Poly::stirling2Row (one convolution) | 10/10 | 109 | [#417090](https://judge.yosupo.jp/submission/417090), 89 |
| `03-Polynomials/40-stirling1fixedk.cpp` | [stirling_number_of_the_first_kind_fixed_k](https://judge.yosupo.jp/problem/stirling_number_of_the_first_kind_fixed_k) | log, pow (valuation k) and egfToOgf: s(n, k) = n! [x^n] log(1 + x)^k / k! | 14/14 | 397 | [#417091](https://judge.yosupo.jp/submission/417091), 237 |
| `03-Polynomials/41-stirling2fixedk.cpp` | [stirling_number_of_the_second_kind_fixed_k](https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind_fixed_k) | ogfToEgf, pow (valuation k) and egfToOgf: S(n, k) = n! [x^n] (e^x - 1)^k / k! | 14/14 | 349 | [#417092](https://judge.yosupo.jp/submission/417092), 202 |
| `03-Polynomials/42-bellnumber.cpp` | [bell_number](https://judge.yosupo.jp/problem/bell_number) | ogfToEgf, exp and egfToOgf: B_n = n! [x^n] exp(e^x - 1) | 11/11 | 213 | [#417093](https://judge.yosupo.jp/submission/417093), 120 |
| `03-Polynomials/43-sharppsubsetsum.cpp` | [sharp_p_subset_sum](https://judge.yosupo.jp/problem/sharp_p_subset_sum) | exp of the harmonic log sum_s c_s log(1 + x^s) | 24/24 | 259 | [#417094](https://judge.yosupo.jp/submission/417094), 147 |
| `03-Polynomials/44-multivariatecyclic.cpp` | [multivariate_convolution_cyclic](https://judge.yosupo.jp/problem/multivariate_convolution_cyclic) | `Poly<DynModInt<>>::multivariateCyclic` (per-axis DFTs of lengths dividing p - 1) with a runtime prime | 24/24 | 557 | [#417095](https://judge.yosupo.jp/submission/417095), 349 |
| `03-Polynomials/45-multivariate.cpp` | [multivariate_convolution](https://judge.yosupo.jp/problem/multivariate_convolution) | Poly::multivariate (truncated product by the chi-graded Kronecker trick) | 17/17 | 937 | [#417096](https://judge.yosupo.jp/submission/417096), 562 |
| `03-Polynomials/46-inv2d.cpp` | [inv_of_formal_power_series_2d](https://judge.yosupo.jp/problem/inv_of_formal_power_series_2d) | Poly::inv2d (Newton in x over F_p[y] / y^m) | 28/28 | 763 | [#417097](https://judge.yosupo.jp/submission/417097), 270 |
| `04-Number Theory/01-primalitytest.cpp` | [primality_test](https://judge.yosupo.jp/problem/primality_test) | Montgomery64 (redc, init) as the reducer of isPrime (deterministic Miller-Rabin, 05-Mathematics/07-primality_factorization.hpp) | 13/13 | 335 | [#417106](https://judge.yosupo.jp/submission/417106), 165 |
| `04-Number Theory/02-factorize.cpp` | [factorize](https://judge.yosupo.jp/problem/factorize) | Montgomery64 (redc, init) as the reducer of factorize (Miller-Rabin and batched Pollard-Brent rho, 05-Mathematics/07-primality_factorization.hpp) | 31/31 | 137 | [#417103](https://judge.yosupo.jp/submission/417103), 35 |

## Commands and Results

Run on 2026-11-15. Environment: GCC 16.2.1 (`20260810`), CPython 3.14, i9-11900H, flags `-O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native` (the judge's x86 command, `07-judge.py`). Tests and reference answers were regenerated from Library Checker commit `73876f1cbcf0`.

```bash
python3 '96-Local Testing/00-Tools/07-judge.py' reproduce <problem> '97-Online Testing/src/01-Yosupo/<topic>/<source>.cpp' --label ours   # 48 problems: every test AC
python3 '97-Online Testing/01-expander.py'                      # run from /tmp: Expanded 68 C++ source(s); removed 0 owned orphan(s)
python3 '96-Local Testing/00-Tools/02-online_tester.py'         # 5 PASS
python3 '96-Local Testing/00-Tools/07-judge.py' reproduce <problem> '97-Online Testing/expanded/01-Yosupo/<topic>/<source>.expanded.cpp' --label expanded --runs 1   # 48 bundles, 1319 of 1327 tests AC; 8 convolution_F_2_64 tests over the limit under load (Expansion)
```

`inv_of_polynomials` needs a driver case. Its `example_01` has the constant modulus `g = 7`, while `tryInvMod` requires `deg m >= 1` (its contract); modulo a unit the answer is `h = 0`, so the driver prints `0` for a constant `g`. With that case all 18 tests pass. Its local maximum varied between 266 ms and 808 ms across runs under machine load; the table records the latest run.

## Expansion

The grab bundler (the default) expanded all 68 sources, and no bundle keeps a quoted library include. The 48 new bundles range from 37 KB (`01-primalitytest`) to 134 KB (`33-polyfactorization`), since Poly's closure is large.

Each new bundle was rerun on its problem's tests with the judge's flags, one run per test, while another session kept the machine loaded (load average 8 to 12). The checker accepted 1319 of the 1327 tests. The other 8 are `convolution_F_2_64` tests that ran past 10 s, which the tool reports as TLE without checking them; run by hand, the checker accepts all 8 outputs. Run back to back on `random_01` under the same load, the source took 12.8 to 14.3 s and the bundle 11.7 to 12.2 s, with byte-identical outputs, so the load caused the excess, not the bundle. Bundle timings from this run are not benchmarks.

## Submission Tool

[`04-submit.py`](../04-submit.py) belongs to the maintainer. Claude never runs it, and the shell hook refuses it (decision log, 2026-11-15 Judge submissions).

- Login: `login` reads the Firebase web key from the judge's public frontend configuration, signs in with email and password through Firebase's REST API and checks `/auth/current_user`. It saves only `{api_key, refresh_token, user}` in `~/.config/compprog/yosupo.json` (directory 0700, file 0600; `CP_SUBMIT_CONFIG` overrides the path). Each run trades the refresh token for an ID token and saves the rotated refresh token. A run stops when no login is saved or the login names another user, so it never submits anonymously.
- Selection: `list` numbers every Yosupo source under `src/`. A selection takes numbers, ranges, problem names, file names (`primalitytest`, `01-primalitytest` or `01-primalitytest.cpp`), src-relative paths or globs, and `new`, `stale` or `all`. Sources run in the order given, without repeats. With no selection the list is shown and a selection asked for. A confirmation precedes any submission; `--yes` skips it.
- States: a `new` source has no `accepted online (#ID` record. A source is `current` when the recorded submission's source, fetched through `07-judge.py`'s cache, equals today's bundle with Status lines, CR and trailing whitespace ignored, and `stale` otherwise. Writing a record therefore never makes its source stale.
- Submission:
  - Every run expands first (`01-expander.py`, about 3 s when nothing changed) and stops without submitting if expansion fails.
  - It sends today's bundle with `lang` `cpp` (C++23, GCC 15.2 at the judge), one source at a time. It polls every 2 s for up to 30 min, retrying a network error within that limit.
  - It prints each verdict with its link; on a terminal only, AC is green and any other verdict red.
  - It renews the ID token after 45 min, since Firebase's tokens expire after an hour.
  - On AC it rewrites the source's Status line as `06-online.md` specifies, replacing an earlier record or a "not verified" clause and keeping other text. The commit is HEAD, with `+dirty` when the source or a library folder has uncommitted changes.
  - A non-AC verdict writes nothing and makes the exit status nonzero.
- Classification of this repository on 2026-11-15, by the script's own functions from the API cache: 48 `current` (this slice), 14 `new` (the linear algebra and mini big-integer sources) and 6 `stale` (the big-integer sources, whose records name a revision from before their review).

| Feature | Test (`96-Local Testing/00-Tools/16-submit_tester.py`) |
|---|---|
| Login saves no password, modes 0700/0600; wrong password and unregistered user refused | `test_login` |
| Refresh token traded and rotated; missing login and user mismatch refused | `test_refresh` |
| Yosupo sources only; new, current, stale; Status lines, CR and trailing spaces ignored | `test_states` |
| Numbers, ranges, problem and file names, globs, paths, words; order and repeats; bad selections refused | `test_pick`, `test_random_pick` (stress) |
| Submit body and bearer, wait through `-`, a stage and `k/n` progress, exact record, current afterwards | `test_submit_ac` |
| No submission without a yes or with an empty selection; WA writes nothing and fails | `test_submit_wa_and_confirm` |
| `+dirty`; Status line inserted when missing | `test_dirty` |
| Record rewrites (migrated, earlier record, old-format sentence kept, bare clause) | `test_record_rewrites` |
| Failing expander: nothing submitted | `test_expander_failure` |
| Green AC and red verdicts and totals on a terminal, plain when redirected; record note only after an AC | `test_colors` |
| ID token renewed by age before a submission | `test_token_age`, `test_submit_ac` (one token while young) |
| Network errors while waiting retried; wait limit while unreachable; unreachable judge or Firebase exits without a traceback | `test_wait_retries` |
| Hook refusal of the script in every command position, reading, staging and testing allowed | `12-hooks_tester.py` `test_submit` |

The tester serves the judge, Firebase and the frontend configuration from a local HTTP server, so it needs no network. Results on 2026-11-15:

- quick: 6 tests PASS (peak 21 MB); full: 13 tests PASS (23 MB); stress `--seed 7 --rounds 3`: PASS (23 MB); `CXX=g++-14` quick: PASS.
- `12-hooks_tester.py --mode full`: 16 tests PASS (27 MB).
- Nine hand mutations of the script were each caught: no token renewal, no retry, colors never, always or inverted, the record note always, tracebacks from `state` and `request`, and file names accepted only with their prefix.

The maintainer's first live run (`factorize`, #417103 as `Ausp3x`, AC 35 ms) proved the login and the authorized submission. It also showed that the judge reports a queued submission as `-`, which the fake server did not serve and the tool took for a verdict. `07-judge.py`'s `pending` now covers `-` and `k/n` progress, and the fake serves both. The record was then written with the script's own `record` from the judge-held source, which equals today's bundle.

## Open Limits

- `convolution_mod` also offers a function-style (grader) submission. `07-judge.py` used to refuse every problem with a grader. With the maintainer's yes (2026-11-15) it now refuses only a problem without `sol/correct.cpp`, since Library Checker accepts stdin solutions beside function submissions ([judge tool evidence](<../../96-Local Testing/Docs/00-judge.md>)). Source and bundle then passed all 53 tests through the tool itself.
- `convolution_F_2_64` takes 7.6 s locally and 8.9 s on the judge (#417051) against a 10 s limit, the only tight margin. Its Karatsuba over carry-less products is an open optimization candidate with no owning open package; P025's plan note records it, and the user decides who takes it. The other slow problems are `convolution_mod_large` (4.7 s, estimate 6.1 s, limit 10 s) and `consecutive_terms_of_linear_recurrent_sequence` (1.5 s, limit 10 s).
- Online acceptance: all 48 are accepted (judge column). `07-judge.py mine Ausp3x <problem>` fetched each submission, and its cached source matched the bundle byte for byte. Each source's `Status` line holds the record: submission id, language `cpp` as the API reports it, UTC date, judge time, and the SHA-256 prefix of the submitted bundle. A source edited later no longer expands to that hash, so a changed source needs a new submission before its record is current.
- Remaining P225 work outside this slice: judge sources for the other Core headers (matrix, bitmatrix and sparse matrix) and for the contest folders.

## History

- 2026-11-15: Poly, Bitset, Rational, Barrett and Montgomery slice: 48 sources written (46 Poly, 2 Montgomery64); every test of all 48 problems accepted locally; expander run on 68 sources, every new bundle rerun with every output accepted by its checker; one driver fix (`inv_of_polynomials`, constant modulus); `07-judge.py` now runs grader problems that ship `sol/correct.cpp` (user's yes), and `convolution_mod` passed through it.
- 2026-11-15: all 48 accepted online (submissions #417048–#417099, 2026-11-14 UTC, by hand); records written to the sources' Status lines and the table.
- 2026-11-15: `04-submit.py` (maintainer-only submission with records), its tester and the shell hook refusal; tester quick, full, stress and hooks full passed.
- 2026-11-15: first live `04-submit.py` run (`factorize`, #417103, AC): login and submission worked; the queued status `-` was read as a verdict, fixed in `07-judge.py` (`pending`); record written; a selection now also matches source file names (the maintainer's `primalitytest` matched nothing, the problem being `primality_test`); the maintainer's rerun (`primalitytest`, #417105, AC) passed through the queue and wrote its record; a recheck then added colored verdicts (user's request), token renewal for runs past an hour and retries of network errors while waiting; both testers passed in quick, full, stress and g++-14.
