# Duplicate and omission re-review 2026-11-10 (P275)

P275 (SUP26) re-reviewed three groups under `03-cpp.md` (One API per operation), as widened on 2026-11-08:

- the pending items of [duplicates.md](../duplicates.md);
- the removals of the 25 `Pxxx: prune duplicates` commits;
- every omission recorded before 2026-11-08.

The decisions that bind it are in the Recorded decisions of [omissions.md](../omissions.md) and [duplicates.md](../duplicates.md), and in [10-decisions.md](../../10-decisions.md) (2026-11-08 One API by context; 2026-11-10 P275 re-review).

## Contracts

### Legacy guard

An item returns to a row only if all three hold:

1. the rule keeps it;
2. a current catalog, judge problem or standard text names it (`OLD`, `97-Legacy` and legacy code never count);
3. it comes back as a call to its owner, under a name that follows `02-structure.md` (Operation names).

Implemented operations that the ledger keeps were re-classified under the rule only. Limits lists the four keeps that cite no source.

### Held items

An item whose source or name is still an open question is not added to its row. It waits in `omissions.md` (Open questions), and its owner's plan note points there.

| Package | Held items |
|---|---|
| P027 | `LazySegmentTree` point combine |
| P043 | `TwoSat::answerLexicographic` |
| P045 | `maxCostFlow` |
| P013, P009, P072 | `multisetPermutationCount`, `TriangleApprox::circumcircle`, `incircle` and Miscellaneous row 29 `knapsack01`: each conforming name is a removed or legacy one |

### User decisions of 2026-11-10

**Re-review.** These are recorded in `omissions.md`.

- **Rare omissions.** Omissions left out only as rare, one-catalog, low-value or of no contest use are re-read too. In `08-Python`, "low PyPy value" still counts as a reason.
- **GR13, GR27 and GR32.** Their 2026-10-28/29 confirmations yield to the 2026-11-08 adoption.
- **CGAL families.** The families the 2026-09-27 Geometry audit excluded return as planned Esoteric rows, owned by new research packages.
- **Open questions.** Item-level questions go to the ledgers' Open questions, and the owning packages settle them.

**`/finish`.** These are recorded in `omissions.md`, `duplicates.md` (Rulings), `02-structure.md` (Tiers) and `10-decisions.md`.

- **NetworkX families.** The families left out of Graphs join planned row 97: communities, link analysis, similarity, summarization, small-world, structural holes, swaps and node classification.
- **CGAL joins.** Spatial Sorting, Polygon Repair, Snap Rounding and Interval Skip List stay joined to the planned rows that own their tasks:

  | CGAL family | Joins |
  |---|---|
  | Spatial Sorting | Geometry row 28 |
  | Polygon Repair | Geometry row 30 |
  | Snap Rounding | Geometry row 33 |
  | Interval Skip List | Data Structures row 68, as `IntervalTree::contains` |

  CGAL 2D mesh refinement joins Geometry row 45 the same way.
- **Tier rule.** A one-problem item that composes a row's own operations stays in that row. A standalone one goes to an Esoteric row.
- **Members.** A type's member beside a free function for the same operation is another context, so the wrapper stays. This settles `Plane3::distanceApprox`, the `DirichletSeries` prefix functions and `BitsetDP::shiftOr` (P068 names the last).
- **Flags.** Every flag that chooses between two operations goes to P010's ask, under one ruling: P010's `longest` and `lexicographic` flags, the MST `maximum` flags and `eulerianTrail`'s `lexicographic` flag.

### Pending ledger items

Before, `duplicates.md` held 176 rows in 98 package sections.

| Decision | Before | After |
|---|---|---|
| ask | 78 | 21 |
| keep | 52 | none (moved) |
| remove | 5 | 11 |
| remove (row only) | 41 | 52 |
| Rows | 176 | 84 |
| Package sections | 98 | 50 |

- **Keeps.** 112 keeps moved to "Kept by the sweep" (151 lines there before, 263 now).
- **Asks.**
  - 11 stay, including P020 `Matrix(vector<T>, bool axis)` (an empty `v` changes the shape), P079 `huffmanCost` and P089 `count2D`.
  - New asks, or keeps reopened as asks:
    - P023 `steadyState`;
    - P088 `primitiveRoot` and `borderOccurrenceCounts`;
    - P010's flags;
    - P051 `complementShortestPath`;
    - P158 `minimumCostCycle`;
    - P166 `minimumHamiltonianCycle`;
    - P181 `wilsonBlock`.
  - Two row-only removals became asks: P068 `BitsetDP::shiftOr` (name only) and P063 `convexHullTrickDp`, `liChaoDp`.
