# 09-modinverse.hpp — evidence

`09-modinverse.hpp` covers scalar modular inverses over `lng` moduli (extended Euclid, Fermat, the Pierce recurrence) and vector operations over a modint type `M` (default `mint`): the linear inverse table, the batch and zero-tolerant list inverses, and the `i^k` power table.

## Contracts

Scalar functions take any `lng a`, normalize it into `[0, m)`, and return the inverse in `[0, m)` or the sentinel -1 when none exists. A real inverse is never negative, so -1 does not collide. Vector functions follow the modint conventions: a static or dynamic modulus, and every `setMod` invalidates results computed earlier. There is no state.

### inverseXgcd

Asserts `m >= 1`. Returns the inverse for any modulus in `O(log(m))`, -1 when `gcd(a, m) > 1`, and 0 when `m = 1` (every residue is the unit 0). It works on 128-bit Bézout coefficients from `extendedGcd`, so `m` up to `2^63 - 1` is safe.

### inverseFermat

Asserts `m >= 2`; the caller promises `m` is prime (not checked). Returns `a^(m-2)` via `modPow` in `O(log(m))`, or -1 when `m | a`.

### inversePierce

The legacy `getInvP` (prime modulus, asserted `p >= 2`), made iterative. It uses `inv(a) = -(p / a) * inv(p mod a)`, accumulating `res *= p - p / a` while `a > 1`. Each step strictly decreases `a`, and `p mod a` is nonzero because `p` is prime and `1 < a < p`. The number of steps is `O(p^(1/3 + e))` for every `e > 0` (Erdős–Shallit bound on Pierce-expansion length, cited from memory; conjectured `O(log(p))`). Returns -1 when `p | a`.

### inverseTable

`inverseTable<M>(n)` asserts `0 <= n < M::mod()` and a prime modulus. It returns `inv[0..n]` with `inv[0] = 0`, computed by `inv[i] = -inv[mod mod i] * (mod / i)` in `O(n)`. `mod mod i < i`, so each step uses an earlier entry.

### inverseBatch, inverseList

`inverseBatch(a)` inverts every entry in place for any modulus with one `tryInv` on the total product, in `O(n + log(mod))`. If some entry is a nonunit, the product is a nonunit, so it returns false and leaves `a` unchanged. An empty vector returns true. Each entry is read before its slot is overwritten, and later steps only read lower indices.

`inverseList(a)` asserts a prime modulus and returns a new vector in which zero entries map to 0 (never the inverse of a unit) and the others to their inverses. The product skips zeros, so the cost is `O(n + log(mod))`.

### powerTable

`powerTable<M>(n, k)` asserts `0 <= n < INT_MAX` and returns `i^k` for `i` in `[0, n]` with `0^0 = 1`, for any modulus (including 1, where every entry is 0). A linear sieve exponentiates only primes and fills composites as `res[i] * res[p]`, since the map is completely multiplicative: `O(n + pi(n) * log(k))`.

## Feature-to-test map

Entry: [`09-modinverse_tester.py`](<../../96-Local Testing/05-Mathematics/09-modinverse_tester.py>). The oracles are brute search for the inverse, direct modint multiplication `x * x^(-1) = 1`, direct powers, and Python `pow(a, -1, m)`.

| Feature | Oracle and edge classes |
|---|---|
| `inverseXgcd` | Brute search for every `a` in `[-3m, 3m]` and `m <= 120`, including `m = 1`. Signed extremes `LLONG_MIN` to `LLONG_MAX` with moduli 1, 2, 3, `2^63 - 1` and the largest 63-bit prime. Python on random 63-bit operands, full and power-of-two-sized moduli. |
| `inverseFermat`, `inversePierce` | Brute search on every prime `m <= 120` and every `a` in `[-3m, 3m]` (multiples of `m` give -1). Python on random primes up to `2^63` and on multiples of `p`. |
| `inverseTable` | `t[i] * i = 1` for every `i` through 2 * 10^4 / 10^6 / 5 * 10^6 with `mint`, and for moduli 13 (n = 12, the maximum), 2, `2^61 - 1` and dynamic 10^9 + 7; empty table. |
| `inverseBatch`, `inverseList` | Sizes 0, 1, 2, 17, 1000 and N/10 with 0, 1 or 3 zeros, for `mint` and modulus 13. Zeros map to 0 in the list. A failing batch leaves the input unchanged. Composite dynamic modulus 1000 with units, and with a nonunit. |
| `powerTable` | Six exponents (0, 1, 2, 5, `p - 1`, `2^64 - 1`) and sizes 0..N/10 against modint `pow` and against plain modular exponentiation for modulus 1000; modulus 1. |
| Preconditions | 8 checked-build probes: `m = 0`, `m = 1` for Fermat, `p = 0` for Pierce, a negative table size, `n = mod`, composite moduli for the table and the list, a negative power-table size. |

## Commands and results

P052 run (2026-10-09), same conditions as [07-primality_factorization.md](07-primality_factorization.md).

```bash
python3 '96-Local Testing/05-Mathematics/09-modinverse_tester.py' --mode quick --seed 1           # PASS, 2 configurations, 8 probes
python3 '96-Local Testing/05-Mathematics/09-modinverse_tester.py' --mode full --seed 1            # PASS, 3 configurations: 3,665,765 C++ checks and 20,000 Python cases each
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/09-modinverse_tester.py' --mode full --seed 1 # PASS, 3 configurations
python3 '96-Local Testing/05-Mathematics/09-modinverse_tester.py' --mode stress --seed 1          # PASS, 3 configurations: 18,065,765 C++ checks and 120,000 Python cases each
```

The header and tester build with no warnings under `-Wall -Wextra -Wconversion`, including two translation units.

`@reviewer` (2026-10-09) found no defect. Its note that the Pierce bound was stated slightly too strongly was accepted: the bound is now `O(p^(1/3 + e))`.

## Benchmarks

None required: the scalar paths use `extendedGcd`/`modPow` and the vector paths use the modint type's own reduction.

## Sources

[00-sources.md](00-sources.md), P052 sweep: cp-algorithms modular inverse (linear table, batch trick), ACL `inv_mod`, KACTL `ModInverse.h`, maspypy `power_table`. Written independently. Legacy: `OLD/5-Mathematics/06-phiandinverse.hpp` (`getInvE` becomes `inverseXgcd`; `getInvP` becomes `inversePierce`).

## Limits and handoffs

- The row's `inverseList (arbitrary values)` was made concrete as a prime-modulus list that tolerates zeros. Per-element inverses over a composite modulus are a loop over `inverseXgcd`.
- `powerTable` is `i^k`, the completely multiplicative table handed over from row 04. Powers `a^i` of one base are a running product at the call site.
- Rejected candidates (O(1) inverse queries, Euler-theorem inverse) are in [00-notes.md](00-notes.md).

## History

- 2026-10-09: P052 initial implementation and verification.
