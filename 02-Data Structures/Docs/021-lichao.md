# 021-lichao.hpp — Evidence

Package P032 (batch DS11). Rewrite of the unchanged legacy extraction (`OLD/algorithms.cpp:3935-4116`, `4232-4423`): three Li Chao trees over one function type `F`, each keeping one envelope chosen by `MAX`. The header has no ISA code, reduction or dispatch threshold, so the Contest profile requires no benchmark. Each call asserts its preconditions once.

## Contracts

### Function Type and Envelope

- `F` is copyable and has `T eval(lng x) const`; `T` is ordered by `<` and has `std::numeric_limits`. The default `Line {m, b}` evaluates `m * x + b` in `lng`; it lives in `lichao_detail` and is public as `LiChaoTree<>::Line` and the other trees' `Line`. Calls take an `F` (`addLine({m, b})`). Floating `T` is compared as computed: the trees return the best computed value, and single crossing must hold for the computed values. Tested types are `lng` lines and parabolas and `double` parabolas; other integer `T` are untested.
- Single crossing: for any two stored functions `f` and `g`, the domain points where `f` is strictly better than `g` form a prefix or a suffix of the domain. Any two lines qualify, and so do parabolas `a (x - p)^2 + c` with a common `a`. Every value at every domain point a function covers is exact (no overflow, no NaN) and strictly between `NONE` and the opposite limit.
- `MAX = false` keeps the lower envelope and `true` the upper one; `better` is strict `<` or `>`. `NONE` is `std::numeric_limits<T>::max()` for min or `lowest()` for max, returned when no stored function covers `x`; it never collides with a value, because values stay strictly inside it.
- Each node keeps the function that is better at its midpoint (the lower midpoint for even length). The loser can win only on one side, which the comparison at the left end decides, and moves down one level. Insertion therefore touches one root path, and every domain point's optimum lies on its root path. Ties keep the stored function.

### LiChaoTree<MAX = False, UNDO = False, F = Line>

- `LiChaoTree(lo, hi)`: the domain is the integers `[lo, hi)`; asserts `lo < hi` and accepts any `lng` bounds, so `x <= LLONG_MAX - 1`. `std::midpoint` never overflows, and a node is a leaf when its midpoint equals its left end, so the depth is at most 64. Nodes are created on demand: `t` holds `{function id, left, right}` per node (an offset into `t`, -1 for none) with the root at 0, and `fs` holds the inserted functions.
- `addLine(f)`: O(log(C)) with C = hi - lo; creates at most one node.
- `addSegment(l, r, f)`: `f` covers `[l, r)` intersected with `[lo, hi)`; an empty intersection stores `f` and changes nothing else. O(log(C)^2) time and O(log(C)) nodes.
- `query(x)`: asserts `lo <= x < hi`; returns the best value at `x` or `NONE`. O(log(C)).
- `merge(o)`: inserts every function stored in a node of `o` at that node's position in `*this`, creating the path, so a function only moves down. Asserts `this != &o` and equal domains; `o` is unchanged. Cost: O(nodes of o) plus one step per level a function moves down, at most log(C) per function over its lifetime. Merging the smaller tree into the larger (`std::swap` the objects first) gives O(N (log(N) + log(C))) over a tree DP of N functions (OI Wiki). Copying node by node keeps trees independent objects (user decision 2026-10-28, against one shared node pool).
- `undo()`: needs `UNDO = true` (`static_assert`). Reverts the latest `addLine`, `addSegment` or `merge` not yet reverted, restoring every node write and removing the nodes and functions it added, in O(writes of that call). Asserts there is a call to revert; under `NDEBUG` it returns. With `UNDO`, each call records its sizes and every node write logs `(slot, old value)`: at most log(C) + 1 pairs per line, O(log(C)^2) per segment and O(nodes of o + moves) per merge.
- Memory: O(n + s log(C)) `int` triples for n calls with s segments, plus the log.

### PerLiChaoTree<MAX = False, F = Line>

- `PerLiChaoTree(lo, hi)`: domain as in `LiChaoTree`; version 0 is empty. `reserve(nodes)` (user decision 2026-10-28) reserves storage for `nodes` nodes, as row 18's persistent trees do, and changes no result.
- `addLine(ver, f)` and `addSegment(ver, l, r, f)` assert `0 <= ver < roots.size()` and return the new version id (`roots.size() - 1`); earlier versions never change. Path copying creates O(log(C)) nodes per line and O(log(C)^2) per segment; an empty segment returns a new id sharing `ver`'s root.
- `query(ver, x)`: asserts a valid version and `lo <= x < hi`. O(log(C)).
- `LiChaoTree::query` and `PerLiChaoTree::query` return the value only; only `LiChaoTreeCompressed` returns a line id (user decision 2026-10-28).

### LiChaoTreeCompressed<MAX = False, UNDO = False, F = Line>

