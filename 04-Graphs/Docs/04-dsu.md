# 04-dsu.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` through `03-toposort.hpp`, `05-shortest_path.hpp` and `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract). Prerequisite: the verified P006 canonical DSU.

## Contracts

### graphComponents

Returns the canonical P006 `DSU` after uniting the ends of every edge: connected components, or weak components of a directed graph. Loops are allowed. The DSU is re-exported by inclusion, with no duplicate implementation.

## Feature-to-test map

`96-Local Testing/04-Graphs/04-dsu_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. Quick uses three vertices/50 random cases, full four vertices/500 cases and stress four vertices/4,000 cases. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `graphComponents` (undirected or directed weak components, Graph/CsrGraph) | 67,166 exhaustive directed/undirected graphs through four vertices and 500 random graphs; CSR and reversal; 100,000-vertex chain | Transitive closure |
| DSU re-export | Canonical DSU identity | Type identity |
| Preconditions | Three assertion probes | Expected assertion failure |

## Commands and results

2026-10-12, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261012, g++ | PASS, all three builds, 6,349,002 checks per build, 3 probes, peak 520 MB |
| Full, seed 20261012, `CXX=g++-14` | PASS, all three builds, same counts, peak 527 MB |
| Stress, seed 7 | PASS, all three builds, 9,925,404 checks per build, peak 634 MB |
| Quick runner, all sixteen graph entries, from `/tmp` | PASS |
| `02-integration.py` (136 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 647 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode stress --seed 7
python3 '/path/to/library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

No external source beyond the canonical P006 DSU evidence. Canonical P006 DSU already assigns historical potential/parity and component edge-count metadata to their Advanced owners; this graph adapter does not claim those variants.

## Limits and handoffs

Weighted, rollback and other DSU variants are owned by Data Structures (see [00-notes.md](00-notes.md#ownership-boundaries)).

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune rerun, header and tests unchanged; full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
