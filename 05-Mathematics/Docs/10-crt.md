# 10-crt.hpp — evidence

`10-crt.hpp` covers Chinese remaindering over `lng` moduli: a status result type, merging two congruences with non-coprime moduli, systems with lcm overflow detection, scaled congruences `a_i x = b_i`, the incremental form, Garner's mixed-radix algorithm with a target modulus, non-coprime systems answered modulo a target, and the legacy `superChiRemThm`.

## Contracts

Moduli are `lng` values of at least 1 (asserted). Residues are any `lng` and are normalized. Wide products are formed in `lll`, so moduli up to `2^63 - 1` are safe. There is no state outside `CrtIncremental`.

### CrtResult

`value`, `modulus`, `ok`, `overflow`. When `ok`, the solutions are exactly `value + t * modulus`, with `value` in `[0, modulus)` and `modulus` the lcm. When `!ok && overflow`, the system is consistent so far but the lcm exceeds `LLONG_MAX`, and the remaining congruences were not examined. When `!ok && !overflow`, it is inconsistent. Failure results have `value = modulus = 0`.

### crt2

Merges `x = a1 mod m1` with `x = a2 mod m2` for any moduli. With `g = gcd(m1, m2)` from `extendedGcd`, it is inconsistent when `g` does not divide `a2 - a1`. Otherwise `t = ((a2 - a1) / g) * x mod (m2 / g)`, where `x` is the Bézout coefficient of `m1`, and the answer is `a1 + m1 * t` modulo `lcm = m1 * (m2 / g)`. Consistency is decided before overflow, so an inconsistent pair never reports overflow. The difference fits `lng`, the product `(a2 - a1) / g * x` fits `lll`, and `a1 + m1 * t < lcm` fits `lng`. `O(log(min(m1, m2)))`.

### crt, crtScaled, superChiRemThm

`crt(cong)` folds `crt2` from `{0, 1}` and stops at the first failure: `O(k * log(max(m)))`, and an empty system gives `{0, 1, ok}`. `crtScaled(eqs)` first reduces every `a_i x = b_i mod m_i` with `03` `solveModEq` to `x = x_i mod m_i / gcd(a_i, m_i)`. Any individually unsolvable equation makes the system inconsistent, and this takes precedence over overflow. Then it calls `crt`. `superChiRemThm(eqs)` is the legacy name: `{value, modulus}`, or `{-1, -1}` for inconsistency or overflow. The legacy template parameter for wide results is dropped; use `crtMod` or `garner` for answers modulo a target.

### garnerDigits, garner

`garnerDigits(cong, digits)` finds `d_i` in `[0, m_i)` with `x = d_0 + d_1 m_0 + d_2 m_0 m_1 + ...`, where `x` is the least nonnegative solution, in `O(k^2 + k * log(max(m)))`. For each `i` it evaluates the earlier digits and the prefix product modulo `m_i` and divides by the product through `extendedGcd`. A non-unit product means some earlier modulus shares a factor with `m_i`, so the call returns false and leaves `digits` untouched. This detects every non-coprime pair. `garner(cong, mod)` asserts `mod >= 1` and returns `x mod mod`, or -1 on a shared factor. For an exact big-integer answer, evaluate the digits by Horner's rule in Core `InfInt`.

### crtMod

Answers an arbitrary system (non-coprime moduli, lcm of any size) modulo `mod >= 1`, or returns -1 when inconsistent. It computes `coprimeBase` of the moduli (`07`). Each `m_i` is then a product of base powers, so for each base element `q` the exact `q`-part `q^(e_i)` of each `m_i` is coprime to the rest. The congruence `x = a_i mod m_i` is equivalent to its projections modulo these parts. For each `q` the largest part dominates, and every other residue must agree with it modulo its own smaller part. The dominant projections form a pairwise coprime system whose product is the lcm, and `garner` reduces that system modulo `mod`. Cost: `coprimeBase` plus `O(k * c * log(max(m)))` for `c` base elements.

### CrtIncremental

`add(a, m)` merges one congruence with `crt2` while the state is ok and returns the new `ok()`. Failure (inconsistency or overflow) is sticky. `value()`, `modulus()` and `ok()` read the state, and `res` exposes the overflow flag. The state starts at `{0, 1, ok}`.

