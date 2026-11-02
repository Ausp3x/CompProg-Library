# 31-radixheap.hpp — evidence

Package P032 (batch DS27). New header. Both structures keep `digits + 1` buckets (33 for 32-bit keys, 65 for 64-bit keys) indexed by `std::bit_width(encode(key) ^ last)`. They need no ISA code or Barrett/Montgomery reduction (Contest profile); the one threshold, the 4096-slot bucket release, has the measurement under [Benchmarks](#benchmarks). The header depends only on `01-template.hpp`. Preconditions are `assert`s checked once per call.

## Contracts

### Keys and `last`

- `K` is one of the six integer types `int`, `unsigned`, `long`, `unsigned long`, `long long`, `unsigned long long` (the `radixheap_detail::Key` concept: a 4- or 8-byte integral type other than `wchar_t` and `char32_t`). This covers `int`, `lng`, `uint`, `ulng` and the other 64-bit spelling on every judge (Linux spells `int64_t` as `long`, Codeforces as `long long`; `long` is 32-bit there). Character types are rejected because `wchar_t` is 4 bytes on Linux but 2 on Codeforces; 8- and 16-bit and 128-bit keys are rejected too (user decision 2026-11-02).
- `radixheap_detail::encode` maps `K` onto its unsigned type preserving order. A signed key flips its sign bit; an unsigned key is unchanged. Negative keys therefore work, and the least `K` (`INT_MIN`, `LLONG_MIN`, 0) encodes to 0.
- `last` (public, stored encoded) starts at the least `K`. `top()` and `pop()` set it to the minimum key in the heap at that call, and every new key must be `>= last`.
  - A key equal to `last` is valid, including between `top()` and `pop()`.
  - After `top()`, a key below the current minimum but at least the previously popped key is invalid (hitonanode's warning): `push(10), push(20), pop(), top(), push(15)` asserts. Dijkstra-style use never does this. Kept as in every catalog, because a `top` that does not pull would rescan the lowest bucket on each call (user decision 2026-11-02).
- Invariant: bucket 0 holds keys equal to `last`. For `b > 0`, bucket `b` holds keys that agree with `last` above bit `b - 1` and have bit `b - 1` set, so they are larger.
- Pull, when bucket 0 is empty, proceeds as follows:
  1. Take the lowest nonempty bucket `b` and set `last` to its minimum, found by one scan; no per-bucket minimum is stored.
  2. Move every element of bucket `b` to `bit_width(encode(key) ^ last) < b`, because they all share the bits from `b - 1` up.
  3. Empty bucket `b`, releasing its storage when its capacity exceeds 4096 slots (`radixheap_detail::release`).

  An element changes buckets at most `b` times and a pull scans at most `b + 1` buckets. Charging the moves to the push gives O(b) amortized push and pop (b = 32 or 64). A heap holding n copies of the maximum key that receives and pops the chain `~0 << j` for j = 63 … 1 moves the n copies once per pop, about n * b moves in all, so pop alone is not O(b) amortized. `decreaseKey` never moves an element to a higher bucket (`last <= x <= key` share the bits above the old bucket), so it adds no moves and costs O(1) amortized.
- Elements of bucket `b` always share the bits from `b - 1` up, even when a precondition was violated, because `last` keeps those bits while the bucket is nonempty. So pulls always move elements strictly down and stay bounded under `NDEBUG`; only the order of the output becomes meaningless.

### RadixHeap

`RadixHeap<K, V>`: a min-heap of `(key, payload)` pairs. Duplicate keys and duplicate payloads are allowed.

- `push(key, val)`: asserts `key >= last`; takes `val` by value and moves it in, so move-only payloads (`std::unique_ptr`) work.
- `top()`: returns a reference to a pair with minimum key. Among equal keys the choice is unspecified (in practice the last one placed in bucket 0). It asserts a nonempty heap, and `push` or `pop` invalidates the reference. It is not `const`, because it pulls.
- `pop()`: removes the pair `top()` would return at that moment; asserts a nonempty heap. A `push` of a key equal to `last` between `top()` and `pop()` can change which pair that is (`push(5, 1)`, `top()` gives `(5, 1)`, `push(5, 3)`, `pop()` removes `(5, 3)`). Under `NDEBUG`, `pop()` on an empty heap does nothing, and `top()` on an empty heap reads `back()` of an empty vector, as `std::priority_queue::top` does.
- `size()`, `empty()`.
- Payloads move during pulls (`V` must be move-constructible). Memory is O(n + b) with n the largest size reached: an element leaves a bucket `b > 0` only by a pull of that bucket, so a bucket's capacity stays within twice its live size or the 4096-slot release limit, and bucket 0's within twice the peak size. Without the release, a cascade such as the chain above retained 67 n slots (about 1 GB at n = 10^6; user decision 2026-11-02). Copies are independent (when `V` is copyable).

### RadixHeapArray

`RadixHeapArray<K>(n)`: ids `[0, n)`, `0 <= n <= INT_MAX`, each holding at most one key. It stores `k[i]` (key), `bk[i]` (bucket, `-1` when absent) and `pos[i]` (index inside the bucket).

- `push(i, x)`: asserts `!contains(i)` and `x >= last`. Under `NDEBUG`, a present id is a no-op.
- `decreaseKey(i, x)`: asserts `contains(i)` and `last <= x <= key(i)`. It removes `i` from its bucket in O(1), by moving the bucket's last id into its slot, then places it again. A larger `x` would also keep the invariants, but the name and the assertion follow `DAryHeap` (user decision 2026-11-02). Under `NDEBUG`, an absent id is a no-op.
- `top()`: the id of a minimum key (ties unspecified); asserts a nonempty heap. `pop()` removes the id `top()` would return at that moment. That id becomes absent and can be pushed again, and `key(i)` is no longer valid for it. A `push` or `decreaseKey` to `last` between `top()` and `pop()` can change which id that is.
- `contains(i)` asserts `0 <= i < n`. `key(i)` asserts `contains(i)`. `size()`, `empty()`.
- Push-or-decrease (hitonanode `chmin`) is `contains(i) ? decreaseKey(i, x) : push(i, x)`.
- Memory is O(n + b + k) with k `decreaseKey` calls: a removal by `decreaseKey` leaves its slot in the old bucket's capacity until that bucket is next pulled. A lazy `RadixHeap` Dijkstra stores those k entries anyway. Copies are independent.

### Violated preconditions

Under `NDEBUG`:

- A key below `last` is stored in some bucket, and later pulls stay bounded (see Keys and `last`), but the pop order is meaningless.
- An out-of-range id reads or writes outside the id vectors.
- A negative `n` builds an empty id range.
- `push` of a present id and `decreaseKey` of an absent id are no-ops, so each id has at most one bucket entry and the element count equals the bucket total; every pull terminates. Without the `push` guard, a second entry would land back in the bucket being pulled (review finding, 2026-11-02).

### Legacy mapping

No legacy source. Catalog names:

| Catalog name | Here |
|---|---|
| Nyaan `RadixHeap<Key, Val>` `push`, `pop` (returns the pair), `top`, `size`, `empty` | `RadixHeap` with `top()` then `pop()` |
| ei1333 `RadixHeap` | the same |
| noshi91 `radix_heap` | the same |
| hitonanode and iwiwi `radix_heap` `push`/`emplace`, `top_key`/`top_label`, `pop` | `push`, `top().first`/`.second`, `pop` |
| iwiwi `pair_radix_heap` | `RadixHeap<K, V>` |
| hitonanode `radix_heap_array` `chmin`/`emplace` | `contains` with `push`/`decreaseKey` |
| hitonanode `radix_heap_array` `argmin_pop` | `top()` then `pop()` |
| hitonanode `radix_heap_array` `top`, `empty` | `top`/`key(top())`, `empty` |
| `clear` | a new object |

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/31-radixheap_tester.cpp`, entry `31-radixheap_tester.py` (driver `_00_runner.py`; three builds; 22 assertion probes).

The oracles never use the heap: a `multiset<pair<K, int>>` for `RadixHeap`, and a `set<pair<K, int>>` with a per-id key array and a present/absent partition for `RadixHeapArray`. The expected `last` is computed by subtraction from the least `K` and checked after every step.

| Operation | Group | Oracle |
|---|---|---|
| `RadixHeap` `push`, `top` (repeated), `pop` (after `top`, or alone with a copy predicting the removed pair), `size`, `empty` | `histories`: all four key types; profiles mixed, many duplicates (payloads 0–2), extremes, tiny heap (at most 3 elements), bucket boundaries; gaps 0, small, `2^j - 1`, `2^j`, `2^j + 1`, uniform to the top, near zero (sign crossing) | Multiset: top key equals the minimum, the pair is stored, that exact pair is erased; final drain |
| `RadixHeapArray` `push`, `decreaseKey` (to the same key, to `last`, uniform, `2^j` lower, 1 lower), `pop`, `top`, `key`, `contains`, `size`, `empty` | `histories`: n = 1, 2, 3, 64, 300 (quick), plus 65 and 5000 (full), plus 50000 (stress); all ids checked every `max(1, n / 64)` steps; id reuse after `pop` | Set, key array and partition (exactly one minimum-key id leaves) |
| Scale | `heapSort` (uniform and narrow keys, both structs, four types); `dijkstra` on lng and ulng weights with a lazy `RadixHeap` and with `RadixHeapArray` | `std::sort`; a `std::priority_queue` Dijkstra, itself checked against Bellman–Ford for n ≤ 64 |
| Extremes, empty and copies | `edges`: `INT_MIN`, `INT_MAX`, `LLONG_MIN`, `LLONG_MAX`, min + 1, max - 1, ±max / 2, -1, 0, 1, `UINT_MAX`, `ULLONG_MAX`; a key equal to `last` between `top` and `pop`; a push at `last` after a full drain; long-string payloads; n = 0, default, n = 1, all 1000 ids present; `decreaseKey` across buckets; copies drained independently | Direct values |
| `long long` and `unsigned long long` keys, move-only `std::unique_ptr` payloads, `pop` after `top` and a `decreaseKey` to `last` | `spellings` | Direct values |
| Key domain | `static_assert`s: `long`, `unsigned long`, `long long`, `unsigned long long` accepted; `wchar_t`, `char32_t`, `short`, `bool`, `lll` rejected | Compile time |
| Retained bucket capacity | `memoryBound`: n = 2^17 copies of `ULLONG_MAX`, then the chain `~0 << j` (j = 63 … 1) pushed and popped, in both structs | Sum of bucket capacities at most 4n + 65 * 4096 (the pre-fix header retained 16,646,145 slots and fails) |
| Preconditions | `invalid` | heap: push below `last` after `pop`, after `top`, after a drain, negative (`int`) and unsigned (`ulng`); `top`/`pop` empty and after a drain. Array: push below `last` after `pop` and after `top`; `top`/`pop` empty; `contains` out of range and negative; push out of range; `key` of an absent and a popped id; push of a present id; `decreaseKey` on an absent id, above `key(i)`, below `last`; negative size |

Sizes per mode:

| Mode | Histories | `heapSort` | Large `dijkstra` |
|---|---|---|---|
| quick | 300 steps | 10^4 | n = 2000 |
| full | 4000 steps | 10^6 per type | n = 2 * 10^5, m = n / 2 and 4n |
| stress | 30000 steps | 4 * 10^6 per type | n = 10^6, m up to 4 * 10^6 |

- Every mode also runs the small Dijkstra grid: n ∈ {1, 2, 5, 64}, m ∈ {0, n / 2, 4n}, four weight caps per type, including all-zero `ulng` weights and the largest safe weight.
- The checked build (`_GLIBCXX_DEBUG`, whose heap checks make the oracle quadratic) runs the large Dijkstra at n = 2000 and, in full and stress, `heapSort` at a tenth of the size (quick sorts 10^4 in every build).
- In stress, the checked and ASan builds run 10000 history steps.

Mutation check (2026-11-02, quick mode, optimized and ASan builds) caught all 10 mutants, and the random histories alone caught every one:

- pull taking `v[b][0]` instead of the minimum (both structs);
- bucket `bit_width - 1` in `push` and in `place`;
- `pop` without `bk[i] = -1`;
- `decreaseKey` without the moved id's `pos` fix;
- `encode` without the sign flip;
- pull without clearing `v[b]`;
- `place` without `^ last`;
- pull without updating `last`.

## Benchmarks

The bucket release threshold (4096 slots), measured 2026-11-02 on the machine below with `g++ -O2`. Drivers are the `/finish` review's scratch probes, not kept as repository drivers:

- `mem.cpp`: the cascade below; retained slots and peak RSS (`VmHWM`).
- `bench.cpp`: Dijkstra from vertex 0 on a random multigraph (`mt19937_64` seed 1), using a lazy `RadixHeap<lng, int>` and a `RadixHeapArray<lng>` with push-or-decrease. Each timing is the median of 5 repetitions on the same graph, and the two structs' distances are cross-checked.

Before is `clear()` on every pulled bucket.

| Workload | Before | After |
|---|---|---|
| Cascade, n = 10^6 copies of `ULLONG_MAX` plus the 63-key chain: `RadixHeap<ulng, int>` | 67.1 n slots, 988 MB peak | 1.0 n slots, 50 MB peak |
| The same cascade, `RadixHeapArray<ulng>` | 67.1 n slots, 265 MB peak | 1.0 n slots, 30 MB peak |
| Dijkstra n = 10^6, m = 4 * 10^6, weights < 10^3: heap / array | 0.215 s / 0.271 s, 191 MB | 0.223 s / 0.279 s, 165 MB |
| The same, weights < 10^18 | 0.499 s / 0.609 s, 191 MB | 0.518 s / 0.524 s, 168 MB |
| Dijkstra n = 2 * 10^5, m = 8 * 10^5, weights < 10^9 | 0.047 s / 0.047 s, 40 MB | 0.049 s / 0.049 s, 35 MB |

On these Dijkstra runs the heap is 3–4% slower, the array from 14% faster to 4% slower, and peak memory drops 12–14%. The review measured 5–20% slowdowns when every pulled bucket is freed.

## Commands and results

Latest run 2026-11-02, Linux x86-64 (AMD Ryzen AI 9 HX 370, WSL2), GNU++20, CPython 3.14.4, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `31-radixheap_tester.py --mode quick` | PASS, 2 configurations, 1,624,169 checks, 22 assertion probes, MEMORY peak 638 MB |
| `31-radixheap_tester.py --mode full --seed 20261102` (`g++` 16.0.1) | PASS, 3 configurations; 38,252,847 checks (optimized and ASan), 38,200,056 (checked); MEMORY peak 751 MB |
| `CXX=g++-14 31-radixheap_tester.py --mode full --seed 20261102` (g++-14 14.3.0, floor) | PASS, the same check counts, MEMORY peak 750 MB |
| `31-radixheap_tester.py --mode stress --seed 7` | PASS, 3 configurations, 352,865,096 checks (optimized), 113,408,184 (checked), 113,437,110 (ASan); MEMORY peak 752 MB |
| `02-integration.py` | PASS, 184 standalone and aggregate headers, scalar and AVX2 multi-TU, workspace; MEMORY peak 1447 MB |
| `03-consistency.py`, `plan.py check` | No errors (maintainer gate `merge_pr.py gates 1`, 2026-11-03) |

The memory peaks cover the whole runner scope, compiles included.

```bash
python3 '96-Local Testing/02-Data Structures/31-radixheap_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/31-radixheap_tester.py' --mode full --seed 20261102
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/31-radixheap_tester.py' --mode full --seed 20261102
python3 '96-Local Testing/02-Data Structures/31-radixheap_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| Nyaan `data-structure/radix-heap.hpp`, ei1333 `structure/heap/radix-heap.hpp`, noshi91 `data_structure/radix_heap.cpp`, hitonanode `data_structure/radix_heap.hpp`, iwiwi `radix_heap.h` | The `bit_width(key ^ last)` buckets, the pull from the lowest nonempty bucket, the key+payload form, signed keys by sign-bit flip (iwiwi), `top` moving `last` (hitonanode). |
| hitonanode `data_structure/radix_heap_array.hpp` | Per-id bucket and position, with O(1) removal by swapping in the bucket's back. Plain `int` fields here remove its 25-bit position cap. |

The completeness sweep fetched these on 2026-11-02 ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds27-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p032-omissions) (user decision 2026-11-02): `clear`, `swap`, floating-point keys, bounded-gap heaps, the Rust crate's extras and `chmin`.
- The `NDEBUG` boundedness claims (Violated preconditions) have no tester group, because optimized builds receive only in-domain inputs (`05-testing.md`, Memory safety). The `/finish` review checked them ad hoc on 2026-11-02 with an `-DNDEBUG` ASan/UBSan fuzz of 12,000 violating histories on all four key types: all bounded, and the element count always matched the bucket totals. `top()` on an empty heap under `NDEBUG` stays undefined, as for `std::priority_queue`.
- Sources not consulted beyond the sweep: suisen (its heap folder lists only binary and interval heaps), tko919 and ACL have no radix heap (index pages checked 2026-11-02). The bounded-gap omission rests on scrapbox, not the primary Ahuja et al. paper. AOJ GRL_1_A and yukicoder 1288 and 1320 were not fetched, and no `97-Online Testing` solution uses these structs yet.
- Graphs row 13 `radixHeapDijkstra` (planned) consumes `RadixHeap`.

## History

- 2026-11-02: P032 DS27 first implementation; independent review, 6 findings and 2 nits fixed (`push` guard for a present id under `NDEBUG`, amortized bounds charged to push, `pop` after an intervening `decreaseKey`, payload taken by value, any 32/64-bit integer key, row status); 10/10 mutants caught; quick, full (seed 20261102) on g++ and g++-14, stress (seed 7) and `02-integration.py` passed after the fixes.
- 2026-11-02: `/finish` audit, 3 findings and 4 limits; user decisions recorded (bucket release above 4096 slots, the six integer key types, the omissions, `top` moving `last`, `decreaseKey` asserting a decrease); diff review, 4 findings fixed (release cost comment, cv-qualified keys rejected, benchmark medians and seed, stale results); `memoryBound` and key-domain checks added; quick, full (seed 20261102) on g++ and g++-14, stress (seed 7) and `02-integration.py` passed.
- 2026-11-03: maintainer `/merge-pr 1`: the real `03-consistency.py` and `plan.py check` results recorded and the clone-specific limit dropped (P250's note keeps the follow-up); the first maintainer gate (2026-11-03) passed quick and full on g++ and g++-14 and `02-integration.py --sanitizers`.
