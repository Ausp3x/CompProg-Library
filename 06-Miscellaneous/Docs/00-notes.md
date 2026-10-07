# 06 Miscellaneous — notes

Contracts, ownership boundaries and migration obligations moved out of the inventory. Operation lists live only in [00-index.md](../00-index.md); the source ledger is [00-sources.md](00-sources.md).

## Status semantics

- Rows are required scope, never implementation claims. P013 (`01`–`07`), P014 (`08`–`11`) and P015 (`12`–`14`) were verified; their evidence is the per-header documents `Docs/01-random.md` … `Docs/14-cyclefinding.md` plus the package records below; rows that gained operations in the 2026-10-06 sweep are `partial` and keep that evidence for the already-verified operations.
- A verified claim covers only the operations and domains recorded in the evidence document. Finite test corpora are paired with the correctness arguments there; no online acceptance is claimed and no online submission was made.

## Basic contracts

- Rows `01`–`07` (P013): full contracts live under `## Contracts` in [01-random.md](01-random.md), [02-customhash.md](02-customhash.md), [03-fastio.md](03-fastio.md), [04-compression.md](04-compression.md), [05-binarysearch.md](05-binarysearch.md), [06-bit_operations.md](06-bit_operations.md) and [07-permutation.md](07-permutation.md). Summary: Random is MT19937-64 with inclusive integer bounds, `randDouble` on `[0,1)` or `[l,r)` and a URBG interface, not cryptographic; CustomHash is seeded SplitMix64 without a collision guarantee; fast I/O uses `Error > Invalid > Overflow` with the destination unchanged on failure, `from_chars`-exact `readDouble` and `to_chars` fixed `writeDouble`; compression ids are comparator-equivalence ranks; the binary-search facade adds no names; BitOps covers unsigned words through 128 bits; permutations are bijections of `[0,n)` with `lng` exponents and offsets.
- Omitted in the P013 re-audit sweep (2026-10-07), one reason each:
  - `randomPermutation(n)`: `iota` plus `shuffle`; sampling families are `18`.
  - Faster engine (xoshiro/xorshift): an engine swap would break replay of recorded seeds and the standard 10000th-word fixture; MT19937-64 costs a few ns per word, which no contest workload here makes the bottleneck.
  - Fast I/O fixed-length string read: `readToken` covers it; YES/NO and `bool` printing: problem-specific template material; `long double` I/O: libstdc++ falls back to platform `strtold`/`printf` on MinGW, unverifiable at the floor.
  - Compression incremental builder, neighbour queries, counting-sort build, hashed encounter ids, 2D compression: see [04-compression.md](04-compression.md) "Limits and handoffs".
  - BitOps `countOnesUpTo`, `bitReverse`; permutation `permute`, argsort, `permutationFromCycles`, `cycleType`, linear non-lexicographic rank, dynamic cycles, roots: see the omitted candidates in [06-bit_operations.md](06-bit_operations.md) and [07-permutation.md](07-permutation.md).
  - Handoff to P015: `12-enumeration.hpp` may add `forEachSupermask`, built on `BitOps::nextSupermask`.
