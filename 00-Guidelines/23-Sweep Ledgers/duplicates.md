# Duplicates

Wrappers and second implementations under `00-Guidelines/03-cpp.md`, One API per operation. `/duplicate-sweep` writes the package sections and `/prune Pxxx` consumes them.

## Recorded decisions

Decided by the user from 2026-10-10 to 2026-10-16; binding, and only the user changes them.

### Changes

- **P030** row 18 (pinned): `PerSegTree` is a second persistent segment tree beside `PersistentSegmentTree`; its entries leave the row and `PersistentSegmentTree` gains `maxRight`/`minLeft` (from `maxR`/`minL`). `KSPerSegTree::kthMin` leaves the row (`rangeKth`); its `cntEql`, `cntRan`, `kthSum`, `maxLeq`, `minGeq` stay as the only persistent forms.
- **P047** row 26 (pinned): `OnlineBridges` leaves the row; `IncrementalBridgeConnectivity` gains `connected` (from `findCC`).
- **P090** `08-Python` row `_05_sequences`: `kadane` and `kadaneCircular` are named `maximumSubarray` and `circularMaximumSubarray`, as in C++ (`04-python.md` naming).
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
| P006 | `SqrtDecomp::opeUpdate`, `SparseTable::operator<<` | Only form |
| P006 | `SqrtDecomp::rebuild` | Decided 2026-10-13: larger domain; resets in place with the stored `id` and `f`, while assigning a new object needs an assignable `F`, which a capturing lambda is not |
| P007 | 2D `angleApprox` | Named special case (unsigned angle; one name for 2D and 3D) |
| P010 | `findNegativeCycle`, `bfsShortestPaths` | Named special case (all-vertex seeding); BFS form of `ShortestPathResult` |
| P011 | `lca_detail::Forest` inclusive `t_in`/`t_out` | Only form |
| P011 | `functionalOrbit`, `condensationDag` | Different domain (walks that stop); replacement differs in weights, order and memory |
| P011 | `getKthAncestor` | Accepts k < 0 |
| P011, P043 | `kosarajuScc`, `kuhn` | Different algorithms from `tarjanScc`, `hopcroftKarp` |
| P012 | `ternSearch`, `ternSearchRealBracket` | Different algorithms from `fibSearch`, `goldenSearchRealBracket` |
| P012 | `binSearch(ok, ng, f)` | Decided 2026-10-11: one orientation-free call for the common ok/ng idiom; the replacement needs `lastTrue` or `firstTrue` by orientation plus a result mapping |
| P012 | `LinearSieve::getPhi`, `getMu`, `getNumDiv`, `getSumDiv` | Reuse the sieve's `spf` table |
| P012 | `PrimeCombinatorics` fields `fac`, `inv_fac` | Data, direct indexing |
| P012 | `SegmentedSieve::getPrimeFac()`, `getTables()`, constructor without a base | Named table; batch form; one-call form beside the base-reuse form |
| P012 | `permuWR`; the `ballot` and `ballotExact` `strict` flag | Named count (0 for negative b); variant inside one operation |
| P012, P052 | `solveModEq` `{-1, -1}`, `garner` and `crtMod` returning -1 | Only form |
| P013 | global `rng`; `CustomHash` convertible and enum overload | Only shared instance; only hash for those types |
| P013 | `BitOps::first`, `last`, `parity` | Named special cases (-1 for zero, index, bool) |
| P229 | `Timer::elapsed`, `elapsedUs`, `elapsedMs`, `elapsedSec` | Different units |
| P016 | `prefixToZ`, `centerEnd`, `prefixPeriods`, `zPeriods`, `StringHash` field arithmetic | Named conversion; listed operation; mirrored convention; self-contained hashing arithmetic |
| P017 | `AhoCorasick::countPatterns` | Named per-pattern count |
| P018, P019 | InfInt infinity flag, `INF` token, saturating conversions | Only signed-infinity arithmetic |
| P023 | `steadyState` | Only form; fixed-iteration power method, kept beside the planned converged solver |
| P052 | `factorizeTrial`, `isPrimeTrial`, `inversePierce`, `inverseFermat` | Different algorithms |
| P052 | `inverseBatch`; `primality_detail::splitmix` | Larger domain (every modular type); internal helper |
| P081 | unit-cost `hirschberg(a, b)` | Named special case (unit-cost alignment) |
| P043 | `TwoSat::ans` | Data behind `answer()` |
| P043 | `addImplication`, `addEquivalent` | Named special cases, as `setValue`; `addNand` stays omitted (decided 2026-10-11) |
| P043 | `hallViolator` | Named certificate (Hall violator) |
| P055 | `FastConv::fctGcd`, `fctLcm` | Reuse a caller's `LinearSieve` |
| P055 | `FastConv::fctGcdSlow`, `fctLcmSlow` | Different algorithm (harmonic loops) |
| P083 | `countDistinctSubsequences` (`19-lcs.hpp`) | Different domain from P084's member |
| P057 | `polynomial_detail::inverses`, `factorials` | Internal helpers |
| P084 | `bwt(s, sentinel)`, `dfaSearch(pattern, text, sigma)` | Headline operation; only form taking a `char` pattern |
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

### Rulings

Decided by the user on 2026-10-11 for the sweep's policy questions.

- A bool test or defaulted read of another operation's result (`contains`, `search`, `isSubstring`, `reachable`, `none`, `get(k, def)`) is a named special case: keep it.
- The complement of another operation's result (`maximumIndependentSet` beside `konigVertexCover`) is a separate operation: keep it.
- An orientation or sign mirror of an operation (`ceilSum`, `upperEnvelope`, concave max-plus convolution) is a named special case: keep it.
- A second implementation or cheaper projection kept for its constant factor (`convexHull`, `truncatedMul`, `CommutativeDualSegmentTree`, the Tarjan inside `TwoSat`, PyPy-inlined variants) stays.
- A one-vertex overload of a multi-source function (`dijkstra(g, s)` beside `dijkstra(g, sources)`) is an argument shape: remove it; callers write `f(g, {s})`.

## Kept by the sweep

Keep decisions of `/duplicate-sweep` that apply the user's rulings and clarified rules; a later sweep revalidates them.

