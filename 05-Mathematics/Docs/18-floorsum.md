# 18-floorsum.hpp — evidence

`18-floorsum.hpp` covers floor sums of linear functions, modular linear sequences (sums, counts, extrema, first hits) and quotient blocks of `floor(n / x)`. It depends only on `01-mod_arithmetic.hpp` (`floorDiv`, `ceilDiv`, `modNorm`). Every operation is deterministic and allocation-free except `enumerateQuotients`, whose output is `O(sqrt(n))`.

## Contracts

Common domain: `n >= 0` (`n >= 1` for the extrema), `1 <= m < 2^63`, and `a`, `b` are `lll` with `|a|, |b| <= 2^126`; `floorSumWide` widens this to `1 <= m < 2^127` and any `lll` `a`, `b`. Any `lng` or `ulng` argument converts implicitly. The precondition violations `m < 1`, `n < 0`, `l > r`, `l < 0`, `r > m` and an empty extremum range are asserted.

### floorSum, ceilSum

`floorSum(n, m, a, b)` returns the sum over `i` in `[0, n)` of `floor((a i + b) / m)`, and `ceilSum` returns the sum of the ceilings. The result is computed modulo 2^128 and returned as `lll`. It is exact whenever the true sum lies in `[-2^127, 2^127)`, which always holds for `|a|, |b| < 2^63` and `n, m < 2^31`, and for nonnegative `a, b < m` with any `n`. ACL's unsigned contract (any `ulng` `a`, `b`, `n, m < 2^32`, result modulo 2^64) is `ulng(floorSum(n, m, a, b))`.

Algorithm: an `a` or `b` outside `[0, 2^64)` is reduced by `q = floorDiv(x, m)`, which contributes `q n (n - 1) / 2` or `q n`. The kernel `floorsum_detail::reduced` is ACL's Euclidean loop over 64-bit `n`, `m`, `a`, `b` with a 128-bit accumulator. It folds `a >= m` and `b >= m` into the accumulator, sets `y = a n + b`, stops when `y < m`, and otherwise continues with `(floor(y / m), a, m, y mod m)`, the lattice-point reciprocity that swaps the roles of `m` and `a`.

Correctness and widths: after the fold, `a, b < m < 2^63` and `n < 2^63`, so `y < m (n + 1) < 2^127` and the new `n` is at most the old one. The modulus sequence `(m, a)` follows Euclid, so there are `O(log(m))` rounds. When `y < 2^64` (always when `n, m < 2^32`), both divisions are 64-bit; otherwise they are 128-by-64 divisions. Every accumulated term is exact modulo 2^128, and the reduced sum is below `n (n + 1) < 2^127`. `ceilSum` is `floorSum(n, m, a, b + m - 1)`.

### floorSumWide

`floorSumWide(n, m, a, b)` is `floorSum` for a 128-bit modulus. It takes `n >= 0`, `1 <= m < 2^127` and any `lll` `a`, `b`, and returns the sum modulo 2^128, exact when the true sum fits `lll`.

Algorithm:
- `a` and `b` are folded into `[0, m)` by `floorDiv`, contributing `q n (n - 1) / 2` and `q n`. The remainder `a - q m` is formed in `ulll`, because `q m` alone can leave `lll` (`a = -2^127`, `m = 3`).
- The kernel `floorsum_detail::reducedWide` runs the same Euclidean loop as `reduced` with 128-bit `m`, `a`, `b`.
- Each round needs `(a n + b) / m` and its remainder, where `a, b < m < 2^127` and `n < 2^63`. `divStep` forms the 191-bit numerator from two 64-by-64-bit partial products (the low and high halves of `a` times `n`) and an addition, propagating both carries. When the numerator fits 128 bits it divides directly. Otherwise the numerator is below `m 2^63`, so `m > 2^65`, and one step of Knuth's Algorithm D gives the quotient word:
  - shift `m` left by `s`, the leading zeros of its high word (`s` in `[1, 62]`), so that the top bit of `d = m 2^s = <d1, d0>` is set, and shift the numerator by the same `s` into three words `<u2, u1, u0>`;
  - estimate `q = <u2, u1> / d1` with one 128-by-64 division and `r = <u2, u1> − q d1`;
  - decrease `q` and add `d1` to `r` while `r < 2^64` and `q d0 > <r, u0>`;
  - the remainder is `(<r, u0> − q d0) >> s`, computed modulo 2^128 because it lies in `[0, d)`.

