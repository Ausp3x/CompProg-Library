# 18-persistentsegmenttree.hpp — evidence

Package P030 (batch DS23). Replaces the unchanged extractions `PerSegTree` (`OLD/algorithms.cpp:2445–2725`) and `KSPerSegTree` (`OLD/algorithms.cpp:3461–3649`; the notebook copies `OLD/Team Notebook/src/ds/persegtree.cpp` and `kspersegtree.cpp` hold the same code). Indices are zero-based and ranges `[l, r)` half-open, value ranges `[lo, hi)` too; `k`-th counts from 0. Preconditions are `assert`s checked once per operation. Every struct owns its node pool; copies are independent snapshots.

## Contracts

### PersistentSegmentTree<A, COMMUTATIVE = false>

`A` is a monoid (`S`, `op`, `e`) or an acted monoid (adds `F`, `mapping`, `composition` new after old, `id`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract), kept as `m` and passed last; the acted laws include `mapping(f, e()) == e()`. A plain monoid gets an empty tag type (`persistentsegmenttree_detail::NoAction`), so its nodes carry no tag and every tag step compiles away, and `apply` is `static_assert`ed away. Positions are `lng` in `[0, n)` for any `n >= 0` up to `INT64_MAX` (decided 2026-10-27, as the legacy tree took `lng` sizes); every node splits `[a, b)` at `std::midpoint(a, b)`, which cannot overflow, so the depth is `ceil(log2(n)) <= 63`. Node ids and version ids are `int`.

- Versions are dense ids from 0 (`versionCount()`); version 0 is the build, and every mutator returns the new id `versionCount() - 1`, leaving every earlier version unchanged (full persistence: any version can be branched from). Node 0 is the null tree, holding `e()` at every position.
- Builds: `PersistentSegmentTree(n)` is all `e()` with a null root, O(1). `PersistentSegmentTree(n, x)` holds `x` everywhere: subtrees of equal length are equal, so one node per distinct length (at most two per level) gives O(log(n)) nodes, also for `n = INT64_MAX`; a linear memo scan makes the time O(log(n)^2). `PersistentSegmentTree(v)` builds `2 * |v| - 1` nodes in O(n).
- `get(v, i)`, `allProd(v)`, `prod(v, l, r)` (`e()` for an empty range), `toVector(v)` (O(n)), `maxRight(v, l, pred)` (largest `r` in `[l, n]` with `pred(prod(v, l, r))`), `minLeft(v, r, pred)` (smallest `l` in `[0, r]`): predicates are deterministic, true on `e()` (asserted) and monotone. Values are returned by value: a later mutation may reallocate the pool.
- `set(v, i, x)`; `apply(v, l, r, f)` (needs an acted monoid); `copyRange(v, w, l, r)`: version `v` outside `[l, r)` and version `w` inside (`v == w` allowed); `split(v, i)`: two new versions, `{positions < i kept, the rest e()}` then `{positions >= i kept, the rest e()}`; `merge(v, w)`: position `i` holds `op(a_v[i], a_w[i])` (`v == w` allowed). An empty range gives a new id sharing the old root.
- `COMMUTATIVE = true` makes `apply` keep tags on the partly covered nodes instead of pushing them down (tag permanence). It needs `mapping(composition(f, g), x) == mapping(composition(g, f), x)` for all actions; the other mutators push as usual, so they stay correct in this mode.
- `reserve(nodes)`, `nodeCount()` (nodes made, null excluded). Pools hold fewer than `2^31` nodes; nothing is recycled.

Representation and correctness. Nodes are immutable after creation, which gives persistence. A node `k` over `[a, b)` stores `x_k`, the product of its positions with every tag at or below `k` applied, and a tag `f_k` pending for its children: `x_k = mapping(f_k, op(x_l, x_r))` for internal nodes, with `x_0 = e()`. A position's value is the leaf value under the composition of its strict ancestors' tags, the root's outermost. `tagged(k, g)` makes the copy `{mapping(g, x_k), composition(g, f_k)}`, which satisfies the invariant by the composition law, and returns `k` itself when `g == id()` (only when `F` has `operator==`) or when `k` is null (by `mapping(g, e()) == e()`). `set`, `apply`, `copyRange` and `merge` rebuild the visited path with tag `id()`, passing the composed tags of the old path down as `g`. Each subtree they leave untouched is reattached as `tagged(subtree, g)`, so it keeps its values, and a covered subtree of `apply` becomes `tagged(k, composition(f, g))`, which applies `f` after everything older. In `COMMUTATIVE` mode `apply` keeps `f_k` on its new path nodes and passes `id()`, so `f` lands below older tags, which is correct exactly when actions commute. `node(f, 0, 0)` returns the null node, so a range of `e()` positions is null, a split half without values is null, and `merge` stops at the first null side; it does work only on nodes present in both versions. Queries never write: `fold` applies a partial node's own tag to the product of its children's partial results (the homomorphism law), and `get` and the descents compose the tags of the path.