| Package | Item | Reason |
|---|---|---|
| P002 | `rangeMask(bits, l, r)` (`18-bitset.hpp`) | Builder: construction plus `setRange` (clarified rule) |
| P002 | `findNext(p)` (`18-bitset.hpp`) | Accepts `p == n`, which `findFrom` rejects (clarified rule) |
| P002 | `none()` (`18-bitset.hpp`) | Named special case (ruling 1) |
| P002 | `combine<Op>` (`18-bitset.hpp`) | Helper of `combineShift`; dropped from the row 2026-10-12 |
| P002 | `setRange(l, r, false)` (`18-bitset.hpp`) | The flag is the value written, as in `set(p, v)` |
| P004 | `redc(x, lazy)` (`04-montgomery.hpp`) | Unchecked kernel public only because there is no `private` |
| P006 | Fenwick `lowerBound`, `upperBound`, `descend` (`02-fenwick.hpp`) | Named special case; larger domain (`x <= 0`); 11% faster (`Docs/02-fenwick.md`) |
| P006 | `SegmentTree::apply` (`03-segmenttree.hpp`) | As the recorded keep `SqrtDecomp::opeUpdate` |
| P006 | `DifferenceArray(2D)::clear` (`05-prefix_sum.hpp`) | Named special case, in place |
| P006 | Braced-list constructors (`02-fenwick.hpp`, `03-segmenttree.hpp`, `05-prefix_sum.hpp`, `06-sqrt_decomposition.hpp`) | Without them `Fenwick<lng> f{7}` picks the size constructor (`Docs/00-notes.md:79`) |
| P006 | `CoordinateCompression::decode` (`07-ordered_set.hpp`) | Named inverse of `encode`; replaces P013's `value` |
| P006 | `previousGreater`, `nextGreater`, `slidingMaximum` (`08-monotone_stack.hpp`) | Named special cases |
| P007 | `signedAreaApprox` (`03-polygon.hpp`) | Another scale: half the doubled area (clarified rule) |
| P007 | `polygonContains`, `PolygonLocation` (`03-polygon.hpp`) | Derived classification of the winding number (clarified rule) |
| P007 | `convexHull` (`04-convexhull.hpp`) | Constant-factor variant (ruling 4) |
| P007 | `projectLineApprox`, `reflectLineApprox`, `reflectDirectionApprox` (`02-line_segment.hpp`) | Different computation (one division, exact 0.5 where the factory gives 0.4999…) and domain (any `Point2<T>`) |
| P008 | `Affine2Approx::orientation()` (`08-coordinate_transform.hpp`) | Derived formula: sign of `determinant()` (clarified rule) |
| P008 | `CircleApprox::onBoundary`, `contains(dpoint)` (`06-circle.hpp`) | Named special case (ruling 1) |
| P008 | `rectangleApprox(bool perimeter)` (`07-rotatingcalipers.hpp`) | Shared helper; dropped from the row 2026-10-12 (flag between two operations) |
| P008 | `circleCentersApprox`, `pointTangentsApprox` (`06-circle.hpp`) | Named special cases |
| P008 | `farthestPair` (`07-rotatingcalipers.hpp`) | Larger domain (any cloud, index ties) |
| P009 | `Triangle2::areaApprox` (`09-triangle.hpp`) | Derived formula: half the absolute doubled area (clarified rule) |
| P009 | `TriangleApprox::area()` (`09-triangle.hpp`) | Derived formula: half the absolute doubled area (clarified rule) |
| P010 | `Graph::read` flags, `CsrGraph::reverse`, `maximum` flags, `dfsForest`, `KruskalReconstruction::bottleneck` (`01-graph.hpp`, `02-traversal.hpp`, `06-mst.hpp`) | Only forms, named special cases |
| P011 | `StrongOrientationResult`, `strongOrientation` (`09-bridges_articulation.hpp`) | Derived `ok` flag and Robbins obstruction witness (clarified rule) |
| P011 | `FunctionalGraph(const G &g)` (`11-functionalgraph.hpp`) | Builds from a `Graph`, not a legacy format (clarified rule) |
| P011 | `FunctionalGraph::reachable` (`11-functionalgraph.hpp`) | Named special case (ruling 1) |
| P011 | `FunctionalGraphFold::segment` (`11-functionalgraph.hpp`) | Helper of `fold`, `cycleAggregate`, `maxStep`; dropped from the row 2026-10-12 |
| P012 | `starsBars`, `starsBarsExact` (`05-combinatorics.hpp`) | The positive variant is a derived formula with a guard (clarified rule) |
| P012 | `fibonacciExact`, `LinearSieve::getAllFac` (`05-combinatorics.hpp`; `04-sieve_algorithms.hpp`) | Larger domain (n = 93); reuses `spf`, unsorted |
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
| P028 | 2D point `apply` (`15-segment_tree_2d.hpp`) | As `SegmentTree::apply` and the recorded `opeUpdate` |
| P028 | `LazySegmentTreeBeats`; merge-sort-tree `countLess`, `countRange`, `countEqual`, `maxLeq`, `minGeq`, `set` (`14-segtreebeats.hpp`; `16-mergesorttree.hpp`) | Larger domain (`fail`); named special cases, cheaper cascades |
| P057 | `polyMul` (`14-polynomial_algorithms.hpp`) | Composition: convolution then trim (clarified rule) |
| P057 | `nthTermLinearRecurrence` (`15-linear_recurrence.hpp`) | Builds the generating function itself (clarified rule) |
| P057 | `consecutiveTerms` (`15-linear_recurrence.hpp`) | Builds the generating function itself (clarified rule) |
| P057 | `polyDivLinear`, `polyGcd`, `polyInverseMod`, `fiduccia`, `kitamasa`, `berlekampMassey` (`14-polynomial_algorithms.hpp`; `15-linear_recurrence.hpp`) | Named special cases; cheaper (`polyGcd`); only form; `tryX` pair |
| P084 | `shiftOr`, `shiftAnd` (`17-bitap.hpp`) | Single-word constant-factor variants (ruling 4) |
| P084 | Pattern overloads of `shiftAnd`, `shiftOr`, `shiftOrMultiword`, `bitapHamming`, `bitapEdit` (`17-bitap.hpp`) | Build the masks themselves (clarified rule) |
| P084 | `isSubstring` (`16-string_matching.hpp`) | Named special case (ruling 1) |
| P084 | `countOccurrences`, `BitsetText::count`, `lfMapping` (`16-string_matching.hpp`; `17-bitap.hpp`; `29-bwt.hpp`) | Only disjoint form; cheaper counts; standard map |
| P036 | `minkowskiDifference` (`11-minkowskisum.hpp`) | Derived: negation then sum (clarified rule) |
| P036 | `minkowskiSumHull` (`11-minkowskisum.hpp`) | Larger domain: any point sets (clarified rule) |
| P036 | `halfplane_detail::vertex`; `polygonKernel` (`10-halfplaneintersection.hpp`) | Detail helper; different algorithm, user decision 2026-10-10 (`Docs/00-notes.md:67`) |
| P033 | `fold(l, r)` with the empty range (`28-disjointsparsetable.hpp`) | Accepts `l == r`, which `query` rejects (clarified rule) |
| P033 | `lcaRmqBridge` (`27-cartesiantree.hpp`) | An LCA–RMQ reduction, not a rename (clarified rule) |

## Conventions

Locations are relative to the package's folder and tests to `96-Local Testing/<folder>/`; "row NN" is the `00-index.md` row of header `NN-`, "clarified rule" cites `03-cpp.md`, One API per operation, and "ruling N" the Nth of the Rulings above. A `remove (row only)` item has no code to change, or its code is a pinned extraction whose body stays until the package rewrites it.

## P018 — 01-Core — infint (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `bin`, `oct`, `hex` | `07-infint.hpp:1542,1553,1565` | `toString(2)`, `toString(8)`, `toString(16)` | remove | another name; the bodies become `toString`'s power-of-two path | `07-infint_tester.cpp:128-137,491-494`; `.py:120-121` | none |
| `operator!` | `07-infint.hpp:1412` | `!a` | remove | another form | `07-infint_tester.cpp:444` | none |
| `operator=(string_view)` | `07-infint.hpp:548` | `a.read(s)` | remove | another name (`a = "123"` stops compiling) | none | none |
| `rand(bits)` | `07-infint.hpp:1396` | planned `sampleBits(bits, rng)` | keep | no existing operation yet, and other results; its hidden seed is a separate `03-cpp.md` issue (clarified rule) | `07-infint_tester.cpp:436-437` | none |
| repeated bodies: word division ×4, `read` repeating `init`, power-cache extension | `07-infint.hpp:299-303,373-385,437-441,456-460,469-484,869-935` | one helper each | remove | body-level second implementations | — | — |
| `shiftBlocksLeft`, `shiftBlocksRight` | `07-infint.hpp:969,973` | `<<=`, `>>=` | keep | helpers outside the row (and `32 * k` overflows); move to `infint_detail` | — | `07-infint.hpp:989-993,1034,1123-1124` |
| `explicit operator bool` beside `isNil()` | `07-infint.hpp:1408,1410` | each other's negation | ask | which survives | `07-infint_tester.cpp:444` | `16-poly.hpp:17` (`BigInt` needs `isNil`) |
| `init`, `read` | `07-infint.hpp:327,362` | constructors | ask | in-place re-initialisers | `07-infint_tester.cpp:111-114` | `>>` (:1647) |
| `convTwosComp`, `fromTwosComp` | `07-infint.hpp:1464,1473` | planned `toLimbs`, `fromLimbs`; inline copy in `bitOpTwosComp` | ask | unlisted: list as signed limb forms or remove | `07-infint_tester.cpp:189-192` | none |
| `FastIintMul` | `07-infint.hpp:6-232` | `04-montgomery.hpp`, Poly NTT | ask | second Montgomery and NTT kernel: reuse or move into `infint_detail` | `07-infint_tester.cpp:197-276` | — |
| row 07: `divTrunc`, `remTrunc`; `fitsLng`, `fitsUlng` | row 07 | `/`, `%`; status of `tryCast` | remove (row only) | another name; projected field | — | — |
| row 07: `schoolbook (Knuth D)`, `newtonReciprocal`, `radixDivideAndConquer`, `quotientOnly`, `remainderOnly` | row 07 | `divModSlow`, `newtonRaphsonInv`, `initFromBlks` and `toString`, `/`, `%` | remove (row only) | listed as missing operations; annotate the existing names | — | — |
| row 07: base 2–36 parse and print, `UInt::fromString`, `isDivisible`, `isPerfectSquare`, `hammingDistance`, `isProbablePrime` | row 07 | `read`/`toString`, constructor, `%`, `isqrtRem`, popcount of xor, Mathematics row 36 `bpsw` | ask | wrappers unless each has its own algorithm or domain | — | — |

## P019 — 01-Core — infintmini, rational (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `divModSlow` | `08-infintmini.hpp:357` | `divMod` (body moves in) | remove | another name | `08-infintmini_tester.cpp:270` | none |
| `operator!` | `08-infintmini.hpp:468` | `!a` | remove | another form | `08-infintmini_tester.cpp:131` | none |
| `operator=(string_view)` | `08-infintmini.hpp:100` | `a.read(s)` | remove | another name | `08-infintmini_tester.cpp:100` | none |
| word loops in `/=`, `%=` | `08-infintmini.hpp:313-319,339-345` | `divMagSmall` (widened) | remove | body-level | — | — |
| `explicit operator bool` beside `isNil` | `08-infintmini.hpp:464,466` | each other | ask | as P018 | `08-infintmini_tester.cpp:131,153` | — |
| row 09 Rational `toDouble`; `parse`, `toString` | row 09 | `toLongDouble`; `>>`, `<<` | ask | narrowing unless correctly rounded; stream shape (decide with InfInt) | — | — |

