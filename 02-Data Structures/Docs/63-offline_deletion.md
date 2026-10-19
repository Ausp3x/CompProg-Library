# 63-offline_deletion.hpp — evidence

Package P034 (batch DS51). New header; no legacy source. A generic add/remove-over-time engine (segment tree on time) and the queue-undo adapter, both driving any undo-only structure through callbacks. Preconditions are `assert`s checked once per call.

## Contracts

### OfflineDeletion<X>

Times `[0, n)`, `0 <= n <= 2^29` (the time tree's `2 * bit_ceil(n)` cells stay inside `int`). `X` is copyable; `insert` and `erase` also need `operator<` on `X` (an ordered map of open copies). A type without `operator<` works with `addInterval` alone.

- `addInterval(l, r, x)`: asserts `0 <= l <= r <= n`. `x` is present at every time in `[l, r)`; empty intervals add nothing. The interval is split into at most `2 log2(sz)` canonical nodes of a perfect binary tree on `sz = bit_ceil(max(n, 1))` leaves, O(log(n)).
- `insert(t, x)`, `erase(t, x)`: an event log per key. `insert` opens a copy of `x` at time `t` (`0 <= t <= n`). `erase` closes the most recently opened copy into `[open, t)`. Copies of equal keys are interchangeable, so the pairing does not change the multiset present at any time. `erase` asserts a live copy opened at time `<= t`, which holds when each key's events arrive in nondecreasing time. Copies still open when `solve` runs close at `n`. O(log(m)) map work plus `addInterval`. Kept beside `addInterval` because it accepts a different input domain: unpaired events with multiplicities (`duplicates.md`, Kept by the sweep).
- `addQuery(t)`: asserts `0 <= t < n`; marking a time twice is the same as once.
- `solve(add, undo, answer)`:
  - It closes open copies, then walks the time tree depth-first. At a node with an asked time below it, it calls `add(x)` for each element assigned there, recurses (`answer(t)` at an asked leaf), then calls `undo()` once per `add`.
  - Calls are LIFO: each `undo` must revert the most recent `add` not yet reverted. `answer(t)` runs once per asked time, in increasing `t`, and sees exactly the elements whose intervals contain `t`. Subtrees without asked times are skipped.
  - Each element costs at most `2 log2(sz)` `add`/`undo` pairs, so `solve` makes O(n + m log(n)) calls and runs in O(n + m log(n) c), where `c` is the structure's worst-case cost of an add plus its undo.
  - An amortized structure such as path compression breaks the bound, because a costly add can be undone and redone; row 09 `RollbackDSU` is the intended client.
  - Calling `solve` again repeats the same calls.
- Memory O(n + m log(n)) `(node, x)` entries, plus O(n + m log(n)) index arrays during `solve`.

### QueueUndo<X, Add, Undo>

`add(const X &)` applies an update and `undo()` reverts the most recently applied one. The structure's state must depend only on the set of applied updates (commuting updates), because `popFront` reorders the stack. `Add` and `Undo` default to `std::function`; passing the lambda types avoids the indirect call.

- `push(x)`: one `add`. `size()` and `empty()` count live updates.
- `popFront()`: asserts a live update and removes the oldest.
  - Entries are B (pushed) or A (reordered). When no A remains, it undoes everything and reapplies it in reverse as A, oldest on top.
  - It then takes the shortest top segment holding at least one A and as many A as B, or every A. It undoes the segment, stably moves the segment's B before its A, drops the topmost A (the oldest), and reapplies the rest.
  - Invariant: A entries are older than every B, the topmost A is the oldest live update, and the applied set equals the stack. So each call removes the oldest update.
- Bound: O(log(n)) amortized `add`/`undo` calls per operation, for n live updates. This is the bound Noam527's tutorial (Codeforces blog 83467) states for this procedure. The blog returned 403 in this session, so the statement comes from the sweep's search snippets, and no proof was checked here.
  - Argued here: a segment with s B entries has at most `max(s, 1)` A entries, so a call costs at most `4(s + 1)` calls. Each update is reversed at most once, since after a reversal it leaves only by removal. What remains open is that each B entry is popped O(log(n)) times, amortized, before its reversal.
  - Measured: the tester bounds total calls by `4 ops (1 + log2(peak + 1))` on five access patterns and prints the worst ratio (see Commands and results). The review probe found 10.6, 15.9 and 19.4 calls per operation in the steady state at n = 2^10, 2^16, 2^20, which tracks log2(n), and a worst ratio of 1.13 over 27 patterns.
- Memory O(n).

### Violated preconditions

Under `NDEBUG`:
- `addInterval` with bad bounds records nodes outside the tree, and `solve` indexes outside its arrays (undefined).
- `erase` of an absent key dereferences `end()` (undefined).
- An `n` outside `[0, 2^29]` is clamped for the tree size.
- `popFront` on an empty adapter returns, and the next call recovers through the reversal.

Every loop is bounded by `n`, `m log(n)` or the stack size.

### Legacy mapping

No legacy code. The planned row's "undoOnly adapter" describes `QueueUndo` itself (its constructor takes the undo-only structure's callbacks); it is not a separate operation. Catalog forms: cp-algorithms `add_query`/`solve` (closed ranges), hitonanode `apply_time_range`/`add_observation`/`generate`, Nyaan `run(add, del, query)` and ei1333 `run(f)` are `addInterval`/`addQuery`/`solve`; maspypy `Add_Remove_Query::add`/`remove`/`calc`, ei1333 `insert`/`erase` and Nyaan `add_edge`/`del_edge` are `insert`/`erase`; ShahjalalShohag `DSUQueue::push`/`pop` are `push`/`popFront`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/63-offline_deletion_tester.cpp`, entry `63-offline_deletion_tester.py` (driver `_00_runner.py`; three builds; 9 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `addInterval`, `addQuery`, `solve` | `recorder` (n <= 40 with m <= 60; n <= 700 with m <= 2000, at most 60 rounds; full also n <= 10^5 with m <= 2 * 10^5; duplicate ids, empty intervals, repeated `addQuery`, `solve` twice) | A recorder structure: the applied multiset at each answer equals the intervals containing `t`, exact when `n * m <= 2 * 10^6` and by count plus two SplitMix multiset hashes against a difference-array sweep otherwise. Answer order, LIFO completeness (`adds == undos`, empty at the end), adds at most the decomposition size. |
| Pruning | `pruning`: only time 0 asked; no time asked | Adds equal the intervals containing 0; no add without queries |
| `insert`, `erase` | `events` (repeated keys, inserts at `n`, erase at the insert time, copies left open) | Inserts minus erases at times `<= t` per key |
| Clients | `clients`: `RollbackDSU` (row 09) connectivity and an undo-only xor basis (n <= 40 times on 9 vertices; stress adds 30 rounds of n <= 120 on 16 vertices) | BFS components and fresh Gaussian elimination (rank, maximum xor) per asked time |
| `push`, `popFront`, `size`, `empty` | `queueUndo<TYPED>` with `std::function` and lambda types, five patterns (random, all pushes then all pops, push-push-pop, blocks of 64, warm-up then alternation), up to 300 ops; full adds three rounds of 2 * 10^5 ops per pattern, stress one more of 6 * 10^5 | `std::deque` of live updates against the recorder's applied multiset |
| Amortized bound | `queueUndo` | `add` + `undo` calls at most `4 ops (1 + log2(peak + 1))`; worst measured ratio `calls / (ops (1 + log2(peak + 1)))` printed |
| `QueueUndo` client | `queueDsu` (RollbackDSU) | BFS components of the live edges after every operation |
| n = 0 and n = 1, empty queue, `X` without `operator<` | `edgesCase` | Direct values |
| Preconditions | `invalid` | Reversed and out-of-range intervals, out-of-range query and insert, erase of an absent key, erase before its insert, negative and oversized `n`, `popFront` on empty |

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/63-offline_deletion_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/63-offline_deletion_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/63-offline_deletion_tester.py' --mode stress --seed 7
```

