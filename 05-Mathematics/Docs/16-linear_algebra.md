# 16-linear_algebra.hpp — evidence

`16-linear_algebra.hpp` is the contest scalar linear-algebra family on `Mat<T> = vector<vector<T>>`. It covers:

- Exact elimination over a field: reduced row echelon form, rank, determinant, inverse, adjugate of any rank, linear systems with the full solution set, several right-hand sides, nullspace.
- Composite moduli and integers: determinant and inverse modulo any m by Euclidean row reduction, and Bareiss fraction-free integer determinants.
- Approximate reals: Crout LU with partial pivoting and its solve, partial-pivoting solve, rank and determinant.
- Structured systems: tridiagonal (Thomas), cyclic tridiagonal and banded solves without pivoting.
- Matrix powers and power sums, Sherman–Morrison–Woodbury updates of an inverse, an incremental `FieldBasis`, and the randomized Freivalds and Schwartz–Zippel checks.

The functions are free and generic over the scalar type `T`, except `determinantMod` and `inverseMod` (`lng` entries). The header depends on `01-mod_arithmetic.hpp` (`gcd64`, `extendedGcd`, `modNorm`). It uses no Barrett/Montgomery and no ISA-specific code. Core `10-matrix.hpp` owns the accelerated matrix type; this header is the scalar duplicate `02-structure.md` allows.

## Contracts

Common conventions:

- **Shapes.** `Mat<T>` is row-major; every row of a matrix has the same length (asserted at entry of every elimination and product). A `Mat` without rows is 0 x 0, so an n x 0 matrix needs n empty rows.
- **Scalar domains.** "Exact field" means `T` with `+ - * /`, `==`, `T(0)`, `T(1)` and exact arithmetic: Core modints with a prime modulus, rationals. Exact-field operations reject floating `T` at compile time (`static_assert`), and the `Real` operations and `luDecompose` accept only floating `T`, so exact and approximate pivoting are separately named (`03-cpp.md`, API contracts). "Any ring" needs only `+ *` and `T(0)`, `T(1)`.
- **Pivoting.** Exact operations pivot on the first nonzero entry of the column. Floating operations pivot on the largest `|x|` in the column (partial pivoting) and treat `|x| <= eps` as zero; `eps` is absolute (default `1e-9`), so scale inputs to magnitude about 1.
- **Status results.** A no-answer is a `bool` `false` (structured solves, inverses, `FieldBasis`) or an empty `Mat` (`solveLinear`, `solveLinearReal`), which never collides with a solution: a solvable system returns at least one row. After `false`, in-place outputs are unspecified unless the function says untouched.
- **Aliasing.** Inputs are taken by value or read completely before an output is written, so an output may share storage with any input (`solveLinearMultiRhs(a, b, b)`, `tridiagonalSolve(v, v, v, v)`, `lowRankUpdateInverse(a, a, a)`).
- **Violated preconditions** (non-square input, mismatched sizes, `mod < 1`, bad `rollback` size) are asserted at entry. Under `NDEBUG` the result is unspecified; no operation loops or allocates beyond its normal bounds.

### Mat, identityMatrix, matrixProduct

- **`Mat<T>`** is an alias, not a type, so judge input reads straight into it.
- **`identityMatrix<T>(n)`** is the n x n identity.
- **`matrixProduct(a, b)`** multiplies n x m by m x k in `O(n m k)`, i-k-j order, skipping zero `a[i][k]`. Any ring.

### rref, matrixRank, determinant, nullspace

- **`rref(a)`** reduces `a` in place to reduced row echelon form (pivots 1, zero above and below) and returns the pivot columns in increasing order. `O(n m r)` for rank r.
- **`matrixRank(a)`** is forward elimination only, about half of `rref`'s work. The name avoids `std::rank`, which `using namespace std` in the template makes ambiguous.
- **`determinant(a)`** is forward elimination with the sign of each row swap; 1 for 0 x 0.
- **`nullspace(a)`** returns a basis of `{x : A x = 0}` as rows: one vector per free column f, with `x[f] = 1`, zero at the other free columns. It is empty when only `x = 0` solves. Kept beside the basis of `solveLinear(A, 0)` by user decision (2026-10-28): no right-hand side.

### solveLinear, solveLinearMultiRhs

- **`solveLinear(a, b)`** returns an empty `Mat` when `A x = b` has no solution. Otherwise row 0 is the solution with zero free variables and rows 1.. are exactly `nullspace(a)`, so the solution set is row 0 plus their span (Library Checker `system_of_linear_equations` output).
- **`solveLinearMultiRhs(a, b, x)`** solves `A X = B` for n x k `B` in one elimination: `x` (m x k) gets the zero-free-variable solution of each column; `false`, with `x` untouched, when any column is inconsistent.

