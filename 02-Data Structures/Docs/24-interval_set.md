# 24-interval_set.hpp — evidence

Package P027 (batch DS47). New header; no legacy excerpt. Intervals are half-open over an integer type, preconditions are `assert`s checked once per operation, and both structures are plain `std::map` wrappers that copy and move like maps.

## Contracts

### IntervalSet<T = lng>

A set of integers stored as maximal disjoint half-open intervals `[l, r)`; touching intervals (`r == l'`) are merged, so the stored intervals are exactly the maximal runs of covered points. `T` is a signed integer type (`int`, `lng`); coordinates may be any value of `T`, but every inserted or erased length `r - l` and `unionLength()` must fit `T` (the return values and the running total are kept in `T`; `IntervalSet<int>{}.insert(-2e9, 2e9)` overflows), and `next` uses `numeric_limits<T>::max()` as its "nothing" sentinel, which no interval can cover because `r` is exclusive.

- `count()` number of intervals, `unionLength()` total covered points (maintained, O(1)), `intervals()` the sorted list (O(n)).
- `find(x)` the interval containing `x` or `{x, x}` when uncovered; `contains(x)`; `covers(l, r)` true when one interval contains `[l, r)` (vacuously for `l == r`); `mex(x)` smallest uncovered point `>= x`; `next(x)` smallest covered point `>= x` or the sentinel.
- `insert(l, r)` returns the number of newly covered points; `erase(l, r)` returns the number of removed points. Both are `O((k + 1) log n)` for `k` intervals touched, amortised `O(log n)` because every call creates at most two intervals and each interval is deleted once.

### IntervalMap<K, V> (alias `ChthollyTree<K, V>`)

A piecewise-constant function on `[lo, hi)` stored as `start -> value`; the constructor takes the initial value for the whole universe (an empty universe has no pieces). `V` needs `==` (used by `assign` and `merge` to coalesce equal neighbours).

- `count()` pieces, `get(x)` the piece `{l, r, value}` containing `x`, `enumerate(l, r, visit)` the pieces of `[l, r)` clipped to it (const, no splitting), `fold(l, r, init, step)` accumulates `step(acc, l', r', value)` over the same pieces.
- `split(x)` ensures a boundary at `x` and returns the map iterator of the piece starting there (`end()` for `x == hi`); `assign(l, r, v[, onRemoved])` replaces `[l, r)` by one piece, calling `onRemoved(l', r', old)` for each removed piece (clipped to `[l, r)`) and merging with equal neighbours; `apply(l, r, f)` splits at `l` and `r` and calls `f(l', r', V &)` on every piece inside; `merge(l, r)` coalesces equal adjacent pieces whose boundary lies in `[l, r]`.
- Costs: `get`, `split` `O(log n)`; `assign` `O((k + 1) log n)` with `k` the removed pieces, amortised `O(log n)` because each assignment creates at most three pieces; `apply`, `enumerate`, `fold`, `merge` `O((k + 1) log n)` for the `k` pieces met, with no amortisation: without assignments the piece count never shrinks, so an adversary alternating `apply` on `[lo, hi)` over `n` distinct pieces pays `O(n)` per call. The classical ODT bound (expected `O(n log n)` pieces touched overall) holds only for uniformly random assignment ranges, as on CF 896C; it is not a worst-case guarantee.

## Correctness and cost

