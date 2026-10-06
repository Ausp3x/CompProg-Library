# Inventory completeness audit — 2026-09-27

This is the historical expansion record (430 targets, 334 batches, 201 archived files at that audit). The later [Core family migration](22-core-migration.md) supersedes current counts and Core paths: 428 targets, 335 batches, 226 unchanged packages and 205 archived files. Original measurements and source hashes remain historical evidence.

The inventories were compared against established contest catalogs, Library Checker problem families, specialist documentation, selected reference APIs/source comments, paper abstracts and the archived library. This is a broad, traceable planning audit. It does **not** establish that every competitive-programming algorithm or research paper has been found, nor that existing implementations satisfy the expanded contracts. Future family-specific research remains part of each implementation job.

## Coverage and resulting scope

| Folder | Before | Current | Added families |
|---|---:|---:|---:|
| 01-Core | 19 | 20 | 1 |
| 02-Data Structures | 38 | 60 | 22 |
| 03-Geometry | 32 | 53 | 21 |
| 04-Graphs | 41 | 84 | 43 |
| 05-Mathematics | 43 | 66 | 23 |
| 06-Miscellaneous | 30 | 48 | 18 |
| 07-Strings | 35 | 49 | 14 |
| 08-Python | 36 | 50 | 14 |
| **Total** | **274** | **430** | **156** |

All original targets retain owners. New operations in existing families are additions to planned scope, not claims that current code already supports them. At that audit, Core kept its original 01–19 order and appended 20-bitset for shared dynamic packed-bit storage. Python deliberately omits impractical ports and uses native integers/Fraction. Each non-Core inventory is ordered Basic, Advanced, Esoteric.

The comparison covered **all 250 non-sample/test Library Checker problem families** in the fetched repository tree: every family has at least one inventoried target in [20-library-checker-coverage.json](20-library-checker-coverage.json). Most mappings used catalog paths/names; ambiguous bounded-sequence/subsequence/subset-sum statements were read directly. This is inventory coverage, not 250 accepted submissions or full test coverage.

## Material additions and corrections

- Core/Mathematics: preservation obligations for legacy InfInt sentinels/conversions and Matrix subspace/Hafnian operations; complete domain-specific dense/sparse/poly contracts; dynamic bitsets; polynomial roots/factorization, specialized convolutions and set FPS, exact normal forms, coding theory, finite fields, nimbers, group/combinatorial/game/arithmetic research and certified numerical variants.
- Data Structures/Graphs: weighted DSU, SWAG, range workload families, integer/persistent heaps and arrays, dynamic succinct structures, common-interval/PQ/finger trees; functional graphs, bounded flows/circulations, weighted blossom, ranked paths, graph classes, planarity/SPQR, dynamic connectivity/MST/SCC, coloring/cycle/postman and specialized flow/decomposition algorithms.
- Geometry/Strings: triangle/circle–polygon/3D primitives, dynamic hulls, lattice queries, point location/triangulation, weighted/site diagrams and solid geometry; reconstruction, subsequences, BWT/r-index, dynamic text/palindromes, 2D/wildcard matching, semi-local comparison and named historical matchers.
- Miscellaneous/Python: previously missing DP/search/knapsack/digit/profile/convex-DP families, parsing, scheduling, heuristic/game search and coding/merging; practical Python ordered/range/graph/numeric/search ports, with exact integer transform inverses and explicit interpreter-cost constraints.
- Support indexes: contest generators/checkers/interactors/shrinking, stronger per-family verification obligations, judge/status distinctions, and notebook dependency/curation/build evidence. Workspace retains only template.cpp as a permanent file.

Corrected or clarified nonconvex Minkowski semantics, randomized dynamic-string guarantees, Carmichael lambda composition, closed nimber-field widths, probable-prime versus certified primality, rounded floating I/O, exact/approximate numerical contracts, and A* reopening conditions. Avoided overlapping owners for weighted blossom, bounded circulation, string-prefix versus integer tries, Ukkonen versus offline suffix trees, tree DP, CHT engines, and Python suffix/reroot/factorization features. Source pitfalls are preserved in the read-scope ledger rather than copied into specifications.

## Research evidence and limits

