# 11-discrete_log_root.hpp — evidence

`11-discrete_log_root.hpp` covers multiplicative structure modulo `m`: discrete logarithms (extended baby-step giant-step with a multiplier, and a generic form over monoid actions), multiplicative orders, primitive roots, the Kronecker and Legendre symbols, and square, k-th and unit roots modulo a prime.

## Contracts

Moduli are `lng` values of at least 1 (asserted; prime moduli at least 2). Residues are any `lng`, including `LLONG_MIN`, and are normalized with `01` `modNorm`. Products are formed in `__int128`, so every modulus below `2^63` is safe; there is no state. Primality of `p` in the prime-modulus functions is a precondition that is not asserted, because a Miller–Rabin check would cost more than `legendreSymbol` itself. A composite `p` still terminates quickly, with a meaningless result:

- The nonresidue scans of `rootPrimePower` and `unitRootModPrime` stop at a candidate sharing a factor with `p`, whose power is never 1: after at most `spf(p)` candidates, below `p^(1/3)` for the Carmichael-like moduli where every unit passes.
- `sqrtModPrimeCipolla` checks `isPrime(p)` once, only after 64 fruitless candidates, and then returns -1. Without that check a perfect square `p` would scan about `p` candidates.
- Each BSGS is bounded by its table.

Violated `m >= 1` and `0 <= lb <= ub` are clamped after the assertion (`m = max(m, 1)`, `lb = max(lb, 0)`, `lb >= ub` gives -1). Ad hoc `-DNDEBUG` probes confirmed that each case returns in about 0.1 s: `discreteLogMonoid` with `lb > ub` and `lb < 0`, `discreteLogEx` with `m = -59, -5, 0`, and `sqrtModPrimeCipolla` with `p = 10007^2`, `(1e9 + 7)^2` and 41041. The header performs no Barrett or Montgomery arithmetic itself, so it needs no Compact twins. It calls `07` `factorize`, `isPrime` and `isPrimePower` and `08` `phi` and `carmichaelLambda`, which are built on Core `Montgomery64`.

`discrete_log_detail` holds four helpers:

- `mul`: `x * y mod m`, with no assertion in hot loops.
- `generates(g, m, t, f)`: true when `g^(t / q) != 1 (mod m)` for every prime `q` listed in `f`.
- `bsgs(a, b, k, m, t)`: the least `x >= 0` with `k * a^x = b (mod m)` for a unit `a` and `t + 1 >= ord(a)`, or -1. It uses `n = isqrt(t) + 1` baby steps (the loop test `n <= t / n` cannot overflow) `b * a^j` in an open-addressing table, with Fibonacci hashing, load at most 1/2, the key `~0` as the empty sentinel, and `uint` values (`n < 2^32` because `t < 2^63`). It then takes giant steps `k * a^(n i)` for `i = 1..n`. The table keeps the largest `j` per key, so the first hit `i * n - j` is the least solution in `[1, n^2]`. A smaller solution in an earlier block would have hit at a smaller `i`; one in the same block would have a larger `j`. `x = 0` is tested first, and `n^2 > t` covers a whole period.
- `rootPrimePower`: Adleman–Manders–Miller, described under `kthRootModPrime`.

### multiplicativeOrder

`multiplicativeOrder(a, m)` returns the least `t >= 1` with `a^t = 1 (mod m)`, or -1 when `gcd(a, m) != 1`; `m = 1` gives 1. Start from `t = lambda(m)` (`carmichaelLambda`), and for each prime `q` of `factorize(t)` divide `t` by `q` while `a^(t / q) = 1`. The result is the order: `ord(a)` divides `lambda(m)`, and every removable prime factor is removed. Cost: two factorizations, expected `O(m^(1/4) * log(m))`, plus `O(log(m))` exponentiations.

### discreteLogEx

`discreteLogEx(a, b, m, k = 1)` returns the least `x >= 0` with `k * a^x = b (mod m)`, with `0^0 = 1`, or -1. The legacy `modLog` multiplier is `k`.