Widths:
- The quotient is at most `n < 2^63`, and the loop keeps `a, b < m`.
- With `T = <u2, u1> < d / 2 < (d1 + 1) 2^63`, `u2 < d1`, so the estimate fits one word. It exceeds the true quotient by at most 1: `q − X / d <= T / d1 − T / (d1 + 1) = T / (d1 (d1 + 1)) < 2^63 / d1 <= 1` for the shifted numerator `X`. So the correction runs at most once; Knuth's general bound of 2 cannot occur here.
- That makes `O(log(m))` rounds of `O(1)` word operations.

Geometry row 19 uses it for rational polygons whose slopes need more than 64 bits (P037). It is kept beside `floorSum` by decision (2026-10-19) for its larger domain.

### modSum, countModInRange

`modSum(n, m, a, b)` returns the exact sum of `(a i + b) mod m` over `i` in `[0, n)`; it lies in `[0, n (m - 1)]`, below 2^126. It is computed as `a' n (n - 1) / 2 + b' n - m floorSum(n, m, a', b')` with `a' = a mod m` and `b' = b mod m`, modulo 2^128.

`countModInRange(n, m, a, b, l, r)` with `0 <= l <= r <= m` counts the `i` in `[0, n)` with `l <= (a i + b) mod m < r`. For `y = a i + b`, `[l <= y mod m < r] = floor((y - l) / m) - floor((y - r) / m)`, so the count is a difference of two floor sums. `l == r` gives 0, and `[0, m)` gives `n`.

### floorSumMoments

`floorSumMoments<T>(n, m, a, b)` returns `{sum f, sum i f, sum f^2}` over `i` in `[0, n)` for `f = floor((a i + b) / m)`. Here `a` and `b` are any `lng`, and `T` is a commutative ring constructible from `lng` with `+`, `-` and `*`: a modint gives the sums modulo its modulus, and `ulll` gives them modulo 2^128. No division happens in `T`, so composite and power-of-two moduli work.

Algorithm: the recursion tracks `F = sum f`, `G = sum i f` and `H = sum C(f + 1, 2)` and returns `2 H - F` as the third moment. Writing `f_i = A i + B + r_i` with `A = floorDiv(a, m)`, `B = floorDiv(b, m)` and `r_i` the reduced floor gives `F`, `G` and `H` from the reduced triple, using `C(x + y + 1, 2) = C(x + 1, 2) + C(y + 1, 2) + x y` and `C(A i, 2) = A^2 C(i, 2) + C(A, 2) i`. For reduced `0 <= a, b < m`, swapping by `f_i = #{j < M : i > t_j}` with `M = f_{n-1}` and `t_j = floor((m j + m - b - 1) / a)` gives:

- `F = M (n - 1) - F'`
- `G = M C(n, 2) - H'`
- `H = (n - 1) C(M + 1, 2) - G' - F'`

for the subproblem `(M, a, m, m - b - 1)`. The integer binomials `C(x + 1, 2)` and `C(n, 3)` are formed by dividing the even factor and the factor divisible by 3 before converting to `T`. A factor that would leave `lng`, such as `A - 1` for `A = -2^63`, is formed inside `T`. The recursion depth is `O(log(m))`.

### minOfModOfLinear, maxOfModOfLinear, firstModInRange

`minOfModOfLinear(n, m, a, b)` with `n >= 1` returns `{v, x}`: the minimum `v` of `(a x + b) mod m` over `x` in `[0, n)` and the least `x` attaining it. `maxOfModOfLinear` returns the maximum and its least `x`, through `(-a x + m - 1 - b) mod m = m - 1 - ((a x + b) mod m)`.

`firstModInRange(m, a, b, l, r)` with `0 <= l <= r <= m` returns the least `x >= 0` with `l <= (a x + b) mod m < r`, or `-1` when no `x` exists (including `l == r`). Such an `x` is below `m / gcd(a, m)`.

Algorithms, both `O(log(m))`; the extrema recurse at most `log2(m) + 1` levels and the first hit at most `2 log2(m) + 2` (a mirror level can precede each halving):

