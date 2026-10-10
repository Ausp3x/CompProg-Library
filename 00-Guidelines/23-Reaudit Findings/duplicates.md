# Duplicates

Wrappers and second implementations under `00-Guidelines/03-cpp.md`, One API per operation. `/duplicate-sweep` writes the package sections and `/prune Pxxx` consumes them.

## Recorded decisions

Decided by the user on 2026-10-10 and 2026-10-11; binding, and only the user changes them.

### Changes

- **P010** `04-Graphs/01-graph.hpp` gains two static builders, used by the P010, P011 and P044 removals (run P010 first):
  - `Graph::fromAdjacency(adj, directed = false)` for `vector<vector<int>>` and for weighted `vector<vector<pair<int, lng>>>` (unweighted entries get weight 1). It has `adj.size()` vertices. Directed: one arc `u -> v` per entry `v` of `adj[u]`. Undirected: the lists must be symmetric as multisets, with equal weights on mirrored entries and a loop listed twice in its own list; each mirrored pair becomes one edge, so parallel edges keep their multiplicity. The symmetry check stays within the O(n + m) build.
  - `Graph::fromMatrix(w, directed = true)` for a square `vector<vector<lng>>`: one arc `u -> v` of weight `w[u][v]` per nonzero entry; undirected requires a symmetric matrix and adds one edge per nonzero entry with `u <= v`.
- **P044** `04-Graphs/19-max_flow.hpp`: `EdmondsKarp` is removed (its `getMaxFlow` ran Dinic; `augmentFlow` found one BFS path without pushing it). `MaxFlow`, the flow network shared by its algorithms, gains `edmondsKarp(s, t, limit = MAX)` (shortest augmenting paths, O(V * E^2), same limit and resumability contract as `flow`) and `augmentPath(s, t)` (pushes one shortest augmenting path and returns the amount, 0 when `t` is unreachable, O(V + E)). Its row reads `MaxFlow (flow network, signed integer Cap): ... flow (Dinic; limit, resumable), edmondsKarp (limit), augmentPath, capacityScalingDinic (limit), pushRelabel ...`.
- **P013** `06-Miscellaneous/05-binarysearch.hpp` is deleted with its row, tester, evidence document, aggregate includes and plan target; `05-Mathematics/02-search_algorithms.hpp` is the only search header.
- **P030** row 18 (pinned): `PerSegTree` is a second persistent segment tree beside `PersistentSegmentTree`; its entries leave the row and `PersistentSegmentTree` gains `maxRight`/`minLeft` (from `maxR`/`minL`). `KSPerSegTree::kthMin` leaves the row (`rangeKth`); its `cntEql`, `cntRan`, `kthSum`, `maxLeq`, `minGeq` stay as the only persistent forms.
- **P047** row 26 (pinned): `OnlineBridges` leaves the row; `IncrementalBridgeConnectivity` gains `connected` (from `findCC`).
- **P090** `08-Python` row `_05_sequences`: `kadane` and `kadaneCircular` are named `maximumSubarray` and `circularMaximumSubarray`, as in C++ (`04-python.md` naming).
- **P012** `05-Mathematics/Docs/02-search_algorithms.md`: correct the claim that `binSearch` evaluates no endpoint and reaches `INT64_MAX` where `firstTrue`/`lastTrue` cannot; its keep reason is the one in Keeps.
- **P053** row 11: the `modLog` multiplier form `k * a^x = b (mod m)` becomes a multiplier parameter of `discreteLogEx`, not a second function.

### Keeps

Items a reader could mistake for duplicates.

| Package | Item | Reason |
|---|---|---|
| all | Every type alias and the macros `fi`, `se`, `pb` (`FPS`, `ChthollyTree`, `FenwickSet`, `Barrett`, `Montgomery`, `ModFac`, ...) | Type aliases are kept |
| P002 | `Debug::slice` inclusive `[l, r]` | Only form |
| P005 | `ModInt::sqrt` returning -1 | `tryX`/`X` pair with `trySqrt` |
| P006 | `maxZeroSubmatrix` | Larger domain: any nonzero cell blocks; `largestBinaryRectangle` asserts 0/1 |
| P006 | `SqrtDecomp::opeUpdate`, `SparseTable::operator<<` | Only form |
| P011 | `lca_detail::Forest` inclusive `t_in`/`t_out` | Only form |
| P011, P043 | `kosarajuScc`, `kuhn` | Different algorithms from `tarjanScc`, `hopcroftKarp` |
| P012 | `ternSearch`, `ternSearchRealBracket` | Different algorithms from `fibSearch`, `goldenSearchRealBracket` |
| P012 | `binSearch(ok, ng, f)` | Decided 2026-10-11: one orientation-free call for the common ok/ng idiom; the replacement needs `lastTrue` or `firstTrue` by orientation plus a result mapping |
| P012 | `LinearSieve::getPhi`, `getMu`, `getNumDiv`, `getSumDiv` | Reuse the sieve's `spf` table |
| P012 | `PrimeCombinatorics` fields `fac`, `inv_fac` | Data, direct indexing |
| P012, P052 | `solveModEq` `{-1, -1}`, `garner` and `crtMod` returning -1 | Only form |
| P052 | `factorizeTrial`, `isPrimeTrial`, `inversePierce`, `inverseFermat` | Different algorithms |
| P055 | `FastConv::fctGcd`, `fctLcm` | Reuse a caller's `LinearSieve` |
| P055 | `FastConv::fctGcdSlow`, `fctLcmSlow` | Different algorithm (harmonic loops) |
| P013 | global `rng`; `CustomHash` convertible and enum overload | Only shared instance; only hash for those types |
| P023 | `steadyState` | Only form; fixed-iteration power method, kept beside the planned converged solver |
| P042 | `HLD::reorderArray` | Only producer of the segment-tree base array |
| P043 | `TwoSat::ans` | Data behind `answer()` |
| P067 | `Mo::operator<<`, `getHilOrd` | Only form |
| P018, P019 | InfInt infinity flag, `INF` token, saturating conversions | Only signed-infinity arithmetic |
| P032 | `LiChaoTree` beside `LiChaoTreeCompressed` | Different domain |
| P090 | Python maximum-subarray functions | Only maximum-subarray functions in their row (renamed, see Changes) |

