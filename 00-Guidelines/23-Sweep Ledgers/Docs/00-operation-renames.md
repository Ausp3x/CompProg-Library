# Operation-name renames (P276, SUP27)

Evidence of the scoped renames that bring existing names in line with `02-structure.md` (Operation names). User decisions of 2026-11-10 are in `10-decisions.md` (2026-11-10 P276 operation renames).

## Scope

- **Inputs.** The brief's list, the P254 note (variant word first, `phiTrial`, `primitiveRoot`) and the P275 handoff (`Docs/00-duplicate-rereview.md`, Limits and handoffs).
- **Own scan.** Every Operations cell of the `01`–`08` inventories, checked for a `get` prefix, a trailing `Real`, `Brute`, `Slow` or `Fast`, a variant word placed first, `Int`/`Arithmetic` suffixes and `Trial`. The scan script lived in the session scratchpad.
- **Per rename.** Each one updated, together:
  - the row and the code;
  - the testers and benchmarks;
  - the call sites;
  - the folder notes and source ledgers;
  - the evidence documents, with a `Legacy mapping` entry and a `Renamed aliases (P276, …)` line so that `grab --find` still reaches the row by its old name;
  - the pending `23-Sweep Ledgers/duplicates.md` rows;
  - the notes of open packages.
- **Not touched.** No judge source, notebook selection or `08-Python` module used a renamed name. History lines and lines that cite `OLD` names keep the name they record.

## Renames in code

| Old | New | Headers (call sites) |
|---|---|---|
| `getSize` | `size` | DS 01, 09, 10, 29, 65 (Graphs 27; `13-grab_tester.py`); DS row 52 planned |
| `getLCA` | `lca` | Graphs 07 (`LCA`, `EulerLCA`; `SparseTableLCA` planned), 15 (14, 27) |
| `getKthAncestor` | `kthAncestor` (merged) | Graphs 07; the `int` overload only clamped negative `k`, so the two names could not coexist. Negative `k` now asserts. The user decided this on 2026-11-10, overriding the P011 keep in `duplicates.md` |
| `offlineLCA` | `lcaOffline` | Graphs 07 |
| `weightedDistance` | `distanceWeighted` | Graphs 07 (27) |
| `getEdge` | `edge` | Graphs 19, 20 (21, 30) |
| `weightedClustering`, `weightedDirectedClustering` | `localClusteringWeighted`, `directedClusteringWeighted` | Graphs 35 |
| `LinearSieve::getLpf`, `getNumDiv`, `getSumDiv`, `getPhi`, `getMu` | `lpfTable`, `divisorCountTable`, `divisorSumTable`, `phiTable`, `mobiusTable` | Math 04 (20). The builders fill the fields `lpf`, `num_div`, `phi`, …, so a bare `phi` would clash with its field; `Table` is the vocabulary word |
| `LinearSieve::getPrimeFac`, `getAllFac` | `factorize`, `divisors` | Math 04, Mathematics row 07's words |
| `SegmentedSieve::getPrimeFac`, `getNumDiv`, `getSumDiv`, `getPhi`, `getMu`, `getTables` | `factorize`, `divisorCountTable`, `divisorSumTable`, `phiTable`, `mobiusTable`, `tables` | Math 06 |
| `solveLinearReal`, `matrixRankReal`, `determinantReal` | `solveLinearApprox`, `matrixRankApprox`, `determinantApprox` | Math 16 |
| `FastConv::fctGcdSlow`, `fctLcmSlow` | `fctGcdSimple`, `fctLcmSimple` | Math 24 (lighter harmonic loops) |
| `latticeInDiskFast` | `latticeInDiskHull` | Geometry 19 (convex-curve hull) |
| `offlineRangeCount` | `rangeCountOffline` | Misc 10 |
| `fpsSparseInverse`, `fpsSparseLog`, `fpsSparseExp`, `fpsSparsePow`, `fpsSparseSqrt` | `fpsInverseSparse`, `fpsLogSparse`, `fpsExpSparse`, `fpsPowSparse`, `fpsSqrtSparse` | Math 14 (found by the review) |

## Renames in rows only

