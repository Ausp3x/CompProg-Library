# 19-lattice_geometry.hpp — evidence

Owned by package P037 / GE27. Every operation in the row has a test.

## Contracts

### Numeric policy

Every count is exact and returned as `lll`.

- Rational inputs are normalized `RationalPoint2` values (`d > 0`, gcd 1).
- For `latticeUnderLine` and `latticeInRationalPolygon`, write `V` for the largest `ceil(|x| / d)` or `ceil(|y| / d)` and `D` for the largest `d`. The domain is `V * D^3 <= 2 * 10^18` with `D <= 1.6 * 10^6`, asserted. Under it the line through two vertices has integral direction `pd * (q − p)` below `4 * 10^18`, the reduced slope denominator fits `lng` as `floorSum` requires, the line constants stay below `8 * 10^36`, and cross products of directions stay below `3.2 * 10^37`.
- For `latticeInHalfplanePolygon`, the bounded intersection must lie within `|coordinate| <= 10^18`, asserted. Its edges lie on the integral lines of the halfplanes, with coefficients below `2 * 10^9` and constants below `4 * 10^18`, even though its vertices have denominators up to `8 * 10^18`.
- Disk counts take `|a|, |b| <= 10^18`, `1 <= q <= 10^18` and `0 <= n <= 4 * 10^18`, asserted. Squares stay below `4 * 10^18` and the slope tests below `1.6 * 10^37`.

### latticeUnderLine

