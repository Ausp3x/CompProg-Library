# 13-segment_union.hpp — evidence

Owned by package P037 / GE05, together with [14-minimumenclosingcircle.md](14-minimumenclosingcircle.md) and [15-convex_polygon_query.md](15-convex_polygon_query.md). Every operation in the row has a test.

## Contracts

### Numeric policy

Rectangles are `array<T, 4>` `{x1, y1, x2, y2}`, meaning the half-open box `[x1, x2) × [y1, y2)`. Segments are `array<T, 3>`: horizontal `{y, x1, x2}` and vertical `{x, y1, y2}`, closed, endpoints in either order. Points, witnesses and boundary pieces are `Point2<T>`. `T` defaults to `lng`, so braced lists still deduce.

- **Signed integral `T` (at most 64 bits):** every result is exact. Rectangle coordinates satisfy `|coordinate| <= 4 * 10^18` (asserted), so widths and covered lengths fit `ulng` and areas, perimeters and depth measures fit `lll` (at most `(8 * 10^18)^2 = 6.4 * 10^37`). Segments accept any value: lengths are taken in `lll`, and orthogonal segments are only compared.
- **Floating `T`:** coordinates are finite (asserted). Every combinatorial decision (sorting, compression, ranks, ring linking) compares input coordinates exactly, with no epsilon, so the structure is that of the exact input. Only lengths, areas and perimeters are rounded `long double` sums. Rectangles with NaN coordinates fail the `x1 < x2` test and are skipped. Orthogonal segments with non-finite coordinates are skipped after the assertion, because NaN would break the sort's ordering.
- Depth counters are `int` (at most `n`).

### segmentUnionLength, coverageCount

Input: segments `pair<T, T>` with endpoints in either order, for signed integral or floating `T`; segments with equal endpoints are ignored. `segmentUnionLength` returns the length of the union, `lll` for integral `T` and rounded `long double` for floating `T`. Open and closed ends give the same length. `coverageCount` returns a vector `c` of size `n + 1`. For `k >= 1`, `c[k]` is the length covered by exactly `k` segments. `c[0]` is the length of the gaps between the extreme endpoints of the nonempty segments. Both sort `2 * n` endpoint events: `O(n * log(n))` time and `O(n)` memory.

`segmentUnionLength` is kept beside `06-Miscellaneous` `intervalUnionMeasure` by decision (2026-10-18, `duplicates.md`): it also takes floating endpoints. `coverageCount` is the 1D depth histogram by decision (2026-10-18); the other readings of the name are owned by `06-Miscellaneous/09-interval_algorithms.hpp`.

### rectangleUnionArea, rectangleUnionPerimeter

Both use a sweep over x with a segment tree over the compressed y coordinates. Each node keeps its cover count and its covered length (no lazy propagation). A node `[l, r)` with `r − l >= 2` is stored at its split point `(l + r) / 2`, which no other node shares, and a leaf's length follows from its count, so the tree holds `m − 1` length records for `m` elementary intervals. Empty rectangles are ignored. The area adds the covered length times the gap to the next event x.

The perimeter sums the vertical boundary length of the sweep and of the transposed sweep. At one x, additions are applied before removals, and the vertical boundary there is `(after adds − before) + (after adds − after removes)`. This equals the measure of the symmetric difference of the covered sets before and after x. Two rectangles that share a side therefore contribute nothing on it, and two that meet only at a corner contribute both sides. `O(n * log(n))`, `O(n)`.

### rectangleCoverageArea

`k >= 1` (asserted; raised to 1 under `NDEBUG`). Returns `{area covered by exactly k rectangles, area covered by at least k}`; `k > n` gives `{0, 0}`. Two paths give the same results, dispatched at `k <= 16` (`rectangle_detail::TREE_K`):