- **Ask to remove** (6):
  - P020 vector products;
  - P020 8-lane REDC, a second implementation of `Montgomery32` `mul8`/`red8`;
  - P096 `primePi`;
  - P101 `weightedChoice`;
  - P105 `floydPath`;
  - P222 `necklaceCountColors`.
- **Ask to remove (row only)** (7):
  - P020 `solveReal`, `invReal`, `rankReal`;
  - P104 `setFalse`;
  - P106 `kthRootMod`;
  - P160 `planarEmbedding`;
  - P206 `StreamReservoir`;
  - P207 `alphabeticCodeLengths`;
  - P213 `injectiveRenamingMatch`.
- **Remove (row only) to keep** (6 rows):
  - `Plane3::distanceApprox` and the Dirichlet prefix functions (members are another context);
  - `circulantMultiply` and `toeplitzMultiply` (row 44 wrappers; SciPy `matmul_toeplitz`);
  - `isIrreducibleModPrime` (a wrapper in another row);
  - `latticeDeterminant` (row 35 lists `gramDeterminant`);
  - P066 `HashMap`, `HashSet`, which were dropped because row 19 already lacks them.
- **Dropped rows.**
  - P042's duplicate `EulerTour`.
  - P205's second `kWayMerge` keep: P066's keep covers rows 15 and 44.
- **Keep stands.** The recorded keep `/=(Matrix)` stands, because MATLAB `mrdivide` names it.

### Prune removals

- All 25 commits were re-read through their `duplicates.md` diffs and their evidence documents' mappings.
- Every removed name stays removed.
- Three removals would return only under their removed names, which guard condition 3 forbids:
  - `multisetPermutationCount` (P013, held);
  - `TriangleApprox::circumcircle`, `incircle` (P009, held);
  - `dagLongestPaths` (P010), in P010's flag ask.

### Omissions

- **Read.** Each folder's dated omission sections and Not adopted lists, plus the Mathematics research-decision sections. These are the about 470 entries the brief counts; the review counted about 490 bullets at HEAD.
- **Two passes.**
  - The first pass read the composition, wrapper, one-liner, same-answer and one-problem reasons.
  - The second pass read the rare-type reasons.
- **Returned to existing rows.** 103 rows got 186 appended entries naming 248 operations, counted as new `missing:` names plus the names in entries appended to planned rows:

  | Folder | Operations | Rows |
  |---|---|---|
  | Data Structures | 68 | 24 |
  | Geometry | 32 | 15 |
  | Graphs | 42 | 19 |
  | Mathematics | 63 | 26 |
  | Miscellaneous | 32 | 11 |
  | Strings | 10 | 7 |
  | Python | 1 | 1 |

  - Every verified row became `partial`, with exactly the returned names after `missing:`.
  - A variant of an existing operation carries its qualifier there, for example `zeroOneBfs ({0, c} weights)`.
- **Reopened packages.** 15 packages went from verified to `in-progress`: P009, P015, P033, P044, P048, P053, P055, P058, P060, P072, P075, P082, P085, P227 and P229.
- **Package notes.** Each reopened package and 45 open ones got one note, "P275 re-review 2026-11-10: implement … in `NN-name.hpp`".
- **New planned Esoteric rows.** 25 rows, owned by 9 new packages, P278–P286:

  | Folder | New rows |
  |---|---|
  | Data Structures | 70–74 |
  | Geometry | 68–76 (eight CGAL families and Grenander distance) |
  | Graphs | 95–97 |
  | Mathematics | 72–75 |
  | Miscellaneous | 57–58 |
  | Strings | 55–56 |

- **False handoffs.** These omissions were recorded as "another row owns it", but that row did not list them. Each now goes to that row: `coprimePairCount`, `harmonicNumberApprox`, `orConvolutionOnline`, `mixedEulerCircuit` and `inclusionTree`. The rest are open questions.
- **Rejected after review.**
  - `offlineSetIntersection`: Miscellaneous row 22 owns it.
  - `GridDSU`: a grid vertex is `r * w + c`, an argument form.
  - `findPrimeFactor`: `pollardRhoBrent` with `isPrime` answers it.
  - `xnorConvolution`: the index complement of `xorConvolution`.
