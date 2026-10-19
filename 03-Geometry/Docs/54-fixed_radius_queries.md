# 54-fixed_radius_queries.hpp — evidence

Owned by package P037 / GE44. Every operation in the row has a test.

## Contracts

### Numeric policy

Points are integral with `|coordinate| <= 10^9`, asserted. Distances are compared exactly as `dist2 <= r^2` in `lll`, so disks are closed. `maxPointsInDisk` takes the squared radius `r2` as an integer with `0 <= r2 <= 4 * 10^18`, asserted (raised or lowered into that range under `NDEBUG`); the radius is `sqrt(r2)`, so every disk that matters for the coordinate domain is allowed. Its count is exact and its centre is rounded.

### GridHash

The constructor takes points and a cell size `r` with `1 <= r <= 4 * 10^9` and `n <= 2^29`, all asserted. Cell keys are the floor quotients `(⌊x / r⌋, ⌊y / r⌋)`, packed into 64 bits; every quotient fits `int32`. A `HashMap` from Data Structures row 61 assigns cell ids, and a counting pass groups the points by cell. Build is `O(n)` expected time and `O(n)` memory. `forEachNear(i, f)` is the helper that scans the 3 × 3 block of cells around `p[i]`.

- `neighbours(i)`: the indices `j != i` with `dist2(p[i], p[j]) <= r^2`, in cell order.
- `pairsWithin()`: every pair `i < j` within `r`.
- `diskCoverCounts()`: for each `i`, the number of points within `r` of `p[i]`, itself included. This is the number of radius-`r` disks centred at input points that cover `p[i]`.

The three queries are shared with `SortedGrid` (`fixed_radius_detail::neighbours`, `pairsWithin`, `diskCoverCounts` over `forEachNear`).

**Output sensitivity.** A cell with `c` points splits into four quarter cells of diameter `r / sqrt(2) < r`. One of them holds `c / 4` points, so the cell has `Omega(c^2)` pairs within `r`. Checking two neighbouring cells costs `c1 * c2 <= (c1^2 + c2^2) / 2`. So `pairsWithin` and `diskCoverCounts` cost `O(n + P)` expected for `P` pairs within `r` (at most `36 (n + 2P)` distance tests; the hash lookups make it expected). A single `neighbours(i)` costs `O(1 + points in the 3 × 3 cells around p[i])`, which is not output-sensitive: with `c` copies of `(19, 19)` and `r = 10`, `neighbours` of `(0, 0)` scans all `c` and returns nothing.

### SortedGrid

The deterministic variant of `GridHash`, kept beside it by decision (2026-10-19): the same domain (asserted the same way), the same cells and the same results in the same order, with no hashing.
- **Build.** Each cell key packs `⌊x / r⌋ + 2^31` and `⌊y / r⌋ + 2^31` into the high and low 32 bits, so key order is the lexicographic order of the cells and a shift by `(dx, dy)` adds a constant without carries. A stable LSD radix sort (eight 8-bit digits, skipping a digit that every key shares) orders the indices, which keeps the indices of one cell increasing as `GridHash` does. Equal keys become cells with start offsets. For each of the 9 shifts `(dx, dy)`, the shifted keys are increasing in cell order, so one monotone pointer finds every neighbouring cell. `O(n)` time (`8 (n + 256)` steps for the sort) and `O(n)` memory (9 ints per cell).
- **Queries.** `forEachNear(i, f)` scans the 9 table entries of `p[i]`'s cell in the order `GridHash` uses (`dx` outer, `dy` inner), so `neighbours`, `pairsWithin` and `diskCoverCounts` return exactly `GridHash`'s vectors. `neighbours(i)` is `O(1 + points in the 3 × 3 cells)`, and `pairsWithin` (`O(n + out)`) and `diskCoverCounts` (`O(n + P)`) are output-sensitive by the argument above, deterministically.
- It is 1.5–2.5× faster than `GridHash` at the measured sizes (see Benchmarks), so `maxPointsInDisk` uses it.

### maxPointsInDisk

