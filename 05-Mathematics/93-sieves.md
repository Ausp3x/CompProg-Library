# Ordinary sieves: MA02 verification

`04-sieve_algorithms.hpp` covers full-prefix sieves: Eratosthenes, the linear sieve, smallest and largest prime-factor tables, positive factor enumeration, and four multiplicative-function tables. Interval sieving belongs to `06-segmentedsieve.hpp`, and general multiplicative transforms to `08-multiplicative_functions.hpp`.

The P012 re-audit (2026-10-07) made these changes:

- Moved the in-code contracts into this document.
- Finding 9: the `getAllFac` complexity comment now reads `Q: O(log(a) + out), M: O(out); 1 <= a <= n`, which replaces the undefined `d(a)`.
- Changed `mu` to `vector<int8_t>`, which is the same type as `signed char`.
- Fixed three closing braces.

## Contracts

Both structs keep the inclusive table convention `[0, n]`: the argument is a maximum value, not a table length. Each asserts `0 <= n < INT_MAX`, so that `n + 1` and loop sentinels stay representable as `int`. Prime lists are ascending.

The public vectors and `n` are query data, not inputs. Callers must not modify them and then call methods that rely on the sieve invariant. Copies own independent vectors, and a moved-from object can be restored with `reset`. There is no global cache and no dynamic-modulus state.

`reset(n)` discards old results but may keep vector capacity. Allocated storage is therefore `O(peak_n)`, where `peak_n` is the largest `n` the object has used (just `n` for a fresh object). Construct a fresh object when releasing that capacity matters.

### SieveOfErath

`SieveOfErath(n)` and `reset(n)` build `is_prime[0..n]` and `prms` in `O(n * log(log(3 + n)))` time. Zero and one are not prime. Ignoring capacity rounding, the state is `n + 1` bytes of flags plus `4 * pi(n)` bytes of primes.

### LinearSieve

`LinearSieve(n)` and `reset(n)` build `spf[0..n]` and `prms` in `O(n)` time. `spf[0] = spf[1] = -1` where those indices exist, and `spf[p] = p` for a prime `p`. The optional tables `lpf`, `num_div`, `sum_div`, `phi` and `mu` stay empty until their `get...()` call, and `reset` clears them. Each builder takes `O(n)` time, may be called repeatedly in any order, and recomputes only its own table.

- `getLpf()` builds the largest prime factor, with `lpf[0] = lpf[1] = -1`.
- `getNumDiv()` builds `num_div[a] = tau(a)`, the number of divisors, using `O(n)` temporary integers for the smallest-prime exponent.
- `getSumDiv()` builds `sum_div[a] = sigma(a)` as `lng`, using `O(n)` temporary `lng` values for `1 + p + ... + p^e`.
- `getPhi()` and `getMu()` build Euler's totient and the Möbius function. `mu` is `int8_t`.
- In all four arithmetic tables, entry 0 is a convenience 0 (it says nothing about divisors of zero) and entry 1 is 1.
- `getPrimeFac(a) const` asserts `1 <= a <= n` and returns ascending `(prime, exponent)` pairs, empty for 1, in `O(log(a))`.
- `getAllFac(a) const` has the same domain and returns every positive divisor once, in unspecified order, with `{1}` for 1. It takes `O(log(a) + out)` time and `O(out)` returned storage.

Fresh linear state uses `4 * (n + 1)` bytes of SPF plus the prime list. LPF, divisor counts and totients each add `4 * (n + 1)` bytes, divisor sums `8 * (n + 1)`, and Möbius `n + 1`. Building every table therefore takes `25 * (n + 1) + 4 * pi(n)` logical bytes, plus up to `8 * (n + 1)` temporary bytes during `getSumDiv`.

## Correctness and arithmetic bounds

**Eratosthenes.** Every composite has a prime factor at most its square root. Even composites are cleared first, then odd multiples of each prime `p` are marked from `p * p`. That marks every composite and never a prime, since the smaller multiples already have a smaller prime factor.

