# 08 Python — Notes

These rules bind every module in the folder; the operation lists are in the index.

## Ownership

- Every row except `_051_pypy_idioms.py` mirrors C++ owner rows. The C++ inventory is the full contract, and the Python row is its documented reduction. Owners are given as `folder/row` (`01` Core, `02` Data Structures, `03` Geometry, `04` Graphs, `05` Mathematics, `06` Miscellaneous, `07` Strings):
  - Basic: `_01` 01/19, 06/50; `_02` 05/02; `_03` 05/01, 05/04, 05/06–09, 05/21; `_04` 05/05, 05/12, 05/19, 06/07, 06/53; `_05` 02/05, 02/08, 02/27, 02/30, 06/04, 06/08, 06/11, 07/09; `_06` 02/01; `_07` 02/02, 02/11; `_08` 02/03; `_09` 04/01–03, 04/05, 04/06, 04/08, 04/43, 06/51; `_10` 07/01–03, 07/05, 07/08, 07/15; `_11` 03/01–04, 03/15; `_12` 05/67, 06/13, 06/29, 07/18, 07/19; `_13` 02/04, 02/28, 02/54; `_52` 02/25.
  - Advanced: `_14` 05/07, 05/08, 05/31; `_15` 04/19, 04/30, 04/37; `_16` 04/21–23; `_17` 04/07, 04/14–17; `_18` 02/09, 02/29, 04/26; `_19` 02/18; `_20` 01/16, 05/13, 05/14, 05/22; `_21` 01/10, 01/12, 05/16; `_22` 02/62, 04/40, 06/06, 06/12, 06/22, 07/19; `_23` 02/26, 07/04; `_24` 07/06; `_25` 07/07, 07/10; `_26` 02/23, 02/32, 02/33, 02/59, 04/07, 06/16, 06/20, 06/21; `_27` 06/01, 06/18; `_29` 02/00, 02/12; `_30` 05/27, 05/53, 06/29, 06/34; `_31` 02/19, 02/36, 02/64; `_32` 02/21, 02/22; `_33` 04/18; `_34` 04/09, 04/27; `_35` 04/10; `_36` 04/05, 04/12, 04/13; `_37` 04/20, 04/23; `_38` 05/03, 05/05, 05/10–12, 05/18, 05/30, 05/31, 05/33; `_39` 05/24; `_40` 04/13, 04/43, 06/30, 06/35, 06/36; `_41` 03/01, 03/02, 03/06, 03/08, 03/14; `_42` 04/11, 06/14; `_43` 04/73, 06/31–33, 06/52; `_53` 07/13; `_54` 07/14, 07/41; `_55` 03/05, 03/07, 03/10, 03/11, 03/13, 03/15; `_56` 02/17; `_57` 05/20; `_58` 05/25; `_59` 04/24, 04/25, 04/35, 04/36, 04/49, 04/67; `_60` 02/06, 02/53; `_61` 02/24.
  - Esoteric: `_44` 03/12; `_45` 02/40; `_46` 07/29, 07/36; `_47` 05/15; `_48` 05/40, 05/64; `_49` 06/26; `_62` 06/25; `_50` 05/17; `_63` 06/45.
- Shared tasks have one owner: GF(2) elimination is `_021_matrix.py`, while `_022_bitset.py` keeps bit tricks and `XorBasis`. Meet-in-the-middle is `_040_state_search.py`. LCS is `_012_dp.py`, with the bit-parallel `bitsetLcs` in `_022_bitset.py`; longest common substring is `_025_suffix.py`. The Basic modular inverse is `_003_number_theory.py` and general CRT `_038_number_theory_advanced.py`. `minRotation` is `_010_strings.py` and Lyndon factorization `_054_lyndon.py`. Running median and removable heaps are `_052_heaps.py`, not `_031_ordered_multiset.py`.

## Names

User decision 2026-11-22 (`/tidy`) applied `04-python.md` (Style) to every row: an operation with a C++ counterpart takes the C++ name and contract, and one C++ operation stays one Python operation. The rows changed as follows.

