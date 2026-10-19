# 26-trie.hpp — evidence

Package P033 (batch DS13). New header; no legacy source. Binary integer tries over `B`-bit keys, most significant bit first. Preconditions are `assert`s checked once per operation; each operation is O(B) with B the key width.

## Contracts

### Keys and signed-key encoding

`T` is an integral type and `1 <= B <= ` its bit width (default: all bits). Keys are `[0, 2^B)` for unsigned `T` and `[-2^(B-1), 2^(B-1))` for signed `T`; every key, xor mask and query bound must lie in that domain (asserted). A key `a` is stored as `enc(a) = (a xor SIGN) & MASK`, where `MASK` keeps the low `B` bits and `SIGN` is bit `B - 1` for signed `T` and 0 otherwise; `dec(s) = T(s - SIGN)` (modular, then sign-extended by the conversion). `enc` is an order isomorphism from the signed domain onto `[0, 2^B)`, and `enc(a xor x) = enc(a) xor (x & MASK)` because both sides flip bit `B - 1` once, so xor masks act on encoded keys directly and every order query (`kth`, `countLess`, `lowerBound`, `xorMin`, `xorMax`) compares in the signed order of `T`. For signed keys the xor of two in-domain keys stays in the domain: bits above `B - 1` all equal bit `B - 1`.

### binary_trie_detail::Nodes<T, B>

