# 07-primality_factorization.hpp — evidence

`07-primality_factorization.hpp` covers deterministic primality and factorization for every unsigned 64-bit integer: trial and table tests, deterministic Miller–Rabin, Brent's Pollard rho, factorizations, divisor lists and the derived point functions, perfect-prime-power detection, coprime bases of lists and the highly composite search. Full-prefix sieves belong to `04-sieve_algorithms.hpp`, multiplicative functions other than divisor count and sum to `08-multiplicative_functions.hpp`.

## Contracts

Inputs are `ulng`, so every function works on all of `[0, 2^64)` unless a precondition is stated. Primes are returned as `ulng`, exponents as `int`, and factorizations as ascending `(prime, exponent)` pairs. There is no global state. Randomness is local and seeded, so every result is reproducible.

The fast functions multiply through Core `Montgomery64` (any odd modulus below 2^64) using its unchecked `redc`, so there are no assertions in hot loops. `primality_detail` holds `SMALL` (the 18 primes below 67), `splitmix` (splitmix64 step), `addMod` (overflow-safe `x + y mod n` for `x, y < n`), `montMul` and `plainMul` (product functors), `kthRoot` (floor k-th root: double estimate corrected by exact overflow-checked powers), and four algorithm templates parametrized by the product functor: `strongProbablePrime` (the deterministic witness loop), `isPrimeWith` (the small-prime filter), `rho` (Brent attempts under a budget) and `factorizeWith` (the factorization driver). Each algorithm is therefore written once and runs on Montgomery forms for the fast functions and on plain residues for the Compact twins.

### isPrimeTrial

Deterministic trial division by 2, 3 and `6k ± 1` up to `floor(sqrt(n))` in `O(sqrt(n))`, false for 0 and 1. The loop condition `i <= n / i` cannot overflow.

### millerRabin64, isPrime

`millerRabin64(n)` is exact for every `ulng`: 0 and 1 are not prime, 2 and 3 are, even `n > 2` are composite. For odd `n` it runs strong-probable-prime tests in Montgomery form, to bases {2, 7, 61} below 2^32 (deterministic below 4,759,123,141) and to the seven bases {2, 325, 9375, 28178, 450775, 9780504, 1795265022} above. A base that is a multiple of `n` is skipped, which is the convention under which the 7-base set was verified. `isPrime(n)` first divides by the 18 primes below 67, decides `n < 67^2` directly, and otherwise calls `millerRabin64`. Both take `O(log(n))` modular multiplications.

Correctness: a strong pseudoprime to all listed bases would contradict the published exhaustive verifications (sources). The tests check the base-skip convention directly on every divisor of every base.

### Compact twins: millerRabin64Compact, isPrimeCompact, pollardRhoBrentCompact, factorizeCompact

These follow the `03-cpp.md` Compact rule. Each has the same contract, domain, seed sequence and results as its fast counterpart, but multiplies with plain `__int128 %` (`plainMul`) and uses no Core reduction type. A contest copy therefore needs only `01-template.hpp` and these detail helpers: `SMALL`, `splitmix`, `addMod`, `plainMul`, `strongProbablePrime`, `isPrimeWith`, `rho` and `factorizeWith`. `rho` maps the drawn constant `c` and the start value through `form` (`montForm` gives `xR mod n`, the plain form is the identity). Each fast iterate is therefore the Montgomery image of the plain iterate: `redc((zR)^2) + cR = (z^2 + c)R`. Every difference and product also maps by the unit `R`, so each gcd is the same and `pollardRhoBrentCompact(n, seed, budget)` returns exactly the divisor of `pollardRhoBrent(n, seed, budget)`. The tester checks this identity on every `n` through the factor bound. The bounds are the same; the constant factor is in the benchmark table. Functions built on the fast versions (`primeFactors`, `divisors`, `divisorCount`, …) have no twins, as the rule specifies.

### isPrimeSpf

Takes a caller-owned `LinearSieve`. It returns `s.spf[n] == n` when `n <= s.n` (O(1)) and falls back to `isPrime(n)` above the table. The sieve must not be modified between `reset` and the call.

### pollardRhoBrent

