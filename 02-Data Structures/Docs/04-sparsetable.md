# 04-sparsetable.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SparseTable

Immutable sequence, `0 <= n <= INT_MAX`; empty construction is valid. f is associative; no identity and no default-constructible T are needed. `query(l, r)` is O(1) on a nonempty `[l, r)` and also requires idempotence; noncommutative idempotent operations (rectangular bands) are valid because the two overlapping blocks are combined in order. `fold(l, r)` is O(log(n + 1)), needs only associativity, and combines disjoint blocks left to right. `queryFast(l, r)` and `querySlow(l, r)` are the inclusive `[l, r]` forms of `query` and `fold`. `operator<<` prints the legacy diagnostic format with inclusive block bounds in O(n * log(n + 1)). Public `v` holds only valid blocks: level `i` has `n - 2^i + 1` entries.

### SparseTable2D

Rectangular `n x m` grid with `n * m <= INT_MAX`; rows of equal length are asserted once at build, before the empty-grid return (the size product is checked before narrowing). Grids with zero rows or zero columns are valid and admit no query. f must be associative, commutative and idempotent (min, max, gcd, and, or). A 2D rectangle has no canonical operand order, and the four overlapping corner blocks repeat cells. `query(x1, y1, x2, y2)` folds the nonempty half-open rectangle `[x1, x2) x [y1, y2)` in O(1) and asserts its bounds once. No identity or default-constructible T is needed. Level `(i, j)` stores the `2^i x 2^j` block folds for every valid top-left corner, row-major with width `m - 2^j + 1`. Total storage is at most `n * m * bit_width(n) * bit_width(m)` values. The input grid is copied.

### Correctness and cost

Dyadic blocks preserve input order. Overlap duplicates a contiguous aggregate B, so associativity and B·B=B suffice, even without commutativity. `fold` partitions into disjoint blocks in order. Build/storage O(n log(n+1)), query O(1), fold O(log(n+1)). Only valid blocks are stored. Associativity and idempotence are semantic caller contracts.

SparseTable2D: level `(i, j)` folds the `2^i x 2^j` block at each corner. It is built from `(i, j - 1)` by joining two horizontally adjacent halves, or for `j = 0` from `(i - 1, 0)` by joining two vertical halves, so each entry costs one f call. For `2^i <= x2 - x1 < 2^(i + 1)` the row intervals `[x1, x1 + 2^i)` and `[x2 - 2^i, x2)` cover `[x1, x2)`, and the same holds for columns. The four corner blocks therefore cover the rectangle exactly, with overlaps that idempotence absorbs. Commutativity and associativity make the combination order irrelevant. Build and storage are `sum over (i, j) of (n - 2^i + 1) * (m - 2^j + 1)`, which is O(n * m * log(n + 1) * log(m + 1)); a query is O(1).

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`04-sparsetable_tester.cpp` group) | Oracle |
|---|---|---|
| `query`, `fold`, `queryFast`, `querySlow`, `operator<<` | `exhaustive` | Ternary arrays through length 7, all min/max/sum folds, inclusive adapters and exact diagnostic output |
| Random and sizes | `randomized` | 80 random arrays and powers of two ±1 through 129 |
| Operand order, payloads | `noncommutative` | Ordered concatenation/direct affine evaluation, noncommutative idempotent rectangular bands, nondefault elements/copy-only callables, ownership/copy/move |
| `SparseTable2D::query` | `grids` | Every ternary grid with n * m <= 8 (all shapes including 0 x m and n x 0), every rectangle for min/max/gcd against incremental brute force; 40 random grids up to 20 x 20 with input ownership; shapes 1x33, 33x1, 8x9, 16x16, 17x15, 2x64; `lng` extremes; nondefault payload and copy-only callable; 7 probes recorded with the re-audit additions, and the review's `grid-ragged-empty-first` (a ragged grid whose first row is empty) |

19 assertion probes in total. Quick: length 4, 2D n * m <= 4. Stress: length 9, 2D n * m <= 9, 150 grids. Per configuration: 3,088,896 checks (full, 2026-09-27), 7,485,617 (full, 2026-10-07), 30,870,208 (stress seed 9, 2026-10-07).

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. After the review fixes the sparse suite passed stress again with seed 9 (30,870,208 checks per configuration).

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Sparse Table](https://cp-algorithms.com/data_structures/sparse-table.html) | Dyadic preprocessing, disjoint logarithmic folds, overlapping idempotent O(1) query, alternative static RMQ structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | RMQ reference. |

References inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun are not claims that the original proofs were inspected.

## Limits and handoffs

Inclusive query names and the printed diagnostic format remain available. The public `v` storage changed from a flat padded array to valid rows; clients should use query methods, and no maintained consumer accesses the old storage. Disjoint sparse tables, Cartesian trees and linear-preprocessing RMQ are future variants, not gaps of this row; no claim is made that ordinary sparse-table preprocessing is the theoretical minimum for RMQ. `maxRight`/`minLeft` were left out of the 2026-10-07 sweep ([00-notes.md](00-notes.md#p006-re-audit-omissions)).

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 5, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++.
