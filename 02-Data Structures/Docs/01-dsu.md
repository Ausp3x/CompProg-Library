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
| Value semantics and preconditions | all groups | Copies/moves/reset; 7 assertion probes |

Quick: n<=4, depth 3, 35 trials, 1,024 vertices. Stress: history depth 6, 600 random cases. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. The DSU suite passed in every run above.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 15 | DSU; the chapter 15 example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind reference. |

References inspected on 2026-09-27. Bibliographic mentions of Tarjan are not claims that the original proofs were inspected.

## Limits and handoffs

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts; its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The outstanding weighted/parity semantics belong to `29-weighteddsu.hpp`; rollback/component metadata belongs to `09-rollbackdsu.hpp` and Graphs dynamic-connectivity orchestration. Their inventories retain this handoff, and those packages remain planned. Rollback/persistent/weighted DSU are future variants, not gaps of this row. DSU grid adapters and delete/move element (HIT, OI) were left out of the 2026-10-07 sweep as variants for row 65 if a motivating problem appears.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 9, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, added `makeSet`, full and stress passed on g++.