Returns `{the largest number of points inside a closed disk of radius sqrt(r2), a rounded centre}`, or `{0, (0, 0)}` for no points. Duplicates count separately, and `r2 = 0` gives the largest multiplicity.

Some optimal disk has an input point `p` on its boundary. For each such pivot `p`, every `q` with `0 < D = |q − p|^2 <= 4 r2` covers an arc of centres on the circle of radius `sqrt(r2)` around `p`. The arc ends are the directions `d * sqrt(D) ± perp(d) * sqrt(X)`, with `d = q − p` and `X = 4 r2 − D`. `SortedGrid` with cell `s = ceil(2 * sqrt(r2))` (at least 1) finds the candidates; `GridHash` would scan them in the same order, so the result is the same with either grid.

**The order of the events** is exact:
- the half-plane class needs the signs of `a sqrt(A) + b sqrt(B)`, by one squaring;
- the cross product is `sqrt(D1) * P + sqrt(X1) * Q`, where `P` and `Q` are such sums in `D2` and `X2`. When their signs differ, `D1 P^2 − X1 Q^2 = 4 r2 * (P^2 − D1 D2 X1)`. This uses `P^2 + Q^2 = 4 r2 D1 D2`, from Lagrange's identity. That leaves one sum `K + L * sqrt(M)` to decide;
- its final squaring compares `K^2` with `L^2 * M`.

**Two arithmetic paths**, chosen by `r2`. Write `q = 4 r2`, `c = cross(d1, d2)`, `t = dot(d1, d2)`, `K = c^2 D2 + t^2 X2 − D1 D2 X1`, `L = 2 e c t` and `M = D2 X2`. For every event `0 < D <= q` and `0 <= X = q − D < q`; by Cauchy–Schwarz `|c|, |t| <= sqrt(D1 D2)`, and by Lagrange's identity `c^2 + t^2 = D1 D2`, so `|L| <= D1 D2`.
- **`r2 <= 2^30`: `lll`.** Then `q <= 2^32`.
  - The half-plane products are at most `D^2` and `D X <= q^2 / 4`.
  - The sums `c^2 D2`, `t^2 X2` and `D1 D2 X1` are at most `q^3 <= 2^96`, so `|K| < 2^97`.
  - The final comparison runs only when `M > 0`, so `X2 > 0`, `D2 <= q − 1` and `|L| <= q (q − 1) < 2^64`. So `L^2 < 2^128` fits `ulll`, and one 256-bit comparison of `|K| * |K|` with `L^2 * M` decides.
  - The bound is tight for this arithmetic: at `r2 = 2^30 + 1`, `q (q − 1)` exceeds `2^64`.
- **`r2 > 2^30`: 384-bit magnitudes** (`Mag`, six limbs with `mag`, `add`, `less` and a truncated `mul`). Coordinates within `10^9` give `D <= 8 * 10^18 < 2^63`; `q <= 1.6 * 10^19 < 2^64`.
  - `c^2`, `t^2`, `D1 D2`, `|L|` and `M <= q^2 / 4` are below `6.4 * 10^37 < 2^126` and stay in `lll`; the half-plane test stays in `lll` (products at most `6.4 * 10^37`).
  - Every product `a^2 x` of the radical signs is at most `6.4 * 10^37 * 1.6 * 10^19 < 2^190`, three limbs from one `ulll × ulng` product.
  - `K` is the difference of `c^2 D2 + t^2 X2 < 2^191` and `D1 D2 X1 < 2^190`, compared before subtracting, so `|K| < 2^191`; `K^2 < 2^382` and `L^2 M < 2^252 * 2^126 = 2^378`, so six-limb truncated products are exact.
  - On the same inputs the 384-bit path costs 1.2–1.8× the `lll` path (see Benchmarks).
  - The `lll` path's 128 × 128-bit product comparison (`compareProducts`, six lines) stays private too: Core `u256` took 4.7× longer on `10^7` random comparisons (615–636 ms against 132–135 ms).
  - Core's `UInt<3>` and `UInt<6>` (`01-Core/07-infint.hpp`) were measured for this path and rejected: with them the 384-bit path was 1.6–2.6× slower end to end than with the private magnitudes (two interleaved runs, all ten workloads).

