# 024-interval_set.hpp — Evidence

Package P027 (batch DS47). New header; no legacy excerpt. Intervals are half-open over an integer type, each operation asserts its preconditions once, and both structures are `std::map` wrappers that copy and move like maps.

## Contracts

### IntervalSet<T = lng>

A set of integers stored as maximal disjoint half-open intervals `[l, r)`. Touching intervals (`r == l'`) are merged, so the stored intervals are exactly the maximal runs of covered points. `T` is a signed integer type (`int`, `lng`), and coordinates may be any value of `T`. Every inserted or erased length `r - l` and `unionLength()` must fit `T`, because the return values and the running total are kept in `T` (`IntervalSet<int>{}.insert(-2e9, 2e9)` overflows). `next` returns `numeric_limits<T>::max()` for nothing found; no interval covers it, because `r` is exclusive.

- `count()` is the number of intervals, `unionLength()` the number of covered points (maintained, O(1)), and `intervals()` the sorted list (O(n)).
- `find(x)` returns the interval containing `x`, or `{x, x}` when `x` is uncovered, so `find(x).second` is the smallest uncovered point `>= x`. `contains(x)` tests a point; `covers(l, r)` is true when one interval contains `[l, r)` (vacuously for `l == r`); `next(x)` returns the smallest covered point `>= x` or the sentinel.
- `insert(l, r)` returns the number of newly covered points and `erase(l, r)` the number of removed points. Both cost `O((k + 1) log n)` for `k` intervals touched, amortised `O(log n)`, because every call creates at most two intervals and each interval is deleted once.

### IntervalMap<K, V> (Alias `ChthollyTree<K, V>`)

A piecewise-constant function on `[lo, hi)` stored as `start -> value`. The constructor takes the initial value of the whole universe; an empty universe has no pieces. `V` needs `==`, which `assign` and `merge` use to coalesce equal neighbours.

- `count()` is the number of pieces and `get(x)` the piece `{l, r, value}` containing `x`. `enumerate(l, r, visit)` visits the pieces of `[l, r)` clipped to it (const, no splitting), and `fold(l, r, init, step)` accumulates `step(acc, l', r', value)` over the same pieces.
- `split(x)` ensures a boundary at `x` and returns the map iterator of the piece starting there (`end()` for `x == hi`). `assign(l, r, v[, onRemoved])` replaces `[l, r)` by one piece, calls `onRemoved(l', r', old)` for each removed piece clipped to `[l, r)`, and merges with equal neighbours. `apply(l, r, f)` splits at `l` and `r` and calls `f(l', r', V &)` on every piece inside. `merge(l, r)` coalesces equal adjacent pieces whose boundary lies in `[l, r]`.
- Costs: `get` and `split` are `O(log n)`. `assign` is `O((k + 1) log n)` for `k` removed pieces, amortised `O(log n)` because each assignment creates at most three pieces. `apply`, `enumerate`, `fold` and `merge` are `O((k + 1) log n)` for the `k` pieces met, without amortisation: without assignments the piece count never shrinks, so alternating `apply` on `[lo, hi)` over `n` distinct pieces costs `O(n)` per call. The classical ODT bound (expected `O(n log n)` pieces touched overall) holds only for uniformly random assignment ranges, as on CF 896C, and is no worst-case guarantee.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `IntervalSet::mex(x)` | `find(x).second` |

### Correctness and Cost

