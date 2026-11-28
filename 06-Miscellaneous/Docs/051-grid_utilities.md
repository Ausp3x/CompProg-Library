# 051-grid_utilities.hpp — Evidence

`051-grid_utilities.hpp` (batch MI32, package P229) provides grid direction tables and transforms, a dice value type with fixed orientation conventions, and the closed-form knight distance. The row's `missing:` operation `Dice::orientationGraph` is not implemented. Prerequisite: the C01 template (P002); independent contest-profile code.

## Contracts

### Grid

Coordinates: in cell `(i, j)` the row `i` grows downward and the column `j` rightward, as text input is read. `DIRS4 = {(0,1) right/east, (1,0) down/south, (0,-1) left/west, (-1,0) up/north}` runs clockwise, so direction `d` turned clockwise is `(d + 1) % 4` and reversed is `d ^ 2`. `DIRS8` runs clockwise from right in 45° steps and satisfies `DIRS8[2 * d] == DIRS4[d]`. `dirIndex(c)` maps `R/E`, `D/S`, `L/W`, `U/N` to `0..3` and asserts on any other byte. `inBounds(i, j, n, m)` is `0 <= i < n && 0 <= j < m` for any `int` arguments. `index(i, j, m) = i * m + j` is the row-major id; the caller keeps `n * m` within `int`. `neighbors<K>(i, j, n, m, f)` calls `f(x, y)` for the in-bounds neighbours of `(i, j)` in `DIRS4` (`K = 4`) or `DIRS8` (`K = 8`) order; `(i, j)` itself need not be in bounds.

Transforms take a rectangular grid `G` whose rows support `size()`, `operator[]` and construction `Row(len, value)`, such as `vector<string>` and `vector<vector<T>>` including `vector<bool>`. They return a new grid of the same type and assert that every row has the length of row 0 (a ragged grid is a precondition failure). `rotate90(a, k = 1)` turns by `k` clockwise quarter turns for any `int k` (negative and extreme values reduce modulo 4); one clockwise turn sends `a[i][j]` to `res[j][n-1-i]`. `transpose` sends it to `res[j][i]`, `flipRows` reverses the row order (`res[n-1-i][j]`, a vertical mirror) and `flipCols` reverses each row (`res[i][m-1-j]`, a horizontal mirror). All four share `remap(a, t, fi, fj)`, public but an implementation detail. `pad(a, k, fill)` returns the `(n + 2k) x (m + 2k)` grid with `a` at offset `(k, k)` and `fill` elsewhere; `k >= 0` is asserted. Degenerate shapes: a grid with zero rows has no column count, so it transforms to an empty grid and pads to `2k x 2k`; a grid of `n > 0` rows with zero columns keeps `n` empty rows under `flipRows`, `flipCols` and half turns, and becomes empty under `transpose` and quarter turns, since zero rows of length `n` cannot be represented. Costs: `O(n * m)` per transform and `O(out)` for `pad`.

### Dice

`Dice` holds `f = {top, front, right, left, back, bottom}` (indices `TOP..BOTTOM`), the AOJ input order, default `{1,2,3,4,5,6}`, where opposite faces are `i` and `5 - i`. The viewer stands south looking north: `front` faces the viewer, `back` faces north, `right` faces east. `roll(c)` tips the die one cell toward `N`, `S`, `E` or `W`: rolling north brings the front face to the top and the top face to the back; rolling east brings the left face to the top and the top face to the right. `rotate(k = 1)` spins the die in place by `k` clockwise quarter turns seen from above (the back face moves to the right, the right face to the front), for any `int k`. Both return `*this` for chaining; `roll` asserts on any other letter. The accessors return labels; `f` is public. `orientations()` lists the 24 orientations reachable by rotation, starting with the die itself, grouped as six faces on top times four spins; with repeated labels some entries coincide. `canonical()` is the lexicographically smallest `f` among them, so two dice are the same die up to rotation exactly when their canonical forms are equal; the mirror image (left and right swapped) of a die with distinct labels is a different die. `operator==` compares the exact orientation.

### knightDistance

