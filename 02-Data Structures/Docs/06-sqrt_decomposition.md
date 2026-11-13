# 06-sqrt_decomposition.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SqrtDecomp

`M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract), kept as `m` and passed last as `mon = {}` (`SqrtDecomp(N = 0, B = 0, mon = {})`, `SqrtDecomp(vector<S>, B = 0, mon = {})`); left-to-right operand order is kept. `0 <= n <= INT_MAX`, zero-based points, half-open ranges, and an empty product returns `e()`. `B = 0` picks block width `width = max(1, floor(sqrt(n)))` (the field is `width` because `m` is the monoid); a positive `B` is the width. The size and vector constructors are `explicit`; the braced-list constructor is not, so a braced list always means values. `rebuild(a, B)` replaces every point and picks a fresh width in O(n), keeping the stored monoid, which also works when the monoid is not assignable (a capturing lambda through `Monoid`). `get` is O(1) (by value for `S = bool`, through `vector<S>::const_reference`), `values` copies in O(n), and `set`/`apply` rebuild one block in O(width). `apply(i, x)` sets `v[i] = op(v[i], x)`. `prod` is O(width + n / width). `pull(bi)` recomputes block `bi` from `v`. It is an internal helper without a bounds check and is not part of the inventory row. Public state must not be edited directly. Costs count `op` calls and `S` copies as O(1).

### SqrtRangeSum

T is a commutative ring with `T(0)`, `T(1)`, `T(length)`, `+`, `*` and `==`. Every intermediate, including pending tags, must fit T unless T is modular. Points, ranges and block widths match SqrtDecomp. `affine(l, r, a, b)` maps each x to `a * x + b`. New tags compose after pending ones: `(a, b)` after `(c, d)` is `(a * c, a * d + b)`. Assignment uses `a = 0` and needs no inverse. `add`, `assign`, `multiply` and `set` are affine special cases. `get` is O(1). `values` is O(n) and leaves tags intact. `apply`, `push` and `pull` are unchecked internal block helpers: `pull` requires the block's tags to be pushed already. They are not part of the row.

### SortedBlocks

`SortedBlocks<T>(a = {}, B = 0)`: T has `T(0)`, copies, `+` and a strict total order `<` that `+` preserves (`y < z` implies `y + x < z + x`, which the merge of shifted runs and the tagged `partition_point` rely on), nothing else (no `+=`, no default constructor). Every current value must fit T, and so must each block's pending add, the sum of the full-block adds since the block's last partial update (as for `SqrtRangeSum`'s tags): `{LLONG_MIN + 5}` with width 1 under `add(0, 1, LLONG_MAX)` then `add(0, 1, LLONG_MAX - 10)` keeps representable values but overflows the tag. Zero-based points, half-open ranges asserted, empty ranges allowed (counts 0, `maxLess` false). `B = 0` picks block width `k = max(1, floor(sqrt(n * log2(n + 1))))`, which balances `O(k)` partial work against `O((n / k) * log(k + 1))` full-block searches; a positive `B` is the width. No universally fastest width is claimed. Each block keeps the raw values `v`, the sorted indices `ord`, the sorted values `s = v[ord]`, a pending add `add_tag` and an assigned flag with `set_val`; a value is `set_val` in an assigned block and `v[i] + add_tag` otherwise. A full-block `add` adds to `set_val` or `add_tag` (O(1)); a full-block `assign` sets the flag (O(1)). A partial update materializes the block, changes `v[lo, hi)`, then `stable_partition`s `ord` into the unchanged and changed indices, each still sorted (an add shifts all changed values equally, an assignment makes them equal), and `inplace_merge`s them: O(k) instead of a sort. `countLess(l, r, x)` counts values `< x`, `countLeq` values `!(x < value)`; the derived counts `countGeq = (r - l) - countLess`, `countGreater = (r - l) - countLeq` and `countEqual = countLeq - countLess` take the same ranges and cost; a full block answers by `partition_point` over `s` with the pending add, or by one comparison when assigned. `maxLess(l, r, x, res)` returns false and leaves `res` unchanged when no value is below `x`, else true with the largest such value; it takes `x` by value, so `x` and `res` may be the same object. `get` is O(1), `values` O(n). `blocks`, `first`, `countBelow` and `reorder` are unchecked internal helpers, not part of the row.

### BlockDecomp

`BlockDecomp<Block, M>(N = 0, B = 0, proto = {}, mon = {})` runs a caller's block policy: `B = 0` picks width `k = max(1, floor(sqrt(n)))`. Each block is a copy of `proto` (so it may carry pointers to caller data) and then receives `build(l, r)` with its global range, in order. `apply(l, r, x)` calls `updateAll(x)` on each fully covered block and `updatePart(l', r', x)` with the global clipped range on at most two partial blocks, left to right; `x` is any type the policy accepts. `prod(l, r)` folds `queryAll()` or `queryPart(l', r')` results left to right with `m.op` from `m.e()` (monoid `M`, stored as `m`; noncommutative allowed). Ranges are asserted half-open, `N >= 0`, `B >= 0`; block ends are computed as `start + min(k, n - start)`, so any width up to `INT_MAX` is safe. Both methods are non-const so a policy may push tags during a query. Cost: O(n / k + 1) policy calls per operation plus what the policy charges; the policy owns its own invariants.

### BlockList

`BlockList<T>(a = {}, B = 0)`: a sequence in a vector of blocks. `insert(i, x)` (0 <= i <= n, n < INT_MAX) moves `x` into place, `erase(i)` (0 <= i < n) removes it, `at(i)` returns `vector<T>::reference` (or `const_reference`), so `BlockList<bool>` yields bit proxies, `size()` is O(1). The block shapes (public `blks`) are unspecified beyond these invariants (user decision 2026-11-12). Invariants after every operation: no block is empty, each holds at most `2 * k` elements, and each adjacent pair holds at least `k`, so there are at most `2 * n / k + 1` blocks. An insert splits an overfull block at `k`; an erase removes an emptied block, or merges the block with its left neighbour when their total drops below `k`, else with its right one: one merge restores the pair bound (the merged block exceeds the old left part by at least one element). With `B = 0` the width follows the size: `k = max(1, floor(sqrt(n)))`, and when n leaves `[k^2 / 4, 4 * k^2]` every block is rebuilt in O(n) with the new width, at least about `3 * n / 4` operations after the previous rebuild, so the amortized cost is O(sqrt(n)). A positive `B` fixes `k`, and operations then cost O(k + n / k). References from `at` are invalidated by any later `insert` or `erase`. T needs only to be movable (a split erases the moved tail instead of resizing, so no default constructor is needed). `reshape` steps by `min(k, n - i)`, so widths up to `INT_MAX` are safe.

### Correctness and cost

Every block loop of `SqrtDecomp`, `SqrtRangeSum`, `SortedBlocks` and `BlockDecomp` runs while `l < min(r, n)`, so an out-of-range `r` under `NDEBUG` ends the loop instead of spinning at `l = n` (`03-cpp.md`, Toolchain and judges); the result is then meaningless.

Blocks store ordered folds; queries concatenate full blocks and at most two tails. Generic point update rebuilds its block, O(B); query O(B+n/B). Scalar affine sum maps s to a*s+b*length and composes new tags after old tags. Partial mutations push old tags then rebuild; reads apply tags without mutation. Scalar range update/query O(B+n/B), point get O(1), point set O(B). Build/materialization O(n), storage O(n+1). Default sqrt block size balances B+n/B analytically; callers can tune B for their workloads. No universally fastest block width is claimed. Generic monoid point updates remain O(B); the scalar affine specialization covers the additional range operations. Public operations check their ranges once at entry.

### Legacy mapping

| Legacy | Replacement |
|---|---|
| `SqrtDecomp::setUpdate(i, x)` (removed by the 2026-10-13 prune) | `set(i, x)` |
| `SqrtDecomp::opeUpdate(i, x)`, `query(l, r)` (renamed 2026-10-17) | `apply(i, x)`, `prod(l, r)` |
| `SqrtDecomp` `(id, f)` arguments, default `std::plus` (renamed 2026-10-17) | one monoid such as `Sum<lng>` |
| `SqrtRangeSum::rebuild(a, B)` (removed by the 2026-10-13 prune) | `x = SqrtRangeSum<T>(a, B)`; its body is now the vector constructor, and the size constructor delegates to it |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`06-sqrt_decomposition_tester.cpp` group) | Oracle |
|---|---|---|
| `SqrtDecomp`: `get`, `values`, `set`, `apply`, `prod`, `rebuild` | generic and ordered groups | Ternary generic arrays through length 5 with default/1/2/n/n+2/INT_MAX widths over `Sum<lng>`; concatenation/reverse concatenation through the stateful monoid `Join{backwards}` (runtime flag) against an independent lambda, with string `set` |
| `SqrtRangeSum`: `affine`, `add`, `assign`, `multiply`, `sum`, `get`, `set`, `values` | scalar exhaustive/history groups | Every length-2 affine history on small ternary arrays and every half-open range; 100 histories of 200 operations |
| Tag order, constness, value semantics, types | regression groups | Assignment/add/multiply order, pending tags, read constness, copy/move/reconstruction (the removed `rebuild` cases, including reconstruction from `values()` with pending tags), 128-bit/modular/exact dyadic/alias cases |
| `S = bool`, construction | regression groups | Bool OR payload through `Monoid{false, any}` with `get` bound to `const bool &` for widths 0–3; braced singleton/list; constructors not implicitly convertible; 21 probes |
| `SortedBlocks`: construction, width, `get`, `values`, `add`, `assign`, `countLess`, `countLeq`, `countGeq`, `countGreater`, `countEqual`, `maxLess` | `sortedExhaustive`, `sortedRandom`, `sortedExtremes`, `sortedBare` | Linear scans over a plain vector after every operation: every array n <= 3 three steps deep and n = 4–5 two steps; 60 random trials per type (int64 small and wide, exact `double`, a lexicographic struct) of 300 operations, n <= 200, widths 0..n+2, `INT_MAX`; a `Bare` type with only `T(int)`, copies, `+` and `<` (80 histories); every count on every query, `countGeq`, `countGreater` and `countEqual` against scans of `!(a[i] < x)`, `x < a[i]` and equivalence (a header copy with an off-by-one `countEqual` failed at once), `Bare` also at x = ±1000 beyond every value; `maxLess(l, r, y, y)` aliasing on every query; the default width against a long-double formula |
| `BlockDecomp`: construction, `apply`, `prod` | `decompCallsWith`, `decompCalls` | A recording block checked call by call (order, kind, global ranges) for every (n <= 12, B, l, r) with concatenation through `Monoid` and `Join` both ways; N near `INT_MAX` with widths 2^30 ± 1 and `INT_MAX`; a range-add range-sum block reaching caller data through a pointer in `proto` against a vector; empty N |
| `BlockList`: `insert`, `erase`, `at` (both overloads), `size`, adaptive width; `BlockList<bool>` writes through the `at` proxy and reads through the const one for widths 0–2 | `listExhaustive`, `listRandom` | `std::vector` oracle with the block invariants (no empty block, at most `2 * k`, adjacent pairs at least `k`) checked after every operation; depth-4 exhaustive histories; 50 random trials over `int`, `std::string`, `std::unique_ptr<int>`; growth to 10^4 and back; widths through `INT_MAX` |
| Full-block paths | `costCounts` | A counting monoid, ring and ordered type over n = 10^4 with the default width: `SqrtDecomp::prod(0, n)` within n / k + 2k + 2 `op` calls, `SqrtRangeSum::sum(0, n)` within n / k + 4k + 4 additions, `SortedBlocks::countLess(0, n, x)` within (n / k + 1)(bit_width(k) + 2) + 2k + 4 comparisons, each besides its value (a scan of every element costs about 10^4) |
| New preconditions | probes | 30 `sorted-*`, `decomp-*` and `list-*` probes for out-of-range points and ranges, positions and negative widths, among them `sorted-countgeq-reversed`, `-end`, `sorted-countgreater-reversed`, `-end`, `sorted-countequal-reversed`, `-end`, `lazy-get-past-end` and `sorted-get-past-end` (index n + 1, where only the library's own assert prints glibc's form), and `generic-max-size` (`INT_MAX + 1` packed bools under the OR monoid, 256 MiB); 60 probes in all |

Quick: generic n<=3, lazy n<=2 depth 1, 20 trials. Stress: 400 trials. New groups, quick / full and stress: sorted exhaustive n <= 3 one step (n <= 4 put the checked quick run over its 10 s bound once the derived counts joined), then as above; sorted random 10 / 60 / 240 trials; decomposition calls n <= 7 / 12, random 20 / 100 / 400; list depth 3 / 4, random 10 / 50 / 200, growth 2000 / 10^4. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 787 MB; optimized 0.3 s compile + 4.0 s run, checked 18.2 s compile + 4.4 s run, ASan-UBSan 21.0 s compile + 32.9 s run; MEMORY peak 1000 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 865 MB; optimized 0.2 s compile + 5.8 s run, checked 24.9 s compile + 5.8 s run, ASan-UBSan 29.0 s compile + 19.3 s run; MEMORY peak 1076 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 27.9 s compile + 0.5 s run, checked 22.6 s compile + 1.7 s run; MEMORY peak 784 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 790 MB; optimized 0.1 s compile + 2.5 s run, checked 8.2 s compile + 3.3 s run, ASan-UBSan 8.9 s compile + 21.4 s run; MEMORY peak 541 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (P006 DS02; experiment: none): GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H, one suite at a time after the DS02 `/finish` mutation campaigns, beside other sessions, so times are indicative.

```bash
python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --mode full --filter '02-Data Structures/06-sqrt_decomposition.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter 06-sqrt_decomposition_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/06-sqrt_decomposition.hpp' '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.cpp'
```

| Command | Result |
|---|---|
| full, g++ | PASS, 3 configurations, 60 probes; run optimized 3.8 s, checked 28.5 s, ASan-UBSan 73.1 s; MEMORY peak 1973 MB |
| full, g++-14 | PASS; run 4.3 / 34.0 / 68.2 s; MEMORY peak 1917 MB |
| stress, seed 7 | PASS; run 5.7 / 46.3 / 144.5 s; MEMORY peak 839 MB |
| extraction ops | PASS: 32 operations in 25 extractions on g++ and g++-14, 0 failures; one template (`BlockDecomp`, whose `Block` is the caller's policy) has no candidate instantiation |
| extraction testers | PASS: 7 names, quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multiple-translation-unit builds, workspace; MEMORY peak 1600 MB |
| `03-consistency.py` | No errors |
| `--braces` | No closing-brace, comment-cap or free-function violations |

Quick runs take 0.3–0.6 s (optimized) and 5.1–9.7 s (checked) beside other sessions and mutation campaigns, 4.3 s checked alone.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 27 | Block decomposition. |
| [OI Wiki: 块状数组](https://oi-wiki.org/ds/block-array/) | Range add over sorted block copies, counts below or at most a value, predecessor below x: `SortedBlocks`. |
| [Nyaan SquareRootDecomposition](https://nyaannyaan.github.io/library/data-structure/square-root-decomposition.hpp), maspypy sqrt_decomposition | Caller block policy with whole and partial update and query: `BlockDecomp`. |
| [OI Wiki: 块状链表](https://oi-wiki.org/ds/block-list/) | Block linked list and the split of an overfull block: `BlockList`. |
| [ei1333 sqrt-decomposition](https://ei1333.github.io/library/structure/others/sqrt-decomposition.hpp) | Derived counts beside `countLess`: `countGeq`, `countGreater`, `countEqual`. |

References inspected on 2026-09-27; the DS02 rows on 2026-11-12 ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)).

## Mutation testing

Record: mewt 4.0.0 on blob `2d30f22296fc2e3c731e15973119e52ee10dc5a1` (2026-11-12, seed 20261112, 4 lines changed since blob `ce8997b8b9b1`): 29 mutants, 23 caught (0 by timeout), 6 survived, 0 untested.

The DS02 `/finish` campaign covered its fixes (the `BlockList::at` reference types, the bounded `locate` loop, the `SortedBlocks` comment): stage seconds quick 97, full on the optimized build 11, full on the checked and ASan-UBSan builds 343. It follows the whole-file campaign on blob `ce8997b8b9b1` (2026-11-12, uniform sample of 500 of 1700 mutants, 464 caught, 36 survived), whose survivors on unchanged lines stay in the table. The probe `generic-max-size` (`INT_MAX + 1` packed bools) closed that campaign's `COS 1120`, and the full-mode build of exactly `INT_MAX` packed bools under an OR monoid whose `op` throws (the assert passes, the block fold stops at once) its `COS 1121`; each was checked against the mutant applied by hand to a scratch copy.

Whole-file campaign stage seconds: first run quick 1579, full on the optimized build 110, full on the checked and ASan-UBSan builds 2779 (41 survivors); the survivor retest with the same workdir quick 1742, 179, 4410, beside other campaigns' sessions. Between them `costCounts` closed the three mutants that never take a full block and so cost O(n) per operation with the same values (`l == start` → `l < start` in `SqrtDecomp::prod` and `SqrtRangeSum::sum`, lines 35 and 91, and `(l == start && stop == end)` → `false` in `SortedBlocks::blocks`, line 165), and the probes `lazy-get-past-end` and `sorted-get-past-end` closed `i < n` → `!=` in both `get` asserts (lines 63, 132), where only the debug containers stopped the read.

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 1678`, `1679`, `1681`, `1685` `std::move(...)` removed | 15, 19, 200 | equivalent: a copy instead of a move, same values |
| `AS 358`, `359`, `388`, `392`, `432`, `451`, `455`, `460`, `461`, `473` (arguments of `min` or `max` swapped) | 33, 34, 89, 102, 163, 202, 209, 217, 218, 278 | equivalent: symmetric functions of `int`s |
| `COS 1205`, `1315`, `1395`, `1465`, `1470`, `1475`, `1495` `<` → `!=` | 43, 89, 135, 176, 182, 184, 188 | equivalent: each counter steps by 1 from at most its bound (`l` reaches `min(r, n)` exactly, as `stop <= min(r, n)`) |
| `COS 1276` `l == start` → `<=`; `COS 1367` `n % width != 0` → `>`; `COS 1380` `it != e` → `<`; `COS 1605` `n < INT_MAX` → `!=`; `COS 1635` `bi + 1 < size` → `!=` | 75, 122, 129, 242, 258 | equivalent: `l >= start`, `n % width >= 0`, random-access iterators meet exactly, `n <= INT_MAX`, `bi < size` |
| `COS 1246` `bi < blks.size()` → `<=` in `values` | 66 | untestable (as Fenwick's size asserts, user decision 2026-11-12): the extra block starts at `bi * m >= n` and its loop is empty unless `bi * m` overflows, which needs `n + m > INT_MAX`, about 2 GiB even for a one-byte `T` |
| `AOS 1005`–`1008` (`bi + 1` → `bi - 1`, `* 1`, `/ 1`, `% 1`), `COS 1660`, `1661` (`<` → `!=`, `<=`) in `locate`'s loop bound | 264 | untestable (as `03-segmenttree.md`'s `COS 411`): `locate` asserts `0 <= i < n`, so the remaining-index test stops the loop by the last block and every variant agrees; they differ only for `i` outside `[0, n)` with the assert compiled out, where some restore the unbounded walk `03-cpp.md` forbids, an input `05-testing.md` (Memory safety) keeps from the `NDEBUG` build |
| `CR 212` (push's early return removed) | 101 | equivalent: pushing identity tags rewrites each value as `1 * v + 0 = v` in a ring, in O(k), on the partial-block path that then loops and pulls in O(k) anyway |
| `COS 1618` (`i > size` → `>=`: insert at a block boundary starts the next block), `COS 1623` (split at exactly `2k`), `COS 1641` (merge also when the pair holds exactly `k`) | 245, 248, 258 | equivalent under the contract (block shapes are unspecified beyond its invariants, user decision 2026-11-12): only the shapes change; the sequence, `at`, `size` and every stated invariant (no empty block, at most `2k` per block, at least `k` per adjacent pair) still hold |
| `COS 1586` (`BlockList`'s size assert `<=` → `<`) | 233 | untestable within the full-mode time budget (user decision 2026-11-12): it differs only at exactly `INT_MAX` elements, a positive build of about 3 s and 522 MB on the checked build; a throwing move cannot shorten it below 2 GiB, since packed bools move without user code and any other element takes at least a byte; the assert stays |
| `COS 1295`, `1296` (`set`'s `i < n` → `!=`, `<=`) | 84 | untestable (as Fenwick's `LOS 302`, user decision 2026-11-12): `set` calls `assign(i, i + 1, x)`, whose assert rejects every index the weakened check admits before any write, which a death probe accepting any glibc assert cannot tell apart; at `i = INT_MAX` the `i + 1` overflow wraps on the checked build and still fails that assert |

## Limits and handoffs

The legacy `SqrtDecomp` (`OLD/algorithms.cpp:1864–1966`) had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction (renames under Legacy mapping). Unreachable lazy bookkeeping and the diagnostic printer remain archived (only the printer is unported); explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Mo (row 23) and the sqrt tree (row 34) are separate rows, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `maxRight`/`minLeft`, `reset`/`build`, O(1)-update group block sums (row 23); block policies and sorted blocks were adopted in 2026-11-08 as `BlockDecomp` and `SortedBlocks`. The DS02 sweep put `CounterBlocks` in planned Esoteric row 71 and left out `BlockList` set, reverse, split and merge (rows 19 and 20; [00-notes.md](00-notes.md#ds02-sweep-2026-11-12)).

## Aliases

Search aliases (P263, grab --find): sqrt decomposition, block decomposition, block array (OI Wiki: 分块, 块状数组); 分块思想, 平方分割, 根号分块, 序列分块; sorted blocks, 块内排序, 数列分块入门, block linked list, 块状链表, sqrt block list.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 2, 3, 4, 6, 7, 10–12, 14, 16, 17, 18, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: prune, removed `SqrtDecomp::setUpdate` and `SqrtRangeSum::rebuild` (cases moved to `set` and the vector constructor); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `opeUpdate`/`query` renamed `apply`/`prod`, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 509/520/509 MB.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS02, added `SortedBlocks` (with `get`, `values`, `countLeq`, `maxLess`, and `countGeq`, `countGreater`, `countEqual` adopted by user decision after the review), `BlockDecomp`, `BlockList`; five defects found by the new tests fixed; independent review: `NDEBUG`-bounded block loops, `BlockList` without default construction and `reshape` overflow fixed; first mutation campaign, full-block operation counts and two probes closed five gaps; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-12: P006 DS02 `/finish`, `BlockList::at` returns `vector<T>::reference` (`BlockList<bool>` compiled no longer and its const form dangled), `locate` bounded under `NDEBUG`, the `SortedBlocks` domain comment states that `+` preserves `<`; block shapes recorded as unspecified (user decision); the `generic-max-size` probe and the largest-size build closed `COS 1120` and `COS 1121`, a `BlockList<bool>` case covers the proxies; changed-line mutation campaign; full on g++ and g++-14, stress, extraction, integration and consistency passed.
