# Compression and binary-search facade — MI02 contracts and verification

First verified 2026-09-28 for P013. Re-audited 2026-10-07: `Compression::ranks` (bulk lower-bound ranks, `/reaudit-review` finding 2) and `stableRanks` were added, header contracts moved here, and the facade row now names `fibSearch` and `expSearch`, which the facade always exported. `05-binarysearch.hpp` exposes the canonical Mathematics search header without copying its engine.

## Contracts

### Compression

`Compression<T,Compare>` extends the existing verified `CoordinateCompression<T,Compare>` from `02-Data Structures/07-ordered_set.hpp`. It inherits construction, sorted unique storage `v`, comparator `cmp`, `size`, `encode`, `rebuild`, and the rank/select APIs. The new type adds interval/endpoint-facing adapters without a second sorting engine or duplicate storage. The preexisting global `CoordinateCompression` API remains unchanged. Construction/`assign(vector<T>)` sorts values, then removes comparator-equivalent duplicates, where equivalence means neither value is ordered before the other. `T` supports the ordinary copy/move/swap operations used by its containers, and `Compare` is a deterministic strict weak ordering. Default floating-point comparison therefore requires no NaNs. Stateful and descending comparators are supported. Public stored values/comparators must retain these invariants.

`size()` reports the number of distinct equivalence classes. `value(i)` gives an original representative at sorted rank `i`; the representative within a comparator-equivalent class is unspecified. For `vector<bool>` storage it returns a value, as required by that container, rather than a dangling reference. `lowerBound(x)` and `upperBound(x)` return insertion positions in `[0,size()]`. `id(x)` returns the matching rank or `-1`; `encode(vector<T>)` applies that rule to every input, preserving repetitions and mapping unknown values to `-1`.

Sorted rank IDs remain stable throughout a snapshot. `assign` (or inherited `rebuild`) replaces the snapshot and invalidates its IDs/references; ranks can change when another coordinate is inserted. Copies are independent snapshots. Snapshot input lengths are at most `INT_MAX`, so every stored rank and the excluded endpoint `size()` fit `int`. The inherited constructor checks this before converting an iterator difference to `int`; the inherited `encode` supports its vector input size without changing the ID width.

`ranks(a)` returns `lowerBound(x)` for every query `x` in order: the insertion rank in `[0,size()]`, so absent values get the rank of the first stored class after them (unlike `encode`, which returns `-1`). It takes O(k log(n+1)) for `k` queries and returns O(k) storage.

`pointRange(l,r,left_closed=true,right_closed=false)` returns a half-open interval of **stored point ranks** satisfying the selected original-coordinate interval. Bounds follow `Compare`, so descending comparators use descending endpoint order. The precondition is `!cmp(r,l)`. Endpoints need not be stored. For equal/equivalent endpoints, all choices are empty except closed/closed, which contains that equivalence class if present. There is no absent-value sentinel disguised as rank zero.

### compressEndpoints

`compressEndpoints(vector<pair<T,T>>, comp)` collects each ordered interval's two endpoints, including duplicates and equal/empty intervals. It supports at most `INT_MAX/2` input intervals and performs no endpoint arithmetic, so signed minima/maxima are valid. For a known original half-open interval `[l,r)`, elementary slab indices are `[id(l),id(r))`; slab `i` lies between `value(i)` and `value(i+1)`. Compression preserves ordering and equality, **not distance, adjacency or unit spacing**. Recover numeric lengths from original values with sufficiently wide arithmetic: the span from `INT64_MIN` to `INT64_MAX` requires 65 signed bits. The point-range closure helper does not manufacture an `r+1` endpoint or implement sweep-event tie rules; interval sweeps belong to `09-interval_algorithms.hpp`.

### stableRanks

