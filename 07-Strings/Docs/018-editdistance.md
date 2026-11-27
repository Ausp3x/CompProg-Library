# 018-editdistance.hpp — Evidence

`018-editdistance.hpp` implements edit distances (unit, weighted, banded, thresholded, output-sensitive, bit-parallel, with witnesses, and both transposition definitions) and score-maximizing sequence alignments (global, semi-global, local, affine).

## Contracts

### Common Conventions

- Every function is a template over the sequence type `S`: `string`, `string_view`, `vector<T>` or another sequence with `size()` and `operator[]`; both arguments have the same type. Symbols are compared with `==`, and with `<` where ranking is stated. Lengths are `n = |a|` and `m = |b|` with `n + m < INT_MAX`; distances are exact `int`.
- Alignments are column lists in left-to-right order: `(i, j)` aligns `a[i]` with `b[j]` (match or substitution), `(i, -1)` deletes `a[i]` and `(-1, j)` inserts `b[j]`; the `i` and `j` entries enumerate `0..n-1` and `0..m-1` in order.
- Scored functions take `score(x, y) -> lng` (any values) and additive gap scores (`gap`, `open`, `extend`), usually negative; totals must fit `lng` with headroom (`|total| < 2^61`). Scored functions maximize; cost functions minimize.

### levenshtein, levenshteinWitness, hirschberg

- `levenshtein(a, b)`: the unit-cost insert, delete and substitute distance in `O(n m)` time and `O(min(n, m))` space (the shorter string indexes the row).
- `levenshteinWitness(a, b)` returns `{distance, columns}` from a full `(n+1)(m+1)` table, `O(n m)` time and space; traceback from `(n, m)` prefers diagonal, then deletion, then insertion.
- `hirschberg(a, b, score, gap)` returns `{score, columns}` of a maximum global alignment (the Needleman–Wunsch witness) in `O(n m)` time and `O(n + m)` space. The two linear rows of a split live only inside `editdistance_detail::split` and are released before recursing, so only `O(log n)` frames of constant size remain; the recursion depth is `log2(n) + 1`. The split takes the smallest `b` index maximizing forward plus reverse scores; a one-symbol `a` range aligns with the first best `b` symbol, or is deleted when every pairing is worse. The returned score is recomputed from the columns.
- `hirschberg(a, b)` is the unit-cost instance (`score = -[x != y]`, `gap = -1`) and returns `{distance, columns}`.

### myersBitVector

- Exact Levenshtein distance by Myers' bit-vector recurrence in 64-row blocks (Myers 1999, block form; global distance with top-row horizontal delta `+1`). The longer string is cut into blocks and processed block-major, each column's horizontal delta carried between blocks in an `int` array. `O(min(n, m) * ceil(max(n, m)/64) + n + m)` time and `O(n + m + sigma)` space, `sigma` being the code range of the longer string's symbols: raw unsigned values (`<= 256`) for 1-byte integral types, otherwise ranks among the longer string's distinct symbols (`O((n + m) log(n+1))` ranking; symbols of the shorter string absent from it get no match bits).

### bandedEditDistance, thresholdEditDistance

- `bandedEditDistance(a, b, w)` (`w >= 0`, asserted) is the minimum unit cost over alignments whose cells all satisfy `|i - j| <= w`, or `-1` when `|n - m| > w` and no such alignment exists. It is exact whenever the distance is at most `w`, since an alignment of cost `d` never leaves the band `|i - j| <= d`. `O(max(n, m) * min(2w + 1, min(n, m) + 1))` time, `O(min(n, m))` space.
- `thresholdEditDistance(a, b, k)` (`k >= 0`, asserted) returns `min(distance, k + 1)`: the band `w = k` with an early exit once a whole row exceeds `k` (every alignment of cost `<= k` crosses each row inside the band at a cell of value `<= k`), and immediately when `|n - m| > k`. Same bounds as the band.

### diagonalEditDistance

- Exact Levenshtein distance `d` by furthest-reaching points on diagonals `k = j - i` (Ukkonen 1985, Myers 1986, with substitutions): round `e` extends round `e - 1` by substitution, deletion or insertion and slides along matches. Ukkonen's cutoff keeps only diagonals with `e + |k - (m - n)| <= max(n, m)`; the cut set is closed under successors and contains every optimal path, so the answer is exact, and at most `min(n, m) + 1` diagonals are live per round. `O((d + 1) * (min(n, m) + 1))` time, never worse than the table DP and fast for similar strings; `O(n + m)` space.

### weightedEditDistance

- `weightedEditDistance(a, b, ins, del, sub)` with costs `>= 0` (asserted): inserting `b[j]` costs `ins`, deleting `a[i]` costs `del`, substituting unequal symbols costs `sub`, and matches are free. `O(n m)` time, `O(m)` space, exact `lng`.

### optimalStringAlignment, damerauLevenshtein

