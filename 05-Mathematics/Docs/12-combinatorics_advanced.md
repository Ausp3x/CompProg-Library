# 12-combinatorics_advanced.hpp — evidence

`12-combinatorics_advanced.hpp` holds the combinatorics beyond factorial tables modulo a large prime: binomials for large indices and composite moduli, binomial prefix sums, valuations, Stirling, Bell, Eulerian, Entringer and Lah numbers, partitions, q-analogues, the binomial and Stirling transforms, pruned inclusion–exclusion, subset sums, Pisano periods, Zeckendorf representations and indecomposable permutations. It depends on `05-combinatorics.hpp` (`PrimeCombinatorics`), `07-primality_factorization.hpp` (`factorize`, `isPrime`), `09-modinverse.hpp` (`inverseTable`, `powerTable`), `13-convolution.hpp`, `14-polynomial_algorithms.hpp` (`fpsInverse`, `fpsLog`, `fpsExp`, `fpsPow`, `taylorShift`, `productTree`) and Core `16-poly.hpp` (`evalMulti`, for `inverseStirlingTransform`). The Stirling rows, the Eulerian row and `partitionCount` are intended duplicates of Core Poly series (user decision 2026-10-19; inventory scope note).

## Contracts

Common domain: `M` is a Core modint. Functions that take `const PrimeCombinatorics<M> &pc` need a prime modulus and assert that pc covers every factorial index they read. Tables of n + 1 values that use inverses of 1..n need a prime modulus > n, as stated per operation. Convolutions go through row 13 `convolution`, which handles every 64-bit modulus and any length, so any modulus works where no inverse is taken.

### factorialValuation, kummerCarries

`factorialValuation(n, p)` returns sum_i floor(n / p^i), Legendre's v_p(n!) for prime p, for any 64-bit n and p >= 2. `kummerCarries(n, k, p)` with 0 <= k <= n returns the number of carries when k and n - k are added in base p, which by Kummer's theorem is v_p(C(n, k)) for prime p. Digit sums are formed in 128 bits, so p up to 2^64 - 1 is exact. O(log_p(n)).

### binomialLucas

`binomialLucas(n, k, pc)` returns C(n, k) mod p = M::mod() for any 64-bit n, k by Lucas' theorem, the product of C(n_i, k_i) over base-p digits (0 as soon as k_i > n_i). Every digit of n must be at most pc.n, so `PrimeCombinatorics<M>(p - 1)` serves all n; a large p with digits below a smaller table also works. O(log_p(n)) per query. Library Checker `binomial_coefficient_prime_mod` (n < p) is row 05's table.

### BinomialSmallPrimeTable

`BinomialSmallPrimeTable<M>(N)` stores u_i = i! / p^(v_p(i!)) mod p, its inverses and v_p(i!) for i <= N, for a prime p = M::mod() of any size. `operator()(n, k)` with 0 <= n <= N returns 0 when v_p(n!) > v_p(k!) + v_p((n - k)!), else u_n / (u_k u_(n-k)), and 0 outside 0 <= k <= n. S: O(N), Q: O(1), M: O(N). It is kept beside Lucas for its constant factor at n <= N (user decision 2026-10-19).

### BinomialPrimePower

`BinomialPrimePower(p, e)` for a prime power q = p^e <= 2^27 (`CAP`, 512 MB of `uint`; p asserted prime, the cap asserted, and under `-DNDEBUG` the table stops growing at the cap) stores f_i, the product of the j <= i coprime to p, modulo q. `operator()(n, k)` returns C(n, k) mod q for any 64-bit n, k (0 for k > n) by Granville's method:

- `unitFactorial(n)`, the p-free part of n! mod q, is f_(n mod q) times f_(q-1)^floor(n / q) times the p-free part of floor(n / p)!. The units of Z/q multiply to f_(q-1) = +-1 (generalized Wilson), so its powers are signs.
- With c = v_p(C(n, k)) from Legendre, C(n, k) = p^c unitFactorial(n) / (unitFactorial(k) unitFactorial(n - k)) mod q; c >= e gives 0. The inverse is a 128-bit extended Euclid inverse modulo q.