For rational `p`, `q` with `p.x != q.x` (asserted), in either order: the sum of `floor(l(x))` over integers `x` in the closed range between `p.x` and `q.x`, where `l` is the line through `p` and `q`. This is the number of lattice points with `0 < y <= l(x)`, counted negatively where `l(x) < 0` (cp-algorithms' `count_lattices`). One `floorSum` call on the gcd-reduced slope: `O(log(C))`. Kept by decision (2026-10-18) beside `floorSum`.

### latticeInRationalPolygon

Input: a simple polygon with rational vertices, either orientation (decided by the turn at the lexicographically lowest vertex, which is convex), nonzero area, no repeated consecutive vertices. `closed = false` counts lattice points strictly inside; `closed = true` adds the boundary. The method follows.

- **The edge sum.** Let `δ = (ε', −ε)` with `0 < ε' << ε`, a symbolic shift down and slightly right. `S = Σ over edges of sign * Σ floor(l(x))`, over the half-open column ranges `x in [x_min, x_max)`. The sign is `+1` for edges moving left (upper edges of a CCW ring) and `−1` for edges moving right; vertical edges are skipped. `S` is exactly the number of lattice points `P` with `P + δ` in the interior. The baseline below the polygon cancels, because every column meets as many rightward as leftward edges.
- **Interior.** Boundary points where `δ` points inward are subtracted: the open lattice points of upper edges, those of downward vertical edges, and every lattice vertex whose open interior cone contains `δ`. The cone test uses the symbolic signs `cross(u, δ) = u.x ? −sign(u.x) : −sign(u.y)`, which are never zero.
- **Boundary.** Open lattice points per edge (extended gcd on the reduced line `dx * y − dy * x = k`, counted in the open x or y range), plus the lattice vertices.

`O(n * log(C))` time, `O(n)` memory.

### latticeInHalfplanePolygon

Input: halfplanes as in `halfplaneIntersection`, whose result must be bounded: Empty, Point, Segment or Polygon (asserted).

- `closed = false` counts the lattice points of the interior. It is 0 unless the kind is Polygon, because a degenerate set has no interior.
- `closed = true` counts the closed set: a lattice Point, the lattice points of the Segment, or the polygon count above.
- The polygon edges are the planes in CCW order, between consecutive vertices, with integral directions and constants. No rational vertex is cross-multiplied. Rational comparisons go through integer parts and remainders.

`O(m * log(m) + k * log(C))` for `m` halfplanes and `k` result edges. Kept by decision (2026-10-18) beside `latticeInRationalPolygon(halfplaneIntersection(h).vertices)`, which overflows on these denominators.

### latticeInDisk, latticeInDiskFast

Both count lattice points `(x, y)` of the closed disk `(q x − a)^2 + (q y − b)^2 <= n`: centre `(a / q, b / q)`, squared radius `n / q^2`. Write `r = sqrt(n) / q`.

`latticeInDisk` scans the columns with `(q x − a)^2 <= n` and adds the height of each column from an exact integer square root: `O(r + 1)` time, `O(1)` memory.

`latticeInDiskFast` gives the same count:
- **Decomposition.** The lattice points are split by the diagonals through the centre into the centre point, two caps with `|u| <= v` (top and bottom) and two with `|u| < v` (left and right, by transposition), where `u = x − a / q` and `v = y − b / q`. The asymmetric tie keeps the partition disjoint.
- **One cap.** Each cap is `Σ (top(x) − low(x) + 1)` over the columns with `2 (q x − a)^2 <= n`. `low` is an arithmetic series, because the diagonal has slope ±1. `top` is the floor of the arc. On each side of the centre column the arc is concave with slope in `[−1, 0]`, the left side by mirroring.
- **The hull walk.** It walks the upper convex hull of the lattice points under the arc. It keeps a Stern–Brocot stack of directions, with sentinel `(1, 1)`, feasible because the slope is at least −1. It takes repeated steps by doubling and closed-form sums along each edge. A new direction is refined by mediants between a fitting direction `f` and a failing one `g`. Refinement stops when the arc at `x + m.x` is at least as steep as `f`: then no mediant between `m` and `f` can fit, since the points `P + m + j f` stay above a concave arc. The slope test is the exact inequality `U^2 f.x^2 >= (n − U^2) f.y^2` with `U = q x − a >= 0`.

Complexity: the lattice hull of a disk has `O(r^(2/3))` vertices (Balog–Bárány), and each costs `O(log(r))` steps, so `O(r^(2/3) * log(r) + 1)`. The benchmark measures about 5× per tenfold `r`. The direction stack is bounded by the number of refinements; at `r = 10^6` and `10^8` it peaked at 848 and 8,477 entries (about `0.85 * sqrt(r)`), with 114,589 and 2,554,636 refinements.

Both disk counts are kept by decision (2026-10-18) beside `gaussCircleCount` (Mathematics row 34, origin) and the planned `gaussCircleFast` (Mathematics row 55): the rational centre and radius are a larger domain.

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/19-lattice_geometry_tester.py` runs quick, full and stress modes in three configurations, with six assertion probes: `q = 0`, `n < 0`, a vertical `latticeUnderLine`, a two-vertex polygon, a zero-area polygon, and an unbounded halfplane set.

**Oracles.**
- Enumeration of the bounding box with exact integer membership: the disk; the rational polygon by exact crossing parity and on-edge tests; the halfplanes by `Halfplane::contains`.
- Per-column rational floors for `latticeUnderLine`.
- Pick counts from 03 `latticeInterior` and `latticeBoundary`, and `gaussCircleCount` from Mathematics row 34.
- Closed-form floor sums for two large polygons.

| Operation | Test | Oracle |
|---|---|---|
| `latticeInDisk` | Exhaustive `q <= 4`, centre numerators within `2q`, `n <= 70`; random `q <= 12`, centres within `10^3`, radius up to 30; origin `n` up to `10^6` | Enumeration; `gaussCircleCount` |
| `latticeInDiskFast` | The same, plus radius up to 1,000 with centres up to `10^9` and `q <= 1000`; radius 300,000–1,000,000 with `q <= 9`; origin and integral centres up to `n = 10^12`; known values `r = 10^3, 10^4, 10^6`; `n = 4 * 10^18`; `q = 10^18` | `latticeInDisk`; enumeration; `gaussCircleCount`; published Gauss counts |
| `latticeUnderLine` | Random rational endpoints, denominators 1–7, both orders; a `2 * 10^9`-wide segment | Per-column floors; closed form |
| `latticeInRationalPolygon` | Random simple polygons (3–9 vertices, denominators 1, 2 and up to 6, checked simple exactly); convex polygons near a circle of radius 1.9 with denominators up to `10^6`, at the domain edge `V * D^3 = 2 * 10^18`; rectilinear L-shapes with vertical edges through lattice and half-lattice points; random lattice polygons; a `±10^9` lattice triangle; a half-integer triangle of width `10^9` | Enumeration, both orientations; Pick; `floorSum` closed form |
| `latticeInHalfplanePolygon` | Random halfplanes inside a box, with doubled lines making segments and points; a thin triangle reaching `(10^18 − 10^9, 10^9)` | Enumeration, closed and strict; `floorSum` and `ceilSum` closed form and exact boundary count |

Mutation checks (2026-10-18, quick, optimized):
- Caught: dropping the centre-column correction of a nonstrict cap; using the convex cone test at reflex vertices.
- Not caught, runtime only: removing the steepness stop. Without it, refinement still ends at the range bound.

## Commands and results

2026-10-18, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `19-lattice_geometry_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 6 probes) | PASS, 133,185 checks per configuration, 6 probes, `MEMORY peak=532MB` |
| same with `CXX=g++-14` | PASS, 133,185 checks per configuration, 6 probes, `MEMORY peak=557MB` |
| `19-lattice_geometry_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 506,058 checks, `MEMORY peak=390MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 167 headers, `MEMORY peak=650MB` |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`19-lattice_geometry_benchmark.py --seed 20260927` (2026-10-18; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; input generation excluded; both disk counts agree wherever the row scan runs). Rows with `r = 10^9` and `q = 7` are capped at `n = 4 * 10^18`, so their effective radius is about `2.9 * 10^8`.

