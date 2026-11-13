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

### floorDivStrict, ceilDivStrict, divModFloor

- `floorDivStrict(a, b)` is the largest integer strictly below `a / b`, `ceilDiv(a, b) - 1`; `ceilDivStrict(a, b)` the smallest strictly above, `floorDiv(a, b) + 1` (Nyaan `under`, `over`). They differ from `floorDiv`/`ceilDiv` exactly when `b` divides `a`. Preconditions (asserted): those of `floorDiv` and a representable result, which excludes `(lll MIN, 1)` for `floorDivStrict` and `(lll MAX, 1)`, `(-lll MAX, -1)` for `ceilDivStrict`.
- `divModFloor(a, b)` returns `{q, r}` with `q = floorDiv(a, b)` and `r = a - b * q`, so `r` has the sign of `b` (zero included) and `|r| < |b|`: Python `divmod`, widened from maspypy's `b > 0`. Preconditions those of `floorDiv`.

Correctness: the strict bounds are the floor/ceiling identities `floor(x) < x` unless `x` is an integer; `ceil(x) - 1` is the largest integer below `x` in both cases, and symmetrically. `b * q` can leave `lll` (`a = lll MIN, b = 3`), so `divModFloor` forms `a - b * q` in wrapping `ulll` arithmetic; the true remainder fits, and arithmetic modulo `2^128` recovers it exactly. O(1).

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

### FixedBasePower

`FixedBasePower(a, m, n, k = 2)` answers `a^e mod m` for one base (光速幂), from `k` tables of base-`B` digits, `B` the least integer at least 2 with `B^k > n`. Domain: any `lng` `a` (normalized), `m > 0`, `n >= 0`, `k >= 2` (asserted); `get(e)` asserts `0 <= e <= n`; `0^0 = 1 % m`. `k = 2` is the sqrt form (`B = isqrt(n) + 1`) with one product per query; `k = 4` serves every `lng` exponent (`n = INT64_MAX` gives `B = 55,109`, 220,436 entries), and larger `k` trades query products for memory (`k = 63` gives `B = 2`). The caller chooses `k` so that `k * B` entries fit in memory. Members: `mod`; the bound `n` and `k`, clamped to 0 and 2 when a violated precondition passes under `NDEBUG`; `base = B`; `pw[j * B + d] = a^(d * B^j)` for `j < k`, `d < B`. Construction makes `k * B` modular products, in O(k * B) time and memory. `get` multiplies the `k` digit entries, `k - 1` products, in O(k); a zero digit's entry is `1 % m`. Under `NDEBUG` an exponent outside `[0, n]` is read through the `k` low base-`B` digits of its `ulng` value, so every index stays inside `pw` and the result is meaningless but bounded. The object is immutable after construction; queries may interleave freely. maspypy's `Power_Query` (base 1024, levels added lazily, any exponent) is the same digit form; here `B` follows from `n` and `k`.

Correctness: `B^k > n >= e`, so `e = sum d_j * B^j` with `k` digits `d_j < B`, and `a^e` is the product of `pw[j * B + d_j]`. The construction keeps `a` equal to `a^(B^j)` while it fills table `j`, multiplying `a^((B - 1) * B^j)` by it to reach `a^(B^(j + 1))`. Every product is formed in `lll` from residues below `m < 2^63`. `B` is found exactly, without floating point: a descending bit search from `2^32` (enough, since `k >= 2`) for the largest `b` with `b^k <= n`, each power check in `lll` stopping once the power exceeds `n`, so a check stays below `2^96`; `B = b + 1`, and `n = 0` gives `B = 2`. For many exponents of one base this replaces `modPow`'s O(log e) per query.

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `exGcd(a, b, x, y)` | `auto [g, x, y] = extendedGcd(a, b)`: `lll` fields, also for `gcd(MIN, 0) = gcd(MIN, MIN) = 2^63` |
| `modPow(a, b)`, default modulus `INF64` | `modPow(a, b, INF64)` |

## Feature-to-test map

Tester: [`01-mod_arithmetic_tester.cpp`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.cpp>) driven by [`01-mod_arithmetic_tester.py`](<../../96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py>). Oracle: exact Python integers. Quick mode uses 200 random rounds, full 5,000, stress 30,000; each adds the exhaustive and boundary corpus (24,276 / 111,027 / 562,506 rows with seed 1). 25 assertion probes run in the checked build.