`insert` walks the predecessor that reaches `l` and every interval starting at or before `r`, takes their hull and returns `hull length − previously covered length`, so touching intervals merge with a zero return. `erase` walks the predecessor that strictly overlaps `l` and every interval starting before `r`, and re-inserts the clipped remnants `[a, l)` and `[r, b)`. `assign` splits at `r` and then at `l` (map iterators survive insertions, so this is only the conventional ODT order), erases the range, emplaces the new piece and checks both neighbours. `split` at an existing boundary is a lookup.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/024-interval_set_tester.cpp`, entry `024-interval_set_tester.py` (driver `_00_contest_runner.py`; three builds; 11 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| IntervalSet `insert`, `erase` (return values), `count`, `unionLength`, `intervals`, `find`, `contains`, `covers`, `next`, copy independence | `setSmall<lng>`, `setSmall<int>` | Dense bitmap over `[-20, 60)` with maximal runs recomputed after every step, `find(x).second` also against a forward scan to the first uncovered point; every point in `[-21, 60]`, every `covers` pair, empty and length-1 ranges, ranges reaching outside the bitmap |
| Same at the numeric extremes of `int` and `lng` (intervals starting at `min`, ending at `max`, `max` never covered, `next(max)` sentinel, erase leaving one point at each end) | `setExtremes<int>`, `setExtremes<lng>` | Hand-computed lengths and pieces with `unionLength` at `max / 2 + 7` |
| Same with 1e18 coordinates | `setLarge` | Sorted-merge union of the inserted intervals, clipped erase, `find`/`next`/`covers` at random and boundary points |
| IntervalMap `assign` with callback (lengths and old values) and without (3-argument form, half of the random assigns), neighbour merging on both sides for both forms, `apply` (piece chain covers `[l, r)`), `fold`, `merge` (no equal neighbours remain inside `[l, r]`; minimal piece count after a full merge), `split`, `get`, `enumerate`, `count`; `ChthollyTree` alias; empty universe | `mapTests` | Dense array over universes `[0,0)`, `[0,1)`, `[-5,7)`, `[0,40)`, `[-30,50)`; pieces must partition the universe and be uniform in the array |
| Preconditions | `invalid` | 11 probes: reversed ranges on both structures, reversed universe, keys and ranges outside `[lo, hi)` for `get`, `assign`, `apply`, `enumerate`, `split`, `merge` |

A hand mutation check on 2026-10-08 (quick mode, temporary copies) detected 6 of 9 injected faults: the insert predecessor test, the touching-interval loop bound, missing neighbour merges on both sides of `assign`, an unclipped `enumerate` and the `find` boundary. The three survivors are equivalent: an extra zero-length pass in `erase`, a redundant `break`, and `emplace_hint` on an existing key.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 809 MB; optimized 0.3 + 3.8 s, checked 18.6 + 6.5 s, ASan-UBSan 22.7 + 46.0 s; MEMORY peak 1000 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 853 MB; optimized 0.1 + 3.8 s, checked 19.4 + 4.8 s, ASan-UBSan 22.1 + 21.7 s; MEMORY peak 1047 MB |
| `01-run.py --mode quick` | Optimized 23.4 + 0.7 s, checked 21.4 + 3.7 s; MEMORY peak 809 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 809 MB; optimized 0.1 + 6.8 s, checked 7.5 + 10.4 s, ASan-UBSan 9.2 + 93.9 s; MEMORY peak 622 MB |

Latest per-header run 2026-10-17 (P027 monoid pass) on the same machine with CPython 3.14.7, in the optimized, checked and ASan/UBSan builds:

| Command (2026-10-17) | Result |
|---|---|
| `024-interval_set_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 4,070,377 checks per configuration, 11 assertion probes, MEMORY peak 515 MB |
| `CXX=g++-14 024-interval_set_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 4,070,377 checks per configuration, MEMORY peak 521 MB |
| `024-interval_set_tester.py --mode stress --seed 7` | PASS, 24,382,531 checks per configuration, MEMORY peak 558 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/024-interval_set_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/024-interval_set_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/024-interval_set_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| suisen `range_set.hpp` | `insert`/`erase` returning counts, `number_of_elements`/`number_of_ranges`, `minimum_excluded`, same-interval test (covered by `covers`). |
| Nyaan `segment-set.hpp` | `next`, `lower_bound`-style iteration, covered test. |
| tko919 `rangeunionset.hpp` | `mex(x)`, here `find(x).second`. |
| maspypy `intervals.hpp` | `get`, `enumerate_range` with removal callback, `merge_at`. |
| OI Wiki Chtholly tree (https://oi-wiki.org/misc/odt/) | `split`/`assign` primitives and the random-data complexity caveat. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and Handoffs

- Row 24 is partial: `RangeAssignSegmentTree` and `IntervalMap::values` are missing (returned by P275). The splay-backed sliding piecewise-constant function is planned Esoteric row 71 (`PiecewiseConstantSlide`).
- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): `sameInterval`, `maxExcluded`, iterator-returning `lowerBound`, and `IntervalsFast` over a fastset (row 36 owns the fastset).
- Dynamic mex over a set of values (the `DynamicMex` legacy note) is `IntervalSet::find(0).second` with `insert(x, x + 1)`.
- `IntervalMap` with a monoid-weighted segment tree gives amortised `O(log n)` range assign with range composite, the alternative to row 00's `RangeAssignRangeComposite`.

## Aliases

Search aliases (P263, grab --find): ODT (old driver tree), Chtholly tree (珂朵莉树), color segments (颜色段均摊); 珂朵莉树/颜色段均摊, 老司机树, 区间推平, 区间赋值, 区间集合, 区间并, 区间合并.

## History

- 2026-10-08: P027 first verification.
- 2026-10-08: independent review (P027), 2 findings fixed.
- 2026-10-08: P027 re-audit pass (second session), 1 finding fixed.
- 2026-10-12: re-audit, 1 finding fixed, full suite passed on g++ and g++-14.
- 2026-10-13: prune, removed `IntervalSet::mex` (cases moved to `find(x).second`); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (header and tests unchanged; rerun for the memory peaks); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
