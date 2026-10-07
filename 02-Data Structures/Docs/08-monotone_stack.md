# 08-monotone_stack.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### previousSmaller, nextSmaller, previousGreater, nextGreater

All sequence and matrix dimensions fit `int`. `less` is a pure strict weak order; "smaller" means earlier in that order. `strict = true` treats comparator-equivalent elements as not smaller; `strict = false` accepts them. A missing previous index is -1 and a missing next index is n. `previousGreater`/`nextGreater` use `std::greater<T>`. The functions take O(n) time and O(n) space including the returned indices.

### slidingMinimum, slidingMaximum

`slidingMinimum`/`slidingMaximum(a, k)` return the index of the extremum of each window `[i, i + k)`, with `k > 0` asserted; `k > n` gives no windows. Equivalent extrema use the leftmost index unless `rightmost`. Workspace is O(min(n, k)) plus O(max(0, n - k + 1)) returned indices.

### HistogramRectangle and BinaryRectangle

Plain result records. With no positive rectangle (including empty input), the area is 0, every coordinate is -1, and `height` is 0.

### largestHistogramRectangle

Nonnegative `lng` heights (asserted once at entry), unit-width bars. The `lll` area covers the full domain. The witness is `[l, r) x [0, height)`, and ties take the smallest `(l, r)`. O(n) time and space.

### largestBinaryRectangle and maxZeroSubmatrix

`largestBinaryRectangle(a, value)` takes a rectangular matrix of 0/1 cells, validated once at entry, and `value` in {0, 1}. Empty dimensions are allowed. The witness is `[top, bottom) x [left, right)`, and ties take the smallest `(top, left, bottom, right)`. `int` dimensions keep the `lng` area exact. `maxZeroSubmatrix(a)` is the legacy area-only adapter: any nonzero `int` blocks, and the `lng` result widens the old `int` area. Both take O(n * m + n) time, where the `+ n` checks row shapes even when m = 0, and O(m) workspace.

### Correctness and cost

Each index is pushed/popped at most once. Discarded neighbor/deque candidates are dominated by nearer/better candidates. The histogram stack emits maximal spans; the final equal-height representative inherits the full left span. Every positive matrix rectangle appears in the histogram at its bottom row. O(n) sequence time/space; matrix O(rows * cols + rows) time and O(cols) workspace.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`08-monotone_stack_tester.cpp` group) | Oracle |
|---|---|---|
| Neighbor functions, sliding windows | boundaries group | Ternary arrays through length 7 for all neighbor/window policies |
| `largestHistogramRectangle` | histograms group | Quaternary histograms through length 7 against every interval; full `lng` heights and a 100,000-bar plateau |
| `largestBinaryRectangle`, `maxZeroSubmatrix` | matrices group | All shapes through 4x4 with at most 12 cells against every rectangle for both bits; arbitrary legacy blockers |
| Random and generic | random/generic groups | 100 random cases, generic comparator equivalence, independent witness validation/complement symmetry; 11 probes |

Stress: length 9, all binary matrices through 4x4 and a million-bar plateau. Per configuration: 808,326 checks (full, 2026-09-27), 808,148 (full, 2026-10-07).

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. The monotone suite passed in every run above (808,148 checks per configuration in the final full run).

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 8 | Sliding windows. |

References inspected on 2026-09-27.

## Limits and handoffs

Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness. General sliding-window aggregation is a future variant, not a gap of this row.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 10–12, 14, 17, 19, 20, 21, 22, 23, 24 (of 24 package findings) addressed in this header, full and stress passed on g++.
