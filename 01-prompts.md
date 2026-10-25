# Session checklist

How to run the work on this library; the package list below is rendered by `plan.py` (other agents follow `AGENTS.md`). Order is the schedule `plan.py rank` keeps (`00-Guidelines/13-Plan/00-index.md`).

## Workflow

Start one fresh session per task with the preset of its work:

- Core full types (`01-Core` non-mini headers, packages tagged `fable`): `claude --agent core-session --effort xhigh`
- Everything else, sweeps included: `claude --agent contest-session --effort high`
- Parallel packages: `python3 '00-Guidelines/13-Plan/plan.py' ready --max 3` picks ready packages from different folders; launch each with `--worktree <name>` added. New worktrees branch from `origin/master` by default (`worktree.baseRef`), so push `master` before launching them.
- Packages with several batches run one batch per session under its batch id (`/package`, Batches); `/package Pxxx` names the next one.

| Skill | Use it | Afterwards |
|---|---|---|
| `/package Pxxx` (or `next`, or a batch id) | Implement or continue a package; `/plan-status` shows progress and the next one | `/finish` |
| `/prune Pxxx` | Remove the wrappers `duplicates.md` lists for a verified package | `/finish` |
| `/duplicate-sweep [02 04 …]` | After many new headers: list wrappers and duplicates in `duplicates.md` | Answer its `ask` items, then `/prune` each verified package it names (`/package` handles planned ones) |
| `/omission-sweep [02 04 … or paths]` | When a folder's packages are mostly done (Mathematics early): list completeness gaps in `omissions.md`. A whole folder starts about 50 agents, so raise "Dynamic workflow size" in `/config` first | `/omission-sweep apply`, then `/package` the packages it reopened or created |
| `/speed-sweep Pxxx [verify]` (or a header path) | In a core-session, for a Core full type: compare it against the fastest Library Checker submissions and write its "Optimality gap" section; `verify` after each package stage that targets the gap | `/package` the targets it names, then commit |
| `/pause [hours]` | Before stepping away from an open session: a handoff plus keep-alive pings (default 2 hours) | Come back and type; a new session loads the handoff |
| `/tidy [scope]` | Every ten or so packages, or after a guideline change: remove stale, restated and superseded text | Answer its questions, then commit |
| `/restyle Pxxx` | Only after the comment cap or closing-brace rule changes | Commit |
| `/finish [Pxxx or batch id]` | After `/package` or `/prune`, in a worktree or the main checkout, even with no gaps reported: closes the work in one pass with one round of questions and lands it on your yes | Push `master` |

Commits: `/finish` commits and lands `/package` and `/prune` work; other skills print their commit command. Pending one-off runs: `/omission-sweep "02-Data Structures/45-kineticheap.hpp" "03-Geometry/26-geometry3d.hpp" "03-Geometry/53-dynamicconvexhull.hpp"`, then `/omission-sweep 05`.

<!-- plan:begin -->

Legend: `[x]` verified, `[ ]` otherwise; **package — batches in order** (model, size): label. Open packages add their status (planned or in-progress) and note; `plan.py show Pxxx` gives a verified package's evidence and note.

### Foundations re-audit

- [x] **P001 — SUP01** (opus, S): Contest testing pipeline and reusable generators/checkers.
- [x] **P002 — C01, C15** (fable, M+L): 01-Core — template, debug, bitset.
- [x] **P003 — SUP05** (opus, S): Refresh and check the standalone contest template.
- [x] **P004 — C02** (fable, L): 01-Core — barrett, montgomery.
- [x] **P005 — C03, C16** (fable, L+L): 01-Core — full and mini modular-integer families.
- [x] **P006 — DS01, DS02** (opus, M+M): 02-Data Structures — dsu, fenwick, segmenttree, sparsetable, prefix sum, sqrt decomposition, ordered set, monotone stack.
- [x] **P007 — GE01** (opus, L): 03-Geometry — point, line segment, polygon, convexhull.
- [x] **P008 — GE02** (opus, L): 03-Geometry — closestpair, circle, rotatingcalipers, coordinate transform.
- [x] **P009 — GE23** (opus, M): 03-Geometry — triangle.
- [x] **P010 — GR01, GR02** (opus, M+L): 04-Graphs — graph, traversal, toposort, dsu, shortest path, mst.
- [x] **P011 — GR03, GR26** (opus, L+M): 04-Graphs — lca, scc, bridges articulation, euleriantrail, functionalgraph.
- [x] **P012 — MA01, MA02** (opus, M+M): 05-Mathematics — mod arithmetic, search algorithms, equation solvers, sieve algorithms, combinatorics, segmentedsieve.
- [x] **P013 — MI01, MI02** (opus, M+M): 06-Miscellaneous — random, customhash, fastio, compression, bit operations, permutation.
- [x] **P014 — MI03, MI14** (opus, M+M): 06-Miscellaneous — sequence algorithms, interval algorithms, offline queries, sorting selection.
- [x] **P015 — MI15, MI16, MI17** (opus, M+M+S): 06-Miscellaneous — enumeration, knapsack, cyclefinding.
- [x] **P016 — ST01, ST02, ST19** (opus, S+M+S): 07-Strings — prefixfunction, z, stringhash, trie, manacher, runlength.
- [x] **P017 — ST03** (opus, L): 07-Strings — aho, suffixarray, palindrome queries.

