# 33-modular_square_roots.hpp — evidence

`33-modular_square_roots.hpp` solves power and polynomial congruences modulo prime powers and composite moduli. It covers square roots (one root, the least root, or every root as residue classes), k-th roots and their count, all roots of a polynomial congruence by Hensel lifting, and quadratic congruences through the discriminant. Prime-field roots are row 11.

## Contracts

Moduli are `lng` values of at least 1 (asserted). Prime-power forms take `(p, e)` with `e >= 1` and `p^e < 2^63` (asserted through an overflow-checked power), except `sqrtMod2k`, which accepts every `k` in `[0, 64]`. Residues and coefficients are any `lng`, `LLONG_MIN` included, normalized with `01` `modNorm`. Products are formed in `__int128`, and nothing is cached. Composite moduli are factored with `07` `factorize` (Pollard–Brent on Core `Montgomery64`). The header performs no Barrett or Montgomery arithmetic itself, so it needs no Compact twins.

Primality of `p` in the prime-power forms is a precondition that is not asserted, as in row 11, because a Miller–Rabin test would dominate. A composite `p` still terminates within the same bounds with a meaningless result: row 11's Cipolla guard, bounded digit loops, and inverses through `extendedGcd`.

Violated `m >= 1`, `k >= 0`, `0 <= k <= 64` and the `p^e` overflow are clamped or return the sentinel after the assertion: `m` becomes 1, `k` becomes 0, `k` is clamped to `[0, 64]`, and an overflowing `p^e` gives -1. An ad hoc `-DNDEBUG` probe, also built with ASan/UBSan, ran every violated precondition of rows 33 and 34 (49 calls) in 0.6 ms with no undefined behaviour. The calls include `p^e` overflow, `e = 0`, `e = 10^9`, `k = 65` and `k = 10^6`, `m = 0`, `m = -9` and `m = LLONG_MIN`, `k < 0`, and composite or square `p`.

### Residue classes

Every all-solutions operation returns `vector<pair<lng, lng>>`: classes `(r, M)` with `M | m` and `0 <= r < M`, sorted and pairwise disjoint. Their union is exactly the solution set in `[0, m)`, so the number of solutions is the sum of `m / M`. This is the `(residue, modulus)` convention of `10` `crt`. The compact form is needed because the root sets can be huge: `x^2 = 0 mod 2^62` alone has `2^31` roots, `x ≡ 0 mod 2^31`. `sqrtModAll` and `quadraticCongruenceMod` produce classes that share one modulus. `polynomialCongruenceRoots` classes may have different moduli. Classes from distinct prime powers are combined by CRT products (`merge`), so the output size is the product of the per-prime class counts.

`modroot_detail` holds:

- `mul`, `pw`, `inv`, `val` and `checkedPow`: arithmetic helpers.
- `merge`: the CRT product of two class lists.
- `sqrtPP`, `unitRoot`, `kthPP` and `countPP`: the per-prime-power kernels described below.
- `trim`, `divmod`, `mulMod`, `powMod`, `gcdPoly` and `rootsModP`: dense polynomials over `F_p`.
- `newton`, `liftRoots` and `quadPP`: lifting and the quadratic solver.

### sqrtModPrimePower, sqrtMod2k

Both return the least root of `x^2 = a`, or -1.

- `sqrtModPrimePower(a, p, e)` handles prime `p`, `e >= 1` and `p^e < 2^63`.
- `sqrtMod2k(a, k)` reads `a` modulo `2^k` as two's complement and accepts `k` in `[0, 64]`. Its least root is always below `2^63`: the roots come in pairs `r` and `2^k - r`, and `2^63` is a root only of 0, whose least root is 0.

`sqrtPP(a, p, e)` returns `(j, sorted residues mod p^j)`:

- `a = 0`: the single class `0 mod p^ceil(e / 2)`.
- `a = p^v u` with `u` a unit and `v` odd: no root.
- Otherwise `v = 2t`, `f = e - v`, and the roots are `p^t y` with `y^2 = u mod p^f`, where `y` matters modulo `p^(e - t)`.
- Odd `p`: Cipolla (row 11) gives `y0` modulo `p`. Newton's inverse square root `z <- z (3 - u z^2) / 2` doubles the p-adic precision on each of `ceil(log2(f))` steps. The two classes are `±p^t u z mod p^(e - t)`; they are distinct because `e - t > t`.
- `p = 2`:
  - `f = 1` gives the class `2^t mod 2^(t + 1)`.
  - `f = 2` gives the same class when `u = 1 mod 4`, and no root otherwise.
  - `f >= 3` needs `u = 1 mod 8`. `y` is lifted one bit at a time: add `2^(i - 1)` when bit `i` of `y^2 - u` is set, which works because `(y + 2^(i - 1))^2 = y^2 + 2^i y mod 2^(i + 1)` for `i >= 3`. The four roots `±y` and `±y + 2^(f - 1)` collapse to the two classes `±2^t y mod 2^(e - t - 1)`.
  - Wrapping 64-bit arithmetic handles `e = 64`.

