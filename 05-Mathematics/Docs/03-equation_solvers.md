# 03-equation_solvers.hpp — evidence

`03-equation_solvers.hpp` covers two-variable linear Diophantine equations, one-variable linear congruences and nonnegative representations by several coins (同余最短路). It builds on `extendedGcd`, `floorDiv`, `ceilDiv` and `modNorm` from `01-mod_arithmetic.hpp`. Every operation is deterministic; the Diophantine and congruence operations are logarithmic in the coefficient sizes with constant-size results, and the representation operations are linear in the smallest coin, with the O(m) residue table as their only allocation. There is no cache.

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

### solveDioMinLinear

`solveDioMinLinear(a, b, c, p, q, xl, xr, yl, yr)` returns `{true, x, y}` for a solution of `a * x + b * y = c` in the half-open box `[xl, xr) * [yl, yr)` minimizing `p * x + q * y`, and `{false, 0, 0}` when the box holds none. Domain: every `lng` input, `xl <= xr` and `yl <= yr` (asserted by `solveDioBox`). Ties return one minimizer: the smallest box parameter on a line, `(xl or xr - 1, yl or yr - 1)` by the signs of `p`, `q` on the whole plane. To maximize, pass `-p`, `-q` (`p, q > INT64_MIN`). The cp-algorithms "minimum x + y" is `p = q = 1`; to minimize over `x, y >= 0` alone, pass the box `[0, INT64_MAX)` for both.

Correctness: on a line the objective is linear in the parameter with slope `p * dx + q * dy`, so its minimum over the parameter range `[l, r)` of `solveDioBox` is at `l` for a nonnegative slope and `r - 1` for a negative one; on the plane each coordinate is chosen independently. Widths: `|dx|, |dy| <= 2^63` with `dy = 2^63` only when `a = INT64_MIN`, so `q * dy < 2^126` and the slope stays below `2^127`; the point lies in the box, so it fits `lng`. O(log) as `solveDioBox`.

### residueShortestPath, residueShortestPathDijkstra, frobeniusNumber, countRepresentable, countNonRepresentable

- `residueShortestPath(a)` takes coins `a` (nonempty, every `a_i >= 1`, `min(a) * max(a) <= INT64_MAX`, asserted) and returns `d` of size `m = min(a)`: `d[r]` is the least nonnegative integer combination of the coins congruent to `r` modulo `m`, or `-1` when none exists (`gcd(a) > 1` makes whole classes unreachable). An integer `t >= 0` is representable exactly when `d[t mod m] != -1` and `d[t mod m] <= t`. O(k * m) time, O(m) memory.
- `residueShortestPathDijkstra(a)` takes the same coins with the same assertions and returns the same table, by Dijkstra from residue 0 with a binary heap: O(k * m * log(k * m)) time, O(k * m) memory for the heap.
- `frobeniusNumber(d)` returns the largest nonrepresentable integer, `max(d) - m`: `-1` when every nonnegative integer is representable (`m = 1`), and `INT64_MAX` when some `d[r] = -1` (infinitely many). O(m).
- `countRepresentable(d, x)` returns the number of representable `t` in `[0, x]` as `ulng`, `sum over d[r] <= x of floor((x - d[r]) / m) + 1`; 0 for `x < 0`, exact up to `x = INT64_MAX` (at most `2^63`). O(m).
- `countNonRepresentable(d)` returns the number of nonnegative integers that are not representable, `sum over r of floor(d[r] / m)`, and `INT64_MAX` when some `d[r] = -1` (infinitely many). It is below `(m - 1) * max(a) < INT64_MAX`, so the sentinel cannot collide. O(m).

All three queries take the table, so one O(k * m) build serves any number of them: `frobeniusNumber(residueShortestPath(a))`.

