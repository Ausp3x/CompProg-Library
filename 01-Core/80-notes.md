# 01 Core — notes

Contracts, numeric policies and ownership boundaries moved out of the inventory. Operation lists live only in [00-index.md](00-index.md); sources in [81-sources.md](81-sources.md).

## Status vocabulary

- `existing-unverified` means a header exists, not that every operation in its row is implemented; the `required:` groups in those rows are planned until implementation and evidence establish otherwise. `legacy-reference` means archived source only. Verified rows link their package evidence.
- Each header must compile when included alone via its declared direct dependencies. Document public operation time/memory, domains and legitimate failures. Full profiles require applicable compile-time acceleration plus a scalar fallback with identical results and measured thresholds; no backend may silently change arithmetic semantics.
- Differential-check full against mini and against independent exact references: empty/zero/singular cases, composite moduli, overflow limits, aliasing, threshold neighbours and scalar versus accelerated paths.

## Core order and minis

- Preserve the fixed order 01–18; append a new foundational type only for a demonstrated shared need. Core has no Basic/Advanced/Esoteric sections; `Basic` selects the compact aggregate/notebook profile, not a performance tier.
- Full and mini implementations live in separate headers with identical semantics on their shared domain. Each mini is independently copyable: no dependency on a sibling, full type, detail namespace or reduction header.
- `98-Basic.hpp` includes minis where they exist plus full types Basic consumers need; `10-matrix.hpp` stays Basic until `11-matrixmini.hpp` exists.

## Reduction and modular types

- Define exact modulus and dividend ranges, canonical versus lazy residues, wide intermediate bounds, zero/modulus-one handling, aliasing and transform boundaries. Barrett applies to every nonzero word modulus including even ones; Montgomery requires an odd modulus. Never infer full unsigned coverage merely from the storage width.
- Modular types state normalized negative/min-signed construction, safe raw construction, the meaning of ordering (canonical representatives), division by nonunits, negative exponents and batch inverse. Static primality knowledge and runtime primality assumptions differ (`prm` hint on `setMod`). Every modulus change invalidates values and caches under the documented rule; IDs and widths remain independent.
- Modulus one is the zero ring: zero is the identity and a unit; prime-root APIs are unavailable there.
- Mini reductions, by design: no primality flags/detection, prime roots, batch inverse, decimal streams, integer casts, truthiness, ordering or increment/decrement; `val()` passes residues outward. `ModInt61` (Mersenne) is a full-type addition only.
- Arithmetic modulo 2^64 is native `ulng` wraparound; a dedicated residue type was not adopted (see the plan delta placement question).

## Integers

