# 61-hashmap.hpp — evidence

Package P227 (batch DS49). New header; no legacy excerpt. Depends on `06-Miscellaneous/02-customhash.hpp` for the default hasher. Preconditions are `assert`s checked once per operation.

## Contracts

### HashTable<K, T, H, FIXED>

The shared open-addressing engine behind `HashMap` and `HashSet`; `T` is `K` for a set and `pair<K, V>` for a map. Slots hold the element and a `uint` stamp next to each other, so one cache line serves the probe and the key comparison.

- Domain: `K` and `T` default-constructible and move-assignable, `K` equality-comparable; `H` returns a `size_t` whose low bits are well mixed (the slot is `hash(k) & (cap - 1)`). The default `CustomHash` (SplitMix64 finalizer, process seed) qualifies; an identity hash on structured keys does not. At most 2^29 elements.
- Capacity `cap` is a power of two (or 0 before the first insertion; a default-constructed table allocates nothing), and `size() <= cap / 2` always. Inserting into a full table doubles `cap` and reinserts every element. `reserve(m)` with `m` in `[0, 2^29]` raises `cap` to at least `max(4, bit_ceil(2 * m))` (no-op for `m = 0`), so the next `m - size()` insertions never rehash. Capacity never shrinks; `M` is O(cap) with `cap <= 4 * max(peak size, reserve)`.
- `size`, `empty`, `contains(k)`; `find(k)` returns an iterator to the element or `end()`.
- `erase(k)` returns whether `k` was present. It uses backward-shift deletion: walking the cluster after the hole, an element at `j` with home `h` moves into the hole `i` when `i` lies cyclically in `[h, j)`, i.e. `(j - h) & mask >= (j - i) & mask`. This restores the invariant that every element is reachable from its home slot without crossing an empty slot, so there are no tombstones and lookups never slow down after erasures.
- `clear()` is O(1) amortized: it increments the generation `gen`, so every slot whose stamp differs from `gen` is empty. When `gen` wraps to 0 every stamp is reset in O(cap) and `gen` restarts at 1. Cleared elements keep their storage until their slot is reused; insertion always overwrites the whole element, so `operator[]` after `clear` or `erase` sees `V()`, never a stale value.
- Iteration (`begin`/`end`, forward iterators with standard traits; a map iterator converts to `const_iterator`) visits each element once in slot order, which depends on the seed; it costs O(cap). Map iterators give `pair<K, V> &` (the key must not be modified); set iterators are always const.
- Aliasing: `operator[]`, `insert` and `HashSet::insert` build the new element before any rehash, so the key and value arguments may refer into the table (`p[p[x]]`, `m.insert(k, m[j])`). An expression whose other operand holds a reference across an insertion is still the caller's hazard: in `q[a] = q[b]` with `V = string`, the right side is evaluated first and `q[a]` may rehash before the assignment reads it; copy first.
- Invalidation: any insertion may rehash (all iterators and references invalid) unless the table is `FIXED` or the insertion stays within a prior `reserve`; `erase` may move one element of the same cluster per step, so it invalidates iterators and references to other elements; `clear` invalidates everything.
- Costs: O(1) expected per operation and amortized over rehashes, under the assumption that the hash behaves randomly for the key set; a seed known to an adversary permits collisions, which cost time (O(n) per operation worst case), never correctness. Load at most 1/2 gives expected probe counts about 1.5 (hit) and 2.5 (miss) for linear probing.
- Copies are independent snapshots, including the hasher and its seed.

### HashMap<K, V, H = CustomHash, FIXED = false>

- `operator[](k)` returns a reference to the value, inserting `V()` when absent.
- `insert(k, v)` inserts only when absent and returns whether it did (like `std::map::insert`, no overwrite).
- `at(k)` returns a reference (const overload too) and asserts presence.
- `get(k, def = V())` returns the value or `def` when absent, without inserting.

### HashSet<K, H = CustomHash, FIXED = false>

- `insert(k)` returns whether `k` was new.

### HashMapFixed<K, V, H>, HashSetFixed<K, H>

