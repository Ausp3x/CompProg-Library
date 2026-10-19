# 17-waveletmatrix.hpp — evidence

Package P029 (batch DS09). Replaces the unchanged extraction `WaveletMatrix` (`OLD/algorithms.cpp:3651–3817`). Indices are zero-based; index ranges `[l, r)` and value ranges `[lo, hi)` are half-open; `kth`, `kthMax` and `select` are 0-based. Preconditions are `assert`s checked once per operation.

## Contracts

All three structures need only `T`'s `operator<` (a strict weak order whose equivalence is `operator==`, which `unique` uses) and count equal values with multiplicity. `maxLeq(l, r, x, res)` and `minGeq(l, r, x, res)` return `false` and leave `res` untouched when no value qualifies. Index preconditions stay preconditions under `NDEBUG` (an out-of-range index reads out of bounds, as with `vector::operator[]`); every loop is bounded by the level count.

### WaveletMatrix<T, W = lng>

- Construction `WaveletMatrix(a)` or `WaveletMatrix(a, w)` with `w.size() == a.size()`, `0 <= n < 2^31`. The values are compressed to ranks `0 .. k - 1` over the `k` distinct values; there are `h = bit_width(k)` levels, so the rank `k` of a bound above every value is representable (one extra all-zero level when `k` is a power of two). Level `d` (from `h - 1` down to 0) stores bit `d` of each rank in the order entering the level, as 64-bit words with prefix popcounts (two bits per position), plus `mid[d]`, its zero count; a stable partition by that bit gives the next order. The bottom order groups equal ranks contiguously (runs in bit-reversed rank order) and keeps positions increasing inside a run; `pos` maps it back to original positions. Build O(n log n) for the sort plus O(n log k); memory O(n log k / 64 + n) words, plus `h (n + 1)` weights when weighted.
- Weights `W` form an additive group with `W()` as zero; `sum[d]` holds prefix sums of the weights in the order after the partition at level `d`, so a descent subtracts two prefix sums per level. Every partial sum must be representable. `sumLess` and `kthSum` assert a weighted build (or `n == 0`).
- `access(i)`: the value at `i`, O(log k).
- `rank(r, x)`: occurrences of `x` in `[0, r)`, 0 for an absent `x`. `countEqual(l, r, x)`: occurrences in `[l, r)`. Both descend along the rank of `x`; O(log k).
- `select(x, k)` (asserts `k >= 0`): position of the `k`-th occurrence of `x`, or -1 when `x` occurs at most `k` times. A descent finds the bottom run of `x`, and `pos` gives the position; O(log k).
- `countLess(l, r, x)`, `countRange(l, r, lo, hi)`: values below `x` or in `[lo, hi)`. `countRange` with `!(lo < hi)` returns 0 after its index assert. O(log k).
- `kth(l, r, k)`, `kthMax(l, r, k)` (assert `0 <= k < r - l`): the `k`-th smallest or largest value; `kthMax` is `kth` at `r - l - 1 - k`. O(log k).
- `maxLeq`, `minGeq`: one count descent with the upper (lower) rank bound of `x`, then `kth`; no `x ± 1` arithmetic, so the extremes of `T` are ordinary inputs. O(log k).
- `sumLess(l, r, x)`: sum of the weights at positions in `[l, r)` whose value is below `x`. `kthSum(l, r, k)` (asserts `0 <= k <= r - l`): sum of the weights of the `k` smallest values, equal values counted by increasing position (the order of `(value, index)` pairs). With `w = a` these are value sums. O(log k).

### RectangleWaveletMatrix<T, W = lng>

- Points `(x[i], y[i])` with optional weights `w[i]` (sizes equal); the points are stably sorted by `x`, `xs` keeps the sorted `x`, and a `WaveletMatrix<T, W>` holds the `y` values (and weights) in that order. Duplicate and arbitrary signed coordinates are allowed.
- `rectangleCount(xl, xr, yl, yr)` and `rectangleSum(xl, xr, yl, yr)` cover `[xl, xr) x [yl, yr)`; a side with `!(lo < hi)` gives 0 or `W()`. The `x` side is two binary searches on `xs`, the `y` side `countRange` or two `sumLess` calls; O(log n). `rectangleSum` asserts weights (or no points). Memory O(n log n) with weights.

