# 03-equation_solvers.hpp — evidence

`03-equation_solvers.hpp` covers two-variable linear Diophantine equations and one-variable linear congruences. It builds on `extendedGcd`, `floorDiv`, `ceilDiv` and `modNorm` from `01-mod_arithmetic.hpp`. Every operation is deterministic, logarithmic in the coefficient sizes, and returns constant-size results; there is no hidden enumeration, allocation, cache or preprocessing.

The P012 re-audit (2026-10-07) made three changes:

- Finding 1: `DioSolution.g` now holds `gcd(|a|, |b|)` on every result, including `dimension == -1`. It used to be 0 there, which contradicted the contract.
- Finding 8: the closing brace of the `restrict` lambda was a style-only fix.
- The in-code contracts moved into this document.

## Contracts

### DioSolution, solveDioEq

`solveDioEq(a, b, c)` solves `a * x + b * y = c` for every `lng` `a`, `b`, `c`. The returned `DioSolution` has:

- `dimension`: `-1` when there is no solution, `1` for the line `(x, y) + t * (dx, dy)` over every integer `t`, and `2` for all of `Z^2` when `a = b = c = 0`.
- `x`, `y`, `dx`, `dy` and `g`, all signed 128-bit.
- `g = gcd(|a|, |b|)` on every result, including no-solution results. It is 0 only when `a = b = 0`.
- For `b != 0`, the canonical `0 <= x < |b / g|`. For `b = 0`, `y = 0`.

Evaluating an arbitrary parameter `t` must fit the caller's chosen arithmetic. Every bounded parameter returned by `solveDioBox` evaluates safely in 128 bits.

The legacy overload `solveDioEq(a, b, c, x, y, g)` keeps its name and signature. It requires pairwise-distinct output references (asserted). On success, the canonical `x`, `y` and `g` must fit `lng` (asserted); use the result overload otherwise. It returns false for a legitimate no-solution case and leaves the outputs unchanged.

### DioBox, solveDioBox

`solveDioBox(a, b, c, xl, xr, yl, yr)` counts and parameterizes the solutions in the half-open box `[xl, xr) * [yl, yr)`. The endpoints are `lng` with `xl <= xr` and `yl <= yr` (asserted). The returned `DioBox` has:

- `solution`: the unrestricted `solveDioEq` result.
- `count`: an unsigned 128-bit point count. A zero count means there is no point in the box, even when an unrestricted solution exists. Empty boxes return count 0.
- `[l, r)`: the integer parameter range for `dimension == 1`. For `dimension == 2`, every point of the box is a solution and `l`, `r` are unused.

Each coordinate width is at most `2^64 - 1`, so the count fits even for `0 * x + 0 * y = 0` on the largest box, `(2^64 - 1)^2` points. The exclusive endpoints are `lng`, so a box cannot contain `INT64_MAX`; the unbounded result still represents such solutions. When converting a legacy inclusive range, check the upper endpoint before adding one. Enumerating a parameter range costs `O(count)` extra.

To optimize a linear objective `p * x + q * y` over a nonempty line intersection, note that its slope in `t` is `p * dx + q * dy`. Pick `l` for a nonnegative slope and `r - 1` for a negative one. On the whole plane, pick each box endpoint by the sign of its coefficient. Integer systems and lattice normal forms belong to `49-integer_linear_algebra.hpp`.

### solveModEq

`solveModEq(a, b, m0)` solves `a * x = b (mod m0)` for any `lng` `a` and `b` and positive `m0` (asserted). It returns `{x, m}` with `0 <= x < m` and all solutions `x + k * m`, where `m = m0 / gcd(a, m0)` is the least positive period. It returns `{-1, -1}` exactly when there is no solution. `m0 == 1`, or `0 * x = 0`, gives `{0, 1}`. In `[0, m0)` there are `m0 / m` solutions, so enumerating them costs that much extra.

## Correctness and width argument

**Existence and parameterization.** For nonzero `(a, b)`, Bézout gives `a * u + b * v = g > 0`, so a solution exists exactly when `g` divides `c`. Multiplying the coefficients by `c / g` gives a particular solution. When `b != 0`, reducing `x` modulo `|b / g|` and recomputing `y = (c - a * x) / b` keeps the solution integral. Two solutions differ by a vector with `(a / g) * dx = -(b / g) * dy`; since `a / g` and `b / g` are coprime, that vector is an integer multiple of `(b / g, -a / g)`. So the line contains all solutions and only solutions. With one zero coefficient, the same primitive vector covers the free coordinate. With both zero, the equation is either inconsistent or no constraint.

**Box.** Each nonconstant coordinate admits a closed interval of integer parameters. Ceiling and floor division give that interval exactly, with the inequality endpoints swapped for a negative step. Making the upper end exclusive and intersecting the two intervals proves `solveDioBox`. A constant coordinate either accepts the whole parameter line or rejects it. The plane case counts the Cartesian product directly.

**Widths.**