Q: O(log_p(n) + log(q)). With e = 1 this is a different algorithm from Lucas with the same results.

### BinomialCompositeMod

`BinomialCompositeMod(m)` accepts any 64-bit m >= 1 whose prime powers are each at most 2^27 (asserted per prime power; under `-DNDEBUG` an oversized part degenerates to a one-entry table). It builds a `BinomialPrimePower` per prime power q of m and the CRT coefficients (m / q) ((m / q)^-1 mod q) mod m. `operator()(n, k)` returns C(n, k) mod m as a `ulng`, combining the residues with 128-bit products: Library Checker `binomial_coefficient` (m <= 10^6, n up to 10^18) and smooth moduli up to 2^64 - 1. S: O(s + m^(1/4) log(m)) and M: O(s), where s is the sum of the prime powers of m. Q: O(w (log(n) + log(m))) for w prime powers. m = 1 gives 0.

### binomialRowCompositeMod

`binomialRowCompositeMod(N, m)` returns C(N, k) mod m for all k <= N, for any 64-bit m >= 1 (suisen). Each step multiplies by N - k + 1 and divides by k: the primes of m that are <= N are stripped into an exponent vector (primes of m above N never divide the factors), the coprime parts multiply a unit, and the divisions use inverses of the stripped k obtained by one batch inversion (prefix products and one 128-bit extended Euclid, so m may reach 2^64 - 1). The value is the unit times the product of p^e_p, read from power tables. O(N w + m^(1/4) log(m)) expected (the factorization of m included) for w primes of m up to N, M: O(N), 128-bit products throughout.

### binomialPrefixSum, binomialPrefixSumQueries, BinomialPrefixSumTable