## P020 — 01-Core — matrix (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Matrix(vector<T>, bool axis)` | `10-matrix.hpp:267` | `Matrix(1, n, v)`, `Matrix(n, 1, v)` | ask | an empty `v` gives 0×0 here and 1×0 or 0×1 from the replacements | none | `10-matrix.hpp:345,346,421,549` |
| `*=(vector)`, `vector * Matrix`, `Matrix * vector` | `10-matrix.hpp:318,345,346` | `a * Matrix(n, 1, b)`, `Matrix(1, n, b) * a` | ask | the same empty-vector edge as the axis constructor | none | none |
| `/=(Matrix)`, `operator/` | `10-matrix.hpp:326,397` | `a * inv(b)` | keep | composition: inverse then product (clarified rule) | none | none |
| Gauss–Jordan loop in `det`, `solveLin`, `adj`, `ker`, `rref` | `10-matrix.hpp:448-467,492-524,551-584,723-749,770-797` | one shared elimination | remove | body-level ×5 | — | — |
| `FastMatMul::addMatTo`, `subMatTo`; `getMod`, `getModInv` | `10-matrix.hpp:24,36,127,129` | `FastGaussian::addRow`, `subRow`; `T::mod()`; `Montgomery32(mod).inv` | remove | body-level second implementations | — | — |
| 8-lane REDC in `mulRow`, `elimRow` | `10-matrix.hpp:161-179,201-222` | `Montgomery32` `mul8`, `red8` | ask | the same REDC, or an assert-free lazy variant like Poly's kept `Mont8` | — | — |
| row 10: `matVec`, `vecMat`, `adjugate (singular rank n-1 case)` | row 10 | `*` with a vector builder; `adj` | remove (row only) | argument and result shape; another name | — | — |
| row 10: `solveAll` | row 10 | `solveLin` plus `ker` | keep | batch form: one elimination for both (clarified rule) | — | — |
| `size()` | `10-matrix.hpp:287` | `.n` | keep | accessor of a public data member | — | — |
| `rref<opt>` | `10-matrix.hpp:768` | — | keep | only row-echelon form (`rnk`) | — | — |
| row 10: `solveReal`, `invReal`, `rankReal` | row 10 | the floating branches of the exact names | ask | exact and approximate variants need distinct names | — | — |
| row 10: `StaticMatrix<T, N, M>`; `rowSpace`, `colSpace` (also BitMatrix) | row 10 | `Matrix`; `rref`, `trp` | ask | same algorithms at a compile-time shape; wrappers unless another basis | — | — |

## P022 — 01-Core — bitmatrix, bitmatrixmini (planned)

`12-bitmatrix.hpp` is pinned. Not a duplicate: `*=(int)` (:113) zeroes only for `o == 0`; GF(2) needs `o & 1`.

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `BitMatrix(string, axis)` | `12-bitmatrix.hpp:31` | `BitMatrix(1, n, s)`, `BitMatrix(n, 1, s)` | remove (row only) | flag between shapes | none | none |
| dead `tmp`, `mul` in `pow` | `12-bitmatrix.hpp:270-292` | `operator*` | remove (row only) | unused second copy | none | none |
| elimination in `det`, `ref`, `rref`, `solveLin` | `12-bitmatrix.hpp:234-253,333-353,361-382,393-420` | one elimination | remove (row only) | body-level; P022 merges | — | — |
| row 12: `solveAll` | row 12 | `solveLin` plus planned `ker` | keep | batch form: one elimination for both (clarified rule) | — | — |
| `size()` | `12-bitmatrix.hpp:84` | `.n` | keep | accessor of a public data member | — | — |
| `-=`, binary `-`, unary `-` | `12-bitmatrix.hpp:109,129,145,151` | `+=`, `+`, unary `+` | ask | equal over GF(2); generic ring code may need them | — | — |

## P023 — 01-Core — sparsematrix (planned)

`14-sparsematrix.hpp` is pinned.

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `polyMulMod(a, b, c)` | `14-sparsematrix.hpp:108` | Poly `mulMod` with `c` reversed | keep | accepts L = 0 and non-field T, which `PolyModulus` rejects (clarified rule) | none | `14-sparsematrix.hpp:255,259` |
| Krylov prologue in `det`, `solveLin`, `pow` | `14-sparsematrix.hpp:132-166,183-204,228-249` | planned `minPoly (Wiedemann)` | remove (row only) | body-level; one implementation | — | — |
| row 14: `spmv`, `powVec` | row 14 | `operator*(vector)` (:50); `steadyState(a, v, k)` | remove (row only) | another name | — | — |
| `berlekampMassey` | `14-sparsematrix.hpp:74` | Poly `findLinearRecurrence` | ask | same algorithm over exact fields; floating T uses a 1e-9 zero test | none | `14-sparsematrix.hpp:132-249` |
| row 14: `toCOO`, `toCSR` | row 14 | `entries`, triplet constructor | ask | wrappers unless COO and CSR are distinct types | — | — |
| `size()` | `14-sparsematrix.hpp:45` | `.n` | keep | accessor of a public data member | — | — |

## P085 — 07-Strings — wildcardmatching, lexicographic queries, dynamicstringhash (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `lce` | row 22 | `SuffixArray::lce` | remove (row only) | same operation | — | — |
| `lexMinSuffixPerPrefix`, `occurrenceCount` | row 22 | `IncrementalLyndon` (`m - mn[m]`); width of `occurrenceRange` | keep | batch form with derived starts; derived count (clarified rule) | — | — |
| `distinctSubstringsPerPrefix` | row 22 | `distinctSubstringsOnline(s)` | ask | the same unless row 22 takes integer sequences (the automaton takes `string_view` only) | — | — |
| `mutableLcp` | row 23 | `PointUpdateHash::lcp` | ask | same query? | — | — |

## P060 — 05-Mathematics — floorsum, josephus, continued fraction (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `floorSumUnsigned` | row 18 | `floorSum` | ask | takes unsigned `a`, `b` up to 2^64 and returns modulo 2^64, beyond signed `floorSum` | — | — |
| `josephusZeroBased`, `josephusOneBased` | row 19 | `josephus` | remove (row only) | index base | — | — |
| `ceilSum` | row 18 | `floorSum(n, m, a, b + m - 1)` | keep | named special case (ruling 3) | — | — |
| `enumerateQuotients` | row 18 | `forEachQuotientBlock` | keep | stored form beside the callback, as the kept `getPrimeFac()` (clarified rule) | — | — |

## P054 — 05-Mathematics — combinatorics advanced, modular power towers, lattice paths (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `gridPaths` | row 67 | `combiNR(dx + dy, dx)` | keep | returns 0 for unreachable targets, which `combiNR` rejects (clarified rule) | — | — |
| `dyckPaths`, `pathsBelowDiagonal` | row 67 | `catalan`; `ballot` | ask | another name, or general endpoints | — | — |
| `inclusionExclusion` | row 12 | `subsetMobius` | ask | the subset Möbius transform under another name, or a pruned search over items | — | — |
| `zeroPowerZeroConvention` | row 30 | contracts | remove (row only) | not an operation | — | — |
| `stirling1Row`, `stirling2Row`, `eulerianRow`, `partitionCount` | row 12 | Core Poly series | ask | the scope note allows only rows 13–15 and 22 to duplicate Core | — | — |
| `factorialPowerConvert` (row 22 `factorialBasisConvert`); `tetrationMod` (`iteratedPowerMod`) | rows 12, 22, 30 | each other | ask | which row owns each | — | — |

## P037 — 03-Geometry — segment union, minimumenclosingcircle, convex polygon query, lattice geometry, fixed radius queries (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `latticeInDisk`, `latticeInDiskFast` | row 19 | Mathematics rows 34, 55 `gaussCircleCount`, `gaussCircleFast` | ask | same algorithms unless centers are arbitrary | — | — |
| `latticeUnderLine`, `latticeInHalfplanePolygon` | row 19 | `floorSum` after a builder; `latticeInRationalPolygon(halfplaneIntersection(h).vertices)` | ask | builders | — | — |
| `segmentUnionLength`, `orthogonalIntersectionCount` | row 13 | Miscellaneous `intervalUnionMeasure`; DS row 33 `staticRectangleSum` | ask | builders unless real endpoints | — | `06-Miscellaneous/Docs/00-notes.md:24` |
| `convexIntersection`, `cut`; `verifyEnclosing`; `maxPointsInDisk` | rows 15, 14, 54 | `clipConvex`; row 23 `maxDiskDepth` | ask | wrappers unless O(n + m) or O(log n + out) | — | — |

## P034 — 02-Data Structures — sqrttree, persistentarray, rollbackdsu, persistentdsu, offline deletion (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `historySize` beside `snapshot()` | row 09 | each other | ask | duplicate if `snapshot()` returns the history length | — | — |
| `RollbackDSUAggregate::contradictions` | row 09 | row 29 `RollbackWeightedDSU` | ask | meaning | — | — |
| `PersistentArray` | row 35 | row 18 `PersistentSegmentTree` with a trivial monoid | ask | k-ary path copying or the same binary tree | — | — |

## P047 — 04-Graphs — directedmst, dynamic connectivity, blockcuttree, path cover (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `OnlineBridges` | `26-dynamic_connectivity.hpp:11-106` | `IncrementalBridgeConnectivity`: `addEdge`, `twoEdgeConnected`, `connected`, `bridgeCount` | remove (row only) | Changes; second implementation (pinned) | none | `99-all.hpp:23` |
| `isArticulation` | row 27 | `lowlink(g).is_art[v]` | remove (row only) | projected field | — | — |
| `bridgeTree`; `sameBlock`, `biconnectedQuery`; `connectedAfterRemovingVertex`, `connectedAfterRemovingEdge` | row 27 | `simplify(contract(g, low.component, k))`; each other; `cut_components[u] <= 1`, `roots.size() + is_bridge[e] == 1` | ask | builder chain; same query; whole-graph or pair query | — | — |
| `dagMinimumChainCover` | row 31 | `dagMinimumPathCover` on `transitiveClosure` | ask | builder | — | — |
| `blockOf` | row 27 | — | keep | maps a vertex to its block-cut-tree node | — | — |
| `OfflineDynamicConnectivity` | row 26 | DS row 63 `OfflineDeletion` plus `RollbackDSU` | keep | named special case if built on row 63; reword `02-Data Structures/00-index.md:89` | — | — |

