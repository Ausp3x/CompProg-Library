# 30-aggregation_queue.hpp — evidence

Package P028 (batch DS26). New header; no legacy source. Indices are not exposed; every structure owns its storage and copies as an independent snapshot. Preconditions are `assert`s checked once per operation.

## Contracts

### Monoid contract

`M` provides `S`, `op(a, b)` (associative, need not be commutative) and `e()` (two-sided identity), under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): each structure keeps it as `m` and takes it in its only constructor `X(M mon = {})`. `allProd()` of an empty structure returns `e()`, so absence and a legitimate identity product are the same value by design (both Library Checker composite problems print `x` for an empty queue).

### SWAGQueue<M>

`push(x)` appends at the back; `pop()` removes the front (asserts nonempty); `front()`, `back()` return references valid until the next mutation (assert nonempty); `allProd()` returns `op(oldest, ..., newest)`; `size()`, `empty()`, `clear()`. Representation: the back stack keeps raw values plus their running product `acc`; the front stack keeps `(value, product of it and every newer front element)`. When `pop` finds the front stack empty it moves the back stack across in O(k) and resets `acc`. Each element is moved at most once, so `pop` is O(1) amortized and every other operation O(1) worst case.

### SWAGDeque<M>

`pushFront`, `pushBack`, `popFront`, `popBack` (pops assert nonempty), `front`, `back`, `allProd` (front to back), `size`, `empty`, `clear`. Both sides store `(value, product toward the middle)`. A pop on an empty side rebuilds both sides from the other side, giving the side that ran out `ceil(k / 2)` elements (`(k + fr.empty()) / 2` to the front). Potential `|size(front) - size(back)|`: it is `k` when a rebuild of `k` elements happens and at most 1 after it, and each push or pop changes it by at most 1, so every operation is O(1) amortized. Moving everything to one side would allow alternating pops to cost O(n) each.

### SWAGStack<M>

`push`, `pop` (asserts nonempty), `top`, `allProd` (bottom to top), `size`, `empty`, `clear`; all O(1) worst case.

### Violated preconditions

Under `NDEBUG`, popping an empty structure is a no-op (the pops are guarded so the vector end never moves below its start); `front`, `back` and `top` on an empty structure read invalid memory. All calls stay O(1) amortized.

### Legacy mapping

| Renamed (P027 monoid pass, 2026-10-17) | Replacement |
|---|---|
| `fold()` | `allProd()` |

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/30-aggregation_queue_tester.cpp`, entry `30-aggregation_queue_tester.py` (driver `_00_runner.py`; three builds; 9 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| SWAGQueue `push`, `pop`, `front`, `back`, `allProd`, `size`, `empty`, `clear` | `queueRandom<Affine>`, `queueRandom<Concat>` | `std::deque` plus a left-to-right fold after every step; affine maps mod 998244353 (Library Checker composite order) and string concatenation are both noncommutative |
| SWAGDeque `pushFront`, `pushBack`, `popFront`, `popBack`, `front`, `back`, `allProd`, `size`, `empty`, `clear` | `dequeRandom<Affine>`, `dequeRandom<Concat>` | Same |
| SWAGStack `push`, `pop`, `top`, `allProd`, `size`, `empty`, `clear` | `stackRandom<Affine>`, `stackRandom<Concat>` | Same |
| Amortized O(1) | `amortized` | Counts `op` calls on patterns that force rebuilds on alternating sides (alternate front/back pops of n elements, refill/drain on switching sides, queue with interleaved pops) and fails above `8 * operations + 8` |
| Empty product, copy and move independence, reuse after `clear`, side switching on one element; pushes of a reference into the structure's own storage (`push(back())`, 200 times per structure); default members copy-list-initialized inside an aggregate; runtime monoid state | `edges` | Direct values; ordered affine composites modulo a member modulus (7 and 1000003) in all three structures over 300 mixed operations against a left fold |
| Preconditions | `invalid` | Pops, `front`, `back` and `top` on empty structures |

Mutation check (2026-10-10, quick mode, temporary copies): 5 of 5 injected faults detected (reversed product order in the queue transfer, deque `pushFront` and queue `allProd`, stack `push`; a rebuild that gives the empty side nothing).

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `30-aggregation_queue_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,446,854 checks per configuration, 9 assertion probes, MEMORY peak 463 MB |
| `CXX=g++-14 30-aggregation_queue_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,446,854 checks per configuration, MEMORY peak 473 MB |
| `30-aggregation_queue_tester.py --mode stress --seed 7` | PASS, 8,673,704 checks per configuration, MEMORY peak 640 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/30-aggregation_queue_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/30-aggregation_queue_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/30-aggregation_queue_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `queue_operate_all_composite`, `deque_operate_all_composite` task statements | Fold order (oldest map applied first) and the empty-fold answer. |
| noshi91 `stack_aggregation`, `queue_aggregation` | `top`, `empty` and the two-stack queue layout. |
| ei1333 `deque-operate-aggregation`, suisen `deque_aggregation`/`queue_aggregation`, Nyaan `slide-window-aggregation-deque`, tko919 `dequeswag`, maspypy `sliding_window_aggregation` | `size`, `empty`, `front`, `back`, `clear` and the half-split rebuild with its potential argument. |
| cp-algorithms, Minimum stack / Minimum queue | Two-stack queue reduction. |

Fetched 2026-10-10 by the completeness sweep ([00-sources.md](00-sources.md)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p028-omissions): random access `operator[]`, separate front/back folds (`lprod`/`rprod`), pop returning the value, iterator constructors, separate value and product types.
- `08-monotone_stack.hpp` keeps the idempotent sliding minimum; `47-persistentqueue.hpp` and `63-offline_deletion.hpp` (queue undo) are separate rows.

## History

- 2026-10-10: P028 first implementation and verification.
- 2026-10-10: independent review (P028): no defects; the deque's per-side helper folds were renamed `frFold`/`bkFold` to mark them as internal.
- 2026-10-17: P027 monoid pass (stored-object contract, `allProd` name, stateful tests; review fix: `SWAGQueue::push` read its argument after `push_back` could reallocate, so `push(back())` used freed memory (ASan); it now combines first, with a regression test, and the constructors stay implicit so default members still copy-list-initialize); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 463/473/640 MB.