### inverse, adjugate, lowRankUpdateInverse

- **`inverse(a)`** is Gauss–Jordan on `[A | I]`; `false` iff `A` is singular.
- **`adjugate(a)`** works for every rank. Rank n: `det(A) A^-1` from one elimination. Rank `<= n - 2`: zero. Rank `n - 1`: `adj(A) = c x y^T` with `A x = 0` (kernel of the reduced matrix) and `y^T A = 0` (the last row of the elimination transform, since `E A = R` has a zero last row); one cofactor `adj[j][i] = (-1)^(i+j) det(A without row i, column j)` with `x_j y_i != 0` fixes `c`. Two eliminations in all, `O(n^3)`.
- **`lowRankUpdateInverse(ainv, u, v)`** (u is n x k, v is k x n, asserted) turns `ainv = A^-1` into `(A + U V)^-1` by Sherman–Morrison–Woodbury: `A^-1 - A^-1 U (I + V A^-1 U)^-1 V A^-1`, `O(n^2 k + k^3)`. `false`, `ainv` untouched, iff the k x k capacitance matrix is singular, which by the matrix determinant lemma means `A + U V` is singular. `k = 0` and `n = 0` return `true` unchanged. Exact field.

### determinantMod, inverseMod

- **Domain.** `mod` in `[1, 2^63)`; any `lng` entries, normalized to `[0, mod)`. Results in `[0, mod)`; `mod = 1` gives determinant 0 and the zero inverse (every element of Z/1 is a unit).
- **Algorithm.** Per column, a row whose entry is a unit mod m becomes the pivot: it is scaled to 1 (the determinant takes the factor) and clears the other rows with one multiply-add per entry. When no entry is a unit, Euclid runs on the integer representatives of the pivot and each lower entry, records the 2 x 2 unimodular transform (sign flips per step), and applies it to the two rows once. Each column's Euclid chains telescope through the decreasing pivot, so the total is `O(n^3 + n^2 log(mod))`.
- **`inverseMod(a, mod)`** runs the same elimination on `[A | I]`, clearing above the pivots too; it stops at the first non-unit pivot and returns `false` iff `det(A)` is not a unit (a non-unit pivot g after Euclid gives `det = g det(rest)` on that block).
- Products use `ulng` for `mod < 2^32` and `ulll` above.

### determinantInteger

- **`determinantInteger<T>(a)`** is Bareiss's fraction-free elimination with row swaps for zero pivots: every intermediate entry is a minor of the input, so each division is exact. `T` is an integer type with exact `/` (`lng`, `lll`, a big integer); the product of any two minors must fit `T` (`lll`: every minor below 2^63 in magnitude). The row named "fraction-free Euclid" before P058; Euclidean reduction over Z has no polynomial bound on entry growth, so Bareiss replaced it.

### luDecompose, luSolve

- **`luDecompose(a, perm, eps)`** is Crout's method with partial pivoting: in place, the lower triangle with the diagonal is `L`, the strict upper triangle is the unit upper `U`, and row i of `L U` is row `perm[i]` of `A`. `false` when the best pivot of a column has `|x| <= eps`. Floating `T` only.
- **`luSolve(a, perm, b)`** solves `A x = b` from a successful factorization in `O(n^2)`, by forward then back substitution.

### matrixPower, matrixPowerSum

- **`matrixPower(a, e)`**: binary powering, `a^0 = I`. Any ring.
- **`matrixPowerSum(a, e)`** returns `sum_{i<e} a^i` (zero for `e = 0`) along the bits of e from the top, keeping `S_k = sum_{i<k} a^i` and `P_k = a^k`: `S_2k = S_k + P_k S_k`, `S_{k+1} = S_k + P_k`. At most 3 products per bit and no inverse, so any ring; the block matrix `[[A, I], [0, I]]` would cost 8 times more per product.

### tridiagonalSolve, cyclicTridiagonalSolve, bandedSolve