## P082 — 07-Strings — suffixtree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `isSubstring` | row 11 | `locate` | keep | named special case (ruling 1) | — | — |

## P072 — 06-Miscellaneous — knapsack advanced, kbest enumeration, fractional programming (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `largeWeightSmallValueKnapsack` | row 29 | `13-knapsack.hpp:97-107` `ValueKnapsackDP::bestWithin` (widen `KnapsackItem::weight`) | ask | takes weights above `INT_MAX`, which `KnapsackItem::weight` rejects | — | — |
| `kLargest` | row 54 | `kSmallest*` with a comparator | keep | mirror of `kSmallest` (ruling 3) | — | — |
| `fractionalBinarySearch` | row 55 | `binSearchRealBracket` with `oracle(t) >= 0` | ask | builder | — | — |
| `treeKnapsack` | row 29 | Graphs row 14 `subtreeMergeDp` | keep | different algorithm, O(n·W) | — | — |

## P035 — 02-Data Structures — weighteddsu, fastset, dsu extensions (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `NoncommutativeWeightedDSU` | row 29 | `WeightedDSU` over a group with a fixed orientation | keep | larger domain: non-commutative groups (clarified rule) | — | — |
| `MergeHistoryForest` | row 65 | `04-Graphs/06-mst.hpp:99` `KruskalReconstruction` weighted by union index | ask | built from a union sequence, while `KruskalReconstruction` needs a weighted graph | — | `Docs/00-notes.md:42` |
| `unionEnumerate` | row 65 | `LinkedDSU::members` before `uniteSets` | keep | derived: the newly merged pairs (clarified rule) | — | — |
| `SparseDSU` | row 65 | `HashMap<K, int>` plus `DSU::makeSet` | ask | builder or larger key domain | — | — |
| `ParityDSU` | row 29 | `WeightedDSU` over Z/2 | ask | alias, or a different (2n-node) algorithm | — | — |
| `ComponentSumDSU` | row 65 | row 09 `RollbackDSUAggregate` | keep | different algorithm (path compression, no undo) | — | — |

## P046 — 04-Graphs — matching general (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `perfectMatchingExists` | row 22 | `2 * size == n` | keep | named special case (ruling 1) | — | — |

## P058 — 05-Mathematics — linear algebra (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `gaussJordan` beside `rref`; `nullspace` | row 16 | each other; the basis of `solveLinear(A, 0)` | ask | same elimination: which survives | — | — |

## P088 — 07-Strings — string periodicity, multiple string, string reconstruction (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `borders`, `periods`, `primitiveRoot`, `borderOccurrenceCounts` | row 27 | `prefixBorders`, `prefixPeriods`, `prefixPeriod(pi, true)`, `prefixOccurrences(pi)` over `prefixFunction(s)` | keep | computed from the string; derived results (clarified rule) | — | `Docs/00-notes.md` (row 27 `primitiveRoot`) |
| `isPrimitive` | row 27 | `n > 0 && prefixPeriod(pi, true) == n` | keep | named special case (ruling 1) | — | — |
| `stringFromPrefixFunction`, `stringFromZ`, `infeasible` | row 31 | `validPrefixFunction(pi, s)`, `zToPrefix` | ask | duplicates unless a minimum-alphabet or lex-min witness | — | — |

## P068 — 06-Miscellaneous — bitset optimization, contestallocator, memoization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `BitsetDP::shiftOr` | row 22 | Core `Bitset::combineShift<Op::Or>(dp, k)` | remove (row only) | another name | — | — |

## P050 — 04-Graphs — stablematching, graphicalsequence, reachability, cactusgraph (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `blockTree`, `isCactus` | row 41 | `blockCutTree`; lowlink blocks | ask | builder and projection unless cactus-specific | — | — |

## P061 — 05-Mathematics — interpolation, matrixtree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `multipointEvaluateGeometric`; `productOfLinearShifts` | row 22 | row 65 `bluesteinTransform`; row 37 entry of the same name | ask | same chirp-z; one owner | — | — |
| row 23 matrix-tree counts (`countSpanningTrees`, `countArborescences`, forests, BEST) | row 23 | Graphs row 53 | ask | one owner (`00-index.md` note gives graph-facing counts to Graphs) | — | — |

## P069 — 06-Miscellaneous — exactcover (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `exactCover` | row 25 | `DancingLinks` plus `addRow` plus `solve` | keep | batch builder over `DancingLinks` (clarified rule) | — | — |

## P030 — 02-Data Structures — persistentsegmenttree (planned)

`18-persistentsegmenttree.hpp` is pinned; both hashes match the ledger.

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `PerSegTree` entries | `18-persistentsegmenttree.hpp:7-287` | `PersistentSegmentTree`: `apply(v, l, r + 1, f)`, `copyRange`, `prod`, `maxRight(v, l, g) - 1`, `minLeft(v, r + 1, g)` | remove (row only) | Changes | none | `KSPerSegTree` (`18:343-457`) builds on it |
| `KSPerSegTree::kthMin` | `18-persistentsegmenttree.hpp:413` | `rangeKth(l, r + 1, k - 1)` | remove (row only) | Changes | none | `maxLeq`, `minGeq` (:467,477) use its descent |

## P032 — 02-Data Structures — lichao, convexhulltrick, radixheap, static range queries, xorbasis (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `staticRangeFrequency` | row 32 | `PointSetRangeFrequency(a).count`, `MergeSortTree::countEqual` | keep | static constant-factor variant (ruling 4) | — | — |
| `queryWitness` beside `query` | rows 21, 22 | each other | ask | `query` is the value field of the witness form | — | — |
| `XorBasis::rank`, `countRepresentable`; `intersection`, `reducedEchelon` | row 62 | `size()`, `1ull << size()`; Core `BitMatrix` | ask | projections; ownership of subspace operations (`01-Core/00-index.md:33`) | — | — |

## P042 — 04-Graphs — all pairs shortest path, shortest path advanced, tree algorithms, hld, levelancestor (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `FloydWarshall::runFloydWarshall` | `12-all_pairs_shortest_path.hpp:21-42` | planned `floydWarshall` | remove (row only) | legacy 1-based format (pinned) | none | `99-all.hpp:14` |
| `SPFA::solve` | `13-shortest_path_advanced.hpp:19-51` | planned `spfa` | remove (row only) | legacy format (pinned) | none | `99-all.hpp:15` |
| `widestPath` | row 13 | `minimaxPath(…, maximum = true)` | keep | mirror of `minimaxPath` (ruling 3) | — | — |
| `radius`, `diameter`, `center`, `periphery`; `treeRadius`, `treeCenter` | rows 12, 14 | fields of the `eccentricities`, `treeEccentricities` results | keep | derived formulas of the eccentricities (clarified rule) | — | — |
| `apspPath`; `rootTree`; `pathRanges`; `pathVertices` | rows 12, 14 | `floydWarshall` path reconstruction; `dfs(g, root)` (no children lists); `HLD::processPath`; `path(g, v).vertices` | ask | projections unless each adds a result | — | — |
| `HLD::addEdge`, `build` | `15-hld.hpp:17-26` | `HLD(Graph, root)` once the row names it | keep | only form today | — | — |
| `EulerTour` | row 14 | `lca_detail::Forest` | keep | public operation; build it on Forest's DFS with one range convention | — | — |

## P066 — 06-Miscellaneous — smalltolarge, hilbertorder, fast io advanced, randomized algorithms, hash families, parallelbinarysearch, cdq (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `HashMap`, `HashSet` | row 19 | DS `61-hashmap.hpp` | remove (row only) | second implementation, same open addressing | — | `Docs/00-notes.md:134` |
| `vectorHash` | row 19 | `CustomHash` range overload | keep | different algorithm: random-base polynomial hash (clarified rule) | — | — |
| `pairHash` | row 19 | `CustomHash{}(p)` | ask | the same mixing as `CustomHash`'s pair overload | — | — |
| `MmapInput`, `SyscallOutput` | row 17 | `FastInput`, `FastOutput` | ask | second parser over another byte source; `03-cpp.md` bans POSIX calls | — | — |
| `mergeSortedVectors`, `kWayMerge` | rows 15, 44 | `std::merge`; row 44 | ask | stdlib and second implementation | — | — |
| `hilbertIndex2D` | row 16 | pinned `Mo::getHilOrd` | keep | row 16 owns Hilbert order (`Docs/00-notes.md:34`); see P067 | — | — |

## P067 — 02-Data Structures — range query offline (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Mo::getHilOrd` | `23-range_query_offline.hpp:19` | Miscellaneous row 16 `hilbertIndex2D` | remove (row only) | second implementation once P066 adds `hilbertIndex2D` (Changes); pinned body stays until P067 rewrites Mo | none | `99-all.hpp:21` |

## P048 — 04-Graphs — cycle basis, planar graph, flow with demands, subgraph enumeration (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `cycleSpaceDimension`, `isIndependentF2` | row 28 | `m - n + connectedComponents(g).count`; `BitMatrix` rank | keep | derived formula; bool form (ruling 1) | — | — |
| `eulerCharacteristic` | row 29 | `n - m + faces().size()` | keep | derived formula (clarified rule) | — | — |
| `countTriangles`, `triangleAggregate` | row 35 | `enumerateTriangles` callback | ask | projections unless cheaper | — | — |