- **Tree, `k <= 16`.** The sweep keeps `len[j]` for `j = 1..k + 1` in every internal node: the measure covered at least `j` times, counting the cover counts of the node and its descendants. `O(n * k * log(n))` time and `O((k + 1) * n)` memory, `8 (k + 1)` bytes per elementary interval for integral coordinates.
- **Blocks, `k > 16`.** The `m` elementary y-intervals are split into `nb` blocks of `b = max(1, floor(sqrt(m / 32)))`. Each interval keeps a base depth and each block a lazy add. A block keeps:
  - its intervals sorted by base;
  - the distinct bases with suffix sums of their weights;
  - the first distinct base reaching `k` (base `>= k − lazy`).

  A `±1` change of the lazy add moves that threshold by one, so the pointer moves at most one distinct value, in `O(1)`. The pointer for `k + 1` is the same one, or one step further when that base reaches exactly `k`. A partially covered block changes the bases of its interval range, merges the changed and unchanged sorted runs (each run stays sorted) and rebuilds in `O(b)`. Each block caches its weight at depth `>= k` and `> k`, and running totals are updated by the change. Integral totals are exact (`ulng` arithmetic modulo `2^64`, final values below `8 * 10^18`). Floating totals are recounted from the block caches every `nb` changes, so rounding cannot drift. An event costs `O(nb + b) = O(sqrt(n))`: `O(n * sqrt(n))` time and `O(n)` memory for every `k`.

The overall bound is `O(n * k * log(n))` for `k <= 16`, else `O(n * sqrt(n))`, with `O(n)` memory. The threshold comes from the benchmark below. On short rectangles the blocks are ahead from `k = 2` (narrowly at `k = 2`, 2× by `k = 16`); on rectangles spanning most of the range (every block shifted on every event) the tree wins up to `k` between 32 and 64 at `n = 10^5`, and, in a one-off tuning measurement not kept in the driver, near 64 at `n = 4 * 10^5`. But the tree's memory grows with `k`: `8 (k + 1)` bytes per interval, 211 MB at `k = 32` and `n = 4 * 10^5`. `k = 16` caps the worst slowdown either way at about 2–3× and keeps memory linear.

### maxRectangleOverlap

Returns `{maximum number of rectangles sharing a point, a witness}`, or `{0, (0, 0)}` when every rectangle is empty. The witness is the lower-left corner of a deepest cell (a lattice point for integral coordinates), and the depth at it, counted with half-open boxes, equals the result. The tree supports range add with a max of children and no push-down. At each x, removals are applied before additions. Closed integral rectangles map to `[x1, x2 + 1) × [y1, y2 + 1)` with the same depth at every lattice point. `O(n * log(n))`.

### rectangleUnionBoundary

Returns the boundary of the closure of the union as maximal axis-parallel segments `{a, b}`, directed so that the union lies on the left. Outer boundaries therefore run counterclockwise and holes clockwise. Segments meet only at endpoints. At a pinch, where two rectangles share only a corner, the boundary passes the corner twice and the pieces stay separate.

At each event x, the event intervals are merged. The uncovered runs of each merged interval are reported before all updates at x and again after them. Every y of an event interval is covered just before x or just after x, so every reported run is a boundary piece. A run uncovered before is a left side, directed downward. A run uncovered after is a right side, directed upward. The horizontal pieces come from the transposed sweep: the reflection is mapped back and the pieces are reversed, because a reflection flips orientation.

Reporting the uncovered runs of an interval visits `O((1 + runs) * log(n))` nodes. The total is `O((n + out) * log(n))` time and `O(n + out)` memory.

### rectangleUnionBoundaryGuting

Returns the same segments as `rectangleUnionBoundary` (as a multiset; the order differs), by Güting's optimal divide and conquer (Acta Informatica 21, 1984, pp. 271–291; the contour algorithm on pp. 277–288).

1. **Division.** The `2n` vertical edges are sorted by x, with left edges first at equal x, and split at the median index. Each subproblem returns:
   - its unmatched left and right edges;
   - the stripe partition `P` of the y-ranks of its edges (plus ±∞);
   - for every stripe, a binary tree whose in-order leaves are the true x transitions of the union inside the subproblem's slab. A leaf is `(x, start of coverage)` or `(x, end of coverage)`; an empty tree means no transition.
2. **Merge.**
   - A rectangle whose left edge is in the left half and right edge in the right half is completed.
   - The left half's unmatched left edges cover the whole right half on their y-range; the right half's unmatched right edges cover the whole left half. Those stripes of the other half are blackened (their trees emptied).
   - Both stripe lists are refined to the union of the partitions, with refined stripes sharing tree pointers, and each stripe's two trees are joined under one new node.
   - Blackening removes every leaf that a spanning rectangle covers, so leaves stay true transitions. Coverage between leaves follows from the neighbouring leaf, and a root stripe starts uncovered at −∞.
