# 13-convolution.hpp — evidence

`13-convolution.hpp` is the standalone contest convolution family, independent of Core Poly. It has three groups:

- Ring-generic naive and Karatsuba products.
- A complex FFT and an NTT over any static NTT-friendly prime modint, sharing one radix-2 kernel pair.
- Exact products by three-prime CRT, a block product beyond the transform length, a dispatcher, and wrappers: truncated, cross-correlation, middle product, cyclic, negacyclic, tensor and 2-D.

It depends on Core `01-template.hpp` and `05-modint.hpp` only. There is no Barrett/Montgomery use and no ISA-specific code.

## Contracts

Common rules:

- Convolution results have length `n + m - 1`, and an empty operand gives an empty result. Vectors are taken by `const &` unless a copy is needed anyway. Inputs and outputs never alias, because every function returns a new vector, except the in-place transforms.
- "NTT prime" means a modint type `M` with a `constexpr` static `M::mod()` that is prime. `RANK = countr_zero(mod - 1)` bounds every transform length by `2^RANK`.
- The dispatcher and the fast paths of the wrappers use `nttPrime<M>()`: a Core static prime modint with `RANK >= 20`. Every other modint (non-NTT primes, composite or dynamic moduli) goes through `convolutionArbitraryMod`.

### primitiveRootNtt

`constexpr ulng primitiveRootNtt(ulng p)` asserts `p >= 2` and returns the smallest primitive root of a prime `p`.

- It factors the odd part `c` of `p - 1` by trial division in `O(sqrt(c))`, so it is fast for every NTT prime, where `c` is small. It then tests candidates `g = 2, 3, ...` with `O(log(c))` exponentiations each.
- It returns 1 for `p = 2`.
- It returns 0 when it detects a composite `p`, through a failed Fermat test `g^(p-1) != 1`. Every composite does fail eventually: no unit has order `p - 1`, and the smallest prime factor is a non-unit candidate. So the loop ends after at most `spf(p)` candidates.
- Used at compile time as `convolution_detail::ROOT<M>`.

### nttRootTable

`nttRootTable<M>(n, inverse = false)` returns a `const` reference to a static per-type, per-direction table of size `bit_ceil(n)`.

- Entry `h + j` (for a power of two `h` and `j < h`) is `w^j`, where `w = g^((p-1)/(2h))` is a primitive `2h`-th root, or `w^-1` for the inverse table.
- The precondition is `bit_ceil(n) <= 2^RANK`, asserted.
- The table grows on demand in `O(bit_ceil(n))` exact modular multiplications and is never shrunk.
- A call that grows the table invalidates references returned earlier for the same type and direction. The transforms take the reference once and do not call it again while using it.
- Single-threaded use only.

### ntt, intt, transposedNtt

These are in place on a power-of-two length `n` that divides `2^RANK`, all asserted.

- `ntt` is a decimation-in-frequency radix-2 transform. With `w` a primitive `n`-th root built from `primitiveRootNtt`, it leaves `A(w^k)` in slot `rev(k)`: the spectrum is bit-reversed (the ACL `butterfly` convention).
- `intt` takes a bit-reversed spectrum, runs the transposed decimation-in-time network with inverse roots, and multiplies by `n^-1`, so `intt(ntt(a)) = a`.
- `transposedNtt` is the exact matrix transpose of `ntt`, with no scaling. Since `ntt = P F` with `F` symmetric, it equals `F P`: it reads a bit-reversed input and writes natural order.
- Each is `O(n log(n))` modular operations.

### nttDoubling

`a` is the `ntt` of a length-`n` polynomial, and `2n` must divide `2^RANK`. It becomes the length-`2n` `ntt` of the same zero-padded polynomial.

- The first half is unchanged: the even indices of a bit-reversed length-`2n` spectrum are the length-`n` spectrum.
- The second half is `ntt(intt(a) * z^j)` with `z` a primitive `2n`-th root.
- Cost: one `intt` and one `ntt` of length `n`.

