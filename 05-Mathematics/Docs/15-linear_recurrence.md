# 15-linear_recurrence.hpp — evidence

`15-linear_recurrence.hpp` is the contest linear-recurrence family. It covers:

- Finding a minimal recurrence: Berlekamp–Massey and its half-GCD (Padé) form.
- Evaluating terms: Kitamasa in any commutative semiring, Fiduccia, Bostan–Mori, consecutive terms, prefix sums and slices of rational series.
- Interpolating recurrent, periodic and polynomial-times-exponential sequences.
- `A^k v` through a Krylov annihilator.

The functions are free and generic over a Core modint `M`, except the semiring and periodic ones, which are generic over any value type. The header depends on `13-convolution.hpp` and `14-polynomial_algorithms.hpp`. It uses no Barrett/Montgomery and no ISA-specific code.

## Contracts

Common conventions:

- **Recurrence form.** A recurrence of order `d` is `c = (c_1, ..., c_d)`, read as `a_i = sum_{j=1}^{d} c_j a_{i-j}` for `i >= d`.
- **Initial terms.** Only `a_0 .. a_{d-1}` define the sequence: `|a| >= d` is asserted, and later entries are ignored.
- **Order zero.** It is the all-zero sequence, an empty sum.
- **Generating function.** It is `P / Q` with `Q = 1 - sum c_j x^j`.
- **Indices.** Every index `k` is unsigned 64-bit.

### findLinearRecurrence, berlekampMassey, findLinearRecurrenceHalfGcd

- **`findLinearRecurrence(s, c)`** is Berlekamp–Massey over any modint, `O(n^2)`.
  - It returns `false` when a discrepancy that must be inverted is not a unit. This can happen only over a composite modulus.
  - On `true`, `c` is a recurrence valid for every `L <= i < n`.
  - Over a field it has the minimal length; over a ring minimality is not claimed.
  - Composite-modulus synthesis (Reeds–Sloane) belongs to row 51.
- **`berlekampMassey(s)`** is the prime-field form; it asserts success.
- **`findLinearRecurrenceHalfGcd(s)`** works over a prime field in `O(n log^2 n)`.
  - Write `A = sum s_i x^(n-1-i)` (the reversed sequence). A recurrence of length `L` is a monic `Lambda` of degree `L` with `Lambda A = r mod x^n` and `deg r < L`.
  - The minimal one is the cofactor `t_j` at the first Euclidean step on `(x^n, A)` with `deg r_j < deg t_j` (Dornstetter / Sugiyama).
  - `polyHalfGcd` reaches the pair `(r_{j*-1}, r_{j*})` with `deg r_{j*} < ceil(n / 2)`. No earlier index qualifies, so the answer is `t_{j*}` or the next cofactor after one more division.
  - It returns the same length as Berlekamp–Massey. When the minimal recurrence is not unique (`L > n / 2`), the coefficients may differ.

### linearRecurrenceSemiring, kitamasa

- **`linearRecurrenceSemiring(c, a, k, add, mul, zero, one)`** computes `a_k` over a commutative semiring, `O(d^2 log k)`.
  - It computes `r = x^k mod (x^d - sum c_j x^(d-j))` by square-and-multiply-by-x.
  - The reduction replaces `x^i` (`i >= d`) by `sum_j c_j x^(i-j)` and only adds, so it is valid in a semiring. The linear functional `x^i -> a_i` is preserved by distributivity, and squaring needs commutativity.
  - The functors must make `(T, add, zero)` a commutative monoid and `mul` distribute over it, with `one` its identity. `zero` must absorb under `mul`, for example a saturating `+` with `INF` in min-plus.
  - Order 0 returns `zero`.
- **`kitamasa(c, a, k)`** is the modint instance (`+`, `*`): transform-free, any modint.

### fiduccia, bostanMori, nthTermLinearRecurrence, prefixSumLinearRecurrence, rationalGeneratingFunction

