# 18-editdistance.hpp — evidence

`18-editdistance.hpp` implements edit distances (unit, weighted, banded, thresholded, output-sensitive, bit-parallel, with witnesses, and both transposition definitions) and score-maximizing sequence alignments (global, semi-global, local, affine).

## Contracts

### Common conventions

- Every function is a template over the sequence type `S`: `string`, `string_view`, `vector<T>` or another sequence with `size()` and `operator[]`; both arguments have the same type. Symbols are compared with `==` (and `<` where ranking is stated). Lengths `n = |a|`, `m = |b|` with `n + m < INT_MAX`; distances are exact `int`.
- Alignments are column lists in left-to-right order: `(i, j)` aligns `a[i]` with `b[j]` (match or substitution), `(i, -1)` deletes `a[i]`, `(-1, j)` inserts `b[j]`; the `i` and `j` entries each enumerate `0..n-1` and `0..m-1` in order.
- Scored functions take `score(x, y) -> lng` (any values) and additive gap scores (`gap`, `open`, `extend`), usually negative; totals must fit in `lng` with headroom (`|total| < 2^61`). They maximize. Cost functions minimize.

### levenshtein, levenshteinWitness, hirschberg

- `levenshtein(a, b)`: unit-cost insert/delete/substitute distance, `O(n m)` time, `O(min(n, m))` space (the shorter string indexes the row).
- `levenshteinWitness(a, b)` returns `{distance, columns}` from a full `(n+1)(m+1)` table (`O(n m)` time and space); traceback from `(n, m)` prefers diagonal, then deletion, then insertion.
- `hirschberg(a, b, score, gap)` returns `{score, columns}` of a maximum global alignment (Needleman–Wunsch witness) in `O(n m)` time and `O(n + m)` space (the two linear rows of a split live only inside `editdistance_detail::split` and are released before recursing, so only `O(log n)` frames of constant size remain; recursion depth `log2(n) + 1`). The split takes the smallest `b` index maximizing forward plus reverse scores; a one-symbol `a` range aligns with the first best `b` symbol or is deleted when every pairing is worse. The returned score is recomputed from the columns.
- `hirschberg(a, b)` is the unit-cost instance (`score = -[x != y]`, `gap = -1`) returning `{distance, columns}`.

### myersBitVector

- Exact Levenshtein distance with Myers' bit-vector recurrence in 64-row blocks (Myers 1999, block form; global distance with top-row horizontal delta `+1`): the longer string is cut into blocks and processed block-major, carrying each column's horizontal delta between blocks in an `int` array. `O(min(n, m) * ceil(max(n, m)/64) + n + m)` time; `O(n + m + sigma)` space where `sigma` is the code range of the longer string's symbols: raw unsigned values (`<= 256`) for 1-byte integral types, otherwise ranks among the longer string's distinct symbols (`O((n + m) log(n+1))` ranking, symbols of the shorter string absent from it get no match bits).

### bandedEditDistance, thresholdEditDistance

- `bandedEditDistance(a, b, w)` (`w >= 0`, asserted): minimum unit cost over alignments whose cells all satisfy `|i - j| <= w`; `-1` when `|n - m| > w` (no such alignment). Exact whenever the distance is at most `w` (an alignment of cost `d` never leaves the band `|i - j| <= d`). `O(max(n, m) * min(2w + 1, min(n, m) + 1))` time, `O(min(n, m))` space.
- `thresholdEditDistance(a, b, k)` (`k >= 0`, asserted) returns `min(distance, k + 1)`: the band `w = k` with an early exit once a whole row exceeds `k` (every alignment of cost `<= k` crosses each row inside the band at a cell of value `<= k`), and immediately when `|n - m| > k`. Same bounds as the band.

### diagonalEditDistance

- Exact Levenshtein distance `d` by furthest-reaching points on diagonals `k = j - i` (Ukkonen 1985, Myers 1986, with substitutions): round `e` extends round `e - 1` by substitution, deletion or insertion and slides along matches. Ukkonen's cutoff keeps only diagonals with `e + |k - (m - n)| <= max(n, m)`; the cut set is closed under successors and contains every optimal path, so the answer is exact, and at most `min(n, m) + 1` diagonals are live per round. `O((d + 1) * (min(n, m) + 1))` time (never worse than the table DP; fast for similar strings), `O(n + m)` space.

### weightedEditDistance

