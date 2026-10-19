# 04-compression.hpp — evidence

`04-compression.hpp` (batch MI02, package P013) provides endpoint collection into `CoordinateCompression`, stable distinct ranks and first-encounter ids. Sorted snapshot ids with bounds, bulk ranks and open/closed point ranges are `CoordinateCompression` operations, with contracts and tests in [07-ordered_set.md](<../../02-Data Structures/Docs/07-ordered_set.md>). Dependencies: P002 (template) and P006 (`CoordinateCompression`), both verified.

## Contracts

### compressEndpoints

`compressEndpoints(vector<pair<T,T>>, comp)` returns the `CoordinateCompression<T,Compare>` of every ordered interval's two endpoints, including duplicates and equal/empty intervals; `Compare` is a strict weak order and each interval satisfies `!comp(r,l)` (asserted). It supports at most `INT_MAX/2` input intervals and performs no endpoint arithmetic, so signed minima/maxima are valid. For a known original half-open interval `[l,r)`, elementary slab indices are `[index(l),index(r))`; slab `i` lies between `decode(i)` and `decode(i+1)`. Compression preserves ordering and equality, **not distance, adjacency or unit spacing**. Recover numeric lengths from original values with sufficiently wide arithmetic: the span from `INT64_MIN` to `INT64_MAX` requires 65 signed bits. `pointRange` does not manufacture an `r+1` endpoint or implement sweep-event tie rules; interval sweeps belong to `09-interval_algorithms.hpp`. Sorting the endpoints of valid intervals needs no fabricated sentinel; collection takes O(m log(m+1)) time and O(m) storage.

### stableRanks

`stableRanks(a, cmp)` returns a permutation `r` of `[0,n)` with `r[i] < r[j]` iff `cmp(a[i],a[j])`, or `a[i]` and `a[j]` are equivalent and `i < j`: comparator-equivalent values get distinct ranks in index order (OI Wiki's second discretization method; maspypy `Index_Compression<SAME=false>`). `n <= INT_MAX` (asserted). It uses `std::stable_sort` over indices: O(n log(n+1)) comparisons, O(n) workspace and output. The input is not modified.

### EncounterCompression

`EncounterCompression<T,Compare>` is a separate append-only dictionary for first-appearance IDs. `add(x)` returns an existing ID or appends the next one; `id(x)` returns `-1` if unseen. New classes never renumber earlier IDs. `value(i)` returns the **first** encountered representative, and its reference may be invalidated by a later append. `clear()` invalidates all IDs/references, restarts at zero and retains vector capacity for reuse. At most `INT_MAX` distinct classes may coexist. Encounter IDs provide no sorted-order or interval-query promise. Existing classes can still be looked up/added at that limit.

Correctness: the map stores the ID assigned at each class's first appearance; lookups cannot change it and a new ID equals the current class count, so IDs are consecutive and the inverse vector stays aligned. Clearing both structures restores the empty invariant. Lookup is O(log(k+1)); insertion amortized O(log(k+1)) with O(k) worst case on reallocation; `clear` O(k); memory O(peak distinct count).

### Legacy mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `Compression<T,Compare>` | `CoordinateCompression<T,Compare>` (`02-Data Structures/07-ordered_set.hpp`); `compressEndpoints` returns it |
| `assign(a)` | `rebuild(a)` |
| `lowerBound(x)`, `upperBound(x)` | `rank(x)`, `upperRank(x)` |
| `id(x)`, `value(i)` | `index(x)`, `decode(i)` |
| `ranks(a)`, `pointRange(l,r,left_closed,right_closed)` | the same members, moved into `CoordinateCompression` |

## Feature-to-test map

| Feature | Verification |
|---|---|
| `compressEndpoints` | Every consecutive pair of every exhaustive and random array as an interval, against a `std::set` of the endpoints; Python sorted endpoint set over full-width signed intervals (extremes, equal endpoints, random and array-derived intervals); empty/repeated/equal intervals, descending and length-equivalence comparators, signed extrema, closed maximum point through `pointRange`, original-distance slab sum through `index`/`decode` in signed 128-bit arithmetic |
| `stableRanks` | C++ counting oracle (`a[j] < a[i]` or equal with `j < i`) on every exhaustive and random array; Python `sorted(range(n), key=(a[i], i))` inverse; equivalence by string length, stateful descending comparator, empty input |
| `EncounterCompression` (`add`, `id`, `value`, `size`, `clear`) | Independent first-appearance linear scan, every prior ID checked after each append, repeated/equivalent keys, first representative, move/reset, alias of stored value, `bool` proxy storage; Python first-appearance scan |
| Invalid inputs | Three assertion probes: negative and excluded `EncounterCompression::value` index, reversed collected interval. Huge-size guards are inspected structurally rather than allocating over `INT_MAX` test elements. Invalid comparators remain caller preconditions. |
| Include orders | `07-ordered_set.hpp` and `04-compression.hpp` included in both orders, compiled and run: `compressEndpoints` equals a direct `CoordinateCompression`, plus `pointRange`, `stableRanks` and encounter IDs |

The removed `Compression` cases now run against `CoordinateCompression` in `07-ordered_set_tester.cpp`: descending and width-equivalence stateful comparators, strings, packed `bool`, composite pairs, 128-bit and int64-extreme keys, full-width random arrays, every probe pair and closure of `pointRange`, bulk `ranks`, rebuild replacement and copy independence, the `range-order` probe, and `decode-end`/`decode-negative` for the inverse-index probes.

Compression quick uses length 0–4 exhaustive arrays over five values, 100 random C++ cases and 105 Python arrays; full uses length 0–6 (19,531 arrays), 1,000 random C++ cases and 1,505 Python arrays; stress uses length 0–7, 10,000 and 15,005. Full makes 1,771,696 C++ checks and 4,515 Python rows per configuration. Random arrays include broad 64-bit values and high duplication.

## Commands and results

P013 prune run, 2026-10-12, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized, checked and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/06-Miscellaneous/_00_runner.py>).