- **Renamed after review.**

  | Proposed | Now |
  |---|---|
  | `divModFloor` | `divMod` (`divModFloor` again since P012, user decision 2026-11-12) |
  | `onlineOrConvolution` | `orConvolutionOnline` |
  | `resistanceDistanceApprox` | `effectiveResistanceApprox` |
  | `allShortestPaths` | `shortestPathsAll` |
  | `lexicographicMaximumMatching` | `maximumMatchingLexicographic` |
  | `lexicographicVertexCover` | `konigVertexCoverLexicographic` |

  - Row 97's floating measures take `Approx`.
  - `ExtendedLiChaoTree` takes row 21's `addLine`, `addSegment` and `query`.
- **Folder records.**
  - Each `Docs/00-notes.md` in 02–08 has a section "P275 re-review 2026-11-10". It lists the returns per row and the items not returned, and it supersedes every earlier bullet naming a returned item.
  - Bullets found by name scan or by the review are annotated in place ("returned by P275 2026-11-10 into row …"): 37 notes bullets and the Graphs `Docs/00-sources.md` Not adopted list. The other folders' Not adopted lists are not annotated (Limits).
- **Source keys.** `BOOST` and `SYMPY` joined the library key list in `09-sources.md`.

## Feature-to-test map

Tests: none, as the brief says. Each part was checked as follows.

| Part | Check | Oracle |
|---|---|---|
| Re-classification | 11 first-pass agents, each read-only and covering one folder; then 5 refutation agents on every proposed return and flip, checking four things: already present, same-context form, source seen, owner and name | Refutation verdicts; two `@reviewer` audits |
| Rare omissions and CGAL rows | 6 second-pass agents | Two `@reviewer` audits |
| Row bookkeeping | Scratch scan: every `missing:` entry is balanced, appears in its row, and does not repeat an existing operation unless it carries a qualifier | No findings after the fixes |
| Counts | Ledger and row recounts from the files against `HEAD` | This document's counts; the second audit reproduced the ledger counts |
| Plan | `plan.py check` | ok |
| Names and links | `03-consistency.py` | No errors |

What the refutation agents changed:

- **Rejected as forms:** `BipartiteMatching::fromGraph`, `IntervalSet::isSameInterval`, `IntervalSet::maxExcluded`, `discreteLogPositive`, `Affine2Approx::applyLine`.
- **Rejected for lack of a source:** `Timer::expiredEvery`, a sorted heap dump, `countXorPairsLess`, Beats `values`.
- **Rejected as already present:** `factorizeSpf` and `divisorsSpf`. Core `fromPowerSums`, `composeExp` and `coefOfRationalFps2` are already owned by Mathematics rows 14 and 40.
- **Moved to another owner:**
  - MovableDSU to row 65.
  - PointSetRangeMajority to row 16.
  - Beats `maxRight`/`minLeft` to `LazySegmentTreeBeats`.
  - IncrementalCentroid, GridDecrementalConnectivity and `cyclicBFlow` to Esoteric rows.
  - `countWalks` to Graphs row 53.
- **Corrected to Operation names**, for example: `binSearchRealGeometric`, `coprimeFactorization`, `eisensteinNormCount`, `minimumPerimeterTriangleApprox`, `MaxFlow::minCutFewestEdges`.

## Commands and results (2026-11-10)

- `python3 '00-Guidelines/13-Plan/plan.py' rank`, `render`, `check`: ok, 285 packages.
- `python3 '96-Local Testing/03-consistency.py'`: no errors.
  - `name_collisions` lists 5. The four from P254 remain: `canonicalForm`, `distanceApprox` and `minimalPolynomial` on different argument types, and `primitiveRoot`, whose meanings P276 settles. The fifth is `polygonsIntersect`: row 17 takes polygons, row 30 takes a `MultiPolygon`.
  - `unsourced_rows` lists Mathematics 43 and 56, keyless since before P254.
- Core defect probe, `g++ -std=c++20 -O2 -march=native`, with `ModInt<4294967291u>`:
  - `Matrix` `+=`, `-=` and scalar `*=` abort at `01-Core/05-modint.hpp:106` (exit 134);
  - the matrix product `*=` fails constant evaluation at `10-matrix.hpp:353`.

## Sources