- **`fiduccia(c, k)`** returns `x^k mod (x^d - sum c_j x^(d-j))` as exactly `d` coefficients, through `polyPowMod`. Then `a_k = sum r_i a_i`. Any modint, `O(M(d) log k)`.
- **`bostanMori(P, Q, k)`** returns `[x^k] P / Q`, with `Q[0]` a unit (asserted). Any modint, `O(M(n) log k)`.
  - Each step multiplies by `Q(-x)` and keeps the numerator half of `k`'s parity and the even half of the denominator.
  - When `k` drops below `max(|P|, |Q|)` it finishes with one series division. A numerator longer than `Q` is allowed.
- **`rationalGeneratingFunction(c, a)`** returns `(P, Q)` with `Q = 1 - sum c_j x^j` (exactly `d + 1` coefficients) and `P = (a_0 .. a_{d-1}) Q mod x^d` (exactly `d` coefficients, not trimmed).
- **`nthTermLinearRecurrence(c, a, k)`** is `bostanMori(P, Q, k)`.
- **`prefixSumLinearRecurrence(c, a, k)`** returns `sum_{i < k} a_i` as `[x^(k-1)] P / (Q (1 - x))`, and 0 for `k = 0`.
- A weighted sum `sum a_i w^i` is the same call with `c_j w^j` and `a_i w^i`. An inhomogeneous polynomial term is handled by multiplying `Q` by `(1 - x)^(deg + 1)` at the call site.

### sliceRationalFps, consecutiveTerms

- **`sliceRationalFps(P, Q, l, r)`** returns the coefficients `[l, r)` of `P / Q`, with `Q[0]` a unit and `l <= r` (asserted). Any modint.
  - After trimming, let `d = deg Q` and `e = max(|P| - d, 0)`. From index `e` on, the coefficients obey the order-`d` recurrence of `Q`.
  - For `l <= e + d` it divides series directly up to `r`.
  - Otherwise it computes `x^(l-e) mod rev(Q) / Q_0` with `polyPowMod`, takes the `d` terms at `l` as a middle product with the first `2d - 1` terms after `e`, and extends them by one division `(state Q mod x^d) / Q`.
  - `d = 0` (constant `Q`) gives the polynomial `P / Q_0`.
  - Cost `O(M(|P| + d) + M(d) log l + M(r - l + d))`.
- **`consecutiveTerms(c, a, k, m)`** returns `a_k .. a_{k+m-1}`, with `m >= 0` asserted. It is `sliceRationalFps` on the generating function.
- The two differ only in input form: a recurrence with its initial terms, or a rational function.

### interpolateLinearRecurrence, interpolatePeriodicSequence, interpolatePolyExp

- **`interpolateLinearRecurrence(s, k)`** works over a prime field.
  - It returns `s_k` directly for `k < |s|`.
  - Otherwise it finds the minimal recurrence of the samples, with Berlekamp–Massey up to `cut(BM)` samples and the half-GCD form above, then evaluates `nthTermLinearRecurrence`.
  - It is correct when the true sequence has order `L` with `2L <= |s|`.
- **`interpolatePeriodicSequence(s, b, k)`** works for any `T` with `==`.
  - Contract: the sequence has preperiod `mu <= b` and period `lambda <= b`, with `b >= 1` and `|s| >= 3b` (asserted).
  - It returns `s_k`, directly for `k < |s|`. Otherwise it finds the smallest period `p` of the window `s[b, |s|)` with the prefix function and returns `s[b + (k - b) mod p]`.
  - Correctness: by Fine and Wilf, the window has length `>= 2b >= lambda + p`, so `gcd(lambda, p)` is a period of a window that contains a full period of the tail. Since `lambda` is minimal, `lambda | p`, and `p <= lambda` forces `p = lambda`.
  - Cost `O(|s|)`.
- **`interpolatePolyExp(s, r, k)`** takes `s_i = f(i) r^i` with known `r` and `deg f < n = |s|`, so the degree bound is the sample count. It requires `1 <= n <= mod` and a prime field (asserted).
  - It returns `s_k`, directly for `k < n`. Otherwise it returns 0 when `r = 0`, and `f(k mod p) r^k` by `O(n)` Lagrange evaluation on the nodes `0 .. n - 1`.
  - An unknown `r` is the `interpolateLinearRecurrence` case, with `2n` samples. The prefix sums `sum f(i) r^i` are row 38.

