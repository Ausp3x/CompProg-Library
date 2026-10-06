# Session checklist

Start a session with `/package Pxxx` (Claude Code) or paste `python3 '00-Guidelines/13-plan/plan.py' show Pxxx` output (other agents).
Statuses: planned, in-progress, audit (verified under the previous system; re-audit required), verified.
Order is the default dependency-respecting schedule; `plan.py next` picks the next ready package.

<!-- plan:begin -->

Legend: `[x]` verified, `[ ]` otherwise; **package — batches in order** (model, size): label. Status: planned | in-progress | audit | verified.

### Foundations re-audit

- [x] **P001 — SUP01** (opus, S): Contest testing pipeline and reusable generators/checkers. Status: verified. [evidence](<09-Contest Testing/10-verification.md>)
- [ ] **P002 — C01, C15** (fable, M+L): 01-Core — template, debug, bitset. Status: audit. Note: Restyle pending: apply the 03-cpp.md comment cap with /restyle (contest-session); tests unchanged, no re-audit needed. [evidence](01-Core/21-c01-verification.md) [evidence](01-Core/22-bitset.md)
- [x] **P003 — SUP05** (opus, S): Refresh and check the standalone contest template. Status: verified. [evidence](<97-Online Testing/04-workspace-verification.md>)
- [ ] **P004 — C02** (fable, L): 01-Core — barrett, montgomery. Status: audit. Note: Restyle pending: apply the 03-cpp.md comment cap with /restyle (contest-session); tests unchanged, no re-audit needed. [evidence](01-Core/23-reduction.md)
- [ ] **P005 — C03, C16** (fable, L+L): 01-Core — full and mini modular-integer families. Status: audit. Note: Restyle pending: apply the 03-cpp.md comment cap with /restyle (contest-session); tests unchanged, no re-audit needed. [evidence](01-Core/24-modint.md) [evidence](01-Core/25-modintmini.md)
- [ ] **P006 — DS01, DS02** (opus, M+M): 02-Data Structures — dsu, fenwick, segmenttree, sparsetable, prefix sum, sqrt decomposition, ordered set, monotone stack. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P006.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](<02-Data Structures/90-foundations.md>)
- [ ] **P007 — GE01** (opus, L): 03-Geometry — point, line segment, polygon, convexhull. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P007.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](03-Geometry/90-foundations.md)
- [ ] **P008 — GE02** (opus, L): 03-Geometry — closestpair, circle, rotatingcalipers, coordinate transform. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P008.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](03-Geometry/91-ge02.md)
- [ ] **P009 — GE23** (opus, M): 03-Geometry — triangle. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P009.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](03-Geometry/92-triangle.md)
- [ ] **P010 — GR01, GR02** (opus, M+L): 04-Graphs — graph, traversal, toposort, dsu, shortest path, mst. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P010.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](04-Graphs/90-foundations.md)
- [ ] **P011 — GR03, GR26** (opus, L+M): 04-Graphs — lca, scc, bridges articulation, euleriantrail, functionalgraph. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P011.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](04-Graphs/95-p011.md)
- [ ] **P012 — MA01, MA02** (opus, M+M): 05-Mathematics — mod arithmetic, search algorithms, equation solvers, sieve algorithms, combinatorics, segmentedsieve. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P012.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](05-Mathematics/96-p012.md)
- [ ] **P013 — MI01, MI02** (opus, M+M): 06-Miscellaneous — random, customhash, fastio, compression, binarysearch, bit operations, permutation. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P013.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](06-Miscellaneous/95-p013.md)
- [ ] **P014 — MI03, MI14** (opus, M+M): 06-Miscellaneous — sequence algorithms, interval algorithms, offline queries, sorting selection. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P014.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](06-Miscellaneous/96-p014.md)
- [ ] **P015 — MI15, MI16, MI17** (opus, M+M+S): 06-Miscellaneous — enumeration, knapsack, cyclefinding. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P015.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](06-Miscellaneous/89-p015.md)
- [ ] **P016 — ST01, ST02, ST19** (opus, S+M+S): 07-Strings — prefixfunction, z, stringhash, trie, manacher, runlength. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P016.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](07-Strings/94-p016.md)
- [ ] **P017 — ST03** (opus, L): 07-Strings — aho, suffixarray, palindrome queries. Status: audit. Note: Confirmed /reaudit-review findings: 00-Guidelines/23-reaudit-findings/P017.md. Fix each one in this re-audit (or record why it is rejected in the evidence), then delete that file, remove its row from 00-Guidelines/23-reaudit-findings/00-index.md and clear this note. [evidence](07-Strings/89-p017.md)

