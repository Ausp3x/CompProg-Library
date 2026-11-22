# 07-ordered_set.hpp — Evidence

Package P006 (DS01, then DS02). The contracts common to rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SortedVector and CoordinateCompression

An immutable sorted sequence with `n <= INT_MAX`. The comparator is a stable strict weak order; custom and stateful comparators, strings, comparator-equivalent unequal objects and `vector<bool>` are supported. Costs: `size` and `empty` are O(1); every rank and neighbour query is O(log(n + 1)).

- `SortedVector` keeps duplicates in unspecified order inside an equivalence class. `rank(x)` counts the elements before x, `upperRank(x)` those not after x, and `count` is their difference. `index(x)` is the first equivalent rank, or -1 if absent; no `operator==` is needed. `findByOrder(k)` returns an iterator, or `end()` for any out-of-range k. `begin`/`end` iterate the sorted values; iterators and ranks last until `rebuild`.
- `minGeq(x, res)`, `minGt(x, res)`, `maxLeq(x, res)` and `maxLt(x, res)` find the least stored value not before x, the least after x, the greatest not after x and the greatest before x under the comparator. Each returns true and copies that value to `res`, or returns false and leaves `res` unchanged; `res` may alias `x` (suisen `coordinate_compressor` `min_geq`/`max_leq`; added in the P013 MI02 `/finish`, user decision 2026-11-17). They read through `pick(k, res)`, which copies the value of rank k when `0 <= k < size()` and otherwise returns false with `res` unchanged, like `OrderedMultiSetOffline::pick`. `CoordinateCompression` inherits them.
- `CoordinateCompression` deduplicates by comparator equivalence into ranks `[0, size())`. The representative of a class is unspecified, and distances are not preserved. `encode` maps each key to its rank, or -1 if unknown, in O(k * log(n + 1)). `decode(i)` returns the representative of rank i (through `vector<T>::const_reference`) and asserts the range. `ranks(a)` returns `rank(x)` for every query in order: the insertion rank in `[0, size()]`, so an absent value gets the rank of the first stored class after it (unlike `encode`, which returns -1), in O(k * log(n + 1)) with O(k) output.
- `pointRange(l, r, left_closed = true, right_closed = false)` returns the half-open rank interval `[a, b)` of the stored classes inside the original-coordinate interval with the chosen closures. Bounds follow `C`, so descending comparators take descending endpoints; the precondition `!cmp(r, l)` is asserted, and endpoints need not be stored. For equivalent endpoints every closure choice is empty except closed/closed, which holds that class when present; an empty result has `a == b`, never an absent sentinel.

### OrderedSet and OrderedMap

GNU PBDS order-statistics trees with unique keys: `OrderedSet<T, C>` (`null_type` mapped) and `OrderedMap<K, V, C>`. The native `insert`/`erase`/`find`/`lower_bound`/`order_of_key`/`find_by_order`/`join`/`split`/`operator[]` APIs are kept; `find_by_order(k)` for `k >= size()` returns `end()`. Never use `less_equal` as `C`. With stateful comparators, use `_GLIBCXX_ASSERTIONS` for checked builds: GNU PBDS's `_GLIBCXX_DEBUG` equality checker default-constructs a separate comparator.

### OrderedMultiSet

A GNU PBDS tree over `(key, insertion ID)` pairs, so equivalent keys stay in insertion order. IDs in `[1, LLONG_MAX)` are never reused, even after `clear` or `rebuild`. At most `LLONG_MAX - 1` lifetime insertions and `INT_MAX` live elements are allowed (asserted). `C` is a stable strict weak order, and comparator equivalence determines key identity. Public state must not be edited directly.

- `insert` returns its token `(key, ID)`, and `erase(token)` removes exactly that occurrence; an erased token returns false and can safely be retried within the current history. `eraseOne(x)` erases the earliest surviving equivalent occurrence, or returns false.
- `lowerBound`/`upperBound` give the first element `>= x` / `> x`, and `prev(x)` the last element `<= x`; all three return `end()` when absent. Ranks and `findByOrder` follow `SortedVector` (a negative or out-of-range k gives `end()`).
- Copies keep existing tokens, and later histories are local to each copy. Assignment replaces the destination history and invalidates its old tokens. Tokens from unrelated or replaced histories must not be passed to `erase`.
- `clear` is O(n) and `rebuild` O(n + k * log(k + 1)); both invalidate iterators and tokens but keep the ID history. Native PBDS iterator lifetime rules apply.

