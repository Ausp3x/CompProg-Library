# 20-continued_fraction.hpp — evidence

`20-continued_fraction.hpp` covers:

- continued fractions of rationals and quadratic irrationals, with convergents and semiconvergents;
- one- and two-sided rational approximation, and binary search over fractions;
- the Stern–Brocot tree;
- Farey sequences, including the k-th term and rational counting.

It depends on `01-mod_arithmetic.hpp` (`floorDiv`, `modNorm`, `extendedGcd`), on `04-sieve_algorithms.hpp` (`LinearSieve::getMu`) and on `18-floorsum.hpp` (`floorSum`, `forEachQuotientBlock`). Fractions are `pair<lng, lng>` `{p, q}`. `1/0` stands for `+infinity` where stated.

## Contracts

### continuedFraction, convergents, semiconvergents

`continuedFraction(p, q)` with `q >= 1` (asserted) and any `lng` `p` returns `[a0; a1, ..., ak]` with:

- `a0 = floor(p / q)`;
- `a_i >= 1` for `i >= 1`;
- `ak >= 2` when `k >= 1`, the canonical form.

Every term fits `lng` because the Euclidean remainders start below `q`. The cost is `O(log(q))`.

`convergents(a)` returns `p_i / q_i` of `[a0; ...; ai]` for every prefix. `semiconvergents(a)` returns `p_0 / q_0` followed by `(t p_{i-1} + p_{i-2}) / (t q_{i-1} + q_{i-2})` for `i >= 1` and `t = 1..a_i`, so each level ends with `p_i / q_i`. The list is sorted by denominator, and its size, `1 + sum of a_i` for `i >= 1`, is the caller's explicit bound.

Both require `a_i >= 1` for `i >= 1` and every result fitting `lng`, checked per term in 128-bit arithmetic. Consecutive convergents satisfy `p_i q_{i-1} - p_{i-1} q_i = (-1)^(i-1)`.

### PeriodicContinuedFraction, continuedFractionQuadratic, continuedFractionSqrt

`continuedFractionQuadratic(P, D, Q)` with `Q != 0`, `0 <= D < 2^62` and `|P|, |Q| < 2^31` (asserted) returns `{pre, period}` with `(P + sqrt(D)) / Q = [pre; period, period, ...]`. Both parts are minimal: `pre` is empty or differs from `period` in its last term, and `period` is primitive. A square `D` gives the finite expansion of the rational value in `pre` and an empty `period`. `continuedFractionSqrt(D)` is `continuedFractionQuadratic(0, D, 1)`, returning `pre = {floor(sqrt(D))}` and the classical period ending in `2 a0`.

Algorithm:

1. **Normalize.** If `Q` does not divide `D - P^2`, multiply through by `|Q|`: `(P |Q| + sqrt(D Q^2)) / (Q |Q|)`. Every later state `(p, q)` keeps `q | d - p^2`. The state then fits 128 bits with `d < 2^124`.
2. **Iterate.** With `r = isqrt(d)`, each term is `a = floor((p + r + [q < 0]) / q)`, and the next state is `p' = a q - p`, `q' = (d - p'^2) / q`. This floor is exact: `(p + sqrt(d)) / q` lies strictly between `(p + r) / q` and `(p + r + 1) / q`, and no integer lies strictly between them.
3. **Detect the period.** The expansion is purely periodic from its first reduced state, where `1 < (p + sqrt(d)) / q` and `-1 < (p - sqrt(d)) / q < 0`. In integers that is `q > 0`, `p <= r`, `p + q > r` and `q <= p + r`. Every state of the cycle is reduced, so the first reduced state starts the minimal period. The loop stops when that state recurs, and terms and states correspond one to one.

Cost: the preperiod is the shared prefix of the expansions of the number and its conjugate, plus at most two terms, so `O(log(|P| + |Q| + 2))`. The period is `O(sqrt(D Q^2) log(D Q^2))` in the worst case, and the output is the caller's bound.

### bestRationalApproximation, rationalInInterval

`bestRationalApproximation(x, y, n)` with `x >= 0`, `y >= 1` and `n >= 1` returns `{lo, hi}`:

- `lo`: the largest reduced `p/q <= x/y` with `0 <= p <= n` and `1 <= q <= n` (`0/1` always qualifies);
- `hi`: the smallest reduced `r/s >= x/y` with `r, s <= n`, or `1/0` when `x/y > n`.

This bounds numerator and denominator, as in Library Checker `rational_approximation`. When reduced `x/y` itself qualifies, both are `x/y`. For a denominator-only bound, apply it to `(x mod y) / y` and add `floor(x / y)`.