Cost. Per call, `set` and each half of `split` make at most 2 nodes per level (path node plus one retagged sibling). `apply` and `copyRange` make at most 4 per level (2 partial path nodes, each with at most one covered and one disjoint retagged child), and `apply` in `COMMUTATIVE` mode at most 2 partial and 2 covered nodes per level. Without `operator==` on `F` the retagged siblings are copied even under `id()`. `merge` makes one node per tree position where both versions have a node, plus retagged frontier copies; it counts positions, not distinct nodes, so merging two fill-built versions visits every position although each holds O(log(n)) nodes.

### PersistentDualSegmentTree<A, COMMUTATIVE = false>

Uses `F`, `composition` and `id` of the stored `m` (`MonoidAction<M>` turns a monoid into such an action). `apply(v, l, r, f)` composes `f` after every action on `[l, r)` of version `v` and returns the new version; `get(v, i)` is the composition along the root-to-leaf path, ancestors newer (identity initially). Without `COMMUTATIVE`, `apply` pushes the tags of the path into retagged copies of the siblings so that ancestors stay newer than descendants; with it, tags stay in place and the actions must commute. Positions, `reserve`, `nodeCount`, `versionCount` and the `NDEBUG` bounds as above. Node 0 is the all-identity tree.

### PersistentValueTree<T, W = lng>

`T` needs only `operator<` (a strict weak order); `W` is an additive group with `W()` as zero, and every partial sum must be representable. Construction `PersistentValueTree(a)` or `PersistentValueTree(a, w)` with `w.size() == a.size()`, `0 <= n < 2^31`: positions are stably sorted by value, leaf `p` is the `p`-th position in `(value, index)` order, and version `i` (`roots[i]`) holds the leaves of `a[0, i)`, each with count 1 and its weight. A query on `a[l, r)` walks versions `r` and `l` together and subtracts. Build O(n log(n)); memory at most `n * (ceil(log2(n)) + 1) + 1` nodes of three `int`s, plus one `W` per node when weighted.

- `countEqual(l, r, x)`, `countRange(l, r, lo, hi)` (values in `[lo, hi)`, 0 when `!(lo < hi)`): rank bounds by binary search over the sorted values, then one count descent per bound. O(log(n)).
- `rangeKth(l, r, k)` (asserts `0 <= k < r - l`): the `k`-th smallest value. O(log(n)).
- `kthSum(l, r, k)` (asserts `0 <= k <= r - l` and a weighted build or `n == 0`): sum of the weights of the `k` smallest values, ties by increasing position, the same order as row 17. With `w = a` it sums values. O(log(n)).
- `maxLeq(l, r, x, res)`, `minGeq(l, r, x, res)`: the largest value `<= x` or smallest `>= x`; they return `false` and leave `res` untouched when none exists. No `x ± 1` arithmetic, so the extremes of `T` are ordinary inputs. O(log(n)).

### PersistentDistinctCount<T>

`T` needs `operator<` whose equivalence is equality. `previous(a)` gives each position the previous position with an equal value, or -1, by a stable sort. The struct keeps a `PersistentValueTree<int, int>` over that array. `countDistinct(l, r)` counts the positions `j` in `[l, r)` with `previous(a)[j] < l`. Each distinct value of `a[l, r)` has exactly one such position, its first occurrence, so the count is the number of distinct values. Online, O(log(n)) per query, O(n log(n)) build and memory.

### Violated preconditions

