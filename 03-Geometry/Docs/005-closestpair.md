# 005-closestpair.hpp — Evidence

Owned by package P008 / GE02, with `006-circle.hpp`, `007-rotatingcalipers.hpp` and `008-coordinate_transform.hpp`. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric Policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `closestPair`, `closestPairRandomized` and `closestPair3` are exact: signed integral coordinates, every predicate widened to `lll`, no epsilon and no floating arithmetic. Only `closestPairRandomized` draws random numbers, and they affect only its running time. `closestPairApprox` and `minimumPerimeterTriangleApprox` compare computed `long double` values under the policy's rounding rules.

### ClosestPair, closestPair

`closestPair(const vector<Point2<T>> &)` returns `ClosestPair { pair<int,int> ids; lll distance2; }`. `T` is a signed integral type of at most 64 bits; the tests instantiate `int8_t`, `int16_t`, `int` and `lng`. The input is preserved. `ids` are original indices in increasing order, and the lexicographically smallest index pair wins every squared-distance tie. Fewer than two points return `ids = {-1, -1}, distance2 = 0`. A duplicate pair is a valid answer with zero distance, told apart from the no-answer case by its indices.

Preconditions: `n <= INT_MAX`, and the squared bounding-box diagonal is at most `2^127 - 1`. `closest_detail::fits` asserts the span with unsigned 128-bit subtraction, which cannot overflow even for a fully spread signed-64-bit input. Differences widen before subtraction, so every distance fits signed `lll`. Coordinates may use the full signed-64-bit range (extreme singletons and tight clusters near either end work); the restriction is on the span. Under `-DNDEBUG` the bounds remain preconditions.

Worst case: deterministic `O(n * log(n))` time and `O(n)` memory (two `int` arrays plus `O(log(n))` stack). The initial sort by (point, original index) reproduces a stable (x, y) sort with `std::sort`'s worst-case bound, and the y merges keep that order on equal y.

Correctness: in (point, index) order each group of equal points is contiguous with its two smallest indices adjacent, so scanning adjacent equal pairs yields the lexicographically smallest zero-distance answer. That prepass returns at once, so no strip meets arbitrarily many tied zero-distance pairs. On a horizontal or vertical line an optimum is an adjacent pair in coordinate order, so the function scans adjacent pairs and returns. The recursion splits the fixed x order; brute-force leaves settle their pairs, and each return produces y order by a linear merge. A cross-half pair at least as good as the current answer lies in the closed x strip and the closed y window. Both filters reject only distances `>` the current one, so equal-distance pairs stay eligible for the tie rule. Each half has no pair closer than the global best, and the half-square packing argument bounds the candidates per point by a constant. Shrinking the best during the scan cannot invalidate an earlier exclusion.

The recursion follows the archived divide and conquer but replaces its arithmetic domain, no-answer value, sort, duplicate handling and ties, avoiding the archive's finite INF64 sentinel, 64-bit squaring and subtraction overflow and unspecified pair order. The archived bytes are unchanged.

### closestPairRandomized

`closestPairRandomized(const vector<Point2<T>> &, ulng seed = CustomHash::rnd)` has `closestPair`'s domain, assertions, result and tie rule, and returns exactly `closestPair`'s answer for every seed. It is Las Vegas: the seed (from `06-Miscellaneous/002-customhash.hpp`) changes only the running time, `O(n)` expected for every input, with `O(n)` memory (a hash table of `2^ceil(log2(2n))` slots and four `int` arrays).

Algorithm (Rabin 1976, as in cp-algorithms): draw `n` uniform pairs of distinct indices and let `d` be their least squared distance. Grid the plane by cells of side `s = cellSide(d)`, keyed by `floor((x - x_min) / s)` and `floor((y - y_min) / s)` as `ulng` (a span of `2^64 - 1` fits), hashed by SplitMix64 with the advanced seed and linear probing. Scan every pair inside a cell and between a cell and its four forward neighbours `(1, -1), (1, 0), (1, 1), (0, 1)`. A neighbour key past an edge, including `y - 1` wrapping to `2^64 - 1` below the first row, names no occupied cell, because `fits` keeps every cell index below `2^63.5 + 1`. `d == 0` takes `s = 1`, whose cells are single points, and updates only each cell's first two members, its two smallest indices, since counting-sort placement keeps members in increasing index order.

