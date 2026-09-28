# P006 foundations — DS01 and DS02

P006 owns headers `01`–`08`, completed in DS01 then DS02 order. C01/P002 supplies the verified template, integer aliases and PBDS imports. This record describes the verified Basic contest profile; advanced structures retain their separate inventory owners.

## Common contracts

Canonical indices are zero-based and ranges are half-open. Empty construction is supported. Invalid indices, sizes, shapes and required predicate identities are preconditions checked by assertions; they remain preconditions under `NDEBUG`. A valid absent result has a documented sentinel. Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants.

Generic arithmetic must satisfy the stated algebra and keep every stored value and intermediate representable, or use explicitly modular arithmetic. `lng` is a convenient sum type, not an overflow check. Complexity counts constant-cost element operations/comparisons/copies; strings and large integers add their own costs. Allocation failure is ordinary C++ allocation failure. Structures are single-threaded and own their storage; copying produces an independent snapshot.

## DS01 APIs and arguments

| Header | Implemented contract | Correctness and cost |
|---|---|---|
| `01-dsu.hpp` | `DSU(n)`, `findSet`, `uniteSets`, `isSameSet`, `getSize`, `count`, `groups`; vertices `[0,n)`, `n <= INT_MAX`. A union returns whether distinct components merged. Groups and members are ordered by smallest vertex. | Union by size bounds uncompressed depth by `log(n)`; compression preserves the root equivalence relation. Sizes and component count change only on successful unions. Amortized inverse-Ackermann union/find, O(n) initialization/storage. With no unions during grouping, each non-root edge is bypassed at most once, giving O(n) grouping including output. |
| `02-fenwick.hpp` | `Fenwick<T>(n/vector)`, `add`, `prefixSum`, `sum`, `lowerBound`; explicit `add1`, `prefixSum1`, inclusive `sum1`, `lowerBound1` adapters. `n <= INT_MAX`. | Each cell covers the interval ending at its low-bit boundary; increasing-index construction sends each completed cell to its parent exactly once, O(n). Update/prefix/search O(log(n)), O(n) storage. Subtraction requires an additive commutative group. |
| `03-segmenttree.hpp` | `SegmentTree<T,F>(n/vector, identity, op)`, `get`, `set`, `query`, `allQuery`, `maxRight`, `minLeft`; `n <= 2^29`. | Ordered left/right accumulators preserve noncommutative products. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allQuery O(1). Bound keeps doubled indices and search arithmetic in signed `int`. |
| `04-sparsetable.hpp` | `SparseTable<T,F>(vector, op)`, nonempty `query` (associative idempotent) and `fold` (associative), inclusive `queryFast`/`querySlow` compatibility methods, diagnostic stream output; `n <= INT_MAX`. No identity or default constructor for T is needed. | Dyadic blocks preserve input order. Overlap duplicates a contiguous aggregate B, so associativity and B·B=B suffice, even without commutativity. `fold` partitions into disjoint blocks in order. Build/storage O(n log(n+1)), query O(1), fold O(log(n+1)). Only valid blocks are stored. |

Fenwick `lowerBound(x)` returns the first element index whose inclusive prefix reaches x, or n if absent; x <= 0 returns 0. Frequencies must stay nonnegative. The one-based search returns 0 if absent, avoiding `n+1` overflow; for x <= 0 it returns 1 for nonempty trees. One-based `sum1(l,r)` permits `l == r+1` when both endpoints are representable.

Segment searches require a deterministic predicate true on the identity, with truth forming a prefix as the queried interval extends. `maxRight(l,p)` returns n if no failure; `minLeft(r,p)` returns 0. Callbacks must not mutate the structure. Associativity, idempotence, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

## DS02 APIs and arguments