Under `NDEBUG` a bad version id reads `roots` out of range (undefined, as with `vector::operator[]`). A negative size is clamped to 0, so every node range is nonempty and halves per level, and an out-of-range index or range only steers the recursion: `point` and `meld` stop at length `<= 1`, `act`, `copy`, `fold` and the descents stop at covered or disjoint nodes, which every length-1 node is. No call runs past depth `ceil(log2(n)) + 1`. The value trees build from vector sizes and walk at most `ceil(log2(n))` levels.

### Legacy mapping

| Legacy | Replacement |
|---|---|
| `PerSegTree<Mon>` with `idS/idF/defR/init/ope/map/cmp` | `PersistentSegmentTree<A>` with `e/id/op/mapping/composition`; a uniform `defR` becomes the fill build `(n, x)`, a position-dependent one a vector build (left out on huge universes, see Limits) |
| `PerSegTree(N)`, `PerSegTree(span)`, fixed `reserve(10'000'000)` | `PersistentSegmentTree(n, x)` or `(n)`, `PersistentSegmentTree(vector)`, `reserve(nodes)` |
| `update(ver, l, r, f)` (closed range; out-of-range ranges silently copied the version) | `apply(v, l, r + 1, f)` (asserted) |
| `modify(ver1, ver2, l, r)` | `copyRange(ver1, ver2, l, r + 1)` |
| `query(ver, l, r)`, `queryAll(ver)` | `prod(v, l, r + 1)`, `allProd(v)` |
| `maxR(ver, l, g)`, `minL(ver, r, g)` | `maxRight(v, l, g) - 1`, `minLeft(v, r + 1, g)` |
| `KSPerSegTree(a)` | `PersistentValueTree<lng>(a, a)` (weights equal to values for `kthSum`) |
| `cntEql(l, r, x)`, `cntRan(l, r, x1, x2)` | `countEqual(l, r + 1, x)`, `countRange(l, r + 1, x1, x2 + 1)` |
| `kthMin(l, r, k)` (1-based), `kthSum(l, r, k)` | `rangeKth(l, r + 1, k - 1)`, `kthSum(l, r + 1, k)` |
| `maxLeq`, `minGeq` returning `-INF64` / `INF64` | `maxLeq(l, r + 1, x, res)`, `minGeq(l, r + 1, x, res)` returning `false` |

