# 11-minkowskisum.hpp — Evidence

Owned by package P036 / GE03, with [10-halfplaneintersection.md](10-halfplaneintersection.md). Every implemented operation has a test; the row is partial because `minkowskiSumReducedConvolution` (nonconvex polygons, holes), returned to it by the P275 re-review of 2026-11-10, is not yet implemented.

## Contracts

### Numeric Policy

All four operations are exact. Points are integral `Point2<T>`; a `static_assert` rejects floating `T`. Every vertex sum, every edge vector (the difference of consecutive vertices of an input or intermediate ring) and, for `minkowskiDifference`, every negated vertex must fit `T`. Sums alone are not enough: `{(-1.5 * 10^9, 0), (1.5 * 10^9, 0)}` in `int` overflows its edge vector. Edge comparisons are cross products of edge vectors in `lll`. For `lng`, `|x|, |y| <= 10^18` suffices: sums and edge vectors stay within `2 * 10^18` and crosses within `8 * 10^36`.

### minkowskiSum (two polygons), minkowskiDifference

Each input is a strictly convex CCW ring, which is what `convexHull(..., false)` returns; any rotation, a single point and two distinct points are accepted. This is asserted in `O(n)` at entry: three or more vertices must turn strictly left everywhere, and two vertices must differ. An empty input gives an empty result. Each ring is rotated to its lexicographic minimum by `(x, y)`, as in the hull convention, so its edges increase in angle over `(-π/2, 3π/2]`, and the two edge sequences are merged. Parallel edges advance both pointers together, so the output is strictly convex.

The result follows the `convexHull` convention: it starts at the lexicographic minimum (the sum of the two minima), runs CCW, never repeats its start, has no collinear vertices, and is a point or a two-point segment when the sum is degenerate. `minkowskiDifference(p, q) = minkowskiSum(p, -q)`, the competitive-programming convention used for collision and distance (`P ∩ Q ≠ ∅` iff the origin lies in `P ⊕ (−Q)`), not morphological erosion. Cost: `O(n + m)` time and memory. `minkowski_detail::merge` returns the vertex index pairs, which [17-polygon_distance.md](17-polygon_distance.md) reuses for witnesses.

### minkowskiSum (family)

Input: `k` strictly convex rings with `n` vertices in total, under the same domain. The family is summed by divide and conquer over halves at `O(n)` per level, so the total is `O(n * log(k))` time and `O(n)` memory, matching a merge of `k` sorted edge lists. The empty family returns the origin, the identity of the sum; a family with an empty member returns an empty result.

### minkowskiSumHull

Input: any two point sets, duplicates and collinear points allowed. It returns `minkowskiSum(convexHull(p), convexHull(q))`, which is the exact convex hull of the sum because `conv(A ⊕ B) = conv(A) ⊕ conv(B)`. `O((n + m) * log(n + m))`.

## Feature-to-Test Map

`96-Local Testing/03-Geometry/11-minkowskisum_tester.py` provides quick, full and stress modes in the optimized, checked and ASan/UBSan configurations, with four assertion probes: a clockwise ring, a collinear vertex, a repeated two-point ring and a nonconvex family member.

**Oracle.** The convex hull of every pairwise vertex sum (or difference), itself validated against a supporting-line brute force on sets of up to 40 points.

| Operation | Test | Oracle |
|---|---|---|
| `minkowskiSum` (two polygons) | Every pair of the distinct hulls of subsets of the 3×3 grid (empty, points, segments, triangles and more), in random rotations; empty, point, collinear segments, perpendicular segments, squares; random hulls at coordinate scales 5, 1,000 and `10^18`; `int` coordinates; commutativity; 200,000-vertex parabola hulls (strict convexity and start); 2,000 unit squares summed into one exact square | Hull of all sums |
| `minkowskiDifference` | The same corpus; wide `±10^18` squares | Hull of all differences |
| `minkowskiSum` (family) | Two-member families equal to the pairwise sum; random families of 1–6 hulls; the empty family; an empty member | Iterated hull of sums |
| `minkowskiSumHull` | Random point sets with duplicates and collinear points | Hull of all pairwise sums of the raw sets |