`pollardRhoBrent(n, seed = 0, budget = 64)` returns a proper divisor `1 < d < n` of a composite `n`. It returns 0 when `n < 4`, when `n` is prime (checked first, so no cycle search runs on primes), or when `budget` attempts all fail; `budget = 0` always gives 0. Even `n` gives 2. Each attempt draws `c` in `[1, n)` and a start in `[0, n)` from splitmix64 seeded by `seed`, so equal arguments give equal results. Failure is Las Vegas with respect to the factor: a returned value always divides `n`. Expected time is `O(n^(1/4))` multiplications per attempt (heuristic random-map model).

Brent's method doubles the window `r`, accumulates `|x - y|` products in batches of 128, and takes one gcd per batch. When a batch reaches `gcd = n`, it replays the batch one step at a time from the saved point. The product lives in Montgomery form. The Montgomery residue of `|x - y|` is the difference of residues, and `R` is a unit modulo odd `n`, so `gcd(q, n)` is the gcd of the plain product.

### factorize, primeFactors, divisorCount, divisorSum, isSquarefree, radical

`factorize(n)` asserts `n >= 1` and returns ascending prime powers, empty for 1. It strips the 18 small primes, then splits the remaining cofactors with rho (fixed internal seed, unlimited retries) until every part passes `isPrime`. Expected `O(n^(1/4) * log(n))`, `O(log(n))` space. `primeFactors(n)` returns the ascending primes with multiplicity. `divisorCount(n)` is exact `d(n)` (at most 184320 below 2^64). `divisorSum(n)` is exact `sigma(n)` as `ulll` (`sigma(n) < 2^70`). `isSquarefree(n)` and `radical(n)` read the exponents and primes. All of them assert `n >= 1`. Under `-DNDEBUG`, `n = 0` returns the result for 1 instead of looping.

### factorizeTrial

Deterministic, with the same output as `factorize`, in `O(sqrt(n))`. It is the replacement for legacy `getPrimeFacSlow`, for callers who want no randomness or who have small inputs.

### divisors

`divisors(f)` returns all positive divisors of the factorization `f` in ascending order. Each existing divisor is multiplied by each power of the next prime, which yields every divisor exactly once by unique factorization. Then the list is sorted: `O(out * log(out))`. `divisors(n)` factorizes first; `n >= 1`.

### isPrimePower

Returns `p` when `n = p^k` with prime `p` and `k >= 1`, and 0 otherwise (0 and 1 give 0). For each prime exponent `k <= 61` it repeatedly replaces `n` by its exact k-th root, then tests the remaining base with `isPrime`. Correctness: if `n = b^e` with `e` maximal, removing prime-degree roots reaches `b` (every prime factor of `e` is at most 63), and `b` is prime iff `n` is a prime power. `O(log(n)^2)` word operations.

### coprimeBase

Asserts every `a_i >= 1` and ignores ones. It returns the natural (coarsest) coprime base of the list in ascending order: pairwise coprime elements above 1 such that every `a_i` is a product of their powers. Empty input gives an empty base. Algorithm: pop `x`; if `x` is coprime to every kept element, keep it; otherwise remove the first kept `b` with `g = gcd(x, b) > 1` and push `g`, `b / g`, `x / g`. Each split replaces `x * b` by `x * b / g`, so there are at most `sum(log2(a_i))` splits, and each pop scans at most `64k` kept elements: `O(k^2 * w^3)` with `w = 64`.

Coarsest: every pushed value is a product of powers of the natural base `B*` (gcds and quotients preserve that), and every spanning coprime base refines `B*` (Bernstein). A result element is therefore a product of `B*` powers that share no prime with other elements, and each `B*` element is a power of one result element; together these force equality. The tester also compares against a brute coarsest base (primes grouped by primitive exponent vector) on random lists.

### maxDivisorCount

