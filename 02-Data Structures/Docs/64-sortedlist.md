# 64-sortedlist.hpp — evidence

Package P227 (batch DS49). New header; no legacy excerpt. A C++ bucket list in the style of tatyam's `SortedMultiset` and Grant Jenks' `sortedcontainers`, with split, merge and size-driven rebuilds. Preconditions are `assert`s checked once per operation.

## Contracts

### SortedList<T, C = std::less<T>>

A sorted multiset stored as a vector of nonempty sorted buckets. `C` is a strict weak order; elements are equivalent when neither precedes the other. Equivalent elements keep insertion order (`insert` places after existing equivalents and both builds use a stable sort), and `eraseOne` removes the earliest of them.

- Construction `SortedList(v, cmp)` and `rebuild(v)`: O(n) when `v` is already sorted (checked with `is_sorted`), O(n log(n + 1)) otherwise. `rebuild()` re-chunks the current contents in O(n); `clear()` empties.
- Bucket policy: the field `load` (written `B` below) is `max(BMIN, floor(sqrt(RATIO * built)))` with `BMIN = 32`, `RATIO = 4`, where `built` is the size at the last rebuild. A bucket above `2B` splits in halves; after removals a bucket below `B / 2` merges with a neighbour (and splits again if the merge exceeds `2B`); an emptied bucket is dropped. A rebuild runs when `n > 2 * built + BMIN` or `2 * n < built`. Invariant (checked by the tester after every operation): every bucket holds `[B / 2, 2B]` elements unless there is only one, `built / 2 <= n <= 2 * built + BMIN`.
- Costs, with `n` the current size: the invariant gives O(n / B + 1) = O(sqrt(n)) buckets of O(B) = O(sqrt(n)) elements. `lowerBound`, `upperBound`, `prev`, `next`, `contains` are O(log(n)) (binary search over bucket maxima, then in the bucket). `rank`, `upperRank`, `count`, `kth`, `operator[]` are O(sqrt(n)) (scan of bucket sizes). `insert`, `eraseOne`, `eraseKth`, `popFront`, `popBack` are O(sqrt(n)) amortized; the amortization covers rebuilds, each O(n) and separated by at least `built / 2` operations. `erase(x)` removes all `k` equivalents in O(sqrt(n) + k) amortized. `front`, `back`, `size`, `empty` are O(1). `M` is O(n).
- `insert(x)`; `erase(x)` returns the number removed; `eraseOne(x)` returns whether one was removed; `eraseKth(k)`, `popFront()`, `popBack()` return the removed element.
- `rank(x)` counts elements `< x`, `upperRank(x)` elements `<= x`, `count(x)` equivalents, `contains(x)`; `position(it)` is the index of an iterator (`n` for `end()`), O(sqrt(n)).
- Aliasing: arguments may refer to elements of the list. `erase` takes its value by copy because it keeps comparing after destroying a run that ends at a bucket boundary; `insert` relies on `vector::insert`, which handles an aliased value, and `eraseOne` does not read its argument after erasing.
- `kth(k)` and `operator[](k)` need `k` in `[0, n)`; `eraseKth` too; `front`, `back`, `popFront`, `popBack` need `n > 0`.
- Searches return const bidirectional iterators: `lowerBound(x)` first element `>= x`, `upperBound(x)` first `> x`, `prev(x)` last `<= x`, `next(x)` first `>= x` (the same as `lowerBound`; `prev`/`next` are the inclusive nearest neighbours, matching `OrderedMultiSet::prev` in row 07). Absent results are `end()`. `rbegin`/`rend` are `std::reverse_iterator`s.
- Invalidation: every mutation (including `insert`, which may split or rebuild) invalidates all iterators and element references. Copies are independent.

## Correctness and cost

Buckets are globally sorted: insertion goes to the first bucket whose maximum is greater than `x` (or the last bucket), at the upper bound inside it. Earlier buckets hold only elements `<= x` and later buckets only elements `> x`, so the order is kept and every existing equivalent of `x` precedes the new element, which is the stability guarantee. `lowerBound` takes the first bucket whose maximum is `>= x`; every earlier bucket lies entirely below `x`. `erase(x)` starts at that bucket and continues to the next one only while the current bucket's equivalents reach its end; the interior buckets of the run become empty and are removed in one pass, and only the two partial end buckets can fall below `B / 2`, so two `fix` calls restore the invariant. `fix` loops because a merge with an equally small neighbour may still be below `B / 2`; each iteration removes a bucket, so it terminates.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/64-sortedlist_tester.cpp`, entry `64-sortedlist_tester.py` (driver `_00_runner.py`; three builds; 7 assertion probes). The oracle is a vector kept sorted by stable insertion at `upper_bound`, independent of the bucket logic; the structural checker runs after every operation.