Final run 2026-10-17: full mode seed 20261017 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 3,659,275 checks per configuration and 9 assertion probes, MEMORY peak 555 MB and 610 MB; stress mode seed 7 (one round) passed all three configurations with 8,360,815 checks per configuration, MEMORY peak 768 MB. Mutation check (temporary header copies, quick mode seed 7, `-O2 -DNDEBUG`): 5 of 5 injected faults detected (left decomposition step not advanced, pruning by AND, oldest update kept by `popFront`, `erase` pairing the oldest copy, `popFront` stopping early). Worst measured QueueUndo ratio `calls / (ops (1 + log2(peak + 1)))`: 1.136 (full), 1.128 (stress). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds; MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (153 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 1820 MB) and `03-consistency.py` reported no errors.

## Sources

| Source | Actual reading and use |
|---|---|
| cp-algorithms *Deleting from a data structure in O(T(n) log n)*, OI Wiki 线段树分治 | Segment tree over time with queries at leaves; amortized structures invalid. |
| hitonanode `offline_dynamic_connectivity.hpp`, maspypy `add_remove_query.hpp`, Nyaan and ei1333 `offline-dynamic-connectivity.hpp` | Observations, insert/erase pairing into half-open intervals with multiplicities, callbacks. |
| ShahjalalShohag `Queue Undo Trick.cpp` (via the sweep) | The queue-undo procedure. Noam527's Codeforces tutorial (blog 83467) returned 403; its O(log(n)) amortized bound is known only from search snippets and is supported here by measurement. |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p034-omissions): priority-queue undo and deque undo.
- Graphs `26-dynamic_connectivity.hpp` owns edge-interval connectivity orchestration and may build on `OfflineDeletion` with row 09; Miscellaneous row 42 owns generic undo logs.

## History

- 2026-10-17: P034 first implementation and verification; independent review: `n <= 2^29` asserted before allocation, in-place `popFront` that recovers after an empty call, `QueueUndo` `size`/`empty` added to the row, `insert`/`erase` recorded as kept, the amortized bound stated with its evidence, unordered-key test.