### matrixPowerVector

`matrixPowerVector(A, v, k)` returns `A^k v` for a `d x d` matrix with `|A| = |v|` asserted, over a prime field.

- The Krylov vectors `v, A v, ...` are reduced incrementally until the first dependence. That dependence gives the monic annihilator `mu` of `v`, with `deg mu <= d`.
- Then `A^k v = sum r_i A^i v` for `r = x^k mod mu`.
- Cost `O(d^3 + M(d) log k)`, memory `O(d^2)`.
- `mu` divides the characteristic polynomial and is deterministic, so this refines the "characteristic polynomial plus Kitamasa" plan without needing row 44's Hessenberg reduction.

## Feature-to-test map

Entry: [`15-linear_recurrence_tester.py`](<../../96-Local Testing/05-Mathematics/15-linear_recurrence_tester.py>), with C++ cases in `15-linear_recurrence_tester.cpp`.

Oracles:

- Direct iteration.
- Companion-matrix powers: an augmented matrix for prefix sums and a min-plus matrix power for the semiring.
- Exhaustive minimal-length search.
- Naive series division.
- The polynomial-part plus recurrence decomposition of `P / Q`.
- Binary matrix powering.

| Operation | Tests |
|---|---|
| berlekampMassey, findLinearRecurrence, findLinearRecurrenceHalfGcd | `exhaustiveMinimal`: every sequence over GF(2) with `n <= 10` and over GF(3) with `n <= 6`, against the exhaustive minimal length. `findSuite`: random recurrences of order up to 1500 (half-GCD recursion), sparse, zero and one-hot sequences; equal lengths and validity |
| findLinearRecurrence (composite) | `compositeSuite` (mod 12): the `false` reproducer `[2, 1]`, Fibonacci, validity of every success |
| kitamasa, linearRecurrenceSemiring | `termSuite` (iteration, companion powers for 64-bit `k`), `semiringSuite`: min-plus with saturating `INF` against iteration and min-plus matrix powers, max-plus, boolean |
| fiduccia | `termSuite`: `sum r_i a_i = a_k`, and repeated multiply-by-x reduction for `d` up to 200 |
| bostanMori | `rationalSuite`: series division (numerator longer than `Q`, trailing zeros) and `bigRational` for 64-bit `k` |
| nthTermLinearRecurrence, prefixSumLinearRecurrence, consecutiveTerms | `termSuite`: iteration and companion powers including `k = 0, d - 1, d, d + 1`; order 0 |
| rationalGeneratingFunction | `termSuite`: sizes, `Q[0] = 1`, series equals the sequence |
| sliceRationalFps | `rationalSuite`: series division for small `l`, `bigRational` for 64-bit `l`, constant `Q`, zero numerator, empty slices |
| interpolateLinearRecurrence | `interpolationSuite`: orders 0 to 1500, both sides of `cut(BM)`, iteration and companion powers |
| interpolatePeriodicSequence | `periodicSuite`: every binary sequence with `mu, lambda <= b <= 4` (all queries `k < 60` and `2^62`), random strings over small alphabets |
| interpolatePolyExp | `interpolationSuite`: direct evaluation for `r = 0, 1`, random `r`, and `k` in `{0, n - 1, n, p, p + n / 2, random}`; `n = p = 7` samples (factorials only up to `(p - 1)!`) |
| matrixPowerVector | `matrixSuite`: random, nilpotent, identity, 0/1, block and rank-1 matrices, zero and one-hot `v`, `d` up to 90, over 998244353 and 5 |
| assertion probes | 13 checked-build probes |

## Commands and results