Cost: `O(log(p) * log(p^e) + z * log(p))` expected, with `z` the Cipolla candidates, and `O(k)` for `sqrtMod2k`.

### sqrtModComposite, sqrtModAll

Both factor `m`.

- `sqrtModComposite(a, m)` combines the least root of each prime power by CRT. The result is a deterministic root, not necessarily the least, or -1.
- `sqrtModAll(a, m)` returns the classes of every root; `sqrtModAll(a, m)[0].first` is the least root.

Cost: `O(m^(1/4) * log(m) + w * log(m)^2)` expected, plus `O(out * w * log(m))` for the classes, where `w` is the number of distinct primes.

### kthRootModPrimePower, kthRootModComposite, rootCount

All three take `k >= 0` with `0^0 = 1`.

- The two root functions return some x with `x^k = a` (deterministic, not the least), or -1.
- `rootCount(a, k, m)` returns the number of x in `[0, m)`, which is at most `m`.
- `k = 0`: every x is a root when `a = 1 mod m`, and the root returned is `1 mod m`.
- `a = 0`: the root is 0.

`kthPP` removes the valuation: `a = p^v w` needs `k | v`, and then `x = p^(v / k) y` with `y^k = w mod p^(e - v)`.

`unitRoot(w, k, p, f)` writes `k = p^s k'` with `p` not dividing `k'`, and uses `j0 = 1` for odd `p` and `j0 = 2` for `p = 2`:

1. Take `r0`, a root modulo `p`: row 11 `kthRootModPrime` for odd `p`, and `r0 = 1` for `p = 2`. Every lift satisfies `u = w r0^-k = 1 mod p`, and whether `u` is a k-th power does not depend on which root `r0` was chosen.
2. In the cyclic group `1 + p^j0 Z` (order `p^(f - j0)`), `x -> x^(k')` is a bijection, inverted by `k'^-1 mod p^(f - 1)`. Then `x -> x^(p^s)` maps `1 + p^j Z` onto `1 + p^(j + s) Z` for `j >= j0`. So a root exists exactly when `u = 1 mod p^min(s + j0, f)`.
3. When `s > 0`, `z^(p^s) = tgt` is solved one p-adic digit at a time, using `(1 + p^j c)^(p^s) = 1 + p^(j + s) c mod p^(j + s + 1)`.
4. The root is `r0 z`.

This replaces Hensel lifting, which fails when `p | k`, and the OI Wiki BSGS digit search: each digit costs one power, not a discrete log.

`countPP` counts the roots. With `v = v_p(a)`:

- `a = 0`: `p^(e - ceil(e / k))`.
- `k` does not divide `v`: 0.
- Otherwise `N p^(v - v / k)`, where `N` is the unit count modulo `p^f` with `f = e - v`:
  - odd `p`: `gcd(k, phi)` when `w^(phi / gcd) = 1`, else 0;
  - `p = 2`: 1 for odd `k` or `f = 1`; otherwise `2^(1 + min(s, f - 2))` when `w = 1 mod 2^min(s + 2, f)`, else 0.

`rootCount` is the product over the prime powers.

Cost: `O(m^(1/4) * log(m) + w * (p^(1/4) * log(p) + log(k) * log(m)^2))` expected for the roots, where `p^(1/4)` comes from row 11's BSGS. `rootCount` costs `O(m^(1/4) * log(m) + log(k) * log(m))`.

### polynomialCongruenceRoots

`polynomialCongruenceRoots(f, m, seed = 0)` takes `f` low to high, with any `lng` coefficients, possibly empty or zero. It returns the disjoint classes of every x with `f(x) = 0 mod m`. The zero polynomial gives `{(0, 1)}`.

For each `p^e || m`, `liftRoots(g, p, K)` returns balls `(r, j)` (every `x = r mod p^j` is a root), following Berthomieu–Lecerf–Quintin cluster lifting:

1. Divide by the content `p^v`. If `v >= K`, everything is a root.
2. Find the distinct roots of `g mod p` with `rootsModP`:
   - when `p <= 64 |f|`, evaluate every residue in `O(p |f|)`;
   - otherwise use Cantor–Zassenhaus: `gcd(g, x^p - x)`, then split with `gcd(h, (x + delta)^((p - 1) / 2) - 1)` and seeded random `delta`.
