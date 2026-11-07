# 21-integer_roots.hpp — evidence

`21-integer_roots.hpp` is the canonical owner of exact integer roots and perfect-power tests on 64- and 128-bit integers (`00-notes.md`, Domains). Every function is a stateless `inline` free function. Results are exact on the whole stated domain: a binary64 estimate followed by an exact integer correction, never a floating-point answer.

## Contracts

### integer_root_detail::powCap

`powCap(r, k, n)` returns `r^k` when `r^k <= n`, otherwise `n + 1`, as `ulll`. The running product stays at most `n < 2^64` before each multiplication by `r < 2^64`, so no product reaches `2^128`. It makes at most `k` multiplications and stops at the first one that exceeds `n`, so for `r >= 2` it runs at most 64 times whatever `k` is. Callers pass `k <= 64`.

### isqrt, isqrt128, isqrtRemainder

- `isqrt(n)` returns `floor(sqrt(n))` for every `ulng`. It starts from `ulng(sqrt(double(n)))` clamped to `2^32 - 1`. `double(n)` has relative error at most `2^-53` and `sqrt` is correctly rounded, so the estimate is within 1 of the true floor. One comparison in each direction corrects it, and the clamp keeps `(r + 1)^2` from overflowing.
- `isqrt128(n)` returns `floor(sqrt(n))` for every `ulll` (the result is below `2^64`). Values below `2^64` go to `isqrt`. Otherwise the binary64 estimate is off by at most about `2^11`. One integer Newton step `(r + n / r) / 2` never falls below the floor (AM–GM for integer Newton), and it lands within `(r - s)^2 / (2 r) + 1 <= 2` of it. The final decrement loop therefore runs at most twice, and the cap at `2^64 - 1` keeps `r * r` in range. The cost is one 128-by-64 division.
- `isqrtRemainder(n)` returns `{s, n - s^2}` with `s = isqrt(n)` (GMP `mpz_sqrtrem`). It is kept beside `isqrt` as a derived formula (`duplicates.md`, Kept by the sweep).

### ikthRoot, ceilKthRoot, kthRootRound

All three take any `ulng n` and `k >= 1` (asserted). When `NDEBUG` removes the assertion, `k <= 0` stays bounded with a meaningless result: `ikthRoot` and `kthRootRound` return `n`, `ceilKthRoot` returns `n + 1`, except 1 for `n = 1` and 0 (wrapped) for `n = 2^64 - 1`.

- `ikthRoot(n, k)` returns `floor(n^(1/k))`, the Library Checker `kth_root_integer` operation (`0 <= A < 2^64`, `1 <= K <= 64`), extended to every larger `k`. `k = 1` returns `n`, and `k = 2` is `isqrt`. For `k >= 64` the result is 1 for `n >= 1` and 0 for `n = 0`, because `2^k > n`. Otherwise it starts from `pow(double(n), 1.0 / k)`, whose error is far below 1 when the root is below `2^22`, and corrects the estimate with exact `powCap` comparisons, normally in zero or one step. It returns the floor only, as Library Checker and the catalog libraries do (user decision 2026-11-03). The remainder of a k-th root is `n - r^k`.
- `ceilKthRoot(n, k)` returns `ceil(n^(1/k))`: the floor, plus one unless `r^k = n`. For `k >= 64` the floor is 0 or 1, so `r^min(k, 64) = r^k`.
- `kthRootRound(n, k)` returns the nearest integer to the real `n^(1/k)`. With `r` the floor and `q = 2 r + 1`, the answer is `r + 1` exactly when `n >= (q / 2)^k`, that is `q^k <= n 2^k`. Since `q^k` is odd and `n 2^k` is even for `k >= 1`, equality never happens, so there are no ties. Write `q^j = f 2^j + rem` with `0 <= rem < 2^j`. Then `q^k <= n 2^k` holds iff `floor(q^k / 2^k) < n` (because `q^k` is odd), and the loop computes `f = floor(q^j / 2^j)` exactly. From `q^(j+1) = q f 2^j + q rem`, with `q rem = a 2^j + b`, `b < 2^j`, it sets `f' = floor((q f + a) / 2)` and `rem' = ((q f + a) mod 2) 2^j + b`. Since `q / 2 >= 3 / 2`, `f` never decreases, so the loop stops as soon as `f >= n`. That happens within 110 steps for every `n < 2^64` (`1.5^110 > 2^64`), so `k = 10^9` costs as much as `k = 110`. Bounds: `q f < 2^34 2^64`. `q rem < 2^128`: for `q = 3`, `j <= 110` gives `rem < 2^110`; for `q >= 5` the loop ends by `j = 49`, where `q 2^j <= 2^(64/k + k + 2)`.

