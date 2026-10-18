# 67-lattice_paths.hpp — evidence

`67-lattice_paths.hpp` counts monotone lattice paths: free and obstructed grid paths, reflection counts below diagonals, between parallel lines and below integer-slope lines, ballot and Bizley counts, paths between monotone boundaries by divide and conquer, the named path numbers (Catalan convolution and triangle, Narayana, Motzkin, Schroeder, Delannoy) and the Lindström–Gessel–Viennot determinant. It depends on `05-combinatorics.hpp` (`PrimeCombinatorics`), `09-modinverse.hpp` (`inverseTable`), `13-convolution.hpp` (`convolution`), `14-polynomial_algorithms.hpp` (`fpsExp`) and `01-mod_arithmetic.hpp` (`floorDiv`). Graphs row 53 owns path families on general DAGs.

## Contracts

Common domain: paths use unit steps right (E, x + 1) and up (N, y + 1), plus diagonal steps (1, 1) where named. Counts are residues of `M`; functions that take `const PrimeCombinatorics<M> &pc` need a prime modulus and assert that pc covers every factorial index they read (`pc.n` at least the path length). Impossible endpoints give 0, never an assertion.

### O(1) counts

- `gridPaths(dx, dy, pc)`: C(dx + dy, dx) for dx, dy >= 0, else 0 (kept beside `combiNR` because it accepts unreachable offsets).
- `dyckPaths(n, h1, h2, pc)`: walks of n steps +-1 from height h1 >= 0 to h2 >= 0 that never go below 0, by reflection about -1: C(n, u) - C(n, u + h1 + 1) with u = (n + h2 - h1) / 2 up steps; 0 on a parity mismatch. General endpoints by user decision (duplicates ledger); `catalan(n)` is `dyckPaths(2n, 0, 0)`.
- `pathsBelowDiagonal(x1, y1, x2, y2, pc)`: E/N paths from (x1, y1) to (x2, y2) with y <= x at every point, by reflecting the start across y = x + 1 to (y1 - 1, x1 + 1); 0 when an endpoint lies above the diagonal. Row 05 `ballot(a, b)` is the origin case.
- `ballotGeneral(c, d, mu, strict, pc)`: words of c A's and d B's where every prefix has #A >= mu #B (weak) or every nonempty prefix #A > mu #B (strict), for any mu >= 0: weak C(c + d, d) - mu C(c + d, d - 1) = (c - mu d + 1) / (c + 1) C(c + d, d) when c >= mu d; strict is the weak count to (c - 1, d) (the first letter is A), (c - mu d) / (c + d) C(c + d, d). The empty word counts 1. This is the cycle-lemma generalized ballot (Krattenthaler Theorem 10.4.5); mu = 1 is row 05 `ballot`, and (mu n, n) gives the Fuss–Catalan numbers.
- `catalanConvolution(n, k, pc)`: [x^n] C(x)^k = k / (2n + k) C(2n + k, n) = C(2n + k - 1, n) - C(2n + k - 1, n - 1) for k >= 1, [n = 0] for k = 0. It equals the weak ballot to (n + k - 1, n), and Dyck paths of semilength n touching the axis exactly t times are `catalanConvolution(n - t, t)`.
- `narayana(n, k, pc)`: Dyck paths of semilength n with k peaks, (1 / n) C(n, k) C(n, k - 1) computed division-free as C(n - 1, k - 1)^2 - C(n - 1, k) C(n - 1, k - 2); N(0, 0) = 1, 0 outside 1 <= k <= n.

### Linear-time counts

