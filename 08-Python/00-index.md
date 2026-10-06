# 08 Python — inventory

Scope: a deliberate PyPy-first contest subset of folders `01`–`07` plus the PyPy performance idioms a Python contestant needs (`04-python.md`). Native `int`, `Fraction`, `heapq`, `bisect`, `itertools`, `math` and `functools` replace Core numeric types and Core/Mini wrappers; nothing here is an automatic port of a C++ header. Full algorithm families, SIMD kernels and research string/geometry indexes stay in the C++ folders; a Python row exists only where a PyPy contestant plausibly runs it inside judge limits. Every numbered module is `planned`; contracts, omissions and cross-folder ownership are in `80-notes.md`, sources in `81-sources.md`.

## Basic

| Module | Operations | Status |
|---|---|---|
| `_01_io.py` | readAll; readInts; readInt; readLine; readTokens; readStr; readMatrix; readGraphEdges; writeLine; writeInts; writeAll; flushOut; FastIO: read, readline, write, flush; Interactive: ask, tell, finish | planned; pyrival, pypy, stdlib |
| `_02_search.py` | lowerBound; upperBound; countRange; firstTrue; lastTrue; binarySearchInt; binarySearchReal; ternaryMinInt; ternaryMinReal; goldenSection; argminUnimodal; bisectKey (3.10 key= adapter) | planned; pyrival, stdlib |
| `_03_number_theory.py` | extgcd; modInv; modPow; floorDiv; ceilDiv; floorMod; divmodFloor; sieve; linearSieve; primeList; segmentedSieve; spfSieve; isqrt; iroot; isSquare; isPrimeTrial; divisorsTrial; phiOne; phiSieve; mobiusSieve; divisorCountSieve; divisorSumSieve; multiplicativeSieve | planned; pyrival, cpalg, stdlib, legacy: OLD/Team Notebook/src/math/old_crt.py (inv) |
| `_04_combinatorics.py` | Comb: fact, invFact, C, P, H, catalan, inv; comb; perm; factorialTable; catalanNumber; ballot; derangements; fibonacci (fast doubling); fibPair; stirling1Table; stirling2Table; bellTable; partitionTable; eulerianTable; permutationRank; permutationUnrank; nextPermutation; bracketNext; bracketRank; bracketUnrank; josephus | planned; pyrival, cpalg, lc, stdlib |
| `_05_sequences.py` | lisLength; lisWitness; lisNonStrict; lisCount; inversions; kadane; kadaneCircular; compress; prefixSums; differenceArray; prefix2d; difference2d; rangeSum2d; prevLess; nextLess; prevGreater; nextGreater; slidingMin; slidingMax; Swag: push, pop, fold; DequeSwag: pushFront, pushBack, popFront, popBack, fold; mex; kthSmallest; majority; cartesianTree; runLengthEncode; twoPointersWindow | planned; pyrival, cpalg, lc, stdlib |
| `_06_dsu.py` | DSU: find, unite, same, size, count, groups, roots, reset | planned; pyrival, aclpy |
| `_07_fenwick.py` | Fenwick: build, add, prefix, sum, get, set, lowerBound, kth; RangeFenwick: add, sum, get; Fenwick2D: add, sum | planned; pyrival, aclpy |
| `_08_segmenttree.py` | SegmentTree: build, set, get, prod, allProd, maxRight, minLeft; SegTreeMin; SegTreeMax; SegTreeSum (inlined operation variants) | planned; pyrival, aclpy |
| `_09_graph.py` | buildAdj; buildCsr; bfs; bfsDist; bfs01; dfsIter; components; isBipartite; oddCycle; topoSort; cycleDirected; cycleUndirected; dijkstra; dijkstraPath; restorePath; kruskal; prim; scc; condensation; gridBfs | planned; pyrival, aclpy, cpalg, lc |
| `_10_strings.py` | prefixFunction; kmpFind; kmpCount; periodOf; borders; zFunction; zFind; manacher; longestPalindrome; countPalindromes; minRotation; RollingHash: get, combine, lcp, equal; hashOf; doubleHash | planned; pyrival, aclpy, cpalg |
| `_11_geometry.py` | cross; dot; orient; distSq; manhattan; rotate90; onSegment; segmentsIntersect; lineIntersection (Fraction); convexHull (strict); convexHullCollinear; polygonArea2; perimeter; pointInPolygon; pointInConvex; sortByAngle; latticePointsOnSegment; pickInterior | planned; pyrival, cpalg, lc |
| `_12_dp.py` | knapsack01; knapsack01Witness; knapsackUnbounded; knapsackBounded (binary splitting); knapsackCount; coinChangeMin; coinChangeWays; subsetSumReachable; lcsLength; lcsWitness; lcsWitnessLowMemory (Hirschberg); editDistance; editDistanceAlignment; gridPathCount; gridMinPathSum; rodCutting | planned; cpalg, oiwiki, legacy: OLD/Team Notebook/src/misc/old_lcs.py |
| `_13_sparsetable.py` | SparseTable: query; SparseTableMin: query, argmin; SparseTableGcd: query; BlockRmq (linear build, word masks): query; DisjointSparseTable (non-idempotent): query | planned; pyrival, cpalg |
| `_51_pypy_idioms.py` | bootstrap (generator trampoline); runWithStack (threading stack size); packPair; unpackPair; packTriple; shieldKey; ShieldedDict; ShieldedSet; modMul64 (float-reciprocal, `__pypy__.intop` with CPython fallback); bucketSort; ordersort; ordersortLong; multikeyOrdersort; isPyPy; localsBind | planned; pyrival, pypy |
| `_52_heaps.py` | RemovableHeap: push, pop, top, remove, size, empty; MaxHeap: push, pop, top; RunningMedian: add, remove, median, size; DoubleEndedHeap: push, popMin, popMax, min, max, size; KeyHeap: push, pop, decrease; heapMerge; nSmallest; nLargest | planned; pyrival, lc, stdlib |

