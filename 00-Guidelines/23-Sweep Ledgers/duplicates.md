# Duplicates

Same-context forms and second implementations under `00-Guidelines/03-cpp.md`, One API per operation. `/duplicate-sweep` writes the package sections and `/prune Pxxx` consumes them. P275 re-classified every package section and kept entry under the 2026-11-08 rule on 2026-11-10 ([Docs/00-duplicate-rereview.md](Docs/00-duplicate-rereview.md)).

## Recorded decisions

Decided by the user since 2026-10-10; binding, and only the user changes them.

### Changes

- **P010** flags choosing between operations become named mirrors (user decision 2026-11-14): `topologicalSort(g, true)` → `topologicalSortSmallest` (GR01, done), `dagShortestPaths(g, s, true)` → `dagLongestPathsFrom` (`dagLongestPaths` stays retired), `kruskal`/`primSparse`/`boruvka(g, true)` → `kruskalMaximum`/`primSparseMaximum`/`boruvkaMaximum`, `eulerianTrail(g, start, true)` → `eulerianTrailSmallest`; `KruskalReconstruction(g, maximum)` keeps its constructor parameter. Row 03's lexicographically smallest inverse permutation is `topologicalSortEarliest`.
- **P012** Mathematics row 01 `divMod` is named `divModFloor`, as Core's flooring `InfInt::divModFloor`, since Core's `divMod` truncates; this reverses P275's `divModFloor` → `divMod` rename (`Docs/00-duplicate-rereview.md`) (user decision 2026-11-12).
- **P090** `08-Python` row `_05_sequences`: `kadane` and `kadaneCircular` are named `maximumSubarray` and `circularMaximumSubarray`, as in C++ (`04-python.md` naming).
- **P048** row 35 `triangleAggregate` is named `triangleProductSum` (decided 2026-10-29).
- **P018** `06-Miscellaneous/03-fastio.hpp` moves to `01-Core/19-fastio.hpp`, a Core full header with no forwarding header in 06, and absorbs P066's planned row 17: `MmapInput` is `FastInput<0>` (a mapped regular file under Linux, else fread), `SyscallOutput` is `FastOutput` (decided 2026-11-01).
- **P061, P191** Mathematics row 23 owns the matrix-tree counts (`countSpanningTrees`, `countArborescences`, `countRootedForests`, `eulerianCircuitCountBest`); Graphs row 53 drops `countSpanningTrees`, `countArborescences`, `countSpanningForests` and `countEulerianCircuits` (decided 2026-11-08).
- **P028** row 14 `RangeChminChmaxAddAssignRangeSumMinMax` stays a BeatsMonoid preset with no builder operations; `SegTreeBeats` keeps chmin, chmax, add and set (decided 2026-11-08).
- **P122** row 54 `LinearRMQ` takes bitmask microblocks instead of Cartesian (Farach-Colton–Bender) ones: the same bound and results with less code (decided 2026-11-08).
- **P011, P276** Graphs row 07 `getKthAncestor` (clamped negative `k`) merges into `kthAncestor`, which asserts `k >= 0` (user decision 2026-11-10).
- **P276** `08-Python` mirrors C++ (user decision 2026-11-10, `04-python.md` Style): `_56` drops its public `BitVector` (C++ row 32 keeps it internal; `_45` `SuccinctBitVector` is the public type); `_25` drops the free `numberOfSubstrings` beside `SuffixAutomaton::distinctSubstrings`; `_38` `crtGeneral` folds into `crt`, which takes noncoprime moduli; `_15` `BoundedFlow` becomes the free `feasibleFlow` and `maxFlowWithLowerBounds` of Graphs row 30; `_18` `ParityDSU` drops `findSet` and `isSameSet`, which C++ lacks (`parity(u, v) >= 0` is the same test); `_43` `digitDp` and `digitDpRange` fold into `DigitDP::count` (closed interval).
- **P067** `02-Data Structures/23-range_query_offline.hpp` (pinned): `Mo::getHilOrd` leaves row 23 (decided 2026-10-11); Mo orders queries with Miscellaneous row 16 `hilbertIndex2D` (P066, a prerequisite).

### Keeps

Items a reader could mistake for duplicates.

