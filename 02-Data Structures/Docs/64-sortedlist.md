# 64-sortedlist.hpp — Evidence

Package P227 (batch DS49). New header; no legacy excerpt. A C++ bucket list in the style of tatyam's `SortedMultiset` and Grant Jenks' `sortedcontainers`, with split, merge and size-driven rebuilds. Each operation asserts its preconditions once.

## Contracts

### SortedList<T, C = std::less<T>>

A sorted multiset stored as a vector of nonempty sorted buckets. `C` is a strict weak order, and elements are equivalent when neither precedes the other. Equivalent elements keep insertion order, since `insert` places after existing equivalents and both builds sort stably; `eraseOne` removes the earliest.

- Construction `SortedList(v, cmp)` and `rebuild(v)` take O(n) when `v` is already sorted (checked with `is_sorted`) and O(n log(n + 1)) otherwise. `rebuild()` re-chunks the current contents in O(n); `clear()` empties.
- Bucket policy: the field `load`, written `B` here, is `max(BMIN, floor(sqrt(RATIO * built)))` with `BMIN = 32`, `RATIO = 4` and `built` the size at the last rebuild. A bucket above `2B` splits in halves. After removals, a bucket below `B / 2` merges with a neighbour, splitting again if the merge exceeds `2B`, and an emptied bucket is dropped. A rebuild runs when `n > 2 * built + BMIN` or `2 * n < built`. Invariant, checked by the tester after every operation: every bucket holds `[B / 2, 2B]` elements unless there is only one, and `built / 2 <= n <= 2 * built + BMIN`.
- Costs for current size `n`: the invariant gives O(n / B + 1) = O(sqrt(n)) buckets of O(B) = O(sqrt(n)) elements. `lowerBound`, `upperBound`, `prev` and `contains` are O(log(n)) (binary search over bucket maxima, then in the bucket). `rank`, `upperRank`, `count` and `kth` are O(sqrt(n)) (a scan of bucket sizes). `insert`, `eraseOne`, `eraseKth`, `popFront` and `popBack` are O(sqrt(n)) amortized, covering rebuilds, each O(n) and at least `built / 2` operations apart. `erase(x)` removes all `k` equivalents in O(sqrt(n) + k) amortized. `front`, `back`, `size` and `empty` are O(1). Memory is O(n).
- `insert(x)` adds; `erase(x)` returns the number removed; `eraseOne(x)` returns whether one was removed; `eraseKth(k)`, `popFront()` and `popBack()` return the removed element.
- `rank(x)` counts elements `< x`, `upperRank(x)` elements `<= x`, and `count(x)` equivalents; `contains(x)` tests membership; `position(it)` is an iterator's index (`n` for `end()`), O(sqrt(n)).
- Aliasing: arguments may refer to elements of the list. `erase` takes its value by copy, because it keeps comparing after destroying a run that ends at a bucket boundary; `insert` relies on `vector::insert`, which handles an aliased value; `eraseOne` does not read its argument after erasing.
- `kth(k)` and `eraseKth(k)` need `k` in `[0, n)`; `front`, `back`, `popFront`, `popBack` need `n > 0`.
- Searches return const bidirectional iterators, `end()` when absent: `lowerBound(x)` the first element `>= x`, `upperBound(x)` the first `> x`, and `prev(x)` the last `<= x` (the inclusive predecessor, as `OrderedMultiSet::prev` in row 07; the inclusive successor is `lowerBound`). `rbegin`/`rend` are `std::reverse_iterator`s.
- Every mutation, `insert` included since it may split or rebuild, invalidates all iterators and element references. Copies are independent.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `next(x)` | `lowerBound(x)` |
| `operator[](k)` | `kth(k)` |

### Correctness and Cost

Buckets are globally sorted. Insertion goes to the first bucket whose maximum is greater than `x` (or the last bucket), at the upper bound inside it; earlier buckets hold only elements `<= x` and later ones only elements `> x`, so the order is kept and every existing equivalent of `x` precedes the new element (stability). `lowerBound` takes the first bucket whose maximum is `>= x`, since every earlier bucket lies entirely below `x`. `erase(x)` starts at that bucket and moves on only while the current bucket's equivalents reach its end; the interior buckets of the run empty and are removed in one pass, and only the two partial end buckets can fall below `B / 2`, so two `fix` calls restore the invariant. `fix` loops because a merge with an equally small neighbour may still be below `B / 2`; each iteration removes a bucket, so it terminates.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/64-sortedlist_tester.cpp`, entry `64-sortedlist_tester.py` (driver `_00_contest_runner.py`; three builds; 7 assertion probes). The oracle is a vector kept sorted by stable insertion at `upper_bound`, independent of the bucket logic; the structural checker runs after every operation.

