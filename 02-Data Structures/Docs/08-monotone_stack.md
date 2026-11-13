# 08-monotone_stack.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### previousSmaller, nextSmaller, previousGreater, nextGreater

All sequence and matrix dimensions fit `int`. `less` is a pure strict weak order; "smaller" means earlier in that order. `strict = true` treats comparator-equivalent elements as not smaller; `strict = false` accepts them. A missing previous index is -1 and a missing next index is n. `previousGreater`/`nextGreater` use `std::greater<T>`. The functions take O(n) time and O(n) space including the returned indices. `nextGreaterDistanceSum(a, strict)` returns `sum over i of nextGreater(a, strict)[i] - i - 1` in `lng` (at most n(n - 1) / 2): with `strict = false` it is POJ 3250's count of the cows each cow sees over strictly shorter ones.

### slidingMinimum, slidingMaximum

`slidingMinimum`/`slidingMaximum(a, k)` return the index of the extremum of each window `[i, i + k)`, with `k > 0` asserted; `k > n` gives no windows. Equivalent extrema use the leftmost index unless `rightmost`. Workspace is O(min(n, k)) plus O(max(0, n - k + 1)) returned indices.

### HistogramRectangle and BinaryRectangle

Plain result records. With no positive rectangle (including empty input), the area is 0, every coordinate is -1, and `height` is 0.

### largestHistogramRectangle

Nonnegative `lng` heights (asserted once at entry), unit-width bars. The `lll` area covers the full domain. The witness is `[l, r) x [0, height)`, and ties take the smallest `(l, r)`. O(n) time and space.

### largestBinaryRectangle and maxZeroSubmatrix

`largestBinaryRectangle(a, value)` takes a rectangular matrix of 0/1 cells, validated once at entry, and `value` in {0, 1}. Empty dimensions are allowed. The witness is `[top, bottom) x [left, right)`, and ties take the smallest `(top, left, bottom, right)`. `int` dimensions keep the `lng` area exact. `maxZeroSubmatrix(a)` is the legacy area-only adapter: any nonzero `int` blocks, and the `lng` result widens the old `int` area. Both take O(n * m + n) time, where the `+ n` checks row shapes even when m = 0, and O(m) workspace.

### maximalHistogramRectangles

Nonnegative `lng` heights, asserted once. Returns every maximal rectangle `[l, r) x [0, height)` with `height > 0` (`height = min a[l, r)`, `a[l - 1] < height` when `l > 0`, `a[r] < height` when `r < n`) exactly once as a `HistogramRectangle` with its `lll` area, ordered by `r`, then by decreasing height. At most n results; O(n) time and space.

### maximalBinaryRectangles

Same matrix contract as `largestBinaryRectangle`. Returns every all-`value` rectangle that no row or column can extend in any of the four directions exactly once, as `BinaryRectangle`s ordered by bottom, then right, then decreasing height. O(n * m + n) time, O(m) workspace plus the output, which has at most n * m entries (each is reported at its bottom row by at most one popped bar).

### largestBinarySquare and countBinarySquares

Same matrix contract. `largestBinarySquare` returns the largest all-`value` square as a `BinaryRectangle` (area = side^2), ties taking the smallest `(top, left)`, and the empty record when none exists. `countBinarySquares` returns the number of all-`value` squares of every size, which fits `lng` while `n * m < 2^42` (it is at most `n * m * min(n, m)`). Both run the side DP `side(i, j) = 1 + min(up, left, diagonal)` over one row of O(m) state in O(n * m + n).

### countBinaryRectangles and countBinaryRectanglesTotal

