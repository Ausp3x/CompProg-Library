# 08 Python — inventory

Scope: a deliberate PyPy-first contest subset of folders `01`–`07` plus the PyPy performance idioms a Python contestant needs (`04-python.md`). Native `int`, `Fraction`, `heapq`, `bisect`, `itertools`, `math` and `functools` replace Core numeric types and Core/Mini wrappers; nothing here is an automatic port of a C++ header. Full algorithm families, SIMD kernels and research string/geometry indexes stay in the C++ folders; a Python row exists only where a PyPy contestant plausibly runs it inside judge limits. Contracts, ownership and omissions are in [00-notes.md](Docs/00-notes.md), sources in [00-sources.md](Docs/00-sources.md).

## Basic

| Module | Operations | Status |
|---|---|---|
| `_01_io.py` | readAll; readInts; readInt; readLine; readTokens; readStr; readMatrix; readGraphEdges; writeLine; writeInts; writeAll; flushOut; FastIO: read, readline, write, flush; Interactive: ask, tell, finish | planned; PYRIVAL, PYPY, STDLIB |
| `_02_search.py` | lowerBound; upperBound; countRange; firstTrue; lastTrue; binarySearchInt; binarySearchReal; ternaryMinInt; ternaryMinReal; goldenSection; argminUnimodal; bisectKey (3.10 key= adapter) | planned; PYRIVAL, STDLIB |
| `_03_number_theory.py` | extgcd; modInv; modPow; floorDiv; ceilDiv; floorMod; divmodFloor; sieve; linearSieve; primeList; segmentedSieve; spfSieve; isqrt; iroot; isSquare; isPrimeTrial; divisorsTrial; phiOne; phiSieve; mobiusSieve; divisorCountSieve; divisorSumSieve; multiplicativeSieve | planned; PYRIVAL, CPA, STDLIB; legacy: OLD/Team Notebook/src/math/old_crt.py (inv) |
| `_04_combinatorics.py` | Comb: fact, invFact, C, P, H, catalan, inv; comb; perm; factorialTable; catalanNumber; ballot; derangements; fibonacci (fast doubling); fibPair; stirling1Table; stirling2Table; bellTable; partitionTable; eulerianTable; permutationRank; permutationUnrank; nextPermutation; bracketNext; bracketRank; bracketUnrank; josephus | planned; PYRIVAL, CPA, LC, STDLIB |
| `_05_sequences.py` | lisLength; lisWitness; lisNonStrict; lisCount; inversions; kadane; kadaneCircular; compress; prefixSums; differenceArray; prefix2d; difference2d; rangeSum2d; prevLess; nextLess; prevGreater; nextGreater; slidingMin; slidingMax; Swag: push, pop, allProd; DequeSwag: pushFront, pushBack, popFront, popBack, allProd; mex; kthSmallest; majority; cartesianTree; runLengthEncode; twoPointersWindow | planned; PYRIVAL, CPA, LC, STDLIB |
| `_06_dsu.py` | DSU: find, unite, same, size, count, groups, roots, reset | planned; PYRIVAL, ACLPY |
| `_07_fenwick.py` | Fenwick: build, add, prefix, sum, get, set, lowerBound, kth; RangeFenwick: add, sum, get; Fenwick2D: add, sum | planned; PYRIVAL, ACLPY |
| `_08_segmenttree.py` | SegmentTree: build, set, get, prod, allProd, fold, foldRight, maxRight, minLeft; SegmentTreeMin; SegmentTreeMax; SegmentTreeSum (inlined operation variants) | planned; PYRIVAL, ACLPY |
| `_09_graph.py` | buildAdj; buildCsr; bfs; bfsDist; bfs01; dfsIter; components; isBipartite; oddCycle; topoSort; cycleDirected; cycleUndirected; dijkstra; dijkstraPath; restorePath; kruskal; prim; scc; condensation; gridBfs | planned; PYRIVAL, ACLPY, CPA, LC |
| `_10_strings.py` | prefixFunction; kmpFind; kmpCount; periodOf; borders; zFunction; zFind; manacher; longestPalindrome; countPalindromes; minRotation; RollingHash: get, combine, lcp, equal; hashOf; doubleHash | planned; PYRIVAL, ACLPY, CPA |
| `_11_geometry.py` | cross; dot; orient; distSq; manhattan; rotate90; onSegment; segmentsIntersect; lineIntersection (Fraction); convexHull (strict); convexHullCollinear; polygonArea2; perimeter; pointInPolygon; pointInConvex; sortByAngle; latticePointsOnSegment; pickInterior | planned; PYRIVAL, CPA, LC |
| `_12_dp.py` | knapsack01; knapsack01Witness; knapsackUnbounded; knapsackBounded (binary splitting); knapsackCount; coinChangeMin; coinChangeWays; subsetSumReachable; lcsLength; lcsWitness; lcsWitnessLowMemory (Hirschberg); editDistance; editDistanceAlignment; gridPathCount; gridMinPathSum; rodCutting | planned; CPA, OI; legacy: OLD/Team Notebook/src/misc/old_lcs.py |
| `_13_sparsetable.py` | SparseTable: prod; SparseTableMin: prod, argmin; SparseTableGcd: prod; BlockRmq: prod; DisjointSparseTable (non-idempotent): prod | planned; PYRIVAL, CPA |
| `_51_pypy_idioms.py` | bootstrap (generator trampoline); runWithStack; packPair; unpackPair; packTriple; shieldKey; ShieldedDict; ShieldedSet; modMul64; bucketSort; ordersort; ordersortLong; multikeyOrdersort; isPyPy; localsBind | planned; PYRIVAL, PYPY |
| `_52_heaps.py` | RemovableHeap: push, pop, top, remove, size, empty; MaxHeap: push, pop, top; RunningMedian: add, remove, median, size; DoubleEndedHeap: push, popMin, popMax, min, max, size; KeyHeap: push, pop, decrease; heapMerge; nSmallest; nLargest | planned; PYRIVAL, LC, STDLIB |