**Ties.** At equal angles, entries come before exits, which is the closed disk. Arcs whose exit sorts before their entry wrap through the starting direction `+x` and start counted.

**The centre** returned is the arc entry where the maximum is reached, rounded, or `p + (√r2, 0)` when the maximum holds at the starting direction (including a pivot without candidates).

**Time `O(n * K * log(K + 1))`, `K` the answer**, deterministic; `O(n^2 * log(n))` in the worst case, since `K <= n`. Memory `O(n)`.
- A closed square of side `sqrt(2 r2)` lies in the closed disk of radius `sqrt(r2)` around its centre, so it holds at most `K` points.
- For `r2 >= 1`, the 3 × 3 cells around a pivot form a square of side `3 s <= 6 sqrt(r2) + 3`, covered by at most `⌈(6 sqrt(r2) + 3) / sqrt(2 r2)⌉^2 <= 49` such squares (25 once `sqrt(r2) >= 3`). For `r2 = 0`, `s = 1` and the block holds 9 lattice positions, each of multiplicity at most `K`.
- So each pivot scans at most `49 K` points, sorts at most `98 K` events in `O(K * log(K + 1))`, and the radix-sorted grid costs `O(n)`. Since `K >= 1` when `n >= 1`, the total is `O(n * K * log(K + 1))`.

