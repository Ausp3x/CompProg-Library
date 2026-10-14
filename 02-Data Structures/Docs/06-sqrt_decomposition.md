# 06-sqrt_decomposition.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SqrtDecomp

f is associative with two-sided identity `id` and keeps left-to-right operand order. `0 <= n <= INT_MAX`, zero-based points, half-open ranges, and an empty query returns `id`. `B = 0` picks block width `m = max(1, floor(sqrt(n)))`; a positive `B` is the width. The size and vector constructors are `explicit`; the braced-list constructor is not, so a braced list always means values. `rebuild(a, B)` replaces every point and picks a fresh width in O(n), keeping the stored `id` and `f`; it also works when `F` is not assignable (a capturing lambda), where `x = SqrtDecomp(a, B, x.id, x.f)` does not compile. `get` is O(1) (by value for `T = bool`, through `vector<T>::const_reference`), `values` copies in O(n), and `set`/`opeUpdate` rebuild one block in O(m). `opeUpdate(i, x)` sets `v[i] = f(v[i], x)`. `query` is O(m + n / m). `pull(bi)` recomputes block `bi` from `v`. It is an internal helper without a bounds check and is not part of the inventory row. Public state must not be edited directly. Costs count T and f operations as O(1).

### SqrtRangeSum

T is a commutative ring with `T(0)`, `T(1)`, `T(length)`, `+`, `*` and `==`. Every intermediate, including pending tags, must fit T unless T is modular. Points, ranges and block widths match SqrtDecomp. `affine(l, r, a, b)` maps each x to `a * x + b`. New tags compose after pending ones: `(a, b)` after `(c, d)` is `(a * c, a * d + b)`. Assignment uses `a = 0` and needs no inverse. `add`, `assign`, `multiply` and `set` are affine special cases. `get` is O(1). `values` is O(n) and leaves tags intact. `apply`, `push` and `pull` are unchecked internal block helpers: `pull` requires the block's tags to be pushed already. They are not part of the row.

### Correctness and cost

Blocks store ordered folds; queries concatenate full blocks and at most two tails. Generic point update rebuilds its block, O(B); query O(B+n/B). Scalar affine sum maps s to a*s+b*length and composes new tags after old tags. Partial mutations push old tags then rebuild; reads apply tags without mutation. Scalar range update/query O(B+n/B), point get O(1), point set O(B). Build/materialization O(n), storage O(n+1). Default sqrt block size balances B+n/B analytically; callers can tune B for their workloads. No universally fastest block width is claimed. Generic monoid point updates remain O(B); the scalar affine specialization covers the additional range operations. Public operations check their ranges once at entry.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `SqrtDecomp::setUpdate(i, x)` | `set(i, x)` |
| `SqrtRangeSum::rebuild(a, B)` | `x = SqrtRangeSum<T>(a, B)`; its body is now the vector constructor, and the size constructor delegates to it |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`06-sqrt_decomposition_tester.cpp` group) | Oracle |
|---|---|---|
| `SqrtDecomp`: `get`, `values`, `set`, `opeUpdate`, `query`, `rebuild` | generic and ordered groups | Ternary generic arrays through length 5 with default/1/2/n/n+2/INT_MAX widths; concatenation/reverse concatenation, with string `set` (formerly `setUpdate`) |
| `SqrtRangeSum`: `affine`, `add`, `assign`, `multiply`, `sum`, `get`, `set`, `values` | scalar exhaustive/history groups | Every length-2 affine history on small ternary arrays and every half-open range; 100 histories of 200 operations |
| Tag order, constness, value semantics, types | regression groups | Assignment/add/multiply order, pending tags, read constness, copy/move/reconstruction (the removed `rebuild` cases, including reconstruction from `values()` with pending tags), 128-bit/modular/exact dyadic/alias cases |
| `T = bool`, construction | regression groups | Bool OR payload with `get` bound to `const bool &` for widths 0–3; braced singleton/list; constructors not implicitly convertible; 21 probes (`generic-setupdate-end` is `generic-set-end`; `lazy-rebuild-block` became `lazy-vector-negative-block`) |

Quick: generic n<=3, lazy n<=2 depth 1, 20 trials. Stress: 400 trials. The suite reports named corpus groups instead of a scalar check count. The probes `generic-pull-end`, `lazy-apply-end`, `lazy-push-end` and `lazy-pull-end` were removed in the re-audit with the helper asserts.

## Commands and results

P006 prune run, 2026-10-13: GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H.

```bash
python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/06-sqrt_decomposition.hpp' '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.cpp'
```

Full, seed 20261013: PASS in the optimized, checked and ASan/UBSan configurations (all with `-Wall -Wextra -Wconversion -Werror`, leak checking on) with all 21 assertion probes, 50 s and peak 506 MB on g++, 47 s and peak 528 MB on g++-14 (rerun after the review restored a checked copy assignment). The other seven P006 suites passed the same commands. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 27 | Block decomposition. |

References inspected on 2026-09-27.

## Limits and handoffs

The legacy `97-Legacy/02-sqrtdecomp.cpp` had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction; the `opeUpdate` name remains (`setUpdate` is `set`). Unreachable lazy bookkeeping and the diagnostic printer remain archived (only the printer is unported); explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Mo and recursive sqrt trees are future variants, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): range actions and block policies, `maxRight`/`minLeft`, `reset`/`build`, O(1)-update group block sums.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 2, 3, 4, 6, 7, 10–12, 14, 16, 17, 18, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: prune, removed `SqrtDecomp::setUpdate` and `SqrtRangeSum::rebuild` (cases moved to `set` and the vector constructor); full suite passed on g++ and g++-14.