### icbrt

`icbrt(n)` returns `floor(cbrt(n))` for every `lng`, rounding toward minus infinity: `icbrt(-9) = -3`, `icbrt(-8) = -2`, `icbrt(INT64_MIN) = -2^21`. The magnitude `|n| <= 2^63` is taken in `ulng`. A negative non-cube rounds down one more than `-ikthRoot(|n|, 3)`.

### isPerfectSquare

`isPerfectSquare(n)` is true iff `n = s^2` for an integer `s`; 0 and 1 count, as in GMP `mpz_perfect_square_p`. A 64-bit table of the 12 squares modulo 64 rejects 81% of inputs before `isqrt`. `isSquare(x)` for a signed `x` is `x >= 0 && isPerfectSquare(x)`.

### isPerfectPower

`isPerfectPower(n)` returns `{b, e}` with `b^e = n` and `e` maximal, which makes `b` minimal and not itself a perfect power. `n` is a perfect power iff `e >= 2`. For `n <= 1` every exponent works, so the result is `{n, 1}` (user decision 2026-11-03); unlike GMP's `mpz_perfect_power_p`, 0 and 1 then read as "not a power", and the caller tests them directly. For each prime `p <= 61` below `bit_width(n)`, it replaces `n` by its p-th root as long as the root is exact, multiplying `e` by `p`. If `n = c^E` with `c` not a perfect power, then for a prime `p | E` the p-th root of `c^E` is `c^(E / p)`, and for a prime `p` not dividing `E` no exact p-th root exists. So the loop strips exactly the prime factors of `E`, with multiplicity. The cost is at most 18 primes times O(log(n)) root work, plus one extra root per prime factor of `E`.

### Legacy mapping

| Archived | Replacement |
|---|---|
| `isSquare(lng x)` (`OLD/5-Mathematics/13-numericalmethods.hpp`, `OLD/Team Notebook/src/math/issquare.hpp`) | `x >= 0 && isPerfectSquare(x)`. The archived 2-adic shortcut (an even number of trailing zeros and an odd part that is 1 mod 8) is subsumed by the mod-64 table plus an exact `isqrt` check. The archived `std::round(std::sqrtl(x))` is replaced by exact integer correction. |

## Feature-to-test map

Entry: [`21-integer_roots_tester.py`](<../../96-Local Testing/05-Mathematics/21-integer_roots_tester.py>), which drives the C++ suite `21-integer_roots_tester.cpp` in the shared runner's optimized `-O2 -DNDEBUG` and checked `-O0 -g -D_GLIBCXX_DEBUG` builds, plus an ASan/UBSan build outside quick mode. A Python exact-integer oracle (`math.isqrt`, integer binary search, the exact `2^k n >= (2 r + 1)^k` rounding test, brute-force power decomposition) is first validated against linear scans, a table of perfect powers and 60-digit `Decimal` roots.

| Feature | Independent verification |
|---|---|
| `ikthRoot`, `ceilKthRoot`, `kthRootRound` | Explicit values: `2^64 - 1` at k = 1–5, 40, 63, 64, 109, 110, 10^9 and `INT_MAX`; `2^63` and `2^63 - 1` at k = 63. Exhaustive small n against roots built by incrementing, with rounding thresholds from exact big-integer arithmetic. `r^k - 1`, `r^k`, `r^k + 1` for every `r >= 2` with `r^k < 2^64` (k = 3..63; k = 2 sampled). Rounding thresholds `floor((r + 1/2)^k)` and the next value, up to k = 200. Seeded random values checked by `r^k <= n < (r + 1)^k` and the big-integer rounding test. The Python oracle. |
| `isqrt`, `isqrtRemainder`, `isPerfectSquare` | Exhaustive small n, square neighbours, random values checked by `s^2 <= n < (s + 1)^2`, the Python oracle. |
| `isqrt128` | Explicit extremes; `s^2 - 1`, `s^2`, `s^2 + 1` and `s^2 + 2 s` around `2^32` and near `2^64`; random 128-bit values checked by the square inequalities. |
| `icbrt` | Exhaustive `abs(n) <= 2^22`; `±(r^3 + {-1, 0, 1})` for every r; `INT64_MIN`, -9, -8; signed random values. |
| `isPerfectPower` | Exhaustive small n against a table listing every `b^e`. Every `b^e` checked to return `c^(f e)` where `b = c^f`. Neighbours, and random values against a separate brute-force exponent search. |
| Preconditions | 6 checked-build assertion probes: k = 0 and k = -1 for each of `ikthRoot`, `ceilKthRoot` and `kthRootRound`. |

