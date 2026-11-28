# 08-infintmini.hpp — Evidence

P019/C05 owns `08-infintmini.hpp`: `InfIntMini` (alias `iintmini`), the independently copyable mini of the arbitrary-precision signed integer. It is a documented reduction of [`07-infint.hpp`](07-infint.md): every operation the two share returns the same value, string and sentinel. The header includes only `01-template.hpp` and depends on no sibling, full type or detail namespace. The legacy original is `OLD/1-Core/06-infintmini.hpp`.

## Contracts

### Representation and Sentinel

`int sgn` (`1` or `-1`), `bool is_inf`, `vector<uint> n` of base-10^5 limbs, little-endian, without leading zero limbs. Zero is `{0}` after any operation (a default-constructed value keeps `n` empty, also zero) with `sgn == 1`; `trim()` restores the canonical form after a caller edits `n` directly. The signed infinity follows the full type's rules exactly ([07-infint.md](07-infint.md), Representation and Sentinel), restricted to the shared operations:

- `read` and the string constructor accept `inf` in any letter case with an optional sign in bases 2–23 (in bases 24–36 the letters are digits).
- `inf + x = inf`, `inf + (-inf) = 0`, `inf - inf = 0`; `inf * x = ±inf` for `x != 0`, `inf * 0 = 0`.
- `inf / p = ±inf`, `x / inf = 0`, `inf / inf = ±1`; `inf % p = 0`, `x % inf = x` (`divMod` agrees).
- `abs`, unary minus, comparisons (`inf` above every finite value), `pow` (`inf^0 = 1`, sign by parity), `gcd(inf, x) = |x|` for `x != 0` and `inf` otherwise, `toString` (`inf`/`-inf`) and the saturating casts.

### Construction, Conversion and I/O

