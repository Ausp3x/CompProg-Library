# 16-mergesorttree.hpp — evidence

Package P028 (batch DS08). Replaces the unchanged extraction `DynMergeSortTree` (`OLD/algorithms.cpp:3185–3459`) and absorbs the legacy reference `97-Legacy/05-mergesorttree_older.cpp` (`OLD/[1] algorithms.cpp:695–799`). Indices are zero-based, index ranges `[l, r)` and value ranges `[lo, hi)` are half-open, `kth` is 0-based, and preconditions are `assert`s checked once per operation.

## Contracts

All three structures need only `T`'s `operator<` (a strict weak order whose equivalence is equality); equal values are counted with multiplicity. `maxLeq(l, r, x, res)` and `minGeq(l, r, x, res)` return `false` and leave `res` untouched when no value qualifies; `countRange` with `!(lo < hi)` returns 0 after its index-range assert, so an invalid `[l, r)` fails even then.

### MergeSortTree<T> (static, fractional cascading)

- `0 <= n <= 2^29`. Values are ranked by a stable sort (ties by index), so ranks are a permutation. Level `d` of `rk` stores each midpoint-split node's ranks sorted in place, and `lc[d][a + p]` counts the left-child ranks among the node's first `p`; a count of ranks below a root bound therefore descends to both children in O(1) (`p == len` uses `m - a`). Memory: `vals` plus `2 (ceil(log2 n) + 1) n` ints; build O(n log n).
- `countLess`, `countRange`, `countEqual`: one or two root binary searches, then O(log n) cascaded nodes.
- `maxLeq`, `minGeq`: one root binary search; each canonical node with cascaded count `p` contributes its rank at `p - 1` or `p`; O(log n).
- `kth(l, r, k)` (asserts `0 <= k < r - l`): binary search over root ranks with cascaded counts, O(log^2 n). Row 17 (wavelet matrix) and row 18 (persistent `rangeKth`) give O(log n) or O(log sigma).

### DynamicMergeSortTree<T>