3. **Queries.** Bottom edges at one y, merged when they touch, ask the stripe just below for the uncovered runs of their x-range; top edges ask the stripe just above. A query costs `O(log(n) + runs)`: one descent for the state before the range, then an in-order walk of the leaves inside it, each a true transition.
4. **Vertical pieces** come from the same procedure on the transposed rectangles, reversed because a reflection flips orientation.

The trees work in rank space: leaves and node keys are x-ranks among the rectangles' distinct x coordinates (nodes are 20 bytes), queries pass the ranks of their x-range, and the pieces are mapped back to coordinates at the end. The same rank-space pieces feed `rectangleUnionContours`.

Time `O(n * log(n) + out)`; memory `O(n * log(n) + out)` for the shared tree nodes. Kept beside the sweep by decision (2026-10-19). In the round-3 measurements below the sweep is as fast or faster at every size but one (the 600-bar lattice, 4%): its extra `log(n)` factor on the output does not show yet, and the shared trees cost memory traffic.

### rectangleUnionContours

Returns the union's boundary as rings of corners: `vector<vector<Point2<T>>>`, every ring directed with the union on its left, so outer boundaries are counterclockwise and holes clockwise. Consecutive vertices always turn (no collinear vertices), and the edges of all rings together are exactly the maximal pieces of `rectangleUnionBoundary`, each used once.

- **Linking.** Güting's pieces, in rank space, are sorted twice by a two-pass counting sort in `O(n + out)`: by start vertex and by end vertex, x-rank then y-rank. Every vertex has as many entering as leaving pieces (one or two), so the two sorted lists hold the same vertex at every position. The entering pieces at a vertex are paired with the leaving pieces at the same positions.
- **Pinch convention.** Where four pieces meet (two rectangles touching only at a corner, or a hole touching the outside at a corner), each entering piece continues with the leaving piece on its right. The ring then keeps the same complement region on its right, so rings bound the components of the closed union:
  - two squares touching at a corner form one ring that visits the corner twice;
  - a hole whose corner touches the outside is its own clockwise ring.

  A ring visits a vertex at most twice and never crosses itself.
- **Order.** The pieces are walked in start-vertex order and a ring starts at the first piece not yet used, so each ring starts at its lexicographically smallest vertex (x, then y) and the rings come out in that order. A ring visits its smallest vertex once: at a four-piece vertex, the pairing that visits it twice includes an edge to its left or below it. A hole can start at a corner it shares with the outer ring (the union in the north-west and south-east quadrants there).

`O(n * log(n) + out)` time and memory, with Güting's pieces. Added in round 3 (2026-10-19).

### orthogonalIntersectionCount, orthogonalIntersections

These count or list the pairs `(horizontal i, vertical j)` that share a point (closed segments, so touching counts). Parallel overlaps are not pairs. The sweep orders events at equal x as horizontal starts, then vertical queries, then horizontal ends. The count uses a `Fenwick<int>` over the compressed y values of the horizontals: `O((n + m) * log(n + m))` time, `O(n + m)` memory, `lng` result. The report keeps a `set` of active `(y, index)` pairs. Each vertical lists the active horizontals with `y` in its range, in increasing `y`, so the output is grouped by vertical in increasing x. That costs `O((n + m) * log(n + m) + out)`.

`orthogonalIntersectionCount` is kept beside Data Structures row 33 `OfflineRectangleSweep` by decision (2026-10-18, `duplicates.md`).

### Legacy mapping

| Old name | Replacement |
|---|---|
| `getLenSegUnion(vector<pair<int, int>> &segs)` (`OLD/Team Notebook/src/geometry/getlensegunion.cpp`; it swapped endpoints in place and returned `lng`) | `segmentUnionLength(segs)` (input is not modified; returns `lll`) |

## Feature-to-test map

`96-Local Testing/03-Geometry/13-segment_union_tester.py` runs quick, full and stress modes in the optimized, checked and ASan/UBSan configurations, with four assertion probes:
- a coordinate beyond `4 * 10^18`;
- `k = 0` for `rectangleCoverageArea`;
- a NaN rectangle coordinate;
- an infinite orthogonal segment.

**Oracles.**
- The depth of every elementary cell of the compressed grid, counted directly against every rectangle or segment, and all pairs for the orthogonal segments. The grid oracle is validated by fixed cases, including the POJ 1177 sample perimeter 228.
- For coverage on hundreds of rectangles, a plain depth array over the elementary y-intervals, updated interval by interval at every event.
- For rings:
  - the ring edges equal the boundary pieces as a multiset;
  - twice the summed signed ring areas equal twice the union area (`ulll` accumulation, exact at `4 * 10^18`);
  - the summed winding number at every cell centre is 1 inside the union and 0 outside;
  - each ring starts at its unique smallest vertex, and rings are ordered;
  - consecutive edges turn;
  - at every vertex shared by four pieces the turn is to the right.