### fft, ifft

These are in place on `vector<complex<double>>` of power-of-two length (asserted).

- `fft` computes `X[k] = sum x[j] exp(-2 pi i j k / n)` into bit-reversed slots.
- `ifft` inverts it, taking bit-reversed input, and scales by `1/n`.
- Roots come from a static table per direction, built in `long double` by halving (KACTL), so each root has error near `long double` precision. Growth invalidates earlier references, as for the NTT table.

### convolutionNaive, convolutionKaratsuba

Both work over any commutative ring `T` with `T()` equal to zero and with `+=`, `-` and `*`. Wrapping unsigned words are fine; signed overflow is the caller's domain.

- Naive is `O(n m)`.
- Karatsuba splits the longer operand at `h = ceil(n/2)`. When the shorter operand fits in one half it splits only the longer one; otherwise it uses three recursive products of the halves. That gives `O(n * m^(log2(3) - 1))` for `n >= m`. Below 32 elements on the short side it switches to the naive loop.
- The recursion allocates `O(n + m)` per level.

### convolutionFft

This is a real product of `vector<double>` using the two-for-one trick: pack `a + i b`, one forward FFT, then `C_k = (X_k^2 - conj(X_{-k})^2) / (4i)`, then one inverse FFT.

- In bit-reversed order the slot of `-k` is `i ^ (bit_floor(i) - 1)`: the mirror inside each power-of-two block.
- It returns doubles, not rounded.
- For integer inputs, rounding with `llround` is exact when `(sum a_i^2 + sum b_i^2) * log2(L) < 9e14`, where `L = bit_ceil(n + m - 1)`. This is KACTL's empirical bound; tested at 8.0e14 and 8.6e14, with worst error 0.0098.

### convolutionNtt

`M` is an NTT prime with `n + m - 1 <= 2^RANK`, asserted inside `ntt`.

- When `min(n, m) <= 60` it uses the naive product (measured cutoff, see Benchmarks).
- When `&a == &b` it does one forward transform and squares pointwise.
- Otherwise it costs two forward transforms, a pointwise product and one `intt` of length `bit_ceil(n + m - 1)`.

### convolutionLong, convolutionU128, convolutionArbitraryMod

All three run three NTT products modulo P1 = 754974721 (`45 * 2^24 + 1`), P2 = 167772161 (`5 * 2^25 + 1`) and P3 = 469762049 (`7 * 2^26 + 1`). The length limit is `n + m - 1 <= 2^24`. The private `garner` combines the residues into mixed-radix digits `c = d0 + P1 d1 + P1 P2 d2`, with `0 <= c < P = P1 P2 P3`, about `5.95e25` or `2^85.6`.

- `convolutionLong`: signed 64-bit inputs; every true coefficient must lie in `[-2^63, 2^63)` (unchecked). Since `P > 2^64`, the residue `x` in `[0, P)` is the true value when `x < 2^63`, and `x - P` otherwise. The result is exact even for `INT64_MIN`, unlike a floating split.
- `convolutionU128`: unsigned 64-bit inputs; every true coefficient must be below `P` (unchecked). A sufficient condition is `min(n, m) * max(a) * max(b) < P`. It returns the exact `ulll` value.
- `convolutionArbitraryMod<M>`: any modint (static, dynamic, 32- or 64-bit modulus) using `val()`. When `min(n, m) <= 60` it is naive, with no bound. Otherwise it asserts `min(n, m) * (mod - 1)^2 < P`, so the true coefficients of the residues stay below `P`. The digits are combined directly in `M` (`d0 + P1 d1 + (P1 P2 mod m) d2`), so no 128-bit division is needed. The assert costs O(1).

### convolutionLarge

`M` is an NTT prime with `RANK >= 1` (`static_assert`). Let `L = 2^min(RANK, 30)` and `B = L/2`. When `n + m - 1 <= L` it is `convolutionNtt`.

