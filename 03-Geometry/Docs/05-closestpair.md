# 05-closestpair.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `06-circle.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`); see [00-notes.md](00-notes.md#ge02-numeric-policy-p008) for the folder numeric policy. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `closestPair`, `closestPairRandomized` and `closestPair3` are exact APIs: they use signed integral coordinates, and every predicate widens to `lll` before arithmetic, with no epsilon or floating arithmetic; only `closestPairRandomized` draws random numbers, and they affect only its running time. `closestPairApprox` and `minimumPerimeterTriangleApprox` compare computed `long double` values under the policy's rounding rules.

### ClosestPair, closestPair

`closestPair(const vector<Point2<T>> &)` returns `ClosestPair { pair<int,int> ids; lll distance2; }`. `T` is a signed integral type of at most 64 bits; the tests instantiate `int8_t`, `int16_t`, `int` and `lng`. The input is preserved. `ids` are original indices, in increasing order, and the lexicographically smallest index pair wins every squared-distance tie. Fewer than two points return `ids = {-1, -1}, distance2 = 0`. A duplicate pair is a valid answer with zero distance, distinguished from the no-answer case by its indices.

The size precondition is `n <= INT_MAX`. The squared bounding-box diagonal must be at most `2^127 - 1`. `closest_detail::fits` asserts this with unsigned 128-bit subtraction, so the check cannot overflow even for a fully spread signed-64-bit input. Differences widen before subtraction, so every evaluated distance fits signed `lll`. Coordinates may use the full signed-64-bit storage range: extreme singletons and tight clusters near either end work. This is a span restriction, not support for every possible signed-64-bit point set. Under `-DNDEBUG` the bounds remain preconditions.

Worst case: deterministic `O(n * log(n))` time and `O(n)` auxiliary memory (two `int` arrays plus `O(log(n))` stack). The initial sort by (point, original index) reproduces a stable (x, y) sort while keeping `std::sort`'s worst-case bound; the y merges preserve that order on equal y.

Correctness: in the sorted (point, index) order, each group of equal points is contiguous and its two smallest indices are adjacent. Scanning adjacent equal pairs therefore yields the lexicographically smallest zero-distance answer. That prepass returns immediately, so no strip ever meets arbitrarily many tied zero-distance pairs. On a horizontal or vertical line, an optimum is an adjacent pair in coordinate order, so the 2D function scans adjacent pairs and returns.

The 2D recursion splits the fixed x order. Brute-force leaves settle their pairs, and each return produces y order by a linear merge. A cross-half pair at least as good as the current answer lies in the closed x strip and in the closed y window. Both filters reject only `>` the current squared distance, so equal-distance pairs stay eligible for the index tie rule. Each half has no pair closer than the global best before the merge, and the half-square packing argument bounds the candidates per point by a constant. Shrinking the global best during the scan cannot invalidate an earlier exclusion.

The 2D implementation follows the archived divide-and-conquer recurrence, but reworks its arithmetic domain, no-answer value, stable sort, duplicate handling and ties. It avoids the archived finite INF64 sentinel, 64-bit squaring/subtraction overflow and unspecified pair order. The original archived bytes are unchanged.

### closestPairRandomized

`closestPairRandomized(const vector<Point2<T>> &, ulng seed = CustomHash::rnd)` has `closestPair`'s domain, assertions, result and tie rule: for every seed it returns exactly `closestPair`'s answer. It is Las Vegas: the seed (from `06-Miscellaneous/02-customhash.hpp`) changes only the running time, which is `O(n)` expected for every input, with `O(n)` memory (a hash table of `2^ceil(log2(2n))` slots and four `int` arrays).

Algorithm (Rabin 1976, as in cp-algorithms): draw `n` uniform pairs of distinct indices and let `d` be their least squared distance. Grid the plane by cells of side `s = cellSide(d)`, the truncated long double square root of `d`, which is `floor(sqrt(d))` or one more (below), keyed by `floor((x - x_min) / s)` and `floor((y - y_min) / s)` as `ulng` (a span of `2^64 - 1` fits), hashed by SplitMix64 with the advanced seed and linear probing. Scan every pair inside a cell and between a cell and its four forward neighbours `(1, -1), (1, 0), (1, 1), (0, 1)`. A neighbour key past an edge, including `y - 1` wrapping to `2^64 - 1` below the first row, names no occupied cell: `fits` keeps every cell index below `2^63.5 + 1`. `d == 0` takes `s = 1`, whose cells are single points, and updates only each cell's first two members, its two smallest indices, since counting-sort placement keeps members in increasing index order.