### Core full types

- [x] **P025 — C11** (fable, XL): 01-Core — poly.
- [x] **P026 — C12** (opus, M): 01-Core — polymini.
- [ ] **P018 — C04** (fable, XL): 01-Core — infint. Status: in-progress. Note: 2026-10-25 re-check, user decisions recorded in Docs/07-infint.md (Optimality re-check): judge hex add 44 ms (#5/40, leaders 9), mul 63 (#6/40, 17), div 200 (#7/29, 28). Route: proven-bound FFT + exact NTT; three stages in C04, one session each, closed by /finish P018 as partial until the last: (1) representation, arena, toChars, P025 handoff; (2) transforms; (3) division. After stage 1 the user runs /speed-sweep P018 (scout: writes the Optimality gap baseline); after stages 2 and 3, /speed-sweep P018 verify; remind the user at each stage's end. toChars added to the row (partial until stage 1). Next: /package P018 in a fresh core-session (stage 1). [evidence](01-Core/Docs/07-infint.md)
- [x] **P019 — C05, C06** (fable, M+M): 01-Core — infintmini, rational.
- [ ] **P020 — C07** (fable, XL): 01-Core — matrix. Status: planned.
- [ ] **P022 — C09, C10** (fable, L+M): 01-Core — bitmatrix, bitmatrixmini. Status: planned. Note: Defects in the pinned 12-bitmatrix.hpp body, fix in the rewrite: operator*=(int) zeroes only for 0 (GF(2) needs o & 1, so a * 2 returns a); BitMatrix(string, axis) leaves fields uninitialised for an axis other than 0 or 1.
- [ ] **P023 — C13** (fable, XL): 01-Core — sparsematrix. Status: planned.
- [ ] **P021 — C08** (opus, M): 01-Core — matrixmini. Status: planned.
- [ ] **P024 — C14** (opus, M): 01-Core — sparsematrixmini. Status: planned.

### Contest core

- [x] **P229 — MI32** (opus, M): 06-Miscellaneous — timer, interactive, grid utilities.
- [x] **P027 — DS04, DS05, DS06, DS47** (opus, M+L+L+M): 02-Data Structures — fenwick tree advanced, monoids, lazysegmenttree, dynamicsegmenttree, interval set.
- [x] **P227 — DS49** (opus, M): 02-Data Structures — hashmap, sortedlist.
- [x] **P052 — MA03, MA04** (opus, L+L): 05-Mathematics — primality factorization, multiplicative functions, modinverse, crt.
- [x] **P231 — SUP06** (opus, S): Memory-capped test execution and Compact-alternative rule.
- [x] **P081 — ST04, ST06, ST33** (opus, L+L+L): 07-Strings — suffixautomaton, sais, editdistance.
- [x] **P044 — GR08** (opus, XL): 04-Graphs — max flow.
- [x] **P043 — GR07, GR10, GR11** (opus, M+L+L): 04-Graphs — twosat, matching bipartite, assignment, dominatortree.
- [x] **P055 — MA06, MA12** (opus, L+L): 05-Mathematics — convolution, transform algorithms.
- [x] **P083 — ST07, ST34** (opus, L+L): 07-Strings — palindromictree, lyndon, minrotation, lcs.
- [x] **P028 — DS07, DS08, DS26** (opus, L+L+M): 02-Data Structures — segtreebeats, segment tree 2d, mergesorttree, aggregation queue.
- [x] **P057 — MA07** (opus, L): 05-Mathematics — polynomial algorithms, linear recurrence.
- [x] **P084 — ST08, ST20, ST23** (opus, L+L+M): 07-Strings — string matching, bitap, subsequence automaton, bwt.
- [x] **P036 — GE03, GE24, GE25** (opus, L+M+L): 03-Geometry — halfplaneintersection, minkowskisum, circle polygon, polygon distance.
- [x] **P029 — DS09, DS10, DS48** (opus, L+L+L): 02-Data Structures — waveletmatrix, treap, heap deque.
- [x] **P045 — GR09** (opus, XL): 04-Graphs — min cost flow.
- [x] **P075 — MI23** (opus, L): 06-Miscellaneous — slope trick.
- [x] **P033 — DS13, DS29** (opus, L+L): 02-Data Structures — trie, cartesiantree, disjointsparsetable, offline rectangle queries.
- [x] **P056 — GR06** (opus, L): 04-Graphs — centroiddecomposition, treeisomorphism.
- [x] **P053 — MA05** (opus, L): 05-Mathematics — discrete log root.
- [x] **P178 — MA17** (opus, L): 05-Mathematics — modular square roots, quadratic congruence.
- [x] **P085 — ST21, ST09** (opus, L+L): 07-Strings — wildcardmatching, lexicographic queries, dynamicstringhash.
- [x] **P060 — MA10** (opus, M): 05-Mathematics — floorsum, josephus, continued fraction.
- [x] **P054 — MA26, MA41, MA52** (opus, L+M+M): 05-Mathematics — combinatorics advanced, modular power towers, lattice paths.
- [x] **P037 — GE05, GE27, GE44** (opus, L+L+M): 03-Geometry — segment union, minimumenclosingcircle, convex polygon query, lattice geometry, fixed radius queries.
- [x] **P034 — DS30, DS31, DS03, DS51** (opus, L+M+L+M): 02-Data Structures — sqrttree, persistentarray, rollbackdsu, persistentdsu, offline deletion.
- [x] **P047 — GR69, GR12, GR28** (opus, L+L+M): 04-Graphs — directedmst, dynamic connectivity, blockcuttree, path cover.
- [x] **P082 — ST05** (opus, XL): 07-Strings — suffixtree.
- [x] **P072 — MI18, MI34** (opus, L+M): 06-Miscellaneous — knapsack advanced, kbest enumeration, fractional programming.
- [x] **P035 — DS25, DS32, DS52** (opus, M+M+M): 02-Data Structures — weighteddsu, fastset, dsu extensions.
- [x] **P238 — SUP07** (opus, M): Header dependency graph, include-derived prerequisites and notebook dependency selection.
- [x] **P247 — SUP16** (opus, M): Parallel test runner and integration build.
- [x] **P239 — SUP08** (opus, L): Documentation site generator, header pages and dependency charts.
- [ ] **P030 — DS23** (opus, XL): 02-Data Structures — persistentsegmenttree. Status: planned.
- [ ] **P032 — DS11, DS27, DS28, DS50** (opus, L+M+L+M): 02-Data Structures — lichao, convexhulltrick, radixheap, static range queries, xorbasis. Status: planned.
- [ ] **P058 — MA08** (opus, L): 05-Mathematics — linear algebra. Status: planned.
- [ ] **P042 — GR04, GR05, GR70** (opus, L+L+S): 04-Graphs — all pairs shortest path, shortest path advanced, tree algorithms, hld, levelancestor. Status: planned.
- [ ] **P048 — GR13, GR27, GR32** (opus, L+L+M): 04-Graphs — cycle basis, planar graph, flow with demands, subgraph enumeration. Status: planned.
- [ ] **P063 — MA14** (opus, L): 05-Mathematics — optimization. Status: planned.
- [ ] **P059 — MA09** (opus, L): 05-Mathematics — numerical methods, integer roots. Status: planned.
- [ ] **P046 — GR25** (opus, XL): 04-Graphs — matching general. Status: planned.
- [ ] **P073 — MI19, MI20, MI21, MI22, MI33** (opus, M+M+L+L+M): 06-Miscellaneous — meetinthemiddle, digitdp, subset dp, profile dp, interval dp, bracket sequences. Status: planned.
- [ ] **P050 — GR35, GR36, GR37, GR38** (opus, M+M+L+M): 04-Graphs — stablematching, graphicalsequence, reachability, cactusgraph. Status: planned.
- [ ] **P061 — MA11** (opus, L): 05-Mathematics — interpolation, matrixtree. Status: planned.
- [ ] **P066 — MI04, MI05, MI06** (opus, M+L+M): 06-Miscellaneous — smalltolarge, hilbertorder, fast io advanced, randomized algorithms, hash families, parallelbinarysearch, cdq. Status: planned.
- [ ] **P062 — MA13** (opus, L): 05-Mathematics — game theory, probability. Status: planned.
- [ ] **P031 — DS24** (opus, XL): 02-Data Structures — balanced bst. Status: planned.
- [ ] **P049 — GR30, GR31, GR34** (opus, L+L+L): 04-Graphs — chordalgraph, edge coloring, graph closure. Status: planned.
- [ ] **P064 — MA27** (opus, XL): 05-Mathematics — simplex. Status: planned.
- [ ] **P079 — MI28, MI29** (opus, M+M): 06-Miscellaneous — scheduling, optimal merge. Status: planned.
- [ ] **P065 — MA29** (opus, XL): 05-Mathematics — polynomial roots. Status: planned.
- [ ] **P040 — GE17** (opus, L): 03-Geometry — spatial index. Status: planned.
- [ ] **P038 — GE28** (opus, XL): 03-Geometry — convex hull updates. Status: planned.
- [ ] **P088 — ST11, ST25** (opus, L+L): 07-Strings — string periodicity, multiple string, string reconstruction. Status: planned.
- [ ] **P068 — MI07** (opus, M): 06-Miscellaneous — bitset optimization, contestallocator, memoization. Status: planned.
- [ ] **P086 — ST35, ST22, ST24** (opus, L+L+L): 07-Strings — regular language, radixtrie, onlinez. Status: planned.
- [ ] **P069 — MI08** (opus, L): 06-Miscellaneous — exactcover. Status: planned.
- [ ] **P067 — DS12** (opus, L): 02-Data Structures — range query offline. Status: planned.
- [ ] **P077 — MI26** (opus, M): 06-Miscellaneous — expression parser. Status: planned.
- [ ] **P076 — MI24, MI25** (opus, L+L): 06-Miscellaneous — state space search, game search. Status: planned.
- [ ] **P051 — GR39, GR58, GR60, GR65** (opus, M+L+M+L): 04-Graphs — graph decomposition, implicit graph, differenceconstraints, graph connectivity. Status: planned.
- [ ] **P087 — ST10** (opus, XL): 07-Strings — runs. Status: planned.
- [ ] **P039 — GE10, GE30, GE18** (opus, L+L+M): 03-Geometry — circleunion, geometric median, spherical geometry. Status: planned.
- [ ] **P074 — GR33** (opus, L): 04-Graphs — steiner tree. Status: planned.
- [ ] **P089 — ST26** (opus, L): 07-Strings — multidimensional matching. Status: planned.
- [ ] **P071 — MI10** (opus, M): 06-Miscellaneous — calendar time, encoding. Status: planned.
- [ ] **P078 — MI27** (opus, L): 06-Miscellaneous — heuristic optimization. Status: planned.
- [ ] **P070 — MI09** (opus, XL): 06-Miscellaneous — satsolver. Status: planned.
- [ ] **P041 — GE32** (opus, XL): 03-Geometry — point set queries. Status: planned.
- [ ] **P128 — GE11** (opus, XL): 03-Geometry — exact predicates. Status: planned.
- [ ] **P130 — GE26** (opus, XL): 03-Geometry — polygontriangulation. Status: planned.
- [ ] **P129 — GE04** (opus, XL): 03-Geometry — segmentintersection. Status: planned.
- [ ] **P131 — GE29, GE16, GE34** (opus, L+L+L): 03-Geometry — circle constructions, linearrangement, visibilitypolygon. Status: planned.
- [ ] **P134 — GE07** (opus, XL): 03-Geometry — delaunay. Status: planned.
- [ ] **P132 — GE31** (opus, XL): 03-Geometry — geometry3d. Status: planned.
- [ ] **P135 — GE09, GE08** (opus, M+L): 03-Geometry — manhattan geometry, geometricmst, voronoi. Status: planned.
- [ ] **P133 — GE06** (opus, XL): 03-Geometry — polygon boolean. Status: planned.
- [ ] **P136 — GE33** (opus, XL): 03-Geometry — pointlocation. Status: planned.
- [ ] **P137 — GE12** (opus, XL): 03-Geometry — convexhull3d. Status: planned.
- [ ] **P117 — DS34, DS38, DS39** (opus, L+L+L): 02-Data Structures — persistentheap, rangeparallelunionfind, rangemode. Status: planned.
- [ ] **P118 — GR29** (opus, L): 04-Graphs — kshortestwalks. Status: planned.

### Python contest layer

- [ ] **P090 — PY01, PY40** (opus, M+S): 08-Python — io, search, number theory, combinatorics, sequences, pypy idioms. Status: planned.
- [ ] **P091 — PY02, PY20, PY41** (opus, M+S+S): 08-Python — dsu, fenwick, segmenttree, sparsetable, heaps. Status: planned.
- [ ] **P092 — PY03** (opus, L): 08-Python — graph. Status: planned.
- [ ] **P093 — PY17** (opus, M): 08-Python — strings. Status: planned.
- [ ] **P095 — PY19** (opus, M): 08-Python — dp. Status: planned.
- [ ] **P098 — PY05, PY36, PY38, PY44** (opus, L+L+M+M): 08-Python — tree, rollback, persistent, bitset, wavelet matrix. Status: planned.
- [ ] **P102 — PY12, PY21, PY48** (opus, L+L+M): 08-Python — lazysegmenttree, ordered multiset, sqrt decomposition, interval map. Status: planned.
- [ ] **P094 — PY18, PY43** (opus, M+M): 08-Python — geometry, geometry advanced. Status: planned.
- [ ] **P096 — PY04** (opus, L): 08-Python — prime factor. Status: planned.
- [ ] **P097 — PY34, PY35** (opus, L+L): 08-Python — flow, matching. Status: planned.
- [ ] **P100 — PY07, PY42** (opus, L+M): 08-Python — trie, aho, suffix, eertree, lyndon. Status: planned.
- [ ] **P099 — PY06, PY37** (opus, L+L): 08-Python — polynomial, matrix. Status: planned.
- [ ] **P101 — PY08, PY11** (opus, M+M): 08-Python — offline, randomized, io advanced. Status: planned.
- [ ] **P103 — PY22, PY39** (opus, L+L): 08-Python — line envelope, dp optimization. Status: planned.
- [ ] **P104 — PY23, PY24, PY25** (opus, M+L+S): 08-Python — twosat, lowlink, eulertrail. Status: planned.
- [ ] **P105 — PY26, PY27** (opus, L+L): 08-Python — shortest paths, mincostflow. Status: planned.
- [ ] **P106 — PY28, PY45** (opus, L+S): 08-Python — number theory advanced, rational. Status: planned.
- [ ] **P107 — PY29** (opus, M): 08-Python — transform algorithms. Status: planned.
- [ ] **P108 — PY30, PY32** (opus, L+M): 08-Python — state search, functionalgraph. Status: planned.
- [ ] **P109 — PY31** (opus, L): 08-Python — geometry float. Status: planned.
- [ ] **P110 — PY33, PY46** (opus, L+S): 08-Python — dp advanced, game theory. Status: planned.
- [ ] **P230 — PY47** (opus, L): 08-Python — graph advanced. Status: planned.

### Specialist and research families

- [ ] **P114 — DS17, DS18, DS21** (opus, L+L+M): 02-Data Structures — succinctbitvector, succincttree, vanemdeboas, matroid oracle. Status: planned.
- [ ] **P185 — MA21** (opus, XL): 05-Mathematics — finite fields. Status: planned.
- [ ] **P179 — MA24** (opus, XL): 05-Mathematics — lattice reduction. Status: planned.
- [ ] **P111 — DS14** (opus, XL): 02-Data Structures — lct. Status: planned.
- [ ] **P119 — DS35** (opus, XL): 02-Data Structures — dynamicbitvector. Status: planned.
- [ ] **P162 — GR45** (opus, XL): 04-Graphs — weightedblossom. Status: planned.
- [ ] **P112 — DS15** (opus, XL): 02-Data Structures — eulertourtree. Status: planned.
- [ ] **P113 — DS16** (opus, XL): 02-Data Structures — toptree. Status: planned.
- [ ] **P123 — DS42** (opus, XL): 02-Data Structures — pqtree. Status: planned.
- [ ] **P138 — GE13** (opus, XL): 03-Geometry — halfspaceintersection3d. Status: planned.
- [ ] **P139 — GE14, GE19, GE20** (opus, L+L+M): 03-Geometry — visibilitygraph, kinetic geometry, geometric duality. Status: planned.
- [ ] **P140 — GE15** (opus, L): 03-Geometry — randomizedlp. Status: planned.
- [ ] **P143 — GE36** (opus, XL): 03-Geometry — weighted voronoi. Status: planned.
- [ ] **P145 — GE37** (opus, XL): 03-Geometry — delaunay3d. Status: planned.
- [ ] **P152 — GR14, GR16, GR23** (opus, L+L+M): 04-Graphs — gomoryhutree, stoerwagner, maximumclique, minimummeancycle. Status: planned. Note: Planned operation (user decision 2026-10-24, from P072): minimumRatioSpanningTree (POJ 2728), 06-Miscellaneous/55 dinkelbach over a dense Prim (06-mst primDense) oracle with weights a - t * b, exact rational result and tree witness; add it to the row 56 operations beside minimumRatioCycle and update 06-Miscellaneous/Docs/00-notes.md (Ownership boundaries) when implemented.
- [ ] **P158 — GR41, GR48, GR49, GR71** (opus, L+L+L+M): 04-Graphs — stnumbering, coloring exact, cycle enumeration, feedback set. Status: planned.
- [ ] **P176 — MA15** (opus, XL): 05-Mathematics — summatory functions. Status: planned.
- [ ] **P181 — MA19** (opus, XL): 05-Mathematics — fastfactorial, bernoulli. Status: planned.
- [ ] **P183 — MA28** (opus, L): 05-Mathematics — generating functions. Status: planned.
- [ ] **P188 — MA30** (opus, XL): 05-Mathematics — polynomial factorization. Status: planned.
- [ ] **P190 — MA32** (opus, XL): 05-Mathematics — combinatorial linear algebra. Status: planned.
- [ ] **P202 — MA48** (opus, XL): 05-Mathematics — multiprecision float. Status: planned.
- [ ] **P211 — ST14** (opus, XL): 07-Strings — compressed text index. Status: planned.
- [ ] **P212 — ST15** (opus, XL): 07-Strings — grammar compression. Status: planned.
- [ ] **P115 — DS22, DS20, DS33, DS53** (opus, L+L+L+L): 02-Data Structures — rangetree, kineticheap, persistentqueue, ordermaintenance. Status: planned.
- [ ] **P116 — DS19** (opus, XL): 02-Data Structures — retroactivequeue. Status: planned.
- [ ] **P120 — DS36** (opus, XL): 02-Data Structures — dynamic wavelet. Status: planned.
- [ ] **P121 — DS37** (opus, XL): 02-Data Structures — sortablesegmenttree. Status: planned.
- [ ] **P122 — DS40, DS41** (opus, L+L): 02-Data Structures — linearrmq, commonintervaltree. Status: planned.
- [ ] **P124 — DS43** (opus, XL): 02-Data Structures — fingertree. Status: planned.
- [ ] **P125 — DS44** (opus, XL): 02-Data Structures — persistent bst. Status: planned.
- [ ] **P126 — DS45** (opus, XL): 02-Data Structures — range sequence queries. Status: planned.
- [ ] **P127 — DS46** (opus, XL): 02-Data Structures — succinct sequence. Status: planned.
- [ ] **P141 — GE22** (opus, XL): 03-Geometry — algebraic geometry. Status: planned.
- [ ] **P142 — GE35** (opus, XL): 03-Geometry — constraineddelaunay. Status: planned.
- [ ] **P144 — GE21** (opus, XL): 03-Geometry — minimumwidthannulus. Status: planned.
- [ ] **P146 — GE38** (opus, XL): 03-Geometry — polygonoffset. Status: planned.
- [ ] **P147 — GE39** (opus, XL): 03-Geometry — minimumenclosingellipse. Status: planned.
- [ ] **P148 — GE40** (opus, XL): 03-Geometry — alphashape. Status: planned.
- [ ] **P149 — GE41** (opus, XL): 03-Geometry — polyhedron boolean. Status: planned.
- [ ] **P150 — GE42** (opus, XL): 03-Geometry — curve distance. Status: planned.
- [ ] **P151 — GE43** (opus, XL): 03-Geometry — dynamicconvexhull. Status: planned.
- [ ] **P153 — GR15** (opus, XL): 04-Graphs — matroidintersection. Status: planned.
- [ ] **P154 — GR17** (opus, XL): 04-Graphs — treedecomposition. Status: planned.
- [ ] **P155 — GR18, GR21, GR24** (opus, L+L+M): 04-Graphs — lct, sensitivity analysis, temporal graph, maximumdensitysubgraph. Status: planned.
- [ ] **P156 — GR20** (opus, XL): 04-Graphs — graph isomorphism. Status: planned.
- [ ] **P157 — GR40** (opus, XL): 04-Graphs — kshortestsimplepaths. Status: planned.
- [ ] **P159 — GR42** (opus, XL): 04-Graphs — triconnectivity. Status: planned.
- [ ] **P160 — GR43** (opus, XL): 04-Graphs — planarity. Status: planned.
- [ ] **P161 — GR44** (opus, XL): 04-Graphs — fully dynamic connectivity. Status: planned.
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
- [ ] **P177 — MA16** (opus, L): 05-Mathematics — dirichlet series. Status: planned.
- [ ] **P180 — MA18** (opus, XL): 05-Mathematics — integer factor advanced. Status: planned.
- [ ] **P182 — MA25** (opus, M): 05-Mathematics — special functions. Status: planned.
- [ ] **P184 — MA20** (opus, L): 05-Mathematics — padic arithmetic, rationalreconstruction. Status: planned.
- [ ] **P186 — MA22** (opus, XL): 05-Mathematics — determinant advanced. Status: planned.
- [ ] **P187 — MA23** (opus, M): 05-Mathematics — contourintegral. Status: planned.
- [ ] **P189 — MA31** (opus, L): 05-Mathematics — nimber. Status: planned.
- [ ] **P191 — GR19** (opus, XL): 04-Graphs — graph counting. Status: planned.
- [ ] **P192 — MA33** (opus, XL): 05-Mathematics — integer linear algebra. Status: planned.
- [ ] **P193 — MA34** (opus, XL): 05-Mathematics — multivariate polynomial. Status: planned.
- [ ] **P194 — MA35** (opus, XL): 05-Mathematics — polynomial matrix. Status: planned.
- [ ] **P195 — MA36, MA37** (opus, L+L): 05-Mathematics — relaxed convolution, semiring convolution. Status: planned. Note: Migration (user decision 2026-10-23, from P072): once 53-semiring_convolution.hpp provides maxPlusConvolutionConcaveArbitrary, 06-Miscellaneous/29-knapsack_advanced.hpp smallWeightKnapsack calls it instead of its knapsack_advanced_detail SMAWK; update Docs/29-knapsack_advanced.md and rerun the 29 tester.
- [ ] **P196 — MA38, MA42, MA44** (opus, L+L+M): 05-Mathematics — discrete log advanced, quadratic integer, zero sum. Status: planned.
- [ ] **P197 — MA39** (opus, L): 05-Mathematics — floor sum polynomial. Status: planned.
- [ ] **P198 — MA40** (opus, XL): 05-Mathematics — coding theory. Status: planned.
- [ ] **P199 — MA43** (opus, L): 05-Mathematics — partizan games. Status: planned.
- [ ] **P200 — MA45, MA50, MA51** (opus, L+L+L): 05-Mathematics — tableau algorithms, convolution specialized, set power series. Status: planned.
- [ ] **P201 — MA46** (opus, XL): 05-Mathematics — algebraic numbers. Status: planned.
- [ ] **P203 — MA47** (opus, XL): 05-Mathematics — ball arithmetic. Status: planned.
- [ ] **P204 — MA49** (opus, XL): 05-Mathematics — group algorithms. Status: planned.
- [ ] **P205 — MI11, MI13** (opus, M+L): 06-Miscellaneous — rollback framework, persistentallocator, external memory. Status: planned.
- [ ] **P206 — MI12** (opus, L): 06-Miscellaneous — probabilistic sketches, streaming algorithms, derandomization. Status: planned.
- [ ] **P207 — MI31** (opus, XL): 06-Miscellaneous — alphabetic tree. Status: planned.
- [ ] **P208 — ST12** (opus, XL): 07-Strings — onlinesuffixtree. Status: planned.
- [ ] **P209 — ST13, ST37, ST16** (opus, L+L+L): 07-Strings — dynamicaho, approximate matching, palindrome advanced. Status: planned.
- [ ] **P210 — ST36** (opus, XL): 07-Strings — dynamicsuffixarray. Status: planned.
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
- [ ] **P232 — ST40** (opus, L): 07-Strings — weightedancestor. Status: planned. Note: Omission sweep 2026-10-19: before choosing a structure, research whether later or more practical weighted-ancestor results for suffix trees exist beyond GLN14 and BKPR21 (citing papers, simplifications, implementations) and record them in 07-Strings/Docs/00-sources.md.
- [ ] **P233 — GE46** (opus, L): 03-Geometry — circle placement. Status: planned. Note: Filed by P037 round 3 (user decision 2026-10-19, 03-Geometry/Docs/00-notes.md); sources in 03-Geometry/Docs/00-sources.md (P037 round-3 audit sources). HPM05 was not fetched and WANG22 only as an abstract; obtain them first.
- [ ] **P234 — GE47** (opus, L): 03-Geometry — enclosing shapes. Status: planned. Note: Filed by P037 round 3 (user decision 2026-10-19, 03-Geometry/Docs/00-notes.md); sources in 03-Geometry/Docs/00-sources.md (P037 round-3 audit sources). Megiddo, OAMB86 and STVWE95 were not read yet; obtain them first.
- [ ] **P235 — GE48** (opus, L): 03-Geometry — rectangle depth. Status: planned. Note: Filed by P037 round 3 (user decision 2026-10-19, 03-Geometry/Docs/00-notes.md); sources in 03-Geometry/Docs/00-sources.md (P037 round-3 audit sources). Güting's J. Algorithms 1984 paper was not readable (paywalled); obtain it first.
- [ ] **P236 — GE49** (opus, M): 03-Geometry — kgon klink. Status: planned. Note: Filed by P037 round 3 (user decision 2026-10-19, 03-Geometry/Docs/00-notes.md); sources in 03-Geometry/Docs/00-sources.md (P037 round-3 audit sources). Area weights are not Monge (15-convex_polygon_query.md); check what AST94 requires before implementing.
- [ ] **P237 — GE50** (opus, L): 03-Geometry — wordram geometry. Status: planned. Note: Filed by P037 round 3 (user decision 2026-10-19, 03-Geometry/Docs/00-notes.md); sources in 03-Geometry/Docs/00-sources.md (P037 round-3 audit sources). Expected to be slower than the comparison-model versions at contest sizes; benchmark and record.

### Final support integration

- [ ] **P224 — SUP02** (opus, S): Whole-library test-runner, aggregate and build integration audit. Status: planned.
- [ ] **P225 — SUP03** (opus, S): Online solution coverage and expansion audit. Status: planned.
- [ ] **P226 — SUP04** (opus, S): Curated notebook, contest notes and printable-build audit. Status: planned. Note: Also: landscape page layout; a CJK fallback font so the template's Chinese motto renders (first build 2026-10-15 showed no glyphs).
- [ ] **P240 — SUP09** (opus, L): Site articles and practice lists: 01-Core. Status: planned.
- [ ] **P241 — SUP10** (opus, XL): Site articles and practice lists: 02-Data Structures. Status: planned.
- [ ] **P242 — SUP11** (opus, XL): Site articles and practice lists: 03-Geometry. Status: planned.
- [ ] **P243 — SUP12** (opus, XL): Site articles and practice lists: 04-Graphs. Status: planned.
- [ ] **P244 — SUP13** (opus, XL): Site articles and practice lists: 05-Mathematics. Status: planned.
- [ ] **P245 — SUP14** (opus, XL): Site articles and practice lists: 06-Miscellaneous. Status: planned.
- [ ] **P246 — SUP15** (opus, XL): Site articles and practice lists: 07-Strings. Status: planned.

<!-- plan:end -->
