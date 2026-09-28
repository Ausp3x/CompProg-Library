# 02 Data Structures — inventory and extraction status

This is a feature inventory. **Existing-unverified** rows now have unchanged extracted headers; their feature checklists still describe future audit/build work. **Planned** means new implementation needed; **legacy-reference** means analogous code exists in `algorithms.cpp` or the Team Notebook and still needs migration and verification. All eight DS01/DS02 Basic foundations are **verified** with [P006 evidence](90-foundations.md); other statuses remain explicit below. Basic/Advanced/Esoteric are sections of this index, never directory names or mixtures of tier headers. Each family gets a general underscore filename; a standalone algorithm gets a lower-case abbreviation filename. Proposed prefixes put common, high-value families first, then dependency order; `98`/`99` remain reserved for aggregate/reference material. Split larger families into topic subfolders if needed.

## Basic

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `01-dsu.hpp` | Path compression + union by size, component sizes/count and ordered member grouping; vertices [0,n). Parity/potential belongs to Advanced weighted DSU. | Verified DS01; [contracts, feature tests and evidence](90-foundations.md). |
| `02-fenwick.hpp` | O(n) build, point add, half-open prefix/range sum, nonnegative-frequency lowerBound; explicit one-based adapters and absence sentinels. | Verified DS01; [contracts, feature tests and evidence](90-foundations.md). |
| `03-segmenttree.hpp` | Iterative ordered monoid tree, point get/set, empty/range/all folds and maxRight/minLeft boundary searches; lazy actions are separate. | Verified DS01; [contracts, feature tests and evidence](90-foundations.md). |
| `04-sparsetable.hpp` | Idempotent O(1) query, associative O(log(n)) ordered fold, half-open canonical and inclusive legacy queries; disjoint O(1) nonidempotent variant is Advanced. | Verified DS01; [contracts, feature tests and evidence](90-foundations.md). |
| `05-prefix_sum.hpp` | 1D/2D prefix sums and offline difference arrays/imos; half-open ranges, rectangular/empty dimensions, clear/rebuild and non-destructive materialization; explicit arithmetic-fit contract. | Verified DS02; [contracts, feature tests and evidence](90-foundations.md). |
| `06-sqrt_decomposition.hpp` | Ordered monoid point updates/folds and rebuilding with configurable blocks; scalar affine range add/assign/multiply/sum with explicit tag order and const reads; Mo is Advanced. | Verified DS02; [contracts, feature tests and evidence](90-foundations.md). |
| `07-ordered_set.hpp` | Comparator-aware coordinate compression and duplicate-preserving sorted-vector rank/select; PBDS unique set and multiset with exact occurrence tokens, stable duplicate order and lifecycle contracts. | Verified DS02; [contracts, feature tests and evidence](90-foundations.md). |
| `08-monotone_stack.hpp` | Strict/nonstrict next/previous greater/smaller; sliding min/max with equal-index tie policy; full-width histogram and maximal binary 0/1 rectangle area/witness; legacy arbitrary-nonzero zero-matrix blockers. Cartesian trees are Advanced. | Verified DS02; [contracts, feature tests and evidence](90-foundations.md). |

