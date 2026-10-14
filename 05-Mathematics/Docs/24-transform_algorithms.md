# 24-transform_algorithms.hpp — evidence

`24-transform_algorithms.hpp` covers scalar set and index transforms over any ring type `T`:

- Subset, superset and Walsh–Hadamard transforms on `2^n` arrays, with the bitwise convolutions they diagonalize.
- Divisor and multiple zeta/Möbius on index arrays `[1, n]`, with gcd and lcm convolutions.
- Ranked (popcount-graded) transforms with subset and q-operand disjoint-union convolution.
- Kronecker powers of any `D x D` kernel.
- The legacy `FastConv` gcd/lcm transforms in place, over a caller's `LinearSieve` or by harmonic loops.

It depends on `01-template.hpp` and `04-sieve_algorithms.hpp` (Eratosthenes primes, and `LinearSieve` for the legacy sieve overloads). Set power series (exp/log/composition) belong to row 66.

## Contracts

Common rules:

- `T` needs a zero `T()`, `+=`, `-=`, `*`, and, for inverse transforms, subtraction. `walshHadamard`'s inverse also needs division (below).
- Transforms work in place. Convolutions take their operands by value and return a new vector, so aliasing (`xorConvolution(a, a)`) is safe.
- No state, no caches.

### subsetZeta, subsetMobius, supersetZeta, supersetMobius

`N = a.size()` must be a power of two (asserted; `N >= 1`).

- `subsetZeta`: `a[S] = sum over T ⊆ S of a[T]`.
- `supersetZeta`: the same sum over `T ⊇ S`.
- The Möbius forms are the exact inverses.

Each is `O(N log(N))` additions, done per bit with a block loop. With the assertion removed, a non-power-of-two size only processes the complete blocks: bounded, meaningless.

### walshHadamard

`a[S] = sum over T of (-1)^|S ∩ T| a[T]` on a power-of-two `N`. With `inverse = true` the butterflies are followed by a division by `N`:

- For integral `T` (including `__int128`) it uses exact integer division in the signed type `make_signed_t<T>`. This is valid when the input is a Walsh–Hadamard image of integers, because every such entry is `N` times an integer. Unsigned words are read as two's complement, so a wrapped negative value such as `2^64 - 1` comes back as -1. The result is exact when `N` times every result entry fits the signed range.
- Otherwise it multiplies by the one inverse `T(1) / T(N)`. For a modint this needs `N` to be a unit, so the modulus must be odd.

This fixes the legacy `fctXor`, which multiplied by `T(1) / T(n)`. That is 0 for integral `T`, so its integer inverse returned all zeros.

### kroneckerPowerTransform

`a` has size `D^k` and `K` is a `D x D` kernel; both are asserted, with `D >= 1`. The function applies `K` tensored `k` times, where digit `t` of an index in base `D` is acted on by `K` at stride `D^t`.

- Cost: `O(N * D * k)` multiplications with an `O(D)` buffer.
- Subset zeta is `{{1,0},{1,1}}`, Hadamard is `{{1,1},{1,-1}}`, and a k-ary max/min zeta is a triangular `D x D` kernel.
- `D = 1` requires `N = 1`.
- A size that is not a power of `D` processes only complete blocks before the final assertion. A ragged kernel skips the transform; both are bounded under `NDEBUG`.

### orConvolution, andConvolution, xorConvolution

Equal power-of-two sizes are asserted; `b` is resized to `|a|` so that a violated precondition stays in bounds under `NDEBUG`. Each computes `res[S] = sum over T op U = S of a[T] b[U]` by transform, pointwise product and inverse, in `O(N log(N))`. `xorConvolution` inherits `walshHadamard`'s inverse rule: an exact integer, or an odd modulus.

### divisorZeta, divisorMobius, multipleZeta, multipleMobius

These act on index array `a[0..n]`, with `n = a.size() - 1`, in place on `[1, n]`; `a[0]` is never touched and an empty vector is a no-op.

- `divisorZeta`: `a[k] = sum over d | k of a[d]`.
- `multipleZeta`: `a[k] = sum over k | m, m <= n, of a[m]`.
- The Möbius forms invert them.

They take one pass per prime `p <= n`, an Euler-product factorization of the divisor lattice, giving `O(n log(log(n)))` with primes from a fresh Eratosthenes sieve. The divisor direction ascends in `i` for zeta and descends for Möbius; the multiple direction is the reverse.

### gcdConvolution, lcmConvolution

Indices start at 1, index 0 is ignored, and `res[0] = 0`.