- Removed as standard-library duplicates: `lowerBound`, `upperBound`, `countRange`, `bisectKey` (`bisect` with `key=`); `modPow` (`pow(a, e, m)`); `isqrt` (`math.isqrt`); `floorDiv`, `floorMod`, `divModFloor` (`//`, `%`, `divmod`); `comb`, `perm` (`math.comb`, `math.perm`); `popcount`, `highbit` (`int.bit_count`, `int.bit_length`); `heapMerge`, `nSmallest`, `nLargest` (`heapq.merge`, `nsmallest`, `nlargest`); `limitDenominator` (`Fraction.limit_denominator`); `norm` (`math.hypot`); the `_27` type `Rng` with its members (`random.Random`); `weightedChoice` (`random.choices`).
- Removed as doubled operations, with the survivor in parentheses: `isBipartite` (`bipartiteCheck`); `mexOf` (`_05.mex`); `longestIncreasingSubsequenceLength` (`longestIncreasingSubsequence`); `fibonacci` (`fibonacciPair`); `primeFactors` (`factorize`); `primePi` (`primeCount`); `assignmentMin` (`hungarian`); `suffixArrayInt` (`suffixArray` with an alphabet bound); `rangeKthSmallest` (`PersistentValueTree.rangeKth`); `setFalse` (`setValue` on a negated literal); `floydPath` (`restorePathAllPairs`); `kthRootMod` (`kthRootModPrime`); `dotF`, `crossF` (`_11.dot`, `_11.cross`); `necklaceCountColors` (`necklaceCount`); `convexHullCollinear` (`convexHull` with `keep_collinear`); `SortedList.lt` (`kth(rank(x) - 1)`); `Doubling.jump` (`FunctionalGraph.jump`).
- `_28_io_advanced.py` merged into `_001_io.py` under the `01-Core/19-fastio.hpp` names: `nextInt`, `nextToken`, `nextLine` became `readInt`, `readToken`, `readLine`; `read` replaces `readAll`, `readInts`, `nextInts`, `readTokens`, `readMatrix` and `readGraphEdges`; `writeValue` replaces `writeInts` and `writeAll`. `FastReader`, `FastWriter`, `remaining` and `tokenizeBytes` were dropped as a second implementation of `FastIO` and `bytes.split`. A line is written by `writeLine`, the C++ `Interactive` name, because a module-level `print` would shadow the builtin.
- Renamed to the C++ name: `DSU.roots` to `leaders`; `SortedList` `add`, `remove`, `discard`, `pop`, `bisectLeft`, `bisectRight`, `ge`, `gt`, `le`, `min`, `max`, `len` to `insert`, `erase` (all equivalents), `eraseOne`, `eraseKth`, `rank`, `upperRank`, `lowerBound`, `upperBound`, `prev`, `front`, `back`, `size`; `sieve` and `primeList` to `SieveOfErath`; `linearSieve` and `spfTable` to `LinearSieve`; `segmentedSieve` to `SegmentedSieve`; `onSegment` to `contains`; `segmentsIntersect` to `intersect`; `sortByAngle` to `argumentSort`; `pointInConvex` to `ConvexPolygonQuery.contains`; `gridPathCount` to `gridPaths`; `eulerTour` to `EulerTour`; `virtualTree` to `VirtualTree`; `reroot` to `Rerooting`; `Doubling` to `FunctionalGraphFold`; `intervalDp` and `reconstruct` to `IntervalDP.solve` and `restore`; `convexDiameter` to `ConvexCalipers.diameter`; `directedMst` to `DirectedMst.arborescence`; `rangeMode` to `RangeMode`; `Dpll` to `SatSolver`.
- `_049_sat.py` and `_050_approximation.py` split along their C++ rows: `DancingLinks` moved to `_062_exactcover.py` (06/25) and the sketches to `_063_probabilistic_sketches.py` (06/45).

## Interpreter and Baseline