- **Geometry.**
  - `reportIntersectionsBrute` → `reportIntersectionsNaive`;
  - `triangulateSimple` → `triangulatePolygon`;
  - `euclideanMstDense` → `euclideanMstQuadratic`;
  - `frechetMatching` → `continuousFrechetWitness`;
  - `offlinePointLocation` → `pointLocationOffline`;
  - the four Geometry 60 names → `rectangleCountOfflineWordRam`, `boxCountOfflineWordRam`, `pointLocationOfflineWordRam`, `nearestNeighbourOfflineWordRam`;
  - `weightedMinimumEnclosingCircle` → `minimumEnclosingCircleWeighted`;
  - `weightedAlphaShape2`/`3` → `alphaShape2Weighted`/`alphaShape3Weighted`;
  - `weightedVoronoiLocate` → `voronoiLocateWeighted`;
  - `weightedGeometricMedianApprox` → `geometricMedianWeightedApprox`;
  - `weightedStraightSkeleton` → `straightSkeletonWeighted`;
  - row 36: `orient2dInt` and `incircleInt` became the `lll` overloads of `orient2d` and `sideOfOrientedCircle`, and the predicate `incircle` became `sideOfOrientedCircle`.
- **Graphs.**
  - Row 64: `weightedBlossomSparse` merged into `maximumWeightMatching`, which is the same heap-driven algorithm;
  - `weightedBlossomDense` → `maximumWeightMatchingDense`;
  - `gabowWeightedMatching` → `maximumWeightMatchingGabow`;
  - `globalMinCutFromExtremeSets` → `globalMinCutExtremeSets`;
  - `offlineDagReachability` → `dagReachabilityOffline`;
  - `denseBfs`, `denseScc` → `bfsDense`, `sccDense`;
  - `offlineDynamicMst` → `dynamicMstOffline`;
  - `exactColoring` → `chromaticNumberWitness`, because `Exact` means a big-integer result elsewhere;
  - `weightedAverageClustering` → `averageClusteringWeighted`.
- **Mathematics.**
  - `interpolateFast` → `interpolate`, the subproduct-tree default beside `lagrangeInterpolate`, as Core `Poly::interpolate`;
  - `gaussCircleFast` → `gaussCircleCountHull`, beside row 34 `gaussCircleCount`;
  - `composedSumArithmetic`, `composedProductArithmetic` → `composedSumExact`, `composedProductExact`;
  - `onlineFpsInverse`, `Exp`, `Log`, `Pow`, `Division`, `Square`, `Sqrt` → `fpsInverseOnline`, `fpsExpOnline`, `fpsLogOnline`, `fpsPowOnline`, `fpsDivOnline`, `fpsSquareOnline`, `fpsSqrtOnline`;
  - row 29 (`/finish P276`): `polyRootsComplex` → `polyRootsApprox` (Aberth, all complex roots; floating result) and `polyRootsReal` → `realRootsApproxSimple` (the lighter KACTL derivative recursion beside `realRootsApprox`).
- **Miscellaneous.** `offlineRangeSum` → `rangeSumOffline`; `offlineSetIntersection` → `setIntersectionOffline`.
- **Strings.**
  - `count2D` → `match2DCount`;
  - `squares (all occurrences)` → `squaresAll`;
  - the Eulerian `deBruijnSequence` → `deBruijnSequenceEulerian`;
  - row 27 `primitiveRoot` → `wordRoot`.
- **Python.** The `04-python.md` mirror rule (user decision 2026-11-10) governs these; all 61 rows were audited against the C++ inventories, and every row is planned.

