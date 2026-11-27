# 009-modinverse.hpp — Evidence

`009-modinverse.hpp` covers scalar modular inverses over `lng` moduli (extended Euclid, Fermat, the Pierce recurrence) and vector operations over a modint type `M` (default `mint`): the linear inverse table, the batch and zero-tolerant list inverses, and the `i^k` power table.

## Contracts

Scalar functions take any `lng a`, normalize it into `[0, m)`, and return the inverse in `[0, m)` or the sentinel -1 when none exists; a real inverse is never negative, so -1 cannot collide. Vector functions follow the modint conventions: a static or dynamic modulus, with every `setMod` invalidating results computed earlier. There is no state.

### inverseXgcd

Asserts `m >= 1`. Returns the inverse for any modulus in `O(log(m))`, -1 when `gcd(a, m) > 1`, and 0 when `m = 1` (every residue is the unit 0). It works on 128-bit Bézout coefficients from `extendedGcd`, so `m` up to `2^63 - 1` is safe.

### inverseFermat

Asserts `m >= 2`; the caller promises `m` is prime (not checked). Returns `a^(m-2)` via `modPow` in `O(log(m))`, or -1 when `m | a`.

### inversePierce

The legacy `getInvP` (prime modulus, asserted `p >= 2`), made iterative. It uses `inv(a) = -(p / a) * inv(p mod a)`, accumulating `res *= p - p / a` while `a > 1`. Each step strictly decreases `a`, and `p mod a` is nonzero because `p` is prime and `1 < a < p`. The sequence of values of `a` is the Pierce sequence `a_(i+1) = p mod a_i` of Erdős and Shallit, and the loop stops one step before it reaches 0. So the number of steps is below their `P(a, p)`, which is `O(p^(1/3 + e))` for every `e > 0` (Theorem 2; [00-sources.md](00-sources.md#fetched-2026-11-22-source-check)). The paper proves no better bound; the authors believe `O((log p)^2)`. Returns -1 when `p | a`.

### inverseTable

`inverseTable<M>(n)` asserts `0 <= n < M::mod()` and a prime modulus. It returns `inv[0..n]` with `inv[0] = 0`, computed by `inv[i] = -inv[mod mod i] * (mod / i)` in `O(n)`; `mod mod i < i`, so each step uses an earlier entry.

### inverseBatch, inverseList

`inverseBatch(a)` inverts every entry in place for any modulus with one `tryInv` on the total product, in `O(n + log(mod))`. If some entry is a nonunit, so is the product: it returns false and leaves `a` unchanged. An empty vector returns true. Each entry is read before its slot is overwritten, and later steps read only lower indices.

`inverseList(a)` asserts a prime modulus and returns a new vector in which zero entries map to 0 (never the inverse of a unit) and the others to their inverses. The product skips zeros, so the cost is `O(n + log(mod))`.

### powerTable

`powerTable<M>(n, k)` asserts `0 <= n < INT_MAX` and returns `i^k` for `i` in `[0, n]` with `0^0 = 1`, for any modulus (including 1, where every entry is 0). A linear sieve exponentiates only primes and fills composites as `res[i] * res[p]`, since the map is completely multiplicative: `O(n + pi(n) * log(k))`.

## Feature-to-Test Map

Entry: [`009-modinverse_tester.py`](<../../96-Local Testing/05-Mathematics/009-modinverse_tester.py>). The oracles are brute search for the inverse, direct modint multiplication `x * x^(-1) = 1`, direct powers, and Python `pow(a, -1, m)`. Counts given as a / b / c are for quick / full / stress mode.

| Feature | Oracle and edge classes |
|---|---|
| `inverseXgcd` | Brute search for every `a` in `[-3m, 3m]` and `m <= 120`, including `m = 1`; signed extremes `LLONG_MIN` to `LLONG_MAX` with moduli 1, 2, 3, `2^63 - 1` and the largest 63-bit prime; Python on random 63-bit operands with full and power-of-two-sized moduli. |
| `inverseFermat`, `inversePierce` | Brute search on every prime `m <= 120` and every `a` in `[-3m, 3m]` (multiples of `m` give -1); Python on random primes up to `2^63` and on multiples of `p`. |
| `inverseTable` | `t[i] * i = 1` for every `i` through 2 * 10^4 / 10^6 / 5 * 10^6 with `mint`, and for moduli 13 (n = 12, the maximum), 2, `2^61 - 1` and dynamic 10^9 + 7; the empty table. |
| `inverseBatch`, `inverseList` | Sizes 0, 1, 2, 17, 1000 and N/10 with 0, 1 or 3 zeros, for `mint` and modulus 13; zeros map to 0 in the list; a failing batch leaves the input unchanged; composite dynamic modulus 1000 with units, and with a nonunit. |
| `powerTable` | Six exponents (0, 1, 2, 5, `p - 1`, `2^64 - 1`) and sizes 0..N/10 against modint `pow`, and against plain modular exponentiation for modulus 1000; modulus 1. |
| Preconditions | 8 checked-build probes: `m = 0`, `m = 1` for Fermat, `p = 0` for Pierce, a negative table size, `n = mod`, composite moduli for the table and the list, a negative power-table size. |

## Commands and Results

Conditions as in [007-primality_factorization.md](007-primality_factorization.md). The 2026-10-09 rows are the latest with recorded counts; their memory peaks were not recorded.

| Date, package | Command | Result |
|---|---|---|
| 2026-10-09 P052 | `009-modinverse_tester.py --mode quick --seed 1` | PASS, 2 configurations, 8 probes |
| 2026-10-09 P052 | `009-modinverse_tester.py --mode full --seed 1` | PASS, 3 configurations, 3,665,765 C++ checks and 20,000 Python cases each |
| 2026-10-09 P052 | same with `CXX=g++-14` | PASS, 3 configurations |
| 2026-10-09 P052 | `009-modinverse_tester.py --mode stress --seed 1` | PASS, 3 configurations, 18,065,765 C++ checks and 120,000 Python cases each |
| 2026-11-10 P274 (seed 42, beside the other suites, build cache emptied before each cold run) | `01-run.py --mode full` (GCC 16.2.1) | PASS; pre-flight quick peak 745 MB; optimized 0.3 s compile + 2.6 s run, checked 16.2 s + 2.6 s, ASan-UBSan 14.9 s + 22.5 s; MEMORY peak 869 MB |
| 2026-11-10 P274 | `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS; pre-flight quick peak 789 MB; optimized 0.3 s + 2.8 s, checked 19.4 s + 2.2 s, ASan-UBSan 22.2 s + 13.4 s; MEMORY peak 923 MB |
| 2026-11-10 P274 | `01-run.py --mode quick` | PASS; optimized 18.1 s + 0.2 s, checked 18.6 s + 0.8 s; MEMORY peak 748 MB |
| 2026-11-10 P274 | `01-run.py --mode stress --rounds 1` | PASS; pre-flight quick peak 747 MB; optimized 0.1 s + 3.2 s, checked 5.1 s + 4.1 s, ASan-UBSan 5.3 s + 34.2 s; MEMORY peak 660 MB; check counts not recorded |

```bash
python3 '96-Local Testing/05-Mathematics/009-modinverse_tester.py' --mode quick --seed 1
python3 '96-Local Testing/05-Mathematics/009-modinverse_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/009-modinverse_tester.py' --mode full --seed 1
python3 '96-Local Testing/05-Mathematics/009-modinverse_tester.py' --mode stress --seed 1
```

The header and tester build without warnings under `-Wall -Wextra -Wconversion`, including two translation units.

## Benchmarks

None required: the scalar paths use `extendedGcd` and `modPow`, and the vector paths use the modint type's own reduction.

## Sources

[00-sources.md](00-sources.md), P052 sweep: cp-algorithms modular inverse (linear table, batch trick), ACL `inv_mod`, KACTL `ModInverse.h`, maspypy `power_table`. Erdős–Shallit (1991), read on 2026-11-22, for the `inversePierce` step bound ([00-sources.md](00-sources.md#fetched-2026-11-22-source-check)). Written independently. Legacy: `OLD/5-Mathematics/06-phiandinverse.hpp` (`getInvE` becomes `inverseXgcd`; `getInvP` becomes `inversePierce`).

## Limits and Handoffs

- `inverseO1` (prime modulus, `p <= 2^30`) is in the row under `missing:`, so the row is `partial`.
- The row's `inverseList (arbitrary values)` became a prime-modulus list that tolerates zeros. Per-element inverses over a composite modulus are a loop over `inverseXgcd`.
- `powerTable` is the completely multiplicative `i^k` table. Powers `a^i` of one base are a running product at the call site.
- The rejected Euler-theorem inverse is in [00-notes.md](00-notes.md).
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): modular inverse, modular multiplicative inverse; 模逆元, 费马小定理 & 欧拉定理, 乘法逆元, 线性求逆元, 批量求逆元, 模反元素, 模倒数.

## History

- 2026-10-09: P052 initial implementation and verification, `@reviewer` found no defect; quick, full and stress passed on g++, full on g++-14.
- 2026-11-07: P263 added the search aliases, then Chinese names from OI Wiki's page titles and common names of the row's operations (Simplified, plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 moved the builds to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; library-wide full (g++ and g++-14), quick and stress runs passed.