| API | Independent coverage |
|---|---|
| `gcd64`, `lcmWide` | `math.gcd`/`math.lcm`, exhaustive `[-24,24]^2`, signed boundary matrix, random pairs (op `g`) |
| `extendedGcd` | Python gcd plus the exact Bézout identity (op `g`), including `gcd(MIN, 0)` and `gcd(MIN, MIN)`. Boundary regression `extendedGcd(MIN, 0)`: `g = 2^63` and Bézout |
| `floorDiv`, `ceilDiv` | Python floor arithmetic on every small sign pair, `lll` minima and maxima, random signed-128 pairs (op `d`); probes for a zero divisor and `MIN / -1` |
| `floorDivStrict`, `ceilDivStrict` | Ops `s`, `t` against `-(-a // b) - 1` and `a // b + 1`: every pair in `[-24, 24]^2`, a 19 x 19 grid of `lll` and `±2^63` extremes with neighbours, exact multiples `b * q` and `b * q ± 1` for ten divisors (`±2^64`, `2^126 + 1`, `MAX / 3`, `MIN / 3` among them) with `q` at its range ends, random 128-bit pairs and random exact multiples; probes `floor-strict-zero`, `ceil-strict-zero`, `floor-strict-overflow` `(MIN, 1)`, `ceil-strict-overflow` `(MAX, 1)`, `ceil-strict-overflow-negative` `(-MAX, -1)` |
| `divModFloor` | Op `r` against Python `divmod` on the same pairs, including those where `b * q` leaves `lll` (`MIN / 3`, `MIN / MAX`); probes `divmod-zero`, `divmod-overflow` |
| `FixedBasePower` | Op `f` against `pow(a, e, m)`: moduli 1, 2, 97, `10^9 + 7`, `INT64_MAX` and random, ten bases (`LLONG_MIN`, `LLONG_MIN + 1`, `-10^18`, -2 to 3, `INT64_MAX`); every `e` for `n` in 0 to 3 and `j^2`, `j^2 ± 1` (`j <= 10`); for `n` near 1,000, `10^6`, `10^8` (moduli 1, 2) and `10^12` the endpoints, `j * B ± 1` and random exponents; level counts `k` = 3, 4, 5, 7, 63, 64, 128 (`n = 5`) and 200 up to `n = INT64_MAX` (quick: `k = 4` at `INT64_MAX`, `k` = 128 and 200, and the small cases), with exponents `c * B^i - 1`, `c * B^i` and `c * B^i + 1` for `c` in 1, 2, `B - 1`, `floor(n / B^i)`, `floor(n / B^i) - 1` and three random values, and small cases where `B^k` meets `n` (`k = 4`, `n` = 15 and 16); five base and modulus pairs for each; the default `k` and an explicit `k = 2`; repeated `get` on one object; `base` against the least `B` with `B^k > n` by exact integer bisection, and `pw.size() == k * B`; probes `power-mod-zero`, `power-mod-negative`, `power-n-negative`, `power-get-negative`, `power-get-past`, `power-levels-one` |
| `modNorm`, `modMul`, `modMul64` | Python `%` on exact products, including minimum 128-bit inputs and `(2^64 - 1)^2` (ops `n`, `m`, `u`); zero and negative modulus probes |
| `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` | Op `c` against exact sums and products with range checks and clamping: signed boundary matrix, random pairs, and random small-times-shifted pairs near the product threshold. Untouched outputs are checked through the sentinel 123 |
| `invMod2p64` | Op `v` against `pow(a, -1, 2^64)` for 1, 3, 5, `2^64 - 1`, `2^64 - 3`, `2^63 ± 1`, the 64 smallest odd words and random odd words; probe `inv-even` |
| `intPow` | Exact powers for bounded exponents, zero and unit huge exponents, the kth-root overflow thresholds for every exponent 2–63, and the boundary regression `(-2)^63` (op `i`) |
| `modPow` | Three-argument `pow` (op `p`) with moduli 1, 2, 6, 97, `INT64_MAX`, `INF64 = 0x3f3f3f3f3f3f3f3f` and random. Nonunits, signed-minimum exponent, modulus one, and the zero-modulus probe |
| `modPow64` | Full-width boundaries and random inputs (op `q`); zero-modulus probes with and without a multiplication (`pow64-zero` on `2, 3, 0`, `pow64-zero-exponent` on `2, 0, 0`) |

