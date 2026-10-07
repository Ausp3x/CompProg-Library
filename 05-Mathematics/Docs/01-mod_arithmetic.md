# 01-mod_arithmetic.hpp — evidence

`01-mod_arithmetic.hpp` implements scalar fixed-width arithmetic. All functions are stateless and header-safe. The signed-input APIs accept every `lng` value, including its minimum; wide results avoid narrowing the magnitude `2^63`. The header uses native division and 128-bit products; it introduces no Barrett/Montgomery context. The P012 re-audit (2026-10-07) added the five operations the row had listed as missing and moved the in-code contracts here.

## Contracts

### ExtendedGcd, gcd64, lcmWide, extendedGcd, exGcd

- `gcd64(a, b)` returns the nonnegative gcd as `ulng` (so `gcd(MIN, 0) = 2^63` fits); `gcd(0, 0) = 0`. `lcmWide` returns the nonnegative lcm in `lll` (at most `2^126`) and 0 when either input is 0.
- `extendedGcd(a, b)` returns `ExtendedGcd {g, x, y}` with `a * x + b * y = g >= 0`, all in `lll`; `(0, 0)` gives `(0, 1, 0)`.
- `exGcd(a, b, x, y)` is the legacy `lng` adapter. It asserts that `g` fits `lng` (it does not for `gcd(MIN, 0)` or `gcd(MIN, MIN)`; use `extendedGcd` there) and that `x` and `y` are distinct objects. Inputs are copied, so they may alias the outputs: `exGcd(a, b, a, b)` is valid.

### floorDiv, ceilDiv, modNorm, modMul, modMul64

- `floorDiv(a, b)` and `ceilDiv(a, b)` divide signed 128-bit values in either sign direction. They assert a nonzero divisor and a representable quotient, which excludes only `lll` minimum divided by `-1`.
- `modNorm(a, m)` takes a signed 128-bit value and a positive `lng` modulus (asserted) and returns the representative in `[0, m)`.
- `modMul(a, b, m)` accepts arbitrary signed operands and positive `m`, normalizing before the 128-bit product. `modMul64(a, b, m)` accepts full unsigned operands and nonzero unsigned `m` (asserted).

### checkedAdd, checkedMul, saturatingAdd, saturatingMul

- `checkedAdd(a, b, out)` and `checkedMul(a, b, out)` return true and write the exact `lng` result when it fits; otherwise they return false and leave `out` unchanged. `out` may alias an input.
- `saturatingAdd` and `saturatingMul` return the exact result when it fits and otherwise the `lng` extreme with the sign of the true result: `INT64_MIN` if the true result is negative, else `INT64_MAX`.

### invMod2p64

`invMod2p64(a)` asserts that `a` is odd and returns the unique `x` with `a * x = 1 (mod 2^64)`. The start `(3 * a) ^ 2` is correct to 5 bits, and each Newton step `x *= 2 - a * x` doubles the correct low bits: 5, 10, 20, 40, 80. So four steps suffice, with all arithmetic intentionally wrapping in `ulng`.

### intPow, modPow, modPow64

- `intPow(a, e, out)` takes a signed base and any unsigned exponent, with `0^0 = 1`. It returns false on signed 64-bit overflow without changing `out`.
- `modPow(a, b, mod = INF64)` keeps the legacy name and default modulus `INF64 = 0x3f3f3f3f3f3f3f3f`. It requires `mod > 0` and accepts every signed exponent, including the minimum. A negative power uses the Bézout unit inverse and returns `-1` when `a` is not a unit. Modulus 1 is the zero ring and returns 0, including for negative powers.
- `modPow64(a, e, m)` accepts a full unsigned base, exponent and nonzero modulus; `0^0 = 1 % m`.

## Correctness and complexity

Euclid preserves two linear combinations of the original inputs while replacing `(u, v)` with `(v, u - q * v)`. When it stops, the remaining value is their gcd up to sign, and negating all three outputs when needed preserves the identity. Coefficients for signed 64-bit inputs and their quotient updates fit the 128-bit workspace. Lcm divides by the gcd before multiplying the absolute values.

Truncating division differs from floor or ceiling by exactly one when its nonzero remainder has the matching sign. Testing remainder signs avoids risky products and never negates the signed minimum. Normalization adds the modulus only to a negative remainder, so the addition cannot overflow.

The checked and saturating operations use GCC overflow builtins on a temporary. Signed addition can overflow only when both operands have the same sign, so the sign of `b` gives the saturation direction. A product overflows toward the sign `(a < 0) != (b < 0)`.

Binary exponentiation preserves `result * base^remaining = original^exponent`. Squaring stops once no exponent bits remain. In checked exponentiation, an overflowing square that is still needed means the final absolute value cannot fit. The exceptions would be 0 and units, and their squares never overflow, so false never rejects a representable result.

Euclid and lcm take O(log) word operations, powers take O(log e) multiplications, `invMod2p64` takes four steps (O(log w)), and everything else is O(1) with O(1) memory. No tuned dispatch or specialized reducer was introduced, so there is no speed claim to benchmark.

## Re-audit findings (P012, 2026-10-07)