- **Minimum.** With reduced `a, b`:
  - If `a <= m - a`, the values rise by `a` in laps that wrap below `m`. Lap 0 starts at `b`, and lap `j >= 1` starts at its own minimum, `(b - j m) mod a`, at `x = ceil((j m - b) / a)`. There are `floor((a (n - 1) + b) / m)` laps after lap 0, so the minimum over laps 1, 2, ... is the same problem modulo `a`, with step `(-m) mod a` and offset `(b - m) mod a`. `b` wins ties because `x = 0` is least.
  - If `a > m - a`, the values fall by `d = m - a`. Each run ends at its minimum, the values below `d`, and these run ends form `(b mod d + j (m mod d)) mod d` at `x = (b + j m - e_j) / d`. Their count `ceil((d n - b) / m)` comes from a telescoping sum. With no run end in range, the values only fall and the minimum is at `x = n - 1`.
  - Either way the modulus at least halves.
- **First hit.** A large step is first mirrored to `m - a` with the interval `[m - r, m - l)`. Lap 0 is checked directly. Lap `j >= 1` starts at `(b - j m) mod a` and meets `[l, r)` exactly when `(start - l) mod a < r - l`, so the least lap solves the same problem modulo `a` with interval `[0, min(r - l, a))`. Its least `x` is `ceil((j m + l - b) / a)`.

### forEachQuotientBlock, enumerateQuotients, forEachJointQuotientBlock

`forEachQuotientBlock(n, f)` with `0 <= n <= 2^62` calls `f(q, l, r)` for every maximal `[l, r)` of `x` in `[1, n]` with `floor(n / x) = q`, in ascending `x`. There are `2 floor(sqrt(n)) - [s^2 + s > n]` blocks, with `s = floor(sqrt(n))`, and `r = n / q + 1`. `enumerateQuotients(n)` returns the distinct `floor(n / x)` in ascending order (`n = 0` gives none), matching Library Checker `enumerate_quotients`.

`forEachJointQuotientBlock(vs, f)` with every `v` in `[0, 2^62]` calls `f(l, r)` for every maximal `[l, r)` of `x` in `[1, max v]` on which every `floor(v / x)` is constant. A value below `l` contributes quotient 0 for the rest of the range. The cost is `O(k)` per block for `k` values, and there are at most `sum 2 sqrt(v)` blocks. This is the multi-bound divisor-block loop of Möbius sums.

### Legacy mapping

| Removed | Replacement |
|---|---|
| `getFloorSum(n, a, b, mod)` (`OLD/5-Mathematics/14-miscellaneous.hpp`, nonnegative `a`, `b`) | `floorSum(n, mod, a, b)`, any signed `a`, `b` |
| `floorSumUnsigned` (row only; user decision 2026-10-18) | `ulng(floorSum(n, m, a, b))` for every `ulng` `a`, `b` |

Aliases: ACL `floor_sum`; KACTL `divsum`, `modsum`; maspypy `floor_sum_of_linear`, `mod_sum_of_linear`, `range_freq_of_linear`, `min_of_linear`, `max_of_linear`, `first_mod_range_of_linear`, `floor_range`; Nyaan `sum_of_floor`, `mod_affine_range_counting`, `enumerate_quotient`; hitonanode `get_quotients`; suisen `enumerate_quotients`, `quotient_ranges`, `multiple_quotients_ranges`; ei1333 `sum_of_floor_of_linear`; Library Checker `sum_of_floor_of_linear`, `min_of_mod_of_linear`, `enumerate_quotients`.

## Feature-to-test map

Entry: [`18-floorsum_tester.py`](<../../96-Local Testing/05-Mathematics/18-floorsum_tester.py>).

