# 05 Mathematics — notes

Contracts and ownership rules moved out of the inventory on 2026-10-06. The inventory lists operations; this file holds the rules every row must satisfy.

## Status semantics

- `verified` rows are backed by the linked per-header evidence documents (`Docs/01-mod_arithmetic.md` … `Docs/06-segmentedsieve.md`); every operation in the row has a test with an independent oracle and recorded commands.
- `partial` rows keep their evidence link; the `missing:` list is planned work, not a defect in the verified part.
- `legacy-reference` rows have an archived implementation in `OLD/5-Mathematics` and no active header; names listed in the row are the intended active API, and legacy names (`exGcd`, `modLog` multiplier form, `superChiRemThm`) remain available under the new header.
- `planned` rows name the API that would be implemented; nothing in them establishes compliance.
- P012 verified the six MA01/MA02 foundation headers and re-audited them on 2026-10-07, completing the previously missing `01-mod_arithmetic.hpp` operations; see the per-header documents [01](01-mod_arithmetic.md)–[06](06-segmentedsieve.md) and the package record below.

## Domains

- Domains are part of the contract: finite fields, composite residue rings, integers/rationals and approximate reals are not interchangeable. Every operation states integer widths and overflow, modulus prime/unit assumptions, characteristic, coefficient bounds, empty/zero/singular cases and legitimate failure.
- Signed 64-bit inputs include the minimum; wide (128-bit) results prevent narrowing the magnitude 2^63. Legacy narrow adapters assert representability.
- Factorial tables in `05` require `n < mod` and a prime modulus; large-index and composite-modulus binomials belong to `12` and `37`.
- Randomized operations distinguish expected time, Monte Carlo error and verifiable Las Vegas retry, with seeds and budgets as parameters.
- Approximate methods state tolerance, error and conditioning and never silently replace exact variants; exact and approximate pivoting APIs are separately named in `16`.
- All-solution and enumeration APIs return compact parameterizations or bounded enumerations and are charged by output size.
- Prime-field roots and logs are Advanced (`11`); prime-power/composite roots (`33`) and large-order discrete-log techniques (`54`) are separate Esoteric families.
- Fast composition, factorization, multivariate and polynomial-matrix families stay in their own Esoteric headers rather than mixing tiers into `14`.

## Ownership boundaries

- Core owns reduction, modint, big-integer, rational, matrix, bitset and polynomial/FPS types and may independently implement accelerated operations from these families. Mathematics uses scalar contest implementations or the allowed Core interfaces, with no handwritten ISA kernels; dependencies remain acyclic and never include an aggregate.
- Core `16-poly.hpp` owns full composition and compositional inverse; `14` keeps scalar duplicates and `40` keeps Lagrange inversion as coefficient extraction.
- Core `10-matrix.hpp` owns characteristic/minimal polynomial and Frobenius/Jordan forms for the full type; `44` owns the scalar Hessenberg/Berkowitz/black-box reference algorithms. Core `14-sparsematrix.hpp` keeps its fixed-iteration `steadyState`; `26` owns stationary distributions that test irreducibility.
- Graphs `53-graph_counting.hpp` owns graph-facing applications and witnesses (LGV, BEST, Tutte, matching counts); matrix-tree, Pfaffian, permanent and lattice-path kernels live here.
- Geometry `19-lattice_geometry.hpp` owns lattice counts in polygons; `34` and `55` own circle and convex-curve arithmetic counts.
- Data Structures owns the convex-hull-trick and Li Chao engines and is the recommended owner of the XOR linear basis; Core `12-bitmatrix.hpp` owns packed GF(2) subspace arithmetic.
- Miscellaneous owns slope trick, bit tricks, enumeration, digit DP, knapsack, randomness and parallel binary search; Mathematics owns scalar transforms and algebraic DP optimizations (`24`, `27`).
- Strings owns distinct-subsequence counting and de Bruijn sequences.
- Integer roots have canonical ownership in `21`; `17` may call them.

## Legacy obligations

