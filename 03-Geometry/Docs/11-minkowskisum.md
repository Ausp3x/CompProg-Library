# 11-minkowskisum.hpp — evidence

Owned by package P036 / GE03, together with [10-halfplaneintersection.md](10-halfplaneintersection.md). Every operation in the row has a test.

## Contracts

### Numeric policy

All four operations are exact. Points are integral `Point2<T>` (a `static_assert` rejects floating `T`). Every vertex sum, every edge vector (the difference of consecutive vertices of an input or intermediate ring), and for `minkowskiDifference` every negated vertex, must fit `T`. Sums alone are not enough: `{(-1.5 * 10^9, 0), (1.5 * 10^9, 0)}` in `int` overflows its edge vector. Edge comparisons are cross products of edge vectors in `lll`. Coordinates with `|x|, |y| <= 10^18` are sufficient for `lng`: sums stay within `2 * 10^18`, edge vectors within `2 * 10^18`, and crosses within `8 * 10^36`.

### minkowskiSum (two polygons), minkowskiDifference

Each input is a strictly convex CCW ring, which is what `convexHull(..., false)` returns. Any rotation is accepted, as are a single point and two distinct points. This is asserted in `O(n)` at entry: three or more vertices must turn strictly left everywhere, and two vertices must differ. An empty input gives an empty result. Each ring is rotated to its lexicographic minimum, the minimum by `(x, y)` as in the hull convention. Its edges are then in increasing angle over `(-π/2, 3π/2]`, and the edge sequences of the two rings are merged. Parallel edges advance both pointers together, so the output is strictly convex.

The result follows the `convexHull` convention. It starts at the lexicographic minimum (the sum of the two minima), runs CCW, never repeats its start, has no collinear vertices, and is a point or a two-point segment when the sum is degenerate. `minkowskiDifference(p, q) = minkowskiSum(p, -q)`, the CP convention used for collision and distance (`P ∩ Q ≠ ∅` iff the origin lies in `P ⊕ (−Q)`). This is not morphological erosion. Cost: `O(n + m)` time and memory. `minkowski_detail::merge` returns the vertex index pairs, and [17-polygon_distance.md](17-polygon_distance.md) reuses them for witnesses.

### minkowskiSum (family)

Input: `k` strictly convex rings with `n` vertices in total, under the same domain. The family is summed by divide and conquer over halves; each level costs `O(n)`, so the total is `O(n * log(k))` time and `O(n)` memory, which matches merging `k` sorted edge lists. The empty family returns the origin, the identity of the sum. A family with an empty member returns an empty result.

### minkowskiSumHull

Input: any two point sets, with duplicates and collinear points allowed. It returns `minkowskiSum(convexHull(p), convexHull(q))`. Because `conv(A ⊕ B) = conv(A) ⊕ conv(B)`, this is the exact convex hull of the sum, not a bound. `O((n + m) * log(n + m))`.

## Feature-to-test map

`96-Local Testing/03-Geometry/11-minkowskisum_tester.py` provides quick, full and stress modes and four assertion probes: a clockwise ring, a collinear vertex, a repeated two-point ring, and a nonconvex family member. It builds the optimized, checked and ASan/UBSan configurations.

**Oracle.** The convex hull of every pairwise vertex sum (or difference). That hull is itself validated against a supporting-line brute force on sets of up to 40 points.

| Operation | Test | Oracle |
|---|---|---|
| `minkowskiSum` (two polygons) | Every pair of the distinct hulls of subsets of the 3×3 grid (empty, points, segments, triangles and more), in random rotations; empty, point, collinear segments, perpendicular segments, squares; random hulls at coordinate scales 5, 1,000 and `10^18`; `int` coordinates; commutativity; 200,000-vertex parabola hulls (strict convexity and start); 2,000 unit squares summed into one exact square | Hull of all sums |
| `minkowskiDifference` | The same corpus; wide `±10^18` squares | Hull of all differences |
| `minkowskiSum` (family) | Two-member families equal to the pairwise sum; random families of 1–6 hulls; the empty family; an empty member | Iterated hull of sums |
| `minkowskiSumHull` | Random point sets with duplicates and collinear points | Hull of all pairwise sums of the raw sets |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 768 MB; optimized 0.2 s compile + 2.0 s run, checked 16.2 s compile + 15.9 s run, ASan-UBSan 16.0 s compile + 23.0 s run; MEMORY peak 955 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 811 MB; optimized 0.3 s compile + 1.8 s run, checked 27.7 s compile + 24.5 s run, ASan-UBSan 27.1 s compile + 31.0 s run; MEMORY peak 996 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 23.2 s compile + 0.1 s run, checked 26.9 s compile + 5.0 s run; MEMORY peak 749 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 770 MB; optimized 0.1 s compile + 2.9 s run, checked 6.5 s compile + 31.1 s run, ASan-UBSan 6.7 s compile + 30.2 s run; MEMORY peak 679 MB. Dated rows below stay those commands' latest results.

2026-10-11, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.7, GNU++20.

| Command | Result |
|---|---|
| `11-minkowskisum_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 4 probes) | PASS, 958,601 checks per configuration, 4 probes, `MEMORY peak=571MB` |
| same with `CXX=g++-14` | PASS, 958,601 checks per configuration, 4 probes, `MEMORY peak=574MB` |
| `11-minkowskisum_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 2,107,315 checks, `MEMORY peak=331MB` |
| `02-integration.py --sanitizers` | Stops at its first failure, outside P036: `96-Local Testing/01-Core/16-poly_tester.cpp` (P025) needs its second unit `16-poly_tester_tu.cpp`, which the script does not link. For this package the same checks were run directly: header alone for `10`, `11`, `16`, `17`, `98-basic` and `99-all`, and a two-translation-unit program using every operation, with `g++` and `g++-14` under `-Wall -Wextra -Wconversion` and with ASan/UBSan. All pass with no warnings |
| `03-consistency.py --braces` on the four headers, testers and benchmark; `03-consistency.py` | No violations; no errors |

Mutation check (2026-10-11): parallel edges advancing only one pointer is caught.

```bash
python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

None required. The merge is linear and allocation-bound; the 200,000-vertex case runs within the full suite.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [cp-algorithms: Minkowski sum of convex polygons](https://cp-algorithms.com/geometry/minkowski.html): the edge merge and the distance application. That page starts from the lowest point, then the leftmost; this header uses the library's lexicographic hull convention instead.
- [maspypy `geo/minkowski_sum.hpp`](https://maspypy.github.io/library/geo/minkowski_sum.hpp): strict output and translation of a single point; it cites CF 87E for sums of several polygons.
- [OI Wiki convex hull page (闵可夫斯基和)](https://oi-wiki.org/geometry/convex-hull/) and [CGAL Minkowski_sum_2](https://doc.cgal.org/latest/Minkowski_sum_2/index.html): scope of the nonconvex methods (not adopted; see 00-notes).
- `OLD/Team Notebook/src/geometry/getminkowskisum.cpp`: legacy, superseded. It assumed at least two vertices per polygon and kept collinear output vertices.

## Limits and handoffs

- Exact sums of nonconvex polygons (decomposition plus union, or reduced convolution) need rows 18 and 30. A disk-offset buffer is row 48.
- Point-in-sum queries are row 15 `contains` applied to the sum or difference.

## Aliases

Search aliases (P263, grab --find): 闵可夫斯基和, 闵科夫斯基和, 明可夫斯基和, 闵氏和, 闵可夫斯基差.

## History

- 2026-10-11: first implementation (P036). Independent review: one finding (edge vectors must also fit `T`) fixed in the contract.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
