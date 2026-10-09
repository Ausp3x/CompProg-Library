# 08-multiplicative_functions.hpp — evidence

`08-multiplicative_functions.hpp` covers multiplicative and additive arithmetic functions. Point values for any 64-bit `n` come from `factorize`. Prefix tables come from one generic linear-sieve hook. Divisor-indexed arrays have zeta and Möbius transforms in both directions, and there are Dirichlet convolution and inversion tables. Point `divisorCount` and `divisorSum` belong to `07`. Sublinear prefix sums (Du, min_25, Lucy) are separate rows.

## Contracts

There is no global state. Point functions take `ulng n >= 1` (asserted via `factorize`) in `O(n^(1/4) * log(n))` expected time. Tables take `0 <= n < INT_MAX` and return `n + 1` entries indexed by value, with entry 0 a convenience 0 (`T(0)`) and entry 1 equal to 1. Ring templates need `T(int)`/`T(ulng)` construction and `+=`, `*=`, `*`, `-`. Unsigned `T` wraps, and modint `T` gives residues.

### phi, mobius, omega, bigOmega, liouville, carmichaelLambda

Euler's totient (exact `ulng`), Möbius in {-1, 0, 1}, the number of distinct primes, the number of primes with multiplicity, Liouville `(-1)^Omega(n)`, and Carmichael's `lambda(n)`, the least `m` with `a^m = 1` for every unit. `lambda` is the lcm over prime powers of `phi(p^e)`, halved for `2^e` with `e >= 3`. It is at most `n`, so the lcm cannot overflow.

### sigmaK, jordanTotient

`sigmaK<T = ulll>(n, k) = sum(d^k)` over divisors, and `jordanTotient<T = ulll>(n, k) = prod(p^(k(e-1)) * (p^k - 1))`. Both compute in `T`: exact while the value fits, wrapping modulo 2^128 for the default type, and modular for a modint. The cost is the factorization plus one `O(log(k))` power per prime.

### multiplicativeTable

`multiplicativeTable<T>(n, f)` calls `f(p, k, p^k)` exactly once for each prime power `p^k <= n` (as `int` arguments, returning `T`) and extends the values multiplicatively. A linear sieve records, for each `j`, its full smallest-prime part `pw[j]` and that part's exponent. When `j` is a prime power, it calls `f`; otherwise `res[j] = res[pw[j]] * res[j / pw[j]]`, where both factors are smaller and already final. Every composite is produced exactly once from `i * spf`. `O(n)` time plus the `f` calls; space is 5 bytes per entry beyond the result. A completely multiplicative table such as `i^K` is `f = (p, k, pk) -> pow(T(pk), K)`; `09` `powerTable` is the specialized version with one power per prime.

### phiTable, mobiusTable, divisorCountTable, divisorSumTable, prefixPhiTable, prefixMobiusTable

Instances of the hook, returning `vector<int>`, `vector<int8_t>`, `vector<int>`, `vector<lng>`, `vector<lng>` (Euler-phi prefix sums, below 2^61 for `n < 2^31`) and `vector<int>` (Mertens). They duplicate `LinearSieve` tables deliberately: they are standalone free functions, and the prefix sums are new.

### dirichletConvolutionTable, dirichletInverseTable

Inputs are vectors of size `n + 1`, indexed 1..n, with index 0 ignored and returned as `T(0)`. Convolution asserts equal non-empty sizes and computes `c[m] = sum(a[d] * b[m / d])` over `d | m` in `O(n * log(n))`. The inverse asserts size at least 2 and that `a[1]` is a unit with exact `T(1) / a[1]` (for integers, `a[1] = ±1`; for modints, nonzero modulo a prime). It finalizes `res[i] = -a[1]^(-1) * sum(a[d] * res[i / d])` over `d > 1` in increasing `i`, pushing contributions forward, in `O(n * log(n))`. The caller must keep `T` from overflowing (signed integer inverses grow quickly; use a modint or an unsigned type).

### DivisorArray

`DivisorArray<T>(n)` or `DivisorArray<T>(factorization)` stores `fac`, `divs` (all divisors of `n` in mixed-radix order: index `sum(e_i * stride_i)` with `stride_0 = 1`, unsorted) and `val` (initialized to `T(0)`). `build` rebuilds with the same rules. Default `n = 1`.

- `size()` returns `d(n)`. `operator[](i)` accesses by index. `get(d)` accesses by divisor through `index(d)`, which asserts `d >= 1` and `d | n` and costs `O(omega(n) + log(d))`.
- `setMultiplicative(f)` fills `val[d] = prod f(p, k, p^k)` (`ulng p`, `int k`, `ulng p^k`).
- `zeta()` replaces `val[d]` with `sum(val[e])` over `e | d`, and `mobius()` inverts it. `multipleZeta()` replaces `val[d]` with `sum(val[e])` over `d | e | n`, and `multipleMobius()` inverts that. Each is a prefix sum or difference along every prime axis of the exponent grid, costing `O(d(n) * omega(n))`.

Fields are public query data. Changing `fac` or `divs` other than through `build` breaks the indexing.