- For floating input: the same rectangles at half scale in `long double` (exact for coordinates below `2^30`, where every quarter-unit area and partial sum stays under `2^62`) must give exactly a quarter of every area, half of every length, and halved witnesses, pieces and rings. `double` must equal `long double`. Random real rectangles are compared with a `long double` grid within `10^-15` relative: areas, coverage, the overlap witness, the perimeter from the covered cells' outer sides, the total length of the boundary pieces, and the rings; the sweep and Güting must return the same pieces exactly. Real orthogonal segments drawn from a pool of six coordinates (so touching happens) are compared with all pairs exactly.

| Operation | Test | Oracle |
|---|---|---|
| `segmentUnionLength` | Random segments at scale 10 and `4 * 10^18`, reversed endpoints, empty segments; `long double` endpoints at halves | Cell union; exact halves |
| `coverageCount` | Same corpus; `double` endpoints at halves | Cell depths |
| `rectangleUnionArea` | Fixed: empty, degenerate, side-sharing, corner-touching, nested and frame shapes. Every pair and (full) triple of rectangles on a 3 × 3 lattice. Random sets at scale 5, 12 and `4 * 10^18`; 13–40 rectangles at scales 4 and 20; `n`-square staircase; nested squares; halves; random reals | Grid union area; closed form `3n + 1`; exact halves; `long double` grid |
| `rectangleUnionPerimeter` | Same corpus; POJ 1177 sample; staircase | Grid boundary length; 228; `4n + 4`; halves |
| `rectangleCoverageArea` | Every `k` from 1 to `n + 1` on the grid corpus (both paths called directly at every `k`, and the public dispatch). 64–600 rectangles (blocks of several intervals) at `k` in {1, 2, 3, 5, 16, 17, 18, random, n}. Nested squares at `k` in {1, 2, 16, 17, 100, n / 2, n, n + 1}. Staircase at `k = 2` and 17; halves; random reals | Grid depths; depth array; `(2 (n − k + 1))^2`; `n − 1` and 0 |
| `maxRectangleOverlap` | Same corpus; nested squares | Grid maximum depth; depth at the witness counted directly; `n` |
| `rectangleUnionBoundary` | Same corpus; staircase segment count; a lattice of 400 thin horizontal and 400 thin vertical bars | Directed elementary boundary pieces of the grid; no two output segments mergeable; `4n`; total length equals `rectangleUnionPerimeter`; halves |
| `rectangleUnionBoundaryGuting` | Every rectangle case above, the staircase and the bar lattice | The same multiset as `rectangleUnionBoundary`, which is checked against the grid; halves |
| `rectangleUnionContours` | Every rectangle case above. Fixed: corner-touching squares (one ring through the corner twice); a hole touching the outside at a corner (its own clockwise ring); a hole starting at a corner shared with the outer ring; the frame. Staircase (one ring of `4n` corners); nested squares (one ring); bar lattice (1 + `(g − 1)^2` rings using every piece); random reals | Pieces, areas, winding numbers, order and turn rules above; exact expected rings; halves; real area and perimeter within `10^-15` |
| `orthogonalIntersectionCount` | Random segments at scale 6 and `4 * 10^18`, reversed endpoints, touching; 2,000 × 2,000 grid plus 100,000 disjoint segments; `long double` halves | All pairs; `g^2`; equal results at half scale |
| `orthogonalIntersections` | Same corpus | All pairs, the grouping order, equal results at half scale |

Build caps (domains are unchanged):
- **Checked (`_GLIBCXX_DEBUG`):** debug `lower_bound` checks the whole range on every call. The build skips the exhaustive triples and runs 50 deep cases, 5 depth-array cases of at most 200 rectangles, a 1,500-square staircase, 1,000 nested squares, 100 bars and a 300 × 300 grid.
- **ASan:** the quarantine and redzones multiply the contour trees' memory (a 3.4 GB peak at `n = 100,000` before this cap). The build caps the large phase at `n = 25,000` (which also selects the 300 × 300 grid), 5,000 nested squares and 200 bars.
- **Optimized:** nested squares are capped at 20,000 in every mode. The blocks touch every block on every event there.