Catalog forms of the same operations (aliases): maspypy `multiply`, ei1333 `apply(t, k, x)` and Nyaan `add` are `set(v, i, op(get(v, i), x))`; suisen `apply_all` is `apply(v, 0, n, f)`; maspypy `copy_interval` and tko919 `copy` are `copyRange`; suisen and tko919 `dump` are `toVector`; Nyaan `update(t, k, x)` is `set`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/18-persistentsegmenttree_tester.cpp`, entry `18-persistentsegmenttree_tester.py` (driver `_00_runner.py`; optimized, checked and ASan/UBSan builds; 49 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `PersistentSegmentTree` builds (empty, fill, vector), `reserve`, `nodeCount`, `versionCount` | `history` initial checks, `nullNodes` | `VecOracle` (a full array copy per version); the empty build makes 0 nodes, the fill build at most `2 * bit_width(n) + 2`, the vector build `2n - 1`; `reserve` leaves `nodeCount` unchanged |
| `get`, `allProd`, `toVector`, `prod`, `maxRight`, `minLeft` | `history` verification (every `(l, r)` for `n <= 12`) | `VecOracle` linear folds and linear predicate scans; `PieceOracle` (a map of pieces with binary search, itself checked against `VecOracle` on small `n`) for `n = 2^31 - 1`, `2^30 + 1`, `2^62 + 3` and `INT64_MAX`, with about a quarter of the positions within 3 of `n` and search thresholds reaching the far end; empty builds of every monoid and fill builds of `PairMonoid` and `RangeAddRangeMin` at every huge `n` (sum fills only up to `INT_MAX`, where the sums stay representable) |
| `set`, `apply`, `copyRange` (`v == w` included), `split`, `merge` (`v == w` included) | `history` steps from random versions | Same oracles; each step checks the returned id, `versionCount` and a bound on new nodes (`2h`, `4h`, `8h` for depth `h`). Every new version is checked in full, one random older version per step, and every version at the end |
| Plain monoids, noncommutative actions, `F` with and without `operator==`, runtime monoid state, `COMMUTATIVE` | `history` over `Sum<lng>` (both modes), `PairMonoid<AffineComposite<mint>, Sum<lng>>`, `RangeAffineRangeSum<mint>` (no `operator==`), a stateful `HashAffine` (runtime modulus and base, noncommutative `op` and `composition`, identity actions included, compared as literal sequences), `RangeAddRangeSum<lng>` and `RangeAddRangeMin<lng>` in both modes | As above |
| Null node and `mapping(f, e()) == e()` | `nullNodes` (n = 1, 5, 64, `INT_MAX`, `INT64_MAX`) | `apply` over never-set positions keeps `nodeCount() == 0` and `get`/`prod`/`allProd` at `e()` |
| `PersistentDualSegmentTree` `get`, `apply`, `reserve`, `nodeCount`, `versionCount` | `dualHistory`: affine `mint` (no `operator==`), `HashAffine`, addition in both modes, `MonoidAction<Monoid<lambda>>` | Composed action per position (dense, or a map oracle up to `n = INT64_MAX`) |
| `PersistentValueTree` `countEqual`, `countRange`, `rangeKth`, `kthSum`, `maxLeq`, `minGeq` | `valueTree` (every `(l, r)` and `k` for `n <= 24`): `lng` with its numeric extremes, `string`, `pair<int, int>` with `W = int`, a key-only order exposing tie order, unweighted builds, `n = 0` | Linear scans and a sort with an explicit `(value, index)` tie-break; `res` untouched on `false` |
| `PersistentDistinctCount` `countDistinct` | `distinctCount` (all-equal, all-distinct, random, `string`) | `std::set` |
| Preconditions | `invalid` | 49 probes: version ids (also in dual `apply` and `minLeft`), indices and reversed ranges (also at `n = INT64_MAX`), predicates false on `e()`, `rangeKth` with `k` out of range, `kthSum` unweighted or with `k > r - l`, negative sizes, weight size mismatch, dual `get` out of range |

Quick: sizes 0–13 with 30 steps, n = 200, the huge universes with 25 steps. Full: 22 sizes up to 100 with 100 steps, n = 1000, the four huge universes with 120 steps, 21 dual sizes with 200 steps, value tree n = 4000, distinct count n = 5000. Stress: the full sizes plus 6 random sizes up to 200 with 260 steps, n = 1500, 600 huge-universe steps, value tree n = 20000, distinct count n = 10000. Mutation check (2026-10-27, scratch copies under `/tmp`): 16 of 16 injected faults detected in quick mode (an `int` midpoint in `fold` and in the dual `get`, caught at `n = INT64_MAX`; composition order in `act`, skipped retagging of disjoint children, `merge` operand order, `split` boundary, `below` boundary, missing left sums in `kthSum`, dual `get` and `act` composition order, `COMMUTATIVE` dropping in-place tags, `copy` with the wrong pending action, `minLeft` result, `tagged` copying null nodes, `countDistinct` bound, `minGeq` absence test).

## Commands and results

Latest run 2026-10-27, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `18-persistentsegmenttree_tester.py --mode quick` | PASS, 2 configurations, 700,017 checks, 49 assertion probes, MEMORY peak 821 MB |
| `18-persistentsegmenttree_tester.py --mode full --seed 20261027` (GCC 16.2.1) | PASS, 3 configurations, 10,804,114 checks per configuration, 49 assertion probes, MEMORY peak 1559 MB |
| `CXX=g++-14 18-persistentsegmenttree_tester.py --mode full --seed 20261027` (GCC 14.4.1, floor) | PASS, 10,804,114 checks per configuration, MEMORY peak 1586 MB |
| `18-persistentsegmenttree_tester.py --mode stress --seed 7` | PASS, 32,822,285 checks per configuration, MEMORY peak 1556 MB |
| `02-integration.py` | PASS: 177 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 1285 MB |
| `03-consistency.py` | No errors |

The memory peaks are the compilers'; the test binary alone peaks at about 11 MB.

```bash
python3 '96-Local Testing/02-Data Structures/18-persistentsegmenttree_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/18-persistentsegmenttree_tester.py' --mode full --seed 20261027
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/18-persistentsegmenttree_tester.py' --mode full --seed 20261027
python3 '96-Local Testing/02-Data Structures/18-persistentsegmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-Data Structures/18-persistentsegmenttree_benchmark.py'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