## P062 — 05-Mathematics — game theory, probability (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mex` | row 25 | `06-Miscellaneous/08-sequence_algorithms.hpp:206-214` `mex(a)` | remove (row only) | second implementation | — | `06-Miscellaneous/00-index.md:80` note |
| `grundySum` | row 25 | `nimSum` | ask | `nimSum` of Grundy values (remove) or of whole games (keep) | — | — |
| `nimWin` | row 25 | `nimSum != 0` | keep | named special case (ruling 1) | — | — |

## P077 — 06-Miscellaneous — expression parser (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `evaluateExpression` | row 37 | `evaluatePostfix(shuntingYard(tokenize(s)))` | ask | composition | — | — |

## P059 — 05-Mathematics — numerical methods, integer roots (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `horner` | row 17 | `polyEval` | remove (row only) | another name | — | — |
| `bisectionRoot` | row 17 | `binSearchRealBracket` on the sign of f | keep | root finding with a predicate derived from f (clarified rule) | — | — |
| `isqrtRemainder` | row 21 | `s = isqrt(n), n - s * s` | keep | derived formula (clarified rule) | — | — |

## P076 — 06-Miscellaneous — state space search, game search (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `TranspositionSet` | row 35 | `HashSet` | remove (row only) | another name | — | — |
| `minimax` beside `negamax` | row 36 | each other | ask | same search: which stays | — | — |

## P051 — 04-Graphs — graph decomposition, implicit graph, differenceconstraints, graph connectivity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `coreNumbers`, `degeneracyOrdering`, `kCore` | row 42 | one peeling result with order and cores; `core[v] >= k` | keep | different results (cores, order) and a derived set (clarified rule) | — | — |
| pair `edgeConnectivity`, `edgeDisjointPaths`, edge `mengerCut` | row 45 | `MaxFlow(g)` with unit capacities: `flow`, `pathDecomposition`, `minCutEdges` | keep | graph connectivity with unit capacities built inside (decided 2026-10-11) | — | — |
| global `edgeConnectivity`; `complementShortestPath`; `differenceConstraints` | rows 45, 43, 44 | `stoerWagner`; `complementBfs`; `bellmanFord` after a builder | ask | builders unless distinct algorithms | — | — |

## P063 — 05-Mathematics — optimization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `convexHullTrickDp`, `liChaoDp` | row 27 | DS rows 21, 22 | remove (row only) | second include path ("bridge") | — | — |

## P073 — 06-Miscellaneous — meetinthemiddle, digitdp, subset dp, profile dp, interval dp, bracket sequences (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `countBalanced` | row 53 | `catalanExact`, `ballotExact` | ask | Catalan under another name, or counted by length with 0 for odd lengths | — | — |
| `setPartitionDP` | row 32 | `submaskTransitionDP` over (min, +) | ask | special case or wrapper | — | — |

## P087 — 07-Strings — runs (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `primitiveRoot` | row 26 | row 27 entry | remove (row only) | listed twice | — | — |
| `tandemRepeats`, `squares`, `repetitionsCompressed` | row 26 | one compact form | ask | three names for all squares | — | — |

## P039 — 03-Geometry — circleunion, geometric median, spherical geometry (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `diskIntersectionArea` | row 23 | `diskCoverageArea` with k = n; row 06 `diskOverlapAreaApprox` | ask | named special case or wrapper | — | — |
| unweighted beside weighted forms | row 25 (and row 48, P146) | the weighted form with unit weights | ask | named special case or wrapper | — | — |
| `greatCircleDistanceApprox`, `antipodal` | row 27 | `r * angleApprox` (3D), `-p` | ask | wrappers if inputs are `Point3` | — | — |

## P080 — 06-Miscellaneous — dynamic dp (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `AffineComposition` | row 41 | `00-monoids.hpp:79-80` `RangeAffineRangeSum::composition`, `id` | remove (row only) | second implementation | — | — |

## P089 — 07-Strings — multidimensional matching (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `count2D` | row 32 | size of the occurrence list | ask | cheaper count or projection | — | — |

## P079 — 06-Miscellaneous — scheduling, optimal merge (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `intervalPartitionMachines` | row 39 | `intervalPartitionAssignment` | remove (row only) | projected `.first` (`Docs/00-notes.md:24`) | — | `Docs/09-interval_algorithms.md:94` |
| `optimalMergePattern` | row 40 | `huffmanTree` (internal nodes in merge order) | remove (row only) | same algorithm | — | — |
| `huffmanCost` | row 40 | the tree's cost | ask | projection | — | — |

## P065 — 05-Mathematics — polynomial roots (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `newtonPolish` | row 29 | row 17 `newtonRoot` | ask | same iteration | — | — |

## P071 — 06-Miscellaneous — calendar time, encoding (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `julianDayNumber` | row 27 | `daysFromCivil(…) + 2440588` | remove (row only) | index base | — | — |
| `daysBetween`, `addDays`, `dayOfYear` | row 27 | compositions of `daysFromCivil`, `civilFromDays` | ask | one-line compositions | — | — |

## P038 — 03-Geometry — convex hull updates (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `DynamicUpperHull` | row 20 | row 53 `DynamicHull` (P151) | ask | same algorithm if both are Overmars–van Leeuwen | — | — |

## P041 — 03-Geometry — point set queries (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `halfplaneCount`; `manhattanNearestNeighbour` | row 29 | row 41 `halfspaceRangeCount`; row 21 octant sweep | ask | same query; same sweep | — | — |

## P128 — 03-Geometry — exact predicates (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `orient2dInt` | row 36 | row 01 `orient` | ask | larger domain only for coordinates beyond about 2^62 | — | — |

## P129 — 03-Geometry — segmentintersection (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `countIntersections` | row 12 | `reportIntersections(…).size()` | ask | projection unless a distinct algorithm | — | — |

## P130 — 03-Geometry — polygontriangulation (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `triangulateSimple`; `trapezoidalDecomposition` | row 18 | `monotonePartition` then `triangulateMonotone`; row 34 `TrapezoidalMap` | ask | composition; same structure | — | — |

## P131 — 03-Geometry — circle constructions, linearrangement, visibilitypolygon (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `tangentCircles(point, point, r)`; point and line cases of `apolloniusCircles` | row 24 | `circleCentersApprox`; `circumcircleApprox`, incircle and excircles | ask | named special cases or wrappers | — | — |

## P135 — 03-Geometry — manhattan geometry, geometricmst, voronoi (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `manhattanMst`; `manhattanFarthestPair` | row 21 | `kruskal` over `manhattanMstEdges`; `chebyshevFarthestPair(toChebyshev(p))` | keep | builders over the edges and the Chebyshev transform (clarified rule) | — | `04-Graphs/00-index.md:112`, `04-Graphs/Docs/00-notes.md:25` say row 22 |
| `manhattanClosestPair`; `euclideanBottleneck`; `euclideanMstDense`; `Voronoi::adjacency` | rows 21, 22, 32 | the octant sweep; the largest `euclideanMst` edge; `primDense`; row 31 `Delaunay::adjacency` | ask | projections unless separate algorithms | — | — |

## P132 — 03-Geometry — geometry3d (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Plane3::distanceApprox` | row 26 | the free point–plane `distanceApprox` | remove (row only) | another name | — | — |

## P137 — 03-Geometry — convexhull3d (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `hull3SurfaceArea`, `hull3Volume` | row 37 | row 26 polyhedron area and volume over the hull's faces | keep | computed from the hull itself (clarified rule) | — | — |

## P090 — 08-Python — io, search, number theory, combinatorics, sequences, pypy idioms (planned)

Stdlib functions are existing operations (`04-python.md:15`).

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `lowerBound`, `upperBound`, `bisectKey` | `_02_search` (`00-index.md:10`) | `bisect_left`, `bisect_right` with `key=` | remove (row only) | stdlib under another name | — | — |
| `countRange` | `_02_search` (`00-index.md:10`) | two bisects | keep | derived: the difference of two bisects (clarified rule) | — | — |
| `modPow`, `isqrt`, `floorDiv`, `floorMod`, `divmodFloor` | `_03` (:11) | `pow(a, e, m)`, `math.isqrt`, `//`, `%`, `divmod` | remove (row only) | stdlib under another name | — | `Docs/00-notes.md:15` |
| `comb`, `perm`; `fibonacci` | `_04` (:12) | `math.comb`, `math.perm`; `fibPair(n)[0]` | remove (row only) | stdlib; projection | — | — |
| `prefixSums` | `_05` (:13) | `accumulate(a, initial=0)` | remove (row only) | stdlib under another name | — | — |
| `lisLength` | `_05` (:13) | `len(lisWitness(a))` | keep | different result: the length only (clarified rule) | — | — |
| `modInv` | `_03` | `pow(a, -1, m)` | ask | `tryX` form returning `None`, or a precondition | — | `Docs/00-notes.md:46,51` |
| `binarySearchInt`; `argminUnimodal` | `_02` | C++ `binSearch` mirror or `firstTrue`; `ternaryMinInt` | ask | contracts unstated | — | — |

## P091 — 08-Python — dsu, fenwick, segmenttree, sparsetable, heaps (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `heapMerge`, `nSmallest`, `nLargest` | `_52` (:23) | `heapq.merge`, `nsmallest`, `nlargest` | remove (row only) | stdlib under another name | — | — |
| inlined `SegTreeMin`, `SegTreeMax`, `SegTreeSum`, `SparseTableGcd` (and `_29` range-add variants, P102) | `_08`, `_13` | the generic structures | keep | PyPy constant-factor variants (ruling 4) | — | — |