### WaveletMatrixMonoid<T, M>

- `M` is a commutative monoid (`S`, `op`, `e`): the points of a rectangle are folded in an order that follows neither coordinate. It is stored as `m` and passed last (`WaveletMatrixMonoid(x, y, v = {}, mon = {})`, stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract)). Points `(x[i], y[i])` are fixed at construction with values `v[i]` (all `e()` when `v` is empty).
- Layout: a `WaveletMatrix<T>` over the `y` values in stable `x` order, plus one bottom-up segment tree of size `2n` per level over the order after that level's partition; `at[i]` is point `i`'s position in `x` order and `step` maps a position to the next level. Build O(n log n); memory O(n log n) values of `S`.
- `get(i)` O(1); `set(i, s)` writes `s` at point `i`'s position on every level, O(log k log n); `apply(i, s)` is `set(i, op(get(i), s))`.
- `prod(xl, xr, yl, yr)`: the product over the points in `[xl, xr) x [yl, yr)`, `e()` when empty. With `a`, `b` the lower rank bounds of `yl`, `yr`, the descent follows the shared high bits of `a` and `b`. Below their first difference, `a`'s path folds the one-side ranges where `a` has a 0 bit (ranks above `a` in that subtree) and finally the run of rank `a`. `b`'s path folds the zero-side ranges where `b` has a 1 bit. O(log k log n).

### Legacy mapping