| Header | Implemented contract | Correctness and cost |
|---|---|---|
| `05-prefix_sum.hpp` | `PrefixSum<T>` / `PrefixSum2D<T>`: size/vector construction, `prefixSum`, `sum`, `rebuild`. `DifferenceArray<T>` / `DifferenceArray2D<T>`: size/vector construction, `add`, non-destructive `values`, `clear`, `rebuild`. Dimensions `[0,INT_MAX-1]`; 2D arguments `(row1,col1,row2,col2)`. | Prefix accumulation and inclusion-exclusion cancel exactly the unwanted intervals. Difference endpoint/four-corner updates cancel outside the added region; axis-by-axis integration is their inverse. 1D build/storage/materialization O(n+1), 2D O((n+1) * (m+1)); queries/updates O(1). Rebuild and returned arrays use a further copy of the corresponding storage. |
| `06-sqrt_decomposition.hpp` | `SqrtDecomp<T,F>`: ordered monoid `query`, `get`, `set`/`setUpdate`, `opeUpdate`, `values`, `rebuild`. `SqrtRangeSum<T>`: `affine`, `add`, `assign`, `multiply`, `sum`, `get`, `set`, `values`, `rebuild`. `n <= INT_MAX`; B=0 chooses max(1,floor(sqrt(n))), positive B specifies block width. | Blocks store ordered folds; queries concatenate full blocks and at most two tails. Generic point update rebuilds its block, O(B); query O(B+n/B). Scalar affine sum maps s to a*s+b*length and composes new tags after old tags. Partial mutations push old tags then rebuild; reads apply tags without mutation. Scalar range update/query O(B+n/B), point get O(1), point set O(B). Build/materialization O(n), storage O(n+1). |
| `07-ordered_set.hpp` | `SortedVector<T,C>` retains duplicates; `CoordinateCompression<T,C>` deduplicates by comparator equivalence. Both provide `rank`, `upperRank`, `count`, `index`, iterators/`findByOrder`, `rebuild`; compression adds `encode`. `OrderedSet<T,C>` retains native PBDS unique-key API. `OrderedMultiSet<T,C>` adds exact insertion tokens, bounds/ranks/count/select, `erase`, earliest-equivalent `eraseOne`, `clear` and `rebuild`. | Sort and binary search preserve order classes; compression preserves order, not coordinate distances. Static build O(n log(n+1)), selection O(1), ranks O(log(n+1)). Strict (key,unique-ID) PBDS ordering gives O(log(n+1)) dynamic operations. IDs stay strictly between rank sentinels; they are never recycled after clear/rebuild, so old tokens cannot erase new occurrences. All storage O(n). |
| `08-monotone_stack.hpp` | Strict/nonstrict previous/next smaller/greater indices; sliding minimum/maximum indices with leftmost/rightmost equal ties; `largestHistogramRectangle`; `largestBinaryRectangle` for bit 0 or 1; widened legacy `maxZeroSubmatrix` for arbitrary integer blockers. | Each index is pushed/popped at most once. Discarded neighbor/deque candidates are dominated by nearer/better candidates. The histogram stack emits maximal spans; the final equal-height representative inherits the full left span. Every positive matrix rectangle appears in the histogram at its bottom row. O(n) sequence time/space; matrix O(rows * cols + rows) time and O(cols) workspace. |

Prefix/difference input vectors are copied; rebuilding safely accepts references to existing storage. An empty outer vector represents 0x0; explicit dimensions also represent 0xm and nx0. Difference updates skip unused far borders, so a whole singleton update by `LLONG_MIN` does not compute its unrepresentable negation. Intermediate sums/differences still must fit T. Online range-update/query structures belong to later batches; difference arrays reconstruct offline.

`SqrtRangeSum` requires commutative ring arithmetic, including conversion of block lengths into T. Pending tag intermediates must fit too. For a new transform (a,b) after (c,d), composition is (a*c,a*d+b); assignment has a=0 and needs no inverse. Default sqrt block size balances B+n/B analytically; callers can tune B for their workloads. No universally fastest block width is claimed. Generic monoid point updates remain O(B); the scalar affine specialization covers the additional range operations.

Static ordered structures support custom/stateful strict weak orders, strings, comparator-equivalent unequal objects, and `vector<bool>`. Selection returns an iterator or `end()`; missing `index`/encoded coordinates use -1. Rebuild invalidates ranks and iterators. Dynamic multisets permit at most `INT_MAX` live elements and at most `LLONG_MAX-1` insertions per history; a token is the `(key,ID)` pair returned by insertion. Copies retain existing tokens, with subsequent histories local to each object. Assignment replaces the destination history and invalidates its old tokens; foreign or replaced-history tokens must not be passed to erase. Comparator equivalence determines key identity. Native PBDS iterator lifetime rules apply; erased elements and clear/rebuild invalidate their corresponding iterators/tokens. Erased tokens within the current history can safely be retried and return false. `less_equal` is never a valid tree comparator.