## P092 — 08-Python — graph (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `dijkstraPath` | `_09` (:17) | `restorePath(dijkstra(…))` | ask | composition | — | — |

## P093 — 08-Python — strings (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `kmpCount` | `_10` (:18) | `len(kmpFind(s, p))` | keep | derived count (clarified rule) | — | — |

## P095 — 08-Python — dp (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `rodCutting` | `_12` (:20) | `knapsackUnbounded(range(1, n + 1), prices, n)` | keep | builder over lengths and prices (clarified rule) | — | — |

## P096 — 08-Python — prime factor (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `primeFactors` | `_14` (:29) | `factorize` flattened | remove (row only) | projection | — | — |
| `primePi` beside `countPrimes` | `_14` | each other | ask | duplicate if both are Lucy | — | — |

## P097 — 08-Python — flow, matching (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `assignmentMin` | `_16` (:31) | `hungarian(a)` | remove (row only) | projection | — | — |

## P098 — 08-Python — tree, rollback, persistent, bitset, wavelet matrix (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `popcount`, `highbit` | `_22` (:37) | `int.bit_count()`, `bit_length() - 1` | remove (row only) | stdlib | — | — |
| `rangeKthSmallest` | `_19` (:34) | `PersistentCountTree(a).kth` | keep | one-shot form of the persistent tree (clarified rule) | — | — |
| `countRange` | `_56` (:62) | `rangeFreq` | remove (row only) | another name | — | — |

## P100 — 08-Python — trie, aho, suffix, eertree, lyndon (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `suffixArrayInt` | `_25` (:40) | `suffixArray(s, upper=None)` | remove (row only) | input format | — | — |

## P101 — 08-Python — offline, randomized, io advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `Rng` | `_27` (:42) | `random.Random` | remove (row only) | stdlib under another name | — | — |
| `randomPermutation` | `_27` (:42) | `rng.sample(range(n), n)` | keep | derived: a sample of `range(n)` (clarified rule) | — | — |
| `FastReader`, `FastWriter`, `tokenizeBytes` | `_28` (:43) | `_01` `FastIO`; `bytes.split` | remove (row only) | second implementation; stdlib | — | — |
| `weightedChoice` | `_27` | `rng.choices(pop, weights)[0]` | ask | stdlib unless the alias method | — | — |

## P102 — 08-Python — lazysegmenttree, ordered multiset, sqrt decomposition, interval map (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `fold` | `_61` (:67) | `functools.reduce(step, m.intervals(l, r), init)` | keep | mirrors the kept C++ `IntervalMap::fold` | — | — |

## P094 — 08-Python — geometry, geometry advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `farthestPair` | `_55` (:61) | `convexDiameter(convexHull(pts))` | ask | builder (follow C++ `farthestPair`, kept) | — | — |

## P104 — 08-Python — twosat, lowlink, eulertrail (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `solve` | `_33` (:48) | `satisfiable` | remove (row only) | mirrors the C++ removal | — | — |
| `setFalse(x)` | `_33` | `setTrue(~x)` | ask | merge into one `setValue` as in C++ | — | — |

## P105 — 08-Python — shortest paths, mincostflow (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `floydPath` beside `restorePathAllPairs` | `_36` (:51) | each other | ask | same result | — | — |

## P106 — 08-Python — number theory advanced, rational (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `limitDenominator` | `_57` (:63) | `Fraction(x).limit_denominator(N)` | remove (row only) | stdlib | — | — |
| `kthRootMod` beside `discreteRoot` | `_38` (:53) | each other | ask | one algorithm (C++ has one) | — | — |

## P109 — 08-Python — geometry float (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `norm`, `dotF`, `crossF` | `_41` (:56) | `math.hypot`; `_11` `dot`, `cross` | remove (row only) | stdlib; same formula | — | — |

## P110 — 08-Python — dp advanced, game theory (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mexOf` | `_58` (:64) | `_05` `mex` | remove (row only) | second implementation | — | — |
| `nimSum` | `_58` (:64) | `reduce(xor, a, 0)` | keep | named special case (clarified rule) | — | — |

## P230 — 08-Python — graph advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `maxIndependentSet` | `_59` (:65) | `maxClique` on the complement | keep | separate operation (ruling 2) | — | — |

## P113 — 02-Data Structures — toptree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `pathCompositeSum` | row 39 | Graphs row 84 (owner, fixed or any root) | remove (row only) | listed twice | — | — |

## P114 — 02-Data Structures — succinctbitvector, succincttree, vanemdeboas, matroid oracle (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `WAryTreeSet` | row 42 | row 36 `FastSet` | remove (row only) | another name | — | — |

## P115 — 02-Data Structures — rangetree, kineticheap, persistentqueue, ordermaintenance (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `RangeTree::count`, `fold` | row 43 | `MergeSortTree::countRange` after an x-sort; `SegmentTree2DSparse::prod` | ask | the merge-sort tree and 2D sparse segment tree after a point-set builder, or a separate structure | — | — |
| `PersistentDeque` | row 47 | row 57 `FingerTree` | ask | duplicate if a finger tree | — | — |

## P122 — 02-Data Structures — linearrmq, commonintervaltree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `strongIntervals` | row 55 | the tree's `nodes` | remove (row only) | projection | — | — |
| `PlusMinusOneRMQ` | row 54 | `04-Graphs/07-lca.hpp:146-178` `EulerLCA` kernel | ask | expose it for `EulerLCA`, or drop it | — | — |

## P138 — 03-Geometry — halfspaceintersection3d (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `feasiblePoint3`, `removeRedundant3` | row 38 | the intersection result | ask | projections unless cheaper | — | — |

## P139 — 03-Geometry — visibilitygraph, kinetic geometry, geometric duality (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `upperEnvelope` | row 41 | `lowerEnvelope` of negated lines | keep | named special case (ruling 3) | — | — |
| `lowerEnvelope` | row 41 | DS row 22 convex hull trick | ask | same structure | — | — |
| `geodesicPath` | row 39 | `funnel` | ask | same algorithm | — | — |
| `shortestPathAmongObstacles` | row 39 | `dijkstra` over `visibilityGraph` | keep | only form (irrational lengths; Graph weights are `lng`) | — | — |

## P140 — 03-Geometry — randomizedlp (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `lp2d`, `lp3d`; `minimumEnclosingBall` (d = 2) | row 42 | `seidelLp` at d = 2, 3; row 14 `minimumEnclosingCircle` | ask | narrowings | — | — |

## P143 — 03-Geometry — weighted voronoi (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `l1Voronoi` | row 46 | `linfVoronoi` after `toChebyshev` | keep | builder: the Chebyshev transform (clarified rule) | — | — |

## P148 — 03-Geometry — alphashape (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `alphaShape2`, `criticalAlphas` | row 50 | views of `alphaComplex2` | ask | projections | — | — |

## P149 — 03-Geometry — polyhedron boolean (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `convexPolyhedronIntersection`; union and difference beside `bspBoolean` | row 51 | a fold of row 38 `clipPolyhedronByPlane`; an operation flag | ask | composition; flag between operations | — | — |

## P150 — 03-Geometry — curve distance (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `continuousFrechet` | row 52 | `frechetMatching` | ask | projection | — | — |

## P152 — 04-Graphs — gomoryhutree, stoerwagner, maximumclique, minimummeancycle (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| exact `minimumVertexCover`; `cliqueCover` | row 49 | complement of exact `maximumIndependentSet`; `exactColoring` of the complement | keep | separate operations (ruling 2) | — | — |

## P153 — 04-Graphs — matroidintersection (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `GraphicMatroid`, `TransversalMatroid`, `PartitionMatroid`, `LinearMatroid` | row 48 | DS row 46 types of the same names | remove (row only) | Graphs 48 consumes row 46 | — | — |
| `matroidUnion` beside `matroidPartition`; row 42 `forestDecomposition` | rows 48, 42 | each other | ask | same algorithm | — | — |

## P155 — 04-Graphs — lct, sensitivity analysis, temporal graph, maximumdensitysubgraph (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `DynamicForest` (`link`, `cut`, `connected`, `lca`, `pathFold`, `pathApply`, `evert`) | row 51 | DS row 37 `LinkCutTree` | ask | same-name subset plus edge-ID keys | — | — |
| `secondBestMst`, `mstForcedEdge`, `mstForbiddenEdge`, `isMstUnique` | row 52 | formulas over `mstEdgeSensitivity` | ask | projections | — | — |

## P158 — 04-Graphs — stnumbering, coloring exact, cycle enumeration, feedback set (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `bipolarOrientation` | row 60 | orient each edge by `st[u] < st[v]` | keep | derived orientation (clarified rule) | — | — |
| `earDecomposition`; `minimumCostCycle` | rows 60, 68 | `chainDecomposition`; `weightedGirth`, `directedGirth` | ask | same algorithm unless negative weights | — | — |

## P160 — 04-Graphs — planarity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `planarEmbedding` | row 62 | `isPlanar` returning row 29 `PlanarEmbedding` | ask | converts `isPlanar`'s embedding to the row 29 half-edge structure: a result shape or a builder | — | — |

## P161 — 04-Graphs — fully dynamic connectivity (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `offlineFallback` | row 63 | row 26 `OfflineDynamicConnectivity` | remove (row only) | another name | — | — |

## P162 — 04-Graphs — weightedblossom (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| minimum and maximum perfect matching pair; `maximumWeightMatching` dense and sparse; row 58 `gabowWeightedMatching` (P163) | rows 64, 58 | a `maximum` flag; each other | ask | flag convention; same algorithm | — | — |

