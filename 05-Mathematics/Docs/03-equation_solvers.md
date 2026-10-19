# 03-equation_solvers.hpp — evidence

`03-equation_solvers.hpp` covers two-variable linear Diophantine equations and one-variable linear congruences. It builds on `extendedGcd`, `floorDiv`, `ceilDiv` and `modNorm` from `01-mod_arithmetic.hpp`. Every operation is deterministic, logarithmic in the coefficient sizes, and returns constant-size results; there is no hidden enumeration, allocation, cache or preprocessing.

## Contracts

### DioSolution, solveDioEq

`solveDioEq(a, b, c)` solves `a * x + b * y = c` for every `lng` `a`, `b`, `c`. The returned `DioSolution` has:

- `dimension`: `-1` when there is no solution, `1` for the line `(x, y) + t * (dx, dy)` over every integer `t`, and `2` for all of `Z^2` when `a = b = c = 0`.
- `x`, `y`, `dx`, `dy` and `g`, all signed 128-bit.
- `g = gcd(|a|, |b|)` on every result, including no-solution results. It is 0 only when `a = b = 0`.
- For `b != 0`, the canonical `0 <= x < |b / g|`. For `b = 0`, `y = 0`.

Evaluating an arbitrary parameter `t` must fit the caller's chosen arithmetic. Every bounded parameter returned by `solveDioBox` evaluates safely in 128 bits.

Correctness: for nonzero `(a, b)`, Bézout gives `a * u + b * v = g > 0`, so a solution exists exactly when `g` divides `c`, and scaling by `c / g` gives a particular solution. When `b != 0`, reducing `x` modulo `|b / g|` and recomputing `y = (c - a * x) / b` keeps it integral. Two solutions differ by a vector with `(a / g) * dx = -(b / g) * dy`; coprimality makes it a multiple of `(b / g, -a / g)`, so the line holds all solutions and only solutions. With one zero coefficient the same primitive vector covers the free coordinate; with both zero the equation is inconsistent or no constraint. Widths: the extended-gcd coefficients and `c / g` are at most `2^63` in magnitude, so products are at most `2^126`; the canonical `x` lies in `[0, 2^63)`, `c - a * x` fits 128 bits, and `|y| <= 2^63 / |b| + |a| / g <= 2^64` (the `b = 0` case also fits).

### DioBox, solveDioBox

`solveDioBox(a, b, c, xl, xr, yl, yr)` counts and parameterizes the solutions in the half-open box `[xl, xr) * [yl, yr)`. The endpoints are `lng` with `xl <= xr` and `yl <= yr` (asserted). The returned `DioBox` has:

- `solution`: the unrestricted `solveDioEq` result.
- `count`: an unsigned 128-bit point count. A zero count means there is no point in the box, even when an unrestricted solution exists. Empty boxes return count 0.
- `[l, r)`: the integer parameter range for `dimension == 1`. For `dimension == 2`, every point of the box is a solution and `l`, `r` are unused.

Each coordinate width is at most `2^64 - 1`, so the count fits even for `0 * x + 0 * y = 0` on the largest box, `(2^64 - 1)^2` points. The exclusive endpoints are `lng`, so a box cannot contain `INT64_MAX`; the unbounded result still represents such solutions. When converting a legacy inclusive range, check the upper endpoint before adding one. Enumerating a parameter range costs `O(count)` extra.

To optimize a linear objective `p * x + q * y` over a nonempty line intersection, note that its slope in `t` is `p * dx + q * dy`. Pick `l` for a nonnegative slope and `r - 1` for a negative one. On the whole plane, pick each box endpoint by the sign of its coefficient.

Correctness: each nonconstant coordinate admits a closed interval of integer parameters, given exactly by ceiling and floor division with endpoints swapped for a negative step; making the upper end exclusive and intersecting the two intervals gives the range. A constant coordinate accepts or rejects the whole line. The plane case counts the Cartesian product directly. Endpoint differences, quotients and range widths fit signed 128 bits, and the unsigned box product fits because each width is at most `2^64 - 1`.

### solveModEq

`solveModEq(a, b, m0)` solves `a * x = b (mod m0)` for any `lng` `a` and `b` and positive `m0` (asserted). It returns `{x, m}` with `0 <= x < m` and all solutions `x + k * m`, where `m = m0 / gcd(a, m0)` is the least positive period. It returns `{-1, -1}` exactly when there is no solution. `m0 == 1`, or `0 * x = 0`, gives `{0, 1}`. In `[0, m0)` there are `m0 / m` solutions, so enumerating them costs that much extra.