- `LiChaoTreeCompressed(xs)`: sorts and deduplicates the m query points in O(m log(m)). The tree is a perfect binary tree over `sz = bit_ceil(max(m, 1))` leaves, padded with the largest point; functions are evaluated only at real points. An empty point set is allowed and then admits no query.
- `addLine(f)`: O(log(m)). `addSegment(l, r, f)`: `f` covers the points `x` with `l <= x < r`, O(log(m)^2).
- `query(x)`: asserts that `x` is a point. Returns `{value, line id}`, where the id is the index in `fs` (insertion order of the `addLine` and `addSegment` calls not undone) of a function attaining the optimum, the first on the leaf-to-root path; or `{NONE, -1}`.
- `undo()`: as in `LiChaoTree` (no nodes are created); the next insertion reuses the reverted function's id.

### Violated Preconditions

Under `NDEBUG`, termination still holds. With `lo >= hi` every segment is empty, and insertions stop where the midpoint reaches the left end; the range halves or is already degenerate at each level, so every path stays within 64 levels and each `addLine` creates at most one node (path copying one per level). A query outside the domain walks one root path and returns a meaningless value. A bad version id reads `roots` out of range. A compressed query at a non-point walks from the leaf of the next larger point and evaluates `F` at `x` (a meaningless value); above the last point it starts from leaf `m`, which is out of range when `m` is a power of two, as with `vector::operator[]`. Overflowing or NaN values break the comparisons, not termination.

### Legacy Mapping

| Legacy | Replacement |
|---|---|
| `LiChaoTree(Mn_x, Mx_x)` over the closed `[Mn_x, Mx_x]`, 2.5M-node reserve | `LiChaoTree<MAX>(lo, hi + 1)`; nodes grow on demand |
| `addLine(m, b)` (updated both envelopes) | `addLine({m, b})` on a `LiChaoTree<false>` and a `LiChaoTree<true>` |
| `addSegm(l, r, m, b)` (closed; ignored unless `mn_x <= l <= r <= mx_x`) | `addSegment(l, r + 1, {m, b})` (clipped to the domain) |
| `maxQuery(x)`, `minQuery(x)` (±`INF64` outside the domain or when empty) | `LiChaoTree<true>::query(x)`, `LiChaoTree<false>::query(x)` (asserted domain, `NONE` when empty) |
| `PerLiChaoTree` `addLine(ver, m, b)`, `addSegm(ver, l, r, m, b)`, `maxQuery(ver, x)`, `minQuery(ver, x)`, 10M-node reserve | `PerLiChaoTree<MAX>` `addLine(ver, {m, b})`, `addSegment(ver, l, r + 1, {m, b})`, `query(ver, x)` |
| `OLD/[1] algorithms.cpp:661-691` `LiChaoTree` (complex-number hull skeleton without queries; was `97-Legacy/04-lichao_older.cpp`) | Not adopted: an unfinished monotone hull; `MonotoneCHT` (row 22) is the finished structure |
| `OLD/Team Notebook/src/geometry/lichaotree.cpp` (notebook copy of the legacy `LiChaoTree`) | As the `LiChaoTree` rows above |
| Planned row names `queryWitness`, `addSegm` | `LiChaoTreeCompressed::query` returning `{value, line id}` (user decision 2026-10-28: `query` was the value field of the witness form); `maxQuery`/`minQuery` → `query` under `MAX` (user decision 2026-10-28); `addSegment` |

Catalog aliases: `chmin_line`, `update`, `add` → `addLine`; `get`, `get_val`, `getmin`, `operator()` → `query`; ei1333 `persistent-dynamic-li-chao-tree`, maspypy `PERSISTENT` flag → `PerLiChaoTree`; Nyaan `li-chao-tree-abstruct`, maspypy `FUNC` → the `F` parameter; Library Checker `line_add_get_min`, `segment_add_get_min` → `LiChaoTree` or `LiChaoTreeCompressed` (half-open segments).

