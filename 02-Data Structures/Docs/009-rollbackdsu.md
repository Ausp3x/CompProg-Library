# 009-rollbackdsu.hpp — Evidence

Package P034 (batch DS03), a new header. It carries the row 01 legacy handoff ([001-dsu.md](001-dsu.md#limits-and-handoffs)): the old DSU's component edge counts `esz`/`getEsiz` (`OLD/algorithms.cpp:1402–1477`) are `RollbackDSUAggregate::edgeCount`. Preconditions are `assert`s checked once per call.

## Contracts

### RollbackDSU

Vertices are `[0, n)` with `0 <= n <= INT_MAX`. Union by size; on equal sizes the first argument's root stays the root. Representatives are arbitrary.

- There is no path compression: `findSet` follows parents. Union by size keeps every depth at most `floor(log2(n))`, because each step up at least doubles the subtree size, so `findSet`, `isSameSet`, `size` and `uniteSets` cost O(log(n)) worst case. That worst-case bound is what keeps rollback sound: path compression's amortized bound does not survive undo.
- History: every `uniteSets` call appends exactly one record, merging or redundant (self-loops included).
  - `undo()` reverts the latest record in O(1) and asserts a nonempty history.
  - `snapshot()` is the record count, used as a token.
  - `rollback(t)` asserts `0 <= t <= snapshot()` and undoes records until `snapshot() == t`, in O(snapshot() − t).
  - A token is a count: after undoing below it and adding new records, it names the new prefix of that length. Copies are independent snapshots.
- `count()` is the number of components.
- `groups()` runs in O(n) by a memoized walk; groups are ordered by smallest vertex and members ascend, as in row 01 `DSU::groups`.
- Memory is O(n + q) for q live records.

### RollbackDSUAggregate<M>

`M` provides `S` and `op(a, b)`, commutative and associative, under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): `RollbackDSUAggregate(a, mon = {})` keeps it as `m`, so static presets, stateful structs and `Monoid` lambdas all work. `e()` is not used. Commutativity is needed because which root absorbs the other depends on sizes, not on argument order. The constructor takes the initial vertex values `a`; `n = a.size()`.

- It inherits `findSet`, `isSameSet`, `size`, `count`, `groups` and `snapshot`, and defines its own `uniteSets`, `undo` and `rollback`, which also maintain the aggregates. Never call `RollbackDSU::uniteSets`, `undo` or `rollback` on an aggregate (qualified, or through a base reference): they skip the aggregate state.
- `componentValue(u)` is `op` over `a[w]` for every member `w` and over every live `addValue` on the component. It returns `vector<S>::const_reference`, valid until the next mutation.
- `addValue(u, x)` records one entry (counted by `snapshot`, reverted by `undo`) and folds `x` into `u`'s component. History is LIFO, so an `addValue` is reverted before any union that formed its component, which makes it the same as folding `x` into `u`'s own value.
- `edgeCount(u)` is the number of live `uniteSets` records whose endpoints lie in `u`'s component, merging and redundant, self-loops included: the legacy `esz`. `edgeCount(u) - size(u) + 1` is the component's cycle rank, so `edgeCount(u) >= size(u)` exactly when the component has a cycle (multi-edges and self-loops count).
- Each record saves the touched root's value and edge count, and `undo` restores them, so `M` needs no inverse. Memory is O(n + q) values of `S`.
- `uniteSets` and `addValue` cost O(log(n)) plus one `op`; `undo` costs O(1) plus one copy of `S`. Edge counts are `int`: at most `INT_MAX` live records.

### Violated Preconditions

Under `NDEBUG`, an out-of-range vertex reads outside the arrays and `undo` on an empty history reads an empty vector (undefined). `rollback(t)` stops at an empty history for `t < 0` and does nothing for `t > snapshot()`. Every loop is bounded by the depth or the history length.

### Legacy Mapping

- `historySize` (planned row) left the row by the user's decision of 2026-10-17: `snapshot()` returns the history length.
- `RollbackDSUAggregate::contradictions` (planned row) became `edgeCount` by the user's decision of 2026-10-17. Potential and parity contradictions belong to row 29.
- `esz`, `getEsiz` (`OLD/algorithms.cpp`) → `edgeCount`.
- Catalog names: KACTL `time()`/`rollback(t)`, Nyaan `get_state()`/`rollback(state)`, tko919 `get()` and maspypy `time()` → `snapshot`/`rollback`; ACL `leader` → `findSet`; hitonanode `set_weight` → `addValue` (folding instead of overwriting); maspypy and hitonanode `reset()` → `rollback(0)`.
- P276 rename, 2026-11-10: `getSize` → `size`.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/009-rollbackdsu_tester.cpp`, entry `009-rollbackdsu_tester.py` (driver `_00_contest_runner.py`, three builds, 10 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `uniteSets` (return value), `undo`, `snapshot`, `rollback`, `findSet`, `isSameSet`, `size`, `count`, `groups` | `baseHistories` (n <= 12 with 60 steps, n <= 60 with 200 steps; self-loops, redundant unions, tokens reused after undos) | Brute force from the live record list: BFS components; `findSet` must be a member and the same vertex for the whole component; exact `groups` order |
| `componentValue`, `addValue`, `edgeCount`, aggregate `uniteSets`, `undo`, `rollback` | `aggregateHistories<SumMax>` (sum mod 1e9+7, maximum and its multiplicity), `<Or>` (`bool` storage); final `rollback(0)` | Brute force: per-component fold of initial values and `addValue` operands; edge counts from unite records |
| Depth bound and large histories | `large`: n = 2 * 10^5 and 2^16 with up to 4 * 10^5 steps of unions and batch rollbacks (full); n = 10^6 with 3 * 10^6 steps (stress) | Checkpoints rebuild row 01's path-compressed `DSU` from the live edges (count, sizes, 2000 random pairs); maximum depth at most `floor(log2(n))` |
| Empty structure, copies, self-loops, redundant counts, `bool` aggregate; runtime monoid state | `edges` | Direct values; component sums modulo a member modulus (5 and 10^9+7) through unions, `addValue` and `rollback` |
| Preconditions | `invalid` | Out-of-range and negative vertices, empty `undo`, future and negative `rollback`, negative size, aggregate empty `undo`, future `rollback`, out-of-range `addValue` |

## Commands and Results

Machine: Linux x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.7; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 782 MB | 0.2 + 2.6 | 17.0 + 3.6 | 19.4 + 21.9 | 994 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 854 MB | 0.2 + 2.8 | 26.7 + 5.3 | 33.4 + 17.2 | 1035 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 23.5 + 0.6 | 20.8 + 2.4 | | 786 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 789 MB | 0.1 + 5.1 | 6.7 + 8.8 | 8.2 + 54.7 | 848 MB |

Package runs:

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P276) | full on g++ and g++-14, stress, and `python3 '96-Local Testing/05-extraction.py'` on this header | passed (counts not recorded) |
| 2026-10-17 (P027) | `python3 '96-Local Testing/02-Data Structures/009-rollbackdsu_tester.py' --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 3,714,962 checks per configuration, 10 assertion probes, MEMORY peak 502 MB |
| 2026-10-17 (P027) | `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/009-rollbackdsu_tester.py' --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 3,714,962 checks per configuration, MEMORY peak 525 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/02-Data Structures/009-rollbackdsu_tester.py' --mode stress --seed 7` | PASS, 27,165,990 checks per configuration, MEMORY peak 824 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace, sanitizer self-tests; MEMORY peak 1834 MB |
| 2026-10-17 (P027) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| KACTL `UnionFindRollback.h`, ei1333 `union-find-undo.hpp`, Nyaan `rollback-union-find.hpp`, suisen `undo_union_find.hpp`, maspypy `rollback_unionfind.hpp`, hitonanode `undo_unionfind.hpp`, tko919 `rollbackunionfind.hpp`, cp-algorithms *Deleting from a data structure* | Time tokens, recording redundant unions (ei1333, hitonanode, tko919) or not (KACTL, Nyaan, suisen, maspypy), `groups`, why amortized path compression is unusable. |
| suisen `undo_union_find_component_sum.hpp`, hitonanode `undo_monoid_unionfind.hpp` | Commutative component aggregates; undoable component weights (`addValue`). |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Left out, with reasons in [00-notes.md](00-notes.md#p034-omissions): `makeSet`, a reporting `undo`, union by rank, a history-clearing `snapshot`, `reset`.
- Row 29 owns potential and parity DSUs and their rollback variant; row 63 drives this structure for offline deletion; Graphs `026-dynamic_connectivity.hpp` owns connectivity orchestration.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Renamed aliases (P276, 2026-11-10): `getSize`.

Search aliases (P263, grab --find): 并查集, 可撤销并查集, 可回滚并查集, 撤销并查集.

## History

- 2026-10-17: P034 first implementation and verification (user decisions on `snapshot`, `edgeCount`); `rollback(t)` bounded for `t < 0` under `NDEBUG`; the independent review found no defect in this header and added the `rollback` cost to the complexity line.
- 2026-10-17: P027 monoid pass (stored-object monoid contract for `RollbackDSUAggregate`, stateful-monoid test); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 502/525/824 MB.
- 2026-11-07: P263 added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-10: P276 rename (`getSize` → `size`); full on g++ and g++-14, stress and extraction passed.
