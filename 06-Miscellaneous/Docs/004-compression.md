# 004-compression.hpp — Evidence

`004-compression.hpp` (batch MI02, package P013) collects interval endpoints into a `CoordinateCompression`, returns the stable sorting permutation (`argsort`) and its inverse (`stableRanks`), builds dense ids and stable ranks of small-span integer keys by counting, compresses points per axis, and assigns first-encounter ids over an ordered or a hashed dictionary. Sorted snapshot ids with bounds, bulk ranks and open/closed point ranges are `CoordinateCompression` operations, with contracts and tests in [007-ordered_set.md](<../../02-Data Structures/Docs/007-ordered_set.md>). Dependencies, all verified: P002 (template), P006 (`CoordinateCompression`) and MI01 (`002-customhash.hpp`, the default hash of `HashedEncounterCompression`).

## Contracts

### compressEndpoints

`compressEndpoints(vector<pair<T,T>>, comp)` returns the `CoordinateCompression<T,Compare>` of both endpoints of every interval, duplicates and equal or empty intervals included. `Compare` is a strict weak order, and each interval satisfies `!comp(r,l)` (asserted). At most `INT_MAX/2` intervals. It does no endpoint arithmetic, so signed minima and maxima are valid, and needs no sentinel. For a known half-open interval `[l,r)`, the elementary slabs are `[index(l),index(r))`, and slab `i` lies between `decode(i)` and `decode(i+1)`. Compression preserves order and equality, **not distance, adjacency or unit spacing**: recover lengths from the original values in wide enough arithmetic (`INT64_MIN` to `INT64_MAX` needs 65 signed bits). `pointRange` does not create an `r+1` endpoint or apply sweep-event tie rules; interval sweeps belong to `009-interval_algorithms.hpp`. O(m log(m+1)) time, O(m) storage.

### argsort and stableRanks

