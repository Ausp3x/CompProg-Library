# 02-fenwick.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### Fenwick

`0 <= n <= INT_MAX`, zero-based `add`/`get`/`set`, `prefixSum(r)` over `[0, r)`, `sum(l, r)` over `[l, r)`. T is an additive commutative group with identity `T(0)`, `+=`, `+`, `-` and `-=`. Every intermediate prefix and range sum must fit T, unless T is explicitly modular. Public `n` and `v` must not be edited directly. `get(i)` is `sum(i, i + 1)` and `set(i, x)` is `add(i, x - get(i))`, both O(log(n)). `values()` returns the point values in O(n) by undoing the linear build. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(sum(l, r))`, and `minLeft(r, pred)` the smallest `l` in `[0, r]` with `pred(sum(l, r))`. Both assert `pred(T(0))`; pred must be deterministic and stay false once the range has grown past a failure. They need no ordering on T (tested with `lng` and wrapping `uint`). `lowerBound(x)` is the first `i` with `prefixSum(i + 1) >= x` and `upperBound(x)` the first `i` with `prefixSum(i + 1) > x`. Both return `n` when absent; `lowerBound` returns 0 for `x <= 0` and `upperBound` for `x < 0`. They require ordered T, nonnegative point values and exact sums; individual updates may be negative if this still holds. Costs count T operations as O(1). Vector, size and braced-list (`std::initializer_list<T>`) constructors exist; `Fenwick<lng> f{n}` means one value.

### FenwickTimestamp

`0 <= n <= INT_MAX`, zero-based `add(i, x)`, `prefixSum(r)` over `[0, r)` and `sum(l, r)` over `[l, r)`, with the Fenwick algebra and asserts; `clear()` sets every value to `T(0)` in O(1). For many test cases on one size, or a sweep that resets between phases (CDQ), without the O(n) refill. Public `n`, `cur`, `v` and `stamp` must not be edited directly. Copies and moves keep the current values.

### Correctness and cost

Each cell covers the interval ending at its low-bit boundary; increasing-index construction sends each completed cell to its parent exactly once, O(n). Update/prefix/search O(log(n)), O(n) storage. Subtraction requires an additive commutative group.

Searches: cell `v[i + d - 1]` covers `(i, i + d]` whenever `i` is a multiple of `2d`, so the top-down descent visits each power of two once and finds the largest `r` with a monotone true-prefix property g(r). `maxRight` uses `g(r) = r <= l or pred(P(r) - P(l))`, which is true at 0 and true-prefix because pred stays false as the range grows. `minLeft` returns 0 when `pred(P(r))` holds. Otherwise it descends on `h(i) = i < r and not pred(P(r) - P(i))`, which is again true-prefix, and returns the largest such `i` plus one. Only differences of prefix sums are evaluated, which the group contract already requires. `lowerBound`/`upperBound` are `maxRight(0, s < x)` and `maxRight(0, s <= x)` in meaning; they share a direct descent (`descend`) instead of calling `maxRight` (see Benchmarks). `values` runs the linear build backwards: when index `i` is processed in decreasing order, cell `i` still holds its tree value, because only smaller indices subtract from it.

FenwickTimestamp: cell `i` holds its Fenwick value when `stamp[i] == cur` and 0 otherwise. `add` zeroes a stale cell before adding to it and `prefixSum` skips stale cells, so every cell reads as the Fenwick cell of the values added since the last `clear`, which only increments `cur`. When `cur` wraps to 0 after 2^32 clears, `clear` zeroes `v` in O(n): every stale cell then reads 0 whatever its stamp, so a stamp left equal to a later `cur` still reads 0 until `add` writes it.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `add1(i, x)`, `i` in `[1, n]` | `add(i - 1, x)` |
| `prefixSum1(r)` over inclusive `[1, r]` | `prefixSum(r)` |
| `sum1(l, r)` over inclusive `[l, r]` | `sum(l - 1, r)` |
| `lowerBound1(x)`, one-based, 0 when absent | `lowerBound(x) + 1`; absent when `lowerBound(x) == n` |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`02-fenwick_tester.cpp` group) | Oracle |
|---|---|---|
| `add`, `prefixSum`, `sum`, `get`, `set`, `values` | `exhaustive` | Signed ternary arrays through length 6 with all ranges/point deltas/`set` values, `get` and `values` at every verify |
| `lowerBound`, `upperBound`, `maxRight`, `minLeft` | `exhaustive` | Quaternary frequencies through length 7 with every `lowerBound`/`upperBound` threshold and sentinel and every `maxRight(l)`/`minLeft(r)` with `s <= cap`, `s < cap` and `0 <= s <= cap` predicates against direct scans (the last is false on the negative partial differences before `l`, exercising the skip guard), a length-17 `s == 0` regression and a wrapping `uint` table |
| Random histories | `randomCases` | 150 random signed/frequency histories with sampled predicate searches |
| Sizes, linear build | `boundaries` | Powers of two ±1 through 65,537; linear-build operation count |
| `FenwickTimestamp`: `add`, `prefixSum`, `sum`, `clear` | `exhaustiveStamped`, `randomStamped`, `wrapStamped`, `typeStamped` | Every history of `add(i, {-1, 2})` and `clear` on n <= 6 by iterative deepening (shortest failing history first), lengths capped by a leaf budget (full: 12, 12, 8, 6, 6, 5, 5 for n = 0..6), from `cur = 0` and `cur = UINT_MAX - 1`, each step against a plain vector and each cell against the Fenwick cell of that vector; 200 random histories of 400 steps on n <= 300 (n = 0, 1, 255–257, repeated clears); clears across the 2^32 wrap with stale cells; `lng`, wrapping `uint`, `lll`, copy and move; 9 `timestamp-*` probes |
| Types, construction | `typeCases`, `boundaries` | Signed extrema, 128-bit (`set`, `upperBound`), unsigned modular, exact dyadic, braced singleton/list, argument aliasing, copy/move; 15 Fenwick probes |

Quick: n<=4, 35 trials, n<=1,025; timestamped histories n<=4, 40 random trials. Stress: 600 trials, boundaries through n<=262,145; timestamped histories n<=7, 800 random trials. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 734 MB; optimized 0.3 s compile + 3.6 s run, checked 14.2 s compile + 6.7 s run, ASan-UBSan 16.4 s compile + 104.5 s run; MEMORY peak 842 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 799 MB; optimized 0.2 s compile + 2.7 s run, checked 15.8 s compile + 6.9 s run, ASan-UBSan 16.5 s compile + 52.8 s run; MEMORY peak 920 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 30.3 s compile + 0.4 s run, checked 21.4 s compile + 5.4 s run; MEMORY peak 733 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 742 MB; optimized 0.1 s compile + 2.0 s run, checked 5.4 s compile + 7.7 s run, ASan-UBSan 5.5 s compile + 60.2 s run; MEMORY peak 505 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (DS01 `/finish`; experiment `none`), Linux 7.2 x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `02-fenwick_tester.py --mode full --seed 20261112` (GCC 16.2.1) | PASS, 3 configurations, 24 assertion probes, MEMORY peak 932 MB |
| `CXX=g++-14 02-fenwick_tester.py --mode full --seed 20261112` (GCC 14.4.1, floor) | PASS, 3 configurations, 24 assertion probes, MEMORY peak 982 MB |
| `02-fenwick_tester.py --mode stress --seed 7` | PASS, 3 configurations, MEMORY peak 954 MB |
| `05-extraction.py ops --filter '02-Data Structures/02-fenwick.hpp'`; `05-extraction.py testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run on the extracted build passes |
| `01-run.py --mode quick` (after this run's header edits) | PASS, 187 suites; NOTE: `00-Tools/15-harness_tester.py` (outside this package) failed beside other suites and passed alone |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 2722 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/02-fenwick.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Mutation testing

Record: mewt 4.0.0 on blob `db6c279969dd130bc25caeebfd293428f9e09f13` (2026-11-12, seed 1, 4 lines changed since blob `b45ba978813a`): 30 mutants, 30 caught (9 by timeout), 0 survived, 0 untested.

That campaign (DS01 `/finish`: both `add` loops bounded for negative indices under `NDEBUG`, `clear` got its own complexity line) took quick 257 s and left no survivor.

Earlier campaign: mewt 4.0.0 on blob `b45ba978813aa97a213ef5a373e7e80442c7b142` (2026-11-11, seed 1, whole file, uniform sample of 543): 500 mutants, 475 caught (28 by timeout), 25 survived, 0 untested. Line numbers below are that blob's.

Stage seconds (DS01, first campaign of this header, both runs together): quick 1380, full on the optimized build 114, full on the checked and ASan-UBSan builds 1469. The first run left 27 survivors; probes `add-past` (`Fenwick(2).add(3, 1)`) and `timestamp-add-past` closed `COS 363` (line 30) and `COS 478` (line 85), where `i < n` → `i != n` let a past-the-end `add` return silently. The rerun with the same workdir retested the survivors. Of the 25 left, 15 are equivalent and 10 untestable:

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 536` `std::move(a)` → `a`; `MR 537`, `MR 538` `std::move(x)` → `x` | 13, 63, 64 | equivalent: a copy instead of a move, same values |
| `CR 50`, `COS 323`, `COS 324` (size assert removed, `<=` → `!=`, `<=` → `<`) | 14 | untestable (user decision 2026-11-12, as level ancestor's 2026-11-09): only a vector of at least `INT_MAX` values, 2 GiB at one byte each, tells them apart, beyond the memory caps; the assert stays |
| `COS 328` `i < n` → `!=` | 15 | equivalent: `i` counts up from 0 by 1 and stops at `n` |
| `COS 329` `i < n` → `<=` | 15 | untestable (user decision 2026-11-12): below `n = INT_MAX` the extra step at `i = n` skips, since `n \| (n + 1) > n`; at `n = INT_MAX`, a 2 GiB vector of one-byte values, `n + 1` overflows |
| `LOS 302`, `COS 343`, `COS 344` (`get`'s assert weakened) | 20 | untestable (user decision 2026-11-12): `get` calls `sum(i, i + 1)`, which asserts `0 <= i` and `i + 1 <= n`, so each index the weakened assert admits still dies at a glibc assert before any read, except `i = INT_MAX` (reachable for any `n` under `LOS 302` and `COS 343`, for `n = INT_MAX` under `COS 344`), where `i + 1` overflows first; in practice it wraps and `sum` still asserts; the original dies at `get`'s assert and the mutants at `sum`'s, which a death probe that accepts any glibc assert cannot tell apart, and only a UBSan probe build, which the runner does not have, would see the overflow |
| `AOS 134`–`136` `n - 1` → `n + 1`, `n * 1`, `n / 1` | 24 | untestable (user decision 2026-11-12): `values` starts at `n` or `n + 1`, where `i \| (i + 1) > n`, so below `n = INT_MAX - 1` the extra steps skip; at `n >= INT_MAX - 1`, a 2 GiB vector, `i + 1` or `n + 1` overflows |
| `COS 383`, `413`, `438`, `458`, `508` `>` → `!=` | 37, 47, 58, 68, 95 | equivalent: `r` is asserted nonnegative and `r &= r - 1` keeps it so; `d` is a power of two halved down to 0 |
| `COS 424` `i + d <= l` → `<` | 50 | equivalent: at `i + d == l`, `next` is `prefixSum(l)`, so `pred(next - pl)` is `pred(T(0))`, asserted true, and the same branch is taken |
| `COS 444` `d < r - i` → `<=` | 59 | equivalent: at `i + d == r`, `pr - next` is `T(0)` and `pred(T(0))` holds, so the step is not taken either way |
| `COS 447`, `COS 449` `x <= T(0)` → `==`, `<` | 63 | equivalent: point values are nonnegative by contract, so `descend` never moves for `x <= 0` and returns 0 as the early exit does |
| `COS 494` `++cur == 0` → `<=` | 90 | equivalent: `cur` is unsigned |
| `COS 516` `stamp == cur` → `>=` | 96 | equivalent: before a wrap no stamp exceeds `cur`; after it, a stamp above `cur` marks a cell last written before the wrap, which the wrap's fill zeroed, so it adds `T(0)` |

## Benchmarks

The [benchmark driver](<../../96-Local Testing/02-Data Structures/00-foundations_benchmark.py>) and `96-Local Testing/02-Data Structures/00-foundations_benchmark.json` compare the final linear constructor with constructing zeros and applying n point additions. Both are checked against an independent sum.

```bash
python3 '96-Local Testing/02-Data Structures/00-foundations_benchmark.py' --runs 7 --seed 20260927
```

Recorded CPU: Intel Core i9-11900H @ 2.50GHz; GCC16, `-std=gnu++20 -O2 -DNDEBUG`, seed 20260927, uniform integer values `[0,100]`, seven independent process samples, one untimed linear warmup per row. Both timed paths include allocation, construction, a checksum query and destruction. The source vector and warmup remain outside the timed construction; peak live vector payload is approximately `3 * n * sizeof(lng)` plus allocator/runtime overhead. Raw samples and source hashes are retained. Other verification work shared the host, so these are noisy observations without a timing gate.

| n | Builds per sample | Linear median total (ms) | Point-add median total (ms) |
|---|---|---|---|
| 32 | 8192 | 0.299186 | 0.640771 |
| 4096 | 64 | 0.276973 | 1.267312 |
| 262144 | 1 | 1.203464 | 3.001825 |

The linear constructor is both asymptotically appropriate and faster in these measured cases. No ISA kernels, Barrett/Montgomery use or empirically selected dispatch thresholds were introduced.

Search descent (2026-10-07, same host, GCC 16.2, `-O2`, n = 2^20 values uniform in [0, 99], 2^23 uniform targets, 3 process runs each, shared with a running stress suite): routing `lowerBound`/`upperBound` through `maxRight` took a median of 2,091 ms against 1,856 ms for the previous loop. The direct descent took 1,853 ms against 1,966 ms for the previous loop in the paired rerun, so it is at parity.

## Sources

| Source | Actual reading and use |
|---|---|
| [OI Wiki: 树状数组, 时间戳优化](https://oi-wiki.org/ds/fenwick/) | Read through its end on 2026-11-12 (the 2026-11-11 reading stopped at 100k characters): a `tag` per node beside `t`, `add` resets a stale node before adding, `getsum` skips stale nodes, `reset` increments `Tag` in O(1); the source of `FenwickTimestamp`. |
| [cp-algorithms: Fenwick Tree](https://cp-algorithms.com/data_structures/fenwick.html) | Zero/one-based low-bit intervals, additive/group limitations, linear construction, multidimensional/range-update boundaries. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | FenwickTree reference. |

References inspected on 2026-09-27.

## Limits and handoffs

Range-add and multidimensional Fenwick are row 11 (`11-fenwick_tree_advanced.hpp`), not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`, noncommutative or xor-group prefix products.

## Aliases

Search aliases (P263, grab --find): BIT (binary indexed tree), Fenwick tree; Library Checker `point_add_range_sum`; 树状数组, 二叉索引树, 二元索引树, 芬威克树, 单点修改区间查询, 树状数组上二分, 树状数组倍增; FenwickTimestamp: timestamp clear, multi-test Fenwick, 时间戳优化, 树状数组清空.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 1, 3, 4, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, added `get`, `set`, `values`, `upperBound`, `maxRight`, `minLeft`, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed (here the `maxRight` skip guard, restated contract comments, the domain line).
- 2026-10-13: prune, removed `add1`, `prefixSum1`, `sum1` and `lowerBound1` (cases moved to the zero-based operations); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-11: P006 DS01: `FenwickTimestamp` (`add`, `prefixSum`, `sum`, O(1) `clear`) added with its test groups and 9 probes, probe `add-past`; full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (22 equivalent, 3 untestable survivors).
- 2026-11-12: DS01 `/finish`: both `add` loops bounded for a negative index under `NDEBUG` (`uint(i) < uint(n)`), `clear` got its own complexity line, OI Wiki's timestamp section read to its end; ten survivors classified untestable (user decision); full on g++ and g++-14, stress, extraction and integration passed; changed-line mutation campaign recorded (no survivors).
