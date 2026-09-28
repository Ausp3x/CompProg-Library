# 08 Python — deliberate contest subset

All numbered algorithm modules below are **planned**, including additions discovered in this audit; none is implemented or verified. Python is a deliberate practical subset of C++ folders 01–07, not an automatic port of every research family. Use Python 3.10 syntax/standard-library APIs on CPython/PyPy 3.10 or newer. Native `int`, `Fraction`, containers and standard algorithms replace unnecessary C++ numeric wrappers.

Names such as `_01_io.py` are importable despite their numeric ordering. Basic/Advanced/Esoteric are inventory sections, not package directories. Keep explicit `_98_basic.py`/`_99_all.py` re-exports and avoid import-time I/O, tests, seed changes or recursion-limit changes. Examples/copied submissions identify dependencies; no automatic Python bundler is implied. Scope descriptions are implementation worklists, not claims of current coverage.

## Basic

| Proposed module | Feature checklist | Status and references |
|---|---|---|
| `_01_io.py` | Buffered bytes input/output, token/line parsing, signed integers, EOF and interactive flushing; use `sys.stdin.buffer`, explicit memory estimates and a streaming alternative for very large input. | Planned |
| `_02_search.py` | Standard `bisect` for sorted arrays, first/last monotone predicate, safe integer bounds and discrete/real unimodal search with precision conditions; insertion into a list remains linear. | Planned |
| `_03_number_theory.py` | Native `math.gcd`/`lcm`/`isqrt`, xgcd, exact gcd/inverse/power helpers, signed division conventions, basic prime sieve and integer roots; `pow(a,b,m)`/`pow(a,-1,m)` where valid. CRT/floor sums/general roots are Advanced. | Planned |
| `_04_combinatorics.py` | Exact `math.comb`/`perm`, factorial and prime-mod factorial/inverse-factorial tables, Catalan/ballot/derangements and Fibonacci fast doubling; validate prime/table bounds. Lucas/composite moduli are Advanced. | Planned |
| `_05_sequences.py` | LIS strict/non-strict with witnesses, inversion count, Kadane, coordinate compression, prefix/difference arrays, monotone stacks/deques and sliding-window minima/maxima. | Planned |
| `_06_dsu.py` | Iterative path compression, size/rank/component count, merge result and component enumeration; reversible/parity variants have separate Advanced ownership. | Planned |
| `_07_fenwick.py` | Linear build, point-add/prefix/range sum, frequency kth/lower-bound with nonnegative-count requirement, range-add variants and empty/endpoints; lists/local bindings where useful. | Planned |
| `_08_segmenttree.py` | Iterative monoid tree, point set, half-open range fold and boundary search; noncommutative operation order and predicate identity rules. Lazy actions are Advanced. | Planned |
| `_09_graph.py` | BFS/0–1 BFS/DFS, components, bipartite coloring/odd-cycle witness, topological order/cycle witness, nonnegative Dijkstra, Kruskal/Prim and SCC/condensation; iterative traversals, paths and multigraph conventions. Negative/all-pairs paths and specialized graph families are separate Advanced modules. | Planned |
| `_10_strings.py` | Prefix/KMP occurrences, Z, Manacher, minimal rotation and simple rolling hash with explicit collision/seed policy; byte/string alphabet and empty-pattern semantics. Suffix indexes are Advanced. | Planned |
| `_11_geometry.py` | Exact integer tuples, dot/cross/orientation, segment membership/intersection, convex hull, polygon area and point-in-polygon; rational intersections use `Fraction`. Boundary/collinear/duplicate rules explicit; approximate circle geometry is separate. | Planned |
| `_12_dp.py` | Practical 0–1/unbounded knapsack, counting versus optimization transitions, simple sequence/grid DAG DP and witness/memory tradeoffs. LCS length and witness plus Levenshtein edit distance/alignment use practical quadratic-time and reduced-memory variants; bitset LCS belongs to `_22_bitset.py`, longest common substring to `_25_suffix.py`. Digit/profile/state-compressed DP and asymptotic optimization are Advanced. | Planned; legacy `OLD/Team Notebook/src/misc/old_lcs.py` supplies the LCS witness reference |
| `_13_sparsetable.py` | Static idempotent RMQ/gcd sparse table with half-open nonempty ranges and argmin tie rules; `O(n * log(n))` stored data. Empty queries require an explicit identity/invalid contract; no unjustified constant-time claim for arbitrary monoids. | Planned |