Aliases with `FIXED = true`: construct with the maximum size `m`; capacity `max(4, bit_ceil(2 * m))` is allocated once and never changes, so references and iterators survive insertions. An insertion that would exceed `cap / 2` (at least `m`) is a precondition violation (assert); with `NDEBUG` the table grows instead, so the violation stays bounded. Combined with the O(1) `clear` this is the multi-test pattern: one allocation, many cheap resets.

## Correctness and cost

`locate(k)` walks from `home(k)` until it finds `k` or an empty slot; termination needs one empty slot, which load <= 1/2 guarantees (also under `NDEBUG` because a violated `FIXED` bound grows the table). `place(k)` locates first and grows only when it must claim a new slot, so a lookup of a present key in a full table never rehashes. `rehash` reinserts by probing only for empty slots (keys are distinct). The backward-shift condition is the standard one for linear probing (Knuth vol. 3, Algorithm 6.4R): an element may move into the hole exactly when the hole lies on its probe path from home, which is the cyclic interval `[home, j)`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/61-hashmap_tester.cpp`, entry `61-hashmap_tester.py` (driver `_00_runner.py`; three builds; 8 assertion probes). Oracles are `std::map`/`std::set`; a structural checker verifies after every 97th step and at the end that every live slot is reachable from its home without an empty slot, the live count equals `size()`, capacity is a power of two and load is at most 1/2.

| Operation | Group | Oracle |
|---|---|---|
| HashMap `operator[]` (accumulating `+=`), `insert` (return value, no overwrite), `erase` (return value), `contains`, `find` (`== end()`, `*it`, `it->second` write-through), `at` (read and write), `get` with and without default, `size`, `empty`, `clear`, `reserve` (capacity bound), mutable and const iteration | `mapRandom` with seeded `CustomHash` on key ranges 3, 40, 2000 and 1e18, and with `ConstHash` (one cluster covering the table), `Mod3Hash` (three clusters, wraparound) and `IdHash` | `std::map`, full contents compared through iteration |
| HashSet `insert`, `erase`, `contains`, `find`, `size`, `empty`, `clear`, `reserve`, const iterators (`static_assert`) | `setRandom` with the same hashers | `std::set` |
| Backward-shift erase in every cluster shape | `exhaustiveSmall` | Identity hash on a 16-slot table, all subsets of keys with homes 14, 15, 0, 1, … and their `+16` collisions (clusters wrapping past the end), every deleted position: membership of all other keys and the structural invariant |
| Aliased arguments across rehashes (`al[al[i]]`, `insert(-i, al[i - 1])` against `std::map`; string keys and values, `sk[sk.at(k)]`, set insert from `*begin()`; caught by ASan before the fix), iterator to `const_iterator` conversion, value reset after `erase` and `clear` (`HashMap<int, vector<int>>`), generation wraparound (stamp set to `UINT_MAX - 1`, four clears), copy independence, empty table without allocation, `reserve` without rehash and stable references, `HashMapFixed`/`HashSetFixed` filled to exactly `cap / 2` without rehash and cleared, string and pair keys, extreme `ulng` keys | `semantics` | Hand-computed contents, `std::map`/`std::set` for generic keys |
| Structured keys `i << 20`, consecutive, `i * 1000000007` under the seeded hash; erase half | `adversarial` | Mean extra probes < 2 per key (2^18 keys in full), lookups after erasing every second key |
| Preconditions | `invalid` | `at` missing (both overloads), fixed overflow via `operator[]`, `insert` and `HashSetFixed::insert`, zero-capacity fixed table, `reserve(-1)`, `reserve(2^30)` |

Mutation check (2026-10-09, quick mode, temporary copies of the pre-review header): 11 of 11 injected faults detected: strict instead of non-strict shift condition, no shift, stale value on `operator[]`, missing stamp reset on generation wrap, load 3/4, rehash resurrecting cleared slots, iteration over cleared slots, lookup ignoring the generation, set insert without writing the key, `get` ignoring its default, `insert` overwriting.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/61-hashmap_tester.py' --mode full --seed 20261009
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/61-hashmap_tester.py' --mode full --seed 20261009
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/6' --no-integration
python3 '96-Local Testing/02-Data Structures/61-hashmap_benchmark.py' --runs 5
python3 '96-Local Testing/02-integration.py'
CXX=g++-14 python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-09: full mode seed 20261009 passed on both compilers, 3,695,536 checks per configuration and 8 assertion probes (MEMORY peak 646 MB on g++, 665 MB on g++-14, both dominated by the compiler); stress mode seed 7 passed all three configurations with 16,915,676 checks per configuration (peak at most 835 MB). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14, `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

