# 02-Data Structures implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../02-Data Structures/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| DS01 | `01-dsu.hpp`, `02-fenwick.hpp`, `03-segmenttree.hpp`, `04-sparsetable.hpp` | M | C01 | DSU, Fenwick, segment tree, sparse table foundations. |
| DS02 | `05-prefix_sum.hpp`, `06-sqrt_decomposition.hpp`, `07-ordered_set.hpp`, `08-monotone_stack.hpp` | M | C01, DS01 | Prefix/block/ordered/monotone sequence structures. |
| DS03 | `09-rollbackdsu.hpp`, `10-persistentdsu.hpp` | L | C01, DS01, DS31 | Rollback and persistent DSU; DS01. If split into rollback and persistent jobs, apply DS31 only to the persistent job; weighted DSU may depend on the rollback job. |
| DS04 | `11-fenwick_tree_advanced.hpp` | M | C01, DS01 | Advanced Fenwick variants; DS01. |
| DS05 | `12-lazysegmenttree.hpp` | L | C01, DS01 | Lazy segment tree and shared monoid/action definitions; DS01. The support header is not an additional numbered inventory target.  Also owns `00-monoids.hpp`. |
| DS06 | `13-dynamicsegmenttree.hpp` | L | C01, DS01, DS05 | Sparse-coordinate dynamic segment tree, destructive meld/range split and optional path copying; DS05 action contract. Prove ownership, leaf-combination rules and total allocation-charged meld work. General persistent versions belong to DS23. |
| DS07 | `14-segtreebeats.hpp` | L | C01, DS01, DS05 | Segment tree beats; DS05 concepts. |
| DS08 | `15-segment_tree_2d.hpp`, `16-mergesorttree.hpp` | L | C01, DS01 | 2D and merge-sort trees; DS01. |
| DS09 | `17-waveletmatrix.hpp` | L | C01, DS01 | Wavelet matrix, rank/select/quantile and weighted queries. |
| DS10 | `19-treap.hpp` | L | C01, DS01 | Key and implicit treap, lazy tags and seed policy. |
| DS11 | `21-lichao.hpp`, `22-convexhulltrick.hpp` | L | C01, DS01 | Li Chao and convex hull trick. |
| DS12 | `23-range_query_offline.hpp` | L | C01, DS01, GR03, MI04 | Mo/offline range queries, including updates/tree paths and Hilbert ordering; own only the query engine. DS47 owns interval sets; DS48 owns heap/deque engines. Tree-Mo reuses GR03 LCA; Hilbert ordering uses MI04. |
| DS13 | `26-trie.hpp`, `27-cartesiantree.hpp`, `28-disjointsparsetable.hpp` | L | C01, DS01 | Trie, Cartesian tree, disjoint sparse table. |
| DS14 | `37-lct.hpp` | XL | C01, DS01 | Link-cut tree, path/virtual-subtree invariants. |
| DS15 | `38-eulertourtree.hpp` | XL | C01, DS01, DS10 | Euler-tour tree dynamic forest. Optional for a separately implemented splay/balanced-tree backend; avoid requiring every balancing family. |
| DS16 | `39-toptree.hpp` | XL | C01, DS01 | Top tree cluster algebra. |
| DS17 | `40-succinctbitvector.hpp`, `41-succincttree.hpp` | L | C01, DS01 | Succinct bitvector and tree navigation. |
| DS18 | `42-vanemdeboas.hpp` | L | C01, DS01 | Integer predecessor structures and realistic 64-bit alternatives. |
| DS19 | `44-retroactivequeue.hpp` | XL | C01, DS01 | Retroactive priority queues and persistence relationships. |
| DS20 | `45-kineticheap.hpp` | L | C01, DS01 | Kinetic heap and event validity. |
| DS21 | `46-matroid_oracle.hpp` | M | C01, DS01 | Matroid independence oracles and exchange utilities. |
| DS22 | `43-rangetree.hpp` | L | C01, DS01 | Orthogonal range reporting/counting and multidimensional search structures. |
| DS23 | `18-persistentsegmenttree.hpp` | XL | C01, DS01, DS05 | General persistent segment tree, version memory, lazy updates and order statistics; DS01/DS05, coordinate with DS06 sparse nodes. |
| DS24 | `20-balanced_bst.hpp` | XL | C01, DS01 | Balanced BST family: splay/AVL/red-black foundations, rank/select and implicit split/join/rope, then AA/size-balanced/weight-balanced/scapegoat/skip-list alternatives. Preserve every planned alternative in a staged feature ledger; do not call the family complete after one tree. |
| DS25 | `29-weighteddsu.hpp` | M | C01, DS01, DS03 | Canonical potential/parity DSU algebra and ordinary/rollback weighted variants, including noncommutative group orientation and consistency. Reuse DS03 snapshot/undo contracts; DS03 should not independently duplicate weighted implementations. Basic DSU; rollback DSU for undo |
| DS26 | `30-aggregation_queue.hpp` | M | C01, DS01 | SWAG queue/deque for associative folds, noncommutative chronology, empty identity and amortized rebuilding. |
| DS27 | `31-radixheap.hpp` | M | C01, DS01 | Monotone integer-key radix heap, duplicate payloads and 32/64-bit boundary behavior. |
| DS28 | `32-static_range_queries.hpp` | L | C01, DS01, DS08, DS09, DS23 | Static distinct/frequency/mex/majority queries; choose Fenwick, persistent or wavelet backends per workload and state exact versus probabilistic guarantees. |
| DS29 | `33-offline_rectangle_queries.hpp` | L | C01, DS01, DS04 | Offline point/rectangle update-query sweeps, endpoint ordering and compressed multidimensional Fenwick engines; use Miscellaneous CDQ only when needed. |
| DS30 | `34-sqrttree.hpp` | L | C01, DS01 | Recursive sqrt-tree range folds, monoid identity and static/point-update complexity tradeoffs. |
| DS31 | `35-persistentarray.hpp` | M | C01, DS01 | Branching persistent arrays, initialization, version ownership and node-arena memory accounting. |
| DS32 | `36-fastset.hpp` | M | C01, C15, DS01 | Hierarchical bitset set operations and predecessor/successor; word boundaries and optional Core bit storage. |
| DS33 | `47-persistentqueue.hpp` | L | C01, DS01, DS31 | Persistent queue/deque with branching versions and persistence-valid operation bounds; array/list/arena storage. |
| DS34 | `48-persistentheap.hpp` | L | C01, DS01, DS48 | Persistent meldable heap, immutable sharing and lazy offsets; build on the ordinary heap contract. |
| DS35 | `49-dynamicbitvector.hpp` | XL | C01, DS01, DS10, DS17 | Mutable packed bitvector access/rank/select and insertion/deletion; blocked balancing and rank metadata invariants. |
| DS36 | `50-dynamic_wavelet.hpp` | XL | C01, DS01, DS09, DS35 | Dynamic wavelet tree/matrix variants over the dynamic bitvector; sequence changes, queries and memory/version costs. |
| DS37 | `51-sortablesegmenttree.hpp` | XL | C01, DS01, DS10 | Range-sort/aggregate structure, duplicate keys, ordering stability, split/merge and noncommutative fold direction. |
| DS38 | `52-rangeparallelunionfind.hpp` | L | C01, DS01 | Bulk interval union with newly merged-pair reporting and an explicit amortized progress argument. |
| DS39 | `53-rangemode.hpp` | L | C01, DS01 | Exact static range mode/least-frequency variants, tie witnesses and preprocessing/query/memory tradeoffs. |
| DS40 | `54-linearrmq.hpp` | L | C01, DS01, DS13 | Linear-preprocessing RMQ and ±1 microblocks via Cartesian trees; retain sparse-table alternatives and deterministic tie rules. |
| DS41 | `55-commonintervaltree.hpp` | L | C01, DS01, DS13 | Common-interval permutation decomposition, strong intervals and linear/prime nodes; compare exhaustive small permutations. |
| DS42 | `56-pqtree.hpp` | XL | C01, DS01 | PQ/PC tree consecutive/circular-ones reductions, ordering certificates and failed-constraint handling. |
| DS43 | `57-fingertree.hpp` | XL | C01, DS01 | Measured persistent finger trees, end operations, concatenation and monoid-guided split/search; prove bounds across branches. |
| DS44 | `58-persistent_bst.hpp` | XL | C01, DS01, DS24 | Persistent ordered/implicit balanced trees, lazy reversal, split/join and immutable version lifetime. |
| DS45 | `59-range_sequence_queries.hpp` | XL | C01, DS01 | Static range inversion/LIS queries with strictness, duplicates and reconstruction contracts; select offline or preprocessing engines. |
| DS46 | `60-succinct_sequence.hpp` | XL | C01, DS01, DS35 | Compressed searchable partial sums and sparse integer sequences; updates, blocked balancing and bit-space accounting. |
| DS47 | `24-interval_set.hpp` | M | C01, DS01 | Disjoint interval set/map, assign/add/split/merge, union length and mex; document ODT adversarial degeneration. |
| DS48 | `25-heap_deque.hpp` | L | C01, DS01 | Heap/deque family with separate indexed/erasable, meldable and median/top-k milestones; compare binary/d-ary, leftist/skew/pairing/binomial/Fibonacci operation bounds and practical workloads. Handles, invalidation and memory costs are explicit. |

Large single-header families keep one owner but require operation-level checkpoints; parallel workers must not independently rewrite the same header. Advanced balanced-BST and historical matching catalogs are several internal milestones, not a one-shot implementation promise.