- **`tridiagonalSolve(a, b, c, d)`** solves `a[i] x[i-1] + b[i] x[i] + c[i] x[i+1] = d[i]` (`a[0]`, `c[n-1]` ignored) by the Thomas algorithm; `d` becomes `x`. No pivoting: `false` iff some leading principal minor of order 1 .. n is zero (the k-th pivot is `D_k / D_{k-1}`). Stable over floats for diagonally dominant or symmetric positive definite matrices; floating pivots are compared with exact zero.
- **`cyclicTridiagonalSolve(a, b, c, d)`** adds `a[0]` at column n-1 of row 0 and `c[n-1]` at column 0 of row n-1. For n >= 3 it eliminates with the last column carried as a fill vector and the last row's fill walking along the diagonal, about twice Thomas. `false` iff a leading principal minor of order 1 .. n-1 or the determinant is zero. n = 1 folds to `(a0 + b0 + c0) x0 = d0` and n = 2 to `[[b0, a0 + c0], [a1 + c1, b1]]`. This matches Thomas's condition, unlike Sherman–Morrison, whose modified diagonal can break down on a matrix Thomas solves.
- **`bandedSolve(band, p, q, d)`** takes `band[i][j - i + p] = A[i][j]` for `-p <= j - i <= q` (rows of p + q + 1, entries outside the matrix ignored) and eliminates without pivoting, so the fill stays inside the band: `O(n p q)`. Same breakdown condition as Thomas.

### solveLinearReal, matrixRankReal, determinantReal

- **`solveLinearReal(a, b, eps)`**: partial-pivoting Gauss–Jordan; columns whose best pivot has `|x| <= eps` are free; inconsistent when a remaining right-hand side has `|r| > eps`. Result shape of `solveLinear`.
- **`matrixRankReal(a, eps)`**: number of pivots above `eps`; the numerical rank depends on `eps` and the scaling.
- **`determinantReal(a)`**: partial pivoting with no tolerance (0 only when a whole remaining column is exactly 0). The relative error grows with the condition number.

### FieldBasis

- **`FieldBasis<T>(m)`** keeps the accepted vectors of length m reduced in insertion order: vector i has a 1 at its pivot `piv[i]`, is zero before it and at every earlier pivot, and `comb[i]` expresses it in the accepted vectors 0..i.
- **`insert(v)`** reduces v and appends it with its pivot; `false` when v is in the span. **`represent(v, coef)`** returns `true` and `v = sum coef[i] accepted_i` when v is in the span. **`rollback(r)`** keeps the first r accepted vectors (no later vector entered an earlier one); `r` outside `[0, size()]` is asserted and clamped. **`size()`** is the rank. Each operation is `O(r (m + r))`. Exact field (`static_assert` in the constructor).

### freivaldsProductCheck, schwartzZippelIdentityCheck

- **Randomness.** `std::mt19937_64(seed)`; each random scalar is `T(x)` for x uniform in `[0, 2^31)`. A value of a prime field of size p then has probability at most `ceil(2^31 / p) / 2^31 <= 2 / min(p, 2^31)`.
- **`freivaldsProductCheck(a, b, c, rounds, seed)`** compares `A (B r)` with `C r` for `rounds` random vectors, `O(rounds (n m + m k + n k))`. Monte Carlo, one-sided: `false` proves `A B != C`; `true` is wrong with probability at most `(2 / min(p, 2^31))^rounds` over a prime field (`2^-31` per round over overflow-free integers). Composite rings void the bound.
- **`schwartzZippelIdentityCheck<T>(f, vars, rounds, seed)`** evaluates a black-box polynomial `f(vector<T>)` at `rounds` random points. `false` proves `f != 0`; `true` is wrong with probability at most `(2 d / min(p, 2^31))^rounds` for total degree d (Schwartz–Zippel). Monte Carlo, one-sided.

### Legacy mapping

| Legacy | Replacement |
|---|---|
| `old_gaussjordan.cpp` `gaussJordan(vector<vector<double>>&)` (EPS pivots, singular flag) | `solveLinearReal`, `matrixRankReal` (partial pivoting; the original took the first entry above EPS) |
| `old_matrix.cpp` `Matrix` (n x n `ll` mod `MOD`), `power` | `Mat<mint>`, `matrixProduct`, `matrixPower` |
| `algs.cpp`, `algsbetter.cpp` `Matrix`: `eye`, `operator*`, `pow` | `identityMatrix`, `matrixProduct`, `matrixPower` |
| `algs.cpp`, `algsbetter.cpp` `Matrix::det` (field branch; Euclid branch for non-field `T`) | `determinant`; `determinantMod`, `determinantInteger` |
| `algs.cpp`, `algsbetter.cpp` `Matrix::solveLin` (empty `Matrix` when inconsistent) | `solveLinearMultiRhs` |
| `algsbetter.cpp` `minPlusMatrixExp` | not adopted here: a semiring power, Core Matrix `mulSemiring` |
| `00-matrix.hpp` `Matrix` arithmetic, `concat`, `FastMatMul` | Core `10-matrix.hpp` (P020); this row keeps `matrixProduct` only |
| row 16 `gaussJordan` (planned name) | `rref` (user decision 2026-10-28) |
| row 16 `rank`, `rankReal` (planned names) | `matrixRank`, `matrixRankReal` (`std::rank` clash) |

