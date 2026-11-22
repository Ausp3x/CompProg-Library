# 02-fenwick.hpp — Evidence

Package P006 (DS01, then DS02). The contracts common to rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### Fenwick

`0 <= n <= INT_MAX`. `add` and `get` are zero-based, `prefixSum(r)` sums `[0, r)` and `sum(l, r)` sums `[l, r)`. `T` is an additive commutative group with identity `T(0)`, `+=`, `+`, `-` and `-=`; every intermediate prefix and range sum must fit `T` unless `T` is explicitly modular. The public `n` and `v` must not be edited directly. `get(i)` is `sum(i, i + 1)` and `set(i, x)` is `add(i, x - get(i))`, both O(log(n)). `values()` returns the point values in O(n) by undoing the linear build.

`maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(sum(l, r))`, and `minLeft(r, pred)` the smallest `l` in `[0, r]` with `pred(sum(l, r))`. Both assert `pred(T(0))`; `pred` must be deterministic and stay false once the range has grown past a failure. They need no ordering on `T` (tested with `lng` and wrapping `uint`).

`lowerBound(x)` is the first `i` with `prefixSum(i + 1) >= x` and `upperBound(x)` the first `i` with `prefixSum(i + 1) > x`; both return `n` when absent. `lowerBound` returns 0 for `x <= 0` and `upperBound` for `x < 0`. They require ordered `T`, nonnegative point values and exact sums; individual updates may be negative while this holds.

Costs count `T` operations as O(1). Vector, size and braced-list (`std::initializer_list<T>`) constructors exist; `Fenwick<lng> f{n}` holds one value.

### FenwickTimestamp

`0 <= n <= INT_MAX`; zero-based `add(i, x)`, `prefixSum(r)` over `[0, r)` and `sum(l, r)` over `[l, r)`, with the Fenwick algebra and asserts. `clear()` sets every value to `T(0)` in O(1). It serves many test cases on one size, or a sweep that resets between phases (CDQ), without the O(n) refill. The public `n`, `cur`, `v` and `stamp` must not be edited directly. Copies and moves keep the current values.

### Correctness and Cost

Each cell covers the interval ending at its low-bit boundary. Construction in increasing index order sends each completed cell to its parent exactly once, so it is O(n). Update, prefix and search are O(log(n)); storage is O(n). Subtraction requires an additive commutative group.

Searches: cell `v[i + d - 1]` covers `(i, i + d]` whenever `i` is a multiple of `2d`, so the top-down descent visits each power of two once and finds the largest `r` with a monotone true-prefix property `g(r)`. `maxRight` uses `g(r) = r <= l or pred(P(r) - P(l))`, which is true at 0 and true-prefix because `pred` stays false as the range grows. `minLeft` returns 0 when `pred(P(r))` holds; otherwise it descends on `h(i) = i < r and not pred(P(r) - P(i))`, again true-prefix, and returns the largest such `i` plus one. Only differences of prefix sums are evaluated, which the group contract already requires. `lowerBound` and `upperBound` mean `maxRight(0, s < x)` and `maxRight(0, s <= x)` but share a direct descent (`descend`) instead of calling `maxRight` (see Benchmarks). `values` runs the linear build backwards: when index `i` is processed in decreasing order, cell `i` still holds its tree value, because only smaller indices subtract from it.

FenwickTimestamp: cell `i` holds its Fenwick value when `stamp[i] == cur` and 0 otherwise. `add` zeroes a stale cell before adding to it and `prefixSum` skips stale cells, so every cell reads as the Fenwick cell of the values added since the last `clear`, which only increments `cur`. When `cur` wraps to 0 after 2^32 clears, `clear` zeroes `v` in O(n): every stale cell then reads 0 whatever its stamp, so a stamp left equal to a later `cur` still reads 0 until `add` writes it.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `add1(i, x)`, `i` in `[1, n]` | `add(i - 1, x)` |
| `prefixSum1(r)` over inclusive `[1, r]` | `prefixSum(r)` |
| `sum1(l, r)` over inclusive `[l, r]` | `sum(l - 1, r)` |
| `lowerBound1(x)`, one-based, 0 when absent | `lowerBound(x) + 1`; absent when `lowerBound(x) == n` |