- The fetched pages are in each folder's `Docs/00-sources.md`, section "Fetched 2026-11-10 (P275 re-review)".
- SciPy `scipy.linalg.matmul_toeplitz` (<https://docs.scipy.org/doc/scipy/reference/generated/scipy.linalg.matmul_toeplitz.html>) backs the `toeplitzMultiply` keep.

## Limits and handoffs

### Limits

- **Pass-two sources.** Pass two had no separate refutation round. Its Fetched lists miss some pages its returns cite: the NetworkX centrality, rich-club and d-separation pages, the maspypy flow files, the Miscellaneous row 57 files, CF 703D, 522D and 220B, and Luogu P7916. Earlier sweeps cite some of them. The owning packages' completeness research fetches and re-checks each one.
- **Main operations only.** These rows carry main operations only:
  - the CGAL rows (Geometry 68–76);
  - the NetworkX rows (Graphs 97);
  - the one-problem rows (Data Structures 71–74, Graphs 95).

  Their packages' step 3 completes the lists and confirms the semantics, for example `GreedySubtractSegmentTree` and the Geometry 70–72 types.
- **Keeps that cite no source.** `Voronoi::adjacency`, `convexPolyhedronIntersection`, `BidirectionalFmIndex::count` and `RIndex::count` are existing keeps. Their owners cite a source or fold them in when they next run.
- **Unannotated bullets.** Not adopted lists outside Graphs `Docs/00-sources.md` are not annotated. The P275 section of each folder's notes supersedes them.
- **Secondary owners.** P259, P260 and P262 own minis or shared support targets of changed rows and carry no P275 note. The full row's owner implements first.
- **Unrecorded candidates.** These were found but never recorded as omissions. They wait for the one-problem `/omission-sweep` pass the P275 note asks the user to plan:
  - NetworkX `is_aperiodic`, `dag_to_branching`, `colliders`, `v_structures`, `transitive_closure_dag`;
  - the maspypy `other/` files, `composition_f_x_plus_1x` and `schroder`;
  - the shortest and thinnest-strip transversals.

### Handoffs

Each handoff below is also in its package's plan note.

- **P020** (Core, Fable): `10-matrix.hpp` recovers MOD as `uint(int(T(-1))) + 1` at lines 127, 295, 306, 353 and 659. `is_mint32` admits any 32-bit ModInt, so MOD ≥ 2^31 fails as the probe shows.
- **P276** (renames):
  - Its Core `invReal`, `rankReal` and `solveReal` items are now remove (row only) in P020's section.
  - New renames:
    - `triangulateSimple` → `triangulatePolygon`;
    - `euclideanMstDense` → `euclideanMstQuadratic`;
    - `frechetMatching` → `continuousFrechetWitness`;
    - the `Int` suffix of `orient2dInt` and `incircleInt`;
    - `weightedBlossomDense`/`Sparse` and `gabowWeightedMatching` → `maximumWeightMatching`, `…Dense`, `…Gabow`;
    - Graphs row 35's `weighted…Clustering` prefixes, `weightedAverageClustering` included;
    - `globalMinCutFromExtremeSets`;
    - `composedSumArithmetic` → `composedSumExact`;
    - `count2D` → `match2DCount`;
    - Core `rref<1>` beside BitMatrix `ref`;
    - Strings row 26 `squares (all occurrences)` → `squaresAll`;
    - row 41's second `deBruijnSequence`;
    - the `08-Python` `_02` and `_48` names, and `_38` `bsgs`/`exBsgs`.
  - New word clashes: `primitiveRoot` (Strings 27 against Mathematics 11), `isSquareFree` and `incircle`.
- **Stale records**, each in its owner's plan note:
  - P018: `01-Core/Docs/07-infint.md` says `nextPrime`/`prevPrime` have no owner.
  - P024: `SparseMatrixMini` lacks `det`, `solveLin` and `pow`.
  - P193: Mathematics row 50 lacks the "(per-coordinate bounds)" note.
- **Folder notes only:**
  - Data Structures row 14 holds one-problem presets in an Advanced header.
  - Gabow–Galil–Spencer–Tarjan arborescence is unmeasured against Graphs row 25.

## History

- 2026-11-10: P275 re-review: 22 read-only agents in three stages wrote the ledgers, 103 rows, 25 new rows and 9 packages; two `@reviewer` audits (14 and 13 findings) were fixed or put to the user (6 decisions); plan check and consistency passed.