### OrderedMultiSet::split and OrderedMultiSet::join

`split(x, other)` asserts `&other != this` and calls PBDS `split` at `(x, LLONG_MAX)`: every element whose key compares greater than `x` moves to `other`, whose previous content is discarded (PBDS clears it first, so its old tokens become invalid), and elements with keys `<= x` stay. `other.next_id` becomes `max(other.next_id, next_id)`, so later inserts into either set cannot reuse a moved id. Tokens of moved elements stay valid in `other` and those of kept elements in this set; a stale token of this set's history passed to `other` returns false.

`join(other)` asserts `&other != this` and that the keys of `other` all compare strictly greater, or all strictly less, than this set's keys (either side may be empty). The check is on keys, because PBDS compares `(key, id)` pairs and equal boundary keys would pass or throw `join_error` depending on the ids. All elements move here, `other` is left empty, `next_id` becomes the larger of the two, and live tokens of both sides stay valid. Stale (erased) tokens of either side must not be passed after a join, since the two histories may share ids. `join` is O(log(n + 1)) structure work. Both operations invalidate the iterators of both sets, and the comparators must be equivalent.

Split cost is a recorded limit (user decision 2026-11-12, keep PBDS). PBDS `split` moves the subtrees above the split key one node at a time (`rb_tree_map_/split_join_fn_imps.hpp`); `bin_search_tree_/split_join_fn_imps.hpp` then counts the moved part with `std::distance` in GCC 14.4.1 (line 136) and reads the maintained subtree size in GCC 16.2.1 (line 138); GCC 15.2, which AtCoder and Library Checker run, was not checked. No worst case is proven, since the rotations of each `split_at_node` step can bring nodes from other subtrees onto the walk; the header's O(n * log(n + 1)) is the observed bound, not a guarantee. A caller who needs guaranteed fast splits uses row 19's treap.

Measurement: `96-Local Testing/02-Data Structures/07-ordered_set_benchmark.py` (2026-11-12, i9-11900H, Linux x86-64, `-O2 -DNDEBUG`, seed 20261112, run alone): `OrderedMultiSet<int>` of keys 0..n-1 inserted sorted or shuffled, a fresh tree per run, one `split(fraction * n)` then `join` back, results checked; median of 5 runs; raw log `07-ordered_set_benchmark.json` (git-ignored). n = 2^20:

| Insertion order, moved part | split, GCC 16.2.1 | split, GCC 14.4.1 |
|---|---|---|
| shuffled, 0.1% moved | 6.1 µs | 110 µs |
| shuffled, half moved | 14.7 µs | 54.3 ms |
| shuffled, all but 0.1% moved | 216 µs | 115 ms |
| sorted, 0.1% moved | 7.4 µs | 11.7 µs |
| sorted, half moved | 79.0 ms | 80.5 ms |
| sorted, all but 0.1% moved | 0.99 ms | 12.3 ms |

On shuffled trees GCC 16 is near polylogarithmic and GCC 14 linear in the moved part. After sorted insertion a midpoint split costs about 150 ns per moved element on both (medians 2.77, 14.7, 79.0 ms for n = 2^16, 2^18, 2^20 on GCC 16; 2.79, 15.7, 80.5 ms on GCC 14). `join` stayed at 0.5–2.6 µs on both.

### OrderedMultiSetOffline

An offline ordered multiset (user decision 2026-11-12: row 07 owns it; maspypy `ds/pbds.hpp` only comments that compression plus a BIT is much faster than PBDS, with no code): a `Fenwick<int>` of counts over the `CoordinateCompression` ranks of a universe fixed at construction, with m distinct classes.

