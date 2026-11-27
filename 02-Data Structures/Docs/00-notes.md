# 02 Data Structures — Notes

Folder-wide contracts, ownership boundaries, migration obligations and per-package omissions. Operation lists live only in [00-index.md](../00-index.md) and evidence in the per-header documents; rows 01–08 share the [Foundations](#foundations-rows-0108-p006) contracts.

## Common Contracts

- Ranges follow `03-cpp.md` (API Contracts): zero-based and half-open; an empty range returns the identity.
- Every structure states its indexing, boundary inclusivity, empty-range identity, key duplicates, signed coordinates, overflow policy and operation algebra (associativity, identity, action homomorphism, commutativity).
- Every structure states whether it is static, online or offline, its update and query memory, whether its bounds are amortized or worst-case, and whether its balancing is randomized or deterministic. Amortized bounds are re-proved under persistence and rollback; ephemeral amortization does not transfer.
- Generic arithmetic keeps every stored value and intermediate representable or is explicitly modular; `lng` is a convenient sum type, not an overflow check.
- Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants. Structures own their storage and copy as independent snapshots.
- The capacity of a public vector member or a returned vector is unspecified; only its size and values are part of the contract (user decisions 2026-11-12: `SparseTable::v`, then DS02 for every row).
- Handles (heap nodes, multiset tokens, versions) state their invalidation rules; a token from another history must never be passed back.

## Numeric and Algebraic Policies

- Fenwick variants require an additive commutative group, except `FenwickPrefixMonoid` (monotone updates, no range subtraction) and `FenwickRangeMax` (idempotent monoid, overwrite in O(log(n)^2)).
- Segment-tree searches need a deterministic predicate that is true on the identity and whose truth forms a prefix as the interval grows.
- `SparseTable::prod` needs idempotence; `associativeProd` and the disjoint sparse table need only associativity.
- Monoids follow the `03-cpp.md` monoid rule; presets, adapters and laws are row 00 ([00-monoids.md](00-monoids.md)). Rows 12, 13, 14 (`LazySegmentTreeBeats` adds `fail`), 15, 18 and 19 consume acted monoids. Rows 13 and 18 need only the monoid part for their plain trees and only the action part for their dual trees; row 15's 2D trees need a commutative `op` or `composition`.
- Li Chao trees and the convex hull trick state an overflow-safe evaluation policy, exact intersection comparison (128-bit products or bounded domains) and tie rules for equal lines, and serve both min and max queries.
- Segment-tree beats variants beyond chmin, chmax, add and assign need their own amortized proof and value-domain statement ([014-segtreebeats.md](014-segtreebeats.md)).
- ODT (Chtholly) workloads state their adversarial worst-case degeneration ([024-interval_set.md](024-interval_set.md)). Kinetic structures distinguish adversarial event counts from ordinary update bounds.
- Hash maps use a process-seeded SplitMix hash; Miscellaneous `002-customhash.hpp` owns the hash functions and the seeding policy.

## Ownership Boundaries

- Data Structures owns data-structure engines. Graphs owns dynamic-connectivity orchestration (`026-dynamic_connectivity.hpp`), HLD, LCA and tree adapters; Graphs `048-matroidintersection.hpp` consumes the oracles of `046-matroid_oracle.hpp` and Mathematics linear algebra.
- Geometry owns point-facing kd-tree, R-tree, quadtree and nearest-neighbour APIs (`028-spatial_index.hpp`), rectangle-union area and perimeter (`013-segment_union.hpp`), kinetic geometry and dynamic or decremental hulls; it may use `043-rangetree.hpp` and `033-offline_rectangle_queries.hpp` as engines.
- Strings owns literal-prefix (`004-trie.hpp`) and radix tries; this folder owns binary integer and XOR tries and the succinct rank/select engines that Strings `036-compressed_text_index.hpp` uses.
- Miscellaneous owns CDQ (`021-cdq.hpp`), parallel binary search (`020-parallelbinarysearch.hpp`), Hilbert order (`016-hilbertorder.hpp`), the generic undo log and checkpoints (`042-rollback_framework.hpp`), the persistent allocator, slope trick (`034-slope_trick.hpp`), coordinate compression by encounter ID, interval algorithms over static endpoint sets and probabilistic sketches. `063-offline_deletion.hpp` owns the generic add/remove-over-time engine and the queue-undo trick, because its contract is the rollback structure, not a graph.
- Core owns reusable accelerated dynamic bitsets and GF(2) matrices (`12-bitmatrix.hpp`: rank, solve, nullspace); `062-xorbasis.hpp` owns the incremental basis, the prefix (timestamped) basis and offline range-basis queries. Miscellaneous owns static mex (`008-sequence_algorithms.hpp`, which Mathematics `025-game_theory.hpp` calls for Grundy values); `024-interval_set.hpp` owns dynamic mex over a set (`IntervalSet::find(x).second`).
- Python mirrors (`08-Python`) own their own sorted-list and xor-basis (`_022_bitset.py`) implementations.
- Rollback DSU owns the generic undo and connectivity substrate; weighted DSU owns potential, parity and noncommutative algebra and the weighted rollback variant. They reuse the undo contract without reciprocal header dependencies. `065-dsu_extensions.hpp` holds member enumeration, component aggregates and actions, sparse keys and the merge-history forest, and depends only on `001-dsu.hpp`, `061-hashmap.hpp` and Miscellaneous `002-customhash.hpp`. Groups for rows 29 and 65 are monoids with `inv` ([00-monoids.md](00-monoids.md#group-contract)).
- Dynamic wavelet and succinct structures are Esoteric rows, never extra modes inside the Advanced wavelet header. Persistent split and merge stay with the persistent segment tree; destructive meld and split stay with the dynamic segment tree.
- Graphs Dijkstra and k-shortest walks and Miscellaneous optimal merge consume the heap and deque engines and do not reimplement heaps.

## Migration Obligations

- Row 23 (`023-range_query_offline.hpp`) is a pinned unchanged extraction: it keeps its original body and comments until its package rewrites it (`03-cpp.md`, One API per Operation), and compiling proves nothing about its semantics. Its audit strips hidden template globals and splits mixed-operation bodies.

## P006 Re-Audit Omissions

Candidates from the 2026-10-07 sweep ([00-sources.md](00-sources.md)) left out of rows 01–08:

- SegmentTree and Fenwick `reset`/`build` (MAS, HIT): constructing a new object does the same.
- Tree prefix sums and path differences (OI): they need LCA, so they belong to Graphs tree algorithms.
- SqrtDecomp monoid range actions (NYA, CPA): the lazy segment tree (row 12) has the better bound.
- SqrtDecomp `maxRight`/`minLeft` (CPA): the segment tree has the same predicate search with a better bound.
- CoordinateCompression distinct-by-position encoding and counting-sort mode (MAS, NYA): an argsort rank, not compression; Miscellaneous row 04 owns them (`stableRanks`, `compressCounting`, `stableRanksCounting`, 2026-11-17).
- DSU grid adapters (HIT): a grid vertex is `r * w + c`, an argument form (user decision 2026-11-12). DSU delete and move (OI) belong to row 65 `MovableDSU`; row 01 keeps neither (user decision 2026-11-13).

## Foundations Rows 01–08 (P006)

Folder-level record for the verified Basic rows `001-dsu.hpp` … `008-monotone_stack.hpp` (package P006, DS01 then DS02). Per-header evidence: [001-dsu.md](001-dsu.md), [002-fenwick.md](002-fenwick.md), [003-segmenttree.md](003-segmenttree.md), [004-sparsetable.md](004-sparsetable.md), [005-prefix_sum.md](005-prefix_sum.md), [006-sqrt_decomposition.md](006-sqrt_decomposition.md), [007-ordered_set.md](007-ordered_set.md), [008-monotone_stack.md](008-monotone_stack.md). C01/P002 supplies the verified template, integer aliases and PBDS imports. Rows 01–08 use no ISA kernels, no Barrett or Montgomery reduction and no empirically selected dispatch thresholds.

### Common Foundation Contracts

The folder's [common contracts](#common-contracts) apply. Empty construction is supported. Invalid indices, sizes, shapes and required predicate identities are preconditions checked by assertions, and they remain preconditions under `NDEBUG`. Complexity counts constant-cost element operations, comparisons and copies; strings and large integers add their own costs. Allocation failure is ordinary C++ allocation failure. Associativity, idempotence, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

A moved-from structure may only be assigned to or destroyed. Its size fields keep their old values while its storage is empty, so every other call is a precondition violation; assignment restores every invariant. Structures with both a size constructor and a vector constructor (`Fenwick`, `SegmentTree`, `PrefixSum`, `DifferenceArray`, `SqrtDecomp`, `SqrtRangeSum`) also take an `std::initializer_list<T>`, so a braced list always means values, as with `std::vector`: `PrefixSum<lng> p({7})` and `PrefixSum<lng> p{7}` hold the single value 7, and `PrefixSum<lng> p(7)` holds seven zeros. Elements of `T = bool` are read by value through `vector<bool>::const_reference` wherever a reference to storage is returned.

### Test Infrastructure

Rows 01–08 run through `96-Local Testing/_00_contest_runner.py`, which sets the optimized, checked and sanitizer builds with their flags and leak checking; modes and failure reporting follow `05-testing.md` (Layout and Modes). Folder-specific parts:

- Invalid probes must end in SIGABRT with an assertion diagnostic.
- The ordered-set suite's checked build uses `_GLIBCXX_ASSERTIONS` instead of `_GLIBCXX_DEBUG` ([007-ordered_set.md](007-ordered_set.md)).
- Exhaustive enumerations stop at the first failing case. The DSU union histories and the `FenwickTimestamp` add/clear histories run by iterative deepening, so their first failure is a shortest history; the other enumerations make no shrinking claim.
- Each suite's source records its concrete mode limits.
- Declared algebra, monotonicity and representability conditions and impractically large allocations are justified contracts, not proved at run time by finite tests.

## P027 Omissions

Candidates from the 2026-10-08 sweep and the 2026-10-17 monoid sweeps ([00-sources.md](00-sources.md)) left out of rows 00, 11, 12, 13 and 24:

- Fenwick01 `kth` from an offset (MAS): `kth(rank(l) + k)`.
- Lazy tree `multiply`, generator build and `reset` (MAS): `apply(p, f)` or `set(p, op(get(p), x))`, and constructing a new object.
- Power sums under range add (MAS): `RangeLinearRangeSum` with a binomial matrix.
- Run-based range-assignment tree (MAS): rare; `IntervalMap` (row 24) plus a point-update tree does the same.
- `Dummy` monoid (MAS): rare; `Monoid` with a user op.
- Euler-tour diameter monoid (CF 1149C): Graphs owns tree diameters.
- Dynamic tree one-node-per-key sparse variant (MAS): a memory variant of the same API.
- Dynamic tree range `clear` (TKO) and split by count (MAS sortable): rare; split by count is `split(root, maxRight(root, lo, count <= k))`, and sortable trees are row 51.
- Dynamic tree vector build (legacy `DynSegTree(span)`): a dense array is row 12.
- Interval set `sameInterval`, `maxExcluded` and iterator `lowerBound` (SUI, NYA): `covers(min, max + 1)`; `contains(x) ? find(x).first - 1 : x` (none when that interval starts at the minimum of `T`); `find` and `next` cover the lookups.
- `IntervalsFast` over a fastset (MAS): row 36 owns the fastset.

## P227 Omissions

Candidates from the 2026-10-09 sweep ([00-sources.md](00-sources.md)) left out of rows 61 and 64:

- HashMap `count` (MAS, NYA, LC): `contains` gives the same answer.
- HashMap `forEach`/`enumerate` (MAS, NYA): range-for over `begin`/`end` does it.
- HashMap `erase(iterator)` returning the next element (NYA): backward-shift erase moves later cluster members into the hole, so erasing while iterating needs a different contract; rare.
- `emplace`/`try_emplace`, a default-value parameter, `build(n)`, `cbegin`/`cend` and `dump` (NYA, MAS, LC): covered by `insert`, `get(k, def)`, `clear` plus `reserve`, const `begin`, and range-for.
- Shrinking on erase (NYA): capacity stays at the peak; assign a new table to release it.
- SortedList `countRange` and `irange`/`islice` (sortedcontainers, tatyam): `rank(r) - rank(l)` and iterators.
- SortedList negative `kth` indices (tatyam, PyRival): a Python convention; C++ uses `kth(n - 1 - k)`.
- Fenwick over bucket sizes (PyRival) or a positional index tree (sortedcontainers): `rank`/`kth` would gain O(log(n)) bucket lookup, but `insert` stays O(sqrt(n)) and every split or merge rebuilds the index. The RATIO sweep in [064-sortedlist.md](064-sortedlist.md) favours smaller buckets (shorter shifts) over fewer buckets, so the linear bucket scan is not the bottleneck at n <= 1e6.

## P028 Omissions

Candidates from the 2026-10-10 sweep ([00-sources.md](00-sources.md#fetched-2026-10-10-p028-sweep)) left out of rows 14, 15, 16 and 30:

- Historic values under chmax (UOJ 169) and the historic-max sum (OI, tjkendev): each needs four or more interacting tags; `LazySegmentTreeBeats` hosts the chmax variant as a user BeatsMonoid.
- Chmin with a change callback (MAS) and historic sums of squares: rare.
- `LazySegmentTreeBeats::values` (SUI, MAS): rare on beats trees; `prod` per prefix, or row 12 when no failures are needed.
- Kinetic segment tree beats (MAS): row 45 owns it.
- `FenwickOfSegmentTrees` (former row 15 entry): `SegmentTree2DSparse::prod` answers prefix-x range-y queries at the same bound; row 11 `CompressedFenwick2D` is the group-valued variant.
- Pluggable range trees (NYA, HIT): rare; row 43 owns range trees, including the k-dimensional compressed `RangeTreeK` (SUI, user decision 2026-11-04).
- Merge sort tree `sumLeq`/`countSumLeq` (LC `static_range_sum_with_upper_bound`): row 17 `sumLess`.
- Online `lng` x coordinates in `DynamicMergeSortTree` (legacy `insert2D`): compress x to slots; huge online universes belong to row 13.
- SWAG `operator[]`, separate front and back folds, pop returning the value, iterator constructors and separate value and product types (SUI, MAS, HIT): rare conveniences; `front`, `back`, `top` and `allProd` cover the uses.

## P029 Omissions

Candidates from the 2026-10-13 sweep ([00-sources.md](00-sources.md#fetched-2026-10-13-p029-sweep)) left out of rows 17, 19 and 25:

- Wavelet xor queries (`kthXor`, `xorMin`/`xorMax`, xor-bounded counts; SUI, MAS): they need raw-bit levels instead of the compressed alphabet; row 26 `PersistentBinaryTrie` answers range xor queries through prefix versions.
- Tree-path wavelets (MAS): rare; a Graphs path-kth owner would build on the multi-range descents.
- Fenwick nodes in `WaveletMatrixMonoid` for invertible weights (EI, NYA): a constant-factor variant of the segment-tree nodes with the same bounds.
- Treap `next` (OI, LC `ordered_set`): `lowerBound` or `upperBound`.
- Treap difference (CPA, bound stated without code): rare.
- `decreaseKey` on `BinomialHeap`/`LeftistHeap` and handle `erase` on the leftist, skew, randomized and binomial heaps (OI, CPA, HIT): they need parent pointers or handle-moving key swaps; `PairingHeap` and `FibonacciHeap` cover handles with better bounds.
- Heap pool `clear` and sorted dumps (HIT, SUI, MAS, OI): a new object, and popping a copy.
- Brodal and soft heaps: no catalog implementation.

## P033 Omissions

Candidates from the 2026-10-14 sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)) left out of rows 26, 27, 28 and 33:

- Binary trie meld (OI, NYA): merging in amortized O(total nodes) needs tries sharing one node pool; small-to-large insertion gives O(n log(n) B).
- The count of pairs with xor below t (HIT, yukicoder 2977): n `countLess` calls.
- Index payloads per key (EI, NYA, MAS): a map beside the trie.
- Per-query xor masks on `BinaryTrie::kth`, `countLess` and `lowerBound` (EI, MAS, SUI): `applyXor` around the query; `PersistentBinaryTrie` takes the mask because versions are immutable.
- Weighted subarray counts by minimum (MAS `count_subrectangle`): `(i - l + 1) * (r - i)` from `range`. Depth or traversal arrays: no catalog offers them.
- Blocked static range product with naive in-block queries (MAS `Static_Range_Product`): O(2^LOG) in-block queries; `staticRangeProducts` answers offline in O(n + q) memory.
- 2D associative products on a segment tree (MAS): rare; row 15 covers commutative rectangle products.
- Alon–Schieber O(α(n)) semigroup queries after linear preprocessing: research-level, no contest implementation.
- Block-decomposed Fenwick trees (OI): an online structure; row 11 would own it if a motivating problem appears.

## P034 Omissions

Candidates from the 2026-10-17 sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)) left out of rows 09, 10, 34, 35 and 63:

- SqrtTree range assignment with O(sqrt(n)) updates and O(log(log(n))) queries (CPA, OI `massUpdate`): the same operation as row 34 `SqrtTree::assign` by another algorithm.
- SqrtTree generator build and fixed three-layer layout (MAS): construction from a vector; row 34 already stops layering at segments of at most 8 elements, which a direct fold answers.
- PersistentArray in-place set (SUI `mut_set`, MAS `make_copy = false`): it writes into nodes shared with other versions. `size` (SUI): the public `n`. Node pools, reset and garbage collection (SUI, MAS): Miscellaneous row 43 owns persistent arenas. Fat-node partially persistent array (EI internals): `PartiallyPersistentDSU` keeps its own attach times. Persistent block array (OI): O(sqrt(n)) per operation.
- RollbackDSU `makeSet` and an `undo` reporting what it reverted: no catalog has them. Union by rank (CPA): `size` needs sizes. History-clearing `snapshot` (EI, SUI) and `reset` (MAS, HIT): `rollback` to a token and `rollback(0)`.
- PersistentDSU `dump` (SUI): debug output.
- Deque undo (Codeforces blog 154645, read on 2026-11-22, [00-sources.md](00-sources.md#fetched-2026-11-22-source-check)): the blog sketches it and states that it has no proof (p. 2). Priority-queue undo (blog 111117) joined row 63 as `PriorityUndo` (user decision 2026-11-22): it undoes the highest-priority update online in O(T(n) log(n)) amortized time, with a proof (pp. 2–4), which neither `OfflineDeletion` nor `QueueUndo` provides.

## P035 Omissions

Candidates from the 2026-10-23 sweep ([00-sources.md](00-sources.md#fetched-2026-10-23-p035-sweep)) left out of rows 29, 36 and 65:

- Persistent weighted DSU: no catalog has one.
- Monoid-only component actions such as chmax: `DualDSU` cancels the absorbing root's tag with `inv`, which a monoid lacks.
- FastSet `clear`/`reset` (maspypy, hitonanode): constructing a new set does the same. `fillOne`: `build` with a true predicate. `toString`: debug output. Counting and k-th: Fenwick `lowerBound` (row 02).

## P030 Omissions

Candidates from the 2026-10-27 sweep ([00-sources.md](00-sources.md#fetched-2026-10-27-p030-sweep)) left out of row 18 (user decision 2026-10-27; pool reset, copy with an action, `kthMax`, `countLess` and `sumLess` were added instead):

- k-th smallest on a tree path (SPOJ COT, versions per vertex combined through the LCA): Graphs owns tree adapters; filed as P248 (Graphs row 88).
- Position-dependent defaults for untouched ranges (legacy `defR(l, r)`, row 13's `fill` functor) in `PersistentSegmentTree`: untouched ranges would hold `fill` instead of `e()`, which breaks the null-node shortcuts of `merge`, `split` and retagging. The fill build `(n, x)` covers uniform defaults on any `lng` universe, and a vector build covers the rest up to memory.

## P032 Omissions

Candidates from the 2026-10-28 DS11 sweep ([00-sources.md](00-sources.md#fetched-2026-10-28-p032-ds11-sweep)) left out of rows 21 and 22 (user decision 2026-10-28; floating-point lines were added to row 22 instead):

- Real x coordinates (cp-algorithms): `LiChaoTreeCompressed` takes any finite point set of `lng`; real points need a rank mapping inside `F::eval`, which a stored function cannot capture.
- Line deletion: `undo` with row 63 `OfflineDeletion`. Node-pool reset (maspypy): a new object.
- Rollback CHT with binary-search insertion (CF blog 51684 and its comments, CEOI 2009 Harbingers; both read on 2026-11-22, [00-sources.md](00-sources.md#fetched-2026-11-22-source-check)): `LiChaoTree` and `LiChaoTreeCompressed` with `UNDO` answer the same tree DP at O(log(C)) per operation.
- `empty`, `size`, `clear` (ei1333, maspypy): the public deque, and a new object.
- Max of `a x + b y` over points (maspypy `CHT_XY`, Nyaan `LineContainer2D`) and static envelope pieces (maspypy `line_min_function`): Geometry rows 20 and 41.

Candidates from the 2026-11-02 DS27 sweep ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds27-sweep)) left out of row 31 (user decision 2026-11-02):

- `clear` (hitonanode, iwiwi): a new object, as in rows 22 and 36; `swap`: `std::swap`.
- Bounded-gap radix heaps with O(log(C)) buckets (Ahuja et al. §2, read on 2026-11-22, [00-sources.md](00-sources.md#fetched-2026-11-22-source-check); scrapbox): the xor buckets already give O(b), and Graphs `dialDijkstra` covers small `C`. The paper's two-level form and its Fibonacci-heap variant (§3–4: Dijkstra in O(m + n log(C) / log(log(C))) and O(m + n sqrt(log(C)))) were not weighed in this decision; P032 benchmarks them and adopts the faster or records the measured reason (user decision 2026-11-22).
- Rust `radix-heap` extras (start bound, `constrain`, iterators, `shrink_to_fit`): no contest use.

Candidates from the 2026-11-02 DS28 sweep ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds28-sweep)) left out of row 32 (user decision 2026-11-02; `PersistentRangeMex` was added, the threshold query lists values with count at least `t`, and the majority candidate comes from a Boyer–Moore segment tree):

- All-distinct test: `staticRangeCountDistinct` equals `r - l`. The k-th missing value: rare.
- k-majority by Misra–Gries summaries: `RangeThresholdFrequency::frequent` with `t = (r - l) / k + 1`.

Candidates from the 2026-11-02 DS50 sweep ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds50-sweep)) left out of row 62 (user decision 2026-11-02; `represent` and `PrefixXorBasis::push` were added):

- `countRepresentable`, `reducedEchelon`, `min`: `1ull << size()`, the stored reduced basis `b`, and `reduce(x)` or `kth(1)` (duplicates ledger, Keeps).
- Undo of `insert` for row 63 `OfflineDeletion`: a client keeps its own pivot-indexed basis with an undo history (row 63's test client `UndoXorBasis`) or snapshots the at most 64-word `b` and `masks`. Codeforces blog 149993 (read on 2026-11-22, [00-sources.md](00-sources.md#fetched-2026-11-22-source-check)) gives the same offline deletion, divide and conquer over time with a rollback basis. It also points to an online deletion method at about O(B^2) per operation, which the copy does not include; P032 finds it and adopts it or records why it stays out (user decision 2026-11-22).
- A segment tree of bases: a segment tree over `merge`. Range minimum or k-th: `rangeXorBasisQuery`'s echelon basis inserted into an `XorBasis`. k-th with multiplicities over n inserted values: `kth(k >> (n - size()))` when `n - size() < 64`, else 0.

## Planned-Row Omissions

- Row 48 `PersistentSkewHeap`: skew-heap melds are only amortized, and persistence lets an expensive version be reused, so no bound holds; `PersistentLeftistHeap` gives persistent melds in O(log n).

## Test Matrix

- Differential-test Fenwick and segment-tree variants against a vector, persistent versions against copied snapshots, dynamic-tree structures against small forests, sorted containers against a sorted vector, and hash maps against `std::unordered_map` with adversarial key patterns.
- Also test repeated update and rollback sequences, noncommutative action order, version branching and failed merges.

## Omission Sweep 2026-11-08

Left out by the 2026-11-08 sweep (user decisions, `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions): exact duplicates, second implementations and items another row owns. Each note names where the candidate is implemented instead.

- `RBST` (acted monoid, implicit): build, at, set, apply (range), prod, reverse, maxRight, values (MAS, NYA): a second implementation of row 19 `ImplicitTreap` with the same operations, domain and expected bounds; persistence is row 58.
- `select0`, `select1` by binary search over block ranks (NOSHI, SUI): a second implementation of row 40's `select0`/`select1` with the same results and a worse bound.
- `RollbackWeightedDSU`: findSet, isSameSet, size, count, groups: found in the library, no catalog source; inherited from row 09 `RollbackDSU`, which owns them, so row 29 lists only what the class defines.
- `rebuild` (pool compaction) for row 58 (EI, SUI, TKO): Miscellaneous row 43 `PersistentArena` owns persistent node pools.

## P275 Re-Review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per Operation), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`; [evidence](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md)). The operations it returned are in the inventory rows, including the new planned Esoteric rows 70–74, and the omission bullets that named them, like those for the operations adopted on 2026-11-08, were deleted (user decision 2026-11-22). Every omission still listed here stands.

- Not returned: `setIntersectionOffline` (Miscellaneous row 22 owns it, from the same maspypy file) and `GridDSU` (a grid vertex is `r * w + c`, an argument form). The `LazySegmentTree` point combine waits for its name (`omissions.md`, Open Questions, P027).
- The review recorded hitonanode's monotone hull merge as verified in row 22, which never had it. It is now the missing operation `MonotoneCHT::merge` of row 22 (source hitonanode; user decision 2026-11-22).

## DS01 Sweep 2026-11-11

The DS01 completeness sweep ([00-sources.md](00-sources.md)) added `DSU::leaders` (MAS `get_all`, a batch form of `findSet`), `SegmentTree::setManySorted` (MAS `set_many_sorted`) and `SegmentTree::prodIds` (MAS `prod_ids`, a node-index result no other operation returns) to rows 01 and 03 (`03-cpp.md`, One API per Operation). It left nothing out.

## DS02 Sweep 2026-11-12

The DS02 completeness sweep ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)) settled its candidates under the recorded decisions of 2026-11-08 and 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`; user decision 2026-11-12 after the DS02 review).

- Compositions and derived counts joined rows 05, 06 and 08: `DifferenceArray::addArithmetic` (hitonanode), `StepSum`, `SortedBlocks::countGeq`, `countGreater` and `countEqual`, and in row 08 `countBinaryRectanglesAreaAtMost` (ABC420 F), `maxMinTimesSum` (UVa 1619), `maxMinTimesSum2D` (ABC311 G), `largestCheckerboardSquare` and `largestCheckerboardRectangle` (P1169), `nextGreaterDistanceSum` (POJ 3250), `countBinarySquares` and `countBinaryRectanglesTotal` (CSES 3413, 3414). The total is its own algorithm: summing `countBinaryRectangles` needs its O(n * m) table, while the stack count keeps O(m).
- The standalone one-problem `CounterBlocks` (EI) joined planned Esoteric row 71, and `slidingLargestHistogramRectangle` (CSES 3227) planned row 73.
- Row 07 owns the offline Fenwick multiset, built in the DS02 `/finish` as `OrderedMultiSetOffline` (user decision 2026-11-12, `omissions.md`).

Left out:

- Prefix-sum constructors from `(n, f(i))` and clamped `sum<true>` (MAS), and `add` then `build` (EI, NYA): argument forms of the vector constructors and `sum`.
- `BlockList` set, range reverse, split and merge (OI): `at` returns a mutable reference; sequence split, merge and reverse are rows 19 and 20.

## Omission Sweep 2026-11-27

Left out from the candidates of the 2026-11-27 catalog survey (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`; user decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`, Recorded Decisions): second implementations and slower algorithms that Priority does not admit.

- Zip tree (WIKI): a second implementation of row 19 `Treap` and row 20 `SkipList`, with the same operations and expected O(log n) bounds (`03-cpp.md`, One API per Operation).
- Conc-tree (WIKI): a second implementation of row 57 `FingerTree` push and concat and of row 58 `PersistentImplicitTreap` concat and split, at equal or better bounds (One API per Operation).
- Pagoda (WIKI): a second implementation of row 25's meldable heaps (push, pop, top, meld) at equal or worse bounds (One API per Operation).
- T-tree (WIKI): a second implementation of row 20 `AVLTree` and of `BTree`'s array nodes, with the same operations and O(log n) bounds (One API per Operation).
- Threaded tree (WIKI): no new operation; its successor and predecessor walks are row 20 `lowerBound` and `upperBound`, and its stackless traversal is row 76 `morrisInorder` (One API per Operation).
- WAVL tree (WIKI): a second implementation of row 20 `AVLTree` and `RedBlackTree` with the same bounds; its O(1) amortized rotations give no operation they lack (One API per Operation).
- Queap (WIKI): a second implementation of row 25 `DAryHeap` and `PairingHeap` push, pop and top, no better in the worst case (One API per Operation).
- Weak heap (WIKI): a second implementation of row 25 `DAryHeap` with the same bounds; its saving in comparisons serves weak-heap sort, which no catalog names here (One API per Operation).
- Beap (WIKI): its O(sqrt n) insert, erase and search lose to row 25 `DAryHeap` for priority operations and to row 20's trees for search (`01-principles.md`, Priority).
- Judy array (WIKI): an engineered 256-ary radix trie whose operations row 61 `HashMap`, row 36 `FastSet`, row 26 `BinaryTrie` and Strings row 25 `RadixTrie` already provide (One API per Operation).
- Hash array mapped trie (WIKI): a second persistent map; row 58 `PersistentTreap` provides its operations, and row 35 `PersistentArray` uses the same 2^B-ary path copying (One API per Operation).
- LSM-tree (WIKI): the logarithmic method over sorted runs, which row 69 `BinaryGrouping` provides; row 20 `BTree` is the in-memory index (One API per Operation).
- Piece table (WIKI): a second editable sequence; row 19 `ImplicitTreap`, row 20 `ImplicitSplay` and `Rope`, and row 06 `BlockList` give its insert, erase and at at equal or better bounds (One API per Operation).

Kept although their only gain is space (user decision 2026-11-27, a Priority exception recorded in `omissions.md`): `RRRBitVector`, `HybridBitVector` and `EliasFano` in row 40, `DirectlyAddressableCodes`, `CodedVector`, `SuccinctPermutation` and `SortedIntStack` in row 60, and `K2Tree` in row 43. A memory gain counts as more capable when a judge memory limit binds; each package keeps the structure's own query bounds as its contract and measures its space against the plain form it compresses.

## Omission Sweep 2026-11-28

Left out from the candidates of the 2026-11-28 sample of the sources the user placed (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`, Sampled and Applied 2026-11-28).

- Weight-biased leftist trees (MEHTASAHNI): A second implementation of row 25 LeftistHeap with the same O(log n) push, pop and meld (03-cpp.md, One API per Operation); row 20's leafy WBLT is a different structure.
- Symmetric min-max heaps, min-max heaps, deaps and dual priority queues (MEHTASAHNI): Second implementations of row 25 DoubleEndedPriorityQueue (interval heap) with the same O(log n) push, popMin and popMax and O(1) min and max (One API per Operation).
- Dynamic perfect hashing (MEHTASAHNI): Same worst-case O(1) lookup and expected amortized O(1) updates as row 61 CuckooHashMap (One API per Operation); static FKS is Miscellaneous row 19 FksHash.
- Chaining, quadratic probing, double hashing and Brent's method (MEHTASAHNI): Second implementations of row 61 HashTable (linear probing) with the same expected O(1) bounds and no faster form in practice (One API per Operation).
- Low-height schemes and relaxed balance (MEHTASAHNI): Relaxed balance decouples rebalancing for concurrency and low-height schemes only tighten constants of the height; sequentially they are second implementations of row 20's trees with the same O(log n) bounds (One API per Operation).
- Randomized finger search trees (MEHTASAHNI): Second implementations of FingerSearchTree (DS2-012) with the same expected O(log d) bounds (One API per Operation).
- Topology trees (MEHTASAHNI): A second implementation of row 39 TopTree link, cut and path or subtree aggregates with the same O(log n) bounds (One API per Operation).
- Concurrent data structures (MEHTASAHNI): 01-principles.md (Profiles) fixes single-threaded execution as the baseline, and sequentially these are second implementations of rows 25, 30, 61 and 20 with the same bounds (One API per Operation).
- Most-specific-range matching (MEHTASAHNI): For nonintersecting and conflict-free ranges the most specific match is the shortest containing range, an argument form of IntervalTree::stabMax (DS2-069) with priority minus length (03-cpp.md, One API per Operation).
- LOUDS lowest common ancestor (NAVARRO): Slower (O(depth)) than row 41 BalancedParenthesesTree::lca and DFUDS lca in the same domain; Priority admits no slower second form here (01-principles.md, Priority).
- Secondary-memory succinct structures (NAVARRO): Disk layouts of the in-memory rows 40, 60, 41, 43 and Strings 36 with the same operations and no better RAM-model bound (One API per Operation).