Correctness: the optimum `D <= d`, and a pair within `sqrt(d)` has integer `|dx|, |dy| <= floor(sqrt(d)) <= s`, so it differs by at most one cell per axis; every pair at distance `D` is examined, and the shared `update` applies the tie rule. With `d == 0` the zero-distance pairs are the pairs of equal points, and the smallest of them is the smallest first-two pair over all cells. Time: the pairs closer than `d` number the rank of the minimum of `n` samples among `n(n - 1) / 2` pairs, `O(n)` expected. The pairs in neighbouring cells number `O(n + #pairs within sqrt(2) * 2s)`, which splitting each cell into four quarter cells bounds by a constant times `n` plus the pairs closer than `d`. For `d >= 6` a quarter cell has diameter `s / sqrt(2) <= (sqrt(d) + 1) / sqrt(2) <= sqrt(d)`, and since cells are half-open, two lattice points in one quarter cell are strictly closer than `d`; below that a cell spans at most 3 x 3 lattice positions, whose repeated points are pairs closer than `d`. A sampled `d == 0` would break the bound, since every pair of a repeated point is within 0; hence the one update per single-point cell.

`cellSide(d)` is `lll(sqrt((long double)ulll(d)))`, which is `floor(sqrt(d))` or one more. For `1 <= d <= 2^127 - 1` (`fits`) let `F = floor(sqrt(d)) < 2^63.5`. The conversion and the root each round to nearest with a 64-bit significand, so the computed root of any `d >= F^2` is at least `F * (1 - 2^-65)` before its own rounding, less than half a spacing below the representable `F`. When `F` is a power of two the gap can reach half a spacing, but then `F^2` is representable and the converted `d` is at least `F^2`, so the root is at least `F`. It therefore rounds to at least `F`, and symmetrically to at most `F + 1` for `d < (F + 1)^2`. A scratch check of 312 million values (every `F < 2 * 10^6`, a window around each power of two up to `2^64`, the top `3 * 10^6` values of `F` and random ones; at each, `F^2`, `F^2 + 1`, `F^2 + F` and the two values below `(F + 1)^2`) found every result in `[F, F + 1]`. The argument needs the 64-bit significand of x87 extended precision, which GCC on x86-64 Linux and on mingw-w64 (Codeforces) provides; a 53-bit `long double` would break it. A negative `d`, possible only past a violated `fits` under `NDEBUG`, converts to an unsigned value below `2^128`, so `s <= 2^64`: `closestPair3` stays `O(n log n)`, while `closestPairRandomized` may truncate `s = 2^64` to a zero side and stop on an integer division trap. Either way the process ends within the operation's bound with no meaningful result (`03-cpp.md`, violated preconditions).

### closestPairApprox

`closestPairApprox(const vector<Point2<T>> &)` takes floating `T` (`float`, `double`, `long double`), finite, `n <= INT_MAX`, and returns the increasing index pair; fewer than two points return `{-1, -1}`. It returns exactly the brute-force pair minimizing the computed `dist2(p[i], p[j])` of `001-point.hpp` (coordinates widened to `long double`, subtracted, squared, summed), the lexicographically smallest on equal computed values. Squared differences must neither overflow nor underflow, so distinct points keep a nonzero computed distance.

It shares `closest_detail::closest2` with `closestPair`. The exclusions stay exact on computed values because rounding is monotone: `fl(fl(dx^2) + fl(dy^2)) >= fl(dx^2)`, so a strip point with `fl(dx^2) > best` cannot beat or tie `best`, and `fl(y_a - y_j)` grows as `y_j` falls, so the backward window scan stops correctly. The collinear shortcut is off: on rounded values a non-adjacent pair can tie the best adjacent pair with smaller indices.

### closestPair3