## Advanced

| Proposed module | Feature checklist | Status and references |
|---|---|---|
| `_14_prime_factor.py` | Moderate-bound SPF, deterministic 64-bit Miller–Rabin, reproducible Brent/Pollard rho and prime-power/divisor output; retry/time budget and bounded deterministic bases. Native arbitrary precision does not extend the primality proof domain. | Planned |
| `_15_flow.py` | Integer-capacity Dinic, edge flow/min-cut witnesses, reset/reuse and optional lower-bound circulation reduction; avoid uncontrolled recursive DFS, audit self-loops/parallel edges and capacity scaling bounds. | Planned |
| `_16_matching.py` | Hopcroft–Karp with minimum vertex cover and rectangular Hungarian assignment; cardinality/cost conventions and impossible assignments explicit. General weighted blossom is intentionally outside this Python subset. | Planned |
| `_17_tree.py` | Binary-lifting/Euler-RMQ LCA, distances/path queries, HLD, virtual tree, diameter, centroid decomposition and rerooting DP; iterative traversal, forests and edge/node indexing. Rerooting has one canonical owner here. | Planned |
| `_18_rollback.py` | Rollback DSU, parity/weighted consistency variants and offline dynamic connectivity; checkpoint/undo and invalidation rules, no path compression where it breaks rollback bounds. | Planned |
| `_19_persistent.py` | Persistent segment tree/counting order statistics using parallel node arrays; version validity, structural sharing and memory costs. General persistent object graphs are omitted. | Planned |
| `_20_polynomial.py` | Naive/NTT and arbitrary-mod CRT convolution for measured practical sizes, optional exact integer-packing backend with coefficient bounds, arithmetic/div-rem/evaluation and selected FPS inverse/log/exp/pow/sqrt; explicit characteristic/unit and precision contracts. | Planned |
| `_21_matrix.py` | Dense multiplication/power, modular rank/determinant/solve/inverse/nullspace and exact rational elimination via `Fraction`; separately named floating solve with pivot/tolerance/residual policy. General optimized sparse/eigen/SVD engines are omitted. | Planned |
| `_22_bitset.py` | Native `int` bitset operations, shifts/masks, subset-sum reachability, graph closure and bitset LCS; GF(2) rank/solve/nullspace and xor basis with maximum/kth/witness variants. State bit-width and limb-cost bounds, and reject/normalize negative bit patterns explicitly. | Planned |
| `_23_trie.py` | Prefix and binary xor tries, duplicate counts/deletion, max/min/kth xor as scoped, memory-aware dictionary/array transitions and terminal conventions. | Planned |
| `_24_aho.py` | Failure automaton via `deque`, duplicate IDs, occurrence enumeration/counts and empty-pattern policy; sparse/dense transition choices and output-size costs. | Planned |
| `_25_suffix.py` | Suffix array (doubling baseline; SA-IS only if a justified measured alternative), Kasai LCP/RMQ and exact substring queries; suffix automaton with occurrences/distinct substrings and witness conventions. Longest common substring across two inputs returns its length and matching spans, with separators outside the input alphabet where used; LCS subsequences belong to `_12_dp.py`. Explicit alphabet/time/memory scope. | Planned; legacy `OLD/Team Notebook/src/misc/old_lcssubstr.py` supplies a substring witness reference |
| `_26_offline.py` | Mo (including updates/tree variants only with clear support), CDQ dominance and parallel binary search; sorting ties, mutation rollback and tuple-allocation costs. | Planned |
| `_27_randomized.py` | Caller-controlled `random.Random`, unbiased bounded draws, shuffle/reservoir sampling and hash-base selection; reproducible seeds and Monte Carlo/Las Vegas labels. Factorization owns its Pollard-rho implementation; share RNG policy only. | Planned |
| `_28_io_advanced.py` | Low-allocation scanner and `os.read`/`os.write` paths, guarded by platform/pipe/interactive behavior; benchmark against buffered standard I/O before adding complexity. | Planned |
| `_29_lazysegmenttree.py` | Monoid-action lazy propagation, composition order, assignment/add examples and boundary searches; iterative or safely bounded traversal and explicit Python object/memory overhead. | Planned |
| `_30_dp_optimization.py` | Divide-and-conquer/Knuth/monotone-queue/SMAWK optimizations, bounded-knapsack optimization and alien/Lagrangian search under proved monotonicity/Monge assumptions; line-envelope bridge. Rerooting calls the tree module. | Planned |
| `_31_ordered_multiset.py` | Insert/erase/count, rank/kth/predecessor/successor with duplicates; bucketed sorted lists and/or a compact randomized treap chosen by measured workloads. No false logarithmic insertion claim from using `bisect`. | Planned |
| `_32_line_envelope.py` | Monotone/deque or binary-search CHT, exact integer separators, duplicate slopes and query ties; dynamic/compressed-coordinate Li Chao with line/segment insertion and explicit x-domain. | Planned |
| `_33_twosat.py` | Implication clauses, forced literals, xor/equivalence helpers, satisfiability and assignment via shared SCC; literal encoding and unsatisfiable result explicit. | Planned |
| `_34_lowlink.py` | Undirected bridges/articulation points, edge/vertex biconnected components and block/bridge forests; iterative DFS, edge IDs, disconnected graphs, self-loops and parallel edges. | Planned |
| `_35_eulertrail.py` | Hierholzer for directed/undirected multigraphs, feasibility/connectivity, selected start and edge-ID witness; isolated vertices and empty graph conventions. | Planned |
| `_36_shortest_paths.py` | Bellman–Ford with reachable negative-cycle witness/affected vertices, Floyd–Warshall with path recovery and bounded Johnson alternative where useful; unreachable/negative-infinite outcomes distinct. | Planned |
| `_37_mincostflow.py` | Successive shortest augmenting paths with potentials, exact integer costs/capacities, limited-flow/min-cost/max-flow result and cost-flow slope; negative initial costs require valid potential initialization and stated negative-cycle policy. | Planned |
| `_38_number_theory_advanced.py` | Generalized CRT/Garner, signed floor sum, multiplicative functions, prime/composite binomial/Lucas, BSGS/exBSGS, order/primitive roots, prime modular square/kth roots and modular towers; finite domains/retries/overflow-free but nonconstant big-int costs. | Planned |
| `_39_transform_algorithms.py` | SOS subset/superset zeta/Möbius, OR/AND/XOR convolution, gcd/lcm transforms and practical subset convolution; exact integer inverse divisibility versus modular unit inverse, power-of-two lengths for bitmask transforms, integer-index bounds for gcd/lcm transforms, and input mutation explicit. | Planned |
| `_40_state_search.py` | Backtracking/branch-and-bound, iterative state traversal, bidirectional BFS, admissible-heuristic A*/IDA* and meet-in-the-middle subset/constraint search; complete witnesses, duplicate-state policy, depth/time budgets and exponential bounds. A* with merely admissible inconsistent heuristics must reopen improved closed states; a no-reopening contract requires consistency. | Planned |
| `_41_geometry_float.py` | Separate approximate point/line/circle operations, distance/projection/reflection/rotation, line/circle and circle/circle intersections, tangents and minimum enclosing circle; scale-aware tolerances, degeneracies, reproducible random seed and residual checks. | Planned |
| `_42_functionalgraph.py` | Cycle/tail decomposition, kth successor, orbit length, reachability/distance, entry point and aggregates along jumps; disconnected functional components and very large Python-int k. | Planned |
| `_43_dp_advanced.py` | Digit DP with leading-zero/bound conventions, subset/TSP/Hamiltonian DP, profile/plug DP for bounded width, interval and game DP, state canonicalization and reconstruction; no single universal template conceals problem-specific state proofs. | Planned |

