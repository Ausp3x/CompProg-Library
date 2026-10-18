# 30-modular_power_towers.hpp — evidence

`30-modular_power_towers.hpp` evaluates power towers a_0^a_1^...^a_(n-1) and iterated powers a^a^...^a^x0 modulo any 64-bit m, exactly capped towers, and the Euler and Carmichael modulus chains they reduce through. It depends on `01-mod_arithmetic.hpp` (`modPow64`, `modMul64`), `07-primality_factorization.hpp` (`factorize`) and `08-multiplicative_functions.hpp` (`phi`, `carmichaelLambda`). Every operation is deterministic; the chains cost one Pollard rho factorization per level and every tower evaluation is allocation-free.

## Contracts

Common domain: bases, counts, tops and moduli are any `ulng`; m >= 1. The convention 0^0 = 1 holds everywhere (Library Checker `tetration_mod`), the empty tower is 1, and a↑↑0 = 1. A tower is E_0 where E_i = a_i^E_(i+1) and E_n = 1 (list form) or the top x0 (iterated form).

### eulerChain, carmichaelChain

`eulerChain(m)` returns m, phi(m), phi(phi(m)), ..., 1 and `carmichaelChain(m)` the same with Carmichael's lambda. Both are strictly decreasing for m > 1 and end in 1; m = 1 gives {1}. phi(n) is even for n >= 3 and phi(phi(n)) <= n / 2, so the length L + 1 is O(log(m)) (at most 2 log2(m) + 2). lambda(n) divides phi(n) and lambda(d) divides lambda(n) for d | n, so by induction the k-th Carmichael value divides the k-th Euler value and the Carmichael chain is never longer. Cost: L factorizations, O(L m^(1/4) log(m)) expected.

### towerThreshold

`towerThreshold(m)` returns the largest exponent e_p in m = prod p^e_p (0 for m = 1). It is the least t with a^(t + e) = a^(t + e + lambda(m)) mod m for every a and e >= 0: for p not dividing a, a^lambda = 1 mod p^e_p; for p | a, a^t = 0 mod p^e_p once t >= e_p, and a = p shows that t = e_p - 1 fails. It is at most floor(log2(m)) <= 63.

### powerTowerMod, iteratedPowerMod, tetrationMod

`powerTowerMod(a, chain)` returns E_0 mod chain[0] for the list a (a `std::span<const ulng>`, so a vector or a slice binds without copying). `iteratedPowerMod(a, k, x0, chain)` returns a^a^...^a^x0 with k copies of a, for any 64-bit k, and `tetrationMod(a, b, chain)` is its named special case x0 = 1, a↑↑b. The chain is `eulerChain(m)` or `carmichaelChain(m)` computed once by the caller: any chain whose next element is a multiple of lambda of the previous one works, so one chain serves many towers modulo the same m (Codeforces 906D slices). The chain is asserted nonempty, strictly decreasing and ending in 1.

Algorithm. Let D = min(n, L). Levels i < D keep the pair (E_i mod c_i, min(E_i, 64)) with c_i = chain[i]; deeper levels matter only through min(E_D, 64), because c_D = 1 when D = L. Going down a level with base a, modulus c and next modulus c1:

- If min(E_(i+1), 64) < 64, the exponent is exact and E_i mod c = a^E_(i+1) mod c.
- Otherwise E_(i+1) >= 64 > 63 >= towerThreshold(c). With r = E_(i+1) mod c1, the replacement exponent is r itself when r >= 64, else r + j c1 with j = floor(63 / c1) + 1 the least making j c1 >= 64. It is congruent to E_(i+1) modulo lambda(c), which divides c1, and both exponents reach the threshold, so a^e = a^E_(i+1) mod c. The second case is computed as a^r a^(j c1): j c1 <= c1 + 63 never wraps (it is c1 itself when c1 >= 64), whereas r + c1 would wrap for c1 > 2^64 - 64 (phi(2^64 - 59) = 2^64 - 60).
- min(E_i, 64) = min(a^min(E_(i+1), 64), 64) with 0^0 = 1, which is exact because a >= 2 and E >= 64 give a^E >= 64.

The capped value at depth D comes from `capped` (below) with need 64. Cost O(D log(m)) for the residues plus the capped tail; the chain length bounds D, so a long list costs O(L log(m) + z), where z is the length of a zero run met while capping.

Edge cases follow from the recursion: m = 1 gives 0; a base 0 gives 0^E = [E = 0]; a base 1 stops the tower above it; a run of z zeros on top of a nonzero exponent alternates, so its value is the parity of z (iterated form: O(1)).

### powerTowerCapped

`powerTowerCapped(a, cap)` returns the exact min(E_0, cap) for any 64-bit cap (cap 0 gives 0). It propagates the needed precision upward: for a base a >= 2 and need N >= 2, min(a^E, N) = min(a^min(E, k), N) with k the least exponent with a^k >= N (found by saturating multiplication in O(log(N))), and k < N, so the need falls 2^64 - 1 -> 64 -> 6 -> 3 -> 2 -> 1 within five frames. A base 1 or a need of 1 stops at value 1; a zero run is resolved by its parity in one pass. Cost O(log(cap) + z). Comparing a tower with a bound B is `powerTowerCapped(a, B + 1) > B`. A capped tetration a↑↑b is the list of min(b, 6) copies of a for a >= 2 (2↑↑5 already exceeds 2^64), 1 for a = 1 and [b even] for a = 0.

### Planned-name mapping