Node pool shared by both tries: `ch[v]` (children, 0 when absent), `cnt[v]` (keys in the subtree). Node 0 is the empty node: count 0 and both children 0, so a walk that leaves the trie stays at node 0 and reads count 0. The kernels walk two roots `u`, `v` in lockstep and use `cnt[v'] - cnt[u']`, which is the count of the multiset difference `v - u` when `u`'s multiset is contained in `v`'s: `count(u, v, s)`, `kth(u, v, k, x)` (the `k`-th smallest of `s xor x`, 0-based, preferring the child whose bit equals `x`'s), `countLess(u, v, t, x)` (the number of `s` with `(s xor x) < t`). `xorMin` is `kth(0)` and `xorMax` is `kth(size - 1)`. Not public API.

### BinaryTrie<T, B>

A multiset of at most `INT_MAX` keys with a global xor `lz`: the current keys are the stored codes xor `lz`.

- `insert(a, c = 1)`: adds `c >= 0` copies (`size() + c <= INT_MAX` asserted); creates at most `B` nodes.
- `erase(a, c = 1)`: removes `min(c, count(a))` copies, `c >= 0`, and returns that number; erasing an absent key is a valid no-op that returns 0 (Library Checker `set_xor_min` erases keys that may be absent). Nodes are never freed: an emptied path keeps count 0 and is reused by a later insert.
- `count(a)`, `size()` (with multiplicity).
- `applyXor(x)`: replaces every current key `a` by `a xor x` in O(1); later inserts are not affected.
- `xorMin(x)`, `xorMax(x)`: the minimum and maximum of `a xor x` over the current keys; assert a nonempty trie.
- `kth(k)`: the `k`-th smallest current key, `0 <= k < size()`.
- `countLess(y)`: the number of current keys `< y`.
- `lowerBound(y, res)`: the smallest current key `>= y`; returns `false` and leaves `res` unchanged when none exists (no in-domain sentinel exists for full-width `T`). It is `kth(countLess(y))`, two O(B) walks.
- A per-query xor on `kth`, `countLess` or `lowerBound` is `applyXor(x)`, the query, `applyXor(x)` (three O(1) or O(B) steps); the library keeps one form.
- Memory: `n` insert calls create at most `n * B + 2` nodes of 12 bytes. A copy is an independent snapshot.

### PersistentBinaryTrie<T, B>

Versions are node ids; version 0 is the empty multiset, and every update returns a new version without changing old ones (path copying: `B + 1` new nodes per update).

- `insert(v, a, c = 1)`: the version `v` plus `c >= 0` copies of `a`; `v` must be an existing version.
- `erase(v, a, c = 1)`: the version `v` minus `min(c, count)` copies of `a`; returns `v` itself when nothing is removed.
- Queries take a pair `(u, v)` and see the multiset difference `v - u`; the caller guarantees that `u` is contained in `v` (for example prefix versions `root[l]`, `root[r]` of an array built by inserts, which answer queries over `a[l, r)`; `u = 0` for a single version). `size(u, v)` and the `count`/`countLess` entry assert only `size(u, v) >= 0`; a non-included pair gives meaningless results.
- `size(u, v)`, `count(u, v, a)`, `xorMin(u, v, x)`, `xorMax(u, v, x)` (assert a nonempty difference), `kth(u, v, k, x = 0)` (the `k`-th smallest of `a xor x`), `countLess(u, v, y, x = 0)` (the number of `a` with `(a xor x) < y`).
- Memory: `(B + 1)` nodes per update. Erase on a persistent version keeps the old nodes; there is no garbage collection.

### Violated preconditions

Under `NDEBUG` an out-of-domain key is truncated to its low `B` bits, an empty `xorMin`/`xorMax`/`kth` returns a meaningless key, and an out-of-range version id reads outside the pool (undefined). Every walk is bounded by `B` steps, so no call loops.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/26-trie_tester.cpp`, entry `26-trie_tester.py` (driver `_00_runner.py`; three builds; 28 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| BinaryTrie `insert`, `erase` (counts, absent keys), `count`, `size`, `applyXor`, `xorMin`, `xorMax`, `kth`, `countLess`, `lowerBound` | `trieRandom` for `uint` (B = 4, 32), `int` (B = 5, 32), `lng` (B = 40, 64), `ulng` (B = 64), `int16_t` (B = 9) | `std::multiset` of up to 200 current keys, xored in place by `applyXor`; keys drawn from a pool with the domain ends, 0, -1 and halves plus uniform keys; every step checks all operations (`xorMin`/`xorMax` against three of the seven arguments) |
| Same, bounded exhaustive | `trieExhaustive` (`uint` and `int`, B = 3) | Every multiset with multiplicities 0..2 over the 8 keys (6561), under each of the 8 global xors, against every key as the query argument |
| Signed-key encoding | `int`, `lng`, `int16_t` configurations, exhaustive `int` | Signed order of `T`; `INT_MIN`, `LLONG_MIN` and the domain ends in `edges` |
| PersistentBinaryTrie `insert`, `size`, `count`, `xorMin`, `xorMax`, `kth`, `countLess` on version differences | `persistentRange` (`int` B = 6, `uint` B = 4, `lng`, `ulng`) | Prefix versions of a random array; each query over `a[l, r)` against the sorted xored subarray |
| PersistentBinaryTrie `insert`, `erase` on branching versions, old versions unchanged | `persistentBranching` (`int` B = 5, `ulng`, `lng`) | A `std::multiset` per version; queries on `(0, v)` and on random pairs whose multisets are included (`std::includes`), against the multiset difference |
| Empty structure, `lowerBound` absence, zero-copy insert, erase of absent and of more copies than present, copies, full-width extremes | `edges` | Direct values |
| Preconditions | `invalid` | BinaryTrie: out-of-domain keys, xor masks and bounds (unsigned and both signed ends) in `insert`, `count`, `countLess`, `applyXor`, `xorMin`, `xorMax`; empty `xorMin`/`xorMax`; `kth` at `size()` and at -1; negative counts in `insert` and `erase`; size overflow past `INT_MAX`. PersistentBinaryTrie: unknown version, out-of-domain key, mask or bound in `insert`, `count`, `kth`, `countLess`, `xorMax`; negative counts in `insert` and `erase`; count overflow; `kth` out of range; empty `xorMin`/`xorMax` differences; a non-included version pair |

Mutation check (2026-10-14, quick mode, temporary copies): 8 of 8 injected faults detected (strict comparison in `kth`, `countLess` not turning after counting, no sign encoding, `applyXor` assigning instead of xoring, `erase` ignoring the count, a persistent child pointing at its own node, `xorMax` computed as `xorMin`, persistent `erase` ignoring the count).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/26-trie_tester.py' --mode full --seed 20261014
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/26-trie_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/26' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-14 (after the review fixes): full mode seed 20261014 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 11,852,623 checks per configuration and 28 assertion probes, MEMORY peak 664 MB and 679 MB; stress mode seed 7 (one round) passed all three configurations with 15,456,616 checks per configuration, MEMORY peak 776 MB. The slowest binary (checked build) takes about 46 s in full and 76 s in stress, against the runner's 180 s timeout. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14, in the runner's optimized, checked and ASan/UBSan builds; MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (143 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 2758 MB) and `03-consistency.py` reported no errors.

## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `set_xor_min` task statement | Insert-if-absent, erase-if-present and min xor queries: erase of an absent key is a no-op. |
| maspypy `ds/binary_trie.hpp`, ei1333 `structure/trie/binary-trie.hpp` and `persistent-binary-trie.hpp`, Nyaan `data-structure/binary-trie.hpp`, suisen `datastructure/binary_trie.hpp`, hitonanode `data_structure/binary_trie.hpp` | Operation names (`xor_min`, `kth`, `count_less`, `erase(val, num)` returning the number removed), the global xor, per-version operations of the persistent trie. |
| OI Wiki 0-1 trie (`string/trie`) and persistent trie (`ds/persistent-trie`) | Range max xor through two prefix versions (count subtraction). |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): trie meld, the LSB-first global +1 trie, kth xor pair and pair counting, range xored mex, index payloads, ordered enumeration.
- Literal-prefix and radix tries are Strings `04-trie.hpp` and `25-radixtrie.hpp`; xor bases are row 62; row 17 (`WaveletMatrix`) defers range xor queries to `PersistentBinaryTrie`.

## History

- 2026-10-14: P033 first implementation and verification; the independent review found no header defects and 11 missing assertion probes, now added.