A hand mutation check (2026-10-11) caught parallel edges advancing only one pointer.

## Commands and Results

Package runs 2026-10-11: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14.7, GNU++20. The library-wide rows are the latest results of those commands (2026-11-10, P274; seed 42, beside the other suites, build cache emptied before each cold run).

| Command | Result |
|---|---|
| 2026-10-11: `11-minkowskisum_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 958,601 checks per configuration, 4 probes, `MEMORY peak=571MB` |
| 2026-10-11: same with `CXX=g++-14` | PASS, 958,601 checks per configuration, 4 probes, `MEMORY peak=574MB` |
| 2026-10-11: `11-minkowskisum_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 2,107,315 checks, `MEMORY peak=331MB` |
| 2026-10-11: `02-integration.py --sanitizers` | Stopped at a failure outside P036 (`96-Local Testing/01-Core/16-poly_tester.cpp` of P025 needs its second unit `16-poly_tester_tu.cpp`, which the script did not link). The same checks ran directly and passed with no warnings: each of `10`, `11`, `16`, `17`, `98-basic` and `99-all` alone, and a two-translation-unit program using every operation, with `g++` and `g++-14` under `-Wall -Wextra -Wconversion` and with ASan/UBSan |
| 2026-10-11: `03-consistency.py --braces` on the four headers, testers and benchmark; `03-consistency.py` | No violations; no errors |
| 2026-11-10 P274: `01-run.py --mode full` (GCC 16.2.1) | PASS (counts not recorded); pre-flight quick peak 768 MB; optimized 0.2 s compile + 2.0 s run, checked 16.2 s + 15.9 s, ASan/UBSan 16.0 s + 23.0 s; MEMORY peak 955 MB |
| 2026-11-10 P274: `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS (counts not recorded); pre-flight quick peak 811 MB; optimized 0.3 s compile + 1.8 s run, checked 27.7 s + 24.5 s, ASan/UBSan 27.1 s + 31.0 s; MEMORY peak 996 MB |
| 2026-11-10 P274: `01-run.py --mode quick` | PASS (counts not recorded); optimized 23.2 s compile + 0.1 s run, checked 26.9 s + 5.0 s; MEMORY peak 749 MB |
| 2026-11-10 P274: `01-run.py --mode stress --rounds 1` | PASS (counts not recorded); pre-flight quick peak 770 MB; optimized 0.1 s compile + 2.9 s run, checked 6.5 s + 31.1 s, ASan/UBSan 6.7 s + 30.2 s; MEMORY peak 679 MB |

```bash
python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/11-minkowskisum_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

None required: the merge is linear and allocation-bound, and the 200,000-vertex case runs within the full suite.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [cp-algorithms: Minkowski sum of convex polygons](https://cp-algorithms.com/geometry/minkowski.html): the edge merge and the distance application. That page starts from the lowest point, then the leftmost; this header uses the library's lexicographic hull convention.
- [maspypy `geo/minkowski_sum.hpp`](https://maspypy.github.io/library/geo/minkowski_sum.hpp): strict output and translation of a single point; it cites CF 87E for sums of several polygons.
- [OI Wiki convex hull page (闵可夫斯基和)](https://oi-wiki.org/geometry/convex-hull/) and [CGAL Minkowski_sum_2](https://doc.cgal.org/latest/Minkowski_sum_2/index.html): the nonconvex methods, of which reduced convolution is planned in this row.
- `OLD/Team Notebook/src/geometry/getminkowskisum.cpp`: legacy, superseded. It assumed at least two vertices per polygon and kept collinear output vertices.

## Limits and Handoffs

- `minkowskiSumReducedConvolution` is missing from this row; P036 implements it. Exact nonconvex sums by decomposition plus union need rows 18 and 30; a disk-offset buffer is row 48.
- Point-in-sum queries are row 15 `contains` applied to the sum or difference.
- No `04-mutation.py` record exists yet.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 闵可夫斯基和, 闵科夫斯基和, 明可夫斯基和, 闵氏和, 闵可夫斯基差.

## History

- 2026-10-11: first implementation (P036). Independent review: one finding (edge vectors must also fit `T`) fixed in the contract.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; inputs and checks unchanged; library-wide runs passed.