| Operation | Group | Oracle |
|---|---|---|
| `insert`, `eraseOne`, `erase` (count), `eraseKth`, `popFront`, `popBack` (returned values), `rebuild()`, `rebuild(v)` sorted and unsorted, `clear`, vector construction, `size`, `empty` | `randomOps` on five workloads: values in [0, 8] up to 300 elements, [0, 1000] up to 3000, `std::greater` in [0, 50] up to 2000, stable pairs compared by first up to 400, full-range `lng` up to 20000; alternating growth and shrink phases cross every rebuild, split and merge threshold | Sorted vector, stable |
| `kth`, `position`, `rank`, `upperRank`, `count`, `contains`, `lowerBound`, `upperBound`, `prev` (positions and dereferenced values), `front`, `back`, `--end()`, forward and reverse iteration | `verify` (every step on the small workload, every 5–400 steps on larger ones, always at the end; every `k` up to 64 elements, 65 evenly spaced `k` otherwise; probes below, between, on and above the values) | `lower_bound`/`upper_bound` on the oracle |
| Empty and singleton lists, last-bucket removal, 40000 ascending then descending inserts, 80000 equal values, `erase` of a run spanning many buckets, an equal run surrounded by other values, draining by `popFront`, `std::greater` construction, stability with `ByFirst`, string elements (moves out of buckets), arguments aliasing stored elements (`erase(front())` over a run ending at a bucket boundary, `insert(back())`, `eraseOne(kth(0))`), `LLONG_MIN`/`LLONG_MAX`, copy independence | `edgeCases` | Sorted vectors built independently |
| Bucket invariant `[B/2, 2B]`, global order, size bookkeeping, rebuild window, `B` formula | `structure` after every operation | Direct check of the stated invariant |
| Preconditions | `invalid` | `kth(-1)`, `kth(n)`, `eraseKth(n)`, `front`, `back`, `popFront`, `popBack` on an empty list |

A hand mutation check on 2026-10-09 (quick mode, temporary copies of the pre-review header) detected 12 of 13 injected faults (split threshold, no merge, single-bucket `erase`, lower-bucket insert, `prev` via `lowerBound`, unstable sort, no growth rebuild, `kth` scan bound, merge index at the last bucket, missing second `fix`, lower-bound insert position, no shrink rebuild). The survivor (`erase` compacting from `b + 1` instead of `b`) is equivalent: `fix(b)` drops an empty bucket at `b`.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 1079 MB; optimized 0.1 + 8.3 s, checked 36.4 + 58.5 s, ASan-UBSan 35.8 + 106.5 s; MEMORY peak 1540 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 1142 MB; optimized 0.3 + 13.8 s, checked 65.8 + 85.6 s, ASan-UBSan 57.2 + 125.3 s; MEMORY peak 1498 MB |
| `01-run.py --mode quick` | Optimized 40.1 + 1.8 s, checked 32.4 + 27.0 s; MEMORY peak 1090 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 58 MB; optimized 0.1 + 8.5 s, checked 21.9 + 52.2 s, ASan-UBSan 19.5 + 113.8 s; MEMORY peak 892 MB |

Latest per-header run 2026-10-13 (P227 prune) on the same machine with GCC 16.2.1 20260810, GCC 14.4.1 20260915 and Python 3.14.7, in the optimized, checked and ASan/UBSan builds; MEMORY peaks are dominated by the compiler.

| Command (2026-10-13) | Result |
|---|---|
| `64-sortedlist_tester.py --mode full --seed 20261013` (GCC 16.2.1) | PASS, 13,028,822 checks per configuration, 7 assertion probes, MEMORY peak 778 MB |
| `CXX=g++-14 64-sortedlist_tester.py --mode full --seed 20261013` (GCC 14.4.1) | PASS, 13,028,822 checks per configuration, 7 assertion probes, MEMORY peak 781 MB |
| `02-integration.py` | PASS: 135 standalone and aggregate headers, scalar and available AVX2 multiple-translation-unit builds, workspace |
| `03-consistency.py`, `03-consistency.py --braces` | No errors; no closing-brace, comment-cap or free-function violations |

