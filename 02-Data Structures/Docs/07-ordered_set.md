# 07-ordered_set.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SortedVector and CoordinateCompression

Immutable sorted sequence. The comparator is a stable strict weak order (custom/stateful comparators, strings, comparator-equivalent unequal objects and `vector<bool>` are supported), `n <= INT_MAX`. SortedVector keeps duplicates in unspecified order inside an equivalence class. `rank(x)` counts elements before x and `upperRank(x)` counts those not after x. `count` is their difference. `index(x)` is the first equivalent rank, or -1 if absent; no `operator==` is needed. `findByOrder(k)` returns an iterator, or `end()` for any out-of-range k. `begin`/`end` iterate the sorted values; iterators and ranks last until `rebuild`. CoordinateCompression deduplicates by comparator equivalence into ranks `[0, size())`. The representative of a class is unspecified, and distances are not preserved. `encode` maps each key to its rank, or -1 if unknown, in O(k * log(n + 1)). `decode(i)` returns the representative of rank i (through `vector<T>::const_reference`) and asserts the range. `ranks(a)` returns `rank(x)` for every query in order: the insertion rank in `[0, size()]`, so an absent value gets the rank of the first stored class after it (unlike `encode`, which returns -1), in O(k * log(n + 1)) with O(k) output. `pointRange(l, r, left_closed = true, right_closed = false)` returns the half-open rank interval `[a, b)` of the stored classes inside the original-coordinate interval with the chosen closures, in O(log(n + 1)). Bounds follow `C`, so descending comparators take descending endpoints; the precondition `!cmp(r, l)` is asserted, and endpoints need not be stored. For equivalent endpoints every closure choice is empty except closed/closed, which holds that class when present; an empty result has `a == b`, never an absent sentinel. `size`/`empty` are O(1).

### OrderedSet and OrderedMap

GNU PBDS order-statistics trees with unique keys: `OrderedSet<T, C>` (`null_type` mapped) and `OrderedMap<K, V, C>`. The native `insert`/`erase`/`find`/`lower_bound`/`order_of_key`/`find_by_order`/`join`/`split`/`operator[]` APIs are kept; `find_by_order(k)` for `k >= size()` returns `end()`. Never use `less_equal` as C. With stateful comparators, use `_GLIBCXX_ASSERTIONS` for checked builds: GNU PBDS's `_GLIBCXX_DEBUG` equality checker default-constructs a separate comparator.

### OrderedMultiSet

GNU PBDS tree over `(key, insertion ID)` pairs, so equivalent keys stay in insertion order. IDs in `[1, LLONG_MAX)` are never reused, even after `clear` or `rebuild`. At most `LLONG_MAX - 1` lifetime insertions and `INT_MAX` live elements (asserted). `insert` returns its token `(key, ID)`, and `erase(token)` removes exactly that occurrence; an erased token returns false and can safely be retried within the current history. `eraseOne(x)` erases the earliest surviving equivalent occurrence, or returns false. `lowerBound`/`upperBound` give the first element `>= x` / `> x`; `prev(x)` gives the last element `<= x`. All three return `end()` when absent. Ranks and `findByOrder` follow SortedVector (negative or out-of-range k gives `end()`). Copies keep existing tokens, and later histories are local to each copy. Assignment replaces the destination history and invalidates its old tokens. Tokens from unrelated or replaced histories must not be passed to `erase`. `clear` is O(n) and `rebuild` is O(n + k * log(k + 1)); both invalidate iterators and tokens but keep the ID history. Native PBDS iterator lifetime rules apply. Comparator equivalence determines key identity. Public state must not be edited directly. C is a stable strict weak order.

### OrderedMultiSet::split and OrderedMultiSet::join

`split(x, other)` asserts `&other != this` and calls PBDS `split` at `(x, LLONG_MAX)`: every element whose key compares greater than `x` moves to `other`, whose previous content is discarded (PBDS clears it first; its old tokens become invalid), and elements with keys `<= x` stay. `other.next_id` becomes `max(other.next_id, next_id)`, so later inserts into either set cannot reuse a moved id. Tokens of moved elements stay valid in `other`, those of kept elements in this set, and a stale token of this set's history passed to `other` returns false. `join(other)` asserts `&other != this` and that the keys of `other` all compare strictly greater, or all strictly less, than this set's keys (either side may be empty); the check is on keys, because PBDS compares `(key, id)` pairs and equal boundary keys would pass or throw `join_error` depending on the ids. All elements move here, `other` is left empty, `next_id` becomes the larger of the two, and live tokens of both sides stay valid. Stale (erased) tokens of either side must not be passed after a join, since the two histories may share ids. Both invalidate the iterators of both sets. The comparators must be equivalent. `join` is O(log(n + 1)) structure work.