- `insert(x)` asserts that x is equivalent to a universe key and that fewer than `INT_MAX` elements are live. `eraseOne(x)` removes one equivalent occurrence or returns false (also for a key outside the universe).
- `count`, `rank` (elements `< x`) and `upperRank` (elements `<= x`) accept any key. `findByOrder(k)` asserts `0 <= k < size()` and returns the universe representative of the k-th element, through `vector<T>::const_reference`.
- `lowerBound`, `upperBound` and `prev` write the first element `>= x`, the first `> x` and the last `<= x` (as `OrderedMultiSet::prev`) to `res` and return true, or return false and leave `res` unchanged; `x` and `res` may alias, since the rank is taken before `res` is written.
- Equivalent occurrences are indistinguishable: there are no tokens and no iteration. `clear` rebuilds the empty Fenwick in O(m); the universe stays. Copies are independent. `pick` is an internal helper, not an operation.
- Build is O(u * log(u + 1)) for a universe of u entries (duplicates included), every other operation O(log(m + 1)), memory O(u).

Correctness: the Fenwick holds the count of each class, so `rank(x)` is the prefix count over the classes below x (`CoordinateCompression::rank`) and `upperRank` the prefix through x's class. Since counts are nonnegative and `0 <= k < size()`, `Fenwick::upperBound(k)` is the first class whose prefix count exceeds k, the class of the k-th element, and its representative is `decode` of it.

### Correctness and Cost

Sorting and binary search preserve order classes; compression preserves order, not coordinate distances. `pointRange` takes the first allowed class (`rank` of a closed left end, `upperRank` of an open one) and the first excluded class (`upperRank` of a closed right end, `rank` of an open one); when both ends exclude the same class, raising the right rank to the left one gives a valid empty range. The static build is O(n log(n+1)), selection O(1), ranks O(log(n+1)). Strict (key, unique-ID) PBDS ordering gives O(log(n+1)) dynamic operations. IDs stay strictly between the rank sentinels and are never recycled after `clear` or `rebuild`, so old tokens cannot erase new occurrences. All storage is O(n).

## Feature-to-Test Map

The Python entry and the C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; the runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`07-ordered_set_tester.cpp` group) | Oracle |
|---|---|---|
| SortedVector `rank`, `upperRank`, `count`, `index`, `findByOrder`, `size`, `empty`, `rebuild`, `begin`..`end` | static group | A `std::multiset` oracle; static 3-symbol vectors through length 6 and stateful, descending, string, bool and int64-extreme keys |
| SortedVector and CoordinateCompression `minGeq`, `minGt`, `maxLeq`, `maxLt` | static group (`neighbours`) | A linear scan for the least or greatest qualifying stored value under the comparator, at every probe of every static case of both structures: the flag, comparator equivalence with the scan's value, the result being a stored element, `res` unchanged on false, and an aliased `minGeq(x, x)`; `pick` called directly at -2, -1, `size()` and `size() + 1`. Covers exhaustive arrays with duplicates, the stateful descending and width comparators, signed extremes, `string`, `bool`, `pair` and `lll` keys, random narrow and full-width trials, a `std::greater<lng>` case with `LLONG_MIN` and `LLONG_MAX` and hand-computed answers, empty structures, and `CoordinateCompression<Box, BoxLess>` (no default constructor) |
| CoordinateCompression `encode`, `decode`, `rebuild`, iteration | static group | A `std::set` oracle; rebuild replacement against a `std::set` of the new values; 2 probes |
| CoordinateCompression `ranks`, `pointRange` | static group | Linear counts over the `std::set` classes: bulk ranks against the per-probe less-than count; for every ordered probe pair and all four closures, `a` equals the classes excluded on the left, `b` equals `max(a, classes admitted on the right)` and `b - a` the classes inside. Exhaustive arrays, stateful descending and width-equivalence comparators, strings, packed `bool`, composite pairs, 128-bit and int64-extreme keys, full-width random arrays, empty query; `range-order` probe |
| OrderedMultiSet `insert`, `erase`, `eraseOne`, `clear`, `rebuild`, `size`, `empty`, `begin`, `end`, `lowerBound`, `upperBound`, `prev`, `rank`, `upperRank`, `count`, `findByOrder` | dynamic-history and random groups | Five-action histories through depth 5 and 12 random histories of 180 operations against a sorted token vector; stale and exact tokens, copy and move, ID exhaustion; `prev` at every rank probe |
| Id limit, `pointRange` defaults | PBDS adapter group | `rebuild` ending exactly at `next_id = LLONG_MAX` and with `next_id = 2^62` succeeds; `pointRange(l, r)` without flags is `[rank(l), rank(r))` |
| OrderedSet, OrderedMap | PBDS adapter group | `std::set`/`std::map` with a stateful comparator |
| OrderedMultiSet `split`, `join` | split/join group | Per-set (key, id) token lists and `next_id`, checked after every step (contents, order, ranks, bounds, `prev`, `findByOrder`, erasing every token): exhaustive two-set histories over keys 0–2 with 22 actions (inserts and erases in both sets, splits both ways at -1..2, joins both ways when the keys are separated), depth 4; descending and width-2 comparators at depth 3; random trials of about 3000 elements (split into a non-empty set, round trips, inserts into both halves then join); a 1500-element sorted-insertion shape; last representable id, signed extremes, string and box keys |
| OrderedMultiSetOffline `ctor (universe)`, `insert`, `eraseOne`, `clear`, `count`, `rank`, `upperRank`, `findByOrder`, `lowerBound`, `upperBound`, `prev`, `size`, `empty` | offline group | The live values in a plain vector, scanned linearly and stably sorted, checked after every step (size, every rank, select against the sorted values and membership in the universe, all three bounds with the unchanged `res` on a miss, the aliased `lowerBound(x, x)`): every universe of up to 3 keys over 0–3 (duplicates included) with ascending and descending comparators, and a width-2 comparator on all of them but four in five 3-key universes; every history of inserts, `eraseOne` of universe keys and of -1 and 4 (outside), and `clear`, through depth 3 (quick 2, stress 4); 20 random trials of 300 steps with random comparators and universes; copy independence, move, empty universe, string, `bool` and signed-extreme keys; 4 probes |

