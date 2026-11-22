# 08 Python — Inventory

Scope: a deliberate PyPy-first subset of folders `01`–`07`, plus the PyPy performance idioms a Python contestant needs (`04-python.md`). No module is an automatic port of a C++ header. Native `int` and `Fraction` replace the Core numeric types, and the standard library (`heapq`, `bisect`, `itertools`, `math`, `functools`, `random`) counts as existing operations, so no row re-wraps it. A row exists only where a PyPy contestant plausibly runs it inside judge limits; SIMD kernels, whole algorithm families and research string and geometry indexes stay in the C++ folders. Contracts, ownership and omissions are in [00-notes.md](Docs/00-notes.md), sources in [00-sources.md](Docs/00-sources.md).

## Basic

| Module | Operations | Status |
|---|---|---|
| `_01_io.py` | readToken; readLine; readInt; read (n values or rows; ints or tokens); readNumbersAll (whole-input integer scan); writeLine; writeValue (sequences); flush; FastIO: read, readline, write, flush; Interactive: ask, writeLine, answer | planned; PYRIVAL, PYPY, STDLIB |
| `_02_search.py` | firstTrue; lastTrue; binSearch; binSearchRealBracket; ternSearch; ternSearchRealBracket; goldenSearchRealBracket; fibSearch | planned; PYRIVAL, STDLIB |
| `_03_number_theory.py` | extendedGcd; inverseXgcd; ceilDiv; SieveOfErath: is_prime, prms; LinearSieve: prms, spf; SegmentedSieve: is_prime, prms; ikthRoot; isPerfectSquare; isPrimeTrial; divisorsTrial; phiTrial; phiTable; mobiusTable; divisorCountTable; divisorSumTable; multiplicativeTable | planned; PYRIVAL, CPA, STDLIB; legacy: OLD/Team Notebook/src/math/old_crt.py (inv) |
| `_04_combinatorics.py` | PrimeCombinatorics: factorial, inverseFactorial, combiNR (binomial), permuNR, combiWR, catalan, inv; factorialTable; catalanExact; ballotExact; derangementTable; fibonacciPair (fast doubling); stirling1Table; stirling2Table; bellTable; partitionTable; eulerianTable; permutationRank; permutationUnrank; nextPermutation; nextBalanced; rankBalanced; unrankBalanced; josephus | planned; PYRIVAL, CPA, LC, STDLIB |
| `_05_sequences.py` | longestIncreasingSubsequence (LIS; strict/non-strict, witness); countLongestIncreasingSubsequences; inversionCount; maximumSubarray; circularMaximumSubarray; compress; PrefixSum: prefixSum, sum; DifferenceArray: add, values; PrefixSum2D: prefixSum, sum; DifferenceArray2D: add, values; previousSmaller; nextSmaller; previousGreater; nextGreater; slidingMinimum; slidingMaximum; SWAGQueue: push, pop, allProd; SWAGDeque: pushFront, pushBack, popFront, popBack, allProd; mex; kthSmallest; majorityElement; cartesianTree; runLengthEncode; monotoneWindowEnds | planned; PYRIVAL, CPA, LC, STDLIB |
| `_06_dsu.py` | DSU: findSet, uniteSets, isSameSet, size, count, groups, leaders, reset | planned; PYRIVAL, ACLPY |
| `_07_fenwick.py` | Fenwick: build, add, prefixSum, sum, get, set, lowerBound, kth; FenwickRangeAdd: add, sum, get; Fenwick2D: add, sum | planned; PYRIVAL, ACLPY |
| `_08_segmenttree.py` | SegmentTree: build, set, get, prod, allProd, fold, foldRight, maxRight, minLeft; SegmentTreeMin; SegmentTreeMax; SegmentTreeSum (inlined operation variants) | planned; PYRIVAL, ACLPY |
| `_09_graph.py` | buildAdj; buildCsr; bfs; bfsShortestPaths; zeroOneBfs; dfs; connectedComponents; bipartiteCheck; topologicalSort; cycleDirected; cycleUndirected; dijkstra; dijkstraWitness; restorePath; kruskal; primSparse; tarjanScc; condensationDag; gridBfs | planned; PYRIVAL, ACLPY, CPA, LC |
| `_10_strings.py` | prefixFunction; kmpOccurrences; countOccurrences; prefixPeriod; prefixBorders; zFunction; zOccurrences; manacher; longestPalindrome; countPalindromes; minRotation; StringHash: get, concat, lcp, equal; hashOf; doubleHash; rabinKarp (StringHash.get) | planned; PYRIVAL, ACLPY, CPA |
| `_11_geometry.py` | cross; dot; orient; dist2; manhattan; perp; contains (point on segment); intersect (segments); lineIntersection (Fraction); convexHull (strict, keep_collinear); signedArea2; polygonPerimeter; polygonContains; ConvexPolygonQuery: contains; argumentSort; latticeOnSegment; latticeInterior | planned; PYRIVAL, CPA, LC |
| `_12_dp.py` | knapsack01; knapsack01Witness; knapsackUnbounded; knapsackBounded (binary splitting); knapsackCount; minimumCoins; coinChangeWays; knapsackFeasible; lcs; lcsWitness; hirschbergLcs (Hirschberg); levenshtein; levenshteinWitness; gridPaths; gridMinPathSum; rodCutting | planned; CPA, OI; legacy: OLD/Team Notebook/src/misc/old_lcs.py |
| `_13_sparsetable.py` | SparseTable: prod; SparseTableMin: prod, argmin; SparseTableGcd: prod; LinearRMQ: query (leftmost argmin); DisjointSparseTable (non-idempotent): prod | planned; PYRIVAL, CPA |
| `_51_pypy_idioms.py` | bootstrap (generator trampoline); runWithStack; packPair; unpackPair; packTriple; shieldKey; ShieldedDict; ShieldedSet; modMul64; bucketSort; ordersort; ordersortLong; multikeyOrdersort; isPyPy; localsBind | planned; PYRIVAL, PYPY |
| `_52_heaps.py` | ErasableHeap: push, pop, top, erase, size, empty; MaxHeap: push, pop, top; RunningMedian: add, remove, median, size; DoubleEndedPriorityQueue: push, popMin, popMax, min, max, size; BinaryHeap: push, pop, decreaseKey | planned; PYRIVAL, LC, STDLIB |