Reduction: while `x = add` is not a solution (`k != b`) and `g = gcd(a, m) > 1`, there is no solution when `g` does not divide `b`. Otherwise every later solution satisfies `k (a / g) a^(x - 1) = b / g (mod m / g)`, so set `m /= g`, `b /= g`, `k = k * (a / g) mod m` and increment `add`; at most `log2(m)` steps. When `a` becomes a unit, `bsgs` searches with `t = multiplicativeOrder(a, m) - 1`, because solutions repeat with period `ord(a)`. A non-unit `k` or `b` is fine, since `a` is invertible. `m = 1`, or a reduction down to 1, gives `add`.

Cost: `O(sqrt(t) + m^(1/4) * log(m))` expected time and `O(sqrt(t))` memory, `t < m`. The least positive solution (KACTL, PyRival) is `discreteLogEx(a, b, m, k * a mod m) + 1` when that is not -1.

### discreteLogMonoid

`discreteLogMonoid(x, s, t, lb, ub, mul, act, hash)` returns the least `n` in `[lb, ub)` with `x^n · s = t`, or -1. It asserts `0 <= lb <= ub` and returns -1 for `lb == ub`. The caller supplies:

- `x` of a copyable type `X`, with `mul(X, X) -> X` associative on the powers of `x`;
- `act(X, S) -> S` with `act(mul(y, z), s) = act(y, act(z, s))`;
- `hash(S)` returning any integer;
- `S == S`, used for exact table membership, so hash collisions only cost probes.

`x` need not be invertible (functional graphs, matrices). The start state `x^lb · s` uses `O(log(lb))` products. With `L = ub - lb` and block size `K = ceil(sqrt(L))` (the code's `len` and `n`), the baby states `x^j · t` for `j = 1..K` are stored. Storing stops early once `t`'s orbit repeats, so memory is `O(sqrt(L))` words of open-addressing table plus `O(min(sqrt(L), orbit of t))` states. Giant steps test whether `x^((k + 1) K) · s_0` is a baby state.

A block holding a solution `n` always hits, with `j = (k + 1) K - n`. A non-injective action can also hit falsely. Suppose a solution `n*` exists after a false hit at block `k`. Then the state at index `(k + 1) K` lies on the cycle of `s_0`'s orbit, and every later hit is congruent modulo the cycle length to a true solution inside its own block. So the second fruitless block proves that there is no solution. This is the hitonanode and maspypy rule. Cost: `O(sqrt(L))` actions and hashes, at most three block scans of `K` actions, and `O(log(ub))` products. A huge `L` is honoured literally: choose `ub` no larger than needed.

### primitiveRootExists, isPrimitiveRoot, primitiveRoot

- `primitiveRootExists(m)` is true exactly for `m` = 1, 2, 4, `p^k` and `2 p^k` with `p` an odd prime (via `isPrimePower`, `O(log(m)^2)`).
- `isPrimitiveRoot(g, m)` is true when the unit group is cyclic, `gcd(g, m) = 1` and `g^(phi(m) / q) != 1` for every prime `q | phi(m)`. For `m = 1`, every `g` (residue 0) generates the trivial group.
- `primitiveRoot(m)` returns the smallest primitive root in `[0, m)`, or -1 when none exists: 0 for `m = 1`, 1 for 2, 3 for 4, 3 for 998244353, 5 for `1e9 + 7`, 37 for `2^61 - 1`.
  - It factorizes `phi(m)` once and tests `g = 1, 2, ...`. The scan terminates because a root below `m` exists whenever the existence test passes.
  - Cost: `O(m^(1/4) * log(m) + r * log(m)^2)` expected, with `r` the candidates tried. The least primitive root of a prime is small in practice; hitonanode reports at most 417 below `1e16`.

### kroneckerSymbol, legendreSymbol

`kroneckerSymbol(a, n)` is the Kronecker symbol for all `lng` `a` and `n`, returned as -1, 0 or 1:

- `(a / 0) = [a = ±1]`;
- `(a / -1)` is -1 for `a < 0` and 1 otherwise;
- `(a / 2)` is 0 for even `a`, 1 for `a = ±1 (mod 8)` and -1 for `a = ±3 (mod 8)`;
- it is multiplicative in `n`.

The algorithm is Cohen's 1.4.10: remove the 2-part and the sign of `n` (an arithmetic shift handles `n = LLONG_MIN`), reduce `a` modulo the odd part, then run the binary Jacobi loop with quadratic reciprocity on unsigned values. Cost: `O(log(|a| + |n| + 2))` divisions.

`legendreSymbol(a, p)` is Euler's criterion `a^((p - 1) / 2) mod p` mapped to -1, 0 or 1 for an odd prime `p` (asserted `p >= 3` and odd), in `O(log(p))` products. It is kept beside `kroneckerSymbol` by decision (Euler's criterion, a different algorithm). The narrow `jacobiSymbol` is `kroneckerSymbol` (Legacy mapping).

