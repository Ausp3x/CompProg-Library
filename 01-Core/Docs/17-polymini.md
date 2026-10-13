# 17-polymini.hpp — evidence

`17-polymini.hpp` provides `PolyMini<T>`, the independently copyable polynomial and formal-power-series mini of C12 (package P026). It is a documented reduction of [`16-poly.hpp`](16-poly.md): the operations it shares with `Poly<T>` return the same coefficients and the same sizes on the shared domain. It includes only `01-template.hpp` and depends on no sibling, full type or detail namespace; every helper is a member of the struct. It has 182 non-blank lines and 7 comment lines.

## Contracts

### PolyMini<T>

`PolyMini<T>` publicly derives from `vector<T>`. `v[i]` is the coefficient of `x^i`, `size()` counts stored terms (trailing zeros allowed), and every `vector` member (`size`, `operator[]`, `resize`, iterators, `==` on stored terms) is available. Constructors: default (empty, the zero polynomial), every `vector<T>` constructor (initializer list, `n` copies, iterator range), and an implicit conversion from `vector<T>`. Inputs are taken by value or `const &`, so aliasing an input with the assigned result (`a = a * a`, `a = divMod(a, a).first`) is safe. The struct holds no static state and no cache: every call is independent, so a dynamic modulus change (`DynModIntMini::setMod`) needs no invalidation beyond the residue type's own rule (old values are stale).

`T` is a prime-field residue: `T::mod()` returns the prime `p`, `T` is constructible from `int` and `ulng`, has `+ - * /`, the compound forms, unary `-` and `==`, and a `pow(T, e)` found through ADL that accepts `ulng` and negative `lng` exponents. All four types of `06-modintmini.hpp` and of `05-modint.hpp` qualify. Two domain conditions apply, both stated in the header's second comment line:

- Transforms. Let `N` be one more than the largest input length, precision `n` or point count of a call (`|c| + 1` for `linearRecurrenceKth`). Every transform the call makes has a power-of-two length at most `2 * bit_ceil(N)`, and that length must divide `p - 1` (asserted in `ntt`). For 998244353 (`2^23 | p - 1`) this allows `N <= 2^22`; for `29 * 2^57 + 1` with `ModInt64Mini` the limit is memory. `a * b` transforms only when both factors exceed 32 terms, and `findLinearRecurrence`, `eval`, `deriv`, `+`, `-` and scalar `*` never transform, so these also work over small fields such as GF(2). The full type instead switches to a block product beyond its transform limit (`convolutionLarge`); the mini narrows the domain.
- Characteristic. `integ(a)` and `taylorShift(a, c)` divide by `1, ..., |a|` and need `p > |a|`; `log`, `exp` and `pow` need `p > n`, which the transform condition already implies whenever they transform.

