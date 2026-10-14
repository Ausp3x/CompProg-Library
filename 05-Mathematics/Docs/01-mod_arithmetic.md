# 01-mod_arithmetic.hpp — evidence

`01-mod_arithmetic.hpp` implements scalar fixed-width arithmetic. All functions are stateless and header-safe. The signed-input APIs accept every `lng` value, including its minimum; wide results avoid narrowing the magnitude `2^63`. The header uses native division and 128-bit products; it introduces no Barrett/Montgomery context.

## Contracts

### ExtendedGcd, gcd64, lcmWide, extendedGcd

- `gcd64(a, b)` returns the nonnegative gcd as `ulng` (so `gcd(MIN, 0) = 2^63` fits); `gcd(0, 0) = 0`. `lcmWide` returns the nonnegative lcm in `lll` (at most `2^126`) and 0 when either input is 0.
- `extendedGcd(a, b)` returns `ExtendedGcd {g, x, y}` with `a * x + b * y = g >= 0`, all in `lll`; `(0, 0)` gives `(0, 1, 0)`.

Correctness: Euclid preserves two linear combinations of the original inputs while replacing `(u, v)` with `(v, u - q * v)`; the final value is the gcd up to sign, and negating all three outputs preserves the identity. Coefficients for signed 64-bit inputs fit the 128-bit workspace. Lcm divides by the gcd before multiplying the absolute values. Euclid and lcm take O(log) word operations.

### floorDiv, ceilDiv, modNorm, modMul, modMul64

- `floorDiv(a, b)` and `ceilDiv(a, b)` divide signed 128-bit values in either sign direction. They assert a nonzero divisor and a representable quotient, which excludes only `lll` minimum divided by `-1`.
- `modNorm(a, m)` takes a signed 128-bit value and a positive `lng` modulus (asserted) and returns the representative in `[0, m)`.
- `modMul(a, b, m)` accepts arbitrary signed operands and positive `m`, normalizing before the 128-bit product. `modMul64(a, b, m)` accepts full unsigned operands and nonzero unsigned `m` (asserted).

Correctness: truncating division differs from floor or ceiling by exactly one when its nonzero remainder has the matching sign; testing remainder signs avoids risky products and never negates the signed minimum. Normalization adds the modulus only to a negative remainder, so it cannot overflow. All O(1).

### checkedAdd, checkedMul, saturatingAdd, saturatingMul

- `checkedAdd(a, b, out)` and `checkedMul(a, b, out)` return true and write the exact `lng` result when it fits; otherwise they return false and leave `out` unchanged. `out` may alias an input.
- `saturatingAdd` and `saturatingMul` return the exact result when it fits and otherwise the `lng` extreme with the sign of the true result: `INT64_MIN` if the true result is negative, else `INT64_MAX`.

Correctness: GCC overflow builtins on a temporary. Signed addition overflows only when both operands share a sign, so the sign of `b` gives the saturation direction; a product overflows toward the sign `(a < 0) != (b < 0)`. O(1).

### invMod2p64

`invMod2p64(a)` asserts that `a` is odd and returns the unique `x` with `a * x = 1 (mod 2^64)`. The start `(3 * a) ^ 2` is correct to 5 bits, and each Newton step `x *= 2 - a * x` doubles the correct low bits: 5, 10, 20, 40, 80. So four steps suffice (O(log w)), with all arithmetic intentionally wrapping in `ulng`.

### intPow, modPow, modPow64

- `intPow(a, e, out)` takes a signed base and any unsigned exponent, with `0^0 = 1`. It returns false on signed 64-bit overflow without changing `out`.
- `modPow(a, b, mod)` requires `mod > 0` and accepts every signed exponent, including the minimum. A negative power uses the Bézout unit inverse and returns `-1` when `a` is not a unit. Modulus 1 is the zero ring and returns 0, including for negative powers.
- `modPow64(a, e, m)` accepts a full unsigned base, exponent and nonzero modulus; `0^0 = 1 % m`.

Correctness: binary exponentiation preserves `result * base^remaining = original^exponent` and stops squaring once no exponent bits remain. In checked exponentiation an overflowing square that is still needed means the final absolute value cannot fit; the exceptions 0 and units never overflow, so false never rejects a representable result. O(log e) multiplications, O(1) memory.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `exGcd(a, b, x, y)` | `auto [g, x, y] = extendedGcd(a, b)`: `lll` fields, also for `gcd(MIN, 0) = gcd(MIN, MIN) = 2^63` |
| `modPow(a, b)`, default modulus `INF64` | `modPow(a, b, INF64)` |

## Feature-to-test map

Tester: [`01-mod_arithmetic_tester.cpp`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.cpp>) driven by [`01-mod_arithmetic_tester.py`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py>). Oracle: exact Python integers. Quick mode uses 200 random rounds, full 5,000, stress 30,000; each adds the exhaustive and boundary corpus. 11 assertion probes run in the checked build.