### Core full types

- [ ] **P025 — C11** (fable, XL): 01-Core — poly. Status: planned.
- [ ] **P026 — C12** (opus, M): 01-Core — polymini. Status: planned.
- [ ] **P018 — C04** (fable, XL): 01-Core — infint. Status: planned.
- [ ] **P019 — C05, C06** (opus, M+M): 01-Core — infintmini, rational. Status: planned.
- [ ] **P020 — C07** (fable, XL): 01-Core — matrix. Status: planned.
- [ ] **P021 — C08** (opus, M): 01-Core — matrixmini. Status: planned.
- [ ] **P022 — C09, C10** (fable, L+M): 01-Core — bitmatrix, bitmatrixmini. Status: planned.
- [ ] **P023 — C13** (fable, XL): 01-Core — sparsematrix. Status: planned.
- [ ] **P024 — C14** (opus, M): 01-Core — sparsematrixmini. Status: planned.

### Contest core

- [ ] **P229 — MI32** (opus, M): 06-Miscellaneous — timer, interactive, grid utilities. Status: planned. Note: contest utilities
- [ ] **P027 — DS04, DS05, DS06, DS47** (opus, M+L+L+M): 02-Data Structures — fenwick tree advanced, lazysegmenttree, dynamicsegmenttree, interval set. Status: planned.
- [ ] **P227 — DS49** (opus, M): 02-Data Structures — hashmap, sortedlist. Status: planned. Note: hash map and sorted list containers
- [ ] **P052 — MA03, MA04** (opus, L+L): 05-Mathematics — primality factorization, multiplicative functions, modinverse, crt. Status: planned.
- [ ] **P081 — ST04, ST06, ST33** (opus, L+L+L): 07-Strings — suffixautomaton, sais, editdistance. Status: planned.
- [ ] **P043 — GR07, GR10, GR11** (opus, M+L+L): 04-Graphs — twosat, matching bipartite, assignment, dominatortree. Status: planned.
- [ ] **P055 — MA06, MA12** (opus, L+L): 05-Mathematics — convolution, transform algorithms. Status: planned.
- [ ] **P083 — ST07, ST34** (opus, L+L): 07-Strings — palindromictree, lyndon, minrotation, lcs. Status: planned.
- [ ] **P028 — DS07, DS08, DS26** (opus, L+L+M): 02-Data Structures — segtreebeats, segment tree 2d, mergesorttree, aggregation queue. Status: planned.
- [ ] **P044 — GR08** (opus, XL): 04-Graphs — max flow. Status: planned.
- [ ] **P057 — MA07** (opus, L): 05-Mathematics — polynomial algorithms, linear recurrence. Status: planned.
- [ ] **P084 — ST08, ST20, ST23** (opus, L+L+M): 07-Strings — string matching, bitap, subsequence automaton, bwt. Status: planned.
- [ ] **P036 — GE03, GE24, GE25** (opus, L+M+L): 03-Geometry — halfplaneintersection, minkowskisum, circle polygon, polygon distance. Status: planned.
- [ ] **P029 — DS09, DS10, DS48** (opus, L+L+L): 02-Data Structures — waveletmatrix, treap, heap deque. Status: planned.
- [ ] **P045 — GR09** (opus, XL): 04-Graphs — min cost flow. Status: planned.
- [ ] **P075 — MI23** (opus, L): 06-Miscellaneous — slope trick. Status: planned.
- [ ] **P033 — DS13, DS29** (opus, L+L): 02-Data Structures — trie, cartesiantree, disjointsparsetable, offline rectangle queries. Status: planned.
- [ ] **P056 — GR06** (opus, L): 04-Graphs — centroiddecomposition, treeisomorphism. Status: planned.
- [ ] **P053 — MA05** (opus, L): 05-Mathematics — discrete log root. Status: planned.
- [ ] **P178 — MA17** (opus, L): 05-Mathematics — modular square roots, quadratic congruence. Status: planned.
- [ ] **P085 — ST21, ST09** (opus, L+L): 07-Strings — wildcardmatching, lexicographic queries, dynamicstringhash. Status: planned.
- [ ] **P060 — MA10** (opus, M): 05-Mathematics — floorsum, josephus, continued fraction. Status: planned.
- [ ] **P054 — MA26, MA41, MA52** (opus, L+M+M): 05-Mathematics — combinatorics advanced, modular power towers, lattice paths. Status: planned.
- [ ] **P037 — GE05, GE27, GE44** (opus, L+L+M): 03-Geometry — segment union, minimumenclosingcircle, convex polygon query, lattice geometry, fixed radius queries. Status: planned.
- [ ] **P034 — DS30, DS31, DS03, DS51** (opus, L+M+L+M): 02-Data Structures — sqrttree, persistentarray, rollbackdsu, persistentdsu, offline deletion. Status: planned.
- [ ] **P047 — GR69, GR12, GR28** (opus, L+L+M): 04-Graphs — directedmst, dynamic connectivity, blockcuttree, path cover. Status: planned.
- [ ] **P082 — ST05** (opus, XL): 07-Strings — suffixtree. Status: planned.
- [ ] **P072 — MI18, MI34** (opus, L+M): 06-Miscellaneous — knapsack advanced, kbest enumeration, fractional programming. Status: planned.
- [ ] **P035 — DS25, DS32, DS52** (opus, M+M+M): 02-Data Structures — weighteddsu, fastset, dsu extensions. Status: planned.
- [ ] **P046 — GR25** (opus, XL): 04-Graphs — matching general. Status: planned.
- [ ] **P058 — MA08** (opus, L): 05-Mathematics — linear algebra. Status: planned.
- [ ] **P088 — ST11, ST25** (opus, L+L): 07-Strings — string periodicity, multiple string, string reconstruction. Status: planned.
- [ ] **P068 — MI07** (opus, M): 06-Miscellaneous — bitset optimization, contestallocator, memoization. Status: planned.
- [ ] **P050 — GR35, GR36, GR37, GR38** (opus, M+M+L+M): 04-Graphs — stablematching, graphicalsequence, reachability, cactusgraph. Status: planned.
- [ ] **P061 — MA11** (opus, L): 05-Mathematics — interpolation, matrixtree. Status: planned.
- [ ] **P086 — ST35, ST22, ST24** (opus, L+L+L): 07-Strings — regular language, radixtrie, onlinez. Status: planned.
- [ ] **P069 — MI08** (opus, L): 06-Miscellaneous — exactcover. Status: planned.
- [ ] **P030 — DS23** (opus, XL): 02-Data Structures — persistentsegmenttree. Status: planned.
- [ ] **P032 — DS11, DS27, DS28, DS50** (opus, L+M+L+M): 02-Data Structures — lichao, convexhulltrick, radixheap, static range queries, xorbasis. Status: planned.
- [ ] **P042 — GR04, GR05, GR70** (opus, L+L+S): 04-Graphs — all pairs shortest path, shortest path advanced, tree algorithms, hld, levelancestor. Status: planned.
- [ ] **P066 — MI04, MI05, MI06** (opus, M+L+M): 06-Miscellaneous — smalltolarge, hilbertorder, fast io advanced, randomized algorithms, hash families, parallelbinarysearch, cdq. Status: planned.
- [ ] **P067 — DS12** (opus, L): 02-Data Structures — range query offline. Status: planned.
- [ ] **P048 — GR13, GR27, GR32** (opus, L+L+M): 04-Graphs — cycle basis, planar graph, flow with demands, subgraph enumeration. Status: planned.
- [ ] **P062 — MA13** (opus, L): 05-Mathematics — game theory, probability. Status: planned.
- [ ] **P077 — MI26** (opus, M): 06-Miscellaneous — expression parser. Status: planned.
- [ ] **P031 — DS24** (opus, XL): 02-Data Structures — balanced bst. Status: planned.
- [ ] **P049 — GR30, GR31, GR34** (opus, L+L+L): 04-Graphs — chordalgraph, edge coloring, graph closure. Status: planned.
- [ ] **P059 — MA09** (opus, L): 05-Mathematics — numerical methods, integer roots. Status: planned.
- [ ] **P076 — MI24, MI25** (opus, L+L): 06-Miscellaneous — state space search, game search. Status: planned.
- [ ] **P051 — GR39, GR58, GR60, GR65** (opus, M+L+M+L): 04-Graphs — graph decomposition, implicit graph, differenceconstraints, graph connectivity. Status: planned.
- [ ] **P063 — MA14** (opus, L): 05-Mathematics — optimization. Status: planned.
- [ ] **P073 — MI19, MI20, MI21, MI22, MI33** (opus, M+M+L+L+M): 06-Miscellaneous — meetinthemiddle, digitdp, subset dp, profile dp, interval dp, bracket sequences. Status: planned.
- [ ] **P087 — ST10** (opus, XL): 07-Strings — runs. Status: planned.
- [ ] **P039 — GE10, GE30, GE18** (opus, L+L+M): 03-Geometry — circleunion, geometric median, spherical geometry. Status: planned.
- [ ] **P080 — MI30** (opus, L): 06-Miscellaneous — dynamic dp. Status: planned.
- [ ] **P074 — GR33** (opus, L): 04-Graphs — steiner tree. Status: planned.
- [ ] **P064 — MA27** (opus, XL): 05-Mathematics — simplex. Status: planned.
- [ ] **P089 — ST26** (opus, L): 07-Strings — multidimensional matching. Status: planned.
- [ ] **P079 — MI28, MI29** (opus, M+M): 06-Miscellaneous — scheduling, optimal merge. Status: planned.
- [ ] **P065 — MA29** (opus, XL): 05-Mathematics — polynomial roots. Status: planned.
- [ ] **P071 — MI10** (opus, M): 06-Miscellaneous — calendar time, encoding. Status: planned.
- [ ] **P040 — GE17** (opus, L): 03-Geometry — spatial index. Status: planned.
- [ ] **P078 — MI27** (opus, L): 06-Miscellaneous — heuristic optimization. Status: planned.
- [ ] **P038 — GE28** (opus, XL): 03-Geometry — convex hull updates. Status: planned.
- [ ] **P070 — MI09** (opus, XL): 06-Miscellaneous — satsolver. Status: planned.
- [ ] **P041 — GE32** (opus, XL): 03-Geometry — point set queries. Status: planned.
- [ ] **P128 — GE11** (opus, XL): 03-Geometry — exact predicates. Status: planned.
- [ ] **P129 — GE04** (opus, XL): 03-Geometry — segmentintersection. Status: planned.
- [ ] **P130 — GE26** (opus, XL): 03-Geometry — polygontriangulation. Status: planned.
- [ ] **P131 — GE29, GE16, GE34** (opus, L+L+L): 03-Geometry — circle constructions, linearrangement, visibilitypolygon. Status: planned.
- [ ] **P133 — GE06** (opus, XL): 03-Geometry — polygon boolean. Status: planned.
- [ ] **P134 — GE07** (opus, XL): 03-Geometry — delaunay. Status: planned.
- [ ] **P135 — GE09, GE08** (opus, M+L): 03-Geometry — manhattan geometry, geometricmst, voronoi. Status: planned.
- [ ] **P136 — GE33** (opus, XL): 03-Geometry — pointlocation. Status: planned.
- [ ] **P132 — GE31** (opus, XL): 03-Geometry — geometry3d. Status: planned.
- [ ] **P137 — GE12** (opus, XL): 03-Geometry — convexhull3d. Status: planned.
- [ ] **P117 — DS34, DS38, DS39** (opus, L+L+L): 02-Data Structures — persistentheap, rangeparallelunionfind, rangemode. Status: planned.
- [ ] **P118 — GR29** (opus, L): 04-Graphs — kshortestwalks. Status: planned.