Asserts `N >= 1` and returns `(n, d(n))` for the smallest `n <= N` with the maximum divisor count, for example `(897612484786617600, 103680)` at 10^18 and `(18401055938125660800, 184320)` at `2^64 - 1`. The depth-first search runs over exponent-nonincreasing products of consecutive primes from 2: sorting any `n`'s exponents onto the smallest primes gives some `n' <= n` with the same count, so the smallest maximizer has that form. 47,616 nodes at `N = 2^64 - 1`.

## Feature-to-test map

Entry: [`07-primality_factorization_tester.py`](<../../96-Local Testing/05-Mathematics/07-primality_factorization_tester.py>) with its C++ helper. The oracles are a separate byte sieve, plain trial division and divisor enumeration in the tester, and a Python 13-base Miller–Rabin (deterministic below 3.3 * 10^24) with exact big-integer reconstruction.

| Feature | Oracle and edge classes |
|---|---|
| `isPrime`, `millerRabin64`, `isPrimeSpf` | Every `n` through 10^5 / 2 * 10^6 / 10^7 (quick/full/stress) against the sieve, with the table covering half the range (fallback above). Every divisor of every base. 14 named strong pseudoprimes and Carmichael numbers. Chernick Carmichael numbers (30 / 400). Products and squares of the six largest 32-bit primes. The three largest 64-bit primes. Python: random 64-bit and random-width values, edges `0..4`, `2^64 - 1`, `2^64 - 59`, `2^63 ± 1`, `2^62 + 1`, `2^32` neighbours, 4759123141. |
| `isPrimeTrial` | Sieve through the factor bound; Python on random 36-bit values. |
| Compact twins | Every primality, rho and factorization check above, including the exhaustive ranges, base divisors, pseudoprimes, Carmichael numbers, semiprimes, large primes, prime powers, budget and seed reproducibility, also runs on `millerRabin64Compact`, `isPrimeCompact`, `pollardRhoBrentCompact` and `factorizeCompact` against the same oracles. The Python oracle independently verifies the fast factorization and requires the Compact one to equal it. `factorizeCompact(0)` is a probe. |
| `pollardRhoBrent` | Every `n` through the factor bound: a proper divisor for composites, 0 for primes and `n < 4`. Carmichael numbers and 32-bit semiprimes. `budget = 0`. Same seed gives the same result. Python checks divisibility on random inputs. |
| `factorize`, `factorizeTrial`, `primeFactors` | Trial factorization of every `n` through 2 * 10^4 / 2 * 10^5 / 10^6. Pseudoprime and Carmichael factorizations. All prime powers of seven primes, including `2^63`. Python reconstruction with an independent primality check on random, semiprime, prime-power, smooth and boundary values. |
| `divisors` (both overloads), `divisorCount`, `divisorSum` | Square-root enumeration and a divisor-count sieve through the factor bound. Python divisor generation from the verified factorization (lists only when `d(n) <= 4096`). Large primes. |
| `isPrimePower`, `isSquarefree`, `radical` | Trial factorization through the factor bound; prime powers and their successors; 32-bit prime squares; Python closed forms. |
| `coprimeBase` | 200 / 3000 / 20000 random lists of 0–6 values, each a product of up to 7 factors (2..41 including composites, or 32-bit primes): sorted, above 1, pairwise coprime, each element divides an input, inputs span. Equality with the brute coarsest base on two thirds of the lists. Empty input, ones, duplicates, `{6, 10}`, `{12, 18}`, `{4, 8}`, `{10^18, 10^18}`. |
| `maxDivisorCount` | Running maximum of the divisor-count sieve, for every `N` through 2 * 10^4 and every 101st beyond; known answers at 1, 10^18 and `2^64 - 1`. |
| Preconditions | 11 checked-build probes: zero for each `n >= 1` function (including `factorizeCompact`), a zero list entry, `N = 0`. |

## Commands and results

P052 Compact rerun (2026-10-09). GCC 16.2.1 and GCC 14.4.1, GNU++20, Python 3.14, Linux x86-64 (i9-11900H). Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1`. Every run is self-capped by `_00_memory_cap.ensure()` (4096 MB main cap; full and stress first pre-flight quick under 2048 MB).

```bash
python3 '96-Local Testing/05-Mathematics/07-primality_factorization_tester.py' --mode quick --seed 1           # PASS, 2 configurations, 11 probes; peak 437 MB
python3 '96-Local Testing/05-Mathematics/07-primality_factorization_tester.py' --mode full --seed 1            # PASS, 3 configurations: 6,079,117 C++ checks and 36,027 Python cases each; pre-flight 436 MB, peak 729 MB
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/07-primality_factorization_tester.py' --mode full --seed 1 # PASS, 3 configurations: same counts; pre-flight 479 MB, peak 710 MB
python3 '96-Local Testing/05-Mathematics/07-primality_factorization_tester.py' --mode stress --seed 1          # PASS, 3 configurations: 30,402,516 C++ checks and 180,027 Python cases each; pre-flight 436 MB, peak 1044 MB
python3 '96-Local Testing/03-consistency.py'                                                                    # no errors, compact_pending empty
```

Warnings: the header, the tester and a two-translation-unit build compile with no warnings under `-Wall -Wextra -Wconversion`.