The re-audit completed `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` and `invMod2p64`, the five operations the row had listed as missing (findings 4 and 5). Confirmed `/reaudit-review` findings for this header:

| # | Finding | Resolution |
|---|---|---|
| 4 | `invMod2p64` listed but absent | Implemented and tested against `pow(a, -1, 2^64)` |
| 5 | Notes/evidence claimed MA01 complete while the row was partial | All five operations implemented; row verified; [00-notes.md](00-notes.md) and this document rewritten |
| 6 | `modPow` default modulus and `exGcd` input/output aliasing untested | Op `P` checks the two-argument overload against `pow(a, b, INF64)`; boundary regression runs `exGcd(a, b, a, b)` |

## Feature-to-test map

Tester: [`01-mod_arithmetic_tester.cpp`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.cpp>) driven by [`01-mod_arithmetic_tester.py`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py>). Oracle: exact Python integers.

| API | Independent coverage |
|---|---|
| `gcd64`, `lcmWide` | `math.gcd`/`math.lcm`, exhaustive `[-24,24]^2`, signed boundary matrix, random pairs (op `g`) |
| `extendedGcd`, `exGcd` | Python gcd plus the exact Bézout identity (op `g`). Death probes for gcd width and output aliasing. Regression `exGcd(a, b, a, b)` with `a = 6`, `b = 4` checks `g = 2` and Bézout against the saved inputs |
| `floorDiv`, `ceilDiv` | Python floor arithmetic on every small sign pair, `lll` minima and maxima, random signed-128 pairs (op `d`); probes for a zero divisor and `MIN / -1` |
| `modNorm`, `modMul`, `modMul64` | Python `%` on exact products, including minimum 128-bit inputs and `(2^64 - 1)^2` (ops `n`, `m`, `u`); zero and negative modulus probes |
| `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` | Op `c` against exact sums and products with range checks and clamping: signed boundary matrix, random pairs, and random small-times-shifted pairs near the product threshold. Untouched outputs are checked through the sentinel 123 |
| `invMod2p64` | Op `v` against `pow(a, -1, 2^64)` for 1, 3, 5, `2^64 - 1`, `2^64 - 3`, `2^63 ± 1`, the 64 smallest odd words and random odd words; probe `inv-even` |
| `intPow` | Exact powers for bounded exponents, zero and unit huge exponents, the kth-root overflow thresholds for every exponent 2–63, and the boundary regression `(-2)^63` (op `i`) |
| `modPow` | Three-argument `pow` with explicit moduli (op `p`). The two-argument default `INF64` overload against `pow(a, b, 0x3f3f3f3f3f3f3f3f)` (op `P`). Nonunits, signed-minimum exponent, modulus one, and the zero-modulus probe |
| `modPow64` | Full-width boundaries and random inputs (op `q`); zero-modulus probe |

## Commands and results

The suite builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations, and runs 13 assertion probes in the checked build. Quick mode uses 200 random rounds, full 5,000, stress 30,000; each adds the exhaustive and boundary corpus. No online submission was made.

Package-wide P012 runs (2026-10-07), recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1, GNU++20, Python 3.14, Linux x86-64 (i9-11900H). Every C++ build uses the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (quick runs the first two). Before any re-audit change, the full suite (`01-run.py --mode full --filter 05-Mathematics/0 --seed 1 --no-integration`) passed all six entries.

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 05-Mathematics/0 --no-integration                     # PASS, 6 suites, 2 configurations
python3 '96-Local Testing/01-run.py' --mode full --filter 05-Mathematics/0 --seed 1 --no-integration              # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/01-run.py' --mode stress --filter 05-Mathematics/0 --seed 1 --rounds 1 --no-integration # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/02-integration.py'                                                                     # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                     # no errors
```

After the independent `@reviewer` pass, the full suite was rerun on all six headers, stress on combinatorics, and integration; all passed with the counts below.

| Mode | Count |
|---|---|
| Full (seed 1), per configuration | 70,022 exact Python cases |
| Stress (seed 1) | 370,022 |
| Assertion probes | 13 |

The `@reviewer` pass independently probed every odd `a < 2 * 10^6` for `invMod2p64`.

## Sources

Inspected on 2026-09-27, plus the 2026-10-07 completeness sweep in [00-sources.md](00-sources.md):

- [cp-algorithms, Extended Euclidean Algorithm](https://cp-algorithms.com/algebra/extended-euclid-algorithm.html): iterative two-row invariants and coefficient derivation.
- [OI Wiki, 最大公约数](https://oi-wiki.org/math/number-theory/gcd/): Euclid and Stein, the gcd-lcm relation, extended-gcd coefficient bounds. The installed libstdc++ `std::gcd` is already Stein's binary gcd, so no separate binary gcd is kept.
- Newton–Hensel inversion modulo `2^64` is the standard lifting `x <- x * (2 - a * x)`, derived and written independently here from the identity `1 - a * x' = (1 - a * x)^2`.
- `OLD/5-Mathematics/01-exgcdandmodpow.hpp` is unchanged. Both legacy public names and the negative-power behavior remain, now with explicit representability rules.

Researcher candidates that were left out are listed with reasons in [00-notes.md](00-notes.md).
