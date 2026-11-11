# 01-dsu.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### DSU

Vertices `[0, n)` with `0 <= n <= INT_MAX`. Public `n`, `ncon`, `par` and `siz` must not be edited directly. Representatives are arbitrary and can change after a union. `findSet`, `size`, `isSameSet`, `leaders` and `groups` compress paths, so they are non-const. `uniteSets` returns whether two distinct components merged. `count` is the number of components. `groups` returns the components ordered by smallest vertex, each with members ascending, in O(n) time, workspace and output. `leaders` returns `findSet(u)` for every `u` in one vector, O(n) time and output. `makeSet` appends the singleton vertex `n` and returns its id; it requires `n < INT_MAX`. Union by size gives O(log(n)) worst-case and inverse-Ackermann amortized find and unite.

### Correctness and cost

Union by size bounds uncompressed depth by `log(n)`; compression preserves the root equivalence relation. Sizes and component count change only on successful unions. Amortized inverse-Ackermann union/find, O(n) initialization/storage. With no unions during grouping, each non-root edge is bypassed at most once, giving O(n) grouping including output. `makeSet` is a one-line composition of verified operations.

### Legacy mapping

- P276 rename, 2026-11-10: `getSize` → `size`.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`01-dsu_tester.cpp` group) | Oracle |
|---|---|---|
| `findSet`, `uniteSets`, `isSameSet`, `size`, `count`, `groups` | `graphSubsets`, `unionHistories` | Every simple graph through 6 vertices, self/duplicate unions, every union history on 3 vertices through length 5, searched by iterative deepening so the first failure is a shortest history; independent relabeling connectivity/size/group oracle |
| `leaders` | every `verify` (all groups) | Called first, before the checks compress paths: size `n`, `leaders[u] == findSet(u)` and the label oracle's class of `leaders[u]` equals that of `u` |
| `makeSet` | `randomCases` | 150 random cases with interleaved `makeSet` (id and singleton oracle) |
| Compression at scale | `balancedTree` | 65,536-vertex balanced merge/compression |
| Union by size (`O(log(n))` depth) | `checkDepths`, in every `verify` and after the 65,536-vertex stars `uniteSets(i,0)` and `uniteSets(0,i)` in `balancedTree` | `par` in range; a vertex at depth `k` of `par` lies in a component of at least `2^k` vertices (P251 mutation campaign) |
| Value semantics and preconditions | all groups | Copies/moves/reset; 9 assertion probes, including `find-past` (`u > n`) and `makeset-full` (`n == INT_MAX`; it sets the public `n` directly, a deliberate white-box exception to the Contracts rule that public members are not edited directly, checked build only) |

Quick: n<=4, depth 3, 35 trials, 1,024 vertices (balanced and stars). Stress: history depth 6, 600 random cases. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 723 MB; optimized 0.3 s compile + 2.8 s run, checked 16.4 s compile + 5.1 s run, ASan-UBSan 17.8 s compile + 62.8 s run; MEMORY peak 829 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 759 MB; optimized 0.2 s compile + 2.2 s run, checked 18.9 s compile + 3.5 s run, ASan-UBSan 15.3 s compile + 35.9 s run; MEMORY peak 845 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 24.1 s compile + 0.6 s run, checked 19.7 s compile + 10.7 s run; MEMORY peak 706 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 723 MB; optimized 0.1 s compile + 3.7 s run, checked 5.1 s compile + 6.9 s run, ASan-UBSan 5.1 s compile + 89.0 s run; MEMORY peak 652 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (DS01 `/finish`; experiment `none`), Linux 7.2 x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `01-dsu_tester.py --mode full --seed 20261112` (GCC 16.2.1) | PASS, 3 configurations, 9 assertion probes, MEMORY peak 527 MB |
| `CXX=g++-14 01-dsu_tester.py --mode full --seed 20261112` (GCC 14.4.1, floor) | PASS, 3 configurations, 9 assertion probes, MEMORY peak 537 MB |
| `01-dsu_tester.py --mode stress --seed 7` | PASS, 3 configurations, MEMORY peak 650 MB |
| `05-extraction.py ops --filter '02-Data Structures/01-dsu.hpp'`; `05-extraction.py testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run on the extracted build passes |
| `01-run.py --mode quick` (after this run's header edits) | PASS, 187 suites; NOTE: `00-Tools/15-harness_tester.py` (outside this package) failed beside other suites and passed alone |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 2722 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/01-dsu.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

DS01 added `leaders`, switched `unionHistories` to iterative deepening and changed the runner's death check, which now requires glibc's ``Assertion ` `` form, so a libstdc++ bounds abort no longer passes as the header's assert. Mutation campaign: [Mutation testing](#mutation-testing).

## Mutation testing

Record: mewt 4.0.0 on blob `5ea52ce75613de104c2c205aa4dba6008146c54f` (2026-11-11, seed 1, 5 lines changed since blob `2192e593eea9`): 11 mutants, 10 caught (0 by timeout), 1 survived, 0 untested.