**Linear sieve.** Every composite `x` has a unique decomposition `x = p * i` with `p = spf[x] <= spf[i]`. Iterating primes only up to `spf[i]` builds each decomposition exactly once, which sets the correct SPF and gives linear time. Removing the SPF repeatedly yields the factorization.

**LPF and divisors.** `lpf[i] = max(spf[i], lpf[i / spf[i]])`, with the sentinel at 1 acting as the empty maximum. `getAllFac` multiplies each existing divisor by each power of a new prime, so by unique factorization it produces every divisor exactly once.

**Multiplicative tables.** Write `i = p * j` with `p = spf[i]`.

- If `p` divides `j`, only that prime's exponent grows.
  - The divisor count replaces the factor `e + 1` by `e + 2`.
  - The divisor sum replaces `1 + ... + p^e` by `1 + ... + p^(e + 1)`.
  - The totient multiplies by `p`.
  - Möbius becomes 0.
- Otherwise a new coprime prime appears.
  - The divisor count doubles.
  - The divisor sum multiplies by `1 + p`.
  - The totient multiplies by `p - 1`.
  - Möbius changes sign.

Induction from entry 1 proves each table, and the recurrences use exact divisions.

**Overflow.** Index products are checked in `lng` before the `int` product is formed, and Eratosthenes counters are `lng`. Divisors, counts, totients and prime powers are at most `n`. A divisor sum is below `n * (n + 1) / 2 < 2^61`, and the geometric prime-power sum is at most `2 * n`.

## Sources and legacy accounting

Inspected on 2026-09-27; written independently with no code copied.

- [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html): marking from prime squares, odd-only work, and memory tradeoffs.
- [cp-algorithms, Linear Sieve](https://cp-algorithms.com/algebra/prime-sieve-linear.html): the unique `spf[x] * i` construction and linear work.
- [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/): SPF-based recurrences for totient, Möbius, divisor count and divisor sum.
- [KACTL, `Eratosthenes.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/Eratosthenes.h), CC0: independent comparison of odd marking and prime-list construction.

`OLD/5-Mathematics/04-sieves.hpp` is unchanged, and every ordinary-sieve legacy operation is kept.

The 2026-10-07 completeness sweep left out four candidates:

- A prime-counting table is `upper_bound` on `prms`.
- Factoring values above `n` is owned by `07` `factorize`.
- The completely multiplicative `i^K` table is handed to `08` `multiplicativeTable`.
- An lcm of a list is caller-side work on top of `getPrimeFac`.

The reasons are recorded in [80-notes.md](80-notes.md).

## Feature-to-test map

Entry: [`04-sieve_algorithms_tester.py`](<../96-Local Testing/05-Mathematics/04-sieve_algorithms_tester.py>). Its exact integer oracles do not share the SPF recurrence.

| Feature | Oracle and edge classes |
|---|---|
| Eratosthenes and linear flags, SPF, LPF, sorted lists | Trial division for every integer through 50,000 and every prefix through 128. The known prime count at one million. Random larger trial factorizations. Zero, one, prime endpoints and prime squares. |
| `getPrimeFac`, `getAllFac` | Trial factorization, and a separate square-root divisor enumeration sorted to check set equality and duplicates. Covers prime powers and squarefree products. |
| `getNumDiv`, `getSumDiv` | An independent divisor-multiple enumeration. |
| `getPhi`, `getMu` | Inversion of `sum(phi(d)) = n` and `sum(mu(d)) = [n = 1]` over divisors. Totients also by gcd counting through 300, Möbius also by trial-factor exponents. |
| State and lifetime | Randomized builder order, repeated calls, lazy tables, copy and copy assignment, move construction, reset after a move, grow and shrink resets. |
| Preconditions | 14 checked-build probes: negative and `INT_MAX` constructor and reset arguments, and out-of-range factor queries. |

Current commands and results: [96-p012.md](96-p012.md). Historical: on 2026-09-27, full mode with seed `20260927` passed 545,142 checks per configuration. Dense-sieve timings are in the [foundations benchmark](<../96-Local Testing/05-Mathematics/90-foundations_benchmark.py>) and its `.json` record.