Search aliases (P263, grab --find): 斜率优化, 李超线段树, 李超树, 李超树合并, 可持久化李超树, 插入线段求最值, 一次函数最值.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/021-lichao_tester.cpp`, entry `021-lichao_tester.py` (driver `_00_contest_runner.py`; three builds; 20 assertion probes). Function types: lines, integer parabolas `a (x - p)^2 + c` with one `a` per tree, and double parabolas `0.5 (x - p)^2 + c` compared exactly against the same doubles. Every generated function is checked in `__int128` against the value domain, so the `-DNDEBUG` build stays inside the contract. Quick mode runs 40 steps, 2 tiny rounds and n = 2000 at scale.

| Operation | Group | Oracle |
|---|---|---|
| `Line::eval` | `edges` (fixed and 1000 random) | `__int128` |
| `LiChaoTree` `addLine`, `addSegment` (clipped, empty, outside), `query`, `merge`, `undo`; both `MAX`, `UNDO` on and off | `treeRandom` (300 steps per tree in full, 1000 in stress; every x of tiny and singleton domains, sampled x on Library Checker and full-`lng` domains including `LLONG_MIN`, `LLONG_MAX - 1`), `mergeChain` (small-to-large with `std::swap`, merges of empty and segment trees, merge then undo), `largeDense`, `largeSparse` (n = 2 * 10^5 full, 4 * 10^5 stress) | Brute force over the stored function list (union after merges, truncated after undo); a dense envelope array at scale |
| `PerLiChaoTree` `addLine(ver)`, `addSegment(ver)`, `query(ver)`, returned ids, `reserve` | `perRandom` (branches from random older versions; `reserve` before updates and on a quarter of the steps, capacity at least 3 * nodes and existing nodes unchanged), `edges`, `largePer` (10^5 full, 2 * 10^5 stress; `reserve(40 n)` then no reallocation) | Parent chain of function lists per version; answers recorded at creation re-checked at the end (old versions unchanged) |
| `LiChaoTreeCompressed` constructor (unsorted, duplicates, empty), `addLine`, `addSegment` (half-open in x, between and outside points), `query` (value and id), `undo` (id reuse) | `compressedRandom`, `edges`, `largeCompressed` (n = 2 * 10^5; 5000 in the checked build, where `_GLIBCXX_DEBUG` makes `lower_bound` linear) | Brute value; the id must be a live stored function covering `x` that attains it, or `{NONE, -1}` |
| Preconditions | `invalid` | `lo >= hi` (both trees), queries below and above the domain, merge across domains and with itself, undo on an empty history (both `UNDO` trees) and on an exhausted one (both), bad versions in `addLine`, `addSegment`, `query`, compressed query at a non-point |

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 1200 MB; optimized 0.1 + 6.0 s, checked 42.4 + 18.1 s, ASan-UBSan 54.1 + 52.3 s; MEMORY peak 1776 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 1208 MB; optimized 0.3 + 12.3 s, checked 92.3 + 26.8 s, ASan-UBSan 113.0 + 105.8 s; MEMORY peak 1739 MB |
| `01-run.py --mode quick` | Optimized 77.4 + 0.3 s, checked 49.0 + 4.5 s; MEMORY peak 1195 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 1202 MB; optimized 0.1 + 11.8 s, checked 28.0 + 21.6 s, ASan-UBSan 31.1 + 81.2 s; MEMORY peak 1050 MB |

Latest per-header run 2026-10-28 on the same machine with CPython 3.14.8, in the optimized, checked and ASan/UBSan builds:

| Command (2026-10-28) | Result |
|---|---|
| `021-lichao_tester.py --mode quick` | PASS, 2 configurations, 165,117 checks per configuration, 20 assertion probes, MEMORY peak 725 MB |
| `021-lichao_tester.py --mode full --seed 20261028` (GCC 16.2.1) | PASS, 3 configurations, 4,643,818 checks per configuration, 20 assertion probes, MEMORY peak 889 MB |
| `CXX=g++-14 021-lichao_tester.py --mode full --seed 20261028` (GCC 14.4.1, floor) | PASS, 3 configurations, 4,643,818 checks per configuration, 20 assertion probes, MEMORY peak 857 MB |
| `021-lichao_tester.py --mode stress --seed 7` | PASS, 3 configurations, 18,429,975 checks per configuration, 20 assertion probes, MEMORY peak 888 MB |
| `02-integration.py` | PASS, 178 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 1592 MB (cap 8192 MB) |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/021-lichao_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/021-lichao_tester.py' --mode full --seed 20261028
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/021-lichao_tester.py' --mode full --seed 20261028
python3 '96-Local Testing/02-Data Structures/021-lichao_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| OI Wiki *李超线段树*; cp-algorithms *Convex hull trick and Li Chao tree* | Midpoint invariant, segment insertion in O(log^2), Li Chao merge and its depth argument. |
| Nyaan `li-chao-tree-abstruct`; maspypy `dynamic_lichao`; ei1333 `li-chao-tree`, `dynamic-li-chao-tree`; suisen, tko919, hitonanode Li Chao trees | Operation catalog, generic function types, persistence, compressed query points, segment conventions. |

Fetched 2026-10-28 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-28-p032-ds11-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p032-omissions) (user decision 2026-10-28): real x coordinates, line deletion, node-pool reset. Minimum over an x-range and lazy-tag extended trees are planned Esoteric row 74 (`ExtendedLiChaoTree`).
- `merge` copies `o` node by node, because trees own separate node arrays. Merging a large accumulated tree into each new small one costs Θ(N^2) node copies, so callers merge the smaller tree into the larger. OI Wiki's pointer-sharing merge is O(N log(C)) in any order but needs one node pool for all trees.
- `LiChaoTree` and `LiChaoTreeCompressed` stay separate: an integer range versus a known point set (`duplicates.md`, Recorded Decisions, Keeps).

## History

- 2026-10-28: P032 DS11 rewrite; the independent review's 3 findings were fixed (degenerate-domain leaf test `md == l`, lowercase constructor parameters, the merge cost note); full (seed 20261028) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-28: `/finish` DS11: `PerLiChaoTree::reserve`, the same-line tie rule and the kept small-to-large merge (user decisions), a compressed exhausted-undo probe, documentation and citation fixes; quick, full on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
