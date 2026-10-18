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

**Output sensitivity.** A cell with `c` points splits into four quarter cells of diameter `r / sqrt(2) < r`. One of them holds `c / 4` points, so the cell has `Omega(c^2)` pairs within `r`. Checking two neighbouring cells costs `c1 * c2 <= (c1^2 + c2^2) / 2`. So `pairsWithin` and `diskCoverCounts` cost `O(n + P)` expected for `P` pairs within `r`. A single `neighbours(i)` costs `O(1 + points in the 3 × 3 cells around p[i])`, which is not output-sensitive: with `c` copies of `(19, 19)` and `r = 10`, `neighbours` of `(0, 0)` scans all `c` and returns nothing.

### maxPointsInDisk

Returns `{the largest number of points inside a closed disk of radius sqrt(r2), a rounded centre}`, or `{0, (0, 0)}` for no points. Duplicates count separately, and `r2 = 0` gives the largest multiplicity.

Some optimal disk has an input point `p` on its boundary. For each such pivot `p`, every `q` with `0 < D = |q − p|^2 <= 4 r2` covers an arc of centres on the circle of radius `sqrt(r2)` around `p`. The arc ends are the directions `d * sqrt(D) ± perp(d) * sqrt(X)`, with `d = q − p` and `X = 4 r2 − D`. `GridHash` with cell `ceil(2 * sqrt(r2))` finds the candidates.

**The order of the events** is exact:
- the half-plane class needs the signs of `a sqrt(A) + b sqrt(B)`, by one squaring;
- the cross product is `sqrt(D1) * P + sqrt(X1) * Q`, where `P` and `Q` are such sums in `D2` and `X2`. When their signs differ, `D1 P^2 − X1 Q^2 = 4 r2 * (P^2 − D1 D2 X1)`. This uses `P^2 + Q^2 = 4 r2 D1 D2`, from Lagrange's identity. That leaves one sum `K + L * sqrt(M)` to decide;
- its final squaring compares `K^2` with `L^2 * M`.

**Two arithmetic paths.** The comparator is a template over its integer type.
- For `r2 <= 4 * 10^8` it runs in `lll`, and the final comparison uses one 256-bit product of `lll` factors. With `R^2 = 4 r2 <= 1.6 * 10^9`, `|K| < 2^127` and `L^2 < 2^127`.
- Larger radii use `I512`, a fixed 512-bit two's-complement integer with `+`, `−`, truncated `×` and signed comparison.
  - The bases are bounded: `D <= 8 * 10^18` and `|c|, |t| <= 8 * 10^18` (coordinate differences are at most `2 * 10^9`), and `X <= 1.6 * 10^19`.
  - Then `|K| <= 3 * 10^57`, `K^2` and `L^2 * M` stay below `2^384`, and truncation mod `2^512` is exact.
  - The half-plane test stays in `lll` because its products are at most `6.4 * 10^37`.
- The threshold is the `lll` path's own bound. On identical scaled inputs the `I512` path is 6.5–10× slower (see Benchmarks).

**Ties.** At equal angles, entries come before exits, which is the closed disk. Arcs whose exit sorts before their entry wrap through the starting direction `+x` and start counted.

**The centre** returned is the arc entry where the maximum is reached, rounded, or `p + (√r2, 0)` when the maximum holds at the starting direction (including a pivot without candidates).

Time `O(n + Σ m * log(m))` expected, `m` the candidates of each pivot; `O(n^2 * log(n))` in the worst case. Memory `O(n)`.

