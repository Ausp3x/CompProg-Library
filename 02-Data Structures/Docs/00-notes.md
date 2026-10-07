# 02 Data Structures — notes

Contracts, ownership boundaries and migration obligations moved out of the inventory. Operation lists live only in [00-index.md](../00-index.md); evidence for the verified Basic rows is in the per-header documents ([01-dsu.md](01-dsu.md) … [08-monotone_stack.md](08-monotone_stack.md)); their shared contracts are under [Foundations rows 01–08](#foundations-rows-0108-p006).

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
- DSU grid adapters, delete/move element (HIT, OI): variants for row 65 if a motivating problem appears.

## Foundations rows 01–08 (P006)

Folder-level record for the verified Basic rows `01-dsu.hpp` … `08-monotone_stack.hpp` (package P006, DS01 then DS02 order), moved from the former package document. Per-header evidence: [01-dsu.md](01-dsu.md), [02-fenwick.md](02-fenwick.md), [03-segmenttree.md](03-segmenttree.md), [04-sparsetable.md](04-sparsetable.md), [05-prefix_sum.md](05-prefix_sum.md), [06-sqrt_decomposition.md](06-sqrt_decomposition.md), [07-ordered_set.md](07-ordered_set.md), [08-monotone_stack.md](08-monotone_stack.md). C01/P002 supplies the verified template, integer aliases and PBDS imports. Advanced structures retain their separate inventory owners.

### Common foundation contracts

Canonical indices are zero-based and ranges are half-open. Empty construction is supported. Invalid indices, sizes, shapes and required predicate identities are preconditions checked by assertions; they remain preconditions under `NDEBUG`. A valid absent result has a documented sentinel. Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants.

Generic arithmetic must satisfy the stated algebra and keep every stored value and intermediate representable, or use explicitly modular arithmetic. `lng` is a convenient sum type, not an overflow check. Complexity counts constant-cost element operations/comparisons/copies; strings and large integers add their own costs. Allocation failure is ordinary C++ allocation failure. Structures are single-threaded and own their storage; copying produces an independent snapshot. Associativity, idempotence, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

A moved-from structure may only be assigned to or destroyed. Its size fields keep their old values while its storage is empty, so every other call is a precondition violation. Assignment restores every invariant (re-audit finding 1, resolved by contract; see [02-fenwick.md](02-fenwick.md)). Structures that have both a size constructor and a vector constructor (`Fenwick`, `SegmentTree`, `PrefixSum`, `DifferenceArray`, `SqrtDecomp`, `SqrtRangeSum`) also take an `std::initializer_list<T>`. A braced list therefore always means values, as with `std::vector`: `PrefixSum<lng> p({7})` and `PrefixSum<lng> p{7}` hold the single value 7, and `PrefixSum<lng> p(7)` holds seven zeros. Elements of `T = bool` are read by value through `vector<bool>::const_reference` wherever a reference to storage is returned.

### Test infrastructure

Each header has one runnable Python entry and one C++ oracle suite under `96-Local Testing/02-Data Structures`, with matching filename stem. The shared `_00_runner.py` resolves paths independently of the working directory. Checks are non-removable under `NDEBUG`. Failures report seed, mode, configuration/command, operation/input or history and expected/actual values; invalid probes require SIGABRT with assertion diagnostics. Exhaustive enumerations stop on the first discovered failing case; no automatic shrinking claim is made.

Quick modes reduce enumeration lengths/history depths/random counts and run optimized plus checked builds. Full runs every feature in optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`; the ordered suite uses `_GLIBCXX_ASSERTIONS`, see [07-ordered_set.md](07-ordered_set.md)), and sanitizer (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`) builds, with leak checking enabled. Since the re-audit every configuration compiles with `-Wall -Wextra -Wconversion -Werror`. Stress modes extend the enumerations/random counts; each suite's source records its concrete mode limits. Declared algebra/monotonicity/representability conditions and impractically large allocations are justified contracts, not runtime proof by finite tests.