Algorithm: a Stern–Brocot descent with `lo < x/y < hi`. Each run moves one bound by the full partial quotient or, if smaller, by the largest step keeping both coordinates within `n`. A capped run leaves the mediant `lo + hi` out of bounds. Every fraction strictly between Stern–Brocot neighbours has numerator and denominator at least the mediant's, so `lo` and `hi` are the answer. `O(log(y))` runs.

`rationalInInterval(a, b, c, d)` with `0 <= a/b < c/d`, `b >= 1` and `d >= 0` (`c/0`, `c >= 1`, is infinity) returns the fraction strictly between them with the least denominator, then the least numerator. It is unique and is the shallowest Stern–Brocot node in the interval. The precondition that `a + c` and `b + d` fit `lng` bounds every intermediate value. Alternating maximal runs keep `lo <= a/b` and `hi >= c/d` until the mediant falls inside, in `O(log(b + d))`. For a closed interval, compare the endpoints' reduced forms with this result. For negative intervals, mirror.

### fractionBinarySearch

`fractionBinarySearch(n, f)` with `n >= 1` takes `f(p, q)`, monotone over positive fractions (false, then true). It returns the adjacent pair `{lo, hi}` of the set `{0/1, 1/0} ∪ {p/q : 1 <= p, q <= n}` with `f(lo)` false or `lo = 0/1`, and `f(hi)` true or `hi = 1/0`. Both are reduced, and `f` is called only on members of the set.

Alternating runs of the Stern–Brocot descent each find their length by galloping and then bisecting below a cap that keeps both coordinates within `n`. The search ends when the mediant leaves the bounds. The galloping steps sum to `O(log(n))` calls, because the run lengths multiply to at most `n` and the runs are `O(log(n))` by Fibonacci growth. Steps and mediants are 128-bit, so `n` up to `2^63 - 1` is safe.

### sternBrocotEncode, sternBrocotDecode, sternBrocotLca, sternBrocotAncestor, sternBrocotRange, sternBrocotChildren

Nodes are reduced `p/q` with `p, q >= 1`; `1/1` is the root at depth 0. Each operation runs in `O(log(max(p, q)))`, except `Decode`, which is `O(runs)`.

- **`sternBrocotEncode(p, q)`** returns the path from the root as alternating runs `{'L' or 'R', count >= 1}`. It is the continued fraction `[a0; ..., ak]` read as `R^a0 L^a1 ...` with the last count reduced by one and zero runs dropped.
- **`sternBrocotDecode(path)`** accepts counts `>= 0` in any order and returns the node reached. The precondition is that the node fits `lng`, which bounds every intermediate.
- **`sternBrocotLca(p, q, r, s)`** decodes the common prefix of the two paths.
- **`sternBrocotAncestor(p, q, k)`** returns the depth-`k` ancestor, or `{-1, -1}` when `k` exceeds the depth (the sum of the counts).
- **`sternBrocotRange(p, q)`** returns the open interval `{lo, hi}` of the subtree, with `0/1` and `1/0` as the outer ends; `p/q` is their mediant.
- **`sternBrocotChildren(p, q)`** returns `{lo + p/q, p/q + hi}` as mediants; the children must fit `lng`.

These answer Library Checker `stern_brocot_tree`: `ENCODE_PATH`, `DECODE_PATH`, `LCA`, `ANCESTOR` (`-1`) and `RANGE`.

### fareySequence, fareyNext, fareyKth, countRationalsInRange

- **`fareySequence(n)`** (`n >= 1`) returns `F_n` from `0/1` to `1/1` by the neighbour recurrence `k = floor((n + b) / d)`, `(c, d) -> (k c - a, k d - b)`. The output has about `0.304 n^2` terms. Elements are `int` pairs to halve memory, and the arithmetic is `lng`.
- **`fareyNext(p, q, n)`** with reduced `0 <= p < q <= n` returns the successor `r/s`. It is the unique fraction with `r q - p s = 1` and `n - q < s <= n`: `s = -p^-1 mod q`, lifted to `(n - q, n]`, in `O(log(q))`.
- **`fareyKth(n, k)`** returns the zero-based k-th term of `F_n` for `0 <= k < |F_n|`.
- **`countRationalsInRange(n, a, b, c, d)`** counts the reduced `r/s` with `1 <= s <= n` and `a/b <= r/s < c/d`, for `0 <= a/b <= c/d` with `b, d >= 1`. The bound `c/d` may exceed 1. The count is computed modulo 2^128 and is exact when it fits `lll`.
- `fareyRank(p/q)` is `countRationalsInRange(n, 0, 1, p, q)`.
- `fareyKth` and `countRationalsInRange` require `n <= 10^13` (asserted), which keeps the `mu` sieve below 2^31 entries.