## P166 — 04-Graphs — tjoin, minimumcyclebasis, hamiltonian (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `cycleBasisWeight` | row 72 | sum over `minimumCycleBasis` | keep | derived sum (clarified rule) | — | — |
| `minimumHamiltonianCycle` | row 73 | `travelingSalesman` (settle n ≤ 2) | keep | takes a graph; `travelingSalesman` takes a matrix (clarified rule) | — | — |
| `eulerize` | row 69 | `g` plus `minimumTJoin(odd)` | ask | builder | — | — |

## P168 — 04-Graphs — dynamicmst (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `mstWeightChange` | row 71 | `offlineDynamicMst` | ask | special case or wrapper | — | — |

## P173 — 04-Graphs — rankedmatching, stableroommates, extremevertexsets (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `globalMinCutFromExtremeSets` | row 83 | minimum d(X) over `extremeVertexSets` (X ≠ V) | keep | derived minimum (clarified rule) | — | — |

## P175 — 04-Graphs — dynamictreedp (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `StaticTopTree` (`build`, `pointUpdate`, `query`) | row 84 | DS row 39 `StaticTopTree` (`rootFold`) | remove (row only) | same type | — | — |
| `dynamicDiameter` | row 84 | DS row 39 `TopTree::diameter` | ask | weight updates or link/cut | — | — |

## P177 — 05-Mathematics — dirichlet series (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `dirichletConvolutionPrefix`, `dirichletInversePrefix` | row 32 | `DirichletSeries::mul`, `inv` | remove (row only) | same algorithm, argument and result shape | — | — |

## P181 — 05-Mathematics — fastfactorial, bernoulli (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `factorialPrimePowerValuation`; `wilsonBlock` | row 37 | row 12 `factorialValuation`; `factorialBlockTable` | ask | same quantity in two rows | — | — |
| `faulhaber(k)`; `bernoulliNumber` | row 38 | row 14 `prefixSumOfPolynomial(x^k)`; `bernoulliTable` | ask | same Bernoulli algorithm; single entry of the table | — | — |

## P182 — 05-Mathematics — special functions (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| gamma, log-gamma, erf, erfc, beta, Bessel, Neumann, zeta, complete elliptic integrals | row 39 | `std::tgamma`, `lgamma`, `erf`, `erfc`, `beta`, `cyl_bessel_*`, `cyl_neumann`, `riemann_zeta`, `comp_ellint_1`, `comp_ellint_2` | ask | `std::` already provides them unless the row needs other arguments or precision | — | — |

## P183 — 05-Mathematics — generating functions (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `unlabelledMultiset` | row 40 | `eulerTransform` (multiset construction) | remove (row only) | another name | — | — |
| `labelledSequence` | row 40 | `unlabelledSequence` (1/(1 − A); with counts, through `ogfToEgf`, `egfToOgf`) | ask | the same as `unlabelledSequence` on series (remove), or on counts through the EGF conversions (keep) | — | — |
| `coefficientExtraction` | row 40 | `bostanMori` (`Docs/00-notes.md:172`) | remove (row only) | another name | — | — |
| `labelledSet`, `labelledCycle`, `logarithmicTransform` | row 40 | row 14 `fpsExp`, `fpsLog` with the EGF conversions | ask | wrappers if the constructions take counts | — | — |
| `burnsideCount` | row 40 | — | keep | reuses the caller's fixed-point counts; see P204 | — | — |

## P185 — 05-Mathematics — finite fields (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `GF2k::carrylessMultiply` | row 42 | row 65 `carrylessMultiply` | ask | same kernel in two rows | — | — |
| `isIrreducible` | row 42 | — | keep | owner; see P188 | — | — |

## P186 — 05-Mathematics — determinant advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `circulantMultiply`, `toeplitzMultiply` | row 44 | `13-convolution.hpp:295,317` `cyclicConvolution`, `middleProduct` | remove (row only) | existing operations | — | — |
| `determinantPolynomialMatrix` | row 44 | row 51 `PolyMatrix::det` | remove (row only) | same algorithm, argument shape | — | — |
| `hankelSolve`, `vandermondeSolve` | row 44 | `toeplitzSolve` on reversed rows; row 22 interpolation | ask | builders unless the pivot conditions differ | — | — |

## P188 — 05-Mathematics — polynomial factorization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `isIrreducibleModPrime` | row 46 | row 42 `isIrreducible` | remove (row only) | same test and domain | — | — |
| `composedSum`, `composedProduct` | row 46 | row 61 `*Arithmetic` | keep | mod-p power sums against exact arithmetic over Z; annotate both domains | — | — |

## P190 — 05-Mathematics — combinatorial linear algebra (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `matrixTreeBridge` | row 48 | row 23 | remove (row only) | second include path ("bridge") | — | — |

## P191 — 04-Graphs — graph counting (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| directed `countSpanningTrees` | row 53 | `countArborescences` (Mathematics row 23 name) | remove (row only) | same matrix-tree count; narrow `countSpanningTrees` to undirected | — | — |
| `countWalks` | row 53 | Core `Matrix::pow`; Mathematics 15 `matrixPowerVector` | ask | builder | — | — |

## P192 — 05-Mathematics — integer linear algebra (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `latticeDeterminant` | row 49 | row 35 `latticeDeterminant` (P179, a prerequisite) | remove (row only) | listed twice | — | — |

## P193 — 05-Mathematics — multivariate polynomial (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `MultivariateFps::mul` beside `multivariateConvolution (truncated)` | row 50 | — | keep | total-degree against per-coordinate truncation; annotate "(per-coordinate bounds)" | — | — |

## P195 — 05-Mathematics — relaxed convolution, semiring convolution (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `shortProduct` | row 52 | `13-convolution.hpp:282-287` `truncatedConvolution` | remove (row only) | same result | — | — |
| `maxPlusConvolutionConcave*` | row 53 | `minPlusConvolutionConvex*` on negated input | keep | named special case (ruling 3) | — | — |

## P198 — 05-Mathematics — coding theory (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `locatorBerlekampMassey` | row 56 | `berlekampMassey` on the syndromes | ask | builder | — | — |

## P200 — 05-Mathematics — tableau algorithms, convolution specialized, set power series (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `connectedComponentsBySubset` | row 66 | `SetPowerSeries::log` | remove (row only) | the row defines it so | — | — |
| `SetPowerSeries::mul`; `spsComposition` | row 66 | `24-transform_algorithms.hpp` `subsetConvolution`; `spsPolynomialComposite` | ask | same algorithm in two places | — | — |

## P201 — 05-Mathematics — algebraic numbers (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `composedSumArithmetic`, `composedProductArithmetic` | row 61 | row 46 pair | keep | exact over Z (see P188) | — | — |

## P204 — 05-Mathematics — group algorithms (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `burnsideOrbitCount` | row 64 | row 40 `burnsideCount` over the fixed counts | keep | one-shot form from a group; `burnsideCount` takes the counts (clarified rule) | — | — |

## P205 — 06-Miscellaneous — rollback framework, persistentallocator, external memory (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `kWayMerge` | row 44 | row 15 `mergeSortedVectors` | ask | decided with P066 | — | — |

## P206 — 06-Miscellaneous — probabilistic sketches, streaming algorithms, derandomization (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `StreamReservoir` | row 46 | row 18 `reservoirSample` | ask | duplicate if row 18 is incremental | — | — |

## P207 — 06-Miscellaneous — alphabetic tree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `alphabeticTreeWitness` | row 48 | the `garsiaWachs` tree | remove (row only) | projection | — | — |
| `alphabeticCodeLengths` | row 48 | leaf depths of that tree | ask | projection | — | — |

## P208 — 07-Strings — onlinesuffixtree (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `isSubstring` | row 33 | `locate` | keep | named special case (ruling 1) | — | — |

## P209 — 07-Strings — dynamicaho, approximate matching, palindrome advanced (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `palindromicLength (per prefix)` | row 39 | `13-palindromictree.hpp:91` `palindromicLength()` | remove (row only) | existing operation | — | — |

## P211 — 07-Strings — compressed text index (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `BidirectionalFmIndex::count` | row 36 | `FmIndex::count` | ask | same backward search | — | — |

## P213 — 07-Strings — stringisomorphism, debruijn, zivlempel, linear string algorithms, combinatorial words (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `deBruijnCyclic` | row 41 | `deBruijnSequence` | remove (row only) | another name | — | — |
| `deBruijnLinear` | row 41 | `deBruijnSequence` plus its first n − 1 symbols | keep | different result: the linear sequence (clarified rule) | — | — |
| `isSquareFree` | row 50 | row 27 entry (next to `isCubeFree`) | remove (row only) | listed twice | — | — |
| `injectiveRenamingMatch` | row 40 | `parameterizedMatch` | ask | same query? | — | — |

## P216 — 07-Strings — rindex (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `RIndex::count` | row 46 | `RlFmIndex::count` | ask | same backward search | — | — |

## P218 — 07-Strings — semilocallcs (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `prefixSubstringLcs` | row 48 | `SemiLocalLcs::prefixSubstring` | ask | another name? | — | — |

## P219 — 07-Strings — historical string matching (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `forwardDawg` | row 49 | `{j + 1 - m : SuffixAutomaton(p).matchingStatistics(t)[j] == m}` | keep | different result: occurrences, not matching statistics (clarified rule) | — | — |

