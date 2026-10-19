# 19-lattice_geometry.hpp — evidence

Owned by package P037 / GE27. Every operation in the row has a test.

## Contracts

### Numeric policy

Every count is exact and returned as `lll`.

- Rational inputs are normalized `RationalPoint2` values (`d > 0`, gcd 1).
- For `latticeUnderLine` and `latticeInRationalPolygon`, write `V` for the largest of 1, `ceil(|x| / d)` and `ceil(|y| / d)`, and `D` for the largest `d`. The domain is `V * D <= 4 * 10^18`, asserted. Examples: coordinates up to `10^9` with denominators up to `4 * 10^9`, coordinates up to `10^5` with up to 13 decimal places, or values below 2 with denominators up to `2 * 10^18`. Under it:
  - Each edge lies on the line `dx * y − dy * x = c` with a primitive direction `(dx, dy)`, a multiple of `q − p`, below `2 V D^2 <= 3.2 * 10^37`.
  - The constant `c = (dx * p.y − dy * p.x) / p.d` is computed from 256-bit products and floored (with an exactness flag). Its size is at most `4 V^2 D^2 <= 6.4 * 10^37`.
  - The floor sums use `floor((c + dy * x) / dx) = floor((floor(c) + dy * x) / dx)` for `dx > 0`, so their modulus is `|dx|`. They call `floorSum` when the modulus fits `lng` and the coefficients stay within `2^126`, and Mathematics row 18 `floorSumWide` otherwise. The shifted constant stays below `9.6 * 10^37`.
  - Cross products of directions (orientation, cone tests) use exact 256-bit signs. The 256-bit products, signs and the word division of the line constant use Core `u256` (`01-Core/07-infint.hpp`) instead of private helpers. Against those helpers it measured +9% at denominators up to `10^6` and +2% at `2 * 10^18` (20,000 convex polygons, load average about 4), accepted to remove duplicated arithmetic.
  - The lattice points on an edge use a 128-bit modular multiplication.
- For `latticeInHalfplanePolygon`, a bounded intersection must lie within `|coordinate| <= 10^18`, asserted; unbounded intersections need no arithmetic beyond `lll` products of input coordinates. Its edges lie on the integral lines of the halfplanes, with coefficients below `2 * 10^9` and constants below `4 * 10^18`, even though its vertices have denominators up to `8 * 10^18`.
- Disk counts take `|a|, |b| <= 10^18`, `1 <= q <= 10^18` and `0 <= n <= 4 * 10^18`, asserted. Squares stay below `4 * 10^18` and the slope tests below `1.6 * 10^37`.

### latticeUnderLine

