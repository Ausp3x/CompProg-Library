# 10-halfplaneintersection.hpp — evidence

Owned by package P036 / GE03, together with [11-minkowskisum.md](11-minkowskisum.md). P036 also owns [16-circle_polygon.md](16-circle_polygon.md) (GE24) and [17-polygon_distance.md](17-polygon_distance.md) (GE25). Every operation in the row has a test.

## Contracts

### Numeric policy

Halfplanes have integral endpoints with `|coordinate| <= 10^9`, asserted once per plane when an operation consumes it. Every predicate is exact. The side of the meeting point of boundaries `p` and `q` relative to `h` is `sign(d * cross(w, p.a - h.a) + n * cross(w, u)) * sign(d)`, where `u`, `w` are the directions of `p` and `h`, `d = cross(u, dir(q))` and `n = cross(q.a - p.a, dir(q))`. Each product is at most `8 * 10^18 * 8 * 10^18`, so the degree-4 sum is at most `1.28 * 10^38 < 2^127`. Vertices are `RationalPoint2` values `(a * d + u * n) / d` with `|x|, |y| <= 2.4 * 10^28` and `0 < d <= 8 * 10^18`. Neither the decisions nor the vertices use floating point. Only `chebyshevCenterApprox` is rounded (`long double`, no certified bound). No operation adds a bounding box to the user's halfplanes. `polygonKernelLinear` adds the polygon's own bounding box internally, which is legitimate because the kernel lies inside the polygon.

### Halfplane

`a`, `b` define the closed left side of `a -> b`. `contains(point, strict)` is the exact `orient(a, b, p) >= 0` (`> 0` when strict). `contains(RationalPoint2, strict)` evaluates `cross(b - a, p - a * d)` in `lll`. It is exact for `|x|, |y| <= 3 * 10^28` and `0 < d <= 8 * 10^18`, a range that covers every vertex this header produces. `contains` does not check `a != b`; the algorithms do.

### HalfplaneKind, HalfplaneResult

The kinds separate every topology of a closed polyhedral subset of the plane. `Unbounded` means two-dimensional and unbounded; the one-dimensional unbounded cases are `Ray` and `Line`.

| Kind | `vertices` | `recession` | `planes` |
|---|---|---|---|
| `Empty` | empty | empty | empty |
| `Point` | the point | empty | input planes whose intersection is the point |
| `Segment` | two endpoints, ordered along the first plane of the coincident antiparallel pair | empty | that pair and the two cutting planes |
| `Ray` | the apex | the ray direction | the pair and the cutting plane |
| `Line` | empty | `{u, -u}` | the pair |
| `Polygon` | CCW, strictly convex, `k >= 3`; `vertices[i]` starts the edge on `planes[i]`, and `planes[0]` has the smallest polar angle (`polarLess` from `+x`) | empty | the irredundant boundary planes |
| `Unbounded` | the finite boundary chain, `vertices[i] = planes[i] ∩ planes[i + 1]`; empty for a halfplane, a strip or the whole plane | CCW generators of the recession cone | the irredundant boundary planes in boundary order, starting after the angular gap |

The recession cone is the set of nonnegative combinations of the generators, each a primitive integral direction. The whole plane gives the four axis directions, a halfplane left of `u` gives `{u, perp(u), -u}`, a strip gives `{u, -u}`, a half-strip gives one direction, and a wedge gives its two edge directions. It equals `{r : cross(d_i, r) >= 0 for every input direction d_i}`, which holds for any nonempty intersection.

### halfplaneIntersection

Input: any number of halfplanes (zero gives the whole plane). Duplicates, parallel and antiparallel families are all allowed. The sort uses `polarLess` on directions and puts the innermost plane first among those with the same direction; only that plane is kept. The algorithm then branches on the cyclic angular gaps between consecutive directions.

- **No gap reaches π.** The intersection is bounded or empty. A cyclic deque sweep with open semantics pops a vertex when it is not strictly inside the new plane, then closes the cycle. A surviving cycle of at least three planes is the interior, and its closure is the `Polygon`.
- **Some gap reaches π.** The intersection is empty or unbounded. The sorted list is rotated to start after the gap, so the directions span at most π, and a stack sweep runs over it. Along that chain the depth in each new plane does not increase, so a plane only ever cuts a suffix and back pops are enough. The only antiparallel neighbours are the first and last planes; their strip is kept when it has positive width. The result is `Unbounded`.
- **The open sweep is empty.** The closed set then has empty interior. If it has positive length, a relative-interior point has active planes only on one line, from both sides, so a coincident antiparallel pair exists. One linear merge of the negated directions, which are a rotation of the sorted order, finds such a pair. Exact 1-D clipping on that line, comparing fractions by cross-multiplication of degree-4 terms, then gives `Empty`, `Point`, `Segment`, `Ray` or `Line`.
- **No pair and a gap.** A nonempty unbounded convex set with empty interior contains a ray, so a doubled line would exist. With no pair the set is `Empty`.
- **No pair and no gap.** The set is empty or a single point on at least three boundaries whose directions surround it. A closed-semantics sweep, which pops only strictly outside vertices, proposes the meeting point of its first two planes. That point is checked exactly against every plane. The check makes the answer sound; completeness rests on the closed sweep keeping every plane through the point (each pop removes a vertex strictly outside the new plane) and on the exhaustive tests.