`knightDistance(x, y)` is the minimum number of knight moves from `(0, 0)` to `(x, y)` on an unbounded board, for `|x|, |y| <= 2^62`. Formula, with `x >= y >= 0` after symmetry: `(1, 0) -> 3`, `(2, 2) -> 4`; otherwise, with `d = x - y`, `d + 2 * ceil((y - d) / 3)` when `y > d` and `d - 2 * floor((d - y) / 4)` otherwise. Correctness: every move changes `x + y` by an odd amount and `max(|x|, |y|)` by at most 2, so `max(ceil(x / 2), ceil((x + y) / 3))`, adjusted to the parity of `x + y`, is a lower bound; the formula attains it except at the two exceptions near the origin, where the bound is unreachable. The closed form is checked against BFS on a bounded box (optimal paths never leave the endpoints' bounding box by more than two cells, so a margin of 8 is safe) and against an independent lower-bound-with-parity form at large coordinates. Intermediate values stay below `2^63`.

## Feature-to-Test Map

| Public feature | Coverage |
|---|---|
| `DIRS4`, `DIRS8`, `dirIndex` | Clockwise-turn identity `DIRS4[d+1] = (dy, -dx)`, unit and king steps, `DIRS8[2d] == DIRS4[d]`, consecutive `DIRS8` cross products −1, eight distinct entries; `dirIndex` on every valid byte against the letter table; `dir-char` assertion probe; `static_assert` constexpr use. |
| `inBounds`, `index`, `neighbors<4/8>` | Exhaustive for all `n, m <= 4`, 7 or 10 (quick, full, stress) and all cells in `[-2, n+2) x [-2, m+2)`: interval test, a row-major bijection hitting every cell once, neighbours in `DIRS` order against the direction table and as a set against a 3x3 brute force. |
| `rotate90`, `transpose`, `flipRows`, `flipCols` | All shapes through 6x6 and 100, 2,000 or 20,000 seeded random grids of `string`, `int`, `bool` and `lng` rows: `rotate90` for k in {−9, −5, …, 7, `INT_MIN`, `INT_MIN+1`, `INT_MAX−1`, `INT_MAX`} against repeated per-cell clockwise quarter turns computed independently; transpose and flips against their index definitions; involutions; `rotate90 == flipCols ∘ transpose`; half turn `== flipRows ∘ flipCols`; empty and zero-column grids; `ragged-rotate` assertion probe. Swapping the flip flags fails the quarter-turn test. |
| `pad` | Bordered definition for `k = 0..3` on every grid above with `'#'`, `-7`, `true`, `LLONG_MIN`; `ragged-pad` and `pad-negative` probes. |
| `Dice` roll, rotate, accessors, `operator==` | Physical model (labels on outward normals, moves as integer rotation matrices), itself checked for order-4 moves and a 24-element closure; 200, 3,000 or 30,000 seeded dice with distinct, consecutive and repeated labels replay up to 40 random moves, including `rotate` with k in [−5, 5], `INT_MIN`, `INT_MAX`; opposite rolls cancel; chaining returns `*this`; explicit checks of `roll('N')`, `roll('E')` and `rotate()` on the default die; `roll-char` probe. |
| `orientations`, `canonical` | The orientation set equals the model closure (24 for distinct labels), the first entry is the die, `canonical` is the closure minimum and is invariant under random moves, and it equals the mirror image's exactly when the mirror is in the closure (never for distinct labels). |
| `knightDistance` | Bounded BFS on absolute coordinates up to 40, 120 or 300 with margin 8, the alternative lower-bound form validated on the same box, then 2,000, 100,000 or 2,000,000 random coordinates up to `±2^62` (uniform, near `y = x/2`, and small `y`) plus the corner cases `(±2^62, ±2^62)`, `(2^62, 0)`, `(2^62, 2^62−1)` against the alternative form and the eight board symmetries; `static_assert` constexpr values. |

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 844 MB; optimized 0.2 s compile + 0.8 s run, checked 17.2 s + 1.2 s, ASan-UBSan 17.9 s + 18.3 s; peak 1073 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 901 MB; optimized 0.3 s + 1.4 s, checked 30.0 s + 1.7 s, ASan-UBSan 33.6 s + 24.2 s; peak 1073 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 35.2 s + 0.1 s, checked 24.9 s + 1.6 s; peak 835 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 850 MB; optimized 0.1 s + 3.4 s, checked 7.4 s + 5.6 s, ASan-UBSan 9.0 s + 66.9 s; peak 806 MB |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/051-grid_utilities_tester.py' --mode quick --seed 1` | PASS, 2 configurations, 195,415 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/051-grid_utilities_tester.py' --mode full --seed 1` | PASS, 3 configurations, 3,673,126 checks, 5 assertion probes |
| 2026-10-08 (P229) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/051-grid_utilities_tester.py' --mode full --seed 2` | PASS, 3 configurations, 3,639,867 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/06-Miscellaneous/051-grid_utilities_tester.py' --mode stress --seed 3` | PASS, 3 configurations, 38,748,887 checks |
| 2026-10-08 (P229) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS; the header is warning-free with `-Wall -Wextra -Wconversion` on GCC 16.2 and 14.4 in a two-TU build through `999-all.hpp` |
| 2026-10-08 (P229) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

None: every operation is `O(1)` or a single pass over its output.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); the code was written independently:

- maspypy [`other/dice.hpp`](https://maspypy.github.io/library/other/dice.hpp) and [`other/knight_distance.hpp`](https://maspypy.github.io/library/other/knight_distance.hpp), ei1333 [`other/dice.hpp`](https://ei1333.github.io/library/other/dice.hpp): face conventions, the all-orientation enumeration (adopted as `orientations` and `canonical`), the O(1) knight formula.
- hitonanode [`utilities/rotate90.hpp`](https://hitonanode.github.io/cplib-cpp/utilities/rotate90.hpp), suisen [`util/rot90.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/util/rot90.hpp) and [`util/grid_utils.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/util/grid_utils.hpp): both rotate pages implement `B[j][i] = A[i][W-1-j]`, a counterclockwise turn described as clockwise; this header fixes the direction as clockwise and tests it.

## Limits and Handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): multidimensional index, Manhattan walk, grid BFS and parsing (Graphs `43`), Chebyshev and Manhattan conversions and king distance (Geometry `21`), bounded-board knight distance (BFS).
- Face labels are `int`; `index` assumes `n * m` fits an `int`.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 方向数组, 网格, 矩阵旋转, 矩阵转置, 骰子旋转, 骰子模拟, 马的最少步数, 八方向.

## History

- 2026-10-08: created and verified (P229).
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