- `gridPathsAvoidingRectangle(x, y, x1, y1, x2, y2, pc)`: paths (0, 0) -> (x, y) avoiding every lattice point of [x1, x2) x [y1, y2). The rectangle is clipped to the box; every path that meets it has a unique first point inside, entered from below at (i, y1) or from the left at (x1, j), and the prefix before it never meets the rectangle by monotonicity. So the count is the total minus O(x2 - x1 + y2 - y1) products of two binomials; a start inside gives 0.
- `pathsBelowLine(a, b, c, d, mu, pc)`: paths (a, b) -> (c, d) with x >= mu y at every point (weakly below y = x / mu), integer mu >= 0, b >= 0. With h = x - mu y, E steps add 1 and N steps subtract mu. A bad path ends at h >= 0, so it visits h = -1 for a last time at Q = (mu y - 1, y), then steps E onto the line and stays good. The count is the total minus the sum over y of free(start -> Q) times the weak ballot from (mu y, y) to (c, d), O(d - b) terms (Korolyuk's formula, Krattenthaler Theorem 10.4.7). Lines with an offset translate the endpoints. Paths below a steeper line y <= mu x map to this case by reversing the path and swapping the axes.
- `pathsBetweenLines(x1, y1, x2, y2, lo, hi, pc)`: paths with lo <= y - x <= hi at every point, by alternating reflections across y - x = lo - 1 and hi + 1 (period w = 2 (hi - lo + 2)), sum over k of N(t + k w) - N(2 (hi + 1) - t + k w) for the free walk counts N; only targets within distance n of the start contribute, O(1 + n / w) terms. Krattenthaler Theorem 10.3.3.
- `motzkinPaths(n, h1, h2, pc)`: walks of n steps in {-1, 0, +1} from h1 to h2 that stay >= 0, T(n, h2 - h1) - T(n, h1 + h2 + 2) with the trinomial counts T(n, j) = sum_u n! / (u! (u - j)! (n - 2u + j)!), O(n). Reflection applies because the steps are skip-free. The Motzkin number M_n is `motzkinPaths(n, 0, 0)`.
- `delannoy(m, n, pc)`: E, N and diagonal paths (0, 0) -> (m, n), sum_k C(m, k) C(n, k) 2^k, O(min(m, n)).

### gridPathsAvoidingPoints

`gridPathsAvoidingPoints(x, y, pts, pc)` counts paths (0, 0) -> (x, y) through no point of pts. Points outside the box are ignored, duplicates merged, and the target appended. In sorted order, dp[i] = paths(0 -> p_i) minus the sum over j < i of dp[j] paths(p_j -> p_i), the paths whose first obstacle is p_i. An obstacle at the origin or the target gives 0. O(k^2 + k log(k)).

### gridPathsBoundaryTransfer

`gridPathsBoundaryTransfer(left, bottom, pc)` takes flows into an all-open h x w rectangle (h = |left| rows entered from the left, w = |bottom| columns entered from below) and returns {right, top}: right[j] is the number of weighted paths reaching (w - 1, j) and top[i] those reaching (i, h - 1), the flows leaving through each side. The four transfers are binomial kernels: left to right C(w - 1 + t, t) and bottom to top C(h - 1 + t, t) are convolutions; the corner transfers C(i + j, i) = (i + j)! / (i! j!) become convolutions against t! after scaling by inverse factorials. O((h + w) log(h + w)), pc.n >= h + w. An empty side passes the other flows through unchanged.

### increasingSequencesBetweenBounds

`increasingSequencesBetweenBounds<M>(a, b)` counts nondecreasing x_0 <= ... <= x_(n-1) with a_i <= x_i < b_i (a_i >= 0, any order of the bounds; Library Checker `number_of_increasing_sequences_between_two_sequences` without its redundant M). The bounds are first made monotone (prefix maxima of a, suffix minima of b, since x is nondecreasing); a crossing gives 0 and n = 0 gives 1. The sequence is the lattice path whose i-th E step is at height x_i, so column x holds the points [a_(x-1), b_x - 1] and consecutive columns overlap. The count is the flow from (0, a_0) to (n, b_(n-1) - 1) inside this staircase region. It is computed by recursive halving of the longer side: an all-open rectangle uses `gridPathsBoundaryTransfer`, a disjoint one returns zero flows (with overlapping columns, a disjoint rectangle lies wholly above or wholly below the region), and the rest split. A monotone boundary meets O(2^(d/2)) rectangles at depth d, so the work is O((n + h) log(n + h)^2) with h = max b - a_0, and a prime modulus > n + h + 1 covers the factorials. Rational-line boundaries such as y <= floor((p x + q) / r) are this function on their height sequence.

### bizleyPaths

`bizleyPaths<M>(a, b, n)` returns, for i in [0, n], the paths (0, 0) -> (a i, b i) weakly below the line through them (by symmetry, also weakly above). With g = gcd(a, b), Bizley's formula gives sum_j phi_j t^j = exp(sum_(j >= 1) C(s j, a' j) / (s j) t^j) for a' = a / g and s = (a + b) / g, and phi at (a i, b i) is the coefficient at g i. O((a + b) n + g n log(g n)), prime modulus > (a + b) n. Paths that touch the line only at their endpoints are 1 - 1 / F of this series.

### nonIntersectingPathsDeterminant