- `gcdConvolution` returns size `min(|a|, |b|)` with `res[k] = sum over gcd(i, j) = k of a[i] b[j]`. Both operands are multiple-zeta'd at their own lengths and then truncated, because a long operand still contributes through common divisors below the short length.
- `lcmConvolution` returns size `max(|a|, |b|)` with `res[k] = sum over lcm(i, j) = k of a[i] b[j]`. Pairs whose lcm is `max(|a|, |b|)` or more are dropped (truncation).

Both cost `O(n log(log(n)))`.

### rankedZeta, rankedMobius

`rankedZeta(a)` returns `z[k][S] = sum over T ⊆ S with |T| = k of a[T]` for `k = 0..n` (`N = 2^n` asserted). The layout is `[rank][set]`, `n + 1` arrays of `N`, so every transform and the rank product run over contiguous memory.

`rankedMobius(z)` asserts `z.size() = n + 1`. It inverts each rank and returns `res[S] = z[|S|][S]`. Under `NDEBUG` it pads or truncates `z` to `n + 1` ranks of size `N`. The rank product returns its left operand unchanged on a shape mismatch, which keeps a violated size precondition in bounds.

Both are `O(n^2 N)` time and `O(n N)` memory.

### subsetConvolution, disjointUnionConvolution

- `subsetConvolution(a, b)`: equal power-of-two sizes asserted. It returns `res[S] = sum over T ⊆ S of a[T] b[S \ T]` in `O(n^2 N)` time and `O(n N)` memory.
- `disjointUnionConvolution(a)`: `q >= 1` operands of equal size (asserted). It returns `res[S] = sum over ordered partitions S = T1 ⊔ ... ⊔ Tq of prod a_i[T_i]` with one ranked Möbius, in `O(q n^2 N)`. With `q = 1` it returns the operand and with `q = 2` it equals `subsetConvolution`.

The rank product overwrites `f[k] = sum over i of f[i] g[k - i]` in descending `k`, so no third buffer is needed. Peak memory for the binary product is two ranked arrays, for example 2 x 21 x 2^20 x 4 bytes = 176 MB for `n = 20` with a 4-byte modint.

### FastConv

These are the legacy static in-place methods with their original names and signatures:

- `fctGcd`/`fctLcm` are the multiple/divisor transforms using the primes of a caller's `LinearSieve`, asserted to cover `a.size() - 1`.
- `fctGcdSlow`/`fctLcmSlow` are the sieve-free harmonic loops in `O(n log(n))`.

The legacy forms had no assertion on the sieve size; a smaller sieve silently gave wrong answers.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `FastConv<T>::fctOr(a, inv)` | `subsetZeta(a)`, or `subsetMobius(a)` when `inv` |
| `FastConv<T>::fctAnd(a, inv)` | `supersetZeta(a)`, or `supersetMobius(a)` when `inv` |
| `FastConv<T>::fctXor(a, inv)` | `walshHadamard(a, inv)` |

## Feature-to-test map

Entry: [`24-transform_algorithms_tester.py`](<../../96-Local Testing/05-Mathematics/24-transform_algorithms_tester.py>), with C++ cases in `24-transform_algorithms_tester.cpp`.

Oracles:

- Direct `O(N^2)` definitions: subset tests by `(s & t) == t` and Hadamard signs by popcount parity.
- `O(3^n)` submask enumeration.
- A dense Kronecker-power matrix.
- Divisor scans.
- Pairwise gcd/lcm.

The types are `lng` (values ±30), 998244353 and 1e9+7, plus double, `__int128` and wrapping `ulng` spot checks.

