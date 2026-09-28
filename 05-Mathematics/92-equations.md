# Linear equations — MA01 verification

`03-equation_solvers.hpp` implements the complete two-variable linear
Diophantine/one-variable linear-congruence inventory. The implementation uses
`extendedGcd`, `floorDiv`, `ceilDiv` and `modNorm` from `01-mod_arithmetic.hpp`.
It is deterministic, logarithmic in coefficient magnitude, and stores only
constant-size results. No enumeration, allocation, cache or preprocessing is
hidden in an API call.

## API and domains

| API | Contract |
|---|---|
| `solveDioEq(a,b,c)` | Every signed 64-bit `a,b,c`; solves `a*x+b*y=c`. `DioSolution.dimension` is `-1` for no solution, `1` for the line `(x,y)+t*(dx,dy)`, every integer `t`, and `2` for every integer pair when `a=b=c=0`. Fields use signed 128-bit integers. `g=gcd(abs(a),abs(b))`. For `b!=0`, `0<=x<abs(b/g)`; for `b=0`, `y=0`. |
| `solveDioEq(a,b,c,x,y,g)` | Preserved legacy name/signature. Outputs are pairwise distinct references. On success the canonical `x,y,g` must fit signed 64 bits; use the result overload otherwise. Returns false for a legitimate no-solution case and then leaves outputs unchanged. Zero coefficients and signs are supported. |
| `solveDioBox(a,b,c,xl,xr,yl,yr)` | Counts and parameterizes the points in `[xl,xr)*[yl,yr)`. Endpoints are signed 64-bit integers and obey `xl<=xr`, `yl<=yr`. Empty boxes return count zero. `DioBox.solution` is the unrestricted result; for a nonempty line intersection the integer parameter range is `[l,r)`. For the whole plane every point of the supplied rectangle is valid and `l,r` are unused. |
| `DioBox.count` | Unsigned 128-bit count. A coordinate width can reach `2^64-1`, so the largest supported rectangle contains `(2^64-1)^2` points. A zero count, even with an unrestricted solution, means there is no point in the box. |
| `solveModEq(a,b,m0)` | Every signed 64-bit `a,b`, positive signed 64-bit `m0`. Returns `{x,m}` with `0<=x<m` and all solutions `x+k*m`, `k` any integer; `m=m0/gcd(a,m0)` is the least positive period. Returns `{-1,-1}` exactly when inconsistent. Modulus one and a zero coefficient have their usual exact semantics; `0*x=0` returns `{0,1}`. |

The bounded API's exclusive endpoints are `lng`: it cannot include
`LLONG_MAX`, whose exclusive upper endpoint is not representable. The
unbounded result still represents such solutions. A caller enumerating a
parameter range pays `O(count)` output work; evaluating arbitrary unbounded
parameters requires the caller's chosen arithmetic to fit. Every returned
bounded parameter evaluates safely in signed 128-bit arithmetic. Converting
legacy inclusive ranges requires checking the upper endpoint before adding
one. There was no prior bounded API to migrate.

For a linear objective `p*x+q*y` over a nonempty line intersection, its slope
in `t` is `p*dx+q*dy`: select `l` for a nonnegative slope and `r-1` for a negative
slope. On the whole plane choose each rectangle endpoint according to the
corresponding objective coefficient. These are constant-time consequences of
the compact representation, with caller-chosen objective arithmetic widths.
General integer systems and lattice normal forms belong to the separate
`49-integer_linear_algebra.hpp` family.

## Correctness and width argument

For nonzero `(a,b)`, Bézout gives `a*u+b*v=g>0`; therefore a solution exists
exactly when `g` divides `c`. Multiplying its coefficients by `c/g` constructs
a particular solution. When `b!=0`, reducing its `x` modulo `abs(b/g)` and
recomputing `y=(c-a*x)/b` preserves integrality and the equation. The difference
of two solutions satisfies `(a/g)*dx=-(b/g)*dy`; coprimality implies that it is
an integer multiple of `(b/g,-a/g)`. This proves the parameterization contains
all and only the solutions. With one zero coefficient the same primitive
vector covers the free coordinate. With both zero coefficients the equation
is either inconsistent or imposes no constraint.

Each nonconstant coordinate gives a closed interval of admissible integer
parameters after solving its lower and upper inequalities. Mathematical
ceiling/floor division, with inequality endpoints exchanged for a negative
step, gives that interval exactly; converting its upper endpoint to an
exclusive bound and intersecting both intervals proves `solveDioBox`.
A constant coordinate either accepts the entire parameter line or rejects it.
The plane case counts the Cartesian product directly.