Rerun review (`@reviewer`, 2026-10-09) confirmed one defect, now fixed. `pollardRhoBrentCompact` returned different divisors from `pollardRhoBrent` (for example 3 versus 7 for `n = 21`, seed 0), because the raw constants were used as Montgomery forms; `rho` now maps them through `form`, and the identity is tested. First review (`@reviewer`, 2026-10-09) found no correctness defect in this header. Its notes were acted on: the `kthRoot` bound was corrected to `O(k)`, the `maxDivisorCount` symbol was defined, and coprimeBase lists now use up to 7 factors per value. Independent probes, all passing: 20,000 high-exponent coprime-base lists against a brute coarsest base; `isPrime` on every `n` in `2^32 ± 2 * 10^5`; 600,000 perfect powers and their neighbours; rho on every odd composite below 3000 with 300 seeds each, with no budget failure; 4,500 semiprimes, three-prime products and prime squares.

## Benchmarks

Driver: [`07-primality_factorization_benchmark.py`](<../../96-Local Testing/05-Mathematics/07-primality_factorization_benchmark.py>) with its [C++ workload](<../../96-Local Testing/05-Mathematics/07-primality_factorization_benchmark.cpp>). Raw output goes to `07-primality_factorization_benchmark.json`. It compares the Montgomery functions with their Compact twins: the same algorithms, bases, batching and seeds, multiplying with plain `__int128 %`. Conditions: i9-11900H on an otherwise idle machine, GCC 16.2.1, `-std=gnu++20 -O2` (no `-march`), seed 20261009, one warmup plus five repetitions, medians. Outputs are compared on every repetition. Peak memory is 334 MB.

| Workload | Montgomery (ms) | Compact (ms) | Saving |
|---|---|---|---|
| `isPrime`, 10^6 random odd 64-bit | 206.7 | 207.7 | 0.5% |
| `isPrime`, 10^5 random 64-bit primes | 236.7 | 237.2 | 0.2% |
| `isPrime`, 10^6 random 32-bit | 43.7 | 58.0 | 24.7% |
| `factorize`, 300 semiprimes of 31- and 32-bit primes | 113.8 | 177.1 | 35.8% |
| `factorize`, 10^4 random 64-bit | 161.0 | 245.6 | 34.4% |

The Montgomery rule from `01-principles.md` is met. Factorization, the representative workload, is 34–36% faster, and no workload regresses. 64-bit primality is level on this CPU, which has a fast 64-bit divider. The Compact twins cost about 1.5x on factorization; that is the price of a copy with no Core reduction dependency.

## Sources

Fetched on 2026-10-09 by the completeness sweep, written independently with no code copied. See [00-sources.md](00-sources.md), P052 sweep: cp-algorithms and OI Wiki on Pollard rho, Nyaan and hitonanode fast factorization (both limited to 2^62), maspypy `divisors`, and OI Wiki on highly composite numbers. The Miller–Rabin base sets and Bernstein's coprime-base paper are cited from memory and were not fetched. Legacy: `OLD/5-Mathematics/05-primesandfactors.hpp`.

## Limits and handoffs

- Legacy accounting: `isPrimeMR` becomes `millerRabin64`/`isPrime`; `getPrimeFacSlow` becomes `factorizeTrial`/`primeFactors`; `getOneFacBPR(n, x0, c)` becomes `pollardRhoBrent` (seeded instead of explicit `x0`/`c`); `getPrimeFacFast` becomes `factorize`/`primeFactors`; `getAllFac(n, opt)` becomes `divisors`; `getNumDiv` and `getSumDiv` become `divisorCount` and `divisorSum` (exact `ulll`). The legacy global `Random rnd` is gone.
- Precondition violations under `-DNDEBUG`: `n = 0` was originally an infinite loop with unbounded `push_back` in `factorize`, `factorizeTrial` and `coprimeBase`. During this package an oracle row sent 0 to the optimized build, and the resulting memory growth got the user's editor killed by the kernel. The loops now stop on 0, and every later test ran under a memory cap.
- Research decisions (adopted and rejected) are in [00-notes.md](00-notes.md).

## History

- 2026-10-09: P052 initial implementation and verification (3,678,282 full checks before the twins, 2 * 10^4..10^7 exhaustive ranges, integration of 112 headers passed).
- 2026-10-09: P052 rerun for the Compact rule (P231): algorithm templates with product functors, four Compact twins, capped runs.