Correctness: residues modulo `m` with an edge `r -> (r + a_i) mod m` of weight `a_i` form a graph whose shortest distances from 0 are `d` (adding `m` keeps the class, so `d[r] + k * m` is representable for every `k >= 0`, and nothing smaller in the class is). The round robin (Böcker and Lipták, COCOON 2005, Theorem 1 and Section 3) adds one coin at a time: with `s = a_i mod m` the edges split into `gcd(m, s)` cycles of length `m / gcd(m, s)`; each cycle is relaxed once around, starting from its minimum, which is final because every path into it along this coin's edges starts at a smaller value. Skipping a cycle with no finite value is correct, and coins with `s = 0` add nothing. Width: every value written is a combination using fewer than `m` coins (the reachable classes before a coin form a subgroup `H`, and a class outside `H` is at most `|H + <s>| / |H| - 1` steps after a class in `H` along its cycle), so values stay at most `(m - 1) * max(a)` and `d[u] + a_i <= m * max(a) <= INT64_MAX`. Frobenius: the largest gap of class `r` is `d[r] - m`. Count: class `r` contributes `d[r], d[r] + m, ..., <= x`. Gaps: class `r` misses `r, r + m, ..., d[r] - m`, which is `floor(d[r] / m)` values; summed this is Selmer's formula `N = (1 / m) * sum(d) - (m - 1) / 2` (Ramírez Alfonsín and Rødseth, eq. (3); the residues add up to `m (m - 1) / 2`).

Dijkstra: each residue is settled once, at its first pop, which carries its least distance, and every edge out of it is relaxed then; a value `du + a_i` stays at most `m * max(a) <= INT64_MAX`, since a shortest path uses fewer than `m` coins. The settled flag also bounds a call past the assertion under `NDEBUG`: with `a = {3, 2^62}` a wrapped `du + a_i` reads as unreached, yet each residue still relaxes its `k` edges once, so at most `k * m + 1` entries are pushed (before the flag, the 2026-11-14 review found it requeued without end). It is slower and lighter, 14 lines against the round robin's 18, short of the half-the-lines test of `01-principles.md` (Priority); it stays as a recorded exception (user decision 2026-11-13, `23-Sweep Ledgers/omissions.md`).

### frobeniusNumberSylvester, countNonRepresentableSylvester

Two coins `a, b >= 1` (asserted), in O(log(max(a, b))) for the gcd: `frobeniusNumberSylvester(a, b) = a * b - a - b` and `countNonRepresentableSylvester(a, b) = (a - 1) * (b - 1) / 2` when `gcd(a, b) = 1` (Sylvester 1882 and 1884; Ramírez Alfonsín and Rødseth, eq. (11)), both as `lll`, exact for every `lng` pair; `-1` and `0` when a coin is 1; `std::numeric_limits<lll>::max()` when `gcd(a, b) > 1`. They agree with `frobeniusNumber` and `countNonRepresentable` on `residueShortestPath({a, b})` without its O(min(a)) memory, so they serve coins up to `INT64_MAX`. The two-coin count of representable values below a bound, through `floorSum`, is planned in row 18 as `countRepresentableSylvester` (user decision 2026-11-12): row 03 cannot include `18-floorsum.hpp`, whose batch MA10 depends on MA01.

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
| `solveDioMinLinear` | Every `a, b` in `±2` (full `±4`), `c` in `±4` (`±8`), `p, q` in `±2` (`±3`) over eight boxes, empty ones included, against enumeration of the objective's minimum; signed64 `p, q` on fixed lines, `a = b = 0` with `c = 0` and `c != 0`, one zero coefficient; random small boxes with signed64 `p, q` (1,000 / 20,000 / 200,000). Python big-integer oracle: the point solves the equation inside the box and its objective equals the better of the two parameter-range ends (2,344 / 6,944 / 51,944 cases). |
| `residueShortestPath`, `residueShortestPathDijkstra`, `frobeniusNumber`, `countRepresentable` | Both tables, compared entry by entry, against an independent knapsack DP over all pairs through 12 (full 25), triples through 7 (12), quadruples through 7 in full, named sets and 200 / 3,000 / 30,000 random sets with duplicates, a coin 1 and `gcd > 1`: every table entry, the Frobenius number (and both sentinels) and prefix counts. Coprime pairs at signed64 scale (100 / 2,000 / 20,000): table entries `j * b mod a`, Sylvester `a * b - a - b`, and exact counts up to `x = INT64_MAX`, including `min * max = INT64_MAX` with gcd 7 and `m = 1`, for both tables. A Python Dijkstra oracle (the oracle binary marks any row where the two tables differ), itself checked against Sylvester and gcd scaling, on 12 boundary sets plus 30 / 250 / 2,500 random sets.; 300,001 coins `{2, 600001, 599999, ..., 3}`, whose one odd residue improves 300,000 times, so a heap that relaxed stale entries would do 9 * 10^10 steps (1,001 coins in the checked build, whose debug heap checks itself on every push). |
| `countNonRepresentable` | The same knapsack DP (a shared reachability sieve, self-checked on `{3, 5}`) over the pairs, triples and quadruples above and 200 / 3,000 / 30,000 random sets of 1–6 coins up to 60 (every fifth with gcd 2–4, every seventh with a duplicate, every eleventh with a 1): the number of unreachable values; `INT64_MAX` when gcd > 1; `{0}` (m = 1) gives 0. Identities `count == F + 1 - countRepresentable(d, F)` and `2 * count >= F + 1`; on coprime pairs up to signed 64-bit scale, `(m - 1)(b - 1) / 2`. |
| `frobeniusNumberSylvester`, `countNonRepresentableSylvester` | Every pair in `[1, 60]^2` against the sieve (largest unreachable value or -1, count of unreachable values, the sieve range closed by a run of `min(a, b)` reachable values) and against `frobeniusNumber` and `countNonRepresentable` of `residueShortestPath({a, b})`, sentinels included (lll max beside `INT64_MAX`); 40 / 400 / 4,000 random pairs up to `10^6` against the table route; 1,076 / 5,676 / 50,676 rows against exact Python integers (`a * b - a - b`, `(a - 1) * (b - 1) // 2`, `2^127 - 1` for gcd > 1): edge pairs near `2^31`, `2^32`, `2^62` and `INT64_MAX`, powers of two with odd numbers, consecutive integers and uniform pairs. Probes `sylvester-zero`, `sylvester-negative`. |
| Preconditions | Sixteen checked-build death probes: zero or negative modulus, with and without a solution, reversed intervals, `residue-empty`, `residue-zero`, `residue-negative`, `residue-overflow` and the same four for `dijkstra`, `sylvester-zero`, `sylvester-negative`. |