`argsort(a, cmp)` returns the permutation `ord` of `[0,n)` that sorts `a` by `cmp` stably, equivalent values in index order (maspypy `argsort`). `stableRanks(a, cmp)` is its inverse: `r[i] < r[j]` iff `cmp(a[i],a[j])`, or the two are equivalent and `i < j`, so equivalent values get distinct ranks in index order (OI Wiki's second discretization method; maspypy `Index_Compression<SAME=false>`). Both need `n <= INT_MAX` (asserted) and leave the input unchanged. `argsort` runs `std::stable_sort` over indices and `stableRanks` inverts it in one pass: O(n log(n+1)) comparisons, O(n) workspace and output.

### compressCounting and stableRanksCounting

Both take `vector<lng>` keys whose span `max - min` is below `INT_MAX` (asserted; the span is computed unsigned, so any two `lng` values are valid inputs to the check). `compressCounting(a)` returns `(ids, values)`: `values` are the sorted distinct keys and `ids[i]` is the index of `a[i]` in them, the same ids as `CoordinateCompression<lng>(a).encode(a)`. `stableRanksCounting(a)` returns the same permutation as `stableRanks(a)`. Empty input gives empty results. O(n + V) time and memory with V = max - min + 1: `compression_detail::valueSpan` finds the minimum and span in one pass, then one table over the span is marked and numbered (`compressCounting`), or counted and prefix-summed into starting ranks taken in index order (`stableRanksCounting`). Under `NDEBUG` a span at or above `INT_MAX` returns empty results without allocating. They are kept beside the sorting forms only below the crossover in Benchmarks (user decision 2026-11-17).

### compressPoints

`compressPoints(points, cmp)` compresses the x and y coordinates of `vector<pair<T,T>>` points separately, with the same comparator, and returns `PointCompression{xs, ys, ids}`: the `CoordinateCompression<T,Compare>` of each axis and `ids[i] = (xs.index(x_i), ys.index(y_i))`, inside `[0, xs.size()) x [0, ys.size())`. `n <= INT_MAX` (asserted). O(n log(n+1)) time, O(n) memory. It keeps order and equality per axis, not distances.

### EncounterCompression

`EncounterCompression<T,Compare>` is an append-only dictionary of first-appearance ids. `add(x)` returns the existing id or appends the next one; `id(x)` returns `-1` if unseen. New classes never renumber earlier ids. `value(i)` returns the **first** representative encountered; a later append may invalidate its reference. `clear()` invalidates all ids and references, restarts at zero and keeps vector capacity. At most `INT_MAX` distinct classes; at that limit existing classes can still be looked up and added. Encounter ids promise no sorted order or interval queries.

`HashedEncounterCompression<T,Hash,Map>` has the same members and contract, with `==` and `Hash` in place of the comparator (default `CustomHash`, the splitmix64 hash of `002-customhash.hpp` with its per-process random seed). `Map` maps keys to id + 1; it is constructed as `Map(0, hash)` and used through `operator[]` (which must value-initialize a new mapped `int` to 0), `find`, `end` and `clear`. The default is `unordered_map<T, int, Hash>`; the Data Structures `HashMap<T, int, Hash>` (`02-Data Structures/061-hashmap.hpp`), 1.3 to 2.1 times faster in its own benchmark, can be passed instead (user decision 2026-11-17: a `HashMap` default would make P013 and P227 prerequisites of each other). With either map, `add` takes O(1) expected amortized time and `id` O(1) expected, O(k) worst case, in O(k) memory. The ids do not depend on the hash or seed; `value(i)` keeps the first representative, which matters only when `==` treats unequal objects as equal.

Correctness: the map stores the id assigned at each class's first appearance; lookups never change it and a new id equals the current class count, so ids are consecutive and the inverse vector stays aligned. Clearing both structures restores the empty invariant. The hashed form stores id + 1 >= 1, so the 0 that `operator[]` inserts for an unseen key marks it new, and one lookup serves both the test and the insertion. Ordered form: lookup O(log(k+1)); insertion amortized O(log(k+1)), O(k) worst case on reallocation; `clear` O(k); memory O(peak distinct count).

### Legacy Mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `Compression<T,Compare>` | `CoordinateCompression<T,Compare>` (`02-Data Structures/007-ordered_set.hpp`); `compressEndpoints` returns it |
| `assign(a)` | `rebuild(a)` |
| `lowerBound(x)`, `upperBound(x)` | `rank(x)`, `upperRank(x)` |
| `id(x)`, `value(i)` | `index(x)`, `decode(i)` |
| `ranks(a)`, `pointRange(l,r,left_closed,right_closed)` | the same members, moved into `CoordinateCompression` |

## Feature-to-Test Map

| Feature | Verification |
|---|---|
| `compressEndpoints` | Every consecutive pair of every exhaustive and random array as an interval, against a `std::set` of the endpoints; Python sorted endpoint set over full-width signed intervals (extremes, equal endpoints, random and array-derived intervals); empty, repeated and equal intervals, descending and length-equivalence comparators, signed extrema, the closed maximum point through `pointRange`, the original-distance slab sum through `index`/`decode` in signed 128-bit arithmetic |
| `stableRanks` | C++ counting oracle (`a[j] < a[i]`, or equal with `j < i`) on every exhaustive and random array; Python inverse of `sorted(range(n), key=(a[i], i))`; equivalence by string length, a stateful descending comparator, empty input |
| `argsort` | `argsort[want[i]] == i` against the `stableRanks` brute rank count on every exhaustive and random array; length-equivalence ties, the stateful descending comparator, empty input, `LLONG_MIN`/`LLONG_MAX`; Python `sorted(range(n), key=(a[i], i))` |
| `compressCounting`, `stableRanksCounting` | On every exhaustive and random array of span below 2^16: values against a `std::set`, ids against their position in it, equality with `CoordinateCompression<lng>(a).encode(a)` and its values, ranks against the brute rank count; empty input, one value (span 0), all equal, values next to `LLONG_MIN` and `LLONG_MAX`, span 2^22 + 1; Python sorted-set and stable-sort oracles on arrays of span below 2^20 |
| `compressPoints` | Points `(a[i], ~a[n-1-i])` of every exhaustive and random array against a `std::set` compression of each axis (`xs`, `ys`, `ids`); the stateful descending comparator with `pointRange` and `index` on the returned axes, length equivalence on strings, empty input; Python per-axis sorted sets on `zip(a, a[::-1])` |
| `HashedEncounterCompression` (`add`, `id`, `value`, `size`, `clear`) | On every exhaustive and random `lng` array against an ordered first-encounter oracle: ids, `value`, `size`, -1 for absent probes (-3..3 and both extremes), earlier ids kept, stored values equal to the linear first-encounter list, `clear` and reuse; `string` keys (longer than 8 bytes, adding its own stored value, copy and move), an always-zero hash, a stateful hash passed to the constructor, a seeded `CustomHash(7)` on `pair<int,int>`, `lll` keys, `bool` proxy storage, 5000 keys through rehashing; every check runs with both `unordered_map` and `HashMap<T, int, Hash>` as `Map`; Python first-encounter ids |
| `EncounterCompression` (`add`, `id`, `value`, `size`, `clear`) | Independent first-appearance linear scan, every prior id checked after each append, repeated and equivalent keys, first representative, move and reset, alias of a stored value, `bool` proxy storage; Python first-appearance scan |
| Invalid inputs | Twelve assertion probes: indices -1, `size()` and `size() + 1` for both `value` members, a reversed interval, a span of exactly `INT_MAX` and the full `LLONG_MIN`..`LLONG_MAX` span for each counting build, and `argsort` on `INT_MAX + 1` packed `bool`s (256 MiB). The other size guards are inspected, not tested with over `INT_MAX` elements. Invalid comparators remain caller preconditions. |
| Include orders | `007-ordered_set.hpp` and `004-compression.hpp` in both orders, compiled and run: `compressEndpoints` equals a direct `CoordinateCompression`, plus `pointRange`, `stableRanks`, encounter ids and the five operations added 2026-11-17 |

The removed `Compression` cases run against `CoordinateCompression` in `007-ordered_set_tester.cpp`: descending and width-equivalence stateful comparators, strings, packed `bool`, composite pairs, 128-bit and int64-extreme keys, full-width random arrays, every probe pair and closure of `pointRange`, bulk `ranks`, rebuild and copy independence, the `range-order` probe, and `decode-end`/`decode-negative` for the inverse-index probes.

Modes: quick takes exhaustive arrays of length 0–4 over five values, 100 random C++ cases and about 100 Python arrays (170,253 C++ and 894 Python checks at seed 1). Full takes length 0–6 (19,531 arrays), 1,000 random C++ cases and 1,508 Python arrays; stress takes length 0–7 (97,656), 10,000 and 15,008. Random arrays include broad 64-bit values and heavy duplication, and random lengths make the C++ count seed-dependent. The counting builds are checked on arrays of span below 2^16 (C++) or 2^20 (Python) and on explicit extreme and wide-span cases.

## Commands and Results

P013 MI02 `/finish` runs: Intel Core i9-11900H, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, optimized, checked and ASan-UBSan builds, beside other suites and subagents.

| Date | Command | Result |
|---|---|---|
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/004-compression_tester.py' --mode full --seed 1` | PASS, 3 configurations, 3,453,365 C++ checks over 19,531 exhaustive and 1,000 random arrays (hashed checks with both maps), 1,508 Python oracle arrays (12,468 checks), 12 assertion probes; run optimized 0.95 s, checked 3.7 s, ASan-UBSan 5.6 s; `MEMORY peak=1577MB` (preflight quick 285 MB) |
| 2026-11-17 | same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=1607MB` (preflight quick 1148 MB) |
| 2026-11-17 | `... --mode stress --seed 2` | PASS, 3 configurations, 25,884,354 C++ checks over 97,656 exhaustive and 10,000 random arrays, 15,008 Python oracle arrays (124,053 checks), 12 probes; checked 23.1 s, ASan-UBSan 33.3 s; `MEMORY peak=931MB` |
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/004-compression_benchmark.py'` | Benchmarks |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' ops --filter 06-Miscellaneous/004-compression.hpp` | PASS, 17 operations in 17 extractions on g++ and g++-14 (15 instantiating their default form), 0 failures |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' testers --filter 004-compression_tester` | PASS, quick run on the extracted build (11 names) |
| 2026-11-17 | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 192 standalone and aggregate headers (with the new `059-permutation_orders.hpp` and `060-permutation_problems.hpp`), scalar and available AVX2 multi-TU builds, workspace, sanitizer self-tests, rerun on the final headers; `MEMORY peak=2703MB` |
| 2026-11-17 | `python3 '96-Local Testing/04-mutation.py' run 06-Miscellaneous/004-compression.hpp` | Mutation Testing |

## Benchmarks

`96-Local Testing/06-Miscellaneous/004-compression_benchmark.py`, 2026-11-17, on the final header (load average about 3 from tests on other cores): Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG -falign-functions=64 -falign-loops=32`, seed 1. Input: n `lng` keys uniform in `[-span/2, span/2)` from `mt19937_64`, one key flipped per call so no call repeats, each output folded into a printed checksum. User-mode cycles per call from `perf stat`: median of 5 runs minus the input-only baseline, divided by the calls (3 to 18,832 per run). `encode` is `CoordinateCompression<lng>(a)` plus `encode(a)`, the sorting form of `compressCounting`; `stableRanks` is the sorting form of `stableRanksCounting`.

| n | span | `encode` | `compressCounting` | `stableRanks` | `stableRanksCounting` |
|---|---|---|---|---|---|
| 1000 | 62 | 98,235 | 7,673 | 43,804 | 8,242 |
| 1000 | 1000 | 212,549 | 11,980 | 41,249 | 8,995 |
| 1000 | 16,000 | 210,372 | 65,476 | 38,204 | 34,200 |
| 1000 | 256,000 | 203,375 | 405,777 | 57,341 | 457,148 |
| 100,000 | 6250 | 40.4M | 1.18M | 26.9M | 1.25M |
| 100,000 | 100,000 | 51.5M | 3.31M | 28.9M | 1.92M |
| 100,000 | 1.6M | 52.3M | 9.54M | 29.1M | 5.83M |
| 100,000 | 25.6M | 52.7M | 70.3M | 29.3M | 77.4M |
| 1,000,000 | 62,500 | 525M | 15.2M | 367M | 15.6M |
| 1,000,000 | 1,000,000 | 721M | 46.3M | 386M | 25.9M |
| 1,000,000 | 16M | 755M | 145M | 390M | 110M |
| 1,000,000 | 33.5M | 754M | 205M | 391M | 166M |

Crossover: both counting builds are faster for every span up to 16n (4.6 to 17.7 times at span n, 1.1 to 5.5 times at 16n), still faster at the largest span tried for n = 10^6 (about 33n, a 128 MiB table), and slower at 256n for n = 1000 and n = 100,000. Use them when the span is at most about 16n; between 16n and 256n the result depends on n and the cache size. Instruction counts (in the JSON log) follow the same pattern.

## Mutation Testing

Record: mewt 4.0.0 on blob `7a6a4ac0ca0524f47e21d87f24187a3491122283` (2026-11-17, seed 1, whole file): 351 mutants, 317 caught (1 by timeout), 34 survived, 0 untested.

Stage seconds: quick 712, full on the optimized build 95, full on the checked and ASan-UBSan builds 580. The 34 survivors are equivalent or unreachable:

| Mutant | Line | Why it survives |
|---|---|---|
| `CR 70`, `AOS 169` | 9 | Dropped or loosened (`INT_MAX - 2`) interval-count assertion in `compressEndpoints`: above `INT_MAX / 2` intervals the `2m` endpoints exceed `INT_MAX`, and the `CoordinateCompression` constructor (`rebuild`) asserts the same bound on the same input. |
| `COS 232`, `COS 233` | 9 | `!=` or `<` for `<=`: they differ only at exactly `INT_MAX / 2` intervals (2^30 - 1 pairs, at least 2 GiB plus a sort of 2^31 - 2 endpoints), unreachable within the 4096 MB test cap, as for the Fenwick size assertion (P006, user decision 2026-11-12). |
| `COS 238` | 19 | `<` for `<=` in `argsort`: differs only at exactly `INT_MAX` elements, whose result alone takes 8 GiB. The probe at `INT_MAX + 1` tests the bound itself. |
| `CR 85`, `COS 247`, `COS 248`, `CR 96`, `COS 267`, `COS 268`, `CR 106`, `COS 287`, `COS 288` | 39, 51, 70 | Size assertions of `compressCounting`, `stableRanksCounting` and `compressPoints`: they differ only at `INT_MAX` or more `lng` keys or pairs (16 GiB or more); unreachable. |
| `CR 118`, `COS 297`, `COS 298`, `COS 317`, `COS 318` | 89, 108 | The id-count assertions of both encounter compressions differ only after `INT_MAX` distinct keys; unreachable. |
| `CR 72`, `AOS 172`, `CR 93`, `CR 101`, `CR 108`, `CR 109`, `CR 112` | 10, 47, 55, 71, 74 | Dropped or changed `reserve`: capacity only. |
| `MR 340`, `MR 341`, `MR 342`, `MR 343`, `MR 344` | 14, 49, 73 | Copy instead of `std::move`: the same result, one O(n) copy within the bound. |
| `COS 242` | 27 | `!=` for `<` in a loop counting by one from 0 up to a nonnegative bound. |
| `COS 282`, `COS 283` | 57 | `v != span` or `v < span` in the prefix sum of `stableRanksCounting`: the skipped last step writes `start[span + 1]`, which no later read uses (keys map to `0..span`). |

## Sources

Code written locally; no external implementation copied.

- [USACO Guide, *Custom Comparators and Coordinate Compression*](https://usaco.guide/silver/sorting-custom) (read 2026-09-27/28): coordinate compression with retained inverse coordinates, sort/deduplicate/binary search, and offline range-query endpoint collection (Example 2). Generalized here to comparator equivalence, explicit absence, all endpoint closures and a separate first-encounter dictionary.
- [cppreference, `std::lower_bound`](https://en.cppreference.com/w/cpp/algorithm/lower_bound.html) (read 2026-09-27/28): partitioning requirements, comparator behavior and complexity. Encounter dictionaries search with their map's member functions instead.
- Local [Data Structures ordered-set header](<../../02-Data Structures/007-ordered_set.hpp>): the verified `CoordinateCompression` supplies ordering, unique classes, lookup and encoding; `compressEndpoints` builds one.
- P013 MI02 `/finish` sweep 2026-11-17 (pages in [00-sources.md](00-sources.md), P013 MI02 `/finish` sweep): [maspypy index_compression](https://raw.githubusercontent.com/maspypy/library/main/ds/index_compression.hpp) (`SMALL=true` builds: the counting forms), [to_small_key](https://raw.githubusercontent.com/maspypy/library/main/ds/to_small_key.hpp) (hashed first-encounter ids) and [maspypy my_template](https://raw.githubusercontent.com/maspypy/library/main/my_template.hpp) (`argsort`). No catalog compresses 2D points; `compressPoints` was added by user decision.
- Completeness sweep 2026-10-07 (`@researcher`): [OI Wiki discretization](https://oi-wiki.org/misc/discrete/), [maspypy index_compression](https://maspypy.github.io/library/ds/index_compression.hpp) and [to_small_key](https://maspypy.github.io/library/ds/to_small_key.hpp), [ei1333 compress](https://ei1333.github.io/library/other/compress.hpp), [suisen coordinate_compressor](https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp), [Nyaan compress](https://nyaannyaan.github.io/library/misc/compress.hpp), [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp). Bulk `ranks` (ei1333 `get(vector)`, now a `CoordinateCompression` member) and `stableRanks` were adopted.

## Limits and Handoffs

- Not adopted (user decisions 2026-11-17 in `omissions.md`): incremental `add`/`build` builders (a form of collecting, then `rebuild`), exact IEEE-754 bisection (Mathematics `02` owns real search), a branchless lower bound (an implementation detail of `rank`), and radix argsort (`011-sorting_selection.hpp` `radixSort` with a projection).
- The neighbour queries `minGeq`, `maxLeq`, `minGt` and `maxLt` are `SortedVector` members that `CoordinateCompression` inherits ([007-ordered_set.md](<../../02-Data Structures/Docs/007-ordered_set.md>)).
- Interval sweeps belong to `009-interval_algorithms.hpp`.

## Aliases

Search aliases (P263, grab --find): discretization (OI Wiki: 离散化), coordinate compression, 坐标压缩, 区间端点离散化.

## History

- 2026-09-28: P013 first verification; full and stress passed.
- 2026-10-07: P013 re-audit; bulk `ranks` and `stableRanks` added, 2 findings fixed; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: prune; `Compression` removed (`ranks` and `pointRange` moved into `CoordinateCompression`, cases to `007-ordered_set_tester.cpp`, `008-sequence_algorithms.hpp` migrated); full passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases, then common Chinese names of the row's operations; no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs passed.
- 2026-11-16: P013 MI02 re-audit; completeness sweep found nothing new; full and stress passed on g++ and g++-14.
- 2026-11-17: P013 MI02 `/finish`; `argsort`, `compressCounting`, `stableRanksCounting`, `PointCompression`/`compressPoints` and `HashedEncounterCompression` added (user decisions 2026-11-17), counting crossover benchmarked, review fixes and two past-end probes; full and stress passed on g++ and g++-14; first whole-file mutation campaign, 34 survivors, all equivalent or unreachable.