### Python contest layer

- [ ] **P090 — PY01, PY40** (opus, M+S): 08-Python — io, search, number theory, combinatorics, sequences, pypy idioms. Status: planned.
- [ ] **P091 — PY02, PY20, PY41** (opus, M+S+S): 08-Python — dsu, fenwick, segmenttree, sparsetable, heaps. Status: planned.
- [ ] **P092 — PY03** (opus, L): 08-Python — graph. Status: planned.
- [ ] **P093 — PY17** (opus, M): 08-Python — strings. Status: planned.
- [ ] **P095 — PY19** (opus, M): 08-Python — dp. Status: planned.
- [ ] **P096 — PY04** (opus, L): 08-Python — prime factor. Status: planned.
- [ ] **P097 — PY34, PY35** (opus, L+L): 08-Python — flow, matching. Status: planned.
- [ ] **P098 — PY05, PY36, PY38, PY44** (opus, L+L+M+M): 08-Python — tree, rollback, persistent, bitset, wavelet matrix. Status: planned.
- [ ] **P099 — PY06, PY37** (opus, L+L): 08-Python — polynomial, matrix. Status: planned.
- [ ] **P100 — PY07, PY42** (opus, L+M): 08-Python — trie, aho, suffix, eertree, lyndon. Status: planned.
- [ ] **P101 — PY08, PY11** (opus, M+M): 08-Python — offline, randomized, io advanced. Status: planned.
- [ ] **P102 — PY12, PY21, PY48** (opus, L+L+M): 08-Python — lazysegmenttree, ordered multiset, sqrt decomposition, interval map. Status: planned.
- [ ] **P094 — PY18, PY43** (opus, M+M): 08-Python — geometry, geometry advanced. Status: planned.
- [ ] **P103 — PY22, PY39** (opus, L+L): 08-Python — line envelope, dp optimization. Status: planned.
- [ ] **P104 — PY23, PY24, PY25** (opus, M+L+S): 08-Python — twosat, lowlink, eulertrail. Status: planned.
- [ ] **P105 — PY26, PY27** (opus, L+L): 08-Python — shortest paths, mincostflow. Status: planned.
- [ ] **P106 — PY28, PY45** (opus, L+S): 08-Python — number theory advanced, rational. Status: planned.
- [ ] **P107 — PY29** (opus, M): 08-Python — transform algorithms. Status: planned.
- [ ] **P108 — PY30, PY32** (opus, L+M): 08-Python — state search, functionalgraph. Status: planned.
- [ ] **P109 — PY31** (opus, L): 08-Python — geometry float. Status: planned.
- [ ] **P110 — PY33, PY46** (opus, L+S): 08-Python — dp advanced, game theory. Status: planned.
- [ ] **P230 — PY47** (opus, L): 08-Python — graph advanced. Status: planned. Note: 08-Python — graph advanced