| Feature | Independent evidence |
|---|---|
| `floorSum`, `ceilSum`, `modSum` | Direct 128-bit loops for every `n <= 12`, `m <= 12` and `a, b` in `[-15, 15]`, every signed64 extreme `a`, `b` with moduli up to `2^63 - 1`, and seeded loops up to length 2000 with any modulus. Exact Python integers through the OI Wiki recursion, an independent derivation, on 3000 rows up to `n = m = 2^63 - 1`. Identities modulo 2^128 for `a`, `b` up to 2^126 in magnitude: additivity in `n`, `a + m`, `b + m`, and `ceilSum` as the negated mirror. Closed forms at `n = m = 2^63 - 1`, at 2^126 coefficients, and across the 2^64 division boundary. |
| `floorSumWide` | Direct loops modulo 2^128 for `n <= 64`, moduli up to `2^126` and coefficients below `2^120`; equality with `floorSum` on random 64-bit moduli with coefficients up to `2^125`; `m = 2^127 - 1` with `n = 2^63 - 1`; extreme coefficients `-2^127` and `2^127 - 1`, including reductions of `-2^127` whose `q m` alone would overflow. `floorsum_detail::divStep` against a tester-local restoring division of the 192-bit numerator: moduli at both ends of the shift range (shift 1: `2^127 − 1`, `2^127 − 2`, `2^126`, `2^126 + 1`, `2^127 − 2^64`; shift 62: `2^65 + 1`, `2^66 − 1`) and on the direct path (`2^65 − 1`, `2^64 + 1`) with the largest quotients (`a = b = m − 1`, `n = 2^63 − 1`); random moduli of 65–127 bits (full 200,000, stress 10^6, quick 20,000); and remainders 0–3, which the correction test must decide by the low word. On an instrumented copy the full run took 86,043 steps without and 607 with one correction, over shifts 1 to 62, the whole reachable range. The exact Python integer oracle checks quick 300, full 3000 and stress 20,000 rows, with moduli up to `2^127 - 1` and coefficients over the whole `lll` range. |
| `countModInRange` | Every `[l, r)` in the exhaustive domain against counting, seeded long loops, additivity over adjacent ranges, the total `n`, and the Python identity. |
| `floorSumMoments` | Direct sums in `ulll` and in `mint` (`ModInt<998244353>`) in the exhaustive and long-loop domains, the first moment against `floorSum`, and the Python OI Wiki `f`, `g`, `h` recursion modulo 2^128 and modulo 998244353. |
| `minOfModOfLinear`, `maxOfModOfLinear` | Direct scans with the least index, and certificates: the value at `x`, no smaller (larger) value counted by `countModInRange`, and `x` recovered by `firstModInRange`. A Python binary search on the value with floor-sum counts and the least argument from a modular inverse, a different algorithm. |
| `firstModInRange` | A one-period scan for every `[l, r)` in the exhaustive domain, certificates (value in range, no earlier hit by count, a full-period count for `-1`), and a Python binary search on the prefix count. |
| `forEachQuotientBlock`, `enumerateQuotients`, `forEachJointQuotientBlock` | Exhaustive `n <= 3000` against the set of quotients, and tiling, maximality and the block-count formula up to `10^12` and `2^40`. Joint blocks against the per-`x` quotient tuples for up to four values below 400. |
| Oracle validation | The Python recursions, the value search and the first-hit search are checked against brute force for `n <= 8`, `m <= 7` and `a, b` in `[-9, 9]` before use. |
| Preconditions | Eleven checked-build death probes: `m = 0` (for `floorSum` and `floorSumWide`), `n < 0`, reversed and oversized ranges, moments with `m = 0`, an empty minimum, `max` with `m = 0`, a negative bound, and negative quotient inputs. A separate `-DNDEBUG` probe (2026-10-18, `-O0` and `-O2`) confirmed that 16 violating calls (zero or negative `m` and `n`, empty and reversed ranges, negative quotient inputs) terminate. |

## Commands and results

Run 2026-10-19 on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. The shared runner builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and (full, stress) ASan/UBSan configurations. Counts are per configuration. With `-Wall -Wextra -Wconversion`, the header and tester compile without warnings on both compilers.