### Package verification history

2026-09-27 (original P006): all eight entries passed `--mode full --seed 20260927` (24 configuration runs, all 100 assertion probes). DS01 completed its full runs and Basic/All multiple-TU check before DS02 implementation began. The per-header full suites supplied the P006 sanitizer checks; unrelated Core sanitizer corpora were not repeated. Integration passed 65 standalone/aggregate headers; consistency passed inventory/batch/package paths, aggregates, source/archive hashes and Markdown links with zero errors. Original archived bytes remain unchanged. Stress mode was available but not run as completion evidence. GCC14 itself, other standard-library versions and other platforms were not executed. No online submissions or acceptance claims were made.

2026-10-07 (re-audit): the 24 confirmed findings in `00-Guidelines/23-Reaudit Findings/p006.md` were fixed or resolved; each per-header document lists the findings that concern it. The feature-map comments at the top of four testers and one benchmark comment line moved to the feature-to-test maps. 112 assertion probes remain, up from 100. `@reviewer` confirmed findings 2–24 fixed and accepted the contract-based resolution of finding 1 after a wording correction; it raised five findings (`SegmentTree::apply` operand order, the Fenwick `maxRight` skip guard, SparseTable2D ragged rows with an empty first row, restated contract comments in the Fenwick and SegmentTree bodies, the Fenwick domain line), all fixed and recorded per header, and accepted one shared complexity line per adjacent function pair. Final full run (seed 20261007): 8 suites x 3 configurations, 4 min 10 s; stress seeds 7 and 8 over two rounds (21 min) before the review fixes; stress seed 9 for the three suites changed by the review. Commands and per-header results are in each per-header document.

### Provenance and ownership

Original files in `OLD` and `97-Legacy` remain byte-for-byte references. The historical monolith map retains source ranges/hashes and distinguishes rewritten destinations from unchanged extractions. Basic aggregates gain all eight headers; existing Advanced headers remain existing-unverified. Future variants remain explicit: rollback/persistent/weighted DSU, range-add/multidimensional Fenwick, lazy/dual/persistent/dynamic segment trees, disjoint sparse tables, Cartesian trees and linear-preprocessing RMQ, Mo and recursive sqrt trees, dynamic balanced/persistent ordered trees, and general sliding-window aggregation. Their existence is not a gap in these Basic ownership batches. No ISA kernels, Barrett/Montgomery use or empirically selected dispatch thresholds were introduced; the families use their standard contest algorithms with stated preprocessing/storage tradeoffs, and no universal performance superiority is claimed.

Source inspection and independent oracle tests are separate evidence. Implementations are written for the contracts; legacy code and reference algorithms inform the audit. The [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf) (saved repository edition) chapters 4, 8, 9, 15 and 27 cover PBDS, sliding windows, prefix/difference arrays, Fenwick/segment structures, DSU and block decomposition; [KACTL](https://github.com/kth-competitive-programming/kactl) (saved repository PDF) supplied the UnionFind, FenwickTree, SegmentTree, RMQ, SubMatrix and OrderStatisticTree references. Saved resource editions/checksums are recorded in `95-Resources/99-sources.json`. References were inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun and Tarjan are not claims that their original proofs were inspected. No online acceptance is claimed.

## Test matrix

- Differential-test Fenwick/segment-tree variants against a vector; persistent versions against copied snapshots; dynamic-tree structures against small forests; sorted containers against a sorted vector; hash maps against `std::unordered_map` with adversarial key patterns.
- Test negative values, empty structures, repeated update/rollback sequences, noncommutative action order, version branching and failed merges.
- Record domains, omissions, provenance, test commands with results and benchmark conditions in one `Docs/<NN-name>.md` evidence document per header, as [01-dsu.md](01-dsu.md) … [08-monotone_stack.md](08-monotone_stack.md) do for `01`–`08`.