Algorithm: let `L(v)` count the pairs `(r, s)` with `1 <= s <= v`, `r >= 0` and `r/s < x/y` (or `<=`). It is `floorSum(v, y, x, x + y - 1)` (or `floorSum(v, y, x, x + y)`). Grouping pairs by `gcd(r, s)` gives `L(n) = sum over g of R(floor(n / g))`, so the reduced count is `sum over g of mu(g) L(floor(n / g))`. That is `O(sqrt(n))` quotient blocks, each with a Mertens difference and one floor sum.

`continued_fraction_detail::Mertens` sieves `mu` with `LinearSieve` up to `n^(2/3)`. It gets the larger quotient values from `M(v) = 1 - sum over g >= 2 of M(floor(v / g))`, in increasing order, in `O(n^(2/3))` time and memory. `fareyKth` builds it once and runs `fractionBinarySearch` with the predicate `#{terms <= p/q} > k`, using `O(log(n))` counts.

### Legacy mapping

No archived implementation existed. Before implementation, the planned row's `SternBrocot: encode, decode, lca, ancestor, child, range` became the free functions `sternBrocotEncode` … `sternBrocotChildren`, because `03-cpp.md` keeps algorithm headers free of holder structs. `child` became `sternBrocotChildren`, which returns both children, since a direction flag would choose between operations.

Aliases: KACTL `fracBS`, maspypy `rational_binary_search`, Nyaan `binary_search_on_stern_brocot_tree` (`fractionBinarySearch`); cp-algorithms `middle` (`rationalInInterval`); Nyaan `go_parent`, suisen `move_up` (`sternBrocotAncestor`); Library Checker `rational_approximation` (`bestRationalApproximation`); maspypy `range_rational_count` count and kth (`countRationalsInRange`, `fareyKth`); the rank in `F_n` (`countRationalsInRange(n, 0, 1, p, q)`).

## Feature-to-test map

Entry: [`20-continued_fraction_tester.py`](<../../96-Local Testing/05-Mathematics/20-continued_fraction_tester.py>).

| Feature | Independent evidence |
|---|---|
| `continuedFraction`, `convergents` | Every `p/q` with `p` in `[-60, 60]` and `1 <= q <= 60`, signed64 extremes and seeded full-width values. Canonical form, last convergent equal to reduced `p/q`, each convergent equal to its prefix evaluated backwards, and the determinant identity. Python Euclid and `Fraction` evaluation on 3000 rows. |
| `semiconvergents` | The definition, checked on every expansion above and on seeded expansions, sorted by denominator. The Python definition from `Fraction` convergents. |
| `continuedFractionSqrt`, `continuedFractionQuadratic`, `PeriodicContinuedFraction` | For `D <= 3000`: `pre = {a0}`, a period ending in `2 a0` with a palindromic body, and squares with no period. Seeded `P`, `Q` in `[-30, 30]` and extremes up to `P`, `Q` of magnitude `2^31 - 1`, `D` near 2^62: minimal preperiod and primitive period. Python expands `(x + y sqrt(D)) / z` exactly with integer square roots and lowest terms, a representation independent of the `(p, q)` recurrence, and matches `pre` followed by two periods. Rational cases are checked against Euclid. |
| `bestRationalApproximation` | Brute force over every bounded fraction for `x, y <= 30` and `n <= 12`. A complete certificate for 62-bit inputs in C++ and Python: bracket, bounds, reduction, Stern–Brocot adjacency `hi.p lo.q - lo.p hi.q = 1`, and a mediant out of bounds, or both equal to a qualifying `x/y`. |
| `rationalInInterval` | Brute force by increasing denominator for every term at most 12, including `c/0`. 62-bit certificates: the result lies in the interval, and the interval lies in the result's subtree, with the subtree computed by `sternBrocotRange` in C++ and by convergent formulas in Python. Python brute force when `b + d <= 400`. |
| `fractionBinarySearch` | Brute force over every bounded fraction for threshold predicates. 62-bit certificates with at most `6 bit_width(n) + 8` predicate calls, and the irrational threshold `p^2 >= 2 q^2`. |
| `sternBrocotEncode`, `sternBrocotDecode`, `sternBrocotRange`, `sternBrocotChildren`, `sternBrocotAncestor`, `sternBrocotLca` | A BFS of the tree to depth 11 for encode, decode, range, children, every ancestor depth and LCA. 62-bit roundtrips, depth through `sternBrocotAncestor`, the node as the mediant of its range, ancestor ranges containing the node, and the LCA containing both nodes, which leave it on different sides. Python paths from continued fractions, ranges from convergent formulas, and LCA and ancestors by walking. |
| `fareySequence`, `fareyNext`, `fareyKth`, `countRationalsInRange` | Enumeration for every `n <= 60`: every term, every successor, every k-th term, and counts between sampled terms and below `x/y` up to 20/1. Counts against an `O(n log(n))` divisor-sieve recurrence that does not use Mertens, up to `n = 200000`. k-th terms with exactly `k` smaller terms, and successor certificates. At `n = 10^9` and `999999937`, ranks and successors agree. Python enumeration with `Fraction` for `n <= 120`. |
| Preconditions | Seventeen checked-build death probes: `q = 0`; quadratic `Q = 0` and `D < 0`; a zero term and an overflowing convergent; a negative semiconvergent term; `y = 0`; a reversed interval; `n = 0` search; a non-coprime node; a bad path character; `k < 0`; a zero node; `fareySequence(0)`; `fareyNext` with `p = q`; `k` past the last term; a reversed count range. A separate `-DNDEBUG` probe (2026-10-18, `-O0` and `-O2`) confirmed that 36 violating calls, including nonpositive nodes, negative `D`, reversed intervals and `n <= 0` searches, terminate. |