- Sequences (`08`, contracts in [08-sequence_algorithms.md](08-sequence_algorithms.md)): dimensions below `INT_MAX`, integral inputs up to 64 bits, `lll` sums; ties deterministic but arbitrary; LIS and chain counting are index-distinct (value-distinct counting and all-witness enumeration remain unowned until scoped); `heavyHitters` is exact Misra–Gries, approximate summaries belong to `45`/`46`; `maximumAverageSubarray` is exact (rational `sum / length`, no tolerance) for integral input, real-valued inputs are out of domain; `mex` is static, dynamic mex belongs to Data Structures `24`/`32`.
- P014 omissions (2026-10-08), one reason each: inversion count of every rotation and the all-range inversion table (rotation is an O(n) post-pass over `inversionCount` ranks; range inversions are Data Structures `59`); subarray-sum counting by target or divisibility (a `PrefixSum` plus hash-map composition); three/four-sum (loops over `sortedPairSum`; four-sum is meet in the middle `30`); k-machine maximum schedule and multi-machine variants (`39-scheduling.hpp`); interval jump queries (doubling over the earliest-finish successor, a query structure for Graphs `11`-style lifting, not a static batch call); circular arc cover (source not read); KACTL ConstantIntervals (Mathematics `02` search); offline set intersection (bitset technique `22`), offline RMQ (Data Structures `28`), offline range k-th (`20` with Data Structures `31`/`32`), offline arithmetic-progression adds (Data Structures `05` DifferenceArray); partial sort and k-way merge (`std::partial_sort`; `44` and `15` own merging); multikey radix (chained stable `radixSort` passes).
- Intervals: integral endpoints with open/closed flags over the continuous or integer domain; doubled coordinates represent half points; empty sets are valid results; dynamic interval maps belong to Data Structures `24-interval_set.hpp`. `intervalPartitionAssignment` is the canonical minimum-machine partition: `39`'s `intervalPartitionMachines` must forward to it. `intervalUnionMeasure` is the exact interval engine; Geometry `13`'s `segmentUnionLength` should forward to it or record why not.
- Offline sweep: strict queries at a key run before updates at that key, inclusive queries after; updates applied at most once; callbacks never mutate keys or comparator order; parallel binary search and CDQ keep their own rows (`20`, `21`).
- Sorting/selection: standard algorithms stay the default; counting/radix are explicit integer-domain trades of memory for time with `k <= INT_MAX`; `quickSelect` is GNU introselect (average linear, heap fallback); `medianOfMedians` is the worst-case linear variant (O(n), recurrence constant 40, measured at most 11.4n comparisons and 1.6–12x `nth_element`, see [11-sorting_selection.md](11-sorting_selection.md)); `threeWayPartition` is the shared Dutch-flag step.
- Enumeration: visitors return `true` to continue, traversals return `true` only on completion, state is borrowed const; outputs are charged by size and never counted in fixed width; ranking/unranking of combinations must match `forEachCombination` order.
- Knapsack: item types with `count=-1` unlimited; axes `[0,limit]` with `limit < INT_MAX`; `finite`/`unreachable`/`unbounded` are distinct statuses; counting counts multiplicity vectors, not optimal selections; `T` arithmetic is caller-selected (exact, arbitrary precision or modular).
- Cycle finding: `found=false` means budget exhausted, not "no cycle"; the budget counts every successor call across all phases; equality is any equivalence preserved by the successor; whole-functional-graph queries belong to Graphs `11-functionalgraph.hpp`.

## Advanced and Esoteric contracts

