# 04-convexhull.hpp — Evidence

Owned by package P007 / GE01 (geometry foundations, with `01-point.hpp`, `02-line_segment.hpp` and `03-polygon.hpp`). Every operation in the row has a test.

## Contracts

### Numeric Policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies.

### convexHull, convexHullIndices, convexHullGraham

Exact integral hulls with `n <= INT_MAX`. Orientation and squared-distance intermediates must fit `lll`, which `|coordinate| <= 10^9` guarantees. Each function works on a copy and removes duplicates. The output starts at the lexicographic minimum, runs CCW and never repeats its start. `keep_collinear` keeps every distinct boundary input point. An entirely collinear set gives its two sorted endpoints, or every sorted distinct point when collinear points are kept. `convexHullIndices` returns the same cycle as `convexHull`, naming each point by its first occurrence in the input; `lowerHullIndices` and `upperHullIndices` do the same for `lowerHull` and `upperHull` (user decision 2026-11-12). All three share the index order `hull_detail::order`. Graham is the angular-scan alternative; its integral angle comparison avoids non-transitive floating predicates.

Correctness and cost: monotone chain keeps the lower and upper hulls by removing turns that violate convexity, and each item enters and leaves each stack at most once. `hull_detail::chain` runs over points or indices through an accessor, so `convexHull` and `convexHullIndices` share one implementation. Ordering indices by `(point, index)` and then removing duplicates by point keeps the first occurrence. Graham scan uses an extreme pivot, exact angular order and, when collinear boundary points are kept, a reversal of the final ray. All three take `O(n * log(n))` time and `O(n)` storage including the output.

### convexHullSorted

Same output as `convexHull(p, keep_collinear)` on lexicographically nondecreasing input, asserted with one `O(n)` scan; duplicates are allowed. It skips the sort and runs in `O(n)`, so it reuses a caller's sorted array (maspypy's presorted mode). `convexHull` is `sort` followed by `convexHullSorted`, sharing `hull_detail::cycle`.

### lowerHull, upperHull, lowerHullSorted, upperHullSorted