There are 18 assertion probes: `id-exhaustion`, `rebuild-id-exhaustion`, `decode-end`, `decode-past-end` (index size + 1, where only the library's own assert prints glibc's form), `decode-negative`, `range-order`, `split-self`, `join-self`, `join-self-empty`, `join-overlap`, `join-equal-above`, `join-equal-below`, `offline-unknown-insert`, `offline-kth-end`, `offline-kth-negative`, `offline-size-limit` (live count set to `INT_MAX`), and `sorted-max-size` and `multiset-rebuild-max-size`. The last two pass `INT_MAX + 1` packed bools (256 MiB) with a comparator that throws when called, so a build without the size assert fails at once instead of sorting or inserting them. Full and stress mode pass exactly `INT_MAX` packed bools to `SortedVector` and `OrderedMultiSet::rebuild` with that comparator and expect its exception, so the largest admitted size passes both asserts. Erasure is checked by id plus comparator-equivalent key, since with a width comparator a token whose key is only equivalent erases the element. A hand mutation check (2026-10-12, quick mode) failed on `pointRange` with its right closure swapped, without its empty-range normalization, and on `ranks` returning `upperRank`.

Checked builds of this suite use `_GLIBCXX_ASSERTIONS` instead of `_GLIBCXX_DEBUG`: GCC 16 PBDS `debug_map_base` default-constructs its diagnostic comparator, which can disagree with a valid supplied stateful comparator. A reproduced equivalence-width example aborts only in that debug layer; all comparator oracle cases remain in the checked suite. This is an instrumentation limit, not a skipped feature.

## Commands and Results

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 876 MB | 0.1 + 0.1 | 24.0 + 0.2 | 54.6 + 3.0 | 1059 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 911 MB | 0.3 + 0.2 | 31.6 + 0.4 | 64.1 + 2.3 | 1023 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 37.2 + 0.0 | 21.9 + 0.2 | | 864 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 877 MB | 0.1 + 0.4 | 9.9 + 0.5 | 22.9 + 8.1 | 740 MB |