- Slots `[0, n)` each hold a multiset; `insert(i, x)`, `erase(i, x)` (removes one copy, returns whether one existed), `set(i, x)` (requires the slot to hold exactly one value; the vector constructor creates that state), `size(l, r)`.
- Layout: a bottom-up segment tree of `2n` GNU order-statistic trees keyed by `(value, uid)` plus one global tree; `uid` makes equal values distinct and fewer than 2^31 insertions are allowed over the lifetime. Every one of the `2n + 1` trees allocates a header node, and each stored value lives in about `log2(n) + 2` trees (roughly 80–90 bytes each with GCC's node and allocator overhead): a memory-capped probe building 10^5 random values peaked at 172 MB (2026-10-10).
- Memory O(n + m log n). `insert`, `erase`, `set`: O(log n log m) for m stored values; under `NDEBUG`, `set` on an empty slot only inserts. `countLess`, `countRange`, `countEqual`, `maxLeq`, `minGeq`: O(log n log m) over the O(log n) canonical nodes. `kth`: binary search over global ranks of `(value, uid)`; the answer's global rank is the largest `g` whose count of range elements below `all[g]` is at most `k`, O(log n log^2 m). Construction from a vector O(n log^2 n).
- Two-dimensional use: point `(x, y)` with `x` compressed to a slot is `insert(x, y)`; rectangle counts are `countRange(xl, xr, yl, yr)`, and `kth`, `maxLeq`, `minGeq` act on the y values of an x-slot range.

### PointSetRangeFrequency<T>

- `0 <= n <= 2^29`; `std::map` from value to the order-statistic set of its positions; empty sets are erased so memory stays O(n). `get(i)` O(1), `set(i, x)` and `count(l, r, x)` O(log n).

### Legacy mapping

| Legacy | New |
|---|---|
| `DynMergeSortTree(a)`; `update1D(i, x)` | `DynamicMergeSortTree(a)`; `set(i, x)` |
| `cntEql1D(l, r, x)`, `cntRan1D(l, r, j1, j2)` (inclusive) | `countEqual(l, r + 1, x)`, `countRange(l, r + 1, j1, j2 + 1)` |
| `kthMin1D(l, r, k)` (1-based k, inclusive) | `kth(l, r + 1, k - 1)` |
| `maxLeq1D`, `minGeq1D` (sentinels `∓INF64`) | `maxLeq`, `minGeq` with a `bool` status |
| `DynMergeSortTree(Mn_i, Mx_i)`; `insert2D(i, j)` returning an id; `erase2D(i, j, id)` | `DynamicMergeSortTree(n)` over compressed x; `insert(x, y)`; `erase(x, y)` (any copy) |
| `cntRan2D`, `cntEql2D`, `kthMin2D`, `maxLeq2D`, `minGeq2D` | `countRange`, `countEqual`, `kth`, `maxLeq`, `minGeq` on slot ranges |
| `MergeSortTree::rangeCountLessThanQuery(l, r, x)` (older reference) | `countLess(l, r + 1, x)` |
| `rangeMedianQuery(l, r)` (value binary search over `[0, 10^18]`) | `kth(l, r + 1, (r - l + 1) / 2)` |

The legacy dynamic tree reserved 2.5 million nodes up front, searched `kth` over the whole `lng` range (64 probes of O(log^2) each) and took online `lng` x coordinates in a dynamic segment tree; the replacement needs x compressed to slots (a documented reduction; online huge coordinates belong to row 13 or row 33's offline sweeps). The older static reference read exhausted merge sides before its guards.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/16-mergesorttree_tester.cpp`, entry `16-mergesorttree_tester.py` (driver `_00_runner.py`; three builds; 18 assertion probes; the checked build uses `-D_GLIBCXX_ASSERTIONS` because GCC's PBDS debug mode validates whole trees on every operation and the full corpus exceeded the runner's 180 s limit, while `-O0` with assertions finishes in about 1 s).

| Operation | Group | Oracle |
|---|---|---|
| MergeSortTree `countLess`, `countRange`, `countEqual`, `kth`, `maxLeq`, `minGeq` for `lng`, `string`, `pair<int, int>` | `staticRun` + shared `compare` | Sorted copy of the window: `lower_bound`/`upper_bound` counts, indexing, predecessor and successor |
| Legacy median | `staticRun` | `nth_element` on every window starting in the first 12 positions |
| DynamicMergeSortTree `insert`, `erase` (present and absent values), `set`, `size` and every query, single-valued and multiset slots, `lng` and `string` | `dynamicRun` + `compare` | Vector of per-slot multisets |
| PointSetRangeFrequency `set`, `get`, `count` for `lng` and `string` | `frequencyRun` | `std::count` over the window |
| Empty structures, untouched `res`, extreme values, duplicates in one slot, erase of one copy, copy independence, erasing emptied position sets | `edges` | Direct values |
| Preconditions | `invalid` | Reversed and out-of-range windows (including `countRange` with `lo >= hi` on both trees: `static-countrange-end`, `dynamic-countrange-reversed`), `kth` with `k` out of range or an empty window, negative size, slot out of range, `set` on a multi-valued or empty slot |

Sizes 0–129 (plus 513 and 1000 in stress) with value spans 0, 3 and 10^12. Mutation check (2026-10-10, quick mode, temporary copies): 9 of 9 injected faults detected (cascade at a full node, cascade counter, `kth` bound, `maxLeq` rank bound, erase of a different value, `countEqual` bound, frequency window end, `minGeq` bound, dynamic `kth` bound).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/16-mergesorttree_tester.py' --mode quick --seed 1
python3 '96-Local Testing/02-Data Structures/16-mergesorttree_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/16-mergesorttree_tester.py' --mode full --seed 1
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/16-mergesorttree.hpp' '96-Local Testing/02-Data Structures/16-mergesorttree_tester.cpp'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/16' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
```

Latest run 2026-10-12 (after the `countRange` assert fix): quick mode seed 1 passed (2 configurations, 55,529 checks, 18 assertion probes); full mode seed 1 passed on GCC 16.2 and GCC 14.4.1 (`CXX=g++-14`) with 960,253 checks per configuration and 18 assertion probes, MEMORY peak 632 MB and 596 MB; `--braces` and `03-consistency.py` reported no errors. Stress (`--mode stress --seed 3`) passed with 4,683,876 checks per configuration, peak 645 MB, and `python3 '96-Local Testing/02-integration.py' --sanitizers` passed (136 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, sanitizer self-tests, peak 2677 MB). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_ASSERTIONS`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds; MEMORY peaks are dominated by compilation. GCC 14.2 exactly, the Windows build and PyPy were not run.

## Sources

| Source | Actual reading and use |
|---|---|
| ei1333 `segment-tree-fractional-cascading`; hitonanode `merge_sort_tree`; cp-algorithms segment tree (merge sort tree, fractional cascading, multiset nodes) | Static layout with cascaded counts and the dynamic multiset variant. |
| tko919 `pointsetrangefreq`; Library Checker `point_set_range_frequency`, `range_kth_smallest`, `static_range_frequency` | One position set per value; query shapes. |
| OI Wiki seg-in-bit, balanced-in-seg | O(log^2) dynamic kth via Fenwick of value trees (left out, see Limits). |

Fetched 2026-10-10 by the completeness sweep ([00-sources.md](00-sources.md)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p028-omissions): `sumLeq`/`countSumLeq` (row 17 `sumLess`), count wrappers `countGreater`/`countGeq`/`countLeq` (`r - l - countLess` and `countLess` of the next value), Fenwick-of-value-trees dynamic kth.
- `MergeSortTree::countRange` on the points `(i, a_i)` overlaps row 43's `RangeTree.count`; this row covers arrays, row 43 general point sets.

## History

- 2026-10-10: P028 first verification; `DynMergeSortTree` split into `MergeSortTree` (static) and `DynamicMergeSortTree`, `PointSetRangeFrequency` added, `97-Legacy/05-mergesorttree_older.cpp` retired.
- 2026-10-10: independent review (P028): no wrong answers; guarded `set` on an empty slot under `NDEBUG` and corrected the memory term of the complexity line.
- 2026-10-12: re-audit fix: `countRange` asserts its index range before the `lo < hi` shortcut in both trees (2 new probes); full suite passed on g++ and g++-14.
