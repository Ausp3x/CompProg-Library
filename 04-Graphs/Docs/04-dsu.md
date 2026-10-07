# 04-dsu.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` through `03-toposort.hpp`, `05-shortest_path.hpp` and `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract). Prerequisite: the verified P006 canonical DSU. First verified on 2026-09-27 and re-audited on 2026-10-07.

## Contracts

### graphComponents

Returns the canonical P006 `DSU` after uniting the ends of every edge: connected components, or weak components of a directed graph. Loops are allowed. The DSU is re-exported by inclusion, with no duplicate implementation.

Correctness: the graph-facing adapter uses the canonical P006 `DSU`, treating a directed graph as weak connectivity; it neither duplicates nor modifies the DSU implementation.

## Feature-to-test map

`96-Local Testing/04-Graphs/04-dsu_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Oracles use non-removable checks. Checked builds also run invalid-precondition subprocesses. Test outputs are temporary; seeds and failed inputs/operations are printed. Quick uses three vertices/50 random cases, full four vertices/500 cases and stress four vertices/4,000 cases. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `graphComponents` (undirected or directed weak components, Graph/CsrGraph) | 67,166 exhaustive directed/undirected graphs through four vertices and 500 random graphs; CSR and reversal; 100,000-vertex chain | Transitive closure |
| DSU re-export | Canonical DSU identity | Type identity |
| Preconditions | Three assertion probes | Expected assertion failure |

## Commands and results

### 2026-09-27 (original P010)

Fresh runs used GCC 16.2.1 (20260810), GNU++20 and CPython 3.14.7 on Linux x86-64. Full mode with seed 20260927 passed all three builds. The sanitizer run encountered the environment's ptrace/LeakSanitizer restriction and passed after an approved execution outside the sandbox with leak detection enabled; this is a completed retry, not a skipped configuration. The shared quick runner passed all six P010 entries with seed 42 from `/tmp`. Integration passed 79 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage/execution, and LOCAL/non-LOCAL workspace compilation. Repository consistency passed with no errors. All 61 checked precondition probes of the package passed. Extended stress modes were implemented but not run in that pass. Independent review found no remaining correctness, domain, complexity or meaningful coverage gap.

### 2026-10-07 re-audit

Before any edit, the unchanged suite passed full mode with seed 20260927 in all three builds. After the changes every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20260927, before any edit | PASS, all three builds |
| Stress, seed 7 | PASS, all three builds, 9,925,404 checks per build |
| Full, seed 20261007, after the review fixes, GCC 16.2.1 | PASS, all three builds, 6,346,458 checks per build, 3 probes |
| Full, seed 20261007, GCC 14.4.1 20260915 (`CXX=g++-14`) | PASS, all three builds, same counts |

The current feature map's full result with seed 20260927 is 6,355,359 checks per build and three assertion probes. The shared quick runner passed all eleven graph entries from `/tmp`. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage, the workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors, and `--braces` reports nothing for the package. All runs used CPython 3.14 on Linux x86-64. GCC 14.2 itself was not run; 14.4.1 is the closest available. No online submission was made.

```bash
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20260927        # before any edit
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261007        # after the review fixes
CXX=g++-14 python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261007
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

The findings are recorded in `00-Guidelines/23-Reaudit Findings/p010.md`.

| # | Finding | Disposition |
|---|---|---|
| 7, 12, 14 | Closing-brace rule (`}} }`, `; }}}`, `; }`) | Fixed in all six P010 headers, testers and both benchmarks. |
| 8 | T/M lines not directly above | Every declaration now has its complexity line directly above it. |

Comment cap: `04-dsu.hpp` has 9 non-blank lines, so its one required complexity line is 11%; the validator and `03-cpp.md` now always allow one comment line. Independent review (`@reviewer`, 2026-10-07) found no correctness defect.

## Sources

No external source beyond the canonical P006 DSU evidence. Canonical P006 DSU already assigns historical potential/parity and component edge-count metadata to their Advanced owners; this graph adapter does not claim those variants.

## Limits and handoffs

No owned implementation or verification gap remains. Weighted, rollback and other DSU variants are owned by Data Structures (see [00-notes.md](00-notes.md#ownership-boundaries)).