### signedRepresentative

Asserts `m >= 1` and returns the residue of any `lll x` in `[-(m - 1) / 2, m / 2]` (the upper half wins ties for even `m`).

## Feature-to-test map

Entry: [`10-crt_tester.py`](<../../96-Local Testing/05-Mathematics/10-crt_tester.py>). The oracles are enumeration of `[0, lcm)` for every equation in the system, and Python exact big-integer CRT with its own merge and overflow rule.

| Feature | Oracle and edge classes |
|---|---|
| `crt2`, `CrtIncremental` | Every modulus pair through 12 / 24 / 28 with residues across three periods, against enumeration. Overflow at the lcm boundary (`2^63 - 25` with 3). Inconsistency detected before overflow. Extreme residues. A lcm of `2 * (2^62 - 57)`. Sticky failure for both kinds. |
| `crt`, `crtScaled`, `superChiRemThm` | 2000 / 30000 / 150000 random systems of 0–4 congruences, half consistent by construction, and scaled systems with signed coefficients, against enumeration of the least solution and its period. Python: up to 6 congruences with moduli up to 2^62 built from shared random primes, consistent or perturbed, with residues shifted beyond the modulus; scaled triples with random signed coefficients. |
| `garner`, `garnerDigits` | Status against pairwise gcds. Output untouched on failure. Digit ranges, mixed-radix reconstruction and target reduction. Python reconstruction of the digits on large coprime systems, including random 62-bit primes. |
| `crtMod` | Every pair and all random systems with random targets. Python on large non-coprime systems. Empty system, target 1. |
| `signedRepresentative` | Every `m <= 50` and `abs(x) <= 200` against enumeration; 128-bit minimum. |
| Preconditions | 9 checked-build probes: zero moduli in every entry point, zero targets. |

## Commands and results

P052 run (2026-10-09), same conditions as [07-primality_factorization.md](07-primality_factorization.md).

```bash
python3 '96-Local Testing/05-Mathematics/10-crt_tester.py' --mode quick --seed 1           # PASS, 2 configurations, 9 probes
python3 '96-Local Testing/05-Mathematics/10-crt_tester.py' --mode full --seed 1            # PASS, 3 configurations: 1,760,419 C++ checks and 8,000 Python cases each
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/10-crt_tester.py' --mode full --seed 1 # PASS, 3 configurations
python3 '96-Local Testing/05-Mathematics/10-crt_tester.py' --mode stress --seed 1          # PASS, 3 configurations: 3,694,495 C++ checks and 40,000 Python cases each
```

The first full run found a contract ambiguity. `crtScaled` reported overflow even though a later equation was individually unsolvable, while the oracle expected inconsistency. The function now reduces every equation before folding, and the contract states that inconsistency takes precedence. The header and tester build with no warnings under `-Wall -Wextra -Wconversion`, including two translation units.

`@reviewer` (2026-10-09) found one style defect, now fixed: `garnerDigits` and `garner` shared a line after an edit lost a newline (the brace checker does not catch this). Independent probes, all passing: 20,000 enumerated `crt`/`crtMod` systems with up to 7 congruences, and 20,000 large non-coprime `crtMod` systems including 6,456 with lcm overflow. The first stress run timed out in the checked build at the runner's 180 s limit; stress pairs now run through 28 instead of 36 (62 s checked).

## Benchmarks

None required (no Barrett, Montgomery or ISA-specific code).

## Sources

[00-sources.md](00-sources.md), P052 sweep: cp-algorithms CRT and Garner, ACL `crt`, KACTL `CRT.h`, maspypy `nt/crt.hpp` (non-coprime CRT modulo a target via a coprime base). Written independently. Legacy: `OLD/5-Mathematics/07-chiremthmandgarner.hpp`. `chiRemThm` becomes `crt`, `superChiRemThm` keeps its name, and `garner` keeps its name (returning -1 on non-coprime moduli, as before).

## Limits and handoffs

- Exact CRT answers above `LLONG_MAX` are not materialized. `crt` reports overflow; use `crtMod` or `garner` with a target, or the digits with `InfInt`.
- Rejected candidates (fixed-prime CRT, big-integer Garner) are in [00-notes.md](00-notes.md).

## History

- 2026-10-09: P052 initial implementation and verification.