Kept by decision (2026-10-18) as the equal-radius special case of row 23 `maxDiskDepth`.

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py` runs quick, full and stress modes in three configurations, with four assertion probes: `r = 0`, a coordinate of `2 * 10^9`, `neighbours` out of range, and `r2 = 4 * 10^18 + 1`.

**Oracles.**
- All pairs, for the grid queries.
- For `maxPointsInDisk`, every input point as a centre, and both centres of every pair with `0 < D <= 4 r2`. Each centre is tested against every point by an independent one-radical sign test, evaluated with the tester's own 256-bit magnitudes, `(|d − 2g|^2 + X − 4 r2) * sqrt(D) − 4 e * cross(d, g) * sqrt(X) <= 0`. That test was derived separately from the sweep's comparator.

| Operation | Test | Oracle |
|---|---|---|
| `GridHash` (build) | Every grid case: empty, one point, duplicates, random sets of up to 60 points at scale 12 with `r` in 1–8, and at scale `10^9` with `r` up to `4 * 10^9` | All-pairs scan |
| `neighbours` | Every point of every grid case | All-pairs scan, sorted |
| `pairsWithin` | Every grid case; 60,000 points with `r = 3000` | All-pairs scan; consistency with `diskCoverCounts` |
| `diskCoverCounts` | Same | All-pairs scan |
| `maxPointsInDisk` | Fixed: a point, duplicates, a diameter pair, a pair beyond the diameter, six cocircular points, a square at radii `sqrt(8)` and `sqrt(7)`. Random sets of 1–19 points at scales 6 and 30 with `r2 < 60`, some duplicated. Sets of 4–29 points within `6 * 10^4` shifted up to `5 * 10^8`, with `r2` up to `4 * 10^8`. The old radius bound with a far point. Sets of 3–25 points at scales `10^3`, `10^6` and `10^9` with `r2` up to `4 * 10^18` (the `I512` path). Both sides of the dispatch threshold, `r2 = 4 * 10^8` and `4 * 10^8 + 1`. The four corners of the `10^9` square plus two points at `r2 = 2 * 10^18 − 1`, `2 * 10^18` and `4 * 10^18`. 200-point sets. 60,000 points | Exhaustive exact centre search; the rounded centre covers the count up to `10^-12` times the scale |
| Event order | Random events at small radii, at `r2 <= 4 * 10^8` and at `r2` up to `4 * 10^18` with differences up to `2 * 10^9` | Irreflexive, asymmetric, transitive (`I512`); the `lll` and `I512` comparators agree wherever both apply; agrees with rounded angles separated by more than `10^-12` |

Mutation checks (2026-10-19, quick, optimized; each caught): exits before entries at equal angles; dropping the wrap-around count; inverting the final radical comparison; `I512` subtraction without borrow; the `I512` final comparison dropping a factor.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `54-fixed_radius_queries_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 4 probes) | PASS, 288,174 checks per configuration, 4 probes, `MEMORY peak=472MB` |
| same with `CXX=g++-14` | PASS, 288,174 checks per configuration, 4 probes, `MEMORY peak=470MB` |
| `54-fixed_radius_queries_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 2,315,875 checks, `MEMORY peak=334MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=654MB` |
| `02-integration.py --sanitizers` | PASS, 167 headers, sanitizer self-tests, `MEMORY peak=1831MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/54-fixed_radius_queries_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`54-fixed_radius_queries_benchmark.py --seed 20260927` (2026-10-19; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; other sessions kept the load average near 2.5 on the 8-core CPU).
- Uniform points use `r2 = 10^8` (the `lll` path), in a square sized for about `m` candidates within `2 * r` of each pivot, at most `10^6` wide so the scaled copy stays within `10^9`. Only row (8,000, 10) reaches the cap, which raises its expected candidates from 10 to 10.05.
- The same points times 1000 with `r2` times `10^6` take the `I512` path.
- Both scales must report the same count.

| n | m | Count | `lll` (µs) | `I512` (µs) |
|---|---|---|---|---|
| 500 | 10 | 9 | 2,726 | 17,731 |
| 500 | 50 | 23 | 18,426 | 182,554 |
| 2,000 | 10 | 11 | 11,687 | 79,787 |
| 2,000 | 50 | 29 | 82,770 | 841,855 |
| 8,000 | 10 | 13 | 46,672 | 328,520 |
| 8,000 | 50 | 30 | 354,487 | 3,382,918 |

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [USACO Guide, LeetCode 1453 solution](https://usaco.guide/problems/lc-maximum-number-of-darts-inside-of-a-circular-dartboard/solution): the angular sweep with floating angles and entries before exits. The exact comparator here is new to the catalogs swept.
- [Chazelle–Lee and later work, arXiv 1312.0389](https://arxiv.org/html/1312.0389v1) and [arXiv 2509.16008](https://arxiv.org/html/2509.16008): the `O(n^2)` bound (optimal under 3SUM) and the weighted, coloured and two-disk variants, none adopted.
- [CGAL Spatial_searching](https://doc.cgal.org/latest/Spatial_searching/index.html): fixed-radius search through kd-trees, owned by row 28.

## Limits and handoffs

- `maxPointsInDisk` is `O(n^2 * log(n))` in the worst case; Chazelle–Lee's `O(n^2)` is not adopted. Radii above `sqrt(4 * 10^8)` pay the 6.5–10× `I512` constant.
- Radius queries around arbitrary points belong to row 28 `KdTree: radius`. Unit-disk graph components belong to row 22 (`euclideanBottleneck`).
- Not adopted (see 00-notes): weighted and coloured disks, two disks, the smallest disk covering `k` points.

## History

- 2026-10-18: first implementation (P037). Independent review: the per-query `GridHash` bound and the centre contract were corrected, `r2` is clamped under `NDEBUG`, and the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit widened `maxPointsInDisk` to `r2 <= 4 * 10^18` with an `I512` comparator. Independent review: the benchmark left the coordinate domain (side now capped), threshold tests added. Full suite passed on g++ and g++-14, stress passed.
