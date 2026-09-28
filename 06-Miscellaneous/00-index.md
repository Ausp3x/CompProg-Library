# 06 Miscellaneous — feature inventory

Reusable contest algorithms whose primary API is not owned by another topic. Rows are required scope, not implementation claims. Basic/Advanced/Esoteric are importance/rarity groups. P013's first seven headers are **verified**; see the [package evidence](95-p013.md). P014's sequence, interval, offline-query and sorting headers are also **verified**; see its [contracts and evidence](96-p014.md). P015's enumeration, knapsack and cycle-finding headers are **verified**; see [contracts and evidence](89-p015.md). Other families are **planned**. See the [inventory audit](../00-Guidelines/16-inventory-audit.md) for source coverage and limits.

## Basic

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `01-random.hpp` | Reproducible seed/reset, unbiased inclusive signed/unsigned 64-bit and bounded draws, Fisher–Yates shuffle; clock-only default seed and noncryptographic guarantees. | Verified MI01; [contracts/evidence](90-random_hash.md) |
| `02-customhash.hpp` | SplitMix64-style hashing of 64/128-bit integers, enum/conversion, pointer identity, binary strings, recursive pair/tuple/ordered ranges; unordered aliases, process/explicit seed and collision limits. | Verified MI01; [contracts/evidence](90-random_hash.md) |
| `03-fastio.hpp` | Buffered byte/token/integer I/O through 128 bits, EOF/ASCII whitespace, overflow/syntax/error statuses, flush, large buffers, borrowed stream lifetime and interactive buffer policy. | Verified MI01; [contracts/evidence](91-fastio.md) |
| `04-compression.hpp` | Sorted comparator-equivalence ranks, inverse/absent lookup, open/closed point ranges and offline interval endpoints; append-only stable encounter IDs, rebuild/reset lifetimes. | Verified MI02; [contracts/evidence](92-compression_search.md) |
| `05-binarysearch.hpp` | Canonical Mathematics search facade: first/last true with absence, overflow-safe full signed endpoints and finite real brackets with precision/convergence status. | Verified MI02; [contracts/evidence](92-compression_search.md) |
| `06-bit_operations.hpp` | Unsigned 8–128-bit scalar count/width/scans, powers, bit manipulation, safe shifts/rotations, terminating submask and fixed-popcount stepping; explicit zero/overflow contracts. | Verified MI02; [contracts/evidence](93-bit_operations.md) |
| `07-permutation.hpp` | Lexicographic next/previous with duplicates, bijection validation/inverse/composition/signed power, scalable Lehmer digits, bounded scalar and multiset rank/unrank with overflow status. | Verified MI02; [contracts/evidence](94-permutation.md) |
| `08-sequence_algorithms.hpp` | Exact widened ordinary/circular/bounded maximum subarray and dimension-aware maximum subrectangle with witnesses; strict/non-strict LIS witnesses, index-distinct counts and weighted optima; inversions and canonical prefix/difference sums, two pointers and monotone/sliding windows; exact-frequency Boyer–Moore/Misra–Gries. | Verified MI03; [contracts/evidence](96-p014.md) |
| `09-interval_algorithms.hpp` | Open/closed integral endpoints over reals or integers; merge and exact union measure, ordered sweep events, maximum overlap, stabbing counts/minimum hitting witnesses, cardinality/weighted disjoint schedules and empty-set semantics. | Verified MI03; [contracts/evidence](96-p014.md) |
| `10-offline_queries.hpp` | Comparator-ordered offline activation sweeps with stable ties and strict/inclusive thresholds; half-open range threshold counts through shared Fenwick. Parallel binary search and CDQ retain their Advanced owners. | Verified MI03; [contracts/evidence](96-p014.md) |
| `11-sorting_selection.hpp` | Dense-domain counting and stable projected counting; stable full-width signed/unsigned 8–128-bit radix, standard stable partition and nth-element selection. Duplicate/stability, average/adversarial bounds and measured tradeoffs are explicit; standard algorithms remain the default. | Verified MI14; [contracts/benchmark](96-p014.md) |
| `12-enumeration.hpp` | Ordinary/repeated combinations, arbitrary-dimensional subsets, submasks, mixed-radix products and reflected Gray traversal; immutable callback state, cancellation, empty cases and full-width 8–128-bit masks. Reuses canonical bit stepping; ranking and specialist orders remain separate. | Verified MI15; [contracts/evidence](89-p015.md) |
| `13-knapsack.hpp` | Mixed 0/1/unlimited/bounded capacity maxima and value-indexed minimum weights; exact/at-most queries, finite/unreachable/unbounded states, signed values, zero weights, linear-memory optima and optional multiplicity witnesses; independent Boolean feasibility and exact/modular multiplicity-vector counts. | Verified MI16; [contracts/tests/benchmark](89-p015.md) |
| `14-cyclefinding.hpp` | Generic Floyd/Brent orbit entry, tail and least period; custom successor-compatible equality, constant-state storage and strict successor-call budgets across all phases. Exhaustion is distinct from no cycle; graph-wide queries retain their Graphs owner. | Verified MI17; [contracts/tests/benchmark](89-p015.md) |

