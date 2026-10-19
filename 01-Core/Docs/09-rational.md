# 09-rational.hpp — evidence

P019/C06 owns `09-rational.hpp`: the canonical rational `Rational<T>` over `T` in {`lng`, `lll`, `InfInt`}, the alias `Frac = Rational<lng>`, the `rational_detail` wide-arithmetic kernels and `std::hash<Rational<T>>`. The header includes only `01-template.hpp`. `Rational<InfInt>` needs `07-infint.hpp` included by the caller; the header does not include it because the full big integer (125 KB) would push every bundled `Frac` solution past Codeforces' 64 KB source limit. Prerequisites P002 and P018 are verified.

## Contracts

### Representation

`T num, den` for `T` = `lng`, `lll` or `InfInt` (other types fail a `static_assert`: `Rational<int>` or Linux's `Rational<long long>` would pick the generic kernels and wrap): `den > 0`, `gcd(|num|, den) = 1`, zero is `0/1`, so `==` is member equality (defaulted) and `std::hash` depends only on the value. Every constructor, builder and operation returns this canonical form; after editing the fields directly a caller restores it with `normalize()`. `BOUNDED` is true for `lng` and `lll`; `M` is the magnitude type (`ulng`, `ulll`, or `InfInt` itself). The bounded types store every canonical value with `MIN(T) <= num <= MAX(T)` and `1 <= den <= MAX(T)`; for `InfInt` the components must be finite (asserted), and InfInt's infinity sentinel is never a rational value.

### Construction and conversion

| Feature | Contract |
|---|---|
| `Rational()`, `Rational(T n, T d = 1)` | Default `0/1`. `d != 0` (asserted); the reduced value must fit `T` (asserted; e.g. `Frac(1, LLONG_MIN)` fails, `Frac(2, LLONG_MIN) = -1/2^62` fits); without assertions an unrepresentable or zero-denominator pair becomes `0/1`. The gcd runs on unsigned magnitudes, so minima are safe. Integer operands of the arithmetic operators and comparisons convert through this constructor. |
| `Rational(I n)` (InfInt only) | Every integral `I` through 128 bits, so `Rational<InfInt> r = 5` and `r + 1` compile (InfInt's own constructor is a user-defined conversion). |
| `explicit Rational(const Rational<U> &o)` | Value-preserving conversion between the three `T`; no gcd (the value stays reduced). Narrowing asserts that both components fit (`0/1` without assertions). |
| Floating arguments | Deleted: `Frac(0.5)`, `Frac(1, 2.5)`, `x + 0.5` and `x == 0.5` do not compile (they would truncate); `fromDouble` is the exact path. |
| `Rational::raw(n, d)` | `d > 0` (asserted; `0/1` without assertions) and `gcd(n, d) = 1` (the caller's guarantee, not checked: checking would cost the gcd that `raw` exists to skip). |
| `Rational::fromDouble(long double x)` | `x` finite (asserted); the exact dyadic value `m 2^e` (a `double` argument converts exactly). For `lng` and `lll` the value must fit (asserted: `0.1` is `3602879701896397/2^55` and fits `Frac`, `1e-30` does not). |
| `normalize()` | Restores the canonical form of `num/den` with `den != 0`, under the constructor's rules. |
| `num`, `den`, `sign()`, `isInteger()` | Public fields; `sign()` is `-1`, `0` or `1`; `isInteger()` is `den == 1`. |

### Arithmetic and comparison

| Feature | Contract |
|---|---|
| `tryAdd(x, y, res)`, `trySub(x, y, res)` | True iff the exact sum or difference fits `T` (always for `InfInt`), then stored in `res`; `res` is unchanged on false and may alias `x` or `y`. Knuth's algorithm (TAOCP 4.5.1, GMP `mpq_add`): `g = gcd(b, d)`; if `g = 1` the result is `(a d ± c b) / (b d)`, else `t = a (d/g) ± c (b/g)`, `g2 = gcd(t mod g, g)` and the result is `(t / g2) / ((b/g) (d/g2))`, which is reduced. The cross sum `t` is computed one width up (`ulll` for `lng`, a (high, low) pair of `ulll` for `lll`) with signs on unsigned magnitudes, so a false status means only that the reduced result does not fit; cancellation of an intermediate above 2^127 is exact. |
| `tryMul(x, y, res)`, `tryDiv(x, y, res)` | Same status contract. Cross-cancelled: `g1 = gcd(a, d)`, `g2 = gcd(c, b)`, result `((a/g1)(c/g2)) / ((b/g2)(d/g1))`, already reduced, so the checked products overflow exactly when the result does not fit. `tryDiv` swaps the divisor's components; a zero divisor asserts (false without assertions). Division by `MIN(T)` works when the quotient fits (`MIN / MIN = 1`). |
| `+=`, `-=`, `*=`, `/=`, `+`, `-`, `*`, `/` | The try forms with an asserted status: overflow is a precondition violation, never undefined behaviour (the products use `__builtin_mul_overflow` on unsigned magnitudes; the result is unchanged under `NDEBUG`). Self-aliasing is allowed. |
| unary `+`, unary `-`, `abs(x)`, `inv(x)` | Exact; `-x` and `abs(x)` assert `num != MIN(T)`; `inv` asserts `x != 0` and that `1/x` fits (`inv(MIN)` does not). |
| `pow(x, e)` | Any `lng` exponent (`LLONG_MIN` included); `e < 0` inverts and asserts `x != 0`; `pow(0, 0) = 1`. Because `num` and `den` are coprime, `num^\|e\| / den^\|e\|` is reduced; the result must fit (asserted; square-and-multiply on magnitudes stops at the first overflow, so the cost stays O(log e) word products). |
| `<=>`, `==` | Exact order: for `lng` the signed 128-bit cross products directly; otherwise signs first, then `\|a\| d` against `\|c\| b` one width up (128-bit products for `lng`, 256-bit products from four 64-bit multiplies for `lll`, InfInt products), never overflowing. Mixed integer operands convert through the constructor. |
| `trunc`, `floor`, `ceil`, `round` | Return `T`: truncation, floor, ceiling and rounding half away from zero (`round(5/2) = 3`, `round(-5/2) = -3`, as `std::round`). Each fits `T` because its magnitude is at most `\|num\|`. |
| `fracPart(x)` | `x - floor(x)` in `[0, 1)`, through `raw` (the remainder stays coprime to `den`). |

### Floating, text and modular conversion

| Feature | Contract |
|---|---|
| `toDouble()`, `toLongDouble()` | Each rounds the exact value once to nearest, ties to even, at its own precision (53 or 64 bits), including subnormal results and overflow to infinity (`InfInt` only; bounded values lie in `[2^-127, 2^127]`); `toDouble` is not `double(toLongDouble())`, which can round twice (both kept by decision 2026-10-17). When both components of a bounded `T` fit the mantissa, one IEEE division of the exact operands rounds once (x87 precision must be the default 64 bits for `long double`, as on Linux and MinGW-w64). Otherwise: `Q = floor(\|num\| 2^s / den)` with `s = min(MANT + 1 - e, 1 - EMIN)` for `e = bitLength(\|num\|) - bitLength(den)`, so `Q` has `MANT + 1` or `MANT + 2` bits (fewer for subnormals) and the dropped part is one or two bits plus a sticky remainder bit; the integer `round(Q / 2^k)` with `k` in {1, 2} times `2^(s - k)` is exact in the target type. The quotient uses native 128-bit division for `lng`, a 256-by-128 shift-subtract loop for `lll` when the shifted numerator exceeds 128 bits, and InfInt division otherwise. |
| `read(string_view s, uint base = 10)` | `base` in `[2, 36]` (asserted; clamped otherwise). Grammar: optional sign, then `p/q` (digit strings, no signs or points inside), or digits with at most one radix point (`5`, `5.`, `.5`, `-2.25`) and, in base 10 only, an exponent `e`/`E`, optional sign, decimal digits (`1.5e-3`, `7E+2`); letters are digits from base 11 on, either case, so `e` is a digit in bases 15 and above. The value is exact. False and no change for malformed text, a zero denominator, a power of the base outside `[-2^31, 2^31)` after the radix-point shift and the removal of trailing zeros, or a value that does not fit `T`; for `lng` and `lll` also when `p`, `q` or the significand (all digits without trailing zeros) exceeds the unsigned type of `T`'s width. A negative power first cancels each prime of the base from the significand, so only the reduced denominator must fit: `512e-20` is `1/195312500000000000` and `0.5000000000000000000000` is `1/2` in `Frac`, while `1e-19` fails there and reads in `Rational<lll>`; `1000...0/1000...0` with 40 digits each is read only by `Rational<InfInt>`. For `InfInt` the power is formed exactly, at a cost of O(M(\|e\|)) time and O(\|e\|) memory (`1e2147483647` builds a 7-billion-bit integer, as Python's `Fraction` does). |
| `toString(uint base = 10)` | `p` or `p/q`, uppercase digits, `-` for negatives; `read` accepts the output in the same base. |
| `toDecimal(int k)` | `k >= 0` (asserted): the integer part, a point and exactly `k` digits (no point for `k = 0`), rounded half away from zero like `round`; a value that rounds to zero prints without a sign (`-1/1000` at `k = 2` is `0.00`). Digits come from ten modular additions per digit, so no intermediate exceeds `2 den`; cost O(k) word operations (InfInt operations for `InfInt`). |
| `toFloat<F>()` | The shared implementation of the two conversions, public only because the library has no `private`; it rounds correctly for any IEEE binary `F` (`float` too). |
| `toMod<Mint>()` | `num * den^-1` in any modular type with an integral constructor through 128 bits and `/` (`ModInt`, `ModInt64`, `DynModInt`, `DynModInt64`, the minis); `den` must be a unit modulo `Mint::mod()` (the modular type's `/` asserts it). `InfInt` components are reduced modulo `Mint::mod()` first. |
| `>>`, `<<` | `>>` reads one whitespace-delimited token through `read` in base 10 (failbit and no change when malformed; end of input leaves the value unchanged); `<<` writes `toString()`. InfInt's `SetBase` does not apply. |
| `std::hash<Rational<T>>` | Combines `std::hash<T>` of the canonical components (boost `hash_combine` mixing); equal values hash equally. |

### rational_detail kernels

`mulW`, `addW`, `subW`, `shlW`, `divW`, `narrowW`, `low128`, `bitLen`, `gcdM` take magnitudes: for `ulng` the wide type is `ulll`, for `ulll` a `(high, low)` pair of `ulll` (`mulW` from four 64-bit products, `divW` by a divisor below 2^127 natively when the high word is zero, else the high word natively and the low word by a 128-step shift-subtract loop), and for any other type (InfInt) the type's own operators, `divMod`, `bitLength` and ADL `gcd`. They are internal helpers, not API.

### Boundedness without assertions

Under `NDEBUG` every violated precondition stays bounded and free of undefined behaviour: overflowing arithmetic leaves the left operand unchanged, the constructor and the narrowing conversion map unrepresentable values and zero denominators to `0/1`, `raw` with `d <= 0` returns `0/1`, `inv(0)` and `pow(0, -k)` return `0/1`, bases are clamped, and `read` rejects powers outside `[-2^31, 2^31)` before forming them (for `lng` and `lll` the checked power stops at the first overflow).

### Operation decisions

Adopted from the 2026-10-17 sweep ([00-sources.md](00-sources.md)): `toMod` (Nyaan `to_mint`, FLINT `fmpq_mod_fmpz`), `raw` (Nyaan `raw`, maspypy's coprime flag), the converting constructor (Boost), `trySub` and `tryDiv` (`tryAdd(a, -b)` fails when `b.num` is the minimum), `toDecimal` (Python `format`; requested 2026-10-17 with decimal input). Decided 2026-10-17 ([duplicates.md](../../00-Guidelines/23-Sweep%20Ledgers/duplicates.md)): `toDouble` beside `toLongDouble`, `read`/`toString` with a base beside the streams; `parse` became `read`, the name InfInt uses. Not adopted: `height`, a rational `gcd`, `%`/`divMod` of rationals and `mulPow2` (rare; `num`/`den` and the operators express them), `limitDenominator`/best approximation, mediant, Stern–Brocot, continued fractions, simplest-between and fraction binary search (Mathematics `20-continued_fraction.hpp` owns them), `floorDiv`/`ceilDiv` (Mathematics `01-mod_arithmetic.hpp`), exact `sqrt` (no catalog; integer roots are Mathematics `21` and InfInt), `numerator()`/`denominator()`, comparison and arithmetic with integers, `min`/`max`, `++`/`--`, `operator bool`/`!`, `isOne`, `addmul`/`submul`, `as_integer_ratio` (the fields, the converting constructor and `std::` algorithms already say the same), a `den == 0` infinity (rationals exclude infinity, [00-notes.md](00-notes.md)).

## Feature-to-test map

`96-Local Testing/01-Core/09-rational_tester.py` builds `09-rational_tester.cpp` with `09-rational_tester_tu.cpp` (two translation units, every `T`) as optimized (`-O2 -DNDEBUG`), checked (`-O1 -g -D_GLIBCXX_ASSERTIONS`) and, from full mode, ASan/UBSan programs (`-Wall -Wextra -Wshadow -Wconversion -Werror`), runs the self-tests and streams the `--protocol` cases for `Rational<lng>`, `Rational<lll>` and `Rational<InfInt>` against Python's `fractions.Fraction` and exact integer models.

| Feature | Verification (self-test; protocol command) |
|---|---|
| Constructors and normalization (negative denominators, zero, `MIN/MIN`, `2/MIN`), `normalize` after a direct edit, `raw`, converting constructors in all six directions (only fitting values for narrowing), InfInt's integral constructor | identities; `nm` (pairs scaled by -6 to 3^39 and 2^69), `rw`, `cv` |
| `tryAdd`, `trySub`, `tryMul`, `tryDiv` (exact status against `fits(T, exact)`, operators against the try forms, the result unchanged after every false status, aliasing), bounds at `MAX`, `MIN`, `-1 - MAX`, `MIN / MIN`; constructed sums whose cross sum overflows the width before a large `gcd(t, g)` cancels it (`lll`: above 2^128, the 256-bit `mulW`/`divW` path) | identities; `ar` |
| `<=>`, `==` with mixed signs, equal values and cross products above 2^127 (`lll`) and 2^64 (`lng`); a sort of 3000 `Frac` against an independent 128-bit cross-product comparator | identities; `ar` |
| unary `-`, `abs`, `inv` (minimum and zero excluded by the protocol, asserted in probes), `sign`, `isInteger`, `floor`, `ceil`, `trunc`, `round` (ties both signs), `fracPart`, hash of a renormalized copy, unary `+` | `un` |
| `pow` with exponents -9 to 40 (results that fit), `0^0`, `LLONG_MIN` exponent, the minimum to the first power | identities; `pw` |
| `toDouble`, `toLongDouble`: an exact Python rounding model (checked against `float(Fraction)` for every case) over random values, 16,000-bit denominators (long double subnormals), 2^-1100 (double subnormals and zero), 2000-bit numerators (infinity), signed zero; fixed boundaries: 2^16384 and the largest long double, its rounding tie, 3^10400 and its inverse, `2^1024 - 2^970` (rounds to infinity) and `2^1024 - 2^971` (largest double), 2^-1075 (tie to zero), `3 / 2^1076`, ±`2^-16446` | `fl` |
| `fromDouble` from hexadecimal `double` (53-bit odd mantissas, exponents -140..79, -1100..999 for InfInt) and `long double` (64-bit mantissas, exponents 5000 and -9000 outside double range) literals | `fd` |
| `read`: canonical and scaled `p/q` in bases 2, 7, 10, 16, 36; radix-point forms with signs, empty integer or fraction parts and trailing zeros; base-10 exponents; 36 edge strings (empty, lone signs and points, two points, `1e`, `e5`, `1/`, `/2`, `1/0`, `1//2`, `1/-2`, `--1`, `1e5.5`, `1.5/2`, `0x10`, `inf`, `1e+`, `5.`, `-.5`, `+3/6`, `1e3` in base 16, `0e999999999`, `1e2147483648`, `0e-2147483648`, `3e-2147483649`, 40 trailing zeros, 40-digit `p/q`, `512e-20`, `2^-20` in decimal, `1e-19`, `1e-38`, base-2 fractions of 2^-31 and 2^-65); the self-tests add the prime cancellation in `Frac`; the model applies the bounded rule and is checked against `Fraction(s)` for every base-10 `InfInt` case | `rd` |
| `toString` in bases 2, 3, 8, 10, 16, 36; `toDecimal` with `k` in {0, 1, 2, 5, 17, 40} and carries into a new digit (`9.999` at 2, `-9.9995` at 3, `19/2` at 0), unsigned zero | identities; `ts`, `dc` |
| `toMod` for `mint`, `ModInt61`, `ModIntMini<1000000007>` and `DynModInt64` modulo 10^12 (composite), nonunits excluded | identities; `md` |
| Streams (`p/q`, decimal with exponent, failbit on `0.5/2`, end of input), `std::hash` with `unordered_set` | identities |
| Preconditions under the checked build (19 `--invalid` cases: zero denominator, unrepresentable constructor, `raw` denominator, `inv` of 0 and of the minimum, `pow(0, -1)`, `pow` overflow, division by zero, `+` and `*` overflow, unary minus and `abs` of the minimum, `fromDouble` of NaN and of an unrepresentable value, bases 37 and 1, `toDecimal(-1)`, a narrowing conversion, an InfInt sentinel) | `--invalid` |
| Header alone without InfInt (`Frac`, `Rational<lll>`), two translation units, `99-all`; compile-fail checks (floating constructor, floating second argument, floating operand, floating comparison, `Rational<int>`); `Poly<Frac>` product, `divMod`, `eval` and `deriv` against hand results | the driver's compile steps; `otherUnit` |

Quick runs 150 values per type (about 6,300 protocol cases), full 1500 per type (about 61,000) plus the sanitizer build (ASan quarantine capped at 64 MB), stress 5000 per type per `--rounds` seed. Line coverage of the header by the quick corpus is complete (gcov, 2026-10-17). Mutation check (2026-10-17, quick mode): each of 13 seeded faults (tie rule of `round`, the sticky bit, the `g2` cancellation, the sign of `<=>` for negatives, trailing-zero stripping, the decimal carry digit, the minimum's range check, the 128-bit shift, the last bit of the 256-bit division, the high word of the 256-bit product, the carry of the 256-bit sum, the guard bit of `s`, the native remainder) failed the suite.

## Commands and results

Final header, 2026-10-17 (GCC 16.2.1 and GCC 14.4.1, CPython 3.14, Intel Core i9-11900H; other worktrees' suites ran on the machine at the same time):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/09-rational.hpp '96-Local Testing/01-Core/09-rational_tester.cpp' '96-Local Testing/01-Core/09-rational_tester_tu.cpp' '96-Local Testing/01-Core/09-rational_benchmark.cpp'   # no violations
python3 '96-Local Testing/01-Core/09-rational_tester.py' --mode quick --seed 11                  # PASS, 2 configurations, 6,327 Python-oracle cases each, 46 s, MEMORY peak=524MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/09-rational_tester.py' --mode full --seed 20261017             # PASS, 3 configurations, 61,409 Python-oracle cases and 47 self-test checks each, 19 probes, 124 s, MEMORY peak=683MB cap=4096MB oom_kills=0
CXX=g++-14 python3 '96-Local Testing/01-Core/09-rational_tester.py' --mode full --seed 20261017   # PASS, the same counts, 139 s, MEMORY peak=687MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/09-rational_tester.py' --mode stress --seed 7 --rounds 2       # PASS, seeds 7 and 8: 204,475 and 204,509 cases per optimized and checked run, sanitized full 61,397 and 61,504, 136 s, MEMORY peak=685MB cap=4096MB oom_kills=0
python3 '96-Local Testing/02-integration.py'                                                     # PASS, 149 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, MEMORY peak=650MB
python3 '96-Local Testing/03-consistency.py'                                                     # no errors
```

Every run builds optimized (`-O2 -DNDEBUG`), checked (`-O1 -g -D_GLIBCXX_ASSERTIONS`) and, from full mode, ASan/UBSan (`-O1 -fsanitize=address,undefined`, quarantine 64 MB) programs with `-Wall -Wextra -Wshadow -Wconversion -Werror` and the second translation unit, and the driver's compile steps (header alone without InfInt, `99-all`, five compile-fail checks, the `Poly<Frac>` consumer); the pre-flight quick runs peaked at 524–555 MB under the 2048 MB cap. Line coverage (gcov, quick corpus): every line of the header executes.

## Benchmarks

`96-Local Testing/01-Core/09-rational_benchmark.py` (driver `09-rational_benchmark.cpp`): 11th Gen Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG -march=x86-64`, seed 20261017, uniform operands of the stated bit sizes, medians of 9 runs (3 for sorts, 5 for InfInt). `naive` is the usual unchecked contest fraction (full 128-bit cross products and one `std::gcd`), whose results are compared with Rational's before the record is written; it is not exact on overflow. Other worktrees were running test suites on the machine.

| T | Operation | Operand bits | Rational ns/op | Naive ns/op |
|---|---|---|---|---|
| lng | add | 8 | 36.5 | 72.1 |
| lng | mul | 8 | 51.2 | 66.7 |
| lng | sort | 8 | 89.8 | 81.6 |
| lng | toDouble | 8 | 6.5 | 1.2 |
| lng | add | 20 | 75.0 | 166.3 |
| lng | mul | 20 | 123.4 | 159.6 |
| lng | sort | 20 | 90.8 | 86.2 |
| lng | toDouble | 20 | 6.4 | 1.2 |
| lng | add | 30 | 103.0 | 239.5 |
| lng | mul | 30 | 178.5 | 232.4 |
| lng | sort | 30 | 94.2 | 86.2 |
| lng | toDouble | 30 | 6.4 | 1.1 |
| lll | tryAdd | 40 | 190.7 | — |
| lll | compare | 40 | 9.7 | — |
| lll | toDouble | 40 | 11.0 | — |
| lll | tryAdd | 60 | 262.4 | — |
| lll | compare | 60 | 9.3 | — |
| lll | toDouble | 60 | 29.5 | — |
| lll | tryAdd | 120 | 672.1 | — |
| lll | compare | 120 | 9.1 | — |
| lll | toDouble | 120 | 570.1 | — |
| InfInt | add | 256 | 2924.4 | — |
| InfInt | add | 2048 | 51076.3 | — |

Knuth's addition is about twice as fast as the textbook full cross product (smaller gcds) and the cross-cancelled product about 25% faster; sorting runs within 10% of the unchecked 128-bit comparator; `toDouble` takes one IEEE division when both components fit the mantissa (6 ns; the naive `double(p) / double(q)` is 1 ns but rounds twice for components above 2^53), and the 256-by-128-bit shift-subtract loop only for `lll` values whose shifted numerator exceeds 128 bits (570 ns at 120-bit operands). `Rational<lll>` additions of 120-bit operands mostly overflow and take the 256-bit path. Command: `python3 '96-Local Testing/01-Core/09-rational_benchmark.py' --output '96-Local Testing/01-Core/09-rational_benchmark.jsonl'` (2026-10-17, 3 min).

## Sources

GMP `mpq` and FLINT `fmpq` (canonical form, Knuth addition, cross-cancelled products, exact conversion), Python `fractions` (grammar of decimal and exponent input, `__round__`, `__format__`), Boost.Rational (converting constructor), and the Nyaan, maspypy, ei1333, suisen, hitonanode and tko919 rational headers; the full list with URLs is in [00-sources.md](00-sources.md).

## Limits and handoffs

- `read` on `lng` and `lll` follows the bounded component rule above; text whose unreduced components overflow is read only by `Rational<InfInt>`.
- P025 (`16-poly.hpp`): `Poly<Frac>` compiles and its product, `divMod`, `eval` and `deriv` match hand results (this tester's consumer step); overflow in `Frac` coefficients asserts. Rational coefficients are outside Poly's row domain until P025 adds them with Poly-suite tests ([16-poly.md](16-poly.md#limits-and-handoffs)).
- `Rational<InfInt>` costs the InfInt gcd per operation; long sums of InfInt rationals are dominated by gcds as in GMP.

## History

- 2026-10-17: P019 new header, tester and benchmark verified; 13 mutants killed; the review's findings (NDEBUG narrowing, unsupported `T`, floating arguments, doc bounds, test gaps) fixed, the base-prime cancellation in `read` and the `lng` comparison and `toDouble` fast paths added; full suites passed on g++ and g++-14, two stress rounds passed.