PBDS `split` moves the subtrees above the split key one node at a time (`rb_tree_map_/split_join_fn_imps.hpp`); `bin_search_tree_/split_join_fn_imps.hpp` then counts the moved part with `std::distance` in GCC 14.4.1 (line 136) and reads the maintained subtree size in GCC 16.2.1 (line 138); GCC 15.2, which AtCoder and Library Checker run, was not checked. Measured with `96-Local Testing/02-Data Structures/07-ordered_set_benchmark.py` (2026-11-12, i9-11900H, Linux x86-64, `-O2 -DNDEBUG`, seed 20261112, alone; `OrderedMultiSet<int>` of keys 0..n-1 inserted sorted or shuffled, a fresh tree per run, one `split(fraction * n)` then `join` back, results checked; median of 5 runs, raw log `07-ordered_set_benchmark.json`, git-ignored), n = 2^20:

| Insertion order, moved part | split, GCC 16.2.1 | split, GCC 14.4.1 |
|---|---|---|
| shuffled, 0.1% moved | 6.1 µs | 110 µs |
| shuffled, half moved | 14.7 µs | 54.3 ms |
| shuffled, all but 0.1% moved | 216 µs | 115 ms |
| sorted, 0.1% moved | 7.4 µs | 11.7 µs |
| sorted, half moved | 79.0 ms | 80.5 ms |
| sorted, all but 0.1% moved | 0.99 ms | 12.3 ms |

On shuffled trees GCC 16 is near polylogarithmic and GCC 14 linear in the moved part; after sorted insertion a midpoint split costs about 150 ns per moved element on both (medians 2.77, 14.7, 79.0 ms for n = 2^16, 2^18, 2^20 on GCC 16, 2.79, 15.7, 80.5 ms on GCC 14). The header states that observed worst case, O(n * log(n + 1)), as the recorded limit below; `join` stayed at 0.5–2.6 µs on both. A caller who needs guaranteed fast splits uses row 19's treap.

Split cost is a recorded limit (user decision 2026-11-12, keep PBDS): no worst case is proven, since the rotations of each `split_at_node` step can bring nodes from other subtrees onto the walk, and the header's O(n * log(n + 1)) is the observed bound, not a guarantee.

### OrderedMultiSetOffline