## Commands and results

Latest runs 2026-11-12 (P012 MA01, experiment `none`; integration, mutation checks and consistency 2026-11-14): Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, the contest runner's optimized `-O2 -DNDEBUG`, checked and ASan-UBSan builds with `-Wall -Wextra -Wconversion -Werror`; each PASS.

| Command | Result |
|---|---|
| `01-mod_arithmetic_tester.py --mode full --seed 1` (GCC 16.2.1) | PASS, 111,027 exact Python cases per build, 25 assertion probes; run 1.5 s optimized, 1.7 s checked, 2.3 s ASan-UBSan; pre-flight 650 MB; `MEMORY peak=749MB` |
| same with `CXX=g++-14` (GCC 14.4.1) | PASS, same counts; pre-flight 673 MB; `MEMORY peak=794MB` |
| `01-mod_arithmetic_tester.py --mode stress --seed 1` | PASS, 562,506 rows per build, 25 probes; run 7.9 s, 5.2 s, 6.8 s; `MEMORY peak=684MB` |
| `05-extraction.py ops --filter 05-Mathematics/01-mod_arithmetic.hpp` | PASS, 24 operations in 21 extractions on g++ and g++-14, 0 failures |
| `05-extraction.py testers --filter 05-Mathematics/01-mod_arithmetic_tester` | PASS, 1 extracted quick run |
| `02-integration.py` | PASS, 190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace; `MEMORY peak=1923MB` (cap 8192 MB), on the final headers |
| `04-mutation.py check` on the three MA01 headers | PASS, 3 of 3 records current (Mutation testing in each document) |
| `03-consistency.py` | no errors |

P012 prune run, 2026-10-12, the latest run of `04-sieve_algorithms` … `06-segmentedsieve` (MA02). GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Every suite runs the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations; counts are per configuration and equal on both compilers.

| Suite (`--mode full --seed 1`) | Result (MB peak g++/g++-14) |
|---|---|
| `04-sieve_algorithms` | PASS, 545,142 C++ checks, 14 assertion probes (420/434) |
| `05-combinatorics` | PASS, 71,352 Python cases plus C++ table/state checks and the narrow-ring compile rejection, 30 assertion probes (524/536) |
| `06-segmentedsieve` | PASS, 2,390,564 C++ checks, 13 assertion probes (463/478) |