`nonIntersectingPathsDeterminant(s, t, pc)` returns det[gridPaths(s_i -> t_j)] over a prime field by Gaussian elimination, O(k^3). By the Lindström–Gessel–Viennot lemma, this is the signed count over all endpoint permutations of vertex-disjoint path systems. It equals the number of disjoint systems when only the identity matching admits any, for example when starts and ends lie on two antichains in compatible order.

### Tables

- `catalanTriangle<T>(n)`: rows i <= n of the ballot numbers C(i, j) = C(i, j - 1) + C(i - 1, j), j <= i, in any ring T (additions only), O(n^2).
- `motzkinTable<M>(n)`: (i + 2) M_i = (2i + 1) M_(i-1) + 3 (i - 1) M_(i-2), prime modulus > n + 2, O(n).
- `schroderLarge<M>(n)`, `schroderSmall<M>(n)`: (i + 1) x_i = 3 (2i - 1) x_(i-1) - (i - 2) x_(i-2) with x_1 = 2 (large: E, N, diagonal paths below the diagonal) or x_1 = 1 (small: s_n = r_n / 2 for n >= 1, the large paths with no diagonal step on the diagonal), prime modulus > n + 1, O(n).
- `delannoyCentral<M>(n)`: i D_i = 3 (2i - 1) D_(i-1) - (i - 1) D_(i-2), prime modulus > n, O(n).

### Planned-name mapping

| Planned | Implemented |
|---|---|
| `pathsBelowLine (floor-sum based)` | `pathsBelowLine` (integer slope, Korolyuk); rational boundaries through `increasingSequencesBetweenBounds`; no floor-sum method counts paths |
| `ballotGeneral (reflection with offset)` | `ballotGeneral` (slope mu, weak or strict); offsets translate the endpoints of `pathsBelowLine` |
| `motzkin (table and nth)` | `motzkinTable`, `motzkinPaths(n, 0, 0)` |
| `increasingSequencesBetweenBounds` (row 12) | moved here beside the rectangle transfer |

Aliases: Library Checker `number_of_increasing_sequences_between_two_sequences`; maspypy `count_grid_path`, `catalan_convolution`; hitonanode `NorthEastLatticePathsInRectangle`, `NorthEastLatticePathsBounded`; Fuss–Catalan and k-ary Dyck paths (`ballotGeneral`); rational Catalan numbers (`bizleyPaths`).

## Feature-to-test map

Entry: [`67-lattice_paths_tester.py`](<../../96-Local Testing/05-Mathematics/67-lattice_paths_tester.py>).

| Feature | Independent evidence |
|---|---|
| `gridPaths`, `dyckPaths`, `motzkinPaths` | Every offset in [-2, 10]^2; n <= 20 with heights <= 10 against walk DP; Python walk DP for n <= 120. |
| `pathsBelowDiagonal`, `pathsBetweenLines` | All endpoints in [0, 7]^2 (quick 5) and every strip with -3 <= lo <= hi <= 3 against filtered grid DP; Python grid DP on strips up to 60 x 60 with widths up to 12. |
| `pathsBelowLine`, `ballotGeneral` | mu <= 3, starts and ends up to (14, 4) against grid DP restricted to x >= mu y; weak and strict ballots against prefix-filtered DP; Python exact fractions (c - mu d + 1) / (c + 1) C(c + d, d) up to 3000 x 2000 and Python grid DP for lines. |
| `catalanConvolution`, `catalanTriangle`, `narayana` | Powers of the Catalan series built by walk DP for k <= 6; the triangle against ballot DP (also in ulng modulo 2^64); Narayana against Dyck words enumerated by peak count for n <= 11; Python k / (2n + k) C(2n + k, n) and (1 / n) C(n, k) C(n, k - 1) up to n = 3000. |
| `motzkinTable`, `schroderLarge`, `schroderSmall`, `delannoyCentral`, `delannoy` | Walk and E/N/diagonal grid DP for n <= 30 (small Schroeder as large paths without diagonal steps on the diagonal), also modulo 1000003 for n <= 80; Python closed-form sums up to n = 2000. |
| `bizleyPaths` | Grid DP below a y <= b x for a, b <= 4 and i <= 6; Python exact rational recursion of Bizley's exponential, validated by grid DP, for a, b <= 6 and n <= 25. |
| `gridPathsAvoidingPoints`, `gridPathsAvoidingRectangle` | 2000 random cases (obstacles outside the box, duplicates, at the origin and the target; rectangles negative, clipped, empty) against blocked-grid DP up to 14 x 14. |
| `gridPathsBoundaryTransfer` | Random flows on rectangles up to 14 x 14 against the cell DP, and empty sides passing through. |
| `increasingSequencesBetweenBounds` | 3000 random bound pairs (arbitrary and banded, crossings, n = 0) with n, m <= 12, and 12 cases up to 1500 x 1500, against an O(n m) prefix-sum DP, also modulo 1000003. |
| `nonIntersectingPathsDeterminant` | 150 random systems with k <= 3 on 4 x 4 grids against the signed enumeration of vertex-disjoint path systems over every endpoint permutation. |
| Oracle validation | The Python closed forms are checked against grid DP and known sequence prefixes before use. |
| Preconditions | Thirteen checked-build death probes: table bounds (gridPaths, catalanConvolution, motzkinPaths, the transfer), negative arguments (dyckPaths, ballotGeneral, the sequence bounds, motzkinTable), mu < 0, lo > hi, size mismatches (bounds, LGV), a zero Bizley slope. |