## Advanced

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `15-smalltolarge.hpp` | Canonical small-to-large container/map merging and DSU-on-tree/sack subtree aggregation with add/remove/reset callbacks, ownership/memory and amortized bounds including callback costs. Reuse Graphs tree layouts; the Graphs family does not independently implement this engine. | Planned |
| `16-hilbertorder.hpp` | 2D/3D Hilbert or Gilbert ordering for Mo queries, coordinate width and deterministic ordering. | Planned |
| `17-fast_io_advanced.hpp` | mmap/read/write backends, fallback on pipes/interactive judges, input lifetime and portability. | Planned |
| `18-randomized_algorithms.hpp` | reservoir sampling, random hashing, Las Vegas versus Monte Carlo guarantees, reproducible seeds. | Planned |
| `19-hash_families.hpp` | universal/tabulation hashing, adversarial-resistant maps, pair/multiset hash and collision accounting. | Planned |
| `20-parallelbinarysearch.hpp` | update rollback/time decomposition, monotonicity proof and batched check API. | Planned |
| `21-cdq.hpp` | 3D dominance/counting, strict tie order, merge invariants. | Planned |
| `22-bitset_optimization.hpp` | Word/bitset applications: subset-sum, reachability, Boolean DP and bit-parallel transitions. Reuse Core 18-bitset or std::bitset for storage/shifts; scalar contest code only, no duplicate dynamic-bitset engine. | Planned |
| `23-contestallocator.hpp` | bump arena, reset semantics, alignment and object destruction policy. | Planned |
| `24-memoization.hpp` | recursive lambda and hash/array memoization, state lifetime and stack-depth cautions. | Planned |
| `25-exactcover.hpp` | Algorithm X / dancing links, solution enumeration and secondary columns. | Planned |
| `26-satsolver.hpp` | 2-SAT lives in Graphs; here optional general DPLL/CDCL only after contest-use case and proof/testing plan. | Planned |
| `27-calendar_time.hpp` | date arithmetic, leap years and timezone-free integer timestamps where tasks require them. | Planned |
| `28-encoding.hpp` | UTF-8 validation, base64/hex, varints, rolling serialization for testing tools. | Planned |
| `29-knapsack_advanced.hpp` | Meet-in-the-middle and bitset subset sum, bounded multiplicity monotone queues (reuse Basic binary splitting), specialized value/small-weight domains, witnesses and count-by-target-sum generating functions. Research leads: optimal-solution counts, k-best/grouped/precedence/multiple-resource variants and modern fine-grained subset sum; see [scope handoff](89-p015.md). Sharp-P subset sum uses Mathematics FPS with positive weights and explicit modulus/degree conditions; do not conflate counting with Boolean reachability. | Planned |
| `30-meetinthemiddle.hpp` | Split-state enumeration and joins, subset sum with witness/counts, duplicate multiplicities, Pareto dominance pruning only when justified, four-list memory tradeoffs as a separately documented research extension. | Planned |
| `31-digitdp.hpp` | Digit automaton/state DP over intervals and arbitrary bases, leading zeros, tight flags, zero representation, signed-domain policy and count/sum aggregates; memoization keys must include all state. | Planned |
| `32-subset_dp.hpp` | Generic subset partition and subset-state transitions, reconstruction and O(3^n) submask enumeration costs; algebraic SOS/zeta/Mobius/subset convolution live in Mathematics. Held–Karp illustrates the recurrence; Graphs owns the Hamiltonian/TSP API. | Planned |
| `33-profile_dp.hpp` | Broken-profile and plug/connectivity DP, canonical frontier labels, obstacles and boundary closure, cycle/path/cover state invariants; exact state encoding and optional witness recovery. | Planned |
| `34-slope_trick.hpp` | Convex piecewise-linear functions using breakpoint heaps: add hinge/absolute value, shift, restrict domain, prefix/suffix minimization, merge and minimizer/value queries; weighted multiplicities and infinity contracts. Math DP optimizations own Monge/Knuth/WQS, not this representation. | Planned |
| `35-state_space_search.hpp` | Reusable backtracking/branch-and-bound, iterative deepening and IDA* over implicit states, transposition pruning and admissible lower-bound contracts; exact exhaustion versus budget-stopped outcomes. Explicit graph shortest-path engines remain in Graphs. | Planned |
| `36-game_search.hpp` | Minimax/negamax, alpha-beta, iterative deepening, move ordering and transposition bounds with correct depth/window semantics; terminal rules, repetition and heuristic horizons distinguished from solved game values. | Planned |
| `37-expression_parser.hpp` | Tokenization, shunting-yard/Pratt operator parsing, unary operators, precedence/associativity and parentheses; exact error offsets and configurable evaluation domain; no arbitrary code evaluation. | Planned |
| `38-heuristic_optimization.hpp` | Simulated annealing, hill climbing, beam/tabu search and budgeted local-search drivers; deterministic seed/replay, incumbent retention, feasible witnesses, score direction and exact versus heuristic stopping. No optimality guarantee. | Planned |
| `39-scheduling.hpp` | Smith/exchange-rule orderings with proof assumptions, deadline/release-time greedy patterns and feasibility witnesses; weighted interval scheduling reuses Basic intervals, general flow formulations reuse Graphs. | Planned |
| `40-optimal_merge.hpp` | Huffman optimal prefix code/merge tree, canonical code assignment, zero weights, one-symbol case, ties and tree/witness extraction; heap engine from Data Structures. Alphabetic constraints have a separate Esoteric owner. | Planned |
| `41-dynamic_dp.hpp` | Maintain chain DP and reusable state-transfer summaries through small matrices or associative products; update/query adapters, composition direction and identities. Graphs owns fixed/dynamic tree DP in dynamictreedp; reuse that API instead of duplicating it here. State dimension and update/query bounds explicit. | Planned |