| Command | Result |
|---|---|
| `18-floorsum_tester.py --mode full --seed 1` | PASS, 32,095,166 C++ checks (400,160 of them on `divStep`), 3000 Python rows and 3000 `floorSumWide` Python rows per configuration, 11 assertion probes, `MEMORY peak=773MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=773MB` |
| `18-floorsum_tester.py --mode stress --seed 7` | PASS, 160,479,218 C++ checks (2,000,160 on `divStep`), 20,000 Python rows and 20,000 `floorSumWide` Python rows per configuration, 11 assertion probes, `MEMORY peak=902MB` |
| `20-continued_fraction_tester.py --mode full --seed 1` (includes this header), `g++` and `g++-14` | PASS, 3,805,206 checks and 3000 Python rows per configuration, 17 probes, `MEMORY peak=818MB` and `817MB` |
| `18-floorsum_benchmark.py 5` | PASS; table below |
| `02-integration.py` | PASS, 168 standalone and aggregate headers, two translation units, workspace, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py` | No errors |
| `03-consistency.py --braces` on the header, tester and benchmark | No violations |

## Benchmarks

`18-floorsum_benchmark.py 5` (2026-10-19, round 3), GCC 16.2.1, `-O2` without `-march`, Intel Core i9-11900H, one warmup and the median of 5 repetitions. Run serially on the final code, with other sessions keeping the load average near 2.7. Outputs are compared across kernels, and the Farey rank is verified.

10^6 random `floorSum` queries with `0 <= a, b < m`:

| Workload | Shipped `floorSum` | Always-128-bit kernel | ACL 64-bit kernel |
|---|---|---|---|
| `n, m <= 10^9` (Library Checker sizes) | 211.6 ms | 224.8 ms | 187.4 ms |
| `n, m < 2^62` | 440.1 ms | 514.5 ms | not valid |

The 64-bit division branch keeps `floorSum` within 13% of ACL's 64-bit-only kernel on its domain and faster than an always-128-bit kernel everywhere. Skipping the 128-bit normalization for `a, b` in `[0, 2^64)` removed a measured 10% overhead on 62-bit inputs.

10^5 random `floorSumWide` queries with `n < 2^63` and `0 <= a, b < m`, against the former kernel with 128 restoring steps per round (kept in the benchmark):

| Workload | Shipped (one Knuth D word step) | Former bitwise kernel |
|---|---|---|
| `m < 2^64` | 59.7 ms | 81.2 ms |
| `2^64 <= m < 2^127` | 97.7 ms | 787.8 ms |

The word step makes `floorSumWide` 8.1× faster on wide moduli. Below `2^64` both take the direct division; the 1.4× there comes from the smaller kernel.

## Sources

Fetched 2026-10-18 by `@researcher` ([00-sources.md](00-sources.md), P060 sweep). The algorithms were implemented from the stated identities, and no external code was copied.

- AC Library `floor_sum`: the unsigned Euclidean loop and its `n, m < 2^32`, modulo 2^64 contract.
- OI Wiki 类欧几里得算法: the `f`, `g`, `h` recursion, used here only as the Python oracle. The header uses the division-free binomial form.
- maspypy `mod/` and Library Checker `min_of_mod_of_linear`, `sum_of_floor_of_linear`, `enumerate_quotients`: contracts of the extrema, first hit and quotient enumeration.
- OI Wiki 数论分块 and suisen `multiple_quotients_ranges`: joint quotient blocks.
- Round 3 (2026-10-19), from the P037 bounds sweep: [GMP manual, Basecase Division](https://gmplib.org/manual/Basecase-Division) and the [Möller–Granlund division draft](https://www.lysator.liu.se/~nisse/archive/draft-division-paper.pdf) (normalized divisor, word estimate, at most two corrections). `divStep` implements Knuth's Algorithm D for one quotient word from those statements.

## Limits and handoffs

- Polynomial-weighted floor sums `sum i^a floor^b` for general `a`, `b`, the universal Euclidean monoid product and convex floor sums are row 55.
- Recursion depth is at most `log2(m) + 1` for the extrema, `2 log2(m) + 2` for the first hit and `O(log(m))` for `floorSumMoments`.
- `OLD/5-Mathematics/14-miscellaneous.hpp` stays in `OLD`; there was no `97-Legacy` excerpt.

## History

- 2026-10-18: P060 initial implementation; `@reviewer` found 8 package issues, all fixed; full suite passed on g++ and g++-14, stress seed 7 and integration passed.
- 2026-10-19: P037 added `floorSumWide` for geometry row 19; review fixes (coefficient reduction modulo `2^128`, direct division when the numerator fits 128 bits) and a tester overflow caught by the stress run; full suite passed on g++ and g++-14, stress seed 7 passed.
- 2026-10-19 (round 3): `divStep` became one Knuth D word step with at most one correction (mutation checks on a private copy caught four planted faults); review found a 128-bit shift reachable by a negative modulus under `NDEBUG` (split, UB-free) and pinned shift-62 moduli; final full suite passed on g++ and g++-14, stress seed 7 and row 20 full passed.
