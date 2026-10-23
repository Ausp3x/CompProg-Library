# 65-dsu_extensions.hpp — evidence

Package P035 (batch DS52). New header. It depends on row 01 `DSU`, row 61 `HashMap` and Miscellaneous `02-customhash.hpp` (the default hash) only. `LinkedDSU` and `ComponentSumDSU` extend `DSU` and inherit its `findSet`, `isSameSet`, `getSize`, `count` and `groups`; `SparseDSU` holds a `DSU`, and `MergeHistoryForest` builds with a local one. `DualDSU` keeps its own arrays, because its path compression must update tags. Groups and monoids follow row 00 ([00-monoids.md](00-monoids.md#group-contract)). Preconditions are `assert`s checked once per call; `DualDSU::findSet` asserts and calls the recursive `root`, whose depth is at most `floor(log2(n)) + 1` under union by size.

## Contracts

### LinkedDSU

Row 01 `DSU` whose components are also circular singly linked lists through `nxt`. A union swaps `nxt[u]` and `nxt[v]`, which splices the two cycles into one.

- `uniteSets(u, v)`: as `DSU::uniteSets`, plus the splice. `makeSet()`: as `DSU::makeSet`, a new singleton cycle. Never call `DSU::uniteSets` or `DSU::makeSet` on it (qualified, or through a base reference): they skip `nxt`.
- `members(u)`: `u`'s component, starting at `u` and following the cycle; not sorted. O(size); the cycle order of a component is the same from every member, rotated.
- `unionEnumerate(u, v, f)`: if `u` and `v` are in different components `A` and `B`, calls `f(a, b)` for every `a` in `A` and `b` in `B` (each newly connected pair once, `a` from `u`'s side) before uniting them, and returns `true`; otherwise it calls nothing and returns `false`. O(|A| * |B| + |A| + |B|) time, O(|A| + |B|) workspace. Summed over a run this is at most `n(n - 1) / 2` calls.

### ComponentSumDSU<M>

`M` provides `S` and `op`, commutative and associative, under the stored-object contract (`mon` kept as `m`; `e` is not used). Commutativity is needed because which root absorbs the other depends on sizes, not on argument order. The constructor takes the initial values `a`; `n = a.size()`.

- `componentValue(u)`: `op` over the initial values of `u`'s component and every `addValue` on it. Returns `vector<S>::const_reference`, valid until the next non-const call.
- `addValue(u, x)`: folds `x` into `u`'s component, which equals folding it into `u`'s own value.
- `uniteSets(u, v)`: as `DSU::uniteSets`, folding the two values on a merge. `makeSet(x)`: a new singleton with value `x`. Never call `DSU::uniteSets` or `DSU::makeSet` on it.
- O(alpha(n)) amortized plus one `op` per call. Row 09 `RollbackDSUAggregate` is the undoable variant without path compression.

### DualDSU<G>

Values in a group `G` (`S`, `op`, `e`, `inv`), not necessarily commutative. The constructor takes the initial values `a`; `n = a.size()`. Union by size with path compression.

- `apply(u, x)`: replaces the value `y` of every vertex in `u`'s component by `op(y, x)` (the action applied after the earlier ones, as `MonoidAction` composes).
- `get(u)`: the current value of `u`. A vertex's value is the product of the tags from the vertex up to its root, in that order; `apply` multiplies the root's tag on the right.
- `uniteSets(u, v)`: merges without changing any value. The absorbed root's tag becomes `op(tag, inv(tag of the new root))`, which cancels the new root's earlier actions. Returns whether it merged. `findSet`, `isSameSet`: as `DSU`.
- O(alpha(n)) amortized group operations per call, memory O(n).
- Typical use: add `x` to every vertex of a component (`Sum<lng>`), or expected values per component accumulated before merges (AtCoder ABC314F).

### SparseDSU<K, H>

Keys of any type `K` that `H` (default `CustomHash`) hashes and `==` compares: integers of any width, strings, pairs, tuples. `index(k)` maps a key to a dense id, creating it with `DSU::makeSet` on first use; `key[id]` maps back. Only `uniteSets` creates keys.

- `uniteSets(a, b)`: as `DSU::uniteSets` on the keys' ids.
- `findSet(k)`: the representative key of `k`'s component; `k` itself for a key never united. `isSameSet`, `getSize` (1 for a key never united) read without allocating.
- O(alpha(k)) amortized plus O(1) expected hashing per call, memory O(k) for `k` distinct united keys. The hash's process seed and adversarial behaviour are row 61's ([61-hashmap.md](61-hashmap.md)).

### MergeHistoryForest

Leaves `[0, n)`, `0 <= n <= 2^30`; built from a union sequence `unions` of `q` pairs. Union `i` (0-based) happens at time `i + 1`, and time `t` is the state after the first `t` unions. A union of two different components makes the next node: node `n + k` for the `k`-th merging union, with the two components' nodes as children. Redundant unions make nothing. Leaves have time 0. Times grow strictly from a node to its parent.

- `nodeTime(x)`: the time node `x` was created.
- `mergeTime(x)`: the time node `x`'s component was absorbed (its parent's time), or `-1` if it never was. Node `x` is a component during `[nodeTime(x), mergeTime(x))`.
- `earliestConnectTime(u, v)`: the first time `u` and `v` are connected: 0 when `u == v`, `-1` if never. It is the time of their lowest common ancestor.
- `componentAt(u, t)`: the node that is `u`'s component at time `t >= 0` (any `t >= q` gives the root): the highest ancestor with time `<= t`.
- `leafRange(x)`: `[l, r)` such that `leaves[l..r)` are the members of node `x`'s component. The leaves of every subtree are contiguous, because each root's subtree gets one block and each node's heavy child precedes the other.
- Heavy paths: `child[x][0]` is the child with at least as many leaves (`cnt`), so a light child has at most half its parent's leaves and every leaf-to-root path crosses at most `log2(n)` light edges. A parent's id exceeds its children's, so one pass over decreasing ids sets `depth`, the path head `head`, `start`, and `pos`, which gives each heavy path a contiguous block from its head down (`order` inverts it). Times fall strictly along a block.
- `earliestConnectTime` lifts the vertex whose path head is deeper to the head's parent until both share a path (`-1` when it passes a root) and returns the time of the shallower one. `componentAt` lifts `u` likewise while the head's time is `<= t`, returning the head when its parent is absent or later than `t`; otherwise it binary searches the block between the head and `u` for the first time `<= t`.
- Construction O(n + q * alpha(n)), queries O(log(n)), memory O(n): ten `int` per node over `m <= 2n - 1` nodes plus `leaves`, about 84 MB at n = 10^6.

### Violated preconditions

Under `NDEBUG`, out-of-range vertices or nodes read outside the arrays, and a negative `t` in `componentAt` returns the leaf. A negative size makes `vector` throw `length_error`, and `MergeHistoryForest` with `n > 2^30` can overflow its node ids. Loops are bounded by component sizes, depths or the union count.

### Legacy mapping

No legacy source. Catalog names: Nyaan `union-find-enumerate` and suisen `linked_union_find` `enumerate`/`connected_component`, noshi91 `union_enumerate::enumerate` → `LinkedDSU::members`; Nyaan `unite(x, y, f)` merge callbacks → `unionEnumerate` for the newly connected pairs, or `members` before `uniteSets`; suisen `union_find_component_sum` `sum` → `ComponentSumDSU::componentValue`; maspypy `dual_unionfind` `apply`/`get` → `DualDSU`; Nyaan `dynamic-union-find` → `SparseDSU`; suisen `merge_history_forest` and maspypy `tree_of_unionfind` → `MergeHistoryForest`, whose `same(u, v, t)` is `earliestConnectTime(u, v) <= t` (connected: not `-1`) and `group(i, t)` is `leafRange(componentAt(i, t))`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/65-dsu_extensions_tester.cpp`, entry `65-dsu_extensions_tester.py` (driver `_00_runner.py`; three builds; 14 assertion probes). Runtime moduli are chosen by the seed from 7, 10^9+7 and 998244353. History groups run 300 rounds each in full mode (30 quick, 2000 stress; some groups a half or a tenth).

| Operation | Group | Oracle |
|---|---|---|
| `LinkedDSU` `uniteSets`, `makeSet`, `members`, `unionEnumerate`; inherited `count`, `groups` | `linkedHistories` (n <= 10 growing by `makeSet`, 40 steps, every vertex after every step) | BFS components of the union edges: `members` sorted equals the component, starts at `u`, and the second member's list is its rotation; `unionEnumerate` pairs sorted equal `A x B` |
| `ComponentSumDSU` `componentValue`, `addValue`, `uniteSets`, `makeSet`, inherited `getSize` | `sumHistories<SumMax>` (sum mod 10^9+7, maximum and its count), `<Or<bool>>` (`bool` storage), `<ModSum>` (runtime modulus) | BFS components folding each vertex's own value, into which `addValue` operands are folded |
| `DualDSU` `apply`, `get`, `uniteSets`, `findSet`, `isSameSet` | `dualHistories` over `Sum<lng>`, `Perm` (S5, non-commutative; also n <= 60 with 300 steps), `Xor<bool>`, `ModAdd`; half the rounds check every step | Explicit values: `apply` multiplies every member of the BFS component on the right |
| `SparseDSU` `uniteSets`, `findSet`, `isSameSet`, `getSize` | `sparseHistories` over `lng` (full-range and small keys), `string`, `pair<int, int>` | BFS components over key indices; untouched keys are their own roots and allocate nothing |
| `MergeHistoryForest` build, `nodeTime`, `mergeTime`, `earliestConnectTime`, `componentAt`, `leafRange` | `forestCases` (n <= 9 with q <= 16, n <= 30 with q <= 80; times 0, q/2, q, q + 1, `INT_MAX`) | BFS components of every prefix of the union sequence: node ids and times from the merging unions, member sets, first time of growth, first connected prefix, lifetime of `componentAt` |
| `MergeHistoryForest` on deep and shallow trees | `forestShapes` (n = 300 quick, 4096 otherwise): a chain in increasing and in decreasing vertex order and a star (one heavy path with a light leaf per level; the growing component is the first argument in the chains and the second in the star), and a balanced tree (`log2(n)` light edges per leaf), each with `n / 4` redundant unions inserted; 4000 queries each | As in `large` below |
| Scale | `large`: n = 2 * 10^5 and 2^16 with 4 * 10^5 and 3 * 10^5 operations (full); n = 10^6 with 3 * 10^6 (stress); `dualMedium` n = 3000 with 3 * 10^4 steps (full), n = 5000 with 10^5 (stress) | Row 01 `DSU` with per-root sums (16 checkpoints, 64 vertices each) and member lists (4 vertices: as many distinct members as the size, all in the component); row 10 `PartiallyPersistentDSU::uniteTime` for `earliestConnectTime`, and for `componentAt` 20,000 queries sorted by time against a row 01 `DSU` replaying the unions (size and a random leaf of the range); a key pool indexed by `std::unordered_map` with a `DSU` for `SparseDSU`; explicit member lists for `DualDSU` |
| Empty and singleton structures, self unions, runtime-modulus sums and actions, non-commutative action order, `string` and `pair` keys, queries that must not allocate | `edges` | Direct values |
| Preconditions | `invalid` | Out-of-range `members` and `uniteSets`, negative `addValue`, out-of-range `DualDSU::findSet` and `apply`, forest size negative and above 2^30, out-of-range union endpoint, `earliestConnectTime`, negative time, out-of-range node in `leafRange`, `nodeTime` and `mergeTime` |

Mutation check (2026-10-23, quick mode, seed 3): faults in the cycle splice, the cancelling tag and its compression order, the leaf block offsets, the heavy-path lengths, the binary search comparison and result, the light-edge stop of `componentAt`, the deeper-head and shallower-vertex choices of `earliestConnectTime`, the absorbing-root value, the untouched-key size and the `-1` of `mergeTime` were each caught (14 mutants).

## Commands and results

Latest run 2026-10-23, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `65-dsu_extensions_tester.py --mode full --seed 20261023` (GCC 16.2.1) | PASS, 3 configurations, 6,009,416 checks per configuration, 14 assertion probes, MEMORY peak 717 MB |
| `CXX=g++-14 65-dsu_extensions_tester.py --mode full --seed 20261023` (GCC 14.4.1, floor) | PASS, 6,009,416 checks per configuration, MEMORY peak 720 MB |
| `65-dsu_extensions_tester.py --mode stress --seed 7` | PASS, 35,318,293 checks per configuration, MEMORY peak 799 MB |
| `02-integration.py` | PASS, 174 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 714 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/65-dsu_extensions_tester.py' --mode full --seed 20261023
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/65-dsu_extensions_tester.py' --mode full --seed 20261023
python3 '96-Local Testing/02-Data Structures/65-dsu_extensions_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| Nyaan `union-find-enumerate.hpp`, `dynamic-union-find.hpp`; suisen `linked_union_find.hpp`, `union_find_component_sum.hpp`, `union_find_lazy.hpp`, `merge_history_forest.hpp`; noshi91 `union_enumerate.cpp`; maspypy `dual_unionfind.hpp`, `tree_of_unionfind.hpp`; hitonanode `grid_unionfind.hpp`; OI Wiki *并查集* | Cycle-swap member lists, component sums, hash-keyed vertices, component actions with point reads, the merge-history forest and its time queries; the lazy, deletable and grid variants left out. |

Fetched 2026-10-23 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-23-p035-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p035-omissions): component sums under component actions, monoid-only component actions, delete and move element, grid adapters, batch merge steps.
- Kept beside other rows by the user's decisions of 2026-10-23 (`duplicates.md`, Keeps): `SparseDSU` beside `HashMap` plus `DSU::makeSet`; `MergeHistoryForest` beside Graphs `06-mst.hpp` `KruskalReconstruction` and row 10 `PartiallyPersistentDSU::uniteTime`.

## History

- 2026-10-23: P035 first implementation (user decisions on `SparseDSU` and `MergeHistoryForest`); `DualDSU` added after the completeness sweep; independent review, 7 findings fixed across the package (here: the direct `CustomHash` include, complexity symbols, three probes, `MergeHistoryForest` rebuilt on heavy paths with O(n) memory instead of binary lifting, deep and shallow forest shapes); full (seed 20261023) on g++ and g++-14 and stress (seed 7) passed.