Same matrix contract. `countBinaryRectangles` returns an n x m table, `res[h - 1][w - 1]` the number of h x w all-`value` subrectangles (an empty vector for n = 0, n empty rows for m = 0), as CSES 1148 prints it; O(n * m + n) time and the O(n * m) output. `countBinaryRectanglesTotal` returns their sum in O(m) workspace and asserts `(n + 1) * (m + 1) <= 2^32`, which keeps the total `<= n(n + 1) / 2 * m(m + 1) / 2` in `lng`. `countBinaryRectanglesAreaAtMost(a, k, value)` (ABC420 F) sums the table's entries with `h * w <= k` (0 for `k < 1`) under the same assertion, in O(n * m + n) time and the table's O(n * m) space.

### maxMinTimesSum and maxMinTimesSum2D

`maxMinTimesSum(a)` (UVa 1619) returns the maximum over nonempty `[l, r)` of `min a[l, r) * sum a[l, r)` as a `HistogramRectangle` (`area` in `lll`, `height` the minimum), ties taking the smallest `(l, r)`, and the empty record when every product is 0. It asserts `a[i] >= 0` and `max(a) * sum(a)` within `lll`. For each i the best interval with minimum `a[i]` is its maximal span from `previousSmaller` and `nextSmaller` (extending a span with nonnegative values never lowers the sum), so `monotone_detail::weightedRectangle(a, a)` checks n spans with prefix sums in O(n). `maxMinTimesSum2D(a)` (ABC311 G) takes a rectangular matrix with `a >= 0` and `max * total <= LLONG_MAX` (asserted) and returns the best submatrix as a `BinaryRectangle`, ties taking the smallest `(top, left, bottom, right)`: for each row pair it folds column minima and sums and runs `weightedRectangle(min, sum)`, O(n^2 * m + n) time and O(m) workspace. `weightedRectangle(h, w)` needs `w[j] > 0` where `h[j] > 0`, which column sums of nonnegative cells satisfy, so the best interval for each height is its maximal span; within a row pair its smallest `(l, r)` is the global smallest for that pair.

### largestCheckerboardSquare and largestCheckerboardRectangle

Same binary matrix contract. A checkerboard region has every pair of side-adjacent cells different (Luogu P1169). Xoring cell `(i, j)` with `(i + j) & 1` maps checkerboards to constant regions, so the result is the better of `largestBinarySquare` or `largestBinaryRectangle` for values 0 and 1 on the transformed copy, ties taking the smallest `(top, left, bottom, right)`, the empty record for an empty matrix. O(n * m + n) time, O(n * m) for the copy.

### Correctness and cost