## Advanced

| Module | Operations | Status |
|---|---|---|
| `_14_prime_factor.py` | isPrime; isProbablePrime; millerRabin; pollardRhoBrent; factorize; primeFactors; divisors; divisorCount; divisorSum; phiFromFactors; isPrimePower; isSquareFree; countPrimes (Lucy); primePi | planned; PYRIVAL, CPA, LC |
| `_15_flow.py` | Dinic: addEdge, flow, minCut, edges, edge, changeEdge, reset; BoundedFlow: addEdge, feasible, flow, edges; projectSelection: addItem, addDependency, solve | planned; PYRIVAL, ACLPY, CPA, LC |
| `_16_matching.py` | hopcroftKarp; minVertexCover; maxIndependentSetBipartite; kuhn; hungarian (rectangular, duals); assignmentMin; blossomMatching (unweighted general) | planned; PYRIVAL, CPA, LC |
| `_17_tree.py` | rootTree; subtreeSizes; eulerTour; LcaBinaryLift: lca, kthAncestor, dist, jump, pathFold; LcaEuler: lca; Hld: head, pos, lca, pathRanges, subtreeRange, kthOnPath; virtualTree; diameter; center; centroid; CentroidDecomposition: build, parent, order; reroot; treeIsomorphismClasses; pruferEncode; pruferDecode | planned; PYRIVAL, CPA, LC |
| `_18_rollback.py` | RollbackDsu: find, unite, same, size, snapshot, rollback, undo; WeightedDsu: find, unite, diff, same; ParityDsu: find, unite, parity, same; offlineDynamicConnectivity | planned; PYRIVAL, LC |
| `_19_persistent.py` | PersistentSegmentTree: build, set, get, prod, versions; PersistentCountTree: build, kth, countLess, countRange; rangeKthSmallest | planned; PYRIVAL, LC |
| `_20_polynomial.py` | convolve; convolveNtt; convolveMod (three primes); convolveInt (Kronecker big-int); convolveFft; ntt; intt; polyAdd; polySub; polyMul; polyDivmod; polyEval; polyDerivative; polyIntegral; polyInv; polyLog; polyExp; polyPow; polySqrt; taylorShift; multipointEval; interpolate; lagrangeAtPoint | planned; PYRIVAL, ACLPY, CPA, LC |
| `_21_matrix.py` | matMul; matPow; matIdentity; matMulMod; matPowMod; gaussMod; rankMod; detMod (any modulus); solveMod; inverseMod; nullspaceMod; gaussFraction; detFraction; solveFraction; inverseFraction; solveFloat; rankF2; detF2; solveF2; inverseF2; nullspaceF2 | planned; PYRIVAL, CPA, LC |
| `_22_bitset.py` | popcount; lowbit; highbit; submasks; supermasks; nextSamePopcount; bitsetFromList; bitsetToList; subsetSumBitset; lcsBitset; transitiveClosure; XorBasis: insert, contains, max, min, kth, size, witness, reduce, intersect | planned; PYRIVAL, CPA, LC |
| `_23_trie.py` | Trie: insert, erase, count, countPrefix, contains, longestPrefix, enumerate; BinaryTrie: insert, erase, count, maxXor, minXor, kth, countLess, xorAll | planned; PYRIVAL, LC |
| `_24_aho.py` | AhoCorasick: addPattern, build, next, findAll, countOccurrences, terminalCounts, stateOf, dictLinks | planned; CPA, LC |
| `_25_suffix.py` | suffixArray; suffixArrayInt; lcpArray; SuffixArrayIndex: find, count, lcp, compare; SuffixAutomaton: extend, build, contains, countOccurrences, distinctSubstrings, longestCommonSubstring, endposSizes; longestCommonSubstring (spans); numberOfSubstrings; longestRepeatedSubstring | planned; PYRIVAL, ACLPY, CPA, LC; legacy: OLD/Team Notebook/src/misc/old_lcssubstr.py |
| `_26_offline.py` | moQueries; moWithUpdates; moOnTree; hilbertOrder; cdqPointAddRectangleSum; cdqDominanceCount; parallelBinarySearch; offlineRangeDistinct; offlineRectangleSum; offlineRangeInversions; offlineLcaTarjan | planned; CPA, LC |
| `_27_randomized.py` | Rng: seed, randInt, randBelow, randReal, shuffle, choice, sample; randomBase; randomPrime; reservoirSample; weightedChoice; randomPermutation; randomTree | planned; PYRIVAL, STDLIB |
| `_28_io_advanced.py` | FastReader: nextInt, nextInts, nextToken, nextLine, remaining; FastWriter: write, writeInt, writeInts, writeLine, flush; readNumbersAll; tokenizeBytes | planned; PYRIVAL, PYPY |
| `_29_lazysegmenttree.py` | LazySegmentTree: build, set, get, prod, allProd, fold, foldRight, apply (range), maxRight, minLeft; DualSegmentTree: apply (range), get; RangeAddRangeSum; RangeAddRangeMin; RangeAffineRangeSum; RangeAssignRangeSum (inlined variants) | planned; PYRIVAL, ACLPY, LC |
| `_30_dp_optimization.py` | divideAndConquerDp; knuthDp; monotoneQueueDp; boundedKnapsackMonotone; smawk; monotoneMinima; alienTrick; SlopeTrick: addAbs, addLeft, addRight, shift, prefixMin, min; minPlusConvolutionConvex; minPlusConvolutionConvexArbitrary; onlineOfflineDp | planned; CPA, OI, LC |
| `_31_ordered_multiset.py` | SortedList: add, remove, discard, pop, count, bisectLeft, bisectRight, kth, contains, lt, le, gt, ge, min, max, len, iter; SortedSet; Treap: insert, erase, kth, rank, split, merge, lowerBound, prev, next; WordSet: insert, erase, contains, prev, next, min, max | planned; PYRIVAL, LC |
| `_32_line_envelope.py` | ConvexHullTrick (monotone): add, query; ConvexHullTrickBinary: add, query; LiChao: addLine, addSegment, query; LiChaoDynamic: addLine, addSegment, query; compareIntersections | planned; PYRIVAL, CPA, LC |
| `_33_twosat.py` | TwoSat: addClause, addImplication, setTrue, setFalse, addXor, addEquivalent, addAtMostOne, satisfiable, assignment | planned; PYRIVAL, ACLPY, LC |
| `_34_lowlink.py` | lowlink; bridges; articulationPoints; twoEdgeConnectedComponents; bridgeTree; biconnectedComponents; blockCutTree | planned; PYRIVAL, CPA, LC |
| `_35_eulertrail.py` | eulerTrailDirected; eulerTrailUndirected; eulerCircuitDirected; eulerCircuitUndirected; hasEulerTrail | planned; PYRIVAL, CPA, LC |
| `_36_shortest_paths.py` | bellmanFord; negativeCycleWitness; affectedByNegativeCycle; floydWarshall; floydPath; johnson; dijkstraDense; dialDijkstra; restorePathAllPairs | planned; PYRIVAL, CPA, LC |
| `_37_mincostflow.py` | MinCostFlow: addEdge, flow, slope, edges, potentials, initPotentials; minCostAssignment | planned; ACLPY, CPA, LC |
| `_38_number_theory_advanced.py` | crt; crtGeneral; garner; floorSum; enumerateQuotients; linearDiophantine; linearCongruence; minOfModLinear; duSieve (phi and mu prefix sums); lucas; binomialPrimePower; binomialAnyMod; bsgs; exBsgs; multiplicativeOrder; primitiveRoot; sqrtMod (Tonelli–Shanks); kthRootMod; discreteRoot; powerTower | planned; PYRIVAL, ACLPY, CPA, LC; legacy: OLD/Team Notebook/src/math/old_crt.py |
| `_39_transform_algorithms.py` | zetaSubset; mobiusSubset; zetaSuperset; mobiusSuperset; walshHadamard; convolveOr; convolveAnd; convolveXor; gcdTransform; gcdInverse; lcmTransform; lcmInverse; convolveGcd; convolveLcm; subsetConvolution | planned; PYRIVAL, LC |
| `_40_state_search.py` | backtrack; branchAndBound; bfsStates; bidirectionalBfs; aStar; idaStar; iterativeDeepening; meetInTheMiddleSubsetSum; meetInTheMiddleEnumerate; alphaBeta | planned; PYRIVAL, OI |
| `_41_geometry_float.py` | addF; subF; scaleF; dotF; crossF; norm; angle; rotate; project; reflect; distPointLine; distPointSegment; lineIntersectionF; circleLineIntersection; circleCircleIntersection; circleTangents; circleFrom3; circumcenter; incenter; minimumEnclosingCircle (Welzl) | planned; PYRIVAL, CPA, LC |
| `_42_functionalgraph.py` | FunctionalGraph: build, cycles, tailLength, kthSuccessor, orbitLength, distance, entryPoint, inCycle, component; Doubling: build, jump, fold; floydCycle | planned; PYRIVAL, CPA |
| `_43_dp_advanced.py` | digitDp; digitDpRange; subsetDp; tspBitmask; hamiltonianPathCount; setCoverBitmask; dominoTiling (broken profile); profileDp; intervalDp; matrixChainOrder; optimalBst; reconstruct | planned; CPA, OI |
| `_53_eertree.py` | Eertree: add, build, len, link, count, occurrences, distinctPalindromes, longestSuffixPalindrome; enumeratePalindromes | planned; LC |
| `_54_lyndon.py` | lyndonFactorization; lyndonEnds; lyndonWords; deBruijn | planned; CPA, LC |
| `_55_geometry_advanced.py` | convexDiameter (rotating calipers); farthestPair; closestPair; minkowskiSum; convexCut; halfPlaneIntersection; segmentUnionLength; rectangleUnionArea | planned; CPA, LC |
| `_56_wavelet_matrix.py` | BitVector: rank1, rank0, select1; WaveletMatrix: access, rank, select, kth, countLess, countRange, prevValue, nextValue, rangeFreq | planned; LC |
| `_57_rational.py` | continuedFraction; convergents; bestRationalApprox; limitDenominator; sternBrocotEncode; sternBrocotDecode; sternBrocotLca; sternBrocotAncestor; sternBrocotRange; fareyNext | planned; CPA, LC, STDLIB |
| `_58_game_theory.py` | nimSum; mexOf; grundy; grundyTable; retrogradeSolve; nimGame; staircaseNim | planned; CPA |
| `_59_graph_advanced.py` | dominatorTree; directedMst; maxClique; maxIndependentSet; chromaticNumber; enumerateTriangles; steinerTree (Dreyfus–Wagner); maximalCliques | planned; LC |
| `_60_sqrt_decomposition.py` | SqrtDecomp: build, pointUpdate, rangeApply, prod, rebuild; rangeMode; blockDistinct | planned; CPA, LC |
| `_61_interval_map.py` | IntervalMap: assign, get, split, intervals, rangeApply, count, fold | planned; CPA |