`02-integration.py` passed on GCC 16.2.1 and 14.4.1: 114 standalone/aggregate headers (this one alone, in `99-all.hpp` and `98-basic.hpp`), the scalar and AVX2 two-translation-unit builds and the workspace. Its `--sanitizers` stage stopped at a link error in another session's uncommitted `01-Core/16-poly_tester.cpp`, unrelated to this header, whose own ASan/UBSan configuration passed above. `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header, its tester and its benchmark.

## Benchmark

`61-hashmap_benchmark.py --runs 5`, seed 20261009, GCC 16.2.1 `-O2 -DNDEBUG`, i9-11900H. Per row: a pool of n keys, then q = 4e6 operations (40% `operator[] +=`, 40% `find`, 20% `erase`) on keys drawn uniformly from the pool; construction and destruction timed; all three containers use `CustomHash` with the default process seed; checksums agree across variants and runs. Medians of 5 runs in seconds (MEMORY peak 313 MB).

| Keys | n | HashMap | gp_hash_table | unordered_map |
|---|---|---|---|---|
| random 63-bit | 1e3 | 0.088 | 0.101 | 0.115 |
| random 63-bit | 1e5 | 0.121 | 0.125 | 0.193 |
| random 63-bit | 2e6 | 0.325 | 0.321 | 0.685 |
| `i << 20` | 1e3 | 0.087 | 0.101 | 0.115 |
| `i << 20` | 1e5 | 0.118 | 0.124 | 0.196 |
| `i << 20` | 2e6 | 0.322 | 0.314 | 0.596 |

Measured after the review fixes. HashMap is 1.3–2.1 times faster than `unordered_map`, 3–14% faster than `gp_hash_table` up to 1e5 keys, and 1–3% slower at 2e6 keys, where both are bound by cache misses (24-byte slots here).

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `ds/hashmap.hpp` | `get` with default; load 1/2. |
| Nyaan `hashmap-base.hpp`, `hashset.hpp`, `hashmap-unerasable.hpp` | Tombstone erase with shrinking (rejected in favour of backward shift), fixed-size option. |
| yosupo `container/hashmap.hpp`, `hashset.hpp`, `hash.hpp` | HashSet `find`; seeded hashing of composite keys (CustomHash already covers it). |
| hitonanode `fast_hash_map.hpp` | Version-stamp O(1) `clear`. |
| KACTL `HashMap.h`, tko919 `hashmap.hpp`, OI Wiki hash | Baselines; none uses backward-shift deletion. |
| Knuth, TAOCP vol. 3, §6.4 Algorithm R | Deletion with linear probing (from memory of the algorithm, not re-read for this package). |

References inspected 2026-10-09 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Omissions with reasons in [00-notes.md](00-notes.md#p227-omissions): `count`, `forEach`, erase by iterator, `emplace`, shrinking.
- `06-Miscellaneous` row `19-hash_families.hpp` still lists HashMap/HashSet; this row owns them.
- Iteration is O(cap), so iterating a large table that was cleared or mostly erased pays for its peak capacity.

## History

- 2026-10-09: P227 first verification. Independent review: fixed a use-after-free when a key or value argument aliases the table across a rehash (`place` now builds the element first), corrected the setup bound to O(m) with reserve, `reserve` doubles in `uint`, added the iterator-to-`const_iterator` conversion and renamed the overloaded `m`. The floor check exposed that `06-Miscellaneous/02-customhash.hpp` never compiled on GCC 14; fixed in this package with the user's approval (see its evidence).
