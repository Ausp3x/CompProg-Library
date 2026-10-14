# 24-interval_set.hpp — evidence

Package P027 (batch DS47). New header; no legacy excerpt. Intervals are half-open over an integer type, preconditions are `assert`s checked once per operation, and both structures are plain `std::map` wrappers that copy and move like maps.

## Contracts

### IntervalSet<T = lng>

A set of integers stored as maximal disjoint half-open intervals `[l, r)`; touching intervals (`r == l'`) are merged, so the stored intervals are exactly the maximal runs of covered points. `T` is a signed integer type (`int`, `lng`); coordinates may be any value of `T`, but every inserted or erased length `r - l` and `unionLength()` must fit `T` (the return values and the running total are kept in `T`; `IntervalSet<int>{}.insert(-2e9, 2e9)` overflows), and `next` uses `numeric_limits<T>::max()` as its "nothing" sentinel, which no interval can cover because `r` is exclusive.

- `count()` number of intervals, `unionLength()` total covered points (maintained, O(1)), `intervals()` the sorted list (O(n)).
- `find(x)` the interval containing `x` or `{x, x}` when uncovered, so `find(x).second` is the smallest uncovered point `>= x`; `contains(x)`; `covers(l, r)` true when one interval contains `[l, r)` (vacuously for `l == r`); `next(x)` smallest covered point `>= x` or the sentinel.
- `insert(l, r)` returns the number of newly covered points; `erase(l, r)` returns the number of removed points. Both are `O((k + 1) log n)` for `k` intervals touched, amortised `O(log n)` because every call creates at most two intervals and each interval is deleted once.

### IntervalMap<K, V> (alias `ChthollyTree<K, V>`)

A piecewise-constant function on `[lo, hi)` stored as `start -> value`; the constructor takes the initial value for the whole universe (an empty universe has no pieces). `V` needs `==` (used by `assign` and `merge` to coalesce equal neighbours).

- `count()` pieces, `get(x)` the piece `{l, r, value}` containing `x`, `enumerate(l, r, visit)` the pieces of `[l, r)` clipped to it (const, no splitting), `fold(l, r, init, step)` accumulates `step(acc, l', r', value)` over the same pieces.
- `split(x)` ensures a boundary at `x` and returns the map iterator of the piece starting there (`end()` for `x == hi`); `assign(l, r, v[, onRemoved])` replaces `[l, r)` by one piece, calling `onRemoved(l', r', old)` for each removed piece (clipped to `[l, r)`) and merging with equal neighbours; `apply(l, r, f)` splits at `l` and `r` and calls `f(l', r', V &)` on every piece inside; `merge(l, r)` coalesces equal adjacent pieces whose boundary lies in `[l, r]`.
- Costs: `get`, `split` `O(log n)`; `assign` `O((k + 1) log n)` with `k` the removed pieces, amortised `O(log n)` because each assignment creates at most three pieces; `apply`, `enumerate`, `fold`, `merge` `O((k + 1) log n)` for the `k` pieces met, with no amortisation: without assignments the piece count never shrinks, so an adversary alternating `apply` on `[lo, hi)` over `n` distinct pieces pays `O(n)` per call. The classical ODT bound (expected `O(n log n)` pieces touched overall) holds only for uniformly random assignment ranges, as on CF 896C; it is not a worst-case guarantee.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `IntervalSet::mex(x)` | `find(x).second` |

### Correctness and cost