3. A simple root (`g'(r)` a unit) lifts to one residue modulo `p^K` by Newton's iteration with doubling precision.
4. A multiple root recurses on the Taylor shift `g(r + p t)`, whose coefficients all vanish modulo `p`. The precision drops by at least 1 per level, so the depth is at most `e`. The reduced degree at a child is at most the root's multiplicity, so there are at most `|f|` clusters per prime and `O(|f| e)` nodes.

Randomness is Las Vegas: the seed changes only the running time, never the result (`std::mt19937_64(seed)`). Cost: `O(m^(1/4) * log(m) + w * n^3 * e * log(p) + out * w * log(m))` expected, with `n = |f|`. The `log(p)` factor becomes `O(p n)` root scans when `p <= 64 n`.

### quadraticCongruenceMod

`quadraticCongruenceMod(a, b, c, m)` returns the classes, all with one modulus, of every x with `a x^2 + b x + c = 0 mod m`. It covers every degenerate case: `a = 0` (linear), all coefficients divisible by `p`, and `a = b = c = 0` (every x).

It is a separate operation from `polynomialCongruenceRoots`: a deterministic O(log)-per-prime discriminant algorithm with no root finding. For each `p^e`:

1. Remove the content `p^v`.
2. If `p` does not divide `a`:
   - odd `p`: x is `(y - b) (2a)^-1` over the square-root classes `y` of `b^2 - 4ac mod p^e`;
   - `p = 2`: `(2ax + b)^2 = b^2 - 4ac mod 2^(e + 2)`, computed in wrapping 64-bit arithmetic (`e + 2 <= 64`). Every root `y` has the parity of `b`, so `x = ((y - b) / 2) a^-1` is a class modulo `2^(j - 1)`.
3. If `p | a` and `p` does not divide `b`, every root modulo `p` is simple, and Newton lifts the unique root.
4. If `p | a` and `p | b`, the constant is a unit and there is no root.

Cost: `O(m^(1/4) * log(m) + w * log(m)^2 + out * w * log(m))` expected.

### Aliases

`sqrt_mod_prime_power`, `n_sqrtmod_primepow` (`sqrtModPrimePower`); `n_sqrtmod_2pow` (`sqrtMod2k`); `n_sqrtmodn`, `sqrt_mod_composite` (`sqrtModComposite`, `sqrtModAll`); `kth_roots_mod_pe`, `mod_kth_root` (prime powers) (`kthRootModPrimePower`); `kth_roots_mod_m` (`kthRootModComposite`); `QuadraticEquation`, `solve_quadratic_mod` (`quadraticCongruenceMod`); `fmpz_mod_poly_roots_factored`, `poly_roots_mod` (`polynomialCongruenceRoots`).

### Legacy mapping

| Legacy name | Replacement |
|---|---|
| `modRoot(a, b, mod, opt)` with `b = 0` and any `mod` (`OLD/5-Mathematics/08-modlogandmodroot.hpp`) | The roots of `x^a = 0` are the single class `0 mod prod p^ceil(e / a)`: `polynomialCongruenceRoots` on `x^a` (or `sqrtModAll(0, mod)` for `a = 2`), with `rootCount(0, a, mod)` for the count |
| `modRoot` with `gcd(b, mod) = 1` and `mod` with a primitive root (`p^k`, `2 p^k`) | One root `kthRootModComposite(b, a, mod)`, count `d = rootCount(b, a, mod)`; all roots, sorted at the call site, are `x0 * (g^(phi(mod) / d))^i` for `i < d`, with `g = primitiveRoot(mod)` (row 11) |
| `modRoot` with a prime modulus | Row 11 (`kthRootModPrime`, `unitRootModPrime`) |
| `opt` (trial or Pollard factorization) | Always Pollard–Brent through `07` `factorize` |

## Feature-to-test map

Entry: [`33-modular_square_roots_tester.py`](<../../96-Local Testing/05-Mathematics/33-modular_square_roots_tester.py>).

The C++ oracles are enumeration, power tables, evaluation at every residue, and exact Taylor-term checks. The Python oracle builds moduli from three kinds of components, each with a count known independently:

- small prime powers up to 4096, counted by brute force;
- large primes up to `2^62`, counted from Euler's criterion and `gcd(k, p - 1)`;
- squared or cubed medium primes, counted from the cyclic group order.

