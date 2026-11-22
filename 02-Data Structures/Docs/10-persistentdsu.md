# 10-persistentdsu.hpp — Evidence

Package P034 (batch DS03), a new header with no legacy source: a partially persistent DSU (queries at any past time) and a fully persistent DSU (unions on any version) built on row 35. Preconditions are `assert`s checked once per call.

## Contracts

### PartiallyPersistentDSU

Vertices are `[0, n)` with `0 <= n <= INT_MAX`. Time `t` is the state after the first `t` `uniteSets` calls; the public `now` is the number of calls. Memory is O(n + q), and copies are independent snapshots.

- `uniteSets(u, v)` acts on the latest state: union by size, the first argument's root kept on ties. It returns whether a merge happened and advances `now` either way. An attached root `v` stores `par[v]` and its attach time `tim[v] = now`; roots keep `tim = INT_MAX`. Each merge appends `(now, new size)` to the surviving root's size history.
- There is no path compression, so depths stay at most `floor(log2(n))`. Attach times strictly increase along every path to a root, because a vertex's new parent was a root when it attached.
- `findSet(t, u)` climbs while the attach time is at most `t`, in O(log(n)), for `0 <= t <= now`; the root found is the representative at time `t`. `isSameSet(t, u, v)` compares representatives.
- `size(t, u)` binary-searches the representative's size history (size 1 before its first merge), in O(log(n)).
- `count(t)` is the number of components at time `t`, in O(1) from per-call counts.
- `uniteTime(u, v)` is the least `t` with `isSameSet(t, u, v)`, or -1 if they are not connected at `now`; 0 when `u == v`. The walk always moves the endpoint with the smaller attach time. Both paths have increasing times, so the moved times never decrease and the walk meets at the lowest common ancestor. The answer is the last moved time, the largest on either path below it: connected then, and one step earlier that edge is absent. O(log(n)).

### PersistentDSU

Vertices are `[0, n)` with `0 <= n <= INT_MAX`. A `PersistentArray<int>` (row 35, default `k = 8`) holds `-size` at roots and the parent elsewhere, and `ver[t]` maps DSU version `t` to an array version. Version 0 has no edges, and every `uniteSets` call creates one version. Copies are independent snapshots.

- `uniteSets(t, u, v)` runs union by size on version `t` (any version) and returns the new version id. A merge makes two array sets (size, then parent); a redundant call reuses the array version, so it costs no memory beyond the id.
- `findSet(t, u)`, `isSameSet(t, u, v)` and `size(t, u)` take O(log(n)) parent steps of one O(log_k(n)) `get` each; `uniteSets` adds two O(k log_k(n)) sets; `count(t)` is O(1). `findSet` asserts the version and the vertex. Its climb stops after 31 steps; union by size keeps depths at or below 30, so the cap only matters for a violated precondition.
- Construction uses row 35's shared fill build, O(k + log_k(n)), so `n = 10^9` is fine (tested). Each merge stores `2 * (h * k + k)` ints, where `h` is the array height.

### Violated Preconditions

Under `NDEBUG`:

- `PartiallyPersistentDSU::findSet` stops at a root for any `t`; out-of-range vertices or times read outside the arrays (undefined).
- `PersistentDSU`: an out-of-range vertex shares a cell of row 35 with an in-range one, and a union through it can write a parent cycle. The 31-step cap keeps every later `findSet` bounded, though its answer may be meaningless. A bad version id reads `ver` out of range (undefined).

### Legacy Mapping

There is no legacy code. Catalog forms: ei1333 `find(t, x)`, `size(t, x)`, `unite(t, ...)` (explicit nondecreasing times) are `findSet`, `size` and `uniteSets` with the implicit call counter; noshi91 `now()` is the public `now`; hitonanode's fully persistent `count(t, x)` is `size`; suisen `merge` returning a new structure and maspypy `merge(np, x, y)` returning a root are `uniteSets` returning a version id.