Monotone missing neighbors use -1 for previous and n for next. Windows require k>0; k>n yields no windows. Histograms support nonnegative full-range `lng` heights with `lll` area and choose the smallest `(l,r)` maximal witness; zero area returns l=r=-1 and height=0. Binary rectangle witnesses use half-open coordinates and smallest `(top,left,bottom,right)` ties; zero area has all coordinates -1. Dimensions fit int, so matrix areas fit `lng`. Rectangular binary input is asserted for the binary API; the legacy zero-only adapter treats every nonzero int as a blocker.

## Migration and ownership boundaries

Original files in `OLD` and `97-Legacy` remain byte-for-byte references. The historical monolith map retains source ranges/hashes and distinguishes rewritten destinations from unchanged extractions. Basic aggregates gain all eight headers; existing Advanced headers remain existing-unverified.

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts. Its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The outstanding weighted/parity semantics belong to `29-weighteddsu.hpp`; rollback/component metadata belongs to `09-rollbackdsu.hpp` and Graphs dynamic-connectivity orchestration. Their inventories retain this handoff.

Sparse-table inclusive query names and printed diagnostic format remain available. Its public `v` storage changes from a flat padded array to valid rows; clients should use query methods. No maintained consumer accesses that old storage. New Basic `Fenwick` and `SegmentTree` names avoid collisions with the existing Advanced `FenTree` and lazy `SegTree`; the DS05-owned `00-monoids.hpp` remains unchanged.

The legacy `97-Legacy/02-sqrtdecomp.cpp` had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction; `setUpdate` and `opeUpdate` names remain. Unreachable lazy bookkeeping and the diagnostic printer remain archived; explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness.

Future variants remain explicit: rollback/persistent/weighted DSU, range-add/multidimensional Fenwick, lazy/dual/persistent/dynamic segment trees, disjoint sparse tables, Cartesian trees and linear-preprocessing RMQ, Mo and recursive sqrt trees, dynamic balanced/persistent ordered trees, and general sliding-window aggregation. Their existence is not a gap in these Basic ownership batches. No claim is made that ordinary sparse-table preprocessing is the theoretical minimum for RMQ.

## Sources inspected