### sqrtModPrimeCipolla

`sqrtModPrimeCipolla(a, p)` returns the smaller square root `min(x, p - x)` of `a` modulo a prime `p`, or -1 for a nonresidue. `a = 0` gives 0, and `p = 2` returns `a mod 2`. The result equals Core `trySqrt` on the shared domain.

After a Kronecker residue test it scans `t = 0, 1, ...` until `t^2 - a` is a nonresidue. Exactly `(p - 1) / 2` values of `t` qualify, and the expected number of candidates is 2. The composite guard is described under Contracts. It then raises `t + i` to `(p + 1) / 2` in `F_p[i]` with `i^2 = t^2 - a`. Cost: `O((1 + z) * log(p))`, independent of the 2-adic valuation of `p - 1`.

### kthRootExists, kthRootModPrime

Both take a prime `p`, `k >= 0` (asserted) and `0^0 = 1`.

`kthRootExists(a, k, p)` decides solvability of `x^k = a (mod p)` in `O(log(p))`:

- `k = 0`: solvable exactly when `a = 1`;
- `a = 0`: always solvable;
- otherwise: solvable when `a^((p - 1) / gcd(k, p - 1)) = 1`.

`kthRootModPrime(a, k, p)` returns a deterministic root (not necessarily the smallest) or -1:

- `a = 0` or `1` returns `a`, which covers `k = 0`.
- Otherwise `r = k mod (p - 1)` and `g = gcd(r, p - 1)`.
- For each prime power `q^e || g`, `rootPrimePower` takes a `q^e`-th root `y`. Then `x = y^u` with `u = (r / g)^-1 mod (p - 1) / g`. This works because `x^r = a^(u r / g) = a` once `a^((p - 1) / g) = 1`.

`rootPrimePower(c, q, e, p)` is Adleman–Manders–Miller with a Pohlig–Hellman correction, for `p - 1 = q^s r` with `q` not dividing `r` and `Q = q^e`:

1. `x0 = c^d` with `d = Q^-1 mod r` (the code's `qe` is `Q`). Then `x0^Q / c` lies in the subgroup of order `q^s` and is a `Q`-th power there. When `s = e` it is 1, and `x0` is the root.
2. Otherwise a `q`-th power nonresidue `z` (scanned from 2) gives the generator `h = z^r`, of order exactly `q^s`. The error `x0^Q / c` equals `H^j` for `H = h^Q`.
3. `j` is found in base `q` over `s - e` digits. Each digit is a `bsgs` discrete log in the order-`q` subgroup `<H^(q^(s - e - 1))>`, and the root is `x0 h^-j`.

The other code names are `hq` for `H`, `ex` for the digit exponent and `qt = q^(s - e)`. Every BSGS runs in a subgroup whose order `q` satisfies `q^2 | p - 1`, so it costs `O(sqrt(q)) = O(p^(1/4))`. Only `g` is factored, never `p - 1`. Cost: `O(p^(1/4) * log(p) + z * log(p)^2)` expected, and `O(p^(1/4))` memory. For `k >= 1` and `a != 0`, all roots are `x * w^i` for `w = unitRootModPrime(gcd(k, p - 1), p)` and `0 <= i < gcd(k, p - 1)`. For `k = 0` and `a = 1` every residue is a root.

### unitRootModPrime

`unitRootModPrime(n, p)` returns an element of order exactly `n` modulo a prime `p` (asserted `n >= 1`, `p >= 2`), or -1 when `n` does not divide `p - 1`. The element is the first `c^((p - 1) / n)` of order `n` over `c = 1, 2, ...`, checked against the primes of `factorize(n)`, so `n = 1` gives 1. `p - 1` is not factored. The expected number of candidates is at most `n / phi(n) = O(log(log(n)))`. Cost: `O(n^(1/4) * log(n) + c * |f| * log(p))` expected.

### Aliases

`bsgs`, `exBsgs`, `modLog`, `mod_log`, `discrete_log` (`discreteLogEx`); `discrete_log_acted`, `discrete_log_monoid`, `DiscreteLogarithm` (`discreteLogMonoid`); `order`, `ord` (`multiplicativeOrder`); `primitive_root`, `getPrimRoot` (`primitiveRoot`); `jacobi` (`kroneckerSymbol`); `sqrt_mod`, `mod_sqrt` (`sqrtModPrimeCipolla`); `kth_root`, `mod_kth_root`, `discreteRoot` (`kthRootModPrime`); `primitive nth root of unity` (`unitRootModPrime`).

### Legacy mapping

| Legacy or removed name | Replacement |
|---|---|
| `modLog(a, b, mod, k)` (`OLD/5-Mathematics/08-modlogandmodroot.hpp`) | `discreteLogEx(a, b, mod, k)`, same least-`x` semantics; `mod >= 1` is asserted |
| `getPrimRoot(mod, opt)` | `primitiveRoot(m)`; the trial/Pollard `opt` flag is gone (factorization is always Pollard–Brent), and `mod <= 0` is asserted instead of returning -1 |
| `modRoot(a, b, mod, opt)` with prime `mod` | One root `kthRootModPrime(b, a, mod)`; all roots, sorted at the call site, are that root times the powers of `unitRootModPrime(gcd(a, mod - 1), mod)` |
| `modRoot` with `mod = p^k`, `2 p^k`, or `b = 0` and composite `mod` | Row 33 (`kthRootModPrimePower`, `kthRootModComposite`, planned) |
| `discreteLog` (prime modulus; row only, decided 2026-10-16) | `discreteLogEx` |
| `sqrtModPrime` (Tonelli–Shanks; row only, decided 2026-10-16) | Core `trySqrt` on `DynModInt`/`DynModInt64`, `kthRootModPrime(a, 2, p)` or `sqrtModPrimeCipolla` |
| `jacobiSymbol` (row only, decided 2026-10-16) | `kroneckerSymbol` |

## Feature-to-test map

Entry: [`11-discrete_log_root_tester.py`](<../../96-Local Testing/05-Mathematics/11-discrete_log_root_tester.py>).

The C++ oracles are iteration, brute order tables, square enumeration and definitions. The Python oracle uses constructed primes below `2^63` with known `p - 1` factorizations: smooth, `2 Q r`, `2^s r`, `q^s r` for `q` up to 1009, `Q^2 r` with `Q` up to `2^28`, the NTT primes, `2^61 - 1` and `1e18 + 3`. It checks its index formula, Kronecker definition and order characterization against brute force on primes below 200 before every run. Ten header mutants (least-`j` table, reduction recheck, reciprocity, AMM correction, monoid failure count, smaller root, `m = 4`, order loop, unit-root test, root exponent) were each caught by the C++ suite in quick mode. The Python oracle caught the nine in its scope: six in quick mode and three more in full; the monoid is C++ only. Failures print the operation, inputs, expected and actual values and the seed.

Limits below are given as quick / full / stress.

| Feature | Oracle and edge classes |
|---|---|
| `discreteLogEx` | Every `(a, b, k)` for `m <= 12 / 20 / 26`, and the default `k` with shifted representatives, against iteration over `[0, m)`. 400 / 4000 / 20000 random cases with `m` up to `2^14` (log-uniform, plus 720720, `2^16`, `3^10`, `2 * 5^6`, primes, `2 p`), 64-bit residues, half planted. Zero cases (`0^0 = 1`, `a = 0`, `k = 0`, `m = 1`). Python: exact least logs through indices of a known primitive root for primes and odd prime powers below `2^36`, including small orders and the zero cases; planted logs on composites up to `2^36` with shared factors and solutions below 4, checked for validity and `x <= planted`. |
| `discreteLogMonoid` | 150 / 900 / 4000 random functional graphs with `N <= 10` states. Every `(s, t)` over four ranges each (`lb = 0`, small, `lb` near `1e18`), with identity, constant and parity hashes (all-collision probing), against a tail-cycle brute force. False-hit-prone cases (`t` unreachable, `x · t` reachable) are counted: 1157 / 6824 / 31274. Modular scalars with ranges and `lb` up to `1e18`, also against `discreteLogEx`. 2 x 2 matrices mod `q <= 7` acting on vectors against iteration. |
| `multiplicativeOrder` | Every `m <= 120 / 320 / 520` and `a` in `[-m, 2m)`, plus 64-bit extremes, against brute order tables. `2^63 - 1`, 998244353. Python: products of constructed primes and powers of 2 up to `2^63`, nonunits giving -1, and orders checked as divisors of `lambda` with no removable prime. |
| `primitiveRootExists`, `primitiveRoot`, `isPrimitiveRoot` | Every `m` up to the same limits against the brute smallest element of order `phi(m)`; every `g` in `[-m, 2m)` plus extremes. Known roots of six NTT and Mersenne primes; `2^63 - 1`. Python: `p`, `p^e` and `2 p^e` up to `2^63`, 1, 2, 4, 8, 16 and `2^62`, non-cyclic products. Each root is checked by characterization and minimality over every smaller candidate, and `isPrimitiveRoot` on random `g`. |
| `kroneckerSymbol` | Every pair in `[-40, 40]^2 / [-120, 120]^2 / [-200, 200]^2` against the definition (sign rule, `(a / 2)` table, square enumeration), and 64-bit extremes (`LLONG_MIN`, `±2^62`, `3 * 2^61`, `45 * 2^55`, `2^63 - 1`). Python: signed `n` from factored products up to `2^63` and `n = -2^63`, by Euler's criterion per prime factor. |
| `legendreSymbol` | Every residue in `[-p, 2p]` plus extremes for odd primes below 150 / 600 / 1500, against square enumeration. Python: planted squares, `z * x^2` nonresidues and 0 for constructed primes up to `2^63`. |
| `sqrtModPrimeCipolla` | Every residue in `[-p, 2p)` plus extremes for every prime below 150 / 600 / 1500 (including 2), against the smaller enumerated root. Python: planted squares give exactly `min(x, p - x)`, and nonresidues give -1. |
| `kthRootModPrime`, `kthRootExists` | Every `a` in `[-p, 2p)` plus extremes, `k` in `[0, p + 1]` and six large `k` (multiples of `p - 1`, `2^63 - 1`, `1e18`), for primes below 50 / 100 / 160, against image tables. 21 primes with high `q`-adic `p - 1` (`2^16`, `3^9`, `5^6`, `7^4`, `11^3`, `13^3`), with `k` covering every `q^e`, `q^e r` and random multiples. Python: constructed primes with `k = 0`, random, multiples of `p - 1` and structured `q^e`, against planted powers, planted non-powers `z^j` with `gcd(k, p - 1)` not dividing `j`, 0 and 1; roots checked with Python `pow`. |
| `unitRootModPrime` | Every `n <= p + 1` for primes below 150 / 500 / 1000, giving order exactly `n` or -1. `p - 1` for three large primes gives a primitive root; `2^23` for 998244353. Python: random divisors and non-divisors of `p - 1` for constructed primes. |
| Preconditions | 14 checked-build probes: `m = 0` (order, `discreteLogEx`, existence, `isPrimitiveRoot`), `m = -5` (`primitiveRoot`), `lb > ub`, `lb < 0`, Legendre with `p = 2` and `p = 4`, `p = 1` (Cipolla, `kthRootExists`, `unitRootModPrime`), `k < 0`, `n = 0`. |

## Commands and results

2026-10-16, i9-11900H, Arch Linux, GCC 16.2 and GCC 14.4.1, CPython 3.14. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` with probes, and ASan/UBSan (full and stress). Counts are per configuration and equal on both compilers.

| Command | Result |
|---|---|
| `11-discrete_log_root_tester.py --mode quick --seed 1` | PASS 2 configurations, 377,089 C++ checks and 320 Python cases, 14 probes; 15 s; MEMORY peak 415 MB |
| `11-discrete_log_root_tester.py --mode full --seed 1` | PASS 3 configurations, 1,543,801 C++ checks and 2,400 Python cases (71 constructed primes), 14 probes; 67 s; MEMORY peak 559 MB |
| same with `CXX=g++-14` | PASS, same counts; 77 s; MEMORY peak 576 MB |
| `11-discrete_log_root_tester.py --mode stress --seed 1` | PASS 3 configurations, 5,005,397 C++ checks and 9,600 Python cases (161 constructed primes), 14 probes; 212 s; MEMORY peak 689 MB |
| `02-integration.py`, `03-consistency.py` and `--braces` | PASS: 145 standalone and aggregate headers, multi-TU and workspace in 397 s, MEMORY peak 651 MB; consistency no errors |

```bash
python3 '96-Local Testing/05-Mathematics/11-discrete_log_root_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/11-discrete_log_root_tester.py' --mode full --seed 1
python3 '96-Local Testing/05-Mathematics/11-discrete_log_root_tester.py' --mode stress --seed 1
```

The header and tester build with no warnings under `-Wall -Wextra -Wconversion -Werror` on both compilers. That includes two translation units with the header alone; with `99-all.hpp` beside it, the only warning comes from `06-segmentedsieve.hpp:151`, outside this package.

## Benchmarks

None are required: there is no Barrett, Montgomery or ISA-specific code. An ad hoc timing probe ran at Library Checker scale: `g++ -O2`, seed 1, three runs, median, with every result used. It measured:

- 100 `discreteLogEx` with random moduli in `[9e8, 1e9)`, half of them prime: 30 ms;
- 5000 `kthRootModPrime` modulo 67 primes below `1e9` (including 998244353 and 469762049) with planted powers and random `k < 1e9`: 7.2 ms;
- 100 `primitiveRoot` of random primes in `[9e17, 1e18)`: 1.7 ms;
- `1e5` `sqrtModPrimeCipolla` modulo primes below `1e9`: 65 ms.

## Sources

[00-sources.md](00-sources.md), P053 sweep: Library Checker `discrete_logarithm_mod`, `kth_root_mod`, `primitive_root` and `sqrt_mod`; cp-algorithms discrete log, primitive root and discrete root; OI Wiki residue pages; hitonanode, maspypy, suisen, Nyaan, KACTL and PyRival discrete-log and root files; FLINT `ulong_extras`. Cohen, *A Course in Computational Algebraic Number Theory*, Algorithm 1.4.10 (Kronecker), is cited from the standard statement. Everything was written independently. Legacy: `OLD/5-Mathematics/08-modlogandmodroot.hpp` (Legacy mapping).

Adopted by the P053 sweep: `discreteLogMonoid`. Rejected candidates are in [00-notes.md](00-notes.md).

## Limits and handoffs

- BSGS memory is `O(sqrt(ord))`, so a prime modulus near `1e18` with a large-order base is out of reach: a table of about `1e9` entries. Pohlig–Hellman, Pollard rho and many-query fixed-modulus logs are row 54.
- Roots modulo prime powers and composite moduli, all-root enumeration and root counts are row 33.
- `kthRootModPrime` returns one root, deterministic but not the smallest.
- `discreteLogMonoid` honours the range literally: its time and table are `O(sqrt(ub - lb))` even when the orbit is short, so pass the tightest `ub`.

## History

- 2026-10-16: P053 initial implementation; duplicates settled with the user (`discreteLog`, `sqrtModPrime`, `jacobiSymbol` removed from the row, `legendreSymbol` kept as Euler's criterion, `modLog` folded into `discreteLogEx`); `discreteLogMonoid` adopted from the sweep; review found 6 issues (NDEBUG bounds for `lb > ub`, `m < 0` and square composite Cipolla moduli; direct include; local names; monoid memory bound; evidence text), all fixed; full passed on g++ and g++-14, stress on g++.