Kept by decision (2026-10-18) as the equal-radius special case of row 23 `maxDiskDepth`.

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py` runs quick, full and stress modes in three configurations, with seven assertion probes: for `GridHash` and for `SortedGrid`, `r = 0`, a coordinate of `±2 * 10^9` and `neighbours` out of range; and `r2 = 4 * 10^18 + 1`.

**Oracles.**
- All pairs, for the grid queries.
- For `maxPointsInDisk`, every input point as a centre, and both centres of every pair with `0 < D <= 4 r2`. Each centre is tested against every point by an independent one-radical sign test, evaluated with the tester's own 256-bit magnitudes, `(|d − 2g|^2 + X − 4 r2) * sqrt(D) − 4 e * cross(d, g) * sqrt(X) <= 0`. That test was derived separately from the sweep's comparator.

| Operation | Test | Oracle |
|---|---|---|
| `GridHash` (build) | Every grid case: empty, one point, duplicates, random sets of up to 60 points at scale 12 with `r` in 1–8, and at scale `10^9` with `r` up to `4 * 10^9`; dense clusters within `r / 2` beside clusters just beyond `r` in the next cell, at both coordinate ends | All-pairs scan |
| `neighbours` | Every point of every grid case | All-pairs scan, sorted |
| `pairsWithin` | Every grid case; 60,000 points with `r = 3000` | All-pairs scan; consistency with `diskCoverCounts` |
| `diskCoverCounts` | Same | All-pairs scan |
| `SortedGrid` (build, `neighbours`, `pairsWithin`, `diskCoverCounts`) | Every grid case above, and the 60,000 points | All-pairs scan; the same vectors as `GridHash`, element by element and in order |
| `maxPointsInDisk` | Fixed: a point, duplicates, a diameter pair, a pair beyond the diameter, six cocircular points, a square at radii `sqrt(8)` and `sqrt(7)`. Random sets of 1–19 points at scales 6 and 30 with `r2 < 60`, some duplicated. Sets of 4–29 points within `6 * 10^4` shifted up to `5 * 10^8`, with `r2` up to `4 * 10^8`. Sets of 3–25 points at scales `10^3`, `10^6` and `10^9` with `r2` up to `4 * 10^18` (the 384-bit path). Both sides of the dispatch threshold, `r2 = 2^30` and `2^30 + 1`, on random sets and on diameter pairs. Rings of 14 points just inside the diameter around a pivot at the origin or at the corner `(−10^9, −10^9)`, at `r2 = 2^30`, `2^30 + 1`, `2^31`, random values in `[9 * 10^8, 2^30]` and up to `4 * 10^18`. The 32 lattice points of `x^2 + y^2 = 1105` scaled by 1000 and shifted (`r2 = 1.105 * 10^9`, all on the circle) and one below. The acute triangle `(33, 4), (−24, 23), (−9, −32)` scaled by 1, 1000 and `3 * 10^7`, at its circumradius squared (3 points) and one below (2). The four corners of the `10^9` square plus two points at `r2 = 2 * 10^18 − 1`, `2 * 10^18` and `4 * 10^18`. The old bound `r2 = 4 * 10^8` with a far point. 200-point sets. 60,000 points | Exhaustive exact centre search; the rounded centre covers the count up to `10^-12` times the scale; the known counts |
| Event order | Random events at small radii, at `r2 <= 4 * 10^8` and at `r2` up to `4 * 10^18` with differences up to `2 * 10^9`; events within 64 of the largest offset (offset length about `min(2 sqrt(r2), 2 sqrt(2) * 10^9)`) at `r2 = 2^30`, `2^30 + 1`, `2 * 10^18` and `4 * 10^18` | Irreflexive, asymmetric, transitive (384-bit); the `lll` and 384-bit comparators agree for every `r2 <= 2^30`; agrees with rounded angles separated by more than `10^-12` |

Both final comparisons are reached: an instrumented full run decided 4.7 million `lll` comparisons (557 exact ties) and 346,000 384-bit ones, and the cocircular and acute-triangle cases add exact 384-bit ties (6,716 in quick mode).

Mutation checks (2026-10-18 and 2026-10-19, on a copy of the tree, optimized unless noted; caught unless noted):
- Sweep: exits before entries at equal angles; dropping the wrap-around count; inverting the final radical comparison.
- 384-bit path: `add` dropping the carry; subtraction dropping the borrow (caught by the acute triangle); `mul` dropping the carry; `less` from the low limb; `mag` dropping the low carry; swapped radical-sign results; a final tie read as `+1` (acute triangle); the sign of `K` flipped.
- `lll` path: `L^2` formed in signed `lll` (undefined behaviour; caught by UBSan in the full ASan/UBSan run).
- `SortedGrid`: `dy` outer instead of `dx`; skipping every radix pass; sorting only the low 32 bits; `<=` in the neighbour pointer; an unbiased `y` key.
- Not caught: the dispatch moved to `r2 > 2^31`. Two million adversarial near-diameter event pairs showed no `lll` and 384-bit disagreement at `r2 <= 2^31` (the `lll` overflow never changed a decision there) and 1,818 at `2^32`; the shipped threshold is the proven bound, not the empirical one.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `54-fixed_radius_queries_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 7 probes) | PASS, 533,011 checks per configuration, 7 probes, `MEMORY peak=575MB` |
| same with `CXX=g++-14` | PASS, 533,011 checks per configuration, 7 probes, `MEMORY peak=574MB` |
| `54-fixed_radius_queries_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 4,281,677 checks, `MEMORY peak=341MB` |
| `-Wall -Wextra -Wconversion` build of the tester and the benchmark with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 168 headers, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py --braces` on the header, tester and benchmark | No violations (comment share 15 of 191 lines) |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`54-fixed_radius_queries_benchmark.py --seed 20260927` (2026-10-19; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; run serially on the final code; other sessions kept the load average near 2.2 on the 8-core CPU).
- **Scaled:** uniform points use `r2 = 10^8` (the `lll` path), in a square sized for about `m` candidates within `2 * r` of each pivot, at most `10^6` wide so the scaled copy stays within `10^9`. Only row (8,000, 10) reaches the cap, which raises its expected candidates from 10 to 10.05. The same points times 1000 with `r2` times `10^6` take the 384-bit path; both scales must report the same count.
- **Threshold:** the same points at `r2 = 2^30` (`lll`) and `2^30 + 1` (384-bit).