Source inspection and independent oracle tests are separate evidence. Implementations are written for the contracts above; legacy code and reference algorithms inform the audit. No online acceptance is claimed.

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [cp-algorithms: Fenwick Tree](https://cp-algorithms.com/data_structures/fenwick.html) | Zero/one-based low-bit intervals, additive/group limitations, linear construction, multidimensional/range-update boundaries. |
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [cp-algorithms: Sparse Table](https://cp-algorithms.com/data_structures/sparse-table.html) | Dyadic preprocessing, disjoint logarithmic folds, overlapping idempotent O(1) query, alternative static RMQ structures. |
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook, Antti Laaksonen](https://cses.fi/book/book.pdf), saved repository edition | Chapters 4, 8, 9, 15 and 27: PBDS, sliding windows, prefix/difference arrays, Fenwick/segment structures, DSU and block decomposition. Chapter 9 pp84–85/93 independently supports 1D/2D prefix inclusion-exclusion and endpoint differences. The chapter 15 DSU example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind, FenwickTree, SegmentTree, RMQ, SubMatrix, OrderStatisticTree references. The SubMatrix empty-input constructor is not copied; empty matrices require an explicit contract here. |
| [OI Wiki: 前缀和 & 差分](https://oi-wiki.org/basic/prefix-sum/) | Read 1D/2D prefix sums, four-corner difference signs and offline reconstruction; page update 2026-03-26. |
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| Installed GCC16 PBDS `tree_policy.hpp` and `order_statistics_imp.hpp` | Strict comparator ordering, `order_of_key` and out-of-range `find_by_order` semantics. |

Saved resource editions/checksums are recorded in `95-Resources/99-sources.json`. References were inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun and Tarjan are not claims that their original proofs were inspected.

## Feature-to-test map

Each header has one runnable Python entry and one C++ oracle suite under `96-Local Testing/02-Data Structures`, with matching filename stem. The shared `_00_runner.py` resolves paths independently of the working directory. Checks are non-removable under `NDEBUG`. Failures report seed, mode, configuration/command, operation/input or history and expected/actual values; invalid probes require SIGABRT with assertion diagnostics. Exhaustive enumerations stop on the first discovered failing case; no automatic shrinking claim is made.

| Header / C++ groups | Full-mode coverage |
|---|---|
| DSU: `graphSubsets`, `unionHistories`, `randomCases`, `balancedTree` | Every simple graph through 6 vertices, self/duplicate unions, every length-5 union history on 3 vertices; independent relabeling connectivity/size/group oracle; 150 random cases; 65,536-vertex balanced merge/compression; copies/moves/reset; 7 assertion probes. |
| Fenwick: `exhaustive`, `randomCases`, `boundaries`, `typeCases` | Signed ternary arrays through length 6 with all ranges/point deltas; quaternary frequencies through length 7 with all search thresholds/sentinels; 150 random signed/frequency histories; powers of two ±1 through 65,537; linear-build operation count; signed extrema, 128-bit, unsigned modular, exact dyadic arithmetic, argument aliasing, copy/move; 13 probes. |
| Segment: `exhaustive`, `randomHistories`, `noncommutative` | Ternary arrays through length 6, every range/search threshold; 100 arrays of up to 220 points with 250 operations; ordered strings and direct affine evaluation modulo 97, direction-sensitive forbidden-substring search, nondefault payload, identity/empty/alias/copy/move; 13 probes. |
| Sparse: `exhaustive`, `randomized`, `noncommutative` | Ternary arrays through length 7, all min/max/sum folds, inclusive adapters and exact diagnostic output; 80 random arrays and powers of two ±1 through 129; ordered concatenation/direct affine evaluation, noncommutative idempotent rectangular bands, nondefault elements/copy-only callables, ownership/copy/move; 12 probes. |
| Prefix: `arrays`, `matrices`, `randomized`, `boundaries` | Ternary arrays through length 7, all range sums/updates and depth-3 histories; every ternary matrix through 2x3 and every rectangle; 100 histories of 120 rectangle updates against literal cell loops; empty dimensions, rebuild/clear/copy/move/repeated reconstruction, argument/input aliases, int64/128-bit/modular/exact dyadic cases; 16 probes. |
| Sqrt: generic, ordered, scalar exhaustive/history and regression groups | Ternary generic arrays through length 5 with default/1/2/n/n+2/INT_MAX widths; concatenation/reverse concatenation; every length-2 affine history on small ternary arrays and every half-open range; 100 histories of 200 operations; assignment/add/multiply order, pending tags, read constness, copy/move/rebuild, 128-bit/modular/exact dyadic/alias cases; 26 probes. |
| Ordered: static, dynamic-history, random and PBDS adapter groups | Static 3-symbol vectors through length 6, five-action dynamic histories through depth 5, 12 histories of 180 random operations; independent sorted-vector oracle; rank/selection/bounds, duplicates and stale/exact tokens, reset/copy/move, ascending/descending/stateful equivalence, strings/bool/int64 extrema, ID exhaustion; 2 probes. |
| Monotone: boundaries, histograms, matrices and random/generic groups | Ternary arrays through length 7 for all neighbor/window policies; quaternary histograms through length 7 against every interval; all shapes through 4x4 with at most 12 cells against every rectangle for both bits; 100 random cases, arbitrary legacy blockers, generic comparator equivalence, full `lng` heights and a 100,000-bar plateau; independent witness validation/complement symmetry; 11 probes. |

Quick modes reduce enumeration lengths/history depths/random counts and run optimized plus checked builds. Full runs every feature above in optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`), and sanitizer (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`) builds. Leak checking is enabled. Ordered checked builds instead use `_GLIBCXX_ASSERTIONS`: GCC16 PBDS `debug_map_base` default-constructs its diagnostic comparator, which can disagree with a valid supplied stateful comparator. A reproduced equivalence-width example aborts only in that debug layer; all comparator oracle cases remain in the checked suite. This is an instrumentation limitation, not a skipped algorithm feature.

Stress modes extend the enumerations/random counts: DSU history depth 6 and 600 random cases; Fenwick 600 cases and boundaries through 262,145; segment/sparse exhaustive lengths 8/9; prefix length 8/history depth 4/400 random cases; monotone length 9, all binary matrices through 4x4 and million-bar plateau. Each suite's source records its concrete mode limits. Declared algebra/monotonicity/representability conditions and impractically large allocations are justified contracts, not runtime proof by finite tests.

## Verification record — 2026-09-27

All eight per-header entries passed `--mode full --seed 20260927`: 24 configuration runs and all 100 assertion probes. DS01 completed its full runs and Basic/All multiple-TU check before DS02 implementation began. Full runs include 564,922 segment checks, 3,088,896 sparse checks, 681,510 prefix checks, 495,927 ordered checks and 808,326 monotone checks per configuration; DSU/Fenwick/sqrt report named corpus groups instead of a scalar check count. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership.

Compiler: GCC 16.2.1 20260810, GNU++20; Python: CPython 3.14.7; Linux x86-64. Actual full commands used the eight `NN-name_tester.py` entries with the options above. The segment sanitizer retry additionally used `--configuration ASan-UBSan`. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass.

`python3 '96-Local Testing/02-integration.py'` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation. The per-header full suites already supplied P006 sanitizer checks; unrelated Core sanitizer corpora were not repeated. `python3 '96-Local Testing/03-consistency.py'` passed inventory/batch/package paths, aggregates, source/archive hashes and Markdown links with zero errors. Original archived bytes remain unchanged.

Shared discovery also passed all eight quick suites with seed 42 when invoked by absolute path from `/tmp`: `01-run.py --mode quick --seed 42 --filter '02-Data Structures' --no-integration`. This checks working-directory independence, environment option forwarding and discovery of the new entries; it does not replace their full runs.

To reproduce the complete package corpus through shared discovery, or extend it:

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --seed 42 --rounds 3 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Stress mode is available but was not run as completion evidence. GCC14 itself, other standard-library versions and other platforms were not executed. There are no remaining P006-owned implementation or verification gaps. The weighted/parity/component metadata handoff above belongs to later packages, which remain planned. No online submissions or acceptance claims were made.

## Construction comparison

The [benchmark driver](<../96-Local Testing/02-Data Structures/90-foundations_benchmark.py>) and [record](<../96-Local Testing/02-Data Structures/90-foundations_benchmark.json>) compare the final Fenwick linear constructor with constructing zeros and applying n point additions. Both are checked against an independent sum. Run:

```bash
python3 '96-Local Testing/02-Data Structures/90-foundations_benchmark.py' --runs 7 --seed 20260927
```

Recorded CPU: Intel Core i9-11900H @ 2.50GHz; GCC16, `-std=gnu++20 -O2 -DNDEBUG`, seed 20260927, uniform integer values `[0,100]`, seven independent process samples, one untimed linear warmup per row. Both timed paths include allocation, construction, a checksum query and destruction. The source vector and warmup remain outside the timed construction; peak live vector payload is approximately `3 * n * sizeof(lng)` plus allocator/runtime overhead. Raw samples and source hashes are retained. Other verification work shared the host, so these are noisy observations without a timing gate.

| n | Builds per sample | Linear median total (ms) | Point-add median total (ms) |
|---|---|---|---|
| 32 | 8192 | 0.299186 | 0.640771 |
| 4096 | 64 | 0.276973 | 1.267312 |
| 262144 | 1 | 1.203464 | 3.001825 |

The linear constructor is both asymptotically appropriate and faster in these measured cases. No ISA kernels, Barrett/Montgomery use or empirically selected dispatch thresholds were introduced. The other families use their standard contest algorithms with stated preprocessing/storage tradeoffs; no universal performance superiority is claimed.