Cost: `O(n * log(n))` for the sort, `O(n)` for everything else; the antiparallel search is linear. Memory `O(n)`.

### clipConvex

Input: a convex CCW polygon, `n >= 3`, positive area, collinear vertices allowed (`polygonConvex(p, false)` and positive area are asserted), plus any halfplanes. Integral `|coordinate| <= 10^9`. The polygon's edges are already in cyclic angular order. `halfplane_detail::edges` rotates them to the first edge of the smallest-direction run (row 15 `convexIntersection` reuses it); they are merged with the sorted halfplanes, then the same exact core runs. Result: `Empty`, `Point`, `Segment` or `Polygon`. `O(n + m * log(m))` time, `O(n + m)` memory.

### polygonKernel

Input: a simple polygon in either orientation with nonzero area (asserted). Repeated vertices are skipped. The kernel is the intersection of the interior-side halfplanes of the edges, computed with `halfplaneIntersection`. Result: `Empty`, `Point`, `Segment` or `Polygon`. `O(n * log(n))`. For a non-simple closed walk this is the algebraic intersection of the edge halfplanes, which has no visibility meaning.

### polygonKernelLinear

Same input, except the polygon must be simple. Zero-angle spikes are asserted at entry over all `n` turns, as no antiparallel consecutive edges. Other non-simple input that passes every assertion still terminates within the normal bounds: a failed first cut returns `Empty` or switches to the degenerate mode. The checked builds run two such polygons. The result has the same kind and vertex set as `polygonKernel`, and a `Polygon` keeps exactly the same vertex order. `planes` may contain bounding-box sides where `polygonKernel` lists a collinear polygon edge. `O(n)` time and memory.

This is a Lee–Preparata march, adapted:

1. `K` starts as the polygon's bounding box cut by the edge `v[n - 1] -> v[0]`. The box is legitimate because the kernel lies inside the polygon.
2. Edge `e_i` starts at the current vertex `v_i`, and `K` lies in the closed halfplane of `e_{i-1}`. Nodes `t1..t2` of `K`'s cyclic line list are the edges that `v_i` is not strictly inside. When `K` is strictly convex they form one contiguous run.
3. The new boundary passes through `v_i`. If any vertex of `K` lies strictly outside it, then so does the run's end vertex `A` (when `v_i` is convex) or its start vertex `B` (when `v_i` is reflex). This follows from the angular extremes of `K` seen from `v_i`. A cut walks only over the removed vertices and inserts the new edge.
4. After a convex cut `t2` becomes the new node; after a reflex cut `t1` does. Moving the viewpoint forward along `e_i` with `K` on its left moves both run ends counterclockwise, so the pointers only advance.
5. Open semantics keep `K` strictly convex and the run contiguous. When the open kernel empties, the closed kernel is `K`'s face on the cutting line: a point, a segment, or nothing. That face is held as at most four defining lines, and each later edge clips it in `O(1)` through the core.
6. A turning guard bounds the pointer marches. Take a kernel point `z`. The angle of the march around `z`, minus the edge direction, stays within `[π, 2π]` plus a constant, because each vertex turn is less than π. So when the kernel is nonempty, the lifted edge direction stays within a 6π window. The code counts net crossings of the `+x` direction, and `|k| >= 4` proves the kernel empty.
7. With the guard, each pointer wraps the boundary of `K` a bounded number of times, and every cut is charged to the vertices it deletes, so the march is `O(n)`. Preparata and Shamos omit the wrap test and refer to the paper. The 6π window and the crossing-count form are this implementation's own.
8. The final lines are already in angular order, so the result comes from the core in `O(k)`.

### chebyshevCenterApprox