```bash
for c in g++ g++-14; do
  for s in 01-mod_arithmetic 02-search_algorithms 03-equation_solvers; do
    CXX=$c python3 "96-Local Testing/05-Mathematics/${s}_tester.py" --mode full --seed 1
  done
done
python3 "96-Local Testing/05-Mathematics/01-mod_arithmetic_tester.py" --mode stress --seed 1
for s in 02-search_algorithms 03-equation_solvers; do
  python3 "96-Local Testing/05-Mathematics/${s}_tester.py" --mode stress --seed 2
done
for s in 01-mod_arithmetic 02-search_algorithms 03-equation_solvers; do
  python3 '96-Local Testing/05-extraction.py' ops --filter "05-Mathematics/$s.hpp"
  python3 '96-Local Testing/05-extraction.py' testers --filter "05-Mathematics/${s}_tester"
  python3 '96-Local Testing/04-mutation.py' run "05-Mathematics/$s.hpp" --seed 1  # the lines changed since each record
  python3 '96-Local Testing/04-mutation.py' check "05-Mathematics/$s.hpp"
done
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

Record: mewt 4.0.0 on blob `c8de1434d7d577eec2df8fff8b1a575d09ebcc94` (2026-11-12, seed 1, 2 lines changed since blob `2b15a0d4a9cc`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The two lines are the `divModFloor` rename (its comment and signature), which yield no mutants, so the run has no stages. The survivors below are those of the whole-file campaign of 2026-11-12 (500 sampled mutants, 30 survived) that no test has closed since; lines are those of the current header, which the rename left in place. Every survivor is equivalent.

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 473`, `a < 0` → `<=` | 8 | At 0 both branches give `ulng(0) = 0`. |
| `COS 483`, `a < 0` → `<=` | 12 | `-lll(0) = 0`, and line 11 returns for a zero input anyway. |
| `AS 182`, `gcd64(a, b)` → `gcd64(b, a)` | 13 | gcd is symmetric. |
| `COS 498`, `COS 523`, `COS 548`, `a == MIN` → `<=` in an assertion; `COS 560`, `a == MAX` → `>=` | 25, 28, 31, 33 | No `lll` lies below the minimum or above the maximum. |
| `COS 568`, `a == -MAX` → `<=` in `ceilDivStrict`'s assertion | 33 | It also rejects `a = MIN, b = -1`, which `floorDiv` on the same line rejects anyway. |
| `COS 513`, `COS 518`, `r < 0` / `b < 0` → `<=` | 26 | Inside `r && ...` the remainder is nonzero, and `b != 0` is asserted (a zero divisor traps in `a / b` first). |
| `AS 191`, `AS 193`, `AS 195`, overflow-builtin operands swapped | 41, 44, 47 | Addition and multiplication commute, and the builtins detect overflow of the exact result. |
| `COS 588`, `b < 0` → `<=` | 48 | The line runs only after an overflowing sum, so `b != 0`. |
| `COS 598`, `COS 603`, `(a < 0) != (b < 0)` operands → `<=` | 51 | The line runs only after an overflowing product, so `a, b != 0`. |
| `COS 607`, `COS 608`, `i < 4` → `!=` / `<=` | 56 | `i` steps by 1 from 0; a fifth Newton step maps the exact inverse to itself (`2 - a * x = 1`). |
| `AS 201`, `(a, a)` swapped | 65 | Identical operands. |
| `COS 624`, `r.g != 1` → `r.g > 1` | 71 | `mod != 0`, so the gcd is at least 1. |
| `IT 173`, `IT 175`, `if (e)` → `if (true)` | 76, 82 | Only adds a final unused squaring, which cannot overflow (`lll` and `ulll` products). |
| `CR 130`, `a %= m` removed | 79 | `modMul64` reduces the full 128-bit product of any `ulng` operands, so an unreduced base gives the same residues, and `e = 0` returns `1 % m` without it. |
| `AS 206`, `modMul64(r, a, m)` → `(a, r, m)` | 81 | Commutative product. |
| `AS 211`, `max(K, 2)` → `max(2, K)` | 90 | Symmetric. |
| `COS 627`, `COS 630`, `m > 0` → `!=` / `>=` | 91 | `modNorm(a, m)` on the same line asserts `m > 0`, so probes `power-mod-zero` and `power-mod-negative` abort either way. |
| `COS 657`, `COS 662`, `COS 677`, loop bound `<` → `!=` | 97, 99, 104 | Each counter steps by 1 from below its bound (`k >= 2`, `base >= 2`). |

## Benchmarks

No tuned dispatch or specialized reducer was introduced, so there is no speed claim to benchmark.

## Sources

Inspected on 2026-09-27, in the 2026-10-07 completeness sweep and in the P012 MA01 sweep of 2026-11-12 ([00-sources.md](00-sources.md)):