## Commands and results

Full runs 2026-10-19 (after the row 18 change, recorded in [18-floorsum.md](18-floorsum.md)), stress 2026-10-18; conditions as in [18-floorsum.md](18-floorsum.md).

| Command | Result |
|---|---|
| `20-continued_fraction_tester.py --mode full --seed 1` | PASS, 3,805,206 C++ checks and 3000 Python rows per configuration, 17 assertion probes, `MEMORY peak=818MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=817MB` |
| `20-continued_fraction_tester.py --mode stress --seed 7` | PASS, 25,257,307 C++ checks and 20,000 Python rows per configuration, 17 assertion probes, `MEMORY peak=1063MB` |

## Benchmarks

From `18-floorsum_benchmark.py` (2026-10-18: GCC 16.2.1, `-O2` without `-march`, i9-11900H, one warmup, median of 5), `k = n / 3`, with the rank of the result verified:

| `n` | `fareyKth` | `countRationalsInRange(n, 0, 1, p, q)` |
|---|---|---|
| 10^6 | 1.3 ms | 0.4 ms |
| 10^9 | 86.2 ms | 40.9 ms |

## Sources

Fetched 2026-10-18 by `@researcher` ([00-sources.md](00-sources.md), P060 sweep). Implemented from the stated identities, with no external code copied.

- Library Checker `stern_brocot_tree` and `rational_approximation`: query set, `-1` convention, open ranges, and bounds on both numerator and denominator.
- cp-algorithms "Continued fractions" and "Stern–Brocot tree and Farey sequences": quadratic irrationals, convergents and semiconvergents, `middle`, and Farey neighbours.
- KACTL `FracBinarySearch.h` and `ContinuedFractions.h`, maspypy `nt/` and Nyaan `math/`: the binary-search contract and the Farey k-th term.
- OI Wiki 连分数 and Stern–Brocot 树与 Farey 序列: Farey rank and the multi-dimensional quotient sums behind the count.

## Limits and handoffs

- Memory for the Farey count is about 9 n^(2/3) bytes (the sieve's smallest-prime-factor and `mu` tables plus the Mertens prefix), so `n = 10^9` uses about 9 MB and `n = 10^13` about 4 GB.
- `continued_fraction_detail::Mertens` is local. Row 31 `mobiusSum` (planned) would own Mertens values on quotient blocks; when it is verified, `fareyKth` and `countRationalsInRange` can call it.
- `continued_fraction_detail::isqrt` (128-bit floor square root) duplicates the planned row 21 `isqrt`; call row 21 once it is verified, as [34-quadratic_congruence.md](34-quadratic_congruence.md) records for its own helper.
- Period lengths of quadratic irrationals grow like `sqrt(D Q^2)`. The output size is the caller's bound, as in row 34 `pellFundamental`.
- Pell equations (row 34), rational reconstruction (row 43) and convex lattice hulls (row 55) stay in their rows.

## History

- 2026-10-18: P060 initial implementation; `@reviewer` found 8 issues, all fixed (precondition checks, `-DNDEBUG` termination, complexity statements, the `n <= 10^13` bound); full suite passed on g++ and g++-14, stress seed 7 passed.
- 2026-10-19: P037 round-3 rerun (included `18-floorsum.hpp` changed), header unchanged; full passed on g++ and g++-14.