## Advanced

| Module | Operations | Status |
|---|---|---|
| `_14_prime_factor.py` | isPrime (deterministic bases below 3.3e24); isProbablePrime; millerRabin; pollardRhoBrent; factorize; primeFactors; divisors; divisorCount; divisorSum; phiFromFactors; isPrimePower; isSquareFree; countPrimes (Lucy); primePi | planned; pyrival, cpalg, lc |
| `_15_flow.py` | Dinic: addEdge, flow, minCut, edges, edge, changeEdge, reset; BoundedFlow: addEdge, feasible, flow, edges; projectSelection: addItem, addDependency, solve | planned; pyrival, aclpy, cpalg, lc |
| `_16_matching.py` | hopcroftKarp; minVertexCover; maxIndependentSetBipartite; kuhn; hungarian (rectangular, duals); assignmentMin; blossomMatching (unweighted general) | planned; pyrival, cpalg, lc |
| `_17_tree.py` | rootTree; subtreeSizes; eulerTour; LcaBinaryLift: lca, kthAncestor, dist, jump, pathFold; LcaEuler: lca; Hld: head, pos, lca, pathRanges, subtreeRange, kthOnPath; virtualTree; diameter; center; centroid; CentroidDecomposition: build, parent, order; reroot (combine, finalize); treeIsomorphismClasses; pruferEncode; pruferDecode | planned; pyrival, cpalg, lc |
| `_18_rollback.py` | RollbackDsu: find, unite, same, size, snapshot, rollback, undo; WeightedDsu: find, unite, diff, same; ParityDsu: find, unite, parity, same; offlineDynamicConnectivity | planned; pyrival, lc |
| `_19_persistent.py` | PersistentSegTree: build, set, get, prod, versions; PersistentCountTree: build, kth, countLess, countRange; rangeKthSmallest | planned; pyrival, lc |
| `_20_polynomial.py` | convolve; convolveNtt; convolveMod (three primes); convolveInt (Kronecker big-int); convolveFft; ntt; intt; polyAdd; polySub; polyMul; polyDivmod; polyEval; polyDerivative; polyIntegral; polyInv; polyLog; polyExp; polyPow; polySqrt; taylorShift; multipointEval; interpolate; lagrangeAtPoint | planned; pyrival, aclpy, cpalg, lc |
| `_21_matrix.py` | matMul; matPow; matIdentity; matMulMod; matPowMod; gaussMod; rankMod; detMod (any modulus); solveMod; inverseMod; nullspaceMod; gaussFraction; detFraction; solveFraction; inverseFraction; solveFloat (partial pivoting, tolerance, residual); rankF2; detF2; solveF2; inverseF2; nullspaceF2 | planned; pyrival, cpalg, lc |
| `_22_bitset.py` | popcount; lowbit; highbit; submasks; supermasks; nextSamePopcount; bitsetFromList; bitsetToList; subsetSumBitset; lcsBitset; transitiveClosure; XorBasis: insert, contains, max, min, kth, size, witness, reduce, intersect | planned; pyrival, cpalg, lc |
| `_23_trie.py` | Trie: insert, erase, count, countPrefix, contains, longestPrefix, enumerate; BinaryTrie: insert, erase, count, maxXor, minXor, kth, countLess, xorAll | planned; pyrival, lc |
| `_24_aho.py` | AhoCorasick: addPattern, build, next, findAll, countOccurrences, terminalCounts, stateOf, dictLinks | planned; cpalg, lc |
| `_25_suffix.py` | suffixArray; suffixArrayInt; lcpArray; SuffixArrayIndex: find, count, lcp, compare; SuffixAutomaton: extend, build, contains, countOccurrences, distinctSubstrings, longestCommonSubstring, endposSizes; longestCommonSubstring (spans); numberOfSubstrings; longestRepeatedSubstring | planned; pyrival, aclpy, cpalg, lc, legacy: OLD/Team Notebook/src/misc/old_lcssubstr.py |
| `_26_offline.py` | moQueries; moWithUpdates; moOnTree; hilbertOrder; cdqPointAddRectangleSum; cdqDominanceCount; parallelBinarySearch; offlineRangeDistinct; offlineRectangleSum; offlineRangeInversions; offlineLcaTarjan | planned; cpalg, lc |
| `_27_randomized.py` | Rng: seed, randInt, randBelow, randReal, shuffle, choice, sample; randomBase; randomPrime; reservoirSample; weightedChoice; randomPermutation; randomTree | planned; pyrival, stdlib |
| `_28_io_advanced.py` | FastReader: nextInt, nextInts, nextToken, nextLine, remaining; FastWriter: write, writeInt, writeInts, writeLine, flush; readNumbersAll; tokenizeBytes | planned; pyrival, pypy |
| `_29_lazysegmenttree.py` | LazySegTree: build, set, get, prod, allProd, apply, applyRange, maxRight, minLeft; DualSegTree: apply, get; RangeAddRangeSum; RangeAddRangeMin; RangeAffineRangeSum; RangeAssignRangeSum (inlined variants) | planned; pyrival, aclpy, lc |
| `_30_dp_optimization.py` | divideAndConquerDp; knuthDp; monotoneQueueDp; boundedKnapsackMonotone; smawk; monotoneMinima; alienTrick; SlopeTrick: addAbs, addLeft, addRight, shift, prefixMin, min; minPlusConvolutionConvex; minPlusConvolutionConvexArbitrary; onlineOfflineDp | planned; cpalg, oiwiki, lc |
| `_31_ordered_multiset.py` | SortedList: add, remove, discard, pop, count, bisectLeft, bisectRight, kth, contains, lt, le, gt, ge, min, max, len, iter; SortedSet; Treap: insert, erase, kth, rank, split, merge, lowerBound, prev, next; WordSet (64-ary bit tree): insert, erase, contains, prev, next, min, max | planned; pyrival, lc |
| `_32_line_envelope.py` | ConvexHullTrick (monotone): add, query; ConvexHullTrickBinary: add, query; LiChao: addLine, addSegment, query; LiChaoDynamic: addLine, addSegment, query; compareIntersections | planned; pyrival, cpalg, lc |
| `_33_twosat.py` | TwoSat: addClause, addImplication, setTrue, setFalse, addXor, addEquivalent, addAtMostOne, solve, satisfiable, assignment | planned; pyrival, aclpy, lc |
| `_34_lowlink.py` | lowlink; bridges; articulationPoints; twoEdgeConnectedComponents; bridgeTree; biconnectedComponents; blockCutTree | planned; pyrival, cpalg, lc |
| `_35_eulertrail.py` | eulerTrailDirected; eulerTrailUndirected; eulerCircuitDirected; eulerCircuitUndirected; hasEulerTrail | planned; pyrival, cpalg, lc |
| `_36_shortest_paths.py` | bellmanFord; negativeCycleWitness; affectedByNegativeCycle; floydWarshall; floydPath; johnson; dijkstraDense; dialDijkstra; restorePathAllPairs | planned; pyrival, cpalg, lc |
| `_37_mincostflow.py` | MinCostFlow: addEdge, flow, slope, edges, potentials, initPotentials (Bellman–Ford for negative costs); minCostAssignment | planned; aclpy, cpalg, lc |
| `_38_number_theory_advanced.py` | crt; crtGeneral; garner; floorSum; enumerateQuotients; linearDiophantine; linearCongruence; minOfModLinear; duSieve (phi and mu prefix sums); lucas; binomialPrimePower; binomialAnyMod; bsgs; exBsgs; multiplicativeOrder; primitiveRoot; sqrtMod (Tonelli–Shanks); kthRootMod; discreteRoot; powerTower | planned; pyrival, aclpy, cpalg, lc, legacy: OLD/Team Notebook/src/math/old_crt.py |
| `_39_transform_algorithms.py` | zetaSubset; mobiusSubset; zetaSuperset; mobiusSuperset; walshHadamard; convolveOr; convolveAnd; convolveXor (exact integer and modular inverses); gcdTransform; gcdInverse; lcmTransform; lcmInverse; convolveGcd; convolveLcm; subsetConvolution | planned; pyrival, lc |
| `_40_state_search.py` | backtrack; branchAndBound; bfsStates; bidirectionalBfs; aStar (reopen policy); idaStar; iterativeDeepening; meetInTheMiddleSubsetSum; meetInTheMiddleEnumerate; alphaBeta | planned; pyrival, oiwiki |
| `_41_geometry_float.py` | addF; subF; scaleF; dotF; crossF; norm; angle; rotate; project; reflect; distPointLine; distPointSegment; lineIntersectionF; circleLineIntersection; circleCircleIntersection; circleTangents; circleFrom3; circumcenter; incenter; minimumEnclosingCircle (Welzl) | planned; pyrival, cpalg, lc |
| `_42_functionalgraph.py` | FunctionalGraph: build, cycles, tailLength, kthSuccessor, orbitLength, distance, entryPoint, inCycle, component; Doubling: build, jump, fold; floydCycle | planned; pyrival, cpalg |
| `_43_dp_advanced.py` | digitDp; digitDpRange; subsetDp; tspBitmask; hamiltonianPathCount; setCoverBitmask; dominoTiling (broken profile); profileDp; intervalDp; matrixChainOrder; optimalBst; reconstruct | planned; cpalg, oiwiki |
| `_53_eertree.py` | Eertree: add, build, len, link, count, occurrences, distinctPalindromes, longestSuffixPalindrome; enumeratePalindromes | planned; lc |
| `_54_lyndon.py` | lyndonFactorization; lyndonEnds; lyndonWords; deBruijn | planned; cpalg, lc |
| `_55_geometry_advanced.py` | convexDiameter (rotating calipers); farthestPair; closestPair; minkowskiSum; convexCut; halfPlaneIntersection; segmentUnionLength; rectangleUnionArea | planned; cpalg, lc |
| `_56_wavelet_matrix.py` | BitVector: rank1, rank0, select1; WaveletMatrix: access, rank, select, kth, countLess, countRange, prevValue, nextValue, rangeFreq | planned; lc |
| `_57_rational.py` | continuedFraction; convergents; bestRationalApprox; limitDenominator; sternBrocotEncode; sternBrocotDecode; sternBrocotLca; sternBrocotAncestor; sternBrocotRange; fareyNext | planned; cpalg, lc, stdlib |
| `_58_game_theory.py` | nimSum; mexOf; grundy; grundyTable; retrogradeSolve; nimGame; staircaseNim | planned; cpalg |
| `_59_graph_advanced.py` | dominatorTree; directedMst; maxClique; maxIndependentSet; chromaticNumber; enumerateTriangles; steinerTree (Dreyfus–Wagner); maximalCliques | planned; lc |
| `_60_sqrt_decomposition.py` | SqrtDecomp: build, pointUpdate, rangeApply, rangeQuery, rebuild; rangeMode; blockDistinct | planned; cpalg, lc |
| `_61_interval_map.py` | IntervalMap: assign, get, split, intervals, rangeApply, count, fold | planned; cpalg |

