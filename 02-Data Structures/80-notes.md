# 02 Data Structures — notes

Contracts, ownership boundaries and migration obligations moved out of the inventory. Operation lists live only in [00-index.md](00-index.md); evidence for the verified Basic rows is in [90-foundations.md](90-foundations.md).

## Status vocabulary

- `verified`: every operation in the row has an independent-oracle test and the recorded commands pass.
- `partial`: a formerly verified row that gained operations; the evidence link still covers the original API and `missing:` names the rest.
- `existing-unverified`: an unchanged monolith extraction compiles but its semantics, indexing, completeness and complexity claims are unaudited; its feature list is future audit/build work.
- `legacy-reference`: analogous code exists in `97-Legacy/` or `OLD/` and must be migrated and verified; the target header is absent.
- `planned`: new implementation needed.

## Common contracts

- Canonical indices are zero-based and ranges are half-open; empty ranges return the identity. Legacy inclusive adapters keep their names where they exist (`sum1`, `queryFast`, `querySlow`).
- State indexing, boundary inclusivity, empty-range identity, key duplicates, signed coordinates, overflow policy and the operation algebra (associativity, identity, action homomorphism, commutativity) for every structure.
- State static versus online/offline, update/query memory, amortized versus worst-case bounds, and whether balancing is randomized or deterministic. Amortized bounds must be re-proved under persistence and rollback; ephemeral amortization does not transfer.
- Generic arithmetic must keep every stored value and intermediate representable or be explicitly modular; `lng` is a convenient sum type, not an overflow check.
- Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants. Structures are single-threaded, own their storage and copy as independent snapshots.
- All-solution and enumeration APIs return explicitly bounded enumerations charged by output size (`groups`, `members`, `mergedPairs`, `intervals`).
- Handles (heap nodes, multiset tokens, versions) state their invalidation rules; tokens from other histories must never be passed back.

## Numeric and algebraic policies

- Fenwick variants require an additive commutative group; prefix-monoid Fenwick requires monotone updates (max/min) and has no range subtraction.
- Segment-tree searches need a deterministic predicate true on the identity whose truth forms a prefix as the interval grows.
- Sparse-table O(1) queries need idempotence; `fold` and disjoint sparse tables need only associativity.
- Lazy actions compose new-after-old; the preset monoids in `00-monoids.hpp` define `idS`, `idF`, `defR`, `init`, `ope`, `map`, `cmp` and length-aware mappings. MonGcd's `opt` template switches the lcm field on.
- Li Chao and convex hull trick must state an overflow-safe evaluation/intersection policy (128-bit products or bounded domains) and tie rules for equal lines.
- Segment-tree beats variants beyond chmin/chmax/add/assign need their own amortized proof and value-domain statement (division, sqrt, modulo).
- ODT/Chtholly workloads must state adversarial worst-case degeneration; kinetic structures must distinguish adversarial event counts from ordinary update bounds.
- Hash maps use a process-seeded SplitMix hash; Miscellaneous `02-customhash.hpp` owns the hash functions and seeding policy.

## Ownership boundaries

