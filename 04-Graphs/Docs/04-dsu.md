# 04-dsu.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` through `03-toposort.hpp`, `05-shortest_path.hpp` and `06-mst.hpp`); see [00-notes.md](00-notes.md#p010-package-record) for the package record and the [common graph contract](00-notes.md#p010-common-contract). Prerequisite: the verified P006 canonical DSU.

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

2026-10-07, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261007, g++ | PASS, all three builds, 6,346,458 checks per build, 3 probes |
| Full, seed 20261007, `CXX=g++-14` | PASS, all three builds, same counts |
| Full, seed 20260927 | PASS, 6,355,359 checks per build, 3 probes |
| Stress, seed 7 | PASS, all three builds, 9,925,404 checks per build |
| Quick runner, all eleven graph entries, from `/tmp` | PASS |
| `02-integration.py --sanitizers` (102 headers, scalar and AVX2 multi-TU linkage, workspace, sanitizer self-tests) | PASS |
| `03-consistency.py`, with `--braces` on the package | No errors |

```bash
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261007
CXX=g++-14 python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261007
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode stress --seed 7
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

No external source beyond the canonical P006 DSU evidence. Canonical P006 DSU already assigns historical potential/parity and component edge-count metadata to their Advanced owners; this graph adapter does not claim those variants.

## Limits and handoffs

Weighted, rollback and other DSU variants are owned by Data Structures (see [00-notes.md](00-notes.md#ownership-boundaries)). GCC 14.2 itself (the judge floor) was not run; 14.4.1 is the closest available.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed.