| Package | Item | Reason |
|---|---|---|
| all | Every type alias and the macros `fi`, `se`, `pb` (`FPS`, `ChthollyTree`, `FenwickSet`, `Barrett`, `Montgomery`, `ModFac`, ...) | Type aliases are kept |
| P002 | `Debug::slice` inclusive `[l, r]` | Only form |
| P002 | Bitset const `operator[]`, `flip(p)`, `debugString` | `std::bitset` spellings; Debug's extension point |
| P004 | Barrett `reduce` | Named special case (the reducer's defining operation) |
| P005 | `ModInt::sqrt` returning -1 | `tryX`/`X` pair with `trySqrt` |
| P005 | `ModInt` explicit integral conversion | Checked `T(x)` conversion |
| P006 | `maxZeroSubmatrix` | Larger domain: any nonzero cell blocks; `largestBinaryRectangle` asserts 0/1 |
| P006 | `SqrtDecomp::apply` (formerly `opeUpdate`), `SparseTable::operator<<` | Only form |
| P006 | `SqrtDecomp::rebuild` | Decided 2026-10-13: larger domain; resets in place with the stored monoid `m`, while assigning a new object needs an assignable `M`, which a `Monoid` over a capturing lambda is not |
| P006, P027 | `SegmentTree` and `LazySegmentTree` `fold`, `foldRight` beside `act(x, prod(l, r))` | Different algorithm: the product is never formed, so `act` replaces `op` (`O(K^2)` instead of `O(K^3)` per node for `K`-vectors over matrices); decided 2026-10-18 |
| P007 | 2D `angleApprox` | Named special case (unsigned angle; one name for 2D and 3D) |
| P010 | `findNegativeCycle`, `bfsShortestPaths` | Named special case (all-vertex seeding); BFS form of `ShortestPathResult` |
| P011 | `lca_detail::Forest` inclusive `t_in`/`t_out` | Only form |
| P011 | `functionalOrbit`, `condensationDag` | Different domain (walks that stop); replacement differs in weights, order and memory |
| P011, P043 | `kosarajuScc`, `kuhn` | Different algorithms from `tarjanScc`, `hopcroftKarp` |
| P012 | `floorDiv` beside `divModFloor` (`01-mod_arithmetic.hpp`) | User decision 2026-11-12: the quotient alone is the common call; `divModFloor` adds the floored remainder for either sign of `b` |
| P012 | `ternSearch`, `ternSearchRealBracket` | Different algorithms from `fibSearch`, `goldenSearchRealBracket` |
| P012 | `binSearch(ok, ng, f)` | Decided 2026-10-11: one orientation-free call for the common ok/ng idiom; the replacement needs `lastTrue` or `firstTrue` by orientation plus a result mapping |
| P012 | `LinearSieve::phiTable`, `mobiusTable`, `divisorCountTable`, `divisorSumTable` | Reuse the sieve's `spf` table |
| P012 | `PrimeCombinatorics` fields `fac`, `inv_fac` | Data, direct indexing |
| P012 | `SegmentedSieve::factorize()`, `tables()`, constructor without a base | Named table; batch form; one-call form beside the base-reuse form |
| P012 | `permuWR`; the `ballot` and `ballotExact` `strict` flag | Named count (0 for negative b); variant inside one operation |
| P012, P052 | `solveModEq` `{-1, -1}`, `garner` and `crtMod` returning -1 | Only form |
| P013 | global `rng`; `CustomHash` convertible and enum overload | Only shared instance; only hash for those types |
| P013 | `BitOps::first`, `last`, `parity` | Named special cases (-1 for zero, index, bool) |
| P229 | `Timer::elapsed`, `elapsedUs`, `elapsedMs`, `elapsedSec` | Different units |
| P016 | `prefixToZ`, `centerEnd`, `prefixPeriods`, `zPeriods`, `StringHash` field arithmetic | Named conversion; listed operation; mirrored convention; self-contained hashing arithmetic |
| P017 | `AhoCorasick::countPatterns` | Named per-pattern count |
| P018, P019 | InfInt infinity flag, `INF` token, saturating conversions | Only signed-infinity arithmetic |
| P019 | `InfIntMini` `explicit operator bool` beside `isNil` | As P018 (decided 2026-10-17) |
| P019 | `Rational` `toDouble` beside `toLongDouble`; `read`, `toString` beside `>>`, `<<` | Each conversion rounds the exact value once to its own precision; base 2–36, status and decimal input (decided 2026-10-17) |
| P023 | `steadyState` | Only form; fixed-iteration power method, kept beside the planned converged solver |
| P030 | `PersistentSegmentTree` over a plain monoid beside row 13 `DynamicSegmentTree<M, true>` and row 35 `PersistentArray` | Decided by the user 2026-10-27: version ids, vector and fill builds, `copyRange`, functional `split`/`merge`, and the binary tree's time and memory tradeoff |
| P030 | `PersistentValueTree` beside rows 16 `MergeSortTree` and 17 `WaveletMatrix` | Decided by the user 2026-10-27: a different algorithm whose prefix versions can be combined |
| P030 | `PersistentDistinctCount` beside row 32 `staticRangeCountDistinct` | Decided by the user 2026-10-27: online, where row 32 is offline |
| P030 | `PersistentSegmentTree::copyRange(v, w, l, r)` beside `copyRange(v, w, l, r, f)` | One operation with an optional action, spelled as an overload because the default `id()` needs the stored monoid; user decision 2026-10-27 |
| P030 | `PersistentValueTree` `kthMax`, `sumLess`, `countLess`; `reset` on both segment trees | Decided by the user 2026-10-27: `kthMax` as row 17 keeps it, `sumLess` and `countLess` need no least value of `T` (`countRange` does), `reset` reuses the pool's capacity |
| P052 | `factorizeTrial`, `isPrimeTrial`, `inversePierce`, `inverseFermat` | Different algorithms |
| P052 | `inverseBatch`; `primality_detail::splitmix` | Larger domain (every modular type); internal helper |
| P081 | unit-cost `hirschberg(a, b)` | Named special case (unit-cost alignment) |
| P043 | `TwoSat::ans` | Data behind `answer()` |
| P043 | `addImplication`, `addEquivalent` | Named special cases, as `setValue`; `addNand` stays omitted (decided 2026-10-11) |
| P043 | `hallViolator` | Named certificate (Hall violator) |
| P055 | `FastConv::fctGcd`, `fctLcm` | Reuse a caller's `LinearSieve` |
| P055 | `FastConv::fctGcdSimple`, `fctLcmSimple` | Different algorithm (harmonic loops) |
| P083 | `countDistinctSubsequences` (`19-lcs.hpp`) | Different domain from P084's member |
| P057 | `polynomial_detail::inverses`, `factorials` | Internal helpers |
| P084 | `bwt(s, sentinel)`, `dfaSearch(pattern, text, sigma)` | Headline operation; only form taking a `char` pattern |
| P085 | `longestCommonSuffix` (row 22) | Decided 2026-10-16: kept as `BidirectionalSuffixArray::longestCommonSuffix`, which owns the forward and reversed suffix arrays (the backward query beside `SuffixArray::substringLce`) |
| P085 | `SequenceHash::get`, `reverseGet` (row 23) | Decided 2026-10-16: the typed `Digest` (length, base) compares soundly with `StringHash` and `PointUpdateHash` digests and across lengths; the raw `prod` value does neither |
| P085 | `distinctSubstringsPerPrefix` (row 22) | Decided 2026-10-16: larger domain (integer sequences through `SuffixArray<T>`) beside `distinctSubstringsOnline(string_view)` |
| P036 | `polygon_distance_detail::touch`, `nearest` | Internal helpers |
| P029 | `WaveletMatrix::countEqual`, `kth`/`kthMax`, inclusive `maxLeq`/`minGeq` | Named special case; mirrors (ruling 3); decided 2026-10-13: the only predecessor/successor forms |
| P029 | `rectangleCount`, `rectangleSum` (point sets) | Decided 2026-10-13: larger domain (arbitrary signed x with duplicates, weights), non-legacy builder |
| P029 | `WaveletMatrix::rank(r, x)` | Decided 2026-10-13: the succinct prefix operation that the `countEqual` keep specializes, as `Fenwick::prefixSum` beside `sum` |
| P029 | `WaveletMatrixMonoid::prod` beside row 15 `SegmentTree2DSparse::prod` | Decided 2026-10-13: different algorithm (wavelet levels with per-level segment trees); same name for the same rectangle fold |
| P045 | `MinCostFlow::flow` (`slope(...).back()`) | Decided 2026-10-13: the family's primary operation; `slope` is its batch form over all amounts |
| P032 | `LiChaoTree` beside `LiChaoTreeCompressed` | Different domain |
| P042 | `HLD::reorderArray` | Only producer of the segment-tree base array |
| P067 | `Mo::operator<<` | Only form |
| P051 | row 45 pair `edgeConnectivity`, `edgeDisjointPaths`, edge `mengerCut` | Graph connectivity operations |
| P090 | Python maximum-subarray functions | Only maximum-subarray functions in their row (renamed, see Changes) |
| P061, P181 | `productOfGeometricShifts` (row 22) beside `productOfLinearShifts` (row 37) | Row 22's is the product of f(r^k x), not a linear shift; renamed apart from row 37's (decided 2026-10-15) |
| P179, P192 | `gramDeterminant` (row 35) beside `latticeDeterminant` (row 49) | Squared covolume from Gram–Schmidt versus the exact lattice determinant; renamed apart (decided 2026-10-15) |
| P053 | `legendreSymbol` (row 11) beside `kroneckerSymbol` | Decided 2026-10-16: Euler's criterion, a different algorithm |
| P034 | `PersistentArray` (row 35) beside row 18 `PersistentSegmentTree` get/set | Decided 2026-10-17: k-ary path copying, a different time and memory tradeoff from the binary tree |
| P047 | `bridgeTree` (row 27) beside `simplify(contract(g, low.component, k))` | Decided 2026-10-18: a dedicated builder |
| P047 | `dagMinimumChainCover` (row 31) beside `dagMinimumPathCover` on row 40 `transitiveClosure` | Decided 2026-10-18: bitset closure and bitset matching in O(n^2/64) memory, no closure Graph |
| P047 | `connectedAfterRemovingVertex`, `connectedAfterRemovingEdge` (row 27) beside `lowlink` `cut_components[x] <= 1`, `roots.size() + is_bridge[e] == 1` | Decided 2026-10-18: pair queries (u, v after deleting x or e), not the whole-graph fields |
| P054 | `stirling1Row`, `stirling2Row`, `eulerianRow`, `partitionCount` (row 12) beside Core Poly `stirling1Row`, `stirling2Row`, `eulerianRow`, `partitionNumbers` | Decided 2026-10-19: intended Mathematics duplicates (inventory scope note) |
| P054 | `tetrationMod` (row 30) beside `iteratedPowerMod` | Decided 2026-10-19: named special case (top exponent 1) |
| P054 | `dyckPaths`, `pathsBelowDiagonal` (row 67) beside row 05 `catalan`, `ballot` | Decided 2026-10-19: general endpoints |
| P054 | `inclusionExclusion` (row 12) beside row 24 `subsetMobius` | Decided 2026-10-19: pruned search over items, a different algorithm |
| P054 | `BinomialSmallPrimeTable` (row 12) beside `binomialLucas` | Decided 2026-10-19: O(N) p-free factorial table with O(1) queries for n <= N, kept for its constant factor |
| P037 | `segmentUnionLength`, `orthogonalIntersectionCount` (row 13) beside `intervalUnionMeasure`, row 33 `OfflineRectangleSweep` | Decided 2026-10-18: geometry entry points; the union length also takes floating endpoints, the count needs no point and rectangle builder |
| P037 | `latticeUnderLine` (row 19) beside `floorSum` | Decided 2026-10-18: lattice points under a segment with rational endpoints |
| P037 | `latticeInHalfplanePolygon` (row 19) beside `latticeInRationalPolygon(halfplaneIntersection(h).vertices)` | Decided 2026-10-18: larger domain; it counts from the integral line equations, while the composition overflows on vertex denominators up to 8 * 10^18 |
| P037 | `latticeInDisk`, `latticeInDiskHull` (row 19) beside `gaussCircleCount`, row 55 `gaussCircleCountHull` | Decided 2026-10-18: larger domain, a rational centre and squared radius |
| P037 | `convexIntersection`, `cut`, `batchedLocate` (row 15) beside `clipConvex`, `polygonCutApprox`, `contains` | Decided 2026-10-18: better bounds, O(n + m), O(log(n) + out) and O(q log(2 + n / q)) for angle-sorted queries (galloping since round 3) |
| P037 | `maxPointsInDisk` (row 54) beside row 23 `maxDiskDepth` | Decided 2026-10-18: named special case, equal radii |
| P037 | `maxAreaInscribedKgonSmawk`, `maxPerimeterInscribedKgonSmawkApprox` (row 15) beside `maxAreaInscribedKgon`, `maxPerimeterInscribedKgonApprox` | Decided 2026-10-19: different algorithm (SMAWK layers, O(k n + n log n)); both kept even where one is faster |
| P037 | `rectangleUnionBoundaryGuting` (row 13) beside `rectangleUnionBoundary` | Decided 2026-10-19: different algorithm (Güting's divide and conquer, O(n log n + out)) |
| P037 | Mathematics `floorSumWide` (row 18) beside `floorSum` | Decided 2026-10-19: larger domain, a 128-bit modulus |
| P037 | `minimumEnclosingCircleApprox` (row 14) beside `minimumEnclosingCircle` | Decided 2026-10-19: larger domain (floating inputs) |
| P037 | `SortedGrid` (row 54) beside `GridHash` | Decided 2026-10-19 by the user: deterministic variant (radix-sorted cells), a different algorithm |
| P037 | `maxAreaInscribedTriangle`, `maxAreaInscribedQuadrilateral` (row 15) beside `maxAreaInscribedKgon` | Decided 2026-10-19: named special cases with a linear bound, different algorithms |
| P037 | `pointDistance2`, `areaLeftOfLine2` (row 15) beside their `Approx` forms | Exact and approximate variants (03-cpp.md, API contracts) |
| P037 | `rectangleUnionContours` (row 13) beside `rectangleUnionBoundary` | Decided 2026-10-19 by the user: a derived result (closed rings, cycle structure and orientation) that the pieces do not give |
| P035 | `SparseDSU` (row 65) beside `HashMap<K, int>` plus `DSU::makeSet` | Decided 2026-10-23 by the user: larger key domain (hashable keys) |
| P035 | `ParityDSU` (row 29) beside `WeightedDSU` over Z/2 | Decided 2026-10-23 by the user: a different algorithm, a 2n-node DSU |
| P035 | `MergeHistoryForest` (row 65) beside `KruskalReconstruction` weighted by union index and `PartiallyPersistentDSU::uniteTime` | Decided 2026-10-23 by the user: a separate builder from a union sequence, with no graph or sort |
| P058 | `nullspace` (`16-linear_algebra.hpp`) beside the basis of `solveLinear(A, 0)` | Decided 2026-10-28 by the user: a standalone kernel basis without a right-hand side; `gaussJordan` left row 16 for `rref` the same day |
| P048 | `countTriangles` (row 35) beside an `enumerateTriangles` counting callback, `Σ countC3C4PerVertex(g).first / 3` and `Σ countTrianglesPerEdge(g) / 3` | Decided 2026-10-29 by the user: a different algorithm on dense graphs, popcount of oriented-adjacency bitsets in O(m n / 64), and one pass with no per-vertex or per-edge arrays |
| P048 | `triangleProductSum` (row 35, formerly `triangleAggregate`) beside `enumerateTriangles` summing `x[a] * x[b] * x[c]` | Decided 2026-10-29 by the user: the named vertex-weighted triangle sum (Library Checker enumerate_triangles) |
| P048 | `countC4` (row 35) beside `countC4PerEdge` summed over the edges of `simplify(g)` and `Σ countC3C4PerVertex(g).second`, each divided by 4 | Decided 2026-10-29 by the user: one counting pass with no per-edge or per-vertex accumulation, kept for its constant factor |
| P048 | `countK4` and `enumerateK4` (row 35) beside each other, `countC4` beside an `enumerateC4` counting callback | Decided 2026-10-29 by the user: the counts take O(m · d^2 / w) and O(m · d) without the output term; the enumerations report each subgraph |
| P048 | `enumerateTriangleEdges` beside `enumerateTriangles` (row 35) | Decided 2026-10-29 by the user: multigraph semantics with edge IDs, every combination of parallel copies, against one report per vertex triangle |
| P048 | `localClustering`, `squareClustering`, `transitivity` and `generalizedDegree` (row 35) beside `countC3C4PerVertex`, `countTriangles` and `countTrianglesPerEdge` with degrees | Decided 2026-10-29 by the user: NetworkX `cluster.py` measures with fixed denominators and zero conventions (`squareClustering` needs P3 and the 4-cycles through v; `generalizedDegree` counts per simple pair) |
| P048 | `averageClustering` (row 35) beside the mean of `localClustering` or `directedClustering` | Decided 2026-10-29 by the user: NetworkX `average_clustering` with `count_zeros`, dispatching on `g.directed` |
| P072 | `kSmallestSubsetSums` (`54-kbest_enumeration.hpp`) beside `kSmallestSortedListSums` over lists `{min(0, a_i), max(0, a_i)}` | Decided 2026-10-23 by the user: named special case (subset witnesses, no list construction) |
| P059 | `isqrt` beside `isqrt128` (row 21) | Decided 2026-11-03 (`/merge-pr 3`): larger domain, a 128-bit argument; `isqrt` keeps the one-`sqrt` 64-bit path |
| P122 | `PlusMinusOneRMQ` (row 54) beside the `EulerLCA` kernel (Graphs row 07) | Decided 2026-11-08 by the user (P254 omission sweep): row 54 keeps it public and `EulerLCA` calls it instead of a copy of its own |
| P121 | `SortableArray` (duplicate keys, no monoid) beside `SortableSegmentTree` (distinct keys) | Decided 2026-11-08 by the user: larger domain |
| P183 | `coefficientExtraction` beside `bostanMori` | Decided 2026-11-08 by the user (P254 omission sweep): a wrapper in another row stays and calls its owner |
| P183 | `labelledSequence` beside `unlabelledSequence` | Decided 2026-11-08 by the user: labelled and unlabelled constructions are separate contexts (`03-cpp.md`, One API per operation); the labelled form calls the unlabelled one |
| P185 | `GF2k::carrylessMultiply` beside row 65 `carrylessMultiply` | Decided 2026-11-08 by the user (P254 omission sweep): a wrapper on another type, calling row 65 |
| P186 | `determinantPolynomialMatrix` (row 44) beside row 51 `PolyMatrix::det` | Decided 2026-11-08 by the user (P254 omission sweep): a wrapper in another row, calling row 51 |

### Rulings

Decided by the user on 2026-10-11 for the sweep's policy questions.

- A bool test or defaulted read of another operation's result (`contains`, `search`, `isSubstring`, `reachable`, `none`, `get(k, def)`) is a named special case: keep it.
- The complement of another operation's result (`maximumIndependentSet` beside `konigVertexCover`) is a separate operation: keep it.
- An orientation or sign mirror of an operation (`ceilSum`, `upperEnvelope`, concave max-plus convolution) is a named special case: keep it.
- A second implementation or cheaper projection kept for its constant factor (`convexHull`, `truncatedMul`, `CommutativeDualSegmentTree`, the Tarjan inside `TwoSat`, PyPy-inlined variants) stays.
- A one-vertex overload of a multi-source function (`dijkstra(g, s)` beside `dijkstra(g, sources)`) is an argument shape: remove it; callers write `f(g, {s})`.
- User decisions 2026-11-10 (P275 `/finish`): a type's member and a free function for the same operation are two contexts, so the one that calls the other is a wrapper and stays (`Plane3::distanceApprox`, the `DirichletSeries` prefix functions, `BitsetDP::shiftOr` under a name of its own); every flag that chooses between two operations (P010's `longest` and `lexicographic`, the MST `maximum` flags, `eulerianTrail`'s `lexicographic`) goes to one P010 ask settled under one ruling.

## Kept by the sweep

Keep decisions of `/duplicate-sweep` that apply the user's rulings and clarified rules; a later sweep revalidates them.

| Package | Item | Reason |
|---|---|---|
| P042 | `widestPath` beside `minimaxPath` (`13-shortest_path_advanced.hpp`) | Mirror (ruling 3) |
| P042 | `radius`, `diameter`, `center`, `periphery` (`12-all_pairs_shortest_path.hpp`) | Fields of the `Eccentricities` result: derived formulas (clarified rule) |
| P042 | `floydWarshall` beside `johnson` (`12-all_pairs_shortest_path.hpp`) | Different algorithm: O(n^3) dense matrix against SCC Bellman–Ford plus n Dijkstras, same results |
| P042 | `minimaxFloydWarshall` (row 12) beside `minimaxPath`, `widestPath` (row 13) | Different algorithm and result: bitset closure for every pair against one bottleneck Dijkstra with parents per source set |
| P042 | `spfa`, `desopoPape`, `negativeSsspScaling` (row 13) beside `bellmanFord` (row 05) | Different algorithms: FIFO and deque label correcting, Goldberg bit scaling |
| P042 | `dialDijkstra`, `radixHeapDijkstra`, `aStar`, `bidirectionalDijkstra` (row 13) beside `dijkstra` (row 05) | Different algorithms: bucket and radix queues; s–t searches with a heuristic or from both ends |
| P042 | `LevelAncestor::query` (row 85) beside `LCA::kthAncestor` (row 07) and `HLD::kthAncestor` (row 15) | Different algorithm (clarified rule): ladders with leaf jump pointers, O(1) query |
| P042 | `levelAncestorOffline` (row 85) beside `LevelAncestor::query` | Batch form (clarified rule), O(n + q) without jump tables |
| P032 | `MonotoneCHT::query` beside `queryMonotone` | Different algorithm: binary search in O(log(n)) against a pointer walk, amortized O(1) for monotone x; same results for `lng` (tie rule, user decision 2026-10-28) |
| P032 | `MonotoneCHT`, `LineContainer` (row 22) beside `LiChaoTree` (row 21) | Different algorithms and domains: an amortized O(1) deque hull for extreme slopes, a breakpoint multiset for any slope and any x, a segment tree over an x range that also takes segments and single-crossing functions |
| P032 | `PerLiChaoTree` beside `LiChaoTree` | Different domain: immutable versions by path copying, O(log(C)) nodes per line against O(1) |
| P032 | `PersistentRangeMex` beside `rangeMex` (row 32) | Decided by the user 2026-11-02: online persistent versions in O(n * log(n)) memory against an offline sweep in O(n) |
| P032 | `RangeMajority` beside `RangeThresholdFrequency::frequent` with `t = (r - l) / 2 + 1` (row 32) | Decided by the user 2026-11-02: a Boyer–Moore segment tree with verification in O(n) memory against wavelet rank sampling |
| P032 | `XorBasis::rank(x)` beside `size()`; `XorBasis::intersection` beside Core `BitMatrix` `intersectSpace` (row 62) | Decided by the user 2026-11-02: `rank(x)` is the order index of `x` among the span values (the inverse of `kth`), not the dimension; `intersection` is a 64-bit word basis in O(64^2) against packed matrices. `countRepresentable` (`1ull << size()`), `reducedEchelon` (the stored basis is kept reduced) and `min` (`reduce(x)`; the smallest nonzero span value is `kth(1)`) left the row |
| P032 | `XorBasis::contains` (`62-xorbasis.hpp`) | Named special case of `reduce` (ruling 1) |
| P032 | `XorBasis::merge` beside `insert` over `o.b` (row 62) | Batch form (clarified rule) |
| P032 | `PrefixXorBasis::rangeMax`, `rangeContains` beside `XorBasisCompanion::rangeXorBasisQuery` followed by `max` or `contains` (row 62) | Different algorithm and domain: online queries in O(B) over O(n * B) memory against an offline sweep in O(n + q) memory |
| P032 | `XorBasis<false>` beside `XorBasis<true>` (row 62) | One template: the flag drops the insert masks and `represent`, which Graphs row 28 does not use (they cost 18–41% there, [62-xorbasis.md](<../../02-Data Structures/Docs/62-xorbasis.md#benchmarks>)) |
| P002 | `rangeMask(bits, l, r)` (`18-bitset.hpp`) | Builder: construction plus `setRange` (clarified rule) |
| P002 | `findNext(p)` (`18-bitset.hpp`) | Accepts `p == n`, which `findFrom` rejects (clarified rule) |
| P002 | `none()` (`18-bitset.hpp`) | Named special case (ruling 1) |
| P002 | `combine<Op>` (`18-bitset.hpp`) | Helper of `combineShift`; dropped from the row 2026-10-12 |
| P002 | `setRange(l, r, false)` (`18-bitset.hpp`) | The flag is the value written, as in `set(p, v)` |
| P004 | `redc(x, lazy)` (`04-montgomery.hpp`) | Unchecked kernel public only because there is no `private` |
| P006 | Fenwick `lowerBound`, `upperBound`, `descend` (`02-fenwick.hpp`) | Named special case; larger domain (`x <= 0`); 11% faster (`Docs/02-fenwick.md`) |
| P006 | `SegmentTree::apply` (`03-segmenttree.hpp`) | As the recorded keep `SqrtDecomp::apply` |
| P006 | `DifferenceArray(2D)::clear` (`05-prefix_sum.hpp`) | Named special case, in place |
| P006 | `PrefixSum` beside `PrefixProduct<Sum<T>>` (`05-prefix_sum.hpp`) | Named special case: the same results for an additive group, taken as a plain `T` with `+` and `-` instead of a group struct, with `prefixSum` and the braced-list constructor; `PrefixProduct` serves noncommutative groups (DS02 review, 2026-11-12) |
| P006 | Braced-list constructors (`02-fenwick.hpp`, `03-segmenttree.hpp`, `05-prefix_sum.hpp`, `06-sqrt_decomposition.hpp`) | Without them `Fenwick<lng> f{7}` picks the size constructor (`Docs/00-notes.md`, Common foundation contracts) |
| P006 | `CoordinateCompression::decode` (`07-ordered_set.hpp`) | Named inverse of `encode`; replaces P013's `value` |
| P006 | `previousGreater`, `nextGreater`, `slidingMaximum` (`08-monotone_stack.hpp`) | Named special cases |
| P007 | `signedAreaApprox` (`03-polygon.hpp`) | Another scale: half the doubled area (clarified rule) |
| P007 | `polygonContains`, `PolygonLocation` (`03-polygon.hpp`) | Derived classification of the winding number (clarified rule) |
| P007 | `convexHull` (`04-convexhull.hpp`) | Constant-factor variant (ruling 4) |
| P007 | `lowerHull`, `upperHull` beside `lowerHullIndices`, `upperHullIndices` (`04-convexhull.hpp`) | Constant-factor variant (ruling 4): points without the index sort and lookups, as `convexHull` beside `convexHullIndices` (P007 /finish review 2026-11-12) |
| P007 | `segmentDistanceApprox` beside `linearDistanceApprox` on two segments (`02-line_segment.hpp`) | Constant-factor variant (ruling 4): long double from the exact topology, no `Rational` normalization (P007 review 2026-11-11); measured 16.2 to 18.5 times faster on random segment pairs (`04-convexhull.md`, Benchmarks, 2026-11-12) |
| P007 | `projectLineApprox`, `reflectLineApprox`, `reflectDirectionApprox` (`02-line_segment.hpp`) | Different computation (one division, exact 0.5 where the factory gives 0.4999…) and domain (any `Point2<T>`) |
| P008 | `Affine2Approx::orientation()` (`08-coordinate_transform.hpp`) | Derived formula: sign of `determinant()` (clarified rule) |
| P008 | `CircleApprox::onBoundary`, `contains(dpoint)` (`06-circle.hpp`) | Named special case (ruling 1) |
| P008 | `rectangleApprox(bool perimeter)` (`07-rotatingcalipers.hpp`) | Shared helper; dropped from the row 2026-10-12 (flag between two operations) |
| P008 | `circleCentersApprox`, `pointTangentsApprox` (`06-circle.hpp`) | Named special cases |
| P008 | `farthestPair` (`07-rotatingcalipers.hpp`) | Larger domain (any cloud, index ties) |
| P009 | `Triangle2::areaApprox` (`09-triangle.hpp`) | Derived formula: half the absolute doubled area (clarified rule) |
| P009 | `TriangleApprox::area()` (`09-triangle.hpp`) | Derived formula: half the absolute doubled area (clarified rule) |
| P010 | `Graph::read` flags, `CsrGraph::reverse`, `dfsForest`, `KruskalReconstruction::bottleneck` (`01-graph.hpp`, `02-traversal.hpp`, `06-mst.hpp`; the `maximum` flags become named mirrors, Changes) | Only forms, named special cases |
| P011 | `StrongOrientationResult`, `strongOrientation` (`09-bridges_articulation.hpp`) | Derived `ok` flag and Robbins obstruction witness (clarified rule) |
| P011 | `FunctionalGraph(const G &g)` (`11-functionalgraph.hpp`) | Builds from a `Graph`, not a legacy format (clarified rule) |
| P011 | `FunctionalGraph::reachable` (`11-functionalgraph.hpp`) | Named special case (ruling 1) |
| P011 | `FunctionalGraphFold::segment` (`11-functionalgraph.hpp`) | Helper of `fold`, `cycleAggregate`, `maxStep`; dropped from the row 2026-10-12 |
| P012 | `starsBars`, `starsBarsExact` (`05-combinatorics.hpp`) | The positive variant is a derived formula with a guard (clarified rule) |
| P012 | `fibonacciExact`, `LinearSieve::divisors` (`05-combinatorics.hpp`; `04-sieve_algorithms.hpp`) | Larger domain (n = 93); reuses `spf`, unsorted |
| P013 | `compressEndpoints` (`04-compression.hpp`) | Builder over interval endpoints (clarified rule) |
| P013 | `randBelow(n)` (`01-random.hpp`) | Helper of `randUlng`, `randLng`, `shuffle`; dropped from the row 2026-10-12 |
| P013 | `permutationRank`, `permutationUnrank` (`07-permutation.hpp`) | Named special case |
| P229 | `answer(a…)` (`50-interactive.hpp`) | Named special case, unbudgeted |
| P229 | `Dice::top()` … `bottom()` (`51-grid_utilities.hpp`) | Accessors of a public data member (as `size()`, `val()`) |
| P016 | `kmpNext(p)` (`01-prefixfunction.hpp`) | Builds the prefix function itself (clarified rule) |
| P016 | `common(…, suffix)` (`03-stringhash.hpp`) | Helper; dropped from the row 2026-10-12 (flag form) |
| P016 | `Trie::search`, `Trie::empty` (`04-trie.hpp`) | Named special case (ruling 1) |
| P017 | `forbidden(u)` (`06-aho.hpp`) | Named special case (ruling 1) |
| P025 | `mulHigh` (`16-poly.hpp`) | Derived: product then shift (clarified rule) |
| P025 | `mulMod(a, b, m)` (`16-poly.hpp`) | Composition: product then remainder (clarified rule) |
| P025 | `reverse(n)` parameter (`16-poly.hpp`) | The length-n reversal, a derived formula (clarified rule) |
| P025 | `integ(a, c)` parameter (`16-poly.hpp`) | A general constant; `integ(a)` is its special case (clarified rule) |
| P025 | `truncatedMul` (`16-poly.hpp`) | Cheaper projection (ruling 4) |
| P025 | `square` (`16-poly.hpp`) | Named special case |
| P025 | `relaxedMul`, `semiRelaxedMul` (`16-poly.hpp`) | Batch forms of an online algorithm |
| P025 | `PolyCompanion::convolutionLng`, `convolution2p64`, `convolutionArbitraryMod` (`16-poly.hpp`) | Only forms of their algorithms; different domain (raw `ulng`, runtime modulus) |
| P025 | `poly_detail::Mont`, `Mont8`; `composeBrentKung` (`16-poly.hpp`) | Different REDC variant without per-call asserts; larger domain (rings) |
| P026 | `integ(a, c)` parameter (`17-polymini.hpp`) | A general constant; `integ(a)` is its special case (clarified rule) |
| P027 | `DualFenwick` (`11-fenwick_tree_advanced.hpp`) | Different purpose: range add and point get (clarified rule) |
| P027 | `IntervalMap::fold` (`24-interval_set.hpp`) | Derived accumulation over `enumerate` (clarified rule) |
| P027 | `IntervalSet::contains` (`24-interval_set.hpp`) | Named special case (ruling 1) |
| P027 | `CommutativeDualSegmentTree` (`12-lazysegmenttree.hpp`) | Constant-factor variant (ruling 4) |
| P027 | `IntervalSet::covers` (`24-interval_set.hpp`); `00-monoids.hpp` presets | Only form; named special cases |
| P227 | `HashTable::contains`, `HashMap::get(k, def)` (`61-hashmap.hpp`) | Named special case (ruling 1) |
| P227 | `SortedList::contains` (`64-sortedlist.hpp`) | Needs the comparator test; `count` is O(√n) |
| P052 | `CrtIncremental` (`10-crt.hpp`) | Online form of `crt` (clarified rule) |
| P052 | `inverseXgcd` (`09-modinverse.hpp`) | Named special case |
| P081 | `sais(const vector<T> &)` (`12-sais.hpp`) | Compresses the alphabet itself (clarified rule) |
| P081 | `compressAlphabet` (`12-sais.hpp`) | Different algorithm: one index sort and a scan (clarified rule) |
| P081 | `SuffixAutomaton::isSubstring` (`10-suffixautomaton.hpp`) | Named special case (ruling 1) |
| P043 | Tarjan inside `satisfiable` (`18-twosat.hpp`) | Constant-factor variant (ruling 4) |
| P043 | `maximumIndependentSet()` (`21-matching_bipartite.hpp`) | Separate operation (ruling 2) |
| P055 | `crossCorrelation` (`13-convolution.hpp`) | Derived: convolution with `b` reversed (clarified rule) |
| P055 | `convolution2d` (`13-convolution.hpp`) | Rank-2 named special case |
| P083 | `bitsetLcs` (`19-lcs.hpp`) | Bit-parallel on types where `lcs` falls back to the scalar DP (clarified rule) |
| P083 | `rotationEquivalent`, `isLyndon` (`15-minrotation.hpp`; `14-lyndon.hpp`) | Named special case (ruling 1) |
| P083 | `countPalindromicSubsequences(s, distinct)` (`19-lcs.hpp`) | Only form of both DPs |
| P028 | `SWAGStack` (`30-aggregation_queue.hpp`) | Constant-factor variant: one array, no extra `op(e(), ·)` per fold (ruling 4) |
| P028 | 2D point `apply` (`15-segment_tree_2d.hpp`) | As `SegmentTree::apply` and the recorded `SqrtDecomp::apply` |
| P028 | `LazySegmentTreeBeats`; merge-sort-tree `countLess`, `countRange`, `countEqual`, `maxLeq`, `minGeq`, `set` (`14-segtreebeats.hpp`; `16-mergesorttree.hpp`) | Larger domain (`fail`); named special cases, cheaper cascades |
| P057 | `polyMul` (`14-polynomial_algorithms.hpp`) | Composition: convolution then trim (clarified rule) |
| P057 | `nthTermLinearRecurrence` (`15-linear_recurrence.hpp`) | Builds the generating function itself (clarified rule) |
| P057 | `consecutiveTerms` (`15-linear_recurrence.hpp`) | Builds the generating function itself (clarified rule) |
| P057 | `polyDivLinear`, `polyGcd`, `polyInverseMod`, `fiduccia`, `kitamasa`, `berlekampMassey` (`14-polynomial_algorithms.hpp`; `15-linear_recurrence.hpp`) | Named special cases; cheaper (`polyGcd`); only form; `tryX` pair |
| P084 | `shiftOr`, `shiftAnd` (`17-bitap.hpp`) | Single-word constant-factor variants (ruling 4) |
| P084 | Pattern overloads of `shiftAnd`, `shiftOr`, `shiftOrMultiword`, `bitapHamming`, `bitapEdit` (`17-bitap.hpp`) | Build the masks themselves (clarified rule) |
| P084 | `isSubstring` (`16-string_matching.hpp`) | Named special case (ruling 1) |
| P084 | `countOccurrences`, `BitsetText::count`, `lfMapping` (`16-string_matching.hpp`; `17-bitap.hpp`; `29-bwt.hpp`) | Only disjoint form; cheaper counts; standard map |
| P085 | `lexMinSuffixPerPrefix`, `occurrenceCount` (`22-lexicographic_queries.hpp`) | Batch form with derived starts over `IncrementalLyndon`; derived count, `n + 1` for the empty range (clarified rule) |
| P085 | `occurrenceRange`, `SubstringOrder` kth queries (`22-lexicographic_queries.hpp`) | Different domain from `SuffixArray::patternRange` (a substring by position, RMQ search) and different algorithm and domain from the automaton kth queries (suffix array, integer sequences) |
| P085 | `PointUpdateHash::common`, `SequenceHash::common` (`23-dynamicstringhash.hpp`) | Helper of `lcp` and `lcs`, as `StringHash::common` |
| P036 | `minkowskiDifference` (`11-minkowskisum.hpp`) | Derived: negation then sum (clarified rule) |
| P036 | `minkowskiSumHull` (`11-minkowskisum.hpp`) | Larger domain: any point sets (clarified rule) |
| P036 | `halfplane_detail::vertex`; `polygonKernel` (`10-halfplaneintersection.hpp`) | Detail helper; different algorithm, user decision 2026-10-10 (`Docs/00-notes.md`, P036 completeness decisions) |
| P036 | `polygonKernelLinear` (`10-halfplaneintersection.hpp`) beside `polygonKernel` | Different algorithm (Lee–Preparata, `O(n)`), user decision 2026-10-10 (`Docs/00-notes.md`, P036 completeness decisions) |
| P033 | `lcaRmqBridge` (`27-cartesiantree.hpp`) | An LCA–RMQ reduction, not a rename (clarified rule) |
| P018 | `explicit operator bool` beside `isNil` (`07-infint.hpp`) | Decided 2026-10-16: both stay, `isNil` for Poly's `BigInt` concept and the modint-style truthiness |
| P018 | `infint_detail` NTT, FFT and Montgomery kernels beside Poly's and `04-montgomery.hpp` | Decided 2026-10-16: InfInt keeps independent kernels and depends on `01-template.hpp` only |
| P018 | `divModSlow` beside `divMod`; `isPerfectSquare` beside `isqrtRem`; `isProbablePrime` beside Mathematics `bpsw` | Knuth D as a separate algorithm; residue filters before the root; BPSW on arbitrary size (decided 2026-10-16) |
| P018 | `schonhageStrassen` with `Fermat` beside `transformMul` (`07-infint.hpp`) | Tested, undispatched backend kept for completeness; user decision 2026-10-25 (`01-Core/Docs/07-infint.md`, Limits) |
| P018 | `infint_detail::divKnuth` beside `divBasecase` (`07-infint.hpp`) | Decided 2026-11-01 by the user: the in-place Knuth D kernel and its copying entry point, both internal; `divRem` runs `divKnuth` on its own normalised copy, one dividend copy fewer |
| P018 | `fastio_detail` input kernels `dec8`, `decWord`, `hexWord`, `hex8`, `dec19`, `hex16` and output kernels `dec8Chars`, `dec16Chars`, `dec19Chars`, `hex16Chars`, `writeDec` (64-bit: InfInt's `writeDec24`; 128-bit: `writeDec2`), `writeHex`, `DIGITS2`, `POW10` and `decLen` beside InfInt's statics (`19-fastio.hpp`, `07-infint.hpp`) | Decided 2026-11-01 by the user: `dec19` and `hex16` read a 32-byte window before the token's end, which needs the reader's padding, where InfInt's take an exact view and never read past it; the other kernels repeat so each header copies alone, and a change to one is measured and mirrored in the other. Names follow one scheme in both headers (user decision 2026-11-01): parsers `<base><digits>` (`dec8`, `hex8`, `hex16`, `hex32`, `dec19`), digit writers `<base><digits>Chars`, out-of-line tails `<operation>Rest` (`readRest`, `toCharsRest`, `divModRest`; fast I/O names the token kind: `tokenRest`, `intRest`, `textRest`) |
| P018 | `FastInput` `readInt` beside `readToken` + `parseInt`, `readToken(string &)` beside `readToken(string_view &)`, `read<B>`, `readText<B>` beside `readToken` + the type's `read`; `FastOutput` `print` beside `writeValue`, `writeValue` beside the typed writers, `writeText<B>` beside `charsBound` + `toChars`, the `end` byte of `write`, `writeInt`, `writeDouble` | Decided 2026-11-01 by the user: the one-call form; an owned copy where the view borrows the buffer; dispatch over ranges, pairs and tuples (`writeValue` per value, `bool` as 0/1); short text tokens taken by assignment and long ones without a copy; one room check for a value and its separator, with one retry after a drain |
| P018 | InfInt `writeDec` beside `writeDec24` (`07-infint.hpp`); `fastio_detail::hex16` beside `hex32` (`19-fastio.hpp`) | Decided 2026-11-01 by the user: `writeDec` stays inside the caller's bounds where `writeDec24` needs 24 bytes of room; `hex16` is the one-word path of 64-bit and narrower types, `hex32` the two-word path of 128-bit ones |
| P034 | `PartiallyPersistentDSU::uniteTime` (`10-persistentdsu.hpp`) beside row 65 `MergeHistoryForest::earliestConnectTime` | Different algorithm: a walk over attach times in the union-by-size forest, no merge forest or LCA (clarified rule) |
| P034 | `OfflineDeletion::insert`, `erase` beside `addInterval` (`63-offline_deletion.hpp`) | Larger domain: unpaired per-key events with multiplicities, paired through an ordered map and closed at `n` (clarified rule) |
| P047 | `blockOf` (`27-blockcuttree.hpp`) | Maps a vertex to its unique block, -1 at articulation points |
| P047 | `maximumBranching` (`25-directedmst.hpp`) beside `DirectedMst::arborescence` | Different problem: no root, any number of roots, only positive arcs; built on the contraction engine, not a sign mirror |
| P047 | `OfflineDynamicConnectivity::insertEdge`, `eraseEdge` beside `addEdge` (`26-dynamic_connectivity.hpp`) | Larger domain: unpaired per-pair events with multiplicities, as P034 `OfflineDeletion::insert`, `erase` |
| P047 | `OfflineDynamicConnectivity` (`26-dynamic_connectivity.hpp`) | Named special case of Data Structures row 63 `OfflineDeletion` with `RollbackDSU` (Data Structures `Docs/00-notes.md`, Ownership boundaries, reworded) |
| P060 | `ceilSum` (`18-floorsum.hpp`) | Named special case (ruling 3) |
| P060 | `enumerateQuotients` (`18-floorsum.hpp`) | Stored form beside the callback `forEachQuotientBlock`, as the kept `factorize()` (clarified rule) |
| P060 | `maxOfModOfLinear` (`18-floorsum.hpp`) beside `minOfModOfLinear` | Mirror (ruling 3) |
| P060 | `josephus` (`19-josephus.hpp`), `josephusKth` at `t = n - 1`; `josephusK2` | Named special case (the survivor); `josephusK2` is a different algorithm (O(1) closed form). The plain O(n) loop and `josephusLarge` were merged into `josephus` by user decision 2026-10-18 |
| P060 | `continuedFractionSqrt` (`20-continued_fraction.hpp`), `continuedFractionQuadratic(0, D, 1)` | Named special case |
| P054 | `gridPaths` (`67-lattice_paths.hpp`) beside `combiNR(dx + dy, dx)` | Larger domain: returns 0 for unreachable targets, which `combiNR` rejects (clarified rule) |
| P054 | `catalanConvolution`, `catalanTriangle` (`67-lattice_paths.hpp`) beside row 05 `ballot` | Named special case (the k-th power of the Catalan series equals the weak ballot to (n + k - 1, n)) and batch form in any ring (additions only) |
| P054 | `combinatorics_advanced_detail::inverse` (`12-combinatorics_advanced.hpp`) beside row 09 `inverseXgcd` | Larger domain: a private 128-bit Euclid for moduli up to 2^64 - 1, where `inverseXgcd` takes `lng` |
| P082 | `isSubstring` (`11-suffixtree.hpp`) | Named special case (ruling 1) |
| P035 | `WeightedDSU` over non-commutative groups (`29-weighteddsu.hpp`; the planned `NoncommutativeWeightedDSU`) | Larger domain: non-commutative groups (clarified rule); one orientation-correct `WeightedDSU<G>` covers both |
| P035 | `LinkedDSU::unionEnumerate` (`65-dsu_extensions.hpp`) | Derived: the newly merged pairs (clarified rule) |
| P035 | `ComponentSumDSU` (`65-dsu_extensions.hpp`) beside row 09 `RollbackDSUAggregate` | Different algorithm: path compression, no undo |
| P035 | `DecrementalSet` (`36-fastset.hpp`) beside `FastSet` | Different algorithm: a DSU over runs of empty words, O(alpha(n)) amortized on the erase-only domain |
| P058 | `tridiagonalSolve` (`16-linear_algebra.hpp`) beside `bandedSolve(·, 1, 1, ·)` | Named special case (clarified rule): three-diagonal input without band packing, O(n) |
| P058 | `inverse` (`16-linear_algebra.hpp`) beside `solveLinearMultiRhs(a, identityMatrix(n), x)` | Named special case (clarified rule): in place, square, singularity status |
| P058 | `matrixRank` (`16-linear_algebra.hpp`) beside `rref(copy).size()` | Cheaper projection kept for its constant factor (ruling 4): forward elimination only |
| P072 | `kLargest` (`54-kbest_enumeration.hpp`), every `kSmallest*` under `std::greater<>` | Mirror of `kSmallest*` (ruling 3) |
| P072 | `treeKnapsack` (`29-knapsack_advanced.hpp`) beside Graphs row 14 `subtreeMergeDp` | Different algorithm: O(n * W) preorder suffix tables against O(n * W^2) subtree merging |
| P072 | `kSmallestPairSums` (`54-kbest_enumeration.hpp`) beside `kSmallestSortedListSums({a, b})` | Named special case (clarified rule): two sorted arrays, `(sum, i, j)` tie order and index pairs, as `kSmallestSubsetSums` (Keeps) |
| P072 | `boundedKnapsackMonotoneQueue`, `smallWeightKnapsack` (`29-knapsack_advanced.hpp`) beside `KnapsackDP::exact` (`13-knapsack.hpp`) | Different algorithms: monotone-queue O((n + 1) * (W + 1)) and SMAWK over D distinct weights O(n log n + (D + 1) * (W + 1)) against binary splitting O((W + 1) * (n + 1 + sum(log(c_i + 1)))) |
| P072 | `knapsackCountsFps` (`29-knapsack_advanced.hpp`) beside `KnapsackCounts<ModInt>` (`13-knapsack.hpp`) | Different algorithm: power-series log/exp O((t + 1) * log(t + 2) + n) against the O((n + 1) * (W + 1)) counting DP, same counts modulo a prime above t |
| P048 | `cycleSpaceDimension`, `isIndependentF2` (row 28) beside `m - n + connectedComponents(g).count` and `BitMatrix` rank | Derived formula; bool form (ruling 1) |
| P048 | `eulerCharacteristic` (row 29) beside `n - m + faces().size()` | Derived formula (clarified rule) |
| P032 | `StaticRangeFrequency::count` (`32-static_range_queries.hpp`) beside `PointSetRangeFrequency(a).count`, `MergeSortTree::countEqual`, `WaveletMatrix::countEqual`, `PersistentValueTree::countEqual` | Constant-factor variant (ruling 4): sorted (value, position) pairs, O(n) words, needs only `<` |
| P059 | `bisectionRoot` (row 17) beside `binSearchRealBracket` | Root finding with a predicate derived from f (clarified rule) |
| P059 | `isqrtRemainder` (row 21) beside `isqrt` | Derived formula (clarified rule) |
| P059 | `ceilKthRoot` beside `ikthRoot` (row 21) | Derived formula: floor plus one unless exact (clarified rule) |
| P059 | `isqrt`, `icbrt` beside `ikthRoot` (row 21) | Named special cases; `icbrt` also takes signed input |
| P059 | `isPerfectSquare` beside `isqrtRemainder` (row 21) | Named special case with a residue prefilter |
| P059 | `kahanSum` beside `neumaierSum` (row 17) | Different recurrence (Neumaier never worse) |
| P059 | `hornerDerivative` (row 17) beside row 14 `polyEval` | Different result: value and derivative in one pass, over any ring |
| P020 | `/=(Matrix)`, `operator/` (`10-matrix.hpp:326,397`) | Composition MATLAB `mrdivide` names (B/A = B·inv(A)); calls `inv` and `*` |
| P020 | row 10: `solveAll` (row 10) | Batch form: one elimination gives a solution and the kernel; calls `solveLin`, `ker` |
| P020 | `size()` (`10-matrix.hpp:287`) | Accessor of a public data member |
| P020 | `rref<opt>` (`10-matrix.hpp:768`) | Only row-echelon form (`rnk`) |
| P020 | row 10: `StaticMatrix<T, N, M>` (row 10) | Own type (compile-time shape, fixed storage); a copy of `Matrix` code needs its reason in `01-Core/Docs/00-notes.md` |
| P020 | row 10: `rowSpace`, `colSpace` (also BitMatrix) (row 10) | Compositions SymPy names (`rowspace`, `columnspace`); call `rref` |
| P022 | row 12: `solveAll` (row 12) | Batch form: one elimination for both (clarified rule) |
| P022 | `size()` (`12-bitmatrix.hpp:84`) | Accessor of a public data member |
| P023 | `polyMulMod(a, b, c)` (`14-sparsematrix.hpp:108`) | Accepts L = 0 and non-field T, which `PolyModulus` rejects (clarified rule) |
| P023 | `size()` (`14-sparsematrix.hpp:45`) | Accessor of a public data member |
| P046 | `perfectMatchingExists` (row 22) | Named special case (ruling 1) |
| P088 | `borders`, `periods` (row 27) | Wrappers in another row (27 beside 01), calling `prefixBorders`, `prefixPeriods` over `prefixFunction(s)`; MAS `periods` |
| P088 | `isPrimitive` (row 27) | Bool test (ruling 1); calls `prefixPeriod` |
| P088 | `stringFromPrefixFunction`, `stringFromZ` (row 31) | Wrappers in row 31 calling `validPrefixFunction` (and `zToPrefix`), or a minimum-alphabet witness: kept either way; MAS `inverse_z_algorithm` |
| P050 | `blockTree`, `isCactus` (row 41) | `blockTree` wraps row 27 `blockCutTree` for cacti (OIW block forest); `isCactus` is a bool test (ruling 1; MAS `is_cactus`) |
| P061 | `multipointEvaluateGeometric` (row 22) | Wrapper in another row: row 22 owns the general chirp-z (LC `multipoint_evaluation_on_geometric_sequence`); row 65 `bluesteinTransform` calls it with a = 1 |
| P069 | `exactCover` (row 25) | Batch builder over `DancingLinks` (clarified rule) |
| P042 | `TreeEccentricities` fields `radius`, `center` (row 14) | Derived formulas of the eccentricities (no `treeRadius`, `treeCenter` functions exist) |
| P042 | `HLD::lca`, `distance`, `kthAncestor` (row 15) beside `LCA`, `EulerLCA` (row 07) (P042 rows) | Different algorithm: heavy-path climbing in O(log(n)) with O(n) memory against binary lifting and Euler RMQ (contributor proposal) |
| P042 | `TreeMonoid::pathFold` (row 15) beside `LCAFold::pathFold` (row 07) (P042 rows) | Larger domain: point updates and path or subtree actions; `LCAFold` is static (contributor proposal) |
| P042 | `EulerTour` (row 14) beside `lca_detail::Forest` (P042 rows) | Public static Euler tour with half-open ranges; `Forest` is a detail helper (contributor proposal) |
| P066 | `vectorHash` (row 19) | Different algorithm: random-base polynomial hash (clarified rule) |
| P066 | `pairHash` (row 19) | Different algorithm: random-base polynomial hash (MAS `hash_pair`), not `CustomHash` splitmix mixing |
| P066 | `kWayMerge` (rows 15, 44) | Heap merge, O(N log k), a larger domain than a two-way merge (ALG external sorting); row 44 `externalMergeSort` calls it |
| P066 | `hilbertIndex2D` (row 16) | Row 16 owns Hilbert order (`Docs/00-notes.md`, Advanced and Esoteric contracts); see P067 |
| P062 | `grundySum` (row 25) | Composition over whole games (`grundy`, then `nimSum`) returning a winning component move; NYA, CPALG |
| P062 | `nimWin` (row 25) | Named special case (ruling 1) |
| P077 | `evaluateExpression` (row 37) | Named composition (OIW expression): calls `tokenize`, `shuntingYard`, `evaluatePostfix` |
| P051 | `coreNumbers`, `degeneracyOrdering`, `kCore` (row 42) | Different results (cores, order) and a derived set (clarified rule) |
| P051 | pair `edgeConnectivity`, `edgeDisjointPaths`, edge `mengerCut` (row 45) | Graph connectivity with unit capacities built inside (decided 2026-10-11) |
| P051 | global `edgeConnectivity` (rows 45, 43, 44) | Wrapper in row 45 (NetworkX `edge_connectivity`): `stoerWagnerSparse` undirected, `MaxFlow` directed (larger domain) |
| P051 | `differenceConstraints` (rows 45, 43, 44) | Named composition (OIW difference constraints): calls `bellmanFord` |
| P073 | `countBalanced` (row 53) | Larger domain: prefix completions and k bracket types (CSES 2187, CPALG, OIW); calls Mathematics `ballot`, `catalan` |
| P073 | `setPartitionDP` (row 32) | Named composition (EDPC U "Grouping"): calls `submaskTransitionDP`, `restoreMask` |
| P039 | `diskIntersectionArea` (row 23) | Named special case (k = n) of `diskCoverageArea` with its own convex O(n log n) algorithm |
| P039 | unweighted beside weighted forms (row 25 (and row 48, P146)) | Named special case (unit weights); the weighted forms have the larger domain |
| P039 | `greatCircleDistanceApprox` (row 27) | Wrapper on `Point3` calling `angleApprox`, and another formula for angle inputs (KACTL `sphericalDistance`) |
| P065 | `newtonPolish` (row 29) | Larger domain: complex polynomial roots after Aberth; row 17 `newtonRoot` is real only |
| P071 | `daysBetween`, `addDays`, `dayOfYear` (row 27) | Named compositions (Python `datetime`): call `daysFromCivil`, `civilFromDays` |
| P038 | `DynamicUpperHull` (row 20) | Separate operation: fixed universe with active flags, upper hull only (MAS `dynamic_upper_hull`) |
| P041 | `halfplaneCount`; `manhattanNearestNeighbour` (row 29) | `halfplaneCount`: offline angular sweep, or a row 29 wrapper of row 41 `halfspaceRangeCount`; `manhattanNearestNeighbour`: bipartite domain (MAS `manhattan_nns`), not row 21's one-set sweep |
| P128 | `orient2d` `lll` overload (row 36, formerly `orient2dInt`) | Larger domain: exact past about 2^62, where row 01 `orient` overflows (SHEWCHUK) |
| P129 | `countIntersections` (row 12) | `Count` variant, cheaper than the pair list (ruling 4; CPALG, CGAL) |
| P130 | `triangulatePolygon`; `trapezoidalDecomposition` (row 18) | `triangulatePolygon` (formerly `triangulateSimple`, renamed P276): named composition of `monotonePartition`, `triangulateMonotone` (MAS); `trapezoidalDecomposition`: row 18 wrapper of row 34 `TrapezoidalMap::build` |
| P131 | `tangentCircles(point, point, r)`; point and line cases of `apolloniusCircles` (row 24) | Wrappers in row 24 calling `circleCentersApprox`, `circumcircleApprox`, `incircleApprox` and the excircles (HIT `problem_of_apollonius`) |
| P135 | `manhattanMst`; `manhattanFarthestPair` (row 21) | Builders over the edges and the Chebyshev transform (clarified rule) |
| P135 | `euclideanBottleneck` (rows 21, 22, 32) | Separate operation: unit-disk components at a threshold, composed over `euclideanMst` |
| P135 | `euclideanMstQuadratic` (rows 21, 22, 32) | Lighter O(n^2) Prim (Priority); formerly `euclideanMstDense`, renamed P276 |
| P135 | `Voronoi::adjacency` (rows 21, 22, 32) | Wrapper on another type calling row 31 `Delaunay::adjacency` |
| P132 | `Plane3::distanceApprox` (row 26) | A type's members are their own context (2026-11-08); calls the free `distanceApprox` |
| P137 | `hull3SurfaceArea`, `hull3Volume` (row 37) | Computed from the hull itself (clarified rule) |
| P090 | `countRange` (`_02_search` (`00-index.md:10`)) | Derived: the difference of two bisects (clarified rule) |
| P090 | `longestIncreasingSubsequenceLength` (`_05` (:13)) | Different result: the length only (clarified rule) |
| P090 | `binarySearchInt`; `argminUnimodal` (`_02`) | `binarySearchInt`: the endpoint-independent `binSearch(ok, ng, f)` C++ keeps beside `firstTrue`, renamed `binSearch`; `argminUnimodal`: Fibonacci search, renamed `fibSearch` |
| P091 | inlined `SegmentTreeMin`, `SegmentTreeMax`, `SegmentTreeSum`, `SparseTableGcd` (and `_29` range-add variants, P102) (`_08`, `_13`) | PyPy constant-factor variants (ruling 4) |
| P092 | `dijkstraWitness` (`_09` (:17)) | Named composition (LC `shortest_path`): `dijkstra`, then `restorePath`; named `dijkstraWitness` |
| P093 | `countOccurrences` (`_10` (:18)) | Derived count (clarified rule) |
| P095 | `rodCutting` (`_12` (:20)) | Builder over lengths and prices (clarified rule) |
| P098 | `rangeKthSmallest` (`_19` (:34)) | One-shot form of the persistent tree (clarified rule) |
| P101 | `randomPermutation` (`_27` (:42)) | Derived: a sample of `range(n)` (clarified rule) |
| P102 | `fold` (`_61` (:67)) | Mirrors the kept C++ `IntervalMap::fold` |
| P094 | `farthestPair` (`_55` (:61)) | Composition (LC `furthest_pair`) mirroring C++ `farthestPair`: `convexHull`, then `convexDiameter` |
| P110 | `nimSum` (`_58` (:64)) | Named special case (clarified rule) |
| P230 | `maximumIndependentSet` (`_59` (:65)) | Separate operation (ruling 2) |
| P115 | `RangeTree::count`, `prod` (row 43) | Wrapper in another row (43) calling row 16 `MergeSortTree::countRange` and row 15 `SegmentTree2DSparse::prod` (NYA abstract range tree) |
| P138 | `feasiblePoint3` (row 38) | Wrapper in row 38 calling row 42 `seidelLp` (CGAL `halfspace_intersection_interior_point_3`); named `halfspaceInteriorPoint3` |
| P139 | `upperEnvelope` (row 41) | Named special case (ruling 3) |
| P139 | `lowerEnvelope` (row 41) | Returns the envelope itself; DS `MonotoneCHT` and `LineContainer` answer queries only (MAS `line_min_function`, CGAL Envelope_2) |
| P139 | `geodesicPath` (row 39) | Named composition (alubiw CS860): row 18 triangulation, then `funnel` |
| P139 | `shortestPathAmongObstacles` (row 39) | Only form (irrational lengths; Graph weights are `lng`) |
| P140 | `minimumEnclosingBall` (d = 2) (row 42) | Floating, any d, beside row 14's exact circle (CGAL Bounding_volumes) |
| P143 | `l1Voronoi` (row 46) | Builder: the Chebyshev transform (clarified rule) |
| P148 | `alphaShape2` (row 50) | Named composition: filters `alphaComplex2` by alpha (CGAL Alpha_shapes_2) |
| P149 | `convexPolyhedronIntersection` (row 51) | Composition in row 51 calling row 38 `clipPolyhedronByPlane` |
| P150 | `continuousFrechet` (row 52) | Distance only, cheaper than the matching (ruling 4); the matching is renamed `continuousFrechetWitness` (P276) |
| P152 | exact `minimumVertexCover`; `cliqueCover` (row 49) | Separate operations (ruling 2) |
| P153 | `matroidUnion` beside `matroidPartition`; row 42 `forestDecomposition` (rows 48, 42) | Named compositions (HIT `matroid_union`): `matroidPartition` calls `matroidUnion`, which calls `matroidIntersection`; `forestDecomposition` calls `matroidPartition` on graphic matroids (Wikipedia, Arboricity) |
| P155 | `DynamicForest` (`link`, `cut`, `connected`, `lca`, `pathFold`, `pathApply`, `evert`) (row 51) | Wrapper in Graphs row 51 over DS row 37 `LinkCutTree` (KACTL, MAS, TKO); it calls it, never copies it |
| P155 | `secondBestMst`, `mstForcedEdge`, `mstForbiddenEdge`, `isMstUnique` (row 52) | Named special cases and bool tests (ruling 1) over `mstEdgeSensitivity` (CPALG `second_best_mst`, CF 160D, OIW) |
| P158 | `bipolarOrientation` (row 60) | Derived orientation (clarified rule) |
| P158 | `earDecomposition` (rows 60, 68) | Named composition (NetworkX chains): calls `chainDecomposition` |
| P162 | minimum and maximum perfect matching pair; `maximumWeightMatching` dense and sparse; row 58 `maximumWeightMatchingGabow` (P163) (rows 64, 58) | The min/max pair is a sign mirror (ruling 3), dense and sparse are different algorithms, Gabow is a third; names to `maximumWeightMatching`, `maximumWeightMatchingDense`, `maximumWeightMatchingGabow` (P276) |
| P166 | `cycleBasisWeight` (row 72) | Derived sum (clarified rule) |
| P166 | `eulerize` (row 69) | Named composition (NetworkX `eulerize`): calls `minimumTJoin` |
| P173 | `globalMinCutExtremeSets` (row 83) | Derived minimum (clarified rule) |
| P175 | `dynamicDiameter` (row 84) | Wrapper in another folder over DS row 39 `TopTree` with its `Diameter` preset (CEOI 2019 "Dynamic Diameter") |
| P177 | `dirichletConvolutionPrefix`, `dirichletInversePrefix` (row 32) | Free functions calling `DirichletSeries::mul`, `inv`, another context (user decision 2026-11-10) |
| P181 | `pFreeFactorialModPrime` (row 37) | Separate operation: the unit part of n! mod a large prime by Wilson blocks, not row 12 `factorialValuation` (CPALG, OIW) |
| P181 | `faulhaber(k)` (row 38) | Named composition in another row by another algorithm (MAS `faulhaber`): calls `bernoulliTable` |
| P183 | `labelledSet`, `labelledCycle`, `logarithmicTransform` (row 40) | Named compositions (OEIS transforms): call row 14 `fpsExp`, `fpsLog` with the EGF conversions |
| P183 | `burnsideCount` (row 40) | Reuses the caller's fixed-point counts; see P204 |
| P185 | `isIrreducible` (row 42) | Owner; see P188 |
| P186 | `circulantMultiply` (row 44) | Wrapper in row 44 calling row 13 `cyclicConvolution` (SUI `circulant_matrix`) |
| P186 | `toeplitzMultiply` (row 44) | Wrapper in row 44 calling row 13 `middleProduct` (SciPy `scipy.linalg.matmul_toeplitz`) |
| P186 | `hankelSolve`, `vandermondeSolve` (row 44) | `hankelSolve`: orientation mirror of `toeplitzSolve` (ruling 3), calling it; `vandermondeSolve`: Björck–Pereyra, a different algorithm, with the dual (transposed) solve the row names too |
| P188 | `isIrreducibleModPrime` (row 46) | Wrapper in row 46 calling row 42 `isIrreducible` (NTL `ZZ_pXFactoring`) |
| P188 | `composedSum`, `composedProduct` (row 46) | Mod-p power sums against exact arithmetic over Z; annotate both domains |
| P191 | `countWalks` (row 53) | Named composition (CPALG fixed-length paths): calls Core `Matrix::pow` or Mathematics `matrixPowerVector`; row 53 owns it |
| P192 | `latticeDeterminant` (row 49) | The only listing now: row 35 lists `gramDeterminant` |
| P193 | `MultivariateFps::mul` beside `multivariateConvolution (truncated)` (row 50) | Total-degree against per-coordinate truncation; annotate "(per-coordinate bounds)" |
| P195 | `maxPlusConvolutionConcave*` (row 53) | Named special case (ruling 3) |
| P198 | `locatorBerlekampMassey` (row 56) | Member of another type and a different algorithm beside `locatorEuclidean`; errors only call row 15 `berlekampMassey` |
| P200 | `SetPowerSeries::mul` (row 66) | Wrapper on another type (LC `subset_convolution`) calling row 24 `subsetConvolution` |
| P201 | `composedSumExact`, `composedProductExact` (row 61) | Exact over Z (see P188) |
| P204 | `burnsideOrbitCount` (row 64) | One-shot form from a group; `burnsideCount` takes the counts (clarified rule) |
| P208 | `isSubstring` (row 33) | Named special case (ruling 1) |
| P211 | `BidirectionalFmIndex::count` (row 36) | Wrapper on another type calling `FmIndex::count`, or its own synchronized-interval search if it holds no `FmIndex` |
| P213 | `deBruijnLinear` (row 41) | Different result: the linear sequence (clarified rule) |
| P216 | `RIndex::count` (row 46) | Wrapper on another type calling `RlFmIndex::count` (r-index, count and locate) |
| P218 | `prefixSubstringLcs` (row 48) | Named composition (LC `prefix_substring_lcs`): `SemiLocalLcs` build, then `prefixSubstring` |
| P219 | `forwardDawg` (row 49) | Different result: occurrences, not matching statistics (clarified rule) |
| P223 | `bisectionRoot` (`_50` (:79)) | Named composition mirroring C++ `bisectionRoot`: calls the real bisection on the sign of f |

## Conventions

Locations are relative to the package's folder and tests to `96-Local Testing/<folder>/`; "row NN" is the `00-index.md` row of header `NN-`, "clarified rule" cites `03-cpp.md`, One API per operation, and "ruling N" the Nth of the Rulings above. A `remove (row only)` item has no code to change, or its code is a pinned extraction whose body stays until the package rewrites it.

## P020 — 01-Core — matrix (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Matrix(vector<T>, bool axis)` | `10-matrix.hpp:267` | `Matrix(1, n, v)`, `Matrix(n, 1, v)` | ask | an empty `v` gives 0×0 here and 1×0 or 0×1 from the replacements, so the results differ at the edge (`03-cpp.md` removes only a form with exactly its purpose and results): remove with the edge accepted, or keep | none | `10-matrix.hpp:345,346,421,549` |
| `*=(vector)`, `vector * Matrix`, `Matrix * vector` | `10-matrix.hpp:318,345,346` | `a * Matrix(n, 1, b)`, `Matrix(1, n, b) * a` | remove | argument shape of `*` with a vector builder; same empty-vector fact as the axis constructor (P275) | none | none |
| Gauss–Jordan loop in `det`, `solveLin`, `adj`, `ker`, `rref` | `10-matrix.hpp:448-467,492-524,551-584,723-749,770-797` | one shared elimination | remove | body-level ×5 | — | — |
| `FastMatMul::addMatTo`, `subMatTo`; `getMod`, `getModInv` | `10-matrix.hpp:24,36,127,129` | `FastGaussian::addRow`, `subRow`; `T::mod()`; `Montgomery32(mod).inv` | remove | body-level second implementations | — | — |
| 8-lane REDC in `mulRow`, `elimRow` | `10-matrix.hpp:161-179,201-222` | `Montgomery32` `mul8`, `red8` | remove | second implementation of `Montgomery32` `mul8`, `red8`; a copy stays only with a recorded benchmark (ruling 4) | — | — |
| row 10: `matVec`, `vecMat`, `adjugate (singular rank n-1 case)` | row 10 | `*` with a vector builder; `adj` | remove (row only) | argument and result shape; another name | — | — |
| row 10: `solveReal`, `invReal`, `rankReal` | row 10 | the floating branches of the exact names | remove (row only) | another name for the floating instantiation of the same templated `solveLin`, `inv`, `rnk`; `Real` names a real-valued domain beside a non-real default | — | — |

## P022 — 01-Core — bitmatrix, bitmatrixmini (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `BitMatrix(string, axis)` | `12-bitmatrix.hpp:31` | `BitMatrix(1, n, s)`, `BitMatrix(n, 1, s)` | remove (row only) | flag between shapes | none | none |
| dead `tmp`, `mul` in `pow` | `12-bitmatrix.hpp:270-292` | `operator*` | remove (row only) | unused second copy | none | none |
| elimination in `det`, `ref`, `rref`, `solveLin` | `12-bitmatrix.hpp:234-253,333-353,361-382,393-420` | one elimination | remove (row only) | body-level; P022 merges | — | — |
| `-=`, binary `-`, unary `-` | `12-bitmatrix.hpp:109,129,145,151` | `+=`, `+`, unary `+` | ask | equal over GF(2); generic ring code may need them | — | — |

## P023 — 01-Core — sparsematrix (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| Krylov prologue in `det`, `solveLin`, `pow` | `14-sparsematrix.hpp:132-166,183-204,228-249` | planned `minPoly (Wiedemann)` | remove (row only) | body-level; one implementation | — | — |
| row 14: `spmv`, `powVec` | row 14 | `operator*(vector)` (:50); `steadyState(a, v, k)` | remove (row only) | another name | — | — |
| `berlekampMassey` | `14-sparsematrix.hpp:74` | Poly `findLinearRecurrence` | ask | floating T is numerically unsound for Wiedemann; over exact fields it copies Poly `findLinearRecurrence`, kept only with a recorded reason (no `16-poly.hpp` include) | none | `14-sparsematrix.hpp:132-249` |
| row 14: `toCOO`, `toCSR` | row 14 | `entries`, triplet constructor | ask | conversions SciPy names (`tocoo`, `tocsr`, `tocsc`): kept as wrappers only if COO and CSC are distinct types | — | — |
| `steadyState` | `14-sparsematrix.hpp:276` | `pow(a, k, v)` | ask | the binding keep ("fixed-iteration power method") does not match the code: k plain products with no normalization; rename `powNaive` or fold into `pow` as a small-k branch? | none | none |

## P088 — 07-Strings — string periodicity, multiple string, string reconstruction (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `wordRoot` | row 27 | `prefixBorders`, `prefixPeriods`, `prefixPeriod(pi, true)`, `prefixOccurrences(pi)` over `prefixFunction(s)` | ask | row 27 wrapper of `prefixPeriod(pi, true)` (wrapper clause) or the same function listed twice (`omissions.md`, 2026-11-08: keep only the owner)? | — | `Docs/00-notes.md` (row 27 `wordRoot`) |
| `borderOccurrenceCounts` | row 27 | `prefixBorders`, `prefixPeriods`, `prefixPeriod(pi, true)`, `prefixOccurrences(pi)` over `prefixFunction(s)` | ask | row 01 `prefixOccurrences(s, s)` gives the same result; the owner name would clash with the `(pi)` overload for `vector<int>` strings | — | `Docs/00-notes.md` (row 27 `primitiveRoot`) |
| `infeasible` | row 31 | `validPrefixFunction(pi, s)`, `zToPrefix` | remove (row only) | a result convention of row 31 (status), not an operation; it goes to the contracts | — | — |

## P089 — 07-Strings — multidimensional matching (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `match2DCount` | row 32 | size of `match2D` | ask | a `Count` variant (name `match2DCount`) only if it counts without building the occurrence list; else it is `match2D(...).size()` and goes | — | — |

## P068 — 06-Miscellaneous — bitset optimization, contestallocator, memoization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `BitsetDP::shiftOr` | row 22 | Core `Bitset::combineShift<Op::Or>(dp, k)` | ask | kept as a `BitsetDP` member calling Core `combineShift<Op::Or>` (user decision 2026-11-10); `shiftOr` means bitap in Strings row 17, so it needs another name | — | — |

## P010 — 04-Graphs — graph, traversal, toposort, dsu, shortest path, mst (in progress)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `dagShortestPaths(g, sources, longest)` flag; `06-mst.hpp` `maximum` flags of `kruskal`, `primSparse`, `boruvka`; `eulerianTrail(g, start, lexicographic)` flag | rows 05, 06, 10 | `dagLongestPathsFrom`; `kruskalMaximum`, `primSparseMaximum`, `boruvkaMaximum`; `eulerianTrailSmallest` (named mirrors, ruling 3) | remove (user decision 2026-11-14) | a flag choosing between operations is a removable form; `dagLongestPaths` stays retired (P275 guard), `KruskalReconstruction` keeps its constructor parameter; GR01 applied row 03 (`topologicalSortSmallest`), the rest waits for the runs owning rows 05, 06 and 10 | the flag cases | callers of the flags |

## P042 — 04-Graphs — all pairs shortest path, shortest path advanced, tree algorithms, hld, levelancestor (in progress)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `HLD::addEdge`, `build` | `15-hld.hpp` (legacy) | `HLD(g, root)` | remove | legacy input format a builder converts (user decision 2026-11-09) | — | — |
| `apspPath`; `apspDijkstra` | row 12 | `AllPairsResult::path`; `johnson` | remove | a member of the result, and a narrowing with identical results and cost (user decision 2026-11-09) | — | — |
| `runFloydWarshall`; `SPFA::solve` | rows 12, 13 (legacy) | `floydWarshall(g)`; `spfa` | remove | legacy forms (user decision 2026-11-09) | — | — |

## P066 — 06-Miscellaneous — smalltolarge, hilbertorder, randomized algorithms, hash families, parallelbinarysearch, cdq (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mergeSortedVectors` | rows 15, 44 | `std::merge`; row 44 | remove (row only) | one `std::merge` call; no source names it | — | — |

## P067 — 02-Data Structures — range query offline (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Mo::getHilOrd` | `23-range_query_offline.hpp:19` | Miscellaneous row 16 `hilbertIndex2D` | remove (row only) | second implementation once P066 adds `hilbertIndex2D` (Changes); pinned body stays until P067 rewrites Mo | none | `99-all.hpp:21` |

## P076 — 06-Miscellaneous — state space search, game search (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `TranspositionSet` | row 35 | `HashSet` | remove (row only) | another name for `HashSet`; sources name only row 36 `TranspositionTable` | — | — |
| `minimax` beside `negamax` | row 36 | each other | ask | same results under zero-sum scores; `minimax` stays only if the row supports non-zero-sum evaluation | — | — |

## P051 — 04-Graphs — graph decomposition, implicit graph, differenceconstraints, graph connectivity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `complementShortestPath` | rows 45, 43, 44 | `stoerWagner`; `complementBfs`; `bellmanFord` after a builder | ask | weighted complement (edges cost a, non-edges b): separate operation; only a path from `complementBfs`: a narrowing in row 43 | — | — |

## P063 — 05-Mathematics — optimization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `convexHullTrickDp`, `liChaoDp` | row 27 | DS rows 21, 22 | ask | the "bridges" to DS rows 21 and 22 are a second include path (remove); CPALG names the CHT DP, so a DP driver dp_i = c_i + min_j(dp_j + b_j·a_i) calling `LineContainer`/`MonotoneCHT` would stay: one driver or two? | — | — |

## P087 — 07-Strings — runs (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `tandemRepeats` | row 26 | one compact form | remove (row only) | another name for squares | — | — |
| `squares (all occurrences)`, `repetitionsCompressed` | row 26 | one compact form | remove (row only) | one result in two shapes: the row keeps one, `squaresAll`, returning the Main–Lorentz compressed groups | — | — |

## P039 — 03-Geometry — circleunion, geometric median, spherical geometry (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `antipodal` | row 27 | `r * angleApprox` (3D), `-p` | remove (row only) | `-p` from another row; no source names it | — | — |

## P079 — 06-Miscellaneous — scheduling, optimal merge (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `huffmanCost` | row 40 | `huffmanTree` cost | ask | a cheaper projection with no tree arrays (ruling 4) and a custom-combine overload with a larger domain; it stays only if a constant-factor gain is measured, else it folds into `huffmanTree` | — | — |
| `optimalMergePattern` | row 40 | `huffmanTree` (internal nodes in merge order) | remove (row only) | a result shape of `huffmanTree` in the same row | — | — |

## P071 — 06-Miscellaneous — calendar time, encoding (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `julianDayNumber` | row 27 | `daysFromCivil(…) + 2440588` | remove (row only) | an index base (epoch + 2440588) of `daysFromCivil` | — | — |

## P135 — 03-Geometry — manhattan geometry, geometricmst, voronoi (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `manhattanClosestPair` | rows 21, 22, 32 | the octant sweep; the largest `euclideanMst` edge; `primDense`; row 31 `Delaunay::adjacency` | remove (row only) | the least edge of `manhattanMstEdges` in the same row; no source names it | — | — |

## P090 — 08-Python — io, search, number theory, combinatorics, sequences, pypy idioms (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `lowerBound`, `upperBound`, `bisectKey` | `_02_search` (`00-index.md:10`) | `bisect_left`, `bisect_right` with `key=` | remove (row only) | stdlib under another name | — | — |
| `modPow`, `isqrt`, `floorDiv`, `floorMod`, `divModFloor` | `_03` (:11) | `pow(a, e, m)`, `math.isqrt`, `//`, `%`, `divmod` | remove (row only) | stdlib under another name | — | `Docs/00-notes.md`, Numeric policy |
| `comb`, `perm`; `fibonacci` | `_04` (:12) | `math.comb`, `math.perm`; `fibonacciPair(n)[0]` | remove (row only) | stdlib; projection | — | — |
| `prefixSums` | `_05` (:13) | `accumulate(a, initial=0)` | remove (row only) | stdlib under another name | — | — |
| `inverseXgcd` (formerly `modInv`) | `_03` | `pow(a, -1, m)` | keep | mirrors C++ Mathematics row 09 `inverseXgcd`, -1 for a nonunit, which stdlib `pow(a, -1, m)` does not give (user decision 2026-11-10, P276 mirror rule) | — | `Docs/00-notes.md`, Legacy migration |

## P091 — 08-Python — dsu, fenwick, segmenttree, sparsetable, heaps (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `heapMerge`, `nSmallest`, `nLargest` | `_52` (:23) | `heapq.merge`, `nsmallest`, `nlargest` | remove (row only) | stdlib under another name | — | — |

## P096 — 08-Python — prime factor (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `primeFactors` | `_14` (:29) | `factorize` flattened | remove (row only) | projection | — | — |
| `primePi` beside `primeCount` | `_14` | each other | remove | both would be Lucy (Meissel–Lehmer and Min_25 are omitted): a second implementation; the survivor takes the C++ name `primeCount` | — | — |

## P097 — 08-Python — flow, matching (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `assignmentMin` | `_16` (:31) | `hungarian(a)` | remove (row only) | projection | — | — |

## P098 — 08-Python — tree, rollback, persistent, bitset, wavelet matrix (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `popcount`, `highbit` | `_22` (:37) | `int.bit_count()`, `bit_length() - 1` | remove (row only) | stdlib | — | — |
| `rangeFreq` | `_56` (:62) | `countRange` | remove (row only) | another name; `countRange` is the C++ Data Structures row 17 name (user decision 2026-11-10, P276) | — | — |

## P100 — 08-Python — trie, aho, suffix, eertree, lyndon (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `suffixArrayInt` | `_25` (:40) | `suffixArray(s, upper=None)` | remove (row only) | input format | — | — |

## P101 — 08-Python — offline, randomized, io advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Rng` | `_27` (:42) | `random.Random` | remove (row only) | stdlib under another name | — | — |
| `FastReader`, `FastWriter`, `tokenizeBytes` | `_28` (:43) | `_01` `FastIO`; `bytes.split` | remove (row only) | second implementation; stdlib | — | — |
| `weightedChoice` | `_27` | `rng.choices(pop, weights)[0]` | remove | stdlib `random.choices` | — | — |

## P104 — 08-Python — twosat, lowlink, eulertrail (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `solve` | `_33` (:48) | `satisfiable` | remove (row only) | mirrors the C++ removal | — | — |
| `setFalse(x)` | `_33` | `setValue(~x)` | remove (row only) | argument shape (`~x`) of `setValue`, which becomes `setValue(lit)` as in C++ | — | — |

## P105 — 08-Python — shortest paths, mincostflow (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `floydPath` beside `restorePathAllPairs` | `_36` (:51) | each other | remove | same context and result as `restorePathAllPairs`; no source names it | — | — |

## P106 — 08-Python — number theory advanced, rational (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `limitDenominator` | `_57` (:63) | `Fraction(x).limit_denominator(N)` | remove (row only) | stdlib | — | — |
| `kthRootMod` beside `kthRootModPrime` | `_38` (:53) | each other | remove (row only) | two names for one operation: the module keeps one, `kthRootModPrime` (C++ 05 row 11) | — | — |

## P109 — 08-Python — geometry float (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `norm`, `dotF`, `crossF` | `_41` (:56) | `math.hypot`; `_11` `dot`, `cross` | remove (row only) | stdlib; same formula | — | — |

## P110 — 08-Python — dp advanced, game theory (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mexOf` | `_58` (:64) | `_05` `mex` | remove (row only) | second implementation | — | — |

## P115 — 02-Data Structures — rangetree, kineticheap, persistentqueue, ordermaintenance (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `PersistentDeque` | row 47 | row 57 `FingerTree` | ask | no current source names a persistent deque (LC has only `persistent_queue`); a finger tree keeps it as a row 57 wrapper, Okasaki or Kaplan–Tarjan catenable deques as its own algorithm, if a source is recorded | — | — |

## P122 — 02-Data Structures — linearrmq, commonintervaltree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `strongIntervals` | row 55 | the tree's `nodes` | remove (row only) | projection | — | — |

## P138 — 03-Geometry — halfspaceintersection3d (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `removeRedundant3` | row 38 | the intersection result | remove (row only) | a projection of the `HalfspaceResult3` faces; no source names it | — | — |

## P140 — 03-Geometry — randomizedlp (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `lp2d`, `lp3d` | row 42 | `seidelLp` at d = 2, 3; row 14 `minimumEnclosingCircle` | remove (row only) | narrowings of `seidelLp` unless a measured specialization beats it (ruling 4); row 10 `chebyshevCenterLpApprox` calls `seidelLp` | — | — |

## P148 — 03-Geometry — alphashape (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `criticalAlphas` | row 50 | views of `alphaComplex2` | remove (row only) | the projected spectrum of `alphaComplex2` | — | — |

## P149 — 03-Geometry — polyhedron boolean (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `bspBoolean` operation flag | row 51 | a fold of row 38 `clipPolyhedronByPlane`; an operation flag | remove (row only) | a flag choosing between operations; the named union, intersection and difference remain | — | — |

## P158 — 04-Graphs — stnumbering, coloring exact, cycle enumeration, feedback set (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `minimumCostCycle` | rows 60, 68 | `chainDecomposition`; `weightedGirth`, `directedGirth` | ask | negative arc weights without a negative cycle: a larger domain, kept; otherwise another name for `weightedGirth`, `directedGirth` | — | — |

## P160 — 04-Graphs — planarity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `planarEmbedding` | row 62 | `isPlanar` returning row 29 `PlanarEmbedding` | remove (row only) | a result shape: `isPlanar` returns the row 29 `PlanarEmbedding` directly, as NetworkX `check_planarity` does | — | — |

## P161 — 04-Graphs — fully dynamic connectivity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `offlineFallback` | row 63 | row 26 `OfflineDynamicConnectivity` | remove (row only) | another name; row 26 owns it and no source names it | — | — |

## P166 — 04-Graphs — tjoin, minimumcyclebasis, hamiltonian (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `minimumHamiltonianCycle` | row 73 | `travelingSalesman` (settle n ≤ 2) | ask | the recorded keep ("takes a graph; `travelingSalesman` takes a matrix") is an argument shape; it stays only for a larger domain (loops, parallel edges, cycles of length 1–2) | — | — |

## P168 — 04-Graphs — dynamicmst (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mstWeightChange` | row 71 | `dynamicMstOffline` | ask | independent single-change queries: a separate operation; cumulative changes: another name for `dynamicMstOffline` | — | — |

## P181 — 05-Mathematics — fastfactorial, bernoulli (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `wilsonBlock` | row 37 | row 12 `factorialValuation`; `factorialBlockTable` | ask | undefined by any note or source: one entry of `factorialBlockTable` or `consecutiveProducts` (remove), or a distinct quantity (keep)? | — | — |
| `bernoulliNumber` | row 38 | row 14 `prefixSumOfPolynomial(x^k)`; `bernoulliTable` | remove (row only) | a single entry of `bernoulliTable` at the same cost, unless P181 records a cheaper single-value algorithm | — | — |

## P182 — 05-Mathematics — special functions (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| gamma, log-gamma, erf, erfc, beta, Bessel, Neumann, zeta, complete elliptic integrals | row 39 | `std::tgamma`, `lgamma`, `erf`, `erfc`, `beta`, `cyl_bessel_*`, `cyl_neumann`, `riemann_zeta`, `comp_ellint_1`, `comp_ellint_2` | ask | `std` is not a library context: keep only forms that differ (an `FnStatus` with an error estimate, another precision, complex or real-order domains)? | — | — |

## P195 — 05-Mathematics — relaxed convolution, semiring convolution (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `shortProduct` | row 52 | `13-convolution.hpp:282-287` `truncatedConvolution` | remove (row only) | same result | — | — |

## P200 — 05-Mathematics — tableau algorithms, convolution specialized, set power series (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `connectedComponentsBySubset` | row 66 | `SetPowerSeries::log` | remove (row only) | the row defines it so | — | — |
| `spsComposition` | row 66 | `24-transform_algorithms.hpp` `subsetConvolution`; `spsPolynomialComposite` | remove (row only) | the same task as `spsPolynomialComposite` (LC `composition_sps`), which keeps its name | — | — |

## P206 — 06-Miscellaneous — probabilistic sketches, streaming algorithms, derandomization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `StreamReservoir` | row 46 | row 18 `reservoirSample` | remove (row only) | a second listing: row 18 owns the single-pass reservoir (Wikipedia), so it must be incremental; `Docs/00-notes.md` "adapts 18" changes with P066 | — | — |

## P207 — 06-Miscellaneous — alphabetic tree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `alphabeticTreeWitness` | row 48 | the `garsiaWachs` tree | remove (row only) | projection | — | — |
| `alphabeticCodeLengths` | row 48 | leaf depths of that tree | remove (row only) | a projected field (leaf depths): the tree result carries depths, as row 40 `huffmanTree` does | — | — |

## P209 — 07-Strings — dynamicaho, approximate matching, palindrome advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `palindromicLength (per prefix)` | row 39 | `13-palindromictree.hpp:91` `palindromicLength()` | remove (row only) | existing operation | — | — |

## P213 — 07-Strings — stringisomorphism, debruijn, zivlempel, linear string algorithms, combinatorial words (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `deBruijnCyclic` | row 41 | `deBruijnSequence` | remove (row only) | another name | — | — |
| `injectiveRenamingMatch` | row 40 | `parameterizedMatch` | remove (row only) | an injective renaming of the pattern alphabet into the window's is Baker's p-match: `parameterizedMatch` in the same row | — | — |

## P222 — 08-Python — algebraic, combinatorial species (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `necklaceCountColors` | `_48` (:77) | `necklaceCount` | remove | the same count as `necklaceCount` in the same module | — | — |

## Open questions

Every remaining `ask` above, with the fact the user decides. `/package` and `/prune` put a package's `ask` rows to the user before changing anything (user decision 2026-11-10).

- **P022** GF(2) `-=`, `-`, unary `-`: must `BitMatrix` offer the full ring operator set for generic code?
- **P020** `Matrix(vector<T>, bool axis)`: accept the empty-`v` shape change (0×0 against 1×0 or 0×1) and remove it, or keep it?
- **P023** `berlekampMassey`: is floating T a supported domain (recommended: no), and is the exact-field copy of Poly `findLinearRecurrence` kept with a recorded reason? `toCOO`, `toCSR`, `toCSC`: are COO and CSC distinct types? `steadyState`: rename it `powNaive` or fold it into `pow` as a small-k branch?
- **P088** row 27 `wordRoot`: a wrapper of row 01 `prefixPeriod` (keep) or the same function listed twice (owner only)? (The name clash with Mathematics row 11 `primitiveRoot` ended with the P276 rename.) `borderOccurrenceCounts`: a row 27 wrapper, or dropped in favour of row 01 `prefixOccurrences(s, s)`?
- **P079** `huffmanCost`: is a constant-factor gain over `huffmanTree` measured (keep), or does it fold in?
- **P068** `BitsetDP::shiftOr` stays (user decision 2026-11-10): which name other than bitap's `shiftOr`?
- **P076** `minimax`: does row 36 support non-zero-sum evaluation?
- **P051** `complementShortestPath`: a weighted complement problem, or a path from `complementBfs`?
- **P063** the CHT "bridges": one DP driver, two (monotone and Li Chao), or none?
- **P089** `match2DCount`: does it count without building the occurrence list (`match2DCount`), or is it `match2D(...).size()`?
- **P090** `modInv`: answered 2026-11-10 by the user's P276 mirror rule: `inverseXgcd`, -1 for a nonunit (row above).
- **P115** `PersistentDeque`: which current source names it, and is it a `FingerTree` wrapper or its own algorithm?
- **P158** `minimumCostCycle`: does it accept negative arcs without a negative cycle?
- **P166** `minimumHamiltonianCycle`: does the graph form have a larger domain than `travelingSalesman`?
- **P168** `mstWeightChange`: independent single-change queries or cumulative changes?
- **P181** `wilsonBlock`: which quantity does it return?
- **P182** the `std::` special functions: which forms differ enough to keep?
