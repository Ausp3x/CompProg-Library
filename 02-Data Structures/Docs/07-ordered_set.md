# 07-ordered_set.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SortedVector and CoordinateCompression

Immutable sorted sequence. The comparator is a stable strict weak order (custom/stateful comparators, strings, comparator-equivalent unequal objects and `vector<bool>` are supported), `n <= INT_MAX`. SortedVector keeps duplicates in unspecified order inside an equivalence class. `rank(x)` counts elements before x and `upperRank(x)` counts those not after x. `count` is their difference. `index(x)` is the first equivalent rank, or -1 if absent; no `operator==` is needed. `findByOrder(k)` returns an iterator, or `end()` for any out-of-range k. `begin`/`end` iterate the sorted values; iterators and ranks last until `rebuild`. CoordinateCompression deduplicates by comparator equivalence into ranks `[0, size())`. The representative of a class is unspecified, and distances are not preserved. `encode` maps each key to its rank, or -1 if unknown, in O(k * log(n + 1)). `decode(i)` returns the representative of rank i (through `vector<T>::const_reference`) and asserts the range. `ranks(a)` returns `rank(x)` for every query in order: the insertion rank in `[0, size()]`, so an absent value gets the rank of the first stored class after it (unlike `encode`, which returns -1), in O(k * log(n + 1)) with O(k) output. `pointRange(l, r, left_closed = true, right_closed = false)` returns the half-open rank interval `[a, b)` of the stored classes inside the original-coordinate interval with the chosen closures, in O(log(n + 1)). Bounds follow `C`, so descending comparators take descending endpoints; the precondition `!cmp(r, l)` is asserted, and endpoints need not be stored. For equivalent endpoints every closure choice is empty except closed/closed, which holds that class when present; an empty result has `a == b`, never an absent sentinel. `size`/`empty` are O(1).

### OrderedSet and OrderedMap

GNU PBDS order-statistics trees with unique keys: `OrderedSet<T, C>` (`null_type` mapped) and `OrderedMap<K, V, C>`. The native `insert`/`erase`/`find`/`lower_bound`/`order_of_key`/`find_by_order`/`join`/`split`/`operator[]` APIs are kept; `find_by_order(k)` for `k >= size()` returns `end()`. Never use `less_equal` as C. With stateful comparators, use `_GLIBCXX_ASSERTIONS` for checked builds: GNU PBDS's `_GLIBCXX_DEBUG` equality checker default-constructs a separate comparator.

### OrderedMultiSet

GNU PBDS tree over `(key, insertion ID)` pairs, so equivalent keys stay in insertion order. IDs in `[1, LLONG_MAX)` are never reused, even after `clear` or `rebuild`. At most `LLONG_MAX - 1` lifetime insertions and `INT_MAX` live elements (asserted). `insert` returns its token `(key, ID)`, and `erase(token)` removes exactly that occurrence; an erased token returns false and can safely be retried within the current history. `eraseOne(x)` erases the earliest surviving equivalent occurrence, or returns false. `lowerBound`/`upperBound` give the first element `>= x` / `> x`; `prev(x)` gives the last element `<= x`. All three return `end()` when absent. Ranks and `findByOrder` follow SortedVector (negative or out-of-range k gives `end()`). Copies keep existing tokens, and later histories are local to each copy. Assignment replaces the destination history and invalidates its old tokens. Tokens from unrelated or replaced histories must not be passed to `erase`. `clear` is O(n) and `rebuild` is O(n + k * log(k + 1)); both invalidate iterators and tokens but keep the ID history. Native PBDS iterator lifetime rules apply. Comparator equivalence determines key identity. Public state must not be edited directly. C is a stable strict weak order.

### Correctness and cost