## Advanced

| Module | Operations | Status |
|---|---|---|
| `_14_prime_factor.py` | isPrime; isProbablePrime; millerRabin; pollardRhoBrent; factorize; divisors; divisorCount; divisorSum; phiFromFactors; isPrimePower; isSquarefree; primeCount (Lucy) | planned; PYRIVAL, CPA, LC |
| `_15_flow.py` | MaxFlow: addEdge, flow (Dinic), minCut, edges, edge, changeEdge, reset; feasibleFlow (lower bounds); maxFlowWithLowerBounds; maximumWeightClosure (project selection) | planned; PYRIVAL, ACLPY, CPA, LC |
| `_16_matching.py` | BipartiteMatching: hopcroftKarp, kuhn, konigVertexCover, maximumIndependentSet; hungarian (rectangular, duals); edmondsBlossom (unweighted general) | planned; PYRIVAL, CPA, LC |
| `_17_tree.py` | rootTree; subtreeSizes; EulerTour: tin, tout; LCA: lca, kthAncestor, distance, jump, pathFold; EulerLCA: lca; HLD: head, pos, lca, processPath, subtreeRange, jump; VirtualTree: build; treeDiameter; treeEccentricities; treeCentroid; CentroidDecomposition: build, parent, order; Rerooting: get; rootedTreeClassification; pruferEncode; pruferDecode | planned; PYRIVAL, CPA, LC |
| `_18_rollback.py` | RollbackDSU: findSet, uniteSets, isSameSet, size, snapshot, rollback, undo; WeightedDSU: findSet, uniteSets, diff, isSameSet; ParityDSU: uniteSets, parity; OfflineDynamicConnectivity: addEdge (interval), insertEdge, eraseEdge, query, run | planned; PYRIVAL, LC |
| `_19_persistent.py` | PersistentSegmentTree: build, set, get, prod, versionCount; PersistentValueTree: build, rangeKth, countLess, countRange | planned; PYRIVAL, LC |
| `_20_polynomial.py` | convolution; convolutionNtt; convolutionArbitraryMod (three primes); convolutionExact (Kronecker big-int); convolutionFft; ntt; intt; polyAdd; polySub; polyMul; polyDivmod; polyEval; polyDerivative; polyIntegral; fpsInverse; fpsLog; fpsExp; fpsPow; fpsSqrt; taylorShift; multipointEvaluate; interpolate; lagrangeEvaluate | planned; PYRIVAL, ACLPY, CPA, LC |
| `_21_matrix.py` | matrixProduct; matrixPower; identityMatrix; matrixProductMod; matrixPowerMod; rrefMod; matrixRankMod; determinantMod (any modulus); solveLinearMod; inverseMod; nullspaceMod; rrefExact; determinantExact; solveLinearExact; inverseExact; solveLinearApprox; BitMatrix (GF(2)): rnk, det, solveLin, inv, ker | planned; PYRIVAL, CPA, LC |
| `_22_bitset.py` | lowBit; forEachSubmask; forEachSupermask; nextCombination; bitsetFromList; bitsetToList; bitsetSubsetSum; bitsetLcs; transitiveClosure; XorBasis: insert, contains, max, min, kth, size, represent, reduce, intersection | planned; PYRIVAL, CPA, LC |
| `_23_trie.py` | Trie: insert, erase, count, countPrefix, search, longestPrefix, forEach; BinaryTrie: insert, erase, count, xorMax, xorMin, kth, countLess, applyXor | planned; PYRIVAL, LC |
| `_24_aho.py` | AhoCorasick: add, build, step, forEachMatch, countPatterns, stateCounts, stateOf, dictLinks | planned; CPA, LC |
| `_25_suffix.py` | suffixArray; lcpArray; SuffixArray: patternRange, count, lce, compare; SuffixAutomaton: extend, build, isSubstring, occurrenceCount, distinctSubstrings, longestCommonSubstring, endposSize; longestCommonSubstring (spans); longestRepeated | planned; PYRIVAL, ACLPY, CPA, LC; legacy: OLD/Team Notebook/src/misc/old_lcssubstr.py |
| `_26_offline.py` | Mo: addQuery, solve; MoUpdates: addQuery, addUpdate, solve; TreeMo: addPathQuery, addUpdate, solve; hilbertOrder; cdqPointAddRectangleSum; dominanceCount3D; parallelBinarySearch; staticRangeCountDistinct; rectangleSumOffline; staticRangeInversions; lcaOffline | planned; CPA, LC |
| `_27_randomized.py` | randomBase; randomPrime; reservoirSample; randomPermutation; randomTree | planned; PYRIVAL, STDLIB |
| `_29_lazysegmenttree.py` | LazySegmentTree: build, set, get, prod, allProd, fold, foldRight, apply (range), maxRight, minLeft; DualSegmentTree: apply (range), get; RangeAddRangeSum; RangeAddRangeMin; RangeAffineRangeSum; RangeAssignRangeSum (inlined variants) | planned; PYRIVAL, ACLPY, LC |
| `_30_dp_optimization.py` | divideConquerDp; knuthDp; monotoneQueueDp; boundedKnapsackMonotoneQueue; smawk; monotoneMinima; aliensTrick; SlopeTrick: addAbs, addAMinusX, addXMinusA, shift, prefixMin, min; minPlusConvolutionConvexConvex; minPlusConvolutionConvexArbitrary; onlineOfflineDp | planned; CPA, OI, LC |
| `_31_ordered_multiset.py` | SortedList: insert, erase, eraseOne, eraseKth, count, contains, rank, upperRank, kth, lowerBound, upperBound, prev, front, back, size; SortedSet; Treap: insert, erase, kth, rank, split, merge, lowerBound, prev, next; FastSet: insert, erase, contains, prev, next, min, max | planned; PYRIVAL, LC |
| `_32_line_envelope.py` | MonotoneCHT: addLine, query (binary search), queryMonotone (pointer); LiChaoTree (nodes on demand): addLine, addSegment, query; compareIntersections | planned; PYRIVAL, CPA, LC |
| `_33_twosat.py` | TwoSat: addClause, addImplication, setValue, addXor, addEquivalent, addAtMostOne, satisfiable, answer | planned; PYRIVAL, ACLPY, LC |
| `_34_lowlink.py` | lowlink; bridges; articulationPoints; twoEdgeConnectedComponents; bridgeTree; biconnectedComponents; blockCutTree | planned; PYRIVAL, CPA, LC |
| `_35_eulertrail.py` | eulerianTrail (directed, undirected; circuit when one exists) | planned; PYRIVAL, CPA, LC |
| `_36_shortest_paths.py` | bellmanFord; findNegativeCycle; affectedByNegativeCycle; floydWarshall; johnson; dijkstraDense; dialDijkstra; restorePathAllPairs | planned; PYRIVAL, CPA, LC |
| `_37_mincostflow.py` | MinCostFlow: addEdge, flow, slope, edges, potentials, initPotentials; linearSumAssignment | planned; ACLPY, CPA, LC |
| `_38_number_theory_advanced.py` | crt (noncoprime moduli); garner; floorSum; enumerateQuotients; solveDioEq; solveModEq; minOfModOfLinear; dujiaoSieve (phi and mu prefix sums); binomialLucas; BinomialPrimePower: ctor, operator() (Granville), unitFactorial; BinomialCompositeMod: operator() (CRT over prime powers); discreteLogEx (extended BSGS); multiplicativeOrder; primitiveRoot; sqrtModPrimeCipolla; kthRootModPrime; powerTowerMod | planned; PYRIVAL, ACLPY, CPA, LC; legacy: OLD/Team Notebook/src/math/old_crt.py |
| `_39_transform_algorithms.py` | subsetZeta; subsetMobius; supersetZeta; supersetMobius; walshHadamard; orConvolution; andConvolution; xorConvolution; multipleZeta; multipleMobius; divisorZeta; divisorMobius; gcdConvolution; lcmConvolution; subsetConvolution | planned; PYRIVAL, LC |
| `_40_state_search.py` | backtrack; branchAndBound; implicitBfs; bidirectionalBfs; aStar; idaStar; iterativeDeepening; subsetSumMitm; splitEnumerate; alphaBeta | planned; PYRIVAL, OI |
| `_41_geometry_float.py` | addApprox; subApprox; scaleApprox; angleApprox; rotateApprox; projectLineApprox; reflectLineApprox; lineDistanceApprox; pointSegmentDistanceApprox; lineIntersectionApprox; circleLineIntersectionApprox; circleIntersectionApprox; commonTangentsApprox; circumcircleApprox; incircleApprox; minimumEnclosingCircleApprox (Welzl) | planned; PYRIVAL, CPA, LC |
| `_42_functionalgraph.py` | FunctionalGraph: build, cycles, depth, jump, orbitLength, distance, entry, inCycle, component; FunctionalGraphFold: build, fold; floydCycle | planned; PYRIVAL, CPA |
| `_43_dp_advanced.py` | DigitDP: count (closed interval, base); subsetDp; travelingSalesman; countHamiltonianPaths; setCoverBitmask; brokenProfileTiling (broken profile); brokenProfileDP; IntervalDP: solve, restore; matrixChainOrder; optimalBinarySearchTree | planned; CPA, OI |
| `_53_eertree.py` | PalindromicTree: append, build, len, link, count, occurrenceCounts, distinctPalindromes, longestSuffixPalindrome; enumeratePalindromes | planned; LC |
| `_54_lyndon.py` | duval; lyndonEnds; lyndonWords; deBruijnSequence | planned; CPA, LC |
| `_55_geometry_advanced.py` | ConvexCalipers: diameter (rotating calipers); farthestPair; closestPair; minkowskiSum; polygonCutApprox; halfplaneIntersection; segmentUnionLength; rectangleUnionArea | planned; CPA, LC |
| `_56_wavelet_matrix.py` | WaveletMatrix: access, rank, select, kth, countLess, countRange, maxLeq, minGeq | planned; LC |
| `_57_rational.py` | continuedFraction; convergents; bestRationalApproximation; sternBrocotEncode; sternBrocotDecode; sternBrocotLca; sternBrocotAncestor; sternBrocotRange; fareyNext | planned; CPA, LC, STDLIB |
| `_58_game_theory.py` | nimSum; grundy; grundyTable; retrogradeAnalysis; nimWin; staircaseNim | planned; CPA |
| `_59_graph_advanced.py` | dominatorTree; DirectedMst: arborescence; maximumClique; maximumIndependentSet; chromaticNumber; enumerateTriangles; steinerTree (Dreyfus–Wagner); bronKerbosch | planned; LC |
| `_60_sqrt_decomposition.py` | SqrtDecomp: build, set, apply, prod, rebuild; RangeMode: build, mode; blockDistinct | planned; CPA, LC |
| `_61_interval_map.py` | IntervalMap: assign, get, split, enumerate, apply, count, fold | planned; CPA |