## P222 — 08-Python — algebraic, combinatorial species (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `necklaceCountColors` | `_48` (:77) | `polyaNecklace` | ask | same count | — | — |

## P223 — 08-Python — sat, approximation (planned)

| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |
|---|---|---|---|---|---|---|
| `bisectionRoot` | `_50` (:79) | `_02` `binarySearchReal` on the sign | ask | builder | — | — |

## Open questions

Every remaining `ask` above, with the fact the user decides. `/package` and `/prune` put a package's `ask` rows to the user before changing anything.

- **P018** `operator bool` or `isNil` (Poly's `BigInt` needs `isNil`)? `init`, `read`: separate in-place operations? `convTwosComp`, `fromTwosComp`: list as signed limb forms or remove? `FastIintMul`: reuse Core Montgomery and Poly NTT, or keep a kernel in `infint_detail`? Row 07 base 2–36 parse/print, `UInt::fromString`, `isDivisible`, `isPerfectSquare`, `hammingDistance`, `isProbablePrime`: does each get its own algorithm or domain?
- **P019** `operator bool` or `isNil` (as P018); Rational `toDouble` (correctly rounded?) and `parse`, `toString` beside the stream operators.
- **P020** move the floating branches to `solveReal`, `invReal`, `rankReal`, or drop those entries? `StaticMatrix`: is a compile-time shape a different domain? `rowSpace`, `colSpace`: another basis? The 8-lane REDC in `mulRow`, `elimRow`: the same as `Montgomery32` `mul8`, `red8`, or an assert-free lazy variant like Poly's kept `Mont8`? `Matrix(vector, axis)` and the vector products: an empty vector gives 0×0 here and 1×0 or 0×1 from `Matrix(1, n, v)`; is that difference real?
- **P022** GF(2) `-=`, `-`: keep for generic ring code?
- **P023** `berlekampMassey`: is floating T a supported SparseMatrix domain? `toCOO`, `toCSR`: distinct types?
- **P085** `mutableLcp`: the same query as `PointUpdateHash::lcp`? `distinctSubstringsPerPrefix`: does row 22 take integer sequences (the suffix automaton takes `string_view` only)?
- **P054** row 12 Stirling, Eulerian and partition rows beside Core Poly: allowed? `factorialPowerConvert` and `tetrationMod`: which row owns each? Row 67 `dyckPaths`, `pathsBelowDiagonal`: `catalan` and `ballot` under other names, or general endpoints? Row 12 `inclusionExclusion`: the subset Möbius transform under another name, or a pruned search over items?
- **P037** `latticeInDisk`: arbitrary centers? `latticeUnderLine`, `latticeInHalfplanePolygon`, `segmentUnionLength`, `orthogonalIntersectionCount`, `convexIntersection`, `cut`, `verifyEnclosing`, `maxPointsInDisk`: own algorithm or bound, or builders?
- **P034** what `snapshot()` returns; what `contradictions` counts; `PersistentArray`: k-ary or binary path copying?
- **P047** row 27 `bridgeTree`, `sameBlock` and `biconnectedQuery`, `connectedAfterRemoving*` (whole graph or a pair?); row 31 `dagMinimumChainCover`: keep?
- **P072** `fractionalBinarySearch`: a named entry point or `binSearchRealBracket`? `largeWeightSmallValueKnapsack`: widen `KnapsackItem::weight` and remove it, or keep it for weights above `INT_MAX`?
- **P035** `SparseDSU`: a larger key domain or a builder? `ParityDSU`: an alias of `WeightedDSU` over Z/2 or a 2n-node DSU? `MergeHistoryForest`: `KruskalReconstruction` after a builder from the union sequence, or a separate operation?
- **P058** `gaussJordan` or `rref`? `nullspace` beside `solveLinear(A, 0)`?
- **P088** row 31 `stringFromPrefixFunction`, `stringFromZ`, `infeasible`: minimum-alphabet or lex-min witnesses?
- **P061** `multipointEvaluateGeometric` beside `bluesteinTransform`; `productOfLinearShifts` in rows 22 and 37; row 23 matrix-tree counts beside Graphs row 53: which row owns each?
- **P032** `queryWitness` beside `query` (rows 21, 22); `XorBasis::rank`, `countRepresentable`, `intersection`, `reducedEchelon` beside `size()` and Core `BitMatrix`?
- **P042** `apspPath`, `rootTree` (children lists?), `pathRanges`, `pathVertices`: does each add a result?
- **P066** row 17 `MmapInput`, `SyscallOutput` (POSIX calls are banned); `mergeSortedVectors`, `kWayMerge` (P205) beside `std::merge`? Row 19 `pairHash`: the same mixing as `CustomHash`'s pair overload?
- **P048** `countTriangles`, `triangleAggregate`: cheaper than enumeration?
- **P077** `evaluateExpression`; **P076** `minimax` or `negamax`; **P073** `setPartitionDP`; **P087** one of `tandemRepeats`, `squares`, `repetitionsCompressed`; **P079** `huffmanCost`; **P071** `daysBetween`, `addDays`, `dayOfYear`; **P206** `StreamReservoir` (is row 18 incremental?); **P207** `alphabeticCodeLengths`: separate operations or compositions?
- **P050** `blockTree`, `isCactus`: cactus-specific results, or `blockCutTree` and lowlink?
- **P051** global `edgeConnectivity`, `complementShortestPath`, `differenceConstraints`: distinct algorithms?
- **P039** `diskIntersectionArea`; unweighted forms in rows 25 and 48; `greatCircleDistanceApprox`, `antipodal`: named special cases or wrappers?
- **P089** `count2D`: cheaper than the occurrence list?
- **P065** `newtonPolish` beside row 17 `newtonRoot`?
- **P038** and **P151**: one dynamic hull, or two algorithms?
- **P041** `halfplaneCount`, `manhattanNearestNeighbour`: the same query and sweep as rows 41 and 21?
- **P128** `orient2dInt`: for coordinates beyond about 2^62?
- **P129** `countIntersections`; **P130** `triangulateSimple`, `trapezoidalDecomposition`; **P131** `tangentCircles`, `apolloniusCircles` cases; **P135** `manhattanClosestPair`, `euclideanBottleneck` (largest MST edge or a threshold test?), `euclideanMstDense`, `Voronoi::adjacency`: own algorithms or wrappers?
- **P090** `modInv`: `None` for non-units (a `tryX` form) or a precondition? `binarySearchInt`: the `binSearch` mirror (renamed) or `firstTrue`? `argminUnimodal`: Fibonacci search or `ternaryMinInt`?
- **P092** `dijkstraPath`; **P096** `primePi` beside `countPrimes`; **P101** `weightedChoice` (alias method?); **P094** `farthestPair`; **P104** `setFalse` (merge into `setValue`?); **P105** `floydPath`; **P106** `kthRootMod` beside `discreteRoot`; **P222** `necklaceCountColors`; **P223** `bisectionRoot`: separate operations?
- **P115** `PersistentDeque`: a finger tree? `RangeTree::count`, `fold`: the merge-sort tree and 2D sparse segment tree after a point-set builder, or a separate structure? **P122** `PlusMinusOneRMQ`: expose it for `EulerLCA`, or drop it?
- **P138** `feasiblePoint3`, `removeRedundant3`; **P140** `lp2d`, `lp3d`, `minimumEnclosingBall` (d = 2); **P148** `alphaShape2`, `criticalAlphas`; **P149** `convexPolyhedronIntersection`, `bspBoolean`'s operation flag; **P150** `continuousFrechet`; **P139** `lowerEnvelope` beside DS row 22, `geodesicPath` beside `funnel`: own algorithms or views?
- **P153** `matroidUnion`, `matroidPartition`, `forestDecomposition`: one algorithm?
- **P155** `DynamicForest`: keep it as an edge-ID adapter or use `LinkCutTree`? MST sensitivity formulas over `mstEdgeSensitivity`?
- **P158** `earDecomposition`, `minimumCostCycle`; **P162** perfect-matching pair, dense and sparse forms, Gabow (P163); **P166** `eulerize`; **P168** `mstWeightChange`; **P175** `dynamicDiameter`; **P191** `countWalks`: own algorithms or wrappers?
- **P181** `factorialPrimePowerValuation`, `wilsonBlock`, `faulhaber`, `bernoulliNumber`; **P182** the `std::` special functions; **P183** `labelledSet`, `labelledCycle`, `logarithmicTransform`; **P185** `carrylessMultiply` (one owner with row 65); **P186** `hankelSolve`, `vandermondeSolve`; **P198** `locatorBerlekampMassey`; **P200** `SetPowerSeries::mul`, `spsComposition`: distinct, or reuse?
- **P211** and **P216** `count` beside `FmIndex::count`, `RlFmIndex::count`; **P213** `injectiveRenamingMatch`; **P218** `prefixSubstringLcs`: the same queries?
- **P073** `countBalanced`: Catalan under another name, or counted by length with 0 for odd lengths?
- **P160** `planarEmbedding`: converts `isPlanar`'s embedding to the row 29 half-edge structure; a result shape (remove) or a builder (keep)?
- **P060** `floorSumUnsigned`: unsigned `a`, `b` up to 2^64 with the result modulo 2^64; keep it beside signed `floorSum`?
- **P062** `grundySum`: `nimSum` of Grundy values (remove) or of whole games (keep)?
- **P183** `labelledSequence`: on series it equals `unlabelledSequence`; do the row 40 constructions take series or counts?