- P276 rename, 2026-11-10: `getSize` → `size`.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/10-persistentdsu_tester.cpp`, entry `10-persistentdsu_tester.py` (driver `_00_contest_runner.py`, three builds, 10 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `PartiallyPersistentDSU`: `uniteSets` (return value), `findSet`, `isSameSet`, `size`, `count` at every time | `partialSmall` (n <= 10 with q <= 25, n <= 40 with q <= 120; self-loops) | BFS labels of each edge prefix; `findSet` must be a member and the same vertex for the whole component |
| `uniteTime` | `partialSmall`, every pair | Least prefix with equal labels, by scan |
| Large partially persistent | `partialLarge`: n = 10^5, q = 2 * 10^5, 20000 sampled queries (full); n = 1.5 * 10^5, q = 3 * 10^5, 30000 samples (stress) | An offline time sweep with row 01's path-compressed `DSU`. Each `uniteTime` answer is confirmed connected at its time and disconnected one step earlier (or at `now` when -1). Depth at most `floor(log2(n))`. |
| `PersistentDSU`: `uniteSets` (version ids), `findSet`, `isSameSet`, `size`, `count` | `persistentSmall`: random version trees, every version checked | BFS labels of each version's ancestor edges |
| Large persistent | `persistentLarge`: n = 10^5 and 2 * 10^5 with 2 * 10^5 unions on random or latest versions (full); n = 1.2 * 10^5 with 2.4 * 10^5 (stress) | Sampled versions rebuilt from their ancestor edges with row 01's `DSU` |
| Empty structures, self-loops, branches, redundant versions, `n = 10^9` | `edges` | Direct values |
| Preconditions | `invalid` | Future and negative times, out-of-range vertices, bad `count` time, out-of-range `uniteTime`, negative size; bad versions in `findSet`, `uniteSets`, `count`; out-of-range persistent vertex |

A hand mutation check (2026-10-17; temporary header copies, quick mode seed 7, `-O2 -DNDEBUG`) detected 5 of 5 injected faults: `uniteTime` off by one, the size history read one step late, the persistent count not decremented, the time counter advancing by two, and a climb cap of 3. The arity sweep in [35-persistentarray.md](35-persistentarray.md#benchmarks) includes the `PersistentDSU` workloads.

## Commands and Results

Machine: Linux x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.8; the runner's optimized, checked and ASan/UBSan builds. MEMORY peaks are dominated by compilation.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 755 MB | 0.3 + 0.9 | 14.1 + 23.3 | 15.1 + 8.8 | 891 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 800 MB | 0.2 + 0.8 | 22.6 + 35.0 | 23.0 + 10.9 | 963 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 22.6 + 0.0 | 19.7 + 0.2 | | 750 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 756 MB | 0.1 + 0.7 | 5.9 + 34.7 | 6.2 + 13.6 | 935 MB |

Package runs:

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P276) | full on g++ and g++-14, stress, and `python3 '96-Local Testing/05-extraction.py'` on this header | passed (counts not recorded) |
| 2026-10-17 (P034) | `python3 '96-Local Testing/02-Data Structures/10-persistentdsu_tester.py' --mode full --seed 20261017` (GCC 16.2.1) | PASS, 628,346 checks per configuration, 10 assertion probes, MEMORY peak 466 MB |
| 2026-10-17 (P034) | `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/10-persistentdsu_tester.py' --mode full --seed 20261017` (GCC 14.4.1) | PASS, 628,346 checks per configuration, 10 assertion probes, MEMORY peak 466 MB |
| 2026-10-17 (P034) | `python3 '96-Local Testing/02-Data Structures/10-persistentdsu_tester.py' --mode stress --seed 7` (one round) | PASS, all three configurations, 4,205,789 checks per configuration, MEMORY peak 927 MB |
| 2026-10-17 (P034) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 153 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 1820 MB |
| 2026-10-17 (P034) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `persistent_unionfind` statement (GitHub `task.md`) | Query shape: unite on or ask about version `k < i`. |
| ei1333 `partially-persistent-union-find.hpp`, `persistent-union-find.hpp`; noshi91 `partially_persistent_union_find.cpp`; hitonanode `partially_persistent_uf.hpp`, `fully_persistent_uf.hpp`; Nyaan `persistent-union-find.hpp`; suisen `persistent_union_find.hpp`; tko919 `persistentunionfind.hpp`; maspypy persistent_unionfind test | Per-root size histories, implicit versus explicit time, version handles, persistent-array arity (ei1333 uses k = 8). |
| OI Wiki persistent segment tree | Persistent DSU by union by rank on a persistent segment tree, O(log^2(n)). |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Row 10 is partial: `PersistentDSU::groups` and `PersistentDSUOffline` are listed under `missing:` in the inventory row.
- `uniteTime` is kept beside row 65 `MergeHistoryForest::earliestConnectTime` as a different algorithm (`duplicates.md`, Kept by the Sweep).
- `dump` stays out, with the reason in [00-notes.md](00-notes.md#p034-omissions).
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Renamed aliases (P276, 2026-11-10): `getSize`.

Search aliases (P263, grab --find): 可持久化并查集, 部分可持久化并查集, 持久化并查集.

## History

- 2026-10-17: P034 first implementation and verification; `PartiallyPersistentDSU::findSet` bounded at roots for any time under `NDEBUG`; the independent review's findings fixed: `PersistentDSU::findSet` asserts the vertex and caps its climb (a union through an out-of-range vertex could loop under `NDEBUG`), `getSize` evaluates `findSet` first, direct template include, depth-10 tree test; full (seed 20261017) on g++ and g++-14, stress (seed 7), integration and consistency passed.
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-10: P276 rename (`getSize` → `size`); full on g++ and g++-14, stress and extraction passed.