## Esoteric

| Module | Operations | Status |
|---|---|---|
| `_44_exact_geometry.py` | anyIntersection (sweep); reportIntersections (Bentley–Ottmann, Fraction); intersectionPoints | planned; CPA |
| `_45_succinct.py` | SuccinctBitVector: rank1, rank0, select1, select0; PackedArray: get, set | planned; CPA |
| `_46_fmindex.py` | bwt; inverseBwt; FmIndex: build, count, locate, extract | planned; CPA |
| `_47_algebraic.py` | berlekampMassey; nthTermLinearRecurrence (Bostan–Mori); kitamasa; consecutiveTerms; bostanMori | planned; PYRIVAL, LC |
| `_48_combinatorial_species.py` | burnsideCount; necklaceCount; braceletCount; polyaCycleIndex (cyclic, dihedral); polyaEnumeration | planned; CPA |
| `_49_sat.py` | SatSolver: addClause, solve | planned; OI |
| `_62_exactcover.py` | DancingLinks: addRow, solve, forEachSolution | planned; OI |
| `_50_approximation.py` | simpsonFixed; simpsonAdaptive; bisectionRoot; newtonRoot; secantRoot; bracketRoot | planned; PYRIVAL, CPA |
| `_63_probabilistic_sketches.py` | BloomFilter: insert, mayContain; CountMinSketch: add, estimate | planned; PYRIVAL, CPA |
