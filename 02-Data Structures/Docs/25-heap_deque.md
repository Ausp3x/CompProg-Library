# 25-heap_deque.hpp — evidence

Package P029 (batch DS48). New header; no legacy source. Every heap is a min-heap for its comparator `C` (a strict weak order, default `std::less<T>`): `top()` is a C-minimum, and `std::greater<T>` gives a max-heap. Preconditions are `assert`s checked once per operation. Under `NDEBUG`, a pop on an empty structure and a `DAryHeap` `decreaseKey` or `erase` of an absent id are no-ops, and `top` on an empty `DAryHeap`, `ErasableHeap` or `DoubleEndedPriorityQueue` reads invalid memory, as `SWAGQueue` does.

## Contracts

### DAryHeap<T, D = 4, C> and BinaryHeap<T, C> = DAryHeap<T, 2, C>

- Ids in `[0, n)`, `0 <= n < 2^30`, each hold at most one key; `key(i)` returns it and asserts `i` present (read it before `pop`). `push(i, x)` asserts `i` absent; `decreaseKey(i, x)` asserts `i` present and `x` not C-greater than `key[i]`; `erase(i)` asserts `i` present; `pop()` removes `top()`; `contains(i)` asserts `0 <= i < n`.
- Layout: `h` holds `(key, id)` pairs in implicit `D`-ary heap order, so a sift compares contiguous keys, and `pos[i]` is the slot of id `i` or -1. The earlier id-only layout read `key[h[j]]` at random, and the pair layout made push/pop of 10^6 keys about 1.8 times faster for D = 2 and 1.25 times faster for D = 4 (prototype comparison, 2026-10-13). `DAryHeap(a)` holds ids `0 .. a.size() - 1` with keys `a`, built bottom-up in O(n). `push` and `decreaseKey` sift up, O(log(n) / log(D)); `pop` and `erase` move the last id into the hole and sift both ways, O(D log(n) / log(D)); `top`, `key`, `size`, `empty`, `contains` O(1). The default `D = 4` follows the benchmark below.

### ErasableHeap<T, C>

- Two binary heaps: `q` of pushed values and `d` of pending deletions. `clean` pops both while their tops are C-equivalent, so `q.top()` is always live and `top()` is O(1). `erase(x)` requires a present copy of `x` (C-equivalent values are interchangeable); `size()` is `|q| - |d|`. `push` O(log n); `erase`, `pop` O(log n) amortized, since each deletion cancels once; construction from a vector O(n); memory O(n + pending deletions).

### Meldable heaps: LeftistHeap, SkewHeap, RandomizedHeap, BinomialHeap, PairingHeap, FibonacciHeap (T, C)