| Command | Result |
|---|---|
| `04-compression_tester.py --mode full --seed 20260927` | PASS, 3 configurations, 1,771,696 checks per configuration, 3 assertion probes, both include orders, `MEMORY peak=546MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=571MB` |
| Other P013 suites (`01`–`03`, `06`, `07`), full, g++ and g++-14 | PASS, 3 configurations each; counts in each evidence document |
| Call-site suites `07-ordered_set` (5 probes, 715,602 checks), `08-sequence_algorithms` (10 probes), `12-enumeration` (28 probes), full, g++ and g++-14 | PASS, 3 configurations each |
| Touched headers and the 98/99 aggregates alone, plus one TU instantiating every changed template, `-Wall -Wextra -Wconversion -Werror`, g++ and g++-14 | No warnings |
| `02-integration.py` | PASS, 135 standalone/aggregate headers, scalar and AVX2 multi-TU builds, workspace, `MEMORY peak=646MB` |
| `03-consistency.py --braces` on the touched headers and testers, and `03-consistency.py` | No errors |

```bash
for c in g++ g++-14; do
  for s in 01-random 02-customhash 03-fastio 04-compression 06-bit_operations 07-permutation 08-sequence_algorithms 12-enumeration; do
    CXX=$c python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927
  done
  CXX=$c python3 '96-Local Testing/02-Data Structures/07-ordered_set_tester.py' --mode full --seed 20260927
done
for c in g++ g++-14; do for h in '02-Data Structures/07-ordered_set' 06-Miscellaneous/01-random 06-Miscellaneous/04-compression 06-Miscellaneous/06-bit_operations 06-Miscellaneous/07-permutation 06-Miscellaneous/08-sequence_algorithms 06-Miscellaneous/12-enumeration 06-Miscellaneous/98-basic 06-Miscellaneous/99-all; do echo "#include \"$h.hpp\"" | $c -std=gnu++20 -Wall -Wextra -Wconversion -Werror -fsyntax-only -I. -x c++ -; done; done
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/07-ordered_set.hpp' 06-Miscellaneous/0[1467]-*.hpp 06-Miscellaneous/08-sequence_algorithms.hpp 06-Miscellaneous/12-enumeration.hpp '96-Local Testing/02-Data Structures/07-ordered_set_tester.cpp' '96-Local Testing/06-Miscellaneous/'0[1467]-*_tester.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Inspected during the 2026-09-27/28 session; code written locally, with no external implementation copied:

- [USACO Guide, *Custom Comparators and Coordinate Compression*](https://usaco.guide/silver/sorting-custom): read coordinate compression, retained inverse coordinates, sort/remove-duplicates/binary-search explanation, and offline range-query endpoint collection in Example 2. Generalized to comparator equivalence, explicit absence, all endpoint closures and a separate first-encounter dictionary.
- [cppreference, `std::lower_bound`](https://en.cppreference.com/w/cpp/algorithm/lower_bound.html): read partitioning requirements, comparator behavior and comparison/iterator complexity. Encounter dictionaries use their map member search rather than linear-iterator generic lower bound.
- Local [Data Structures ordered-set header](<../../02-Data Structures/07-ordered_set.hpp>): the verified `CoordinateCompression` supplies ordering, unique classes, lookup and encoding; `compressEndpoints` builds one.
- Completeness sweep 2026-10-07 (`@researcher`): [OI Wiki discretization](https://oi-wiki.org/misc/discrete/), [maspypy index_compression](https://maspypy.github.io/library/ds/index_compression.hpp) and [to_small_key](https://maspypy.github.io/library/ds/to_small_key.hpp), [ei1333 compress](https://ei1333.github.io/library/other/compress.hpp), [suisen coordinate_compressor](https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp), [Nyaan compress](https://nyaannyaan.github.io/library/misc/compress.hpp), [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp). Bulk `ranks` (ei1333 `get(vector)`, now a `CoordinateCompression` member) and `stableRanks` were adopted.

## Limits and handoffs

Not adopted in the 2026-10-07 sweep, with reasons: incremental `add`/`build` builders (collect then `rebuild`), neighbour queries `minGeq`/`maxLeq` (one `rank`/`upperRank` plus `decode`), a counting-sort build (`11-sorting_selection.hpp` owns `countingSort`), hashed first-encounter ids (the ordered map keeps comparator equivalence and worst-case bounds; use `safe_unordered_map` directly for expected O(1)), exact IEEE-754 bisection (Mathematics `02` owns real search), branchless lower bound (constant factor only), 2D compression (compress each axis).

Interval sweeps belong to `09-interval_algorithms.hpp`. No benchmark: no tuned threshold or reduction backend.

## History

- 2026-09-28: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, bulk `ranks` and `stableRanks` added, 2 findings fixed, contracts moved out of code comments; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: prune, removed `Compression` (`ranks` and `pointRange` moved into `CoordinateCompression`, its cases to `07-ordered_set_tester.cpp`; `compressEndpoints` returns `CoordinateCompression`; `08-sequence_algorithms.hpp` migrated); full suite passed on g++ and g++-14.