Otherwise:

1. Each operand is cut into blocks of `B` coefficients, and each block is transformed at length `L`. A block product has length at most `2B - 1 < L`, so it never wraps.
2. For every output block `k`, the pointwise sums of `A_i B_{k-i}` are accumulated and inverted once, then overlap-added at offset `kB`.

The cost is `O(nm / B + (n + m) log(B))`. For 998244353, `B = 2^22`, and a `2^24 x 2^24` product needs 8 forward and 7 inverse transforms of length `2^23`. Peak memory is about `(n/B + m/B + 1) L` words plus the result. The result length `n + m - 1` must fit in `int`.

### convolution

The dispatcher works over Core modints:

- `convolutionLarge`, and through it `convolutionNtt`, when `nttPrime<M>()` (static prime, `RANK >= 20`).
- `convolutionArbitraryMod` otherwise: 1e9+7, small-rank primes, composite and dynamic moduli. That path keeps its own domain.

The choice is made at compile time.

### truncatedConvolution, crossCorrelation, middleProduct

- `truncatedConvolution(a, b, k)` asserts `k >= 0`. It returns the first `k` coefficients of `a * b`, zero-padded when `k > n + m - 1`, computing only from the first `k` entries of each operand.
- `crossCorrelation(a, b)` returns all `n + m - 1` lags: `res[k] = sum_j a[k - (m-1) + j] * b[j]` over valid `j`. It equals `convolution(a, reverse(b))`.
- `middleProduct(a, b)` asserts `m >= 1`. It returns coefficients `[m - 1, n)` of `a * b`: `res[i] = sum_j a[i + m - 1 - j] * b[j]`, which is empty when `m > n`.
  - For an NTT prime and `m > 60` it uses one cyclic product of length `bit_ceil(n)`. Wrap-around reaches only indices below `m - 1`, because `n + m - 2 - L < m - 1` when `L >= n`.
  - For small `m` it is a direct `O((n - m + 1) m)` loop.
  - Otherwise it takes a slice of the full product.

### cyclicConvolution, negacyclicConvolution

Both require `|a| = |b| = n`, asserted; `b` is resized defensively. They compute the product modulo `x^n - 1` and `x^n + 1` respectively.

- Cyclic: for an NTT prime and a power-of-two `n <= 2^RANK` it is one length-`n` transform pair. Otherwise it folds the linear product.
- Negacyclic: for an NTT prime and a power-of-two `n` with `2n <= 2^RANK` it twists by powers of a primitive `2n`-th root `z`, substituting `x = z y`, which turns `x^n + 1` into `y^n - 1`. It then runs the cyclic product and untwists. Otherwise it folds the linear product with a sign.
- `n = 0` gives an empty result.

### convolutionTensor, convolution2d

`convolutionTensor(a, da, b, db)` multiplies row-major multi-dimensional arrays of equal rank `d`.

- Preconditions (asserted): `|a| = prod(da)`, `|b| = prod(db)`, nonnegative extents, and result size `R = prod(da_i + db_i - 1) < 2^31`. Under `NDEBUG` a rank mismatch uses the common prefix of dimensions, and negative or oversized extents return empty.
- It returns the row-major result of shape `da_i + db_i - 1`, or empty when either operand is empty.
- Kronecker substitution places both operands at the result's strides. Per dimension the indices add without carry, so one 1-D `convolution` of total length `R` suffices.
- Rank 0 is the scalar product.
- `convolution2d` flattens rectangular `vector<vector<M>>` operands (raggedness is asserted) and calls the tensor form with rank 2. It is empty when any dimension is 0.

## Feature-to-test map

Entry: [`13-convolution_tester.py`](<../../96-Local Testing/05-Mathematics/13-convolution_tester.py>), with C++ cases in `13-convolution_tester.cpp`.

Oracles:

- The tester's own naive products over the same ring.
- Brute bit-reversed DFTs, using a primitive root found independently from a hand-supplied factorization of `p - 1` and checked in Python.
- A long-double DFT.
- `__int128` brute force.
- Sampled brute coefficients for large sizes: endpoints, middle, operand boundaries and 40 random indices.
- Python big integers.

| Feature | Oracle and edge classes |
|---|---|
| `primitiveRootNtt` | Order-counting brute for every `p <= 2000` (quick) or `6000`, with composites returning 0. Carmichael numbers 561, 1105, 41041, `2^32 + 1` and a 60-bit semiprime return 0. Values for 1e9+7 (5), `2^61 - 1` (37), 998244353 and 754974721 (`static_assert`). |
| `nttRootTable` | Every entry of the forward and inverse tables against `g^((p-1)/(2h) j)`, with primitivity checked by `z^h = -1`, at lengths 1, 2, 4, 8, `2^lgBrute` and the largest tested length. Growth goes from small to large. |
| `ntt`, `intt` | Brute bit-reversed DFT for every length `2^0 .. 2^9` (998244353) and smaller ranges for 12289, 13, 7340033, 754974721 and the 64-bit prime `29 * 2^57 + 1`. Round trips at the brute lengths and at the largest length: `2^16` checked, `2^20` full, `2^23` stress. |
| `transposedNtt` | Brute `F P` at every brute length, and the duality `<ntt(x), y> = <x, transposedNtt(y)>` at large lengths. |
| `nttDoubling` | Brute length-`2n` DFT for every brute length below the rank, and agreement with a zero-padded `ntt` at large lengths. |
| `fft`, `ifft` | Long-double DFT (error at most `1e-9 * n * max abs(x)`) for lengths `2^0 .. 2^9`; round trips at `2^0 .. 2^9` and `2^18`. |
| `convolutionFft` | Exact integer brute for sizes 0..200 with values up to `10^5`. Rounding near the stated bound at `2^12` (quick) and `2^16`, with alternating and all-positive signs. |
| `convolutionNaive`, `convolutionKaratsuba` | Brute over wrapping `ulng` and 1e9+7 for all sizes around the base case (31, 32, 33, 64, 65, ...) and unbalanced pairs up to `257 x 1000`. Sampled `20000 x 15001`. A double product. |
| `convolutionNtt` | Brute for `n` from 0 to 70 against `m` in {0, 1, 2, 59, 60, 61, 62, 100}, both orders, so it straddles the naive cutoff. Squaring via `&a == &b`. Sampled at exactly `n + m - 1 = 2^RANK` (or the tested maximum) for six primes. |
| `convolutionLong` | `__int128` brute for 200 random size pairs and magnitudes at the int64 edge. Extremes: `INT64_MIN * 1`, `INT64_MAX + INT64_MIN`, `-1 * (INT64_MIN + 1)`, an empty operand. Sampled at `n + m - 1 = 2^21` (full) and `2^24` (stress, the limit). Python big integers, signed. |
| `convolutionU128` | `__int128` brute; values just below `P` (`(2^42 - 1) * floor((P - 1) / (2^42 - 1))`); Python big integers up to `P`. |
| `convolutionArbitraryMod` | Brute for moduli 1e9+7 (static); 1, 2, 998244353, `2^31 - 1`, 4294967291 (dynamic); `10^12 + 39` (dynamic 64-bit, naive range); sizes across the cutoff. Sampled `300000 x 200000`. Python oracle for random moduli up to `2^62` inside the bound. |
| `convolutionLarge` | Brute on the block path with transform lengths 2 (p = 3), 4 (p = 13) and 4096 (p = 12289), including uneven and one-element blocks. Sampled 7340033 beyond `2^20`. A sampled `2^24 x 2^24 - 3` mint product in stress (optimized build). |
| `convolution` | Brute through the dispatcher for 998244353 (NTT), 1e9+7 (CRT), a dynamic modulus and the 64-bit NTT prime; sampled large. |
| `truncatedConvolution` | Brute prefix for `k` in {0, 1, `n + m - 1`, `n + m + 3`, random}, so it covers zero padding and empty operands. |
| `crossCorrelation` | Direct lag sum for all lags, including empty operands. |
| `middleProduct` | Brute slice for `m` across the 60 cutoff and `m > n` (empty), on the NTT fast path and the non-NTT paths; sampled at large sizes. |
| `cyclicConvolution`, `negacyclicConvolution` | Folded brute for `n` in {1, 2, 3, 5, 8, 16, 61, 64, 100, 128, 1024}, which covers the power-of-two transform and twist paths and the fold paths; `n = 0`; all four moduli. |
| `convolutionTensor` | Multi-index brute for ranks 0 to 3 with random extents 0..4, including zero extents. |
| `convolution2d` | Brute double sum for random shapes including empty ones; `40 x 90` by `30 x 70` shape. |
| Preconditions | 18 checked-build probes: `p = 1`; root table and `ntt` beyond the rank; non-power-of-two `ntt`, `intt`, `transposedNtt`, `fft`, `ifft`; doubling beyond the rank; the arbitrary-mod bound violated; `middleProduct` with `m = 0`; cyclic and negacyclic size mismatch; negative `k`; tensor rank mismatch, size mismatch and negative extent; ragged 2-D input. |