[17-research-sources.json](17-research-sources.json) records **101 successful fetches**, two failed obsolete/incorrect URLs with working alternatives, content hashes and specific read scopes. Compared ACL, CP-algorithms, OI Wiki, CSES, Library Checker, KACTL/Stanford and Japanese/Korean libraries, plus GMP/FLINT/NTL/Sage, CGAL/robust predicates, OGDF/NetworkX/DYNAMIC, PyRival/Python ACL and selected string papers. Saved reference books/notebooks supplement live catalogs. No claim is made that every listed paper proof or reference implementation was fully audited.

The [source guide](09-sources.md) offers more choices for implementation research. Primary proofs, numeric stability, actual performance, adversarial cases, licenses and relevant latest literature must still be checked for the selected family. A named broad family is a feature ledger with milestones; it is not permission to hide unfinished variants behind a complete checkbox. Infinite or huge answer sets require parameterizations/generators or output-sensitive bounds.

[19-archive-map.json](19-archive-map.json) classifies all **201** archived files with no unclassified entries and accounts for legacy files and maps algorithm families to current owners, retaining incomplete/obsolete/test-only material honestly. The original monolith source-range/body-hash evidence stays in [15-monolith-map.json](15-monolith-map.json). Archived code remains unchanged; this pass does not transfer or repair additional algorithm bodies.

## Work scheduling and renumbering

The bottom of [01-prompts.md](../01-prompts.md) contains **226 assignable packages** for **334 stable algorithm batches** plus five support passes. Related manageable units run sequentially inside a package; 94 large Core/research jobs remain isolated with checkpoints. The research expansion adds substantial work, so collapsing it into a few giant folder assignments would make the requested sizing misleading. The practical foundations are scheduled first; support tests and evidence remain part of every implementation job.

Use `Complete package Pxxx from 01-prompts.md` in a new session. Read the shared workflow and selected checklist line/table rows, not every inventory or source ledger. Package prerequisites and exact targets are also in [packages.json](13-plan/packages.json); target ownership is in [batches.json](13-plan/batches.json). All checkboxes begin unchecked.

Existing batch IDs were retained; oversized groups were split with ownership transfers recorded in the batch map. [18-path-updates.json](18-path-updates.json) maps every original target to its current prefix, including planned targets. Eight existing Graph headers were renamed and their aggregate/provenance references updated; algorithm bodies were preserved. Historical decisions retain historical counts and point here for the current plan.

## Follow-up review before implementation — 2026-09-27

A second pass reviewed every algorithm inventory and batch table, the shared workflow and support indexes, current source interfaces and relevant legacy bodies, the archive/coverage maps, and the previously fetched reference catalogs. Repository-wide checks cover all 47 Markdown files. The PDFs remain chapter-level reference material; this pass did not read every page of every book, re-prove every algorithm, or re-audit every generated source copy.

No additional standalone family was justified by this comparison. Several operations and migration obligations were missing or too implicit, so the existing owners now include them explicitly:

| Area | Scope clarified or added |
|---|---|
| Core and Mathematics | Banded/tridiagonal/cyclic solves; scaled CRT equations and discrete logarithms; with-replacement counting; Mini InfInt base conversion/sentinel obligations; closed-to-half-open segmented-sieve migration; fixed-count sparse iteration versus converged Markov stationary-state behavior. |
| Data Structures and Graphs | Sparse segment-tree destructive meld/split and ownership bounds; binary/d-ary, binomial and Fibonacci heap alternatives with handles and per-operation costs; paired directed-degree realization through max flow. DSU-on-tree/sack remains canonically owned by MI04, with Graphs supplying tree layouts. |
| Geometry | Minimum-area enclosing parallelogram, extremal inscribed fixed-k polygons, and largest empty axis-aligned rectangle inside a supplied box with point obstacles; domain, degeneracy and complexity contracts remain explicit. |
| Miscellaneous and Strings | Circular/bounded-length maximum subarray, maximum-sum subrectangle, counting/weighted LIS, retained hash adapters/aliases, DSU-on-tree callback-cost accounting, and substring shortest-border/period queries. |
| Python | Explicit LCS length/witness and edit-distance/alignment ownership, longest-common-substring spans, and bipartite/odd-cycle witnesses. The legacy CRT map now distinguishes the Basic inverse helper from Advanced CRT. |