Returns false unless the intersection is a `Polygon`. In that case it returns true with an approximate center of a largest inscribed disk and its radius `r > 0`. The method bisects on the inward offset of the polygon's irredundant planes, which are sorted once, so each feasibility test is an `O(k)` floating deque with strict semantics. Bisection starts on `[0, perimeter / 4]`, valid because `2 * r <= diameter <= perimeter / 2`. It stops when the midpoint no longer moves or after 256 steps. The center is the vertex mean of the last feasible offset polygon. `O(n * log(n) + k * 256)` time, `O(n)` memory. The result is rounded, with no certified error. The optimum is a three-variable LP, so an exact-arithmetic route belongs to row 42 (`lp3d`), which is still planned.

### Renames

| Planned name | Implemented as | Reason |
|---|---|---|
| `chebyshevCenter` | `chebyshevCenterApprox` | Naming policy: rounded `long double` result |

## Feature-to-test map

`96-Local Testing/03-Geometry/10-halfplaneintersection_tester.py` runs quick, full and stress modes in the geometry runner's optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan builds ([`_00_runner.py`](<../../96-Local Testing/03-Geometry/_00_runner.py>)), with ten assertion probes.

**Oracle.** Vertices come from Cramer intersections of every pair of input boundaries, written from `A x + B y <= C` and independent of the header's formulas. A vertex survives if it passes an exact membership test against every input plane. Survivors go through an exact rational hull. The recession cone is compared on a witness set: every candidate direction plus one strictly interior direction per angular gap. Equality of membership on that set decides equality of two planar cones. A parallel-only family gets a 1-D offset test. Each `planes` field is re-run through the oracle and must reproduce the same intersection and cone. At `|coordinate| = 10^9` the rational hull would overflow. There, Helly triples certify `Empty` (some three planes already have empty intersection), and boundary certificates check `Polygon` results: every vertex is feasible, each edge lies on its plane, edges have positive length, and the planes turn left once around. Scaling and translation metamorphic tests carry small exact cases up to `10^9`.

| Operation | Test | Oracle |
|---|---|---|
| `Halfplane::contains` (point, rational; closed, strict) | Every plane of the 3×3 pool against a 7×7 integral grid and a shifted rational grid; a rational point at the stated bound; every output vertex of every case | The oracle's independent `A x + B y <= C` depth |
| `HalfplaneKind`, `HalfplaneResult` fields | Every case: kind, vertex set and order, recession generators, planes taken from the input and reproducing the set | Brute-force classification, witness-direction cone |
| `halfplaneIntersection` | Fixed degeneracies: no planes, one plane, parallel duplicates, strips, empty strips, half-strips, wedges, rays, lines, segments, concurrent triples and quintuples. All pairs and triples from the 72 halfplanes of the 3×3 pool; all quadruples in stress mode. Random small, concurrent and medium sets. Wide metamorphic and random certificate sets. Tangent families of up to 400,000 planes | Brute force; Helly and boundary certificates at `10^9` |
| `clipConvex` | Random hulls with and without collinear vertices plus up to four planes; disjoint and empty clip sets; `int` coordinates | Brute force on edges plus planes |
| `polygonKernel`, `polygonKernelLinear` | Convex, clockwise, notched, cross-shaped, slit, point-kernel, segment-kernel and repeated-vertex polygons. Random 2-opt simple polygons and star polygons in every rotation and both orientations. Stress mode: every simple polygon on the 3×3 grid. A 200,000-vertex star (both versions must agree exactly) and a 10-turn spiral corridor with an empty kernel and turning far beyond 6π | Brute force on the edge planes; the linear version must match kind, vertices and order, and its planes must reproduce the kernel |
| `chebyshevCenterApprox` | Square (exact 2), rectangle, 3-4-5 triangle, false on unbounded input, random planes | Brute-force maximum over plane triples of the 3×3 tangent-circle systems (the vertices of the 3D LP), relative tolerance `1e-9`; the center keeps the radius to every line |

## Commands and results

2026-10-19, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1, Python 3.14.8, GNU++20.

| Command | Result |
|---|---|
| `10-halfplaneintersection_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan, 10 probes) | PASS, 9,042,753 checks per configuration (9,042,755 in the checked build, which adds two non-simple cases), 10 probes, `MEMORY peak=593MB` |
| same with `CXX=g++-14` | PASS, same counts, 10 probes, `MEMORY peak=588MB` |
| `10-halfplaneintersection_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 75,968,253 checks, `MEMORY peak=404MB` |
| `02-integration.py --sanitizers` | PASS, 167 headers, two translation units, sanitizer self-tests, `MEMORY peak=1831MB` |
| `03-consistency.py --braces` on the four headers, testers and benchmark; `03-consistency.py` | No violations; no errors |

Mutation check (2026-10-11). Each mutant was built from a copy of the tree and run through the optimized full suite. 8 mutants.