## Feature-to-test map

Entry: [`16-linear_algebra_tester.py`](<../../96-Local Testing/05-Mathematics/16-linear_algebra_tester.py>), with C++ cases in `16-linear_algebra_tester.cpp`. Every check uses the tester's own oracle code; `validateOracles` checks the oracles first.

Oracles:

- Leibniz (permutation) expansion in `__int128` or the field; cofactor adjugates.
- Exhaustive enumeration over GF(2), GF(3), GF(5), GF(7): row spaces, whole solution sets, spans.
- The tester's own Gauss elimination and inverse; exact residuals `A x = b`, `A X = X A = I`.
- P L U products with a known determinant; repeated products and the block matrix `[[A, I], [0, I]]`.
- Dense matrices with leading-minor checks for the structured solves; residual tolerances for doubles.

| Operation | Tests |
|---|---|
| identityMatrix, matrixProduct | `productSuite`: mint, Z/12, `ulng`, double, n x 0 shapes |
| rref, matrixRank, determinant, nullspace, solveLinear, solveLinearMultiRhs, inverse, adjugate | `exactSuite` over GF(2), GF(3), GF(5), GF(7), `DynModInt` mod 11, 998244353, 1e9+7: RREF shape, enumerated row spaces and solution sets, own ranks and exact residuals for large p, Leibniz determinants, two-sided inverses, cofactor adjugates at rank n, n - 1 and `<= n - 2`, n = 0 and 1 |
| determinantMod, inverseMod | `modSuite`: 27 moduli in `[1, 2^63)` (1, 6, 12, powers of 2, composites above 2^32, 2^63 - 1), entries including `INT64_MIN`/`MAX`, unit-free columns (Euclid path); Leibniz for n <= 6, P L U for n <= 24; success iff `gcd(det, m) = 1`; divisor reduction and prime-field cross-checks |
| determinantInteger | `integerSuite` (`int`, `lng`, `lll`), `integerKnown`: Leibniz in `lll`, P L U products, zero pivots needing swaps |
| matrixPower, matrixPowerSum | `powerSuite` (mint, GF(2), Z/12, `ulng`): repeated products for e <= 20, own block-matrix power up to e = 2^64 - 1 |
| tridiagonalSolve, cyclicTridiagonalSolve, bandedSolve | `bandSuite` (GF(2..7), mint, double; n <= 30; p, q <= 3): success iff every required leading minor is nonzero, exact `A x = d`; diagonally dominant residuals over doubles; cyclic n = 1, 2 folds |
| solveLinearReal, matrixRankReal, determinantReal, luDecompose, luSolve | `realSuite`: Leibniz with a stated relative tolerance, rank-r products, residuals and kernels, LU reconstruction with `perm`, singular inputs and `eps` |
| lowRankUpdateInverse | `lowRankSuite` (998244353, GF(2), GF(3), GF(5)): own inverse of `A + U V`, n <= 8, k <= 4 including k = 0 and n = 0, singular updates leave `ainv` untouched |
| FieldBasis: insert, represent, rollback | `basisSuite`: random interleavings against a model list, brute spans or own ranks, coefficients reconstruct v |
| freivaldsProductCheck | `freivaldsSuite`: exact products always pass, single-entry perturbations caught (GF(2) with 40 rounds), empty shapes, seed reproducibility |
| schwartzZippelIdentityCheck | `szSuite`: determinant identities (`det M - det M^T`), expanded binomials, nonzero polynomials rejected, recorded evaluation points per seed |
| determinantInteger limits | `integerBoundary`: `lng` n = 2 with entries up to 2^31 - 1, n = 3 up to 2^15; `lll` n = 2 up to 2^62 - 1, n = 3 up to 2^20 (products of minors at the edge of T), Leibniz in `lll` |
| aliasing | `aliasSuite`: `solveLinearMultiRhs(a, b, b)`, `tridiagonalSolve` and `cyclicTridiagonalSolve` with `a = b = c = d`, `lowRankUpdateInverse(a, a, a)` against unaliased calls |
| assertion probes | 35 checked-build probes (non-square, ragged rows in every elimination and product, mismatched shapes, `mod < 1`, `rollback` above and below range, low-rank shapes) |
| builds | header alone; two translation units calling every operation, with and without `-DNDEBUG`, `-Wall -Wextra -Wconversion -Werror` |

