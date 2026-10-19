# 35-persistentarray.hpp — evidence

Package P034 (batch DS31). New header; no legacy source. A fully persistent array by 2^B-ary path copying; row 10's `PersistentDSU` is built on it. Preconditions are `assert`s checked once per call.

## Contracts

### PersistentArray<T, B>

`T` is copyable and default-constructible: `T()` pads the last leaf chunk. `1 <= B <= 8`, `k = 2^B`, default `B = 3` (see Benchmarks); `0 <= n <= INT_MAX`.

- Layout: values live in leaf chunks of `k` (`val`), internal nodes hold `k` child ids (`ch`), and `h` is the least height with `k^(h + 1) >= n`. When `h = 0` the root is a leaf chunk. Index `p` is read in base `k` from the digit at shift `B * h` down to the leaf digit. Versions are dense ids from 0 (the build); `roots[v]` is version `v`'s root.
- Build from a vector: O(n) time; `max(ceil(n / k), 1) * k` values plus about `n / (k - 1)` child ids. Children past the last chunk point at a valid node and are never followed for `p < n`.
- Build from `(n, x)`: one internal node of `k` zero ids and one leaf chunk of `k` copies of `x`. Id 0 names both, so node 0 serves every level and every path ends in the shared chunk. Construction is O(k + log_k(n)) time and O(k) memory even for `n = INT_MAX`, which is how row 10 starts a DSU of size n without O(n) work.
- `get(v, p)`: `h` child reads. Returns `vector<T>::const_reference` into `val`, valid until the next `set` (pool growth reallocates).
- `set(v, p, x)`: branches from any version (full persistence). It copies the `h` internal nodes and the leaf chunk on `p`'s path, shared fill nodes included, so no other version changes, and returns the new id `versionCount() - 1`. Exactly `h * k` ids and `k` values per set. `x` is taken by value, so passing a reference from `get` is safe.
- `toVector(v)`: version `v` as a vector, by DFS in O(n).
- `versionCount()`: number of versions.
- Pools hold fewer than 2^31 entries (int ids). Copies are independent snapshots.

Arity tradeoff: `get` costs `log_k(n)` reads and `set` costs `k * log_k(n)` copied words. Memory per set is smallest at `k = 2` or `4`, and reads get cheaper as `k` grows. The default `B = 3` is a time and memory compromise. It is fastest on the Library Checker-shaped DSU workload and the mixed array workload at 2 * 10^5, and within 30% of the fastest arity everywhere measured. `B = 4` is up to 23% faster on the 10^6-scale workloads but stores 1.4–1.7 times as much (see Benchmarks).

### Violated preconditions

Under `NDEBUG` a bad version id reads `roots` out of range (undefined). An out-of-range `p` reaches some stored chunk: digits are masked, padding children are valid, and both builds hold at least one chunk. A negative fill size builds an empty array. No loop runs past `h` levels.

### Legacy mapping

No legacy code. Catalog forms of the same operations: Nyaan `update(root, k, v)`, ei1333 `mutable_get`, hitonanode `ch`/`set`, tko919 `set` and maspypy `set` are `set`; suisen `dump` is `toVector`; ei1333 and hitonanode `build(vector)` and maspypy `new_node(vector)` are the vector build.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/35-persistentarray_tester.cpp`, entry `35-persistentarray_tester.py` (driver `_00_runner.py`; three builds; 7 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| Both builds, `get`, `set`, `toVector`, `versionCount` | `branching<T, B>` for every `n <= 140` (300 in stress, 40 in quick): B = 1, 2, 3 (`int`), 4 (`lng`), 3 (`string`), 2 (`bool`), vector and fill builds; sets branch from the latest, the root or a random version | Full copy of the array per version; every version's `toVector` and every `get` compared for `n <= 64` |
| Larger arities and sizes | `branching` for n in 255–257, 4095–4097, 32768, 32769 with B = 3, 4 (fill), 8 | Same copies, checked on every 37th and the last version |
| Memory accounting | `branching` | `h` is minimal; the fill build holds exactly `k` ids and `k` values; each set adds exactly `h * k` ids and `k` values |
| Long histories | `large<B>`: n = 10^5 with 2 * 10^5 sets (B = 3), 65536 (B = 1), 100003 (B = 4) in full; 10^6 sets on n = 10^6 (B = 3) and 5 * 10^5 on 2^20 (B = 2) in stress | Offline DFS over the version tree with one mutable array answers sampled `get`s |
| Aliasing, copies, empty arrays, `n = INT_MAX` fill with B = 1, B = 8 | `edges` | Direct values |
| Preconditions | `invalid` | Bad versions in `get`, `set`, `toVector`; out-of-range and negative positions; negative fill size |

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/35-persistentarray_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/35-persistentarray_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/35-persistentarray_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-Data Structures/35-persistentarray_benchmark.py' --runs 5
```

