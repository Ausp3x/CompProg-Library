# MA01 arithmetic contracts and verification

`01-mod_arithmetic.hpp` implements scalar fixed-width arithmetic. All functions are stateless and header-safe. The signed-input APIs accept every `lng` value, including its minimum; wide results prevent narrowing the magnitude `2^63`. This header uses ordinary native division and 128-bit products; it does not introduce Barrett/Montgomery contexts.

## Public feature map

| API | Contract | Independent coverage |
|---|---|---|
| `gcd64`, `lcmWide` | Nonnegative gcd in `ulng`, lcm in `lll`; both-zero gcd and any-zero lcm are zero | Python `math.gcd`/`math.lcm`, exhaustive `[-24,24]²`, signed boundaries and random pairs |
| `extendedGcd` | `g,x,y` in signed 128 bits, `a*x+b*y=g>=0`; `(0,0)` returns `(0,1,0)` | Independent Python gcd and exact Bézout identity |
| `exGcd` | Legacy `lng` adapter; gcd must fit and output references must differ; inputs may alias output variables because inputs are copied | Same identities and asserted width/alias preconditions |
| `floorDiv`, `ceilDiv` | Signed 128-bit division in either sign direction; nonzero divisor and representable quotient | Python floor arithmetic, all small sign pairs, 128-bit minima/maxima, random signed-128 pairs and exceptional assertions |
| `modNorm` | Signed 128-bit input, positive signed 64-bit modulus, result in `[0,m)` | Python `%`, minimum 128-bit input and random full-width values |
| `modMul` | Arbitrary signed 64-bit operands, positive signed modulus | Exact unbounded Python product then remainder |
| `modMul64` | Full unsigned 64-bit operands and positive unsigned modulus | Unbounded product including `(2^64-1)^2` |
| `intPow` | Full unsigned exponent, signed base/result; `0^0=1`; false on overflow without changing output | Python exact powers for bounded exponents, explicit zero/unit huge-exponent cases, exact kth-root overflow thresholds for every exponent 2–63 and `(-2)^63` |
| `modPow` | Legacy name/default `INF64`; full signed exponent; negative powers use a unit inverse, return `-1` if absent | Python three-argument `pow`, nonunits, signed-minimum exponent, modulus one |
| `modPow64` | Full unsigned base/exponent/modulus; nonzero modulus, `0^0=1%m` | Python modular powers at full-width boundaries and random inputs |

`exGcd` cannot represent a gcd of `2^63`; use `extendedGcd` or `gcd64` there. `floorDiv`/`ceilDiv` exclude only zero divisors and signed-128 minimum divided by `-1`. The output of `lcmWide` is at most `2^126`, and normalized signed modular products are below `2^126`. Unsigned products fit unsigned 128 bits exactly. Output aliases in the legacy gcd adapter are rejected because two generally different coefficients cannot occupy one object.

## Correctness and complexity

Euclid preserves two linear combinations of the original inputs while replacing `(u,v)` with `(v,u-q*v)`. At termination the remaining nonnegative value is their gcd; negating all three outputs when needed preserves the identity. Coefficients for signed 64-bit inputs and their quotient updates fit the 128-bit workspace. Lcm divides by the gcd before multiplying absolute magnitudes.

Truncating division differs from floor/ceiling by exactly one when its nonzero remainder has the appropriate sign. Testing remainder signs avoids dangerous products or negating the signed minimum. Normalization adds a modulus only to a negative remainder, so addition cannot overflow.

Binary exponentiation preserves `result * base^remaining = original_base^original_exponent`. Squaring is skipped once no exponent bits remain. In checked integer exponentiation, a required overflowing square with remaining exponent implies the final absolute value cannot fit, except zero and units, whose squares cannot overflow. Thus false never rejects a representable result. Overflow detection uses GCC builtins, not signed overflow. Modular negative powers first apply the Bézout unit inverse; modulus one consistently returns zero, including negative powers.

Euclid/lcm take logarithmically many word operations. Powers take logarithmically many exponent-bit operations; negative modular powers additionally require a gcd. Division/product/normalization are constant word operations. All auxiliary storage is constant. No tuned dispatch or specialized reducer was introduced, so no speedup or universal fastest claim is made and no arithmetic benchmark is needed for an optimization decision.

## Sources and legacy accounting

Inspected on 2026-09-27:

- [cp-algorithms, Extended Euclidean Algorithm](https://cp-algorithms.com/algebra/extended-euclid-algorithm.html): iterative two-row invariants, sign handling and coefficient derivation.
- [OI Wiki, 最大公约数](https://oi-wiki.org/math/number-theory/gcd/): Euclidean/Stein alternatives, gcd-lcm relation, extended-gcd coefficient bounds. The standard library unsigned gcd supplies the compact optimized gcd implementation; arbitrary-precision half-gcd is outside this fixed-width family.
- `OLD/5-Mathematics/01-exgcdandmodpow.hpp`: retained unchanged. Both legacy public names and the negative-power behavior remain available, with explicit representability rules and corrected normalization/intermediate arithmetic.

The routines were independently implemented from these mathematical identities and audited legacy behavior. Unit-inverse tables/batches belong to MA04; arbitrary-precision algorithms belong to Core. Those are separate inventory owners, not incomplete MA01 operations.

## Verification record

GCC 16.2.1, GNU++20, Python 3.14.7, Linux x86-64, seed `20260927`:

```bash
python3 '96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py' --mode full --seed 20260927 --configuration ASan-UBSan
```

Final full mode passed 49,709 exact Python cases in each of optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan builds; the checked build passed 12 assertion probes. An initial sandbox run blocked LeakSanitizer's process inspection. The final full command ran outside the sandbox with leak detection enabled after adding exponent overflow thresholds and random 128-bit divisions. No sanitizer failure was suppressed. Quick reduces random cases to 200, full uses 5,000, and stress uses 30,000; all include the bounded exhaustive/boundary corpus. Tests run from arbitrary working directories through script-relative paths and retain checks under NDEBUG. No online submission was made.
