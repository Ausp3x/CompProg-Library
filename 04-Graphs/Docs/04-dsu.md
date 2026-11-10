# 04-dsu.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `01-graph.hpp` through `03-toposort.hpp`, `05-shortest_path.hpp` and `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract). Prerequisite: the verified P006 canonical DSU.

## Contracts

### graphComponents

Returns the canonical P006 `DSU` after uniting the ends of every edge: connected components, or weak components of a directed graph. Loops are allowed. The DSU is re-exported by inclusion, with no duplicate implementation.

## Feature-to-test map

`96-Local Testing/04-Graphs/04-dsu_tester.py` uses the shared contest runner, which builds optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and full/stress `-O1` ASan/UBSan variants. Quick uses three vertices/50 random cases, full four vertices/500 cases and stress four vertices/4,000 cases. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `graphComponents` (undirected or directed weak components, Graph/CsrGraph) | 67,166 exhaustive directed/undirected graphs through four vertices and 500 random graphs; CSR and reversal; 100,000-vertex chain | Transitive closure |
| DSU re-export | Canonical DSU identity | Type identity |
| Preconditions | Three assertion probes | Expected assertion failure |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 754 MB; optimized 0.2 s compile + 1.3 s run, checked 13.2 s compile + 3.7 s run, ASan-UBSan 12.8 s compile + 46.5 s run; MEMORY peak 892 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 802 MB; optimized 0.2 s compile + 1.2 s run, checked 14.8 s compile + 2.4 s run, ASan-UBSan 14.2 s compile + 27.8 s run; MEMORY peak 942 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 21.9 s compile + 0.0 s run, checked 17.4 s compile + 0.8 s run; MEMORY peak 744 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 758 MB; optimized 0.1 s compile + 0.7 s run, checked 6.0 s compile + 2.4 s run, ASan-UBSan 6.0 s compile + 27.2 s run; MEMORY peak 621 MB. Dated rows below stay those commands' latest results.

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

## Aliases

Search aliases (P263, grab --find): 弱连通分量, 弱连通组件, 连通分量, 连通块, 并查集.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-07: re-audit, 2 findings fixed (brace rule, complexity line placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: P010 prune rerun, header and tests unchanged; full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign (`05-testing.md`, Mutation testing).
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