## Commands and results

Run 2026-10-19 on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. The shared runner builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and (full, stress) ASan/UBSan configurations. With `-Wall -Wextra -Wconversion`, the header compiles without warnings on both compilers.

| Command | Result |
|---|---|
| `67-lattice_paths_tester.py --mode full --seed 1` | PASS, 130,150 C++ checks and 400 Python rows per configuration, 3 configurations, 13 assertion probes, `MEMORY peak=558MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=573MB` |
| `67-lattice_paths_tester.py --mode stress --seed 7` | PASS, 218,410 C++ checks and 2000 Python rows per configuration, 13 assertion probes, `MEMORY peak=756MB` |
| `67-lattice_paths_benchmark.py 5` | PASS; table below, `MEMORY peak=356MB` |
| `02-integration.py` | PASS, 165 standalone and aggregate headers, two translation units, workspace |
| `03-consistency.py` | PASS, no errors |

## Benchmarks

`67-lattice_paths_benchmark.py`, GCC 16.2.1, `-O2` without `-march`, Intel Core i9-11900H, one warmup and the median of 5 repetitions, modulo 998244353, seeded inputs; repeated outputs are compared. Library Checker `number_of_increasing_sequences_between_two_sequences` has N, M <= 131072.

| Workload | Median |
|---|---|
| `increasingSequencesBetweenBounds`, N = M = 131072, band of width 4000 around the diagonal | 864.9 ms |
| `increasingSequencesBetweenBounds`, N = M = 131072, random bounds | 148.2 ms |
| `gridPathsBoundaryTransfer`, h = w = 131072 | 92.7 ms |
| `bizleyPaths`, a = 3, b = 5, n = 10^5 | 56.8 ms |
| `gridPathsAvoidingPoints`, 3000 obstacles in a 10^5 x 10^5 box | 22.7 ms |
| `motzkinTable`, `schroderLarge`, `delannoyCentral`, n = 10^6 each | 33.7 ms |

A banded region keeps the staircase boundaries long, which is the worst case of the divide and conquer; random bounds collapse after monotonization.

## Sources

Fetched 2026-10-19 by `@researcher` and the package session ([00-sources.md](00-sources.md), P054 sweep). The algorithms were implemented from the stated identities, and no external code was copied.

- Krattenthaler, Lattice path enumeration, pp. 591–610: strips (10.3.3), the slope-mu ballot (10.4.5), Korolyuk's formula (10.4.7) and rational lines (10.4.1).
- Bizley's formula as stated in arXiv 1606.08412, Theorem 2.1.
- Library Checker `number_of_increasing_sequences_between_two_sequences`: arbitrary bounds, x_i < b_i.
- hitonanode `north_east_lattice_paths.hpp` and maspypy `count_grid_path.hpp`: the rectangle transfer and the divide and conquer contracts.
- cp-algorithms Catalan numbers and inclusion-exclusion (obstacle DP); OEIS A006318, A001003, A001850.

## Limits and handoffs

- Path families on general DAGs and LGV beyond grid paths are Graphs row 53.
- Step sets beyond {E, N, diagonal} need a bounded-height DP or row 15 recurrences; Sato's matrix formula for rational offsets is left out (notes, P054 decisions).
- No `97-Legacy` excerpt existed for this row.

## History

- 2026-10-19: P054 initial implementation; `@reviewer` found a misplaced detail namespace, the Bizley cost statement and do-while braces in the tester, all fixed; full suite passed on g++ and g++-14, stress seed 7 passed.