## Feature-to-Test Map

The Python entry and the C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; the runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`02-fenwick_tester.cpp` group) | Oracle |
|---|---|---|
| `add`, `prefixSum`, `sum`, `get`, `set`, `values` | `exhaustive` | Signed ternary arrays through length 6 with all ranges, point deltas and `set` values; `get` and `values` at every verify |
| `lowerBound`, `upperBound`, `maxRight`, `minLeft` | `exhaustive` | Quaternary frequencies through length 7 with every `lowerBound`/`upperBound` threshold and sentinel, and every `maxRight(l)`/`minLeft(r)` with `s <= cap`, `s < cap` and `0 <= s <= cap` predicates, against direct scans (the last is false on the negative partial differences before `l`, which exercises the skip guard); a length-17 `s == 0` regression and a wrapping `uint` table |
| Random histories | `randomCases` | 150 random signed and frequency histories with sampled predicate searches |
| Sizes, linear build | `boundaries` | Powers of two ±1 through 65,537; linear-build operation count |
| `FenwickTimestamp`: `add`, `prefixSum`, `sum`, `clear` | `exhaustiveStamped`, `randomStamped`, `wrapStamped`, `typeStamped` | Every history of `add(i, {-1, 2})` and `clear` on n <= 6 by iterative deepening (shortest failing history first), lengths capped by a leaf budget (full: 12, 12, 8, 6, 6, 5, 5 for n = 0..6), from `cur = 0` and `cur = UINT_MAX - 1`, each step against a plain vector and each cell against the Fenwick cell of that vector; 200 random histories of 400 steps on n <= 300 (n = 0, 1, 255–257, repeated clears); clears across the 2^32 wrap with stale cells; `lng`, wrapping `uint`, `lll`, copy and move; 9 `timestamp-*` probes |
| Types, construction | `typeCases`, `boundaries` | Signed extrema, 128-bit (`set`, `upperBound`), unsigned modular, exact dyadic, braced singleton and list, argument aliasing, copy and move; 15 Fenwick probes |

Quick mode uses n <= 4, 35 trials and boundaries through n <= 1,025, with timestamped histories on n <= 4 and 40 random trials. Stress uses 600 trials and boundaries through n <= 262,145, with timestamped histories on n <= 7 and 800 random trials. The suite reports named corpus groups instead of a scalar check count.

## Commands and Results

Machine: Linux 7.2 x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.8; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 734 MB | 0.3 + 3.6 | 14.2 + 6.7 | 16.4 + 104.5 | 842 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 799 MB | 0.2 + 2.7 | 15.8 + 6.9 | 16.5 + 52.8 | 920 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 30.3 + 0.4 | 21.4 + 5.4 | | 733 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 742 MB | 0.1 + 2.0 | 5.4 + 7.7 | 5.5 + 60.2 | 505 MB |

