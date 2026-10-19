# 34-quadratic_congruence.hpp — evidence

`34-quadratic_congruence.hpp` solves quadratic Diophantine problems:

- Pell equations: the fundamental unit, families, and the generalized equation `x^2 - D y^2 = N`;
- Cornacchia's equation `x^2 + d y^2 = m`;
- sums of two, three and four squares, and three triangular numbers;
- primitive Pythagorean triples and Gauss circle counts;
- positive definite binary quadratic forms: reduction, Gauss composition, all representations, and the reduced forms of a discriminant.

## Contracts

Inputs are `lng`, and the arithmetic uses `__int128` where products can exceed 64 bits. The class-member enumerations (`x += M` in `cornacchia`, `pellGeneralized` and `represent`) count in `__int128`, so moduli near `2^63` cannot overflow. Nothing is cached. The header calls:

- `07` `factorize`, `isPrime` and `divisors` (Pollard–Brent and Miller–Rabin on Core `Montgomery64`);
- row 11 `sqrtModPrimeCipolla`;
- row 33 `sqrtModAll` and `quadraticCongruenceMod`.

It performs no Barrett or Montgomery arithmetic itself, so it needs no Compact twins.

Violated preconditions return an empty result or the sentinel after the assertion. A `-DNDEBUG` ASan/UBSan probe ran every violated call in 0.6 ms with no undefined behaviour; the calls are listed in [33-modular_square_roots.md](33-modular_square_roots.md), Contracts.

`quadratic_detail` holds:

- `isqrt`: exact floor square root of any `ulng`, from a `double` estimate corrected with overflow-free divisions. Row 21 will own the public `isqrt`.
- `descent(d, m, r)`: Cornacchia's Euclidean descent. Starting from `(m, r)`, it takes remainders until `r^2 < m`, then accepts `(r, y)` when `(m - r^2) / d = y^2`.
- `gaussPrime(p)`: `a^2 + b^2 = p`, from the square root of -1 by Cipolla.
- `pellUnit<T>`, `normalize<T>`, `definite` and `reduce`: described below.

### pellFundamental, pellSolutions

`pellFundamental<T>(D)` returns the least positive `(x, y)` with `x^2 - D y^2 = 1`. It returns `(0, 0)` when `D < 2` or `D` is a square; that pair is not a solution, so it cannot be confused with one.

- `T` needs construction from `lng`, `+` and `*`. `lll` is exact while `x < 2^127`, `iint` (Core `InfInt`) is always exact, and a modint gives residues.
- `pellUnit` runs the continued fraction of `sqrt(D)` with `lng` states (`0 < P <= s`, `0 < Q <= 2s`), updating the convergents `p_i` and `q_i` in `T`. It stops at the first `Q = 1`, the end of the period `l`. Then `p_(l-1)^2 - D q_(l-1)^2 = (-1)^l`.
- When `l` is odd, the result is the square `(x^2 + D y^2, 2 x y)`.
- Cost: `O(l)` ring operations, with `l = O(sqrt(D) * log(D))`.

`pellSolutions<T>(x, y, x1, y1, D, k)` returns `(x + y sqrt(D)) (x1 + y1 sqrt(D))^k` by binary powering, for a unit with `x1^2 - D y1^2 = 1` (asserted, so `x1^2` must fit `T` and `T` needs `==`). A negative `k` uses the conjugate unit, which is the inverse.

- It generates a family: the unit's powers from `(1, 0)`, or every member of a generalized-Pell class from its fundamental solution.
- With a modint `T` it gives the k-th member modulo a prime for huge `k`.
- Cost: `O(log(|k| + 2))` ring operations.

### pellGeneralized

`pellGeneralized<T>(D, N)` returns one solution per solution class of `x^2 - D y^2 = N`:

- Domain: `D >= 2` not a square, and `N > LLONG_MIN` (asserted).
- `T` must be ordered, exact and constructible from `lll`: `iint`, or `lll` when `64 x1^2 max(|N|, 1) < 2^127` with `x1` the fundamental solution. The unit, the expansion's convergents and the products unit times solution are formed in `T`, so a large unit overflows `lll` even when the answers are small (`D = 1021`, `N = -1020` has the class `(-1022, 32)` but a unit near `10^37`). A tracking number type over `iint` recorded the largest magnitude formed in 35,345 cases (every non-square `D <= 3000` with `N` in `{±1, ±4}`, random `|N| <= 10^3` and `<= 10^12`, and planted solutions): every case stayed below `64 x1^2 |N|`, by at least one decimal digit.
- Two solutions are in one class when they differ by a factor `±(x1 + y1 sqrt(D))^k`.
- The solution reported for a class is its fundamental one: the least `y >= 0`, and then the larger `x` (`x >= 0` for an ambiguous class).
- The output is sorted by `(y, x)`, and `N = 0` gives `(0, 0)`.
- All solutions are `±pellSolutions(x, y, x1, y1, D, k)` over the returned `(x, y)` and every `k`.