## Feature-to-test map

Entry: [`08-multiplicative_functions_tester.py`](<../../96-Local Testing/05-Mathematics/08-multiplicative_functions_tester.py>). The oracles are the tester's own trial factorization, gcd counting, brute multiplicative orders, divisor enumeration, `O(n^2)` divisor sums, and Python closed forms on `n` built from known primes.

| Feature | Oracle and edge classes |
|---|---|
| `phi`, `mobius`, `omega`, `bigOmega`, `liouville` | Trial-factor formulas for every `n` through 3000 / 30000 / 150000. `phi` by gcd counting through 400. Python closed forms on 300 / 4000 / 20000 values up to 2^64 with up to 8 known primes. |
| `carmichaelLambda` | lcm of brute multiplicative orders through 400; through 3000, every unit satisfies `a^lambda = 1` and some unit fails `lambda / q` for each prime `q`; Python. |
| `sigmaK`, `jordanTotient` | Divisor sums of `d^k` for `k` in 0..3, exact and modint with exponent `k + 10^9 + 7`, through 2000. `sum J_k(d) = n^k` for `k` in 1..3. Pair counting `J_2` and `J_1 = phi` through 60. Python: `sigma_0`, `sigma_1`, `sigma_3` and `J_2` modulo 2^128, `sigma` with `k = 10^18` and `J` with `k = 123456789` modulo 998244353. |
| Six tables | Sizes 0, 1, 2, 3 and N against trial-factor values, divisor-multiple sieves and running sums. |
| `multiplicativeTable` | Salted hash of `(p, k, p^k)` compared with the product over trial factors. A completely multiplicative modint `i^K` table with random `K`. Sizes 0 and 1. |
| `dirichletConvolutionTable` | `O(n^2)` divisor sums for sizes 0, 1, 2, 7 and 300 / 3000 / 12000, random signed entries. |
| `dirichletInverseTable` | `a * a^(-1) = e` for wrapping `ulng` with `a[1] = 1` and for modint with random `a[1] != 0`. The inverse of all-ones equals `mobiusTable`. |
| `DivisorArray` | 11 fixed `n` (1, primes, 360, 720720, `600851475143`, `897612484786617600` with 103680 divisors, `2^64 - 1`) and 40 / 400 random `n`. Both constructors; divisor sets against `07` `divisors` and brute enumeration below 10^5. Every `index`. `get` and `operator[]`. `zeta` and `multipleZeta` against brute sums when `d(n) <= 3000`. Inverse round trips everywhere. `setMultiplicative` (phi then zeta gives the identity; Möbius against trial factors). Default object. |
| Preconditions | 10 checked-build probes: zero `n`, negative and `INT_MAX` table sizes, mismatched or empty convolution inputs, a too-small inverse input, `index(0)`, `index` of a non-divisor. |

## Commands and results

P052 run (2026-10-09), same conditions as [07-primality_factorization.md](07-primality_factorization.md).

```bash
python3 '96-Local Testing/05-Mathematics/08-multiplicative_functions_tester.py' --mode quick --seed 1           # PASS, 2 configurations, 10 probes
python3 '96-Local Testing/05-Mathematics/08-multiplicative_functions_tester.py' --mode full --seed 1            # PASS, 3 configurations: 350,193 C++ checks and 4,001 Python cases each
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/08-multiplicative_functions_tester.py' --mode full --seed 1 # PASS, 3 configurations
python3 '96-Local Testing/05-Mathematics/08-multiplicative_functions_tester.py' --mode stress --seed 1          # PASS, 3 configurations: 1,088,377 C++ checks and 20,001 Python cases each
```

The first full run failed under UBSan with signed overflow in `dirichletInverseTable` on random `lng` input. That is a test-domain error: integer Dirichlet inverses grow exponentially. The test now uses wrapping `ulng` and modint, and the contract states the overflow responsibility. The header and tester build with no warnings under `-Wall -Wextra -Wconversion`, including two translation units.

`@reviewer` (2026-10-09) found no defect. Its note that `DivisorArray::index` asserts once per prime of `n` was rejected: the check is `O(1)` per prime, does not change the `O(omega(n) + log(d))` bound, and is not inside an element loop.

## Benchmarks

None required. No Barrett, Montgomery or ISA-specific code is used directly; Montgomery runs only inside `07` `factorize`.

## Sources

[00-sources.md](00-sources.md), P052 sweep: maspypy `array_on_divisors`, hitonanode `sieve` (divisor-array multiplicative fill and multiple-direction transforms), OI Wiki number-theory basics (`omega`, `bigOmega`), OI Wiki sieve (linear-sieve recurrences). Written independently. Legacy: `OLD/5-Mathematics/06-phiandinverse.hpp` (`getPhi` becomes `phi`).

## Limits and handoffs

- The row's original `divisorCountFn` and `divisorSumFn` were replaced by `divisorCountTable` and `divisorSumTable`, because point values already exist as `07` `divisorCount` and `divisorSum`.
- Rejected candidates are in [00-notes.md](00-notes.md).

## History

- 2026-10-09: P052 initial implementation and verification.
