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

Stress: length 9, all binary matrices through 4x4 and a million-bar plateau.

## Commands and results

Latest result of each command: the P006 prune rerun, 2026-10-13 (first four lines; GCC 16.2.1 and GCC 14.4.1, Python 3.14.7, Linux x86-64, i9-11900H), the floor check, 2026-10-14, and the latest stress run, 2026-10-07 (last line):

```bash
python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
```

Full, seed 20261013: PASS on g++ and g++-14 in the runner's optimized, checked and ASan/UBSan builds (header and tests unchanged since the re-audit); check counts, probe counts and MEMORY peaks not recorded. Integration and consistency passed with the package run recorded in [02-fenwick.md](02-fenwick.md#commands-and-results). The 2026-10-14 floor check on GCC 14.4.1 passed 808,282 checks in each of the three configurations and 11 assertion probes, `MEMORY peak=469MB`. Stress over two rounds (seeds 7 and 8, GCC 16.2.1) passed on 2026-10-07, before the re-audit's review fixes; its counts and peak were not recorded.

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 8 | Sliding windows. |

References inspected on 2026-09-27.

## Limits and handoffs

Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness. General sliding-window aggregation is row 30 (`30-aggregation_queue.hpp`), not a gap of this row.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 11 findings fixed, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