## Commands and results

Latest runs 2026-11-14 (P012 MA01, experiment `none`), conditions as in [01-mod_arithmetic.md](01-mod_arithmetic.md); each PASS.

| Command | Result |
|---|---|
| `03-equation_solvers_tester.py --mode full --seed 1` (GCC 16.2.1) | PASS per build: parameterization 88,980, boxes 669,509, congruences 4,818,425, extrema and random 5,019,506, min-linear 6,555,937, residue tables 7,628,044, Sylvester pairs 7,709,380, Sylvester forms 7,719,184 C++ checks (cumulative); Python oracles 5,729 inverse/box, 6,944 min-linear, 262 Dijkstra sets, 5,676 Sylvester; 16 assertion probes; run 1.6 s optimized, 5.2 s checked, 12.3 s ASan-UBSan; pre-flight 49 MB; `MEMORY peak=948MB` |
| same with `CXX=g++-14` (GCC 14.4.1) | PASS, same counts; pre-flight 824 MB; `MEMORY peak=1022MB` |
| `03-equation_solvers_tester.py --mode stress --seed 2` | PASS: 200,000 random cases, 8,751,787 min-linear, 18,082,123 residue, 18,880,347 Sylvester-pair and 18,893,751 Sylvester-form checks (cumulative), Python 50,729 / 51,944 / 2,512 / 50,676; 16 probes; run 10.3 s, 36.3 s, 39.9 s; `MEMORY peak=1054MB` |
| `05-extraction.py ops --filter 05-Mathematics/03-equation_solvers.hpp` | PASS, 21 operations in 13 extractions on g++ and g++-14, 0 failures |
| `05-extraction.py testers --filter 05-Mathematics/03-equation_solvers_tester` | PASS, 1 extracted quick run |
| Other MA01 suites, `02-integration.py` and `03-consistency.py` | details and commands in [01-mod_arithmetic.md](01-mod_arithmetic.md) |

## Benchmarks

No specialized arithmetic or threshold exists, so no benchmark is needed. Böcker and Lipták (2005) give the round robin Θ(k * m) time and know no other O(k * m) algorithm; Dijkstra with a binary heap (Nijenhuis) costs O(k * m * log(k * m)) and is kept only as the lighter alternative.

## Mutation testing

Record: mewt 4.0.0 on blob `072248d0adc29d7235a409cb3e0ec4c03010636f` (2026-11-14, seed 1, 2 lines changed since blob `3441460708a8`): 8 mutants, 8 caught (1 by timeout), 0 survived, 0 untested.

