# 06-sqrt_decomposition.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SqrtDecomp

`M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract), kept as `m` and passed last as `mon = {}` (`SqrtDecomp(N = 0, B = 0, mon = {})`, `SqrtDecomp(vector<S>, B = 0, mon = {})`); left-to-right operand order is kept. `0 <= n <= INT_MAX`, zero-based points, half-open ranges, and an empty product returns `e()`. `B = 0` picks block width `width = max(1, floor(sqrt(n)))` (the field is `width` because `m` is the monoid); a positive `B` is the width. The size and vector constructors are `explicit`; the braced-list constructor is not, so a braced list always means values. `rebuild(a, B)` replaces every point and picks a fresh width in O(n), keeping the stored monoid, which also works when the monoid is not assignable (a capturing lambda through `Monoid`). `get` is O(1) (by value for `S = bool`, through `vector<S>::const_reference`), `values` copies in O(n), and `set`/`apply` rebuild one block in O(width). `apply(i, x)` sets `v[i] = op(v[i], x)`. `prod` is O(width + n / width). `pull(bi)` recomputes block `bi` from `v`. It is an internal helper without a bounds check and is not part of the inventory row. Public state must not be edited directly. Costs count `op` calls and `S` copies as O(1).

### SqrtRangeSum

T is a commutative ring with `T(0)`, `T(1)`, `T(length)`, `+`, `*` and `==`. Every intermediate, including pending tags, must fit T unless T is modular. Points, ranges and block widths match SqrtDecomp. `affine(l, r, a, b)` maps each x to `a * x + b`. New tags compose after pending ones: `(a, b)` after `(c, d)` is `(a * c, a * d + b)`. Assignment uses `a = 0` and needs no inverse. `add`, `assign`, `multiply` and `set` are affine special cases. `get` is O(1). `values` is O(n) and leaves tags intact. `apply`, `push` and `pull` are unchecked internal block helpers: `pull` requires the block's tags to be pushed already. They are not part of the row.

### Correctness and cost

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

Quick: generic n<=3, lazy n<=2 depth 1, 20 trials. Stress: 400 trials. The suite reports named corpus groups instead of a scalar check count.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 787 MB; optimized 0.3 s compile + 4.0 s run, checked 18.2 s compile + 4.4 s run, ASan-UBSan 21.0 s compile + 32.9 s run; MEMORY peak 1000 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 865 MB; optimized 0.2 s compile + 5.8 s run, checked 24.9 s compile + 5.8 s run, ASan-UBSan 29.0 s compile + 19.3 s run; MEMORY peak 1076 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 27.9 s compile + 0.5 s run, checked 22.6 s compile + 1.7 s run; MEMORY peak 784 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 790 MB; optimized 0.1 s compile + 2.5 s run, checked 8.2 s compile + 3.3 s run, ASan-UBSan 8.9 s compile + 21.4 s run; MEMORY peak 541 MB. Dated rows below stay those commands' latest results.

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `06-sqrt_decomposition_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, n/a checks per configuration, 21 assertion probes, MEMORY peak 509 MB |
| `CXX=g++-14 06-sqrt_decomposition_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, n/a checks per configuration, MEMORY peak 520 MB |
| `06-sqrt_decomposition_tester.py --mode stress --seed 7` | PASS, n/a checks per configuration, MEMORY peak 509 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 27 | Block decomposition. |

References inspected on 2026-09-27.

## Limits and handoffs

The legacy `SqrtDecomp` (`OLD/algorithms.cpp:1864–1966`) had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction (renames under Legacy mapping). Unreachable lazy bookkeeping and the diagnostic printer remain archived (only the printer is unported); explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Mo (row 23) and the sqrt tree (row 34) are separate rows, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): range actions and block policies, `maxRight`/`minLeft`, `reset`/`build`, O(1)-update group block sums.

## Aliases

Search aliases (P263, grab --find): sqrt decomposition, block decomposition, block array (OI Wiki: 分块, 块状数组); 分块思想, 平方分割, 根号分块, 序列分块.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 2, 3, 4, 6, 7, 10–12, 14, 16, 17, 18, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: prune, removed `SqrtDecomp::setUpdate` and `SqrtRangeSum::rebuild` (cases moved to `set` and the vector constructor); full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `opeUpdate`/`query` renamed `apply`/`prod`, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 509/520/509 MB.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