Each index is pushed/popped at most once. Discarded neighbor/deque candidates are dominated by nearer/better candidates. The histogram stack emits a span for every popped positive bar with `below`, the larger of its two neighbouring heights; the final equal-height representative inherits the full left span, and exactly the spans with `height > below` are maximal (an earlier equal bar has `below = height`). For each threshold `t`, the maximal runs of columns with height `>= t` are exactly the spans with `below < t <= height`, so the by-size table adds one run of width `W = r - l` at every height in `(below, height]` (a difference over heights, then two suffix sums over widths turn runs into `W - w + 1` placements per width), and the total adds `W(W + 1) / 2 * (height - below)`. A rectangle emitted at its bottom row is maximal upward, left and right; it is maximal downward exactly when the next row holds a non-`value` cell inside `[l, r)`, checked with that row's prefix counts. The square side DP is the standard largest-square recurrence. Every positive matrix rectangle appears in the histogram at its bottom row. O(n) sequence time/space; matrix O(rows * cols + rows) time and O(cols) workspace.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`08-monotone_stack_tester.cpp` group) | Oracle |
|---|---|---|
| Neighbor functions, sliding windows | boundaries group | Ternary arrays through length 7 for all neighbor/window policies |
| `largestHistogramRectangle` | histograms group | Quaternary histograms through length 7 against every interval; full `lng` heights and a 100,000-bar plateau |
| `largestBinaryRectangle`, `maxZeroSubmatrix` | matrices group | All shapes through 4x4 with at most 12 cells against every rectangle for both bits; arbitrary legacy blockers |
| Random and generic | random/generic and size-limit groups | full and stress mode: `slidingMinimum` with `n = k = INT_MAX` and `2^30` packed bools and a throwing comparator expects the comparator's exception (the largest admitted size and window); 100 random cases, generic comparator equivalence, independent witness validation/complement symmetry; `slidingMinimum` on a half-length window over 65536 equal values (`n * k = 2^31`), leftmost `i` and rightmost `i + k - 1` expected; 11 probes |
| `maximalHistogramRectangles` | `exhaustiveFamilies`, `randomFamilies`, `scale` | O(n^2) span enumeration with the maximality test, compared exactly in order, over every histogram with heights 0..3 through length 7; its largest area equals `largestHistogramRectangle`; flat and rising closed forms |
| `maximalBinaryRectangles`, `largestBinarySquare`, `countBinarySquares`, `countBinaryRectangles`, `countBinaryRectanglesTotal` | `families` (from `exhaustiveFamilies`, `randomFamilies`), `identities`, `scale` | `bruteCensus` over every rectangle (O(n^2 m^2) with prefix sums; itself checked against a cell-by-cell count and `bruteMatrix`): four-direction extension test, order (bottom, right, top), square tie rule; every matrix through 4x4 with at most 12 cells for both values; 100 random matrices up to 30x30; metamorphic identities (table sum = total, diagonal sum = squares, largest side from the table, complement symmetry, default `value`); all-ones 2000x1500 and staircase-2000 closed forms |
| `nextGreaterDistanceSum` | `exhaustiveAdopted`, `randomAdopted`, `scaleAdopted` | Direct scan for both `strict` settings and the default, also on strings; every array over {-1, 0, 1} through length 7 (stress 9); flat, rising and falling arrays of 10^5 against closed forms past 2^31 |
| `maxMinTimesSum` | `exhaustiveAdopted`, `randomAdopted`, `scaleAdopted` | O(n^2) scan of every interval, the exact record with the tie rule; every array over 0..3 through length 7 and over 0..4 through length 6 (stress 9 and 8; 0..4 reaches tie shapes such as `{2, 4, 1, 4, 1, 4}`); the tight `lll` bound `{LLONG_MAX, LLONG_MAX, 2}`; random values up to 2^59; reversal keeps the value; `{LLONG_MAX, LLONG_MAX}`; plateau and rising 10^5 closed forms |
| `maxMinTimesSum2D` | `exhaustiveAdopted`, `randomAdopted`, `scaleAdopted`, `binaryExtras` | O(n^2 m^2) scan of every submatrix; every 0..2 matrix through 4x4 with at most 8 cells (stress 9); random up to 30x30 with values up to 10^8; one row or column equals `maxMinTimesSum`, transposition keeps the value, 0/1 matrices give `largestBinaryRectangle`'s area; `{{INT_MAX, INT_MAX}}`; all-equal 200x150 |
| `largestCheckerboardSquare`, `largestCheckerboardRectangle` | `binaryExtras` (through `families`), `randomAdopted`, `scaleAdopted` | Every rectangle scanned with prefix counts of equal neighbours; the complement gives the same record; near-checkerboards with random flips; 2000x1500 perfect boards of both phases, row and column stripes, constant and empty matrices |
| `countBinaryRectanglesAreaAtMost` | `families`, `identities`, `scaleAdopted` | The brute by-size table summed over h * w <= k for k from `LLONG_MIN` through cells + 1 and `LLONG_MAX`; k >= n * m gives `countBinaryRectanglesTotal`, k = 0 gives 0, k = 1 the cell count; complement and default `value`; all-ones 2000x1500 at k = n * m / 7 against a formula |
| Default `rightmost` | boundaries group | `slidingMinimum` and `slidingMaximum` called without the flag on every array with ties and every window, leftmost index expected |
| New preconditions | probes | 39 probes: `maximal-histogram-negative`, `maximal-histogram-late-negative` and the `value`, cell and ragged probes of `maximal-binary`, `square`, `count-squares`, `count-rectangles`, `total`; `value` -1 and 2 on all seven value-taking functions; negative entries, short and long rows and non-0/1 cells of the adopted operations; `min-sum-overflow` (three `LLONG_MAX`) and `min-sum-2d-overflow` (three `INT_MAX`), `legacy-short-row`, `next-smaller-max-size` (`INT_MAX + 1` packed bools, 256 MiB); 51 in all. The `(n + 1) * (m + 1) <= 2^32` bound needs about 2^31 cells and has no probe |