Offline ordered multiset (user decision 2026-11-12: row 07 owns it; maspypy `ds/pbds.hpp` only comments that compression plus a BIT is much faster than PBDS, with no code): a `Fenwick<int>` of counts over the `CoordinateCompression` ranks of a universe fixed at construction, m distinct classes. `insert(x)` asserts that x is equivalent to a universe key and that fewer than `INT_MAX` elements are live. `eraseOne(x)` removes one equivalent occurrence or returns false (also for a key outside the universe). `count`, `rank` (elements `< x`) and `upperRank` (elements `<= x`) accept any key. `findByOrder(k)` asserts `0 <= k < size()` and returns the universe representative of the k-th element, through `vector<T>::const_reference`. `lowerBound`, `upperBound` and `prev` write the first element `>= x`, the first `> x` and the last `<= x` (as OrderedMultiSet's `prev`) to `res` and return true, or return false and leave `res` unchanged; `x` and `res` may alias, since the rank is taken before `res` is written. Equivalent occurrences are indistinguishable: there are no tokens and no iteration. `clear` rebuilds the empty Fenwick in O(m); the universe stays. Copies are independent. Build O(u * log(u + 1)) for a universe of u entries (duplicates included), every other operation O(log(m + 1)), memory O(u). Correctness: the Fenwick holds the count of each class, so `rank(x)` is the prefix count over the classes below x (`CoordinateCompression::rank`) and `upperRank` the prefix through x's class; since counts are nonnegative and `0 <= k < size()`, `Fenwick::upperBound(k)` is the first class whose prefix count exceeds k, the class of the k-th element, and its representative is `decode` of it. `pick` is an internal helper, not an operation.

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
| Id limit, `pointRange` defaults | PBDS adapter group | `rebuild` ending exactly at `next_id = LLONG_MAX` and with `next_id = 2^62` succeeds; `pointRange(l, r)` without flags is `[rank(l), rank(r))` |
| OrderedSet, OrderedMap | PBDS adapter group | `std::set`/`std::map` with a stateful comparator |
| OrderedMultiSet `split`, `join` | split/join group | Per-set (key, id) token lists and `next_id`, checked after every step (contents, order, ranks, bounds, `prev`, `findByOrder`, erasing every token): exhaustive two-set histories over keys 0–2 with 22 actions (inserts and erases in both sets, splits both ways at -1..2, joins both ways when the keys are separated), depth 4; descending and width-2 comparators at depth 3; random trials of about 3000 elements (split into a non-empty set, round trips, inserts into both halves then join); a 1500-element sorted-insertion shape; last representable id, signed extremes, string and box keys |
| OrderedMultiSetOffline `ctor (universe)`, `insert`, `eraseOne`, `clear`, `count`, `rank`, `upperRank`, `findByOrder`, `lowerBound`, `upperBound`, `prev`, `size`, `empty` | offline group | The live values in a plain vector, scanned linearly and stably sorted, checked after every step (size, every rank, select against the sorted values and membership in the universe, all three bounds with the unchanged `res` on a miss, the aliased `lowerBound(x, x)`): every universe of up to 3 keys over 0–3 (duplicates included) with ascending and descending comparators, and a width-2 comparator on all of them but four in five 3-key universes; every history of inserts, `eraseOne` of universe keys and of -1 and 4 (outside), and `clear`, through depth 3 (quick 2, stress 4); 20 random trials of 300 steps with random comparators and universes; copy independence, move, empty universe, string, `bool` and signed-extreme keys; 4 probes |

18 assertion probes: `id-exhaustion`, `rebuild-id-exhaustion`, `decode-end`, `decode-past-end` (index size + 1, where only the library's own assert prints glibc's form), `decode-negative`, `range-order`, `split-self`, `join-self`, `join-self-empty`, `join-overlap`, `join-equal-above`, `join-equal-below`, `offline-unknown-insert`, `offline-kth-end`, `offline-kth-negative`, `offline-size-limit` (live count set to `INT_MAX`), and `sorted-max-size` and `multiset-rebuild-max-size`, which pass `INT_MAX + 1` packed bools (256 MiB) with a comparator that throws when called, so a build without the size assert fails at once instead of sorting or inserting them. Full and stress mode pass exactly `INT_MAX` packed bools to `SortedVector` and `OrderedMultiSet::rebuild` with that comparator and expect its exception: the largest admitted size passes both asserts. Erasure is checked by id plus comparator-equivalent key, since with a width comparator a token whose key is only equivalent erases the element. Mutation check (2026-10-12, quick mode): `pointRange` with its right closure swapped, without its empty-range normalization, and `ranks` returning `upperRank` each failed; the header was restored. Checked builds of this suite use `_GLIBCXX_ASSERTIONS` instead of `_GLIBCXX_DEBUG`: GCC16 PBDS `debug_map_base` default-constructs its diagnostic comparator, which can disagree with a valid supplied stateful comparator. A reproduced equivalence-width example aborts only in that debug layer; all comparator oracle cases remain in the checked suite. This is an instrumentation limitation, not a skipped algorithm feature.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 876 MB; optimized 0.1 s compile + 0.1 s run, checked 24.0 s compile + 0.2 s run, ASan-UBSan 54.6 s compile + 3.0 s run; MEMORY peak 1059 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 911 MB; optimized 0.3 s compile + 0.2 s run, checked 31.6 s compile + 0.4 s run, ASan-UBSan 64.1 s compile + 2.3 s run; MEMORY peak 1023 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 37.2 s compile + 0.0 s run, checked 21.9 s compile + 0.2 s run; MEMORY peak 864 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 877 MB; optimized 0.1 s compile + 0.4 s run, checked 9.9 s compile + 0.5 s run, ASan-UBSan 22.9 s compile + 8.1 s run; MEMORY peak 740 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (P006 DS02; experiment: none): GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H, one suite at a time after the DS02 `/finish` mutation campaigns, beside other sessions, so times are indicative.

```bash
python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --mode full --filter '02-Data Structures/07-ordered_set.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter 07-ordered_set_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/07-ordered_set.hpp' '96-Local Testing/02-Data Structures/07-ordered_set_tester.cpp'
```

| Command | Result |
|---|---|
| full, g++ | PASS, 3 configurations, 38,642,878 checks per configuration, 18 probes; run optimized 2.1 s, checked 2.3 s, ASan-UBSan 38.4 s; MEMORY peak 1213 MB |
| full, g++-14 | PASS; run 2.5 / 3.2 / 40.5 s; MEMORY peak 1214 MB |
| stress, seed 7 | PASS, 177,577,276 checks; run 10.6 / 13.6 / 248.3 s; MEMORY peak 963 MB |
| extraction ops | PASS: 48 operations in 36 extractions on g++ and g++-14, 0 failures |
| extraction testers | PASS: 6 names, quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multiple-translation-unit builds, workspace; MEMORY peak 1600 MB |
| `03-consistency.py` | No errors |
| `--braces` | No closing-brace, comment-cap or free-function violations (header, tester and `07-ordered_set_benchmark.cpp`) |

## Sources

| Source | Actual reading and use |
|---|---|
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 4 | PBDS. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | OrderStatisticTree reference. |
| Installed GCC16 PBDS `tree_policy.hpp` and `order_statistics_imp.hpp` | Strict comparator ordering, `order_of_key` and out-of-range `find_by_order` semantics. |
| [libstdc++ PBDS `split_join_fn_imps.hpp`](https://raw.githubusercontent.com/gcc-mirror/gcc/master/libstdc++-v3/include/ext/pb_ds/detail/bin_search_tree_/split_join_fn_imps.hpp) and the installed GCC 16 `rb_tree_map_/split_join_fn_imps.hpp` | `split` clears `other` and moves the keys above the split key node by node; `join` needs separated ranges: `OrderedMultiSet::split`, `join` and the recorded split limit. |
| [maspypy ds/pbds.hpp](https://maspypy.github.io/library/ds/pbds.hpp) | Offline ordered multiset as a Fenwick over compressed ranks: `OrderedMultiSetOffline`. |

References inspected on 2026-09-27; the DS02 rows on 2026-11-12 ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)).

## Mutation testing

Record: mewt 4.0.0 on blob `f04d55060a0461d3291f8154c4191012513a960e` (2026-11-12, seed 20261112, 1 lines changed since blob `33e5882689f8`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The DS02 `/finish` campaign on blob `33e5882689f8` covered `OrderedMultiSetOffline` and its include (36 changed lines: 133 mutants, 122 caught, 11 survived; stage seconds quick 300, full on the optimized build 18, full on the checked and ASan-UBSan builds 502); the record's blob only restates its complexity line in the universe length, which yields no mutants. It follows the whole-file campaign on blob `d60653681fc0` (2026-11-12, 323 mutants, 296 caught, 27 survived), whose survivors on unchanged lines stay in the table with that blob's line numbers (one less than today's). The probes `sorted-max-size` and `multiset-rebuild-max-size` closed that campaign's `CR 76`, `COS 217`, `CR 122`, `LOS 212` and `COS 267`, and the largest-size checks its `COS 218` and `COS 268`; the probe `offline-size-limit` closed this campaign's `CR 169`, `LOS 283` and `COS 379`; each was checked against the mutant applied by hand to a scratch copy.

Whole-file campaign stage seconds: first run quick 579, full on the optimized build 40, full on the checked and ASan-UBSan builds 1060 (34 survivors); the survivor retest with the same workdir quick 655, 56, 1661. Between them the probes `decode-past-end` (`i < size()` → `!=` on line 42 let `decode(size() + 1)` read past the end, which only libstdc++'s own bounds check caught, in its `'` form the runner rejects) and `join-self-empty` (removing `assert(&other != this)` on line 108 let an empty set join itself silently), the default `pointRange(l, r)` (`left_closed` / `right_closed` defaults flipped, line 51) and the id-limit rebuilds (`rebuild` ending at `next_id = LLONG_MAX` and with `next_id = 2^62`, which `<=` → `<`, `!=` and `-` → `/` on line 100 rejected) closed 7.

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 430`, `MR 431` `std::move(...)` removed in `OrderedMultiSetOffline`'s constructor | 132 | equivalent: a copy instead of a move, same values |
| `COS 369` `n == 0` → `<=` (`empty`); `COS 389` `cnt.get(i) == 0` → `<=` (`eraseOne`); `COS 378` `n < INT_MAX` → `!=` (`insert`) | 135, 143, 139 | equivalent: `n` and every count are at least 0, and `n` never exceeds `INT_MAX` |
| `COS 412` `k >= n` → `==` (`pick`) | 155 | equivalent: `pick` receives `rank(x)`, `upperRank(x)` or `upperRank(x) - 1`, all in `[-1, n]` |
| `COS 403`, `COS 404` (`findByOrder`'s `k < n` → `!=`, `<=`) | 150 | untestable (as Fenwick's `LOS 302`, user decision 2026-11-12): for `k >= n` the Fenwick descent returns `m`, and `decode(m)` asserts its index before any read, which the probe `offline-kth-end` cannot tell apart |
| `MR 300`–`308` `std::move(...)` removed | 12, 21, 34, 37, 81, 82, 91 | equivalent: a copy instead of a move, same values |
| `COS 252`, `COS 253` (`tree.size() < INT_MAX` → `!=`, `<=`) | 90 | untestable (as Fenwick's size asserts, user decision 2026-11-12): only `INT_MAX` live elements in the tree, at least 2 GiB, tell them apart, beyond the memory caps; the asserts stay |
| `COS 222` `i < size()` → `!=` | 26 | equivalent: `i = rank(x)` lies in `[0, size()]` |
| `COS 233` `k < size()` → `<=` | 27 | equivalent: at `k = size()` both return `v.begin() + size() == v.end()` |
| `AS 157`, `AS 162`, `AS 168`, `AS 171` (arguments of `same`, `max(a, b)`, `max` of the ids swapped) | 39, 55, 106, 112 | equivalent: symmetric functions |
| `COS 257` `next_id < LLONG_MAX` → `!=` | 90 | equivalent: `next_id` never exceeds `LLONG_MAX` (an `lng`), so `!=` and `<` agree |
| `CR 93`, `CR 97` (`res.reserve` removed in `encode`, `ranks`) | 44, 48 | equivalent under the contract (the capacity of a returned vector is unspecified, user decision 2026-11-12, as `SparseTable::v`'s): only the capacity changes |

## Limits and handoffs

Dynamic balanced and persistent ordered trees are rows 19, 20 and 58, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): CoordinateCompression distinct-by-position encoding and counting-sort mode. `OrderedMultiSet::split` has no proven worst case (recorded limit, user decision 2026-11-12; Contracts, OrderedMultiSet::split and OrderedMultiSet::join). `OrderedMultiSetOffline` needs its universe up front and offers no tokens or iteration; `OrderedMultiSet` covers both.

## Aliases

Search aliases (P263, grab --find): discretization (OI Wiki: 离散化), coordinate compression, order statistic tree, 坐标压缩, 平板电视, 名次树, 顺序统计树, 有序集合, 查询排名; pbds split, pbds join, 分裂合并; offline ordered multiset, Fenwick order statistics, BIT kth element, 权值树状数组, 树状数组求第 k 小.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 11 findings fixed, CoordinateCompression `decode`, OrderedMultiSet `prev` and the `OrderedMap` alias added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-12: P013 prune moved `ranks` and `pointRange` into CoordinateCompression with linear-count oracles, the moved `Compression` cases and the `range-order` probe; full suite passed on g++ and g++-14.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS02, added `OrderedMultiSet::split` and `join` (PBDS split cost measured on GCC 16 and 14); independent review passed this header; first mutation campaign, two probes and the id-limit and default-bound checks closed seven gaps; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-12: P006 DS02 `/finish`, added `OrderedMultiSetOffline` (row 07 owns it, user decision), the split cost recorded as a limit with the committed benchmark driver and medians (user decision); two max-size probes, the largest-size checks and `offline-size-limit` closed seven earlier survivors and three new ones; changed-line mutation campaign; full on g++ and g++-14, stress, extraction, integration and consistency passed.