For rational `p`, `q` with `p.x != q.x` (asserted), in either order: the sum of `floor(l(x))` over integers `x` in the closed range between `p.x` and `q.x`, where `l` is the line through `p` and `q`. This is the number of lattice points with `0 < y <= l(x)`, counted negatively where `l(x) < 0` (cp-algorithms' `count_lattices`). One floor sum on the primitive direction and floored constant: `O(log(C))`, with a larger constant when it needs `floorSumWide`. Kept by decision (2026-10-18) beside `floorSum`.

### latticeInRationalPolygon

Input: a simple polygon with rational vertices, either orientation (decided by the turn at the lexicographically lowest vertex, which is convex), nonzero area, no repeated consecutive vertices. `closed = false` counts lattice points strictly inside; `closed = true` adds the boundary. The method follows.

- **The edge sum.** Let `δ = (ε', −ε)` with `0 < ε' << ε`, a symbolic shift down and slightly right. `S = Σ over edges of sign * Σ floor(l(x))`, over the half-open column ranges `x in [x_min, x_max)`. The sign is `+1` for edges moving left (upper edges of a CCW ring) and `−1` for edges moving right; vertical edges are skipped. `S` is exactly the number of lattice points `P` with `P + δ` in the interior. The baseline below the polygon cancels, because every column meets as many rightward as leftward edges. Partial sums can pass `2^127` before cancelling (a unit-width spiral at the domain edge does), so `S` accumulates modulo `2^128`; the final count fits `lll`.
- **Interior.** Boundary points where `δ` points inward are subtracted: the open lattice points of upper edges, those of downward vertical edges, and every lattice vertex whose open interior cone contains `δ`. The cone test uses the symbolic signs `cross(u, δ) = u.x ? −sign(u.x) : −sign(u.y)`, which are never zero.
- **Boundary.** Open lattice points per edge, plus the lattice vertices. A primitive line `dx * y − dy * x = c` holds lattice points only when `c` is an integer. Its x values form one residue class modulo `|dx|`, found by an extended gcd and a 128-bit modular multiplication, and are counted in the open x or y range.

`O(n * log(C))` time, `O(n)` memory.

### latticeInHalfplanePolygon

Input: halfplanes as in `halfplaneIntersection`. The result is `-1` when the set holds infinitely many lattice points, which happens only for unbounded sets.

- `closed = false` counts the lattice points of the interior. It is 0 for Empty, Point, Segment, Ray and Line, because a degenerate set has no interior.
- `closed = true` counts the closed set: a lattice Point, the lattice points of the Segment, or the polygon count above.
- The polygon edges are the planes in CCW order, between consecutive vertices, with integral directions and constants. No rational vertex is cross-multiplied. Rational comparisons go through integer parts and remainders.
- **Unbounded sets (Ray, Line, Unbounded) hold 0 or infinitely many lattice points.** Proof:
  - Closed: every such set other than the whole plane has a boundary ray or line on a plane through two lattice points, with an integral direction `w`, so it contains `s.a + t * w / gcd(w)` for infinitely many integers `t`. The whole plane is infinite too. Result `-1`.
  - Open, Ray or Line: the interior is empty. Result 0.
  - Open, Unbounded with a two-dimensional recession cone `C` (two non-parallel generators, or three or four): the interior contains `p + C` for an interior point `p`, which holds disks of every radius. Result `-1`.
  - Open, Unbounded with `C` spanned by one primitive direction `d` (a half-strip or a strip): write `L(X) = d.x X.y − d.y X.x`. The two irredundant planes parallel to `d` confine the set to `lo <= L(X) <= hi`, with integers `lo < hi` because each plane passes through a lattice point, and `L` takes every integer value on lattice points. If `hi − lo >= 2`, take a lattice `X0` with `L(X0) = lo + 1`. For every other input plane `cross(dir, d) > 0`, because `d` lies in the recession cone and is not parallel to it, so `X0 + t d` is strictly inside every plane for all large integers `t`. Result `-1`. If `hi − lo = 1`, the interior lies in `lo < L(X) < lo + 1`, which has no lattice point. Result 0.
  - This is the 2D case of Meyer's theorem (Thomas, *Integer Hulls of Rational Polyhedra*, Thm 8.2): a nonempty integer hull of a rational polyhedron has the polyhedron's recession cone.

`O(m * log(m) + k * log(C))` for `m` halfplanes and `k` result edges; `O(m * log(m))` for unbounded sets. Kept by decision (2026-10-18) beside `latticeInRationalPolygon(halfplaneIntersection(h).vertices)`, which overflows on these denominators.

### latticeInDisk, latticeInDiskFast

Both count lattice points `(x, y)` of the closed disk `(q x − a)^2 + (q y − b)^2 <= n`: centre `(a / q, b / q)`, squared radius `n / q^2`. Write `r = sqrt(n) / q`.

`latticeInDisk` scans the columns with `(q x − a)^2 <= n` and adds the height of each column from an exact integer square root: `O(r + 1)` time, `O(1)` memory.

`latticeInDiskFast` gives the same count:
- **Decomposition.** The lattice points are split by the diagonals through the centre into the centre point, two caps with `|u| <= v` (top and bottom) and two with `|u| < v` (left and right, by transposition), where `u = x − a / q` and `v = y − b / q`. The asymmetric tie keeps the partition disjoint.
- **One cap.** Each cap is `Σ (top(x) − low(x) + 1)` over the columns with `2 (q x − a)^2 <= n`. `low` is an arithmetic series, because the diagonal has slope ±1. `top` is the floor of the arc. On each side of the centre column the arc is concave with slope in `[−1, 0]`, the left side by mirroring.
- **The hull walk.** It walks the upper convex hull of the lattice points under the arc, with directions ordered from the sentinel `(1, 1)` (feasible because the slope is at least −1) to the shallowest. It takes repeated steps by doubling and closed-form sums along each edge. A new direction is refined by mediants between a fitting direction `f` and a failing, shallower one `g`. Refinement stops when the arc at `x + m.x` is at least as steep as `f`: then no mediant between `m` and `f` can fit, since the points `P + m + j f` stay above a concave arc. The slope test is the exact inequality `U^2 f.x^2 >= (n − U^2) f.y^2` with `U = q x − a >= 0`.
- **Runs.** The directions are kept as runs `{f, g, k}`, meaning `f + j g` for `j` in `[1, k]`.
  - Refinement pushes a whole run of fitting mediants, found by doubling.
  - When the walk leaves a direction, the largest still-fitting `j` of the top run is found by galloping down from its top.
  - Both searches rely on a prefix property: within a run, the fitting directions form a prefix. Right of the current point `P`, the region under the arc is convex and closed downward. If `P + f + j g` fits, then `P + f` lies on or below the chord from `P` to it, because `g` is shallower than `f`, so `P + f` fits and so does every `P + f + j' g` with `j' < j`.
  - The stack holds the left endpoints of one Stern–Brocot path. Each pop leaves the interval `[E_i, E_(i+1)]`, reached by a right move. So the runs are the left-move blocks of that path, at most one per partial quotient: `O(log(r))` runs, because every stored direction has coordinates `O(r)`.
  - The walk visits the same directions as a stack with one entry per direction, so the counts are identical. It makes fewer slope tests, because a run is pushed in `O(log(k))` tests instead of `k`.

Complexity: the lattice hull of a disk has `O(r^(2/3))` vertices (Balog–Bárány), and each costs `O(log(r))` tests, so `O(r^(2/3) * log(r) + 1)` time and `O(log(r))` memory.

Measured on instrumented copies (2026-10-19, origin, `q = 1`; the stack peak is the maximum over the four caps):

| r | Hull edges | Tests per edge | Stack peak (runs) |
|---|---|---|---|
| 10^4 | 1,608 | 9.12 | 4 |
| 10^5 | 7,272 | 9.24 | 5 |
| 10^6 | 34,736 | 9.31 | 6 |
| 10^7 | 160,568 | 9.35 | 7 |
| 10^8 | 746,056 | 9.37 | 7 |
| 10^9 | 3,459,448 | 9.38 | 8 |

Tests per edge stay flat, so the observed time grows like `r^(2/3)`. The proven bound keeps the logarithm: no published proof removes it for this walk (Sladkey proves `O(n^(1/3))` for the hyperbola with a different, tangent-based recursion). The centre `(12345, −6789) / 7` gives the same tests per edge (9.14–9.39) and at most 8 runs.

Both disk counts are kept by decision (2026-10-18) beside `gaussCircleCount` (Mathematics row 34, origin) and the planned `gaussCircleFast` (Mathematics row 55): the rational centre and radius are a larger domain.

No legacy code exists for this header.

## Feature-to-test map

`96-Local Testing/03-Geometry/19-lattice_geometry_tester.py` runs quick, full and stress modes in three configurations, with six assertion probes: `q = 0`, `n < 0`, a vertical `latticeUnderLine`, a two-vertex polygon, a zero-area polygon, and a bounded halfplane triangle whose far vertex lies near `2 * 10^18`, outside the domain.

**Oracles.**
- Enumeration of the bounding box with exact integer membership: the disk; the rational polygon by exact crossing parity and on-edge tests; the halfplanes by `Halfplane::contains`.
- Per-column rational floors for `latticeUnderLine`.
- Pick counts from 03 `latticeInterior` and `latticeBoundary`, and `gaussCircleCount` from Mathematics row 34.
- Closed-form floor sums for two large polygons.
- A Python oracle with exact `Fraction` column scans for segments and convex polygons with denominators up to `10^15` (quick 40, full 400, stress 4,000 segment rows plus polygons).
- A tester-local 256-bit location test for polygons with denominators up to `2 * 10^18`.
- Pick's formula from a tester-local shoelace sum and edge gcds for a 25-vertex unit-width spiral with coordinates below `4 * 10^18` in magnitude.
- Agreement between the halfplane and rational-vertex paths, and invariance under integral translation and transposition.
- Exact per-column scans of the windows `[-W, W]^2` with `W = 2V + 60` and `4V + 120` past every vertex `V` of an unbounded set: equal and zero counts mean 0; a larger count means infinite.

| Operation | Test | Oracle |
|---|---|---|
| `latticeInDisk` | Exhaustive `q <= 4`, centre numerators within `2q`, `n <= 70`; random `q <= 12`, centres within `10^3`, radius up to 30; origin `n` up to `10^6` | Enumeration; `gaussCircleCount` |
| `latticeInDiskFast` | The same, plus radius up to 1,000 with centres up to `10^9` and `q <= 1000`; radius 300,000–1,000,000 with `q <= 9`; origin and integral centres up to `n = 10^12`; known values `r = 10^3, 10^4, 10^6`; `n = 4 * 10^18`; `q = 10^18` | `latticeInDisk`; enumeration; `gaussCircleCount`; published Gauss counts |
| `latticeUnderLine` | Random rational endpoints, denominators 1–7, both orders; a `2 * 10^9`-wide segment; Python rows with denominators up to `10^15` | Per-column floors; closed form; `Fraction` column sums |
| `latticeInRationalPolygon` | Random simple polygons (3–9 vertices, denominators 1, 2 and up to 6, checked simple exactly); convex polygons near a circle of radius 1.9 with denominators up to `10^6`, and with denominators up to `2 * 10^18` at the domain edge `V * D = 4 * 10^18`; Python convex polygons with denominators up to `10^14`; vertices of random halfplane polygons (denominators up to about `10^12`), translated and transposed; rectilinear L-shapes with vertical edges through lattice and half-lattice points; a unit-width rectilinear spiral with three turns with coordinates below `4 * 10^18` in magnitude; random lattice polygons; a `±10^9` lattice triangle; a half-integer triangle of width `10^9` | Enumeration, both orientations; 256-bit enumeration; Pick (03 functions; shoelace and gcds for the spiral); `floorSum` closed form; `Fraction` column scans; `latticeInHalfplanePolygon`; invariance |
| `latticeInHalfplanePolygon` | Random halfplanes inside a box, with doubled lines making segments and points; a thin triangle reaching `(10^18 − 10^9, 10^9)`. Unbounded sets: 0–3 random halfplanes within `±5`, plus doubled lines and antiparallel pairs making rays, lines, strips and half-strips; fixed cases: the whole plane, a halfplane, strips of lattice width 1 and 2 (axis-parallel and along `(2, 1)`, one given by a non-primitive direction), half-strips of width 1 and 2, half-strips and strips with redundant parallel planes in either order, a wedge, a ray and a line | Enumeration, closed and strict; `floorSum` and `ceilSum` closed form and exact boundary count; window column scans and fixed values for unbounded sets |

Mutation checks (2026-10-19, quick, optimized, each on a private copy of the tree):
- Caught: dropping the centre-column correction of a nonstrict cap; using the convex cone test at reflex vertices; the 256-bit floor rounding toward zero; keeping the floor when the line constant is negated.
- Caught, unbounded sets: a strip of lattice width 1 counted infinite; the two strip lines swapped; a closed unbounded count of 0; a one-dimensional recession cone treated as two-dimensional.
- Caught, runs: the boundary direction `g` taken from the fitting side of a popped run; the push-side binary search stopping one step early.
- Caught: the floor of a negative 256-bit quotient rounding toward zero.
- Caught by UBSan only (full, ASan/UBSan, spiral phase): accumulating `S` in signed `lll`. Optimized builds wrap to the same count.
- Caught by the runner's 180 s timeout in full mode: removing the steepness stop. Without it the tests per edge grow about 4.7× per tenfold radius (219 at `r = 10^4`, 1,044 at `10^5`, 4,867 at `10^6`) and the time about 21× (1.3 s per call at `10^6`), and the `n = 4 * 10^18` case cannot finish. Quick mode, which skips that case, does not catch it.
- Equivalent, not caught: a popped run keeping one direction that no longer fits. That costs one wasted refinement, and the next walk takes no step and drops it. Also equivalent: `crossSign` returning 1 for 0. Zero arises only at a straight vertex, where both cone tests agree, and never at the lexicographically lowest vertex.
- `floorSumWide` losing the carry of `b` passes this suite, which rarely reaches that carry, and is caught by Mathematics row 18's suite.

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `19-lattice_geometry_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 6 probes) | PASS, 138,906 checks and 600 Python `Fraction` rows per configuration, 6 probes, `MEMORY peak=606MB` |
| same with `CXX=g++-14` | PASS, same counts, 6 probes, `MEMORY peak=601MB` |
| `19-lattice_geometry_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 550,936 checks and 6,000 Python `Fraction` rows, `MEMORY peak=455MB` |
| `-Wall -Wextra -Wconversion` build of the tester with `g++` and `g++-14` | No warnings |
| `02-integration.py` (every header alone, aggregates, two translation units including `99-all.hpp`) | PASS, 168 headers, `MEMORY peak=652MB` |
| `02-integration.py --sanitizers` | PASS, 168 headers, sanitizer self-tests, `MEMORY peak=1833MB` |
| `03-consistency.py` | No errors |
| `03-consistency.py --braces` on the header, tester and benchmark | No violations |
| Mathematics `20-continued_fraction_tester.py --mode full --seed 1` (includes `18-floorsum.hpp`), `g++` and `g++-14` | PASS, 3,805,206 checks and 3000 Python rows per configuration, 17 probes, `MEMORY peak=818MB` and `817MB` |

```bash
python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/19-lattice_geometry_tester.py' --mode stress --seed 42 --configuration optimized
```

## Benchmarks

`19-lattice_geometry_benchmark.py --seed 20260927` (2026-10-19, run serially on the final code; other sessions kept the load average near 2.6 on the 8-core CPU; Intel Core i9-11900H, GCC 16.2.1 `-O2 -DNDEBUG`, one warmup, median of five; input generation excluded; both disk counts agree wherever the row scan runs). Rows with `r = 10^9` and `q = 7` are capped at `n = 4 * 10^18`, so their effective radius is about `2.9 * 10^8`.

| Centre | r | `latticeInDiskFast` (µs) | `latticeInDisk` (µs) |
|---|---|---|---|
| origin | 10^4 | 128 | 575 |
| origin | 10^5 | 590 | 5,641 |
| origin | 10^6 | 2,906 | 57,226 |
| origin | 10^7 | 15,620 | 543,649 |
| origin | 10^8 | 71,332 | — |
| origin | 10^9 | 521,782 | — |
| (a, b) / 7 | 10^4 | 131 | 681 |
| (a, b) / 7 | 10^5 | 654 | 6,116 |
| (a, b) / 7 | 10^6 | 3,222 | 57,222 |
| (a, b) / 7 | 10^7 | 13,927 | 578,352 |
| (a, b) / 7 | 10^8 | 70,116 | — |
| (a, b) / 7 | 10^9 (capped, about `2.9 * 10^8`) | 144,823 | — |

Per tenfold radius the hull walk grows 4.3–7.3× (`10^(2/3) ≈ 4.6`, with tests per edge flat) and the row scan 10×. The walk is faster at every measured radius; the row scan stays as the simpler `O(r)` reference with the same domain.

## Sources

The implementation was written independently. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p037-completeness-sweep-fetched-2026-10-18).

