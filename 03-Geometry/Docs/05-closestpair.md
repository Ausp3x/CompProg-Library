# 05-closestpair.hpp — evidence

Owned by package P008 / GE02 (closest pair, circles, calipers and transforms, together with `06-circle.hpp`, `07-rotatingcalipers.hpp` and `08-coordinate_transform.hpp`); see [00-notes.md](00-notes.md#p008--ge02-package-record) for the package record and the folder numeric policy. The 2026-10-07 re-audit implemented `closestPair3`, which the row had left `missing`. Every operation in the row has a test with an independent oracle.

## Contracts

### Numeric policy

The GE02 numeric policy in [00-notes.md](00-notes.md#ge02-numeric-policy-p008) applies. `closestPair` and `closestPair3` are exact APIs: they use signed integral coordinates, and every predicate widens to `lll` before arithmetic. No epsilon, floating arithmetic or randomization is involved.

### ClosestPair, closestPair

`closestPair(const vector<Point2<T>> &)` returns `ClosestPair { pair<int,int> ids; lll distance2; }`. `T` is a signed integral type of at most 64 bits; the tests instantiate `int8_t`, `int16_t`, `int` and `lng`. The input is preserved. `ids` are original indices, in increasing order, and the lexicographically smallest index pair wins every squared-distance tie. Fewer than two points return `ids = {-1, -1}, distance2 = 0`. A duplicate pair is a valid answer with zero distance, distinguished from the no-answer case by its indices.

The size precondition is `n <= INT_MAX`. The squared bounding-box diagonal must be at most `2^127 - 1`. `closest_detail::fits` asserts this with unsigned 128-bit subtraction, so the check cannot overflow even for a fully spread signed-64-bit input. Differences widen before subtraction, so every evaluated distance fits signed `lll`. Coordinates may use the full signed-64-bit storage range: extreme singletons and tight clusters near either end work. This is a span restriction, not support for every possible signed-64-bit point set. Under `-DNDEBUG` the bounds remain preconditions.

Worst case: deterministic `O(n * log(n))` time and `O(n)` auxiliary memory (two `int` arrays plus `O(log(n))` stack). The initial sort by (point, original index) reproduces a stable (x, y) sort while keeping `std::sort`'s worst-case bound; the y merges preserve that order on equal y.

Correctness: in the sorted (point, index) order, each group of equal points is contiguous and its two smallest indices are adjacent. Scanning adjacent equal pairs therefore yields the lexicographically smallest zero-distance answer. That prepass returns immediately, so no strip ever meets arbitrarily many tied zero-distance pairs. On a horizontal or vertical line, an optimum is an adjacent pair in coordinate order, so the 2D function scans adjacent pairs and returns.

The 2D recursion splits the fixed x order. Brute-force leaves settle their pairs, and each return produces y order by a linear merge. A cross-half pair at least as good as the current answer lies in the closed x strip and in the closed y window. Both filters reject only `>` the current squared distance, so equal-distance pairs stay eligible for the index tie rule. Each half has no pair closer than the global best before the merge, and the half-square packing argument bounds the candidates per point by a constant. Shrinking the global best during the scan cannot invalidate an earlier exclusion.

The 2D implementation follows the archived divide-and-conquer recurrence, but reworks its arithmetic domain, no-answer value, stable sort, duplicate handling and ties. It avoids the archived finite INF64 sentinel, 64-bit squaring/subtraction overflow and unspecified pair order. The original archived bytes are unchanged.

### closestPair3

`closestPair3(const vector<Point3<T>> &)` has the same result type, tie rule, input preservation, no-answer value and type domain as `closestPair`. The asserted span condition is `dx^2 + dy^2 + dz^2 <= 2^127 - 1`. Its worst case is deterministic `O(n * log(n))` time and `O(n)` memory: six `int` arrays and one `lll` row-key array, plus `O(log(n))` stack. No hashing or randomization is used.

Correctness: the same recursion maintains two merged orders, by y and by z. At a merge, fix `d` as the current best (`d >= 1` after the prepass) and an integer `s` with `sqrt(d) <= s < sqrt(d) + 2`. Slab points satisfy `(x - x_m)^2 <= d`. Bucket the slab into rows `floor(y / s)`: ranks come from the y order, and a stable counting sort of the z order makes every row z-sorted in `O(slab)`. A pair within `sqrt(d)` has `|dy| <= s`, so it lies in one row or in two adjacent rows. Each point back-scans its own row while `dz^2 <= best`, and a monotone pointer scans the previous adjacent row for `|dz| <= s`. Every point on one side is at least `sqrt(d)` from every other point on its side, so a `2 sqrt(d) x 4 sqrt(d) x 4 sqrt(d)` box holds `O(1)` of them, and both scans examine `O(1)` points each. Each level is `O(n)`, which gives `O(n * log(n))` after the initial sort. This matches the `Omega(n * log(n))` algebraic decision-tree lower bound, deterministically.

## Feature-to-test map

`96-Local Testing/03-Geometry/05-closestpair_tester.py` drives the C++ suite and the Python bigint oracle in both dimensions. Mode coverage is stated in the entry's docstring. C++ oracle failures shrink by single-point deletion to a locally minimal input before printing it. Python failures print the seed, the full input, the expected and actual values and the command. Assertions are never the oracle.

| Operation | Test | Oracle |
|---|---|---|
| `ClosestPair`, `closestPair`, `closestPair3`: empty/singleton/no-answer, extreme singletons, duplicate groups, input preserved | Fixed C++ regressions and the Python bigint protocol in every configuration | Expected values; Python bigint |
| `closestPair`: original indices and lexicographic ties | Exhaustive ordered 2x2 lists through length 7; all 512 3x3 subsets in natural and shuffled order; tied-x/tied-y grids | O(n^2) oracle |
| `closestPair`: divide/merge strip crossings | 1,200 random grid, sparse wide, tied-x and tied-y clouds; shuffles, quarter turns and translations | O(n^2) oracle |
| `closestPair3`: ties and degeneracies | Exhaustive ordered 2x2x2 lists through length 5 (37,449 lists); 4,000 random 3x3x3 subsets, each also shuffled | O(n^2) oracle |
| `closestPair3`: slab rows and adjacent-row scan | 1,200 random clouds (wide 3e18, dense 13^3, 81^3) including fixed-x planes, fixed-y planes and the slanted `y = x, z = 0` plane, with axis rotations, quarter turns and translations; 64,001-point lattices (cube, plane) and a diagonal line with a known index answer | O(n^2) oracle; known answer |
| Arithmetic width/domain (2D and 3D) | 505 Python arbitrary-integer cases per dimension per configuration: exact distances beyond float precision, near-`2^127` squared span, signed-64-bit endpoint clusters, wide random and duplicate data | Python bigint |
| Coordinate types | `int8_t`, `int16_t`, `int`, `lng` (2D); `int8_t`, `int` (3D); `int` distances above signed 64 bits | O(n^2) oracle |
| Preconditions | Five checked-build assertion probes: 2D x span, y span and diagonal; 3D z span and three-axis diagonal. The `n <= INT_MAX` assertion is inspected; the huge allocation is not run | Expected assertion failure |

## Commands and results

### 2026-09-27 (original P008)

The full suite passed optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan with leak checking, at seed `20260927`, with 55,373 checks per configuration. The row was incomplete then (`closestPair3` missing).

### 2026-10-07 re-audit

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 is not installed, so no GCC 14 run is claimed. No online submission was made. The baseline full run before any edit reproduced the 2026-09-27 count exactly.

| Entry | Full, seed `20260927` (optimized, checked, ASan/UBSan) | Stress, seed `42`, optimized |
|---|---|---|
| `05-closestpair_tester.py` | PASS, 157,493 checks per configuration; 505 + 505 Python bigint cases (2D + 3D); 5 assertion probes | PASS, 1,633,673 checks; 4,005 + 4,005 Python cases |

Also run:

- Mutation checks (each applied temporarily, the optimized full suite run, the header restored). Caught: the 3D adjacent-row test and slab filter. Survived, and equivalent: truncating instead of flooring the row index (rows merge, which changes only the sparsity constant) and `<` for the cross-row `|dz| <= s` bound (such a pair is farther than `sqrt(d)`).
- A warning sweep under `-Wall -Wextra -Wconversion`, instantiating every operation for `int8_t`, `int16_t`, `int` and `lng`, found no header warnings.
- A two-translation-unit program mixing all four GE02 headers passed optimized and ASan/UBSan, and the header compiles alone.
- The sanitized integration (102 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) passed. The geometry quick run and the repository consistency check passed, and `03-consistency.py --braces` is clean on the header, tester and benchmark.
- Independent review (`@reviewer`) reproduced the full count and probes, and its own 400k random 3D clouds under ASan/UBSan found no mismatch.

```bash
python3 '96-Local Testing/03-Geometry/05-closestpair_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/05-closestpair_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/03-Geometry/05-closestpair_benchmark.py'
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[5-8]*.hpp '96-Local Testing/03-Geometry/'0[5-8]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

| # | Finding | Disposition |
|---|---|---|
| 3 | `closestPair3` absent; evidence claimed nothing owned remained | Implemented as a deterministic `O(n * log(n))` slab-row divide and conquer, tested against an O(n^2) oracle and Python bigints; text rewritten |
| 8 | `ClosestPair` lacked a complexity comment | `// T: O(1), M: O(1).` line added |
| 9 | `closestPair` lambdas closed on their own line | Normalized; `03-consistency.py --braces` is clean |
| 13 | Tester function bodies closed on their own line | Tester and benchmark normalized |

Other changes: the contract comment blocks moved from the header into `## Contracts`; the header meets the two-line and 8% comment caps, and grouped functions share one complexity line, as in the P007 restyle. `closestPair` shares `closest_detail::{fits, update, sorted}` with `closestPair3`; its behavior and test counts are unchanged. No public name or documented behavior changed, apart from the addition.

## Benchmarks

`python3 '96-Local Testing/03-Geometry/05-closestpair_benchmark.py'` writes the record to `96-Local Testing/03-Geometry/05-closestpair_benchmark.json`. The record holds CPU, compiler, flags, seed, one warmup plus five repetitions, medians, dependency hashes and the output checksum. The benchmark covers sizes 32, 512, 2,048 and 200,000:

- **2D:** `closestPair` against the preserved legacy recurrence (on its safe domain, distance only) and brute force for `n <= 2048`.
- **3D:** `closestPair3` against brute force for `n <= 2048`.

Every timed output is checked. Input generation and checking are excluded; allocation and sorting are included. Peak memory is not measured.

Rerun 2026-10-07 (72 measurements, checksum `316767797226`), on an Intel Core i9-11900H with GCC 16.2.1, `-O2 -DNDEBUG`:

| n = 200,000 | median (ms) | compared |
|---|---:|---|
| 2D random | 50.56 | legacy 35.81 |
| 2D grid | 53.21 | legacy 26.18 |
| 2D vertical | 4.47 | legacy 13.66 |
| 2D duplicates | 5.81 | legacy 8.97 |
| 3D random | 92.06 | — |
| 3D lattice | 141.81 | — |
| 3D plane | 132.11 | — |
| 3D duplicates | 6.13 | — |

At n = 2,048, 2D random takes 0.31 ms against 4.96 ms for brute force, and 3D random 0.63 ms against 7.83 ms. The legacy code is faster on equal-distance grids because it neither keeps exact 128-bit distances nor resolves index ties. The lattice and plane inputs are the 3D worst observed: every point is tied at the minimum distance, so the slab scans do their maximal constant work.

## Sources

- The 2D recursion follows the archived divide-and-conquer recurrence (bytes unchanged), reworked as described under Contracts; the folder notes list `OLD/Team Notebook/src/geometry/getnearestpair.cpp` as superseded by GE02. The benchmark times the preserved legacy recurrence.
- Completeness-sweep candidates not adopted (minimum-perimeter triangle, randomized grid, floating closest pair) are in [00-notes.md](00-notes.md#p008-completeness-decisions-2026-10-07).

## Limits and handoffs

No failing reproducer and no unfinished owned operation. Range-restricted closest pair belongs to row 29 (`29-point_set_queries.hpp`).