## Commands and results

2026-10-28, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`). Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan+UBSan.

```bash
python3 '96-Local Testing/05-Mathematics/16-linear_algebra_tester.py' --mode quick                                # PASS (pre-flight of each run below); MEMORY peak=966MB
python3 '96-Local Testing/05-Mathematics/16-linear_algebra_tester.py' --mode full --seed 20261028                 # PASS, 3 configurations, 35 probes, builds, 148,112 checks each; MEMORY peak=1113MB
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/16-linear_algebra_tester.py' --mode full --seed 20261028      # PASS, 3 configurations, 35 probes, builds, 148,112 checks each; MEMORY peak=1133MB
python3 '96-Local Testing/05-Mathematics/16-linear_algebra_tester.py' --mode stress --seed 7                      # PASS, 584,947 checks each; MEMORY peak=1115MB
python3 '96-Local Testing/02-integration.py'                                                                      # PASS: 178 standalone/aggregate headers, multi-TU, workspace; MEMORY peak=1392MB
```

- Mode coverage: quick runs a tenth of the random cases, full all of them, stress four times as many with fresh seeds; every mode runs every suite, the probes and the build checks.
- Test programs take about 1 s optimized and 8.5 s checked in full mode; the reported peaks are the compiler.

## Benchmarks

Contest profile: no benchmark is required (no Barrett/Montgomery). Ad-hoc probe (`_00_memory_cap.py`, 2026-10-28, GCC 16.2 `-O2 -DNDEBUG`, uniform random entries, seed 1, median of 5), recorded to show Library Checker sizes are comfortable:

| Workload | ms |
|---|---|
| `determinantMod` n = 500, mod 1e9 / 998244353 / 2^62 + 135 | 101 / 101 / 117 |
| `determinantMod` n = 500, mod 12, even entries (Euclid path every column) | 312 |
| `determinant` n = 500, 998244353 | 48 |
| `inverse` n = 500, 998244353 | 219 |
| `inverseMod` n = 500, mod 1e9 | 448 |
| `solveLinear` n = 500, 998244353 | 66 |
| `adjugate` n = 500, 998244353 | 195 |
| `matrixProduct` n = 1024, 998244353 | 1147 |
| `matrixPower` / `matrixPowerSum` n = 200, e = 1e18 | 692 / 1176 |
| `luDecompose` / `solveLinearReal` n = 500, double | 33 / 27 |

Peak memory 22 MB. Core `10-matrix.hpp` is the fast path for large products.

## Sources

[00-sources.md](00-sources.md), P058 sweep: Library Checker `linear_algebra` problems, KACTL numerical (`SolveLinear`, `Tridiagonal`, `IntDeterminant`, `MatrixInverse`), maspypy `linalg/solve_linear_lower`, `linalg/basis`, OI Wiki linear algebra (basis, matrix), cp-algorithms (Gauss, determinant, Crout LU). Bareiss: E. H. Bareiss, "Sylvester's identity and multistep integer-preserving Gaussian elimination", Math. Comp. 22 (1968). The code was written independently.

## Limits and handoffs

- Hessenberg reduction, characteristic and minimal polynomials, black-box methods and `det(A + x B)` are row 44 and Core Matrix; permanents, Pfaffians, hafnians row 48; Hermite/Smith forms, integer kernels and `modularSolve` row 49; GF(2) Core `12-bitmatrix.hpp`.
- `inverse` and `inverseMod` eliminate on the full `[A | I]` width, about 1.5 n^3 multiply-adds instead of the n^3 of an in-place inversion.
- `tridiagonalSolve`, `cyclicTridiagonalSolve` and `bandedSolve` do not pivot; a nonsingular matrix with a zero leading minor returns `false`, and the caller uses `solveLinear` instead.
- Rejected candidates are in [00-notes.md](00-notes.md), under P058 research decisions.

## History

- 2026-10-28: P058 initial implementation and verification; the tester found 2 header defects (`-DNDEBUG` unused variables, `lowRankUpdateInverse` with n = 0 < k) and `@reviewer` 8 findings (unbounded `rollback`, unchecked ragged rows, output terms in complexity lines, `FieldBasis` floating guard, aliasing, `determinantInteger` limit tests, 3 keeps recorded), all fixed; quick, full and stress passed on g++, full on g++-14, integration passed.
