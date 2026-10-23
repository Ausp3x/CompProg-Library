# 29-weighteddsu.hpp — evidence

Package P035 (batch DS25). New header. It closes the weighted and parity part of the row 01 legacy handoff ([01-dsu.md](01-dsu.md#limits-and-handoffs)) and reuses row 09's undo contract ([09-rollbackdsu.md](09-rollbackdsu.md)) by inheritance; row 09 does not depend on this header. Groups are row 00 monoids with `inv` ([00-monoids.md](00-monoids.md#group-contract)). Preconditions are `assert`s checked once per call; `WeightedDSU::findSet` asserts and calls the recursive `root`, whose depth is at most `floor(log2(n)) + 1` under union by size.

## Contracts

### Relations and groups

`G` provides `S` (with `==`), `op` (associative), `e` (two-sided identity) and `inv` (two-sided inverse), under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): `G mon = {}` is the constructor's last argument and is kept as `m`, so presets (`Sum<T>`, `Xor<T>`), stateful structs (a runtime modulus) and user groups all work. `G` need not be commutative.

Each vertex `u` carries an unknown value `x_u` in `S`. `uniteSets(u, v, w)` records the relation `x_u^-1 x_v = w`, that is `x_v = op(x_u, w)` (for `Sum`: `x_v - x_u = w`; for `Xor<bool>`: `x_u xor x_v = w`). `diff(u, v)` is `x_u^-1 x_v` as implied by the accepted relations, so `diff(v, u) = inv(diff(u, v))` and `diff(u, v) · diff(v, y) = diff(u, y)`. Relations compose along a chain in chain order, which is why the orientation matters for non-commutative groups.

### WeightedDSU<G>

Vertices `[0, n)`, `0 <= n <= INT_MAX`. Union by size (on equal sizes the first argument's root stays the root) with path compression; representatives are arbitrary and may change after a union.

- `uniteSets(u, v, w)` returns `true` exactly when it merges two components. If `u` and `v` are already connected, it returns `false`. A consistent relation changes nothing; a contradicting one (`diff(u, v) != w`) increments its component's contradiction count and is otherwise ignored, so the potentials always satisfy every accepted relation. `uniteSets(u, u, w)` contradicts exactly when `w != e`.
- `potential(u)`: `x_r^-1 x_u` for `r = findSet(u)`. It changes when the root changes. Returns `vector<S>::const_reference`, valid until the next `uniteSets` or `findSet`-based call.
- `diff(u, v)`: asserts that `u` and `v` are connected.
- `contradictions(u)`: number of contradicting `uniteSets` calls inside `u`'s component, summed through merges. `contradictions(u) == 0` means the accepted relations are the only ones recorded (bipartiteness for `Xor<bool>` with `w = true` on every edge).
- `findSet`, `isSameSet`, `getSize`, `count`, `groups` as row 01 `DSU` (`groups` in O(n), groups and members ordered by smallest vertex).
- A root's stored potential is always `e`. A union costs two `op` and one or two `inv`; compressing a node costs one `op`. Bounds: O(alpha(n)) amortized group operations per call, O(n) memory of `S`.
- `Sum<T>` over a signed `T` needs every value and every sum along a chain of relations to be representable, and no value equal to the lowest `T` (negation).

### ParityDSU

Vertices `[0, n)`, `0 <= n <= INT_MAX / 2`. A row 01 `DSU` over `2n` nodes: node `u` is "same parity as `u`" and node `u + n` is "opposite parity of `u`".

- `uniteSets(u, v, w)` records `parity(u) xor parity(v) = w` (`false`: same, `true`: different) by two DSU unions. It returns `true` exactly when `u` and `v` were unrelated before the call (`parity(u, v) == -1`). Every relation is applied. A contradicting one merges a component with its mirror, which makes the component non-bipartite, as an odd cycle does in a graph. Callers who must reject contradictions check `parity` first.
- `parity(u, v)`: `0` when some chain of relations of even total parity links `u` and `v`, else `1` when an odd chain does, else `-1`. In a component that is not bipartite both exist, so the result is `0` for every pair there.
- `isBipartite(u)`: no odd closed chain in `u`'s component.
- O(alpha(n)) amortized per call, memory O(n).
- `WeightedDSU<Xor<bool>>` differs: it ignores and counts contradicting relations, so its potentials stay a valid 2-colouring of the accepted relations.

### RollbackWeightedDSU<G>

`RollbackDSU` (row 09) plus `WeightedDSU`'s relation contract, without path compression. Depth stays at most `floor(log2(n))`, so every query and `uniteSets` costs O(log(n)) worst case, and `undo` costs O(1) plus one integer copy. Memory O(n + q) for q live records.

- It inherits `findSet`, `isSameSet`, `getSize`, `count`, `groups` and `snapshot`. It defines its own `uniteSets(u, v, w)`, `undo` and `rollback`, which keep the contradiction counts. Never call `RollbackDSU::uniteSets`, `undo` or `rollback` on it (qualified, or through a base reference).
- History: as row 09, every `uniteSets` call is one record, merging, consistent-redundant or contradicting. `undo` reverts the latest record and restores its component's contradiction count; `rollback(t)` undoes down to `snapshot() == t`. Tokens are counts.
- `climb(u)`: the root of `u` and `x_root^-1 x_u` in one walk. `potential(u)` is its second part, `diff(u, v)` asserts a common root and `contradictions(u)` reads the root.
- A root's stored potential is never read, so `undo` leaves a separated root's stale value in place.

### Violated preconditions

Under `NDEBUG`, an out-of-range vertex reads outside the arrays, `diff` across components returns a meaningless product, `undo` on an empty history reads an empty vector, `ParityDSU(N)` with `N > INT_MAX / 2` overflows `2N`, and a negative size makes `vector` throw `length_error`. `rollback(t)` stops at an empty history for `t < 0` and does nothing for `t > snapshot()`. Every loop and recursion is bounded by the depth or the history length.

### Legacy mapping

- `OLD/algorithms.cpp:1402–1477` and `OLD/Team Notebook/src/graph/dsu.cpp` `DSU`: `uniteSets(u, v, w)` recorded `x_u - x_v = w`, which is `WeightedDSU<Sum<lng>>::uniteSets(v, u, w)`; `dis` and `getDis` (narrowed to `int`) → `potential`; `is_bip` and `isBipartite` with the default `w = 1` → `ParityDSU::uniteSets(u, v, true)` and `isBipartite`, or `WeightedDSU<Xor<bool>>::contradictions(u) == 0`; `esz` and `getEsiz` → row 09 `RollbackDSUAggregate::edgeCount`.
- Planned `NoncommutativeWeightedDSU` → `WeightedDSU<G>` with a non-commutative `G` (decision recorded in `duplicates.md`, Kept by the sweep).
- Catalog names: maspypy `get(v)` → `findSet` and `potential`; maspypy `get_path`, noshi91 `distance`, Nyaan and suisen `diff` → `diff`; ei1333 `weight` → `potential`; Nyaan and hitonanode `unite`/`merge` returning `false` on conflict → `isSameSet` and `diff` before `uniteSets`, or `contradictions`; ei1333 `BipartiteGraph` and cp-algorithms parity DSU → `ParityDSU`; OI Wiki extended-domain DSU (k copies, NOI2011 food chain) → `WeightedDSU` over Z/k (`Sum<ModInt>` or a runtime `ModAdd`); maspypy `time()`/`rollback(t)` → `snapshot`/`rollback`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/29-weighteddsu_tester.cpp`, entry `29-weighteddsu_tester.py` (driver `_00_runner.py`; three builds; 13 assertion probes). Groups: `Sum<lng>`, `Xor<bool>`, `Perm` (S5 under composition, non-commutative) and `ModAdd` (runtime modulus chosen by the seed from 7, 10^9+7 and 998244353). History groups run 300 rounds each in full mode (30 quick, 1000 stress; `ModAdd` and the rollback `Xor<bool>` half, the n <= 40 group a tenth).

| Operation | Group | Oracle |
|---|---|---|
| `WeightedDSU` `uniteSets` (return value), `findSet`, `potential`, `diff`, `isSameSet`, `getSize`, `count`, `groups`, `contradictions` | `weightedHistories` over all four groups (n <= 12, 60 steps; `Perm` also n <= 40 with 150 steps), relations true to hidden values 3 times in 4 and random otherwise; half the rounds check after every step, the rest every 16 steps (deeper uncompressed paths) | Brute force: BFS over the accepted relations composes edge labels in chain order (`d[b] = op(d[a], w)`); a relation is accepted when its endpoints were unreachable, and contradiction events are counted per BFS component; `findSet` must be a member and the same vertex for the whole component; exact `groups` order |
| `ParityDSU` `uniteSets` (return value), `parity`, `isBipartite` | `parityHistories` (n <= 10, 40 steps, every pair after every step; relations true to hidden colours 5 times in 6) | Brute force: BFS over (vertex, parity) states using every relation |
| `RollbackWeightedDSU` `uniteSets`, `undo`, `snapshot`, `rollback` (random targets and reused tokens), `climb` through `potential` and `diff`, `contradictions`, inherited queries | `rollbackHistories` over `Sum<lng>`, `Perm`, `Xor<bool>` (n <= 10, 50 steps) | The brute force rebuilt from the live record list after every operation; the `uniteSets` return value against the rebuilt brute force before the call |
| Scale, depth bound | `large`: n = 2 * 10^5 and 2^16 with 4 * 10^5 and 3 * 10^5 operations (full); n = 10^6 with 3 * 10^6 (stress) | Hidden potentials `x` with wrong relations only inside components: `diff == x_v - x_u`; row 01 `DSU` (weighted) and row 09 `RollbackDSU` driven in parallel (rollback) for connectivity, sizes, counts and per-component contradiction counts from a replay of the live records; rollback depth at most `floor(log2(n))`; `ParityDSU` against hidden colours and per-component odd flags |
| Empty structures, self relations, copies, non-commutative composition and its swapped order, `undo` of a contradiction, runtime-modulus diffs, empty-group layout | `edges` | Direct values; `sizeof` against a mirror struct without `m` |
| Preconditions | `invalid` | Out-of-range and negative vertices, negative size, `diff` across components, oversized and negative `ParityDSU`, out-of-range `parity` and `isBipartite`, empty `undo`, future `rollback`, rollback `diff` across components, out-of-range `potential` |

Mutation check (2026-10-23, quick mode, seed 3): eight seeded faults were each caught: union orientation, compression order, the missing inverse on a swapped union, lost contradiction counts on merge and on `undo`, a rollback potential stored unoriented, `climb` composing in reverse, and `parity` preferring odd chains.

## Commands and results

Latest run 2026-10-23, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `29-weighteddsu_tester.py --mode full --seed 20261023` (GCC 16.2.1) | PASS, 3 configurations, 15,257,795 checks per configuration, 13 assertion probes, MEMORY peak 869 MB |
| `CXX=g++-14 29-weighteddsu_tester.py --mode full --seed 20261023` (GCC 14.4.1, floor) | PASS, 15,257,795 checks per configuration, MEMORY peak 849 MB |
| `29-weighteddsu_tester.py --mode stress --seed 7` | PASS, 59,186,521 checks per configuration, MEMORY peak 1205 MB |
| `02-integration.py` | PASS, 174 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 714 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/29-weighteddsu_tester.py' --mode full --seed 20261023
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/29-weighteddsu_tester.py' --mode full --seed 20261023
python3 '96-Local Testing/02-Data Structures/29-weighteddsu_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `potentialized_unionfind.hpp`, `rollback_potentialized_unionfind.hpp`; ei1333 `weighted-union-find.hpp`, `bipartite-graph.hpp`; Nyaan `union-find-with-potential.hpp`; suisen `weighted_union_find.hpp`; hitonanode `potentialized_unionfind.hpp`; tko919 `weightedunionfind.hpp`; noshi91 `potentialized_union_find.cpp` | Potentials over groups and their orientation, `getSize`/`count`/`groups`, rollback by time token, the 2n-node parity DSU. |
| cp-algorithms *Disjoint Set Union*, OI Wiki *并查集* | Distance to the representative, online bipartiteness by parity, extended-domain DSU. |

Fetched 2026-10-23 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-23-p035-sweep)); the implementation was written from the contract. The Library Checker statements `unionfind_with_potential` and `unionfind_with_potential_non_commutative_group` were not read (the pages render client-side).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p035-omissions): a persistent weighted DSU.
- Row 09 owns the generic undo substrate, row 63 drives rollback structures for offline deletion, and Graphs `26-dynamic_connectivity.hpp` owns bipartiteness under deletions. Component-wide actions with point reads are row 65 `DualDSU`.

## History

- 2026-10-23: P035 first implementation (user decisions: `ParityDSU` as a 2n-node DSU, `inv` on row 00 `Sum`/`Xor`); independent review, 7 findings fixed across the package (here: `findSet` split from the recursive `root`, a negative `ParityDSU` probe); full (seed 20261023) on g++ and g++-14 and stress (seed 7) passed.
