# 01-dsu.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### DSU

Vertices `[0, n)` with `0 <= n <= INT_MAX`. Public `n`, `ncon`, `par` and `siz` must not be edited directly. Representatives are arbitrary and can change after a union. `findSet`, `getSize`, `isSameSet` and `groups` compress paths, so they are non-const. `uniteSets` returns whether two distinct components merged. `count` is the number of components. `groups` returns the components ordered by smallest vertex, each with members ascending, in O(n) time, workspace and output. `makeSet` appends the singleton vertex `n` and returns its id; it requires `n < INT_MAX`. Union by size gives O(log(n)) worst-case and inverse-Ackermann amortized find and unite.

### Correctness and cost

Union by size bounds uncompressed depth by `log(n)`; compression preserves the root equivalence relation. Sizes and component count change only on successful unions. Amortized inverse-Ackermann union/find, O(n) initialization/storage. With no unions during grouping, each non-root edge is bypassed at most once, giving O(n) grouping including output. `makeSet` is a one-line composition of verified operations.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`01-dsu_tester.cpp` group) | Oracle |
|---|---|---|
| `findSet`, `uniteSets`, `isSameSet`, `getSize`, `count`, `groups` | `graphSubsets`, `unionHistories` | Every simple graph through 6 vertices, self/duplicate unions, every length-5 union history on 3 vertices; independent relabeling connectivity/size/group oracle |
| `makeSet` | `randomCases` | 150 random cases with interleaved `makeSet` (id and singleton oracle) |
| Compression at scale | `balancedTree` | 65,536-vertex balanced merge/compression |
| Union by size (`O(log(n))` depth) | `checkDepths`, in every `verify` and after the 65,536-vertex stars `uniteSets(i,0)` and `uniteSets(0,i)` in `balancedTree` | `par` in range; a vertex at depth `k` of `par` lies in a component of at least `2^k` vertices (P251 mutation campaign) |
| Value semantics and preconditions | all groups | Copies/moves/reset; 9 assertion probes, including `find-past` (`u > n`) and `makeset-full` (`n == INT_MAX`; it sets the public `n` directly, a deliberate white-box exception to the no-edit rule, checked build only) |

Quick: n<=4, depth 3, 35 trials, 1,024 vertices (balanced and stars). Stress: history depth 6, 600 random cases. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Latest result of each command, 2026-11-03, after P251 strengthened the tester (header unchanged; g++ 16.0.1 20260322 and g++-14 14.3.0, Python 3.14.4, Linux 6.18 WSL2, AMD Ryzen AI 9 HX 370):

```bash
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 20261103
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures/01-dsu' --no-integration
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/02-integration.py'
```

All passed: quick in 2 configurations (`MEMORY peak=389MB`); full on g++ and g++-14 in the optimized, checked and ASan/UBSan builds with 9 assertion probes (peaks 539 MB and 521 MB); stress, two rounds (peaks 667 MB and 666 MB); consistency without errors in a linked worktree carrying the change (the main checkout of this clone lacks the git-ignored `OLD/Team Notebook/src/algs.exe` the validator requires there). Integration last ran 2026-10-13 with the P006 prune rerun, recorded in [02-fenwick.md](02-fenwick.md#commands-and-results); the header has not changed since. Mutation campaign (mewt 4.0.0, 140 mutants; 6 equivalent survivors): [00-mutation.md](../../96-Local%20Testing/Docs/00-mutation.md).

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 15 | DSU; the chapter 15 example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind reference. |

References inspected on 2026-09-27. Bibliographic mentions of Tarjan are not claims that the original proofs were inspected.

## Limits and handoffs

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts; its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The weighted and parity semantics are `WeightedDSU` and `ParityDSU` in `29-weighteddsu.hpp` ([29-weighteddsu.md](29-weighteddsu.md#legacy-mapping), P035); the component edge counts `esz`/`getEsiz` are `RollbackDSUAggregate::edgeCount` in `09-rollbackdsu.hpp` ([09-rollbackdsu.md](09-rollbackdsu.md), P034), and Graphs owns dynamic-connectivity orchestration. Rollback, persistent and weighted DSU are separate rows, not gaps of this row. DSU grid adapters and delete/move element are left out with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions).

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 10 findings fixed, `makeSet` added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-03: P251 mutation campaign, tester gained the union-by-size depth check and probes `find-past`, `makeset-full`; quick, full on g++ and g++-14, and 2 stress rounds passed.