Every returned class is checked exactly: `f(r) = 0` and `f^(j)(r) M^j / j! = 0 mod m` for all `j`. It must also be disjoint from the others, and the class sizes must add up to the constructed count. All tags in the table are present in the code. Limits below are given as quick / full / stress.

| Feature | Oracle and edge classes |
|---|---|
| `sqrtModAll`, `sqrtModComposite` | Every residue of every `m <= 120 / 400 / 700` plus six random 64-bit representatives, `LLONG_MIN`, `LLONG_MAX`, `-m` and -1, against enumerated roots: exact class expansion, sorted and disjoint. Planted squares modulo `P q` for every planted prime power `P` below with `P q < 2^63` (`q` = 3 or `1e9 + 7`): class shape and validity by `r^2 = a`, `2 r M = 0`, `M^2 = 0 mod m`, and total size `= rootCount`. Python: constructed composites with planted squares, nonresidues and zero per component, with exact class checks and counts. |
| `sqrtModPrimePower` | The same exhaustive window on prime powers, as least roots. On every planted iteration, squares modulo `2^62`, `3^39`, `5^27`, `7^22`, `11^18`, `13^17`, `(1e6 + 3)^3`, `(2^31 - 1)^2`, `2^61 - 1` and `(1e9 + 7)^2`, where the root is at most `min(x, P - x)`. Python: planted unit squares give exactly `min(x, P - x)`, and nonresidues give -1. |
| `sqrtMod2k` | Exhaustive least roots for every `k <= 14` and residue, plus 40 shifted 64-bit representatives per `k`. 400 / 3000 / 12000 cases with `k` in `[15, 64]`: planted squares (some with high 2-adic valuation) whose root is at most the four known roots, and random residues against the characterization of squares (`v` even, `u` 1 mod 8, 4 or 2). Agrees with `sqrtModPrimePower(a, 2, k)` for `k <= 62`. Python: exact least root `min(±y, ±y + 2^(k - 1))` of planted odd squares for `k` in `[3, 64]`. |
| `kthRootModComposite`, `kthRootModPrimePower`, `rootCount` | Every `(a, k)` with `k <= 10 / 18 / 24`, plus `LLONG_MAX`, `LLONG_MAX - 1`, `2^62`, `720720 * 1000003`, `3 * 2^60` and `6 * (1e9 + 7)`, for every `m <= 60 / 150 / 230`, against power tables: validity, -1 exactly when no root, exact counts. 300 / 2500 / 10000 planted roots modulo the large prime powers above, with p dividing k, up to `p^6` multipliers, non-units and random `k` up to `2^63`. `rootCount` multiplicativity on `P q`. Python: per-component counts from tables, `gcd(k, p - 1)`, `p^e` for `k = 0`, and 1 for zero; small prime powers with q dividing k against planted roots. |
| `quadraticCongruenceMod` | Every `m <= 50 / 120 / 180`, the coefficient grid `{0, ±1, 2, 3, 4, 6, 8, 9, 12, 25, 27}^3` (8 values in quick), and 20 random 64-bit triples per `m`, against evaluation; common modulus checked. On every planted iteration of the large prime powers above, `lead (t - x)(t - z)`, with a content `p` in one case of four: classes checked by `f(r)`, `f'(r) M` and `lead M^2`, both planted roots covered, total equal to `polynomialCongruenceRoots`, and exactly 2 for two simple roots. Python: components with random, linear (`a = 0`), two-root, double-root and content-`p` coefficients, counted by the discriminant or brute force; `2^e` (`e <= 40`) and `p^2`, `p^3` components with two planted simple roots (count 2) or, for odd `p`, a nonresidue discriminant (count 0). |
| `polynomialCongruenceRoots` | 400 / 4000 / 16000 cases with `m <= 300` and the moduli 64, 81, 128, 243, 625, 1024, 2187, 3125, 4608 and 2048, against evaluation at every residue: zero polynomial, random coefficients with multiples of `m`, products of `(x - r)^k` with multiplicity up to 3, contents 2, 3, 4, 8, 9, and an extra constant term. 40 / 300 / 1200 planted products modulo `2^61 - 1`, `2^62 - 57`, `1e9 + 7`, 998244353, `1e6 + 3` and `(1e9 + 7)^2`, through Cantor–Zassenhaus with Newton or cluster lifting; every Taylor term is checked and every planted root covered. Python: random small components; `prod (x - r)^m (x^2 - n)` modulo large `p`, counted as distinct `r`; and `(x - r1)^2 (x - r2)(x^2 - n)` modulo `p^2`, counted as `p + 1`. |
| Preconditions | 15 checked-build probes: `p^e` overflow (`2^63`, `3^40`), `e = 0`, `p = 1`, `k = 65`, `k = -1`, `m = 0` and `m = -3` (composite forms, `rootCount`, polynomial, quadratic), and `k < 0`. |