- Rational: denominator positive and reduced, unique zero, cancellation before multiplication, exact overflow/failure rules for bounded `T` (`tryAdd`/`tryMul`), comparison without overflowing cross products. Ordinary rational values exclude NaN/infinity; do not couple this to InfInt's legacy sentinel.
- InfInt sentinel audit: both full and mini contain `is_inf`, signed-infinity parsing/arithmetic and truncating native conversions; full InfInt also has exact-bit-length `rand`. These are visible migration decisions: define the supported sentinel operations and no-answer cases and the conversion overflow behaviour, or account for a replacement before removal. Finite integer semantics must remain unambiguous.
- Full integer contracts: import/export limbs and bases, checked native conversion, decimal/hex/general-base parse and print, signed division modes stated as truncating/floor/Euclidean, exact division, two's-complement bitwise semantics, shifts, bit length/scan/popcount. Sampling needs a reproducible caller-supplied seed and distinct bounded versus exact-bit-length contracts. `UInt<N>` is a fixed-limb unsigned type with wraparound semantics, distinct from the dynamic signed `InfInt`.
- Backend policy: select implementations and measured crossovers for supported workloads (GMP's threshold names are the model). Schönhage–Strassen, Fürer and Harvey–van der Hoeven stay in the tracked research comparison; never claim asymptotic machinery is practically fastest without measurement.
- Mini InfInt keeps base 2–36 I/O, the legacy sentinel decision and native conversions explicitly; it adds only `pow` and `gcd`, omitting roots, bitwise operations and modular inverses.

## Matrices

- State operation requirements individually: semiring products need no division; field elimination does; composite residue rings require unit pivots or ring-safe methods (Bareiss, CRT, Berkowitz). Finite-field rank is not an epsilon test.
- Full Matrix must derive moduli through `T::mod()` with correct word widths and replace static field constraints with a runtime contract for dynamic types; see the [C03 handoff](24-modint.md#dependent-owner-handoff). Reconstruct matrices and cached factors after dynamic modulus changes.
- Approximate algebra is separately named and API-scoped: pivoting/rank tolerance policy, conditioning, residual/error criteria and nonconvergence are stated; Cholesky requires positive-definite symmetric/Hermitian input and pivoted LDL its stated indefinite domain. Complex conjugation is distinct from transpose. Add numerical extensions only with defensible stability evidence.
- Structured solves (Toeplitz/Hankel/Vandermonde, banded/tridiagonal including cyclic, rank-one updates) state unit-pivot conditions and a generic fallback. The existing `hafnian` is preserved and audited; permanent/Pfaffian/Hafnian/loop-Hafnian carry explicit characteristic assumptions, and Mathematics `48-combinatorial_linear_algebra.hpp` owns the scalar research APIs (duplication allowed).
- GF(2): packed layouts, padding bits, rectangular multiplication, Four Russians crossover, row/column spaces and intersection, all-solution witnesses and word-boundary cases. Boolean OR-AND reachability multiplication is a separately named semiring operation, never GF(2) multiplication.
- Sparse: sorted indices, duplicates and explicit zeros, rectangular/empty shapes, transpose/conversions, dense crossover and fill-in. Wiedemann/block Wiedemann/Lanczos and random preconditioning need field-size assumptions and Monte Carlo versus Las Vegas guarantees. `steadyState` performs a fixed number of products and tests no convergence; keep it under an honest contract and specify `stationaryState` separately with residual, convergence and periodic/reducible-chain behaviour. CG requires SPD/Hermitian PD; GMRES/BiCGSTAB and preconditioners need separate convergence/breakdown contracts. Mathematics `26-probability.hpp` owns the Markov-chain model.
- Mini matrices: `MatrixMini` over stated fields only; `BitMatrixMini` provides elimination, rank, consistency, one solution and a nullspace basis; `SparseMatrixMini` keeps dimensions, triplet ingestion with duplicate/zero rules, vector product and repeated-product powers. Narrow the mini scope before accepting slower algorithms.

## Polynomials and FPS

- Representation: coefficient order, zero normalization versus retained FPS precision, truncation, ring/field/characteristic constraints and sparse/dense crossover. Integers/rationals, prime fields, composite residues and approximate coefficients have distinct supported operation sets; characteristic-induced nonexistence or nonuniqueness must be visible (exp/log/sqrt/kth root conditions, valuation and branch choices).
- Convolution: FFT with a quantified exactness envelope; NTT with a documented root cache lifetime and maximum transform length; arbitrary-mod CRT with signed reconstruction. Convolution modulo 2^64 and over binary extension fields are separate exact domains, also duplicated as scalar Mathematics `65-convolution_specialized.hpp`.
- Evaluation/interpolation: repeated points require Hermite data or explicit rejection. Composition names its algorithm (Brent–Kung baseline; Kinoshita–Li as the researched faster alternative) and reversion its method (Lagrange inversion via power projection).
- General multivariate systems, Gröbner bases, polynomial matrices, set power series and specialized combinatorial counts remain Mathematics families, not promises of the univariate type.
- Mini Poly is restricted to an NTT-friendly prime field with the listed subset; omitted features and characteristic restrictions are documented in its header.

## Bitset

- Shared primitive for BitMatrix, subset DP and graph search. Define size mismatch, padding, shifts at/above length, tail masking, proxy invalidation and mutation aliasing. The generic bitset must not depend on BitMatrix (no dependency cycle). Dynamic succinct rank/select and compressed bitmap indexes belong to Data Structures; `std::bitset` remains the fixed-size alternative.

## Legacy and provenance

- Original bodies remain in `OLD` and [97-Legacy](97-Legacy/00-index.md); compilation of an excerpt does not verify semantics, completeness, performance, indexing or complexity claims. The extraction map in `00-Guidelines/15-monolith-map.json` is authoritative for provenance; delete from the archive only after an audited implementation accounts for every feature.
- Legacy `Poly` (double FFT over `lng`) and `Moly` (NTT over `mint`) in `OLD/5-Mathematics` are the behavioural references for `16-poly.hpp`; their public names (`divMod`, `chirpZ`, `inter`, `comp`, `invS`, `powS`, `solveKthTerm`, `calcMinLinRec`, `guessKthTerm`, `shift`) must each map to a listed operation or a recorded removal.