| Module | Renames |
|---|---|
| `_01` | `readStr` → `readToken`; `flushOut` → `flush`; `tell` → `writeLine`; `finish` → `answer` |
| `_02` | `binarySearchInt` → `binSearch`; `binarySearchReal` → `binSearchRealBracket`; `ternaryMinInt` → `ternSearch`; `ternaryMinReal` → `ternSearchRealBracket`; `goldenSection` → `goldenSearchRealBracket`; `argminUnimodal` → `fibSearch` |
| `_03` | `phiSieve` → `phiTable`; `mobiusSieve` → `mobiusTable`; `divisorCountSieve` → `divisorCountTable`; `divisorSumSieve` → `divisorSumTable`; `multiplicativeSieve` → `multiplicativeTable`; `extgcd` → `extendedGcd`; `iroot` → `ikthRoot`; `isSquare` → `isPerfectSquare`; `phiOne` → `phiTrial`; `modInv` → `inverseXgcd`; `spfSieve` → `spfTable` |
| `_04` | `fact` → `factorial`; `invFact` → `inverseFactorial`; `fibPair` → `fibonacciPair`; `bracketNext` → `nextBalanced`; `bracketRank` → `rankBalanced`; `bracketUnrank` → `unrankBalanced`; `C` → `combiNR`; `P` → `permuNR`; `H` → `combiWR`; `catalanNumber` → `catalanExact`; `derangements` → `derangementTable`; `Comb` → `PrimeCombinatorics`; `ballot` → `ballotExact` |
| `_05` | `kadane` → `maximumSubarray`; `kadaneCircular` → `circularMaximumSubarray`; `lisCount` → `countLongestIncreasingSubsequences`; `inversions` → `inversionCount`; `prevLess` → `previousSmaller`; `nextLess` → `nextSmaller`; `prevGreater` → `previousGreater`; `slidingMin` → `slidingMinimum`; `slidingMax` → `slidingMaximum`; `Swag` → `SWAGQueue`; `DequeSwag` → `SWAGDeque`; `majority` → `majorityElement`; `lisLength` → `longestIncreasingSubsequenceLength`; `twoPointersWindow` → `monotoneWindowEnds`; `prefix2d, rangeSum2d` → `PrefixSum2D: prefixSum, sum`; `difference2d` → `DifferenceArray2D: add, values`; `prefixSums` → `PrefixSum: prefixSum, sum`; `differenceArray` → `DifferenceArray: add, values`; `lisWitness, lisNonStrict` → `longestIncreasingSubsequence (strict/non-strict, witness)` |
| `_06` | `find` → `findSet`; `unite` → `uniteSets`; `same` → `isSameSet` |
| `_07` | `prefix` → `prefixSum`; `RangeFenwick` → `FenwickRangeAdd` |
| `_09` | `bfs01` → `zeroOneBfs`; `dfsIter` → `dfs`; `components` → `connectedComponents`; `topoSort` → `topologicalSort`; `dijkstraPath` → `dijkstraWitness`; `condensation` → `condensationDag`; `bfsDist` → `bfsShortestPaths`; `oddCycle` → `bipartiteCheck`; `prim` → `primSparse`; `scc` → `tarjanScc` |
| `_10` | `kmpFind` → `kmpOccurrences`; `periodOf` → `prefixPeriod`; `zFind` → `zOccurrences`; `RollingHash` → `StringHash`; `combine` → `concat`; `kmpCount` → `countOccurrences`; `borders` → `prefixBorders` |
| `_11` | `distSq` → `dist2`; `perimeter` → `polygonPerimeter`; `pointInPolygon` → `polygonContains`; `latticePointsOnSegment` → `latticeOnSegment`; `pickInterior` → `latticeInterior`; `rotate90` → `perp`; `polygonArea2` → `signedArea2` |
| `_12` | `coinChangeMin` → `minimumCoins`; `lcsLength` → `lcs`; `lcsWitnessLowMemory` → `hirschbergLcs`; `editDistance` → `levenshtein`; `editDistanceAlignment` → `levenshteinWitness`; `subsetSumReachable` → `knapsackFeasible` |
| `_13` | `BlockRmq: prod` → `LinearRMQ: query (leftmost argmin)` |
| `_14` | `isSquareFree` → `isSquarefree`; `countPrimes` → `primeCount` |
| `_15` | `Dinic` → `MaxFlow` (`flow (Dinic)`); `projectSelection` (class) → `maximumWeightClosure` (function, as C++); `BoundedFlow: addEdge, feasible, flow, edges` → `feasibleFlow (lower bounds)`, `maxFlowWithLowerBounds` (Graphs row 30 functions) |
| `_16` | `minVertexCover` → `konigVertexCover`; `hopcroftKarp, kuhn, minVertexCover, maxIndependentSetBipartite` → `BipartiteMatching: hopcroftKarp, kuhn, konigVertexCover, maximumIndependentSet`; `blossomMatching` → `edmondsBlossom` |
| `_17` | `LcaBinaryLift` → `LCA`; `dist` → `distance`; `LcaEuler` → `EulerLCA`; `Hld` → `HLD`; `diameter` → `treeDiameter`; `centroid` → `treeCentroid`; `treeIsomorphismClasses` → `rootedTreeClassification`; `pathRanges` → `processPath`; `kthOnPath` → `jump`; `center` → `treeEccentricities` |
| `_18` | `RollbackDsu` → `RollbackDSU`; `WeightedDsu` → `WeightedDSU`; `ParityDsu` → `ParityDSU`; `find` → `findSet`; `unite` → `uniteSets`; `same` → `isSameSet`; `offlineDynamicConnectivity` → `OfflineDynamicConnectivity: addEdge, insertEdge, eraseEdge, query, run`; `ParityDSU` drops `findSet`, `isSameSet` (not in C++) |
| `_19` | `PersistentCountTree` → `PersistentValueTree`; `kth` → `rangeKth`; `versions` → `versionCount` |
| `_20` | `convolve` → `convolution`; `convolveNtt` → `convolutionNtt`; `convolveMod` → `convolutionArbitraryMod`; `convolveFft` → `convolutionFft`; `polyInv` → `fpsInverse`; `polyLog` → `fpsLog`; `polyExp` → `fpsExp`; `polyPow` → `fpsPow`; `polySqrt` → `fpsSqrt`; `multipointEval` → `multipointEvaluate`; `convolveInt` → `convolutionExact`; `lagrangeAtPoint` → `lagrangeEvaluate` |
| `_21` | `matMul` → `matrixProduct`; `matPow` → `matrixPower`; `matIdentity` → `identityMatrix`; `matMulMod` → `matrixProductMod`; `matPowMod` → `matrixPowerMod`; `rankMod` → `matrixRankMod`; `detMod` → `determinantMod`; `solveMod` → `solveLinearMod`; `solveFraction` → `solveLinearExact`; `inverseFraction` → `inverseExact`; `solveFloat` → `solveLinearApprox`; `gaussMod` → `rrefMod`; `gaussFraction` → `rrefExact`; `detFraction` → `determinantExact`; `rankF2, detF2, solveF2, inverseF2, nullspaceF2` → `BitMatrix (GF(2)): rnk, det, solveLin, inv, ker` |
| `_22` | `lowbit` → `lowBit`; `nextSamePopcount` → `nextCombination`; `subsetSumBitset` → `bitsetSubsetSum`; `lcsBitset` → `bitsetLcs`; `intersect` → `intersection`; `submasks` → `forEachSubmask`; `supermasks` → `forEachSupermask`; `witness` → `represent` |
| `_23` | `maxXor` → `xorMax`; `minXor` → `xorMin`; `xorAll` → `applyXor`; `Trie.contains` → `search`; `Trie.enumerate` → `forEach` |
| `_24` | `addPattern` → `add`; `next` → `step`; `findAll` → `forEachMatch`; `countOccurrences` → `countPatterns`; `terminalCounts` → `stateCounts` |
| `_25` | `lcp` → `lce`; `countOccurrences` → `occurrenceCount`; `SuffixArrayIndex` → `SuffixArray`; `find` → `patternRange`; `endposSizes` → `endposSize`; `numberOfSubstrings` dropped for `SuffixAutomaton.distinctSubstrings`; `SuffixAutomaton.contains` → `isSubstring`; `longestRepeatedSubstring` → `longestRepeated` |
| `_26` | `offlineLcaTarjan` → `lcaOffline`; `offlineRangeDistinct` → `staticRangeCountDistinct`; `offlineRangeInversions` → `staticRangeInversions`; `offlineRectangleSum` → `rectangleSumOffline`; `cdqDominanceCount` → `dominanceCount3D`; `moQueries` → `Mo: addQuery, solve`; `moWithUpdates` → `MoUpdates: addQuery, addUpdate, solve`; `moOnTree` → `TreeMo: addPathQuery, addUpdate, solve` |
| `_30` | `divideAndConquerDp` → `divideConquerDp`; `boundedKnapsackMonotone` → `boundedKnapsackMonotoneQueue`; `alienTrick` → `aliensTrick`; `minPlusConvolutionConvex` → `minPlusConvolutionConvexConvex`; `addLeft` → `addAMinusX`; `addRight` → `addXMinusA` |
| `_31` | `WordSet` → `FastSet` |
| `_32` | `ConvexHullTrick, ConvexHullTrickBinary (add)` → `MonotoneCHT (addLine, query, queryMonotone)`; `LiChao, LiChaoDynamic` → `LiChaoTree` |
| `_33` | `setTrue` → `setValue`; `assignment` → `answer` |
| `_35` | `eulerTrailDirected, eulerTrailUndirected, eulerCircuitDirected, eulerCircuitUndirected, hasEulerTrail` → `eulerianTrail (directed, undirected; circuit when one exists)` |
| `_36` | `negativeCycleWitness` → `findNegativeCycle` |
| `_37` | `minCostAssignment` → `linearSumAssignment` |
| `_38` | `bsgs, exBsgs` → `discreteLogEx`; `discreteRoot` → `kthRootModPrime`; `linearDiophantine` → `solveDioEq`; `linearCongruence` → `solveModEq`; `minOfModLinear` → `minOfModOfLinear`; `duSieve` → `dujiaoSieve`; `lucas` → `binomialLucas`; `powerTower` → `powerTowerMod`; `binomialPrimePower` → `BinomialPrimePower: ctor, operator(), unitFactorial`; `binomialAnyMod` → `BinomialCompositeMod: operator()`; `sqrtMod` → `sqrtModPrimeCipolla` (its Tonelli–Shanks qualifier dropped: the C++ row has Cipolla only); `crtGeneral` folded into `crt` (noncoprime moduli) |
| `_39` | `zetaSubset` → `subsetZeta`; `mobiusSubset` → `subsetMobius`; `zetaSuperset` → `supersetZeta`; `mobiusSuperset` → `supersetMobius`; `convolveOr` → `orConvolution`; `convolveAnd` → `andConvolution`; `convolveXor` → `xorConvolution`; `convolveGcd` → `gcdConvolution`; `convolveLcm` → `lcmConvolution`; `gcdTransform` → `multipleZeta`; `gcdInverse` → `multipleMobius`; `lcmTransform` → `divisorZeta`; `lcmInverse` → `divisorMobius` |
| `_40` | `bfsStates` → `implicitBfs`; `meetInTheMiddleSubsetSum` → `subsetSumMitm`; `meetInTheMiddleEnumerate` → `splitEnumerate` |
| `_41` | `angle` → `angleApprox`; `rotate` → `rotateApprox`; `project` → `projectLineApprox`; `reflect` → `reflectLineApprox`; `distPointLine` → `lineDistanceApprox`; `distPointSegment` → `pointSegmentDistanceApprox`; `lineIntersectionF` → `lineIntersectionApprox`; `circleLineIntersection` → `circleLineIntersectionApprox`; `circleCircleIntersection` → `circleIntersectionApprox`; `minimumEnclosingCircle` → `minimumEnclosingCircleApprox`; `addF` → `addApprox`; `subF` → `subApprox`; `scaleF` → `scaleApprox`; `circleTangents` → `commonTangentsApprox` (two circles; `pointTangentsApprox` is outside the subset); `incenter` → `incircleApprox`; `circleFrom3, circumcenter` → `circumcircleApprox` |
| `_42` | `kthSuccessor` → `jump`; `tailLength` → `depth`; `entryPoint` → `entry` |
| `_43` | `tspBitmask` → `travelingSalesman`; `hamiltonianPathCount` → `countHamiltonianPaths`; `dominoTiling` → `brokenProfileTiling`; `optimalBst` → `optimalBinarySearchTree`; `digitDp, digitDpRange` → `DigitDP: count (closed interval, base)`; `profileDp` → `brokenProfileDP` |
| `_44` | `anySegmentsIntersect` → `anyIntersection`; `segmentIntersectionsSweep` → `reportIntersections`; `intersectionPointsExact` → `intersectionPoints` |
| `_45` | `RankSelect` → `SuccinctBitVector` |
| `_47` | `linearRecurrenceKth` → `nthTermLinearRecurrence`; `linearRecurrenceKthKitamasa` → `kitamasa`; `rationalSeriesCoefficient` → `bostanMori`; `linearRecurrenceTerms` → `consecutiveTerms` |
| `_48` | `burnside` → `burnsideCount`; `cycleIndexCyclic, cycleIndexDihedral` → `polyaCycleIndex (cyclic, dihedral)`; `evaluateCycleIndex` → `polyaEnumeration`; `polyaNecklace` → `necklaceCount`; `polyaBracelet` → `braceletCount` |
| `_49` | `solutions` → `forEachSolution` |
| `_50` | `add` → `insert`; `simpson` → `simpsonFixed`; `adaptiveSimpson` → `simpsonAdaptive` |
| `_52` | `RemovableHeap` → `ErasableHeap`; `remove` → `erase`; `DoubleEndedHeap` → `DoubleEndedPriorityQueue`; `decrease` → `decreaseKey`; `KeyHeap` → `BinaryHeap` |
| `_53` | `Eertree` → `PalindromicTree`; `add` → `append`; `occurrences` → `occurrenceCounts` |
| `_54` | `deBruijn` → `deBruijnSequence`; `lyndonFactorization` → `duval` |
| `_55` | `halfPlaneIntersection` → `halfplaneIntersection`; `convexCut` → `polygonCutApprox` |
| `_56` | `prevValue` → `maxLeq`; `nextValue` → `minGeq`; `BitVector` dropped: C++ row 32 keeps it internal and `_45` `SuccinctBitVector` is the public type; `rangeFreq` dropped for `countRange` |
| `_57` | `bestRationalApprox` → `bestRationalApproximation` |
| `_58` | `retrogradeSolve` → `retrogradeAnalysis`; `nimGame` → `nimWin` |
| `_59` | `maxClique` → `maximumClique`; `maxIndependentSet` → `maximumIndependentSet`; `maximalCliques` → `bronKerbosch` |
| `_60` | `pointUpdate` → `set`; `rangeApply` → `apply` |
| `_61` | `rangeApply` → `apply`; `intervals` → `enumerate` |