- Small-to-large (`15`) is the canonical merging/DSU-on-tree engine; Graphs `14-tree_algorithms.hpp` supplies subtree layouts and must not duplicate container merging. Amortized bounds include callback costs; ownership of merged containers is explicit.
- Hilbert order (`16`) is usable by Data Structures `23-range_query_offline.hpp` without requiring Mo; coordinate width and tie order are deterministic.
- Advanced I/O (`17`) falls back to `03` on pipes and interactive judges; input lifetime and portability are documented.
- Randomized algorithms (`18`) and hash families (`19`) state Las Vegas versus Monte Carlo guarantees, reproducible seeds and collision accounting; the open-addressing map documents load factor, tombstones and iteration invalidation.
- Parallel binary search (`20`) requires a monotone check and either a reset-and-replay or rollback update model; CDQ (`21`) requires a strict tie order and merge invariants; 2D offline rectangle sums belong to Data Structures `33`.
- Bitset optimization (`22`) reuses Core `18-bitset` or `std::bitset` for storage and shifts; scalar contest code only, no second dynamic-bitset engine. Transitive closure is Graphs `40`, bitset LCS is Strings `19`.
- Allocator (`23`), memoization (`24`), rollback (`42`) and persistent arena (`43`) document reset semantics, alignment, object destruction, state lifetime, stack depth and version lifetime; side effects must be recorded in the undo log before mutation.
- Exact cover (`25`) enumerates bounded solutions with secondary columns; SAT (`26`) stays optional behind a concrete contest use case and a proof/testing plan; 2-SAT is Graphs `18-twosat.hpp`.
- Calendar (`27`) uses the proleptic Gregorian calendar and timezone-free day numbers; encoding (`28`) rejects overlong UTF-8, surrogates and code points above `0x10FFFF`.
- Advanced knapsack (`29`) owns value/count/witness variants, grouped, two-resource and tree (dependency) knapsack, k-best and optimal-selection counts; sharp-P subset sum uses Mathematics FPS with positive weights and explicit modulus/degree conditions and is never conflated with Boolean reachability. Boolean bitset reachability is `22`; meet-in-the-middle split/join is `30` and `29` calls it.
- Meet in the middle (`30`): duplicate multiplicities are counted exactly; Pareto pruning only when justified; the four-list split documents its memory tradeoff.
- Digit DP (`31`): memoization keys include every state (position, tight, leading zero, automaton state); signed intervals are handled by an explicit policy. Subset DP (`32`) documents `O(3^n)` submask costs and reconstruction; Graphs `73` owns the Hamiltonian/TSP API and depends on `32`, never the reverse. Profile DP (`33`) states canonical frontier labels, boundary closure and state invariants.
- Slope trick (`34`) represents convex piecewise-linear functions by breakpoint heaps with explicit infinity contracts and weighted multiplicities; Mathematics `27-optimization.hpp` bridges to it and owns Monge/Knuth/WQS.
- State-space search (`35`) distinguishes exact exhaustion from budget-stopped outcomes and requires admissible lower bounds for IDA*; explicit-graph engines remain in Graphs (`43-implicit_graph.hpp` owns implicit BFS/Dijkstra). Game search (`36`) distinguishes solved values from heuristic horizons and documents depth/window semantics; retrograde analysis and Sprague–Grundy are Mathematics `25`.
- Expression parsing (`37`) reports exact error offsets, supports configurable evaluation domains and never evaluates arbitrary code. Heuristic optimization (`38`) gives no optimality guarantee; seeds replay deterministically and incumbents are retained.
- Scheduling (`39`) records the exchange-argument assumptions of each ordering rule (Livshits–Kladov limits them to linear, exponential and identical monotone penalties); weighted interval scheduling reuses `09`, flow formulations reuse Graphs; rooted-tree precedence orders are Graphs `79`.
- Optimal merge (`40`) handles zero weights, the one-symbol case and ties, using the Data Structures heap engine; alphabetic (ordered) constraints are `48`, whose claimed `O(n log n)` Garsia–Wachs bound must be verified independently of the OI Wiki wording.
- Dynamic DP (`41`) is the chain engine with explicit state dimension and update/query bounds; it may use an existing Core/Mathematics matrix; Graphs `84-dynamictreedp.hpp` owns tree adapters and no dependency on it is needed.
- External memory (`44`) covers cache-aware/offline algorithms for unusually large input; external sorting is the only sorting variant outside `11`. Sketches (`45`) and streaming summaries (`46`) state error, failure probability, memory and adversarial-stream contracts; mergeability is claimed only for KLL/HLL/Count-Min, not GK. Derandomization (`47`) distinguishes deterministic constructions, Monte Carlo repetition and parameterized costs.
- New rows: `49` Timer uses `steady_clock` only; `50` Interactive always flushes before reading and asserts its query budget; `51` grid/dice helpers are value types with documented orientation conventions; `52` interval DP reuses the Mathematics Knuth adapter for optimal BST; `53` bracket sequences order lexicographically with `(` before `)` and reuse Mathematics Catalan numbers; `54` k-best enumerations are charged `O(k log k)` with explicit tie order; `55` Dinkelbach terminates exactly on integer weights by comparing rationals in `lll`.

