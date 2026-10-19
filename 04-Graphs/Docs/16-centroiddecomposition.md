# 16-centroiddecomposition.hpp — evidence

Owned by package P056 / GR06; omissions are in [00-notes.md](00-notes.md#p056-omissions). Contest profile. The header has an iterative centroid decomposition of a forest. On top of it sit nearest-marked and distance-sum structures, contour queries for groups (Fenwick, subtraction) and for monoids or commutative actions (binarized decomposition, segment trees), the all-distance histogram through Mathematics convolution, and weighted path counting. It replaces the unchanged monolith excerpt `OLD/algorithms.cpp:5005-5044`.

## Contracts

### CentroidDecomposition

`CentroidDecomposition<W>(g, weighted = false)` takes an undirected `Graph` or `CsrGraph` forest with `n <= 2^24` vertices (asserted).

- **Forest check.** Undirectedness and the forest condition `m = n - trees` are asserted after the first BFS, so directed input, loops, multiedges and cycles are rejected. Under `NDEBUG`, any of these returns the edgeless decomposition: every vertex is its own level-0 centroid. This keeps every later index in range.
- **Distances.** Without `weighted`, every edge counts 1. With it, the build adds `W(w)`, and every path sum must fit in `W`. Operations that add two distances (`NearestMarked`) need every path sum at most `max(W) / 2`.
- **Lifetime.** No reference to `g` is kept.

Fields:

- `parent[c]`: the centroid parent, −1 for the one root per tree.
- `level[c]`: depth in the centroid forest, always below `L = bit_width(n)`.
- `size[c] = |comp(c)|`.
- `tin`, `order`: creation order, which is a preorder of the centroid forest.
  - `comp(c)` is `order[tin[c], tin[c] + size[c])`.
  - Its child components are consecutive sub-blocks: `for (p = tin[c] + 1; p < tin[c] + size[c]; p += size[order[p]])` visits the children `order[p]`.
- `dist`, read through `distance(v, k)`: the distance from `v` to its level-`k` centroid ancestor. `0 <= k <= level[v]` is asserted.

Build:

1. One BFS per tree computes BFS parents `up` and subtree sizes.
2. A stack holds component roots `r`; `up[r]` is the centroid that split `r` off, or −1.
3. From `r`, the build walks into the BFS child with `2 * sub > size(component)` until none exists. The vertex reached is the centroid: every remaining side has at most half the vertices.
4. A BFS from the centroid writes the distances at its level. The same BFS recomputes `up` and `sub` for the components it leaves behind, so each component needs one BFS.

Level `k` components have at most `n / 2^k` vertices, so levels stay below `L`. Every vertex is visited once per level, which gives O(n log n) time and n·L stored distances. The pop order is a DFS of the centroid forest, so creation order is a preorder and every `comp(c)` is a contiguous block.

### NearestMarked

`NearestMarked<W>(g, weighted = false)` requires nonnegative weights when weighted (asserted).

- `mark(v)` is idempotent. It pushes `(distance(v, k), v)` onto the heap of every centroid ancestor.
- `unmark(v)` clears a flag in O(1); stale entries are popped lazily.
- `nearest(v)` returns `min dist(v, u)` over marked `u` in `v`'s tree, or −1 when there is none. It pops the unmarked tops of each ancestor heap and minimises `top + distance(v, k)`.

An entry for a marked vertex always carries its true distance, so duplicates from re-marking are harmless. Heap memory grows by at most `L` entries per effective `mark`. The query bound O(log² n) is amortized over the pops. Path sums must be at most `max(W) / 2`.

### DistanceSum

`DistanceSum<T, W>(g, weighted = false)` takes a commutative ring `T` constructible from `W`; weights may have any sign.

- `add(v, x)` adds weight `x` at `v`.
- `sum(v)` returns `Σ x_u · dist(v, u)` over the added `(u, x_u)` with `u` in `v`'s tree.

Per centroid `c` it stores:

- `cnt[c]`: the weight in `comp(c)`;
- `own[c]`: `Σ x · dist(·, c)`;
- `rel[c]`: `Σ x · dist(·, parent[c])`.

A query walks up from `v` and adds `own[c] + cnt[c] · d` at each ancestor. It subtracts the child `prev`'s share `rel[prev] + cnt[prev] · d`, which removes the pairs whose path does not pass through `c`. Overflow of `T` is the caller's concern.

### ContourSum and ContourAdd

Both take edge-count distances (weights are ignored) and a commutative group `T` (`+`, `-`, zero `T()`).

- **Contour.** A contour `[l, r)` requires `0 <= l <= r` (asserted). `r` may exceed `n` and is clipped. Distance 0, the vertex itself, is included when `l = 0`.
- **Banks.** Each centroid `c` owns two Fenwick banks of `size[c] + 1` slots:
  - `F_c`, indexed by the distance to `c`, over `comp(c)`;
  - `G_c`, indexed by the distance to `parent[c]`, over the same vertices.
- **Writes.** `pointAdd` and `rangeContourAdd` write `+x` into `F_c` and `G_prev` for every ancestor `c`, where `prev` is the previous ancestor.
- **Reads.** `rangeContourSum` and `pointGet` return `Σ F_c[l - d, r - d) - G_prev[l - d, r - d)` with `d = dist(v, c)`.

At the lowest common centroid ancestor `L` of `v` and `w`, the path passes through `L`, so `dist = dist(v, L) + dist(w, L)`. Above `L` both vertices lie in the same child component, and the `F` and `G` terms cancel.

- `ContourSum(g, a)` builds both banks from `a` in linear time per bank, O(n log n) overall.
- `ContourAdd(g, a)` keeps `a` and returns `a[v]` plus every covering `rangeContourAdd`.

The two are separate structures: a contour sum over contour-added values counts pairs under two distance constraints, which this decomposition cannot answer. Library Checker `vertex_add_range_contour_sum_on_tree` maps to `ContourSum`, and `vertex_get_range_contour_add_on_tree` to `ContourAdd`.

### ContourFold and ContourApply

Both use edge-count distances. `ContourFold<M>(g, a, mon = {})` takes a commutative monoid `M` (`S`, `op`, `e`). `ContourApply<A>(g, mon = {})` takes commutative actions `A` (`F`, `composition`, `id`; `MonoidAction<M>` adapts a commutative monoid). Both keep the monoid as `m` under the [stored-object contract](<../../02-Data Structures/Docs/00-monoids.md#monoid-contract>). Neither needs inverses. Commutativity is required because the folds visit vertices in no fixed order.

**Binarization.** `centroid_detail::binarize` roots each tree by BFS. It chains every child list through virtual vertices (ids `>= n`) joined by 0-weight edges; real edges get weight 1.

- Every vertex then has degree at most 3.
- There are fewer than `n` virtual vertices.
- 0/1-weighted distances equal the original edge counts.
- The forest assertion is repeated here.

**Decomposition.** The weighted decomposition of this forest (at most `2n` vertices, hence `n <= 2^23`) gives every centroid at most 3 child components. Each child component lists its real vertices sorted by distance to the parent centroid, using a counting sort. That list backs a bottom-up segment tree for folds or a tag tree for actions; distance ranges map to index ranges by `lower_bound`.

**Queries.**

- `rangeContourFold(v, l, r)` walks the ancestors. At each one it takes the centroid itself, when it is real and in range, and the matching index range of every child component except the one holding `v`.
- `set(v, x)` rewrites the leaf of `v` in every component that contains it.
- `rangeContourApply` mirrors the fold with tags.
- `pointGet(v)` composes the self tag with the tags on each leaf-to-root path.

Each pair is counted once, at its lowest common centroid ancestor. Contour bounds are as above, and virtual vertices hold `e()` / `id()`. Memory is O(n log n): the decomposition's distances and one leaf per real vertex per level.

### treeDistanceFrequency

`treeDistanceFrequency(g)` takes a forest with `n < 2^24` (asserted) and edge-count distances. It returns `n` counts (empty for `n = 0`): `res[d]` is the number of unordered pairs `{u, v}`, `u ≠ v`, in the same tree at distance `d`, and `res[0] = 0`.

For each centroid `c` it builds two distance histograms:

- `h_c`: distances from `comp(c)` to `c`;
- `h'_c`: distances from `comp(c)` to `parent[c]`.

Then `Σ_c (h_c * h_c - h'_c * h'_c)` is twice the pair count at every `d ≥ 1`. Indices `≥ n` cancel and are dropped.

A square uses a naive symmetric product when `top < 800` (the maximum distance; crossover measured below) and `convolutionLong` (exact multi-prime NTT, row 13) otherwise. Coefficients are at most `n² < 2^63`, so they are exact, and row 13 accepts any length. Time is O(n log² n); memory is O(n log n) for the decomposition.

### countPathsWithin

`countPathsWithin(g, k)` takes a forest whose edge weights may have any sign, with every path sum in `[-2^62, 2^62)`. Sums of two distances to a centroid then lie in `[-2^63, 2^63 - 2]` and never overflow. `k` is any `lng`.

It counts unordered pairs `u ≠ v` in the same tree with `dist(u, v) <= k`. For every centroid `c` it applies the same subtraction identity to two-pointer pair counts over the sorted distances of `comp(c)`: to `c`, and to `parent[c]`. Sorting makes the bound O(n log² n).

## Legacy mapping

| Legacy (`OLD/algorithms.cpp:5005-5044`) | Replacement |
|---|---|
| `CentroidDecomposition(N, adj)` (recursive, vertex 0's tree only) | `CentroidDecomposition<int>(Graph::fromAdjacency(adj))`: iterative, every tree |
| `par` | `parent` |
| `sz` (scratch subtree sizes left by the last DFS) | `size` (component size of each centroid) |
| `rmv` | internal (`level[v] != -1` during the build) |

## Aliases

- Centroid tree `root`, `children` and `tree` (ei1333, Nyaan) are `parent` and the preorder blocks.
- Decomposition order (hitonanode) is `order`.
- Per-centroid callbacks (maspypy MODE 0, hitonanode `run`) are the blocks `order[tin[c], tin[c] + size[c])` with child sub-blocks.
- Paths of length exactly `k` (cp-algorithms, CSES Fixed-Length Paths I/II) are `treeDistanceFrequency` entries or prefix sums of them.
- Xenia and Tree (CF 342E) and QTREE5 map to `NearestMarked`; POJ 1741 and Luogu P4178 map to `countPathsWithin`.
- suisen `point_set_range_contour_sum` and `point_get_range_contour_operate` are `ContourFold` and `ContourApply`.

## Feature-to-test map

`96-Local Testing/04-Graphs/16-centroiddecomposition_tester.py` uses the shared graph runner with `-Werror` warnings, in optimized, checked (`_GLIBCXX_DEBUG`, with probes) and ASan/UBSan (full and stress) builds.

The independent oracle is a DFS distance matrix from every vertex, weighted and unweighted, together with tree ids, validated through the closed forms on paths and stars. It is used with random forests of six shapes:

- random recursive;
- path;
- star;
- caterpillar;
- binary;
- broom.

Each forest has 1–4 trees, random labels and orientations, and unit, signed ±1000, ±2^40 or ±(2^62 − 1)/(n − 1) weights; the last reach the `countPathsWithin` domain bound. Cases also include `n = 0..5`.

| Operation | Test | Oracle |
|---|---|---|
| `CentroidDecomposition` build, `parent`, `level`, `size`, `tin`/`order`, `distance` | Every random forest, with `int` unweighted, `lng` weighted and CSR input. Large path, star, random, caterpillar and binary trees (200,000 vertices in full, 1,000,000 in optimized stress) | Preorder permutation and nested blocks; each block equals the component of `c` among levels `>= level[c]`; each part left after deleting `c` has at most `size[c] / 2` vertices; levels below `L`; one root per tree; distances equal the matrix |
| `NearestMarked`: `mark`, `unmark`, `nearest` | Random mark/unmark/query mixes, weighted and CSR unweighted; large path against a `std::set` of marks | Minimum over marked vertices in the tree; −1 when there are none |
| `DistanceSum`: `add`, `sum` | `DistanceSum<lll, lng>` with signed 2^40 weights; `DistanceSum<lng>` unweighted; large weighted star | `Σ x_u · d(q, u)` in `lll` |
| `ContourSum`: `pointAdd`, `rangeContourSum` | Random updates and contours, including `l = r`, `r = INT_MAX` and `l > n`; large path closed form; random total | Direct sum over the matrix |
| `ContourAdd`: `rangeContourAdd`, `pointGet` | Random contour adds on CSR input; large random totals | Per-vertex accumulation |
| `ContourFold`: `set`, `rangeContourFold` | Sum monoid and the `(max, count)` monoid (neither invertible nor idempotent, so double counting shows); a capturing-lambda sum modulo 7 or 10^9+7 through `Monoid`; large star | Direct fold |
| `ContourApply`: `rangeContourApply`, `pointGet` | `(add, max)` commutative actions; a capturing-lambda add modulo 7 or 10^9+7 through `MonoidAction{Monoid}`; large caterpillar | Per-vertex composition |
| `treeDistanceFrequency` | Every random forest (Graph and CSR); n = 1700 (quick), 1700/3000 (full) and 1700/3000/4000 (stress, optimized and ASan) in six shapes, whose paths take the NTT branch; paths n = 1599..1602, where the root square has top 799 and 800 (threshold neighbours); large path and star closed forms; large random total `C(n, 2)` | Pair counts from the matrix or from BFS; `n - d` on paths |
| `countPathsWithin` | Every random forest with `k` = a sampled distance ± 1, `LLONG_MAX` and `LLONG_MIN`; saved regression with two centroid distances of 2^62 − 1 (the review's overflow case, moved inside the domain); large path | Pair counts from the matrix; hand count |
| Preconditions | 14 probes: directed input, cycle, distance level, nearest range, negative weight, sum range, contour size, `l > r`, negative `l`, fold on a cycle, fold range, apply range, frequency on a cycle, directed path count | Expected assertion failure |

Planted mutants were each compiled against the quick suite on 2026-10-16. All 12 were killed:

- no centroid walk;
- contour sum without the same-child subtraction;
- `ContourAdd` upper bound off by one;
- fold over the query's own child component;
- apply self tag inclusive at `r`;
- nearest ignoring `unmark`;
- distance sum with the wrong parent distance;
- naive square without the symmetry factor;
- NTT square replaced;
- strict path count;
- `upper_bound` range;
- the Fenwick linear build stopping early.

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `16-centroiddecomposition_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 31,217,625 checks per configuration, 14 assertion probes, MEMORY peak 964 MB |
| `CXX=g++-14 16-centroiddecomposition_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 31,217,625 checks per configuration, MEMORY peak 952 MB |
| `16-centroiddecomposition_tester.py --mode stress --seed 7` | PASS, 39,037,872 checks per configuration, MEMORY peak 1560 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/16-centroiddecomposition_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/04-Graphs/16-centroiddecomposition_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/04-Graphs/16-centroiddecomposition_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

The checked build (`-O0 -D_GLIBCXX_DEBUG`) runs full and stress at frequency n ≤ 3000 and 50,000-vertex shapes; the ASan/UBSan build runs stress at 200,000-vertex shapes. Both caps keep each run inside the runner's 180 s timeout, and the optimized build runs the full sizes.

On 2026-10-19 the P054 follow-up reran this suite in full mode as a dependent of row 13's new `convolutionLong`; [13-convolution.md](../../05-Mathematics/Docs/13-convolution.md#commands-and-results) records the PASS.

## Benchmarks

`16-centroiddecomposition_benchmark.py` was run on 2026-10-16: GCC 16.2.1, `-O2 -DNDEBUG`, 11th Gen Intel Core i9-11900H, seed 20260927, one warmup run, then medians of 5 runs. Every result is checked inside the run: naive squares equal `convolutionLong`, frequency totals equal `C(n, 2)`, and `ContourSum` equals `ContourFold` with the sum monoid. Peak memory was 352 MB.

The only dispatch threshold is the histogram square in `treeDistanceFrequency`. Squares use uniform counts below 1000 at length `t`:

| t | naive symmetric (µs) | convolutionLong (µs) |
|---|---|---|
| 256 | 21.0 | 47.1 |
| 512 | 80.6 | 104.4 |
| 640 | 125.6 | 223.1 |
| 768 | 183.1 | 227.8 |
| 800 | 214.7 | 234.5 |
| 896 | 273.5 | 232.5 |
| 1024 | 337.5 | 241.1 |
| 1536 | 755.1 | 598.0 |

The naive symmetric square wins up to t ≈ 860; `convolutionLong` steps at powers of two. The header squares naively when `top < 800` (t ≤ 800).

End-to-end timings on n = 200,000 trees use 200,000 contour queries `[l, l + 4)` with `l < 8`:

| Shape (n = 200,000) | build (ms) | treeDistanceFrequency (ms) | 200,000 rangeContourSum (ms) | 200,000 rangeContourFold (ms) |
|---|---|---|---|---|
| random | 104.5 | 142.4 | 60.2 | 460.7 |
| path | 49.3 | 1093.4 | 66.7 | 163.2 |
| star | 10.8 | 21.1 | 17.6 | 282.3 |
| caterpillar | 44.0 | 508.2 | 67.5 | 190.1 |
| binary | 32.1 | 59.1 | 71.0 | 189.4 |

Paths and caterpillars take the NTT branch at the upper levels; random trees stay naive almost everywhere. `ContourFold` costs about 3–8 times `ContourSum`. It walks the binarized decomposition, which has up to `2n` vertices and up to two segment-tree folds per level.

## Sources

Fetched 2026-10-16; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-16-p056).

| Source | Inspected claim and application |
|---|---|
| [Library Checker tree tasks](https://github.com/yosupo06/library-checker-problems/tree/master/tree) | Contour problems' `[l, r)` with distance 0; frequency table as unordered pairs at distances 1..n-1 |
| [maspypy centroid_decomposition](https://maspypy.github.io/library/graph/centroid_decomposition.hpp), contour_query_range, distance_sum | Binarized subtraction-free contours, distance sums, per-centroid hook |
| [suisen point_get_range_contour_operate](https://suisen-cp.github.io/cp-library-cpp/library/tree/point_get_range_contour_operate.hpp), point_set_range_contour_sum | Monoid and commutative-action contour contracts |
| [hitonanode](https://hitonanode.github.io/cplib-cpp/tree/centroid_decomposition.hpp), [Nyaan](https://nyaannyaan.github.io/library/tree/centroid-decomposition.hpp), [ei1333](https://ei1333.github.io/library/graph/tree/centroid-decomposition.hpp) | Centroid tree views and callback forms (aliases) |
| [OI Wiki 点分治](https://oi-wiki.org/graph/tree-divide/), [cp-algorithms](https://cp-algorithms.com/graph/centroid_decomposition.html) | Weighted pair counting with dist <= k; centroid halving argument; length-k path counting |

The binarization, the two-bank subtraction identity and the per-centroid squaring are standard techniques, written from their descriptions; no implementation was copied.

## Limits and handoffs

- The contour structures, `treeDistanceFrequency` and `ContourFold`/`ContourApply` use edge counts. Weighted contours and contours on edges are not offered by any catalog.
- `NearestMarked` heap memory grows with the number of effective `mark` calls.
- Static (update-free) contour sums are `ContourSum` without updates.
- Row 14 owns centroid finding, diameter and reroot DP; row 15 owns path queries.

## History

- 2026-10-16: verified (P056); legacy excerpt replaced; completeness sweep added unmark, `DistanceSum`, `ContourFold`, `ContourApply` and `countPathsWithin`; independent review: 3 findings fixed (`countPathsWithin` domain narrowed to `[-2^62, 2^62)`, directed input routed to the `NDEBUG` fallback, `treeDistanceFrequency` asserts `n < 2^24`); 12 planted mutants killed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 964/952/1560 MB.
- 2026-10-19: P054 follow-up, `treeDistanceFrequency` contract updated for row 13's any-length `convolutionLong`; full suite passed as a row 13 dependent ([13-convolution.md](../../05-Mathematics/Docs/13-convolution.md#commands-and-results)).