- **Caught (6):**
  - a non-strict pop in the open sweep;
  - the antiparallel-pair search skipped;
  - the 1-D clipping bound `>` changed to `>=`;
  - a wrong half-strip recession generator;
  - the turning guard at `|k| >= 2`;
  - the `t1` advance removed.
- **Not distinguished (2):**
  - The convex `t2 = z` repair. This one is equivalent: the advance re-extends from the surviving node to the new edge.
  - The reflex `t1 = z` repair. A stale pointer differs only along the new chord, which is collinear with the next viewpoint, so the candidate test agrees until the next reflex cut resets it. A differential search over 611,579 random simple polygons in every rotation found no difference. The repair is kept because it is the stated invariant.

```bash
python3 '96-Local Testing/03-Geometry/10-halfplaneintersection_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/03-Geometry/10-halfplaneintersection_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/10-halfplaneintersection_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/03-Geometry/10-halfplaneintersection_benchmark.py' --seed 20260927
```

## Benchmarks

[Benchmark driver](<../../96-Local Testing/03-Geometry/10-halfplaneintersection_benchmark.py>), raw log `10-halfplaneintersection_benchmark.json` (ignored by git). Settings: `-O2 -DNDEBUG`, one warmup, median of five calls. Timings include copies, sorting and output construction. Input generation and the output comparison (both kernels must agree on kind and vertex sequence) are excluded. Intel Core i9-11900H, GCC 16.2.1, seed 20260927, 2026-10-11.

| Case | n | `halfplaneIntersection` | `polygonKernel` | `polygonKernelLinear` |
|---|---|---|---|---|
| Tangent lines of a circle (all irredundant) | 10^5 | 78.0 ms | | |
| Tangent lines of a circle | 10^6 | 524 ms | | |
| Random lines | 10^6 | 484 ms | | |
| Star polygon, wide kernel | ~10^6 | | 343 ms | 65.2 ms |
| Star polygon, thin kernel | ~10^6 | | 359 ms | 55.9 ms |
| Spiral corridor (empty kernel) | 10^6 | | 377 ms | 24.9 ms |

The linear kernel is 5.3–6.4× faster on stars and 15× faster on spirals, where the turning guard stops the march early.

## Sources

The implementation was written independently. The legacy notebook routine was read for its feature list only; its epsilon predicates and its `±2 * 10^9` box are not adopted. Sources of the completeness sweep are in [00-sources.md](00-sources.md#p036-completeness-sweep-fetched-2026-10-10).

- [cp-algorithms: Half-plane intersection](https://cp-algorithms.com/geometry/halfplane-intersection.html): the deque structure, pop order and the bounding-box convention that this header replaces.
- [OI Wiki: 半平面交](https://oi-wiki.org/geometry/half-plane/) and tko919 `Geometry/geometry.hpp`: same-direction handling and empty results.
- Preparata & Shamos, *Computational Geometry: An Introduction*, Section 7.2.6, book pp. 299–306 ([PDF](https://www.cs.kent.edu/~dragan/CG/CG-Book.pdf), PDF pp. 312–319): the Lee–Preparata kernel march, its case analysis, and the remark that a wrap test is needed for the linear bound. The original JACM paper could not be retrieved (HTTP 403).
- Bornstein & Bruckstein, *Finding the kernel of planar shapes*, Pattern Recognition 24(11), 1991, pp. 1020–1022: confirms that the wrap test exists but does not state it.
- `OLD/Team Notebook/src/geometry/halfplane.cpp`: legacy, superseded.

## Limits and handoffs

- Completeness of the single-point recovery path rests on the closed-sweep argument plus exhaustive tests; soundness is certified by the exact check.
- `chebyshevCenterApprox` uses bisection. A fixed-dimension LP (row 42 `lp3d`) would give expected `O(n)`, still rounded because the constraint norms are irrational.
- Polygons with holes have no kernel API. The intersection of all ring edge halfplanes is available directly through `halfplaneIntersection`.
- Convex polygon queries such as `contains`, `tangents` and `convexIntersection` belong to row 15. The 3D halfspace intersection belongs to row 38.

## History

- 2026-10-11: first implementation (P036). Independent review: one finding (out-of-bounds read on non-simple input in `polygonKernelLinear`, spike assertion incomplete) fixed with new probes and checked-build cases.
- 2026-10-12: P007 prune migrated `onSegment` call sites in the tester; full suite passed ([02-line_segment.md](02-line_segment.md)).
- 2026-10-18: P037 factored the edge rotation of `clipConvex` into `halfplane_detail::edges` for `convexIntersection`; full suite passed on g++ and g++-14, stress passed.
- 2026-10-19: P037 re-audit rerun (row 15 changed), header unchanged; full suite passed on g++ and g++-14, stress passed.