Final run 2026-10-17: full mode seed 20261017 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 2,863,577 checks per configuration and 7 assertion probes, MEMORY peak 588 MB and 589 MB; stress mode seed 7 (one round) passed all three configurations with 5,413,519 checks per configuration, MEMORY peak 959 MB. Mutation check (temporary header copies, quick mode seed 7, `-O2 -DNDEBUG`): 5 of 5 injected faults detected (leaf chunk not copied by `set`, wrong digit shift in `get`, fill value ignored, non-minimal height, `toVector` overrun). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds; MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (153 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 1820 MB) and `03-consistency.py` reported no errors.

## Benchmarks

Driver `96-Local Testing/02-Data Structures/35-persistentarray_benchmark.py` (C++ `35-persistentarray_benchmark.cpp`). It rewrites the header's default `B` in a temporary copy, so `PersistentDSU` runs on each arity. Conditions: Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 `-std=gnu++20 -O2 -DNDEBUG`, seed 20261017, 5 runs, median seconds. Pool = `ch` + `val` + `roots` bytes, excluding vector slack. Checksums agree across arities. Run 2026-10-17 with no other suite running.

Workloads:
- `array-mixed`: half `set` and half `get` on random versions.
- `array-chain`: q sets on the latest version, then q gets on random versions.
- `dsu-checker`: the Library Checker shape; query i acts on a random earlier version, half `uniteSets`, half `isSameSet`.
- `dsu-chain`: q unions on the latest version, then q `isSameSet` + `getSize` on random versions.

Each cell is median seconds / pool MB.

| Workload | n | q | B = 1 | B = 2 | B = 3 (default) | B = 4 | B = 5 |
|---|---|---|---|---|---|---|---|
| array-mixed | 2 * 10^5 | 10^6 | 0.391 / 72.1 | 0.245 / 71.6 | **0.218** / 94.3 | 0.263 / 155.2 | 0.286 / 246.7 |
| array-mixed | 10^6 | 10^6 | 0.550 / 85.7 | 0.307 / 83.2 | 0.279 / 112.9 | **0.254** / 158.4 | 0.286 / 249.7 |
| array-chain | 10^6 | 10^6 | 1.386 / 156.4 | 0.792 / 156.4 | 0.578 / 217.4 | **0.446** / 309.0 | 0.495 / 492.1 |
| dsu-checker | 2 * 10^5 | 2 * 10^5 | 0.123 / 28.3 | 0.066 / 28.3 | **0.051** / 37.5 | 0.074 / 62.0 | 0.088 / 98.7 |
| dsu-checker | 10^6 | 10^6 | 0.950 / 156.5 | 0.511 / 156.5 | 0.529 / 217.6 | **0.478** / 309.2 | 0.551 / 492.5 |
| dsu-chain | 2 * 10^5 | 2 * 10^5 | 0.745 / 47.3 | 0.435 / 47.3 | 0.316 / 62.6 | 0.277 / 103.5 | **0.274** / 164.8 |

`B = 3` is within 30% of the fastest arity on every row and stores 1.3–1.4 times the minimum; `B = 4` wins the 10^6-scale rows by up to 23% at 1.4–1.7 times `B = 3`'s memory. A caller can pass `B` explicitly.

## Sources

| Source | Actual reading and use |
|---|---|
| Nyaan `persistent-array.hpp`, ei1333 `persistent-array.hpp`, suisen `persistent_array.hpp`, hitonanode `persistent_array.hpp`, tko919 `persistentarray.hpp`, maspypy `dynamic_array.hpp` | 2^B-ary path copying and its arity defaults (3 or 4), fill construction with shared nodes, `dump`, in-place set (left out). |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Decided 2026-10-17 (`duplicates.md`, Keeps): k-ary path copying, kept beside row 18's binary `PersistentSegmentTree` get/set as a different time and memory tradeoff.
- Left out with reasons in [00-notes.md](00-notes.md#p034-omissions): in-place set, `size`, node pools and garbage collection, the persistent block array.

## History

- 2026-10-17: P034 first implementation and verification; the fill build shrank to one shared node after a mutation check; independent review: an empty vector build now holds one chunk, the default-arity wording matches the benchmark.
