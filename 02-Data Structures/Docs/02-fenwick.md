# 02-fenwick.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### Fenwick

`0 <= n <= INT_MAX`, zero-based `add`/`get`/`set`, `prefixSum(r)` over `[0, r)`, `sum(l, r)` over `[l, r)`. T is an additive commutative group with identity `T(0)`, `+=`, `+`, `-` and `-=`. Every intermediate prefix and range sum must fit T, unless T is explicitly modular. Public `n` and `v` must not be edited directly. `get(i)` is `sum(i, i + 1)` and `set(i, x)` is `add(i, x - get(i))`, both O(log(n)). `values()` returns the point values in O(n) by undoing the linear build. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(sum(l, r))`, and `minLeft(r, pred)` the smallest `l` in `[0, r]` with `pred(sum(l, r))`. Both assert `pred(T(0))`; pred must be deterministic and stay false once the range has grown past a failure. They need no ordering on T (tested with `lng` and wrapping `uint`). `lowerBound(x)` is the first `i` with `prefixSum(i + 1) >= x` and `upperBound(x)` the first `i` with `prefixSum(i + 1) > x`. Both return `n` when absent; `lowerBound` returns 0 for `x <= 0` and `upperBound` for `x < 0`. They require ordered T, nonnegative point values and exact sums; individual updates may be negative if this still holds. One-based adapters: `add1(i)` with `i` in `[1, n]`, `prefixSum1(r)` over inclusive `[1, r]`, `sum1(l, r)` over inclusive `[l, r]`, allowing empty `l == r + 1` without computing `r + 1` (both endpoints representable). `lowerBound1` returns a value in `[1, n]`, or 0 when absent, avoiding `n+1` overflow; for `x <= 0` it returns 1 for nonempty trees. Costs count T operations as O(1). Vector, size and braced-list (`std::initializer_list<T>`) constructors exist; `Fenwick<lng> f{n}` means one value. A moved-from Fenwick keeps `n` with empty `v` and may only be assigned or destroyed.

### Correctness and cost

Each cell covers the interval ending at its low-bit boundary; increasing-index construction sends each completed cell to its parent exactly once, O(n). Update/prefix/search O(log(n)), O(n) storage. Subtraction requires an additive commutative group.

Searches: cell `v[i + d - 1]` covers `(i, i + d]` whenever `i` is a multiple of `2d`, so the top-down descent visits each power of two once and finds the largest `r` with a monotone true-prefix property g(r). `maxRight` uses `g(r) = r <= l or pred(P(r) - P(l))`, which is true at 0 and true-prefix because pred stays false as the range grows. `minLeft` returns 0 when `pred(P(r))` holds. Otherwise it descends on `h(i) = i < r and not pred(P(r) - P(i))`, which is again true-prefix, and returns the largest such `i` plus one. Only differences of prefix sums are evaluated, which the group contract already requires. `lowerBound`/`upperBound` are `maxRight(0, s < x)` and `maxRight(0, s <= x)` in meaning; they share a direct descent (`descend`) instead of calling `maxRight` (see Benchmarks). `values` runs the linear build backwards: when index `i` is processed in decreasing order, cell `i` still holds its tree value, because only smaller indices subtract from it. Associativity, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`02-fenwick_tester.cpp` group) | Oracle |
|---|---|---|
| `add`, `prefixSum`, `sum`, `get`, `set`, `values` | `exhaustive` | Signed ternary arrays through length 6 with all ranges/point deltas/`set` values, `get` and `values` at every verify |
| `lowerBound`, `upperBound`, `maxRight`, `minLeft` | `exhaustive` | Quaternary frequencies through length 7 with every `lowerBound`/`upperBound` threshold and sentinel and every `maxRight(l)`/`minLeft(r)` with `s <= cap`, `s < cap` and `0 <= s <= cap` predicates against direct scans (the last is false on the negative partial differences before `l`, exercising the skip guard), a length-17 `s == 0` regression and a wrapping `uint` table |
| Random histories | `randomCases` | 150 random signed/frequency histories with sampled predicate searches |
| Sizes, linear build | `boundaries` | Powers of two ±1 through 65,537; linear-build operation count |
| One-based adapters, types, construction | `typeCases`, `boundaries` | Signed extrema, 128-bit (`set`, `upperBound`), unsigned modular, exact dyadic, braced singleton/list, argument aliasing, copy/move; 19 probes (6 added for the searches) |

Quick: n<=4, 35 trials, n<=1,025. Stress: 600 trials, boundaries through n<=262,145. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. After the review fixes the Fenwick suite passed stress again with seed 9.

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
| [cp-algorithms: Fenwick Tree](https://cp-algorithms.com/data_structures/fenwick.html) | Zero/one-based low-bit intervals, additive/group limitations, linear construction, multidimensional/range-update boundaries. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | FenwickTree reference. |

References inspected on 2026-09-27.

## Limits and handoffs

The Basic `Fenwick` name avoids a collision with the existing Advanced `FenTree` (`11-fenwick_tree_advanced.hpp`). Range-add and multidimensional Fenwick are future variants, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`, timestamp clearing, noncommutative or xor-group prefix products.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 1, 3, 4, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, added `get`, `set`, `values`, `upperBound`, `maxRight`, `minLeft`, full and stress passed on g++.
