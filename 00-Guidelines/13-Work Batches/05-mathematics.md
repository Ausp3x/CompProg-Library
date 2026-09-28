# 05-Mathematics implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../05-Mathematics/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| MA01 | `01-mod_arithmetic.hpp`, `02-search_algorithms.hpp`, `03-equation_solvers.hpp` | M | C01 | Modular arithmetic, search and equation solvers. |
| MA02 | `04-sieve_algorithms.hpp`, `05-combinatorics.hpp`, `06-segmentedsieve.hpp` | M | C01, C03, MA01 | Sieve families and combinatorics, including with-replacement counting and explicit migration of closed segmented-sieve intervals; the existing combinatorics header uses C03 modular integers. |
| MA03 | `07-primality_factorization.hpp` | L | C01, MA01 | Deterministic 64-bit primality and factorization. |
| MA04 | `08-multiplicative_functions.hpp`, `09-modinverse.hpp`, `10-crt.hpp` | L | C01, MA01, MA03 | Multiplicative functions, inverses and CRT, including systems of scaled linear congruences. |
| MA05 | `11-discrete_log_root.hpp` | L | C01, MA01, MA03, MA04 | Discrete logs including a multiplier, primitive roots and prime-field roots; preserve zero/nonunit and least-solution semantics. |
| MA06 | `13-convolution.hpp` | L | C01, MA01 | Standalone ordinary convolution; independent of Core Poly. |
| MA07 | `14-polynomial_algorithms.hpp`, `15-linear_recurrence.hpp` | L | C01, MA01, MA06 | Ordinary polynomial algorithms and recurrences; MA06. |
| MA08 | `16-linear_algebra.hpp` | L | C01, MA01 | Scalar linear algebra and banded/tridiagonal solves with explicit exact/approximate pivoting domains. |
| MA09 | `17-numerical_methods.hpp`, `21-integer_roots.hpp` | L | C01, MA01 | Stable numerical roots, quadrature, compensated sums and exact integer roots. |
| MA10 | `18-floorsum.hpp`, `19-josephus.hpp`, `20-continued_fraction.hpp` | M | C01, MA01 | Floor sum, Josephus and continued fractions. |
| MA11 | `22-interpolation.hpp`, `23-matrixtree.hpp` | L | C01, MA01, MA07, MA08 | Interpolation and matrix-tree; MA07–MA08 as needed. |
| MA12 | `24-transform_algorithms.hpp` | L | C01, MA01, MA02 | Audit current OR/AND/XOR/GCD/LCM transforms, then add SOS/ranked subset convolution; MA51 owns set FPS. |
| MA13 | `25-game_theory.hpp`, `26-probability.hpp` | L | C01, MA01, MA08 | Impartial games and probability/Markov models, including hitting and stationary-state behavior; reuse MA08 for linear-system solves. MA43 owns partizan/matrix games. |
| MA14 | `27-optimization.hpp` | L | C01, DS11, MA01 | Discrete DP optimization with proof assumptions; DS11 where useful. |
| MA15 | `31-summatory_functions.hpp` | XL | C01, MA01 | Summatory functions and prime-counting algorithms. |
| MA16 | `32-dirichlet_series.hpp` | L | C01, MA01, MA04, MA15 | Dirichlet-series algebra; MA04/MA15. |
| MA17 | `33-modular_square_roots.hpp`, `34-quadratic_congruence.hpp` | L | C01, MA01, MA05 | Modular roots and quadratic congruences. |
| MA18 | `36-integer_factor_advanced.hpp` | XL | C01, MA01, MA03 | Advanced integer factorization with explicit randomized budgets; MA03. |
| MA19 | `37-fastfactorial.hpp`, `38-bernoulli.hpp` | XL | C01, MA01 | Fast factorial and Bernoulli/polynomial-exponential sums; keep separate from another XL job. |
| MA20 | `41-padic_arithmetic.hpp`, `43-rationalreconstruction.hpp` | L | C01, MA01 | p-adic arithmetic and rational reconstruction. |
| MA21 | `42-finite_fields.hpp` | XL | C01, C03, MA01, MA05, MA07 | Finite fields and irreducible-polynomial machinery; C03/MA07, with MA05 group algorithms where needed. |
| MA22 | `44-determinant_advanced.hpp` | XL | C01, C07, MA01, MA08 | Advanced determinant algorithms; MA08/C07 as appropriate. |
| MA23 | `45-contourintegral.hpp` | M | C01, MA01, MA09 | Contour-integral/root research with stability contract. |
| MA24 | `35-lattice_reduction.hpp` | XL | C01, MA01 | Lattice reduction and exact/approximate coefficients. |
| MA25 | `39-special_functions.hpp` | M | C01, MA01, MA09 | Special functions with numeric stability contract. |
| MA26 | `12-combinatorics_advanced.hpp` | L | C01, MA01, MA02, MA03, MA04 | Composite-modulus and advanced combinatorics; MA01–MA04. |
| MA27 | `28-simplex.hpp` | XL | C01, MA01, MA08 | Simplex with Phase I/II, exact/floating policy and degeneracy. |
| MA28 | `40-generating_functions.hpp` | L | C01, MA01 | Generating functions, species and Pólya/Burnside. |
| MA29 | `29-polynomial_roots.hpp` | XL | C01, MA01, MA07, MA09 | Certified real-root isolation and separately specified approximate real/complex root finding. |
| MA30 | `46-polynomial_factorization.hpp` | XL | C01, MA01, MA07, MA21, MA24 | Finite-field and integer/rational polynomial factorization, with normalization, lifting and certification contracts. |
| MA31 | `47-nimber.hpp` | L | C01, MA01 | Finite nimber fields, arithmetic, basis conversion and compact alternatives to large tables. |
| MA32 | `48-combinatorial_linear_algebra.hpp` | XL | C01, MA01, MA08, MA12 | Permanent, Pfaffian, Hafnian and loop-Hafnian, with characteristic constraints and graph-counting interfaces. |
| MA33 | `49-integer_linear_algebra.hpp` | XL | C01, C04, MA01, MA08, MA24 | Integer normal forms, unimodular witnesses and complete integer/modular solution lattices. |
| MA34 | `50-multivariate_polynomial.hpp` | XL | C01, MA01, MA07 | Multivariate polynomial/FPS arithmetic, evaluation, elimination and Groebner-basis research. |
| MA35 | `51-polynomial_matrix.hpp` | XL | C01, MA01, MA07, MA08 | Polynomial matrices, approximant/normal forms, P-recursive products and composite-ring recurrence synthesis. |
| MA36 | `52-relaxed_convolution.hpp` | L | C01, MA01, MA06, MA07 | Online, relaxed and semi-relaxed convolution and associated FPS/composition operations. |
| MA37 | `53-semiring_convolution.hpp` | L | C01, MA01, MA14 | Min/max-plus convolution with explicit convexity, Monge or bounded-domain assumptions. |
| MA38 | `54-discrete_log_advanced.hpp` | L | C01, MA01, MA03, MA05 | Advanced discrete logs, subgroup/order reduction and composite-group decomposition with explicit randomness and bounds. |
| MA39 | `55-floor_sum_polynomial.hpp` | L | C01, MA01, MA10, MA19 | Polynomial floor-sum moments, floor products, lattice-point generating functions and floor-monoid products. |
| MA40 | `56-coding_theory.hpp` | XL | C01, MA01, MA07, MA21 | Finite-field codes, unique decoding and separate list-decoding research with distance limits. |
| MA41 | `30-modular_power_towers.hpp` | M | C01, MA01, MA04 | Modular exponent towers, noncoprime valuation thresholds and stabilization conventions. |
| MA42 | `57-quadratic_integer.hpp` | L | C01, C04, MA01, MA03 | Quadratic integer rings, valid Euclidean gcds, unit normalization and norm representations. |
| MA43 | `58-partizan_games.hpp` | L | C01, MA01, MA13, MA27 | Partizan-game values and canonical forms, matrix zero-sum games and separate misere research. |
| MA44 | `59-zero_sum.hpp` | M | C01, MA01 | Constructive zero-sum subsequences in finite cyclic groups with witnesses and explicit bounds. |
| MA45 | `60-tableau_algorithms.hpp` | L | C01, MA01, MA26, MA28 | Young tableaux, RSK/inverse and symmetric-function identities with shape and exact-divisibility conventions. |
| MA46 | `61-algebraic_numbers.hpp` | XL | C01, MA01, MA29, MA30 | Exact algebraic numbers, isolating representations, arithmetic and embeddings with degree/height growth. |
| MA47 | `62-ball_arithmetic.hpp` | XL | C01, MA01, MA09, MA48 | Certified real/complex enclosures, directed rounding and interval root certificates with precision escalation. |
| MA48 | `63-multiprecision_float.hpp` | XL | C01, C04, MA01 | Variable-precision binary floats, rounding, special values and correctly rounded I/O/function research. |
| MA49 | `64-group_algorithms.hpp` | XL | C01, MA01 | Permutation groups, stabilizer chains, Schreier-Sims and finite Abelian decomposition. |
| MA50 | `65-convolution_specialized.hpp` | L | C01, MA01, MA06, MA21 | Special convolution domains, multiplication-index transforms and unusual transform lengths. |
| MA51 | `66-set_power_series.hpp` | L | C01, MA01, MA07, MA12 | Subset power-series algebra, composition/power projection and ranked/multivariate extensions. |