`stableRanks(a, cmp)` returns a permutation `r` of `[0,n)` with `r[i] < r[j]` iff `cmp(a[i],a[j])`, or `a[i]` and `a[j]` are equivalent and `i < j`: comparator-equivalent values get distinct ranks in index order (OI Wiki's second discretization method; maspypy `Index_Compression<SAME=false>`). `n <= INT_MAX` (asserted). It uses `std::stable_sort` over indices: O(n log(n+1)) comparisons, O(n) workspace and output. The input is not modified.

### EncounterCompression

`EncounterCompression<T,Compare>` is a separate append-only dictionary for first-appearance IDs. `add(x)` returns an existing ID or appends the next one; `id(x)` returns `-1` if unseen. New classes never renumber earlier IDs. `value(i)` returns the **first** encountered representative, and its reference may be invalidated by a later append. `clear()` invalidates all IDs/references, restarts at zero and retains vector capacity for reuse. At most `INT_MAX` distinct classes may coexist. Encounter IDs provide no sorted-order or interval-query promise. Existing classes can still be looked up/added at that limit.

### Binary search facade

The inventory explicitly assigns Mathematics as the canonical owner of shared search. Consequently the Miscellaneous target is an intentional direct-include facade, despite the default preference against redundant alias-only wrappers. Its direct dependency is `../05-Mathematics/02-search_algorithms.hpp`, whose complete API/proofs are in [91-search.md](../05-Mathematics/91-search.md).

- `firstTrue` and `lastTrue` search `[l,r)` with their respective monotone predicate direction and return `{found,position}`. Absence is `{false,r}`, including an empty range. All representable signed 64-bit endpoints are accepted, but an excluded endpoint beyond `INT64_MAX` is not representable.
- `binSearch(ok,ng,f)` preserves known true/false endpoints in either order, never evaluates those endpoints, and uses overflow-safe `std::midpoint`. Equal endpoints are already converged. It supports a known-true maximum signed endpoint without adding one to it.
- `binSearchRealBracket` accepts finite binary64 endpoints and nonnegative finite tolerances/iteration caps. It reports the remaining sorted bracket, feasible endpoint, iteration count and actual convergence. Width tolerance or adjacent representable endpoints establishes convergence; merely exhausting an iteration cap does not. Predicate evaluations must preserve the claimed monotonicity.
- The canonical header also exposes `ternSearch`, `fibSearch` (Fibonacci-section leftmost minimum), `expSearch` (upward galloping, unbounded), the real ternary/golden bracket methods and point wrappers. The facade introduces no alternate versions, names or stopping rules; all thirteen names in the inventory row resolve to the canonical definitions.

Integer searches take O(log(n+1)) predicate work with O(1) memory and at most 64 bisection predicate evaluations over the full signed domain. Real bisection takes O(iteration cap) work and O(1) memory. Objective/predicate cost multiplies those counts. No canonical Mathematics implementation was changed by this task.

## Correctness and costs

Sorting places each strict-weak-order equivalence class contiguously. Removing adjacent equivalents leaves exactly one representative per class in strict sorted order. A lower bound finds the first class not before the query; testing that the query is not before this representative distinguishes equality from absence. Upper/lower bounds select the first allowed point and first excluded point for any choice of endpoint closure. If both endpoints exclude the same class, normalizing the right rank to at least the left yields a valid empty range. Sorting the two endpoints of every valid interval needs no fabricated sentinel outside the coordinate type.

The encounter map stores the ID assigned when each equivalence class first appears. Lookup of an existing class cannot change it; a new ID equals the current number of classes, so IDs are consecutive and inverse-vector entries stay aligned. Clearing both structures restores the empty invariant.

With `n` input values and `k` distinct classes, snapshot construction/rebuild uses O(n log(n+1)) comparisons and O(n) stored capacity; the sort's transient stack is O(log(n+1)). Bounds/IDs/point ranges take O(log(k+1)), size/inverse access O(1), and encoding `q` values O(q log(k+1)) with O(q) returned storage. Endpoint collection takes O(m log(m+1)) time and O(m) storage. These are optimal comparison-based general-coordinate bounds; no integer-specific radix assumption is imposed.

Encounter lookup takes O(log(k+1)); insertion takes amortized O(log(k+1)), with O(k) worst-case work when its inverse vector reallocates. Clearing costs O(k). Stored memory is O(peak distinct count), including the retained vector capacity. Operation counts assume unit-cost `T` comparisons/copies; strings or other expensive types contribute their own costs. No specialization or threshold requires a performance benchmark, and no universal throughput claim is made.

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
| Actual search facade availability | Compile-time uses of first/last-true, integer bracket and real bracket **before** including shared canonical test source; an empty/broken facade fails to compile |
| Search behavior and edge cases | Reused canonical exhaustive monotone arrays, brute-force minima, full signed endpoints, real extremes/subnormals/adjacency, analytic roots/minima, iteration/tolerance/call counts, every exported API, 15 assertion probes |

The per-header Python entries run from any working directory and expose quick/full/stress modes plus seed/configuration selection. Compression quick uses length 0–4 exhaustive arrays, 100 random C++ cases and 105 Python arrays; full uses length 0–6, 1,000 random C++ cases and 1,505 Python arrays; stress uses length 0–7, 10,000 and 15,005. Full compression makes 4,868,825 C++ checks and 58,695 Python checks per configuration (stress: 32,073,827 and 585,195). Random arrays include broad 64-bit values and high duplication. Failures carry seed, operation and input context; oracles survive `-DNDEBUG`.

Search modes reuse the established canonical suite: quick checks minimum arrays through length 6 and 100 random cases, full through length 8 and 3,000 random cases, stress through length 9 and 30,000. Full performs 2,375,977 checks per configuration after the P012 re-audit enlarged the canonical suite (stress 22,081,188). Output from the shared C++ suite intentionally names `02-search_algorithms`; the Python entry names the tested `05-binarysearch` facade. This reuse avoids copying both engine and test logic. The package integration separately checks Basic/All and cross-translation-unit inclusion.

## Commands and results

GCC 16.2.1, GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H, 2026-10-07:

```sh
python3 '96-Local Testing/06-Miscellaneous/04-compression_tester.py' --mode full --seed 20260927                               # PASS, 3 configurations, 6 probes
python3 '96-Local Testing/06-Miscellaneous/05-binarysearch_tester.py' --mode full --seed 20260927                              # PASS, 3 configurations, 15 probes
python3 '96-Local Testing/06-Miscellaneous/04-compression_tester.py' --mode stress --seed 20260928 --configuration optimized   # PASS
python3 '96-Local Testing/06-Miscellaneous/05-binarysearch_tester.py' --mode stress --seed 20260928 --configuration optimized  # PASS
```

Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan with leak detection. No benchmark is required: there is no tuned threshold or reduction backend. No online submission was made.

## Omitted candidates

From the 2026-10-07 sweep; reasons are recorded in [80-notes.md](80-notes.md): incremental `add`/`build` builders (collect then `assign`), neighbour queries `minGeq`/`maxLeq` (one `lowerBound`/`upperBound` plus `value`), a counting-sort build (`11-sorting_selection.hpp` owns `countingSort`), hashed first-encounter ids (the ordered map keeps comparator equivalence and worst-case bounds; use `safe_unordered_map` directly for expected O(1)), exact IEEE-754 bisection (Mathematics `02` owns real search), branchless lower bound (constant factor only), 2D compression (compress each axis).

## Sources

Inspected during the 2026-09-27/28 session; code written locally, with no external implementation copied:

- [USACO Guide, *Custom Comparators and Coordinate Compression*](https://usaco.guide/silver/sorting-custom): read coordinate compression, retained inverse coordinates, sort/remove-duplicates/binary-search explanation, and offline range-query endpoint collection in Example 2. Generalized to comparator equivalence, explicit absence, all endpoint closures and a separate first-encounter dictionary.
- [cppreference, `std::lower_bound`](https://en.cppreference.com/w/cpp/algorithm/lower_bound.html): read partitioning requirements, comparator behavior and comparison/iterator complexity. Snapshot vector iterators give logarithmic lookup; encounter dictionaries use their map member search rather than linear-iterator generic lower bound.
- [cp-algorithms, *Binary Search*](https://cp-algorithms.com/num_methods/binary_search.html): read the monotone-predicate invariant, midpoint-overflow discussion, absent transitions and continuous-search section. Compared these with the existing canonical implementation and its stronger finite-width/status contracts.
- Local [Data Structures ordered-set header](../02-Data%20Structures/07-ordered_set.hpp): inspected the existing verified `SortedVector` / `CoordinateCompression` API and reused its ordering, unique-class, lookup and encode engine. The new `Compression` name avoids a global declaration conflict.
- Completeness sweep 2026-10-07 (`@researcher`): [OI Wiki discretization](https://oi-wiki.org/misc/discrete/), [maspypy index_compression](https://maspypy.github.io/library/ds/index_compression.hpp) and [to_small_key](https://maspypy.github.io/library/ds/to_small_key.hpp), [ei1333 compress](https://ei1333.github.io/library/other/compress.hpp), [suisen coordinate_compressor](https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp), [Nyaan compress](https://nyaannyaan.github.io/library/misc/compress.hpp), [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp). Bulk `ranks` (ei1333 `get(vector)`) and `stableRanks` were adopted.
- Local [Mathematics search header](../05-Mathematics/02-search_algorithms.hpp), [verification note](../05-Mathematics/91-search.md) and existing search tester: read API, proof, test coverage and dependency before reusing them. Their cited historical sources/acceptance are not new claims made by the facade.
