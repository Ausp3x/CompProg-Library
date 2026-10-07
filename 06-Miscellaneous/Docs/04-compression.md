# 04-compression.hpp — evidence

`04-compression.hpp` (batch MI02, package P013) provides sorted snapshot ids with bounds, bulk ranks and open/closed point ranges, endpoint collection, stable distinct ranks and first-encounter ids. Dependencies: P002 (template) and P006 (`CoordinateCompression`), both verified.

## Contracts

### Compression

`Compression<T,Compare>` extends the existing verified `CoordinateCompression<T,Compare>` from `02-Data Structures/07-ordered_set.hpp`. It inherits construction, sorted unique storage `v`, comparator `cmp`, `size`, `encode`, `rebuild`, and the rank/select APIs. The new type adds interval/endpoint-facing adapters without a second sorting engine or duplicate storage. The preexisting global `CoordinateCompression` API remains unchanged. Construction/`assign(vector<T>)` sorts values, then removes comparator-equivalent duplicates, where equivalence means neither value is ordered before the other. `T` supports the ordinary copy/move/swap operations used by its containers, and `Compare` is a deterministic strict weak ordering. Default floating-point comparison therefore requires no NaNs. Stateful and descending comparators are supported. Public stored values/comparators must retain these invariants.

`size()` reports the number of distinct equivalence classes. `value(i)` gives an original representative at sorted rank `i`; the representative within a comparator-equivalent class is unspecified. For `vector<bool>` storage it returns a value, as required by that container, rather than a dangling reference. `lowerBound(x)` and `upperBound(x)` return insertion positions in `[0,size()]`. `id(x)` returns the matching rank or `-1`; `encode(vector<T>)` applies that rule to every input, preserving repetitions and mapping unknown values to `-1`.

Sorted rank IDs remain stable throughout a snapshot. `assign` (or inherited `rebuild`) replaces the snapshot and invalidates its IDs/references; ranks can change when another coordinate is inserted. Copies are independent snapshots. Snapshot input lengths are at most `INT_MAX`, so every stored rank and the excluded endpoint `size()` fit `int`. The inherited constructor checks this before converting an iterator difference to `int`; the inherited `encode` supports its vector input size without changing the ID width.

`ranks(a)` returns `lowerBound(x)` for every query `x` in order: the insertion rank in `[0,size()]`, so absent values get the rank of the first stored class after them (unlike `encode`, which returns `-1`). It takes O(k log(n+1)) for `k` queries and returns O(k) storage.

`pointRange(l,r,left_closed=true,right_closed=false)` returns a half-open interval of **stored point ranks** satisfying the selected original-coordinate interval. Bounds follow `Compare`, so descending comparators use descending endpoint order. The precondition is `!cmp(r,l)`. Endpoints need not be stored. For equal/equivalent endpoints, all choices are empty except closed/closed, which contains that equivalence class if present. There is no absent-value sentinel disguised as rank zero.

Correctness: sorting places each strict-weak-order equivalence class contiguously; removing adjacent equivalents leaves one representative per class in strict order. A lower bound finds the first class not before the query, and testing that the query is not before it distinguishes equality from absence. Upper/lower bounds select the first allowed and first excluded point for any endpoint closure; if both endpoints exclude the same class, normalizing the right rank to at least the left gives a valid empty range.

Costs: with `n` input values and `k` classes, construction/rebuild uses O(n log(n+1)) comparisons, O(n) stored capacity and O(log(n+1)) sort stack. Bounds/IDs/point ranges take O(log(k+1)), size/inverse access O(1), encoding `q` values O(q log(k+1)) with O(q) output. These are optimal comparison-based bounds for general coordinates; no radix assumption is imposed. Operation counts assume unit-cost `T` comparisons/copies.

### compressEndpoints

`compressEndpoints(vector<pair<T,T>>, comp)` collects each ordered interval's two endpoints, including duplicates and equal/empty intervals. It supports at most `INT_MAX/2` input intervals and performs no endpoint arithmetic, so signed minima/maxima are valid. For a known original half-open interval `[l,r)`, elementary slab indices are `[id(l),id(r))`; slab `i` lies between `value(i)` and `value(i+1)`. Compression preserves ordering and equality, **not distance, adjacency or unit spacing**. Recover numeric lengths from original values with sufficiently wide arithmetic: the span from `INT64_MIN` to `INT64_MAX` requires 65 signed bits. The point-range closure helper does not manufacture an `r+1` endpoint or implement sweep-event tie rules; interval sweeps belong to `09-interval_algorithms.hpp`. Sorting the endpoints of valid intervals needs no fabricated sentinel; collection takes O(m log(m+1)) time and O(m) storage.

### stableRanks