| Legacy | New |
|---|---|
| `WaveletMatrix(a)` over `lng` | `WaveletMatrix<lng>(a)`; `WaveletMatrix<lng>(a, w)` adds weights |
| `cntEql(l, r, x)` (inclusive) | `countEqual(l, r + 1, x)` |
| `cntRan(l, r, x1, x2)` (inclusive both) | `countRange(l, r + 1, x1, x2 + 1)`; at `x2 = INT64_MAX`, `(r - l + 1) - countLess(l, r + 1, x1)` (decided 2026-10-13) |
| `kthMin(l, r, k)` (1-based, inclusive) | `kth(l, r + 1, k - 1)` |
| `maxLeq`, `minGeq` (inclusive, sentinels `∓INF64`) | `maxLeq`, `minGeq` on `[l, r + 1)` with a `bool` status |
| `BitVec` (`set`, `build`, `rank0`, `rank1`), `_cntLeq`, `_cntLeqOrd` | internal levels and the `descend` helper; row 40 `SuccinctBitVector` owns the bit-vector API |
| planned `rangeFreq`, `prevValue` (strict), `nextValue`, `pointUpdate`, `rectangleFold`, `WaveletTree` | `countRange`; `maxLeq(l, r, x - 1, res)` for integers; `minGeq`; `set`/`apply`; `prod` (row 15's name); `WaveletMatrix` and `WaveletMatrixMonoid` (decided 2026-10-13) |

The legacy `cntEql`, `cntRan`, `maxLeq` and `minGeq` computed `x - 1` and overflowed at `x = INT64_MIN`. It also had no weights, `select` or rectangle forms, and returned `∓INF64`, which collides with stored values.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/17-waveletmatrix_tester.cpp`, entry `17-waveletmatrix_tester.py` (driver `_00_runner.py`; three builds; 22 assertion probes; the checked build uses `-D_GLIBCXX_ASSERTIONS`, because debug-mode `lower_bound` validates partitioning in O(n) per call and every query binary-searches the alphabet).

| Operation | Group | Oracle |
|---|---|---|
| `access`, `rank`, `select` | `wholeArray` | Direct scan of the array (every probe on exhaustive arrays, 20 sampled probes otherwise; `select` up to `count + 1`) |
| `countLess`, `countRange`, `countEqual`, `kth`, `kthMax`, `maxLeq`, `minGeq`, `sumLess`, `kthSum` | `window` over every `[l, r)` of arrays up to n = 22 (full) and 40 (stress), sampled windows on n = 1000–100000 | Sorted `(value, index)` copy of the window: indexing, counts, predecessor and successor, weight sums with ties by index; untouched `res` on absence |
| Alphabet sizes 1–64 around powers of two (level count), `int`, `string` and `lng` values, separate weights | `arrays` | Same |
| `INT64_MIN`/`INT64_MAX` values and bounds (legacy overflow), empty structure | `extremes` | Direct values |
| `rectangleCount`, `rectangleSum`, weighted and unweighted, duplicate and extreme coordinates, empty and reversed sides | `rectangles` | Scan over all points |
| `WaveletMatrixMonoid` `get`, `set`, `apply`, `prod` with a sum mod 998244353, a non-idempotent max-with-count monoid and a stateful sum modulo a member modulus (7); copy independence | `monoidRun` | Scan over all points with the current values |
| Whole-level folds through the segment-tree root (power-of-two point counts, one `y`) | `monoidEdges` | Direct values |
| Preconditions | `invalid` | Index ranges of every query, `kth`/`kthMax`/`kthSum` ranks, negative `select` rank, `sumLess`/`kthSum`/`rectangleSum` without weights, mismatched sizes, monoid point indices |

Value distributions: one value, `[-2, 1]`, `[0, 20]`, full signed range, `{INT64_MIN, INT64_MIN + 1, -1, 0, 1, INT64_MAX - 1, INT64_MAX}`, and `k` exact distinct values. Query probes are every present value, its neighbours and the type extremes. Mutation check (2026-10-13, quick mode, temporary copies): 10 of 10 injected faults detected. They covered the zero-side descent, `bit_width(k - 1)` levels, the `kth` boundary, the dropped final `kthSum` run, the `select` bound, `maxLeq` with the lower rank, `present` without equality, the rectangle `x` upper bound, the dropped rank-`a` run and a segment-tree update that skips the root.

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `17-waveletmatrix_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 5,061,905 checks per configuration, 22 assertion probes, MEMORY peak 572 MB |
| `CXX=g++-14 17-waveletmatrix_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 5,061,905 checks per configuration, MEMORY peak 558 MB |
| `17-waveletmatrix_tester.py --mode stress --seed 7` | PASS, 28,851,144 checks per configuration, MEMORY peak 683 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/17-waveletmatrix_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/17-waveletmatrix_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/17-waveletmatrix_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| ei1333 `wavelet-matrix`, `wavelet-matrix-rectangle-sum`, `wavelet-matrix-point-add-rectangle-sum`; noshi91, hitonanode, tko919, kopricky, suisen wavelet matrices | Level layout, rank descents, weighted prefix sums per level, rectangle reduction by sorting `x`. |
| Nyaan `segment-tree-on-wavelet-matrix`, `fenwick-tree-on-wavelet-matrix`; maspypy `wavelet_matrix_2d_range` | Per-level trees for point updates with rectangle folds. |
| Library Checker `range_kth_smallest`, `static_range_frequency`, `static_range_sum_with_upper_bound`, `rectangle_sum`, `point_add_rectangle_sum` | Query shapes. |

Fetched 2026-10-13 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-13-p029-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p029-omissions): xor queries (row 26), value-order `maxRight`, multi-range and tree-path descents, `topK`/`rangeList`/`intersect`, Fenwick nodes.
- `08-Python/00-index.md` row `_56_wavelet_matrix.py` still lists `prevValue`, `nextValue` and `rangeFreq`; its package should adopt this row's names (`04-python.md`).
- `WaveletMatrixMonoid` keeps `prod` beside row 15's `SegmentTree2DSparse::prod` as a different algorithm, and `WaveletMatrix::rank` stays beside `countEqual` (both recorded keeps, decided 2026-10-13). It needs a commutative monoid; noncommutative rectangle folds have no order to follow. Its memory is `2n` values of `S` per level.

## History

- 2026-10-13: P029 first verification; the unchanged extraction was rewritten (MergeSortTree names, half-open ranges, `bool` status), with weights, `select`, `RectangleWaveletMatrix` and `WaveletMatrixMonoid` added. The independent review found no wavelet defects. The user renamed `rectangleFold` to `prod` and recorded `rank` and `prod` as keeps.
- 2026-10-17: P027 monoid pass (stored-object monoid contract for `WaveletMatrixMonoid`, stateful-monoid test added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 572/558/683 MB.