P057 prune run, 2026-10-13: GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`); this header is unchanged, and the run covers the `polyMod` removal in row 14, which it includes. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan+UBSan `-O1`.

```bash
python3 '96-Local Testing/05-Mathematics/15-linear_recurrence_tester.py' --mode full --seed 20261013             # PASS, 3 configurations, 13 probes: 137,440 / 135,712 / 135,712 checks; MEMORY peak=891MB
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/15-linear_recurrence_tester.py' --mode full --seed 20261013  # PASS, 3 configurations, 13 probes: 137,440 / 135,712 / 135,712 checks; MEMORY peak=890MB
python3 '96-Local Testing/02-integration.py'                                                                      # PASS: 135 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace
```

- Mode coverage:
  - Quick uses GF(2) up to `n = 7`, GF(3) up to `n = 4`, and reduced repetitions.
  - Full adds the exhaustive GF(2) `n <= 10` and GF(3) `n <= 6` searches, orders up to 1500, and the `cut(BM)` neighbours (the 1e9+7 ones in the optimized build).
  - Stress repeats every random loop four times in the optimized build. The checked builds rerun the full sizes with a new seed and stay below the 180 s limit.
- Test programs peak below 10 MB; the reported peaks are the compiler.

## Benchmarks

The shared driver `14-polynomial_algorithms_benchmark.py` covers both headers; the full table is in [14-polynomial_algorithms.md](14-polynomial_algorithms.md#benchmarks).

Headline timings, 998244353, with the same conditions as the 14 table:

| Workload | ms |
|---|---|
| `nthTermLinearRecurrence` d = 1e5, k = 1e18 | 1167 |
| `consecutiveTerms` d = 1e5, k = 1e18, m = 1e5 | 3691 |
| `berlekampMassey` n = 1e4 | 210 |
| `findLinearRecurrenceHalfGcd` n = 2e5 | 1545 |

- `bostanMori` uses four transforms per halving step for NTT primes. The spectrum of `Q(-x)` is the bit-reversed spectrum of `Q` with slots `i` and `i ^ 1` swapped. Before this change it used two full products (six transforms) and took 2094 ms on the same workload.
- `consecutiveTerms` and `sliceRationalFps` spend most of their time in `polyPowMod` (the state at `l`). A Graeffe-based slice could roughly halve that; see Limits.
- The `BM` cutoff row is in the 14 table.

## Sources

[00-sources.md](00-sources.md), P057 sweep: KACTL `BerlekampMassey.h`/`LinearRecurrence.h`, maspypy `seq/find_linear_rec.hpp` (half-GCD form), hitonanode `linear_recurrence.hpp` and `coeff_of_rational_function.hpp`, Nyaan `fps/nth-term.hpp`, OI Wiki linear recurrence (slice coefficients), Library Checker `find_linear_recurrence`, `kth_term_of_linearly_recurrent_sequence`, `consecutive_terms_of_linear_recurrent_sequence`. The code was written independently.

## Limits and handoffs

- Composite-modulus minimal recurrences (Reeds–Sloane) and P-recursive sequences are row 51. Black-box minimal polynomials and Wiedemann are row 44. Polynomial-times-exponential prefix sums are row 38.
- `matrixPowerVector` is cubic in `d`. Sparse or black-box matrices belong to row 44.
- The state at a large index `l` (`sliceRationalFps`, `consecutiveTerms`) comes from `polyPowMod` with a precomputed reversed inverse, which is optimal at `O(M(d) log l)`. A Graeffe-based slice (Bostan–Mori on `1 / Q`, OI Wiki "slice coefficients") would have a smaller constant and is the natural next optimization.
- Rejected candidates are in [00-notes.md](00-notes.md), under P057 research decisions.

## History

- 2026-10-10: P057 initial implementation and verification; `@reviewer` found that `interpolatePolyExp` inverted `p!` at `n = mod` (now factorials up to `(p - 1)!`, regression case `n = p = 7`); the row's `NDEBUG` bounds are in the 2026-10-10 line of [14-polynomial_algorithms.md](14-polynomial_algorithms.md#history).
- 2026-10-11: verification run, quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: P057 prune rerun (row 14 `polyMod` removed), header and tests unchanged; full suite passed on g++ and g++-14.