Quick: histograms through length 4, matrices up to 9 cells, 10 random cases, all-ones 120x90, staircase 60. Stress: length 9, all binary matrices through 4x4 (16 cells), 500 random cases, a million-bar plateau.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 752 MB; optimized 0.3 s compile + 0.7 s run, checked 16.5 s compile + 1.2 s run, ASan-UBSan 20.7 s compile + 7.4 s run; MEMORY peak 889 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 817 MB; optimized 0.2 s compile + 0.8 s run, checked 20.0 s compile + 1.1 s run, ASan-UBSan 21.7 s compile + 7.2 s run; MEMORY peak 981 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 24.3 s compile + 0.1 s run, checked 19.8 s compile + 0.5 s run; MEMORY peak 752 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 755 MB; optimized 0.1 s compile + 2.2 s run, checked 6.3 s compile + 4.1 s run, ASan-UBSan 6.6 s compile + 30.9 s run; MEMORY peak 923 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (P006 DS02; experiment: none): GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H, one suite at a time after the DS02 `/finish` mutation campaigns, beside other sessions, so times are indicative.

```bash
python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/08-monotone_stack_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --mode full --filter '02-Data Structures/08-monotone_stack.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter 08-monotone_stack_tester
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/08-monotone_stack.hpp' '96-Local Testing/02-Data Structures/08-monotone_stack_tester.cpp'
```

| Command | Result |
|---|---|
| full, g++ | PASS, 3 configurations, 2,515,279 checks, 51 probes; run optimized 3.8 s, checked 26.0 s, ASan-UBSan 64.8 s; MEMORY peak 1238 MB |
| full, g++-14 | PASS; run 9.1 / 41.1 / 143.1 s; MEMORY peak 1151 MB |
| stress, seed 7 | PASS, 24,160,108 checks; run 20.9 / 83.8 / 274.0 s (limit 720 s, ASan-UBSan 38% of it); MEMORY peak 1634 MB |
| extraction ops | PASS: 23 operations in 23 extractions on g++ and g++-14, 0 failures |
| extraction testers | PASS: 23 names, quick run on the extracted build |
| `02-integration.py` | PASS: 190 standalone and aggregate headers, scalar and available AVX2 multiple-translation-unit builds, workspace; MEMORY peak 1600 MB |
| `03-consistency.py` | No errors |
| `--braces` | No closing-brace, comment-cap or free-function violations |

Quick builds run in 0.14 s (optimized) and 2.5–4.5 s (checked).