## Ownership summary

Scalar transforms, DP optimizations, combinatorial counts and game theory belong to Mathematics; graph decompositions, explicit shortest paths, tree DP and Hamiltonian APIs to Graphs; data-structure engines (heaps, Mo, rollback/persistent containers, range queries, histogram rectangles) to Data Structures; text indexes, LCS and run-length coding to Strings. ISA-specific engines remain in full Core; calls to those types are allowed. A broad family is implemented in recorded milestones with unsupported variants left `planned`.

## Legacy and migration

- `OLD` (the former `97-Legacy` excerpts were deleted once the rows were verified) keeps unchanged `01-random.cpp` (old global `rng`, no bounds asserts) and `02-customhash.cpp` (different seeding and pair/tuple/string handling). Both are superseded by the active headers; they stay until the inventory audit records every retained feature, and must never be included by aggregates or notebook discovery.
- Legacy `FastConv` is an arithmetic transform, not fast I/O, and belongs to Mathematics. Legacy `knapsack01`/`knapsackComplete`/`knapsackMultiple` in `OLD` are accounted for by `13` (see [13-knapsack.md](13-knapsack.md)); legacy `old_kadane.cpp` by `08`. Transfer provenance is in [2026-09-27-monolith-transfer.md](../../00-Guidelines/History/2026-09-27-monolith-transfer.md); compilation is not correctness evidence.

## P013 package record (MI01, MI02)

First verified 2026-09-28 (MI01, then MI02). Re-audited 2026-10-07 under the current rules: every operation in the seven inventory rows was compared against the code and tests; the missing operations were implemented with independent oracles; header contracts moved to the per-header evidence documents (comment cap); closing braces were normalized in the headers, testers and benchmark; and every `/reaudit-review` finding was resolved. `OLD` and `97-Legacy` are unchanged.

| MI01 / `01-random.hpp` | MT19937-64 with explicit/clock seeds, URBG interface, unbiased inclusive integer draws over full 64-bit domains, `randDouble` on `[0,1)` and `[l,r)`, Fisher–Yates, global `rng` | [01-random.md](01-random.md) |
| MI01 / `02-customhash.hpp` | Seeded SplitMix64 for scalars, 128-bit, pointers, strings, pairs, tuples and ordered ranges; `safe_unordered_map`/`set`, `safe_gp_hash_table` | [02-customhash.md](02-customhash.md) |
| MI01 / `03-fastio.hpp` | Buffered bytes, tokens, lines, integers through 128 bits, exactly rounded doubles, fixed-precision double output, variadic `read`/`print`, status precedence, sticky output errors | [03-fastio.md](03-fastio.md) |
| MI02 / `04-compression.hpp` | Sorted snapshot ids with bounds, bulk ranks and open/closed point ranges; endpoint collection; stable distinct ranks; first-encounter ids | [04-compression.md](04-compression.md) |
| MI02 / `05-binarysearch.hpp` | Include facade for the canonical Mathematics search API (13 names) | [05-binarysearch.md](05-binarysearch.md) |
| MI02 / `06-bit_operations.hpp` | Unsigned 8–128-bit counts/scans/powers, parity, bit manipulation, shifts/rotations, Gray codes, submask/supermask/fixed-popcount steps | [06-bit_operations.md](06-bit_operations.md) |
| MI02 / `07-permutation.hpp` | Lexicographic and k-th steps, inverse/composition/powers, cycles/sign/order, Lehmer digits, bounded scalar and multiset ranks | [07-permutation.md](07-permutation.md) |

Dependencies: P002 (template), P006 (Fenwick, `CoordinateCompression`), P012 (search engine), all verified. `98-basic.hpp` and `99-all.hpp` include all seven headers; `01-run.py` discovers all seven testers.

## Re-audit gap list and resolution

Before any change, the comparison of rows against code found these gaps; all are closed:

Re-audit gap list (all closed):