Correctness: the congruence is the integer equation `a * x + m0 * y = b`. Dividing by the gcd is necessary and sufficient, and the Bézout coefficient of `a` is the inverse of `a / g` modulo `m0 / g`; reducing `u * (b / g)` gives the least nonnegative representative. A positive modulus guarantees a positive period, so the sentinel cannot collide with a valid answer.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `solveDioEq(a, b, c, x, y, g)` returning `bool` | `auto s = solveDioEq(a, b, c)`: `s.dimension >= 0` for success, `lll` fields `s.x`, `s.y`, `s.g` (`g = 2^63` for `(MIN, 0, 0)`, `y = 2^63` for `(0, -1, MIN)`) |

## Feature-to-test map

Entry: [`03-equation_solvers_tester.py`](<../../96-Local Testing/05-Mathematics/03-equation_solvers_tester.py>).

| Feature | Independent evidence |
|---|---|
| `solveDioEq`, `DioSolution`: solvability, canonical point, primitive direction, `g` | Exhaustive small signed coefficients with point substitution and unsigned `std::gcd`. Every boundary coefficient/RHS triple and seeded full-width cases, with explicit `g = 2^63` for `(MIN, 0, 0)` and `y = 2^63` for `(0, -1, MIN)`. The Python built-in modular inverse with arbitrary-precision substitution. `s.g` is checked before the no-solution branch, and the Python oracle expects `g` on no-solution results. |
| Complete parameterization | Every solution in a brute-force `[-8, 9)^2` grid maps to an integral parameter, and every bounded parameter maps back to a solution. |
| `solveDioBox`, `DioBox`: half-open boxes, signs and zeros | Every box with endpoints in `[-2, 3]` against direct enumeration. Independently computed Python parameter bounds over arbitrary signed 64-bit ranges. |
| Wide arithmetic and count | Signed minima and maxima with their neighbours, `-1`, zero, impossible-gcd cases, the full-length diagonal `2^64 - 1`, and the exact count `(2^64 - 1)^2`. |
| `solveModEq` | Every residue for signed `a`, `b` through magnitude 40 and moduli through 40: the sentinel, the least representative, the least period and every solution residue. A Python arbitrary-precision inverse oracle. |
| Preconditions | Four checked-build death probes: zero or negative modulus and reversed intervals. |

## Commands and results

P012 prune run, 2026-10-12, recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Every suite runs the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations; counts are per configuration and equal on both compilers.

| Command | Result |
|---|---|
| `03-equation_solvers_tester.py --mode full --seed 1` | PASS, 5,019,506 C++ checks and 5,729 Python cases, 4 assertion probes, `MEMORY peak=411MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=412MB` |
| Other P012 suites, `02-integration.py` and `03-consistency.py` | PASS; details and commands in [01-mod_arithmetic.md](01-mod_arithmetic.md) |

## Benchmarks

No specialized arithmetic or threshold exists, so no benchmark is needed.

## Sources

Inspected on 2026-09-27; implemented independently from the proofs, with no external code copied.

- [cp-algorithms, Linear Diophantine Equation](https://cp-algorithms.com/algebra/linear-diophantine-equation.html): gcd solvability, the solution lattice, interval intersection and linear-objective endpoints. Its bounded code excludes zero coefficients and uses closed intervals; this header handles zeros and uses half-open ranges.
- [KACTL, `euclid.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/euclid.h): independent comparison of the Bézout and inverse contract.
- [OI Wiki, 线性同余方程](https://oi-wiki.org/math/number-theory/linear-equation/): the least nonnegative solution and the `n / gcd(a, n)` progression. It follows the same e-maxx lineage, so it serves as exposition only.

## Limits and handoffs

- Integer systems and lattice normal forms belong to `49-integer_linear_algebra.hpp`.
- The 2026-10-07 completeness sweep left out two candidates, recorded in [00-notes.md](00-notes.md): `firstModInRange` (maspypy `first_mod_range_of_linear`) is handed to `18-floorsum.hpp`, and a named linear-objective helper is unnecessary because the endpoint rule above is O(1).
- The previously active header exposed only a particular solution and one modular progression. `solveDioEq` and `solveModEq` keep their names, with the signed-minimum and normalization overflows removed; the reference-output overload maps as in Legacy mapping. `OLD` material is unchanged.

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed on all configurations.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, 2 findings fixed; after `@reviewer`, full, stress (seed 1) and integration passed.
- 2026-10-12: prune, removed the reference-output `solveDioEq` overload (its `g` and `y` overflow inputs now checked on the result overload); full suite passed on g++ and g++-14.