## Sources

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 8 | Sliding windows. |
| [suisen maximum_rectangle](https://suisen-cp.github.io/cp-library-cpp/library/algorithm/maximum_rectangle.hpp), maximum_square; tko919 maximalrect | Half-open witnesses and the zero-count filter for maximal rectangles: `largestBinarySquare`, `maximalHistogramRectangles`, `maximalBinaryRectangles`. |
| [CSES 1148](https://cses.fi/problemset/task/1148), 3413, 3414 | Per-size rectangle table, total squares, total rectangles: `countBinaryRectangles`, `countBinarySquares`, `countBinaryRectanglesTotal`. |
| [OI Wiki: 悬线法](https://oi-wiki.org/misc/hoverline/) (UVa 1619, Luogu P1169) | Max of min times sum, the checkerboard parity xor: `maxMinTimesSum`, `largestCheckerboardSquare`, `largestCheckerboardRectangle`. |
| [OI Wiki: 单调栈](https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/docs/ds/monotonic-stack.md) (POJ 3250) | Sum of next-greater distances: `nextGreaterDistanceSum`. |
| [AtCoder ABC420 F](https://atcoder.jp/contests/abc420/tasks/abc420_f), [ABC311 G](https://atcoder.jp/contests/abc311/tasks/abc311_g) | Rectangles of area at most K; subgrid sum times minimum: `countBinaryRectanglesAreaAtMost`, `maxMinTimesSum2D`. |

References inspected on 2026-09-27; the DS02 rows on 2026-11-12 ([00-sources.md](00-sources.md#fetched-2026-11-12-ds02-sweep)).

## Mutation testing

Record: mewt 4.0.0 on blob `73e39c51a2597cec8531648bdfb2e73ea33a27b1` (2026-11-12, seed 20261112, whole file, uniform sample of 1253): 500 mutants, 451 caught (4 by timeout), 49 survived, 0 untested.

Stage seconds: first run on this blob quick 848, full on the optimized build 116, full on the checked and ASan-UBSan builds 2517 (53 survivors); the survivor retest with the same workdir quick 917, 162, 4528, beside other campaigns. A campaign on the blob before the adopted operations (`23cca153d120`, 52 survivors) led to the default-`rightmost` window checks (`false` → `true` defaults of `slidingMinimum` and `slidingMaximum`), the `value` -1 and 2 probes of every value-taking function (`value == 0 || value == 1` → `<=`, `>=`) and the removal of `maximalBinaryRectangles`' redundant `i + 1 == n ||` (the last row's `next` is all zero, so the count test already holds there). On this blob the tight `lll` bound `{LLONG_MAX, LLONG_MAX, 2}` closed `<=` → `<` in `maxMinTimesSum`'s overflow assert (line 162), `legacy-short-row` closed the row-length assert `==` → `<=` (line 93, a short row read past its end), and arrays over 0..4 (`{2, 4, 1, 4, 1, 4}`, where the span found later has the smaller `l`) closed the swapped tie `std::tie(r, l)` and `v == ans.area` → `v > ans.area` (line 119).

In the DS02 `/finish` (header unchanged, so the record stands) the half-length window check (`n = 65536`, `k = 32768`, where `n * k + 1` leaves `int`) closed `AOS 399`, and the probe `next-smaller-max-size` (`INT_MAX + 1` packed bools, 256 MiB) closed `CR 128`, and `slidingMinimum` with `n = k = INT_MAX` and `n = k = 2^30` packed bools and a comparator that throws at its first call (the assert and the reserve pass; 256 and 128 MiB) closed `COS 796` and `AOS 398`; each was checked against the mutant applied by hand to a scratch copy (`AOS 398` and `AOS 399` on the checked and the optimized build, the others on the checked build).

| Mutant | Line | Why it survives |
|---|---|---|
| `CR 144`, `AOS 396`, `400`, `401` (`ans.reserve(n - k + 1)` removed or resized) | 44 | equivalent under the contract (the capacity of a returned vector is unspecified, user decision 2026-11-12, `00-notes.md`): only the capacity changes, every resized request stays nonnegative |
| `COS 875`, `880`, `885`, `925`, `930`, `1110`, `1115` `<` → `!=` | 94, 102, 104, 126, 127, 195 | equivalent: each counter steps by 1 from at most its bound (`i + 1 <= n` on line 195) |
| `IT 265` (`if (height)` → `if (true)`) | 75 | equivalent: a zero-height span has `below = 0` and area 0, which every consumer ignores (no area beats an empty record, `height > below` fails, `height == below` returns, the total adds `w(w + 1) / 2 * 0`) |
| `AS 298`, `AS 333`, `AS 339` (arguments of `max` swapped) | 75, 162, 168 | equivalent: symmetric functions |
| `COS 905`, `COS 1000`, `COS 1125` `>` → `!=`; `COS 1130` `<` → `!=` | 119, 157, 197 | equivalent: `v >= 0` for nonnegative inputs, `height >= below` (the stack is strictly increasing and `h <= height`), and `next[r] - next[l] <= r - l` |
| `COS 958`, `983`, `1068` `area == ans.area` → `>=` | 139, 150, 178 | equivalent: the left disjunct already took `area > ans.area` |
| `COS 986`, `1071` tie `<` → `<=` | 150, 178 | equivalent: equal areas with equal coordinates are the same record |
| `AOS 462` `(i + j) & 1` → `(i - j) & 1` | 127 | equivalent: `i + j` and `i - j` have the same parity in two's complement |
| `AS 313` (`x.bottom, x.right` swapped in the checkerboard tie) | 129 | equivalent: `x` and `y` come from opposite values of the transformed matrix, so two nonempty records never share their top-left cell, and the first two keys decide; empty records are equal either way |
| `COS 771`, `COS 860`, `861`, `991`, `1020` (size asserts `<=` → `<`, `!=`) | 8, 89, 154, 165 | untestable (as Fenwick's size asserts, user decision 2026-11-12): only `INT_MAX` or more elements, at least 2 GiB, tell them apart; the asserts stay |
| `AOS 623`, `624`, `626`–`628`, `630`, `COS 1210`; `CR 238`, `AOS 660`, `661`, `667`, `669`, `COS 1220`, `1221` (the `(n + 1) * (m + 1) <= 2^32` asserts of the total and area-at-most counts weakened, tightened at equality or removed) | 229, 243 | untestable (as above): every variant agrees below about 2^31 cells, so telling them apart needs a row of 2^31 `int`s (8 GiB) or 2^32 empty rows (96 GiB) |

## Limits and handoffs

Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness. General sliding-window aggregation is row 30 (`30-aggregation_queue.hpp`), not a gap of this row.

## Aliases

Search aliases (P263, grab --find): 单调队列/单调栈优化, 单调栈, 单调队列, 悬线法, 滑动窗口最值, 直方图最大矩形, 最大子矩形, 最大全零子矩阵, 下一个更大元素, 单调堆栈; maximal rectangles, 极大子矩形, largest square, 最大正方形, count rectangles, 子矩形计数, 全1子矩形个数, feel good, 区间最小值乘区间和, 棋盘制作, checkerboard rectangle, bad hair day.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 11 findings fixed, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed.
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change; `悬线法` names a related technique answering the same queries, kept as a search synonym (user decision 2026-11-07).
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS02, added `largestBinarySquare`, `countBinarySquares`, `maximalHistogramRectangles`, `maximalBinaryRectangles`, `countBinaryRectangles`, `countBinaryRectanglesTotal` (the histogram helper now reports each span's neighbour floor) and, adopted by user decision after the review, `countBinaryRectanglesAreaAtMost`, `largestCheckerboardSquare`, `largestCheckerboardRectangle`, `maxMinTimesSum`, `maxMinTimesSum2D`, `nextGreaterDistanceSum`; the tests found `maxMinTimesSum2D`'s assert-only locals breaking the `-DNDEBUG -Werror` build (fixed); mutation campaigns on the pre-adoption and final blobs, gaps closed by tests, a redundant last-row check removed; full on g++ and g++-14, stress, extraction, integration and consistency passed.
- 2026-11-12: P006 DS02 `/finish`, header unchanged; the half-length window check and the `next-smaller-max-size` probe closed `AOS 399` and `CR 128`, the largest admitted windows with a throwing comparator `COS 796` and `AOS 398`; DS02 sources recorded; full on g++ and g++-14, stress, extraction, integration and consistency passed.