## Seed list

The 2026-10-10/11 review, agreed by the user. `/duplicate-sweep` revalidates every item and moves it into its package section; an item it finds a keep reason for becomes an open question.

| Package | Header or row | Items | Replacement |
|---|---|---|---|
| P010 | `04-Graphs/05-shortest_path.hpp` | `Dijkstra` (`runGraph`, `runDijkstra`) | `dijkstra(Graph::fromAdjacency(adj, true), sources)`, `bfsShortestPaths` |
| P011 | `04-Graphs/09-bridges_articulation.hpp` | `Tarjan`, `BridgeAlgo`, `lowlink_detail::graph` | `lowlink(Graph::fromAdjacency(adj))` |
| P011 | `04-Graphs/07-lca.hpp` | `LCA(n, root, adj)`, `lca_detail::fromAdjacency`; `getKthAncestor` | `LCA(Graph::fromAdjacency(adj), root)`; `kthAncestor` |
| P043 | `04-Graphs/23-assignment.hpp` | `Hungarian` (`addEdge`, `solve`, `getAssignment`) | `hungarian(a)` on a filled matrix |
| P043 | `04-Graphs/18-twosat.hpp` | `addClause(a, na, b, nb)`, `solve()` | `addClause(a, b)` with `~x`, `satisfiable()` |
| P044 | `04-Graphs/19-max_flow.hpp` | `EdmondsKarp` | see Changes; `MaxFlow(Graph::fromMatrix(cap)).flow(s, t)`, `minCut(s)` |
| P012 | `05-Mathematics/01-mod_arithmetic.hpp` | `exGcd`; `modPow`'s default modulus `INF64` | `extendedGcd`; an explicit modulus or `intPow` |
| P012 | `05-Mathematics/02-search_algorithms.hpp` | `binSearchReal`, `ternSearchReal`, `goldenSearchReal` | `.x` of the `*RealBracket` functions |
| P012 | `05-Mathematics/03-equation_solvers.hpp` | `solveDioEq` reference overload | `solveDioEq(a, b, c)` |
| P012 | `05-Mathematics/06-segmentedsieve.hpp` | `fromClosed` | half-open constructors |
| P052 | `05-Mathematics/10-crt.hpp` | `superChiRemThm` | `crtScaled` |
| P055 | `05-Mathematics/24-transform_algorithms.hpp` | `FastConv::fctOr`, `fctAnd`, `fctXor` | subset/superset zeta and Mobius, `walshHadamard` |
| P006 | `02-Data Structures/02-fenwick.hpp` | `add1`, `prefixSum1`, `sum1`, `lowerBound1` | zero-based forms |
| P006 | `02-Data Structures/04-sparsetable.hpp` | `queryFast`, `querySlow` | `query`, `fold` |
| P006 | `02-Data Structures/06-sqrt_decomposition.hpp` | `setUpdate` | `set` |
| P013 | `06-Miscellaneous/04-compression.hpp` | `Compression::assign`, `lowerBound`, `upperBound`, `id`, `value` (migrate `c.id` in `08-sequence_algorithms.hpp`; `EncounterCompression::id`/`value` are that struct's only API and stay) | `rebuild`, `rank`, `upperRank`, `index`, `decode` of `CoordinateCompression` (`02-Data Structures/07-ordered_set.hpp`) |
| P013 | `06-Miscellaneous/05-binarysearch.hpp` | whole header | see Changes |
| P014 | `06-Miscellaneous/08-sequence_algorithms.hpp` | `kadane` | `maximumSubarray(a).sum` |
| P016 | `07-Strings/05-manacher.hpp` | `oddInclusive`, `evenInclusive` | `oddInterval`, `evenInterval` |
| P042 | rows `12`, `13` (pinned) | `runFloydWarshall`, `SPFA::solve` (row only) | `floydWarshall`, `spfa` |
| P047 | row `26` (pinned) | `OnlineBridges` (row only) | see Changes |
| P056 | row `16` (pinned) | recursive `par`/`sz` build (row only) | `build` |
| P029 | Data Structures row `17` (pinned) | `WaveletMatrix` legacy names `cntEql`, `cntRan`, `kthMin`, `maxLeq`, `minGeq` (row only) | `rangeFreq`, `kthMax`, `prevValue`, `nextValue` in the same row |
| P030 | Data Structures row `18` (pinned) | `PerSegTree` entries, `KSPerSegTree::kthMin` (row only) | see Changes |
| P090 | `08-Python` row `_02_search` | `bisectKey`; `lowerBound`, `upperBound`, `countRange` unless they do more than `bisect` | `bisect` with `key=` |

## Packages

Not yet written: run `/duplicate-sweep`.

## Open questions

None.