Coverage per mode (the runner has no `--rounds`; stress uses fixed larger sizes and the seed):

| | quick | full | stress |
|---|---|---|---|
| exhaustive k-th roots, k <= 20 | n < 2^12 | n < 2^16 | n < 2^18 |
| exhaustive sqrt, cbrt, perfect powers | n < 2^16 | n < 2^22 | n < 2^23 |
| random rounds | 5,000 | 200,000 | 600,000 |
| Python oracle rows | 300 | 3,000 | 20,000 |
| C++ checks per build | 1.5M | 114M | 240M |

## Commands and results

P059 run, 2026-11-03. GCC 16.0.1 (`g++`) and GCC 14.3.0 (`CXX=g++-14`), GNU++20, CPython 3.14.4, Linux x86-64 (WSL2), AMD Ryzen AI 9 HX 370. Every suite runs the shared runner's optimized `-O2 -DNDEBUG` and checked `-O0 -g -D_GLIBCXX_DEBUG` builds, plus ASan/UBSan outside quick mode; counts are per configuration and equal on both compilers.

| Command | Result |
|---|---|
| `21-integer_roots_tester.py --mode quick` | PASS, 2 configurations, `MEMORY peak=402MB` |
| `21-integer_roots_tester.py --mode full --seed 1` | PASS, 3 configurations, 113,762,108 C++ checks, 3,000 Python oracle rows, 6 assertion probes, `MEMORY peak=816MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=817MB` |
| `21-integer_roots_tester.py --mode stress --seed 2` | PASS, 240,355,320 C++ checks, 20,000 Python rows, `MEMORY peak=893MB` |
| `02-integration.py` (header alone, `99-all`, two translation units) | PASS, `MEMORY peak=1704MB cap=8192MB` |
| `03-consistency.py --braces` on the header and tester | PASS |

Mutation check (scratch copies): rounding `<` changed to `<=`, the dropped negative correction in `icbrt` and a rounding loop capped at 100 steps are each caught. Turning `while` into `if` in `isqrt128` and moving the `k >= 64` cutoff to 65 keep identical results, as the bounds in Contracts predict.

## Benchmarks

No timing benchmark: every operation is O(1), or bounded by a small constant times `min(k, 128)`. Library Checker `kth_root_integer` (`T <= 5 * 10^5`) needs about one `pow` and one or two `powCap` per query. No Barrett/Montgomery or ISA-specific code is used.

## Sources

| Reference | Inspected claim and use |
|---|---|
| [Library Checker, kth_root_integer](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/number_theory/kth_root_integer/task.md) | Domain of `ikthRoot`. |
| [GMP manual, Integer Roots](https://gmplib.org/manual/Integer-Roots) | `sqrtrem`, `rootrem`, perfect-square and perfect-power semantics, including 0 and 1. |
| [suisen, kth_root_round.hpp](https://suisen-cp.github.io/cp-library-cpp/library/number/kth_root_round.hpp) | Floor, ceil and round variants. The exact rounding test here is written independently, with the odd-power tie argument. |
| [Nyaan, kth-root-integral.hpp](https://nyaannyaan.github.io/library/math/kth-root-integral.hpp), [maspypy, integer_kth_root.hpp](https://maspypy.github.io/library/nt/integer_kth_root.hpp) | A floating estimate followed by exact correction. No code copied. |
| [cp-algorithms, Newton's method](https://cp-algorithms.com/num_methods/roots_newton.html) | The integer Newton square root never undershoots the floor. |

## Limits and handoffs

- The `isqrt` helpers inside `20-continued_fraction.hpp`, `34-quadratic_congruence.hpp`, Geometry `19-lattice_geometry.hpp` and `05-closestpair.hpp`, and Data Structures `14-segtreebeats.hpp`, plus the floor k-th root `primality_detail::kthRoot` in `07-primality_factorization.hpp`, are detail copies owned by their packages. Migrating them to this header is a follow-up for those packages.
- Roots of `InfInt` values belong to Core.

## Aliases

Search aliases (P263, grab --find): 开方, 整数平方根, 开平方, 开根号, 开立方, 完全平方数, 完全幂.

## History

- 2026-11-03: P059 implementation; review found no correctness defect (one handoff added); full suite passed on g++ and g++-14.
- 2026-11-03: /finish audit, no code defect; NDEBUG note corrected, 2 user decisions recorded; quick suite passed.
- 2026-11-03: maintainer review in `/merge-pr 3`, no code defect; `isqrt` beside `isqrt128` kept as a larger domain (user decision, `duplicates.md`), and the user confirmed the decisions recorded as theirs.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