## Esoteric

| Proposed module | Feature checklist | Status and references |
|---|---|---|
| `_44_exact_geometry.py` | Fraction-based sweep intersections and rational output, exact degeneracies and big-integer growth; practical ceiling explicit. General 3D/arrangement/geometric algebra systems are omitted. | Planned |
| `_45_succinct.py` | Static rank/select over packed arrays/native ints with samples; charge Python big-int masking/popcount costs honestly and measure representation overhead. | Planned |
| `_46_fmindex.py` | BWT/FM backward search and sampled locate using suffix/rank helpers, for research or permissive limits; alphabet and sampling costs. | Planned |
| `_47_algebraic.py` | Berlekamp–Massey, Kitamasa/Bostan–Mori and recurrence/rational-series coefficients; modulus assumptions and convolution crossover. General algebraic-number/finite-field packages are omitted. | Planned |
| `_48_combinatorial_species.py` | Exact/modular Burnside/Pólya, rotation/reflection orbits and useful generating-function helpers; a selected toolkit rather than a full symbolic species system. | Planned |
| `_49_sat.py` | Small DPLL/CDCL experiment and Algorithm X/exact cover, explicit exponential/resource bounds and witnesses. 2-SAT is separately owned by `_33_twosat.py`. | Planned |
| `_50_approximation.py` | Selected Bloom/count sketches/reservoir statistics and adaptive numerical integration/root bracketing, with explicit error/confidence and convergence contracts; exact variants remain separate. | Planned |