| API | Independent coverage |
|---|---|
| `gcd64`, `lcmWide` | `math.gcd`/`math.lcm`, exhaustive `[-24,24]^2`, signed boundary matrix, random pairs (op `g`) |
| `extendedGcd` | Python gcd plus the exact Bézout identity (op `g`), including `gcd(MIN, 0)` and `gcd(MIN, MIN)`. Boundary regression `extendedGcd(MIN, 0)`: `g = 2^63` and Bézout |
| `floorDiv`, `ceilDiv` | Python floor arithmetic on every small sign pair, `lll` minima and maxima, random signed-128 pairs (op `d`); probes for a zero divisor and `MIN / -1` |
| `modNorm`, `modMul`, `modMul64` | Python `%` on exact products, including minimum 128-bit inputs and `(2^64 - 1)^2` (ops `n`, `m`, `u`); zero and negative modulus probes |
| `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` | Op `c` against exact sums and products with range checks and clamping: signed boundary matrix, random pairs, and random small-times-shifted pairs near the product threshold. Untouched outputs are checked through the sentinel 123 |
| `invMod2p64` | Op `v` against `pow(a, -1, 2^64)` for 1, 3, 5, `2^64 - 1`, `2^64 - 3`, `2^63 ± 1`, the 64 smallest odd words and random odd words; probe `inv-even` |
| `intPow` | Exact powers for bounded exponents, zero and unit huge exponents, the kth-root overflow thresholds for every exponent 2–63, and the boundary regression `(-2)^63` (op `i`) |
| `modPow` | Three-argument `pow` (op `p`) with moduli 1, 2, 6, 97, `INT64_MAX`, `INF64 = 0x3f3f3f3f3f3f3f3f` and random. Nonunits, signed-minimum exponent, modulus one, and the zero-modulus probe |
| `modPow64` | Full-width boundaries and random inputs (op `q`); zero-modulus probe |

## Commands and results

P012 prune run, 2026-10-12, recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Every suite runs the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations; counts are per configuration and equal on both compilers.

| Suite (`--mode full --seed 1`) | Result (MB peak g++/g++-14) |
|---|---|
| `01-mod_arithmetic` | PASS, 70,022 exact Python cases, 11 assertion probes (374/385) |
| `02-search_algorithms` | PASS, 2,340,101 C++ checks, 16 assertion probes (390/408) |
| `03-equation_solvers` | PASS, 5,019,506 C++ checks and 5,729 Python cases, 4 assertion probes (411/412) |
| `04-sieve_algorithms` | PASS, 545,142 C++ checks, 14 assertion probes (420/434) |
| `05-combinatorics` | PASS, 71,352 Python cases plus C++ table/state checks and the narrow-ring compile rejection, 30 assertion probes (524/536) |
| `06-segmentedsieve` | PASS, 2,390,564 C++ checks, 13 assertion probes (463/478) |
| `02-integration.py` | PASS, 135 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace |
| `03-consistency.py` | no errors |

```bash
for c in g++ g++-14; do
  for s in 01-mod_arithmetic 02-search_algorithms 03-equation_solvers 04-sieve_algorithms 05-combinatorics 06-segmentedsieve; do
    CXX=$c python3 "96-Local Testing/05-Mathematics/${s}_tester.py" --mode full --seed 1
  done
done
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

No tuned dispatch or specialized reducer was introduced, so there is no speed claim to benchmark.

## Sources

Inspected on 2026-09-27, plus the 2026-10-07 completeness sweep in [00-sources.md](00-sources.md):

- [cp-algorithms, Extended Euclidean Algorithm](https://cp-algorithms.com/algebra/extended-euclid-algorithm.html): iterative two-row invariants and coefficient derivation.
- [OI Wiki, 最大公约数](https://oi-wiki.org/math/number-theory/gcd/): Euclid and Stein, the gcd-lcm relation, extended-gcd coefficient bounds. The installed libstdc++ `std::gcd` is already Stein's binary gcd, so no separate binary gcd is kept.
- Newton–Hensel inversion modulo `2^64` is the standard lifting `x <- x * (2 - a * x)`, derived and written independently here from the identity `1 - a * x' = (1 - a * x)^2`.

## Limits and handoffs

`OLD/5-Mathematics/01-exgcdandmodpow.hpp` is unchanged. `modPow` keeps its legacy name and negative-power behavior; `exGcd` and the default modulus map as in Legacy mapping. Researcher candidates that were left out are listed with reasons in [00-notes.md](00-notes.md).

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed before `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` and `invMod2p64` existed.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, added `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` and `invMod2p64`, 3 findings fixed; after `@reviewer` (which probed every odd `a < 2 * 10^6` for `invMod2p64`), full, stress (seed 1) and integration passed.
- 2026-10-12: prune, removed `exGcd` and the `modPow` default modulus (cases moved to `extendedGcd` and op `p` with modulus `INF64`); full suite passed on g++ and g++-14.