| Centre | r | `latticeInDiskFast` (µs) | `latticeInDisk` (µs) |
|---|---|---|---|
| origin | 10^4 | 113 | 609 |
| origin | 10^5 | 538 | 6,022 |
| origin | 10^6 | 2,794 | 58,876 |
| origin | 10^7 | 14,511 | 603,009 |
| origin | 10^8 | 73,690 | — |
| origin | 10^9 | 496,754 | — |
| (a, b) / 7 | 10^4 | 111 | 687 |
| (a, b) / 7 | 10^5 | 583 | 6,408 |
| (a, b) / 7 | 10^6 | 3,033 | 63,041 |
| (a, b) / 7 | 10^7 | 14,574 | 641,072 |
| (a, b) / 7 | 10^8 | 70,562 | — |

Per tenfold radius the hull walk grows about 5× (`10^(2/3) ≈ 4.6` times a logarithm) and the row scan 10×. The walk is faster at every measured radius; the row scan stays as the simpler `O(r)` reference with the same domain.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [cp-algorithms: lattice points inside non-lattice polygon](https://cp-algorithms.com/geometry/lattice-points.html): the per-edge floor sums. It leaves boundary points to manual care; the symbolic shift here settles them.
- [maspypy convex/lattice_point_count.hpp](https://maspypy.github.io/library/convex/lattice_point_count.hpp): halfplane input with closed constraints.
- [maspypy nt/convex_floor_sum.hpp](https://maspypy.github.io/library/nt/convex_floor_sum.hpp), [259-momone, lattice points under convex curves](https://259-momone.hatenablog.com/entry/2021/07/25/025655) and [Nyaan enumerate-convex](https://nyaannyaan.github.io/library/math/enumerate-convex.hpp): the Stern–Brocot hull walk with an inside oracle and a slope oracle. Their code was not copied; the walk and its stop rule were derived and tested here.
- [AtCoder ABC191 D](https://atcoder.jp/contests/abc191/tasks/abc191_d): a closed disk with a decimal centre and radius, the rational-centre use case.

## Limits and handoffs

- The generic oracle walk over any convex region (`latticeInConvexRegion`), the lattice hull under a line, and polynomial lattice sums belong to Mathematics row 55 (`convexLatticeHull`, `convexFloorSum`, `floorSumPolynomial`); see 00-notes.
- Unbounded halfplane intersections are rejected: the closed count is infinite, and the open count may be finite but is not computed.

## History

- 2026-10-18: first implementation (P037). Independent review: `isqrt` of a negative value looped under `NDEBUG` (now 0); the line-constant bound became `8 * 10^36`; `latticeInDiskFast` has its own complexity line; large-denominator cases were added; the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