The Sorted forms take lexicographically nondecreasing input, asserted with one `O(n)` scan, duplicates allowed, and run in `O(n)` (maspypy's sorted mode); `lowerHull` and `upperHull` sort first. All four remove duplicates and return the distinct points of the lower or upper chain from left to right, with both lexicographic extremes included, as in maspypy and Nyaan (CGAL runs both chains CCW and omits one end). `keep_collinear` keeps every distinct input point on the chain.

- A vertical left edge belongs to the upper chain and a vertical right edge to the lower chain.
- Empty input gives an empty result and a single distinct point gives that point. For a collinear set both chains are the two extremes, or every sorted distinct point when collinear points are kept.
- For a hull of positive area, `lowerHull` without its last point followed by `upperHull` reversed without its last point is `convexHull` under the same policy.

Each is one monotone-chain pass of `hull_detail::chain` (the upper one reversed) with no degenerate special case, since a collinear set is a valid chain. Cost: `O(n * log(n))` time unsorted, `O(n)` for the Sorted forms, `O(n)` storage.

### convexHullMelkman

The input is the vertices of a simple polyline or simple polygon ring (no repeated closing vertex), in order and in either orientation. Consecutive equal vertices are allowed, and `n <= INT_MAX / 2` is asserted, since the deque has `2 * n + 1` slots. The output equals `convexHull(p, keep_collinear)`: CCW from the lexicographic minimum, strict by default. Cost: `O(n)` time and storage; with `keep_collinear` (user decision 2026-11-12, to be removed: Limits and Handoffs) `O(n * log(out) + k * log(k))` for `k` boundary input points. Simplicity is not checked: non-simple input gives an unspecified hull, but the loops stay bounded.

Correctness (Melkman's algorithm, Sources):
- A simple polyline is monotone along its collinear prefix, so `p[0]` and the prefix's last point span it.
- The deque starts as the CCW triangle on the first point off that line, which sits at both ends.
- A new point strictly left of both end edges lies inside the hull of the polyline so far: by simplicity, the segment from the last vertex into the hull cannot leave it. Any other point pops the ends that see it, weakly (`orient <= 0`), and is pushed on both.
- A point on a middle hull edge is strictly left of both end edges and is skipped. A point on the line of an end edge next to the last vertex is pushed while one end pops that vertex and the other keeps it; later pushes move the kept collinear vertex into the middle, so the deque is a weakly convex CCW cycle. One chain pass from its lexicographic minimum, a strict vertex, plus a wrap check removes those vertices.
- With `keep_collinear`, every input point lies in the strict hull `h` (`h[0]` its lexicographic minimum). `hull_detail::boundary` tests each point in `O(log(out))`. On the line of edge `h[0] h[1]` or `h[last] h[0]` the point is on that edge, since it is inside the hull. Otherwise a binary search on `orient(h[0], h[mid], x)` finds the fan triangle `h[0] h[lo] h[lo + 1]` holding it strictly inside the wedge, and it is on the boundary exactly when it is on edge `h[lo] h[lo + 1]` (a point on the diagonal `h[0] h[hi]` meets that edge only at vertex `h[hi]`). `convexHull(on, true)` of those points is then `convexHull(p, true)`, since both keep exactly the distinct boundary points in the same cyclic order. A collinear input returns `convexHull(p, true)`, every sorted distinct point.

### convexHullJarvis

Any input, duplicates allowed; the output equals `convexHull(p, keep_collinear)`. Cost: `O(n * out)` time and `O(out)` storage, which beats the `O(n * log(n))` hulls when the hull is small (Benchmarks).

The march starts at the lexicographic minimum. At each point `a` it chooses the most clockwise other point; among collinear candidates it takes the farthest, as CGAL does, or the nearest when collinear points are kept. It skips the points on the incoming edge's line behind `a`, which are already visited. The remaining candidates lie in a half-open cone narrower than `pi`: directions in `(-pi / 2, pi / 2]` from the start, and later from the incoming direction up to, but excluding, its reverse. The exact `orient` comparison is therefore a strict order on them, and equal orientation means the same ray. The march ends on returning to the start or when no candidate remains, which happens at the far end of a collinear set and gives the documented collinear outputs. In the domain the hull has at most `n` points; the loop also stops at `n`, so coordinates whose `orient` overflows its wide type (a violated precondition) still terminate in `O(n^2)`.

## Feature-to-Test Map

`96-Local Testing/03-Geometry/04-convexhull_tester.py` runs the optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds of the shared [contest runner](<../../96-Local Testing/_00_contest_runner.py>). Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

| Operation | Test | Oracle |
|---|---|---|
| `convexHull`, `convexHullGraham` (both `keep_collinear` policies) | Canonical orientation and start; duplicates; empty and singleton input; all-collinear input under both policies; caller's input unchanged; exhaustive 3×3-grid subsets and seeded adversarial point sets | Independent supporting-edge (Jarvis) oracle; agreement between hull implementations is supplemental |
| `convexHullIndices` | Index ranges, first-duplicate indices and agreement with the oracle hull on every case, including 20,001-point parabolas | Supporting-edge oracle |
| `convexHullSorted` | Every `verify` case on a sorted copy (both policies), large parabola and collinear sets; probe `sorted-unsorted` | Supporting-edge oracle; expected assertion failure |
| `convexHullJarvis` | Every `verify` case (both policies, input unchanged), large square spiral, 20,001 random points at ±10^9, large collinear set | Supporting-edge oracle; `convexHull` on the large cases |
| `lowerHull`, `upperHull` | Every `verify` case; chain-convention hand cases (square with collinear points, vertical left and right edges, vertical segment, duplicates, empty); large parabola and collinear sets | The oracle's CCW cycle split at the lexicographic maximum; literal expected values |
| `lowerHullSorted`, `upperHullSorted` | Every `verify` case on a sorted copy (both policies), large parabola and collinear sets; probes `lower-sorted-unsorted`, `upper-sorted-unsorted` and the three `-retain` probes through the shared sortedness assertion | The same split oracle; expected assertion failures |
| `lowerHullIndices`, `upperHullIndices` | Every `verify` case under both policies (index range, first-duplicate index, order); `duplicate-heavy and collinear multisets`; empty, duplicate-segment and 32-bit cases; large parabola and line | Supporting-edge oracle chains |
| `convexHullMelkman` | Hand cases with explicit retained answers (n = 0, 1, 2, equal neighbours, collinear polylines, the 3 x 3 lattice ring, points exactly on diagonals from the lexicographic minimum); every case below under both `Melkman strict` and `Melkman retain`, plus `rectangle(w, h)` with every lattice boundary point and `fan(n, r)` rings with edge lattice points and diagonal dents; sorted sequences and convex-position rings in `verify`; simple polylines and rings (x-monotone polylines with collinear prefixes and vertical runs, star polygons, self-avoiding grid walks and their thinnings, x-monotone rings, rectangles) under the 8 lattice symmetries, reversed, rotated, sliced, with repeated neighbours, input unchanged; 20,001-vertex parabola and square spiral; `hull_detail::boundary` directly (`boundary location`) | Supporting-edge oracle (retained; the strict answer drops edge-interior vertices); edge determinants for `boundary`. An independent review on 2026-11-12 (scratch, outside the suite) checked every simple polyline on the 3 x 3 grid up to length 7 and on the 4 x 3 and 4 x 4 grids up to length 6, also with repeated neighbours and as closed rings, under both policies: 0 mismatches against `convexHull`. The generators are simple by construction; no brute-force check of that is recorded |

## Commands and Results

Package run, 2026-11-12 (`/finish P007`, experiment none): Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and 14.4.1, Python 3.14, GNU++20, seed 42; the 18 `03-Geometry` suites ran in sequence on the final headers and testers, after the mutation campaigns and with no benchmark running.

| Command | Result |
|---|---|
| `01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` (GCC 16.2.1) | PASS, 2,151,259 checks per configuration, 6 assertion probes; pre-flight quick peak 40 MB; `MEMORY peak=496MB` |
| `CXX=g++-14 01-run.py --mode full --filter 03-Geometry --seed 42 --no-integration` | PASS, 2,151,259 checks per configuration, 6 probes; pre-flight quick peak 940 MB; `MEMORY peak=1138MB` |
| `01-run.py --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration` | PASS, 16,141,883 checks per configuration; `MEMORY peak=587MB` |
| `05-extraction.py ops` and `testers` on this header and tester | PASS, 12 operations on g++ and g++-14 |
| `02-integration.py` (190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) | PASS; `MEMORY peak=3241MB` |

Library-wide runs, 2026-11-10 (P274; same machine, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), this suite's lines, all PASS:

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | `MEMORY` peak |
|---|---|---|---|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | 787 MB | 0.2 s compile + 0.3 s run | 19.2 + 1.6 s | 20.6 + 1.6 s | 1001 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | 850 MB | 0.3 + 0.4 s | 29.4 + 4.2 s | 28.6 + 2.7 s | 1025 MB |
| `01-run.py --mode quick` | — | 19.4 + 0.0 s | 21.4 + 0.3 s | — | 790 MB |
| `01-run.py --mode stress --rounds 1` | 799 MB | 0.1 + 0.5 s | 8.2 + 4.6 s | 9.2 + 3.5 s | 541 MB |

```bash
python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --rounds 1 --filter 03-Geometry --seed 42 --no-integration
python3 '96-Local Testing/05-extraction.py' ops --filter 03-Geometry/04-convexhull.hpp
python3 '96-Local Testing/05-extraction.py' testers --filter '96-Local Testing/03-Geometry/04-convexhull_tester.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/03-Geometry/00-foundations_benchmark.py>) with `96-Local Testing/03-Geometry/00-foundations_benchmark.json` times `convexHull` (monotone chain), `convexHullGraham`, `convexHullSorted` (on a presorted copy), `convexHullJarvis` (skipped when hull size times `n` exceeds 5·10^7) and `convexHullMelkman` (on the star polygon) at 32, 2,048 and 32,768 points, under both retention policies. Inputs: random coordinates, duplicate-heavy grids, collinear points, parabolic convex-position points, a square's 4 corners plus interior points, and a star-shaped simple polygon in angular order. The same driver times `segmentDistanceApprox` against `linearDistanceApprox` on 2,048 and 32,768 random segment pairs at `|coordinate| <= 10^9` (row 02's ruling-4 keep, `duplicates.md`). There are 151 measurements, each the median of five timed calls after one warmup. Hull timings include the input copy and output allocation; every complete canonical output is checked outside the timed interval. Distances are stored in the timed loop and compared afterwards with the exact rational distance (relative `1e-9`). Flags are `-std=gnu++20 -O2 -DNDEBUG`. The record includes the seed, CPU and compiler, input settings, code hashes and a checksum. Every hull uses `O(n)` storage (Jarvis `O(out)`); no peak-RSS claim is made.

Latest run: 2026-11-12 (`/finish P007`, experiment none), Intel Core i9-11900H, GCC 16.2.1, seed 20260927, run alone after the mutation campaigns on the final headers.

- Monotone chain stays the default, about 2.3 times faster than Graham on random points. `convexHullSorted` saves the sort (2.5 to 20 times faster).
- Jarvis wins below a crossover of about 5 to 10 hull points: at 32,768 points it is faster with 2, 4 and 5 hull points (collinear strict, square, grid strict) and slower from 12 (grid at 2,048) and 27 (random) on. Collinear retention makes it quadratic.
- On the unsorted simple polygon, strict Melkman beats monotone chain by 3.7 times, but Melkman with `keep_collinear` is 1.1 to 1.3 times slower than `convexHull(p, true)`: its per-point `O(log(out))` boundary search costs more than the sort it avoids (Limits and Handoffs).
- `segmentDistanceApprox` is 18.5 (2,048 pairs) and 16.2 (32,768 pairs) times faster than `linearDistanceApprox`.

Medians in microseconds, strict policy unless noted:

| Distribution (n, hull size) | Monotone chain | Graham | Sorted | Jarvis | Melkman |
|---|---|---|---|---|---|
| Random (32,768, 27) | 2,914 | 6,604 | 1,183 | 6,384 | — |
| Random (2,048, 19) | 136 | 279 | 53 | 240 | — |
| Square corners (32,768, 4) | 2,843 | 6,604 | 1,074 | 994 | — |
| Square corners (2,048, 4) | 142 | 307 | 58 | 64 | — |
| Grid (32,768, 5) | 2,446 | 3,865 | 367 | 1,225 | — |
| Grid (2,048, 12) | 121 | 266 | 31 | 149 | — |
| Collinear (32,768, 2) | 1,866 | 1,767 | 92 | 553 | — |
| Collinear retained (2,048, 2,048) | 62 | 73 | 6 | 36,672 | — |
| Parabola (32,768, 32,768) | 2,122 | 3,922 | 396 | skipped | — |
| Star polygon (32,768, 111) | 2,647 | 6,556 | 1,076 | 25,636 | 725 |
| Star polygon retained (32,768, 111) | 2,637 | 6,491 | 1,060 | 25,612 | 2,995 |
| Star polygon (2,048, 41) | 123 | 292 | 54 | 577 | 33 |
| Star polygon retained (2,048, 41) | 121 | 293 | 53 | 566 | 158 |

| Segment pairs | `segmentDistanceApprox` | `linearDistanceApprox` |
|---|---|---|
| 32,768 | 1,768 | 28,626 |
| 2,048 | 95 | 1,753 |

```bash
python3 '96-Local Testing/03-Geometry/00-foundations_benchmark.py' --seed 20260927
```

## Mutation Testing

Record: mewt 4.0.0 on blob `019084d9869ec30b89d0e23f77d7c4dc481e470e` (2026-11-12, seed 1, 41 lines changed since blob `e1ed061aec3c`): 210 mutants, 192 caught (4 by timeout), 18 survived, 0 untested.

Stage times, summed over two retests of the same workdir: quick 457 s, full optimized 23 s, full on the other builds 553 s. The open survivors below come from this record (C) and from two earlier campaigns of 2026-11-12 whose lines this record did not cover: a whole-file sample of 500 of 666 mutants (W), and a 3-line campaign after Melkman's redundant equal-neighbour test was removed (M). Line numbers are those of the current blob.

| Mutants (record) | Line | Why they survive |
|---|---|---|
| `COS 433` (W; `q.size() <= 2` → `< 2`) | 7 | Two points are collinear, so the scan returns the same degenerate result. |
| `AS 192`, `AS 193`, `AS 237`, `AS 238`, `AS 275`, `AS 276` (W); `AS 248`, `AS 249` (M) | 9, 70, 107, 144 | Swapped `orient` arguments where only `orient == 0` or a nonzero test is used: the swap negates the value. |
| `COS 496`, `COS 497` (C); `CR 125`, `AOS 349`, `COS 522`, `COS 523`, `COS 602`, `COS 603` (W) | 13, 101, 134 | Size assertions (`n <= INT_MAX`, `INT_MAX / 2`): a distinguishing input needs over 10^9 points, beyond every memory cap. |
| `COS 460`, `COS 520` (W; `s == 0` → `>= 0`) | 27, 76 | They follow `s > 0 \|\|`. |
| `AOS 324`, `AOS 326`, `AOS 327`, `CR 116` (W); `CR 107` (C) | 32, 37, 72 | `reserve` changes capacity only. |
| `MR 659`, `MR 663`, `MR 664` (W); `MR 738`, `MR 742` (C) | 53, 58, 90, 93, 130 | A copy in place of `std::move` gives the same values; cost only. |
| `COS 545`, `COS 550`, `COS 567` (C; `<= 0` or `>= 0` → `== 0`, `== 0` → `<= 0` in `boundary`) | 43, 47 | Every tested point lies in the closed hull, where these orientations never have the excluded sign. |
| `COS 556` (C; `hi - lo > 1` → `!=`) | 44 | `hi > lo` always. |
| `AS 248` (C; `midpoint(hi, lo)`) | 45 | The midpoint still lies strictly between when `hi - lo >= 2`. |
| `COS 564` (C; `> 0` → `>= 0`) | 46 | A point on the diagonal `h[0] h[mid]` keeps `lo = mid`, and the final edge test still finds it only at vertex `h[mid]`. |
| `AS 251`, `AS 252` (C) | 47 | Argument swaps negating an `orient` compared with 0. |
| `AS 235`, `AS 236`, `AS 256`, `AS 277`, `AS 280` to `AS 283` (W) | 67, 110, 144, 146 | Symmetric `dist2` and `dot`, and `swap`. |
| `AS 292`, `AS 293` (C) | 109 | Symmetric `min`, `max`. |
| `AS 310` (C) | 124 | Swaps two identical `false` arguments. |
| `COS 490`, `COS 553`, `COS 628` (W) | 67, 110, 146 | Under a nonzero `s` or `orient`, or after `o < 0 \|\|`. |
| `AOS 333`, `AOS 334` (W; `size() - 1` → `* 1`, `/ 1` in Graham) | 69 | `i` starts one higher, and the first test, `orient(o, p.back(), p.back()) == 0`, steps it back. |
| `COS 497`, `COS 500` (W; `i > 1` → `!= 1`, `>= 1`) | 70 | Reaching `i == 1` needs every point after the pivot on one ray, the degenerate case returned earlier. |
| `COS 528`, `COS 557` (W); `COS 529` (M); `COS 624` (C; `i == n` → `>=`) | 103, 106, 109, 114 | `n >= 0`; `i` steps by one up to `n`. |
| `COS 565`, `COS 570` (W; `> 0` → `>= 0` in Melkman's skip test) | 116 | A point on an end edge's line that is processed instead of skipped leaves at most a collinear deque vertex, which the final chain pass removes; behind the last vertex the other end test fails by convexity, so it is processed either way. |
| `AOS 380` to `AOS 383`, `AOS 389`, `AOS 390`, `COS 582`, `COS 585` (W) | 117, 119 | The deque-size guards of the pop loops never bind on a simple polyline (a point outside the hull never sees all its edges); they only bound the loops on non-simple input. |
| `COS 592`, `COS 596` (W; wrap check `>= 3` → `!= 3`, `<= 0` → `== 0`) | 125 | The deque has positive area and is weakly convex, so the wrap orientation is never negative and the check never pops at 3 vertices. |
| `IT 203` (C; `boundary` → `true` in Melkman's collection) | 129 | Output-equivalent: `convexHull(p, true)` of every point is the same hull; only the cost degrades from `O(n * log(out) + k * log(k))` to `O(n * log(n))`. The direct `boundary location` test pins the predicate itself. |
| `COS 638` (W; `<` → `<=` in Jarvis's nearer-collinear rule) | 146 | Equal distance on the same ray means `c == b`, a duplicate. |
| `COS 652`, `COS 653` (W; `h.size() < p.size()` → `!=`, `<=`) | 147 | In the domain the hull has at most `n` points and the march returns to the start first; the bound only stops overflowing inputs. |

## Sources

The implementation was written independently from the algorithms below. Notebook implementations were compared, not copied as correctness guarantees. Completeness-sweep sources: [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted: [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07). P007 re-audit sources: [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11); its decisions: [00-notes.md](00-notes.md#p007-re-audit-decisions-2026-11-11).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), November 24, 2024 edition, printed pp. 16–18 (hull): operations, overflow warnings and degeneracy conventions.
- [cp-algorithms: Convex hull construction](https://cp-algorithms.com/geometry/convex-hull.html), inspected September 27, 2026.
- A. A. Melkman, "On-line construction of the convex hull of a simple polyline", Information Processing Letters 25 (1987), cited for `convexHullMelkman`, not fetched; the implementation was compared with CGAL's `ch_melkman_impl.h` (fetched 2026-11-11, [00-sources.md](00-sources.md#p007-re-audit-fetched-2026-11-11)).

## Limits and Handoffs

Higher-dimensional hulls belong to P137 / GE12. Chan's output-sensitive hull is a separately scoped variant in row 41.

The user decided on 2026-11-22 to drop the `keep_collinear` policy of `convexHullMelkman`. It was added by user decision 2026-11-12 before it was measured, returns the same hull as `convexHull(p, true)` on a narrower domain, and measured 1.1 to 1.3 times slower on the star polygon (Benchmarks), so `01-principles.md` (Priority) does not keep it; its exception in `omissions.md` (Recorded Decisions, 2026-11-12) ends with it. P007 reopens for the code, row and document change; until then the contracts above describe the current header.

Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 凸包, 二维凸包, 求凸包, 安德鲁算法, 单调链, 格雷厄姆扫描, 葛立恒扫描法.
Aliases (P007): Melkman, Melkman algorithm, simple polygon hull, Jarvis march, gift wrapping, lower hull, upper hull, presorted hull, hull indices, lower hull indices, 卷包裹法, 礼品包装算法, 上凸壳, 下凸壳.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 1,151,038 checks) and benchmark passed on g++ 16.
- 2026-10-07: re-audit, added `convexHullIndices` and the shared `hull_detail::chain`; 3 findings fixed; full suite, stress and benchmark passed on g++ 16; independent review confirmed three package defects, all fixed, none in this header.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (OI Wiki page titles and common names, Simplified and Taiwan or Hong Kong terms); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed.
- 2026-11-12: P007 re-audit (GE01, started 2026-11-11): added `convexHullSorted`, `lowerHull`, `upperHull`, `lowerHullSorted`, `upperHullSorted`, `convexHullMelkman` and `convexHullJarvis`, with `hull_detail` reorganized (`dedupe`, `chain`, `cycle`, `side`); independent review: 4 findings fixed (the Sorted chain forms, Melkman `n <= INT_MAX / 2`, the Jarvis loop bound, the Melkman collinear-vertex argument); first mutation campaign, whose survivors added default-policy calls and removed Melkman's redundant equal-neighbour test; benchmark rerun with five algorithms and six distributions; full on g++ and g++-14, stress, extraction and `02-integration.py` passed.
- 2026-11-12: `/finish P007`: added `lowerHullIndices`, `upperHullIndices` and Melkman `keep_collinear` by user decision; Melkman source entry; benchmark rerun on the final headers (Melkman `keep_collinear` slower than `convexHull(p, true)`); the tester's probe fallback, which passed the runner's death check, fixed and `hull_detail::boundary` tested directly; changed-lines mutation campaign; full on g++ and g++-14 and stress passed.