```bash
python3 '96-Local Testing/02-Data Structures/64-sortedlist_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/64-sortedlist_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/64-sortedlist.hpp' '96-Local Testing/02-Data Structures/64-sortedlist_tester.cpp'
```

## Benchmarks

`64-sortedlist_benchmark.py --runs 5` (2026-10-09): seed 20261009, GCC 16.2.1 `-O2 -DNDEBUG`, i9-11900H. The driver rebuilds the benchmark with header copies whose `BMIN`/`RATIO` are substituted and compares a PBDS order-statistics tree over `(value, id)`. Mixed: n random inserts of values in [0, 4n), then 1e6 operations (25% `insert`, 25% `eraseOne`, 20% `rank`, 20% `kth`, 10% `lowerBound`). Front-heavy: 1e6 ascending inserts, then 1e6 `popFront`. Checksums agree across variants and runs. Medians of 5 runs in seconds; MEMORY peak 332 MB. The run predates the review fixes, which touched only `erase(x)` (now by value), a field name and the rank helper; of these only `rank` is timed, and its loop is unchanged.

| Variant | mixed n=1e3 | mixed n=1e5 | mixed n=1e6 | popFront 1e6 |
|---|---|---|---|---|
| BMIN=32 RATIO=1 | 0.089 | 0.154 | 0.448 | 0.069 |
| BMIN=32 RATIO=2 | 0.088 | 0.166 | 0.525 | 0.096 |
| **BMIN=32 RATIO=4 (library)** | **0.084** | **0.143** | **0.501** | **0.109** |
| BMIN=32 RATIO=8 | 0.088 | 0.158 | 0.582 | 0.139 |
| BMIN=32 RATIO=16 | 0.090 | 0.152 | 0.625 | 0.184 |
| BMIN=32 RATIO=32 | 0.094 | 0.166 | 0.741 | 0.254 |
| BMIN=32 RATIO=64 | 0.098 | 0.188 | 0.933 | 0.435 |
| BMIN=16 / 64 / 128, RATIO=8 | 0.088 / 0.088 / 0.087 | 0.156 / 0.148 / 0.146 | 0.527 / 0.527 / 0.525 | 0.143 / 0.139 / 0.139 |
| PBDS tree | 0.154 | 0.380 | 1.615 | — |

Threshold choice: `RATIO = 4` is the fastest at n = 1e3 and 1e5 and within 12% of the best (RATIO 1) at 1e6; a preliminary 3-run sweep had RATIO 2–4 ahead at 1e6, so the 1e6 ordering of RATIO 1–4 is run-to-run noise. Larger ratios lose steadily because the in-bucket shift dominates the bucket scan. `BMIN` matters only below a few thousand elements and is flat between 16 and 128. SortedList is 1.8–3.2 times faster than the PBDS tree on this workload.

## Sources

| Source | Actual reading and use |
|---|---|
| tatyam `SortedMultiset.py` | Operation set (`pop(i)`, `__contains__`, `lt`/`le`/`gt`/`ge`), bucket-ratio policy without merging. |
| PyRival `SortedList.py` | Fenwick over block sizes (not adopted, see notes), block size 700. |
| sortedcontainers SortedList and implementation notes | Split above 2x load and merge below 1/2 load: the policy adopted here, with the load tied to `sqrt(n)`. |
| OI Wiki block list | Split above 2·sqrt(n), O(sqrt(n)) operations. |

References inspected 2026-10-09 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and Handoffs

- Row 64 is partial: `SortedList::eraseRange`, `SortedList::insertAll` and `SortedList::operator==` are missing, returned by P275.
- Left out with reasons in [00-notes.md](00-notes.md#p227-omissions): `countRange`, slices, negative indices, bucket-size Fenwick.
- For O(log(n)) worst-case updates use `OrderedMultiSet` (row 07); the benchmark shows SortedList 1.8–3.2 times faster on the mixed workload up to 1e6 elements.
- The Python mirror is planned as `08-Python/_31_ordered_multiset.py` (row status `planned`).

## Aliases

Search aliases (P263, grab --find): 有序列表, 有序容器, 有序多重集, 块状链表.

## History

- 2026-10-09: P227 first verification; full on g++ and g++-14, stress (seed 7) on g++ and integration on both passed; `BMIN`/`RATIO` benchmark sweep; the review's 5 findings fixed.
- 2026-10-13: prune removed `next(x)` and `operator[](k)`, whose checks already ran on `lowerBound` and `kth` against the same oracle; full passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
