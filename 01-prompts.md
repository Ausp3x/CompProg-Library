# Prompts for implementation work

Open a session at `/home/Ausp3x/Documents/CompProg Library` (or the current checkout root). Choose the next unchecked package from the **session checklist at the bottom of this file**. The [folder plans](00-Guidelines/13-Work%20Batches/00-index.md) resolve its smaller batch IDs and exact files. Each table groups related smaller algorithms and isolates large families; the guides remain authoritative. This file schedules future work and does not request implementation of every batch now.

## Quick prompts

Recommended: assign one checklist package to a fresh session.

```text
Complete package P001 from the checklist at the bottom of 01-prompts.md.
Follow its batch IDs in the listed order, the shared workflow in that file,
and the relevant repository guidelines. Check off the package only after
all owned features and verification are complete; record a handoff for gaps.
```

Replace `P001` with the next unchecked package. A finished checkbox is a claim
backed by its feature inventory and evidence, not merely by files existing.
The larger XL packages may require continuation sessions under the same ID.

### Starting and continuing a session

1. Open the repository root in a fresh session using your chosen Astra/Ultra configuration, then paste the short package prompt above. The model/reasoning selection belongs in the session settings; the prompt assigns the work.
2. Read only that package's checklist line, the shared workflow above the checklist, its selected batch rows and the relevant guides/inventories. The short prompt applies the same requirements as the full prompt; copying both is unnecessary.
3. Keep debugging and verification in the same session while its context is useful. For a continuation, resume the same package ID and link its durable handoff; start the next package after the current one meets its completion criteria.
4. Run the checklist top to bottom by default. For parallel or out-of-order work, inspect the selected package's prerequisites and give each worker disjoint ownership. A fresh chat does not isolate files in the checkout.