| Planned | Implemented |
|---|---|
| `zeroPowerZeroConvention` (removed from the row, duplicates ledger) | the 0^0 = 1 contract above |
| `tetrationMod (Euler chain)` | `tetrationMod(a, b, chain)` with either chain |

Aliases: Library Checker `tetration_mod`; maspypy `tetration`; Nyaan `tetration`; ei1333 `mod_tetration`; suisen `tetration_mod`, `saturation_pow`, `saturation_tetration`; Codeforces 906D power tower queries; SPOJ POWTOW; Project Euler 188 (hyperexponentiation).

## Feature-to-test map

Entry: [`30-modular_power_towers_tester.py`](<../../96-Local Testing/05-Mathematics/30-modular_power_towers_tester.py>).

| Feature | Independent evidence |
|---|---|
| `eulerChain`, `carmichaelChain`, `towerThreshold` | Every m <= 400 (quick 120): each step against brute phi (counting units) and brute lambda (lcm of unit orders found by iteration), the threshold against the least t with a^t = a^(t + lambda) for every residue. The Python oracle rebuilds both chains and the largest exponent with its own Pollard rho for m up to 2^64 - 1. |
| `powerTowerMod` | Every list of length <= 3 over bases < 6 (quick 4) for m <= 90 (quick 40), then 4000 seeded lists of length up to 40 with zeros, ones, small and 64-bit bases for m <= 3000, against a cycle oracle that simulates the powers of each base mod m (preperiod and period) and recurses modulo the period, using no Euler or Carmichael theory; both chains. Exact short towers against 128-bit evaluation modulo any 64-bit m. The Python oracle reduces over the prime powers of m with Euler's theorem for coprime bases and exact valuations otherwise, for m up to 2^64 - 1. |
| `iteratedPowerMod`, `tetrationMod` | The cycle oracle on constant lists and on constant lists with another top; counts up to 2^64 - 1 against the same parity at depth 150 and across both chains; the Python oracle with cycle detection on v -> min(a^v, cap) for every count up to 2^64 - 1. |
| `powerTowerCapped` | Exact values of short towers by a top-down uniform-cap evaluation (an independent algorithm) for caps 0, 1, 2, 64, 1000, 2^62, 2^64 - 1 and random caps. |
| Edges | m = 1, the empty tower, 0^0 = 1, cap 0, 0↑↑b parity, a 100000-long zero run; the largest 64-bit prime m = 2^64 - 59, where phi(m) = lambda(m) > 2^64 - 64 (review regression), against Fermat: a^((m - 2)^2) = a^(m^1) = a mod m. |
| Oracle validation | The Python oracle is checked against exact big-integer towers, brute powers and its own iterated form before use. |
| Preconditions | Six checked-build death probes: m = 0 for both chains and the threshold, a chain not ending in 1, an empty chain, a chain that is not decreasing. A separate `-DNDEBUG` probe (2026-10-19, `-O2` on g++ and `-O0` on g++-14) confirmed that the violating calls of the package terminate, including an empty chain with a 2^64 - 1 count and m = 0. |

## Commands and results

Run 2026-10-19 on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. The shared runner builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and (full, stress) ASan/UBSan configurations. With `-Wall -Wextra -Wconversion`, the header compiles without warnings on both compilers.

| Command | Result |
|---|---|
| `30-modular_power_towers_tester.py --mode full --seed 1` | PASS, 89,235 C++ checks and 800 Python rows per configuration, 3 configurations, 6 assertion probes, `MEMORY peak=410MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=436MB` |
| `30-modular_power_towers_tester.py --mode stress --seed 7` | PASS, 280,602 C++ checks and 4000 Python rows per configuration, 6 assertion probes, `MEMORY peak=524MB` |
| `02-integration.py` | PASS, 165 standalone and aggregate headers, two translation units, workspace |
| `03-consistency.py` | PASS, no errors |

## Sources

Fetched 2026-10-19 by `@researcher` ([00-sources.md](00-sources.md), P054 sweep). The algorithms were implemented from the stated identities, and no external code was copied.

- Library Checker `tetration_mod` statement and verifier: 0 <= A, B <= 10^9, 1 <= M <= 10^9, 0^0 = 1.
- OI Wiki 欧拉定理 and cp-algorithms Euler's totient: the extended Euler theorem a^e = a^(e mod phi(m) + phi(m)) for e >= log2(m), and the warning that lambda can lie below the threshold (lambda(8) = 2 < 3), handled here by exponents of at least 64.
- Nyaan, maspypy, ei1333 and suisen tetration code: edge cases (m = 1, a = 0, a = 1, b = 0, b = 1) and the capped-pow idea; maspypy asserts positive bases, a narrowing not copied.

## Limits and handoffs

- Multiplicative orders are row 11; reducing exponents by orders or by suisen's exact preperiods gives the same values by another route and is not a separate operation.
- Scope decisions (infinite towers, higher hyperoperations, exact tower comparison) are in [00-notes.md](00-notes.md), P054 decisions.
- No `97-Legacy` excerpt existed for this row.

## History

- 2026-10-19: P054 initial implementation; `@reviewer` found 1 bug in this header (exponent overflow at m = 2^64 - 59) and a cost statement for `powerTowerCapped`, both fixed with a regression; full suite passed on g++ and g++-14, stress seed 7 passed.
- 2026-10-19: P054 follow-up: shared Pollard rho moved into the folder runner, limits section reduced to handoffs; full and stress passed on g++, full on g++-14.
