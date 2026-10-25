# 07-infint.hpp — evidence

P018/C04 owns `07-infint.hpp`: the arbitrary-precision signed integer `InfInt` (alias `iint`), the fixed-width unsigned `UInt<N>` (alias `u256 = UInt<4>`), and the `infint_detail` word kernels they share. The header depends on `01-template.hpp` only; by decision (2026-10-16) it keeps its own three-prime Montgomery NTT, Goldilocks NTT and double FFT kernels instead of reusing Poly's, and no Core Barrett or Montgomery type. Prerequisites P002, P005 and P025 are verified.

## Contracts

### Representation and sentinel

`InfInt` is sign-magnitude: `int sgn` (`1` or `-1`), `bool is_inf`, `vector<uint> n` of base-2^32 limbs, little-endian, without leading zero limbs. Zero is `{0}` after any operation (a default-constructed value keeps `n` empty, also zero) with `sgn == 1`. Poly's `BigInt` concept reads `n`, `sgn`, `trim()` and `isNil()`; `trim()` restores the canonical form after a caller edits `n` directly. Kernels work on base-2^64 words (`infint_detail::W`) obtained by `memcpy` from the limbs (little-endian hosts only, `static_assert`ed), so a conversion of O(n) precedes every multiplication, division, gcd and radix call.

The signed infinity sentinel (kept by the duplicate sweep, 2026-10-16) is produced by `read`/the string constructor from `inf` in any letter case with an optional sign in bases 2–23 (in bases 24–36 the same letters are digits: `INF` is 24171 in base 36, so the sentinel does not round-trip through text there), by `fromDouble(±infinity)`, and by arithmetic. Finite semantics never involve it. Rules, with `x` finite, `p` finite nonzero: `inf + x = inf`, `inf + inf = inf`, `inf + (-inf) = 0`, `inf - inf = 0`; `inf * x = ±inf` for `x != 0`, `inf * 0 = 0`, `inf * inf = ±inf`; `inf / p = ±inf`, `x / inf = 0`, `inf / inf = ±1`; `inf % p = 0`, `x % inf = x`; `divMod` and `divModSlow` agree with `/` and `%`; `abs`, unary `-`, `+`, comparisons (`inf` above every finite value, `-inf` below, `inf == inf`), `++`/`--` (no change), `<<=`/`>>=` (no change), `pow` (`inf^0 = 1`, sign by parity), `sqrt(inf) = inf`, `gcd(inf, x) = |x|` for `x != 0` and `inf` otherwise, `exGcd` with cofactors `±1`/`0`, `lcm(x, inf) = inf` for `x != 0`, `toString` (`inf`/`-inf`), `toDouble` (`±infinity`), the native casts (saturating: `LLONG_MAX`/`LLONG_MIN`, `ULLONG_MAX`/`0`), `tryCast` (false), streams and `std::hash` accept it. Every other operation asserts finite operands (`divModFloor`/`Ceil`/`Euclid`, `divExact`, `removeFactor`, bitwise operators, bit queries, roots, `jacobi`, `kronecker`, `isProbablePrime`, `inv`, `powMod`, `toDigits`, `toLimbs`, `UInt` conversion, `newtonRaphsonInv`, sampling bounds); under `NDEBUG` each of these returns a bounded meaningless value (0, -1, false, the operand or an empty result) instead of looping or reading out of bounds, as do a zero divisor or modulus, a negative shift count or bit index, `pow` with a negative exponent, `removeFactor` by ±1, `kthRootRem` with `k < 1`, bases outside the documented ranges and `UInt` shifts outside `[0, 64 N)`.

### Construction, assignment and conversion

