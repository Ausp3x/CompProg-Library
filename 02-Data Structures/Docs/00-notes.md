# 02 Data Structures — notes

Folder-wide contracts, ownership boundaries, migration obligations and per-package omissions. Operation lists live only in [00-index.md](../00-index.md); evidence is in the per-header documents, and rows 01–08 share the [Foundations](#foundations-rows-0108-p006) contracts.

## Common contracts

- Ranges follow `03-cpp.md`, API contracts (zero-based, half-open); empty ranges return the identity.
- State indexing, boundary inclusivity, empty-range identity, key duplicates, signed coordinates, overflow policy and the operation algebra (associativity, identity, action homomorphism, commutativity) for every structure.
- State static versus online/offline, update/query memory, amortized versus worst-case bounds, and whether balancing is randomized or deterministic. Amortized bounds must be re-proved under persistence and rollback; ephemeral amortization does not transfer.
- Generic arithmetic must keep every stored value and intermediate representable or be explicitly modular; `lng` is a convenient sum type, not an overflow check.
- Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants. Structures own their storage and copy as independent snapshots.
- Handles (heap nodes, multiset tokens, versions) state their invalidation rules; tokens from other histories must never be passed back.

## Numeric and algebraic policies

- Fenwick variants require an additive commutative group; prefix-monoid Fenwick requires monotone updates (max/min) and has no range subtraction.
- Segment-tree searches need a deterministic predicate true on the identity whose truth forms a prefix as the interval grows.
- `SparseTable::prod` needs idempotence; `associativeProd` and the disjoint sparse table need only associativity.
- Monoids follow the `03-cpp.md` monoid rule; presets, adapters and laws are row 00 ([00-monoids.md](00-monoids.md)). Rows 12, 13, 14 (`LazySegmentTreeBeats` adds `fail`), 15, 18 and 19 consume acted monoids (rows 13 and 18 also need only the monoid part, or only the action part, for their plain and dual trees; row 15's 2D trees need a commutative `op` or `composition`).
- Li Chao and convex hull trick must state an overflow-safe evaluation policy and exact intersection comparison (128-bit products or bounded domains) and tie rules for equal lines, and serve both min and max queries.
- Segment-tree beats variants beyond chmin/chmax/add/assign need their own amortized proof and value-domain statement ([14-segtreebeats.md](14-segtreebeats.md)).
- ODT/Chtholly workloads must state adversarial worst-case degeneration ([24-interval_set.md](24-interval_set.md)). Kinetic structures must distinguish adversarial event counts from ordinary update bounds.
- Hash maps use a process-seeded SplitMix hash; Miscellaneous `02-customhash.hpp` owns the hash functions and seeding policy.

## Ownership boundaries

- Data Structures owns data-structure engines. Graphs owns dynamic-connectivity orchestration (`26-dynamic_connectivity.hpp`), HLD, LCA and tree adapters; Graphs `48-matroidintersection.hpp` consumes the oracles of `46-matroid_oracle.hpp` and Mathematics linear algebra.
- Geometry owns point-facing kd/R/quadtree and nearest-neighbour APIs (`28-spatial_index.hpp`), rectangle-union area/perimeter (`13-segment_union.hpp`), kinetic geometry and dynamic/decremental hulls; it may use `43-rangetree.hpp` and `33-offline_rectangle_queries.hpp` as engines.
- Strings owns literal-prefix (`04-trie.hpp`) and radix tries; this folder owns binary integer/XOR tries and succinct rank/select engines used by Strings `36-compressed_text_index.hpp`.
- Miscellaneous owns CDQ (`21-cdq.hpp`), parallel binary search (`20-parallelbinarysearch.hpp`), Hilbert order (`16-hilbertorder.hpp`), generic undo log/checkpoints (`42-rollback_framework.hpp`), persistent allocator, slope trick (`34-slope_trick.hpp`), coordinate compression by encounter ID, interval algorithms over static endpoint sets and probabilistic sketches. `63-offline_deletion.hpp` owns the generic add/remove-over-time engine and the queue-undo trick because its contract is the rollback structure, not a graph.
- Core owns reusable accelerated dynamic bitsets and GF(2) matrices (`12-bitmatrix.hpp`: rank, solve, nullspace); `62-xorbasis.hpp` owns the incremental basis, prefix (timestamped) basis and offline range basis queries. Mathematics owns Sprague–Grundy mex (`25-game_theory.hpp`); `24-interval_set.hpp` owns dynamic mex over a set (`IntervalSet::find(x).second`).
- Python mirrors (`08-Python`) own their own sorted-list and xor-basis (`_22_bitset.py`) implementations.
- Rollback DSU owns the generic undo/connectivity substrate; weighted DSU owns potential/parity/noncommutative algebra and the weighted rollback variant. Reuse the undo contract without reciprocal header dependencies. `65-dsu_extensions.hpp` holds member enumeration, component aggregates and actions, sparse keys and the merge-history forest; it depends on `01-dsu.hpp`, `61-hashmap.hpp` and Miscellaneous `02-customhash.hpp` only. Groups for rows 29 and 65 are monoids with `inv` ([00-monoids.md](00-monoids.md#group-contract)).
- Dynamic wavelet and succinct structures are Esoteric rows, never extra modes hidden inside the Advanced wavelet header. Persistent split/merge stays with the persistent segment tree; destructive meld/split stays with the dynamic segment tree.
- Heap and deque engines are consumed by Graphs Dijkstra and k-shortest walks and Miscellaneous optimal merge; those clients do not reimplement heaps.

## Migration obligations

- Unchanged extractions (`23`) keep original bodies and comments; compilation proves nothing about semantics. Audits must strip hidden template globals and compile each header alone. Mixed-operation bodies are split during audit.

## P006 re-audit omissions

Candidates from the 2026-10-07 sweep ([00-sources.md](00-sources.md)) left out of rows 01–08:

- SparseTable `maxRight`/`minLeft` (MAS): the table has no identity; a binary search over `query` gives the same O(log(n)) bound.
- SegmentTree/SqrtDecomp/Fenwick `reset`/`build` (MAS, HIT): constructing a new object does the same.
- Fenwick timestamp clearing (OI): a per-problem multi-test trick, not a structure operation.
- Prefix product over a noncommutative or xor group (MAS `Static_Range_Product_Group`): a custom T with `+`/`-` covers commutative groups; noncommutative prefix products are rare.
- 3D prefix sums and difference arrays (OI): rare; apply the 2D axis-by-axis method one more time.
- Tree prefix sums and path differences (OI): need LCA, so they belong to Graphs tree algorithms.
- SqrtDecomp range actions and block policies (NYA, CPA): the lazy segment tree (row 12) has a better bound for monoid actions.
- SqrtDecomp `maxRight`/`minLeft` (CPA): the segment tree has the same predicate search with a better bound.
- O(1)-update / O(sqrt(n))-query group block sums (MAS): a Mo value-domain tool, owned with Mo (row 23).
- OrderedMultiSet `split`/`join` (OI pb_ds): rare; `OrderedSet` keeps the native PBDS API, including both.
- CoordinateCompression distinct-by-position encoding and counting-sort mode (MAS, NYA): an argsort rank, not compression.
- DSU grid adapters, delete/move element (HIT, OI): left out by P035 ([P035 omissions](#p035-omissions)).

## Foundations rows 01–08 (P006)

Folder-level record for the verified Basic rows `01-dsu.hpp` … `08-monotone_stack.hpp` (package P006, DS01 then DS02 order). Per-header evidence: [01-dsu.md](01-dsu.md), [02-fenwick.md](02-fenwick.md), [03-segmenttree.md](03-segmenttree.md), [04-sparsetable.md](04-sparsetable.md), [05-prefix_sum.md](05-prefix_sum.md), [06-sqrt_decomposition.md](06-sqrt_decomposition.md), [07-ordered_set.md](07-ordered_set.md), [08-monotone_stack.md](08-monotone_stack.md). C01/P002 supplies the verified template, integer aliases and PBDS imports. Rows 01–08 use no ISA kernels, Barrett/Montgomery reduction or empirically selected dispatch thresholds.

### Common foundation contracts

The folder [common contracts](#common-contracts) apply. Empty construction is supported. Invalid indices, sizes, shapes and required predicate identities are preconditions checked by assertions; they remain preconditions under `NDEBUG`. Complexity counts constant-cost element operations/comparisons/copies; strings and large integers add their own costs. Allocation failure is ordinary C++ allocation failure. Associativity, idempotence, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

A moved-from structure may only be assigned to or destroyed. Its size fields keep their old values while its storage is empty, so every other call is a precondition violation. Assignment restores every invariant. Structures that have both a size constructor and a vector constructor (`Fenwick`, `SegmentTree`, `PrefixSum`, `DifferenceArray`, `SqrtDecomp`, `SqrtRangeSum`) also take an `std::initializer_list<T>`. A braced list therefore always means values, as with `std::vector`: `PrefixSum<lng> p({7})` and `PrefixSum<lng> p{7}` hold the single value 7, and `PrefixSum<lng> p(7)` holds seven zeros. Elements of `T = bool` are read by value through `vector<bool>::const_reference` wherever a reference to storage is returned.

### Test infrastructure

Failure reporting follows `05-testing.md`; invalid probes require SIGABRT with assertion diagnostics. Exhaustive enumerations stop on the first discovered failing case; no automatic shrinking claim is made.

Quick modes reduce enumeration lengths/history depths/random counts and run optimized plus checked builds. Full runs every feature in optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`; the ordered suite uses `_GLIBCXX_ASSERTIONS`, see [07-ordered_set.md](07-ordered_set.md)), and sanitizer (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`) builds, with leak checking enabled. Every configuration compiles with `-Wall -Wextra -Wconversion -Werror`. Stress modes extend the enumerations/random counts; each suite's source records its concrete mode limits. Declared algebra/monotonicity/representability conditions and impractically large allocations are justified contracts, not runtime proof by finite tests.

## P027 omissions

Candidates from the 2026-10-08 sweep and the 2026-10-17 monoid sweeps ([00-sources.md](00-sources.md)) left out of rows 00, 11, 12, 13 and 24:

- Hash-map dynamic Fenwick, dynamic 2D Fenwick (NYA): row 13 covers huge universes; a hash-map Fenwick is a constant-factor variant without new operations.
- Persistent Fenwick (SUI): row 18 owns persistence.
- Compressed rectangle-add/rectangle-sum, compressed dual 2D Fenwick (SUI, MAS): row 33 owns the offline rectangle engines; `Fenwick2DRangeAdd` covers the dense online case.
- Fenwick range max with arbitrary overwrite in O(log(n)^2) (OI): the segment tree (row 03) does it in O(log(n)).
- 2D dual Fenwick (rectangle add, point get) (NYA): four `Fenwick2D::add` corners plus `prefixSum`.
- K-dimensional compressed prefix monoid (SUI, EI): rare; `CompressedFenwick2D` with a group covers dominance sums, and 2D prefix max is a sorted sweep over `FenwickPrefixMonoid`.
- Fenwick01 `kth` from an offset (MAS): `kth(rank(l) + k)`.
- Lazy tree `multiply`, generator build, `reset` (MAS): `apply(p, f)` or `set(p, op(get(p), x))`; constructing a new object.
- Lazy tree xor-index range apply, rollback lazy tree, run-based range-assignment tree (MAS): rare; the last is `IntervalMap` plus a point-update tree. Power sums under add or assignment (MAS) are `RangeLinearRangeSum` with a binomial or rank-one matrix.
- Plain `Assign` monoid (MAS): any assign preset's action drives a dual tree (`DualSegmentTree<RangeAssignRangeSum<T>>`), and `MonoidAction<Max<pair<time, value>>>` assigns by timestamp.
- `AddDivide` floor-division composite (MAS, CF 878E), Fibonacci-progression add (CF 446C), elementary symmetric sums under add (P4247), pairs of acted monoids (CF 1114F): one problem each; user structs under the contract.
- `Dummy` and dihedral-group monoids, public `monoid_pow` (MAS): rare; `Monoid` with a user op, and powers by squaring with `m.op`.
- Leftmost free block of length k (P2894 Hotel): needs a custom descent over the run fields of `MaxSubarray`, which `maxRight` cannot express.
- Euler-tour diameter monoid (CF 1149C): Graphs owns tree diameters.
- Dynamic tree one-node-per-key sparse variant (MAS): a memory variant of the same API.
- Dynamic tree range `clear` (TKO), split by count (MAS sortable), meld on the lazy tree (NYA), dual `getAll` (MAS): rare; split by count is `split(root, maxRight(root, lo, count <= k))`; sortable trees are row 51.
- Dynamic tree vector build (legacy `DynSegTree(span)`): a dense array is row 12.
- Interval set `sameInterval`, `maxExcluded`, iterator `lowerBound` (SUI, NYA): `covers(min, max + 1)`; `contains(x) ? find(x).first - 1 : x` (none when that interval starts at the minimum of `T`); `find`/`next` cover the lookups.
- `IntervalsFast` over a fastset, splay-backed `PiecewiseConstant` with slides (MAS): row 36 owns the fastset; the sliding structure is an Esoteric candidate without a current owner.
- `IntervalMap::toArray` (MAS): `enumerate` over `[lo, hi)`.

## P227 omissions

Candidates from the 2026-10-09 sweep ([00-sources.md](00-sources.md)) left out of rows 61 and 64:

- HashMap `count` (MAS, NYA, YC): `contains` gives the same answer.
- HashMap `forEach`/`enumerate` (MAS, NYA): range-for over `begin`/`end` does it.
- HashMap `erase(iterator)` returning the next element (NYA): backward-shift erase moves later cluster members into the hole, so erase while iterating needs a different contract; rare.
- `emplace`/`try_emplace`, default-value parameter, `build(n)`, `cbegin`/`cend`, `dump` (NYA, MAS, YC): covered by `insert`, `get(k, def)`, `clear` plus `reserve`, const `begin`, and range-for.
- Shrinking on erase (NYA): capacity stays at the peak; assign a new table to release it.
- SortedList `countRange`, slice erase, `irange`/`islice`, bulk `update`, `operator==` (sortedcontainers, tatyam): `rank(r) - rank(l)`, iterators, repeated `insert` or `rebuild`, and comparing iteration.
- SortedList negative `kth` indices (tatyam, PyRival): Python convention; C++ uses `kth(n - 1 - k)`.
- Fenwick over bucket sizes (PyRival) or a positional index tree (sortedcontainers): `rank`/`kth` would gain O(log(n)) bucket lookup, but `insert` stays O(sqrt(n)) and every split or merge rebuilds the index; the RATIO sweep in [64-sortedlist.md](64-sortedlist.md) favours smaller buckets (shorter shifts) over fewer buckets, so the linear bucket scan is not the bottleneck at n <= 1e6.

## P028 omissions

Candidates from the 2026-10-10 sweep ([00-sources.md](00-sources.md#fetched-2026-10-10-p028-sweep)) left out of rows 14, 15, 16 and 30:

- Historic values under assignment (CPU monitor), under chmin with add (Luogu P6242) or chmax (UOJ 169), and the historic-max sum (OI, tjkendev): each needs four or more interacting tags; `HistoricSegTree` covers range add, and `LazySegmentTreeBeats` hosts the chmin variants as a user BeatsMonoid.
- Range gcd-with plus max/sum (yukicoder 880, HIT), chmin with a change callback (MAS), change counters and two-array `max(A_i + B_i)` (OI), max-plus tags (UOJ 164), historic sums of squares: rare; the first is a ten-line BeatsMonoid failing unless every element divides the operand.
- `LazySegmentTreeBeats` `maxRight`/`minLeft` and `values` (SUI, MAS): rare on beats trees; `prod` per prefix or row 12 when no failures are needed.
- Kinetic segment tree beats (MAS): row 45 owns it.
- `FenwickOfSegmentTrees` (former row 15 entry): `SegmentTree2DSparse::prod` answers prefix-x range-y queries at the same bound; row 11 `CompressedFenwick2D` is the group-valued variant.
- Sparse 2D `maxUp`/`minDown` searches, k-dimensional compressed trees (SUI), pluggable range trees (NYA, HIT): rare; row 43 owns range trees.
- Online rectangle-apply/point-get on registered points (MAS `Dual_FenwickTree_2D`): row 33 sweeps offline, `Fenwick2DRangeAdd` (row 11) is the dense case and `LazyKdTree` the O(sqrt(k)) online case.
- Fully online 2D trees on unknown points (NYA hash-map Fenwick, OI seg-in-seg): memory O(q log^2 U); register points offline instead.
- Merge sort tree `sumLeq`/`countSumLeq` (YC `static_range_sum_with_upper_bound`): row 17 `sumLess`; `countGreater`, `countGeq`, `countLeq` (HIT): `r - l - countLess` or `countLess` of the next value.
- Fenwick of value segment trees for O(log^2) dynamic kth (OI seg-in-bit): needs values known offline and O(n log^2 n) memory; `DynamicMergeSortTree` answers kth online in O(log n log^2 m) without either.
- Online `lng` x coordinates in `DynamicMergeSortTree` (legacy `insert2D`): compress x to slots; huge online universes belong to row 13.
- SWAG `operator[]`, separate front/back folds, pop returning the value, iterator constructors, separate value and product types (SUI, MAS, HIT): rare conveniences; `front`/`back`/`top` and `allProd` cover the uses.

## P029 omissions

Candidates from the 2026-10-13 sweep ([00-sources.md](00-sources.md#fetched-2026-10-13-p029-sweep)) left out of rows 17, 19 and 25:

- Wavelet xor queries (`kthXor`, `xorMin`/`xorMax`, xor-bounded counts; SUI, MAS): they need raw-bit levels instead of the compressed alphabet; row 26 `PersistentBinaryTrie` answers range xor queries through prefix versions.
- Wavelet `maxRight` over values with a (count, sum) predicate, multi-range descents and tree-path wavelets (MAS): rare; `kth` and `kthSum` cover the fixed-k uses, and a Graphs path-kth owner would build on multi-range descents.
- Wavelet `topK`, `rangeList`, `rangeMinK`/`rangeMaxK`, `intersect` (MitI-7): rare; the distinct values of a window are repeated `minGeq` plus `countEqual`, O(log k) each.
- Fenwick nodes in `WaveletMatrixMonoid` for invertible weights (EI, NYA): a constant-factor variant of the segment-tree nodes with the same bounds; value-range sums and the sum of the k largest are differences of `sumLess` and `kthSum`.
- Treap `prev`/`next` keys and `count` (OI, YC `ordered_set`): `kth(v, rank(v, x) - 1)`, `kth(v, upperRank(v, x) - 1)`, `lowerBound`, `upperBound`, `upperRank - rank`.
- Treap `intersect` and difference (CPA, bound stated without code), split by count (OI `split_by_rk`; `split(v, kth(v, k))` for distinct keys), key-range monoid products (SUI; row 20 owns augmented ordered maps): rare.
- ImplicitTreap `rotate`, point `apply`, `allProd`, end pushes and pops (SUI, KACTL `move`, NYA): compositions of `split`/`merge`, range `apply` and `prod`.
- `decreaseKey` on `BinomialHeap`/`LeftistHeap` and handle `erase` on the leftist, skew, randomized and binomial heaps (OI, CPA, HIT): they need parent pointers or handle-moving key swaps; `PairingHeap` and `FibonacciHeap` cover handles with better bounds.
- Heap pool `clear`, sorted dumps, lazy multiply tags, `DAryHeap` push-or-decrease (HIT, SUI, MAS, OI): a new object, popping a copy, rare, and `contains` with `push`/`decreaseKey`.
- Brodal and soft heaps: no catalog implementation.

## P033 omissions

Candidates from the 2026-10-14 sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)) left out of rows 26, 27, 28 and 33:

- Binary trie meld (OI, NYA): merging in amortized O(total nodes) needs tries sharing one node pool; small-to-large insertion gives O(n log(n) B).
- LSB-first trie with global +1 and subtree xor sums (OI): a different bit orientation for one problem family (P6623).
- kth xor pair and the count of pairs with xor below t (HIT, yukicoder 2977): the count is n `countLess` calls; the parallel descent for the kth pair is rare.
- Range xored mex (SUI): a wavelet-like structure, not a trie; rare.
- Index payloads per key and ordered enumeration (EI, NYA, MAS): a map beside the trie; enumeration is `kth` for each rank at the same bound.
- Per-query xor masks on `BinaryTrie::kth`, `countLess`, `lowerBound` (EI, MAS, SUI): `applyXor` around the query; `PersistentBinaryTrie` takes the mask because versions are immutable.
- Weighted subarray counts by minimum (MAS `count_subrectangle`): `(i - l + 1) * (r - i)` from `range`; depth or traversal arrays: no catalog offers them.
- DisjointSparseTable `maxRight`/`minLeft` (MAS): a binary search over `prod` gives the same O(log(n)) bound.
- Blocked static range product with naive in-block queries (MAS `Static_Range_Product`): O(2^LOG) in-block queries; `staticRangeProducts` answers offline in O(n + q) memory.
- Xor-indexed disjoint sparse table, 2D associative products on a segment tree (MAS): rare; row 15 covers commutative rectangle products.
- Alon–Schieber O(α(n)) semigroup queries after linear preprocessing: research-level, no contest implementation.
- Higher-order coefficient queries `[x^p y^q] f / ((1 - x)^A (1 - y)^B)` (MAS `coefficient_query_2d`): A = B = 1 and A = B = 2 are the engine's kernels; higher orders are rare.
- Online rectangle-add rectangle-sum and rectangle-add point-get on registered points (SUI, MAS): row 33 sweeps offline, row 11 `Fenwick2DRangeAdd` is the dense online case (as recorded in the P027 and P028 omissions).
- K-dimensional compressed, dynamic 2D and block-decomposed Fenwick trees (SUI, NYA, OI): online structures; row 11 would own them if a motivating problem appears.

## P034 omissions

Candidates from the 2026-10-17 sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)) left out of rows 09, 10, 34, 35 and 63:

- SqrtTree range assignment (CPA, OI `massUpdate`): an assigned block's products need x^k, which a monoid gives only by repeated squaring, so queries meeting a lazy block lose O(1); row 12 assigns ranges with O(log(n)) queries. The O(sqrt(n)) update with O(log(log(n))) queries is the same operation by another algorithm.
- SqrtTree generator build and fixed three-layer layout (MAS): construction from a vector; row 34 already stops layering at segments of at most 8 elements, which a direct fold answers.
- PersistentArray in-place set (SUI `mut_set`, MAS `make_copy = false`): writes into nodes shared with other versions. `size` (SUI): the public `n`. Node pools, reset and garbage collection (SUI, MAS): Miscellaneous row 43 owns persistent arenas. Fat-node partially persistent array (EI internals): `PartiallyPersistentDSU` keeps its own attach times. Persistent block array (OI): O(sqrt(n)) per operation.
- RollbackDSU `makeSet` and an `undo` reporting what it reverted: no catalog has them. Union by rank (CPA): `getSize` needs sizes. History-clearing `snapshot` (EI, SUI) and `reset` (MAS, HIT): `rollback` to a token and `rollback(0)`.
- PersistentDSU `groups(version)` and `dump` (SUI): `findSet` over every vertex; debug output. Offline persistent DSU by DFS over the version tree (the usual Library Checker solution): a use of rows 09 and 63.
- Priority-queue undo (Codeforces blog 111117) and deque undo (blog 154645): both tutorials returned 403, so neither algorithm nor its amortized proof could be checked; `OfflineDeletion` covers deletions with known times and `QueueUndo` the FIFO case.

## P035 omissions

Candidates from the 2026-10-23 sweep ([00-sources.md](00-sources.md#fetched-2026-10-23-p035-sweep)) left out of rows 29, 36 and 65:

- Persistent weighted DSU: no catalog has one.
- Component sums under a component-wide action (suisen `UnionFindLazy`): needs an action on sums that merges can cancel; rare. `DualDSU` reads single values, `ComponentSumDSU` folds without actions. Monoid-only component actions (chmax) are left out too: `DualDSU` cancels the absorbing root's tag with `inv`.
- Delete and move element (OI Wiki virtual nodes, UVa 11987) and grid adapters with a border merge (hitonanode `grid_unionfind`): a move needs a group to remove the old value from its component; a grid is `r * w + c` and a loop.
- Merges of one batch as a single time step (suisen `merge_simultaneously`): `MergeHistoryForest` stamps each union with its index; a caller maps indices to steps.
- FastSet `clear`/`reset` (maspypy, hitonanode): constructing a new set does the same. `fillOne`: `build` with a true predicate. `toString`: debug output. `any(l, r)`: `next(l) < r`. Range erase: `enumerate` with `erase` in the callback. Counting and k-th: Fenwick `lowerBound` (row 02).
- Arpa's offline RMQ (cp-algorithms, a DSU over next-smaller links): rows 04 and 28 answer the same queries online.

## P030 omissions

Candidates from the 2026-10-27 sweep ([00-sources.md](00-sources.md#fetched-2026-10-27-p030-sweep)) left out of row 18 (user decision 2026-10-27; pool reset, copy with an action, `kthMax`, `countLess` and `sumLess` were added instead):

- Persistent Fenwick tree (SUI): the left-sum tree with path copying is `PersistentSegmentTree<Sum<lng>>` at the same bounds; a scratch measurement (2026-10-27, n = q = 2 * 10^5, half point adds, half range sums on random versions) made 1.86M nodes either way in about 65 ms against 64 ms.
- k-th smallest on a tree path (SPOJ COT, versions per vertex combined through the LCA): Graphs owns tree adapters; filed as P248 (Graphs row 88).
- Position-dependent defaults for untouched ranges (legacy `defR(l, r)`, row 13's `fill` functor) in `PersistentSegmentTree`: untouched ranges would hold `fill` instead of `e()`, which breaks the null-node shortcuts of `merge`, `split` and retagging; the fill build `(n, x)` covers uniform defaults on any `lng` universe and a vector build covers the rest up to memory.

## P032 omissions

Candidates from the 2026-10-28 DS11 sweep ([00-sources.md](00-sources.md#fetched-2026-10-28-p032-ds11-sweep)) left out of rows 21 and 22 (user decision 2026-10-28; floating-point lines were added to row 22 instead):

- Minimum over an x-interval (Nyaan, monotone lines only) and lazy additive tags on the envelope (maspypy `extended_lichao`; CF 1120F named there, not fetched): rare; range linear add with range minimum is row 45.
- Real x coordinates (cp-algorithms): `LiChaoTreeCompressed` takes any finite point set of `lng`; real points need a rank mapping inside `F::eval`, which a stored function cannot capture.
- Line deletion: `undo` with row 63 `OfflineDeletion`; node-pool reset (maspypy): a new object.
- Rollback CHT with binary-search insertion (CEOI 2009 Harbingers, named by the sweep, not fetched; CF blog 51684 returned 403): `LiChaoTree` and `LiChaoTreeCompressed` with `UNDO` answer the same tree DP at O(log(C)) per operation.
- `empty`, `size`, `clear` (ei1333, maspypy): the public deque, a new object. Merging two monotone hulls (hitonanode, unverified there), single-crossing functions in a monotone CHT (maspypy `CHT_Monotone_F`): one catalog each; the Li Chao trees cover general functions.
- Max of `a x + b y` over points (maspypy `CHT_XY`, Nyaan `LineContainer2D`) and static envelope pieces (maspypy `line_min_function`): Geometry rows 20 and 41.

## Planned-row omissions

- Row 48 `PersistentSkewHeap`: skew-heap melds are only amortized, and persistence lets an expensive version be reused, so no bound holds; `PersistentLeftistHeap` gives persistent melds in O(log n).

## Test matrix

- Differential-test Fenwick/segment-tree variants against a vector; persistent versions against copied snapshots; dynamic-tree structures against small forests; sorted containers against a sorted vector; hash maps against `std::unordered_map` with adversarial key patterns.
- Also test repeated update/rollback sequences, noncommutative action order, version branching and failed merges.