- `binomialPrefixSum(n, k, pc, r = 1)` returns S_r(n, k) = sum_(i <= k) r^i C(n, i) for any 64-bit n (suisen's weighted form). k < 0 gives 0 and k >= n gives (1 + r)^n. For n <= pc.n the shorter side is summed, using (1 + r)^n minus the tail, so the cost is O(min(k, n - k)). Otherwise C(n, i) runs by C(n, i + 1) = C(n, i) (n - i) / (i + 1), which is exact modulo p for i < p, so the cost is O(k) with k <= pc.n.
- `binomialPrefixSumQueries(qs, pc, r = 1)` answers offline queries (n, k) with n <= pc.n by Mo's order. It moves k by one with r^k C(n, k), and n by S_r(n + 1, k) = (1 + r) S_r(n, k) - r^(k + 1) C(n, k) and its inverse, which divides by 1 + r. When 1 + r = 0 (r = -1, or r = 1 modulo 2) the sum is (-1)^k C(n - 1, k) for n >= 1, so every query is answered directly in O(1). k is clamped to [-1, n]. O(N sqrt(q) + q log(q)) for N = max n, any r.
- `BinomialPrefixSumTable<M>(N, B = 0, r = 1)` stores S_r(n, jB) for every n <= N and every block boundary jB <= N (default B = sqrt(N)), each column by S_r(n + 1, K) = (1 + r) S_r(n, K) - r^(K + 1) C(n, K), which needs no division, so any weight works. `sum(n, k)` adds at most B weighted binomials to the nearest stored column: S: O(N^2 / B), Q: O(B), M: O(N^2 / B) (maspypy), prime modulus > N.

### risingFactorial, fallingFactorial

x (x + 1) ... (x + n - 1) and x (x - 1) ... (x - n + 1) in any commutative ring T, 1 for n = 0, O(n) multiplications. This is optimal for a general ring; the O(sqrt(n) log(n)) sample-point shift over prime fields is row 37 `productOfLinearShifts`.

### stirling1Row, stirling2Row, stirling1Column, stirling2Column

- `stirling1Row<M>(n)`: signed s(n, k) for k <= n (Library Checker convention, x (x - 1) ... (x - n + 1) = sum s(n, k) x^k). For a prime modulus > n it doubles x^(2m falling) = x^(m falling) (x - m)^(m falling) with `taylorShift`, plus one linear factor per set bit, O(n log(n)). Under any other modulus (composite, or n >= p, where the shift's factorials do not exist) it multiplies the n linear factors by `productTree`, O(n log(n)^2).
- `stirling2Row<M>(n)`: S(n, k) = sum_i i^n / i! (-1)^(k-i) / (k - i)!, one convolution, prime modulus > n.
- `stirling1Column<M>(n, k)`, `stirling2Column<M>(n, k)`: s(i, k) and S(i, k) for i <= n from (log(1 + x))^k / k! and (e^x - 1)^k / k!, as x^k times the k-th power of log(1 + x) / x or (e^x - 1) / x through `fpsPow`. O(n log(n)), prime modulus > n; k = 0 gives [i = 0] and k > n gives zeros.

### StirlingSmallPrime

`StirlingSmallPrime(p)` for a prime p < 2^16 stores s(b, j) for j <= b < p and the coefficients of c_b(x) = x^b prod_(b < i < p) (1 - i x), two tables of p (p + 1) / 2 uint16 entries (50 MB at p = 5000 and 200 MB at p = 10007, so p up to about 10^4 is practical). For any 64-bit n and k (0 for k > n):

- `stirling1(n, k)`: x^(p falling) = x^p - x mod p, so x^(n falling) = (x^p - x)^a x^(b falling) for n = a p + b. Hence s(n, k) is the sum over j <= b with j = k - a (mod p - 1) of s(b, j) C(a, i) (-1)^(a - i), i = (k - a - j) / (p - 1); at most two terms.
- `stirling2(n, k)`: prod_(1 <= i < p) (1 - i x) = 1 - x^(p - 1) mod p. So for k = a p + b, the column generating function is x^(a p) c_b(x) / (1 - x^(p - 1))^(a + 1). S(n, k) is the sum over j in [b, p) with j = n - a p (mod p - 1) of c_b[j] C(a + i, i), i = (n - a p - j) / (p - 1); at most two terms.

Binomials modulo p use Lucas with factorial tables of size p. Q: O(log(n)). Library Checker `stirling_number_of_the_first_kind_small_p_large_n` and `..._second_kind_small_p_large_n` (p fixed per test); maspypy `stirling_number_query` is the same struct.

### bellTable, bellNumberLarge

- `bellTable<M>(n)`: B_0..B_n from exp(e^x - 1), O(n log(n)), prime modulus > n (Library Checker `bell_number`).
- `bellNumberLarge<M>(n)`: B_n mod a prime p = M::mod() < 2^31 for any 64-bit n. Touchard's congruence B_(m + p) = B_m + B_(m + 1) makes x^p - x - 1 annihilate the sequence, so B_n = sum r_i B_i for x^n = sum r_i x^i mod x^p - x - 1. The power is formed left to right: one squaring per bit and a shift for set bits, with reduction in O(p). O(p log(p) log(n)); n < p reads the table.

### eulerianRow, eulerianNumber

A(n, k) counts permutations of n with k descents, A(0, 0) = 1, A(n, k) = 0 for k >= n >= 1. `eulerianRow<M>(n)` evaluates A(n, k) = sum_j (-1)^j C(n + 1, j) (k + 1 - j)^n for all k < n by one convolution with `powerTable`, prime modulus > n + 1. `eulerianNumber<M>(n, k)` uses the same sum for one k, after the symmetry k -> n - 1 - k when that is smaller, for any 64-bit n: C(n + 1, j) runs multiplicatively with inverses of j <= k + 1 (prime modulus > k + 1) and the powers come from the linear sieve. O(k + (k / log(k)) log(n)).

### entringerTable, lahNumber

- `entringerTable<T>(n)`: E(i, j) for j <= i <= n, the down-up permutations of [i + 1] starting with j + 1, by E(i, j) = E(i, j - 1) + E(i - 1, i - j) (seidel–Entringer boustrophedon), any ring, O(n^2); E(n, n) are the zigzag (Euler up/down) numbers.
- `lahNumber(n, k, pc)`: unsigned L(n, k) = C(n - 1, k - 1) n! / k!, L(0, 0) = 1, 0 outside 1 <= k <= n.

### partitionTable, partitionCount, partitionsBoundedParts, partitionsDistinct, enumeratePartitions

- `partitionTable<T>(n)`: p(0..n) by Euler's pentagonal recurrence, additions and subtractions only, any ring, O(n sqrt(n)). A fixed-width signed T must hold partial sums up to about 2 p(n) (`lng` through n = 300 is checked); `ulng` gives p(n) mod 2^64.
- `partitionCount<M>(n)`: the inverse of the pentagonal series prod (1 - x^i), any modint, O(n log(n)) (Library Checker `partition_function`).
- `partitionsBoundedParts<M>(n, k)`: partitions into parts <= k (equivalently at most k parts), exp of sum_(j <= k) sum_m x^(j m) / m, O(n log(n)), prime modulus > n. Partitions into exactly k parts are entry n - k of the table for k.
- `partitionsDistinct<M>(n)`: prod (1 + x^i) = E(x^2) / E(x) for the pentagonal series E, any modint, O(n log(n)); odd-part partitions have the same counts.
- `enumeratePartitions(n, f)`: calls f with a nonincreasing `std::span<const int>` once per partition of n, in reverse lexicographic order from {n} to {1, ..., 1}, by Zoghbi–Stojmenović's ZS1 (constant amortized time per partition); n = 0 gives one empty span. The span is valid during the call only.

### surjectionCount, boundedStarsBars

- `surjectionCount(n, k, pc)`: onto maps from an n-set to a k-set, k! S(n, k) = sum_i (-1)^(k-i) C(k, i) i^n, for any 64-bit n and k <= pc.n, O(k + (k / log(k)) log(n)).
- `boundedStarsBars(n, k, c, pc)`: k-tuples in [0, c] with sum n, sum_j (-1)^j C(k, j) C(n - j (c + 1) + k - 1, k - 1), O(min(k, n / (c + 1))) (OI Wiki); k = 0 gives [n = 0].

### QCombinatorics

`QCombinatorics<M>(q, N)` with a prime modulus > N and any q stores [i]_q! for i < d, where d is the least i <= N with [i]_q = 1 + q + ... + q^(i-1) = 0 (then q has order d; d = N + 1 when no such i exists, including q = 0 and q = 1). `qFactorial(n)` is 0 for n >= d. `qBinomial(a, b)` for 0 <= a <= N is C(a / d, b / d) [a mod d, b mod d]_q by the q-Lucas theorem, with an ordinary factorial table of size N / d, and 0 outside 0 <= b <= a. S: O(N + log(mod)), Q: O(1) (Library Checker `q_binomial_coefficient_prime_mod`; maspypy `q_analogue`). `reset` rebuilds for a new q or N.

### binomialTransform, inverseBinomialTransform, stirlingTransform, inverseStirlingTransform

For a = (a_0..a_n), prime modulus > n:

- `binomialTransform`: b_i = sum_j C(i, j) a_j, the exponential generating function times e^x. `inverseBinomialTransform` multiplies by e^(-x), giving sum_j (-1)^(i-j) C(i, j) b_j. O(n log(n)).
- `stirlingTransform`: b_i = sum_j S(i, j) a_j. With j! d_j = sum_(k >= j) a_k (-1)^(k-j) / (k - j)! (one correlation), b_i = sum_j d_j j^i is the coefficient of x^i in sum_j d_j / (1 - j x), summed by divide and conquer over fractions. O(n log(n)^2).
- `inverseStirlingTransform`: a_i = sum_j s(i, j) b_j. Solving b_i = sum_j d_j j^i: P = B Q mod x^(n + 1) with Q = prod_(j = 1..n) (1 - j x) is the exact numerator, and the residue at x = 1 / j gives d_j = rev(P)(j) (-1)^(n - j) / (j! (n - j)!). rev(P) is evaluated at 1..n by Core Poly `evalMulti` (transposed subproduct tree), d_0 = b_0 - sum d_j, and a_k = sum_(j >= k) j! d_j / (j - k)!. O(n log(n)^2).

### inclusionExclusion

`inclusionExclusion<R>(n, init, extend, f)` returns the sum over subsets S of [0, n) of (-1)^|S| f(fold_S) by depth-first search over increasing index sequences. `extend(acc, i, next)` writes the fold of item i into next and returns false only when f vanishes on every superset; that branch is pruned. Cost O(s c) for s visited subsets and c the cost of extend and f; recursion depth at most n. Example: integers in [1, X] coprime to a list uses lcm folds pruned at lcm > X and f(l) = floor(X / l). Kept beside row 24 `subsetMobius` as a different algorithm (user decision 2026-10-19).

### subsetSumCount

`subsetSumCount<M>(s, t)` returns, for every sum in [0, t], the number of subsets of s with that sum (Library Checker `sharp_p_subset_sum`). Equal values are grouped, log(1 + x^v) = sum_k (-1)^(k+1) x^(v k) / k gives the logarithm in O(t log(t)), then one exp; zeros multiply by 2^z and values above t are ignored. O(t log(t) + |s|), prime modulus > t.

### pisanoPeriod

`pisanoPeriod(m)` for any 64-bit m >= 1 returns the least period of F_n mod m as a `ulll`, since it is at most 6 m < 2^67. For each prime power p^e of m, pi(p) divides p - 1 when p = +-1 mod 5, divides 2 (p + 1) when p = +-2 mod 5, and pi(2) = 3, pi(5) = 20. The candidate is reduced by every prime factor q while candidate / q stays a period (checked by 128-bit fast doubling; the candidate 2 (p + 1) is factored through p + 1, which fits 64 bits). Then pi(p^e) = pi(p) p^j for the least j, because A^L = I mod p^k implies A^(p L) = I mod p^(k + 1). The result is the lcm. O(m^(1/4) log(m)^2) expected.

### zeckendorf, indecomposablePermutations

- `zeckendorf(n)`: the unique representation of a 64-bit n as F_(k_1) + F_(k_2) + ... with k_1 > k_2 + 1 > ... and k_i >= 2 (F_1 = F_2 = 1), greedy from F_93, returned as descending indices; 0 gives none. O(log(n)).
- `indecomposablePermutations<M>(n)`: c_0..c_n with sum c_i x^i = 1 - 1 / sum i! x^i (c_0 = 0), the permutations with no proper prefix mapping onto an initial segment, any modint, O(n log(n)).

### Planned-name mapping

| Planned | Implemented |
|---|---|
| `binomialSmallPrimeOffline` | `BinomialSmallPrimeTable` (user decision 2026-10-19) |
| `binomialPrimePower (Granville)`, `binomialCompositeMod` | `BinomialPrimePower`, `BinomialCompositeMod` (tables built once, queries by `operator()`) |
| `stirling1SmallPrimeLargeN`, `stirling2SmallPrimeLargeN`, `stirlingQueries (offline)` | `StirlingSmallPrime::stirling1`, `stirling2` (user decision 2026-10-19) |
| `qBinomial`, `qFactorial` | `QCombinatorics::qBinomial`, `qFactorial` |
| `increasingSequencesBetweenBounds` | moved to row 67 ([67-lattice_paths.md](67-lattice_paths.md)) |
| `factorialPowerConvert` | row 22 `factorialBasisConvert` (user decision 2026-10-19, [00-notes.md](00-notes.md), P054 research decisions) |

Aliases: Library Checker `binomial_coefficient`, `binomial_coefficient_prime_mod`, `stirling_number_of_the_first_kind`, `..._fixed_k`, `..._small_p_large_n`, `stirling_number_of_the_second_kind` (same three forms), `bell_number`, `partition_function`, `q_binomial_coefficient_prime_mod`, `sharp_p_subset_sum`; maspypy `prefix_sum_of_binom`, `stirling_number_query`, `bell_number_large`, `q_analogue`, `surjection`; suisen `binomial_coefficient_enumeration`, `binomial_coefficient_sum`; Nyaan `stirling_matrix` (transforms), `pascal_matrix` (binomial transform); OEIS A000111 (zigzag), A003319 (indecomposable permutations).

## Feature-to-test map

Entry: [`12-combinatorics_advanced_tester.py`](<../../96-Local Testing/05-Mathematics/12-combinatorics_advanced_tester.py>).

| Feature | Independent evidence |
|---|---|
| `factorialValuation`, `kummerCarries` | Summed v_p(i) for n <= 3000 and seven primes; carries against Legendre for n <= 200, against digit sums for 2000 random 64-bit cases, at p = 2^64 - 1 for the 128-bit sum; Python exact v_p(C(n, k)) for n <= 5000. |
| `binomialLucas`, `BinomialSmallPrimeTable`, `BinomialPrimePower` | Pascal's triangle modulo p for n <= 300 and six primes (k outside [0, n] included), and modulo p^e for eight prime powers up to 2^20; Python `math.comb` for n up to 10^18 with k or n - k <= 30, and all k for n <= 3000. |
| `BinomialCompositeMod`, `binomialRowCompositeMod` | Pascal modulo eleven composites (m = 1 included) up to 2^27 * 3^16 * 7 * 11 * 23, above 2^63; rows against Pascal for 40 moduli up to 2^64 - 1; Python `math.comb` rows (hashed) up to N = 2500 and composite queries up to 10^18 for m up to 2 * 10^5 and smooth m up to 2^64. |
| `binomialPrefixSum`, `binomialPrefixSumQueries`, `BinomialPrefixSumTable` | `binomialPrefixSum` against Pascal sums for weights 1, 2, -1, 0 and random; Mo's queries (weights 1, 5, random and -1, also r = 2 modulo 3) and the block table (weights 1, -1, 0, 7 with B = 1, 3, 17, 1000, default) against it; Python exact sums for n up to 2^64 - 1. |
| `risingFactorial`, `fallingFactorial` | C(x, n) n! modulo 998244353 and products modulo 2^64. |
| `stirling1Row`, `stirling2Row`, `stirling1Column`, `stirling2Column` | The recurrences modulo 998244353, 1000003 and the composite 10^6 (rows; columns for primes), and rows modulo 101 up to n = 260 (n >= p takes the product tree). |
| `StirlingSmallPrime` | DP for n <= 400 and every k <= n + 2 at p = 2, 3, 5, 7, 11, 13, 101; the explicit sum for 64-bit n (also within 3p of 2^64, the review regression) and k < p; s(n, n) = 1 and s(n, n - 1) = -C(n, 2) at 64-bit n. |
| `bellTable`, `bellNumberLarge` | The Bell triangle for n <= 500; `bellNumberLarge` against the triangle modulo six primes for n <= 1500, B_(p^m) = m + 1 mod p, and the period (p^p - 1) / (p - 1) for p <= 5. |
| `eulerianRow`, `eulerianNumber` | The descent recurrence for n <= 500; closed forms A(n, 1) = 2^n - n - 1, A(n, 2) and symmetry at 64-bit n; Python DP for n <= 1500, k <= 30. |
| `entringerTable`, `lahNumber`, `surjectionCount`, `boundedStarsBars` | Alternating permutations enumerated for n <= 8; the Lah recurrence; the surjection recurrence (and Python DP); coefficients of (1 + ... + x^c)^k. |
| `partitionTable`, `partitionCount`, `partitionsBoundedParts`, `partitionsDistinct`, `enumeratePartitions` | Coin-change DP for n <= 2000 (also modulo 2^64), bounded parts for eight bounds; enumeration against a recursive generator in order for n <= 30 and by count at n = 60. |
| `QCombinatorics` | q-Pascal for every q modulo p = 2..17 and 101 (orders d <= N exercise q-Lucas), and q = 0, 1, -1, random modulo 998244353; `reset` on a live object from N = 12 to smaller tables with other q. |
| `binomialTransform`, `inverseBinomialTransform`, `stirlingTransform`, `inverseStirlingTransform` | Direct O(n^2) sums with Pascal and both Stirling tables, and round trips. |
| `inclusionExclusion` | A direct scan of [1, X] for random divisor lists, pruned (lcm > X) and unpruned. |
| `subsetSumCount` | 0/1 knapsack DP, zeros and values above t included. |
| `pisanoPeriod` | Simulation for every m <= 20000; Python certificates (F_pi = 0, F_(pi + 1) = 1, pi / q not a period for every prime factor q of pi, factored by Pollard rho) for products of primes below 2^20 and for primes up to 2^64 times such products, every m below 2^64. |
| `zeckendorf`, `indecomposablePermutations` | Validity and sum for 3000 values (0 to 2^64 - 1); the first-block recurrence for n <= 400 and enumeration for n <= 8. |
| 64-bit moduli | The Stirling tables, `partitionCount`, `partitionsDistinct`, `partitionsBoundedParts`, `indecomposablePermutations`, `bellTable` and `subsetSumCount` modulo the prime 2^64 - 59 and the composite 10^18 against the same DP oracles (n up to 600). |
| Oracle validation | The Python Pisano certificate and factorization are checked by simulation for m < 300 before use. |
| Preconditions | Twenty-one checked-build death probes: p = 1, k > n, a Lucas digit beyond the table, table bounds (small-prime table, prefix sums, queries, block table, q-table, surjections), a prime power beyond 2^27 (struct and a composite with a large prime), a composite prime-power base, m = 0 (struct and row), a negative factorial power, a composite Stirling or Bell modulus, negative partition, enumeration and subset inputs, m = 0 for Pisano. A separate `-DNDEBUG` probe (2026-10-19, `-O2` on g++ and `-O0` on g++-14) confirmed that 22 violating calls across the package terminate, among them p = 0 and 1 in the valuation, carry, Lucas and prime-power loops, negative table sizes, and prime powers beyond the cap (a struct stops its table at 2^27, peak 520 MB; a composite part degenerates to one entry). |

## Commands and results

Run 2026-10-19 on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. The shared runner builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and (full, stress) ASan/UBSan configurations. With `-Wall -Wextra -Wconversion`, the header compiles without warnings on both compilers.

| Command | Result |
|---|---|
| `12-combinatorics_advanced_tester.py --mode full --seed 1` | PASS, 2,838,479 C++ checks and 400 Python rows per configuration, 3 configurations, 21 assertion probes, `MEMORY peak=1131MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=1154MB` |
| `12-combinatorics_advanced_tester.py --mode stress --seed 7` | PASS, 8,514,695 C++ checks (three randomized rounds) and 2000 Python rows per configuration, 21 assertion probes, `MEMORY peak=1172MB` |
| `12-combinatorics_advanced_benchmark.py 5` | PASS; table below, `MEMORY peak=489MB` |
| `02-integration.py` | PASS, 165 standalone and aggregate headers, two translation units, workspace |
| `03-consistency.py` | PASS, no errors |

## Benchmarks

`12-combinatorics_advanced_benchmark.py`, GCC 16.2.1, `-O2` without `-march`, Intel Core i9-11900H, one warmup and the median of 5 repetitions, modulo 998244353 unless stated, seeded inputs at Library Checker sizes, machine shared with other work (load about 2.5); repeated outputs are compared, and the two `bellNumberLarge` kernels must agree.

| Workload | Median |
|---|---|
| `stirling1Row`, n = 5 * 10^5 | 146.1 ms |
| `stirling2Row`, n = 5 * 10^5 | 93.7 ms |
| `stirling1Column`, `stirling2Column`, n = 5 * 10^5, k = 1000 | 501.0 ms, 480.4 ms |
| `bellTable`, n = 5 * 10^5 | 230.8 ms |
| `partitionCount`, n = 5 * 10^5; `partitionTable`, n = 10^5 | 133.9 ms; 38.7 ms |
| `subsetSumCount`, n = t = 5 * 10^5 | 250.3 ms |
| `stirlingTransform`, `inverseStirlingTransform`, n = 10^5, random input | 275.3 ms, 507.8 ms |
| `BinomialCompositeMod`, m = 10^6, 2 * 10^5 queries with n up to 10^18 | 225.3 ms |
| `binomialRowCompositeMod`, N = 10^7, m = 10^18 + 9 | 366.4 ms |
| `binomialPrefixSumQueries`, N = q = 10^5 | 157.0 ms |
| `StirlingSmallPrime`, p = 4999, 2 * 10^5 queries of each kind | 53.6 ms |
| `QCombinatorics`, N = 10^7, 10^6 queries | 168.4 ms |
| `bellNumberLarge`, p = 10007, n = 10^18: shipped left-to-right powering | 166.2 ms |
| the same with right-to-left binary powering | 249.5 ms |

Left-to-right powering replaces the multiplication by x of each set bit with a shift (33% faster here). On random input, Core Poly `evalMulti` evaluates the 10^5-point step of `inverseStirlingTransform` in about 335 ms against 465 ms for the former private remainder tree (probe on the same machine).

## Sources

Fetched 2026-10-19 by `@researcher` and the package session ([00-sources.md](00-sources.md), P054 sweep). The algorithms were implemented from the stated identities, and no external code was copied.

- Library Checker statements for the binomial, Stirling (signed first kind), Bell, partition, q-binomial and sharp-P subset-sum problems.
- cp-algorithms Binomial coefficients (Granville's prime-power method) and OI Wiki partition, Stirling and bounded stars-and-bars pages.
- maspypy `q_analogue`, `prefix_sum_of_binom`, `stirling_number_query`, `bell_number_large`; suisen `binomial_coefficitnt_enumeration`, `binomial_coefficient_sum`; Nyaan `fps-famous-series`, `stirling-matrix`.
- Zoghbi and Stojmenović, Fast algorithms for generating integer partitions (ZS1).

## Limits and handoffs

- n! mod p and C(n, k) mod p for n beyond any table (sample-point shift) are row 37; the factorial-basis conversion is row 22.
- Scope decisions (left-out catalog candidates, each with its reason) are in [00-notes.md](00-notes.md), P054 research decisions.
- No `97-Legacy` excerpt existed for this row.

## History

- 2026-10-19: P054 initial implementation; `@reviewer` found 1 bug in this header (`stirling2` start index wrapping near n = 2^64), unbounded `-DNDEBUG` loops for p <= 1 and Pisano beyond its domain, two cost statements and a memory figure, all fixed with regressions; full suite passed on g++ and g++-14, stress seed 7 passed.
- 2026-10-19: P054 follow-up: prime powers capped at 2^27, `BinomialCompositeMod`, `binomialRowCompositeMod` and `pisanoPeriod` widened to every 64-bit modulus, r = -1 in Mo's queries, a weighted block table, product-tree `stirling1Row` for other moduli, Core Poly `evalMulti` replacing the private remainder tree, 64-bit moduli through row 13's generalized convolution; full and stress passed on g++, full on g++-14.