## Esoteric

| Module | Operations | Status |
|---|---|---|
| `_44_exact_geometry.py` | anySegmentsIntersect (sweep); segmentIntersectionsSweep (Bentley–Ottmann, Fraction); intersectionPointsExact | planned; CPA |
| `_45_succinct.py` | RankSelect: rank1, rank0, select1, select0; PackedArray: get, set | planned; CPA |
| `_46_fmindex.py` | bwt; inverseBwt; FmIndex: build, count, locate, extract | planned; CPA |
| `_47_algebraic.py` | berlekampMassey; linearRecurrenceKth (Bostan–Mori); linearRecurrenceKthKitamasa; linearRecurrenceTerms; rationalSeriesCoefficient | planned; PYRIVAL, LC |
| `_48_combinatorial_species.py` | burnside; polyaNecklace; polyaBracelet; cycleIndexCyclic; cycleIndexDihedral; necklaceCountColors; evaluateCycleIndex | planned; CPA |
| `_49_sat.py` | DancingLinks: addRow, solve, solutions; Dpll: addClause, solve | planned; OI |
| `_50_approximation.py` | BloomFilter: add, mayContain; CountMinSketch: add, estimate; simpson; adaptiveSimpson; bisectionRoot; newtonRoot; secantRoot; bracketRoot | planned; PYRIVAL, CPA |