- [cp-algorithms, Extended Euclidean Algorithm](https://cp-algorithms.com/algebra/extended-euclid-algorithm.html): iterative two-row invariants and coefficient derivation.
- [OI Wiki, 最大公约数](https://oi-wiki.org/math/number-theory/gcd/): Euclid and Stein, the gcd-lcm relation, extended-gcd coefficient bounds. The installed libstdc++ `std::gcd` is already Stein's binary gcd, so no separate binary gcd is kept.
- Newton–Hensel inversion modulo `2^64` is the standard lifting `x <- x * (2 - a * x)`, derived and written independently here from the identity `1 - a * x' = (1 - a * x)^2`.
- [OI Wiki, 快速幂 (光速幂)](https://oi-wiki.org/math/binary-exponentiation/) and [maspypy `power_query.hpp`](https://maspypy.github.io/library/ds/power_query.hpp): the sqrt tables and base-B digit tables of `FixedBasePower`, written as one type with `k` digit levels.
- [Nyaan `int_div.hpp`](https://nyaannyaan.github.io/library/misc/int_div.hpp): `floorDivStrict`, `ceilDivStrict` (`under`, `over`); [maspypy `my_template.hpp`](https://raw.githubusercontent.com/maspypy/library/main/my_template.hpp): the floor quotient and remainder of `divModFloor`, widened to either divisor sign.

## Limits and handoffs

`OLD/5-Mathematics/01-exgcdandmodpow.hpp` is unchanged. `modPow` keeps its legacy name and negative-power behavior; `exGcd` and the default modulus map as in Legacy mapping. Researcher candidates that were left out are listed with reasons in [00-notes.md](00-notes.md).

Follow-up for the owner of `09-Contest Testing/10-grab.py` (P012 MA01): its `local_names` heuristic took `FixedBasePower,` in a comment inside a `def` of the tester's Python entry for a local name and left the struct out of the extracted tester build. The comment was reworded; the heuristic is unchanged.

- `divModFloor` is named as Core's flooring `InfInt::divModFloor`, since Core's `divMod` truncates (`02-structure.md`, Operation names); this reverses P275's `divModFloor` → `divMod` rename, and `floorDiv` stays beside it as a recorded keep (`23-Sweep Ledgers/duplicates.md`, user decision 2026-11-12). With the user's yes of 2026-11-13, the rename also reached the planned 08-Python row `_03_number_theory.py` (`divmodFloor` → `divModFloor`, also in its `duplicates.md` entry) and P275's `Docs/00-duplicate-rereview.md`, which now notes the reversal.

## Aliases

Search aliases (P263, grab --find): modular exponentiation, binary exponentiation, fast power (快速幂), GCD (greatest common divisor), LCM (least common multiple), Euclidean algorithm, extended Euclid, Bezout identity; 最大公约数, 裴蜀定理 & 一次不定方程, 模算术简介, 扩展欧几里得, 扩欧, 最小公倍数, 辗转相除法, 最大公因数, 快速乘, 龟速乘, 光速乘, fixed-base exponentiation, 光速幂, floor division, divmod, 带余除法, 向下取整, 向上取整.

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed before `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` and `invMod2p64` existed.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, added `checkedAdd`, `checkedMul`, `saturatingAdd`, `saturatingMul` and `invMod2p64`, 3 findings fixed; after `@reviewer` (which probed every odd `a < 2 * 10^6` for `invMod2p64`), full, stress (seed 1) and integration passed.
- 2026-10-12: prune, removed `exGcd` and the `modPow` default modulus (cases moved to `extendedGcd` and op `p` with modulus `INF64`); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change; `龟速乘` and `光速乘` name related methods for the same problem, kept as search synonyms (user decision 2026-11-07).
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-10: /finish P276 (user decision 2026-11-10): search alias `modular exponentiation` restores the `simple` corpus rank that rows added by P254 and P275 had taken; `14-search_tester.py --mode quick` passed.
- 2026-11-12: P012 MA01, added `floorDivStrict`, `ceilDivStrict`, `divModFloor` (first named `divMod`) and `FixedBasePower` after a `@researcher` sweep; the `ceilDivStrict(-MAX, -1)` overflow now asserts and `get` asserts `e <= n`; after two `@reviewer` passes, `FixedBasePower` takes `k` digit levels (exact search for `B`, fixed-length `get`), `modNorm`'s `NDEBUG` addition is wrap-safe, and probe `pow64-zero-exponent` was added; first mutation campaigns, ending with a whole-file campaign on the final blob; full on g++ and g++-14, stress (seed 1), extraction and integration passed.
- 2026-11-12: P012 MA01 reopened (user decisions 2026-11-12): `divMod` renamed `divModFloor`, `floorDiv` kept beside it (`duplicates.md`); changed-lines mutation campaign (no mutants); full on g++ and g++-14, stress (seed 1) and extraction passed.
- 2026-11-14: `/finish MA01`: evidence brought to the latest-run form (commands block per seed and changed-lines campaigns, sources of the MA01 sweep, Mutation testing without resolved findings); with the user's yes the rename reached the 08-Python row and P275's ledger; integration and consistency passed on the final headers; header and tests unchanged.