The algorithm is Lagrange–Matthews–Mollin (Robertson's formulation):

1. For each `f` with `f^2 | N`, set `m = N / f^2`.
2. For each square root `z` of `D` modulo `|m|` in `(-|m| / 2, |m| / 2]`, taken from row 33 `sqrtModAll`, expand `(z + sqrt(D)) / |m|` with `__int128` `P` and `Q`. The floor is exact for either sign of `Q`.
3. Stop at the first `Q_i = ±1` (`i >= 1`), or when the first reduced state `(P, Q)` repeats, which means no solution for this `z`.
4. On a stop, `G_(i-1)^2 - D B_(i-1)^2 = (-1)^i Q_i |m|`. When this equals `m`, `(f G, f B)` is a solution. When it equals `-m`, the solution exists only if the negative Pell equation is solvable (odd period), and is then multiplied by that unit.
5. `normalize` walks along the class by the unit while `|y|` decreases. Along the class `|y|` is unimodal, with at most one tie at the bottom, so the walk ends at the minimum, and the tie is resolved to the larger `x`.

Cost: the factorization of `|N|`, plus, for every `(f, z)` pair, the continued-fraction length of `(z + sqrt(D)) / |m|` in ring operations.

### cornacchia

`cornacchia(d, m)` returns every `(x, y)` with `x, y >= 0`, `gcd(x, y) = 1` and `x^2 + d y^2 = m`, sorted. Domain: `d >= 1`, `m >= 1`, `gcd(d, m) = 1` (asserted); the reduction for `gcd(d, m) > 1` belongs to `BinaryQuadraticForm::represent`.

- `m = 1` gives `(1, 0)`, and also `(0, 1)` when `d = 1`.
- For `m >= 2`, every primitive solution has `x, y >= 1` and determines the root `x / y` of `-d mod m`. The descent from the root `r <= m / 2` of each pair `{r, m - r}` finds the unique solution with `x = ±r y`.
- For `d = 1`, the pair `{λ, -λ}` also contains `(y, x)`, which is added explicitly.
- Cost: `O(m^(1/4) * log(m) + 2^w * log(m))` expected, with `w` the number of distinct primes.

### twoSquares, twoSquaresAll, twoSquaresCount

All three take `n >= 0`. They factor `n` once and use Gaussian integers.

- `twoSquares` returns `x >= y >= 0` with `x^2 + y^2 = n`, or `(-1, -1)`.
- `twoSquaresAll` returns every nonnegative ordered pair, sorted. This is the Library Checker `two_square_sum` output: the size is `r2(n) / 4 + [n is a square]`, and `n = 0` gives `(0, 0)`.
- `twoSquaresCount` returns `r2(n) = 4 prod_(p = 1 mod 4) (e_p + 1)`, or 0 when a prime `3 mod 4` has an odd exponent. Signs and order are counted, and `r2(0) = 1`.

The Gaussian integers of norm `n`, up to units, are `prod pi_p^k conj(pi_p)^(e_p - k)` with `pi_p = gaussPrime(p)`, times `(1 + i)^e_2` and `q^(e_q / 2)`. Each is rotated into `x > 0, y >= 0`, and `(0, sqrt(n))` is added for a square.

Cost: `O(n^(1/4) * log(n) + log(n)^2)` expected, plus `O(out * log(out))` for the list.

### threeSquares, fourSquares, threeTriangular

All take `n >= 0` and return their values in descending order.

`threeSquares(n)` returns `(x, y, z)`, or `(-1, -1, -1)` exactly when `n = 4^a (8 b + 7)` (Legendre).

1. Strip `4^a`.
2. Scan `z` downward from `isqrt(n)`, with the parity that makes `n - z^2` (halved when `n = 3 mod 8`) congruent to 1 mod 4.
3. Accept a perfect square or a prime, which `gaussPrime` splits. A halved remainder `p = a^2 + b^2` gives `2p = (a + b)^2 + (a - b)^2`.
4. If the scan finds nothing, a second scan over every `z` uses the factoring `twoSquares`; Legendre's theorem guarantees it ends.

The method is deterministic: the Rabin–Shallit idea (a prime remainder is split cheaply) in maspypy's top-down form.

- An ad hoc probe found that the fallback is needed only for `n` = 214, 526 and 1414 and their `4^k` multiples (24 values below `2 * 10^7`). The exhaustive suite covers them.
- In the same probe the scan tried 4.7 candidates on average for `n <= 2 * 10^7`, and 12.9 (at most 420) for `2 * 10^5` random 63-bit `n`.
- Expected `O(log(n))` candidates is heuristic: prime density on the quadratic values, unproven, as in Rabin–Shallit. The worst case, through the fallback, is `O(sqrt(n) * n^(1/4) * log(n))`.

`fourSquares(n)` strips `4^a`, takes `threeSquares(m)` or `threeSquares(m - 1) + 1^2` for `m = 7 mod 8`, and scales by `2^a`. It is a separate operation because its domain is every `n`.

`threeTriangular(n)` returns `(a, b, c)` with `T_a + T_b + T_c = n` (Gauss's Eureka theorem), for `n <= (2^63 - 4) / 8` (asserted). It uses `threeSquares(8 n + 3)`, whose three entries are odd; then `a = (v - 1) / 2`.

### pythagoreanTriples, gaussCircleCount

`pythagoreanTriples(N)` returns every primitive `(a, b, c)` with `a` odd, `b` even and `c <= N`, in Berggren-tree DFS order (unsorted).

- Domain: `0 <= N <= 2^63 / 8` (asserted), so the children's `c' <= 7 c` cannot overflow.
- The three Berggren matrices generate every primitive triple exactly once from `(3, 4, 5)` and increase `c`. Pruning at `c > N` makes the cost `O(out)` with no gcd.

`gaussCircleCount(n)` returns `#{(x, y) in Z^2 : x^2 + y^2 <= n}` as `lll`, because the count can exceed `2^63`.

- A two-pointer sweep over the columns (`y` decreasing as `x` increases) costs `O(sqrt(n))` with no square roots.
- Row 55 owns the `O(n^(1/3))` method.

### BinaryQuadraticForm

`BinaryQuadraticForm{a, b, c}` is the form `a x^2 + b x y + c y^2`, with public data and no invariant.

- Domain (`quadratic_detail::definite`, asserted): positive definite (`a > 0`, `D = b^2 - 4ac < 0`) with every coefficient and `|D|` below `2^62`. Then `D` and every product fit `__int128`, `represent`'s constant `((D mod 2) - D) / 4` fits `lng`, and the reduced and composed coefficients fit `lng`. Forms violating the bound return unchanged (`reduce`), unchanged `*this` (`compose`) or empty (`represent`) after the assertion.
- Indefinite forms are out of scope (00-notes).
- `quadratic_detail::reduce` applies the steps `x -> x + q y` (`q = floor((a - b) / 2a)`, which puts `b` in `(-a, a]`) and `(x, y) -> (-y, x)` while `a > c`. It finishes with one swap when `a = c` and `b < 0`. It records the `SL2(Z)` transform `t`, with `reduced = f(t (x, y))`. Each swap strictly lowers `a`, so it terminates.

The operations:

- `reduce()` returns the unique reduced form in the proper equivalence class: `|b| <= a <= c`, with `b >= 0` when `|b| = a` or `a = c`. Cost: `O(log(max(|b|, c) / a) + 1)` steps.
- `compose(o)` returns the Gauss composition of two primitive forms of one discriminant, reduced (asserted, together with definiteness). It reduces both forms first, then applies Shanks' composition (Cohen, *A Course in Computational Algebraic Number Theory*, Algorithm 5.4.7). The tests check it against Dirichlet's united-form definition. Cost: `O(log(|D|))`.
- `represent(n)` returns every `(x, y)` in `Z^2` with `f(x, y) = n`, sorted, for `n >= 0`. `n = 0` gives `(0, 0)`.
  1. Remove the content `k`; if `k` does not divide `n`, there is no representation.
  2. For every `g` with `g^2 | n`, the primitive representations of `n' = n / (k g^2)` correspond to roots `b0` of `b0^2 = D mod 4 n'`. These are taken as `b0 = 2t + (D mod 2)` with `t` from row 33 `quadraticCongruenceMod(1, D mod 2, ((D mod 2) - D) / 4, n')`, so the moduli stay `<= n'`, and `b0` is moved into `(-n', n']`.
  3. Each primitive form `(n', b0, ·)` equivalent to `f` (same reduced form) gives one representation `t_f t_h^-1 e1`.
  4. That representation is rotated through `Aut+(f)`: order 4 for `x^2 + y^2`, 6 for `x^2 + x y + y^2`, otherwise `±I`. The result is scaled by `g`.

  Cost: `O(n^(1/4) * log(n) * q + R * log(n))` expected, where `q` is the number of square divisors and `R` the number of roots `t` enumerated.
- `reducedForms(D)` (static) returns every primitive reduced form of discriminant `D`, sorted by `(a, b, c)`; the class number `h(D)` is its size. Domain: `-2^62 < D < 0` and `D = 0, 1 mod 4` (asserted). For each `b >= 0` with `b = D mod 2` and `3 b^2 <= -D`, the divisors `a` of `(b^2 - D) / 4` with `b <= a <= c` give `(a, ±b, c)`. Cost: `O(|D|^(3/4))` expected.

### Aliases

- `pell_equation`, `solve_pell` (`pellFundamental`); `diop_DN` (`pellGeneralized`);
- `SumOfTwoSquaresPrime`, `two_square_prime`, `prime_as_sum_of_two_squares` (`twoSquares` and `cornacchia` with `d = 1`); `two_square`, `two_square_sum` (`twoSquaresAll`); `r2` (`twoSquaresCount`);
- `three_square` (`threeSquares`), `four_square` (`fourSquares`), `three_triangular` (`threeTriangular`), `enumerate_pythagorean_triple` (`pythagoreanTriples`);
- class number (`reducedForms(D).size()`).

## Feature-to-test map

Entry: [`34-quadratic_congruence_tester.py`](<../../96-Local Testing/05-Mathematics/34-quadratic_congruence_tester.py>).

C++ oracles:

- enumerated pairs and triples, and Legendre's set by enumeration;
- prefix sums of `r2` and independent column sums;
- scans and Euclid's parametrization for triples;
- scans over `a <= sqrt(|D| / 3)` for reduced forms, and ellipse scans for representations;
- Dirichlet's united forms for composition;
- `y` scans, repeated multiplication and a hand-written `2 x 2` matrix power for Pell;
- Nagell-bounded exhaustive scans with class grouping for the generalized equation.

The Python oracle uses constructed factorizations and closed formulas, listed in the table. Limits below are given as quick / full / stress.

| Feature | Oracle and edge classes |
|---|---|
| `twoSquaresAll`, `twoSquaresCount`, `twoSquares` | Every `n <= 5000 / 60000 / 200000` against enumerated pairs. 300 / 3000 / 12000 random 64-bit and smooth `n`: validity, sorted and distinct lists of size `r2 / 4 + [square]`. Python: constructed `n` with primes `1` and `3 mod 4` (odd exponents included), powers of 2 and repeated primes, against the `r2` product formula. |
| `threeSquares`, `fourSquares`, `threeTriangular` | Every `n <= 5000 / 60000 / 200000` against Legendre's set by enumeration; this covers the fallback inputs 214, 526, 1414 and their `4^k` multiples. Random 64-bit `n`: validity and the exclusion `4^a (8 b + 7)`. Extremes `2^63 - 1`, `2^63 - 2`, `2^62`, `2^62 - 1`, `10^18`, and the prime `10^18 - 11`. Python: random `n` up to `2^63 - 1` and constructed excluded forms. |
| `cornacchia` | Every coprime `(d, m)` with `d <= 12 / 40 / 80` and `m <= 2000 / 8000 / 20000` against enumerated primitive pairs; this is where `m = 1` with `d = 1`, and `(1, 1)` for `m = 2`, are tested. 200 / 2000 / 8000 random 64-bit coprime `(d, m)`: validity, primitivity and sorted distinct output; Python: odd `m` from split primes, with `2^w` pairs for `d = 1` and `2^(w - 1)` for `d = 2`. |
| `pythagoreanTriples` | Brute force over `(a, c)` for `N` in `{0, 1, 4, 5, 12, 13, 16, 17, 25, 29, 100, 997, 600 / 2500}`; Euclid's parametrization for `N = 10^4` and `10^5 / 3 * 10^6`; exact sets and sizes. |
| `gaussCircleCount` | Every `n <= 5000 / 60000 / 200000` against prefix sums of enumerated `r2`; `n` in `{10^6, 10^12 - 1, 10^12, 4 * 10^12}` against an independent column sum. |
| `reducedForms` | Every discriminant down to -1500 / -12000 / -30000 against the scan. Python: Dirichlet's analytic class number formula `h = (w / 2 abs(D)) abs(sum (D / a) a)` for 3 / 12 / 30 fundamental `D` down to -20000 / -120000 / -250000. |
| `reduce` | Every reduced form for `abs(D) <= 150 / 400 / 800`, moved by 1–5 random `SL2(Z)` steps, reduces back to itself and is reduced. |
| `represent` | The moved forms and their multiples by 2–4 (non-primitive) against the ellipse scan, 2 / 4 / 4 values of `n` up to 3000 per form. Near `2^63`: `x^2 + y^2` (count equal to `twoSquaresCount`) and `x^2 + x y + y^2`, validity. Python: the nine class-number-one fundamental discriminants (-3, -4, -7, -8, -11, -19, -43, -67, -163) under random `SL2(Z)` moves, with `n <= 10^15` from small and 10–30-bit primes, counted as `w * sum (D / d)`. |
| `compose` | Every pair of reduced forms for `abs(D) <= 150 / 400 / 800`: shape, and Dirichlet's united form after moving the second form to a coprime leading coefficient. 40 / 200 / 600 random discriminants up to `8 * 10^7`: identity, inverse, commutativity, associativity, and `f^h = 1` for `abs(D) <= 4 * 10^6`. Python: united forms built by CRT for `abs(D)` up to `10^12`, reduced independently. |
| `pellFundamental` | Every `D` in `[-3, 300 / 2500 / 8000]`: `(0, 0)` for `D < 2` and squares, the equation in `iint`, minimality by scanning `y` when `y < 20000`, and the `mint` image. Python: an independent continued fraction for `D` up to `10^6`. |
| `pellSolutions` | Seven powers and their inverses against repeated `iint` multiplication (`D <= 400`); random `k < 2^63` with `mint` against a hand-written `2 x 2` matrix power. |
| `pellGeneralized` | Every non-square `D <= 40 / 120 / 250` with `x1 <= 10^12`, and every `abs(N) <= 25 / 80 / 150` whose Nagell bound is at most `10^5` (1700 / 17440 scans in quick / full), against the scan grouped into classes (least `y`, then larger `x`), exact list; `iint` agrees with `lll`. `N` near `±2^63` (`±(2^63 - 1)`, `2^62 - 57`): validity. Python: planted `(x0, y0)` with `D <= 10^4` and `abs(N)` up to `10^14`, which must have its class present; validity, distinct classes, order, minimality against both unit neighbours, and the tie rule. With `iint` for `D` in `{1021, 1117, 991, 661, 9949}` (units beyond the `lll` bound) and nine `N`: equation, `(y, x)` order, minimality against both unit neighbours, distinct classes, and `(-1022, 32)` present for `D = 1021`, `N = -1020`. |
| Preconditions | 24 checked-build probes: `cornacchia` with `d = 0`, `m = 0`, `gcd > 1`; negative `n` for each sum-of-squares function, `pythagoreanTriples` and `gaussCircleCount`; `threeTriangular` above its bound; `pellGeneralized` with a square `D`, `D = 1` and `N = LLONG_MIN`; `reducedForms` with `D > 0` or `D = 2 mod 4`; `reduce` on indefinite and negative definite forms; `compose` across discriminants and on a non-primitive form; `represent` on an indefinite form; `reduce` with coefficients below `2^62` but `abs(D) >= 2^62`; `pellSolutions` with a non-unit. |

## Commands and results

2026-10-17, i9-11900H, Arch Linux, GCC 16.2 and GCC 14.4.1, CPython 3.14. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` with probes, and ASan/UBSan (full and stress). Counts are per configuration and equal on both compilers.

| Command | Result |
|---|---|
| `34-quadratic_congruence_tester.py --mode quick --seed 1` | PASS 2 configurations, 236,791 C++ checks and 283 Python cases, 24 probes; MEMORY peak 583 MB |
| `34-quadratic_congruence_tester.py --mode full --seed 1` | PASS 3 configurations, 2,316,828 C++ checks and 2,112 Python cases, 24 probes; MEMORY peak 753 MB |
| same with `CXX=g++-14` | PASS, same counts; MEMORY peak 774 MB |
| `34-quadratic_congruence_tester.py --mode stress --seed 2` | PASS 3 configurations, 6,813,251 C++ checks (68,372 Pell scans) and 8,430 Python cases, 24 probes; MEMORY peak 781 MB |

```bash
python3 '96-Local Testing/05-Mathematics/34-quadratic_congruence_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/05-Mathematics/34-quadratic_congruence_tester.py' --mode full --seed 1
python3 '96-Local Testing/05-Mathematics/34-quadratic_congruence_tester.py' --mode stress --seed 2
```

The header and tester build with no warnings under `-Wall -Wextra -Wconversion -Werror` on both compilers. With `99-all.hpp` included, the only warning comes from `06-segmentedsieve.hpp:151`, outside this package.

## Benchmarks

None are required: there is no Barrett, Montgomery or ISA-specific code. An ad hoc timing probe ran at Library Checker scale: `g++ -O2`, seed 1, three runs, median, with every result folded into a checksum. It measured:

- 100 `twoSquaresAll` with `n <= 10^18`, half of them multiples of `5 * 13 * ... * 97`: 8.0 ms;
- `10^5` `threeSquares` with random `n <= 10^18`: 245 ms;
- `fourSquares` on the same input: 306 ms;
- 100 `pellFundamental<iint>` with `D <= 10^6`: 4.3 ms;
- 100 `pellGeneralized<iint>` with `D <= 10^4` and `|N| <= 10^12`: 1.2 ms;
- `reducedForms(-4 * 10^10 - 3)`: 131 ms;
- 100 `represent` by `x^2 + y^2` with `n <= 10^18`: 86 ms;
- `gaussCircleCount(10^16)`: 369 ms;
- `pythagoreanTriples(10^7)`: 20 ms.

## Sources

[00-sources.md](00-sources.md), P178 sweep:

- Library Checker `two_square_sum`: output convention and count;
- OI Wiki Pell and quadratic-form pages;
- maspypy `nt/` (three-square scan, four through three squares, Berggren enumeration);
- Nyaan `two-square.hpp`, hitonanode `square_sums.hpp`;
- the SymPy diophantine module (LMM, Cornacchia returning primitive solutions);
- Wikipedia's binary quadratic form article (class number as the count of reduced forms).

The following are cited from their standard published statements; none was fetched:

- J. P. Robertson, "Solving the generalized Pell equation x^2 - D y^2 = N" (2004): LMM, PQa, the classes and the negative-Pell step;
- Cohen, Algorithms 5.4.2 (reduction) and 5.4.7 (composition);
- Rabin and Shallit (1986): randomized sums of squares;
- Nagell: bounds for the fundamental solutions, used only by the tests;
- Berggren's tree.

Everything was written independently.

Research decisions (adopted and rejected) are in [00-notes.md](00-notes.md), P178.

## Limits and handoffs

- Pell sizes: the fundamental unit has `O(sqrt(D) log(D))` digits. `pellFundamental` and `pellSolutions` with `lll` need the results below `2^127`, and `pellGeneralized` with `lll` needs `64 x1^2 max(|N|, 1) < 2^127`; `iint` has no limit. Modular images of generalized solutions are taken after an exact `iint` run.
- `threeSquares` has a heuristic expected bound. The deterministic fallback is exponential in the worst case, and was reached only by 214, 526, 1414 and their `4^k` multiples.
- `BinaryQuadraticForm` is positive definite only. Indefinite reduction cycles and NUCOMP are out of scope (00-notes).
- `quadratic_detail::isqrt` duplicates the planned row 21 `isqrt` and the private `primality_detail::kthRoot(n, 2)` of `07`. When row 21 is implemented, that package should make this header call it. None of these helpers is public API.
- Row 57 lists `gaussianPrimeFactor` (two squares of a prime) and `fourSquaresFromHurwitz`, which overlap `twoSquares` and `fourSquares` here. The row 57 package should settle them under One API per operation.

## History

- 2026-10-17: P178 initial implementation, `reducedForms` adopted from the sweep, `fourSquares` made deterministic through `threeSquares`, UBSan found 2 overflows (fixed, with regressions), review found 7 issues, all fixed; full suite passed on g++ and g++-14, stress on g++.