Stage seconds of that campaign (DS01, `leaders`): quick 11, full on the optimized build 3, full on the other builds 50. The survivor is equivalent:

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 132`, `u < n` → `u != n` | 39 | `leaders` counts `u` up from 0 by 1 and `n >= 0`, so the loop stops at `n` either way. |

Earlier campaign (P276 rename of `getSize`, stage seconds quick 6): mewt 4.0.0 on blob `2192e593eea929c2bcd293464782e62d0be01ed6` (2026-11-10, seed 1, 1 lines changed since blob `7fcf47230655`): 3 mutants, 3 caught (0 by timeout), 0 survived, 0 untested.

Earlier campaign: mewt 4.0.0 on blob `7fcf472306554c4f232dbaa6087bfb3148c4feb3` (2026-11-10, seed 1, whole file): 140 mutants, 134 caught (5 by timeout), 6 survived, 0 untested.

Written by P252 (user decision 2026-11-04), rerun by P274 through the build cache (`--whole --jobs 6`, 2026-11-10, warm; the cold run's seconds in parentheses); procedure, machine and cost in [00-mutation.md](../../96-Local%20Testing/Docs/00-mutation.md). Stages: quick 140 → 29 survivors on 6 shards, then its five timeouts retested on 5 and timing out again, 207 s together (256); full on the optimized build 29 → 29 in 11 s on 6 (11); full on the checked and ASan-UBSan builds 29 → 6 in 103 s on 6 (140). The five timeouts (`COS 112, 114` on line 30, `COS 115, 117, 119` on line 31) make the root walk or the compression loop spin at a root. The six survivors (line numbers of blob `7fcf472`; mutant ids restart in each campaign) are equivalent; `COS 97` and `CR 58` only under the contract (representatives and `groups()` capacity are unspecified, user decision 2026-11-03):

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 86`, `n < INT_MAX` → `n != INT_MAX` | 16 | `n` is an `int`, so `n <= INT_MAX` always and the two conditions agree. |
| `AS 72`, `swap(u, v)` → `swap(v, u)` | 23 | `std::swap` is symmetric. |
| `COS 97`, `siz[u] < siz[v]` → `<=` | 23 | Under the contract: on equal sizes the other root becomes the parent (`DSU d(2); d.uniteSets(0,1); d.findSet(1)` gives 1 instead of 0), but representatives are arbitrary by contract and the depth invariant holds both ways. |
| `CR 58`, `res.reserve(ncon)` removed | 40 | Under the contract: only `groups().capacity()` changes; contents and order are identical. |
| `COS 126`, `u < n` → `u != n` | 41 | `u` starts at 0 and steps by 1, and `n >= 0`. |
| `COS 132`, `id[r] == -1` → `id[r] <= -1` | 43 | `id` holds only `-1` or indices in `[0, n)`. |

## Sources

| Source | Actual reading and use |
|---|---|
| [maspypy unionfind](https://maspypy.github.io/library/ds/unionfind/unionfind.hpp) | Fetched 2026-11-11: `get_all`, the source of `leaders`. |
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 15 | DSU; the chapter 15 example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind reference. |

References inspected on 2026-09-27. Bibliographic mentions of Tarjan are not claims that the original proofs were inspected.

## Limits and handoffs

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts; its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The weighted and parity semantics are `WeightedDSU` and `ParityDSU` in `29-weighteddsu.hpp` ([29-weighteddsu.md](29-weighteddsu.md#legacy-mapping), P035); the component edge counts `esz`/`getEsiz` are `RollbackDSUAggregate::edgeCount` in `09-rollbackdsu.hpp` ([09-rollbackdsu.md](09-rollbackdsu.md), P034), and Graphs owns dynamic-connectivity orchestration. Rollback, persistent and weighted DSU are separate rows, not gaps of this row. DSU grid adapters and delete/move element are left out with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions).

## Aliases

Renamed aliases (P276, 2026-11-10): `getSize`.

Search aliases (P263, grab --find): union-find, disjoint set union; Library Checker `unionfind`; 并查集, 并查集复杂度, 并查集应用, 不相交集合, 路径压缩, 按秩合并, 连通块.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 10 findings fixed, `makeSet` added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-03: P251 mutation campaign, tester gained the union-by-size depth check and probes `find-past`, `makeset-full`; quick, full on g++ and g++-14, and 2 stress rounds passed.
- 2026-11-04: P252 recorded the mutation campaign (140 mutants, 6 equivalent survivors) under Mutation testing; header and tests unchanged.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-10: P276 rename (`getSize` → `size`); full on g++ and g++-14, stress and extraction passed; changed-line mutation campaign recorded.
- 2026-11-11: P006 DS01: `leaders` added (sweep), `unionHistories` verifies by iterative deepening, the runner's death check requires glibc's assert; full on g++ and g++-14, stress, extraction and integration passed; changed-line mutation campaign recorded (1 equivalent survivor).
- 2026-11-12: DS01 `/finish`: header unchanged (a briefly added `MovableDSU` was reverted, the header is again blob `5ea52ce`); the stale omission note on delete/move (row 65 `MovableDSU`, P035) fixed, grid adapters left out (user decision); full on g++ and g++-14, stress, extraction and integration passed; the 2026-11-11 mutation record stands.
