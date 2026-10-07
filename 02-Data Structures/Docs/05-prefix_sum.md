# 05-prefix_sum.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### PrefixSum and PrefixSum2D

Zero-based half-open ranges and rectangles `[x1, x2) x [y1, y2)` in (row, column) order; 2D arguments are `(row1, col1, row2, col2)`. Each dimension is in `[0, INT_MAX - 1]`. T is an additive commutative group with zero `T(0)`. Every stored prefix and intermediate must fit T, or T is modular. Input vectors are copied, so `rebuild` may take references to the object's own storage. An empty outer vector means 0 x 0; the dimension constructor also represents 0 x m and n x 0. Rows must have equal length (asserted). `rebuild` uses O(storage) workspace. The 1D types take a braced list (`std::initializer_list<T>`): `PrefixSum<lng> p({7})` and `PrefixSum<lng> p{7}` hold the single value 7, and `PrefixSum<lng> p(7)` holds seven zeros.

### DifferenceArray and DifferenceArray2D

Offline range or rectangle additions over the same domains; `add` on an empty range or rectangle is a no-op. `values()` reconstructs the array in O(n) or O(n * m) without changing the state. `clear` resets to zeros; `rebuild` replaces the initial values. Unused far-border subtractions are skipped, so a whole-array (singleton) update by `LLONG_MIN` never computes its unrepresentable negation. Intermediate sums and differences must still fit T. The 2D update delta is named `w` so that it does not clash with the coordinates `x1` and `x2`.

### Correctness and cost

Prefix accumulation and inclusion-exclusion cancel exactly the unwanted intervals. Difference endpoint/four-corner updates cancel outside the added region; axis-by-axis integration is their inverse. 1D build/storage/materialization O(n+1), 2D O((n+1) * (m+1)); queries/updates O(1). Rebuild and returned arrays use a further copy of the corresponding storage. `checkedSize` carries `// T: O(1), M: O(1)`.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`05-prefix_sum_tester.cpp` group) | Oracle |
|---|---|---|
| `PrefixSum`/`DifferenceArray`: `prefixSum`, `sum`, `add`, `values` | `arrays` | Ternary arrays through length 7, all range sums/updates and depth-3 histories |
| `PrefixSum2D`/`DifferenceArray2D` | `matrices` | Every ternary matrix through 2x3 and every rectangle |
| Rectangle update histories | `randomized` | 100 histories of 120 rectangle updates against literal cell loops |
| `rebuild`, `clear`, construction, types | `boundaries` | Empty dimensions, rebuild/clear/copy/move/repeated reconstruction, argument/input aliases, int64/128-bit/modular/exact dyadic cases, braced singleton/list and non-convertibility from `int`; 16 probes |

Quick: length 4, 2x2, depth 2, 15 cases. Stress: length 8, history depth 4, 400 cases. Per configuration: 681,510 checks (full, 2026-09-27), 681,515 (full, 2026-10-07).

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. The prefix suite passed in every run above (681,515 checks per configuration in the final full run).

## Sources

| Source | Actual reading and use |
|---|---|
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | pp84–85/93 independently support 1D/2D prefix inclusion-exclusion and endpoint differences. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SubMatrix reference. Its empty-input constructor is not copied; empty matrices require an explicit contract here. |
| [OI Wiki: 前缀和 & 差分](https://oi-wiki.org/basic/prefix-sum/) | Read 1D/2D prefix sums, four-corner difference signs and offline reconstruction; page update 2026-03-26. |

References inspected on 2026-09-27.

## Limits and handoffs

Online range-update/query structures belong to later batches; difference arrays reconstruct offline. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): 3D prefix sums and difference arrays, tree prefix sums and path differences (Graphs), noncommutative or xor-group prefix products.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 3, 4, 10–12, 13, 14, 15, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++.
