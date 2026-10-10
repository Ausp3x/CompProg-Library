# 30-aggregation_queue.hpp — evidence

Package P028 (batch DS26). New header; no legacy source. Indices are not exposed; every structure owns its storage and copies as an independent snapshot. Preconditions are `assert`s checked once per operation.

## Contracts

### Monoid contract

`M` provides `S`, `static S op(const S &, const S &)` (associative, need not be commutative) and `static S e()` (two-sided identity). `fold()` of an empty structure returns `e()`, so absence and a legitimate identity product are the same value by design (both Library Checker composite problems print `x` for an empty queue).

### SWAGQueue<M>

`push(x)` appends at the back; `pop()` removes the front (asserts nonempty); `front()`, `back()` return references valid until the next mutation (assert nonempty); `fold()` returns `op(oldest, ..., newest)`; `size()`, `empty()`, `clear()`. Representation: the back stack keeps raw values plus their running product `acc`; the front stack keeps `(value, product of it and every newer front element)`. When `pop` finds the front stack empty it moves the back stack across in O(k) and resets `acc`. Each element is moved at most once, so `pop` is O(1) amortized and every other operation O(1) worst case.

### SWAGDeque<M>

`pushFront`, `pushBack`, `popFront`, `popBack` (pops assert nonempty), `front`, `back`, `fold` (front to back), `size`, `empty`, `clear`. Both sides store `(value, product toward the middle)`. A pop on an empty side rebuilds both sides from the other side, giving the side that ran out `ceil(k / 2)` elements (`(k + fr.empty()) / 2` to the front). Potential `|size(front) - size(back)|`: it is `k` when a rebuild of `k` elements happens and at most 1 after it, and each push or pop changes it by at most 1, so every operation is O(1) amortized. Moving everything to one side would allow alternating pops to cost O(n) each.

### SWAGStack<M>

`push`, `pop` (asserts nonempty), `top`, `fold` (bottom to top), `size`, `empty`, `clear`; all O(1) worst case.

### Violated preconditions

Under `NDEBUG`, popping an empty structure is a no-op (the pops are guarded so the vector end never moves below its start); `front`, `back` and `top` on an empty structure read invalid memory. All calls stay O(1) amortized.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/30-aggregation_queue_tester.cpp`, entry `30-aggregation_queue_tester.py` (driver `_00_runner.py`; three builds; 9 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| SWAGQueue `push`, `pop`, `front`, `back`, `fold`, `size`, `empty`, `clear` | `queueRandom<Affine>`, `queueRandom<Concat>` | `std::deque` plus a left-to-right fold after every step; affine maps mod 998244353 (Library Checker composite order) and string concatenation are both noncommutative |
| SWAGDeque `pushFront`, `pushBack`, `popFront`, `popBack`, `front`, `back`, `fold`, `size`, `empty`, `clear` | `dequeRandom<Affine>`, `dequeRandom<Concat>` | Same |
| SWAGStack `push`, `pop`, `top`, `fold`, `size`, `empty`, `clear` | `stackRandom<Affine>`, `stackRandom<Concat>` | Same |
| Amortized O(1) | `amortized` | Counts `op` calls on patterns that force rebuilds on alternating sides (alternate front/back pops of n elements, refill/drain on switching sides, queue with interleaved pops) and fails above `8 * operations + 8` |
| Empty fold, copy and move independence, reuse after `clear`, side switching on one element | `edges` | Direct values |
| Preconditions | `invalid` | Pops, `front`, `back` and `top` on empty structures |

Mutation check (2026-10-10, quick mode, temporary copies): 5 of 5 injected faults detected (reversed product order in the queue transfer, deque `pushFront` and queue `fold`, stack `push`; a rebuild that gives the empty side nothing).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/30-aggregation_queue_tester.py' --mode full --seed 20261010
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/30-aggregation_queue_tester.py' --mode full --seed 20261010
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/30' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-10 (after the review fixes): full mode seed 20261010 passed on GCC 16.2 and GCC 14.4.1 (`CXX=g++-14`) with 1,445,109 checks per configuration and 9 assertion probes, MEMORY peak 445 MB and 456 MB; stress mode seed 7 (one round) passed all three configurations with 8,671,900 checks per configuration, MEMORY peak 626 MB. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds. MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (112 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 2693 MB), and a separate two-unit probe instantiating `SegTreeBeats`, `HistoricSegTree`, `SegmentTree2DDense`, `SWAGQueue`, `SWAGDeque` and `MergeSortTree` in both units linked and ran. `03-consistency.py` cannot run inside this worktree (seven gitignored binaries under `OLD/` and one benchmark log exist only in the main checkout); on a copy with those files restored after rebasing onto master (97ed2db) it reported no errors, and the four suites passed quick mode again. GCC 14.2 exactly, the Windows build and PyPy were not run.

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
