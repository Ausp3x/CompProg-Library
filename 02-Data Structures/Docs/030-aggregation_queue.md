# 030-aggregation_queue.hpp — Evidence

Package P028 (batch DS26). New header; no legacy source. No indices are exposed; every structure owns its storage and copies as an independent snapshot. Each operation asserts its preconditions once.

## Contracts

### Monoid Contract

`M` provides `S`, an associative `op(a, b)` that need not be commutative, and a two-sided identity `e()`, under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): each structure keeps it as `m` and takes it in its only constructor `X(M mon = {})`. `allProd()` of an empty structure returns `e()`, so by design an empty structure and an identity product look the same; both Library Checker composite problems print `x` for an empty queue.

### SWAGQueue<M>

`push(x)` appends at the back and `pop()` removes the front (asserts nonempty). `front()` and `back()` assert nonempty and return references valid until the next mutation. `allProd()` returns `op(oldest, ..., newest)`; `size()`, `empty()` and `clear()` complete the interface. The back stack keeps raw values and their running product `acc`; the front stack keeps pairs of a value and the product of it and every newer front element. When `pop` finds the front stack empty, it moves the back stack across in O(k) and resets `acc`. Each element moves at most once, so `pop` is O(1) amortized and every other operation O(1) worst case.

### SWAGDeque<M>

`pushFront`, `pushBack`, `popFront`, `popBack` (pops assert nonempty), `front`, `back`, `allProd` (front to back), `size`, `empty`, `clear`. Both sides store pairs of a value and the product toward the middle. A pop on an empty side rebuilds both sides from the other side and gives the side that ran out `ceil(k / 2)` elements (`(k + fr.empty()) / 2` to the front). With potential `|size(front) - size(back)|`, which is `k` when a rebuild of `k` elements happens, at most 1 after it, and changes by at most 1 per push or pop, every operation is O(1) amortized. Moving everything to one side instead would let alternating pops cost O(n) each.

### SWAGStack<M>

`push`, `pop` (asserts nonempty), `top`, `allProd` (bottom to top), `size`, `empty`, `clear`; all O(1) worst case.

### Violated Preconditions

Under `NDEBUG`, popping an empty structure is a no-op, because the pops never move the vector end below its start; `front`, `back` and `top` on an empty structure read invalid memory. All calls stay O(1) amortized.

### Legacy Mapping

| Renamed (P027 monoid pass, 2026-10-17) | Replacement |
|---|---|
| `fold()` | `allProd()` |

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/030-aggregation_queue_tester.cpp`, entry `030-aggregation_queue_tester.py` (driver `_00_contest_runner.py`; three builds; 9 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| SWAGQueue `push`, `pop`, `front`, `back`, `allProd`, `size`, `empty`, `clear` | `queueRandom<Affine>`, `queueRandom<Concat>` | `std::deque` plus a left-to-right fold after every step; affine maps mod 998244353 (Library Checker composite order) and string concatenation are both noncommutative |
| SWAGDeque `pushFront`, `pushBack`, `popFront`, `popBack`, `front`, `back`, `allProd`, `size`, `empty`, `clear` | `dequeRandom<Affine>`, `dequeRandom<Concat>` | Same |
| SWAGStack `push`, `pop`, `top`, `allProd`, `size`, `empty`, `clear` | `stackRandom<Affine>`, `stackRandom<Concat>` | Same |
| Amortized O(1) | `amortized` | Counts `op` calls on patterns that force rebuilds on alternating sides (alternate front/back pops of n elements, refill/drain on switching sides, queue with interleaved pops) and fails above `8 * operations + 8` |
| Empty product, copy and move independence, reuse after `clear`, side switching on one element; pushes of a reference into the structure's own storage (`push(back())`, 200 times per structure); default members copy-list-initialized inside an aggregate; runtime monoid state | `edges` | Direct values; ordered affine composites modulo a member modulus (7 and 1000003) in all three structures over 300 mixed operations against a left fold |
| Preconditions | `invalid` | Pops, `front`, `back` and `top` on empty structures |

A hand mutation check on 2026-10-10 (quick mode, temporary copies) detected 5 of 5 injected faults: a reversed product order in the queue transfer, in deque `pushFront`, in queue `allProd` and in stack `push`, and a rebuild that gives the empty side nothing.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 751 MB; optimized 0.3 + 0.8 s, checked 17.4 + 2.0 s, ASan-UBSan 21.4 + 9.4 s; MEMORY peak 902 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 797 MB; optimized 0.2 + 1.0 s, checked 20.5 + 1.7 s, ASan-UBSan 26.2 + 7.2 s; MEMORY peak 935 MB |
| `01-run.py --mode quick` | Optimized 24.8 + 0.0 s, checked 20.8 + 0.2 s; MEMORY peak 743 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 751 MB; optimized 0.1 + 3.4 s, checked 6.4 + 14.6 s, ASan-UBSan 7.0 + 34.9 s; MEMORY peak 618 MB |

Latest per-header run 2026-10-17 (P027 monoid pass) on the same machine with CPython 3.14.7, in the optimized, checked and ASan/UBSan builds:

| Command (2026-10-17) | Result |
|---|---|
| `030-aggregation_queue_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,446,854 checks per configuration, 9 assertion probes, MEMORY peak 463 MB |
| `CXX=g++-14 030-aggregation_queue_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,446,854 checks per configuration, MEMORY peak 473 MB |
| `030-aggregation_queue_tester.py --mode stress --seed 7` | PASS, 8,673,704 checks per configuration, MEMORY peak 640 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/030-aggregation_queue_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/030-aggregation_queue_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/030-aggregation_queue_tester.py' --mode stress --seed 7
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

## Limits and Handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p028-omissions): random access `operator[]`, separate front and back folds (`lprod`/`rprod`), pop returning the value, iterator constructors, separate value and product types.
- `008-monotone_stack.hpp` keeps the idempotent sliding minimum; `047-persistentqueue.hpp` and `063-offline_deletion.hpp` (queue undo) are separate rows.

## Aliases

Search aliases (P263, grab --find): 双栈模拟队列, 双栈队列, 滑动窗口聚合, 双栈双端队列.

## History

- 2026-10-10: P028 first implementation and verification.
- 2026-10-10: P028 review: no defects; the deque's per-side helper folds became `frFold`/`bkFold` to mark them internal.
- 2026-10-17: P027 monoid pass (stored-object contract, `allProd` name, stateful tests). Review fix: `SWAGQueue::push` read its argument after `push_back` could reallocate, so `push(back())` used freed memory (ASan); it now combines first, with a regression test, and the constructors stay implicit so default members still copy-list-initialize. Full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