| `01` | `randDouble` missing | Added both overloads; exact Python oracle replaying engine words |
| `03` | `readLine`, `readDouble`, `writeDouble` missing; the fastio evidence contradicted the row (finding 1) | Implemented; oracles: independent splitter and Python `bytes.split`, `strtod` and Python `float()`, glibc `printf` and Python `format` |
| `04` | `ranks (bulk)` missing; evidence claimed no gap (finding 2) | Added `ranks`; linear-count and `bisect_left` oracles; evidence corrected |
| `07` | `kthNextPermutation`, `permutationCycles`, `permutationSign`, `permutationOrder` missing; evidence claimed completeness (finding 3) | Implemented, plus `permutationOrderMod`; enumeration, property, repeated-step and Python factorial-base/`lcm` oracles |
| all | Contracts in multi-line header comments over the 8% cap | Moved to `## Contracts` sections; headers pass the cap |
| all | `; }` closing braces in headers, testers, benchmark (finding 7) | Normalized; `03-consistency.py --braces` reports none |

Additions from the completeness sweep: the Random URBG interface, `safe_gp_hash_table`, `read`/`print`/`writeValue`/`write` end byte/`precision`, `stableRanks`, `parity`/`grayCode`/`grayDecode`/`nextSupermask`, `permutationOrderMod`; the facade row now lists `fibSearch`/`expSearch`. Omitted candidates and reasons are in the "Basic contracts" list above.

## /reaudit-review findings

| # | Finding | Resolution |

`/reaudit-review` findings:

| 3 | permutation missing four operations | Fixed (above) |
| 4 | `RealSearchResult` lacks a complexity line | Already fixed by the P012 re-audit: `05-Mathematics/02-search_algorithms.hpp` carries `// T: O(1), M: O(1); ...` above the struct; the facade needs no change |
| 5 | Unqualified `uintptr_t` in customhash | Now `std::uintptr_t` |
| 6 | String-hash comment used `n`, prose bounds | Struct line now reads `O(L) string, O(k) composite with k recursively visited elements` |
| 7 | `; }` closing braces | Fixed in all P013 headers, C++ testers, the C++ fragment embedded in `04-compression_tester.py` (found by the independent review) and the benchmark |
| 8 | `permutation_detail::Multiset` lacks a complexity line | Added `// T: O(n * log(m + 1)), M: O(m), m distinct labels; ...` |

## Independent review

`@reviewer` (2026-10-08) found no correctness defects: an independent `lll` rank/unrank probe agreed with `kthNextPermutation` on 200,000 cases (n up to 33, extreme and random offsets, |k| >= n!), and exhaustive or random probes confirmed the supermask, Gray code, order, sign, `randDouble` and I/O behaviour. Confirmed items, all fixed: the findings file had not yet been closed (done in the plan update); a lone `}` in the C++ embedded in `04-compression_tester.py`; `kthNextPermutation`'s memory bound ignored the checked-build validation vector (now stated in [07-permutation.md](07-permutation.md)). The tester comment-cap report on `05-binarysearch_tester.cpp` is outside the rule, which covers only headers.

GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H, 2026-10-07. Baseline before changes: all seven suites passed full mode with seed 20260927. After the changes:

Package probes: 68 checked assertion probes (7 + 0 + 4 + 6 + 15 + 16 + 20 for `01`–`07`) plus compile-time rejections. Package commands are repeated in each per-header `## Commands and results`.

## P014 package record (MI03, MI14)