Package runs of 2026-11-17 (P013 MI02 `/finish`, the neighbour queries; experiment none): GCC 16.2.1, GCC 14.4.1 (`CXX=g++-14`), CPython 3.14, Linux x86-64, i9-11900H, beside other suites and subagents, so times are indicative.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 1` | PASS, 3 configurations, 38,521,070 checks per configuration, 18 probes; MEMORY peak 1446 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 1` | PASS, same counts; MEMORY peak 1414 MB |
| `python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode stress --seed 2` | PASS, 178,105,129 checks; ASan-UBSan run 218 s of the 720 s limit; MEMORY peak 950 MB |
| `python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/07-ordered_set.hpp'` | PASS: 52 operations in 40 extractions on g++ and g++-14, 0 failures |
| `python3 '96-Local Testing/05-extraction.py' testers --filter 07-ordered_set_tester` | PASS: 6 names, quick run on the extracted build |
| `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 192 standalone and aggregate headers (the new `59-permutation_orders.hpp` and `60-permutation_problems.hpp` included), scalar and available-AVX2 multi-TU builds, workspace, sanitizer self-tests, rerun on the final headers; MEMORY peak 2703 MB |
| `python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/07-ordered_set.hpp' '96-Local Testing/02-Data Structures/07-ordered_set_tester.cpp'` | no violations |

## Sources

| Source | Actual reading and use |
|---|---|
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 4 | PBDS. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | OrderStatisticTree reference. |
| Installed GCC16 PBDS `tree_policy.hpp` and `order_statistics_imp.hpp` | Strict comparator ordering, `order_of_key` and out-of-range `find_by_order` semantics. |
| [libstdc++ PBDS `split_join_fn_imps.hpp`](https://raw.githubusercontent.com/gcc-mirror/gcc/master/libstdc++-v3/include/ext/pb_ds/detail/bin_search_tree_/split_join_fn_imps.hpp) and the installed GCC 16 `rb_tree_map_/split_join_fn_imps.hpp` | `split` clears `other` and moves the keys above the split key node by node; `join` needs separated ranges: `OrderedMultiSet::split`, `join` and the recorded split limit. |
| [maspypy ds/pbds.hpp](https://maspypy.github.io/library/ds/pbds.hpp) | Offline ordered multiset as a Fenwick over compressed ranks: `OrderedMultiSetOffline`. |
| [suisen coordinate_compressor](https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp) | `min_geq`/`max_leq`: the neighbour queries `minGeq`, `minGt`, `maxLeq`, `maxLt` (fetch record in `06-Miscellaneous/Docs/00-sources.md`). |

References inspected on 2026-09-27; the DS02 rows on 2026-11-12 ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)).

## Mutation Testing

Record: mewt 4.0.0 on blob `f9780322f1a9e1a225caf7b8a1605d4dd338ef37` (2026-11-17, seed 1, 5 lines changed since blob `97ad9780c58e`): 24 mutants, 24 caught (0 by timeout), 0 survived, 0 untested.