Correctness: the optimum `D <= d`, and a pair within `sqrt(d)` has integer `|dx|, |dy| <= floor(sqrt(d)) <= s`, so it differs by at most one cell in each axis and every pair at distance `D` is examined and the shared `update` applies the index tie rule. With `d == 0` the optimum is 0, the zero-distance pairs are the pairs of equal points, and the lexicographically smallest of them is the smallest first-two pair over all cells. Time: the number of pairs closer than `d` is the rank of the minimum of `n` samples among `n(n - 1) / 2` pairs, `O(n)` in expectation; the pairs in neighbouring cells number `O(n + #pairs within sqrt(2) * 2s)`, which splitting each cell into four quarter cells bounds by a constant times `n` plus the pairs closer than `d`: a quarter cell has diameter `s / sqrt(2) <= (sqrt(d) + 1) / sqrt(2) <= sqrt(d)` once `d >= 6`, and since cells are half-open, two lattice points in one quarter cell differ by less than its side in each axis and are strictly closer than `d`; and below that a cell spans at most 3 x 3 lattice positions, whose repeated points are pairs closer than `d`. A sampled `d == 0` breaks that bound, since every pair of a repeated point is within 0, hence one update per single-point cell.

`cellSide(d)` is `lll(sqrt((long double)ulll(d)))`. For `1 <= d <= 2^127 - 1` (`fits`) let `F = floor(sqrt(d)) < 2^63.5`. The conversion of `d` and the square root each round to nearest with a 64-bit significand, so the computed root of any `d >= F^2` is at least `F * (1 - 2^-65)` before its own rounding, less than half the long double spacing below the representable integer `F` (when `F` is a power of two the gap can reach half a spacing, but then `F^2` is representable and the converted `d` is at least `F^2` exactly, so the root is at least `F`); it therefore rounds to at least `F`, and symmetrically to at most `F + 1` for `d < (F + 1)^2`. A scratch check of 312 million values (every `F < 2 * 10^6`, a window around each power of two up to `2^64`, the top `3 * 10^6` values of `F` and random ones; at each, `F^2`, `F^2 + 1`, `F^2 + F` and the two values below `(F + 1)^2`) found every result in `[F, F + 1]`. The argument needs the 64-bit long double significand of x87 extended precision, which GCC on x86-64 Linux and on mingw-w64 (Codeforces) provides; a 53-bit `long double` would break it. A negative `d`, possible only past a violated `fits` under `NDEBUG`, converts to an unsigned value below `2^128`, so `s <= 2^64`: `closestPair3` stays `O(n log n)`, while `closestPairRandomized` may truncate `s = 2^64` to a zero `ulng` side and stop on an integer division trap. Either way the process ends within the operation's bound, with no meaningful result (`03-cpp.md`, violated preconditions).

### closestPairApprox

`closestPairApprox(const vector<Point2<T>> &)` takes floating `T` (`float`, `double`, `long double`), finite, `n <= INT_MAX`, and returns the increasing index pair; fewer than two points return `{-1, -1}`. It returns exactly the brute-force pair minimizing the computed `dist2(p[i], p[j])` of `01-point.hpp` (coordinates widened to `long double`, subtracted, squared, summed), the lexicographically smallest on equal computed values. Squared differences must neither overflow nor underflow (distinct points keep a nonzero computed distance).

It shares `closest_detail::closest2` with `closestPair`. The exclusions stay exact on computed values because rounding is monotone: `fl(fl(dx^2) + fl(dy^2)) >= fl(dx^2)`, so a strip point with `fl(dx^2) > best` cannot beat or tie `best`, and `fl(y_a - y_j)` grows as `y_j` falls, so the backward window scan stops correctly. The collinear shortcut of the integral case is off: on rounded values a non-adjacent pair can tie the best adjacent pair with smaller indices.

### closestPair3

`closestPair3(const vector<Point3<T>> &)` has the same result type, tie rule, input preservation, no-answer value and type domain as `closestPair`. The asserted span condition is `dx^2 + dy^2 + dz^2 <= 2^127 - 1`. Its worst case is deterministic `O(n * log(n))` time and `O(n)` memory: six `int` arrays and one `lll` row-key array, plus `O(log(n))` stack. No hashing or randomization is used.

Correctness: the same recursion maintains two merged orders, by y and by z. At a merge, fix `d` as the current best (`d >= 1` after the prepass) and `s = cellSide(d)`, so `floor(sqrt(d)) <= s <= floor(sqrt(d)) + 1`. Slab points satisfy `(x - x_m)^2 <= d`. Bucket the slab into rows by the truncated quotient `y / s` (row 0 spans `(-s, s)`, every other row `s` values; each is at least `s` tall, so a pair with `|dy| <= s` shares a row or spans two adjacent rows, and the quotient is monotone in y): ranks come from the y order, and a stable counting sort of the z order makes every row z-sorted in `O(slab)`. A pair within `sqrt(d)` has `|dy| <= s`, so it lies in one row or in two adjacent rows. Each point back-scans its own row while `dz^2 <= best`, and a monotone pointer scans the previous adjacent row for `|dz| <= s`. Every point on one side is at least `sqrt(d)` from every other point on its side, so a `2 sqrt(d) x 4 sqrt(d) x 4 sqrt(d)` box holds `O(1)` of them, and both scans examine `O(1)` points each. Each level is `O(n)`, which gives `O(n * log(n))` after the initial sort. This matches the `Omega(n * log(n))` algebraic decision-tree lower bound, deterministically.