`insert` walks the predecessor that reaches `l` and every interval starting at or before `r`, takes the hull, and charges `hull length − previously covered length`, so touching intervals merge with a zero return. `erase` walks the predecessor that strictly overlaps `l` and every interval starting before `r`, re-inserting the clipped remnants `[a, l)` and `[r, b)`. `assign` splits at `r` and then at `l` (map iterators survive insertions, so the order is only the conventional ODT one), erases the range, emplaces the new piece and checks both neighbours. `split` on an existing boundary is a no-op lookup.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/24-interval_set_tester.cpp`, entry `24-interval_set_tester.py` (driver `_00_runner.py`; three builds; 11 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| IntervalSet `insert`, `erase` (return values), `count`, `unionLength`, `intervals`, `find`, `contains`, `covers`, `next`, copy independence | `setSmall<lng>`, `setSmall<int>` | Dense bitmap over `[-20, 60)` with maximal runs recomputed after every step, `find(x).second` also against a forward scan to the first uncovered point; every point in `[-21, 60]`, every `covers` pair, empty and length-1 ranges, ranges reaching outside the bitmap |
| Same at the numeric extremes of `int` and `lng` (intervals starting at `min`, ending at `max`, `max` never covered, `next(max)` sentinel, erase leaving one point at each end) | `setExtremes<int>`, `setExtremes<lng>` | Hand-computed lengths and pieces with `unionLength` at `max / 2 + 7` |
| Same with 1e18 coordinates | `setLarge` | Sorted-merge union of the inserted intervals, clipped erase, `find`/`next`/`covers` at random and boundary points |
| IntervalMap `assign` with callback (lengths and old values) and without (3-argument form, half of the random assigns), neighbour merging on both sides for both forms, `apply` (piece chain covers `[l, r)`), `fold`, `merge` (no equal neighbours remain inside `[l, r]`; minimal piece count after a full merge), `split`, `get`, `enumerate`, `count`; `ChthollyTree` alias; empty universe | `mapTests` | Dense array over universes `[0,0)`, `[0,1)`, `[-5,7)`, `[0,40)`, `[-30,50)`; pieces must partition the universe and be uniform in the array |
| Preconditions | `invalid` | 11 probes: reversed ranges on both structures, reversed universe, keys and ranges outside `[lo, hi)` for `get`, `assign`, `apply`, `enumerate`, `split`, `merge` |

Mutation check (2026-10-08, quick mode, temporary copies): 6 of 9 injected faults detected (insert predecessor test, touching-interval loop bound, missing neighbour merges on both sides of `assign`, unclipped `enumerate`, `find` boundary); the three survivors are equivalent (an extra zero-length pass in `erase`, a redundant `break`, `emplace_hint` on an existing key).

## Commands and results

P027 prune run, 2026-10-13: GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H.

```bash
python3 '96-Local Testing/02-Data Structures/24-interval_set_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/24-interval_set_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/24-interval_set.hpp' '96-Local Testing/02-Data Structures/24-interval_set_tester.cpp'
```

Full, seed 1: PASS in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the 11 assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, all with `-Wall -Wextra -Wconversion -Werror`, 4,070,016 checks per configuration, peak 516 MB on g++ and 521 MB on g++-14. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations. Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

## Sources

| Source | Actual reading and use |
|---|---|
| suisen `range_set.hpp` | `insert`/`erase` returning counts, `number_of_elements`/`number_of_ranges`, `minimum_excluded`, same-interval test (covered by `covers`). |
| Nyaan `segment-set.hpp` | `next`, `lower_bound`-style iteration, covered test. |
| tko919 `rangeunionset.hpp` | `mex(x)`, here `find(x).second`. |
| maspypy `intervals.hpp` | `get`, `enumerate_range` with removal callback, `merge_at`. |
| OI Wiki Chtholly tree (https://oi-wiki.org/misc/odt/) | `split`/`assign` primitives and the random-data complexity caveat. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): dense `IntervalsFast` over a fastset, splay-backed `PiecewiseConstant`, `toArray`, `maxExcluded`, iterator-returning `lowerBound`.
- Dynamic mex over a *set of values* (the `DynamicMex` legacy note) is `IntervalSet::find(0).second` with `insert(x, x + 1)`.
- `IntervalMap` with a monoid-weighted segment tree gives `O(log n)` amortised range-assign/range-composite, the alternative to row 12's `RangeSetRangeComposite`.

## History

- 2026-10-08: P027 first verification.
- 2026-10-08: independent review (P027), 2 findings fixed.
- 2026-10-08: P027 re-audit pass (second session), 1 finding fixed.
- 2026-10-12: re-audit, 1 finding fixed, full suite passed on g++ and g++-14.
- 2026-10-13: prune, removed `IntervalSet::mex` (cases moved to `find(x).second`); full suite passed on g++ and g++-14.