- `OLD/5-Mathematics/05-primesandfactors.hpp`, `06-phiandinverse.hpp` and `07-chiremthmandgarner.hpp` are fully accounted for by the verified headers `07`–`10` (P052; per-legacy-name mapping in each evidence document). `08-modlogandmodroot.hpp`, `13-numericalmethods.hpp` and `14-miscellaneous.hpp` stay pending until `11`, `17`–`19` account for every feature, including the `modLog` multiplier parameter. All originals stay in `OLD`.
- `OLD/5-Mathematics/09-montgomery.hpp` is a Core reference; `10-poly.hpp` and `11-moly.hpp` are references for Core Poly.
- The [transfer record](../../00-Guidelines/History/2026-09-27-monolith-transfer.md) preserves source-range/hash provenance for the former local excerpts. The last one (`FastConv`) was removed when P055 verified row 24; the original stays in `OLD/algorithms.cpp`.
- The archived closed segmented-sieve interval migrated to half-open `[l, r)` with an explicit `fromClosed` adapter.

## Evidence rules

- Check small signed domains by exhaustive references; factor products and primality, equation substitution, CRT residuals, convolution versus O(n m), interpolation reconstruction, normal-form identities, root intervals and algebraic invariants.
- Tests on source libraries are inspiration, not copied evidence. Finite tests pair with a correctness argument.
- Barrett or Montgomery outside Core needs a recorded end-to-end benchmark (see `01-principles.md`).
- A selected implementation batch refines its own API/variant checklist before coding and records discovered gaps rather than declaring unsupported completeness.

## P012 research decisions (2026-10-07)

Adopted: `fibSearch` and `expSearch` (row 02), `combiLarge`, `binomialTable` and `derangementTable` (row 05). Left out, one line each:

- `floorDivStrict`/`ceilDivStrict` (Nyaan `int_div`): the largest integer strictly below `a / b` is `ceilDiv(a, b) - 1` and the smallest strictly above is `floorDiv(a, b) + 1`; one-liners at the call site.
- `divMod` (maspypy): `floorDiv` plus `a - b * q` at the call site; the compiler merges the two divisions.
- `gcd128` (maspypy): GNU `std::gcd` accepts `__int128` directly; no wrapper needed.
- Binary gcd (hitonanode, Nyaan): the installed libstdc++ `std::gcd` is already Stein's algorithm, and `gcd64` uses it.
- `binSearchRealRelative` (maspypy): logarithmic-scale bisection is `binSearch` over the order-preserving bit pattern of nonnegative doubles (64 steps); not a separate API.
- `firstModInRange` (maspypy `first_mod_range_of_linear`): belongs with `18-floorsum.hpp` beside `minOfModOfLinear`; handed off to that row's owner, not added here.
- `dioMinLinear` (cp-algorithms "minimum x + y"): a linear objective over `DioBox` is evaluated at its two endpoint parameters `l` and `r - 1`; no extra API.
- `primePi` table (hitonanode `bs_sieve`): `upper_bound(prms, x)` on the existing prime list.
- Factorization above the sieve bound (hitonanode `Sieve::factorize`): owned by `07-primality_factorization.hpp` `factorize`.
- `powTable`, the completely multiplicative i^K table (hitonanode `enumerate_kth_pows`): handed off to `08-multiplicative_functions.hpp` `multiplicativeTable`.
- `lcmOfList` (maspypy `all_lcm`): caller-side maximum of `getPrimeFac` exponents.
- `combiNegative` and `combiInverse` (maspypy `C_negative`, `C_inv`): `(-1)^k * combiWR(n, k)` and the product `inverseFactorial(a) * factorial(b) * factorial(a - b)` from the existing tables.

## P012 package record (MA01 and MA02)

P012 owns six existing mathematics headers. MA01 was implemented and passed its full per-header verification before MA02 implementation began. Prerequisites P002/C01 and P005/C03 were already checked and their template/modular APIs were inspected. Other Mathematics inventory families and their separate packages remain outside this work. Package-wide commands and per-header counts are in each header's `## Commands and results`.