- Post-3.10 APIs such as `math.cbrt`, `itertools.batched` and `Fraction.is_integer` are not used unconditionally. Modules rely on these 3.10 features: `int.bit_count`, `bisect` `key=`, `itertools.pairwise`, `math.isqrt`, `comb`, `perm`, `lcm`, multi-argument `gcd`, `pow(a, -1, m)` and `functools.cache`.
- PyPy specifics beyond `04-python.md`: bytes I/O, no `locals()` or `sys._getframe()` in hot code, strings built with `join`, and the generator trampoline `_051_pypy_idioms.bootstrap`. Integers above 64 bits run in non-JIT support code on PyPy, so bounds count bit length separately from element count.
- No module raises the recursion limit, reseeds the global RNG or performs I/O at import time.
- `_051_pypy_idioms.modMul64` uses the float-reciprocal method and `__pypy__.intop` behind an `isPyPy` guard, with a plain-`int` fallback that returns identical results. It is the only module allowed to import a PyPy-specific name, and that import is guarded.
- `_051_pypy_idioms.runWithStack` runs its function in a `threading` thread with a raised stack size.

## Numeric Policy

- Floor division and modulo are Python's (floor toward negative infinity).
- `Fraction(float)` keeps the binary float, not the intended decimal; exact rational inputs are integer pairs or strings.
- Python big-int bit operations are not constant-time machine instructions: `_22`, `_45` and `_56` charge `O(n / 64)` per word-level operation and state the bit width of every mask. Negative bit patterns are rejected or normalized explicitly.
- Deterministic Miller–Rabin in `_14` is proved only below the recorded bound (3.3e24 with the fixed base set); above it `isProbablePrime` is the only honest name. Arbitrary precision does not extend the proof domain.
- `_020_polynomial`: NTT paths require an NTT-friendly prime; `convolutionArbitraryMod` reconstructs through three NTT primes; `convolutionExact` packs coefficients into one big int under a documented coefficient bound. FPS inverse, log, exp and sqrt require the characteristic, unit and square-root preconditions their docstrings name.
- `_39`: XOR, AND and OR inverse transforms stay exact over integers (divisibility by the transform length) or use modular unit inverses; the float `/=` of the reviewed source is never copied. Bitmask transforms require a power-of-two length; gcd and lcm transforms require integer indices within the stated bound.
- `_21`: `solveLinearApprox` uses partial pivoting with a stated tolerance and reports the residual.
- `_37`: potentials are initialized by Bellman–Ford when any cost is negative, and a negative cycle in the initial residual graph is a rejected input. Zero potentials are correct only for nonnegative costs.

## Structural and Domain Contracts

