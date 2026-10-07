# 06-sqrt_decomposition.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SqrtDecomp

f is associative with two-sided identity `id` and keeps left-to-right operand order. `0 <= n <= INT_MAX`, zero-based points, half-open ranges, and an empty query returns `id`. `B = 0` picks block width `m = max(1, floor(sqrt(n)))`; a positive `B` is the width. Constructors (size, vector, braced list) are `explicit`. `rebuild(a, B)` replaces every point and picks a fresh width in O(n). `get` is O(1) (by value for `T = bool`, through `vector<T>::const_reference`), `values` copies in O(n), and `set`/`setUpdate`/`opeUpdate` rebuild one block in O(m). `opeUpdate(i, x)` sets `v[i] = f(v[i], x)`. `query` is O(m + n / m). `pull(bi)` recomputes block `bi` from `v`. It is an internal helper without a bounds check and is not part of the inventory row. Public state must not be edited directly. Costs count T and f operations as O(1).

### SqrtRangeSum

T is a commutative ring with `T(0)`, `T(1)`, `T(length)`, `+`, `*` and `==`. Every intermediate, including pending tags, must fit T unless T is modular. Points, ranges and block widths match SqrtDecomp. `affine(l, r, a, b)` maps each x to `a * x + b`. New tags compose after pending ones: `(a, b)` after `(c, d)` is `(a * c, a * d + b)`. Assignment uses `a = 0` and needs no inverse. `add`, `assign`, `multiply` and `set` are affine special cases. `get` is O(1). `values` is O(n) and leaves tags intact. `rebuild` clears pending tags. `apply`, `push` and `pull` are unchecked internal block helpers: `pull` requires the block's tags to be pushed already. They are not part of the row.

### Correctness and cost

Blocks store ordered folds; queries concatenate full blocks and at most two tails. Generic point update rebuilds its block, O(B); query O(B+n/B). Scalar affine sum maps s to a*s+b*length and composes new tags after old tags. Partial mutations push old tags then rebuild; reads apply tags without mutation. Scalar range update/query O(B+n/B), point get O(1), point set O(B). Build/materialization O(n), storage O(n+1). Default sqrt block size balances B+n/B analytically; callers can tune B for their workloads. No universally fastest block width is claimed. Generic monoid point updates remain O(B); the scalar affine specialization covers the additional range operations. Public operations check their ranges once at entry.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`06-sqrt_decomposition_tester.cpp` group) | Oracle |
|---|---|---|
| `SqrtDecomp`: `get`, `values`, `set`, `setUpdate`, `opeUpdate`, `query`, `rebuild` | generic and ordered groups | Ternary generic arrays through length 5 with default/1/2/n/n+2/INT_MAX widths; concatenation/reverse concatenation |
| `SqrtRangeSum`: `affine`, `add`, `assign`, `multiply`, `sum`, `get`, `set`, `values`, `rebuild` | scalar exhaustive/history groups | Every length-2 affine history on small ternary arrays and every half-open range; 100 histories of 200 operations |
| Tag order, constness, value semantics, types | regression groups | Assignment/add/multiply order, pending tags, read constness, copy/move/rebuild, 128-bit/modular/exact dyadic/alias cases |
| `T = bool`, construction | regression groups | Bool OR payload with `get` bound to `const bool &` for widths 0–3; braced singleton/list; constructors not implicitly convertible; 22 probes |

Quick: generic n<=3, lazy n<=2 depth 1, 20 trials. Stress: 400 trials. The suite reports named corpus groups instead of a scalar check count. The probes `generic-pull-end`, `lazy-apply-end`, `lazy-push-end` and `lazy-pull-end` were removed in the re-audit with the helper asserts.

## Commands and results

### Original P006 verification — 2026-09-27

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode quick --seed 42 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

All eight per-header entries passed `--mode full --seed 20260927` on 2026-09-27 (24 configuration runs, all 100 assertion probes of the package), invoked directly as `python3 '96-Local Testing/02-Data Structures/06-sqrt_decomposition_tester.py' --mode full --seed 20260927`. Compiler GCC 16.2.1 20260810, GNU++20; CPython 3.14.7; Linux x86-64. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass. `02-integration.py` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation; `03-consistency.py` passed with zero errors. The quick run through shared discovery (seed 42, invoked by absolute path from `/tmp`) passed all eight suites and checks working-directory independence, option forwarding and discovery; it does not replace the full runs. The package stress command (`--mode stress --seed 42 --rounds 3`) was available but not run as completion evidence. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership.

### Re-audit — 2026-10-07

Package P006 was re-audited under the current rules, treating the previous verification as existing-unverified. The rows were compared with the code and the testers before any edit; the 24 confirmed findings in `00-Guidelines/23-Reaudit Findings/p006.md` were then fixed or resolved. Findings that concern this header:

| # | Finding | Disposition |
|---|---|---|
| 2 | Dangling reference for `T = bool` (SegmentTree) | The same defect in `SqrtDecomp::get` is avoided the same way (`vector<T>::const_reference`). |
| 3, 4 | A braced singleton picks the size constructor (`SqrtDecomp`, `SqrtRangeSum`); SqrtDecomp constructors are implicit | Fixed: `std::initializer_list<T>` constructors make a braced list always mean values. Both SqrtDecomp constructors are now `explicit`. The tester checks braced singletons and lists, and `static_assert` non-convertibility from `int`/`vector`. |
| 6 | `pull` listed but untested and internal | Removed from the row; documented as an unchecked internal helper, like SqrtRangeSum's `apply`/`push`/`pull`. |
| 7 | No bool payload test | Added a bool SqrtDecomp case; runs under ASan/UBSan. |
| 10–12, 17, 19, 23, 24 | Closing-brace rule (DSU, Fenwick, segment do-while, sqrt, ordered lambda, monotone lambdas, testers) | Fixed in every header, tester and the benchmark; `03-consistency.py --braces` reports nothing for the package. The tester anonymous namespaces now close with `} // namespace`. |
| 14 | Complexity lines split or not directly above the struct | Every struct and free function now has its single-line bound directly above it; contract prose moved to this document. |
| 16 | SqrtDecomp parameters `ID`, `f_` | Renamed to `id`, `f`, as in SegmentTree. |
| 18 | Block-index asserts inside affine/rebuild loops | Removed from the internal helpers; the public operations check their ranges once at entry. |
| 20 | Methods not grouped | OrderedMultiSet and SortedVector are now grouped construction / access / mutation / queries with blank lines; the other headers were regrouped the same way. |

Changes beyond the findings: Comment cap: every header keeps at most two comment lines per struct or function and at most 8% comment lines; the removed contract text is under Contracts. The shared runner compiles every configuration with `-Wall -Wextra -Wconversion -Werror`; no header or tester produces a warning.

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration   # before any edit
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The unchanged suites passed full mode with seed 20260927 before any edit. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. GCC 14.2 itself was not run. No online submission was made. The sqrt suite passed in every run above.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 27 | Block decomposition. |

References inspected on 2026-09-27.

## Limits and handoffs

The legacy `97-Legacy/02-sqrtdecomp.cpp` had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction; `setUpdate` and `opeUpdate` names remain. Unreachable lazy bookkeeping and the diagnostic printer remain archived (only the printer is unported); explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Mo and recursive sqrt trees are future variants, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): range actions and block policies, `maxRight`/`minLeft`, `reset`/`build`, O(1)-update group block sums. No P006-owned gap remains.