Mutation checks (2026-10-19, quick, optimized):
- Caught:
  - removing the adds-first split from the perimeter;
  - making `maxRectangleOverlap` keep right ends closed;
  - Güting without blackening the left half;
  - Güting losing the state before the query range;
  - (round 3) the block pointer moving on `>` instead of `>=`;
  - the `k + 1` pointer never stepping past an exact `k`;
  - a partial block update skipping its merge (by the depth-array phase);
  - a tree leaf counting `j < count`;
  - rings turning left where four pieces meet;
  - rings started in end-vertex order.
- Not caught, equivalent in results:
  - right edges first at equal x in Güting: the extra transitions it leaves bound zero-length runs, which are never reported;
  - moving the dispatch threshold to `k = 64`;
  - never recounting floating block totals: the drift is a few rounding errors per change, within the tolerance at test sizes.

## Commands and results

2026-10-19 (round 3, final code), Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `13-segment_union_tester.py --mode quick` (optimized, checked, 4 probes) | PASS, 194,820 checks (optimized), 220,669 (checked), 4 probes |
| `13-segment_union_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 4 probes) | PASS, 1,853,795 checks in the optimized and ASan/UBSan builds, 731,567 in the capped checked build, 4 probes, `MEMORY peak=2091MB` |
| same with `CXX=g++-14` | PASS, same counts, 4 probes, `MEMORY peak=2083MB` |
| `13-segment_union_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 17,600,194 checks, `MEMORY peak=489MB` |
| `-Wall -Wextra -Wconversion` builds of the tester and benchmark with `g++` and `g++-14`, with and without `-DNDEBUG`; the header alone with `int` rectangles and `double` segments | No warnings; correct results |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 168 headers, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py` | No errors |
| `03-consistency.py --braces` on the header, tester and benchmark | No violations; comment share 32 of 419 lines |

The full-mode peak comes from the ASan/UBSan build: Güting's contour trees on the 25,000-square staircase, built twice (pieces and rings) under redzones and quarantine.

```bash
python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/13-segment_union_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/03-Geometry/13-segment_union_benchmark.py' --seed 20260927
```

## Benchmarks

`13-segment_union_benchmark.py --seed 20260927` (2026-10-19, round 3; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; other sessions kept the load average near 2.2 on the 8-core CPU; run serially after all round-3 changes).
- **Validation:** both boundary versions return the same segment multiset, the rings use every piece, and both coverage paths agree.
- **Inputs:**
  - random rectangles have sides up to `3 * 10^7` in a `10^9` square;
  - the bar lattice is `g` thin horizontal and `g` thin vertical bars, so the output is `Theta(g^2)` segments;
  - coverage uses short rectangles (sides up to `6 * 10^9 / sqrt(n)`) or tall ones (sides up to `10^9`, so most events shift every block).

| Case | n | Output | `rectangleUnionBoundary` (µs) | `rectangleUnionBoundaryGuting` (µs) | `rectangleUnionContours` (µs) |
|---|---|---|---|---|---|
| random | 1,000 | 3,900 | 3,515 | 3,801 | 4,170 |
| random | 10,000 | 14,452 | 46,742 | 52,223 | 49,312 |
| random | 100,000 | 1,120 | 521,527 | 524,256 | 559,593 |
| bar lattice | 200 | 40,002 | 965 | 1,201 | 3,323 |
| bar lattice | 600 | 360,002 | 13,069 | 12,550 | 24,895 |
| bar lattice | 2,000 | 4,000,002 | 96,079 | 185,768 | 444,542 |

`rectangleUnionContours` as a whole costs 2.0–2.8× Güting on output-heavy inputs; the ring linking itself (two counting sorts and a walk) adds 1.0–1.8× Güting's time.

`rectangleCoverageArea` paths, called directly (µs):

| Rectangles | n | k = 2 | 8 | 16 | 17 | 32 | 64 |
|---|---|---|---|---|---|---|---|
| short, tree | 10,000 | 16,034 | 21,440 | 33,505 | 35,841 | 47,657 | 61,228 |
| short, blocks | 10,000 | 13,104 | 13,381 | 14,596 | 14,759 | 14,398 | 12,717 |
| tall, tree | 10,000 | 15,586 | 24,908 | 29,772 | 32,314 | 40,405 | 83,449 |
| tall, blocks | 10,000 | 33,533 | 34,046 | 33,158 | 37,885 | 30,711 | 32,048 |
| short, tree | 100,000 | 263,074 | 343,361 | 510,937 | 553,607 | 860,205 | 1,208,277 |
| short, blocks | 100,000 | 261,820 | 241,192 | 271,670 | 227,221 | 229,935 | 262,649 |
| tall, tree | 100,000 | 229,335 | 389,037 | 485,774 | 530,281 | 869,002 | 1,291,957 |
| tall, blocks | 100,000 | 912,137 | 951,993 | 979,358 | 902,858 | 1,010,699 | 978,289 |

The blocks cost does not depend on `k`; the tree grows linearly with `k`. The dispatch at `k = 16` costs at most about 2.3× either way here: 1.9× on short rectangles at `k = 16` (`n = 10^5`), 1.7× on tall ones at `k = 17`; at `k = 2` the tree is up to 4× faster on tall rectangles, which is why small `k` keeps it. During tuning (min of three, same machine), tall rectangles at `n = 4 * 10^5` put the crossover near `k = 64`. That is where the tree's memory, `8 (k + 1)` bytes per elementary interval, reaches 400 MB, so the threshold stays at 16. The block size `sqrt(m / 32)` was the fastest of `m / 8`, `m / 16`, `m / 32`, `m / 64` on short rectangles and tied on tall ones.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [cp-algorithms: length of the union of segments](https://cp-algorithms.com/geometry/length-of-segments-union.html) and [OI Wiki 扫描线](https://oi-wiki.org/geometry/scanning/): the 1D union and the area and perimeter sweeps (POJ 1177).
- [CSES 1740 Intersection Points](https://cses.fi/problemset/task/1740), [CSES 1741 Area of Rectangles](https://cses.fi/problemset/task/1741) and [suisen geom/segment_intersections.hpp](https://suisen-cp.github.io/cp-library-cpp/library/geom/segment_intersections.hpp): the orthogonal count (closed ranges there too) and the union area.
- [Barbay, Pérez-Lantero, Rojas-Ledesma, Depth distribution in high dimensions](https://arxiv.org/abs/1705.10022): the `O(n^1.5 * log(n))` all-depth distribution, which is not adopted.
- [Güting, Optimal divide-and-conquer to compute measure and contour for a set of iso-rectangles, Acta Informatica 21 (1984) 271–291](https://radhesh-sarma.github.io/DAA_Assignment/assets/Guting%20paper.pdf) and Preparata & Shamos §8.5: the divide-and-conquer contour (implemented from the paper's description, pp. 277–288) and the Lipski–Preparata sweep family the plain version belongs to.
- `OLD/Team Notebook/src/geometry/getlensegunion.cpp`: legacy, superseded. Its events at equal x sorted removals first, which does not affect the length.

## Limits and handoffs

- `rectangleCoverageArea` answers one `k` in `O(n * k * log(n))` for `k <= 16` and `O(n * sqrt(n))` above, with `O(n)` memory. The whole depth distribution in `O(n^1.5 * log(n))` (Barbay et al.) belongs to the planned esoteric rectangle-depth package (round 3, E2).
- `rectangleUnionBoundary` is `O((n + out) * log(n))`. `rectangleUnionBoundaryGuting` and `rectangleUnionContours` reach the optimal `O(n * log(n) + out)` at `O(n * log(n))` working memory. A linear-memory contour sweep (Güting's contracted segment tree) belongs to the same planned package (E4).
- Floating coordinates are exact in structure and rounded in sums; there is no tolerance-based merging of nearly equal coordinates. Online anchored unions (maspypy `incremental_rectangle_union`) belong to Data Structures.

## History

- 2026-10-18: first implementation (P037). Independent review: the documented `4 * 10^18` bounds for segments were removed (lengths are taken in `lll`), `k` is raised to 1 under `NDEBUG`, and the order check of the report test was tightened. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit added `rectangleUnionBoundaryGuting`. Independent review: page references unified, the ASan cap documented. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19 (round 3): floating coordinates for every rectangle and orthogonal operation, a square-root-decomposition coverage path with `O(n)` memory beside a compact tree, and `rectangleUnionContours`. Güting moved to rank space at unchanged speed. Full suite passed on g++ and g++-14, stress passed. Independent review: the half-scale exactness guard lowered to `2^30`; independent real-coordinate perimeter, boundary and orthogonal oracles added; benchmark wording corrected. Final full suite passed on g++ and g++-14, stress passed.