| Operation | Group | Oracle |
|---|---|---|
| `insert`, `eraseOne`, `erase` (count), `eraseKth`, `popFront`, `popBack` (returned values), `rebuild()`, `rebuild(v)` sorted and unsorted, `clear`, vector construction, `size`, `empty` | `randomOps` on five workloads: values in [0, 8] up to 300 elements, [0, 1000] up to 3000, `std::greater` in [0, 50] up to 2000, stable pairs compared by first up to 400, full-range `lng` up to 20000; alternating growth and shrink phases cross every rebuild, split and merge threshold | Sorted vector, stable |
| `kth`, `operator[]`, `position`, `rank`, `upperRank`, `count`, `contains`, `lowerBound`, `upperBound`, `prev`, `next` (positions and dereferenced values), `front`, `back`, `--end()`, forward and reverse iteration | `verify` (every step on the small workload, every 5–400 steps on larger ones, always at the end; every `k` up to 64 elements, 65 evenly spaced `k` otherwise; probes below, between, on and above the values) | `lower_bound`/`upper_bound` on the oracle |
| Empty and singleton lists, last-bucket removal, 40000 ascending then descending inserts, 80000 equal values, `erase` of a run spanning many buckets, an equal run surrounded by other values, draining by `popFront`, `std::greater` construction, stability with `ByFirst`, string elements (moves out of buckets), arguments aliasing stored elements (`erase(front())` over a run ending at a bucket boundary, `insert(back())`, `eraseOne(kth(0))`; caught by ASan before the fix), `LLONG_MIN`/`LLONG_MAX`, copy independence | `edgeCases` | Sorted vectors built independently |
| Bucket invariant `[B/2, 2B]`, global order, size bookkeeping, rebuild window, `B` formula | `structure` after every operation | Direct check of the stated invariant |
| Preconditions | `invalid` | `kth(-1)`, `kth(n)`, `eraseKth(n)`, `front`, `back`, `popFront`, `popBack` on an empty list |

Mutation check (2026-10-09, quick mode, temporary copies of the pre-review header): 12 of 13 injected faults detected (split threshold, no merge, single-bucket `erase`, lower-bucket insert, `prev` via `lowerBound`, unstable sort, no growth rebuild, `kth` scan bound, merge index at the last bucket, missing second `fix`, lower-bound insert position, no shrink rebuild). The survivor (`erase` compacting from `b + 1` instead of `b`) is equivalent: `fix(b)` drops an empty bucket at `b`.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/64-sortedlist_tester.py' --mode full --seed 20261009
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/64-sortedlist_tester.py' --mode full --seed 20261009
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/6' --no-integration
python3 '96-Local Testing/02-Data Structures/64-sortedlist_benchmark.py' --runs 5
python3 '96-Local Testing/02-integration.py'
CXX=g++-14 python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-09: full mode seed 20261009 passed on both compilers, 16,640,685 checks per configuration and 7 assertion probes (MEMORY peak 814 MB on g++, 847 MB on g++-14, both dominated by the compiler); stress mode seed 7 passed all three configurations with 33,008,506 checks per configuration (peak at most 835 MB). The checked build takes about 71 s in full and 105 s in stress, inside the runner's 180 s limit. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14, `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

`02-integration.py` passed on GCC 16.2.1 and 14.4.1: 114 standalone/aggregate headers (this one alone, in `99-all.hpp`), the scalar and AVX2 two-translation-unit builds and the workspace. Its `--sanitizers` stage stopped at a link error in another session's uncommitted `01-Core/16-poly_tester.cpp`, unrelated to this header, whose own ASan/UBSan configuration passed above. `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header, its tester and its benchmark.

## Benchmark

`64-sortedlist_benchmark.py --runs 5`, seed 20261009, GCC 16.2.1 `-O2 -DNDEBUG`, i9-11900H. The driver rebuilds the benchmark with header copies whose `BMIN`/`RATIO` are substituted, and compares a PBDS order-statistics tree over `(value, id)`. Mixed: n random inserts of values in [0, 4n), then 1e6 operations (25% `insert`, 25% `eraseOne`, 20% `rank`, 20% `kth`, 10% `lowerBound`). Front-heavy: 1e6 ascending inserts then 1e6 `popFront`. Checksums agree across variants and runs. Medians of 5 runs in seconds (MEMORY peak 332 MB), measured before the review fixes, which touch only `erase(x)` (now by value), a field name and the rank helper, none of them on the timed paths except `rank`, whose loop is unchanged.

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

Threshold choice: `RATIO = 4` is the fastest at n = 1e3 and 1e5 and within 12% of the best (RATIO 1) at 1e6; a preliminary 3-run sweep had RATIO 2–4 ahead at 1e6, so the 1e6 ordering of RATIO 1–4 is within run-to-run noise. Larger ratios lose steadily because the in-bucket shift dominates the bucket scan. `BMIN` matters only below a few thousand elements and is flat between 16 and 128. SortedList is 1.8–3.2 times faster than the PBDS tree on this workload.

## Sources

| Source | Actual reading and use |
|---|---|
| tatyam `SortedMultiset.py` | Operation set (`pop(i)`, `__contains__`, `lt`/`le`/`gt`/`ge`), bucket-ratio policy without merging. |
| PyRival `SortedList.py` | Fenwick over block sizes (not adopted, see notes), block size 700. |
| sortedcontainers SortedList and implementation notes | Split above 2x load and merge below 1/2 load: the policy adopted here, with the load tied to `sqrt(n)`. |
| OI Wiki block list | Split above 2·sqrt(n), O(sqrt(n)) operations. |

References inspected 2026-10-09 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Omissions with reasons in [00-notes.md](00-notes.md#p227-omissions): `countRange`, slices, negative indices, bucket-size Fenwick.
- For O(log(n)) worst-case updates use `OrderedMultiSet` (row 07); the benchmark shows SortedList 1.8–3.2 times faster on the mixed workload up to 1e6 elements.
- The Python mirror is planned as `08-Python/_31_ordered_multiset.py` (row status `planned`).

## History

- 2026-10-09: P227 first verification. Independent review: fixed `erase(x)` use-after-free when `x` aliases an element (now by value), renamed the runtime field `B` to `load`, merged the `rank`/`upperRank` prefix loops into `position`, corrected two evidence statements.