### minimumPerimeterTriangleApprox

`minimumPerimeterTriangleApprox(const vector<Point2<T>> &)` takes integral `T` of at most 64 bits or floating `T`, finite, `n <= INT_MAX`; for floating `T`, coordinate differences must not underflow (zero or normal), since the slack below is relative. With `q` the points cast to `long double`, it returns increasing ids `i < j < k` minimizing the computed `distanceApprox(q[i], q[j]) + distanceApprox(q[j], q[k]) + distanceApprox(q[i], q[k])` (that order on the sorted ids), the lexicographically smallest triple on equal computed values. Collinear and repeated points form degenerate triangles (three equal points give 0); fewer than three points return `{-1, -1, -1}`. Worst case `O(n * log(n))` time, `O(n)` memory.

Algorithm (the divide and conquer of cp-algorithms' closest-pair article, extended to triangles): sort by (point, id). Runs of three or more equal points settle a zero perimeter at once and return their smallest triple. Otherwise recurse on the x order, brute-force blocks of fewer than six points, merge by y, keep the points within `per / 2` in x of the split, and for each strip point scan the pairs that follow it within `per / 2` in y.

Correctness: a triangle with perimeter at most `per` has every side at most `per / 2`, so a triangle crossing the split lies in the strip and spans at most `per / 2` in y from its lowest vertex. The filters use `per / 2 * (1 + 2^-40)`, a slack far above the rounding of the computed sums and differences, so every triple whose computed perimeter ties or beats `per` is examined and the result equals the brute force on computed values; the slack only admits extra candidates. Time: after the prepass each point repeats at most twice, and every triple on one side has perimeter at least `per`, so a square of side `per / 5` holds at most two points of a side (three would form a triangle of perimeter at most `4 per / 5`); a `per x per / 2` window therefore holds at most 36 points, and each strip point examines `O(1)` triples.

### closestPairNaive, closestPair3Naive, minimumPerimeterTriangleApproxNaive

All-pairs (and all-triples) scans with exactly the domain, assertions, result and tie rule of `closestPair`, `closestPair3` and `minimumPerimeterTriangleApprox` (user decision 2026-11-14, Priority: lighter). The pair scans share `closest_detail::update` and the exact `lll` distances, so ties fall to the lexicographically smallest ids in any scan order; the triangle scan shares `update3`, the same sorted-triple perimeter expression the divide and conquer evaluates, so both return the same triple. Cost: `O(n^2)` (`O(n^3)` for triangles) time and `O(1)` extra memory (`O(n)` for the triangle's long double copy).

## Feature-to-test map

`96-Local Testing/03-Geometry/05-closestpair_tester.py` drives the C++ suite and the Python bigint oracle in both dimensions. Mode coverage is stated in the entry's docstring. C++ oracle failures shrink by single-point deletion to a locally minimal input before printing it. Python failures print the seed, the full input, the expected and actual values and the command.

| Operation | Test | Oracle |
|---|---|---|
| `ClosestPair`, `closestPair`, `closestPair3`: empty/singleton/no-answer, extreme singletons, duplicate groups, input preserved | Fixed C++ regressions and the Python bigint protocol in every configuration | Expected values; Python bigint |
| `closestPair`: original indices and lexicographic ties | Exhaustive ordered 2x2 lists through length 7; all 512 3x3 subsets in natural and shuffled order; tied-x/tied-y grids | O(n^2) oracle |
| `closestPair`: divide/merge strip crossings | 1,200 random grid, sparse wide, tied-x and tied-y clouds; shuffles, quarter turns and translations; 500/50,000/250,000-point lattice, vertical line, horizontal line, all-equal and duplicates-off-a-line inputs with known answers, the last within 5 s in the optimized build | O(n^2) oracle |
| `closestPair3`: ties and degeneracies | Exhaustive ordered 2x2x2 lists through length 5 (37,449 lists); 4,000 random 3x3x3 subsets, each also shuffled | O(n^2) oracle |
| `closestPair3`: slab rows and adjacent-row scan | 1,200 random clouds (wide 3e18, dense 13^3, 81^3) including fixed-x planes, fixed-y planes and the slanted `y = x, z = 0` plane, with axis rotations, quarter turns and translations; 64,001-point lattices (cube, plane) and a diagonal line, and 1,000,001-point (1,001 in quick) y-line and two-row plane that put the whole input in one slab, each with a known index answer | O(n^2) oracle; known answer |
| Arithmetic width/domain (2D and 3D) | Python arbitrary-integer cases per configuration (509 in 2D, 505 in 3D), each also through `closestPairNaive` and `closestPair3Naive`: exact distances beyond float precision, near-`2^127` squared span, signed-64-bit endpoint clusters, wide random and duplicate data | Python bigint |
| Coordinate types | `int8_t`, `int16_t`, `int`, `lng` (2D); `int8_t`, `int` (3D); `int` distances above signed 64 bits | O(n^2) oracle |
| `closestPairNaive`, `closestPair3Naive`, `minimumPerimeterTriangleApproxNaive` | Every oracle-checked case of their fast forms (all exhaustive, subset and random rounds; 2D typed rounds; 3D `int` and `int8_t` extremes), and, for the pair scans, the Python bigint protocol (`--oracle-naive`, `--oracle3-naive`); probes `naive-span-x` and `naive-diagonal3` | O(n^2) and O(n^3) oracles; Python bigint |
| Type domains (`static_assert`) | Once per full or stress run, 24 `-fsyntax-only` compiles: each operation compiles for documented coordinate types and fails its `static_assert` for `lll` and for the other number kind (`double` for the exact operations, `int` for `closestPairApprox`) | Compiler verdict and `static assertion failed` |
| `closestPairRandomized`: every seed equals the exact answer | Every 2D `verify` case (ordered 2x2 lists, 3x3 subsets, random tied grids) also runs with seeds 0, `~0` and 1/4/8 more (quick/full/stress); typed rounds (28/700/5,600 per `int8_t`, `int16_t`, `int`, `lng`) with one-axis spans at both ends of the type (about 1.3e19 for `lng`), tight clusters, pools of 1 to 3 repeated points, 5x5 lattices, diagonals and wide clouds; large clouds (random, clustered, extreme span with tiny second spread) and the known-answer shapes, including 50,000 copies of one point | O(n^2) oracle; `closestPair`; known answer |
| `closestPairRandomized`: arithmetic width | `--oracle-randomized`: three seeds per Python bigint 2D case, the extreme-span cases included | Python bigint |
| `closestPairApprox` | Fixed `-0` equal to `0` and a rounding tie that must return `(0, 1)` although the exact minimum is `(2, 3)`; random rounds (24/1,200/8,000 per `float`, `double`, `long double`) of grids, sub-ulp offsets that tie after rounding, values near `+-1e16`, wide exponents, signed zeros, three kinds of collinear line and far clusters; planted shapes of 500/50,000/250,000 points and as many equal points, the latter within 5 s in the optimized build | Brute force on the same `long double` expression, exact ids; known answer |
| `minimumPerimeterTriangleApprox` | Fixed cases; all ordered lists over a 2x2 grid (length 5/7/8) and over a 3-point line (length 7); all 3x3 subsets, and all 4x4 subsets outside quick; random rounds (16/800/6,000 per seven types) of grids, groups of 2 and 3+ duplicates, collinear sets, thin strips, 1e9 and 1e18 clouds and clusters; 2,000 points in quick and 200,000 otherwise (`lng`, `double`, plus `int`, `float` outside quick): a planted triangle in a jittered lattice, a line, duplicate pairs, all-equal points and one point followed by copies of another, each under a 20 s bound in the optimized build | O(n^3) brute force with the identical summation, exact ids; known answer |
| Preconditions | Ten checked-build assertion probes: 2D x span, y span and diagonal; the same three for `closestPairRandomized`; 3D z span and three-axis diagonal; `closestPairNaive` x span and `closestPair3Naive` diagonal. The `n <= INT_MAX` assertion is inspected; the huge allocation is not run | Expected assertion failure |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 905 MB; optimized 0.2 s compile + 3.5 s run, checked 33.5 s compile + 12.0 s run, ASan-UBSan 37.3 s compile + 10.9 s run; MEMORY peak 1178 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 963 MB; optimized 0.2 s compile + 3.8 s run, checked 36.4 s compile + 18.1 s run, ASan-UBSan 44.1 s compile + 16.8 s run; MEMORY peak 1136 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 33.6 s compile + 0.3 s run, checked 26.2 s compile + 1.0 s run; MEMORY peak 890 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 903 MB; optimized 0.1 s compile + 8.0 s run, checked 12.5 s compile + 36.4 s run, ASan-UBSan 14.1 s compile + 29.9 s run; MEMORY peak 751 MB. Dated rows below stay those commands' latest results.

2026-11-14 (P008; experiment none), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns. Optimized counts include 32 time-bound checks that only the optimized build runs.

| Command | Result |
|---|---|
| `05-closestpair_tester.py --mode full --seed 1` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 788,266 checks optimized and 788,234 in each other configuration; Python bigint 509 cases (2D) through `closestPair` and `closestPairNaive`, 1,527 randomized answers (509 cases, three seeds), and 505 cases (3D) through `closestPair3` and `closestPair3Naive`; 10 assertion probes; 24 type-domain compiles; optimized run 83.1 s (75.1 s of it the Python oracle and the compiles), ASan/UBSan 107.5 s; `MEMORY peak=2088MB` |
| `CXX=g++-14 05-closestpair_tester.py --mode full --seed 2` (GCC 14.4.1) | PASS, the same counts, probes and compiles; ASan/UBSan run 124.8 s; `MEMORY peak=2169MB` |
| `05-closestpair_tester.py --mode stress --seed 3` (all three configurations) | PASS, 7,770,704 checks optimized and 7,770,672 in each other configuration; Python bigint 4,009 + 4,009 + 12,027 + 4,005 + 4,005 answers; 24 type-domain compiles; ASan/UBSan run 277.7 s; `MEMORY peak=901MB` |
| `05-extraction.py ops --filter 05-closestpair` | PASS: 10 operations in 9 extractions on g++ and g++-14; `closestPairApprox`, which only floating types instantiate, has no default-form candidate |
| `05-extraction.py testers --filter 03-Geometry/0` | PASS, 16 names for this tester, quick on the extracted build |
| `04-mutation.py run 03-Geometry/05-closestpair.hpp` | See Mutation testing |

```bash
python3 '96-Local Testing/03-Geometry/05-closestpair_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/03-Geometry/05-closestpair_tester.py' --mode full --seed 2
python3 '96-Local Testing/03-Geometry/05-closestpair_tester.py' --mode stress --seed 3
python3 '96-Local Testing/05-extraction.py' ops --filter 05-closestpair
python3 '96-Local Testing/05-extraction.py' testers --filter 03-Geometry/0
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/05-closestpair.hpp
python3 '96-Local Testing/03-Geometry/05-closestpair_benchmark.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

`python3 '96-Local Testing/04-mutation.py' run 03-Geometry/05-closestpair.hpp`, rerun on the lines `/finish` changed (the three naive scans, the 3D `fits` overload, `updateTriangle`, `points`):

Record: mewt 4.0.0 on blob `9d277c5d9534ccfc76020111e68abc85714b1d2b` (2026-11-14, seed 1, 51 lines changed since blob `a135635df565`): 216 mutants, 186 caught (1 by timeout), 30 survived, 0 untested.

Stage seconds: quick 597, full-optimized 1270, full-rest 2791 (baselines 15.3 s, 61.0 s and 128.1 s). The 30 survivors are equivalent:

- Lines 29 and 30, the 3D `fits` with `min` and `max` arguments commuted (AS 297 to 302): integer coordinates, so both orders give the same value.
- Counting loops whose index rises by one to a bound it starts at or below: line 39 `j < n` with `n >= 2` (COS 951), lines 40, 270 and 271 `i < j`, `j < k` (COS 956, 1471, 1476), each `<` → `!=`.
- Commuted ids: line 40 `update(p, b, j, i)` (AS 307), which orders them through `minmax` and computes the symmetric exact `dist2`; lines 232 and 271, `updateTriangle` with two ids swapped (AS 396, 397, 425, 426), which sorts them first.
- Line 44, `distanceApprox` with its two points swapped (AS 309, 310, 311): the difference only changes sign, exactly, so the squares, sum and root are bitwise equal.
- Line 45, `v == per` → `<=` (COS 972): `v < per` is already accepted by the previous disjunct; `t < best` → `<=` (COS 977): an equal triple has the same computed perimeter, so the state is unchanged.
- Line 255, the strip filter `|dx| <= half()` → `<` (COS 1427): a triple across the split holds a point on each side of `x`, so a point at `|dx| >= half()` gives it an x span, hence an exact perimeter, of at least `2 half() = per (1 + 2^-40)`. `per > 0` here (three equal points return earlier), and the rounding of the perimeter, the difference and `half()` is near `2^-63` relative, so the computed perimeter exceeds `per` and the triple can neither win nor tie; dropping the point removes only such triples.
- The naive scans' `n <= INT_MAX` assertions at lines 104, 215 and 265, removed or `<=` → `!=` or `<` (CR 166, 211, 233; COS 1096, 1097, 1341, 1342, 1461, 1462): a distinguishing input needs at least `2^31` points, beyond every memory cap; the bounds were inspected, as for the fast forms' assertions in the first campaign.

Earlier campaign, after `cellSide`: mewt 4.0.0 on blob `a135635df565abe7b15991818217ff7b528cacd4` (2026-11-14, seed 1, 11 lines changed since blob `34617e164e75`): 26 mutants, 25 caught (0 by timeout), 1 survived, 0 untested. Stage seconds: quick 72, full-optimized 76, full-rest 189 (baselines 13.0 s, 46.2 s and 100.9 s). The survivor is equivalent: line 114, the counting loop `i < n` → `i != n` (COS 1050).

Earlier campaign, after the review fixes: mewt 4.0.0 on blob `f9140ca2eb53b9089b0ff19ad65d2529333066b6` (2026-11-14, seed 1, 6 lines changed since blob `34617e164e75`): 29 mutants, 23 caught (0 by timeout), 6 survived, 0 untested. Stage seconds: quick 77, full-optimized 24, full-rest 352 (baselines 14.4 s, 19.7 s and 97.9 s).

- Four survivors removed or weakened the `d <= 0` guard that the review had added to the loop-corrected `ceilSqrt` (CR 105, IF 199, COS 888, 890). Without it a negative `d`, reachable only past a violated `fits` under `NDEBUG`, ran its correction loop about `2^64` times, and no permitted test can feed that input to an `NDEBUG` build (`05-testing.md`, inputs to optimized builds). `cellSide` replaced the function, guard and loops (Contracts, `closestPairRandomized`), so the mutants no longer exist.
- The triangle's `static_assert` with `&&` → `||` (LOS 867), which admits `lll`: the type-domain compiles now reject it (`minimumPerimeterTriangleApprox(vector<Point2<lll>>)` compiled under the mutant in a scratch tree).
- Line 118, `start[c + 1] - start[c] > 1` → `!= 1` (COS 1094): every cell holds at least one point, so the two tests agree.

Earlier campaign, on the lines the first campaign's removals changed: mewt 4.0.0 on blob `34617e164e75a52b558cf2f775fbd118831d18dd` (2026-11-14, seed 1, 7 lines changed since blob `39b33be94d0a`): 41 mutants, 36 caught (2 by timeout), 5 survived, 0 untested.

Stage seconds: quick 188, full-optimized 183, full-rest 462 (baselines 18.8 s, 25.3 s and 123.8 s). The 5 survivors are equivalent:

- Lines 117 and 118, counting loops `<` → `!=` (COS 1084, 1089).
- Line 126, `cx + dx` → `cx - dx` or `cy + dy` → `cy - dy` (AOS 578, 582): the four offsets become `(-1, -1), (-1, 0), (-1, 1), (0, 1)` or `(1, 1), (1, 0), (1, -1), (0, -1)`, each again one of every pair `+-v` of the eight neighbour offsets, so every pair of neighbouring cells is still scanned once; a key below 0 wraps to an index no cell has.
- Line 177, the new-row test `key[rows - 1] != q` → `<` (COS 1184): the slab is scanned in y order and truncated division is monotone, so `key[rows - 1] <= q` always holds.

Earlier campaign: mewt 4.0.0 on blob `39b33be94d0af1417f0181e15cdba9ae843ce689` (2026-11-14, seed 1, whole file, uniform sample of 1458): 500 mutants, 442 caught (9 by timeout), 58 survived, 0 untested.

Stage seconds: quick 1632, full-optimized 238, full-rest 2340 (baselines 12.2 s, 14.1 s and 71.7 s).

Five survivors exposed redundant code, which was then removed: the grid scan's boundary guards and the two `max_element` passes behind them (COS 1153, 1154, 1173 at blob lines 128 and 129; a neighbour key past an edge, including `cy - 1` wrapping to `2^64 - 1`, names no occupied cell, because `fits` keeps every cell index below `2^64 - 1`), and the floor correction of the 3D row key (IF 219, COS 1254 at line 181; truncated rows are still at least `s` tall, so a pair with `|dy| <= s` still shares a row or spans two adjacent ones, and truncation is monotone, so rows stay contiguous in y order). Four survivors only slowed the 3D slab scan: putting every slab point in one row (AOS 709, line 183) and never advancing the previous-row window (WF 258 and COS 1305, 1309 at line 192, which make `j0 < b` false). Both scan a superset of the candidate pairs. The tester's million-point 3D y-line and two-row plane were added for them: at `-O2 -DNDEBUG` the original solves each in 1.5 s, while the one-row mutant exceeds 90 s on the y-line and the frozen window exceeds 90 s on the two-row plane, beyond the campaign's per-mutant timeout. The independent review found two more runtime-only survivors, closed the same way with timed shapes: line 53, `b.d == 0` → `<` (COS 986), runs the strips at distance 0, quadratic on duplicates off a line (21.8 s for `closestPair` on 49,999 copies of `(0, 0)` and `(1, 2)`, 23.3 s for `closestPairApprox` on 50,000 equal `double` points, against under 0.01 s; the tester now bounds both at 5 s in the optimized build), and line 217, `best[0] >= 0` → `== 0` (COS 1390), drops the early return after three equal points, cubic on `(1, 0)` followed by copies of `(0, 0)` (37.2 s at n = 2,000 against under 0.01 s; the triangle test's 20 s bound now covers that shape). The other 47 survivors are equivalent, by blob line:

- Commuted symmetric arguments: `update` and `dist2` pairs, which sort the ids through `minmax` or `sort` (AS 278, 284, 316, 327, 348, 349, 363, 364, 365, 381), and `max` of equal-typed coordinates (AS 321, 322).
- Counting loops whose index rises by one to the bound: `<` → `!=` (COS 961, 1011, 1016, 1111, 1116, 1241, 1341, 1406, 1411, 1436, 1306 for `j0 != b` with `j0 <= b`, 1316 for `j != b` with `j0 <= b`), and `<` → `<=` where the inner loop then runs no iteration (COS 1142, 1232).
- Assertions of `n <= INT_MAX` (CR 132, 182; COS 1191, 1207, 1331): a distinguishing input needs 2^31 points, beyond every memory cap; the bounds were inspected.
- Line 24, the y `minmax_element` comparator reversed (COS 913): the span becomes the negated difference, whose square in `ulll` arithmetic equals the true square modulo `2^128`, so `fits` checks the same value.
- Lines 28 and 29 of `ceilSqrt` (COS 915, 924; AOS 421, 422, 424): each returns an `s` between `floor(sqrt(d))` and `ceil(sqrt(d)) + 1`, never 0 for `d >= 1`. With integer coordinates every pair within `sqrt(d)` has `|dx|, |dy|, |dz| <= floor(sqrt(d))`, so cells and rows of any such side find the same pairs; only the constant changes.
- Line 33, `ij < b.ids` → `<=` (COS 942): equal ids are the same pair at the same distance.
- Line 40, the equal-point test made always true (IT 229, AOS 427): every adjacent pair in `(point, id)` order becomes a candidate. Extra genuine candidates cannot change the minimum of the total order (distance, ids), which every caller still examines in full.
- Line 49, the vertical-line test `q.x == p[0].x` → `<` (COS 971): the shortcut is never taken and the divide and conquer, correct for every input, runs instead.
- Line 53, `b.d == 0` → `<=` (COS 987): squared distances are non-negative.
- Line 107, the cell hash `c.first + seed` → `*` (AOS 551): the probe sequence changes; lookups compare whole keys.
- Line 122, `> 1` → `!= 1` for a cell's size (COS 1136): every cell holds at least one point.
- Line 188, `t > 1` → `t != 1` under `t >= 1` (COS 1286).
- Line 193, `z <= a.z + s` → `<` (COS 1322): on that blob `s >= sqrt(d)`, so a pair across rows (`|dy| >= 1`) at `|dz| = s` exceeded `d`. Since `cellSide` may return `floor(sqrt(d))` the argument no longer holds, and the current full suite kills this mutant (3D subsets; for example `closestPair3({{-10,100,100},{-10,102,102},{0,1,2},{1,2,0},{10,200,200},{10,202,202}})` must return `(2, 3, 6)`).
- Line 230, `midpoint(l, r)` → `midpoint(r, l)` (AS 368): the split rounds up instead of down; both halves keep at least three points.

## Benchmarks

`python3 '96-Local Testing/03-Geometry/05-closestpair_benchmark.py'` writes the record to `96-Local Testing/03-Geometry/05-closestpair_benchmark.json`. The record holds CPU, compiler, flags, seed, one warmup plus five repetitions, medians, dependency hashes and the output checksum. The benchmark covers sizes 32, 512, 2,048 and 200,000:

- **2D:** `closestPair` and `closestPairRandomized` (a fresh seed per repetition) against the preserved legacy recurrence (on its safe domain, distance only) and brute force for `n <= 2048`; `closestPairApprox` on long double points against a floating brute force.
- **3D:** `closestPair3` against brute force for `n <= 2048`.
- **Triangle:** `minimumPerimeterTriangleApprox` on random, grid and duplicate-pair inputs, against an `O(n^3)` brute force for `n <= 512` and a windowed brute-force check above.

Every timed output is checked. Input generation and checking are excluded; allocation and sorting are included. Peak memory is not measured.

2026-11-14 (P008; experiment none; 113 measurements, seed 20260927, checksum `248219233356`), on an Intel Core i9-11900H with GCC 16.2.1, `-O2 -DNDEBUG`, load average 2.4 from other sessions:

| n = 200,000 | median (ms) | compared |
|---|---:|---|
| 2D random | 48.75 | randomized 32.75, legacy 41.02 |
| 2D grid | 51.67 | randomized 28.99, legacy 25.81 |
| 2D vertical | 4.60 | randomized 19.25, legacy 12.73 |
| 2D duplicates | 5.62 | randomized 5.16, legacy 8.44 |
| 2D random, `closestPairApprox` | 63.53 | — |
| 3D random | 90.65 | — |
| 3D lattice | 134.86 | — |
| 3D plane | 119.78 | — |
| 3D duplicates | 5.97 | — |
| Triangle random | 61.01 | — |
| Triangle grid | 84.54 | — |
| Triangle duplicate pairs | 246.72 | — |

At n = 2,048, 2D random takes 0.31 ms against 5.34 ms for brute force, and 3D random 0.58 ms against 7.56 ms. `closestPairRandomized` overtakes the deterministic `closestPair` between n = 512 (57.1 µs against 41.0 µs) and n = 2,048 (0.25 ms against 0.31 ms) on random and grid inputs, and is about 1.5 to 1.8 times faster at 200,000; on the sorted-axis early return (vertical line) the deterministic code is about 4 times faster. Both stay, as the row records (expected `O(n)` beside deterministic `O(n log n)`). The legacy code is faster on equal-distance grids because it neither keeps exact 128-bit distances nor resolves index ties. The 3D lattice and plane inputs are `closestPair3`'s worst observed: every point is tied at the minimum distance, so the slab scans do their maximal constant work. The triangle is slowest on its duplicate-pair input, every point of a spacing-10 grid given twice, where many triangles tie near the minimum perimeter.

## Sources

- The 2D recursion follows the archived divide-and-conquer recurrence (bytes unchanged), reworked as described under Contracts; the folder notes list `OLD/Team Notebook/src/geometry/getnearestpair.cpp` as superseded by GE02. The benchmark times the preserved legacy recurrence.
- `closestPairRandomized` follows Rabin's sampled grid as cp-algorithms presents it, `minimumPerimeterTriangleApprox` the divide-and-conquer extension in the same article, and `closestPairApprox` the floating closest pair of suisen and maspypy. The ordered-set sweep, the incremental grid and the other alternatives not adopted are in [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Limits and handoffs

Range-restricted closest pair belongs to row 29 (`29-point_set_queries.hpp`).

## Aliases

Search aliases (P263, grab --find): nearest pair of points, nearest points (OI Wiki: 平面最近点对), closest pair, 三维最近点对, 分治求最近点对.

Search aliases (P008, grab --find): closest pair randomized, Rabin closest pair, expected linear closest pair, 随机化最近点对, 期望线性最近点对, floating closest pair, minimum perimeter triangle, closest triple, 最小周长三角形, 平面最小周长三角形, brute force closest pair, all pairs closest pair, 暴力最近点对.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 55,373 checks) passed on g++ 16; `closestPair3` missing.
- 2026-10-07: re-audit, implemented `closestPair3`; 4 findings fixed; full suite, stress and benchmark passed on g++ 16; independent review: reproduced every full count and probe, confirmed one package defect (in `06-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed. Header edit for `-Werror` without demotions: `[[maybe_unused]]` on names only an `assert` uses; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
- 2026-11-14: P008 re-audit (GE02, experiment none): implemented `closestPairRandomized`, `closestPairApprox` and `minimumPerimeterTriangleApprox` after the completeness sweep; three mutation campaigns: the first removed the grid-edge guards and the 3D floor correction and added the 3D line and two-row plane shapes and timed duplicate shapes; independent review: 5 confirmed findings fixed (two runtime mutants now killed by timed shapes, an unbounded `ceilSqrt` under `NDEBUG` past a violated precondition, the triangle's underflow domain, stale text) plus naming and `static_assert` nits; the review's `d <= 0` guard left survivors no permitted test can kill, so the bounded truncated root `cellSide` replaced the function; type-domain compiles added; full on g++ and g++-14, stress, extraction, benchmark and `02-integration.py --sanitizers` passed.
- 2026-11-14: `/finish P008` (experiment none): added `closestPairNaive`, `closestPair3Naive` and `minimumPerimeterTriangleApproxNaive` (user decision, a Priority exception recorded in `omissions.md`), checked on every oracle case of their fast forms, the Python bigint protocol, typed rounds and two probes; `closestPair3` now asserts its own spans through a 3D `fits`; review fixes: the 2D division trap past a violated `fits`, the 64-bit significand and power-of-two clauses of the `cellSide` argument, the earlier COS 1322 argument, stale counts, `std::abs`, two missing type-domain compiles (24); fourth mutation campaign on the 51 changed lines, 30 equivalent survivors; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