Stage times: quick 133 s (with the survivor's retest), full optimized 4 s, full rest 41 s. The survivors below are those of this package's campaigns since the whole-file campaign of 2026-11-12 (500 sampled mutants, 27 survived) and the changed-lines campaigns of 2026-11-13 (91 mutants, 4 survived) and 2026-11-14 (`residueShortestPathDijkstra`, 75 mutants, whose 3 survivors went with the stale-entry test the settled flag replaced; then the flag's 2 lines, 8 mutants, where `IF 138`, the skip never taken, survived until the stale-heap-entries case made it time out alone); lines are those of the current header. Every survivor is equivalent; the six marked NDEBUG differ only when `NDEBUG` removes an assertion a caller violates (`xl <= xr`, `yl <= yr`, or `a, b >= 1`), and stay within the operation's bound there, which `05-testing.md` (Mutation testing) counts as equivalent (user decision 2026-11-12).

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 398`, `m < 0` → `<=` | 16 | `m = b / g` is nonzero for `b != 0`. |
| `CR 52`, `IF 92`, early return removed or never taken; `COS 419`, `COS 420`, `COS 424`, `COS 425` (NDEBUG), `xl == xr` / `yl == yr` → `>` / `>=`; `COS 426`, `COS 428`, `dimension < 0` → `== 0` / `<= 0` | 30 | A fast path: `xl <= xr`, `yl <= yr` are asserted and `dimension` is -1, 1 or 2. Without it, an empty box makes `restrict` fail or leave `l = r = 0`, the plane case multiplies by a zero width, and a no-solution result has `x = y = dx = dy = 0`, so both `restrict` calls take the `d == 0` branch and leave the zero range: count 0 and `l = r = 0` in every case. |
| `COS 435`, `COS 475`, `dimension == 2` → `>=` | 31, 60 | No dimension exceeds 2. |
| `AS 128`, `swap(u, v)` → `swap(v, u)`; `AS 131`, `AS 132`, `max`/`min` operands swapped | 37, 40 | Symmetric operations. |
| `COS 453`, `d < 0` → `<=` | 37 | `d == 0` returned two lines above. |
| `AS 149`, `gcd(m, s)` → `gcd(s, m)`; `AS 151`, `max` operands swapped | 72, 104 | Symmetric. |
| `COS 502`, `r < m / len` → `!=` | 73 | `r` steps by 1 from 0 and `m / len >= 1`. |
| `COS 507`, `COS 527`, `i < len` → `!=` | 75, 78 | `i` steps by 1 from 1 and `len >= 1`. |
| `COS 508`, `COS 528`, `i < len` → `<=` | 75, 78 | One more step returns to the cycle's start, which is already the minimum (search) or cannot improve on the minimum it started from (relaxation, `d[last] + b > d[best]`). |
| `COS 513`, `COS 533`, `u + s < m` → `<=` | 75, 79 | `u + s = m` would need `u = m - s`, the last element of class 0's cycle from 0, which neither walk steps out of (each makes `len - 1` steps from the start, and class 0 starts at `d[0] = 0`, its unique minimum). |
| `COS 518`, `d[u] < d[best]` → `<=` | 75 | A later tied minimum is an equally valid start: one pass from any minimum of the cycle is final. |
| `COS 525`, `COS 540`, `== INF` → `>=` | 76, 81 | No value exceeds `INF`. |
| `AS 168`, `AS 169`, `gcd(a, b)` → `gcd(b, a)`; `COS 628`, `COS 643` (NDEBUG), `gcd(a, b) > 1` → `!= 1` | 120, 123 | `gcd` is symmetric, and for `a, b >= 1` it is at least 1, so `> 1` and `!= 1` agree; only `gcd(0, 0) = 0`, past the violated `a, b >= 1` under `NDEBUG`, tells them apart, and both forms return at once. |

## Sources

Inspected on 2026-09-27 and in the P012 MA01 sweep of 2026-11-12 ([00-sources.md](00-sources.md)); implemented independently from the proofs, with no external code copied.

- [cp-algorithms, Linear Diophantine Equation](https://cp-algorithms.com/algebra/linear-diophantine-equation.html): gcd solvability, the solution lattice, interval intersection and linear-objective endpoints (also `solveDioMinLinear`). Its bounded code excludes zero coefficients and uses closed intervals; this header handles zeros and uses half-open ranges.
- [KACTL, `euclid.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/euclid.h): independent comparison of the Bézout and inverse contract.
- [OI Wiki, 线性同余方程](https://oi-wiki.org/math/number-theory/linear-equation/): the least nonnegative solution and the `n / gcd(a, n)` progression. It follows the same e-maxx lineage, so it serves as exposition only.
- [OI Wiki, 同余最短路](https://oi-wiki.org/graph/mod-shortest-path/): the residue graph, Dijkstra on it and the representable count.
- [dansesacrale, round-robin Frobenius](https://dansesacrale.wordpress.com/2010/09/20/frobenius-numbers-round-robin-algorithm/) and [Böcker and Lipták, COCOON 2005](https://www.yumpu.com/en/document/view/37799071/the-money-changing-problem-revisited-universitat-bielefeld) (LNCS 3595, pp. 965–974): the round robin from each cycle's minimum, Θ(k * m) time (Theorem 1).
- [Ramírez Alfonsín and Rødseth](https://imag.umontpellier.fr/~ramirez/Frobenius-Hilbert.pdf), eqs. (2), (3), (11): Selmer's gap count and the two-coin forms; [OI Wiki, 裴蜀定理](https://oi-wiki.org/math/number-theory/bezouts/) and [Wikipedia, Coin problem](https://en.wikipedia.org/wiki/Coin_problem) for Sylvester's formulas.

## Limits and handoffs

- Integer systems and lattice normal forms belong to `49-integer_linear_algebra.hpp`.
- The 2026-10-07 completeness sweep handed `firstModInRange` (maspypy `first_mod_range_of_linear`) to `18-floorsum.hpp`. The P012 MA01 sweep planned the n-variable Bézout solve in row 49 (`solveDioEqMulti`) and the two-coin bounded count in row 18 (`countRepresentableSylvester`), and the left-out items with reasons are in [00-notes.md](00-notes.md); `/finish` added `residueShortestPathDijkstra` (user decision 2026-11-13).
- `residueShortestPath` needs O(min(a)) memory, so `min(a)` up to about `3 * 10^7` fits a 256 MB limit (8 bytes per residue); `residueShortestPathDijkstra` adds a settled byte per residue and up to `k * min(a) + 1` heap entries of 16 bytes; two larger coins take the Sylvester forms.
- The previously active header exposed only a particular solution and one modular progression. `solveDioEq` and `solveModEq` keep their names, with the signed-minimum and normalization overflows removed; the reference-output overload maps as in Legacy mapping. `OLD` material is unchanged.

## Aliases

Search aliases (P263, grab --find): 线性同余方程, 同余方程, 一次不定方程, 二元一次不定方程, 丢番图方程, 不定方程, 同余最短路, Frobenius number, coin problem, money changing problem, Sylvester formula, Chicken McNugget theorem, Apéry set, numerical semigroup genus, 硬币问题, 弗罗贝尼乌斯数.

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed on all configurations.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, 2 findings fixed; after `@reviewer`, full, stress (seed 1) and integration passed.
- 2026-10-12: prune, removed the reference-output `solveDioEq` overload (its `g` and `y` overflow inputs now checked on the result overload); full suite passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P012 MA01, added `solveDioMinLinear`, `residueShortestPath`, `frobeniusNumber` and `countRepresentable` after a `@researcher` sweep; the dead `y *= c / g` in `solveDioEq` was removed; after two `@reviewer` passes, probes `mod-zero-nosolution` and `mod-negative-nosolution`; first mutation campaigns, ending with a whole-file campaign on the final blob; full on g++ and g++-14, stress (seed 1), extraction and integration passed.
- 2026-11-13: P012 MA01 reopened (user decisions 2026-11-12): added `countNonRepresentable`, `frobeniusNumberSylvester` and `countNonRepresentableSylvester`, Sylvester and table-route checks with an exact Python oracle, probes `sylvester-zero`, `sylvester-negative`; the two-coin count went to row 18 (planned), since this row cannot include `18-floorsum.hpp`; a focused `@reviewer` pass fixed a contract heading; changed-lines mutation campaign; full on g++ and g++-14, stress (seed 2), extraction and integration passed.
- 2026-11-14: `/finish MA01` (user decision 2026-11-13): added `residueShortestPathDijkstra`, checked against the knapsack DP, the pair tables and the round robin, probes `dijkstra-empty`, `dijkstra-zero`, `dijkstra-negative`, `dijkstra-overflow`, a recorded Priority exception; the step-5 review's fix: a settled flag bounds it under `NDEBUG` past the overflow assertion, and a stale-heap-entries case kills the skip's survivor; Böcker–Lipták fetched (COCOON 2005); changed-lines mutation campaigns; full on g++ and g++-14, stress (seed 2), extraction and integration passed.