## Commands and results

P055 run (2026-10-09), GCC 16.2 and GCC 14.4.1, configurations optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan+UBSan `-O1`.

```bash
python3 '96-Local Testing/05-Mathematics/13-convolution_tester.py' --mode quick --seed 1           # PASS, 2 configurations, 18 probes: 50,693 C++ checks, 180 Python cases each
python3 '96-Local Testing/05-Mathematics/13-convolution_tester.py' --mode full --seed 1            # PASS, 3 configurations: 2,222,684 (optimized) / 256,597 (checked, ASan) C++ checks, 1,800 Python cases each; MEMORY peak=863MB
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/13-convolution_tester.py' --mode full --seed 1 # PASS, 3 configurations: same counts as full; MEMORY peak=869MB
python3 '96-Local Testing/05-Mathematics/13-convolution_tester.py' --mode stress --seed 2          # PASS, 3 configurations: 9,562,764 (optimized) / 256,597 C++ checks, 9,000 Python cases each; MEMORY peak=1057MB
```

- Quick and full pre-flight peaks were 813 MB under the 2048 MB cap. Most of that is the compiler on the `_GLIBCXX_DEBUG` build.
- What each mode adds:
  - Full adds the 6000-prime root sweep, transform lengths up to `2^20` (optimized) or `2^16` (checked builds), and `2^21`-length CRT runs.
  - Stress (optimized build) reaches `2^23` transforms, the `2^24` CRT limit and a `2^25` mint product. The checked builds repeat the full sizes with a new seed.
- `convolutionFft` worst rounding error at the bound: 0.0078 to 0.0156.
- The header builds alone, with `05-Mathematics/99-all.hpp`, and with every folder's `99-all.hpp` in two translation units, with no warnings under `-Wall -Wextra -Wconversion`. The only aggregate warning is pre-existing, in `06-segmentedsieve.hpp`.

`@reviewer` (2026-10-09) found two defects in this header, both fixed:

- `convolutionLarge` never ended for `RANK = 0` (`ModInt<2>`): `B = 0` made the block loop stall. It is now rejected by `static_assert`; the dispatcher never selects it for such types.
- `nttDoubling` divided by zero on an empty vector under `NDEBUG`, and `convolutionTensor` read `db` out of bounds on a rank mismatch under `NDEBUG`. Both are now bounded.

The minor suggestions were adopted: lowercase locals, the precise `primitiveRootNtt` cost line, the "one forward transform" wording, and an empty result when the tensor size exceeds the cap. Every reviewer reproducer was rerun with `-O1 -DNDEBUG -fsanitize=address,undefined`; all terminate cleanly.

