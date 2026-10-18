# 54-fixed_radius_queries.hpp — evidence

Owned by package P037 / GE44. Every operation in the row has a test.

## Contracts

### Numeric policy

Points are integral with `|coordinate| <= 10^9`, asserted. Distances are compared exactly as `dist2 <= r^2` in `lll`, so disks are closed. `maxPointsInDisk` takes the squared radius `r2` as an integer with `0 <= r2 <= 4 * 10^8`, asserted; the radius is `sqrt(r2)`. Its count is exact and its centre is rounded.

### GridHash

The constructor takes points and a cell size `r` with `1 <= r <= 4 * 10^9` and `n <= 2^29`, all asserted. Cell keys are the floor quotients `(⌊x / r⌋, ⌊y / r⌋)`, packed into 64 bits; every quotient fits `int32`. A `HashMap` from Data Structures row 61 assigns cell ids, and a counting pass groups the points by cell. Build is `O(n)` expected time and `O(n)` memory. `forEachNear(i, f)` is the helper that scans the 3 × 3 block of cells around `p[i]`.

- `neighbours(i)`: the indices `j != i` with `dist2(p[i], p[j]) <= r^2`, in cell order.
- `pairsWithin()`: every pair `i < j` within `r`.
- `diskCoverCounts()`: for each `i`, the number of points within `r` of `p[i]`, itself included. This is the number of radius-`r` disks centred at input points that cover `p[i]`.

**Output sensitivity.** A cell with `c` points splits into four quarter cells of diameter `r / sqrt(2) < r`. One of them holds `c / 4` points, so the cell has `Omega(c^2)` pairs within `r`. Checking two neighbouring cells costs `c1 * c2 <= (c1^2 + c2^2) / 2`. So `pairsWithin` and `diskCoverCounts` cost `O(n + P)` expected for `P` pairs within `r`. A single `neighbours(i)` costs `O(1 + points in the 3 × 3 cells around p[i])`, which is not output-sensitive: with `c` copies of `(19, 19)` and `r = 10`, `neighbours` of `(0, 0)` scans all `c` and returns nothing.

### maxPointsInDisk

Returns `{the largest number of points inside a closed disk of radius sqrt(r2), a rounded centre}`, or `{0, (0, 0)}` for no points. Duplicates count separately, and `r2 = 0` gives the largest multiplicity.

Some optimal disk has an input point `p` on its boundary. For each such pivot `p`, every `q` with `0 < D = |q − p|^2 <= 4 r2` covers an arc of centres on the circle of radius `sqrt(r2)` around `p`. The arc ends are the directions `d * sqrt(D) ± perp(d) * sqrt(X)`, with `d = q − p` and `X = 4 r2 − D`. `GridHash` with cell `ceil(2 * sqrt(r2))` finds the candidates.

**The order of the events** is exact:
- the half-plane class needs the signs of `a sqrt(A) + b sqrt(B)`, by one squaring;
- the cross product is `sqrt(D1) * P + sqrt(X1) * Q`, where `P` and `Q` are such sums in `D2` and `X2`. When their signs differ, `D1 P^2 − X1 Q^2 = 4 r2 * (P^2 − D1 D2 X1)`. This uses `P^2 + Q^2 = 4 r2 D1 D2`, from Lagrange's identity. That leaves one sum `K + L * sqrt(M)` to decide;
- its final squaring compares `K^2` with `(L^2) * M` as 256-bit products of `lll` factors. The radius bound keeps `|K| < 2^127` and `L^2 < 2^127`.

**Ties.** At equal angles, entries come before exits, which is the closed disk. Arcs whose exit sorts before their entry wrap through the starting direction `+x` and start counted.

**The centre** returned is the arc entry where the maximum is reached, rounded, or `p + (√r2, 0)` when the maximum holds at the starting direction (including a pivot without candidates).

Time `O(n + Σ m * log(m))` expected, `m` the candidates of each pivot; `O(n^2 * log(n))` in the worst case. Memory `O(n)`.