Evidence was read from the saved KACTL tridiagonal contract, CGAL package descriptions, OI Wiki navigation, Nyaan/ei1333/MASPYPY/PyRival catalogs and relevant CP-algorithms/CSES entries, plus current and archived local interfaces/bodies. Catalog entries support feature discovery; they do not establish the implementation or its proof. The existing [source ledger](17-research-sources.json) retains retrieval/read-scope evidence; this follow-up made no new algorithm-source fetches and adopted no code. Family indexes record the relevant new source leads and migration obligations.

All Basic/Advanced/Esoteric inventory sections now share the same three columns: proposed header/module, feature checklist, status and references. Core retains its Basic-eligibility column. Table separators, status presentation, shell fence labels and heading spacing are consistent; support indexes retain their purpose-specific columns, and notebook notes retain their renderer's supported section levels. Existing verification status and original archive bytes are preserved.

Stale pre-split ownership instructions were corrected, and duplicated batch descriptions were synchronized into the machine map. Added prerequisites cover circle primitives, polygon arrangements, counting/weighted LIS, suffix/BWT/FM-index construction, directed degree realization, the existing modular-combinatorics include, and Markov linear solves. The standalone-template support package now names its Core prerequisite; every support package points to its batch table. **All additions fit the existing default package order. Counts remain 430 targets, 334 algorithm batches, 226 packages and five support passes; no IDs or target paths changed.**

The [prompt entry point](../01-prompts.md) now explains that the short package prompt is sufficient in a fresh repository-root session. The full prompt has one unambiguous package placeholder, and unfinished packages resume under the same ID with a durable handoff. Separate sessions sharing one checkout still need disjoint ownership.

Follow-up validation passed: unique target/batch/package ownership and counts; exact agreement between Markdown rows and machine maps; complete prerequisite order, including within packages; existing direct-header dependencies covered by prerequisite closure; local Markdown links, table widths and code fences; all 250 recorded Library Checker mappings; all 201 archive files/hashes; and all 55 monolith-map entries with their recorded source/extract hashes. Algorithm code and source paths did not change, so C++ behavioral/benchmark suites were not rerun for this documentation pass.

The resulting roadmap is ready for package implementation with no known unassigned family in the reviewed scope. This is a scoped coverage conclusion, not a guarantee that no relevant algorithm or future paper can ever be missing. Each package still performs its own domain-specific completeness, correctness and performance review before being checked off.

## Original expansion validation

- Inventory/target/batch/package coverage is checked for unique ownership, valid tier order, sequential prefixes and prerequisite order; planned files are not added to aggregates.
- Every Library Checker family at the recorded snapshot maps to a current inventory target; archive and source maps retain honest read/verification scope.
- Original algorithm and archive bytes are checked against their prior hashes across renames; extracted body hashes/ranges remain intact.
- `python3 "96-Local Testing/02-integration.py"` passed: 57 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL workspace syntax. This checks integration after path changes; it is not a new behavioral or performance certification.

Remaining limits: GCC14/interpreter matrix and the expanded per-family behavioral/benchmark suites are future implementation work. No online submissions or notebook PDF rendering were performed by this inventory audit. The specialist backlog remains planned until independently implemented and verified.

## Function-level rewrite — 2026-10-06

Every algorithm inventory was rewritten to the `Header | Operations | Status` contract with a live catalog sweep per folder (Library Checker problem trees, cp-algorithms, ACL, KACTL, OI Wiki, Nyaan, maspypy, ei1333, suisen, hitonanode, tko919, noshi91, PyRival, GMP/FLINT/NTL, CGAL and domain papers; each folder's `81-sources.md` lists the pages fetched). Contract prose moved to each folder's `80-notes.md`. Counts are now **459 targets, 359 batches, 230 packages**: 31 new rows (DS 6, GE 2, GR 3, MA 1, MI 7, ST 1, PY 11), roughly 1,900 named operations, and 24 previously verified headers downgraded to `partial` with explicit `missing:` lists for operations the catalogs showed. The plan manifest in `13-plan/` owns every target once; `plan.py check` and the consistency validator pass.
