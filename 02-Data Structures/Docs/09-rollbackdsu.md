# 09-rollbackdsu.hpp — evidence

Package P034 (batch DS03). New header. It carries the row 01 legacy handoff ([01-dsu.md](01-dsu.md#limits-and-handoffs)): the old DSU's component edge counts `esz`/`getEsiz` (`OLD/algorithms.cpp:1402–1477`) are `RollbackDSUAggregate::edgeCount`. Preconditions are `assert`s checked once per call.

## Contracts

### RollbackDSU

Vertices `[0, n)`, `0 <= n <= INT_MAX`. Union by size; on equal sizes the first argument's root stays the root. Representatives are arbitrary.

- No path compression: `findSet` follows parents. Union by size keeps every depth at most `floor(log2(n))`, because each step up at least doubles the subtree size. So `findSet`, `isSameSet`, `getSize` and `uniteSets` cost O(log(n)) worst case. That worst-case bound is what keeps rollback sound: path compression's amortized bound does not survive undo.
- History: every `uniteSets` call appends exactly one record, merging or redundant (self-loops included).
  - `undo()` reverts the latest record in O(1) and asserts a nonempty history.
  - `snapshot()` is the record count, used as a token.
  - `rollback(t)` asserts `0 <= t <= snapshot()` and undoes records until `snapshot() == t`, in O(snapshot() − t).
  - A token is a count: after undoing below it and adding new records, it names the new prefix of that length. Copies are independent snapshots.
- `count()`: number of components.
- `groups()`: O(n) by a memoized walk. Groups are ordered by smallest vertex and members ascend, as in row 01 `DSU::groups`.
- Memory O(n + q) for q live records.

### RollbackDSUAggregate<M>

`M` provides `S` and `op(a, b)`, commutative and associative, under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): `RollbackDSUAggregate(a, mon = {})` keeps it as `m` (static presets, stateful structs and `Monoid` lambdas all work; no `e` is needed). Commutativity is needed because which root absorbs the other depends on sizes, not on argument order. `e()` is not used. The constructor takes the initial vertex values `a`; `n = a.size()`.

- Inherits `findSet`, `isSameSet`, `getSize`, `count`, `groups` and `snapshot`. Defines its own `uniteSets`, `undo` and `rollback`, which also maintain the aggregates. Never call `RollbackDSU::uniteSets`, `undo` or `rollback` on an aggregate (qualified, or through a base reference): they skip the aggregate state.
- `componentValue(u)`: `op` over `a[w]` for every member `w`, and over every live `addValue` on the component. Returns `vector<S>::const_reference`, valid until the next mutation.
- `addValue(u, x)`: records one entry (counted by `snapshot`, reverted by `undo`) and folds `x` into `u`'s component. History is LIFO, so an `addValue` is reverted before any union that formed its component. That makes it the same as folding `x` into `u`'s own value.
- `edgeCount(u)`: number of live `uniteSets` records whose endpoints lie in `u`'s component, merging and redundant, self-loops included. This is the legacy `esz`. `edgeCount(u) - getSize(u) + 1` is the component's cycle rank, so `edgeCount(u) >= getSize(u)` exactly when the component has a cycle (multi-edges and self-loops count).
- Each record saves the touched root's value and edge count, and `undo` restores them, so `M` needs no inverse. Memory O(n + q) values of `S`.
- `uniteSets` and `addValue` cost O(log(n)) plus one `op`; `undo` costs O(1) plus one copy of `S`. Edge counts are `int`: at most `INT_MAX` live records.

### Violated preconditions

Under `NDEBUG`, an out-of-range vertex reads outside the arrays and `undo` on an empty history reads an empty vector (undefined). `rollback(t)` stops at an empty history for `t < 0` and does nothing for `t > snapshot()`. Every loop is bounded by the depth or the history length.

### Legacy mapping

- `historySize` (planned row) left the row by the user's decision of 2026-10-17: `snapshot()` returns the history length.
- `RollbackDSUAggregate::contradictions` (planned row) became `edgeCount` by the user's decision of 2026-10-17. Potential and parity contradictions belong to row 29.
- `esz`, `getEsiz` (`OLD/algorithms.cpp`) → `edgeCount`.
- Catalog names: KACTL `time()`/`rollback(t)`, Nyaan `get_state()`/`rollback(state)`, tko919 `get()` and maspypy `time()` → `snapshot`/`rollback`; ACL `leader` → `findSet`; hitonanode `set_weight` → `addValue` (folding instead of overwriting); maspypy and hitonanode `reset()` → `rollback(0)`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/09-rollbackdsu_tester.cpp`, entry `09-rollbackdsu_tester.py` (driver `_00_runner.py`; three builds; 10 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `uniteSets` (return value), `undo`, `snapshot`, `rollback`, `findSet`, `isSameSet`, `getSize`, `count`, `groups` | `baseHistories` (n <= 12 with 60 steps, n <= 60 with 200 steps; self-loops, redundant unions, tokens reused after undos) | Brute force from the live record list: BFS components; `findSet` must be a member and the same vertex for the whole component; exact `groups` order |
| `componentValue`, `addValue`, `edgeCount`, aggregate `uniteSets`, `undo`, `rollback` | `aggregateHistories<SumMax>` (sum mod 1e9+7, maximum and its multiplicity), `<Or>` (`bool` storage); final `rollback(0)` | Brute force: per-component fold of initial values and `addValue` operands; edge counts from unite records |
| Depth bound and large histories | `large`: n = 2 * 10^5 and 2^16 with up to 4 * 10^5 steps of unions and batch rollbacks (full); n = 10^6 with 3 * 10^6 steps (stress) | Checkpoints rebuild row 01's path-compressed `DSU` from the live edges (count, sizes, 2000 random pairs); maximum depth at most `floor(log2(n))` |
| Empty structure, copies, self-loops, redundant counts, `bool` aggregate; runtime monoid state | `edges` | Direct values; component sums modulo a member modulus (5 and 10^9+7) through unions, `addValue` and `rollback` |
| Preconditions | `invalid` | Out-of-range and negative vertices, empty `undo`, future and negative `rollback`, negative size, aggregate empty `undo`, future `rollback`, out-of-range `addValue` |

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `09-rollbackdsu_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 3,714,962 checks per configuration, 10 assertion probes, MEMORY peak 502 MB |
| `CXX=g++-14 09-rollbackdsu_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 3,714,962 checks per configuration, MEMORY peak 525 MB |
| `09-rollbackdsu_tester.py --mode stress --seed 7` | PASS, 27,165,990 checks per configuration, MEMORY peak 824 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/09-rollbackdsu_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/09-rollbackdsu_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/09-rollbackdsu_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| KACTL `UnionFindRollback.h`, ei1333 `union-find-undo.hpp`, Nyaan `rollback-union-find.hpp`, suisen `undo_union_find.hpp`, maspypy `rollback_unionfind.hpp`, hitonanode `undo_unionfind.hpp`, tko919 `rollbackunionfind.hpp`, cp-algorithms *Deleting from a data structure* | Time tokens, recording redundant unions (ei1333, hitonanode, tko919) or not (KACTL, Nyaan, suisen, maspypy), `groups`, why amortized path compression is unusable. |
| suisen `undo_union_find_component_sum.hpp`, hitonanode `undo_monoid_unionfind.hpp` | Commutative component aggregates; undoable component weights (`addValue`). |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p034-omissions): `makeSet`, reporting `undo`, union by rank, history-clearing `snapshot`, `reset`.
- Row 29 owns potential and parity DSUs and their rollback variant; row 63 drives this structure for offline deletion; Graphs `26-dynamic_connectivity.hpp` owns connectivity orchestration.

## History

- 2026-10-17: P034 first implementation and verification (user decisions on `snapshot`, `edgeCount`); `rollback(t)` bounded for `t < 0` under `NDEBUG`; independent review: no defect in this header, `rollback` cost added to the complexity line.
- 2026-10-17: P027 monoid pass (stored-object monoid contract for `RollbackDSUAggregate`, stateful-monoid test added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 502/525/824 MB.