| Feature | Oracle and edge classes |
|---|---|
| `subsetZeta`, `subsetMobius`, `supersetZeta`, `supersetMobius` | Definitions for every `N = 2^0 .. 2^6/2^9/2^10` (quick/full/stress); Möbius of a brute image; round trips; wrapping `ulng` round trip at `2^10`. |
| `walshHadamard` | Definition (default and explicit `inverse = false`) and exact inverse over `lng` and both modints; round trips over double (tolerance `1e-12`), `__int128` values near `2^100` and wrapping `ulng` and `uint` (two's-complement inverse). These and the zeta/Möbius checks carry the removed `fctOr`, `fctAnd` and `fctXor` cases. |
| `orConvolution`, `andConvolution`, `xorConvolution` | `O(N^2)` pairwise brute at every tested size, `xorConvolution(a, a)` aliasing; at `2^16` (full) or `2^20` (stress) the subset-sum identity `zeta(or)[S] = zeta(a)[S] zeta(b)[S]` on 12 masks including 0 and full, and the total-sum identity for xor; `xorConvolution` over wrapping `ulng` with a negative result. |
| `divisorZeta`, `divisorMobius`, `multipleZeta`, `multipleMobius` | Divisor-scan brute for every `n` from -1 (empty) to 40/150/300, round trips at `10^6` (full) or `3 * 10^6` (stress), and spot checks at `k` = 1, 2, n, n - 1, n/2. |
| `gcdConvolution`, `lcmConvolution` | Pairwise brute for every `n` above against six `m` values, including 0, 1, equal, longer and `3n + 2` (unequal sizes, truncation); `10^6` spot checks at four `k`. |
| `rankedZeta`, `rankedMobius` | The definition per rank and set; round trip. |
| `subsetConvolution` | Submask brute at every tested size; at `2^16`/`2^20` submask brute on 12 masks with up to 14 bits. |
| `disjointUnionConvolution` | `q = 1` identity, `q = 2` and `q = 3` against iterated submask brute. |
| `kroneckerPowerTransform` | Dense Kronecker power for `D = 1..4` and every `k` up to `9 / D` (`D = 1` with `N = 1`), random kernels; the subset and Hadamard kernels against their brute forms. |
| `FastConv` (`fctGcd`, `fctLcm`, `fctGcdSlow`, `fctLcmSlow`) | The divisor-scan definitions and round trips for every `n` above; `fctGcd`/`fctLcm` with an exactly fitting `LinearSieve`, including `n = 0`. |
| Preconditions | 14 checked-build probes: non-power-of-two and empty sizes; size mismatches in or/xor/subset/disjoint-union; non-square, empty and non-power kernels; ranked size and rank-count mismatch; no operands; a sieve smaller than the array for `fctGcd` and `fctLcm`. |

## Commands and results

P055 prune run, 2026-10-13: GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H. Configurations as in [13-convolution.md](13-convolution.md#commands-and-results).

```bash
python3 '96-Local Testing/05-Mathematics/24-transform_algorithms_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/24-transform_algorithms_tester.py' --mode full --seed 20261013
g++ -std=gnu++20 -Wall -Wextra -Wshadow -Wconversion -pedantic <configuration flags> -c '96-Local Testing/05-Mathematics/24-transform_algorithms_tester.cpp' -o /dev/null  # each configuration, g++ and g++-14, memory-capped
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '05-Mathematics/24-transform_algorithms.hpp' '96-Local Testing/05-Mathematics/24-transform_algorithms_tester.cpp'
```

Full, seed 20261013: PASS in all three configurations with all 14 assertion probes, 28,167 checks per configuration; peak 501 MB on g++ and 547 MB on g++-14 (quick pre-flights 482 MB and 546 MB). The warning build reports no warnings in any configuration on either compiler. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations.

## Benchmarks

Measured with the row 13 driver; conditions in [13-convolution.md](13-convolution.md).

| Workload | Median |
|---|---|
| `subsetConvolution`, n = 20, mint | 1126 ms (`disjointUnionConvolution` with q = 2: 1128 ms) |
| `xorConvolution`, n = 20, mint | 51.7 ms |
| `gcdConvolution`, n = 10^6, mint | 23.6 ms |

The subset product runs every `(k, i)` rank pair over all sets, about `n^2 N / 2` multiply-adds. A popcount-restricted product only pays off for the final factor (chained products need every rank), so it is not used.

## Sources

[00-sources.md](00-sources.md), P055 sweep: Library Checker set-power-series problems, maspypy `ranked_zeta`/`nt/zeta`, hitonanode zeta/Möbius and subset convolution, suisen `kronecker_power`, noshi91 transforms, OI Wiki FWT. Written independently. Legacy: `OLD/algorithms.cpp:3823-3933` (`FastConv`); its local excerpt was removed once this row was verified. The four gcd/lcm names keep their signatures and the sieve size is now asserted; `fctOr`, `fctAnd` and `fctXor` map to the transforms in the Legacy mapping, with the `fctXor` integer inverse fixed in `walshHadamard`.

Adopted by the P055 sweep: `kroneckerPowerTransform` widened from a 2 x 2 to any D x D kernel, and `disjointUnionConvolution` defined as the q-operand ranked product.

## Limits and handoffs

- Index transforms ignore index 0. Callers needing `gcd(0, x) = x` handle 0 separately.
- Möbius transforms and inverses need additive inverses. Max/min (semigroup) zeta is a call-site loop or a Kronecker kernel over a semiring type; see the rejected candidates in [00-notes.md](00-notes.md).
- Set power series (exp, log, composition, power projection) are row 66. Online subset transforms were handed off there or to row 52.

## History

- 2026-10-09: P055 initial implementation and verification, legacy `FastConv` migrated to wrappers, `@reviewer` found 2 defects, fixed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `FastConv::fctOr`, `fctAnd` and `fctXor` (cases already covered by the zeta/Möbius and Walsh–Hadamard checks; explicit `inverse = false` forward check added); full suite passed on g++ and g++-14.