- `optimalStringAlignment(a, b)`: restricted Damerau–Levenshtein (OSA), unit insert, delete and substitute plus swapping two adjacent symbols when no substring is edited twice. `O(n m)` time, `O(m)` space. It is not a metric (`"ca" -> "abc"` is 3).
- `damerauLevenshtein(a, b)`: unrestricted Damerau–Levenshtein (Lowrance–Wagner), the true metric over insert, delete, substitute and adjacent transposition (`"ca" -> "abc"` is 2). `O(n m + (n + m) log(n+1))` time, the log term only for non-byte symbols; `O(n m)` space. `DL <= OSA <= Levenshtein`.

### needlemanWunsch, semiGlobalAlignment, smithWaterman, gotoh

- `needlemanWunsch(a, b, score, gap)`: the maximum global alignment score, each gap column adding `gap`. `O(n m)` time, `O(m)` space. Its witness is `hirschberg(a, b, score, gap)`.
- `semiGlobalAlignment(a, b, score, gap)` (`gap <= 0`, asserted): gaps in a string's row before its first symbol or after its last score 0, so the result is at least 0. `O(n m)` time, `O(m)` space. With a positive gap, continuing past a free end would outscore the free end and the definition degenerates, hence the precondition.
- `smithWaterman(a, b, score, gap)` (`gap <= 0`, asserted) returns `{score, la, ra, lb, rb}`: the maximum over substring pairs of the global alignment score of `a[la, ra)` and `b[lb, rb)`, with `ra, rb` the first maximum end cell in row-major order. The start follows predecessors preferring diagonal, up, left, then a fresh start, so the region's global score equals `score`. It returns `{0, 0, 0, 0, 0}` when no alignment scores above 0. `O(n m)` time, `O(m)` space.
- `gotoh(a, b, score, open, extend)`: the maximum global alignment with affine gaps; a maximal run of `L` consecutive insertions (or deletions) adds `open + extend * L`, and an insertion run next to a deletion run opens twice. Three linear rows: `O(n m)` time, `O(m)` space; unreachable states use `NEG = LLONG_MIN / 4`.

Correctness: each DP evaluates the standard recurrence whose states are prefix pairs; the min (max) over the three or four predecessor moves is the min (max) over alignments ending with that column, so the final cell is the optimum over all alignments. Hirschberg's split is valid because any alignment crosses the middle row of `a` at some column of `b`, and forward plus reversed prefix scores give the best alignment through each crossing. Myers' recurrence encodes the vertical and horizontal `+-1` deltas of the DP table, and the block hand-off passes exactly the horizontal delta at a block's bottom row.

## Feature-to-Test Map

Runnable entry: [018-editdistance_tester.py](<../../96-Local Testing/07-Strings/018-editdistance_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/018-editdistance_tester.cpp>).