| Batch | Header | Evidence and scope |
|---|---|---|
| MA01 | `01-mod_arithmetic.hpp` | [01-mod_arithmetic.md](01-mod_arithmetic.md): signed boundaries, wide gcd/lcm/division, checked and saturating add/multiply, checked integer powers, native modular arithmetic and the inverse mod 2^64. |
| MA01 | `02-search_algorithms.hpp` | [02-search_algorithms.md](02-search_algorithms.md): integer and real binary/unimodal searches, Fibonacci-section and galloping searches, precision and termination status. |
| MA01 | `03-equation_solvers.hpp` | [03-equation_solvers.md](03-equation_solvers.md): complete linear parameterizations, bounded boxes and congruences. |
| MA02 | `04-sieve_algorithms.hpp` | [04-sieve_algorithms.md](04-sieve_algorithms.md): Eratosthenes, linear SPF/LPF, factors and multiplicative tables. |
| MA02 | `05-combinatorics.hpp` | [05-combinatorics.md](05-combinatorics.md): prime factorial tables with large-n binomials, any-ring Pascal and derangement tables, checked exact counts, Fibonacci/Lucas, Catalan, ballot and derangements. |
| MA02 | `06-segmentedsieve.hpp` | [06-segmentedsieve.md](06-segmentedsieve.md): reusable blocked bases, plain/odd/wheel-30 streaming, factorization and interval migration. |

Re-audit (2026-10-07) rules changes across all six headers: every header meets the closing-brace rule and the comment cap; the in-code contracts moved to the `## Contracts` sections of the six per-header documents; the code uses `int8_t` for Möbius tables and `lng` instead of `size_t` for indices. Sources are in [00-sources.md](00-sources.md); rejected candidates are in the research decisions above.