`insert` walks the predecessor that reaches `l` and every interval starting at or before `r`, takes the hull, and charges `hull length − previously covered length`, so touching intervals merge with a zero return. `erase` walks the predecessor that strictly overlaps `l` and every interval starting before `r`, re-inserting the clipped remnants `[a, l)` and `[r, b)`. `assign` splits at `r` and then at `l` (map iterators survive insertions, so the order is only the conventional ODT one), erases the range, emplaces the new piece and checks both neighbours. `split` on an existing boundary is a no-op lookup.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/24-interval_set_tester.cpp`, entry `24-interval_set_tester.py` (driver `_00_runner.py`; three builds; 11 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| IntervalSet `insert`, `erase` (return values), `count`, `unionLength`, `intervals`, `find`, `contains`, `covers`, `mex`, `next`, copy independence | `setSmall<lng>`, `setSmall<int>` | Dense bitmap over `[-20, 60)` with maximal runs recomputed after every step; every point in `[-21, 60]`, every `covers` pair, empty and length-1 ranges, ranges reaching outside the bitmap |
| Same at the numeric extremes of `int` and `lng` (intervals starting at `min`, ending at `max`, `max` never covered, `next(max)` sentinel, erase leaving one point at each end) | `setExtremes<int>`, `setExtremes<lng>` | Hand-computed lengths and pieces with `unionLength` at `max / 2 + 7` |
| Same with 1e18 coordinates | `setLarge` | Sorted-merge union of the inserted intervals, clipped erase, `find`/`mex`/`next`/`covers` at random and boundary points |
| IntervalMap `assign` (callback lengths and old values, neighbour merging on both sides), `apply` (piece chain covers `[l, r)`), `fold`, `merge` (no equal neighbours remain inside `[l, r]`; minimal piece count after a full merge), `split`, `get`, `enumerate`, `count`; `ChthollyTree` alias; empty universe | `mapTests` | Dense array over universes `[0,0)`, `[0,1)`, `[-5,7)`, `[0,40)`, `[-30,50)`; pieces must partition the universe and be uniform in the array |
| Preconditions | `invalid` | 11 probes: reversed ranges on both structures, reversed universe, keys and ranges outside `[lo, hi)` for `get`, `assign`, `apply`, `enumerate`, `split`, `merge` |

Mutation check (2026-10-08, quick mode, temporary copies): 6 of 9 injected faults detected (insert predecessor test, touching-interval loop bound, missing neighbour merges on both sides of `assign`, unclipped `enumerate`, `find` boundary); the three survivors are equivalent (an extra zero-length pass in `erase`, a redundant `break`, `emplace_hint` on an existing key).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/24-interval_set_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/24-interval_set_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/24' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-08 (re-audit pass, after its fixes): full mode seed 20261008 passed on both compilers, 4,071,443 checks per configuration and 11 assertion probes; stress mode seed 7 passed all three configurations with 24,391,671 checks per configuration. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, with `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). `02-integration.py --sanitizers` passed 108 standalone/aggregate headers (including this one alone, in `99-all.hpp` and in the two-translation-unit build), the workspace build and the sanitizer self-tests; `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header and its tester. Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

## Sources

| Source | Actual reading and use |
|---|---|
| suisen `range_set.hpp` | `insert`/`erase` returning counts, `number_of_elements`/`number_of_ranges`, `minimum_excluded`, same-interval test (covered by `covers`). |
| Nyaan `segment-set.hpp` | `next`, `lower_bound`-style iteration, covered test. |
| tko919 `rangeunionset.hpp` | `mex(x)`. |
| maspypy `intervals.hpp` | `get`, `enumerate_range` with removal callback, `merge_at`. |
| OI Wiki Chtholly tree (https://oi-wiki.org/misc/odt/) | `split`/`assign` primitives and the random-data complexity caveat. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): dense `IntervalsFast` over a fastset, splay-backed `PiecewiseConstant`, `toArray`, `maxExcluded`, iterator-returning `lowerBound`.
- Dynamic mex over a *set of values* (the `DynamicMex` legacy note) is `IntervalSet::mex(0)` with `insert(x, x + 1)`.
- `IntervalMap` with a monoid-weighted segment tree gives `O(log n)` amortised range-assign/range-composite, the alternative to row 12's `RangeSetRangeComposite`.

## History

- 2026-10-08: P027 first verification.
- 2026-10-08: independent review (P027) — insert/erase counts, touching merges, split and neighbour merging confirmed; added the partial-`merge` effect check (finding 7) and corrected the split-order explanation (finding 9).
- 2026-10-08: P027 re-audit pass (second session): the reviewer found the documented "unbounded coordinates" domain false for lengths (signed overflow in `insert`/`total`); the domain now requires every length and the union length to fit `T` (header domain line and contract), with a new numeric-extremes test.