First implemented and verified on 2026-09-28. Re-audited on 2026-10-08 against the function-level inventory. That record called the package complete while six inventory operations were missing (`/reaudit-review` findings 1–4). The re-audit implemented them: `maximumAverageSubarray`, `mex`, `intervalCover`, `intervalPartitionAssignment`, `removeNestedIntervals` and `medianOfMedians`. The `@researcher` sweep added five more: `increasingSubsequenceLengths`, `countIncreasingSubsequences`, `adjacentSwapDistance`, `nestedIntervalCounts` and `threeWayPartition`. Header contracts moved to the per-header documents, and the code now follows the current comment cap, closing-brace rule and `res` naming. Every operation in rows `08`–`11` has an independent-oracle test (see each per-header feature-to-test map). Prerequisites P002, P006 and P013 are verified. `08` reuses DS02 prefix/difference sums, sliding extrema, Fenwick and SegmentTree, plus MI02 compression. `09` reuses the DS Fenwick. Legacy `OLD/Team Notebook/src/misc/old_kadane.cpp` is accounted for by `kadane`. No online submission was made.

`/reaudit-review` findings and resolutions (each per-header document repeats its rows):

| # | Finding | Resolution |
|---|---|---|
| 1 | `intervalCover`, `intervalPartitionAssignment`, `removeNestedIntervals` missing | Implemented with oracles above |
| 2 | `maximumAverageSubarray`, `mex` missing; evidence claimed completeness | Implemented; evidence rewritten |
| 3 | Evidence said complete while rows were partial | Rewritten; rows are verified only after implementation |
| 4 | `medianOfMedians` missing | Implemented, with an O(n) proof and benchmark |
| 5 | Unused tail updates untested | `checkSweep` checks that the state is unchanged after the last answer; the suggested mutant fails |
| 6, 10, 12 | Missing complexity lines on structs and detail helpers | Every struct and helper has a `T:`/`M:` line. Closely related declarations with no blank line between them (result records, a struct and its only producer, `kadane` under `maximumSubarray`) share one line, as in the verified `07-permutation.hpp`, to stay within the 8% comment cap. |
| 7 | `ans` naming | Renamed to `res` (also `out` and `result` in `09`/`10`) |
| 8, 11, 15, 17 | Closing braces | All four headers and all `08`–`11` testers and the benchmark normalized; `--braces` clean |
| 9 | Assert inside the sort comparator and repeated `bounds` | `interval_detail::nonempty` caches bounds once per interval |
| 13 | Per-element domain asserts | Entry `min_element`/`max_element` assert in `countingSort`; a single post-loop `assert(!bad)` in `stableCountingSort` |
| 14 | `-Wsign-compare` at the key check | `ulll(value) < ulll(alphabet)` |
| 16 | Split or off-vocabulary complexity comments | Single lines using `S`, a defined `d`, worst case first, average labelled |

`@reviewer` (2026-10-08) found no correctness defects. Its own sanitized brute-force program ran 400k cases over every new operation (maximum average, interval cover, partition and nesting across both domains and all flags, selection, chains, swaps, mex) and reported no failures or sanitizer reports. It confirmed four issues, all fixed. An unqualified `max_element` in `08` is now `std::max_element`. A literal `40n` proof that ignored additive constants is now O(n) with leading constant 40 plus measured counts. A prefix bound stated as `2^94` is now `2^95`, and a "forwards" wording for the planned `39` header is now "must forward". Its note on grouped complexity lines is recorded under findings 6/10/12.

## P015 package record (MI15, MI16, MI17)

P015 is complete in the requested order: MI15 enumeration, MI16 knapsack, MI17 cycle finding. Every owned API has a documented domain/correctness argument and feature coverage, all three full configurations passed, and both appropriate comparison benchmarks passed. The three headers are included by both Miscellaneous aggregates. MI15's newly explicit dependency on canonical MI02 bit stepping is reflected in the batch table, batch map and P015 prerequisite package map.

No owned feature or failing reproducer requires continuation. Explicit follow-up boundaries remain visible: specialist enumeration orders/ranking need their own scoped extension; MI18 retains advanced knapsack algorithms and the research leads listed above; Graphs retains whole-functional-graph queries, and alternative cycle time-memory tradeoffs remain research. These features are not claimed implemented by P015. Extended stress mode and execution on other compiler/interpreter versions were not performed. No external online acceptance is claimed, and no online submission was made.