| Case | n | m | Count | `lll` (µs) | 384-bit (µs) |
|---|---|---|---|---|---|
| scaled | 500 | 10 | 9 | 3,089 | 3,714 |
| scaled | 500 | 50 | 23 | 20,496 | 37,006 |
| scaled | 2,000 | 10 | 11 | 12,406 | 17,324 |
| scaled | 2,000 | 50 | 29 | 91,049 | 154,947 |
| scaled | 8,000 | 10 | 13 | 48,134 | 67,847 |
| scaled | 8,000 | 50 | 30 | 369,356 | 628,947 |
| threshold | 2,000 | 10 | 11 | 11,605 | 16,195 |
| threshold | 2,000 | 50 | 29 | 89,367 | 150,124 |
| threshold | 8,000 | 10 | 14 | 47,727 | 66,779 |
| threshold | 8,000 | 50 | 31 | 370,228 | 613,692 |

**Grids:** `r = 1000`, uniform points with about 8 others within `r`, or clusters of 10 inside a 300 × 300 square; both grids on the same points, which must give identical pair lists and cover counts.

| Points | n | Pairs | `GridHash` build + `diskCoverCounts` (µs) | `SortedGrid` build + `diskCoverCounts` (µs) | `GridHash` `pairsWithin` (µs) | `SortedGrid` `pairsWithin` (µs) |
|---|---|---|---|---|---|---|
| uniform | 100,000 | 397,934 | 58,723 | 36,511 | 66,696 | 35,378 |
| clustered | 100,000 | 851,222 | 50,216 | 31,131 | 52,704 | 34,116 |
| uniform | 1,000,000 | 3,994,071 | 2,037,991 | 819,841 | 1,997,469 | 1,067,133 |
| clustered | 1,000,000 | 8,522,937 | 1,423,075 | 832,527 | 1,699,453 | 817,693 |

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [USACO Guide, LeetCode 1453 solution](https://usaco.guide/problems/lc-maximum-number-of-darts-inside-of-a-circular-dartboard/solution): the angular sweep with floating angles and entries before exits. The exact comparator here is new to the catalogs swept.
- [Chazelle–Lee and later work, arXiv 1312.0389](https://arxiv.org/html/1312.0389v1) and [arXiv 2509.16008](https://arxiv.org/html/2509.16008): the `O(n^2)` bound (optimal under 3SUM) and the weighted, coloured and two-disk variants, filed as row 56 (P233).
- [CGAL Spatial_searching](https://doc.cgal.org/latest/Spatial_searching/index.html): fixed-radius search through kd-trees, owned by row 28.

## Limits and handoffs

- `maxPointsInDisk` is `O(n * K * log(K + 1))`, `O(n^2 * log(n))` in the worst case; Chazelle–Lee's `O(n^2)` and the output-sensitive `O(n * K)` are planned as row 56 `maxPointsInDiskQuadratic` and `maxPointsInDiskOutputSensitive` (P233). Radii above `sqrt(2^30)` pay the 1.2–1.8× 384-bit constant.
- Radius queries around arbitrary points belong to row 28 `KdTree: radius`. Unit-disk graph components belong to row 22 (`euclideanBottleneck`).
- Weighted and coloured disks, two disks and the smallest disk covering `k` points are row 56 (`56-circle_placement.hpp`, P233, planned).

## History

- 2026-10-18: first implementation (P037). Independent review: the per-query `GridHash` bound and the centre contract were corrected, `r2` is clamped under `NDEBUG`, and the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit widened `maxPointsInDisk` to `r2 <= 4 * 10^18` with an `I512` comparator. Independent review: the benchmark left the coordinate domain (side now capped), threshold tests added. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19 (round 3): 384-bit comparator instead of `I512` (4.4–5.1× faster), `lll` threshold raised to `r2 <= 2^30` with a proof, bound restated as `O(n * K * log(K + 1))`, deterministic `SortedGrid` added and used by `maxPointsInDisk`; Core `UInt` measured and rejected (1.6–2.6× slower); near-diameter, cocircular and acute-triangle tests; 15 mutants, one practically equivalent. Full suite passed on g++ and g++-14, stress passed. Independent review: the benchmark compares the full cover-count vectors; the private 128 × 128-bit product kept after Core `u256` measured 4.7× slower; stale wording fixed. Final full suite passed on g++ and g++-14, stress passed.