## Kept, with reasons

- `phiTrial`, `isPrimeTrial`, `factorizeTrial`: `Trial` is the short name of trial division (user decision).
- **Catalog phrases** (user decision):
  - the leading-word forms `sparseSuffixArray`, `exactCover`, `onlineOfflineDp`, `offlineSweep`, `offlineFallback`, `naiveSearch`, `sortedPairSum`;
  - `maximumWeightMatching`, `maximumWeightedAntichain` and `maximumWeightedIndependentSet`, each the catalog's maximum-weight problem name;
  - the `weighted…` names `weightedIntervalSchedule`, `weightedMatroidIntersection`, `weightedAncestor(s)`, `weightedEditDistance`, `weightedGirth`, `weightedReservoirSample`, `weightedIncreasingSubsequence`;
  - types such as `OfflineDynamicConnectivity`. The rule names operations.
- `count…` and `all…` task names (`countTriangles`, `allTopologicalSorts`, `allProd`): the verb or quantifier is part of the task's name, not a variant suffix.
- `getManhattanMSTEdges`, `getDis`: these names appear only in `legacy:` cells and legacy mappings, never as operations.
- `bestRationalApproximationReal`, `isReal`, Python `randReal`: they take a real-valued domain, which `Real` names.
- Strings `isSquareFree`: the same meaning as Mathematics `isSquarefree` on another argument type.
- `getHilOrd` leaves with P067, as the brief says.
- Python `_27` `weightedChoice` stays: its owner removes it (`duplicates.md`, stdlib `random.choices`).

