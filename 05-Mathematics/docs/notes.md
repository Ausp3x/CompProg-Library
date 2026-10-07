# 05 Mathematics — notes

Contracts and ownership rules moved out of the inventory on 2026-10-06. The inventory lists operations; this file holds the rules every row must satisfy.

## Status semantics

- `verified` rows are backed by the linked package evidence (`90`–`96`); every operation in the row has a test with an independent oracle and recorded commands.
- `partial` rows keep their evidence link; the `missing:` list is planned work, not a defect in the verified part. In `24-transform_algorithms.hpp` the present `FastConv` operations are migrated code that was never verified; the row is partial only because the planned additions are explicit.
- `legacy-reference` rows have an archived implementation in `OLD/5-Mathematics` and no active header; names listed in the row are the intended active API, and legacy names (`exGcd`, `modLog` multiplier form, `superChiRemThm`) remain available under the new header.
- `planned` rows name the API that would be implemented; nothing in them establishes compliance.
- P012 verified the six MA01/MA02 foundation headers and re-audited them on 2026-10-07, completing the previously missing `01-mod_arithmetic.hpp` operations; see [package evidence](p012.md).

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

- `OLD/5-Mathematics/05-primesandfactors.hpp`, `06-phiandinverse.hpp`, `07-chiremthmandgarner.hpp`, `08-modlogandmodroot.hpp`, `13-numericalmethods.hpp` and `14-miscellaneous.hpp` stay in `OLD` until the active headers `07`–`11`, `17`–`19` account for every feature, including `superChiRemThm` and the `modLog` multiplier parameter.
- `OLD/5-Mathematics/09-montgomery.hpp` is a Core reference; `10-poly.hpp` and `11-moly.hpp` are references for Core Poly.
- [Local legacy excerpts](../97-Legacy/00-index.md) and the [transfer record](../../00-Guidelines/14-monolith-transfer.md) preserve unchanged bodies with source-range/hash provenance; compilation does not verify behavior.
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