## Advanced

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `09-rollbackdsu.hpp` | Undoable DSU with explicit snapshots, component aggregates and contradiction counts; parity/potential shares weighted-DSU algebra. Canonical time-segment traversal/orchestration is Graphs dynamic connectivity. | Planned; [P006 legacy edge-count/component metadata handoff](90-foundations.md#migration-and-ownership-boundaries). |
| `10-persistentdsu.hpp` | Partially persistent and fully persistent tradeoffs, versioned connectivity/size, immutable branching versions; use persistent-array storage where appropriate. | Planned |
| `11-fenwick_tree_advanced.hpp` | Range-add/range-sum, range-add/point-get, multidimensional 2D/3D and offline compressed BIT/Fenwick of vectors, linear construction and monotone prefix search where valid; rectangle sweep APIs are separate. | Existing-unverified; unchanged `FenTreeRangeAdd1D`, `FenTree` excerpts. Remaining listed features await audit/build. |
| `12-lazysegmenttree.hpp` | ACL-style monoid action, composition order, assignment/add/affine cases, max-right/min-left; dual range-action/point-get variant, segment-length-aware mappings and noncommutative folds. | Existing-unverified; unchanged `SegTree` excerpts. Remaining listed features await audit/build. |
| `13-dynamicsegmenttree.hpp` | Sparse coordinate domain, node allocation and pruning, persistent path-copying, lazy tags; destructive meld and range/key split with explicit leaf-combination algebra, consumed-root ownership and allocation-charged amortized bounds. Persistent split/merge remains with the persistent segment tree. | Legacy-reference; `DynSegTree`, `PerSegTree`. |
| `14-segtreebeats.hpp` | Chmin/chmax/add/assign/sum/min/max and historical minima/maxima; modulo/division/sqrt-style pruning variants only with distinct amortized proof and value domain, adversarial checks. | Existing-unverified; unchanged `SegTreeBeats` excerpts. Remaining listed features await audit/build. |
| `15-segment_tree_2d.hpp` | Dense/sparse 2D trees and Fenwick-of-segment-trees, compressed online point updates, point-set/rectangle affine/rectangle sum domains and monoid actions; fully dynamic point insertion distinguishes preknown coordinates. | Existing-unverified; unchanged `SegTree2D` excerpts. Remaining listed features await audit/build. |
| `16-mergesorttree.hpp` | Static count/order stats, dynamic ordered containers, fractional cascading. | Existing-unverified; unchanged `DynMergeSortTree` excerpts. Remaining listed features await audit/build. |
| `17-waveletmatrix.hpp` | Static rank/select/quantile/range frequency, predecessor/successor, compressed signed alphabets, weighted sums and range kth-by-weight where meaningful. Fully dynamic variants have an Esoteric target. | Existing-unverified; unchanged `WaveletMatrix` excerpts. Remaining listed features await audit/build. |
| `18-persistentsegmenttree.hpp` | Kth order statistic, range count/sum, persistent lazy actions, sparse/dense versions, functional split/merge when algebra permits, version-difference and memory-lifetime contracts. | Existing-unverified; unchanged `PerSegTree`, `KSPerSegTree` excerpts. Remaining listed features await audit/build. |
| `19-treap.hpp` | Key and implicit treaps, split/merge, lazy reverse/affine, deterministic seed option. | Legacy-reference; Team Notebook `impltreap.cpp`. |
| `20-balanced_bst.hpp` | Splay, AVL/red-black/AA/size-balanced/weight-balanced, scapegoat and skip-list alternatives; ordered set/multiset and implicit sequence/rope rank/select, split/join and rebuild rules. Compare contest value and preserve each planned alternative explicitly. | Planned |
| `21-lichao.hpp` | Minimum/maximum lines, compressed/dynamic domains, line segments, persistent queries and offline deletion/rollback; exact overflow-safe evaluation/intersection policy. | Existing-unverified; unchanged `LiChaoTree`, `PerLiChaoTree` excerpts. Remaining listed features await audit/build. |
| `22-convexhulltrick.hpp` | Monotone-slope/query deque hull and arbitrary-slope dynamic line container, exact intersections/ties, min/max and query witnesses; Li Chao reuses its canonical header. | Planned |
| `23-range_query_offline.hpp` | Mo with Hilbert order, updates/time dimension, rollback Mo and tree-path adapters; reusable add/remove/undo contract. Parallel binary search/CDQ remain canonical Miscellaneous implementations. | Existing-unverified; unchanged `Mo` excerpts. Remaining listed features await audit/build. |
| `24-interval_set.hpp` | Disjoint interval set/map, assign/add/split/merge, union length and dynamic mex; ordered disjoint tree (ODT/Chtholly) workloads must state adversarial worst-case degeneration. | Planned |
| `25-heap_deque.hpp` | Compact ring-buffer deque, binary/d-ary indexed/decrease-key and erasable heaps; leftist/skew/pairing/binomial/Fibonacci meldable variants, min-max queue, two-heaps median and top-k sums. Specify handles and invalidation, per-operation worst-case/amortized bounds and practical tradeoffs. Sliding extrema reuse Basic monotone deque; radix/persistent heaps are separate. | Planned |
| `26-trie.hpp` | Integer binary trie for xor-min/max, count-below-xor, order statistics, signed-key encoding, lazy xor, multiplicities and persistence; literal-prefix/string trie is canonical in Strings. | Planned |
| `27-cartesiantree.hpp` | Min/max Cartesian tree, linear stack build, RMQ/LCA bridge. | Legacy-reference; `cartesiantree.cpp`. |
| `28-disjointsparsetable.hpp` | Associative nonidempotent range query in O(1) after preprocessing; empty-range identity. | Planned |
| `29-weighteddsu.hpp` | Parity and potential differences over an explicit group, noncommutative orientation/composition, contradiction detection and component consistency; rollback variant reuses the rollback DSU contract. | Planned; sources: YC, MAS. [P006 legacy weighted/parity handoff](90-foundations.md#migration-and-ownership-boundaries). |
| `30-aggregation_queue.hpp` | SWAG queue and deque for associative, possibly noncommutative folds; push/pop at supported ends, empty identity, amortized rebuilding and chronological composition. | Planned; sources: YC, NYA, SUI. |
| `31-radixheap.hpp` | Monotone unsigned integer-key priority queue, equal keys, duplicate payloads and 32/64-bit bounds; graph Dijkstra is a client, not another heap implementation. | Planned; sources: NYA, HIT. |
| `32-static_range_queries.hpp` | Static distinct count, frequency, mex and majority/threshold-frequency witnesses; offline sweep/persistent alternatives with exact versus probabilistic guarantees. Dynamic point-set/range-frequency reuses the merge-sort tree. Quantile and range sums reuse wavelet/segment structures. | Planned; sources: YC, MAS. |
| `33-offline_rectangle_queries.hpp` | Point-add/rectangle-sum, rectangle-add/point-get and rectangle-add/rectangle-sum with static/offline event sweeps, coordinate compression, endpoint/tie policies; reusable count/report engine for geometric clients. | Planned; sources: MAS, YC. |
| `34-sqrttree.hpp` | Sqrt tree for associative static range folds, build/query tradeoffs and optional point-update variant; distinguish this recursively layered structure from ordinary sqrt decomposition. | Planned; sources: OI, MAS. |
| `35-persistentarray.hpp` | Path-copying persistent array and versioned point set/get; branch/version ownership, batch initialization and node allocation, reusable by fully persistent DSU. | Planned; sources: NYA, SUI. |
| `36-fastset.hpp` | Fixed-universe hierarchical bitset predecessor/successor, insert/erase/membership and interval enumeration; word boundaries, empty universe and rank support only when explicitly implemented. | Planned; sources: HIT, MAS, YC. |

## Esoteric

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `37-lct.hpp` | Dynamic forest link/cut/evert/connectivity/LCA, directional path fold/actions, path kth, reversal tags, virtual subtree aggregates and exact invariant tests. | Planned |
| `38-eulertourtree.hpp` | Balanced-sequence dynamic connectivity and subtree aggregates; avoid confusing static Euler tours with this structure. | Planned |
| `39-toptree.hpp` | Static top tree/tree DP point updates and fully dynamic cluster aggregates, diameter and subtree/path updates; separate invariants and complexity for static versus dynamic variants. | Planned |
| `40-succinctbitvector.hpp` | Rank/select with sampled blocks; wavelet tree/matrix integration and memory accounting. | Planned |
| `41-succincttree.hpp` | Balanced parentheses/LOUDS, preorder/subtree navigation and static RMQ reductions. | Planned |
| `42-vanemdeboas.hpp` | Integer predecessor choices: vEB, x-fast/y-fast trie, fusion-tree theory versus realistic 64-bit alternatives. | Planned |
| `43-rangetree.hpp` | Rank-compressed orthogonal range counting/reporting, priority search tree and fractional cascading; Geometry owns point-facing kd/R/quadtree/nearest-neighbor APIs, using this engine when appropriate. | Planned |
| `44-retroactivequeue.hpp` | Partially/fully retroactive priority queue and persistence relationships, only after a motivating task. | Planned |
| `45-kineticheap.hpp` | Kinetic tournament/heap under linear motion, range-linear-add/range-min specialization, event validity and precision policy; distinguish adversarial event counts from ordinary segment-tree update bounds. | Planned |
| `46-matroid_oracle.hpp` | Independence oracle, exchange graph utilities, intersection bridge to graph algorithms. | Planned |
| `47-persistentqueue.hpp` | Fully persistent queue/deque, branching versions, concatenation only if supported; distinguish amortized ephemeral bounds from bounds valid across persistent branches. | Planned; sources: YC, NYA, SUI. |
| `48-persistentheap.hpp` | Persistent meldable leftist/skew heap with immutable versions and optional lazy key offsets; merge/delete-min, ownership and sharing bounds. Supports k-shortest-walk sidetrack heaps. | Planned; sources: EI, OI. |
| `49-dynamicbitvector.hpp` | Insert/erase/update plus access/rank/select in a mutable packed bitvector; blocked balancing, rank metadata and bit-width boundary invariants. General dynamic bitset arithmetic belongs to Core. | Planned; sources: DYNAMIC. |
| `50-dynamic_wavelet.hpp` | Dynamic wavelet tree and matrix variants: sequence access/insert/erase/update, rank/select/quantile, frequency and weighted range queries when supported; explicit dynamic-bitvector dependency and version/memory costs. DYNAMIC implements the wavelet-tree approach; its matrix alternative is explicitly still a research target. | Planned; sources: DYNAMIC. |
| `51-sortablesegmenttree.hpp` | Range sort ascending/descending with range aggregate, point assignment and split/merge; key duplicates, stable ordering policy and noncommutative aggregate direction. | Planned; sources: YC, MAS. |
| `52-rangeparallelunionfind.hpp` | Bulk equal-length interval unions, enumeration of newly merged pairs and amortized progress accounting; ordinary/rollback variants only with separately justified bounds. | Planned; sources: YC, NYA. |
| `53-rangemode.hpp` | Exact static range mode and least-frequency variants when supported, tie/witness rules, preprocessing versus query/memory tradeoffs; keep approximate heavy-hitter sketches in Miscellaneous. | Planned; sources: YC, SUI, KOO. |
| `54-linearrmq.hpp` | Fischer–Heun/Farach–Colton–Bender linear-preprocessing static RMQ and ±1 RMQ microblocks, Cartesian-tree reduction and tie policy; retain compact sparse-table alternative. | Planned; sources: CPA, MAS. |
| `55-commonintervaltree.hpp` | Common-interval decomposition tree of a permutation, strong/common intervals and linear/prime node semantics; distinguish permutation intervals from geometric interval sets. | Planned; sources: YC, MAS, SUI. |
| `56-pqtree.hpp` | Consecutive-ones constraints via PQ trees, reduction/ordering certificates and failed constraints; PC-tree/circular-ones alternative documented separately. | Planned; sources: OI. |
| `57-fingertree.hpp` | Measured finger-tree sequence with end operations, concatenation, split/search by accumulated monoid and persistent sharing; amortized bounds must remain valid under persistent branches. | Planned; sources: OI. |
| `58-persistent_bst.hpp` | Fully persistent ordered and implicit balanced trees, reversible lazy updates, split/join/concatenate and version lifetime; immutable sharing and amortized/expected bounds distinct from ephemeral balancing. | Planned; sources: EI, SUI. |
| `59-range_sequence_queries.hpp` | Static range inversion count and static range LIS queries; offline/batched and preprocessing/query tradeoffs, strict versus non-strict LIS, duplicate values and reconstruction where feasible. | Planned; sources: YC, TKO, SUI. |
| `60-succinct_sequence.hpp` | Compressed integer sequences with searchable partial sums and insert/delete/update, gap-encoded sparse vectors and blocked/B-tree representations; bit-space accounting and integer-domain assumptions. | Planned; sources: DYNAMIC. |

## Contract and test matrix

- Specify indexing, half-open ranges, inclusive boundaries, empty-range identity, key duplicates, signed coordinates, overflow, and operation algebra (associativity, identity, action homomorphism).
- Record static versus online/offline status, update/query memory, amortized versus worst-case cost, and whether randomized balancing is expected or deterministic.
- Differential-test Fenwick/segment-tree variants against a vector; persistent versions against copied snapshots; dynamic-tree structures against small forests; test negative values, empty trees and repeated update/rollback sequences.
- Avoid pretending a legacy monolith `struct` has a standalone API. Migration must strip hidden template globals and compile each header alone.

## Unchanged monolith extraction

See [local legacy references](97-Legacy/00-index.md) for older, incomplete or integration-blocked variants and [the transfer record](../00-Guidelines/14-monolith-transfer.md) for scope and build evidence. Original bodies and old comments were preserved; compilation does not verify semantics, completeness, performance, indexing or complexity claims. The source-range/hash map remains authoritative for provenance.

`00-monoids.hpp` is temporary shared legacy support (`MonAlg`, `MonBin`, `MonGcd`, `MonSar`), outside algorithm tier ordering and owned by the lazy-segment-tree work batch. It is in All, not Basic. `FenTree` includes multidimensional operations and is placed in Advanced `11-fenwick_tree_advanced.hpp`; the ordinary Basic Fenwick target remains planned. `SegTree` has lazy actions and belongs in `12-lazysegmenttree.hpp`; the Basic iterative segment tree remains planned. Existing mixed-operation bodies await splitting during future audits. `SqrtDecomp` and `DynSegTree` remain reference-only because they do not compile unchanged with current declarations; their target headers remain absent.

## Completeness audit and ownership (2026-09-27)

The audit compares named families and public-operation catalogs, not source-code correctness. All new entries remain **planned**; extracted algorithm bodies and existing statuses are unchanged. This is a broad, extensible inventory, not a claim that all algorithms in every publication have been enumerated. Exact source paths and read scope are recorded in the audit record.

Canonical ownership: this folder owns data-structure engines; Graphs owns dynamic-connectivity orchestration and graph adapters; Geometry owns geometric point-query APIs; Strings owns literal-prefix tries; Miscellaneous owns CDQ/parallel binary search and hashing; Core owns reusable accelerated dynamic bitsets. Dynamic wavelet and succinct rank/select structures are Esoteric rather than extra modes hidden in a common Advanced header.

For precise per-family read scope and caveats, see [the completeness audit](../00-Guidelines/16-inventory-audit.md).

The final scope review made destructive sparse segment-tree meld/split and heap alternatives explicit under their existing owners. The saved [OI Wiki merge/split navigation entry](https://oi-wiki.org/ds/seg-merge-split/) and [ei1333 Fibonacci-heap catalog entry](https://ei1333.github.io/library/structure/heap/fibonacchi-heap.hpp) identify source leads; their proofs and implementations remain future audit work. Binary/d-ary/binomial heap variants are explicit comparison targets, not newly verified implementations.

Additional unchanged archive leads: `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` contain `DynamicMex`; their `LcaO1` implementation is relevant to the linear-RMQ/FCB bridge. These are legacy references only.

Source keys (catalog inspection does not establish implementation correctness):

- **YC**: [https://github.com/yosupo06/library-checker-problems](https://github.com/yosupo06/library-checker-problems) — Graph/data_structure/tree problem-directory catalog, used to identify independently testable operations.
- **CPA**: [https://cp-algorithms.com/](https://cp-algorithms.com/) — Data Structures/Graphs navigation entries for sqrt-tree, RMQ and bounded flow; implementation-time article review still required.
- **OI**: [https://oi-wiki.org/](https://oi-wiki.org/) — Data-structure and graph navigation/catalog; uncommon graph decomposition and optimization families.
- **MAS**: [https://maspypy.github.io/library/](https://maspypy.github.io/library/) — ds, graph, flow, tree catalog paths.
- **NYA**: [https://nyaannyaan.github.io/library/](https://nyaannyaan.github.io/library/) — data-structure, graph and tree catalog paths.
- **EI**: [https://ei1333.github.io/library/](https://ei1333.github.io/library/) — structure and graph catalog paths.
- **SUI**: [https://suisen-cp.github.io/cp-library-cpp/](https://suisen-cp.github.io/cp-library-cpp/) — data structure dynamic/persistent/sequence catalog paths.
- **HIT**: [https://hitonanode.github.io/cplib-cpp/](https://hitonanode.github.io/cplib-cpp/) — data_structure and graph catalog paths.
- **KACTL**: [https://github.com/kth-competitive-programming/kactl](https://github.com/kth-competitive-programming/kactl) — Saved notebook Data Structures/Graphs contents and declarations; repository graph catalog.
- **CSES**: [https://cses.fi/problemset/](https://cses.fi/problemset/) — Graph/tree problem titles including functional graphs and Hamiltonian flights.
- **ODS**: [https://opendatastructures.org/ods-cpp.pdf](https://opendatastructures.org/ods-cpp.pdf) — Saved PDF table of contents, chapters 4/8/9/10/13 (skip lists, balancing, heaps and integer predecessor choices).
- **TKO**: [https://tko919.github.io/library/](https://tko919.github.io/library/) — Range LIS and general weighted matching catalog; weighted-matching source documentation inspected.
- **DYNAMIC**: [https://github.com/xxsds/DYNAMIC](https://github.com/xxsds/DYNAMIC) — README data-structure features and TODO; dynamic bitvector/wavelet tree supported, dynamic wavelet matrix explicitly TODO.
- **HDT**: [https://github.com/tomtseng/dynamic-connectivity-hdt](https://github.com/tomtseng/dynamic-connectivity-hdt) — README algorithm scope and Holm/de Lichtenberg/Thorup 2001 citation; implementation covers connectivity only.
- **OGDF**: [https://ogdf.github.io/doc/ogdf/group__decomp.html](https://ogdf.github.io/doc/ogdf/group__decomp.html) — Graph-decomposition API catalog, static/dynamic SPQR classes.
- **KOO**: [https://github.com/koosaga/olympiad/tree/master/Library/codes](https://github.com/koosaga/olympiad/tree/master/Library/codes) — Current Library/codes catalog; focused skew-symmetric flow special-case comments, extreme-set function and tree-packing augmentation code. The former repository is archived.
- **NX**: [https://networkx.org/documentation/stable/reference/algorithms/](https://networkx.org/documentation/stable/reference/algorithms/) — Planarity/cycles/eulerize API documentation; Eulerize is unweighted, Edmonds–Johnson reference motivates weighted T-join research.
- **STANFORD**: [https://github.com/jaehyunp/stanfordacm](https://github.com/jaehyunp/stanfordacm) — Saved 2015–16 team notebook contents and graph/data-structure declarations.

Rollback DSU owns the generic undo/connectivity substrate; weighted DSU owns potential/parity and weighted rollback specializations. Reuse the undo contract without reciprocal header dependencies.