Sort and binary search preserve order classes; compression preserves order, not coordinate distances. `pointRange` takes the first allowed class (`rank` of a closed left end, `upperRank` of an open one) and the first excluded class (`upperRank` of a closed right end, `rank` of an open one); when both ends exclude the same class, raising the right rank to the left one gives a valid empty range. Static build O(n log(n+1)), selection O(1), ranks O(log(n+1)). Strict (key, unique-ID) PBDS ordering gives O(log(n+1)) dynamic operations. IDs stay strictly between rank sentinels; they are never recycled after clear/rebuild, so old tokens cannot erase new occurrences. All storage O(n).

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`07-ordered_set_tester.cpp` group) | Oracle |
|---|---|---|
| SortedVector `rank`, `upperRank`, `count`, `index`, `findByOrder`, `size`, `empty`, `rebuild`, `begin`..`end` | static group | `std::multiset` oracle; static 3-symbol vectors through length 6 and stateful/descending/string/bool/int64-extreme keys |
| CoordinateCompression `encode`, `decode`, `rebuild`, iteration | static group | `std::set` oracle; rebuild replacement against a `std::set` of the new values; 2 probes |
| CoordinateCompression `ranks`, `pointRange` | static group | Linear counts over the `std::set` classes: bulk ranks against the per-probe less-than count; for every ordered probe pair and all four closures, `a` equals the classes excluded on the left, `b` equals `max(a, classes admitted on the right)` and `b - a` the classes inside. Exhaustive arrays, stateful descending/width-equivalence comparators, strings, packed `bool`, composite pairs, 128-bit and int64-extreme keys, full-width random arrays, empty query; `range-order` probe |
| OrderedMultiSet `insert`, `erase`, `eraseOne`, `clear`, `rebuild`, `size`, `empty`, `begin`, `end`, `lowerBound`, `upperBound`, `prev`, `rank`, `upperRank`, `count`, `findByOrder` | dynamic-history and random groups | Five-action histories through depth 5 and 12 random histories of 180 operations against a sorted token vector; stale/exact tokens, copy/move, ID exhaustion; `prev` at every rank probe |
| OrderedSet, OrderedMap | PBDS adapter group | `std::set`/`std::map` with a stateful comparator |

5 assertion probes: `id-exhaustion`, `rebuild-id-exhaustion`, `decode-end`, `decode-negative` and `range-order`. Mutation check (2026-10-12, quick mode): `pointRange` with its right closure swapped, without its empty-range normalization, and `ranks` returning `upperRank` each failed; the header was restored. Checked builds of this suite use `_GLIBCXX_ASSERTIONS` instead of `_GLIBCXX_DEBUG`: GCC16 PBDS `debug_map_base` default-constructs its diagnostic comparator, which can disagree with a valid supplied stateful comparator. A reproduced equivalence-width example aborts only in that debug layer; all comparator oracle cases remain in the checked suite. This is an instrumentation limitation, not a skipped algorithm feature.

## Commands and results

P006 prune rerun, 2026-10-13: GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64 (Intel Core i9-11900H).

```bash
python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Full, seed 20261013: PASS on g++ and g++-14 in the runner's optimized, checked (`_GLIBCXX_ASSERTIONS`, see above) and ASan/UBSan builds (header and tests unchanged since the 2026-10-12 P013 prune); check counts, probe counts and MEMORY peaks not recorded. Integration and consistency passed with the package run recorded in [02-fenwick.md](02-fenwick.md#commands-and-results).

## Sources

| Source | Actual reading and use |
|---|---|
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 4 | PBDS. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | OrderStatisticTree reference. |
| Installed GCC16 PBDS `tree_policy.hpp` and `order_statistics_imp.hpp` | Strict comparator ordering, `order_of_key` and out-of-range `find_by_order` semantics. |

References inspected on 2026-09-27.

## Limits and handoffs

Dynamic balanced and persistent ordered trees are rows 19, 20 and 58, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): OrderedMultiSet `split`/`join` (`OrderedSet` keeps the native PBDS API, including both), CoordinateCompression distinct-by-position encoding and counting-sort mode.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 11 findings fixed, CoordinateCompression `decode`, OrderedMultiSet `prev` and the `OrderedMap` alias added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-12: P013 prune moved `ranks` and `pointRange` into CoordinateCompression with linear-count oracles, the moved `Compression` cases and the `range-order` probe; full suite passed on g++ and g++-14.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