## Handoffs

- **Core, Fable** (in each package's plan note):
  - P258: `InfInt::getBaseId` → `baseId`, and `divModSlow` (Knuth D only) → a bound or short name;
  - P256: `InfIntMini::getBaseId`;
  - P258, as well: `Poly::sparseMul`;
  - P023: `sparseGauss`;
  - P020: `rref<1>` beside `BitMatrix::ref`; Core `invReal`, `rankReal` and `solveReal` stay P020's remove (row only) items.

## Commands and results

- `python3 '96-Local Testing/01-run.py' --mode quick` (2026-11-11, after the `/finish` fixes): PASS, 186 suites; its NOTE lists quick builds over 10 s timed beside other suites, as in every mode.
- Full on g++ (2026-11-10): the 23 affected testers in `96-Local Testing` passed, `--mode full --seed 1`:
  - DS 01, 09, 10, 29, 65;
  - Graphs 04, 07, 14, 15, 19, 20, 21, 26, 27, 30, 35;
  - Math 04, 06, 16, 20, 24;
  - Geometry 19;
  - Misc 10.
- Full on g++-14 (2026-11-10): the same 23 passed with `CXX=g++-14`.
- Stress (2026-11-10): the same 23 passed, `--mode stress --seed 7`.
- Math 14 after the review's `fpsSparse…` renames: `14-polynomial_algorithms_tester.py` full (g++ and g++-14) and stress; `07-lca_tester.py` quick after the brace fix. All passed (2026-11-10).
- Extraction (2026-11-10): `python3 '96-Local Testing/05-extraction.py' ops --filter <header>` passed for each of the 21 changed headers and Math 14; `testers --filter <tester>` passed for each of the 19 changed C++ testers and Math 14's.
- Mutation (2026-11-10): changed-line campaigns for the three changed headers that hold a record, `python3 '96-Local Testing/04-mutation.py' run <header>`, recorded in their documents:
  - `01-dsu.hpp`: 3 of 3 caught;
  - `14-tree_algorithms.hpp`: 23 of 25 caught, 2 equivalent (`lca` symmetry);
  - `15-hld.hpp`: 21 of 23 caught, 2 equivalent.

  `check` passes for all three. As with `/prune` (`05-testing.md`, Mutation testing), the changed headers without a record need no campaign for a rename.
- `python3 '96-Local Testing/03-consistency.py'` (2026-11-10): no errors. The `primitiveRoot` entry left `name_collisions`, and the four others remain.

- Search corpus (`/finish P276`, 2026-11-10): `14-search_tester.py` `test_repo_corpus` failed on `simple`, MRR 0.877 against a floor of 0.88, and HEAD gave the same. Rows from P254 (`matchingStatistics`) and P275 had outranked `matching`, `convex hull trick`, `modular exponentiation` and `longest common subsequence`. Alias lines in Graphs 21, Data Structures 22, Mathematics 01 and Strings 19 brought `simple` to 0.919 with every source above its floor, and `python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode quick` passed. No weights changed. After the Python fixes of the `/finish` review, which had taken `py` to recall 0.900 (`python dinic`, `python binomial`, `python lis`), the qualifiers `flow (Dinic)`, `(LIS)` and `combiNR (binomial)` gave `py` 1.000 / 0.811 (after the step 5 fixes) against floors 0.94 / 0.78 and `simple` 1.000 / 0.917; quick passed (2026-11-11).
- Python audit (`/finish P276`, 2026-11-10): a subagent compared all 61 rows with the C++ inventories and found 186 renames, 82 ambiguous items and 36 owner removals. Every proposed name not in a C++ row was checked as a vocabulary-built variant. The ambiguous items were settled by the mirror rule, and two unintended member renames (`CountMinSketch::add`, `RunningMedian::remove`) were reverted. The `/finish` review then found nine gaps in this pass; they were mirrored the same way (table above; `10-decisions.md`, 2026-11-10 P276), and `03-consistency.py` now rejects a Python type in two modules, as `02-structure.md` (Operation names) states. `03-consistency.py` reports no errors.

## History

- 2026-11-10: P276 renames, 5 user decisions, independent review with 10 findings, all fixed; quick, full, g++-14 full, stress, extraction and mutation passed for the affected headers.
- 2026-11-10: /finish P276: Mathematics row 29 renamed, all 61 Python rows mirrored to C++ (4 user decisions), search-corpus floor restored by aliases.
- 2026-11-11: /finish P276 review, 10 findings: Python mirror gaps, ledger and note records fixed; Python type uniqueness check added; search floors pass.
- 2026-11-11: /finish P276 step 5 review, 8 findings fixed (`BoundedFlow`, `ballotExact`, `PrefixSum`/`DifferenceArray`, `ParityDSU`, rule wording, alias qualifiers, ledger names, check scope).