Driver `96-Local Testing/02-Data Structures/18-persistentsegmenttree_benchmark.py` (`.cpp` beside it; raw log `18-persistentsegmenttree_benchmark.json`, not committed). Run 2026-10-27 with `lng` positions on an Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed 20261027, one warmup then the median of 5 runs, no `reserve`. Every variant of a workload returns the same checksum. MEMORY peak 443 MB. Another test suite ran on the machine at the same time, so compare the columns rather than the absolute times.

Range add (`RangeAddRangeSum<lng>`, `PersistentSegmentTree` built from n = 2 * 10^5 values) and dual add (`MonoidAction<Sum<lng>>`), q = 2 * 10^5 operations on a random earlier or the latest version; mixed is half `apply`, half `prod` or `get`. The cells give ms and nodes made:

| Workload | Lazy | Lazy `COMMUTATIVE` | Dual | Dual `COMMUTATIVE` |
|---|---|---|---|---|
| Mixed, random version | 324 / 6.34M | 311 / 5.11M | 160 / 5.88M | 136 / 4.70M |
| Mixed, latest version | 374 / 6.41M | 264 / 5.09M | 156 / 6.00M | 137 / 4.69M |
| Apply only, random version | 539 / 12.19M | 481 / 9.81M | 310 / 11.79M | 268 / 9.41M |
| Apply only, latest version | 473 / 12.45M | 417 / 9.81M | 302 / 12.06M | 259 / 9.41M |

`COMMUTATIVE` makes 19–22% fewer nodes and is 4–29% faster, which is why the flag exists. The `f == id()` test in `tagged` (affine maps mod 998244353 on (sum, len), 2 * 10^4 range applies, then 2 * 10^5 operations, half `set`, half `prod`) saves 12.7% of the nodes and 8% of the time (348 against 379 ms) over the same `F` without `operator==`.

Static trees, n = q = 2 * 10^5, values in `[0, 10^9)`: `PersistentValueTree` builds in 83 ms (97 ms weighted) with 3.74M nodes; 2 * 10^5 `rangeKth` take 255 ms, `countRange` 195 ms, `kthSum` 289 ms. Row 17's `WaveletMatrix` builds in 49 ms and answers `kth` in 52 ms: the wavelet matrix is the faster choice for plain static range order statistics, and this tree's place is version arithmetic. `PersistentDistinctCount` with n = q = 5 * 10^5 (the `static_range_count_distinct` shape) builds in 258 ms with 9.98M nodes and answers in 482 ms.

## Sources

| Source | Actual reading and use |
|---|---|
| suisen `persistent_segment_tree`, `persistent_lazy_segment_tree`, `persistent_dual_segment_tree`, `persistent_commutative_dual_segment_tree` | Operation catalog, the commutative tag-in-place dual tree (the `COMMUTATIVE` flag), pool sizing. |
| maspypy `dynamic_segtree.hpp`, `dynamic_lazy_segtree.hpp` | Persistent mode, `copy_interval`, `enumerate`. |
| ei1333, Nyaan, hitonanode persistent segment trees; tko919 `persistentlazysegtree` | Operation catalog (point update, range apply, copy, dump). |
| OI Wiki `ds/persistent-seg`, `ds/seg` | Prefix versions for range k-th; tag permanence (标记永久化) for commutative tags. |
| Library Checker `range_kth_smallest`, `static_range_count_distinct`, `persistent_range_affine_range_sum` | Target workloads. |

References fetched 2026-10-27 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-27-p030-sweep)).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p030-omissions): pool reset, persistent Fenwick tree, copy with an action, `kthMax`/`countLess`/`sumLess` on the value tree, k-th smallest on a tree path, position-dependent defaults on huge universes.
- Nodes are never reclaimed; a long history needs `O(q log n)` nodes, and reclaiming unreachable versions is Miscellaneous row 43.
- `merge` on two dense versions (vector-built or fill-built) costs O(n): only null subtrees short-circuit it.
- Destructive meld and split stay with row 13; row 35 is the k-ary persistent array.

## History

- 2026-10-27: P030 rewrite and first verification; legacy `PerSegTree` and `KSPerSegTree` replaced.
- 2026-10-27: independent review (P030), 6 findings fixed (`NDEBUG` termination, `lng` positions by user decision, cost and memory wording, loop index type, ledger records); full (seed 20261027) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 1559/1586/1556 MB.