Stage seconds: quick 63. Earlier campaigns: the first P013 MI02 `/finish` campaign of 2026-11-17 on the neighbour queries (46 mutants, 45 caught; its survivor `COS 343` was not equivalent, since the then-public helper read past the end, and closed by renaming the helper `pick` and testing it out of range); the DS02 `/finish` campaign of 2026-11-12 on `OrderedMultiSetOffline` (133 mutants, 122 caught, 11 survived, three closed by `offline-size-limit`); and the whole-file campaign of 2026-11-12 (323 mutants, 296 caught, 27 survived, several closed by probes and the largest-size checks). The survivors below come from the last two and sit on lines the later records did not cover; line numbers are the current header's.

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 430`, `MR 431` `std::move(...)` removed in `OrderedMultiSetOffline`'s constructor (DS02 `/finish`) | 139 | equivalent: a copy instead of a move, same values |
| `COS 369` `n == 0` → `<=` (`empty`); `COS 378` `n < INT_MAX` → `!=` (`insert`); `COS 389` `cnt.get(i) == 0` → `<=` (`eraseOne`) (DS02 `/finish`) | 142, 146, 150 | equivalent: `n` and every count are at least 0, and `n` never exceeds `INT_MAX` |
| `COS 403`, `COS 404` (`findByOrder`'s `k < n` → `!=`, `<=`) (DS02 `/finish`) | 157 | untestable (as Fenwick's `LOS 302`, user decision 2026-11-12): for `k >= n` the Fenwick descent returns `m`, and `decode(m)` asserts its index before any read, which the probe `offline-kth-end` cannot tell apart |
| `COS 412` `k >= n` → `==` (`pick`) (DS02 `/finish`) | 162 | equivalent: `pick` receives `rank(x)`, `upperRank(x)` or `upperRank(x) - 1`, all in `[-1, n]` |
| `MR 300`–`308` `std::move(...)` removed (whole file) | 13, 22, 42, 45, 89, 90, 99 | equivalent: a copy instead of a move, same values |
| `COS 222` `i < size()` → `!=` (whole file) | 27 | equivalent: `i = rank(x)` lies in `[0, size()]` |
| `COS 233` `k < size()` → `<=` (whole file) | 28 | equivalent: at `k = size()` both return `v.begin() + size() == v.end()` |
| `AS 157`, `AS 162`, `AS 168`, `AS 171` (arguments of `same`, `max(a, b)`, `max` of the ids swapped) (whole file) | 47, 63, 114, 120 | equivalent: symmetric functions |
| `CR 93`, `CR 97` (`res.reserve` removed in `encode`, `ranks`) (whole file) | 52, 56 | equivalent under the contract (the capacity of a returned vector is unspecified, user decision 2026-11-12, as for `SparseTable::v`): only the capacity changes |
| `COS 252`, `COS 253` (`tree.size() < INT_MAX` → `!=`, `<=`) (whole file) | 98 | untestable (as Fenwick's size asserts, user decision 2026-11-12): only `INT_MAX` live elements in the tree, at least 2 GiB, tell them apart, beyond the memory caps; the asserts stay |
| `COS 257` `next_id < LLONG_MAX` → `!=` (whole file) | 98 | equivalent: `next_id` never exceeds `LLONG_MAX` (an `lng`), so `!=` and `<` agree |

## Limits and Handoffs

Dynamic balanced and persistent ordered trees are rows 19, 20 and 58, not gaps of this row. Distinct-by-position ranks and the counting builds are Miscellaneous row 04 (`stableRanks`, `compressCounting`, `stableRanksCounting`; [00-notes.md](00-notes.md#p006-re-audit-omissions)). `OrderedMultiSet::split` has no proven worst case (recorded limit, user decision 2026-11-12; see Contracts). `OrderedMultiSetOffline` needs its universe up front and offers no tokens or iteration; `OrderedMultiSet` covers both.

## Aliases

Search aliases (P263, grab --find): discretization (OI Wiki: 离散化), coordinate compression, order statistic tree, 坐标压缩, 平板电视, 名次树, 顺序统计树, 有序集合, 查询排名, min_geq, max_leq, successor, predecessor, 前驱, 后继; pbds split, pbds join, 分裂合并; offline ordered multiset, Fenwick order statistics, BIT kth element, 权值树状数组, 树状数组求第 k 小.

## History

- 2026-09-27: original P006 verification; full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 11 findings fixed; CoordinateCompression `decode`, OrderedMultiSet `prev` and the `OrderedMap` alias added; full and stress passed on g++. The independent review confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1 and raised five findings, all fixed.
- 2026-10-12: P013 prune moved `ranks` and `pointRange` into CoordinateCompression with linear-count oracles, the moved `Compression` cases and the `range-order` probe; full passed on g++ and g++-14.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full (seed 20261013) passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases, including common Chinese names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-12: P006 DS02 added `OrderedMultiSet::split` and `join` (PBDS split cost measured on GCC 16 and 14); the independent review passed this header; the first mutation campaign's probes (`decode-past-end`, `join-self-empty`), the `pointRange` default check and the id-limit rebuilds closed seven gaps; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-12: P006 DS02 `/finish` added `OrderedMultiSetOffline` (row 07 owns it, user decision) and recorded the split cost as a limit with the committed benchmark driver and medians (user decision); two max-size probes, the largest-size checks and `offline-size-limit` closed seven earlier survivors and three new ones; changed-line mutation campaign recorded; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-17: P013 MI02 `/finish` added SortedVector `minGeq`, `minGt`, `maxLeq` and `maxLt` (inherited by CoordinateCompression) in this P006 file, with the user's yes; the review's public-helper gap (`COS 343`) closed by renaming the helper `pick` and testing it out of range; full and stress passed on g++ and g++-14; two mutation campaigns on the changed lines.