Confirmed findings from `/reaudit-review` (each header's document repeats its own rows):

| # | Header | Finding | Resolution |
|---|---|---|---|
| 1 | 03 | `DioSolution.g` was 0 on no solution | Fixed: `g = gcd(abs(a), abs(b))` on every result; both oracles check it |
| 2 | 06 | Odd/Wheel30 3–6x slower than Plain at high endpoints | Fixed: branch-free start offsets (`k \|= 1`, `NEXT[k % 30]`) and direct index walks; A/B table in [06-segmentedsieve.md](06-segmentedsieve.md) |
| 3 | 06 | Shared-base materialization copied every base prime | Fixed: copies only `p <= floor(sqrt(r - 1))`; 100 objects from a `10^16` base went from 1087 ms to 0.02 ms |
| 4 | 01 | `invMod2p64` listed but absent | Implemented and tested against `pow(a, -1, 2^64)` |
| 5 | 01 | Notes/evidence claimed MA01 complete while the row was partial | All five operations implemented; row verified; these notes and [01-mod_arithmetic.md](01-mod_arithmetic.md) rewritten |
| 6 | 01 | `modPow` default modulus and `exGcd` input/output aliasing untested | Op `P` checks the two-argument overload against `pow(a, b, INF64)`; boundary regression runs `exGcd(a, b, a, b)` |
| 7 | 02 | `RealSearchResult` lacked a complexity line | Added |
| 8 | 03 | Lambda closing brace `; };` | Fixed |
| 9 | 04 | `getAllFac` comment used undefined `d(a)` and `a` | Now `Q: O(log(a) + out), M: O(out); 1 <= a <= n` |
| 10 | 05, 06, 02 | `ratioProduct` lacked a complexity line | Added (also for the segmented-sieve and search detail helpers) |
| 11 | all | Space before closing braces in the segmented sieve | Fixed with every other brace violation in the six headers and six testers |
| 12 | 04, 06 | `size_t` loops and callback index; detail helpers without complexity lines; `signed char` | Loops and the `forEachFactor` callback index are `lng`; helpers commented; `int8_t` |
| 13 | all testers | Tester `main` closed with a lone brace | Fixed |

The `@reviewer` pass found no correctness defect in the code; its per-header notes and independent probes are recorded in the per-header `## Commands and results`. The oracles across the package are Python unbounded integers and built-in modular powers and inverses, matrix powers, enumerated arrangements and paths, trial factors and divisors, divisor-inversion identities, analytic monotone and minimum positions, linear scans, and direct enumeration of bounded equations. Finite tests are paired with the correctness arguments in each per-header document. No Barrett/Montgomery or ISA-specific code is used.

Legacy originals remain untouched in `OLD` and `97-Legacy`. Handoffs from the research sweep (not P012 work): `firstModInRange` to `18-floorsum.hpp`, the completely multiplicative power table to `08-multiplicative_functions.hpp`. There is no unfinished owned P012 feature or verification gap. Separate planned inventory families remain planned. No online submission or online-acceptance claim was made.

## P052 research decisions (2026-10-09)

Adopted: `factorizeTrial` (legacy `getPrimeFacSlow`), `divisors` from a factorization and `maxDivisorCount` (row 07); `omega`, `bigOmega`, `DivisorArray` `setMultiplicative`/`multipleZeta`/`multipleMobius`/`size`/`operator[]` (row 08); `crtMod` and the `CrtResult.overflow` flag (row 10). Renamed: `divisorCountFn`/`divisorSumFn` became `divisorCountTable`/`divisorSumTable` (point values already in row 07); `powerTable` is fixed as `i^k`. Left out, one line each:

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

## P052 package record (MA03 and MA04)

| Batch | Header | Evidence and scope |
|---|---|---|
| MA03 | `07-primality_factorization.hpp` | [07-primality_factorization.md](07-primality_factorization.md): deterministic u64 Miller–Rabin and Brent rho on Core `Montgomery64` (benchmark: factorization 33–36% faster than `__int128 %`), factorizations, divisors, prime powers, coarsest coprime base, highly composite search. |
| MA04 | `08-multiplicative_functions.hpp` | [08-multiplicative_functions.md](08-multiplicative_functions.md): point functions, the linear-sieve multiplicative hook and its tables, Dirichlet tables, divisor arrays. |
| MA04 | `09-modinverse.hpp` | [09-modinverse.md](09-modinverse.md): extended Euclid, Fermat and Pierce inverses, linear table, batch and zero-tolerant inverses, `i^k` table. |
| MA04 | `10-crt.hpp` | [10-crt.md](10-crt.md): status CRT with overflow detection, scaled congruences, Garner digits and target reduction, non-coprime CRT modulo a target, incremental form, legacy `superChiRemThm`. |

Compact rerun (2026-10-09, after P231): `07` now has dependency-free `millerRabin64Compact`, `isPrimeCompact`, `pollardRhoBrentCompact` and `factorizeCompact`. Its algorithms are templates over a product functor, so they are written once. Headers `08`–`10` use no Barrett or Montgomery directly (`09` is generic over the modint type), so they need no twins.

Incident: a first oracle row sent `n = 0` to the optimized (`-DNDEBUG`) `factorize`, whose stripped precondition left an infinite `push_back` loop. Memory growth killed the user's editor. The loops in `factorize`, `factorizeTrial` and `coprimeBase` now terminate on 0, the oracle sends only valid inputs, and every later run used `systemd-run --user --scope -p MemoryMax=... -p MemorySwapMax=0`.


## P055 research decisions (2026-10-09)

Adopted: `convolution` (dispatcher), `nttDoubling` and `convolutionU128` (row 13); `kroneckerPowerTransform` widened from a 2 x 2 to any D x D kernel, and `disjointUnionConvolution` defined as the q-operand ranked product (row 24). Changed: `convolutionLong` uses three-prime CRT lifted to signed 64 bits instead of a split FFT (exact with no floating-point error analysis); `ntt`/`fft` produce bit-reversed spectra (ACL convention), which is what makes `transposedNtt` and `nttDoubling` distinct operations. Left out, one line each:

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

## P055 package record (MA06 and MA12)

| Batch | Header | Evidence and scope |
|---|---|---|
| MA06 | `13-convolution.hpp` | [13-convolution.md](13-convolution.md): naive, Karatsuba, complex FFT, NTT over any static NTT prime (bit-reversed spectra, transposed, doubling, root tables), three-prime CRT products (signed 64-bit, unsigned 128-bit, any modulus), block products beyond the transform limit, the dispatcher and the cyclic/negacyclic/truncated/middle/correlation/tensor/2-D wrappers. |
| MA12 | `24-transform_algorithms.hpp` | [24-transform_algorithms.md](24-transform_algorithms.md): subset/superset zeta and Mobius, Walsh-Hadamard with exact inverse, bitwise and gcd/lcm convolutions, divisor/multiple transforms, ranked transforms, subset and q-operand disjoint-union convolution, D x D Kronecker powers, and the legacy `FastConv` names as thin wrappers. |