## Commands and results

2026-10-17, i9-11900H, Arch Linux, GCC 16.2 and GCC 14.4.1, CPython 3.14. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` with probes, and ASan/UBSan (full and stress). Counts are per configuration and equal on both compilers.

| Command | Result |
|---|---|
| `33-modular_square_roots_tester.py --mode quick --seed 1` | PASS 2 configurations, 257,547 C++ checks and 420 Python cases, 15 probes; MEMORY peak 454 MB |
| `33-modular_square_roots_tester.py --mode full --seed 1` | PASS 3 configurations, 1,950,562 C++ checks and 2,800 Python cases, 15 probes; MEMORY peak 551 MB |
| same with `CXX=g++-14` | PASS, same counts; MEMORY peak 549 MB |
| `33-modular_square_roots_tester.py --mode stress --seed 2` | PASS 3 configurations, 4,538,120 C++ checks and 10,500 Python cases, 15 probes; MEMORY peak 665 MB |

```bash
python3 '96-Local Testing/05-Mathematics/33-modular_square_roots_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/33-modular_square_roots_tester.py' --mode full --seed 1
python3 '96-Local Testing/05-Mathematics/33-modular_square_roots_tester.py' --mode stress --seed 2
```

The header and tester build with no warnings under `-Wall -Wextra -Wconversion -Werror` on both compilers. With `99-all.hpp` included, the only warning comes from `06-segmentedsieve.hpp:151`, outside this package.

## Benchmarks

None are required: there is no Barrett, Montgomery or ISA-specific code. An ad hoc timing probe ran at Library Checker scale: `g++ -O2`, seed 1, three runs, median, with every result folded into a checksum. It measured:

- `1e5` `sqrtModComposite` with random `m <= 1e18`, half of them planted: 1490 ms, about 15 µs each, dominated by Pollard–Brent factorization;
- `sqrtModAll` on the same input: 1786 ms;
- `quadraticCongruenceMod` on the same input: 1737 ms;
- `rootCount` on the same input: 1512 ms;
- `1e4` planted `kthRootModComposite` with `m, k <= 1e18`: 173 ms;
- 100 degree-10 planted `polynomialCongruenceRoots` modulo `1e18 + 3`, `2^61 - 1` or `999999937^2`: 81 ms.

## Sources

[00-sources.md](00-sources.md), P178 sweep:

- OI Wiki quadratic and k-th residues: roots when `p | a`, counts for `p^e` and `2^e`, and the `p | k` digit search;
- FLINT `ulong_extras` (`n_sqrtmod_primepow`, `n_sqrtmod_2pow`, `n_sqrtmodn`) and `fmpz_mod_poly_roots_factored`;
- Nyaan `quadratic-equation.hpp`: the discriminant-1 case, tested;
- maspypy `mod_kth_root`.

The cluster lifting follows Berthomieu, Lecerf and Quintin, "Polynomial root finding over local rings and application to error correcting codes" (2013), cited from its published statement; the paper was not fetched. Cantor–Zassenhaus root splitting is the textbook algorithm. Everything was written independently. Legacy: `OLD/5-Mathematics/08-modlogandmodroot.hpp` (Legacy mapping).

Research decisions (adopted and rejected) are in [00-notes.md](00-notes.md), P178.

## Limits and handoffs

- `sqrtModComposite` and the k-th root functions return a deterministic root, not the least. The least square root is `sqrtModAll(a, m)[0].first`.
- k-th roots of a unit modulo `p^e` call row 11 `kthRootModPrime`, so a large prime power `q^s || p - 1` with `q^2 | p - 1` costs `O(q^(1/2))`. All k-th roots of a composite modulus are a coset, not a union of classes, and are not enumerated (00-notes).
- `rootsModP` here is a private Cantor–Zassenhaus over `F_p` for 64-bit `p`. When row 46 `rootsModPrime` (prime-field polynomial roots) is implemented, that package should make this header call it rather than keep two public-equivalent kernels.
- `modroot_detail::mul` repeats row 11's `discrete_log_detail::mul` (one line, private to each family; neither is public API).
- Polynomial degree enters cubically (`O(n^3 e log(p))`). The intended use is small-degree congruences, not row 46-scale factorization.

## History

- 2026-10-17: P178 initial implementation, sweep added no operation (five candidates rejected), UBSan audit found no class enumeration in the header, review found no header defect, test coverage and style issues fixed; full suite passed on g++ and g++-14, stress on g++.
