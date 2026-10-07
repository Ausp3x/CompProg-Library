# Unchanged monolith transfer — 2026-09-27

The user requested two tasks: a reusable prompt/batch plan for future implementation, and extraction of algorithms from `OLD/algorithms.cpp` and `OLD/[1] algorithms.cpp` without algorithm changes. This pass organizes existing code; correctness, completeness, performance and style audits remain future work.

## What moved

| Folder | New active headers | Unchanged content |
|---|---:|---|
| 01-Core | 2 | BitMatrix and SparseMatrix |
| 02-Data Structures | 12 | Shared monoid helpers; DSU; sparse table; advanced Fenwick; lazy, beats, 2D and persistent segment trees; dynamic merge-sort tree; wavelet matrix; Li Chao variants; Mo |
| 04-Graphs | 11 | Dijkstra; LCA; BridgeAlgo/Tarjan lowlink; FloydWarshall; SPFA; HLD; centroid decomposition; TwoSat; EdmondsKarp; Hungarian; OnlineBridges |
| 06-Miscellaneous | 1 | Kadane, as the initial sequence-algorithm family member |

All 26 headers are **existing-unverified**. Their original bodies remain byte-for-byte identical; only `#pragma once`, direct include dependencies and provenance comments were added around them. Existing active implementation headers were not overwritten. Aggregate includes and inventories were updated. Family filenames describe intended inventory scope; a single extracted member does not implement that whole checklist.

Fourteen distinct older, incomplete or integration-blocked excerpts are placed in local `97-Legacy/` folders under Core, Data Structures, Graphs, Mathematics and Miscellaneous. Each has a short index explaining its role. These `.cpp` files are reference text, not standalone programs; they are excluded from aggregates and notebook selection. Notebook discovery now prunes these directories. The complete source monoliths remain unchanged in OLD.

[15-monolith-map.json](../Ledgers/monolith-map.json) maps 55 source blocks: 47 extracted bodies (some share a header), six exact duplicates mapped to their chosen copy, and two archive-only items (TODO labels and the empty main). It records source ranges, original file hashes, excerpt hashes, target body ranges and known issues. Every nonblank source line outside these ranges is a heading or historical comment, not an omitted algorithm body. There are no implemented Geometry or Strings algorithms to extract from these two monoliths.

## Placement and known integration limits

- `SegTree` is lazy and goes to DS12; multidimensional `FenTree` goes to Advanced DS11. Basic Fenwick and iterative segment-tree implementations remain planned. Temporary `00-monoids.hpp` support is outside tier ordering; DS05 owns its future audit/refactoring. Any mixed-operation legacy API still needs its planned split.
- The monolith's `Tarjan` finds bridges/articulation, not SCC. It is grouped with `BridgeAlgo` in Graphs09; the SCC implementation remains planned.
- Reference-only `SqrtDecomp` has invalid default arguments; `DynSegTree` has a `max(l, 0LL)` type mismatch with the current template's LP64 `lng` alias. No code was changed to repair these.
- Free `floydWarshall` needs the removed `sze` macro. Free `isBipartite` has external linkage and fails multi-translation-unit inclusion unchanged. Their source is preserved for the future graph batches.
- Commented `EulerTourTree` is static subtree flattening with an obsolete segment-tree dependency, not a dynamic Euler-tour tree. Old `LiChaoTree` and `MonSet` are unfinished scaffolds; older `MergeSortTree` retains suspected exhausted-merge-side access. The commented `ModFac` uses old modular-integer member APIs.
- Earlier Random, CustomHash, FastConv, template and debug variants are available for comparison in local references. Their already-active counterparts remain unchanged. Historical TESTED comments and the source's AI-generated/inferior labels are context, not verification evidence.

## Checks performed

- Every extracted body was compared byte-for-byte against its recorded original line range and SHA-256; both original file hashes match the pre-transfer snapshot. All original non-comment code is accounted for. No existing nonaggregate algorithm header changed.
- `python3 '96-Local Testing/02-integration.py'` passes with GCC16: **57 standalone/aggregate headers**, scalar and available AVX2 combined multiple-translation-unit linkage, and LOCAL/non-LOCAL workspace syntax checks. The graph-specific wrapper check also instantiated `Hungarian<lng>`.
- `python3 '96-Local Testing/00-Tools/03-notebook_tester.py'` passes all five fixtures, including reference-directory discovery/selection exclusion. Actual notebook PDF rendering was not part of this task.
- Batch-map checks assign every one of the 274 numbered inventory targets once across 163 algorithm batches; the shared DS monoid helper has an additional explicit owner. Five separate support/integration passes cover tooling. Local documentation links and target paths were checked.

These are extraction and integration checks, not algorithm feature suites or performance benchmarks. A syntax-only template check does not instantiate every operation/type combination. New per-header behavioral tests, GCC14 execution, numerical/randomized guarantees, full-Core ISA optimization and completeness reviews remain with their implementation batches. See [01-prompts.md](../../01-prompts.md) to start one.

Later inventory audit: [2026-09-27-inventory-audit.md](2026-09-27-inventory-audit.md) records the expanded scope/current counts; [18-path-updates.json](../Ledgers/path-updates.json) resolves later prefix changes. Counts and placement descriptions above are historical snapshots. The live monolith map uses current target paths while preserving original source/body hashes.
