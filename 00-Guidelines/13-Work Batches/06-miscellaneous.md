# 06-Miscellaneous implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../06-Miscellaneous/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| MI01 | `01-random.hpp`, `02-customhash.hpp`, `03-fastio.hpp` | M | C01 | Random, hash and fast I/O basics. |
| MI02 | `04-compression.hpp`, `05-binarysearch.hpp`, `06-bit_operations.hpp`, `07-permutation.hpp` | M | C01, DS01, DS02, MA01 | Compression, binary search, bits, permutations. |
| MI03 | `08-sequence_algorithms.hpp`, `09-interval_algorithms.hpp`, `10-offline_queries.hpp` | M | C01, DS01, DS02, MI02 | Sequence, interval and offline query patterns; circular/bounded maximum subarrays, maximum-sum subrectangles and counting/weighted LIS reuse shared range-query engines. |
| MI04 | `15-smalltolarge.hpp`, `16-hilbertorder.hpp` | M | C01, GR05 | Canonical small-to-large map merging/DSU-on-tree with add/remove/reset callbacks, ownership and callback-cost bounds; reuse GR05 subtree layouts. Standalone Hilbert/Gilbert ordering is usable by DS12 and must not require Mo itself. |
| MI05 | `17-fast_io_advanced.hpp`, `18-randomized_algorithms.hpp`, `19-hash_families.hpp` | L | C01 | Advanced I/O, randomized algorithms, hash families. |
| MI06 | `20-parallelbinarysearch.hpp`, `21-cdq.hpp` | M | C01 | Parallel binary search and CDQ. |
| MI07 | `22-bitset_optimization.hpp`, `23-contestallocator.hpp`, `24-memoization.hpp` | M | C01, C15 | Bitset optimization, arena, memoization. |
| MI08 | `25-exactcover.hpp` | L | C01 | Exact cover / dancing links. |
| MI09 | `26-satsolver.hpp` | XL | C01 | General SAT solver research, only after a concrete use case. |
| MI10 | `27-calendar_time.hpp`, `28-encoding.hpp` | M | C01 | Calendar arithmetic and encoding. |
| MI11 | `42-rollback_framework.hpp`, `43-persistentallocator.hpp` | M | C01 | Rollback and persistent allocator lifetimes. |
| MI12 | `45-probabilistic_sketches.hpp`, `46-streaming_algorithms.hpp`, `47-derandomization.hpp` | L | C01 | Sketches, streaming and derandomization, with error contracts. |
| MI13 | `44-external_memory.hpp` | L | C01 | External-memory/cache-aware algorithm research. |
| MI14 | `11-sorting_selection.hpp` | M | C01 | sorting selection. Complete all selected inventory contracts and variants. |
| MI15 | `12-enumeration.hpp` | M | C01, MI02 | enumeration. Complete all selected inventory contracts and variants; reuse canonical bit stepping. |
| MI16 | `13-knapsack.hpp` | M | C01 | knapsack. Complete all selected inventory contracts and variants. |
| MI17 | `14-cyclefinding.hpp` | S | C01 | cyclefinding. Complete all selected inventory contracts and variants. |
| MI18 | `29-knapsack_advanced.hpp` | L | C01, MA07, MI16 | knapsack advanced. Complete all selected inventory contracts and variants. |
| MI19 | `30-meetinthemiddle.hpp` | M | C01 | meetinthemiddle. Complete all selected inventory contracts and variants. |
| MI20 | `31-digitdp.hpp` | M | C01 | digitdp. Complete all selected inventory contracts and variants. |
| MI21 | `32-subset_dp.hpp` | L | C01 | Generic subset-state/partition DP and reusable transitions/reconstruction. Held–Karp may illustrate the recurrence; Graph GR54 owns the graph-facing Hamiltonian/TSP API. Mathematics owns SOS/zeta/Mobius/subset convolution. Do not add MI21 -> GR54: GR54 -> MI21 is the correct direction. |
| MI22 | `33-profile_dp.hpp` | L | C01 | profile dp. Complete all selected inventory contracts and variants. |
| MI23 | `34-slope_trick.hpp` | L | C01, DS48 | slope trick. Complete all selected inventory contracts and variants. |
| MI24 | `35-state_space_search.hpp` | L | C01 | state space search. Complete all selected inventory contracts and variants. |
| MI25 | `36-game_search.hpp` | L | C01 | game search. Complete all selected inventory contracts and variants. |
| MI26 | `37-expression_parser.hpp` | M | C01 | expression parser. Complete all selected inventory contracts and variants. |
| MI27 | `38-heuristic_optimization.hpp` | L | C01 | heuristic optimization. Complete all selected inventory contracts and variants. |
| MI28 | `39-scheduling.hpp` | M | C01 | scheduling. Complete all selected inventory contracts and variants. |
| MI29 | `40-optimal_merge.hpp` | M | C01, DS48 | optimal merge. Complete all selected inventory contracts and variants. |
| MI30 | `41-dynamic_dp.hpp` | L | C01, DS01 | Chain DP and reusable associative/state-transfer summaries with point updates and composition-order tests. Graphs GR68 owns fixed/dynamic tree DP; do not implement tree adapters here. Optional matrix policy can use an existing Core/Math matrix; no dependency on GR68 is needed for the chain engine. |
| MI31 | `48-alphabetic_tree.hpp` | XL | C01, MI29 | alphabetic tree. Complete all selected inventory contracts and variants. |