| Feature | Contract |
|---|---|
| `InfIntMini()`, `InfIntMini(T)`, `operator=(T)` for every integral `T` through `lll`/`ulll` | Exact; `bool` converts to 0/1; assignment clears the sentinel. |
| `InfIntMini(string_view s, uint base = 10)` | Asserts that `read` succeeds. |
| `read(string_view s, uint base = 10)` | `base` in `[2, 36]` (asserted; clamped without assertions). One optional `+`/`-`, then `inf` (bases 2–23) or one or more digits `0-9`, `A-Z`, `a-z` below the base, leading zeros allowed. Any other string (empty, lone or repeated sign, embedded space, a digit at or above the base, `infinity`) returns false and leaves the value unchanged; a successful parse replaces the sentinel. Decimal parses five digits per limb in O(L); other bases multiply and add per digit in O(L^2). |
| `toString(uint base = 10)` | `base` in `[2, 36]`; uppercase digits, `-` for negatives, `0` for zero, `inf`/`-inf`. Decimal prints limbs in O(n); other bases divide by the base per digit in O(n^2). |
| `explicit operator lng()`, `explicit operator ulng()` | Finite values reduce modulo 2^64 (two's complement, like a C cast): Horner over the low 13 limbs suffices because 10^65 is a multiple of 2^64. The sentinel saturates (`LLONG_MAX`/`LLONG_MIN`, `ULLONG_MAX`/`0`). |
| `explicit operator bool()`, `isNil()` | Nonzero test and its negation (both kept by decision 2026-10-17, as in P018); the sentinel is nonzero. |
| Streams | `>>` reads one whitespace-delimited token through `read` in the stream's base: the `SetBase{b}` manipulator stores `b` in the per-stream `iword(getBaseId())` (default 10), and the `std::hex`/`std::oct` flags take precedence in that order, as in the full type. A malformed token sets `failbit` and keeps the destination; end of input leaves it unchanged. `<<` prints `toString` in the stream's base. The mini's `iword` index is its own, so `InfIntMini::SetBase` does not affect `InfInt` streams. |

### Arithmetic and Division

| Feature | Contract |
|---|---|
| `+=`, `-=`, `+`, `-`, unary `+`, unary `-`, `abs` | Exact; `a += a` and `a -= a` are allowed (`-=` adds a negated copy). |
| `*=`, `*` | Exact; `a *= a` squares with one forward transform. Native scalars convert through the constructor (at most eight limbs for 128-bit values, so the product is O(n)). Schoolbook with 64-bit column sums and the shorter operand in the outer loop while `n * m <= MUL_NTT * L * bit_width(L)`, with `MUL_NTT = 13` (Benchmarks) and the transform length `L = bit_ceil(n + m)`; otherwise a cyclic NTT modulo `P = 2^64 - 2^32 + 1` with generator 7. Every column sum is below `min(n, m) * (10^5 - 1)^2 < P` while `n + m <= 2^30` limbs (asserted), so the convolution is exact. |
| `/=`, `%=`, `/`, `%`, `divMod(a, b)` | Truncating division, divisor nonzero (asserted; `{0, a}` without assertions); the remainder keeps the dividend's sign and `\|r\| < \|b\|`; `a /= a = 1`, `a %= a = 0`. Divisors of at most three limbs (below 10^15) use `divWord`, others `divLong`. Native scalars convert through the constructor. |
| `divWord(d)` | In-place `\|this\| / d` for a word `1 <= d < 2^64` (asserted; 0 is treated as 1 without assertions), returns the remainder; one 128-bit division per limb. |
| `divLong(a, b)` | Schoolbook long division of `\|a\|` by `\|b\|` (at least two limbs, asserted; quotient 0 and `a` unchanged without assertions; `divMod` passes four or more) in O((n - m + 1) m) limb operations; `a` keeps the remainder. Correctness: with `R` the top three limbs of the current window `r < B b` and `D >= B` the top two of `b`, `r < (R + 1) B^(m-2)` and `b >= D B^(m-2)` give `q <= floor(R / D) = q'`, and `r >= R B^(m-2)`, `b < (D + 1) B^(m-2)` with `R < B (D + 1)` give `q' - q < R / (D (D + 1)) + 1 < B / D + 1 <= 2`, so `q' - q <= 1`: the digit `min(q', B - 1)` is lowered at most once, adding `b` back when the window's carry is negative. |
| `magCmp`, `<=>`, `==` | Magnitude order (`inf` above everything finite), signed order and equality, as in the full type. |
| `pow(a, e)` | `e >= 0` (asserted; 0 without assertions), right-to-left square-and-multiply with transform squaring; `pow(0, 0) = 1`. |
| `gcd(a, b)` | Nonnegative; Euclid by `%` in O(n^2) limb operations overall; `gcd(0, 0) = 0`. The sentinel rules follow from `x % inf = x` and `inf % x = 0`. |

### Boundedness without Assertions

Under `NDEBUG` every violated precondition stays bounded: bases outside `[2, 36]` are clamped, a zero divisor returns `{0, a}` from `divMod` (so `/` gives 0 and `%` the dividend), `divWord(0)` divides by 1, `divLong` with a one-limb divisor returns 0, `pow` with a negative exponent returns 0, and the string constructor keeps zero on malformed input. Products above `2^30` limbs are outside the stated domain, because the transform length would exceed the coefficient bound.

### Independent Copy

The struct holds every helper (`mulAdd`, `addMag`, `mulP`, `powP`, `ntt`, `divWord`, `divLong`) and needs nothing beyond the template's standard headers and aliases. The tester pastes `01-template.hpp` and the header body without its `#include` line into one file, then compiles and runs it.

### Reductions against InfInt

The mini keeps the full type's names and results for the shared operations above. Left out after the 2026-10-17 sweep ([00-sources.md](00-sources.md)), with what replaces each:

- `++`/`--` (`a += 1`) and `lcm` (`a / gcd(a, b) * b`).
- `toDouble` and 128-bit conversions out: the full type's rounding and `tryCast` cost more lines than the mini's budget.
- Shifts by limbs: the full type's `<<`/`>>` shift bits, so the same expression would mean something else.
- Digit sums, `log10`, `square`, the `ZERO`/`ONE` constants, roots, bitwise operations, modular inverse and power, rounded division modes, limb and digit-vector conversions, sampling and hashing: the full type owns them.

Radix conversion outside base 10 is quadratic; inputs of more than about 10^5 hexadecimal digits belong to the full type.

### Legacy Mapping

| Removed (P019, 2026-10-17) | Replacement |
|---|---|
| `divModSlow(a, b)` | `divMod(a, b)` (the long division moved in) |
| `operator!` | `!a` through `explicit operator bool` |
| `operator=(string_view)` | `a.read(s)` or `a = InfIntMini(s)` |
| scalar `*=`, `/=`, `%=` templates, `operator*(T, InfIntMini)`, scalar `/`, `%`, `isFastScalar`, `sgnMag` | the `InfIntMini` operators; native scalars convert through the constructor |
| word loops in the scalar `/=` and `%=`, `divMagSmall` | `divWord` (widened to every 64-bit divisor) |
| `addMagSmall`, `mulMagSmall`, `subMag`, `rsubMag`, `mulMag` | `mulAdd`, `addMag(o, s)` and the schoolbook branch of `*=` |
| `NTT_P`, `NTT_G`, `modP`, `nttPow`, `DIG`, `BLEN`, `operator-() &&` | `P`, generator 7 inside `ntt`, `mulP`, `powP`, inline digit table, the single `operator-` |
| `read` returning `void`, accepting repeated signs and mapping the empty string to zero | `read` returning a status and leaving the value unchanged on malformed input, as the full type does |

## Feature-to-Test Map

`96-Local Testing/01-Core/08-infintmini_tester.py` builds `08-infintmini_tester.cpp` with the second translation unit `08-infintmini_tester_tu.cpp` as optimized (`-O2 -DNDEBUG`), checked (`-O1 -g -D_GLIBCXX_ASSERTIONS`) and, from full mode, ASan/UBSan (`-O1 -fsanitize=address,undefined`, quarantine capped at 64 MB) programs, all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. It runs the self-tests and streams the `--protocol` cases against Python's integers and a model of the sentinel rules.

| Feature | Verification (self-test phase; protocol command) |
|---|---|
| Integral constructors and `operator=` for every width (minima, maxima, `bool`), default zero, `trim` (zero limbs, negative zero, empty), sentinel construction and clearing | construction; `ct` (eleven types, eight values each) |
| `read`/string constructor in bases 2–36 (upper and lower case, `+` with leading zeros, rejections: empty, lone and repeated signs, bad digits, digit at the base, `infinity`, `0x`; `inf` only in bases 2–23, `inf` as the base-24 numeral 10935, `INF` as the base-36 numeral 24171), `toString` round trips | construction; `rd`, `tb` |
| `lng`/`ulng` casts (mod 2^64 over long values, saturation), `explicit bool`, `isNil` | construction; `cs` |
| `+`, `-`, `*`, `/`, `%`, compound forms (the protocol checks each binary operator against its compound form), unary `+`, unary `-`, `abs`, `divMod`, `magCmp`, `<=>`, `==`, including every sentinel combination and operands up to 443,000 bits (full) | arithmetic (aliasing, `a + b - b`, schoolbook product); `ar` |
| Native scalar operands (`int32_t`, `uint32_t`, `lng`, `ulng`, zero, extremes) through the constructor | `sc` |
| Mul backends: schoolbook and NTT on both sides of the cost bound (382 x 382 schoolbook, 383 x 383 and 400 x 400 transform, 512 x 512, 2000 x 7, 5000 x 300, 300 x 5000, 5000 x 900), a 2^15 x 2^14-limb product and an all-99999 square of 2^14 limbs (full), against an independent base-10^5 schoolbook with row carries | arithmetic (`school`) |
| Div backends: `divWord` (divisors below 10^15, including 10^5 and 10^15 - 1) and `divLong` (quotient and remainder by construction `n = q b + r` with `r < b`; adversarial windows with top limbs 1 and 99999, quotient digits 99999 and `r = b - 1`, divisors of 4, 5, 9 and 33 limbs) | arithmetic; `ar`, `sc` |
| `pow` (exponents 0–64, sentinel parity), `gcd` (sentinel cases, zeros, equal operands) | `pw`, `gd` |
| Streams: `SetBase`, `std::hex`, `std::oct`, lowercase input, failbit on a malformed token, unchanged value at end of input, `getBaseId` | streams; `io` |
| Preconditions under the checked build (8 `--invalid` cases: `/`, `%`, `divMod` and scalar `/` by zero, `read` base 1, `toString` base 37, malformed string constructor, negative exponent) | `--invalid` |
| Two translation units, header alone, standalone copy (pasted template), `998-basic` and `999-all`, full and mini in one program (`08-infintmini_tester_mixed.cpp`: `+ - * divMod gcd pow`, `toString` in random bases, casts, comparisons and `read` agree with `InfInt` on 100/600/2000 random pairs including the sentinel; supporting evidence only) | the driver's compile steps; `otherUnit` |

Quick runs the small corpus (about 800 protocol cases). Full adds the sanitizer build (the 64 MB quarantine keeps its peak near 0.4 GB instead of 3.7 GB), operands up to 443,000 bits and the large transforms (about 3,800 protocol cases and 3,700 self-test checks per configuration). Stress repeats with `--rounds` seeds and operands up to 2.2 million bits. gcd cases keep the smaller operand below 100,000 bits because Euclid is quadratic. The quick corpus executes every line of the header (gcov, 2026-10-17). Hand-seeded faults (2026-10-17, quick mode): each of 6 faults (the NTT reduction's overflow fold, the butterfly's wrap-around, the `inf` base limit, the cast window, the word-divisor bound, the gcd's sign normalization) failed the suite.

## Commands and Results

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run; compile limits in `96-Local Testing/Docs/00-parallel.md`), all PASS. Times are compile + run in seconds; peaks in MB.

| Command | Optimized | Checked | Sanitized | Pre-flight quick peak | Peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 0.1 + 86.2 | 0.2 + 87.5 | 20.2 + 120.0 | 778 | 528 |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 0.3 + 138.0 | 0.4 + 137.9 | 31.0 + 194.6 | 780 | 505 |
| `python3 '96-Local Testing/01-run.py' --mode quick` | 11.9 + 0.1 | 13.2 + 0.2 | — | — | 753 |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 0.1 + 1228.3 | 0.1 + 1182.3 | 10.7 + 45.2 | 779 | 561 |

2026-11-22 (`/finish C03`, rerun on the committed `05-modint.hpp`, [05-modint.md](05-modint.md#commands-and-results)): `python3 '96-Local Testing/01-Core/08-infintmini_tester.py' --mode quick --seed 1` passed, 2 configurations, 8 s.

Final header, 2026-10-17 (GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Intel Core i9-11900H; other worktrees' suites ran at the same time; pre-flight quick peaks 413–420 MB under the 2048 MB cap):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/08-infintmini.hpp '96-Local Testing/01-Core/08-infintmini_tester.cpp' '96-Local Testing/01-Core/08-infintmini_tester_tu.cpp' '96-Local Testing/01-Core/08-infintmini_tester_mixed.cpp' '96-Local Testing/01-Core/08-infintmini_benchmark.cpp'   # no violations
python3 '96-Local Testing/01-Core/08-infintmini_tester.py' --mode quick --seed 3                 # PASS, 2 configurations, 25 s, MEMORY peak=413MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/08-infintmini_tester.py' --mode full --seed 20261017           # PASS, 3 configurations, 3,831 Python-oracle cases and 3,664 self-test checks each, 5,400 mixed checks, 133 s, MEMORY peak=486MB cap=4096MB oom_kills=0
CXX=g++-14 python3 '96-Local Testing/01-Core/08-infintmini_tester.py' --mode full --seed 20261017   # PASS, the same counts, 137 s, MEMORY peak=491MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/08-infintmini_tester.py' --mode stress --seed 7 --rounds 2     # PASS, seeds 7 and 8: 11,086 and 11,100 cases per optimized and checked run, sanitized full 3,860 and 3,846, 18,000 mixed checks, 2,520 s, MEMORY peak=533MB cap=4096MB oom_kills=0
python3 '96-Local Testing/02-integration.py'                                                     # PASS, 149 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, MEMORY peak=650MB
python3 '96-Local Testing/03-consistency.py'                                                     # no errors
```

## Benchmarks

Command: `python3 '96-Local Testing/01-Core/08-infintmini_benchmark.py' --output '96-Local Testing/01-Core/08-infintmini_benchmark.jsonl'` (driver `08-infintmini_benchmark.cpp`; 2026-10-17, 2 min). Conditions: 11th Gen Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG -march=x86-64`, seed 20261017, uniform random limbs, median of 21 runs (5 above 10^6 limb products). Both backends' outputs are compared before timing. The dispatcher column includes the limb copies and carries around either backend. Other worktrees ran test suites at the same time, so absolute numbers carry noise; the record fixes the crossover.

| Shape (limbs) | Schoolbook µs | NTT µs | Dispatcher µs | cost_ratio | Dispatcher picks |
|---|---|---|---|---|---|
| 16 x 16 | 0.15 | 5.91 | 0.21 | 1.33 | schoolbook |
| 32 x 32 | 0.40 | 8.06 | 0.53 | 2.29 | schoolbook |
| 48 x 48 | 1.19 | 11.74 | 1.35 | 2.25 | schoolbook |
| 64 x 64 | 1.96 | 11.71 | 2.17 | 4.00 | schoolbook |
| 96 x 96 | 4.13 | 18.79 | 4.45 | 4.00 | schoolbook |
| 128 x 128 | 7.05 | 18.66 | 7.46 | 7.11 | schoolbook |
| 192 x 192 | 15.34 | 32.79 | 15.93 | 7.20 | schoolbook |
| 256 x 256 | 26.81 | 32.95 | 28.30 | 12.80 | schoolbook |
| 512 x 512 | 106.60 | 64.99 | 79.62 | 23.27 | NTT |
| 1024 x 1024 | 405.91 | 122.34 | 127.72 | 42.67 | NTT |
| 4096 x 4096 | 7409.11 | 668.45 | 652.46 | 146.29 | NTT |
| 65536 x 8 | 251.55 | 13620.76 | 388.28 | 0.22 | schoolbook |
| 65536 x 16 | 508.45 | 13159.18 | 646.20 | 0.44 | schoolbook |
| 65536 x 32 | 982.11 | 13328.92 | 1139.26 | 0.89 | schoolbook |
| 65536 x 64 | 1991.04 | 13050.67 | 2130.53 | 1.78 | schoolbook |
| 65536 x 128 | 4079.33 | 13885.40 | 3436.43 | 3.56 | schoolbook |
| 65536 x 256 | 7108.65 | 11655.43 | 7236.04 | 7.11 | schoolbook |
| 65536 x 384 | 10949.65 | 11744.90 | 11096.58 | 10.67 | schoolbook |
| 65536 x 512 | 14954.69 | 11853.72 | 12745.38 | 14.22 | NTT |
| 65536 x 768 | 22472.72 | 12005.27 | 12193.78 | 21.33 | NTT |
| 16384 x 64 | 450.84 | 2672.39 | 481.05 | 2.00 | schoolbook |
| 16384 x 128 | 909.97 | 2687.75 | 954.42 | 4.00 | schoolbook |
| 16384 x 256 | 1835.65 | 2684.15 | 1843.84 | 8.00 | schoolbook |
| 16384 x 512 | 3645.88 | 2682.54 | 2817.76 | 16.00 | NTT |

`cost_ratio` is `n m / (L bit_width(L))`. Balanced products cross between 256 and 512 limbs (interpolated near 388 limbs, ratio 13.4), the 65536 x m rows near `m = 410` (ratio 11.4) and the 16384 x m rows near `m = 373` (ratio 11.7). `MUL_NTT = 13` therefore keeps every balanced size on the faster side and costs at most about 14% in the unbalanced band between the two ratios. Putting the shorter operand in the outer loop made the unbalanced schoolbook about 20% faster than the legacy orientation.

## Sources

The 2026-10-17 completeness sweep (`@researcher`) read cp-algorithms, OI Wiki, KACTL, the Library Checker `big_integer` tree and the Nyaan, maspypy, ei1333, hitonanode, tko919 and suisen big-integer pages ([00-sources.md](00-sources.md)); it found no operation a mini should add. The reduction policy is the folder's ([00-notes.md](00-notes.md), Integers).

## Limits and Handoffs

- Non-decimal radix conversion is O(L^2); the Library Checker hexadecimal problems at full size need the full type.
- P025 (`16-poly.hpp`): since P018 stage 1 (2026-10-26) `poly_detail::BigInt` requires `n.size() -> int` and `n.data() -> ulng *` (base-2^64 words). `InfIntMini`'s `vector<uint> n` of base-10^5 limbs does not satisfy it, so `Poly<InfIntMini>` takes Poly's generic `karatsuba` path (user decision 2026-10-26).
- The judge wrappers `97-Online Testing/src/01-Yosupo/02-Big Integers/02-additionmini.cpp`, `04-multiplicationmini.cpp` and `06-divisionmini.cpp` use only `>>`, `+=`, `*=`, `divMod` and `<<`, which keep their meaning.
- Verified before the mutation rule of 2026-11-03; no record yet. P256 (InfIntMini parity) records it (user decision 2026-11-22).

## Aliases

Search aliases (P263, grab --find): big integer, bignum, arbitrary-precision arithmetic, high-precision arithmetic (OI Wiki: 高精度计算); Library Checker `addition_of_big_integers`, `multiplication_of_big_integers`, `division_of_big_integers`, 大整数, 大数运算, 高精度乘法, 高精度除法, 压位高精度.

## History

- 2026-10-17: P019 rewrite verified: new header (513 to 252 lines), tester, mixed full/mini program and benchmark; 6 seeded faults caught and 3 equivalent ones simplified away; the review's findings (backend preconditions, cost-bound wording, the unbalanced crossover and loop orientation, documented limits) fixed; full suites passed on g++ and g++-14, two stress rounds passed.
- 2026-10-26: the P018 `/finish` audit updated the P025 handoff: `poly_detail::BigInt` now requires base-2^64 words, so the mini no longer matches it and the earlier concern about a wrong `kronecker` product is closed (user decision 2026-10-26).
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, with the 10 s quick bound and the raised compile limit; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-22: `/finish C03` reran the quick suite on the committed `05-modint.hpp`; passed.