- `weightedEditDistance(a, b, ins, del, sub)` with costs `>= 0` (asserted): inserting `b[j]` costs `ins`, deleting `a[i]` `del`, substituting unequal symbols `sub`, matches are free. `O(n m)` time, `O(m)` space, exact `lng`.

### optimalStringAlignment, damerauLevenshtein

- `optimalStringAlignment(a, b)`: restricted Damerau–Levenshtein (OSA): unit insert/delete/substitute plus swapping two adjacent symbols when no substring is edited twice. `O(n m)` time, `O(m)` space. Not a metric (`"ca" -> "abc"` is 3).
- `damerauLevenshtein(a, b)`: unrestricted Damerau–Levenshtein (Lowrance–Wagner), the true metric over insert, delete, substitute and adjacent transposition (`"ca" -> "abc"` is 2). `O(n m + (n + m) log(n+1))` time (the log term only for non-byte symbols), `O(n m)` space. `DL <= OSA <= Levenshtein`.

### needlemanWunsch, semiGlobalAlignment, smithWaterman, gotoh

- `needlemanWunsch(a, b, score, gap)`: maximum global alignment score, each gap column adds `gap`. `O(n m)` time, `O(m)` space. Witness: `hirschberg(a, b, score, gap)`.
- `semiGlobalAlignment(a, b, score, gap)` (`gap <= 0`, asserted): gaps in a string's row before its first symbol or after its last symbol score 0; the result is at least 0. `O(n m)` time, `O(m)` space. With a positive gap, continuing past a free end would outscore the free end and the definition degenerates, hence the precondition.
- `smithWaterman(a, b, score, gap)` (`gap <= 0`, asserted) returns `{score, la, ra, lb, rb}`: the maximum over substring pairs of the global alignment score of `a[la, ra)` and `b[lb, rb)`, with `ra, rb` the first maximum in row-major order of end cells; the start follows predecessors preferring diagonal, up, left, then a fresh start, so the region's global score equals `score`. `{0, 0, 0, 0, 0}` when no alignment scores above 0. `O(n m)` time, `O(m)` space.
- `gotoh(a, b, score, open, extend)`: maximum global alignment with affine gaps; a maximal run of `L` consecutive insertions (or deletions) adds `open + extend * L`; an insertion run next to a deletion run opens twice. Three linear rows: `O(n m)` time, `O(m)` space; unreachable states use `NEG = LLONG_MIN / 4`.

Correctness: each DP evaluates the standard recurrence whose states are prefix pairs; the min (max) over the three or four predecessor moves is the min (max) over alignments ending with that column, so the final cell is the optimum over all alignments. Hirschberg's split is valid because any alignment crosses the middle row of `a` at some column of `b`, and forward plus reversed prefix scores give the best alignment through each crossing. Myers' recurrence encodes the vertical and horizontal `+-1` deltas of the DP table, and the block hand-off passes exactly the horizontal delta at a block's bottom row.

## Feature-to-test map