## Esoteric

| Proposed header | Feature checklist | Status and references |
|---|---|---|
| `42-rollback_framework.hpp` | generic undo log and scoped checkpoints, side-effect discipline. | Planned |
| `43-persistentallocator.hpp` | arena and garbage collection for persistent structures, ownership and version lifetime. | Planned |
| `44-external_memory.hpp` | cache-aware/offline algorithms and succinct storage for unusually large input. | Planned |
| `45-probabilistic_sketches.hpp` | Bloom filter, HyperLogLog, Count-Min and heavy hitters; error and false-positive contracts. | Planned |
| `46-streaming_algorithms.hpp` | Mergeable quantile and sliding-window summaries, streaming sampling adapters and turnstile/insertion-only assumptions; reservoir primitive belongs to randomized_algorithms. Explicit error, failure probability, memory and adversarial-stream contracts. | Planned |
| `47-derandomization.hpp` | Small-bias/perfect-hash families, conditional expectation and color-coding/splitter adapters; distinguish deterministic constructions, Monte Carlo repetition and parameterized/exponential costs. | Planned |
| `48-alphabetic_tree.hpp` | Hu–Tucker/Garsia–Wachs optimal alphabetic binary tree and ordered merging, nonnegative weights, reconstruction and degeneracies; distinguish cost-only and witness variants and verify the claimed O(n * log(n)) implementation. | Planned |

## P014 implementation record

[Contracts, verification, benchmarks and future handoffs](96-p014.md) records completed MI03 followed by MI14. Value-distinct/all-witness LIS and theoretical rectangle methods remain explicit research extensions; the supported index-distinct and exact contest APIs are verified.

## Contracts and ownership

Fast I/O covers EOF, min signed integer, buffering and interactive flushing. Randomness records seeds, uniformity and Las Vegas/Monte Carlo/heuristic distinctions. Sequence/search/DP APIs state domains, reconstruction and impossible outcomes; test against exhaustive small references. ISA-specific engines remain in full Core; calls to those types are allowed. A broad research family must be implemented in recorded milestones, with unsupported variants explicitly left planned.

Scalar transforms and algebraic DP optimizations belong to Mathematics; graph decompositions and explicit shortest paths to Graphs; data-structure engines to Data Structures. Legacy FastConv is arithmetic transforms, not fast I/O. [Local legacy excerpts](97-Legacy/00-index.md) and [transfer provenance](../00-Guidelines/14-monolith-transfer.md) preserve unchanged source; compilation is not correctness evidence.

## Sources inspected for this audit

Read OI Wiki pages for [knapsack](https://oi-wiki.org/dp/knapsack/), [digit DP](https://oi-wiki.org/dp/number/), [state DP](https://oi-wiki.org/dp/state/), [plug DP](https://oi-wiki.org/dp/plug/), [dynamic DP](https://oi-wiki.org/dp/dynamic/), [slope trick](https://oi-wiki.org/dp/opt/slope-trick/), [IDA*](https://oi-wiki.org/search/idastar/), [alpha-beta](https://oi-wiki.org/search/alpha-beta/), [job ordering](https://oi-wiki.org/misc/job-order/), [expression parsing](https://oi-wiki.org/misc/expression/), [majority candidates](https://oi-wiki.org/misc/main-element/), [Garsia–Wachs](https://oi-wiki.org/misc/garsia-wachs/) and [annealing](https://oi-wiki.org/misc/simulated-annealing/). Also compared CP-algorithms, CSES, Library Checker, OI Wiki navigation and Japanese library catalogs. Catalog presence is a feature lead; algorithm proofs, current fastest variants and licensing must be checked during implementation. The Garsia–Wachs page has conflicting complexity wording; the inventory requires independent verification rather than adopting its headline. Research timestamps/read scopes live in the [source ledger](../00-Guidelines/17-research-sources.json).