- Containers: vertices are `0..n-1`, ranges are half-open `[l, r)`, and an empty-range fold returns the identity. A monoid is a pair of `op`/`e` callables (acted monoids add `mapping`, `composition`, `id`) in place of the C++ monoid object.
- Graph inputs are adjacency lists (`buildAdj`) or CSR arrays (`buildCsr`). Multigraphs, self-loops and disconnected graphs are accepted unless a function says otherwise; `_34` edge ids distinguish parallel edges, and `_35` reports isolated vertices and empty graphs.
- `_01`: `os.read`/`os.write` paths are benchmarked against buffered `sys.stdin.buffer` before use. Interactive programs use `Interactive`, which flushes after every query.
- `_031_ordered_multiset`: `list.insert` is linear, so `bisect`-based structures never claim logarithmic insertion. The module states the measured per-operation cost of the bucketed list (amortized square-root blocks) and of the treap (expected logarithmic), derived independently of PyRival's claims. `FastSet` (64-ary bit tree) is the integer-key fast path.
- `_13`: the sparse table's `O(1)` query holds for idempotent operations only; `DisjointSparseTable` is the non-idempotent alternative, and empty queries need an explicit identity. `LinearRMQ` builds in linear time over word masks, with bounds derived independently of PyRival `RangeMinimumQuery2`.
- `_15`: `MaxFlow` augments iteratively; the recursive augmentation and fixed capacity-scaling loop of PyRival `dinic.py` are not copied.
- `_08`, `_29`: noncommutative monoids fold left to right; `maxRight`/`minLeft` predicates must be monotone and accept the identity. `_29` has one `apply(l, r, f)`, and a point update is `apply(p, p + 1, f)`, since Python cannot overload by arity.
- `_17`: `Rerooting` is the one rerooting owner, with `combine` and `finalize` callbacks. Trees are forests where stated, and node and edge indices are zero-based.
- `_18`: the rollback DSU uses union by size without path compression. `WeightedDSU.uniteSets` returns `False` when the two are already connected, as C++ Data Structures row 29 does. Offline dynamic connectivity is the segment-tree-over-time reduction.
- `_19`, `_56`: persistent trees use parallel node arrays with explicit version ids. The wavelet matrix keeps its compact bit vector internal, as C++ row 17 does; the public rank/select type is `_45.SuccinctBitVector`.
- `_26`: Mo variants state their sort order (block or Hilbert), tie rules and mutation rollback, and avoid a tuple per query by packing (`_51.packPair`).
- `_27`: every randomized routine takes a caller-controlled `random.Random`; Monte Carlo and Las Vegas labels are explicit. `_14` owns its Pollard-rho loop and shares only this RNG policy.
- `_40`: `aStar` reopens closed states under a merely admissible heuristic; a no-reopen contract requires a consistent one. Depth and time budgets and duplicate-state policies are parameters, not defaults.
- `_41`: tolerances are scale-aware and documented; Welzl's minimum enclosing circle shuffles with the caller's seed; residual checks accompany every returned intersection.
- `_43`: digit DP, profile DP and interval DP are templates with callbacks and do not hide problem-specific state proofs. `_58` owns Sprague–Grundy and retrograde analysis.
- `_44`, `_45`, `_46`, `_49`, `_50`, `_62`, `_63` have low PyPy value (big-int masking, pointer-heavy nodes, exponential search, probabilistic structures rarely needed in contests). They stay Esoteric with explicit resource bounds.

## Legacy Migration

- `OLD/Team Notebook/src/math/old_crt.py`: `inv` is the reference for `_03.inverseXgcd` and `findMinX` (pairwise-coprime CRT) for `_38.crt`. Both lack input validation and the non-coprime case, which the active modules add.
- `OLD/Team Notebook/src/misc/old_lcs.py`: the LCS length and witness reference for `_12.lcsWitness`. It prints instead of returning and uses `O(n * m)` memory.
- `OLD/Team Notebook/src/misc/old_lcssubstr.py`: the longest-common-substring witness reference for `_25.longestCommonSubstring`, which returns spans instead of a printed string.

## Testing and Omissions

- Testing follows `05-testing.md`. Python tests add singular and composite-modulus algebra and exact/approximate pairs, and cross-check the C++ variants without copying their assumptions. External oracles may use third-party packages.
- Deliberate omissions, added back only for a concrete Python use case with an inventory update: Python Barrett, Montgomery, modint and bigint wrappers; ISA kernels; weighted blossom; dynamic planarity; segment tree beats; link-cut and Euler-tour trees; implicit treap sequence operations (range reverse); persistent union-find; high-dimensional geometry; multivariate symbolic algebra; large-FPS composition; Min_25 and Meissel–Lehmer beyond Lucy; k-shortest walks; min-cost b-flow; Dirichlet series; nim product; research string indexes beyond the explicit rows.

## P275 Re-Review 2026-11-10

P275 re-read every omission recorded here before 2026-11-08 under `00-Guidelines/03-cpp.md` (One API per Operation, 2026-11-08), its legacy guard and the user decisions of 2026-11-10 (`00-Guidelines/23-Sweep Ledgers/omissions.md`, where low PyPy value stays a reason); see [the re-review](../../00-Guidelines/23-Sweep%20Ledgers/Docs/00-duplicate-rereview.md). The remaining omissions stay: the PyRival, Library Checker and cp-algorithms items excluded for low PyPy value, and the one-problem items, which the subset leaves out by design (`04-python.md`).