| Feature/domain | Independent verification |
|---|---|
| `levenshtein`, `myersBitVector`, `diagonalEditDistance` | Minimum over explicitly enumerated alignments, itself checked against BFS over strings with insert, delete and substitute (every exhaustive pair, every fourth random pair); every ternary pair through lengths 3/4/4 (quick/full/stress) and 150/1,500/2,000 random pairs up to length 5 over 1–4 letters; 40/300/450 medium pairs (lengths 0, 1, 63, 64, 65, 127–129, 191–193, 300, random up to 400) and large pairs against a prefix-table DP; `string`, `string_view`, `vector<int>`, `vector<char>`, `vector<lng>` extremes, the full byte alphabet, wide integer symbols (non-byte ranking with symbols absent from the longer string), both argument orders. |
| `levenshteinWitness`, `hirschberg` (unit and scored) | Column validity and recomputed cost or score equal to the oracle optimum; the scored version against enumeration (small) and the prefix-table NW (medium). |
| `bandedEditDistance`, `thresholdEditDistance` | Enumerated alignments restricted to the band for every `w` in `0..n+m` (`-1` results included); thresholds at `0, d-1, d, d+1, n+m`; large `k = 25`. |
| `weightedEditDistance` | Enumeration with fixed and random costs in `0..4`, zero costs included. |
| `optimalStringAlignment` | Suffix-form recursion of the OSA definition; relabeling invariance and `DL <= OSA <= Lev` on medium inputs; the `ca -> abc` separation. |
| `damerauLevenshtein` | BFS over strings with adjacent transpositions (true metric), byte and `lng` symbols; relabeling invariance; `{1, 2} -> {2, 1}`. |
| `needlemanWunsch`, `semiGlobalAlignment`, `gotoh` | Enumerated alignments scored by the definitions (free end gaps per row, affine runs per gap type) with fixed and random score tables, gaps and affine parameters, positive values included (non-positive for semi-global). |
| `smithWaterman` | Maximum over all substring pairs of the prefix-table NW; first row-major end; the returned region's NW score equals the score; empty result. |
| Preconditions | Five checked subprocesses require an assertion `SIGABRT`: negative band, negative threshold, negative weighted cost, positive semi-global gap, positive local gap. |
| Large and adversarial | Random 4-letter, near-equal 26-letter (20 substitutions), unary versus half-unary plus a symbol, unbalanced (`50n` versus 5 symbols) and skewed disjoint (`n/16` versus `10n`, the review's Hirschberg memory case) pairs at `n` = 600/3,000/5,000. |

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.1 + 13.8 s, checked 22.2 + 35.9 s, ASan-UBSan 28.3 + 273.5 s. Peak 1090 MB, pre-flight quick 870 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.1 + 14.6 s, checked 24.1 + 42.6 s, ASan-UBSan 29.5 + 171.5 s. Peak 1072 MB, pre-flight quick 928 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 28.0 + 1.6 s, checked 16.3 + 31.5 s. Peak 868 MB. |
| `python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode stress --seed 42` (three suites at a time) | PASS. Optimized 0.1 + 8.8 s, checked 9.5 + 22.3 s, ASan-UBSan 11.8 + 176.4 s. Peak 935 MB, pre-flight quick 53 MB. |

Tester runs 2026-10-09 and 2026-10-25, the latest with case and check counts, on GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`):

```bash
python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode quick --seed 20261009  # PASS, 2 configurations, 1,795 cases and 127,926 checks each, 5 assertion probes
python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode full  # 2026-10-25, 360 s per-command limit: PASS, 3 configurations, 16,446 cases and 1,209,915 checks each, 229 s, MEMORY peak=862MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode full  # 2026-10-25: PASS, 3 configurations, same counts, 320 s, MEMORY peak=771MB
python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode full --configuration ASan-UBSan  # 2026-10-25: PASS alone, 195 s including pre-flight and build
python3 '96-Local Testing/07-Strings/018-editdistance_tester.py' --mode stress --seed 20261010  # PASS, 3 configurations, 17,096 cases and 1,248,763 checks each, MEMORY peak=953MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 114 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=2737MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261009 --no-integration  # PASS, 12 suites
python3 '96-Local Testing/03-consistency.py'  # only a pre-existing error outside this package ('Renamed/new verified suites missing from quick discovery', 16-poly_tester.py, from P231)
```

## Benchmarks

No benchmark is required: there is no dispatch threshold and no Barrett or Montgomery arithmetic. Each function is the direct algorithm for its stated bound; choosing among `levenshtein`, `myersBitVector`, `diagonalEditDistance` and `thresholdEditDistance` is the caller's decision, guided by those bounds.

## Sources

| Source | Use |
|---|---|
| [Wikipedia, Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance), [Edit distance](https://en.wikipedia.org/wiki/Edit_distance) | Definitions, Wagner–Fischer DP, Hirschberg, Ukkonen's banded and threshold algorithm and `O(nd)` diagonals. |
| [Wikipedia, Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) | OSA versus unrestricted definitions, Lowrance–Wagner recurrence, `ca -> abc` example. |
| [Wikipedia, Sequence alignment](https://en.wikipedia.org/wiki/Sequence_alignment), [Gap penalty](https://en.wikipedia.org/wiki/Gap_penalty) | Global, semi-global and local alignment, affine gaps (Gotoh). |
| G. Myers, “A fast bit-vector algorithm for approximate string matching based on dynamic programming”, J. ACM 1999 (as summarized in [curiouscoding.nl](https://curiouscoding.nl/posts/approximate-string-matching/) and the bitap page; the paper was not re-read) | Bit-vector recurrence and the block hand-off of horizontal deltas. |
| maspypy `edit_distance.hpp`, ei1333 `dp/edit-distance.hpp` | API survey only (plain `O(nm)` DP). |

The code was written independently; no source code was copied. The Python subset in `08-Python` (`_012_dp.py`) keeps its own documented reduction.

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- The tester gives each command 1200 s, not the runner default of 720 s (P274, 2026-11-10; first raised to 360 s by user decision 2026-10-25): its ASan-UBSan full run took 586 s in a cold full run beside the other suites ([00-parallel.md](../../96-Local%20Testing/Docs/00-parallel.md)). Alone it takes 108 s with the reused DP table.
- LCS, indel distance and shortest edit scripts are row `19`; approximate occurrence search (bitap, Landau–Vishkin `O(n + d^2)` with LCE) is `17`/`38`; cyclic and semi-local edit distance is `48`.

## Aliases

Search aliases (P263, grab --find): 编辑距离, 莱文斯坦距离, 序列比对, 全局比对, 局部比对, 达梅劳-莱文斯坦距离, 仿射间隙, 史密斯-沃特曼算法.

## History

- 2026-10-09: implemented and verified (P081), independent review found 1 defect and 1 naming issue, fixed.
- 2026-10-25: P247 `/finish` (user decision 2026-10-25): the tester passes `timeout=360` to `07-Strings/_00_runner.py`'s new `timeout` parameter (default 180 s), since its ASan-UBSan full run, which passes alone, exceeded 180 s beside 15 other suites in the parallel full run; `--mode full --configuration ASan-UBSan` passed alone in 195 s, including the pre-flight and the build.
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound, the command limit raised to 1200 s; full and stress inputs and checks unchanged; every library-wide run above passed.