`closestPair3(const vector<Point3<T>> &)` has `closestPair`'s result type, tie rule, input preservation, no-answer value and type domain. The asserted span condition is `dx^2 + dy^2 + dz^2 <= 2^127 - 1`. Worst case: deterministic `O(n * log(n))` time and `O(n)` memory (six `int` arrays and one `lll` row-key array, plus `O(log(n))` stack), with no hashing or randomization.

Correctness: the recursion maintains two merged orders, by y and by z. At a merge, `d` is the current best (`d >= 1` after the prepass) and `s = cellSide(d)`, so `floor(sqrt(d)) <= s <= floor(sqrt(d)) + 1`. Slab points satisfy `(x - x_m)^2 <= d`. The slab is bucketed into rows by the truncated quotient `y / s`; row 0 spans `(-s, s)` and every other row `s` values, each at least `s` tall, and the quotient is monotone in y. Ranks come from the y order, and a stable counting sort of the z order makes every row z-sorted in `O(slab)`. A pair within `sqrt(d)` has `|dy| <= s`, so it lies in one row or two adjacent rows. Each point back-scans its own row while `dz^2 <= best`, and a monotone pointer scans the previous adjacent row for `|dz| <= s`. Points on one side are pairwise at least `sqrt(d)` apart, so a `2 sqrt(d) x 4 sqrt(d) x 4 sqrt(d)` box holds `O(1)` of them and both scans examine `O(1)` points each. Each level is `O(n)`, giving `O(n * log(n))` after the initial sort, which matches the `Omega(n * log(n))` algebraic decision-tree lower bound deterministically.

### minimumPerimeterTriangleApprox

`minimumPerimeterTriangleApprox(const vector<Point2<T>> &)` takes integral `T` of at most 64 bits or floating `T`, finite, `n <= INT_MAX`; for floating `T` coordinate differences must be zero or normal, since the slack below is relative. With `q` the points cast to `long double`, it returns increasing ids `i < j < k` minimizing the computed `distanceApprox(q[i], q[j]) + distanceApprox(q[j], q[k]) + distanceApprox(q[i], q[k])` (in that order on the sorted ids), the lexicographically smallest triple on equal computed values. Collinear and repeated points form degenerate triangles (three equal points give 0); fewer than three points return `{-1, -1, -1}`. Worst case `O(n * log(n))` time, `O(n)` memory.

