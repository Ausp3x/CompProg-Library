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

### Original P006 verification — 2026-09-27

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode quick --seed 42 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

All eight per-header entries passed `--mode full --seed 20260927` on 2026-09-27 (24 configuration runs, all 100 assertion probes of the package), invoked directly as `python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20260927`. Compiler GCC 16.2.1 20260810, GNU++20; CPython 3.14.7; Linux x86-64. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass. `02-integration.py` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation; `03-consistency.py` passed with zero errors. The quick run through shared discovery (seed 42, invoked by absolute path from `/tmp`) passed all eight suites and checks working-directory independence, option forwarding and discovery; it does not replace the full runs. The package stress command (`--mode stress --seed 42 --rounds 3`) was available but not run as completion evidence. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership.

### Re-audit — 2026-10-07

Package P006 was re-audited under the current rules, treating the previous verification as existing-unverified. The rows were compared with the code and the testers before any edit; the 24 confirmed findings in `00-Guidelines/23-Reaudit Findings/p006.md` were then fixed or resolved. Findings that concern this header:

| # | Finding | Disposition |
|---|---|---|
| 10–12, 17, 19, 23, 24 | Closing-brace rule (DSU, Fenwick, segment do-while, sqrt, ordered lambda, monotone lambdas, testers) | Fixed in every header, tester and the benchmark; `03-consistency.py --braces` reports nothing for the package. The tester anonymous namespaces now close with `} // namespace`. |
| 14 | Complexity lines split or not directly above the struct | Every struct and free function now has its single-line bound directly above it; contract prose moved to this document. |
| 20 | Methods not grouped | OrderedMultiSet and SortedVector are now grouped construction / access / mutation / queries with blank lines; the other headers were regrouped the same way. |
| 21 | Missing complexity lines above the result structs and `nextGreater` | Fixed: one line above each adjacent pair (`previousSmaller`/`nextSmaller`, the greater adapters, the sliding pair, the two result records, the binary/zero-matrix pair), within the 8% comment cap. The review accepted this shared-line form. |
| 22 | Per-element asserts in the histogram and binary loops | Nonnegative heights are asserted once at entry with `std::ranges::all_of`; 0/1 cells are validated once at entry. The negative-height, late-negative, negative-cell and large-cell probes still fire. |

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

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The unchanged suites passed full mode with seed 20260927 before any edit. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. GCC 14.2 itself was not run. No online submission was made. The monotone suite passed in every run above (808,148 checks per configuration in the final full run).

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 8 | Sliding windows. |

References inspected on 2026-09-27.

## Limits and handoffs

Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness. General sliding-window aggregation is a future variant, not a gap of this row. No P006-owned gap remains.