- Data Structures owns data-structure engines. Graphs owns dynamic-connectivity orchestration (`26-dynamic_connectivity.hpp`), HLD, LCA and tree adapters; Graphs `48-matroidintersection.hpp` consumes the oracles of `46-matroid_oracle.hpp` and Mathematics linear algebra.
- Geometry owns point-facing kd/R/quadtree and nearest-neighbour APIs (`28-spatial_index.hpp`), rectangle-union area/perimeter (`13-segment_union.hpp`), kinetic geometry and dynamic/decremental hulls; it may use `43-rangetree.hpp` and `33-offline_rectangle_queries.hpp` as engines.
- Strings owns literal-prefix and radix tries; this folder owns binary integer/XOR tries and succinct rank/select engines used by Strings `36-compressed_text_index.hpp`.
- Miscellaneous owns CDQ, parallel binary search, Hilbert order, generic undo log/checkpoints, persistent allocator, slope trick, coordinate compression by encounter ID, interval algorithms over static endpoint sets and probabilistic sketches. `63-offline_deletion.hpp` owns the generic add/remove-over-time engine and the queue-undo trick because its contract is the rollback structure, not a graph.
- Core owns reusable accelerated dynamic bitsets and GF(2) matrices; `62-xorbasis.hpp` owns the incremental basis, prefix (timestamped) basis and offline range basis queries. Mathematics owns Sprague–Grundy mex; `24-interval_set.hpp` owns dynamic mex over a set.
- Python mirrors (`08-Python`) own their own sorted-list and xor-basis implementations.
- Rollback DSU owns the generic undo/connectivity substrate; weighted DSU owns potential/parity/noncommutative algebra and the weighted rollback variant. Reuse the undo contract without reciprocal header dependencies. `65-dsu_extensions.hpp` holds member enumeration, component aggregates, sparse keys and the merge-history forest; it depends on `01-dsu.hpp` only.
- Dynamic wavelet and succinct structures are Esoteric rows, never extra modes hidden inside the Advanced wavelet header. Persistent split/merge stays with the persistent segment tree; destructive meld/split stays with the dynamic segment tree.
- Heap and deque engines are consumed by Graphs Dijkstra, Miscellaneous optimal merge and k-shortest walks; those clients do not reimplement heaps.

## Migration obligations

- `OLD/algorithms.cpp:1402–1477` DSU mixed connectivity, weighted distances, parity and edge counts, and narrowed `lng` to `int`. Basic `DSU` keeps `n/ncon/par/siz`; weighted unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz`, `isBipartite` belong to `29-weighteddsu.hpp` (potential/parity) and `09-rollbackdsu.hpp`/Graphs (component metadata).
- Unchanged extractions (`11`, `12`, `14`–`18`, `21`, `23`, `00-monoids.hpp`) keep original bodies and comments; compilation proves nothing about semantics. Audits must strip hidden template globals and compile each header alone. Mixed-operation bodies (`FenTree` 1D/2D/3D, `SegTree2D` with lazy monoid, `DynMergeSortTree` 1D/2D) are split during audit; the Basic `Fenwick` and `SegmentTree` names were chosen to avoid collisions with `FenTree` and lazy `SegTree`.
- `97-Legacy/02-sqrtdecomp.cpp` (invalid defaults, clamped inclusive ranges, unused lazy array) is superseded by `06-sqrt_decomposition.hpp`; only its diagnostic printer remains unported. `97-Legacy/03-dynsegtree.cpp` fails with Core `lng=int64_t` (`max(l, 0LL)`), `97-Legacy/04-lichao_older.cpp` is a query-less hull skeleton, `97-Legacy/05-mergesorttree_older.cpp` is the static fractional-cascading reference, `97-Legacy/01-monset.cpp` is a placeholder scaffold. Delete a legacy file only after its owning row accounts for every feature.
- `OLD/Team Notebook/src/ds/impltreap.cpp` and `cartesiantree.cpp` are the legacy references for rows 19 and 27; `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` contain `DynamicMex` (row 24) and `LcaO1` (row 54 bridge).
- The sparse-table public `v` storage changed from a flat padded array to valid rows; clients use query methods.

## Test matrix

- Differential-test Fenwick/segment-tree variants against a vector; persistent versions against copied snapshots; dynamic-tree structures against small forests; sorted containers against a sorted vector; hash maps against `std::unordered_map` with adversarial key patterns.
- Test negative values, empty structures, repeated update/rollback sequences, noncommutative action order, version branching and failed merges.
- Record domains, omissions, provenance, test commands with results and benchmark conditions in a `9x-*.md` evidence document per package, as [90-foundations.md](90-foundations.md) does for `01`–`08`.