| Operation | Contract |
|---|---|
| `deg()`, `trim()` | Degree ignoring trailing zeros, `-1` for zero; `trim` drops trailing zeros in place and returns `*this`. |
| `PolyMini<T>::ntt(a, inverse = false)` | In place on a `vector<T>` whose length `n` is a power of two dividing `p - 1`. Forward: `a[j] = A(w^{rev(j)})` with `rev` the bit reversal on `log2(n)` bits and `w = z^((p - 1) / n)`, `z` the least quadratic non-residue from 2 (Euler's criterion, at most 1024 candidates): the same map as `Poly<T>::ntt`. Inverse: the exact inverse map, consuming bit-reversed input and scaling by `1/n` (`Poly<T>::intt`). Roots are recomputed per call (`O(n)` multiplications plus `O(log(n) * log(p))` for the powers). |
| `a * b` | Full product: empty when either factor is empty, otherwise exactly `\|a\| + \|b\| - 1` terms (no trimming). Schoolbook when `min(\|a\|, \|b\|) <= 32`, otherwise three transforms of length `bit_ceil(\|a\| + \|b\| - 1)`. |
| `a + b`, `a - b`, `a * c` | Coefficientwise on `max(\|a\|, \|b\|)` terms; scalar product keeps `\|a\|` terms. The scalar goes on the right. |
| `eval(a, x)` | Horner; 0 for the empty polynomial. |
| `deriv(a)`, `integ(a, c = 0)` | `max(\|a\| - 1, 0)` terms; `\|a\| + 1` terms with constant term `c` (inverses from one factorial inversion). |
| `divMod(a, b)` | `b` nonzero (asserted; `{0, trimmed a}` without assertions). Quotient of `deg a - deg b + 1` terms (empty when `deg a < deg b`), remainder trimmed with degree below `deg b`. Newton: `q = rev(rev(a) / rev(b) mod x^{deg a - deg b + 1})`, `r = a - q b` on the low `deg b` terms. |
| `inv(a, n)` | `n >= 0` terms of `1 / a`; `a[0] != 0` when `n > 0`; the result depends only on `a[0, n)` (the last doubling reads up to `bit_ceil(n)` terms). Each doubling `k -> 2k` transforms `a mod x^{2k}` and `g` at length `4k` and forms `g (2 - a g)` pointwise, exact because `deg(a g^2) < 4k`. |
| `log(a, n)` | `a[0] = 1`; `n` terms of `integ(a' / a)`; zeros for `n <= 1`. |
| `exp(a, n)` | `a` empty or `a[0] = 0`; Newton `f <- f (1 + a - log f) mod x^{2k}`; empty for `n = 0`. |
| `pow(a, k, n)` | Any `lng` exponent, `n >= 0` terms. `k = 0` gives `1` (also for the zero series); the zero series with `k > 0`, or valuation `v` with `v k >= n`, gives zero; `k < 0` needs `a[0] != 0`. Otherwise `a[v]^k x^{vk} exp(k log(a / (a[v] x^v)))` with `k` reduced into `T`, exact because `(1 + x h)^p = 1 mod x^p` and `n - vk <= n < p`. The full `pow` has the same contract; outside it (`k < 0` with `n = 0` and an empty or zero-valued constant term) the mini asserts where the full type returns empty. |
| `evalMulti(a, xs)` | `a(x_i)` for every point; duplicates and zero points allowed; empty for no points. Subproduct tree over the points, remainder descent with Horner below 33 points. |
| `PolyMini<T>::interpolate(xs, ys)` | `\|xs\| = \|ys\|` (asserted) distinct points (asserted through `P'(x_i) != 0`); the unique polynomial of degree `< n` as exactly `n` terms; empty for no points. Weights `y_i / P'(x_i)` combined up the subproduct tree. |
| `taylorShift(a, c)` | `a(x + c)` with `\|a\|` terms by the binomial convolution `sum_j a_j j! c^{j-i} / (j - i)!`. |
| `PolyMini<T>::findLinearRecurrence(a)` | Berlekamp–Massey over any prime field: the shortest `c` with `a_i = sum_{j=1}^{\|c\|} c[j-1] a_{i-j}` for `\|c\| <= i < \|a\|`; empty for the empty or zero sequence. When the shortest recurrence is not unique (`2\|c\| > \|a\|`) the result is the one the classical update sequence produces, which is the full type's choice: the mini runs the same updates with the leading 1 kept in the connection polynomial. |
| `PolyMini<T>::linearRecurrenceKth(a, c, k)` | `k >= 0`, `\|a\| >= d = \|c\|` (asserted). Returns `a[k]` for `k < \|a\|`, 0 for `d = 0` beyond `a`, otherwise `[x^k] P / Q` with `Q = 1 - sum c_j x^j` and `P = a[0, d) Q mod x^d` by Bostan–Mori (only the first `d` terms define the sequence beyond `a`). |

`facts`, `tree` and `evalTree` are implementation helpers, public only because the mini has no detail namespace: `facts(n)` returns factorials and inverse factorials up to `n` (`p > n`), `tree(xs)` the subproduct tree in heap layout over `4|xs|` nodes, `evalTree(t, xs, a)` the remainder descent.

### Boundedness without assertions

Every loop whose termination rests on a precondition has a hard bound: the non-residue search stops after 1024 candidates, the transform loops run while `2 * len <= n` and `i + 2 * len <= n`, so a length that is not a power of two (asserted) neither writes past the root table nor takes a zero stride; `divMod` by zero returns `{0, a}`; `inv` reads the constant term through `a.empty() ? 0 : a[0]`; `interpolate` resizes `ys` to `|xs|`; `linearRecurrenceKth` returns 0 for `k < 0` and copies only the available prefix of a short `a`. A negative precision reaches `vector` as a size above `max_size()` and throws `std::length_error`. Divisions by a nonunit follow the residue type (the minis return 0).

### Correctness argument

The NTT is the radix-2 decimation-in-frequency network with stage roots `w_{2len}^j`, `w_{2len} = z^((p - 1) / (2 len))`, which is a primitive `2len`-th root because `z` is a non-residue (`z^((p-1)/2) = -1`). The inverse runs the decimation-in-time stages in the opposite order with the inverse roots `z^(p - 1 - (p - 1) / (2 len))`, obtained without a division since `z^(p-1) = 1`; each stage maps `(x, y)` to `(x + w^{-j} y, x - w^{-j} y)`, twice the inverse of the forward stage, hence the final `1/n`. Since the forward map, its root and its output order coincide with `Poly<T>::ntt`, and every other shared operation computes a uniquely determined exact result (products, quotients, truncated series, interpolants, shifts, recurrence terms) with the same size convention, mini and full agree coefficient by coefficient; Berlekamp–Massey agrees because both run the identical discrepancy updates. Newton iterations double the precision per round, so `inv`, `log`, `exp` and `pow` cost `O(n log n)`; the subproduct tree has `O(log n)` levels of `O(n log n)` products and divisions; Bostan–Mori halves `k` with two products of size `O(d)` per round.

### Independent copy

The struct needs `<bits/stdc++.h>`, `<cassert>` (GCC 16's `bits/stdc++.h` no longer pulls it in; `01-template.hpp` gets it through the PBDS headers), `using namespace std;` and the aliases `uint`, `lng`, `ulng`, plus a residue type. The tester extracts `PolyMini` and `ModIntMini` from the headers, compiles them with exactly that preamble (plus `lll`/`ulll` for `ModIntMini`) under `-Wall -Wextra -Wshadow -Wconversion -Werror`, and runs a fixed program over every operation.

### Reductions against Poly<T>

Not in the mini, with reasons recorded in [00-notes.md](00-notes.md): the transform-limit block product, composite and arbitrary moduli, every ring and approximate coefficient type, `sqrt`, `powMod`, chirp-z, gcd/resultant, composition, sparse series, online products and the other full-only families. The mini's constant factors are larger for `exp` (Newton through `log` instead of the fused inverse), `evalMulti` and `interpolate` (remainder tree instead of the transposed tree); asymptotic bounds are the same.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `convolution(a, b)` | `a * b` (the product body is `operator*`) |

## Feature-to-test map

The [test entry](<../../96-Local Testing/01-Core/17-polymini_tester.py>) drives [the C++ oracle](<../../96-Local Testing/01-Core/17-polymini_tester.cpp>) and [a second translation unit](<../../96-Local Testing/01-Core/17-polymini_tester_tu.cpp>). Every operation runs over `ModIntMini` with 998244353, 7340033, 257, 97 and 17 (transform limits 2^23 down to 16, so the cap boundaries and small-field characteristic edges are exercised), `ModInt64Mini<29 * 2^57 + 1>`, the full `mint` as the coefficient type, and `DynModIntMini` set to 469762049 and then reset to 65537. Sizes cover 0, 1, powers of two and their neighbours, the 32/33 schoolbook and Horner cutoffs, and each modulus's largest allowed `N`. Every operation the full type shares except `+`, `-`, scalar `*`, `deg` and `trim` (whose oracles are direct loops) is also compared with `Poly<T>` for exact coefficient and size agreement, which is supporting evidence, never the only oracle.

| Feature | Independent oracle |
|---|---|
| `ntt`, `ntt(a, true)` | Naive bit-reversed DFT with an independently searched non-residue (lengths <= 512), spot evaluations `a(w^{rev(j)})` above; inverse round trip; `Poly::ntt`/`intt` agreement at every length up to the modulus limit (2^16 full, 2^19 stress); GF(2), GF(3), GF(5) at every allowed length (1; 1, 2; 1, 2, 4) |
| `*` | Schoolbook over all size pairs of the size list; random-point evaluation `c(x) = a(x) b(x)` up to 2^16 + 3 (full) and 2^19 - 1 (stress); aliasing `a = a * a` |
| `+`, `-`, scalar `*` | Coefficientwise loops over unequal sizes; `a + a`, `a - a`, multiplication by zero |
| `deg`, `trim`, `eval`, `deriv`, `integ` | Trailing-zero and zero polynomials; power sums `sum a_i x^i`; explicit formulas `i a_i` and `a_i / (i + 1)` with direct inverses; `deriv(integ(a, c)) = a` |
| `divMod` | Schoolbook long division (dividend and divisor with trailing zeros, constant divisor, zero dividend, `deg a < deg b`); large sizes: shape and `a = q b + r` at random points; `a = a / a` |
| `inv`, `log`, `exp` | Quadratic recurrences for `1/a`, `log a` (`i b_i = i a_i - sum j b_j a_{i-j}`) and `exp a`, inputs shorter and longer than `n`; large: `a * inv(a) = 1`, `exp(log a) = a`, `log(exp a) = a` |
| `pow` | Truncated binary powering by schoolbook for `k` in {0, 1, 2, 3, 5, 17, p - 1, p, 10^9 + 7, 2^63 - 1, -1, -2, -7} and valuations 0, 1, 2, 5; zero series, `0^0`, `x^k mod x^n` at the boundary `v k = n`, `n = 0`; large: `a^3` by products, `a^3 a^2 = a^5`, `u^{-1} u = 1` |
| `evalMulti` | Power sums per point, with duplicate and zero points; large: 20 spot points and `interpolate(evalMulti(a)) = a` |
| `interpolate` | O(n^2) Lagrange; distinct points drawn from the whole field for small moduli |
| `taylorShift` | Horner expansion `a(x + c)` by repeated multiplication with `x + c` for `c` in {0, 1, -1, random}; large: `s(x) = a(x + c)` at random points |
| `findLinearRecurrence` | Generated sequences (`2d <= n`): validity, length at most `d` and equality with the generator when the length is `d`; random sequences: validity at every index; exhaustive minimality over every sequence up to length 10 in GF(2), 7 in GF(3), 5 in GF(5) (full) by enumerating every shorter recurrence; empty, zero and `0 0 1` sequences |
| `linearRecurrenceKth` | Direct iteration for `k <= 4000`, Kitamasa (`x^k mod` the characteristic polynomial by schoolbook) above, `k` up to `2^63 - 1`; `\|a\| > d`, `d = 0` inside and beyond `a` |
| Oracles | Hand-computed cases for every oracle (products, inverse, exp, log, division, shift, Lagrange, powering, DFT, Fibonacci by iteration and Kitamasa across the 4000 seam, brute recurrence length) |
| Preconditions | 15 checked-build assertion deaths: zero divisor, `inv` of a zero or empty constant term and negative `n`, `log`/`exp` constant terms, negative exponent with valuation, negative `n` in `pow`, non-power-of-two and over-limit `ntt`, over-limit `*` in GF(97), `interpolate` size mismatch and duplicate points, negative `k` and short `a` in `linearRecurrenceKth` |
| Builds | Header alone with `-Werror`; `98-basic.hpp` and `99-all.hpp`; extracted copy; two translation units in every configuration; optimized scalar `-O2 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS`, optimized AVX2/FMA/BMI2 (the mini has no ISA path; the compared `Poly` kernels do), ASan/UBSan scalar in full and stress |

Quick runs every operation with sizes up to 65 (series) and 1100 (large identities), the brute recurrences to length 6 in GF(2) and 4 in GF(3), and the optimized, checked and AVX2 builds. Full extends sizes to 513 and 2^16 + 3, adds GF(5) and the sanitizer build. Stress extends to 1000 and 2^19 - 1 and evaluation/interpolation identities to 2^17.

## Commands and results

Run 2026-10-13 on the pruned header (`### Legacy mapping`; GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Intel Core i9-11900H); the two full runs, the integration run and the review ran concurrently. The stress row is the 2026-10-10 run of the header before the prune, which differed only in carrying `convolution` with `operator*` forwarding to it; the product body and every other operation are unchanged, so it was not rerun.

```bash
python3 '96-Local Testing/01-Core/17-polymini_tester.py' --mode quick --seed 1
python3 '96-Local Testing/01-Core/17-polymini_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/01-Core/17-polymini_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/01-Core/17-polymini_tester.py' --mode stress --rounds 1 --seed 20261011
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/17-polymini.hpp '96-Local Testing/01-Core/17-polymini_tester.cpp' '96-Local Testing/01-Core/17-polymini_tester_tu.cpp' '96-Local Testing/01-Core/17-polymini_benchmark.cpp'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

| Run | Result | Checks per configuration | MEMORY peak |
|---|---|---|---|
| quick, g++ | PASS, 3 configurations (no sanitizer in quick) in 149.9 s, 15 precondition deaths, header-alone/aggregates/extracted copy | 25,682 | 1,433 MB |
| full, g++ | PASS, pre-flight quick (peak 1,432 MB), 4 configurations in 502.3 s (optimized 16.4 s, checked 21.8 s, AVX2 12.7 s, ASan/UBSan 130.5 s) | 127,762 | 1,749 MB |
| full, g++-14 (floor check) | PASS, pre-flight quick (peak 1,400 MB), 4 configurations in 484.0 s (optimized 16.5 s, checked 24.1 s, AVX2 13.1 s, ASan/UBSan 111.5 s) | 127,762 | 1,823 MB |
| stress, 1 round, g++ (2026-10-10, previous header) | PASS, 4 configurations in 1,030.0 s (sanitized build on the full lists) | 138,461 (sanitized 133,763) | 1,747 MB |
| integration | PASS, 135 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, 303 s | | 650 MB |
| review differential (`/tmp/p026_review/diff.cpp`): HEAD's header with the struct renamed against the pruned header; static assertions that both resolve `*` alike (compiles or not, result type) for 27 operand kinds on each side over five coefficient types; products over every size pair 0..70 and sampled sizes up to 4097 with lvalue, `const`, rvalue, moved and aliased operands, and every internal caller (`divMod`, `inv`, `log`, `exp`, `pow`, `evalMulti`, `interpolate`, `taylorShift`, `linearRecurrenceKth` up to `k = 2^62 + 5`); `g++ -O2 -D_GLIBCXX_ASSERTIONS`, `g++-14 -O2`, ASan/UBSan | 459,215 comparisons identical in every build; sanitizers clean | | 700 MB |

The full-mode count at seed 20261013 is 133,761 with HEAD's tester rebuilt (optimized scalar) and 127,762 now; the 5,999 difference is exactly the deleted `convolution(a, b)` comparison on each size pair and the deleted `a = convolution(a, a)` check per coefficient type, whose inputs stay checked through `a * b`. The memory peaks are the compiler on the tester translation unit, which instantiates the full `Poly<T>` for nine coefficient types; the test programs themselves stay below 40 MB (1.3 GB under ASan). Brace, comment-cap and type-header checks report no violation; consistency reports no errors. Exact GCC 14.2 and a Windows build were not run.

## Benchmarks

[`17-polymini_benchmark.py`](<../../96-Local Testing/01-Core/17-polymini_benchmark.py>) with [its C++ driver](<../../96-Local Testing/01-Core/17-polymini_benchmark.cpp>); raw log `96-Local Testing/01-Core/17-polymini_benchmark.jsonl` (ignored by git).

```bash
python3 '96-Local Testing/01-Core/17-polymini_benchmark.py' --config baseline --output '96-Local Testing/01-Core/17-polymini_benchmark.jsonl'
```

Conditions (2026-10-10, header before the prune; `a * b` runs the body it then reached through `convolution`, so the table was not remeasured): GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG -march=x86-64 -mno-avx -mno-avx2 -mno-fma -mno-bmi2 -mno-pclmul` (the mini has no ISA path; the full type runs its scalar fallback), seed 20261010, uniform random residues modulo 998244353, warmup doubling to 3 ms, median of 5 repetitions, every measured result checked against `Poly<mint>` before timing; host not frequency-isolated (the constant full-type reference in the cutoff rows moved between 64 and 103 ms). Peak memory 445 MB. Medians in milliseconds:

| Operation | n | PolyMini | Poly | Ratio |
|---|---|---|---|---|
| convolution n x n | 131072 | 30.59 | 24.16 | 1.27 |
| convolution n x n | 500000 | 146.08 | 113.35 | 1.29 |
| divMod n by n/2 | 500000 | 260.22 | 273.78 | 0.95 |
| inv | 500000 | 273.06 | 233.74 | 1.17 |
| log | 500000 | 423.03 | 392.81 | 1.08 |
| exp | 500000 | 1096.73 | 580.65 | 1.89 |
| pow, k = 10^18 | 500000 | 1698.62 | 976.49 | 1.74 |
| taylorShift | 500000 | 163.07 | 170.07 | 0.96 |
| evalMulti, n points | 131072 | 1481.66 | 776.56 | 1.91 |
| interpolate, n points | 131072 | 2625.06 | 1097.37 | 2.39 |
| findLinearRecurrence | 16384 | 308.18 | 314.30 | 0.98 |
| linearRecurrenceKth, d = n/2, k = 10^18 | 16384 | 261.65 | 254.99 | 1.03 |

At 1024 terms the mini is 1.3 to 2.5 times slower (per-call root tables and allocations dominate). The larger factors for `exp`, `pow`, `evalMulti` and `interpolate` are the accepted cost of the shorter algorithms (Newton through `log`, remainder tree); the bounds are unchanged.

Thresholds. The schoolbook cutoff `min(|a|, |b|) <= 32` comes from the balanced crossover (schoolbook versus a three-transform product: 0.005 against 0.009 ms at 32, 0.010 against 0.016 at 48, 0.017 against 0.016 at 64) lowered to 32 because an unbalanced product with a 33-to-64-term factor pays the schoolbook cost on every term of the long factor; it equals the full type's scalar `SCHOOLBOOK`. The Horner leaf of the remainder descent at 32 points: `evalMulti` with 16384 points took 309, 242, 225, 132, 128 and 127 ms at leaf sizes 1, 8, 16, 32, 64 and 128, so 32 sits at the start of the plateau.

## Sources

Independently written from the algorithms in the cited references; no code copied. The completeness sweep of 2026-10-10 is recorded in [00-sources.md](00-sources.md) (KACTL numerical chapter, Benq cp-notebook polynomials, cp-algorithms, OI Wiki, ei1333 and Nyaan FPS, the Library Checker polynomial tree); its rejected operations and reasons are in [00-notes.md](00-notes.md). The algorithms are the ones the full type documents in [16-poly.md](16-poly.md) (Newton iteration, subproduct tree, Bostan–Mori, Berlekamp–Massey), which serves as a reference implementation, not as a dependency.

## Limits and handoffs

Exact GCC 14.2, a Windows/MinGW build and PyPy were not run; no online judge submission was made. The mini has no transform-limit fallback: products beyond `2^v` terms for `2^v || p - 1` assert, where the full type switches to its block product.

## History

- 2026-10-10: P026 implementation and verification: quick (seed 1, 26,591 checks), full (seed 20261010, 133,760 checks, 4 configurations, 589.6 s on GCC 16.2.1 and 684.0 s on GCC 14.4.1), integration (126 headers), plus the stress and benchmark runs still listed above.
- 2026-10-13: prune (`/prune P026`): `convolution(a, b)` removed (`### Legacy mapping`), its body is now `operator*`; its two tester checks repeated `a * b` checks on the same inputs and were deleted; review found no defect; quick, full, floor check, integration and consistency as recorded above.