The extended-gcd coefficients and `c/g` have magnitude at most `2^63`, so the
initial products have magnitude at most `2^126`. Canonical `x` lies in
`[0,2^63)`, and `c-a*x` fits signed 128 bits. The corresponding `y` has magnitude
at most `2^63/abs(b)+abs(a)/g`, hence at most `2^64`; the `b=0` case also fits.
All endpoint differences, ceiling/floor quotients and parameter-range widths
therefore fit signed 128 bits. The unsigned rectangle product fits because
each signed-endpoint width is at most `2^64-1`. A bounded parameter's coordinate
product equals a signed 64-bit coordinate minus its particular coordinate,
which is also safely representable in signed 128 bits.

The modular equation is the integer equation `a*x+m0*y=b`. Dividing by its gcd
is both necessary and sufficient, and the Bézout coefficient of `a` is the
inverse of `a/g` modulo `m0/g`. Reducing `u*(b/g)` produces the least nonnegative
representative; the same product bound applies. A positive modulus guarantees
a positive period, including modulus one, so the no-solution sentinel cannot
collide with a valid answer.

## Sources and migration

Inspected 2026-09-27; the code was implemented independently from the proofs,
with no external source code copied.

- [cp-algorithms, Linear Diophantine Equation](https://cp-algorithms.com/algebra/linear-diophantine-equation.html), page update 2025-10-29: gcd solvability, all-solution lattice, interval intersection and linear-objective endpoints. Its bounded reference code excludes zero coefficients and uses closed integer intervals; this implementation explicitly handles them and uses half-open ranges. Its short discussion of a zero coefficient is not used as a completeness argument for the free coordinate.
- [KACTL, `euclid.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/euclid.h), source comment dated 2002-09-15: independent Bézout/inverse contract comparison. Its recursive signed-word routine does not establish this header's signed-minimum and wide-output bounds.
- [OI Wiki, 线性同余方程](https://oi-wiki.org/math/number-theory/linear-equation/), page update 2026-09-24: least nonnegative congruence solution and complete `n/gcd(a,n)` progression. That page credits the same e-maxx/cp-algorithms lineage, so it is supplementary exposition, not an independent implementation oracle.

The migrated active header previously exposed only a particular solution and
one modular progression. Those names are retained; the zero-coefficient
signed-minimum division/absolute-value hazards and modular normalization
addition overflow are removed. Full-width results now have a separate overload
instead of silently narrowing. Original `OLD` material remains unchanged.

## Feature-to-test map and results

`96-Local Testing/05-Mathematics/03-equation_solvers_tester.py` is runnable from
any working directory and uses the shared mathematics runner.

| Feature | Independent evidence |
|---|---|
| Solvability, canonical point, primitive direction, legacy overload | Exhaustive signed small coefficients, point substitution, unsigned `std::gcd`, every boundary coefficient/RHS triple, seeded full signed64 cases; Python built-in modular inverse with arbitrary-precision substitution. |
| Complete parameterization | Every satisfying pair in a brute-force `[-8,9)^2` grid maps to an integral parameter; every returned bounded parameter maps back to a valid pair. |
| Half-open boxes, signs and zeros | Every box with endpoints from `[-2,3]`, including empty/reversed-coordinate-sign cases, against direct point enumeration; independently computed Python parameter bounds over arbitrary signed64 endpoint ranges. |
| Wide arithmetic/count | Signed minima/maxima and neighbors, `-1`, zero, impossible gcd cases, maximum diagonal length `2^64-1`, exact unsigned count `(2^64-1)^2`. |
| Congruences | Enumerate every residue for signed `a,b` through magnitude 40 and moduli through 40; verify no-solution sentinel, least representative, least period and every residue; Python arbitrary-integer inverse across 5729 cases. |
| Preconditions | Nine checked-build death probes: zero/negative modulus, reversed x/y intervals, legacy gcd/point overflow and all three output-reference aliases. |

Fresh command, seed `20260927`, GNU C++20, GCC 16.2.1 (20260810):

```text
python3 '96-Local Testing/05-Mathematics/03-equation_solvers_tester.py' --mode full --seed 20260927
```

Passed all three configurations: `-O2 -DNDEBUG`, `-O0 -g -D_GLIBCXX_DEBUG`,
and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` with no recovery,
frame pointers and non-PIE. Each configuration passed 5,043,338 C++ checks,
20,000 deterministic random C++ cases, and 5,729 Python arbitrary-precision
cases. The checked build also passed all nine assertion probes. The sandbox's
LeakSanitizer ptrace restriction required the final full command to run outside
the sandbox; the complete rerun passed. The final log is
`/tmp/p012-equations-final.log`.

Quick/full/stress coverage is explicitly documented by the test entry; stress
is available but was not claimed as run. There is no specialized arithmetic,
threshold, large table or competing implementation needing a performance
benchmark. No online submission was made. Package-level standalone,
aggregate/multiple-translation-unit and consistency verification is recorded
by the P012 integration owner.
