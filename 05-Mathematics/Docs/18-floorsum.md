# 18-floorsum.hpp — evidence

`18-floorsum.hpp` covers floor sums of linear functions, modular linear sequences (sums, counts, extrema, first hits) and quotient blocks of `floor(n / x)`. It depends only on `01-mod_arithmetic.hpp` (`floorDiv`, `ceilDiv`, `modNorm`). Every operation is deterministic and allocation-free except `enumerateQuotients`, whose output is `O(sqrt(n))`.

## Contracts

Common domain: `n >= 0` (`n >= 1` for the extrema), `1 <= m < 2^63`, and `a`, `b` are `lll` with `|a|, |b| <= 2^126`. Any `lng` or `ulng` argument converts implicitly. The precondition violations `m < 1`, `n < 0`, `l > r`, `l < 0`, `r > m` and an empty extremum range are asserted.

### floorSum, ceilSum

`floorSum(n, m, a, b)` returns the sum over `i` in `[0, n)` of `floor((a i + b) / m)`, and `ceilSum` returns the sum of the ceilings. The result is computed modulo 2^128 and returned as `lll`. It is exact whenever the true sum lies in `[-2^127, 2^127)`, which always holds for `|a|, |b| < 2^63` and `n, m < 2^31`, and for nonnegative `a, b < m` with any `n`. ACL's unsigned contract (any `ulng` `a`, `b`, `n, m < 2^32`, result modulo 2^64) is `ulng(floorSum(n, m, a, b))`.

Algorithm: an `a` or `b` outside `[0, 2^64)` is reduced by `q = floorDiv(x, m)`, which contributes `q n (n - 1) / 2` or `q n`. The kernel `floorsum_detail::reduced` is ACL's Euclidean loop over 64-bit `n`, `m`, `a`, `b` with a 128-bit accumulator. It folds `a >= m` and `b >= m` into the accumulator, sets `y = a n + b`, stops when `y < m`, and otherwise continues with `(floor(y / m), a, m, y mod m)`, the lattice-point reciprocity that swaps the roles of `m` and `a`.

Correctness and widths: after the fold, `a, b < m < 2^63` and `n < 2^63`, so `y < m (n + 1) < 2^127` and the new `n` is at most the old one. The modulus sequence `(m, a)` follows Euclid, so there are `O(log(m))` rounds. When `y < 2^64` (always when `n, m < 2^32`), both divisions are 64-bit; otherwise they are 128-by-64 divisions. Every accumulated term is exact modulo 2^128, and the reduced sum is below `n (n + 1) < 2^127`. `ceilSum` is `floorSum(n, m, a, b + m - 1)`.

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
| `countModInRange` | Every `[l, r)` in the exhaustive domain against counting, seeded long loops, additivity over adjacent ranges, the total `n`, and the Python identity. |
| `floorSumMoments` | Direct sums in `ulll` and in `mint` (`ModInt<998244353>`) in the exhaustive and long-loop domains, the first moment against `floorSum`, and the Python OI Wiki `f`, `g`, `h` recursion modulo 2^128 and modulo 998244353. |
| `minOfModOfLinear`, `maxOfModOfLinear` | Direct scans with the least index, and certificates: the value at `x`, no smaller (larger) value counted by `countModInRange`, and `x` recovered by `firstModInRange`. A Python binary search on the value with floor-sum counts and the least argument from a modular inverse, a different algorithm. |
| `firstModInRange` | A one-period scan for every `[l, r)` in the exhaustive domain, certificates (value in range, no earlier hit by count, a full-period count for `-1`), and a Python binary search on the prefix count. |
| Quotient blocks | Exhaustive `n <= 3000` against the set of quotients, and tiling, maximality and the block-count formula up to `10^12` and `2^40`. Joint blocks against the per-`x` quotient tuples for up to four values below 400. |
| Oracle validation | The Python recursions, the value search and the first-hit search are checked against brute force for `n <= 8`, `m <= 7` and `a, b` in `[-9, 9]` before use. |
| Preconditions | Ten checked-build death probes: `m = 0`, `n < 0`, reversed and oversized ranges, moments with `m = 0`, an empty minimum, `max` with `m = 0`, a negative bound, and negative quotient inputs. A separate `-DNDEBUG` probe (2026-10-18, `-O0` and `-O2`) confirmed that 16 violating calls (zero or negative `m` and `n`, empty and reversed ranges, negative quotient inputs) terminate. |

## Commands and results

Run 2026-10-18 on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. The shared runner builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and (full, stress) ASan/UBSan configurations. Counts are per configuration. With `-Wall -Wextra -Wconversion`, the header and tester compile without warnings on both compilers.

| Command | Result |
|---|---|
| `18-floorsum_tester.py --mode full --seed 1` | PASS, 32,873,542 C++ checks and 3000 Python rows per configuration, 10 assertion probes, `MEMORY peak=438MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=439MB` |
| `18-floorsum_tester.py --mode stress --seed 7` | PASS, 156,726,328 C++ checks and 20,000 Python rows per configuration, 10 assertion probes, `MEMORY peak=642MB` |
| `18-floorsum_benchmark.py 5` | PASS; table below |
| `02-integration.py` | PASS, 159 standalone and aggregate headers, two translation units, workspace, `MEMORY peak=651MB` |
| `03-consistency.py` | PASS, no errors |

## Benchmarks

`18-floorsum_benchmark.py`, GCC 16.2.1, `-O2` without `-march`, Intel Core i9-11900H, one warmup and the median of 5 repetitions, 10^6 random queries with `0 <= a, b < m`. Outputs are compared across kernels, and the Farey rank is verified.

| Workload | Shipped `floorSum` | Always-128-bit kernel | ACL 64-bit kernel |
|---|---|---|---|
| `n, m <= 10^9` (Library Checker sizes) | 210.5 ms | 222.9 ms | 207.7 ms |
| `n, m < 2^62` | 513.9 ms | 542.9 ms | not valid |

The 64-bit division branch keeps `floorSum` within 1.5% of ACL's 64-bit-only kernel on its domain and faster than an always-128-bit kernel everywhere. Skipping the 128-bit normalization for `a, b` in `[0, 2^64)` removed a measured 10% overhead on 62-bit inputs.

## Sources

Fetched 2026-10-18 by `@researcher` ([00-sources.md](00-sources.md), P060 sweep). The algorithms were implemented from the stated identities, and no external code was copied.

- AC Library `floor_sum`: the unsigned Euclidean loop and its `n, m < 2^32`, modulo 2^64 contract.
- OI Wiki 类欧几里得算法: the `f`, `g`, `h` recursion, used here only as the Python oracle. The header uses the division-free binomial form.
- maspypy `mod/` and Library Checker `min_of_mod_of_linear`, `sum_of_floor_of_linear`, `enumerate_quotients`: contracts of the extrema, first hit and quotient enumeration.
- OI Wiki 数论分块 and suisen `multiple_quotients_ranges`: joint quotient blocks.

## Limits and handoffs

- Polynomial-weighted floor sums `sum i^a floor^b` for general `a`, `b`, the universal Euclidean monoid product and convex floor sums are row 55.
- Recursion depth is at most `log2(m) + 1` for the extrema, `2 log2(m) + 2` for the first hit and `O(log(m))` for `floorSumMoments`.
- `OLD/5-Mathematics/14-miscellaneous.hpp` stays in `OLD`; there was no `97-Legacy` excerpt.

## History

- 2026-10-18: P060 initial implementation; `@reviewer` found 8 package issues, all fixed; full suite passed on g++ and g++-14, stress seed 7 and integration passed.
