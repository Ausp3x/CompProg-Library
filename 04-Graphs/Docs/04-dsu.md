# 04-dsu.hpp — Evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` through `03-toposort.hpp`, `05-shortest_path.hpp` and `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract). Prerequisite: the verified P006 canonical DSU.

## Contracts

### Scope

The row is the graph-facing DSU adapter only. Weighted, rollback and other DSU variants belong to Data Structures ([00-notes.md](00-notes.md#ownership-boundaries)).

### graphComponents

Returns the canonical P006 `DSU` after uniting the ends of every edge: connected components, or weak components of a directed graph. Loops are allowed. The DSU is re-exported by inclusion, with no duplicate implementation.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/04-dsu_tester.py` uses the shared contest runner: optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1` ASan/UBSan in full and stress. Quick tests 50 random cases on three vertices, full 500 on four and stress 4,000 on four.

| Operation | Test | Oracle |
|---|---|---|
| `graphComponents` (undirected or directed weak components, Graph/CsrGraph) | 67,166 exhaustive directed and undirected graphs through four vertices and 500 random graphs; CSR and reversal; 100,000-vertex chain | Transitive closure |
| DSU re-export | Canonical DSU identity | Type identity |
| Preconditions | Three assertion probes | Expected assertion failure |

## Commands and Results

Linux x86-64, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14, GNU++20. The shared runner adds `-Wall -Wextra -Wconversion -Werror` to every build.

| Date | Run | Result |
|---|---|---|
| 2026-11-14 | Full, seed 20261114, g++ | PASS, all three builds, 6,349,260 checks per build (optimized 0.6 s, checked 1.6 s, ASan-UBSan 19.6 s), peak 899 MB |
| 2026-11-14 | Full, seed 20261114, `CXX=g++-14` | PASS, same counts (0.6 s, 1.9 s, 23.5 s), peak 951 MB |
| 2026-11-14 | `05-extraction.py testers --filter 04-dsu_tester` | PASS |
| 2026-11-14 | `01-run.py --mode quick --filter 04-Graphs`; `02-integration.py` | PASS |
| 2026-11-27 | Stress, seed 7 (experiment none) | PASS, all three builds, 9,925,404 checks per build, 3 probes (optimized 0.7 s, checked 2.3 s, ASan-UBSan 26.8 s), peak 608 MB |

The 2026-11-14 full runs were repeated after /finish GR01 changed comment lines of `01-graph.hpp`; the times above are from that repeat.

```bash
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261114
CXX=g++-14 python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode full --seed 20261114
python3 '96-Local Testing/04-Graphs/04-dsu_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' testers --filter 04-dsu_tester
python3 '96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

No external source beyond the canonical P006 DSU evidence.

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): 弱连通分量, 弱连通组件, 连通分量, 连通块, 并查集.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune rerun, header and tests unchanged; full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run passed.
- 2026-11-14: P010 GR01 rerun after the `01-graph.hpp` additions, repeated after /finish GR01's comment-only change; header and tests unchanged; full suite passed on g++ and g++-14, extraction testers, quick runner and integration passed.
- 2026-11-27: P010 limits recheck (experiment none): the ownership boundaries moved from Limits and Handoffs to Contracts (Scope), leaving no open limit; stress rerun after the 2026-11-14 changes to `01-graph.hpp` in the include closure passed; no code or test change.
