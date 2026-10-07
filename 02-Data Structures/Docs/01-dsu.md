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

### Original P006 verification — 2026-09-27

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode quick --seed 42 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

All eight per-header entries passed `--mode full --seed 20260927` on 2026-09-27 (24 configuration runs, all 100 assertion probes of the package), invoked directly as `python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 20260927`. Compiler GCC 16.2.1 20260810, GNU++20; CPython 3.14.7; Linux x86-64. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass. `02-integration.py` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation; `03-consistency.py` passed with zero errors. The quick run through shared discovery (seed 42, invoked by absolute path from `/tmp`) passed all eight suites and checks working-directory independence, option forwarding and discovery; it does not replace the full runs. The package stress command (`--mode stress --seed 42 --rounds 3`) was available but not run as completion evidence. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership.

### Re-audit — 2026-10-07

Package P006 was re-audited under the current rules, treating the previous verification as existing-unverified. The rows were compared with the code and the testers before any edit; the 24 confirmed findings in `00-Guidelines/23-Reaudit Findings/p006.md` were then fixed or resolved. Findings that concern this header:

| # | Finding | Disposition |
|---|---|---|
| 9 | Complexity line not directly above `struct DSU` | Fixed; alpha is defined on the complexity line. |
| 10–12, 17, 19, 23, 24 | Closing-brace rule (DSU, Fenwick, segment do-while, sqrt, ordered lambda, monotone lambdas, testers) | Fixed in every header, tester and the benchmark; `03-consistency.py --braces` reports nothing for the package. The tester anonymous namespaces now close with `} // namespace`. |
| 14 | Complexity lines split or not directly above the struct | Every struct and free function now has its single-line bound directly above it; contract prose moved to this document. |
| 20 | Methods not grouped | OrderedMultiSet and SortedVector are now grouped construction / access / mutation / queries with blank lines; the other headers were regrouped the same way. |

Changes beyond the findings: Comment cap: every header keeps at most two comment lines per struct or function and at most 8% comment lines; the removed contract text is under Contracts. The shared runner compiles every configuration with `-Wall -Wextra -Wconversion -Werror`; no header or tester produces a warning. The completeness sweep ([00-sources.md](00-sources.md)) added `makeSet`, tested interleaved with random unions.

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration   # before any edit
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The unchanged suites passed full mode with seed 20260927 before any edit. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. GCC 14.2 itself was not run. No online submission was made. The DSU suite passed in every run above.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 15 | DSU; the chapter 15 example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind reference. |

References inspected on 2026-09-27. Bibliographic mentions of Tarjan are not claims that the original proofs were inspected.

## Limits and handoffs

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts; its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The outstanding weighted/parity semantics belong to `29-weighteddsu.hpp`; rollback/component metadata belongs to `09-rollbackdsu.hpp` and Graphs dynamic-connectivity orchestration. Their inventories retain this handoff, and those packages remain planned. Rollback/persistent/weighted DSU are future variants, not gaps of this row. DSU grid adapters and delete/move element (HIT, OI) were left out of the 2026-10-07 sweep as variants for row 65 if a motivating problem appears. No P006-owned gap remains.