Codex loads repository `AGENTS.md` instructions when a session starts; see the [official instruction-discovery guide](https://developers.openai.com/codex/guides/agents-md/). The package prompt then directs it to this workflow and the selected files. A model or reasoning setting does not replace feature checks, tests or recorded evidence.

### Other assignments

For a longer sequential assignment, request an explicit package range such as P010–P012. Each checkbox still needs separate completion evidence.

You can still assign an individual ownership batch directly:

```text
Implement batch C04 using 01-prompts.md and
00-Guidelines/13-Work Batches/01-core.md. Complete its implementation,
research, tests and appropriate benchmarks under the repository guidelines.
```

Change `C04` to `C07` for full Matrix, `C11` for full Poly, or another ID in the selected folder table. For a modest group:

```text
Complete DS01 and DS02 sequentially using 01-prompts.md and
00-Guidelines/13-Work Batches/02-data_structures.md.
```

For a previous unfinished batch:

```text
Resume batch <ID> using 01-prompts.md, its folder table, the current
inventory and the previous handoff. Inspect the actual files and recorded
evidence, then finish the remaining work. Preserve completed changes.
```

For an unfinished package, use `Resume package Pxxx from 01-prompts.md and its linked handoff; inspect current files and evidence, then finish the remaining owned work.` Keep its checkbox unchecked until all its batches are complete.

### Updating a completed package after a rule change

An existing session should reread the changed guide from disk before editing. Use a scoped maintenance request rather than restarting the package or opening unrelated backlog. The current defaults have one authoritative definition in [C++ integer types and aliases](00-Guidelines/03-cpp.md#integer-types-and-aliases), [namespaces and cohesive headers](00-Guidelines/03-cpp.md#namespaces-and-cohesive-headers), and [compressed style](00-Guidelines/03-cpp.md#compressed-style), including method grouping, compression by logical step and consistent vocabulary. The [Core migration record](00-Guidelines/22-core-migration.md) identifies the applied layout and verification; use current files rather than restoring paths from an older session.

```text
Review completed package Pxxx against the current integer, namespace,
cohesive-header and mini-independence defaults; preserve the applied migration.
Reread 00-Guidelines/03-cpp.md, especially Integer types and aliases
and Namespaces and cohesive headers and Compressed style, plus the current
inventory, selected batch rows and 00-Guidelines/22-core-migration.md.
Apply the defaults only to its owned C++ code and affected test/support code.
Group related methods, compress one logical step at a time, and use clear,
consistent vocabulary. Preserve brace style, member initialization order,
behavior, public names/domains, exact type requirements and legacy originals.
Retain justified size_t/ptrdiff_t uses; do not mechanically change signedness.
Keep standalone examples standalone, with only needed exact local aliases.
Run affected existing checks and update the package evidence to match the
final files. Distinguish previous test/benchmark records from fresh runs.
Pure layout changes do not require new performance comparisons.
Report changes, justified exceptions, checks/results and remaining gaps.
```

P001 owns the contest tools and their fixtures; its C++ generator retains the full unsigned 64-bit seed domain and stays standalone using only needed exact local aliases. For generator changes, run the existing `test_generator_templates_seeded_domain_and_cpp` fixture or the full contest-tool suite; quick mode omits that fixture. C++ style defaults do not change Python conventions or tool protocols.

P002 owns C01/C15 and their tests, now `01-template.hpp`, `02-debug.hpp` and `18-bitset.hpp`; preserve standard-template `size_t` matching, Debug's public namespace and Bitset's documented shift/size domains. Check the affected scalar/accelerated configurations when Bitset code changes. If template/debug changes make the standalone Workspace snapshot stale, report that to its SUP05/P003 owner instead of silently expanding a parallel session's ownership.

P003 owns SUP05: verify the current standalone Workspace against its template/debug inputs with explicit generation and `--check`, plus LOCAL/non-LOCAL behavior. Review the resolved missing-workspace reporting handoff; do not restore stale generated contents.

P004 owns C02: `03-barrett.hpp` and `04-montgomery.hpp` remain independent reusable backends. The co-location rule does not merge them into modint. Preserve their exact domains, APIs, scalar/ISA paths and historical benchmark data; run affected checks if code changes.

P005 now owns C03 **and C16**, in that order: `05-modint.hpp` contains all four full types and `modint_detail`; `06-modintmini.hpp` contains four independent structs. Review both evidence files, full-domain behavior, mini omissions/standalone extraction, context/ID rules and current tests. Preserve `mint` and public full-type names; do not recreate the retired alias wrappers or share mini helpers externally. Run both current tester entries for affected changes.

Other sessions may be editing the same checkout. Keep implementation ownership disjoint, preserve completed migration work, and report shared index/map changes to one integration owner rather than editing those files concurrently. Existing verification is evidence to inspect, not a reason to repeat untouched benchmarks or the whole backlog.

## Full reusable prompt

Replace `[Pxxx]` with the package ID. For a direct batch assignment, replace the first sentence with `Complete batch <ID/list> using <folder batch-table path>.` Reading this file as instructed by a quick prompt applies the same workflow.

```text
Complete package [Pxxx] from the checklist in 01-prompts.md.
A package expands to its listed batch IDs, in order.
Resolve exact target files from the selected table rows and current inventory.
Check prerequisite packages/APIs before editing; implement selected owned scope
only. If a prerequisite is missing, report the exact earlier package to run
and progress on independent owned work without silently taking its ownership.
Complete the selected scope; keep unrelated backlog outside this task.

Read AGENTS.md, the shared principles, the affected folder inventory,
and only the applicable guides. Read migration notes and relevant local
97-Legacy excerpts when auditing transferred code. Open sources and the
decision log only as needed; do not load the entire library or archive.

Inspect the current APIs, dependencies and verification status. Research
the complete relevant feature set and update the inventory for omissions;
its checklist is a starting point, not a limit on completeness. Follow
Optimality, Correctness, Completeness and Elegance in the required profile.

Implement the owned targets, document contracts, and complete meaningful
tests and applicable performance comparisons. Update inventories,
aggregates, dependent paths and source/verification evidence together.
Preserve original legacy references while accounting for their features.
Do not automatically submit solutions to online judges.

Proceed through implementation and verification. Resolve routine choices
using the guidelines; ask only about material unresolved scope/contracts.
Report completed features, commands/results, benchmark evidence where
relevant, and specific remaining gaps. If a handoff is necessary, leave
the API decisions, current stage, failing reproducers and exact next steps
in a durable per-batch note linked from the folder inventory.
```

## Sizing, dependencies and parallel sessions

`S` is a small focused task; `M` groups related ordinary algorithms; `L` is a substantial family; `XL` is a large type or research-heavy family with checkpoints. These labels estimate review/implementation effort, not elapsed time or guaranteed session capacity. Full InfInt, Matrix, SparseMatrix and Poly each have their own batch; their table gives internal stages. A fresh session per checklist package is the default. Its smaller batch IDs are ownership units, so the user does not have to regroup them. Continue the same session while debugging and use a handoff if it outgrows the session.

You can select several IDs for sequential work, including C04 and C07, but those remain two large jobs with independent completion evidence. For Data Structures, DS01 and DS02 cover the first eight foundational targets; choosing a literal half of the folder would also bundle several much larger families. Select by batch scope rather than file count. Batch IDs are labels, not a mandatory execution order: stabilize actual prerequisites first, and read other tables only when a dependency is relevant.

Parallel workers need disjoint implementation ownership and stable shared APIs. Use isolated worktrees or explicit file ownership. Assign one integration owner for shared inventories, aggregates and cross-folder paths; workers provide their corresponding changes for that owner to apply with the implementation. Test suites and integration work remain part of completion. Do not have two agents independently rewrite the same full/mini header or shared support code.

An inventory entry explicitly conditional on a motivating use case begins with a scoped research assessment. Record deferred variants and the reason; do not call an unsupported or impractical feature complete. A later justified split/rename updates the batch map, inventory and dependent paths together. Routine agents should open only this entry point, their selected table/rows and the relevant guides.

## Planned session checklist

Run top to bottom for the default dependency order. One checkbox is one assignable
work package; IDs inside it run sequentially. Related small/medium families are
grouped, while large Core types and research families retain separate packages.
An XL package is a staged job, not a promise that it fits one context window.
Resume its ID until complete. Checked packages link completed verification;
unchecked packages remain planned or have an explicit unfinished handoff.

This plan covers **226 packages, 335 algorithm ownership batches, 428 numbered targets, and five support passes**. The large research tail is intentionally separate so practical contest work does not turn into an unbounded folder task. Per-algorithm tests, judge adapters where useful, documentation and benchmarks belong to each package; the final support passes do not postpone them.

Prerequisites are enforced in this order and recorded in [98-session-plan.json](00-Guidelines/13-Work%20Batches/98-session-plan.json); ordinary sessions need only their checklist line, table rows and relevant inventory. For out-of-order/parallel work, consult that package’s prerequisite list and give workers disjoint target ownership. If scope grows, split a package explicitly and update both maps; do not quietly mark a partial package complete.

### Contest foundations

- [x] **P001 — SUP01**: Contest testing pipeline and reusable generators/checkers. [Verified 2026-09-27](<09-Contest Testing/10-verification.md>).
- [x] **P002 — C01, C15**: 01-Core — template, debug, bitset. [C01 evidence](01-Core/21-c01-verification.md), [C15 evidence](01-Core/22-bitset.md).
- [x] **P003 — SUP05**: Refresh and check the standalone contest template. [Verification and support handoff](<97-Online Testing/04-workspace-verification.md>).
- [x] **P004 — C02**: 01-Core — barrett, montgomery. [Contracts, verification and benchmarks](01-Core/23-reduction.md).
- [x] **P005 — C03, C16**: 01-Core — full and mini modular-integer families. [Full contracts and evidence](01-Core/24-modint.md); [mini contracts and evidence](01-Core/25-modintmini.md).
- [x] **P006 — DS01, DS02**: 02-Data Structures — dsu, fenwick, segmenttree, sparsetable, prefix sum, sqrt decomposition, ordered set, monotone stack. [Contracts, verification and legacy handoffs](<02-Data Structures/90-foundations.md>).
- [x] **P007 — GE01**: 03-Geometry — point, line segment, polygon, convexhull. [Contracts, verification and legacy handoffs](03-Geometry/90-foundations.md).
- [x] **P008 — GE02**: 03-Geometry — closestpair, circle, rotatingcalipers, coordinate transform. [Contracts, verification, sources and benchmarks](03-Geometry/91-ge02.md).
- [x] **P009 — GE23**: 03-Geometry — triangle. [Contracts, sources and verification](03-Geometry/92-triangle.md).
- [x] **P010 — GR01, GR02**: 04-Graphs — graph, traversal, toposort, dsu, shortest path, mst. [Contracts, verification and scope boundaries](04-Graphs/90-foundations.md).
- [x] **P011 — GR03, GR26**: 04-Graphs — lca, scc, bridges articulation, euleriantrail, functionalgraph. [Contracts, verification and benchmarks](04-Graphs/95-p011.md).
- [x] **P012 — MA01, MA02**: 05-Mathematics — mod arithmetic, search algorithms, equation solvers, sieve algorithms, combinatorics, segmentedsieve. [Contracts and verification](05-Mathematics/96-p012.md).
- [x] **P013 — MI01, MI02**: 06-Miscellaneous — random, customhash, fastio, compression, binarysearch, bit operations, permutation. [Evidence](06-Miscellaneous/95-p013.md).
- [x] **P014 — MI03, MI14**: 06-Miscellaneous — sequence algorithms, interval algorithms, offline queries, sorting selection. [Contracts, verification and benchmarks](06-Miscellaneous/96-p014.md).
- [x] **P015 — MI15, MI16, MI17**: 06-Miscellaneous — enumeration, knapsack, cyclefinding. [Contracts, verification and benchmarks](06-Miscellaneous/89-p015.md).
- [x] **P016 — ST01, ST02, ST19**: 07-Strings — prefixfunction, z, stringhash, trie, manacher, runlength. [Contracts and verification](07-Strings/94-p016.md).
- [x] **P017 — ST03**: 07-Strings — aho, suffixarray, palindrome queries. [Evidence](07-Strings/89-p017.md).

### Full Core types and matching minis

- [ ] **P018 — C04**: 01-Core — infint. **XL: staged, separate job.**
- [ ] **P019 — C05, C06**: 01-Core — infintmini, rational.
- [ ] **P020 — C07**: 01-Core — matrix. **XL: staged, separate job.**
- [ ] **P021 — C08**: 01-Core — matrixmini.
- [ ] **P022 — C09, C10**: 01-Core — bitmatrix, bitmatrixmini.
- [ ] **P023 — C13**: 01-Core — sparsematrix. **XL: staged, separate job.**
- [ ] **P024 — C14**: 01-Core — sparsematrixmini.
- [ ] **P025 — C11**: 01-Core — poly. **XL: staged, separate job.**
- [ ] **P026 — C12**: 01-Core — polymini.

### Advanced C++ families

- [ ] **P027 — DS04, DS05, DS06, DS47**: 02-Data Structures — fenwick tree advanced, lazysegmenttree, dynamicsegmenttree, interval set.
- [ ] **P028 — DS07, DS08, DS26**: 02-Data Structures — segtreebeats, segment tree 2d, mergesorttree, aggregation queue.
- [ ] **P029 — DS09, DS10, DS48**: 02-Data Structures — waveletmatrix, treap, heap deque.
- [ ] **P030 — DS23**: 02-Data Structures — persistentsegmenttree. **XL: staged, separate job.**
- [ ] **P031 — DS24**: 02-Data Structures — balanced bst. **XL: staged, separate job.**
- [ ] **P032 — DS11, DS27, DS28**: 02-Data Structures — lichao, convexhulltrick, radixheap, static range queries.
- [ ] **P033 — DS13, DS29**: 02-Data Structures — trie, cartesiantree, disjointsparsetable, offline rectangle queries.
- [ ] **P034 — DS30, DS31, DS03**: 02-Data Structures — sqrttree, persistentarray, rollbackdsu, persistentdsu.
- [ ] **P035 — DS25, DS32**: 02-Data Structures — weighteddsu, fastset.
- [ ] **P036 — GE03, GE24, GE25**: 03-Geometry — halfplaneintersection, minkowskisum, circle polygon, polygon distance.
- [ ] **P037 — GE05, GE27**: 03-Geometry — segment union, minimumenclosingcircle, convex polygon query, lattice geometry.
- [ ] **P038 — GE28**: 03-Geometry — convex hull updates. **XL: staged, separate job.**
- [ ] **P039 — GE10, GE30, GE18**: 03-Geometry — circleunion, geometric median, spherical geometry.
- [ ] **P040 — GE17**: 03-Geometry — spatial index.
- [ ] **P041 — GE32**: 03-Geometry — point set queries. **XL: staged, separate job.**
- [ ] **P042 — GR04, GR05**: 04-Graphs — all pairs shortest path, shortest path advanced, tree algorithms, hld.
- [ ] **P043 — GR07, GR10, GR11**: 04-Graphs — twosat, matching bipartite, assignment, dominatortree.
- [ ] **P044 — GR08**: 04-Graphs — max flow. **XL: staged, separate job.**
- [ ] **P045 — GR09**: 04-Graphs — min cost flow. **XL: staged, separate job.**
- [ ] **P046 — GR25**: 04-Graphs — matching general. **XL: staged, separate job.**
- [ ] **P047 — GR69, GR12, GR28**: 04-Graphs — directedmst, dynamic connectivity, blockcuttree, path cover.
- [ ] **P048 — GR13, GR27, GR32**: 04-Graphs — cycle basis, planar graph, flow with demands, subgraph enumeration.
- [ ] **P049 — GR30, GR31, GR34**: 04-Graphs — chordalgraph, edge coloring, graph closure.
- [ ] **P050 — GR35, GR36, GR37, GR38**: 04-Graphs — stablematching, graphicalsequence, reachability, cactusgraph.
- [ ] **P051 — GR39, GR58, GR60, GR65**: 04-Graphs — graph decomposition, implicit graph, differenceconstraints, graph connectivity.
- [ ] **P052 — MA03, MA04**: 05-Mathematics — primality factorization, multiplicative functions, modinverse, crt.
- [ ] **P053 — MA05**: 05-Mathematics — discrete log root.
- [ ] **P054 — MA26, MA41**: 05-Mathematics — combinatorics advanced, modular power towers.
- [ ] **P055 — MA06, MA12**: 05-Mathematics — convolution, transform algorithms.
- [ ] **P056 — GR06**: 04-Graphs — centroiddecomposition, treeisomorphism.
- [ ] **P057 — MA07**: 05-Mathematics — polynomial algorithms, linear recurrence.
- [ ] **P058 — MA08**: 05-Mathematics — linear algebra.
- [ ] **P059 — MA09**: 05-Mathematics — numerical methods, integer roots.
- [ ] **P060 — MA10**: 05-Mathematics — floorsum, josephus, continued fraction.
- [ ] **P061 — MA11**: 05-Mathematics — interpolation, matrixtree.
- [ ] **P062 — MA13**: 05-Mathematics — game theory, probability.
- [ ] **P063 — MA14**: 05-Mathematics — optimization.
- [ ] **P064 — MA27**: 05-Mathematics — simplex. **XL: staged, separate job.**
- [ ] **P065 — MA29**: 05-Mathematics — polynomial roots. **XL: staged, separate job.**
- [ ] **P066 — MI04, MI05, MI06**: 06-Miscellaneous — smalltolarge, hilbertorder, fast io advanced, randomized algorithms, hash families, parallelbinarysearch, cdq.
- [ ] **P067 — DS12**: 02-Data Structures — range query offline.
- [ ] **P068 — MI07**: 06-Miscellaneous — bitset optimization, contestallocator, memoization.
- [ ] **P069 — MI08**: 06-Miscellaneous — exactcover.
- [ ] **P070 — MI09**: 06-Miscellaneous — satsolver. **XL: staged, separate job.**
- [ ] **P071 — MI10**: 06-Miscellaneous — calendar time, encoding.
- [ ] **P072 — MI18**: 06-Miscellaneous — knapsack advanced.
- [ ] **P073 — MI19, MI20, MI21, MI22**: 06-Miscellaneous — meetinthemiddle, digitdp, subset dp, profile dp.
- [ ] **P074 — GR33**: 04-Graphs — steiner tree.
- [ ] **P075 — MI23**: 06-Miscellaneous — slope trick.
- [ ] **P076 — MI24, MI25**: 06-Miscellaneous — state space search, game search.
- [ ] **P077 — MI26**: 06-Miscellaneous — expression parser.
- [ ] **P078 — MI27**: 06-Miscellaneous — heuristic optimization.
- [ ] **P079 — MI28, MI29**: 06-Miscellaneous — scheduling, optimal merge.
- [ ] **P080 — MI30**: 06-Miscellaneous — dynamic dp.
- [ ] **P081 — ST04, ST06, ST33**: 07-Strings — suffixautomaton, sais, editdistance.
- [ ] **P082 — ST05**: 07-Strings — suffixtree. **XL: staged, separate job.**
- [ ] **P083 — ST07, ST34**: 07-Strings — palindromictree, lyndon, minrotation, lcs.
- [ ] **P084 — ST08, ST20, ST23**: 07-Strings — string matching, bitap, subsequence automaton, bwt.
- [ ] **P085 — ST21, ST09**: 07-Strings — wildcardmatching, lexicographic queries, dynamicstringhash.
- [ ] **P086 — ST35, ST22, ST24**: 07-Strings — regular language, radixtrie, onlinez.
- [ ] **P087 — ST10**: 07-Strings — runs. **XL: staged, separate job.**
- [ ] **P088 — ST11, ST25**: 07-Strings — string periodicity, multiple string, string reconstruction.
- [ ] **P089 — ST26**: 07-Strings — multidimensional matching.

### Python contest layer

- [ ] **P090 — PY01**: 08-Python — io, search, number theory, combinatorics, sequences.
- [ ] **P091 — PY02, PY20**: 08-Python — dsu, fenwick, segmenttree, sparsetable.
- [ ] **P092 — PY03**: 08-Python — graph.
- [ ] **P093 — PY17**: 08-Python — strings.
- [ ] **P094 — PY18**: 08-Python — geometry.
- [ ] **P095 — PY19**: 08-Python — dp.
- [ ] **P096 — PY04**: 08-Python — prime factor.
- [ ] **P097 — PY34, PY35**: 08-Python — flow, matching.
- [ ] **P098 — PY05, PY36, PY38**: 08-Python — tree, rollback, persistent, bitset.
- [ ] **P099 — PY06, PY37**: 08-Python — polynomial, matrix.
- [ ] **P100 — PY07**: 08-Python — trie, aho, suffix.
- [ ] **P101 — PY08, PY11**: 08-Python — offline, randomized, io advanced.
- [ ] **P102 — PY12, PY21**: 08-Python — lazysegmenttree, ordered multiset.
- [ ] **P103 — PY22, PY39**: 08-Python — line envelope, dp optimization.
- [ ] **P104 — PY23, PY24, PY25**: 08-Python — twosat, lowlink, eulertrail.
- [ ] **P105 — PY26, PY27**: 08-Python — shortest paths, mincostflow.
- [ ] **P106 — PY28**: 08-Python — number theory advanced.
- [ ] **P107 — PY29**: 08-Python — transform algorithms.
- [ ] **P108 — PY30, PY32**: 08-Python — state search, functionalgraph.
- [ ] **P109 — PY31**: 08-Python — geometry float.
- [ ] **P110 — PY33**: 08-Python — dp advanced.

### Specialist and research families

- [ ] **P111 — DS14**: 02-Data Structures — lct. **XL: staged, separate job.**
- [ ] **P112 — DS15**: 02-Data Structures — eulertourtree. **XL: staged, separate job.**
- [ ] **P113 — DS16**: 02-Data Structures — toptree. **XL: staged, separate job.**
- [ ] **P114 — DS17, DS18, DS21**: 02-Data Structures — succinctbitvector, succincttree, vanemdeboas, matroid oracle.
- [ ] **P115 — DS22, DS20, DS33**: 02-Data Structures — rangetree, kineticheap, persistentqueue.
- [ ] **P116 — DS19**: 02-Data Structures — retroactivequeue. **XL: staged, separate job.**
- [ ] **P117 — DS34, DS38, DS39**: 02-Data Structures — persistentheap, rangeparallelunionfind, rangemode.

### Advanced C++ families — prerequisite continuation

- [ ] **P118 — GR29**: 04-Graphs — kshortestwalks.

### Specialist and research families

- [ ] **P119 — DS35**: 02-Data Structures — dynamicbitvector. **XL: staged, separate job.**
- [ ] **P120 — DS36**: 02-Data Structures — dynamic wavelet. **XL: staged, separate job.**
- [ ] **P121 — DS37**: 02-Data Structures — sortablesegmenttree. **XL: staged, separate job.**
- [ ] **P122 — DS40, DS41**: 02-Data Structures — linearrmq, commonintervaltree.
- [ ] **P123 — DS42**: 02-Data Structures — pqtree. **XL: staged, separate job.**
- [ ] **P124 — DS43**: 02-Data Structures — fingertree. **XL: staged, separate job.**
- [ ] **P125 — DS44**: 02-Data Structures — persistent bst. **XL: staged, separate job.**
- [ ] **P126 — DS45**: 02-Data Structures — range sequence queries. **XL: staged, separate job.**
- [ ] **P127 — DS46**: 02-Data Structures — succinct sequence. **XL: staged, separate job.**
- [ ] **P128 — GE11**: 03-Geometry — exact predicates. **XL: staged, separate job.**

### Advanced C++ families — prerequisite continuation

- [ ] **P129 — GE04**: 03-Geometry — segmentintersection. **XL: staged, separate job.**
- [ ] **P130 — GE26**: 03-Geometry — polygontriangulation. **XL: staged, separate job.**
- [ ] **P131 — GE29, GE16, GE34**: 03-Geometry — circle constructions, linearrangement, visibilitypolygon.
- [ ] **P132 — GE31**: 03-Geometry — geometry3d. **XL: staged, separate job.**
- [ ] **P133 — GE06**: 03-Geometry — polygon boolean. **XL: staged, separate job.**
- [ ] **P134 — GE07**: 03-Geometry — delaunay. **XL: staged, separate job.**
- [ ] **P135 — GE09, GE08**: 03-Geometry — manhattan geometry, geometricmst, voronoi.
- [ ] **P136 — GE33**: 03-Geometry — pointlocation. **XL: staged, separate job.**

### Specialist and research families

- [ ] **P137 — GE12**: 03-Geometry — convexhull3d. **XL: staged, separate job.**
- [ ] **P138 — GE13**: 03-Geometry — halfspaceintersection3d. **XL: staged, separate job.**
- [ ] **P139 — GE14, GE19, GE20**: 03-Geometry — visibilitygraph, kinetic geometry, geometric duality.
- [ ] **P140 — GE15**: 03-Geometry — randomizedlp.
- [ ] **P141 — GE22**: 03-Geometry — algebraic geometry. **XL: staged, separate job.**
- [ ] **P142 — GE35**: 03-Geometry — constraineddelaunay. **XL: staged, separate job.**
- [ ] **P143 — GE36**: 03-Geometry — weighted voronoi. **XL: staged, separate job.**
- [ ] **P144 — GE21**: 03-Geometry — minimumwidthannulus. **XL: staged, separate job.**
- [ ] **P145 — GE37**: 03-Geometry — delaunay3d. **XL: staged, separate job.**
- [ ] **P146 — GE38**: 03-Geometry — polygonoffset. **XL: staged, separate job.**
- [ ] **P147 — GE39**: 03-Geometry — minimumenclosingellipse. **XL: staged, separate job.**
- [ ] **P148 — GE40**: 03-Geometry — alphashape. **XL: staged, separate job.**
- [ ] **P149 — GE41**: 03-Geometry — polyhedron boolean. **XL: staged, separate job.**
- [ ] **P150 — GE42**: 03-Geometry — curve distance. **XL: staged, separate job.**
- [ ] **P151 — GE43**: 03-Geometry — dynamicconvexhull. **XL: staged, separate job.**
- [ ] **P152 — GR14, GR16, GR23**: 04-Graphs — gomoryhutree, stoerwagner, maximumclique, minimummeancycle.
- [ ] **P153 — GR15**: 04-Graphs — matroidintersection. **XL: staged, separate job.**
- [ ] **P154 — GR17**: 04-Graphs — treedecomposition. **XL: staged, separate job.**
- [ ] **P155 — GR18, GR21, GR24**: 04-Graphs — lct, sensitivity analysis, temporal graph, maximumdensitysubgraph.
- [ ] **P156 — GR20**: 04-Graphs — graph isomorphism. **XL: staged, separate job.**
- [ ] **P157 — GR40**: 04-Graphs — kshortestsimplepaths. **XL: staged, separate job.**
- [ ] **P158 — GR41, GR48, GR49**: 04-Graphs — stnumbering, coloring exact, cycle enumeration.
- [ ] **P159 — GR42**: 04-Graphs — triconnectivity. **XL: staged, separate job.**
- [ ] **P160 — GR43**: 04-Graphs — planarity. **XL: staged, separate job.**
- [ ] **P161 — GR44**: 04-Graphs — fully dynamic connectivity. **XL: staged, separate job.**
- [ ] **P162 — GR45**: 04-Graphs — weightedblossom. **XL: staged, separate job.**
- [ ] **P163 — GR22**: 04-Graphs — gabowmatching. **XL: staged, separate job.**
- [ ] **P164 — GR46**: 04-Graphs — costscalingflow. **XL: staged, separate job.**
- [ ] **P165 — GR47**: 04-Graphs — networksimplex. **XL: staged, separate job.**
- [ ] **P166 — GR50, GR53, GR54**: 04-Graphs — tjoin, minimumcyclebasis, hamiltonian.
- [ ] **P167 — GR51**: 04-Graphs — incrementalscc. **XL: staged, separate job.**
- [ ] **P168 — GR52**: 04-Graphs — dynamicmst. **XL: staged, separate job.**
- [ ] **P169 — GR55, GR56, GR62**: 04-Graphs — threeedgecomponents, minimumdiameterspanningtree, tree ordering.
- [ ] **P170 — GR57**: 04-Graphs — dynamicstarmincut. **XL: staged, separate job.**
- [ ] **P171 — GR59**: 04-Graphs — structured graph. **XL: staged, separate job.**
- [ ] **P172 — GR61**: 04-Graphs — grouplabeledshortestpath. **XL: staged, separate job.**
- [ ] **P173 — GR63, GR64, GR67**: 04-Graphs — rankedmatching, stableroommates, extremevertexsets.
- [ ] **P174 — GR66**: 04-Graphs — skew symmetric flow. **XL: staged, separate job.**
- [ ] **P175 — GR68**: 04-Graphs — dynamictreedp. **XL: staged, separate job.**
- [ ] **P176 — MA15**: 05-Mathematics — summatory functions. **XL: staged, separate job.**
- [ ] **P177 — MA16**: 05-Mathematics — dirichlet series.
- [ ] **P178 — MA17**: 05-Mathematics — modular square roots, quadratic congruence.
- [ ] **P179 — MA24**: 05-Mathematics — lattice reduction. **XL: staged, separate job.**
- [ ] **P180 — MA18**: 05-Mathematics — integer factor advanced. **XL: staged, separate job.**
- [ ] **P181 — MA19**: 05-Mathematics — fastfactorial, bernoulli. **XL: staged, separate job.**
- [ ] **P182 — MA25**: 05-Mathematics — special functions.
- [ ] **P183 — MA28**: 05-Mathematics — generating functions.
- [ ] **P184 — MA20**: 05-Mathematics — padic arithmetic, rationalreconstruction.
- [ ] **P185 — MA21**: 05-Mathematics — finite fields. **XL: staged, separate job.**
- [ ] **P186 — MA22**: 05-Mathematics — determinant advanced. **XL: staged, separate job.**
- [ ] **P187 — MA23**: 05-Mathematics — contourintegral.
- [ ] **P188 — MA30**: 05-Mathematics — polynomial factorization. **XL: staged, separate job.**
- [ ] **P189 — MA31**: 05-Mathematics — nimber.
- [ ] **P190 — MA32**: 05-Mathematics — combinatorial linear algebra. **XL: staged, separate job.**
- [ ] **P191 — GR19**: 04-Graphs — graph counting. **XL: staged, separate job.**
- [ ] **P192 — MA33**: 05-Mathematics — integer linear algebra. **XL: staged, separate job.**
- [ ] **P193 — MA34**: 05-Mathematics — multivariate polynomial. **XL: staged, separate job.**
- [ ] **P194 — MA35**: 05-Mathematics — polynomial matrix. **XL: staged, separate job.**
- [ ] **P195 — MA36, MA37**: 05-Mathematics — relaxed convolution, semiring convolution.
- [ ] **P196 — MA38, MA42, MA44**: 05-Mathematics — discrete log advanced, quadratic integer, zero sum.
- [ ] **P197 — MA39**: 05-Mathematics — floor sum polynomial.
- [ ] **P198 — MA40**: 05-Mathematics — coding theory. **XL: staged, separate job.**
- [ ] **P199 — MA43**: 05-Mathematics — partizan games.
- [ ] **P200 — MA45, MA50, MA51**: 05-Mathematics — tableau algorithms, convolution specialized, set power series.
- [ ] **P201 — MA46**: 05-Mathematics — algebraic numbers. **XL: staged, separate job.**
- [ ] **P202 — MA48**: 05-Mathematics — multiprecision float. **XL: staged, separate job.**
- [ ] **P203 — MA47**: 05-Mathematics — ball arithmetic. **XL: staged, separate job.**
- [ ] **P204 — MA49**: 05-Mathematics — group algorithms. **XL: staged, separate job.**
- [ ] **P205 — MI11, MI13**: 06-Miscellaneous — rollback framework, persistentallocator, external memory.
- [ ] **P206 — MI12**: 06-Miscellaneous — probabilistic sketches, streaming algorithms, derandomization.
- [ ] **P207 — MI31**: 06-Miscellaneous — alphabetic tree. **XL: staged, separate job.**
- [ ] **P208 — ST12**: 07-Strings — onlinesuffixtree. **XL: staged, separate job.**
- [ ] **P209 — ST13, ST37, ST16**: 07-Strings — dynamicaho, approximate matching, palindrome advanced.
- [ ] **P210 — ST36**: 07-Strings — dynamicsuffixarray. **XL: staged, separate job.**
- [ ] **P211 — ST14**: 07-Strings — compressed text index. **XL: staged, separate job.**
- [ ] **P212 — ST15**: 07-Strings — grammar compression. **XL: staged, separate job.**
- [ ] **P213 — ST17, ST18, ST38**: 07-Strings — stringisomorphism, debruijn, zivlempel, linear string algorithms.
- [ ] **P214 — ST27**: 07-Strings — dynamic suffix automaton. **XL: staged, separate job.**
- [ ] **P215 — ST28**: 07-Strings — dynamicpalindrome. **XL: staged, separate job.**
- [ ] **P216 — ST29**: 07-Strings — rindex. **XL: staged, separate job.**
- [ ] **P217 — ST30**: 07-Strings — dynamiclce. **XL: staged, separate job.**
- [ ] **P218 — ST31**: 07-Strings — semilocallcs. **XL: staged, separate job.**
- [ ] **P219 — ST32**: 07-Strings — historical string matching. **XL: staged, separate job.**
- [ ] **P220 — PY09**: 08-Python — exact geometry.
- [ ] **P221 — PY13, PY14**: 08-Python — succinct, fmindex.
- [ ] **P222 — PY10**: 08-Python — algebraic, combinatorial species.
- [ ] **P223 — PY15, PY16**: 08-Python — sat, approximation.

### Final support integration

- [ ] **P224 — SUP02**: Whole-library test-runner, aggregate and build integration audit.
- [ ] **P225 — SUP03**: Online solution coverage and expansion audit.
- [ ] **P226 — SUP04**: Curated notebook, contest notes and printable-build audit.