Runnable entry: [18-editdistance_tester.py](<../../96-Local Testing/07-Strings/18-editdistance_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/18-editdistance_tester.cpp>). All oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| `levenshtein`, `myersBitVector`, `diagonalEditDistance` | Minimum over explicitly enumerated alignments, itself checked against BFS over strings with insert/delete/substitute (every exhaustive pair, every fourth random pair); every ternary pair through lengths 3/4/4 (quick/full/stress) and 150/1,500/2,000 random pairs up to length 5 over 1–4 letters; 40/300/450 medium pairs (lengths 0, 1, 63, 64, 65, 127–129, 191–193, 300, random up to 400) and large pairs against a prefix-table DP; `string`, `string_view`, `vector<int>`, `vector<char>`, `vector<lng>` extremes, full byte alphabet, wide integer symbols (non-byte ranking with symbols absent from the longer string), both argument orders. |
| `levenshteinWitness`, `hirschberg` (unit and scored) | Column validity and recomputed cost/score equal to the oracle optimum; scored version against enumeration (small) and the prefix-table NW (medium). |
| `bandedEditDistance`, `thresholdEditDistance` | Enumerated alignments restricted to the band for every `w` in `0..n+m` (including `-1` results); thresholds at `0, d-1, d, d+1, n+m`; large `k = 25`. |
| `weightedEditDistance` | Enumeration with fixed and random costs in `0..4` (including zero costs). |
| `optimalStringAlignment` | Suffix-form recursion of the OSA definition; relabeling invariance and `DL <= OSA <= Lev` on medium inputs; the `ca -> abc` separation. |
| `damerauLevenshtein` | BFS over strings with adjacent transpositions (true metric), byte and `lng` symbols; relabeling invariance; `{1, 2} -> {2, 1}`. |
| `needlemanWunsch`, `semiGlobalAlignment`, `gotoh` | Enumerated alignments scored by the definitions (free end gaps per row; affine runs per gap type) with fixed and random score tables, gaps and affine parameters including positive values (non-positive for semi-global). |
| `smithWaterman` | Maximum over all substring pairs of the prefix-table NW; first row-major end; the returned region's NW score equals the score; empty result. |
| Preconditions | Five checked subprocesses require assertion `SIGABRT`: negative band, negative threshold, negative weighted cost, positive semi-global gap, positive local gap. |
| Large/adversarial | Random 4-letter, near-equal 26-letter (20 substitutions), unary versus half-unary plus a symbol, unbalanced (`50n` versus 5 symbols) and skewed disjoint (`n/16` versus `10n`, the review's Hirschberg memory case) pairs at `n` = 600/3,000/5,000. |

## Commands and results

Run 2026-10-09 on an 11th Gen Intel Core i9-11900H with GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`). Configurations (shared string runner): optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` (with the assertion probes), and in full/stress `-O1 -g -fsanitize=address,undefined`; all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress runs passed the quick pre-flight under the 2048 MB cap; `MEMORY peak` is the run's capped peak (4096 MB cap, 0 OOM kills).

```bash
python3 '96-Local Testing/07-Strings/18-editdistance_tester.py' --mode quick --seed 20261009  # PASS, 2 configurations, 1,795 cases and 127,926 checks each, 5 assertion probes
python3 '96-Local Testing/07-Strings/18-editdistance_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, 16,446 cases and 1,209,818 checks each, MEMORY peak=744MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/18-editdistance_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, same counts, MEMORY peak=768MB
python3 '96-Local Testing/07-Strings/18-editdistance_tester.py' --mode stress --seed 20261010  # PASS, 3 configurations, 17,096 cases and 1,248,763 checks each, MEMORY peak=953MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 114 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=2737MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261009 --no-integration  # PASS, 12 suites
python3 '96-Local Testing/03-consistency.py'  # only the pre-existing, out-of-scope error 'Renamed/new verified suites missing from quick discovery' (16-poly_tester.py, introduced by P231)
```

## Benchmarks

No benchmark is required: the profile has no dispatch thresholds here and no Barrett/Montgomery arithmetic. Each function is the direct algorithm for its stated bound; choosing among `levenshtein`, `myersBitVector`, `diagonalEditDistance` and `thresholdEditDistance` is a caller decision guided by those bounds.

## Sources

| Source | Use |
|---|---|
| [Wikipedia, Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance), [Edit distance](https://en.wikipedia.org/wiki/Edit_distance) | Definitions, Wagner–Fischer DP, Hirschberg, Ukkonen's banded/threshold algorithm and `O(nd)` diagonals. |
| [Wikipedia, Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) | OSA versus unrestricted definitions, Lowrance–Wagner recurrence, `ca -> abc` example. |
| [Wikipedia, Sequence alignment](https://en.wikipedia.org/wiki/Sequence_alignment), [Gap penalty](https://en.wikipedia.org/wiki/Gap_penalty) | Global, semi-global and local alignment, affine gaps (Gotoh). |
| G. Myers, “A fast bit-vector algorithm for approximate string matching based on dynamic programming”, J. ACM 1999 (as summarized in [curiouscoding.nl](https://curiouscoding.nl/posts/approximate-string-matching/) and the bitap page; the paper was not re-read) | Bit-vector recurrence and the block hand-off of horizontal deltas. |
| maspypy `edit_distance.hpp`, ei1333 `dp/edit-distance.hpp` | API survey only (plain `O(nm)` DP). |

The code was written independently; no source code was copied. The Python subset in `08-Python` (`_12_dp.py`) keeps its own documented reduction.

## Limits and handoffs

- LCS, indel distance and shortest edit scripts are `19`; approximate occurrence search (bitap, Landau–Vishkin `O(n + d^2)` with LCE) is `17`/`38`; cyclic and semi-local edit distance is `48`.

## History

- 2026-10-09: implemented and verified (P081). Independent review: Hirschberg rows held across recursion (`O(n + m log n)` memory) fixed by the `split` helper; Gotoh locals renamed.