## Esoteric

| Module | Operations | Status |
|---|---|---|
| `_44_exact_geometry.py` | anySegmentsIntersect (sweep); segmentIntersectionsSweep (Bentley–Ottmann, Fraction); intersectionPointsExact | planned; cpalg |
| `_45_succinct.py` | RankSelect: rank1, rank0, select1, select0 (sampled); PackedArray: get, set | planned; cpalg |
| `_46_fmindex.py` | bwt; inverseBwt; FmIndex: build, count, locate, extract | planned; cpalg |
| `_47_algebraic.py` | berlekampMassey; linearRecurrenceKth (Bostan–Mori); linearRecurrenceKthKitamasa; linearRecurrenceTerms; rationalSeriesCoefficient | planned; pyrival, lc |
| `_48_combinatorial_species.py` | burnside; polyaNecklace; polyaBracelet; cycleIndexCyclic; cycleIndexDihedral; necklaceCountColors; evaluateCycleIndex | planned; cpalg |
| `_49_sat.py` | DancingLinks: addRow, solve, solutions; Dpll: addClause, solve | planned; oiwiki |
| `_50_approximation.py` | BloomFilter: add, mayContain; CountMinSketch: add, estimate; simpson; adaptiveSimpson; bisectionRoot; newtonRoot; secantRoot; bracketRoot | planned; pyrival, cpalg |

## Notes

- `_98_basic.py` and `_99_all.py` exist as aggregates: explicit re-exports of Basic and of all implemented modules; both currently export nothing and do no work on import.
- Every row mirrors a C++ owner (`02` data structures, `03` geometry, `04` graphs, `05` mathematics, `06` miscellaneous, `07` strings); the C++ inventory is the full contract, the Python row is the documented reduction.
- GF(2) elimination lives in `_21_matrix.py`; `_22_bitset.py` keeps bit tricks and `XorBasis`. Meet-in-the-middle is owned by `_40_state_search.py` only.
- `_44`, `_45`, `_46`, `_49` and `_50` are low-value on PyPy (big-int masking, pointer-heavy structures, exponential search); they stay Esoteric and are scheduled last.
- `_50` mixes sketches with numerical integration/roots; `_49` mixes SAT with exact cover. Both stay as listed until implementation splits them (see `80-notes.md`).
- Source keys: `pyrival`, `aclpy`, `lc`, `cpalg`, `oiwiki`, `pypy`, `stdlib` are defined in `81-sources.md`.