### Specialist and research families

- [ ] **P111 — DS14** (opus, XL): 02-Data Structures — lct. Status: planned.
- [ ] **P112 — DS15** (opus, XL): 02-Data Structures — eulertourtree. Status: planned.
- [ ] **P113 — DS16** (opus, XL): 02-Data Structures — toptree. Status: planned.
- [ ] **P114 — DS17, DS18, DS21** (opus, L+L+M): 02-Data Structures — succinctbitvector, succincttree, vanemdeboas, matroid oracle. Status: planned.
- [ ] **P115 — DS22, DS20, DS33, DS53** (opus, L+L+L+L): 02-Data Structures — rangetree, kineticheap, persistentqueue, ordermaintenance. Status: planned.
- [ ] **P116 — DS19** (opus, XL): 02-Data Structures — retroactivequeue. Status: planned.
- [ ] **P119 — DS35** (opus, XL): 02-Data Structures — dynamicbitvector. Status: planned.
- [ ] **P120 — DS36** (opus, XL): 02-Data Structures — dynamic wavelet. Status: planned.
- [ ] **P121 — DS37** (opus, XL): 02-Data Structures — sortablesegmenttree. Status: planned.
- [ ] **P122 — DS40, DS41** (opus, L+L): 02-Data Structures — linearrmq, commonintervaltree. Status: planned.
- [ ] **P123 — DS42** (opus, XL): 02-Data Structures — pqtree. Status: planned.
- [ ] **P124 — DS43** (opus, XL): 02-Data Structures — fingertree. Status: planned.
- [ ] **P125 — DS44** (opus, XL): 02-Data Structures — persistent bst. Status: planned.
- [ ] **P126 — DS45** (opus, XL): 02-Data Structures — range sequence queries. Status: planned.
- [ ] **P127 — DS46** (opus, XL): 02-Data Structures — succinct sequence. Status: planned.
- [ ] **P138 — GE13** (opus, XL): 03-Geometry — halfspaceintersection3d. Status: planned.
- [ ] **P139 — GE14, GE19, GE20** (opus, L+L+M): 03-Geometry — visibilitygraph, kinetic geometry, geometric duality. Status: planned.
- [ ] **P140 — GE15** (opus, L): 03-Geometry — randomizedlp. Status: planned.
- [ ] **P141 — GE22** (opus, XL): 03-Geometry — algebraic geometry. Status: planned.
- [ ] **P142 — GE35** (opus, XL): 03-Geometry — constraineddelaunay. Status: planned.
- [ ] **P143 — GE36** (opus, XL): 03-Geometry — weighted voronoi. Status: planned.
- [ ] **P144 — GE21** (opus, XL): 03-Geometry — minimumwidthannulus. Status: planned.
- [ ] **P145 — GE37** (opus, XL): 03-Geometry — delaunay3d. Status: planned.
- [ ] **P146 — GE38** (opus, XL): 03-Geometry — polygonoffset. Status: planned.
- [ ] **P147 — GE39** (opus, XL): 03-Geometry — minimumenclosingellipse. Status: planned.
- [ ] **P148 — GE40** (opus, XL): 03-Geometry — alphashape. Status: planned.
- [ ] **P149 — GE41** (opus, XL): 03-Geometry — polyhedron boolean. Status: planned.
- [ ] **P150 — GE42** (opus, XL): 03-Geometry — curve distance. Status: planned.
- [ ] **P151 — GE43** (opus, XL): 03-Geometry — dynamicconvexhull. Status: planned.
- [ ] **P152 — GR14, GR16, GR23** (opus, L+L+M): 04-Graphs — gomoryhutree, stoerwagner, maximumclique, minimummeancycle. Status: planned.
- [ ] **P153 — GR15** (opus, XL): 04-Graphs — matroidintersection. Status: planned.
- [ ] **P154 — GR17** (opus, XL): 04-Graphs — treedecomposition. Status: planned.
- [ ] **P155 — GR18, GR21, GR24** (opus, L+L+M): 04-Graphs — lct, sensitivity analysis, temporal graph, maximumdensitysubgraph. Status: planned.
- [ ] **P156 — GR20** (opus, XL): 04-Graphs — graph isomorphism. Status: planned.
- [ ] **P157 — GR40** (opus, XL): 04-Graphs — kshortestsimplepaths. Status: planned.
- [ ] **P158 — GR41, GR48, GR49, GR71** (opus, L+L+L+M): 04-Graphs — stnumbering, coloring exact, cycle enumeration, feedback set. Status: planned.
- [ ] **P159 — GR42** (opus, XL): 04-Graphs — triconnectivity. Status: planned.
- [ ] **P160 — GR43** (opus, XL): 04-Graphs — planarity. Status: planned.
- [ ] **P161 — GR44** (opus, XL): 04-Graphs — fully dynamic connectivity. Status: planned.
- [ ] **P162 — GR45** (opus, XL): 04-Graphs — weightedblossom. Status: planned.
- [ ] **P163 — GR22** (opus, XL): 04-Graphs — gabowmatching. Status: planned.
- [ ] **P164 — GR46** (opus, XL): 04-Graphs — costscalingflow. Status: planned.
- [ ] **P165 — GR47** (opus, XL): 04-Graphs — networksimplex. Status: planned.
- [ ] **P166 — GR50, GR53, GR54** (opus, L+L+L): 04-Graphs — tjoin, minimumcyclebasis, hamiltonian. Status: planned.
- [ ] **P167 — GR51** (opus, XL): 04-Graphs — incrementalscc. Status: planned.
- [ ] **P168 — GR52** (opus, XL): 04-Graphs — dynamicmst. Status: planned.
- [ ] **P169 — GR55, GR56, GR62** (opus, L+L+L): 04-Graphs — threeedgecomponents, minimumdiameterspanningtree, tree ordering. Status: planned.
- [ ] **P170 — GR57** (opus, XL): 04-Graphs — dynamicstarmincut. Status: planned.
- [ ] **P171 — GR59, GR72** (opus, XL): 04-Graphs — structured graph, graph class recognition. Status: planned.
- [ ] **P172 — GR61** (opus, XL): 04-Graphs — grouplabeledshortestpath. Status: planned.
- [ ] **P173 — GR63, GR64, GR67** (opus, L+L+L): 04-Graphs — rankedmatching, stableroommates, extremevertexsets. Status: planned.
- [ ] **P174 — GR66** (opus, XL): 04-Graphs — skew symmetric flow. Status: planned.
- [ ] **P175 — GR68** (opus, XL): 04-Graphs — dynamictreedp. Status: planned.
- [ ] **P176 — MA15** (opus, XL): 05-Mathematics — summatory functions. Status: planned.
- [ ] **P177 — MA16** (opus, L): 05-Mathematics — dirichlet series. Status: planned.
- [ ] **P179 — MA24** (opus, XL): 05-Mathematics — lattice reduction. Status: planned.
- [ ] **P180 — MA18** (opus, XL): 05-Mathematics — integer factor advanced. Status: planned.
- [ ] **P181 — MA19** (opus, XL): 05-Mathematics — fastfactorial, bernoulli. Status: planned.
- [ ] **P182 — MA25** (opus, M): 05-Mathematics — special functions. Status: planned.
- [ ] **P183 — MA28** (opus, L): 05-Mathematics — generating functions. Status: planned.
- [ ] **P184 — MA20** (opus, L): 05-Mathematics — padic arithmetic, rationalreconstruction. Status: planned.
- [ ] **P185 — MA21** (opus, XL): 05-Mathematics — finite fields. Status: planned.
- [ ] **P186 — MA22** (opus, XL): 05-Mathematics — determinant advanced. Status: planned.
- [ ] **P187 — MA23** (opus, M): 05-Mathematics — contourintegral. Status: planned.
- [ ] **P188 — MA30** (opus, XL): 05-Mathematics — polynomial factorization. Status: planned.
- [ ] **P189 — MA31** (opus, L): 05-Mathematics — nimber. Status: planned.
- [ ] **P190 — MA32** (opus, XL): 05-Mathematics — combinatorial linear algebra. Status: planned.
- [ ] **P191 — GR19** (opus, XL): 04-Graphs — graph counting. Status: planned.
- [ ] **P192 — MA33** (opus, XL): 05-Mathematics — integer linear algebra. Status: planned.
- [ ] **P193 — MA34** (opus, XL): 05-Mathematics — multivariate polynomial. Status: planned.
- [ ] **P194 — MA35** (opus, XL): 05-Mathematics — polynomial matrix. Status: planned.
- [ ] **P195 — MA36, MA37** (opus, L+L): 05-Mathematics — relaxed convolution, semiring convolution. Status: planned.
- [ ] **P196 — MA38, MA42, MA44** (opus, L+L+M): 05-Mathematics — discrete log advanced, quadratic integer, zero sum. Status: planned.
- [ ] **P197 — MA39** (opus, L): 05-Mathematics — floor sum polynomial. Status: planned.
- [ ] **P198 — MA40** (opus, XL): 05-Mathematics — coding theory. Status: planned.
- [ ] **P199 — MA43** (opus, L): 05-Mathematics — partizan games. Status: planned.
- [ ] **P200 — MA45, MA50, MA51** (opus, L+L+L): 05-Mathematics — tableau algorithms, convolution specialized, set power series. Status: planned.
- [ ] **P201 — MA46** (opus, XL): 05-Mathematics — algebraic numbers. Status: planned.
- [ ] **P202 — MA48** (opus, XL): 05-Mathematics — multiprecision float. Status: planned.
- [ ] **P203 — MA47** (opus, XL): 05-Mathematics — ball arithmetic. Status: planned.
- [ ] **P204 — MA49** (opus, XL): 05-Mathematics — group algorithms. Status: planned.
- [ ] **P205 — MI11, MI13** (opus, M+L): 06-Miscellaneous — rollback framework, persistentallocator, external memory. Status: planned.
- [ ] **P206 — MI12** (opus, L): 06-Miscellaneous — probabilistic sketches, streaming algorithms, derandomization. Status: planned.
- [ ] **P207 — MI31** (opus, XL): 06-Miscellaneous — alphabetic tree. Status: planned.
- [ ] **P208 — ST12** (opus, XL): 07-Strings — onlinesuffixtree. Status: planned.
- [ ] **P209 — ST13, ST37, ST16** (opus, L+L+L): 07-Strings — dynamicaho, approximate matching, palindrome advanced. Status: planned.
- [ ] **P210 — ST36** (opus, XL): 07-Strings — dynamicsuffixarray. Status: planned.
- [ ] **P211 — ST14** (opus, XL): 07-Strings — compressed text index. Status: planned.
- [ ] **P212 — ST15** (opus, XL): 07-Strings — grammar compression. Status: planned.
- [ ] **P213 — ST17, ST18, ST38, ST39** (opus, M+L+L+S): 07-Strings — stringisomorphism, debruijn, zivlempel, linear string algorithms, combinatorial words. Status: planned.
- [ ] **P214 — ST27** (opus, XL): 07-Strings — dynamic suffix automaton. Status: planned.
- [ ] **P215 — ST28** (opus, XL): 07-Strings — dynamicpalindrome. Status: planned.
- [ ] **P216 — ST29** (opus, XL): 07-Strings — rindex. Status: planned.
- [ ] **P217 — ST30** (opus, XL): 07-Strings — dynamiclce. Status: planned.
- [ ] **P218 — ST31** (opus, XL): 07-Strings — semilocallcs. Status: planned.
- [ ] **P219 — ST32** (opus, XL): 07-Strings — historical string matching. Status: planned.
- [ ] **P220 — PY09** (opus, L): 08-Python — exact geometry. Status: planned.
- [ ] **P221 — PY13, PY14** (opus, L+L): 08-Python — succinct, fmindex. Status: planned.
- [ ] **P222 — PY10** (opus, L): 08-Python — algebraic, combinatorial species. Status: planned.
- [ ] **P223 — PY15, PY16** (opus, L+M): 08-Python — sat, approximation. Status: planned.
- [ ] **P228 — GE45** (opus, L): 03-Geometry — ham sandwich. Status: planned. Note: geometric partition theorems

### Final support integration

- [ ] **P224 — SUP02** (opus, S): Whole-library test-runner, aggregate and build integration audit. Status: planned.
- [ ] **P225 — SUP03** (opus, S): Online solution coverage and expansion audit. Status: planned.
- [ ] **P226 — SUP04** (opus, S): Curated notebook, contest notes and printable-build audit. Status: planned.

<!-- plan:end -->