## Python-specific acceptance and omissions

- Prefer built-in arbitrary precision `int`, `pow`, `divmod`, `math.isqrt`/`gcd`/`comb`, `deque`, `heapq`, `bisect`, `itertools` and `Fraction`. Exact rational construction from `float` preserves the binary float, not an intended decimal; choose integer pairs or decimal strings when that is the intended exact value. APIs added after 3.10 in current documentation cannot be used unconditionally.
- Preserve floor-division/modulo semantics, mutability, iterator/cache lifetime, recursion depth and output costs. Python big-int bit operations are not constant-time machine instructions; distinguish graph size and coefficient/bit length in bounds. Never globally raise recursion limits or mutate RNG state on import.
- Validate on CPython/PyPy where available; absent interpreters are coverage gaps. Benchmark practical domains before promising Python suitability. External test oracles/packages are allowed, while canonical algorithm modules use the standard library.
- Differential/exhaustive small-case tests cover empty/negative/duplicate inputs, ties, disconnected multigraphs, overflow-sized integers, singular/composite-modulus algebra, mutation and exact/approximate distinctions. Cross-check appropriate C++ variants without copying their assumptions blindly.
- No custom Python Barrett/Montgomery/modint/bigint wrappers, ISA kernels, automatic port of every Core optimization, advanced integer factorization, general blossom, dynamic planarity, high-dimensional geometry, full multivariate symbolic algebra or research string indexes beyond the explicit rows. These are deliberate omissions; add only after a concrete Python use case and inventory update.

## Research notes

The [2026-09-27 inventory audit](../00-Guidelines/16-inventory-audit.md) records exact sources and read scope: PyRival catalog plus selected RMQ/ordered-list/2-SAT/xor/CHT/FWT/flow bodies, ac-library-python README/min-cost-flow body, and Python `bisect`/`Fraction` documentation. This is feature/contract research, not a verification of those libraries.

Two reviewed-source traps are explicit future test requirements: PyRival's FWT inverse uses `/=` and therefore converts integer coefficients to floats; exact inverse paths here must remain exact. The reviewed Python min-cost-flow implementation initializes potentials to zero, so negative-cost support cannot be inferred merely because edge insertion accepts a signed cost. Ordered-list complexity and small-block RMQ claims also need independent derivation for the chosen implementation.