## Benchmarks

`python3 '96-Local Testing/05-Mathematics/13-convolution_benchmark.py' 5`. Conditions:

- CPU: Intel i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2` (no `-march`).
- One warmup and 5 timed repetitions; the table shows medians.
- Outputs of the two compared implementations are checked equal.
- Seed 20261009, uniform random inputs.
- The machine was shared with a concurrent test run, so treat single numbers as ±15%.

| Workload | A | B |
|---|---|---|
| mint `2^16 x s`, s = 16 / 32 / 48 / 60 / 64 / 96 / 128 | naive 1.47 / 3.26 / 4.44 / 6.35 / 7.00 / 8.11 / 13.55 ms | NTT path 6.98 / 7.24 / 6.80 / 8.12 / 8.38 / 6.78 / 8.27 ms |
| `ulng s x s`, s = 16 / 32 / 64 / 128 / 256 / 1024 (4096 / s repetitions) | naive 0.05 / 0.08 / 0.20 / 0.36 / 0.68 / 2.63 ms | Karatsuba 0.05 / 0.09 / 0.15 / 0.23 / 0.36 / 0.84 ms |
| mint `2^19 x 2^19` | `convolutionNtt` 72.3 ms | `convolution` 75.2 ms |
| 1e9+7 `2^19 x 2^19` | `convolutionArbitraryMod` 254 ms | `convolution` 264 ms |
| int `2^19 x 2^19`, values in [-1000, 1000] | `convolutionLong` 270 ms | `convolutionFft` (rounded) 76.1 ms |
| `ulng 10^5 x 10^5` | Karatsuba 266 ms | |
| mint `2^24 x 2^24` | `convolutionLarge` 4312 ms | `convolution` 4337 ms |

Benchmark MEMORY peak was 848 MB.

Thresholds:

- **NAIVE = 60.** Naive beats the bare transform up to about 64 on the short side, with a 2^16 long side. At 96 the transform wins.
- **KARATSUBA = 32.** At 32 the two are level; Karatsuba gains from 64 on.

The radix-2 kernel is generic over `M`. A separate probe timed one length-`2^20` `ntt` at 20–22 ms, against 19–21 ms for the same loop on raw `uint` words. The modint wrapper therefore costs about 10%, and the rest is the `%` reduction. Radix-4 with lazy reduction on raw words (ACL) would trade genericity for a further constant. Core Poly owns that accelerated engine.

## Sources

[00-sources.md](00-sources.md), P055 sweep: AC Library `convolution.hpp` (CRT primes, naive cutoff, butterfly order), KACTL FFT/NTT (root halving, two-for-one product, rounding bound), Nyaan `ntt.hpp` and `arbitrary-ntt.hpp` (doubling, aliasing square, u128 product), maspypy `convolution.hpp`/`ntt_doubling.hpp` (dispatcher, doubling), Library Checker convolution problems. Written independently.

## Limits and handoffs

- `fft`/`ntt` spectra are bit-reversed. A natural-order DFT is a bit-reversal permutation away and is not provided.
- Karatsuba and the naive product need a commutative ring: the swap of operands reorders products.
- `convolutionFft` returns unrounded doubles. Exactness beyond the KACTL bound is not claimed. Exact integer products use `convolutionLong`/`convolutionU128`.
- `convolutionLarge` memory is about `(n/B + m/B + 1) * 2B` words of `M` plus the result; with 998244353 and `2^24 x 2^24` that is about 550 MB.
- Root tables are static and grow monotonically; they are not thread-safe.
- Rejected candidates are in [00-notes.md](00-notes.md), P055 research decisions. Core Poly owns FPS operations; rows 14, 52, 53 and 65 own product trees, relaxed, semiring and specialized convolutions.

## History

- 2026-10-09: P055 initial implementation and verification.