`stableRanks(a, cmp)` returns a permutation `r` of `[0,n)` with `r[i] < r[j]` iff `cmp(a[i],a[j])`, or `a[i]` and `a[j]` are equivalent and `i < j`: comparator-equivalent values get distinct ranks in index order (OI Wiki's second discretization method; maspypy `Index_Compression<SAME=false>`). `n <= INT_MAX` (asserted). It uses `std::stable_sort` over indices: O(n log(n+1)) comparisons, O(n) workspace and output. The input is not modified.

### EncounterCompression

`EncounterCompression<T,Compare>` is a separate append-only dictionary for first-appearance IDs. `add(x)` returns an existing ID or appends the next one; `id(x)` returns `-1` if unseen. New classes never renumber earlier IDs. `value(i)` returns the **first** encountered representative, and its reference may be invalidated by a later append. `clear()` invalidates all IDs/references, restarts at zero and retains vector capacity for reuse. At most `INT_MAX` distinct classes may coexist. Encounter IDs provide no sorted-order or interval-query promise. Existing classes can still be looked up/added at that limit.

Correctness: the map stores the ID assigned at each class's first appearance; lookups cannot change it and a new ID equals the current class count, so IDs are consecutive and the inverse vector stays aligned. Clearing both structures restores the empty invariant. Lookup is O(log(k+1)); insertion amortized O(log(k+1)) with O(k) worst case on reallocation; `clear` O(k); memory O(peak distinct count).

## Feature-to-test map

| Feature | Verification |
|---|---|
| Sort/unique/rank/inverse/encode | All 19,531 arrays of lengths 0–6 over five values; ordered-set and linear-reference oracles; missing-ID encode fixture; Python exact-integer oracle |
| Bounds and every endpoint closure | Exhaustive small endpoint pairs with all four closure choices; empty/equal/absent endpoints and full signed-width Python cases |
| `ranks` (bulk) | C++ linear count oracle over probes -3..3 with repeats on every exhaustive and random array, empty query; Python `bisect_left` over every point-range query left endpoint (full-width signed values) |
| `stableRanks` | C++ counting oracle (`a[j] < a[i]` or equal with `j < i`) on every exhaustive and random array; Python `sorted(range(n), key=(a[i], i))` inverse; equivalence by string length, stateful descending comparator, empty input |
| Snapshot lifetime | Copy/move and independent rebuild; empty/default construction and replacing all previous values |
| Encounter IDs | Independent first-appearance linear scan, every prior ID checked after each append, repeated/equivalent keys, first representative, move/reset and alias of stored value |
| Generic domains | Stateful descending comparator, equivalence by string length, bool proxy storage, pairs, 128-bit coordinates |
| Offline endpoints | Empty/repeated/equal intervals, descending endpoints, signed extrema, closed maximum point, original-distance slab sum evaluated in signed 128-bit arithmetic |
| Invalid compression inputs | Six assertion probes: negative/excluded inverse index in both types, reversed point interval and reversed collected interval. Huge-size guards are inspected structurally rather than allocating over `INT_MAX` test elements. Invalid comparators remain caller preconditions. |
| Compression/backend coexistence | Existing `CoordinateCompression` and new `Compression` instantiated together; both include orders compiled and run; inherited and adapter queries checked against expected values |

The per-header Python entries run from any working directory and expose quick/full/stress modes plus seed/configuration selection. Compression quick uses length 0–4 exhaustive arrays, 100 random C++ cases and 105 Python arrays; full uses length 0–6, 1,000 random C++ cases and 1,505 Python arrays; stress uses length 0–7, 10,000 and 15,005. Full compression makes 4,868,825 C++ checks and 58,695 Python checks per configuration (stress: 32,073,827 and 585,195). Random arrays include broad 64-bit values and high duplication. Failures carry seed, operation and input context; oracles survive `-DNDEBUG`.

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The compression full run passed all 3 configurations and 6 assertion probes.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Sources

Inspected during the 2026-09-27/28 session; code written locally, with no external implementation copied:

- [USACO Guide, *Custom Comparators and Coordinate Compression*](https://usaco.guide/silver/sorting-custom): read coordinate compression, retained inverse coordinates, sort/remove-duplicates/binary-search explanation, and offline range-query endpoint collection in Example 2. Generalized to comparator equivalence, explicit absence, all endpoint closures and a separate first-encounter dictionary.
- [cppreference, `std::lower_bound`](https://en.cppreference.com/w/cpp/algorithm/lower_bound.html): read partitioning requirements, comparator behavior and comparison/iterator complexity. Snapshot vector iterators give logarithmic lookup; encounter dictionaries use their map member search rather than linear-iterator generic lower bound.
- Local [Data Structures ordered-set header](<../../02-Data Structures/07-ordered_set.hpp>): inspected the existing verified `SortedVector` / `CoordinateCompression` API and reused its ordering, unique-class, lookup and encode engine. The new `Compression` name avoids a global declaration conflict.
- Completeness sweep 2026-10-07 (`@researcher`): [OI Wiki discretization](https://oi-wiki.org/misc/discrete/), [maspypy index_compression](https://maspypy.github.io/library/ds/index_compression.hpp) and [to_small_key](https://maspypy.github.io/library/ds/to_small_key.hpp), [ei1333 compress](https://ei1333.github.io/library/other/compress.hpp), [suisen coordinate_compressor](https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp), [Nyaan compress](https://nyaannyaan.github.io/library/misc/compress.hpp), [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp). Bulk `ranks` (ei1333 `get(vector)`) and `stableRanks` were adopted.

## Limits and handoffs

From the 2026-10-07 sweep; reasons are recorded in [00-notes.md](00-notes.md): incremental `add`/`build` builders (collect then `assign`), neighbour queries `minGeq`/`maxLeq` (one `lowerBound`/`upperBound` plus `value`), a counting-sort build (`11-sorting_selection.hpp` owns `countingSort`), hashed first-encounter ids (the ordered map keeps comparator equivalence and worst-case bounds; use `safe_unordered_map` directly for expected O(1)), exact IEEE-754 bisection (Mathematics `02` owns real search), branchless lower bound (constant factor only), 2D compression (compress each axis).

Interval sweeps belong to `09-interval_algorithms.hpp`. No benchmark: no tuned threshold or reduction backend. No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-28: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, finding 2 (missing bulk `ranks`) and finding 7 (braces, including the tester's embedded C++) fixed, `stableRanks` added, contracts moved out of code comments; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