- Each struct is a pool of nodes; node 0 is the empty heap, and a heap is a root id. `push(v, x)` returns the new node id; `meld(a, b)` returns the root of the union, after which `a` and `b` are no longer separate heaps; `pop(v)` and `top(v)` assert a nonempty heap; `size(v)` is O(1). Nodes are never reused (memory O(pushes)); copying a struct copies every heap in it. `T` is default-constructible (the sentinel's key).
- `LeftistHeap`: `s` is the null-path length (`s(0) = 0`, `s(l) >= s(r)`), so the right spine has at most `log2(n + 1)` nodes and the recursive `meld`, `push` and `pop` are O(log n) worst case; `sz` is the subtree size.
- `SkewHeap`: the top-down iterative meld swaps children along the merge path (no recursion); O(log n) amortized.
- `RandomizedHeap`: the iterative meld descends into a child chosen by a coin flip (`CustomHash::splitMix64(seed++)`, `seed` defaulting to `CustomHash::rnd`), expected O(log n). Las Vegas: the seed affects only time.
- `applyAdd(v, x)` (leftist and skew): adds `x` to every key of heap `v` in O(1) through a lazy tag, which a node passes to its children before they are touched. It takes `x` by value, so `applyAdd(v, top(v))` is valid, and it exists only when `T` supports `+=` (the `ADD` constraint), so `pair` keys still work without it. `C` must be preserved by adding one value to every key (true for `<` and `>` on numbers without overflow). Only `top(v)` is guaranteed current; inner `t[u].key` may hold pending tags.
- `BinomialHeap`: a root list linked through `sib` with increasing degrees, each tree of degree `d` holding `2^d` nodes; the handle is the head of the list. `meld` merges the lists by degree and links equal degrees (CLRS), O(log n); `top` scans the O(log n) roots; `pop` removes the minimum root, reverses its child list into a root list and melds, O(log n); `push` O(log n), O(1) amortized over pushes only.
- `PairingHeap`: children are linked through `child`/`sib`, and `prev` is the left sibling or, for a first child, the parent. `meld` and `push` O(1); `pop` pairs the children left to right and folds the pairs right to left (the member scratch `buf` is reused), O(log n) amortized; `decreaseKey(v, x, k)` cuts `x` and melds it back, O(log n) amortized (the best known bound is lower); `erase(v, x)` cuts `x` and melds the pairing of its children back, O(log n) amortized.
- `FibonacciHeap`: siblings and roots form circular lists through `lft`/`rgt`, and the handle is a minimum root. `meld` splices the root lists, O(1). `pop` moves the children to the root list and consolidates with a 64-entry degree table: the maximum degree is at most `log_φ n < 45` for `n < 2^31`. It is O(log n) amortized. `decreaseKey` cuts `x` when it beats its parent, then cascades through marked ancestors, O(1) amortized. `erase` cuts `x` to the roots, makes it the handle and pops, O(log n) amortized.
- Handles (pairing, Fibonacci): the id returned by `push` stays a handle while the element is in the heap, including after melds; `decreaseKey` and `erase` take the root of the heap that holds it. A popped or erased handle, or a handle in another heap, is an unchecked precondition, since checking it costs O(n). `decreaseKey` asserts the new key is not C-greater. Sizes are kept only at the handle, and every root change rewrites them.

### DoubleEndedPriorityQueue<T, C> (interval heap)

- Node `k` holds the interval `[a[2k], a[2k + 1]]`, nested in its parent's; a single last slot is both ends of its node. `push` appends, orders the last node's pair and sifts the new element up the min or max side; `popMin`/`popMax` move the last element into the root slot and sift it down that side, reordering each pair it lands in. O(log n) each, `min`/`max`/`size`/`empty` O(1). Construction from a vector sifts every node from the last to the root, O(n).

### PrioritySum<T, C>

- `in` holds the `min(k, size)` C-greatest values (its top is the C-smallest kept value, `boundary()`), and `out` holds the others; both are `ErasableHeap`s. Invariant: `in` has `min(k, size)` values and no kept value is C-less than an unkept one. `insert` pushes into `in` and moves the smallest kept value out when `in` exceeds `k`. `erase(x)` (a present copy required) goes to `in` when `x` is not C-less than `boundary()`, where a copy then always exists, and otherwise to `out`; both then rebalance. `setK(k)` moves `|Δk|` values. `insert` and `erase` take their argument by value, so `erase(boundary())` and `insert(boundary())` are valid. `sum()` and `sumRest()` are the sums of the kept and the other values; `T` needs `+=`, `-=` and `T()` as zero, and every partial sum must be representable. Each operation is O(log n) amortized per moved value; `k` asserts nonnegative (clamped to 0 under `NDEBUG`).
- `std::greater<T>` keeps the `k` smallest values. Running median: `setK((size() + 1) / 2)` then `boundary()` is `sorted[size() / 2]`, the upper median for even sizes; sums of absolute deviations follow from `sum()` and `sumRest()`.

### Removed planned names

| Planned | Replacement (decided 2026-10-13) |
|---|---|
| `BinaryHeap` as a separate implementation | alias `BinaryHeap<T, C> = DAryHeap<T, 2, C>`; `erase` lives in `DAryHeap` |
| `RingDeque` | `std::deque` (same operations and bounds) |
| `MinMaxQueue` | `SWAGQueue` (row 30) over a (min, max) monoid |
| `TwoHeapMedian` | `PrioritySum` with `setK((size() + 1) / 2)` and `boundary()` |

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/25-heap_deque_tester.cpp`, entry `25-heap_deque_tester.py` (driver `_00_runner.py`; three builds; 31 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| DAryHeap `push`, `pop`, `top`, `key`, `decreaseKey` (including an equal key), `erase`, `contains`, `size`, `empty`, construction from keys; D = 2 (`BinaryHeap`), 3, 4, 8; `less` and `greater`; spans 2 and 10^6 | `daryRandom` | Key array with presence flags and a `std::multiset` of present keys under `C` |
| ErasableHeap `push`, `erase`, `top`, `pop`, `size`, `empty`, construction; heavy duplicates | `erasableRandom` | `std::multiset` under `C` |
| Leftist, skew, randomized, binomial, pairing, Fibonacci `push`, `pop`, `top`, `meld`, `size`; leftist and skew `applyAdd`; `greater` orders; `pair` keys (no `ADD`) | `meldPlain` (four heaps per pool, then drained) | `std::multiset` per heap; shifted copies for `applyAdd` |
| Pairing and Fibonacci `decreaseKey`, `erase` by handle across melds | `meldHandles` | Map from live handle to key per heap; the root must be a live minimum before each `pop` |
| Fibonacci cascading cuts under many decreases | `fibonacciCascade` | Live-handle table |
| Leftist ranks and subtree sizes, binomial degrees and tree sizes, Fibonacci distinct root degrees and degree bound | `shapes` | Traversals of the pools |
| DoubleEndedPriorityQueue `push`, `popMin`, `popMax`, `min`, `max`, `size`, `empty`, construction; `less` and `greater` | `depqRandom` | `std::multiset`; interval nesting checked every 50 steps |
| PrioritySum `insert`, `erase`, `setK` (0 to size + 2), `sum`, `sumRest`, `boundary`, `size`; largest and smallest | `prioritySumRandom` | Sorted copy and prefix sums |
| Running median through `PrioritySum` | `median` | Sorted copy |
| Empty structures, single elements, duplicates, `pair` keys, copy independence, randomized-heap seed reproducibility, arguments referencing the structure (`erase(boundary())`, `insert(boundary())`, `applyAdd(v, top(v))`, `erase(top())`, `push(min())`) | `edges` | Direct values |
| Preconditions | `invalid` | Every empty `top`/`pop`, present or out-of-range ids, absent ids (`key`, `decreaseKey`, `erase`), C-greater decreases, null handles, negative `k` |

Mutation check (2026-10-13, quick mode, temporary copies): 11 of 11 injected faults detected. They covered a larger child chosen in `down`, `erase` without `up`, an inverted `clean`, a leftist meld ignoring ranks, a missing skew size update, three equal binomial degrees linked wrongly, a pairing `detach` keeping `prev`, a Fibonacci decrease keeping the old minimum, an interval pair left unordered, `PrioritySum` routing a boundary tie to `out` and a missing randomized size update.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/25-heap_deque_tester.py' --mode quick --seed 1
python3 '96-Local Testing/02-Data Structures/25-heap_deque_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/25-heap_deque_tester.py' --mode full --seed 1
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/25-' --no-integration
python3 '96-Local Testing/02-Data Structures/25-heap_deque_benchmark.py' --runs 5
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/25-heap_deque.hpp' '96-Local Testing/02-Data Structures/25-heap_deque_tester.cpp' '96-Local Testing/02-Data Structures/25-heap_deque_benchmark.cpp'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
```

Latest run 2026-10-13 (after the review fixes and the pair layout): quick mode passed as the pre-flight of every run (peak about 525 MB); full mode seed 1 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 7,415,440 checks per configuration and 31 assertion probes, MEMORY peak 696 MB and 724 MB; stress (`--seed 7`, one round, 120,000 random steps per group) passed with 14,662,383 checks per configuration, peak 695 MB; the benchmark below peaked at 334 MB. `--braces` reported no violations and `03-consistency.py` no errors; `python3 '96-Local Testing/02-integration.py' --sanitizers` passed on 2026-10-14 (137 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, sanitizer self-tests; peak 2757 MB). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14.7, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds; MEMORY peaks are dominated by compilation.

## Benchmarks

Driver `96-Local Testing/02-Data Structures/25-heap_deque_benchmark.py` (C++ `25-heap_deque_benchmark.cpp`; raw log `25-heap_deque_benchmark.json`, git-ignored). Run on 2026-10-13: `--runs 5`, seed 20261013, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, Intel Core i9-11900H, Linux x86-64. Other sessions kept the load average near 3 on 16 threads, so treat the medians as indicative. Checksums (pop order, distances) agreed across implementations in every workload. Workloads: `pushpop` pushes n uniform keys from `[0, 10^12)` and pops them all; `meld` builds n/16 heaps of 16 keys, melds them pairwise in rounds and drains the result; `dijkstra` runs from vertex 0 on a random directed graph with weights in `[1, 10^9]`. The indexed heaps use `decreaseKey`, against `std::priority_queue` with lazy deletion. At n = 1000 every implementation takes at most 0.3 ms. Medians in ms:

| Implementation | pushpop 10^5 | pushpop 10^6 | meld 10^5 | meld 10^6 | dijkstra n = 2·10^5, m = 10^6 | dijkstra n = 2·10^5, m = 4·10^6 |
|---|---|---|---|---|---|---|
| `std::priority_queue` (reference) | 6.8 | 130 | — | — | 100 (lazy) | 489 (lazy) |
| `BinaryHeap` | 11.2 | 298 | — | — | 58 | 100 |
| `DAryHeap<4>` (default) | 12.5 | 207 | — | — | 59 | 102 |
| `DAryHeap<8>` | 13.2 | 207 | — | — | 59 | 102 |
| `DoubleEndedPriorityQueue` (`popMin`) | 8.8 | 175 | — | — | — | — |
| `PairingHeap` | 18.9 | 515 | 16.3 | 515 | 102 | 186 |
| `SkewHeap` | 31.4 | 806 | 21.6 | 662 | — | — |
| `RandomizedHeap` | 33.5 | 744 | 25.0 | 617 | — | — |
| `FibonacciHeap` | 43.7 | 881 | 42.8 | 901 | 148 | 229 |
| `LeftistHeap` | 46.5 | 1070 | 35.2 | 993 | — | — |
| `BinomialHeap` | 54.9 | 1176 | 50.4 | 1180 | — | — |

Readings:
- `D = 4` is the default. It ties `D = 2` on Dijkstra and is 1.44 times faster on large push/pop, where each sift-down level reads one cache line of four pairs.
- The indexed heaps carry ids and `pos` updates, so plain push/pop is 1.6 times slower than `std::priority_queue` at 10^6. With `decreaseKey`, Dijkstra is 1.7 to 4.8 times faster than the lazy `std::priority_queue`, because the heap never holds stale entries.
- Among meldable heaps, pairing is the fastest on every workload. Fibonacci loses to pairing and to the d-ary heaps despite its O(1) amortized `decreaseKey`, as the folklore says. Leftist and binomial heaps trade speed for worst-case bounds.

## Sources

| Source | Actual reading and use |
|---|---|
| OI Wiki heap overview, binary, leftist and pairing heaps; cp-algorithms randomized heap | Operation bounds table, Floyd build, leftist ranks and lazy tags, two-pass pairing, the randomized meld. |
| ei1333 skew, leftist, Fibonacci and erasable heaps, priority-sum structure; Nyaan skew heap; maspypy meldable heap, removable queue, `slide_split_sum`; hitonanode Fibonacci heap; noshi91 pairing heap | Pool-based meldable interfaces, lazy add, handle erase, lazy-deletion heap, top-k sums in both directions. |
| suisen interval heap, binary heap, double-ended priority queue, priority sum; Library Checker `double_ended_priority_queue` | Interval-heap layout and the initial-array build. |

Fetched 2026-10-13 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-13-p029-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p029-omissions): `decreaseKey` on binomial and leftist heaps, handle `erase` on the other meldable heaps, pool `clear`, sorted dumps, multiply tags, push-or-decrease.
- The file name keeps its planned stem although the deque structures left the row (renaming is a separately scoped task, `02-structure.md`).
- Row 31 owns radix heaps, row 45 kinetic heaps, row 48 persistent leftist heaps; Graphs Dijkstra and Miscellaneous optimal merge consume these heaps.

## History

- 2026-10-13: P029 first verification. The independent review led to fixes, with regression tests, for argument aliasing in `PrioritySum` and `applyAdd` and for `NDEBUG` writes on absent `DAryHeap` ids. After the benchmark, `DAryHeap` moved to a `(key, id)` pair layout with a `key(i)` accessor.