- The extended-gcd coefficients and `c / g` have magnitude at most `2^63`, so their products are at most `2^126`.
- The canonical `x` lies in `[0, 2^63)`, and `c - a * x` fits 128 bits.
- `|y| <= 2^63 / |b| + |a| / g <= 2^64`, and the `b = 0` case also fits.
- Endpoint differences, ceiling and floor quotients, and range widths all fit signed 128 bits.
- The unsigned box product fits because each width is at most `2^64 - 1`.

**Congruence.** The congruence is the integer equation `a * x + m0 * y = b`. Dividing by the gcd is necessary and sufficient, and the Bézout coefficient of `a` is the inverse of `a / g` modulo `m0 / g`. Reducing `u * (b / g)` gives the least nonnegative representative. A positive modulus guarantees a positive period, so the sentinel `{-1, -1}` cannot collide with a valid answer.

## Re-audit findings (P012, 2026-10-07)

Confirmed `/reaudit-review` findings for this header:

| # | Finding | Resolution |
|---|---|---|
| 1 | `DioSolution.g` was 0 on no solution | Fixed: `g = gcd(abs(a), abs(b))` on every result; both oracles check it |
| 8 | Lambda closing brace `; };` | Fixed |

## Feature-to-test map

Entry: [`03-equation_solvers_tester.py`](<../../96-Local Testing/05-Mathematics/03-equation_solvers_tester.py>).

| Feature | Independent evidence |
|---|---|
| Solvability, canonical point, primitive direction, `g`, legacy overload | Exhaustive small signed coefficients with point substitution and unsigned `std::gcd`. Every boundary coefficient/RHS triple and seeded full-width cases. The Python built-in modular inverse with arbitrary-precision substitution. `s.g` is checked before the no-solution branch, and the Python oracle expects `g` on no-solution results. |
| Complete parameterization | Every solution in a brute-force `[-8, 9)^2` grid maps to an integral parameter, and every bounded parameter maps back to a solution. |
| Half-open boxes, signs and zeros | Every box with endpoints in `[-2, 3]` against direct enumeration. Independently computed Python parameter bounds over arbitrary signed 64-bit ranges. |
| Wide arithmetic and count | Signed minima and maxima with their neighbours, `-1`, zero, impossible-gcd cases, the full-length diagonal `2^64 - 1`, and the exact count `(2^64 - 1)^2`. |
| `solveModEq` | Every residue for signed `a`, `b` through magnitude 40 and moduli through 40: the sentinel, the least representative, the least period and every solution residue. A Python arbitrary-precision inverse oracle. |
| Preconditions | Nine checked-build death probes: zero or negative modulus, reversed intervals, legacy gcd or point overflow, and the three output aliases. |

Historical: on 2026-09-27, full mode with seed `20260927` passed 5,043,338 C++ checks per configuration and 5,729 Python cases. No specialized arithmetic or threshold exists, so no benchmark is needed.

## Commands and results

Package-wide P012 runs (2026-10-07), recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1, GNU++20, Python 3.14, Linux x86-64 (i9-11900H). Every C++ build uses the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (quick runs the first two). Before any re-audit change, the full suite (`01-run.py --mode full --filter 05-Mathematics/0 --seed 1 --no-integration`) passed all six entries.

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 05-Mathematics/0 --no-integration                     # PASS, 6 suites, 2 configurations
python3 '96-Local Testing/01-run.py' --mode full --filter 05-Mathematics/0 --seed 1 --no-integration              # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/01-run.py' --mode stress --filter 05-Mathematics/0 --seed 1 --rounds 1 --no-integration # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/02-integration.py'                                                                     # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                     # no errors
```

After the independent `@reviewer` pass, the full suite was rerun on all six headers, stress on combinatorics, and integration; all passed with the counts below.

| Mode | Count |
|---|---|
| Full (seed 1), per configuration | 5,050,043 C++ checks, 5,729 Python cases |
| Stress (seed 1) | 7,011,138 C++ checks, 50,729 Python cases |
| Assertion probes | 9 |

## Sources

Inspected on 2026-09-27; implemented independently from the proofs, with no external code copied.

- [cp-algorithms, Linear Diophantine Equation](https://cp-algorithms.com/algebra/linear-diophantine-equation.html): gcd solvability, the solution lattice, interval intersection and linear-objective endpoints. Its bounded code excludes zero coefficients and uses closed intervals; this header handles zeros and uses half-open ranges.
- [KACTL, `euclid.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/euclid.h): independent comparison of the Bézout and inverse contract.
- [OI Wiki, 线性同余方程](https://oi-wiki.org/math/number-theory/linear-equation/): the least nonnegative solution and the `n / gcd(a, n)` progression. It follows the same e-maxx lineage, so it serves as exposition only.

The 2026-10-07 completeness sweep left out two candidates. `firstModInRange` (maspypy `first_mod_range_of_linear`) is handed to `18-floorsum.hpp`. A named linear-objective helper is not needed: the endpoint rule above is O(1). Both are recorded in [00-notes.md](00-notes.md). The previously active header exposed only a particular solution and one modular progression. Those names remain, with the signed-minimum and normalization overflows removed. `OLD` material is unchanged.