- [cp-algorithms: lattice points inside non-lattice polygon](https://cp-algorithms.com/geometry/lattice-points.html): the per-edge floor sums. It leaves boundary points to manual care; the symbolic shift here settles them.
- [maspypy convex/lattice_point_count.hpp](https://maspypy.github.io/library/convex/lattice_point_count.hpp): halfplane input with closed constraints.
- [maspypy nt/convex_floor_sum.hpp](https://maspypy.github.io/library/nt/convex_floor_sum.hpp), [259-momone, lattice points under convex curves](https://259-momone.hatenablog.com/entry/2021/07/25/025655) and [Nyaan enumerate-convex](https://nyaannyaan.github.io/library/math/enumerate-convex.hpp): the Stern–Brocot hull walk with an inside oracle and a slope oracle. Their code was not copied; the walk and its stop rule were derived and tested here.
- [AtCoder ABC191 D](https://atcoder.jp/contests/abc191/tasks/abc191_d): a closed disk with a decimal centre and radius, the rational-centre use case.
- Read 2026-10-19: [Thomas, *Integer Hulls of Rational Polyhedra*, Thm 8.2](https://sites.math.washington.edu//~thomas/teaching/m583_s2008_web/berlinnotes.pdf) (Meyer's theorem) for the 0-or-infinite count of unbounded sets. [Sladkey, arXiv 1206.3369](https://arxiv.org/abs/1206.3369) proves `O(n^(1/3))` time and `O(log(n))` space for the hyperbola by tangent recursion; it is not used here.

## Limits and handoffs

- The generic oracle walk over any convex region (`latticeInConvexRegion`), the lattice hull under a line, and polynomial lattice sums belong to Mathematics row 55 (`convexLatticeHull`, `convexFloorSum`, `floorSumPolynomial`); see 00-notes.
- A bounded halfplane intersection must lie within `|coordinate| <= 10^18` (128-bit floor sums); widening was declined on 2026-10-19.
- `latticeInDiskFast` keeps `O(r^(2/3) * log(r))` as its proven bound although the measured tests per edge are flat; see the hull walk above.

## History

- 2026-10-18: first implementation (P037). Independent review: `isqrt` of a negative value looped under `NDEBUG` (now 0); the line-constant bound became `8 * 10^36`; `latticeInDiskFast` has its own complexity line; large-denominator cases were added; the comment share was reduced. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit widened rational polygons to `V * D <= 4 * 10^18` (primitive directions, floored constants, 256-bit products, `floorSumWide`) with a Python `Fraction` oracle. Independent review: the edge sum overflowed `lll` on spirals (now modulo `2^128`, with a spiral regression), complexity comments fixed. Full suite passed on g++ and g++-14, stress passed.
- 2026-10-19 (round 3): unbounded halfplane sets return 0 or `-1` (Meyer's dichotomy, window-scan oracle); the hull-walk stack became `O(log(r))` runs with identical counts and fewer tests (at most 8 runs where the former stack reached 44,721 entries); Core `u256` replaced the private 256-bit helpers (+2–9% measured); the steepness-stop mutant is caught by the full-mode timeout; the unbounded probe became an out-of-domain bounded probe. Full suite passed on g++ and g++-14, stress passed, Mathematics row 20 full passed. Independent review: counts refreshed, the steepness-mutant growth figure corrected; strips with redundant parallel planes added to the unbounded tests. Final full suite passed on g++ and g++-14, stress passed.