| Feature | Contract |
|---|---|
| `InfInt()`, `InfInt(T)` for every integral `T` through `lll`/`ulll`, `operator=(T)` | Exact; `bool` converts to 0/1. |
| `InfInt(string_view s, uint base = 10)` | Asserts `read` succeeds. |
| `InfInt(const vector<uint> &digits, uint base = 10, int sign = 1)` | Digits most significant first, each `< base`, `base` in `[2, 2^32)` (asserted), `sign` is `1` or `-1`; empty digits give zero. |
| `InfInt(const UInt<N> &)` | The unsigned value, exactly. |
| `fromLimbs(vector<uint>, sign)`, `fromLimbs(vector<ulng>, sign)` | Little-endian magnitude limbs (base 2^32 or 2^64), leading zeros allowed, sign `1`/`-1` (zero stays `+0`). |
| `toLimbs<uint>()` (default), `toLimbs<ulng>()` | Little-endian magnitude, no leading zeros, `{0}` for zero; finite only. |
| `read(string_view s, uint base = 10)` | `base` in `[2, 36]` (asserted). Accepts one optional `+`/`-`, then either `inf` (any case, bases 2–23 only) or one or more digits `0-9`, `A-Z`, `a-z` below the base (leading zeros allowed). Returns false and leaves the value unchanged for any other string, including empty, lone sign, embedded whitespace, `infinity`; a successful parse replaces the sentinel. Power-of-two bases parse in O(L) (hex through 8-digit SWAR and an AVX2 32-digit kernel, which validate the digits as they go: a bad character makes `parsePow2` return no limbs); one-word values (at most 12 digits, or 19 decimal digits; user decision 2026-10-25: the 12-digit cut stays for every non-decimal base) accumulate directly; longer decimals go through 19-digit blocks with an 8-digit SWAR parse, other bases through Horner blocks, both then building the value by the subquadratic block tree. |
| `toString(uint base = 10)` | `base` in `[2, 36]` (asserted); uppercase digits, `-` for negatives, `0` for zero, `inf`/`-inf`. Power-of-two bases print in O(L) (hex SWAR/AVX2, one reversed 8- or 32-byte store per group); one-word values print directly (`std::to_chars` for decimal); other bases through the divide-and-conquer block split with cached powers and reciprocals, decimal with a 4-digit table. |
| `toDigits(uint base)` | `base >= 2` (any `uint`), digits most significant first, `{0}` for zero, magnitude only; finite only. The inverse of the digit constructor. |
| `tryCast<T>(T &out)` | `T` integral through 128 bits. True iff the value is representable (signed minima included), then stores it; otherwise `out` is unchanged. False for the sentinel. |
| `explicit operator lng()`, `explicit operator ulng()` | Finite values reduce modulo 2^64 (two's complement wrap, like a C cast); the sentinel saturates. |
| `explicit operator bool()`, `isNil()` | Nonzero test and its negation (both kept by decision 2026-10-16); the sentinel is nonzero. |
| `toDouble()` | Correctly rounded to nearest, ties to even (a 64-bit window with a sticky bit); magnitudes at or above 2^1024 after rounding give `±infinity`. |
| `fromDouble(double d)` | `d` not NaN (asserted). The integer part toward zero, exactly (`1e300` reproduces its binary value); `±infinity` give the sentinel. |
| Streams | `>>` reads one whitespace-delimited token through `read` in the stream's base (`SetBase(b)` manipulator stored in a per-stream `iword`, default 10; `std::hex` and `std::oct` flags override it); a malformed token sets `failbit` and keeps the destination; end of input leaves the destination unchanged. `<<` prints `toString` in the stream's base. `getBaseId()` is the `iword` index. |
| `std::hash<InfInt>` | Mixes sign, sentinel flag and limbs (empty and `{0}` zero hash equally; `fromLimbs` trims, so padded limbs hash as the value). |

### Arithmetic and division

| Feature | Contract |
|---|---|
| `+=`, `-=`, `+`, `-`, `++`, `--`, unary `+`, unary `-`, `abs` | Exact; `a -= a` and `a += a` are allowed. |
| `*=` (`InfInt`), `*` | Exact; `a *= a`, `a * a` and equal operands take the squaring backends; one-word operands take the word multiply; the dispatcher chooses basecase, Karatsuba, Toom-3 or a transform by the measured thresholds below, and in the transform range cuts the longer operand into chunks that fill their transforms when that costs less (`infint_detail` kernels). |
| `*=`, `*` with a native integer (`isScalar`: integral types of at most 64 bits except `bool`) | Exact, O(n); powers of two shift. |
| `/=`, `%=`, `/`, `%` (`InfInt` and scalars), `divMod` | Truncating division, divisor nonzero (asserted); the remainder keeps the dividend's sign and `\|r\| < \|b\|`; `a /= a = 1`, `a %= a = 0`. `divMod` on one-word operands divides natively (the scalar `/=` and `%=` keep the word division); a power-of-two divisor (`popcount(b) == 1`) shifts and masks in O(n); other word divisors use Möller–Granlund reciprocal division, longer ones the dispatcher (Knuth D below `BURNIKEL` words or for short quotients, Burnikel–Ziegler above). |
| `divModSlow(a, b)` | The same results through Knuth D only (the schoolbook backend), kept as a separate algorithm. |
| `divModFloor(a, b)` | Floor division pair: remainder with the divisor's sign (`.first` and `.second` are the floor quotient and remainder; there are no separate single-result forms). |
| `divModCeil(a, b)` | Ceiling division pair: remainder with the opposite sign of the divisor. |
| `divModEuclid(a, b)` | Quotient and remainder with the remainder in `[0, \|b\|)`. |
| `divExact(a, b)` | `b != 0` and `b \| a` (asserted modulo 2^61 - 1 after the fact; an inexact input returns a meaningless value under `NDEBUG`). After removing the common power of two: word-by-word Hensel (2-adic) division for quotients of at most `HENSEL` words, the quotient-only Burnikel–Ziegler division above (measured 1.3–2x faster than Newton–Hensel lifting at every larger size). |
| `removeFactor(x, f)` | `x != 0`, `\|f\| >= 2` (asserted). Divides `x` by `f` while divisible and returns the count (0 when not divisible); negative `f` flips the sign per removed factor; powers of two use shifts. |
| `newtonRaphsonInv(a)` | `a > 0` with the top bit of its top limb set (asserted). Returns `floor(2^(64 n) / a)` for `a` of `n` limbs, exactly (`n + 1` limbs). |

### Number theory

| Feature | Contract |
|---|---|
| `gcd(a, b)` | Nonnegative; `gcd(0, 0) = 0`. Backends by size: native `std::gcd` below two limbs, `gcdBinary` up to `BINARY_GCD` words, Lehmer steps (`lehmerHalfGcd`) and divisions in between, recursive `halfGcd` above `HALFGCD` limbs. |
| `exGcd(a, b, x, y)` | Returns `g = gcd(a, b)` with `a x + b y = g`; when `a` and `b` are nonzero, `\|x\| <= \|b\| / g` and `\|y\| <= \|a\| / g` (the Euclidean cofactors; half-GCD and Lehmer matrices reproduce the same quotient sequence). |
| `lcm(a, b)` | Nonnegative; 0 when either is 0. |
| `inv(a, mod)` | `mod != 0`; the inverse of `a` modulo `\|mod\|` in `[0, \|mod\|)`, `-1` when `gcd(a, mod) != 1`, `0` for `\|mod\| == 1`. |
| `pow(a, e)` | `e >= 0` (asserted), square-and-multiply with the squaring backends. |
| `powMod(a, e, mod)` | `mod != 0`; `a^e` modulo `\|mod\|` in `[0, \|mod\|)`; `e < 0` uses `inv` and returns `-1` when `a` is not a unit; `\|mod\| == 1` gives 0. Sliding windows of a cost-chosen width (1 to 8 bits); one-word moduli use native 128-bit products, odd multiword moduli Montgomery REDC with a word inverse, even moduli reduction by a reciprocal cached above `NEWTON_DIV` words. |
| `sqrt(a)`, `isqrtRem(a)` | `a >= 0` (asserted; `sqrt(inf) = inf`). Floor root and `a - root^2` by Zimmermann's recursive square root with remainder (the Karatsuba-square-root backend) on words after an even normalising shift; `sqrt` stays beside the pair because it accepts the sentinel. |
| `kthRootRem(a, k)` | `k >= 1`; `a < 0` only for odd `k` (asserted). Truncated root toward zero and `a - root^k` by integer Newton iteration from a floating-point estimate of the root with a 2^-20 relative margin (so the number of iterations is logarithmic in the bit length for every `k`), finished by an exact `(root + 1)^k` check. |
| `isPerfectSquare(a)` | False for negatives, true for 0; residue filters modulo 64, 63, 65 and 11 before the root. |
| `isPerfectPower(a)` | True for 0, ±1 and every `x^k` with `k >= 2` (odd `k` only for negatives). Even values are tested only at the exponents dividing the power of two; odd values filter each prime exponent `p` through up to two primes `q ≡ 1 (mod p)` below 2^31 (`a^((q-1)/p) mod q`) before a root. |
| `jacobi(a, m)` | `m` odd positive (asserted); the binary Jacobi algorithm (native for one-word `m`). `kronecker(a, m)` extends it to every finite `m` (`m = 0` gives 1 iff `\|a\| = 1`; even and negative `m` by the standard rules). |
| `isProbablePrime(a)` | False below 2. Deterministic Miller–Rabin (seven bases) below 2^64. Above: trial division by the primes below 140 (three word products and `gcd`), perfect-square rejection, a strong base-2 Miller–Rabin test and a strong Lucas test with Selfridge's parameters (D = 5, -7, 9, ...; P = 1, Q = (1 - D) / 4), i.e. Baillie–PSW. Monte Carlo: composites could pass only if a BPSW pseudoprime exists; none is known. |
| `sample(bound, rng)` | `bound > 0` finite (asserted); uniform on `[0, bound)` by rejection on `bitLength(bound)` random bits (expected under two draws). `rng` is `std::mt19937`, `std::mt19937_64` or another generator with `min() == 0` and `max()` equal to 2^32 - 1 or 2^64 - 1 (checked at compile time); 32-bit generators are combined in pairs. Results are reproducible from the caller's seed. |
| `sampleBits(bits, rng)` | `bits <= 0` gives 0, else a uniform value with exactly `bits` bits (top bit set). |

### Bits and shifts

Mixed operands: `gcd(a, 18)` and `lcm(a, 18)` do not compile because `std::gcd` is the better match under `using namespace std`; write `gcd(a, InfInt(18))`.

Two's-complement semantics on finite values: `&`, `|`, `^`, `~` (`~a = -a - 1`), `testBit`, `setBit`, `clearBit`, `flipBit`, `scan0(a, i)` / `scan1(a, i)` (index of the first clear / set bit at or after `i >= 0`, or `-1` when none exists: no set bit above a nonnegative value, no clear bit above a negative one), `>>` and `>>=` (floor division by 2^s, so `-1 >> k == -1`), `<<` and `<<=` (exact products; shift counts `>= 0` asserted). Magnitude semantics: `ctz` (0 for zero), `popcount` (of `|a|`), `bitLength` (of `|a|`, 0 for zero). `setBit`/`clearBit`/`flipBit` mutate in place and return the object; on nonnegative values they are O(1) amortised, on negatives O(n).

### UInt<N>

`UInt<N>` holds `array<ulng, N> w` little-endian with wraparound modulo 2^(64 N) in every operation: constructors from any integral type (negative values and signed 128-bit types sign-extend), from `InfInt` (two's-complement image of a finite value) and from a string (`UInt(string_view, base)`, which parses as `InfInt` does and asserts validity); `+ - * / % & | ^ ~`, unary `-`, `<< >>` (counts in `[0, 64 N)` asserted, bits leave at the ends; other counts give 0 under `NDEBUG`), the compound forms, `divMod` (divisor nonzero, asserted), `divWord(d)` (in-place quotient by a nonzero word, returns the remainder), `mulHigh` (the high `N` words of the `2N`-word product), `<=>`, `==`, `bitLength`, `ctz`, `popcount`, `toString(base)`, `explicit ulng` (low word), `explicit bool`, `<<` to a stream, `std::hash`. Products are schoolbook (`truncatedMul` for the low half, `mulBasecase` for `mulHigh`), division is the shared word division.

### infint_detail kernels

All kernels take little-endian base-2^64 words and sizes in words; results never alias inputs unless stated. Multiplication: `mulBasecase` (schoolbook), `squareBasecase`, `karatsuba(r, a, n, b, m)` for `n >= m > n / 2`, `karatsubaSquare`, `toom3`/`toom3Square` (equal lengths `>= 6`, Bodrato evaluation at 0, 1, -1, 2, ∞ with exact division by 3), `unbalancedMul(r, a, n, b, m, c)` (chunks of `c` words of `a` times `b`, each through the dispatcher), `truncatedMul` (`a b mod B^n`), `middleProduct(r, a, na, b, nb, lo, cnt)` (`r` = words `[lo, lo + cnt)` of `a b` summed over the terms with `i + j >= lo` only, so the exact words exceed `r` by less than `min(na, nb) B` at word `lo`; a direct sum below `MIDDLE`, the exact window of the full product below `FFT`, and above it a wrapped cyclic transform of `max(lo + cnt, na + nb - 1 - lo, na, nb)` words (never shorter than an input, so no input folds), which `reciprocal` uses with two guard words for the residual `B^(m + h) - b xh`), `transformMul` through `nttCyclic`, which chooses `fftCyclic` (double, 16-bit pieces, exact while the shorter operand has at most `FFT_DOUBLE_MAX = 2^14` words, i.e. coefficients below 2^48, and the length at most 2^26 limbs; the 2026-10-25 review measured a maximum rounding error of 0.125 for all-ones operands at 2^18–2^24 pieces, flat in length; scalar builds only), `ntt3Cyclic` (primes 2013265921, 1811939329, 469762049 with Montgomery arithmetic, 8-lane AVX2 kernels under `__AVX2__`, length at most 2^26 limbs, coefficients below 2^89) or `ntt64Cyclic` (2^64 - 2^32 + 1 on 16-bit pieces, scalar) for the lengths above 2^26 limbs up to `NTT_MAX_WORDS = 2^28` words. `mul`/`square` dispatch by the thresholds `KARATSUBA`, `KARATSUBA_SQR`, `TOOM3`, `TOOM3_SQR`, `FFT`, `FFT_SQR`; from `FFT` words `mul` scores the whole padded transform (allowed by the padding rule, kept with a 10% edge for the chunked variants' extra passes) against cutting `a` into chunks of `s - m` words for every power of two `s > m` below the padded length (a chunk shorter than `FFT` words re-enters the dispatcher through `mulAny` and takes Karatsuba or Toom-3), each chunk costing `transformWork` (transform limbs times log length) with the last chunk at its own length, and runs the cheapest through `unbalancedMul`; below `FFT` an `n >= 2 m` product uses `m`-word chunks. Division: `invWord`/`div2by1` (Möller–Granlund), `divWord`, `divBasecase` (Knuth D on a normalised divisor of `>= 2` words), `div2n1n`/`div3n2n`/`divBurnikelZiegler` (block recursion padded to `bs 2^k` words), `reciprocal` (exact `floor(B^(2m) / b)` by recursive Newton on the top half, the residual through the guarded middle product, and a final correction), `divNewton` (block division by a cached reciprocal), `divRem` (normalises, then Newton when a reciprocal is passed or, from `NEWTON_LONG` divisor words, when the quotient has at least four times the divisor's words; Knuth D below `BURNIKEL` words or quotients, Burnikel–Ziegler otherwise), `invMod64`, `divExactHensel` (word-by-word 2-adic division, quadratic). GCD: `gcdBinary`. Caches: the NTT root tables, the FFT twiddles and the radix powers/reciprocals (`InfInt::radix`) are function-local statics that grow to the largest size used and live for the process; they hold no per-value state, so nothing invalidates them. `schonhageStrassen(r, a, n, b, m)` (n >= m >= 1, n + m <= 2^30, r of n + m words, which may alias a or b: the inputs are split before r is cleared) is the Fermat-ring product: K = 2^k pieces of p words with k = (bit_width(n + m) + 1) / 2 and p = (n + m) / K + 1, convolved over Z/(2^N + 1) with N = 64 L, L = roundUp(2p + 1, max(1, K / 128)) words, so that 2^(2N / K) is a principal K-th root (K | 2N), every coefficient is below K 2^(128 p) <= 2^N and ceil(n / p) + ceil(m / p) - 1 <= K pieces never wrap; `Fermat` holds the ring (elements of L + 1 words valued in [0, 2^N]: `add`, `sub`, `neg`, `shift` by 2^s for s < 2N with the overflow negated, `reduce` of a product, and the radix-2 `transform`, decimation in frequency forward and in time back with the 1 / K = 2^(2N - k) scaling), the pointwise products go through `mulAny` and the coefficients are added at word offsets j p. It is not dispatched (Limits).

### Legacy mapping

| Removed (P018, 2026-10-17) | Replacement |
|---|---|
| `init(digits, base, sign)`, `initFromBlks` | the digit-vector constructor (`a = InfInt(digits, base, sign)`); `read` stays as the one in-place parser |
| `operator=(string_view)` | `a.read(s)` or `a = InfInt(s)` |
| `operator!` | `!a` through `explicit operator bool` (kept beside `isNil`) |
| `bin(a)`, `oct(a)`, `hex(a)` | `a.toString(2)`, `a.toString(8)`, `a.toString(16)` |
| `rand(bits)` (hidden time seed) | `InfInt::sampleBits(bits, rng)` with the caller's generator |
| `convTwosComp`, `fromTwosComp` | `toLimbs()` with `sgn`, `fromLimbs`; the two's-complement image is `InfInt::twosComplement`/`fromTwos`, helpers of the bitwise operators |
| `shiftBlocksLeft`, `shiftBlocksRight` | `<<=`/`>>=` by `32 k` bits |
| `FastIintMul` namespace, `InfInt::ntt`, `modP`, `nttPow`, `karatsuba`, `karatsubaSquare` members, `getBpow2`, `addMag`/`subMag`/`rsubMag` (public helpers) | `infint_detail` kernels (`ntt3Cyclic`, `Ntt64`, `karatsuba`, ...) and `InfInt::radix`; the limb add/sub helpers stay as members |
| `divMod(a, b, const InfInt *b_inv)` third argument | `divMod(a, b)`; cached reciprocals are internal (`ModCtx`, radix) |
| `divFloor`, `remFloor`, `divCeil`, `remCeil`, `divEuclid`, `remEuclid` | `divModFloor(a, b).first` / `.second` and the ceiling and Euclidean pairs |
| `kthRoot(a, k)` | `kthRootRem(a, k).first` |
| `toString`/`read` bracket format `[d, d, ...]` for bases above 16 | `toDigits(base)` and the digit constructor for bases above 36 |
| `isIdMat`, `leftEucMul`, `backEucMul`, `leftMatMul`, `adjustHalfGcd` | `isIdentity`, `eucStep`, `eucBack`, `matApply`, `adjust` (half-GCD helpers, not operations) |

Row items settled by the sweep (`duplicates.md`) and the review: `divTrunc`/`remTrunc` are `/` and `%`; `divFloor`/`remFloor`/`divEuclid`/`remEuclid` are the projections of the `divMod*` pairs; `kthRoot` is `kthRootRem(a, k).first`; `fitsLng`/`fitsUlng` are `tryCast`'s status; base 2–36 parse/print is the domain of `read`/`toString`; `UInt::fromString` is the `UInt(string_view, base)` constructor; `isDivisible(a, b)` is `(a % b).isNil()`; `hammingDistance(a, b)` is `popcount(a ^ b)`; `schoolbook (Knuth D)`, `newtonReciprocal`, `radixDivideAndConquer`, `quotientOnly`, `remainderOnly` are `divModSlow`/`divBasecase`, `newtonRaphsonInv`/`reciprocal`, `fromBlocks`/`toBlocks`, `/`, `%`.

## Feature-to-test map

`96-Local Testing/01-Core/07-infint_tester.py` builds `07-infint_tester.cpp` with `07-infint_tester_tu.cpp` (two translation units) in optimized scalar, checked scalar and AVX2/FMA/BMI2 configurations (full and stress add ASan/UBSan scalar and AVX2), runs the C++ self-test phases, then the `--protocol` differential run against Python big integers (`build_cases` generates cases lazily and `run_protocol` streams them through one C++ process, validating each answer as it arrives, so the corpus never sits in memory), and the `--invalid` precondition probes on the checked build. Oracles: `omul` (limb schoolbook product) for every product, division and gcd identity; native `lng`/`lll`/`ulll` arithmetic for the exhaustive small range and `UInt<2>`; a two's-complement bit model; sieves and trial division; Python `int`, `math.gcd`, `math.isqrt`, `pow`, a Python Jacobi/Kronecker and a 20-base Miller–Rabin.

| Feature | Verification (self-test phase; protocol command) |
|---|---|
| Integral constructors, `operator=`, `tryCast` (all widths, signed minima, 128-bit), `lng`/`ulng` casts (wrap and sentinel saturation), `explicit bool`, `isNil`, `trim` (zero limbs, negative zero, empty), postfix `++`/`--`, `getBaseId`, zero forms | construction; `trycast`, `limbs`, `neg`, `inc`, `dec` |
| `fromLimbs`/`toLimbs` (32/64, padded limbs, trimming), `fromDouble`/`toDouble` (exact scaling, ties to even, sticky, overflow to infinity, truncation, 1e300) | construction; `limbs`, `todouble` (Python `float(int)`), `fromdouble` (`int(d)`) |
| `read`/string constructor in bases 2–36 (upper and lower case, signs, leading zeros, rejections: empty, lone sign, bad digit, digit at or above the base, embedded space, `infinity`; the `inf` token only in bases 2–23 and `INF` as the base-36 numeral 24171; a long decimal, a hex string and a stream token replacing the sentinel), `toString` 2–36, hex/binary/octal/base-32 fast paths (`parsePow2`, `appendPow2`), every byte that is not a hex digit (and not a sign at the first position) at every position of random mixed-case hex strings of lengths 1–100 and every 97th position of a 4097-digit string in full mode, at two positions in quick mode (AVX2 32-digit, SWAR 8-digit and tail groups, each against a Horner oracle; a rejection keeps the value and the sentinel), digits at the base in bases 2, 8 and 32, the decimal `parse8`/`ITOS4` tables, large (up to 2500 words) round trips through `fromBlocks`/`toBlocks`, `toDigits` and the digit constructor for bases 2, 10, 1000, 65536, 10^9+7, 2^32-1 | construction and infinity (round trips against the value, rejections keep the value); `tobase`, `frombase` (Python `to_base`), `todigits`, `fromdigits` (Python `digits_of`) |
| `+`, `-`, `*`, `/`, `%`, compound forms, `++`/`--`, unary, `abs`, `magCmp`, `<=>`, `==`, `&`, `\|`, `^`, `~`, shifts 0–70, `divMod`, `divModSlow`, `divModFloor`/`divModCeil`/`divModEuclid`, `divExact`, `gcd`, `lcm`, `exGcd` (Bezout), `inv`, `jacobi` (Euler criterion for primes), `removeFactor`, `pow` | small exhaustive (`[-40, 40]^2`, native `lng` oracle); `add`, `sub`, `mul`, `cmp`, `div`, `divslow`, `divfloor`, `divceil`, `diveuclid`, `divexact`, `gcd`, `lcm`, `exgcd` (identity and cofactor bounds), `inv`, `removefactor`, `pow` |
| Multiplication: `*`, `*=`, squaring (same object, equal operands), every word size around `KARATSUBA`, `TOOM3`, `FFT` and the square thresholds, unbalanced shapes (always `2 FFT + 100` × `FFT + 1`, a whole transform, and `4 FFT + 3` × `FFT`, `3 FFT`-word chunks, so quick mode reaches the estimator in both builds; in full mode also 16391×1025, 12000×700, 9000×2100, 4100×600, 2600×1300, 1300×1000, 7000×2100, 16391×2100 and 20000×2048 words; the first six take the transform-range paths in the AVX2 build, four in chunks and 9000 × 2100 and 2600 × 1300 whole, the last three take chunks in both builds), all-ones operands up to 20000 words (stress), scalar products including powers of two | multiplication (oracle `omul`); `mul`, `muls`, `mulk basecase/karatsuba/toom3/unbalanced/transform/ssa/square/karatsubasquare/toom3square/truncated/middle/dispatch` |
| Kernels `mulBasecase`, `karatsuba`, `karatsubaSquare`, `toom3`, `toom3Square`, `squareBasecase`, `unbalancedMul` (chunks of 1, 2, m/2 + 1, m - 1, m, m + 1, the estimator's split, n/3 + 1, n - 1, n and n + 5 words at every shape with n >= 2m), `transformMul`, `schonhageStrassen` (every shape up to 20000 words and the squares, random and all-ones; protocol kind `ssa`), `truncatedMul`, `middleProduct` (random `na`, `nb`, `lo`, `cnt`; error bound `< min(na, nb) B`), `ntt3Cyclic`, `ntt64Cyclic`, `fftCyclic` (each backend directly, random and all-ones limbs up to 2000 words), the double FFT at its exactness limit (all-ones 16384×16384 and 32768×16384 words against the schoolbook oracle, full mode) | multiplication |
| Division: `divMod` against `q b + r` built with `omul` for quotient/divisor shapes across `BURNIKEL` and `NEWTON_DIV`, three divisor patterns (random, all ones, short top word), four remainders and four sign combinations, divisors `±2^k` (`k` in 0–65 and random up to 3000, dividends random, zero, `±b`, `b - 1` and of up to `k + 1` bits) against `divModSlow` with the identity, bound and sign checks, `divBasecase`, `divBurnikelZiegler`, `reciprocal` (exact floor, through the guarded middle product), `divNewton`, `divModSlow`, `newtonRaphsonInv` (floor bound), `divExactHensel`, `divWord` and scalar `/`, `%` (ulng, lng, powers of two), `a /= a` and `a %= a` | division, aliasing; `div`, `divslow`, `divs`, `divk basecase/bz/newton/hensel/dispatch`, `newton` |
| Rounded modes and `removeFactor` on random signed values | division (identities `q b + r = a`, remainder sign and bound); protocol as above |
| `gcd`, `exGcd`, `lcm`, `inv` on constructed common factors across `BINARY_GCD` and `HALFGCD` (4500-limb operands in full mode reach the half-GCD recursion), `gcdBinary`, `halfGcd` matrix properties (rows, unimodular, ordering, halving, gcd kept), `lehmerHalfGcd` matrix rows, 64-bit random pairs against `std::gcd` | gcd; `gcd`, `exgcd`, `lcm`, `inv`, `gcdk binary/halfgcd/lehmer` |
| `powMod` (window widths 1–8, odd and even moduli from 2 to 4000 bits, an 800-word even modulus through the cached reciprocal in full mode, negative modulus, all-ones and sparse exponents, exponent 0 and 1, negative exponents with and without an inverse, modulus 1) against a plain square-and-multiply oracle | powMod; `powmod` (Python `pow`) |
| `isqrtRem`, `sqrt`, `kthRootRem` (identities `s^k <= a < (s + 1)^k`, odd roots of negatives, `k = 1, 2`), all-ones inputs and their squares, `isPerfectSquare`, `isPerfectPower` (constructed powers, `-300..300` brute force, `2^1000`, `3^101`) | roots; `sqrt`, `kthroot` (Python `isqrt`/`iroot`), `issquare`, `ispower` |
| `isProbablePrime`: sieve to 200,000 (full), Carmichael and strong pseudoprimes, 2^64 boundary values, Mersenne primes and composites, Fermat F7, random semiprimes and prime squares; `jacobi` multiplicativity, periodicity and factor rules; `kronecker` against the definition for `[-60, 60]^2` | number theory; `isprime` (Python Miller–Rabin), `jacobi`, `kronecker` |
| Bits: `popcount`, `bitLength`, `ctz`, `testBit`, `scan0`, `scan1`, `setBit`, `clearBit`, `flipBit` against a two's-complement bit model, de Morgan and shift identities | bits; `bitlength`, `ctz`, `popcount`, `testbit`, `setbit`, `clearbit`, `flipbit`, `scan0`, `scan1`, `shl`, `shr`, `and`, `or`, `xor`, `not` |
| `sample` (range, uniformity within 5 sigma over 97 buckets, reproducibility, 32-bit generators), `sampleBits` (exact length), `std::hash` (equal values, trimmed limbs, sentinel, at most 2 collisions among 200,000 distinct values), streams (`SetBase`, `std::hex`, failbit, EOF, 20000-bit round trip) | sampling, hash, streams; `sample`, `hash` |
| Sentinel rules for every supported operation and the clearing by assignment, `read` (short and long decimal, hex) and stream input | infinity |
| `UInt<1>`, `UInt<2>`, `UInt<4>`: every operator, `divMod`, `divWord`, `mulHigh`, shifts, comparisons, bit queries, `toString` round trips, integral and `InfInt` conversions, self-aliasing, `UInt<2>` against `ulll` | UInt<N> (oracle: `InfInt` modulo 2^(64 N) and native 128-bit); `u256 add/sub/mul/div/mod/mulhigh/and/or/xor/not/shl/shr/cmp/divword/tostr/parse` |
| Aliasing: `a += a`, `a -= a`, `a *= a`, `a /= a`, `a %= a`, bitwise self-assignment, `exGcd(a, a)`, moved negation, vector elements | aliasing |
| Preconditions under the checked build (35 `--invalid` cases: zero divisors and moduli, inexact `divExact`, sentinel operands, negative shifts and indices, negative even roots, `k = 0`, even Jacobi modulus, negative exponent, bases outside 2–36 or below 2, malformed string and digit constructors, bad sign, NaN, zero sampling bound, `removeFactor` by 1, unnormalised `newtonRaphsonInv`, `UInt` shifts, zero word divisor and divisor, sentinel conversions) | `07-infint_tester.py` death cases |
| Two translation units, header alone, `99-all` aggregate, Poly consumer (`16-poly_tester.py` quick with `Poly<InfInt>`) | `otherUnitSum`; the driver's compile steps; the consumer command below |

Quick runs the small corpora (seconds); full runs every phase at the listed sizes with 1.4 million C++ checks and about 20,000 protocol cases per configuration; stress adds 20,000-word products, 12,000/6,000-word divisions, 6,000-limb gcds and triples the protocol corpus with operands up to 150,000 bits, repeating `--rounds` times with consecutive seeds.

## Commands and results

Final header, 2026-10-17 (GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7, Intel Core i9-11900H; the suites, the benchmark, the integration build and the Poly consumer run ran beside each other):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/07-infint.hpp '96-Local Testing/01-Core/07-infint_tester.cpp' '96-Local Testing/01-Core/07-infint_tester_tu.cpp' '96-Local Testing/01-Core/07-infint_benchmark.cpp'   # no violations
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-infint --no-integration                # PASS, 3 configurations, 88 s
python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode full --seed 20261017                   # PASS, 5 configurations, 1059 s, MEMORY peak=3553MB cap=4096MB oom_kills=0
CXX=g++-14 python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode full --seed 20261017        # PASS, 5 configurations, 1039 s, MEMORY peak=3631MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode stress --seed 7 --rounds 1             # PASS, 5 configurations, 1524 s, MEMORY peak=3527MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode quick --seed 1                           # PASS (consumer smoke, Poly<InfInt>), 3 configurations, 51,261 checks each, 387 s, peak 1318 MB
python3 '96-Local Testing/02-integration.py'                                                         # PASS, 148 headers, scalar/AVX2 multi-TU, workspace, 652 MB
python3 '96-Local Testing/02-integration.py' --sanitizers                                            # PASS (earlier the same day on the pre-review header), plus the ASan/UBSan quick self-tests of every Core tester, 460 s, peak 1829 MB
python3 '96-Local Testing/03-consistency.py'                                                         # no errors
python3 '96-Local Testing/01-Core/07-infint_benchmark.py' --seed 20261017 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/07-infint_benchmark.jsonl'   # 2,100 verified measurements, 94 s scalar + 88 s AVX2
```

After these runs the four `infint_detail` blocks were merged into one (six namespace lines removed, no other change); the compile matrix (both compilers, scalar/AVX2, optimized/checked, `-Werror`), the brace validator, the quick suite (3 configurations, 4,105 Python-oracle cases each) and the benchmark were rerun on the merged header, which is the committed one.

Each full run builds optimized scalar (`-O2 -DNDEBUG`), checked scalar (`-O1 -D_GLIBCXX_ASSERTIONS`), optimized AVX2/FMA/BMI2 and the two ASan/UBSan builds, all with `-Wall -Wextra -Wshadow -Wconversion -Werror` and the second translation unit; the pre-flight quick run peaks at 730 MB under the 2048 MB cap (the compiler dominates). Per configuration the self-test makes 1,380,277 checks (1,380,337 with AVX2, where the hex kernels add cases) in 31–42 s optimized and 137–164 s sanitized, the protocol validates 20,241 Python-oracle cases streamed through the process, and the checked build runs the 35 precondition cases. The stress round (seed 7) raises the self-test to 1,382,233 checks and the protocol to 61,024 cases per optimized or checked configuration (operands to 150,000 bits, 20,000-word products, 12,000/6,000-word divisions, 6,000-limb gcds), while the sanitized configurations execute the full-mode lists (1,381,456 checks, 20,333 cases). The GCC 14 floor run is identical in counts. The memory peaks are those of the sanitized builds; the optimized self-test stays below 30 MB.

Re-review and `/finish`, 2026-10-25 (GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7, Intel Core i9-11900H; the suites ran one after another on the committed header, the judge timings and the benchmark afterwards on the idle host):

```bash
python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode quick --seed 1                         # PASS, 3 configurations, 80 s (seed 5 likewise; seeds 2 and 3 by the reviewers)
python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode full --seed 20261025                   # PASS, 5 configurations, 911 s, MEMORY peak=3553MB cap=4096MB oom_kills=0
CXX=g++-14 python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode full --seed 20261025        # PASS, 5 configurations, 890 s, MEMORY peak=3446MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/07-infint_tester.py' --mode stress --seed 7 --rounds 1             # PASS, 5 configurations, 1318 s, MEMORY peak=3250MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/16-poly_tester.py' --mode quick --seed 1                           # PASS (consumer, Poly<InfInt>), 3 configurations, 272 s, peak 1324 MB
python3 '96-Local Testing/02-integration.py'                                                         # PASS, 177 headers, scalar/AVX2 multi-TU, workspace, peak 2292 MB
python3 '96-Local Testing/02-integration.py' --sanitizers                                            # PASS, plus the sanitizer self-tests, peak 2585 MB
python3 '96-Local Testing/01-Core/07-infint_benchmark.py' --seed 20261025 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/07-infint_benchmark.jsonl'   # 2,181 verified measurements appended (re-review header), peak 646 MB
python3 '96-Local Testing/01-Core/07-infint_benchmark.py' --seed 20261025 --repetitions 5 --milliseconds 2 --only mul --output '96-Local Testing/01-Core/07-infint_benchmark.jsonl'   # 511 verified measurements appended (committed header), peak 644 MB
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/07-infint.hpp '96-Local Testing/01-Core/07-infint_tester.cpp' '96-Local Testing/01-Core/07-infint_tester_tu.cpp' '96-Local Testing/01-Core/07-infint_benchmark.cpp'   # no violations
python3 '96-Local Testing/03-consistency.py'                                                         # no errors
```

Per configuration the full self-test makes 1,511,770 checks (1,511,966 with AVX2) in 26–42 s optimized and 121–177 s sanitized and the protocol validates 20,404 cases; the stress round makes 1,522,988 (1,523,026) checks and validates 61,324 cases, its sanitized configurations 1,522,075 (1,522,093) checks and 20,433 cases. The Library Checker reproduction (Benchmarks) ran on the re-review header, whose parse, print, product and division paths the `/finish` changes left untouched (the committed header adds the undispatched `schonhageStrassen`, lowers `NTT_MAX_WORDS` and shortens one comment). Reviewer probes, all uncommitted and all PASS: the package review's exhaustive 256-byte sweep at every position of 1–40-character hex strings plus 64–200-character strings and bases 2, 3, 4, 8, 10, 32, 36 against a Horner oracle (386,000 cases each in scalar, AVX2 and ASan/UBSan builds), `divMod` by ±2^k for k < 4000 and 200,000 word/word pairs against `divModSlow` and native division, 172 unbalanced shapes through `mul`, `operator*` and `unbalancedMul(..., c)` against `mulBasecase` in both builds, the double FFT's rounding margin at 2^18–2^24 pieces; the `/finish` audit's 2,744,255-case byte sweep (lengths 1–72 and sampled positions to 4097, bases 2/4/8/32), 280,152 `divMod` checks by ±2^k with k to 4200, 171/199 product shapes around every threshold, 225,984 one-word `read`/`toString` boundary cases and interleaved timings, each in four builds; the diff review's 22,452 `Fermat` and `schonhageStrassen` checks (ring operations against an InfInt oracle including aliasing, transform round trips to K = 1024, 354 shapes × 6 operand patterns to 4097 words) and 3,173,964 `divMod` checks against `__int128`, each in four builds.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/01-Core/07-infint_benchmark.py>) wrote `96-Local Testing/01-Core/07-infint_benchmark.jsonl` on 2026-10-17 (seed 20261017, header hash `da5abf69be8830a4`, 2,100 measurements beside the running suites) and appended two 2026-10-25 runs on the idle host after the suites: all families with seed 20261025 (header hash `a5b4175ea67d83d8`, 2,181 measurements) and, after the `/finish` changes, the product family again (`--only mul`, header hash `86302dfded34c8f0`, that of the committed header, 511 measurements), all on GCC 16.2.1 and an Intel Core i9-11900H (a laptop host that is not frequency-isolated): per run a scalar build (`-O2 -DNDEBUG -march=x86-64 -mno-avx2 -mno-fma -mno-bmi2`) and an AVX2 build (`-mavx2 -mfma -mbmi2`), each workload doubling its iteration count until a pass takes 2 ms and then taking the median of five repetitions with the result digest verified on every call. Operands are random words with the top bit set (divisors normalised, gcd operands sharing no structure). The two product tables below are the committed header's run, the radix table the 2,181-measurement run (its kernels are unchanged since), the others the 2026-10-17 run. Timings support the thresholds, not portable pass/fail gates: neighbouring sizes differ by up to 15% between runs, and a variant measured after a long transform workload can read up to 60% slower than the identical code path measured first (the scalar 4096 × 256 and 4096 × 1024 dispatch rows of the 2,181-measurement run, which call the `m`-word chunk path), which an interleaved re-measurement (nine rounds, minimum per variant; the uncommitted `/tmp` harness of 2026-10-25) resolves to a tie, as does the committed header's run (838 against 836 and 418 against 418 µs).

Thresholds set from these runs (`infint_detail`, words unless stated): `KARATSUBA = 28`, `KARATSUBA_SQR = 40`, `TOOM3 = TOOM3_SQR = 128`; `FFT = 512` and `FFT_SQR = 768` (AVX2, 2026-10-25: the three-prime NTT ties Toom-3 at 512 words, 66 against 70 µs in the committed header's run, 63 against 66 in the re-review run and 68 against 71 interleaved, and wins from 768; squaring: the committed 512-word row reads 44.7 against 55.3 µs for the transform, measured after the product rows, and the finish audit's interleaved probe 46.3 against 49.7; from 513 to 1023 words the square's transform pads to 2048 limbs and costs about 92 µs whatever the length, which Toom-3 undercuts until about 740 words (85.8 at 704) and loses to from 768 (94.5 against 92.5), so `FFT_SQR = 768` forgoes a transform edge of at most 7% on squares of 512 to about 520 words and avoids an 8% loss on 683 to 740; user decision 2026-10-25: the more optimal threshold, 768) and `FFT = 2048`, `FFT_SQR = 4096` (scalar: the double FFT ties Toom-3 at 1792–2048 words and Toom-3 squaring at 2048, which still wins by 25% at 3072), together with the padding rule (a transform of the whole product needs a product length of at least two thirds of the padded power of two) and, from `FFT` words, the chunk estimator of `mul` (`transformWork`: a product of `len` words costs its `2 bit_ceil(len)` transform limbs times the log length; chunks of `s - m` words of the longer operand for each power of two `s > m` are summed with the last chunk at its own length and taken when below 90% of the padded whole; a product with `n < 2m` and `m` below `3 FFT / 2` without acceptable padding stays with Toom-3 or Karatsuba, which win by 15–25% in that band in both builds); `MIDDLE = 128` (AVX2) / `256` (scalar), `TRUNC = 32`, `BURNIKEL = 48` (also the quotient-length bound below which Knuth D stays), `NEWTON_DIV = 512` (AVX2) / `768` (scalar) for cached reciprocals (`powMod` with even moduli, radix conversion), `NEWTON_LONG = 32` (AVX2; never in the scalar build) for one-off divisions whose quotient has at least four times the divisor's words, where a fresh reciprocal amortises over the blocks (Burnikel–Ziegler beats a fresh reciprocal at every other measured shape; at exactly four divisor lengths, 83334 by 16667 words, the two tie at 108 and 102 ms; user decision 2026-10-25: Burnikel–Ziegler stays), `HENSEL = 48`, `BINARY_GCD = 96` (AVX2) / `48` (scalar), `HALFGCD = 4096` limbs, `RADIX = 64` blocks, `MONTGOMERY_POW = 2` (every odd multiword modulus), `FFT_DOUBLE_MAX = 2^14`, `NTT_MAX_LOG = 26`. The scalar build takes the double FFT up to `FFT_DOUBLE_MAX` and 2^26 limbs and the three-prime NTT above it (the Goldilocks NTT only beyond 2^26 limbs); the AVX2 build takes the three-prime NTT throughout.

Balanced products, microseconds per product (basecase / Karatsuba / Toom-3 / transform / Schönhage–Strassen / dispatch; the square columns, whose paths the `/finish` changes left untouched, are the 2,181-measurement run)

| Words | Scalar product | AVX2 product | Scalar square | AVX2 square |
|---|---|---|---|---|
| 2 | 0.0 / 0.0 / – / – / – / 0.0 | 0.0 / 0.1 / – / – / – / 0.0 | 0.0 / 0.0 / – / – / 0.0 | 0.0 / 0.1 / – / – / 0.0 |
| 4 | 0.0 / 0.1 / – / – / – / 0.0 | 0.0 / 0.1 / – / – / – / 0.0 | 0.0 / 0.1 / – / – / 0.0 | 0.0 / 0.1 / – / – / 0.0 |
| 8 | 0.1 / 0.1 / 0.3 / – / – / 0.1 | 0.1 / 0.1 / 0.3 / – / – / 0.1 | 0.0 / 0.1 / 0.3 / – / 0.1 | 0.1 / 0.1 / 0.3 / – / 0.1 |
| 12 | 0.1 / 0.2 / 0.4 / – / – / 0.1 | 0.1 / 0.2 / 0.4 / – / – / 0.1 | 0.1 / 0.2 / 0.4 / – / 0.1 | 0.1 / 0.2 / 0.3 / – / 0.1 |
| 16 | 0.2 / 0.3 / 0.6 / 2.3 / – / 0.2 | 0.2 / 0.3 / 0.6 / 2.0 / – / 0.2 | 0.2 / 0.2 / 0.5 / 2.3 / 0.2 | 0.2 / 0.3 / 0.5 / 1.3 / 0.2 |
| 20 | 0.4 / 0.4 / 0.6 / 5.1 / – / 0.4 | 0.4 / 0.4 / 0.7 / 3.6 / – / 0.4 | 0.3 / 0.3 / 0.5 / 4.8 / 0.3 | 0.3 / 0.4 / 0.6 / 2.5 / 0.3 |
| 24 | 0.5 / 0.5 / 0.8 / 5.1 / – / 0.5 | 0.6 / 0.5 / 0.9 / 3.6 / – / 0.6 | 0.3 / 0.4 / 0.7 / 4.8 / 0.4 | 0.4 / 0.4 / 0.6 / 2.2 / 0.4 |
| 28 | 0.7 / 0.7 / 1.0 / 5.0 / – / 0.7 | 0.7 / 0.7 / 1.0 / 3.8 / – / 0.7 | 0.5 / 0.5 / 0.8 / 4.8 / 0.5 | 0.5 / 0.5 / 1.5 / 3.3 / 0.5 |
| 32 | 0.9 / 0.9 / 1.1 / 5.2 / – / 0.9 | 0.9 / 0.9 / 1.1 / 4.0 / – / 0.9 | 0.6 / 0.6 / 0.9 / 4.9 / 0.6 | 0.6 / 0.6 / 0.9 / 2.5 / 0.6 |
| 40 | 1.4 / 1.3 / 1.5 / 11.1 / – / 1.3 | 1.4 / 1.3 / 1.5 / 7.5 / – / 1.3 | 0.9 / 0.9 / 1.3 / 10.7 / 1.0 | 0.9 / 0.9 / 1.2 / 4.6 / 0.9 |
| 48 | 2.3 / 1.8 / 2.0 / 11.2 / – / 1.8 | 2.4 / 1.8 / 2.0 / 7.8 / – / 1.8 | 1.3 / 1.2 / 1.6 / 10.9 / 1.3 | 1.3 / 1.3 / 1.5 / 4.7 / 1.2 |
| 64 | 4.2 / 2.8 / 3.3 / 11.4 / – / 3.0 | 4.3 / 3.0 / 3.2 / 8.3 / – / 3.0 | 2.3 / 1.9 / 2.4 / 11.0 / 2.0 | 2.1 / 2.0 / 2.3 / 4.9 / 2.0 |
| 80 | 4.7 / 3.4 / 3.8 / 20.9 / – / 3.6 | 6.3 / 4.2 / 4.5 / 15.7 / – / 4.2 | 3.3 / 3.0 / 3.2 / 23.2 / 3.1 | 3.1 / 3.1 / 3.2 / 9.7 / 3.1 |
| 96 | 7.5 / 4.9 / 5.2 / 21.7 / – / 5.2 | 9.0 / 4.7 / 4.6 / 15.0 / – / 5.9 | 4.8 / 3.8 / 4.1 / 23.5 / 4.0 | 4.4 / 4.0 / 4.1 / 10.0 / 4.0 |
| 112 | 10.3 / 6.4 / 6.8 / 22.3 / – / 6.5 | 12.2 / 7.3 / 6.6 / 12.7 / – / 6.0 | 6.1 / 4.9 / 5.7 / 23.6 / 5.1 | 5.8 / 5.1 / 5.3 / 10.2 / 5.0 |
| 128 | 13.4 / 8.1 / 8.3 / 22.5 / – / 8.4 | 12.5 / 7.8 / 7.9 / 14.4 / – / 8.1 | 8.0 / 6.0 / 6.7 / 24.2 / 6.7 | 7.2 / 6.4 / 6.6 / 10.4 / 6.6 |
| 160 | 20.8 / 11.6 / 12.0 / 46.0 / – / 12.2 | 20.8 / 11.6 / 11.4 / 28.5 / – / 11.5 | 11.9 / 9.1 / 9.0 / 48.6 / 9.3 | 11.2 / 9.6 / 9.3 / 20.2 / 9.1 |
| 192 | 29.9 / 15.8 / 15.9 / 48.5 / – / 16.3 | 29.7 / 15.8 / 15.6 / 29.3 / – / 15.6 | 14.1 / 9.9 / 10.0 / 41.4 / 10.4 | 15.7 / 12.7 / 12.2 / 20.7 / 12.0 |
| 256 | 54.9 / 26.3 / 25.3 / 48.8 / – / 26.7 | 53.5 / 25.5 / 24.3 / 31.3 / – / 24.2 | 26.5 / 18.2 / 17.4 / 44.8 / 17.6 | 27.4 / 20.4 / 19.7 / 21.7 / 19.2 |
| 320 | 90.7 / 39.9 / 36.3 / 99.9 / – / 35.3 | 83.6 / 36.2 / 34.8 / 59.6 / – / 34.7 | 40.5 / 25.6 / 24.1 / 104.4 / 24.3 | 41.8 / 28.7 / 27.4 / 41.8 / 26.8 |
| 384 | 118.6 / 48.7 / 47.2 / 103.7 / – / 47.9 | 118.9 / 48.7 / 45.6 / 62.8 / – / 45.1 | 57.4 / 32.8 / 33.9 / 103.3 / 34.4 | 59.0 / 36.5 / 36.9 / 42.9 / 36.7 |
| 512 | 210.3 / 75.2 / 71.2 / 106.8 / – / 72.9 | 206.2 / 72.7 / 69.8 / 66.0 / – / 64.4 | 100.9 / 54.3 / 51.5 / 110.6 / 55.0 | 102.6 / 60.7 / 55.3 / 44.7 / 55.7 |
| 640 | 333.2 / 108.9 / 102.0 / 217.1 / – / 104.7 | 321.0 / 108.8 / 101.5 / 126.9 / – / 100.1 | 160.4 / 73.7 / 71.5 / 207.6 / 73.4 | 166.5 / 83.1 / 77.1 / 89.0 / 78.9 |
| 768 | 470.6 / 145.7 / 134.6 / 219.5 / – / 138.0 | 470.5 / 142.2 / 132.4 / 128.0 / – / 127.1 | 226.4 / 104.7 / 97.1 / 208.3 / 99.2 | 256.6 / 120.0 / 102.3 / 88.7 / 88.8 |
| 1024 | 834.7 / 221.8 / 204.6 / 227.1 / 360.5 / 203.8 | 830.6 / 207.0 / 201.2 / 137.3 / 340.0 / 136.4 | 403.5 / 153.3 / 143.8 / 210.0 / 148.3 | 404.3 / 170.7 / 159.3 / 92.8 / 92.7 |
| 1536 | – / 409.4 / 387.5 / 469.0 / 608.1 / 397.2 | – / 384.0 / 370.8 / 272.3 / 566.2 / 271.4 | – / 297.1 / 274.9 / 433.9 / 281.3 | – / 275.3 / 297.3 / 187.1 / 187.8 |
| 2048 | – / 635.9 / 593.7 / 475.1 / 851.1 / 465.7 | – / 409.8 / 646.4 / 284.0 / 827.1 / 284.9 | – / 453.8 / 434.3 / 444.5 / 437.2 | – / 353.1 / 462.7 / 194.8 / 194.7 |
| 3072 | – / 1228.7 / 1090.5 / 1077.8 / 1396.4 / 1046.0 | – / 821.4 / 751.5 / 628.0 / 1377.4 / 638.6 | – / 1124.9 / 1052.3 / 1568.9 / 762.5 | – / 574.6 / 702.0 / 427.3 / 426.6 |
| 4096 | – / 1605.8 / 1791.2 / 1066.1 / 2066.7 / 1041.7 | – / 871.7 / 1404.1 / 610.4 / 1973.8 / 553.7 | – / 1346.9 / 1262.1 / 1000.5 / 1024.8 | – / 864.4 / 1055.0 / 411.5 / 416.0 |
| 8192 | – / 3202.3 / 5281.3 / 2511.5 / 5006.1 / 2303.1 | – / 1829.5 / 2990.5 / 1366.6 / 5028.9 / 1325.8 | – / 3328.9 / 4874.5 / 3620.3 / 2215.6 | – / 1898.9 / 2205.6 / 934.9 / 946.4 |
| 16384 | – / – / – / 5372.6 / 11803.5 / 5296.2 | – / – / – / 2865.5 / 11792.1 / 2810.4 | – / – / – / 5024.8 / 5017.6 | – / – / – / 2007.2 / 1987.0 |
| 32768 | – / – / – / 20944.8 / 30287.3 / 21116.2 | – / – / – / 6021.8 / 30058.2 / 5884.7 | – / – / – / 13649.4 / 13668.3 | – / – / – / 4276.1 / 4210.2 |

Unbalanced products of n and m words, microseconds (basecase / m-word chunks / one split at bit_ceil(n + m) / 2 - m / whole transform / dispatch)

| n × m | Scalar | AVX2 |
|---|---|---|
| 64 × 4 | 0.3 / 0.7 / – / 10.0 / 0.3 | 0.3 / 0.7 / – / 6.6 / 0.3 |
| 64 × 16 | 0.9 / 1.0 / – / 10.1 / 1.0 | 0.9 / 1.1 / – / 6.7 / 0.9 |
| 64 × 32 | 1.8 / 1.7 / – / 10.2 / 1.8 | 1.8 / 1.7 / – / 6.9 / 1.8 |
| 256 × 4 | 1.0 / 7.1 / – / 45.5 / 1.0 | 1.0 / 7.4 / – / 30.5 / 1.1 |
| 256 × 16 | 3.4 / 5.1 / – / 46.2 / 3.6 | 3.8 / 4.9 / – / 27.6 / 3.4 |
| 256 × 64 | 13.1 / 11.0 / – / 47.8 / 11.2 | 13.3 / 10.8 / – / 29.4 / 10.8 |
| 256 × 128 | 26.7 / 16.9 / – / 47.9 / 17.7 | 27.3 / 16.8 / – / 29.8 / 16.8 |
| 1024 × 4 | 4.1 / 96.8 / – / 213.1 / 4.2 | 4.1 / 96.2 / – / 121.2 / 4.0 |
| 1024 × 16 | 13.4 / 37.3 / – / 215.9 / 14.0 | 13.7 / 38.2 / – / 121.7 / 13.8 |
| 1024 × 64 | 51.3 / 48.9 / – / 217.9 / 50.1 | 51.6 / 48.2 / – / 124.8 / 48.2 |
| 1024 × 256 | 208.4 / 98.0 / – / 219.9 / 102.9 | 210.1 / 99.2 / – / 127.3 / 99.2 |
| 1024 × 512 | 416.5 / 144.5 / – / 220.8 / 149.2 | 417.3 / 129.9 / 129.8 / 127.8 / 130.5 |
| 4096 × 4 | 20.3 / 1488.3 / – / 1027.7 / 20.7 | 15.9 / 1509.4 / – / 537.2 / 15.9 |
| 4096 × 16 | 60.4 / 424.8 / – / 979.8 / 63.6 | 55.0 / 431.1 / – / 542.3 / 54.9 |
| 4096 × 256 | 816.1 / 418.0 / – / 1025.4 / 418.5 | 846.1 / 415.0 / – / 561.2 / 414.3 |
| 4096 × 1024 | 3314.2 / 836.4 / – / 1016.4 / 838.1 | 3410.1 / 555.1 / 417.1 / 570.6 / 428.1 |
| 4096 × 2048 | 6507.6 / 960.5 / 982.9 / 1035.8 / 1043.4 | 6814.3 / 573.7 / 560.2 / 587.6 / 586.1 |
| 16384 × 4 | 83.4 / 23419.5 / – / 4994.1 / 84.1 | 72.7 / 23489.3 / – / 2346.3 / 70.8 |
| 16384 × 16 | 252.6 / 6093.6 / – / 5061.1 / 271.9 | 235.4 / 6091.9 / – / 2375.6 / 236.1 |
| 16384 × 1024 | 13121.8 / 3435.6 / – / 4941.9 / 3522.1 | 13098.0 / 2235.8 / 1410.4 / 2442.5 / 1461.4 |
| 16384 × 4096 | 52366.6 / 4221.4 / 5377.7 / 5061.5 / 3549.3 | 52771.3 / 2419.4 / 1893.7 / 2551.2 / 1983.8 |
| 16384 × 8192 | 104944.3 / 6543.0 / 4719.8 / 5063.0 / 5076.9 | 104973.1 / 2500.5 / 2539.4 / 2537.5 / 2539.4 |

Schönhage–Strassen (`schonhageStrassen`) against the transform (`transformMul`: three-prime NTT in the AVX2 build, double FFT in the scalar build) and Toom-3 over the transforms, balanced n-word products and squares, milliseconds (minimum of three on the idle host, 2026-10-25 probe; the `ssa` rows of the balanced table above are the benchmark driver's medians up to 32768 words):

| n (words) | AVX2 ssa / transform / toom3 | AVX2 square ssa / transform / toom3Square | scalar ssa / transform / toom3 | scalar square ssa / transform / toom3Square |
|---|---|---|---|---|
| 2^14 | 10.7 / 3.3 / 6.9 | 9.6 / 1.8 / 4.4 | 11.6 / 5.9 / 13.3 | 10.8 / 5.8 / 13.3 |
| 2^15 | 28.8 / 5.4 / 12.5 | 25.7 / 3.7 / 9.3 | 35.3 / 22.4 / 28.3 | 29.4 / 15.9 / 28.5 |
| 2^16 | 71.7 / 11.8 / 26.9 | 61.9 / 7.8 / 19.8 | 81.7 / 48.6 / 103.2 | 66.8 / 33.8 / 75.9 |
| 2^17 | 193.5 / 25.7 / 59.8 | 171.8 / 17.3 / 44.8 | 220.7 / 105.9 / 243.5 | 188.8 / 73.2 / 165.1 |
| 2^18 | 385.7 / 55.2 / 127.8 | 387.2 / 35.9 / 92.9 | 502.5 / 221.8 / 514.6 | 422.3 / 151.5 / 370.4 |
| 2^19 | 1224.3 / 116.7 / 275.8 | 1184.9 / 78.6 / 200.2 | 1294.9 / 415.4 / 986.0 | 1172.5 / 280.4 / 684.6 |
| 2^20 | 2631.9 / 292.4 / 603.0 | 2794.5 / 174.0 / 435.8 | 3029.3 / 897.2 / 2069.2 | 2679.3 / 606.3 / 1467.8 |

The pointwise products dominate (K elements of about 2 (n + m) / K words each, so twice the operand volume, each through the dispatcher), and the ratio to the transform widens with the size; above `NTT_MAX_WORDS`, where the alternative is Toom-3 over 2^28-word transforms, one Toom-3 level costs about 3.3 transforms of the whole length and two levels about 11, against the 9–10 measured for Schönhage–Strassen at 2^19–2^20 words, so the dispatcher keeps Toom-3 and Karatsuba there (Limits, which also record the AVX2 caveat: above 2^26 limbs those transforms are the 5x slower Goldilocks NTT).

Cyclic transforms of two n-word operands, microseconds (double FFT / three-prime NTT / Goldilocks NTT)

| Words | Scalar | AVX2 |
|---|---|---|
| 16 | 2.3 / 5.4 / 7.1 | 2.1 / 1.9 / 6.3 |
| 64 | 12.8 / 23.4 / 31.6 | 10.4 / 8.3 / 33.3 |
| 256 | 49.9 / 113.9 / 150.0 | 48.7 / 34.6 / 150.3 |
| 1024 | 254.2 / 552.1 / 740.4 | 241.9 / 147.9 / 717.6 |
| 4096 | 1148.3 / 2517.0 / 3223.7 | 1061.8 / 698.3 / 3332.1 |
| 16384 | 5452.4 / 10895.0 / 14900.1 | 4695.3 / 2955.3 / 12198.1 |
| 32768 | – / 23434.5 / 32520.2 | – / 7185.2 / 33729.9 |

Division of a 3m-word dividend by an m-word divisor, microseconds (Knuth D / Burnikel–Ziegler / Newton with a fresh reciprocal / Newton with a cached reciprocal / dispatch)

| m | Scalar | AVX2 |
|---|---|---|
| 8 | 0.3 / 0.5 / 0.8 / 0.5 / 0.4 | 0.4 / 0.6 / 0.8 / 0.5 / 0.5 |
| 16 | 0.9 / 1.0 / 1.9 / 1.3 / 1.0 | 1.5 / 1.6 / 2.0 / 1.3 / 1.6 |
| 32 | 3.0 / 3.2 / 5.8 / 4.0 / 3.1 | 4.9 / 5.3 / 6.6 / 3.9 / 5.5 |
| 48 | 8.4 / 8.0 / 12.7 / 8.6 / 8.1 | 10.7 / 11.0 / 13.0 / 7.8 / 11.5 |
| 64 | 13.2 / 10.8 / 20.1 / 12.7 / 10.6 | 19.3 / 14.5 / 20.2 / 12.7 / 14.8 |
| 128 | 57.0 / 34.7 / 64.0 / 38.4 / 33.8 | 76.3 / 41.3 / 62.7 / 38.4 / 42.6 |
| 256 | 219.1 / 107.4 / 196.5 / 111.4 / 111.1 | 292.4 / 118.8 / 194.3 / 110.2 / 130.9 |
| 512 | 883.1 / 328.7 / 580.0 / 322.1 / 329.6 | 1149.9 / 340.9 / 561.5 / 316.8 / 362.0 |
| 768 | 1911.5 / 637.7 / 1108.6 / 606.5 / 673.1 | 2543.2 / 659.8 / 1066.9 / 591.9 / 702.8 |
| 1024 | 3964.5 / 1084.0 / 1798.1 / 939.7 / 1021.1 | 4360.8 / 996.4 / 1547.7 / 749.3 / 1034.9 |
| 2048 | 13493.8 / 2518.9 / 4348.8 / 2099.7 / 2531.2 | 17341.6 / 2572.4 / 3908.5 / 1992.7 / 2710.4 |
| 4096 | – / 8143.3 / 11883.1 / 6135.6 / 8829.1 | – / 6760.2 / 9709.7 / 4633.9 / 6797.8 |
| 8192 | – / 22450.2 / 34991.9 / 17809.0 / 22365.4 | – / 16328.4 / 21645.7 / 9535.7 / 16593.3 |

Long quotients (9m by m) and equal sizes (2m by m), microseconds, same backends

| m | Scalar long | AVX2 long | Scalar equal | AVX2 equal |
|---|---|---|---|---|
| 16 | 3.4 / 4.0 / 5.9 / 5.3 / 3.6 | 5.5 / 6.1 / 5.3 / 4.6 / 5.9 | 0.5 / 0.6 / 1.3 / 0.7 / 0.5 | 0.7 / 0.8 / 1.5 / 0.7 / 0.8 |
| 64 | 57.8 / 48.7 / 58.7 / 51.8 / 45.9 | 77.8 / 60.3 / 55.4 / 49.5 / 58.7 | 6.5 / 5.0 / 13.8 / 6.6 / 5.5 | 9.8 / 7.1 / 14.1 / 6.2 / 7.7 |
| 256 | 870.9 / 435.9 / 537.0 / 445.5 / 438.5 | 1166.8 / 481.0 / 520.6 / 444.4 / 538.3 | 112.2 / 54.0 / 139.0 / 57.9 / 55.7 | 146.2 / 60.0 / 134.8 / 55.5 / 63.7 |
| 1024 | 13739.2 / 4321.5 / 5047.9 / 4019.6 / 4157.7 | 20109.0 / 4308.7 / 3992.7 / 3110.8 / 3920.1 | 1883.2 / 529.6 / 1336.1 / 498.9 / 561.8 | 2286.9 / 500.8 / 1153.1 / 386.5 / 528.4 |
| 4096 | – / 37382.0 / 30713.5 / 24475.0 / 31342.8 | – / 27093.5 / 24643.1 / 18840.1 / 23934.3 | – / 3968.8 / 8948.1 / 3560.4 / 5195.1 | – / 3385.5 / 7425.4 / 2353.7 / 3484.9 |

Exact division (Hensel / Burnikel–Ziegler quotient) by divisor words, the reciprocal's residual window of an m-word divisor (middleProduct / full product) and the low product (truncatedMul / full product), microseconds

| Words | Scalar exact | AVX2 exact | Scalar middle | AVX2 middle | Scalar low | AVX2 low |
|---|---|---|---|---|---|---|
| 16 | 0.4 / 0.5 | 0.4 / 0.9 | 0.1 / 0.2 | 0.1 / 0.1 | 0.1 / 0.3 | 0.1 / 0.2 |
| 32 | 1.7 / 1.7 | 1.5 / 2.6 | 0.3 / 0.5 | 0.3 / 0.5 | 0.9 / 0.9 | 0.8 / 0.8 |
| 48 | 4.1 / 4.9 | 3.1 / 5.9 | 0.7 / 1.3 | 0.6 / 1.2 | 1.8 / 1.9 | 1.6 / 1.6 |
| 64 | 6.9 / 5.4 | 5.5 / 7.6 | 1.1 / 2.1 | 1.2 / 1.8 | 2.9 / 3.1 | 2.6 / 2.6 |
| 128 | 28.3 / 17.6 | 20.6 / 21.6 | 4.6 / 6.4 | 4.0 / 5.6 | 9.3 / 9.5 | 7.9 / 8.0 |
| 256 | 110.2 / 54.2 | 76.8 / 63.7 | 16.0 / 19.2 | 16.8 / 16.8 | 27.0 / 27.3 | 23.3 / 24.1 |
| 512 | 430.7 / 161.8 | 309.5 / 177.6 | 56.2 / 56.7 | 49.2 / 50.3 | 78.1 / 78.7 | 69.7 / 68.4 |
| 1024 | 1913.4 / 541.3 | 1192.1 / 516.3 | 160.5 / 161.5 | 138.8 / 141.7 | 187.3 / 219.7 | 135.2 / 132.2 |
| 2048 | 7366.6 / 1575.0 | 4754.9 / 1325.0 | 493.5 / 480.6 | 258.5 / 268.1 | 545.1 / 550.1 | 278.8 / 280.2 |

gcd of two operands of the given limb count, microseconds (binary / Lehmer loop / half-GCD loop / dispatch)

| Limbs | Scalar | AVX2 |
|---|---|---|
| 8 | 1.9 / 4.1 / – / 2.0 | 1.9 / 4.0 / – / 2.0 |
| 32 | 12.7 / 25.7 / – / 13.1 | 11.6 / 25.3 / – / 11.8 |
| 64 | 45.8 / 62.7 / 61.7 / 45.6 | 34.4 / 61.2 / 62.0 / 33.9 |
| 96 | 105.7 / 105.6 / 106.9 / 108.6 | 71.1 / 106.3 / 105.6 / 73.1 |
| 128 | 184.8 / 155.8 / 153.7 / 162.8 | 117.7 / 157.6 / 155.8 / 114.6 |
| 192 | 396.8 / 301.9 / 299.4 / 298.9 | 233.6 / 299.6 / 290.6 / 240.8 |
| 256 | 682.9 / 466.0 / 475.5 / 460.8 | 405.1 / 454.2 / 452.5 / 392.5 |
| 512 | 2882.7 / 1638.5 / 1638.8 / 1636.3 | 1544.1 / 1437.5 / 1437.6 / 1350.3 |
| 1024 | – / 4615.0 / 4613.6 / 4648.1 | – / 4542.8 / 4561.5 / 4468.3 |
| 2048 | – / 15730.4 / 15675.9 / 15821.0 | – / 15866.8 / 15589.3 / 15699.1 |
| 4096 | – / 59013.6 / 59161.3 / 59113.2 | – / 57832.6 / 57368.2 / 58650.8 |
| 8192 | – / 239355.5 / 200916.1 / 205482.4 | – / 240323.4 / 190182.9 / 196649.2 |

powMod with a full-size exponent, microseconds (windowed Montgomery / binary ladder with Montgomery / binary ladder with reciprocal reduction); even modulus windowed in the last columns

| Words | Scalar odd | AVX2 odd | Scalar even | AVX2 even |
|---|---|---|---|---|
| 1 | 1.2 / 4.7 / 4.9 | 1.0 / 4.5 / 4.6 | 1.1 | 0.9 |
| 2 | 16.3 / 13.2 / 28.7 | 11.3 / 11.5 / 28.1 | 30.0 | 23.6 |
| 4 | 34.1 / 41.8 / 72.5 | 32.6 / 38.5 / 76.6 | 59.0 | 62.2 |
| 8 | 143.7 / 174.9 / 275.9 | 134.7 / 168.6 / 333.4 | 212.9 | 263.1 |
| 16 | 798.1 / 1059.6 / 1329.4 | 776.2 / 1025.6 / 1819.6 | 1059.5 | 1385.3 |
| 32 | 6046.1 / 7888.1 / 8267.9 | 5829.5 / 7892.5 / 11086.8 | 6311.1 | 8312.0 |
| 64 | 38561.3 / 51563.1 / 49718.1 | 38691.7 / 51631.8 / 63407.2 | 37330.1 | 46088.0 |
| 128 | 261917.6 / 355197.9 / 318296.8 | 243075.3 / 326652.8 / 376402.9 | 253441.5 | 281158.6 |
| 256 | 1395211.0 / 1966402.8 / 1954278.8 | 1273649.0 / 1729371.0 / 2017195.1 | 1398604.8 | 1417017.1 |

Radix conversion, microseconds (parse / print), with the digit count

| Words | Decimal digits | Scalar decimal | AVX2 decimal | Scalar hex | AVX2 hex | Scalar base 36 | AVX2 base 36 |
|---|---|---|---|---|---|---|---|
| 1 | 20 | 0.1 / 0.0 | 0.1 / 0.0 | 0.0 / 0.0 | 0.0 / 0.0 | 0.1 / 0.0 | 0.1 / 0.0 |
| 4 | 77 | 0.1 / 0.2 | 0.1 / 0.2 | 0.0 / 0.0 | 0.0 / 0.0 | 0.1 / 0.3 | 0.1 / 0.3 |
| 16 | 309 | 0.3 / 0.7 | 0.3 / 0.8 | 0.1 / 0.1 | 0.0 / 0.1 | 0.3 / 1.0 | 0.3 / 1.3 |
| 64 | 1233 | 2.0 / 7.6 | 2.3 / 8.7 | 0.4 / 0.4 | 0.1 / 0.2 | 2.2 / 8.8 | 2.0 / 9.7 |
| 256 | 4932 | 20.2 / 57.7 | 20.1 / 66.2 | 1.4 / 1.5 | 0.2 / 0.8 | 20.8 / 62.9 | 21.2 / 77.3 |
| 1024 | 19729 | 197.6 / 543.8 | 195.0 / 606.9 | 5.4 / 5.9 | 0.9 / 3.1 | 196.1 / 583.6 | 196.4 / 666.0 |
| 4096 | 78914 | 1855.4 / 4443.1 | 1486.0 / 3867.1 | 21.2 / 23.7 | 3.6 / 12.5 | 1665.6 / 4354.8 | 1410.0 / 4069.6 |
| 16384 | 315653 | 12370.2 / 28036.8 | 8948.1 / 21778.3 | 86.6 / 95.0 | 15.0 / 50.2 | – | – |
| 65536 | 1262612 | 83422.2 / 184682.5 | 45689.9 / 109369.3 | 347.5 / 385.2 | 64.0 / 208.7 | – | – |

Roots, bits and primality, microseconds

| Words | isqrtRem S / A | kthRoot(3) S / A | popcount S / A | shift left 13 S / A | isProbablePrime (odd random) S / A |
|---|---|---|---|---|---|
| 4 | 0.4 / 0.4 | 2.5 / 2.5 | 0.0 / 0.0 | 0.0 / 0.0 | 34.6 / 29.4 |
| 64 | 3.1 / 3.3 | 22.8 / 28.0 | 0.3 / 0.0 | 0.2 / 0.1 | 0.3 / 0.3 |
| 1024 | 163.9 / 180.6 | 2404.8 / 2591.1 | 4.3 / 0.2 | 2.0 / 1.7 | – / – |
| 16384 | 10997.2 / 7729.5 | 202550.6 / 133232.9 | 69.4 / 3.2 | 33.0 / 28.3 | – / – |

Observations: the dispatcher is within noise of the best one-off backend at every size except the smallest divisions (vector allocations cost about 0.1 µs below 8 divisor words) and the shared-host outliers of this run (the scalar 160- and 3072-word products, whose repeat measurements agree with their neighbours); the AVX2 three-prime NTT is 2–3.5x faster than the scalar double FFT from 256 words up, which moves the transform threshold from 2048 to 512 words (1024 until 2026-10-25) and halves large products; in the transform range the chunk estimator keeps the dispatcher within 5% of the best measured variant at every unbalanced shape (AVX2 16384 × 1024: 1461 µs against 2443 for the padded whole transform and 2236 for 1024-word chunks; 4096 × 1024: 428 against 571 and 555), where the 2026-10-17 dispatcher paid up to 2x (Library Checker reproduction below); Montgomery REDC in `powMod` beats reciprocal reduction by 1.1–2.3x on odd moduli, so the Core rule (Barrett/Montgomery wherever beneficial) is met; the word-by-word Hensel division wins below 48 quotient words and the quotient-only Burnikel–Ziegler division above (a Newton–Hensel lifting variant measured 1.3–2x slower and was removed); a fresh reciprocal pays for long AVX2 quotients (nine times the divisor) from 32 divisor words; the middle product beats the full product in its basecase range and matches it elsewhere (its transform saving depends on the power-of-two rounding of the two lengths); binary gcd beats Lehmer up to 48 words (scalar) and 96 words (AVX2), the half-GCD recursion pays only from about 4096 limbs; the hex fast paths print a 1,048,576-digit number at 2.7 GB/s (scalar) and 5.0 GB/s (AVX2) and parse it, cache-resident, at 3.0 and 16 GB/s (the 2026-10-17 header validated the digits in a separate byte loop that cost 250 of its 315 µs).

### Library Checker reproduction (2026-10-25)

The six `big_integer` problems' tests were regenerated from the judge's seeded generators (`yosupo06/library-checker-problems`, `big_integer/*/gen` with the `info.toml` counts and parameters: 139 files, 3.2–5.3 MB each) and every solution under `97-Online Testing/src/01-Yosupo/02-Big Integers` was built with the judge's command (`g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`; GCC 16.2 here, 15.2 on the judge) once against the archived `OLD/1-Core/05-infint.hpp` of the 2024 submissions and once against this header; each test's wall time is the minimum of three runs on the i9-11900H and the two outputs are byte-identical on every test. Totals as new / old, with the tests whose submissions had regressed: hex addition 0.89 (`max_max` 0.96–0.97, `small` 0.81), hex multiplication 0.94 (`large_small` 0.82, `large` 0.95, `max_max` 0.98–1.01, `fft_killer` 1.08), hex division 0.62 (`max` 0.89–0.97, `power` 0.94, `large` 0.97, `burnikel_ziegler_bound_01` 0.16), decimal addition 0.63, decimal multiplication 0.63, decimal division 0.57 (the many-tiny-case `small`, `medium` and `r_nearly_zero` tests read 1.06–1.35 before the one-word fast paths and 0.62–0.73 now). The 2026-10-17 header on the same tests: hex addition 1.05 (`max_max` up to 1.16), hex multiplication 1.01 (`large_small` 1.64, `large` 1.12–1.43), hex division 0.71 (`max` 1.29–1.39, `power` 1.34, `large` 1.21–1.26).

Stage times on the 3.2 MB inputs (old / new, ms, as `cin >> string` / `read` / the operation / `toString` / `cout`): addition `max_max_00` 3.48 / 0.34 / 0.39 / 1.02 / 0.00 against 3.46 / 0.42 / 0.16 / 0.60 / 0.00, where the 2026-10-17 header spent 1.61 ms in `read` (a byte-by-byte validation loop before the AVX2 kernel); multiplication `large_small_00` (1.44 million by 160,000 digits) 28.96 against 24.53 in the products (63.41 on 2026-10-17: 1024-word Toom-3 chunks instead of transforms); division `max_01` (`B = 1`) and `power_00` (`B` in {1, 16, 256}, 3570 cases) 0.20 and 0.40 against 0.26 and 0.40 in the divisions (0.97 and 2.13 on 2026-10-17: no power-of-two shortcut). The stream extraction itself (`is >> string`, about 1.1 ns per character) is 60% of a hex addition's run time with either header and outside the header's control.

The causes and their fixes: (1) `read` validated every character in a separate table loop before `parsePow2`; the hex kernels now validate as they go (AVX2 compares, a SWAR range check, the table in the tail) and a bad character returns no limbs. (2) `mul` sent every `n >= 2m` product to `m`-word chunks and every balanced product below `FFT = 1024` to Toom-3; the thresholds are the measured 512 and 768 (AVX2) and in the transform range the chunk estimator takes the cheapest tight chunking or the whole transform. (3) `divMod` had lost the power-of-two divisor path; it shifts and masks again (`popcount(b) == 1`) and word-by-word operands divide natively. Also from this run: the hex printer stores each group once (`bswap`, `_mm256_shuffle_epi8`) and one-word values parse and print without the radix cache.

## Sources

Inspected 2026-10-16 and 2026-10-17; the implementation is written independently from the descriptions below, no code copied.

| Reference and inspected scope | Use and boundaries |
|---|---|
| [GMP Algorithms](https://gmplib.org/manual/Algorithms): multiplication (Karatsuba, Toom-3, FFT), division (Knuth D, divide-and-conquer, block-wise Barrett, exact division), GCD (binary, Lehmer, subquadratic), powering (sliding window, Montgomery), roots (Karatsuba square root, Newton k-th roots, perfect square/power filters), radix conversion (subquadratic both ways) | The backend set and threshold names; the dispatch rules are measured here, not GMP's values. |
| [GMP Integer Functions](https://gmplib.org/manual/Integer-Functions): division modes, `mpz_remove`, `scan0/scan1`, `kronecker`, `mpz_perfect_square_p`/`perfect_power_p`, import/export, `urandomb`/`urandomm`, `get_d`/`set_d`, `probab_prime_p` | Operation contracts: ceiling mode, `removeFactor`, bit scans, Kronecker, roots, limb import/export, sampling, conversions; GMP truncates `get_d`, this header rounds to nearest even (documented difference); GMP's `probab_prime_p` adds Miller–Rabin rounds beyond BPSW, this header stops at BPSW (Mathematics owns further primality work). |
| Brent and Zimmermann, [Modern Computer Arithmetic](https://members.loria.fr/PZimmermann/mca/mca-cup-0.5.9.pdf) v0.5.9 §1.3–1.6 (multiplication, division, roots), §2.4 (Montgomery REDC), §3 (Newton), as recalled; the saved copy was not reopened for this package | Toom-3 interpolation (verified numerically in the tester), Newton reciprocal structure, REDC with a word inverse, Hensel lifting. |
| Möller and Granlund, Improved division by invariant integers, IEEE Trans. Comput. 60(2), 2011 (Algorithm 4 as described in the GMP source comments) | `invWord`/`div2by1`: the 2-by-1 reciprocal division used by `divWord` and Knuth D. |
| Zimmermann, Karatsuba square root, INRIA RR-3805, 1999 (as described in the GMP Square Root Algorithm page) | `sqrtRemWords`; the `q = β` overflow case is handled with an extra word. |
| Burnikel and Ziegler, Fast recursive division, MPI-I-98-1-022 (structure as described in the GMP and Java `BigInteger` notes) | `div2n1n`/`div3n2n`/`divBurnikelZiegler` with the block padding of the dividend. |
| Hanrot, Quercia and Zimmermann, The middle product algorithm I, AAECC 14 (2004) (statement only) | `middleProduct` contract: the wrapped cyclic transform of length `2n - 1` leaves the wanted words unpolluted; the lower-term carry bound `< n B` is the tester's check. |
| Baillie and Wagstaff, Lucas pseudoprimes, Math. Comp. 35 (1980); the strong Lucas test with Selfridge's method A as in GMP and FLINT | `isProbablePrime`. |
| cp-algorithms [Arbitrary-Precision Arithmetic](https://cp-algorithms.com/algebra/big-integer.html), OI Wiki [高精度](https://oi-wiki.org/math/bignum/), Library Checker `big_integer` problems | Coverage of the contest-facing operations and the hex problems; the legacy `OLD/1-Core/05-infint.hpp` supplied the sentinel rules, the window-width cost model of `powMod`, the half-GCD recursion and the AVX2 hex and popcount kernels, which were rewritten in the new layout. |

The 2026-10-17 catalog sweep is in [00-sources.md](00-sources.md) (Fetched 2026-10-17); rejected items with reasons are in [00-notes.md](00-notes.md).

Legacy: the original remains unchanged in `OLD/1-Core/05-infint.hpp`. Every legacy user operation is kept or mapped above; intentional changes: `read` validates and reports instead of parsing garbage, bases above 36 use `toDigits`/the digit constructor instead of the bracket format, `powMod` reduces by Montgomery REDC for odd moduli, `sqrt` uses the recursive square root instead of Newton with full divisions, `gcd` uses binary gcd and Lehmer steps below the half-GCD size, `newtonRaphsonInv` is exact, the native casts of finite values are modulo 2^64 as before, `popcount` of a negative value is still that of its magnitude.

## Limits and handoffs

- Products need `n + m <= 2^28` words (`NTT_MAX_WORDS`) through the transforms, so that the Goldilocks NTT's piece count 2^30 fits an `int` (longer products fall back to Toom-3 and Karatsuba on top of them); the three-prime NTT caps transform lengths at 2^26 limbs, the double FFT is used only while the shorter operand has at most 2^14 words (coefficients below 2^48; the all-ones full-mode case at that size is exact) and the transform at most 2^26 limbs, beyond which the Goldilocks NTT (scalar) or the three-prime NTT (AVX2) takes over. At its 2^26-limb bound the double FFT's two value and two root tables need 4 GB; `mul`'s chunking keeps its transforms below about 2^19 limbs, so only a direct `transformMul` call reaches the bound (user decision 2026-10-25: recorded, not guarded further).
- `schonhageStrassen` is implemented and tested but not dispatched: at 2^14–2^20 words it measured 3.2–10.5x the three-prime NTT and 1.6–4.4x Toom-3 over the transforms in the AVX2 build, 1.6–3.4x and 0.8–1.5x in the scalar build (Benchmarks), so above `NTT_MAX_WORDS`, where no whole transform applies, Toom-3 and Karatsuba over the transforms stay (user decision 2026-10-25: the backend is kept for completeness, not dispatched). The extrapolation there is not one-sided: the sub-products beyond 2^26 limbs run on the Goldilocks NTT, 5.0x the three-prime cost in the AVX2 build and 1.3x in the scalar build (Cyclic transforms table), so one Toom-3 level costs about 16 three-prime-equivalent transforms in the AVX2 build against the 9–10 measured for Schönhage–Strassen, whose pointwise products stay on the three-prime NTT; a 2 GB-operand measurement would be needed to decide it (user decision 2026-10-25, asked with this extrapolation: Toom-3 stays).
- `isPerfectPower` costs O(pi(b) * n) word operations plus the roots that survive the residue filter: fine to a few hundred thousand bits, slow beyond; `jacobi` and `kronecker` are quadratic; `kthRootRem` is Newton with full-precision divisions (O(M(n) log(b) log(k)) after the floating start), so the cube root of 16,384 words takes about 0.3 s; `isqrtRem` is O(M(n)).
- `isProbablePrime` is Monte Carlo above 2^64 (BPSW, no counterexample known); callers needing a proof use Mathematics' primality rows.
- `divExact` above `HENSEL` quotient words uses the (measured faster) quotient-only Burnikel–Ziegler division; a Newton–Hensel lifting variant measured 1.3–2x slower than it at every size above 48 words and was removed, so `divExactHensel` is the quadratic word-by-word backend.
- `truncatedMul` above 32 words is the full product truncated (no low-half Karatsuba); the measured gain of the basecase below 32 words is 1.5–2x.
- The transform cyclic kernels use function-local static scratch vectors and root tables, so a call is not reentrant; single-threaded contest execution is the baseline.
- The middle product's transform path only shortens the reciprocal's residual when the power-of-two rounding of the transform lengths differs between the window and the full product (1.0–2x by size), so below `FFT` it takes the window of the full product; `divNewton` and the Hensel quotient use full products.
- The Python driver streams the protocol corpus, so its memory stays small; the recorded scope peaks (up to 3.6 GB) come from the ASan/UBSan builds of the self-test, whose quarantine and shadow memory multiply the 1.4 million checks' allocations; the optimized self-test stays below 30 MB.
- `operator>>` extracts the token with `is >> string` (about 1.1 ns per character here, most of a Library Checker hex addition's run time); a solution that needs faster input reads the token itself and calls `read`.
- `std::hash` is not cryptographic and not stable across library versions.
- Python 3.10 execution of the driver was not run (3.10 grammar only); the six Library Checker sources under `97-Online Testing/src/01-Yosupo/02-Big Integers` compile unchanged.

### Optimality re-check (2026-10-25)

Judge data pulled through the Library Checker API (`00-Guidelines/09-sources.md`, Fastest submissions) for the latest submissions: hex addition #413221 44 ms (#5 of 40 distinct users; fastest 9, 9, 28, 36), hex multiplication #413222 63 ms (#6 of 40; 17, 19, 36, 44, 48; GMP through `LD_PRELOAD` 60), hex division #413223 200 ms (#7 of 29; 28, 40, 113 GMP, 185 GMP, 192); per case, division `small_00` 154 against 11 and `length_ratio_integer_02` 200 against 24, multiplication `max_max` 53 against 16, addition `small_00` 44 against 9 with the large addition tests at the 4–7 ms floor. Causes measured here: per-value heap allocation in `read`, `toString`, `divMod` and the word copies (about 50 ns each, the whole gap on the 160k–576k-case tests); 9,246 minor page faults and 42 MB RSS per maximal product (12 MB root tables, 12 MB NTT scratch, 8 MB CRT output) against about 90 faults with huge pages; nine breadth-first radix-2 passes per prime plus a scalar CRT (23 ms user against 12.2 for the leaders' AVX2-FMA double FFT at 100k × 100k words on the same CPU family; GMP 25.2); Burnikel–Ziegler below four divisor lengths and two uncached full products per Newton block (88 ms per ratio-5 division against about 22). Every faster multiplication submission is a floating-point FFT with an unchecked error budget (about 2^47–2^48.7 at the maximal shape); this header's kernel is the only provably exact one among them. User decisions (2026-10-25): proven-bound FFT (Percival 2003; Brent, Percival and Zimmermann 2007) with the exact NTT kept as a backend, three stages of the single batch C04 (below; the plan tool allows one owning batch per header, so each stage is one `/package P018` session closed by `/finish P018` as `partial` until the last), `toChars` and stream-buffer stream operators (the row is `partial` until stage 1 implements `toChars`), `MADV_HUGEPAGE` behind `#ifdef __linux__`, a `/speed-sweep` skill for Core full types with Library Checker as its first source, the P025 sweep after P018 and P004/P005 only if that sweep implicates the reductions. After stage 1, `/speed-sweep P018` (scout) writes the `### Optimality gap` baseline with `96-Local Testing/00-Tools/07-judge.py`; after stages 2 and 3, `/speed-sweep P018 verify` refetches the lists and per-case tables and reruns the reproduction (user decision 2026-10-25: these prototype-era numbers are not converted, since stage 1 changes them).

Stages: (1) representation and memory: 64-bit limbs with four inline words, allocation-free parse and print (`toChars`, stream operators through the stream buffer), const-reference `divMod`, 64-bit carry kernels, one thread-local scratch arena (2 MiB-aligned, `MADV_HUGEPAGE` behind `#ifdef __linux__`) replacing the per-call vectors and the 12 MB root tables, the P025 handoff; no numerical change. (2) Large multiplication: AVX2-FMA depth-first radix-4 real-packed double FFT with the piece width from the proven Percival / Brent–Percival–Zimmermann worst-case bound (derivation recorded before dispatch; expected k = 13 at 2^20 points), a scalar twin with identical integer output, the three-prime NTT kept as the exact backend beyond the proven table and upgraded (depth-first radix-4, fused split and CRT, small twiddles), dispatcher thresholds re-measured, judge reproduction against the leaders' per-case times. (3) Division: Newton reciprocal with one-sided error, Barrett blocks with cached spectra of divisor and reciprocal, cyclic mod B^m − 1 remainder product, Knuth D to about 128 quotient words, one-shot Newton for short quotients, Burnikel–Ziegler kept only where measured faster, radix conversion and `divExact` re-measured, judge reproduction.

### Dependent-owner handoff

- Mathematics rows 05 and 36 (P012/P180 owners): `nextPrime`, `prevPrime` and `primorial` have no owner in any inventory; the sweep found them in GMP and FLINT. `bpsw` in row 36 can delegate big inputs to `isProbablePrime` and keep deterministic native proofs.
- P019 (`08-infintmini.hpp`, `09-rational.hpp`): the mini keeps its own sentinel decision; `Rational<InfInt>` can rely on `divModFloor`, `gcd` and `divExact` here.
- P025 (`16-poly.hpp`): `Poly<InfInt>` and `kronecker` keep working on the unchanged `n`/`sgn`/`trim`/`isNil` surface (quick consumer run recorded above); `16-poly_tester.cpp` still wraps the include in diagnostic pragmas that are no longer needed.

- P025 (`16-poly.hpp`), from stage 1 of the 2026-10-25 re-check: the limb surface becomes 64-bit words with inline small storage, so `Poly<InfInt>`, `kronecker` and the `BigInt` concept are re-run and adapted in that batch's handoff.

## History

- 2026-10-17: P018 rewrite verified: new word-kernel header, tester, benchmark; full suites on GCC 16.2 and GCC 14.4 and one stress round passed; the review's findings (sentinel-clearing `read`, `inf` token in bases 24–36, unbounded `NDEBUG` paths, `/=` aliasing, projections, unreachable kernels, naming) fixed and the suites rerun.
- 2026-10-17: `infint_detail` merged into one namespace block after the runs; quick suite, compile matrix and benchmark rerun.
- 2026-10-25: optimality re-review after the Library Checker hex regressions (Library Checker reproduction above): hex digit validation fused into the `parsePow2` kernels, one reversed store per hex output group, one-word fast paths in `read`, `toString` and `divMod`, the power-of-two divisor path, AVX2 `FFT` 1024 → 512 and `FFT_SQR` → 768, scalar `FFT_SQR` → 4096, the chunk estimator in `mul` with `unbalancedMul(..., c)`, a 2^26-limb bound on the double FFT; tester (rejection sweep, power-of-two divisors, unbalanced shapes) and benchmark grid extended; the review found no wrong answer and its findings (comment cap, `s > m` wording, both-build chunk shapes, the FFT bound, two evidence nits) were fixed; full suites on GCC 16.2 and 14.4, one stress round, the consumer and integration runs passed and the benchmark was rerun. `/finish` the same day: the audit's findings fixed (`NTT_MAX_WORDS` 2^29 → 2^28 against an `int` overflow of the Goldilocks piece count, the stale 2^24 limit, the shapes and squaring-threshold wording, the `mul` comment), user decisions recorded (omission list, `FFT_SQR = 768`, the FFT memory bound, Burnikel–Ziegler and the one-word cut kept) and, at the user's request, `schonhageStrassen` with `Fermat` added as a tested backend (not dispatched: measured slower than Toom-3 over the transforms); tester extended (every byte at every hex position, eleven chunk lengths per shape, the estimator in quick mode, the new backend in the self-test and the protocol); suites, consumer, integration and the product benchmark rerun.