Kept by decision (2026-10-18) as the equal-radius special case of row 23 `maxDiskDepth`.

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py` runs quick, full and stress modes in three configurations, with four assertion probes: `r = 0`, a coordinate of `2 * 10^9`, `neighbours` out of range, and `r2` above `4 * 10^8`.

**Oracles.**
- All pairs, for the grid queries.
- For `maxPointsInDisk`, every input point as a centre, and both centres of every pair with `0 < D <= 4 r2`. Each centre is tested against every point by an independent one-radical sign test, `(|d − 2g|^2 + X − 4 r2) * sqrt(D) − 4 e * cross(d, g) * sqrt(X) <= 0`. That test was derived separately from the sweep's comparator.

| Operation | Test | Oracle |
|---|---|---|
| `GridHash` (build) | Every grid case: empty, one point, duplicates, random sets of up to 60 points at scale 12 with `r` in 1–8, and at scale `10^9` with `r` up to `4 * 10^9` | All-pairs scan |
| `neighbours` | Every point of every grid case | All-pairs scan, sorted |
| `pairsWithin` | Every grid case; 60,000 points with `r = 3000` | All-pairs scan; consistency with `diskCoverCounts` |
| `diskCoverCounts` | Same | All-pairs scan |
| `maxPointsInDisk` | Fixed: a point, duplicates, a diameter pair, a pair beyond the diameter, six cocircular points, a square at radii `sqrt(8)` and `sqrt(7)`. Random sets of 1–19 points at scales 6 and 30 with `r2 < 60`, some duplicated. Sets of 4–29 points within `6 * 10^4` shifted up to `5 * 10^8`, with `r2` up to `4 * 10^8`. The radius bound with a far point. 200-point sets. 60,000 points | Exhaustive exact centre search; the rounded centre covers the count up to `10^-12` times the scale |
| Event order | Random events at small radii and at the radius bound | Irreflexive, asymmetric, transitive; agrees with rounded angles separated by more than `10^-12` |

Mutation checks (2026-10-18, quick, optimized; each caught): exits before entries at equal angles; dropping the wrap-around count; inverting the final radical comparison.

## Commands and results

2026-10-18, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `54-fixed_radius_queries_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 4 probes) | PASS, 153,248 checks per configuration, 4 probes, `MEMORY peak=454MB` |
| same with `CXX=g++-14` | PASS, 153,248 checks per configuration, 4 probes, `MEMORY peak=454MB` |
| `54-fixed_radius_queries_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 1,219,823 checks, `MEMORY peak=331MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=650MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

None required by the contest profile. The 60,000-point case runs inside the full suite.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [USACO Guide, LeetCode 1453 solution](https://usaco.guide/problems/lc-maximum-number-of-darts-inside-of-a-circular-dartboard/solution): the angular sweep with floating angles and entries before exits. The exact comparator here is new to the catalogs swept.
- [Chazelle–Lee and later work, arXiv 1312.0389](https://arxiv.org/html/1312.0389v1) and [arXiv 2509.16008](https://arxiv.org/html/2509.16008): the `O(n^2)` bound (optimal under 3SUM) and the weighted, coloured and two-disk variants, none adopted.
- [CGAL Spatial_searching](https://doc.cgal.org/latest/Spatial_searching/index.html): fixed-radius search through kd-trees, owned by row 28.

## Limits and handoffs

- `maxPointsInDisk` is `O(n^2 * log(n))` in the worst case; Chazelle–Lee's `O(n^2)` is not adopted. The radius bound `r2 <= 4 * 10^8` keeps every intermediate in 256-bit products of `lll` factors.
- Radius queries around arbitrary points belong to row 28 `KdTree: radius`. Unit-disk graph components belong to row 22 (`euclideanBottleneck`).
- Not adopted (see 00-notes): weighted and coloured disks, two disks, the smallest disk covering `k` points.

## History

- 2026-10-18: first implementation (P037). Independent review: the per-query `GridHash` bound and the centre contract were corrected, `r2` is clamped under `NDEBUG`, and the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