Package runs of 2026-11-12 (DS01 `/finish`; experiment `none`):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode full --seed 20261112` (GCC 16.2.1) | PASS, 3 configurations, 24 assertion probes, MEMORY peak 932 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode full --seed 20261112` (GCC 14.4.1, floor) | PASS, 3 configurations, 24 assertion probes, MEMORY peak 982 MB |
| `python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 7` | PASS, 3 configurations, MEMORY peak 954 MB |
| `python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/02-fenwick.hpp'`; `python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run passes on the extracted build |
| `python3 '96-Local Testing/01-run.py' --mode quick` (after the header edits) | PASS, 187 suites; `00-Tools/15-harness_tester.py` (outside this package) failed beside other suites and passed alone |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 190 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace; MEMORY peak 2722 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Mutation Testing

Record: mewt 4.0.0 on blob `db6c279969dd130bc25caeebfd293428f9e09f13` (2026-11-12, seed 1, 4 lines changed since blob `b45ba978813a`): 30 mutants, 30 caught (9 by timeout), 0 survived, 0 untested.

Stage seconds: quick 257. Earlier campaign: the whole-file campaign of 2026-11-11 (DS01; whole file, uniform sample of 543; 500 mutants, 475 caught, 25 survived after probes `add-past` and `timestamp-add-past` closed two). Its 25 survivors sit on lines the 2026-11-12 record did not cover and stay open: 15 equivalent and 10 untestable. The untestable ones need a vector of `INT_MAX` or more values, 2 GiB at one byte each and beyond the memory caps, or a UBSan probe build the runner does not have (user decision 2026-11-12, as for level ancestor on 2026-11-09). Line numbers are the current header's.

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 536` `std::move(a)` → `a`; `MR 537`, `MR 538` `std::move(x)` → `x` | 13, 63, 64 | equivalent: a copy instead of a move, same values |
| `CR 50`, `COS 323`, `COS 324` (size assert removed, `<=` → `!=`, `<=` → `<`) | 14 | untestable: only a vector of at least `INT_MAX` values tells them apart; the assert stays |
| `COS 328` `i < n` → `!=` | 15 | equivalent: `i` counts up from 0 by 1 and stops at `n` |
| `COS 329` `i < n` → `<=` | 15 | untestable: below `n = INT_MAX` the extra step at `i = n` skips, since `n \| (n + 1) > n`; at `n = INT_MAX` (a 2 GiB vector) `n + 1` overflows |
| `LOS 302`, `COS 343`, `COS 344` (`get`'s assert weakened) | 20 | untestable: `get` calls `sum(i, i + 1)`, which asserts `0 <= i` and `i + 1 <= n`, so each index the weakened assert admits still dies at a glibc assert before any read. The exception is `i = INT_MAX` (reachable for any `n` under `LOS 302` and `COS 343`, for `n = INT_MAX` under `COS 344`), where `i + 1` overflows first; in practice it wraps and `sum` still asserts. The original dies at `get`'s assert and the mutants at `sum`'s, which a death probe accepting any glibc assert cannot tell apart; only a UBSan probe build would see the overflow. |
| `AOS 134`–`136` `n - 1` → `n + 1`, `n * 1`, `n / 1` | 24 | untestable: `values` starts at `n` or `n + 1`, where `i \| (i + 1) > n`, so below `n = INT_MAX - 1` the extra steps skip; at `n >= INT_MAX - 1` (a 2 GiB vector) `i + 1` or `n + 1` overflows |
| `COS 383`, `413`, `438`, `458`, `508` `>` → `!=` | 37, 47, 58, 68, 96 | equivalent: `r` is asserted nonnegative and `r &= r - 1` keeps it so; `d` is a power of two halved down to 0 |
| `COS 424` `i + d <= l` → `<` | 50 | equivalent: at `i + d == l`, `next` is `prefixSum(l)`, so `pred(next - pl)` is `pred(T(0))`, asserted true, and the same branch is taken |
| `COS 444` `d < r - i` → `<=` | 59 | equivalent: at `i + d == r`, `pr - next` is `T(0)` and `pred(T(0))` holds, so the step is not taken either way |
| `COS 447`, `COS 449` `x <= T(0)` → `==`, `<` | 63 | equivalent: point values are nonnegative by contract, so `descend` never moves for `x <= 0` and returns 0 as the early exit does |
| `COS 494` `++cur == 0` → `<=` | 91 | equivalent: `cur` is unsigned |
| `COS 516` `stamp == cur` → `>=` | 97 | equivalent: before a wrap no stamp exceeds `cur`; after it, a stamp above `cur` marks a cell last written before the wrap, which the wrap's fill zeroed, so it adds `T(0)` |

## Benchmarks

The [benchmark driver](<../../96-Local Testing/02-Data Structures/00-foundations_benchmark.py>) (raw log `96-Local Testing/02-Data Structures/00-foundations_benchmark.json`) compares the linear constructor with constructing zeros and applying n point additions; both are checked against an independent sum.

```bash
python3 '96-Local Testing/02-Data Structures/00-foundations_benchmark.py' --runs 7 --seed 20260927
```

Conditions: Intel Core i9-11900H @ 2.50GHz; GCC 16, `-std=gnu++20 -O2 -DNDEBUG`; seed 20260927; uniform integer values in `[0,100]`; seven independent process samples, one untimed linear warmup per row. Both timed paths include allocation, construction, a checksum query and destruction; the source vector and the warmup stay outside the timing. Peak live vector payload is about `3 * n * sizeof(lng)` plus allocator overhead. Raw samples and source hashes are retained. Other verification work shared the host, so these are noisy observations without a timing gate.

| n | Builds per sample | Linear median total (ms) | Point-add median total (ms) |
|---|---|---|---|
| 32 | 8192 | 0.299186 | 0.640771 |
| 4096 | 64 | 0.276973 | 1.267312 |
| 262144 | 1 | 1.203464 | 3.001825 |

The linear constructor is asymptotically appropriate and faster in every measured case. The header has no ISA kernels, Barrett or Montgomery reduction, or empirically selected dispatch thresholds.

Search descent (2026-10-07, same host, GCC 16.2, `-O2`, n = 2^20 values uniform in [0, 99], 2^23 uniform targets, 3 process runs each, beside a running stress suite): routing `lowerBound`/`upperBound` through `maxRight` took a median of 2,091 ms against 1,856 ms for the previous loop. The direct descent took 1,853 ms against 1,966 ms for the previous loop in the paired rerun, so it is at parity and was kept.

## Sources

| Source | Actual reading and use |
|---|---|
| [OI Wiki: 树状数组, 时间戳优化](https://oi-wiki.org/ds/fenwick/) | Read through its end on 2026-11-12 (the 2026-11-11 reading stopped at 100k characters): a `tag` per node beside `t`, `add` resets a stale node before adding, `getsum` skips stale nodes, `reset` increments `Tag` in O(1); the source of `FenwickTimestamp`. |
| [cp-algorithms: Fenwick Tree](https://cp-algorithms.com/data_structures/fenwick.html) | Zero- and one-based low-bit intervals, additive and group limitations, linear construction, multidimensional and range-update boundaries. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick and segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | FenwickTree reference. |

References inspected on 2026-09-27.

## Limits and Handoffs

Range-add and multidimensional Fenwick trees are row 11 (`11-fenwick_tree_advanced.hpp`), not gaps of this row. `reset`/`build` stay out, with the reason in [00-notes.md](00-notes.md#p006-re-audit-omissions).

## Aliases

Search aliases (P263, grab --find): BIT (binary indexed tree), Fenwick tree; Library Checker `point_add_range_sum`; 树状数组, 二叉索引树, 二元索引树, 芬威克树, 单点修改区间查询, 树状数组上二分, 树状数组倍增; FenwickTimestamp: timestamp clear, multi-test Fenwick, 时间戳优化, 树状数组清空.

## History

- 2026-09-27: original P006 verification; full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit: findings 1, 3, 4, 10–12, 14, 17, 19, 20, 23 and 24 (of 24 package findings) addressed in this header; `get`, `set`, `values`, `upperBound`, `maxRight`, `minLeft` added; full and stress passed on g++. The independent review confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1 and raised five findings, all fixed (here the `maxRight` skip guard, restated contract comments, the domain line).
- 2026-10-13: prune removed `add1`, `prefixSum1`, `sum1` and `lowerBound1` (cases moved to the zero-based operations); full passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases, including OI Wiki's page titles and common Chinese names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-11: P006 DS01: `FenwickTimestamp` (`add`, `prefixSum`, `sum`, O(1) `clear`) added with its test groups and 9 probes, probe `add-past`; full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (22 equivalent, 3 untestable survivors).
- 2026-11-12: DS01 `/finish`: both `add` loops bounded for a negative index under `NDEBUG` (`uint(i) < uint(n)`), `clear` got its own complexity line, OI Wiki's timestamp section read to its end; ten survivors classified untestable (user decision); full on g++ and g++-14, stress, extraction and integration passed; changed-line mutation campaign recorded (no survivors).