Algorithm (the divide and conquer of cp-algorithms' closest-pair article, extended to triangles): sort by (point, id); runs of three or more equal points settle a zero perimeter at once and return their smallest triple. Otherwise recurse on the x order, brute-force blocks of fewer than six points, merge by y, keep the points within `per / 2` in x of the split, and for each strip point scan the pairs that follow it within `per / 2` in y.

Correctness: a triangle of perimeter at most `per` has every side at most `per / 2`, so a triangle crossing the split lies in the strip and spans at most `per / 2` in y from its lowest vertex. The filters use `per / 2 * (1 + 2^-40)`, far above the rounding of the computed sums and differences, so every triple whose computed perimeter ties or beats `per` is examined and the result equals the brute force on computed values; the slack only admits extra candidates. Time: after the prepass each point repeats at most twice, and every triple on one side has perimeter at least `per`, so a square of side `per / 5` holds at most two points of a side (three would form a triangle of perimeter at most `4 per / 5`). A `per x per / 2` window therefore holds at most 36 points, and each strip point examines `O(1)` triples.

### closestPairNaive, closestPair3Naive, minimumPerimeterTriangleApproxNaive

All-pairs and all-triples scans with exactly the domain, assertions, result and tie rule of `closestPair`, `closestPair3` and `minimumPerimeterTriangleApprox` (user decision 2026-11-14, Priority: lighter). The pair scans share `closest_detail::update` and the exact `lll` distances, so ties fall to the smallest ids in any scan order. The triangle scan shares `updateTriangle`, the sorted-triple perimeter expression the divide and conquer evaluates, so both return the same triple. Cost: `O(n^2)` time (`O(n^3)` for triangles) and `O(1)` extra memory (`O(n)` for the triangle's `long double` copy).

## Feature-to-Test Map

`96-Local Testing/03-Geometry/005-closestpair_tester.py` drives the C++ suite and a Python bigint oracle in both dimensions; its docstring states the mode coverage. C++ oracle failures shrink by single-point deletion to a locally minimal input before printing it. Python failures print the seed, the full input, the expected and actual values and the command.

| Operation | Test | Oracle |
|---|---|---|
| `ClosestPair`, `closestPair`, `closestPair3`: empty, singleton, no answer, extreme singletons, duplicate groups, input preserved | Fixed C++ regressions and the Python bigint protocol in every configuration | Expected values; Python bigint |
| `closestPair`: original indices and lexicographic ties | Exhaustive ordered 2x2 lists through length 7; all 512 3x3 subsets in natural and shuffled order; tied-x and tied-y grids | O(n^2) oracle |
| `closestPair`: divide, merge and strip crossings | 1,200 random grid, sparse wide, tied-x and tied-y clouds; shuffles, quarter turns and translations; 500, 50,000 and 250,000-point lattice, vertical line, horizontal line, all-equal and duplicates-off-a-line inputs with known answers, the last within 5 s in the optimized build | O(n^2) oracle |
| `closestPair3`: ties and degeneracies | Exhaustive ordered 2x2x2 lists through length 5 (37,449 lists); 4,000 random 3x3x3 subsets, each also shuffled | O(n^2) oracle |
| `closestPair3`: slab rows and adjacent-row scan | 1,200 random clouds (wide 3e18, dense 13^3, 81^3) including fixed-x planes, fixed-y planes and the slanted `y = x, z = 0` plane, with axis rotations, quarter turns and translations; 64,001-point lattices (cube, plane) and a diagonal line; a 1,000,001-point (1,001 in quick) y-line and two-row plane that put the whole input in one slab, each with a known index answer | O(n^2) oracle; known answer |
| Arithmetic width and domain (2D and 3D) | Python arbitrary-integer cases per configuration (509 in 2D, 505 in 3D), each also through `closestPairNaive` and `closestPair3Naive`: exact distances beyond float precision, near-`2^127` squared span, signed-64-bit endpoint clusters, wide random and duplicate data | Python bigint |
| Coordinate types | `int8_t`, `int16_t`, `int`, `lng` (2D); `int8_t`, `int` (3D); `int` distances above signed 64 bits | O(n^2) oracle |
| `closestPairNaive`, `closestPair3Naive`, `minimumPerimeterTriangleApproxNaive` | Every oracle-checked case of their fast forms (exhaustive, subset and random rounds; 2D typed rounds; 3D `int` and `int8_t` extremes) and, for the pair scans, the Python bigint protocol (`--oracle-naive`, `--oracle3-naive`); probes `naive-span-x` and `naive-diagonal3` | O(n^2) and O(n^3) oracles; Python bigint |
| Type domains (`static_assert`) | Once per full or stress run, 24 `-fsyntax-only` compiles: each operation compiles for its documented coordinate types and fails its `static_assert` for `lll` and for the other number kind (`double` for the exact operations, `int` for `closestPairApprox`) | Compiler verdict and `static assertion failed` |
| `closestPairRandomized`: every seed equals the exact answer | Every 2D `verify` case (ordered 2x2 lists, 3x3 subsets, random tied grids) also runs with seeds 0, `~0` and 1, 4 or 8 more (quick, full, stress); typed rounds (28, 700 or 5,600 per `int8_t`, `int16_t`, `int`, `lng`) with one-axis spans at both ends of the type (about 1.3e19 for `lng`), tight clusters, pools of 1 to 3 repeated points, 5x5 lattices, diagonals and wide clouds; large clouds (random, clustered, extreme span with tiny second spread) and the known-answer shapes, including 50,000 copies of one point | O(n^2) oracle; `closestPair`; known answer |
| `closestPairRandomized`: arithmetic width | `--oracle-randomized`: three seeds per Python bigint 2D case, the extreme-span cases included | Python bigint |
| `closestPairApprox` | Fixed `-0` equal to `0`, and a rounding tie that must return `(0, 1)` although the exact minimum is `(2, 3)`; random rounds (24, 1,200 or 8,000 per `float`, `double`, `long double`) of grids, sub-ulp offsets that tie after rounding, values near `+-1e16`, wide exponents, signed zeros, three kinds of collinear line and far clusters; planted shapes of 500, 50,000 and 250,000 points and as many equal points, the latter within 5 s in the optimized build | Brute force on the same `long double` expression, exact ids; known answer |
| `minimumPerimeterTriangleApprox` | Fixed cases; all ordered lists over a 2x2 grid (length 5, 7, 8) and over a 3-point line (length 7); all 3x3 subsets, and all 4x4 subsets outside quick; random rounds (16, 800 or 6,000 per seven types) of grids, groups of 2 and 3+ duplicates, collinear sets, thin strips, 1e9 and 1e18 clouds and clusters; 2,000 points in quick and 200,000 otherwise (`lng`, `double`, plus `int`, `float` outside quick): a planted triangle in a jittered lattice, a line, duplicate pairs, all-equal points and one point followed by copies of another, each under a 20 s bound in the optimized build | O(n^3) brute force with the identical summation, exact ids; known answer |
| Preconditions | Ten checked-build assertion probes: 2D x span, y span and diagonal; the same three for `closestPairRandomized`; 3D z span and three-axis diagonal; `closestPairNaive` x span and `closestPair3Naive` diagonal. The `n <= INT_MAX` assertion is inspected; the huge allocation is not run | Expected assertion failure |

## Commands and Results

Package runs 2026-11-14 (P008 `/finish`; experiment none): Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.8, GNU++20, beside other sessions' mutation campaigns. Optimized counts include 32 time-bound checks that only the optimized build runs. The library-wide rows are the latest results of those commands (2026-11-10, P274; seed 42, beside the other suites, build cache emptied before each cold run).

| Command | Result |
|---|---|
| `005-closestpair_tester.py --mode full --seed 1` (optimized, checked, ASan/UBSan; GCC 16.2.1) | PASS, 788,266 checks optimized and 788,234 in each other configuration; Python bigint 509 cases (2D) through `closestPair` and `closestPairNaive`, 1,527 randomized answers (509 cases, three seeds) and 505 cases (3D) through `closestPair3` and `closestPair3Naive`; 10 assertion probes; 24 type-domain compiles; optimized run 83.1 s (75.1 s of it the Python oracle and the compiles), ASan/UBSan 107.5 s; `MEMORY peak=2088MB` |
| `CXX=g++-14 005-closestpair_tester.py --mode full --seed 2` (GCC 14.4.1) | PASS, the same counts, probes and compiles; ASan/UBSan run 124.8 s; `MEMORY peak=2169MB` |
| `005-closestpair_tester.py --mode stress --seed 3` (all three configurations) | PASS, 7,770,704 checks optimized and 7,770,672 in each other configuration; Python bigint 4,009 + 4,009 + 12,027 + 4,005 + 4,005 answers; 24 type-domain compiles; ASan/UBSan run 277.7 s; `MEMORY peak=901MB` |
| `05-extraction.py ops --filter 005-closestpair` | PASS: 10 operations in 9 extractions on g++ and g++-14; `closestPairApprox`, which only floating types instantiate, has no default-form candidate |
| `05-extraction.py testers --filter 03-Geometry/0` | PASS, 16 names for this tester, quick on the extracted build |
| `02-integration.py` | PASS |
| `04-mutation.py run 03-Geometry/005-closestpair.hpp` | See Mutation Testing |
| 2026-11-10 P274: `01-run.py --mode full` (GCC 16.2.1) | PASS (counts not recorded); pre-flight quick peak 905 MB; optimized 0.2 s compile + 3.5 s run, checked 33.5 s + 12.0 s, ASan/UBSan 37.3 s + 10.9 s; MEMORY peak 1178 MB |
| 2026-11-10 P274: `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS (counts not recorded); pre-flight quick peak 963 MB; optimized 0.2 s compile + 3.8 s run, checked 36.4 s + 18.1 s, ASan/UBSan 44.1 s + 16.8 s; MEMORY peak 1136 MB |
| 2026-11-10 P274: `01-run.py --mode quick` | PASS (counts not recorded); optimized 33.6 s compile + 0.3 s run, checked 26.2 s + 1.0 s; MEMORY peak 890 MB |
| 2026-11-10 P274: `01-run.py --mode stress --rounds 1` | PASS (counts not recorded); pre-flight quick peak 903 MB; optimized 0.1 s compile + 8.0 s run, checked 12.5 s + 36.4 s, ASan/UBSan 14.1 s + 29.9 s; MEMORY peak 751 MB |

```bash
python3 '96-Local Testing/03-Geometry/005-closestpair_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/03-Geometry/005-closestpair_tester.py' --mode full --seed 2
python3 '96-Local Testing/03-Geometry/005-closestpair_tester.py' --mode stress --seed 3
python3 '96-Local Testing/05-extraction.py' ops --filter 005-closestpair
python3 '96-Local Testing/05-extraction.py' testers --filter 03-Geometry/0
python3 '96-Local Testing/04-mutation.py' run 03-Geometry/005-closestpair.hpp
python3 '96-Local Testing/03-Geometry/005-closestpair_benchmark.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Mutation Testing

Record: mewt 4.0.0 on blob `ea634f4bb5e76291cec1862b06c1b77c69a36267` (2026-11-14, seed 1, 51 lines changed since blob `a135635df565`): 216 mutants, 186 caught (1 by timeout), 30 survived, 0 untested.

Stage times: quick 597 s, full optimized 1,270 s, full checked and ASan/UBSan 2,791 s (baselines 15.3 s, 61.0 s and 128.1 s). This run covered the lines `/finish` changed (the three naive scans, the 3D `fits`, `updateTriangle`, `points`). Four earlier campaigns on 2026-11-14 (a whole-file sample of 500 of 1,458 mutants, then 7, 6 and 11 changed lines) left the survivors marked below on lines no later campaign covered; their other survivors were closed by tests or removed with their code. Every open survivor is equivalent; lines are those of the current header.

| Mutant | Line | Why equivalent |
|---|---|---|
| `AS 297`–`AS 302` | 29, 30 | The 3D `fits` with `min` and `max` arguments commuted: integer coordinates give the same value in either order |
| `COS 951`, `COS 956`, `COS 1471`, `COS 1476` | 39, 40, 270, 271 | A counting loop `<` → `!=` whose index rises by one to a bound it starts at or below (`j < n` with `n >= 2`; `i < j`; `j < k`) |
| `AS 307` | 40 | `update` with its ids commuted orders them through `minmax` and computes the symmetric exact `dist2` |
| `AS 396`, `AS 397`, `AS 425`, `AS 426` | 232, 271 | `updateTriangle` with two ids swapped sorts them first |
| `AS 309`, `AS 310`, `AS 311` | 44 | `distanceApprox` with its points swapped: the difference only changes sign, exactly, so square, sum and root are bitwise equal |
| `COS 972`, `COS 977` | 45 | `v == per` → `<=`: `v < per` is already accepted by the previous disjunct; `t < best` → `<=`: an equal triple has the same computed perimeter, so the state is unchanged |
| `COS 1427` | 255 | Strip filter `<=` → `<`: a triple across the split holds a point on each side, so a point whose x distance from the split is at least `half()` gives it an x span, hence an exact perimeter, of at least `per (1 + 2^-40)`. Here `per > 0` (three equal points return earlier) and the rounding of the perimeter, the difference and `half()` is near `2^-63` relative, so such a triple can neither win nor tie |
| `CR 166`, `CR 211`, `CR 233`; `COS 1096`, `1097`, `1341`, `1342`, `1461`, `1462` | 104, 215, 265 | The naive scans' `n <= INT_MAX` assertions removed or weakened: a distinguishing input needs `2^31` points, beyond every memory cap; the bounds were inspected |
| `COS 913` (whole-file campaign) | 24 | The y `minmax_element` comparator reversed: the span becomes the negated difference, whose square in `ulll` equals the true square modulo `2^128`, so `fits` checks the same value |
| `COS 942` (whole-file) | 36 | `ij < b.ids` → `<=`: equal ids are the same pair at the same distance |
| `IT 229`, `AOS 427` (whole-file) | 56 | The equal-point test made always true: every adjacent pair in (point, id) order becomes a candidate, and extra genuine candidates cannot change the minimum of the total order (distance, ids) |
| `COS 971` (whole-file) | 65 | `q.x == p[0].x` → `<`: the line shortcut is never taken, and the divide and conquer, correct for every input, runs instead |
| `COS 987` (whole-file) | 69 | `b.d == 0` → `<=`: squared distances are non-negative |
| `AOS 551` (whole-file) | 131 | The cell hash `c.first + seed` → `*`: only the probe sequence changes; lookups compare whole keys |
| `COS 1050` (11-line campaign) | 141 | Counting loop `i < n` → `!=` |
| `COS 1089` (7-line campaign) | 142 | Counting loop `c < cells` → `!=` |
| `COS 1136` (whole-file), `COS 1094` (6-line campaign) | 144 | A cell's size `> 1` → `!= 1`: every cell holds at least one point |
| `AOS 578`, `AOS 582` (7-line campaign) | 150 | `cx + dx` → `cx - dx` or `cy + dy` → `cy - dy`: the four offsets become `(-1, -1), (-1, 0), (-1, 1), (0, 1)` or `(1, 1), (1, 0), (1, -1), (0, -1)`, again one of each pair `+-v` of the eight neighbour offsets, so every pair of neighbouring cells is still scanned once; a key below 0 wraps to an index no cell has |
| `COS 1184` (7-line campaign) | 197 | New-row test `key[rows - 1] != q` → `<`: the slab is scanned in y order and truncated division is monotone, so `key[rows - 1] <= q` always holds |
| `COS 1286` (whole-file) | 203 | `t > 1` → `t != 1` where `t >= 1` |
| `AS 368` (whole-file) | 249 | `midpoint(l, r)` → `midpoint(r, l)`: the split rounds up instead of down, and both halves keep at least three points |
| Whole-file campaign, lines not recorded: `AS 278`, `284`, `316`, `327`, `348`, `349`, `363`, `364`, `365`, `381`, `321`, `322` | — | Commuted symmetric arguments: `update` and `dist2` pairs, which sort the ids through `minmax` or `sort`, and `max` of equal-typed coordinates |
| Whole-file campaign, lines not recorded: `COS 961`, `1011`, `1016`, `1111`, `1116`, `1241`, `1341`, `1406`, `1411`, `1436`, `1306`, `1316`, `1142`, `1232` | — | Counting loops whose index rises by one to the bound, `<` → `!=` (`1306`: `j0 != b` with `j0 <= b`; `1316`: `j != b` with `j0 <= b`), and `<` → `<=` where the inner loop then runs no iteration (`1142`, `1232`) |
| Whole-file campaign, lines not recorded: `CR 132`, `CR 182`; `COS 1191`, `1207`, `1331` | — | The fast forms' `n <= INT_MAX` assertions: a distinguishing input needs `2^31` points, beyond every memory cap; the bounds were inspected |

## Benchmarks

`python3 '96-Local Testing/03-Geometry/005-closestpair_benchmark.py'` writes its record (CPU, compiler, flags, seed, one warmup plus five repetitions, medians, dependency hashes, output checksum) to `96-Local Testing/03-Geometry/005-closestpair_benchmark.json`. Sizes are 32, 512, 2,048 and 200,000:

- **2D:** `closestPair` and `closestPairRandomized` (a fresh seed per repetition) against the preserved legacy recurrence (on its safe domain, distance only) and brute force for `n <= 2048`; `closestPairApprox` on `long double` points against a floating brute force.
- **3D:** `closestPair3` against brute force for `n <= 2048`.
- **Triangle:** `minimumPerimeterTriangleApprox` on random, grid and duplicate-pair inputs, against an `O(n^3)` brute force for `n <= 512` and a windowed brute-force check above.

Every timed output is checked. Input generation and checking are excluded; allocation and sorting are included. Peak memory is not measured.

2026-11-14 (P008; experiment none; 113 measurements, seed 20260927, checksum `248219233356`), Intel Core i9-11900H, GCC 16.2.1, `-O2 -DNDEBUG`, load average 2.4 from other sessions:

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

At n = 2,048, 2D random takes 0.31 ms against 5.34 ms for brute force, and 3D random 0.58 ms against 7.56 ms. `closestPairRandomized` overtakes `closestPair` between n = 512 (57.1 µs against 41.0 µs) and n = 2,048 (0.25 ms against 0.31 ms) on random and grid inputs and is about 1.5 to 1.8 times faster at 200,000; on the vertical line, where the sorted-axis early return applies, the deterministic code is about 4 times faster. Both stay, as the row records (expected `O(n)` beside deterministic `O(n log n)`). The legacy code is faster on equal-distance grids because it neither keeps exact 128-bit distances nor resolves index ties. The 3D lattice and plane are `closestPair3`'s worst observed inputs: every point ties at the minimum distance, so the slab scans do their maximal constant work. The triangle is slowest on duplicate pairs (every point of a spacing-10 grid given twice), where many triangles tie near the minimum perimeter.

## Sources

- The 2D recursion follows the archived divide-and-conquer recurrence (bytes unchanged), reworked as described under Contracts; the folder notes list `OLD/Team Notebook/src/geometry/getnearestpair.cpp` as superseded by GE02. The benchmark times the preserved legacy recurrence.
- `closestPairRandomized` follows Rabin's sampled grid as cp-algorithms presents it, `minimumPerimeterTriangleApprox` the divide-and-conquer extension in the same article, and `closestPairApprox` the floating closest pair of suisen and maspypy. The ordered-set sweep, the incremental grid and the other alternatives not adopted are in [00-notes.md](00-notes.md#p008-re-audit-decisions-2026-11-14).

## Limits and Handoffs

Range-restricted closest pair belongs to row 29 (`029-point_set_queries.hpp`, `rangeClosestPair`).

## Aliases

Search aliases (P263, grab --find): nearest pair of points, nearest points (OI Wiki: 平面最近点对), closest pair, 三维最近点对, 分治求最近点对.

Search aliases (P008, grab --find): closest pair randomized, Rabin closest pair, expected linear closest pair, 随机化最近点对, 期望线性最近点对, floating closest pair, minimum perimeter triangle, closest triple, 最小周长三角形, 平面最小周长三角形, brute force closest pair, all pairs closest pair, 暴力最近点对.

## History

- 2026-09-27: original P008, full suite (seed 20260927, 55,373 checks) passed on g++ 16; `closestPair3` missing.
- 2026-10-07: re-audit, implemented `closestPair3`; 4 findings fixed; full suite, stress and benchmark passed on g++ 16; independent review reproduced every full count and probe and confirmed one package defect (in `006-circle.hpp`).
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 added the search aliases (`grab --find`), then common Chinese names (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; inputs and checks unchanged; library-wide runs passed. `[[maybe_unused]]` added on names only an `assert` uses, no logic changed; no mutation campaign (user decision 2026-11-10).
- 2026-11-14: P008 re-audit (experiment none): implemented `closestPairRandomized`, `closestPairApprox` and `minimumPerimeterTriangleApprox` after the completeness sweep; three mutation campaigns (grid-edge guards and the 3D floor correction removed; 3D line, two-row plane and timed duplicate shapes added); independent review: 5 findings fixed (two runtime mutants killed by timed shapes, an unbounded `ceilSqrt` under `NDEBUG` past a violated precondition, replaced by `cellSide`; the triangle's underflow domain; stale text) plus naming and `static_assert` nits; type-domain compiles added; full on g++ and g++-14, stress, extraction, benchmark and `02-integration.py --sanitizers` passed.
- 2026-11-14: `/finish P008` (experiment none): added `closestPairNaive`, `closestPair3Naive` and `minimumPerimeterTriangleApproxNaive` (user decision, a Priority exception recorded in `omissions.md`), with two probes; `closestPair3` asserts its own spans through a 3D `fits`; review fixes (the 2D division trap past a violated `fits`, the 64-bit significand and power-of-two clauses of the `cellSide` argument, stale counts, `std::abs`, two missing type-domain compiles); mutation campaign on the 51 changed lines, 30 equivalent survivors; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
