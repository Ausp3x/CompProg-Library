# 51-grid_utilities.hpp — evidence

`51-grid_utilities.hpp` (batch MI32, package P229) provides grid direction tables and transforms, a dice value type with fixed orientation conventions, and the closed-form knight distance. Prerequisite: the C01 template (P002). Independent contest-profile code; no online submission was made.

## Contracts

### Grid

Coordinates: cell `(i, j)` has row `i` growing downward and column `j` growing rightward, as text input is read. `DIRS4 = {(0,1) right/east, (1,0) down/south, (0,-1) left/west, (-1,0) up/north}` runs clockwise, so direction `d` turned clockwise is `(d + 1) % 4` and reversed is `d ^ 2`. `DIRS8` runs clockwise from right in 45° steps and satisfies `DIRS8[2 * d] == DIRS4[d]`. `dirIndex(c)` maps `R/E`, `D/S`, `L/W`, `U/N` to `0..3` and asserts on any other byte. `inBounds(i, j, n, m)` is `0 <= i < n && 0 <= j < m` for any `int` arguments. `index(i, j, m) = i * m + j` is the row-major id (caller keeps `n * m` within `int`). `neighbors<K>(i, j, n, m, f)` calls `f(x, y)` for the in-bounds neighbours of `(i, j)` in `DIRS4` (`K = 4`) or `DIRS8` (`K = 8`) order; `(i, j)` itself need not be in bounds.

Transforms take a rectangular grid `G` whose rows support `size()`, `operator[]` and construction `Row(len, value)`: `vector<string>`, `vector<vector<T>>` including `vector<bool>`. They return a new grid of the same type and assert that every row has the length of row 0 (a ragged grid is a precondition failure). `rotate90(a, k = 1)` turns by `k` quarter turns clockwise for any `int k` (negative and extreme values reduce modulo 4): one clockwise turn sends `a[i][j]` to `res[j][n-1-i]`. `transpose` sends it to `res[j][i]`, `flipRows` reverses the row order (`res[n-1-i][j]`, a vertical mirror) and `flipCols` reverses each row (`res[i][m-1-j]`, a horizontal mirror). All four share `remap(a, t, fi, fj)`, which is public but is an implementation detail. `pad(a, k, fill)` returns the `(n + 2k) x (m + 2k)` grid with `a` at offset `(k, k)` and `fill` elsewhere; `k >= 0` is asserted. Degenerate shapes: a grid with zero rows has no column count, so it transforms to an empty grid and pads to `2k x 2k`; a grid with `n > 0` rows of zero columns keeps `n` empty rows under `flipRows`, `flipCols` and half turns, and becomes empty under `transpose` and quarter turns (zero rows of length `n` cannot be represented). Costs are `O(n * m)` per transform and `O(out)` for `pad`.

### Dice

`Dice` holds `f = {top, front, right, left, back, bottom}` (indices `TOP..BOTTOM`), the AOJ input order, default `{1,2,3,4,5,6}` where opposite faces are `i` and `5 - i`. The viewer stands south looking north: `front` faces the viewer, `back` faces north, `right` faces east. `roll(c)` tips the die one cell toward `N`, `S`, `E` or `W`: rolling north brings the front face to the top and the top face to the back; rolling east brings the left face to the top and the top face to the right. `rotate(k = 1)` spins the die in place by `k` quarter turns clockwise as seen from above (the back face moves to the right, the right face to the front), any `int k`. Both return `*this` for chaining; `roll` asserts on any other letter. Accessors return the labels; `f` is public. `orientations()` lists the 24 orientations reachable by rotation, starting with the die itself, grouped as six faces on top times four spins; with repeated labels some entries coincide. `canonical()` is the lexicographically smallest `f` among them, so two dice are the same die up to rotation exactly when their canonical forms are equal; a mirror image (left and right swapped) of a die with distinct labels is a different die. `operator==` compares exact orientation.

### knightDistance

`knightDistance(x, y)` is the minimum number of knight moves from `(0, 0)` to `(x, y)` on an unbounded board for `|x|, |y| <= 2^62`. Formula: with `x >= y >= 0` after symmetry, `(1, 0) -> 3`, `(2, 2) -> 4`, otherwise with `d = x - y`: `d + 2 * ceil((y - d) / 3)` when `y > d`, else `d - 2 * floor((d - y) / 4)`. Correctness argument: every move changes `x + y` by an odd amount and `max(|x|, |y|)` by at most 2, so `max(ceil(x / 2), ceil((x + y) / 3))` with the parity of `x + y` is a lower bound; the formula attains it except at the two exceptions, where the bound is unreachable near the origin. The closed form is verified against BFS on the bounded box (optimal paths never leave the bounding box of the endpoints by more than two cells, so margin 8 is safe) and against an independent lower-bound-with-parity form at large coordinates. Intermediate values stay below `2^63`.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| `DIRS4`, `DIRS8`, `dirIndex` | Clockwise-turn identity `DIRS4[d+1] = (dy, -dx)`, unit and king steps, `DIRS8[2d] == DIRS4[d]`, consecutive `DIRS8` cross products −1, eight distinct entries; `dirIndex` on every valid byte against the letter table; `dir-char` assertion probe; `static_assert` constexpr use. |
| `inBounds`, `index`, `neighbors<4/8>` | Exhaustive for all `n, m <= 4/7/10` (quick/full/stress) and all cells in `[-2, n+2) x [-2, m+2)`: interval test, row-major bijection hitting every cell once, neighbours in `DIRS` order against the direction table and as a set against a 3x3 brute force. |
| `rotate90`, `transpose`, `flipRows`, `flipCols` | All shapes through 6x6 and 100/2,000/20,000 seeded random grids of `string`, `int`, `bool` and `lng` rows: `rotate90` for k in {−9, −5, …, 7, `INT_MIN`, `INT_MIN+1`, `INT_MAX−1`, `INT_MAX`} against repeated per-cell clockwise quarter turns computed independently, transpose/flips against index definitions, involutions, `rotate90 == flipCols ∘ transpose`, half turn `== flipRows ∘ flipCols`, empty and zero-column grids; `ragged-rotate` assertion probe. Mutation check: swapping the flip flags fails the quarter-turn test. |
| `pad` | Bordered definition for `k = 0..3` on every grid above with `'#'`, `-7`, `true`, `LLONG_MIN`; `ragged-pad` and `pad-negative` probes. |
| `Dice` roll, rotate, accessors, `operator==` | Physical model (labels on outward normals, moves as integer rotation matrices), itself checked for order-4 moves and a 24-element closure; 200/3,000/30,000 seeded dice with distinct, consecutive and repeated labels replay up to 40 random moves, including `rotate` with k in [−5, 5], `INT_MIN`, `INT_MAX`; opposite rolls cancel; chaining returns `*this`; explicit checks of `roll('N')`, `roll('E')` and `rotate()` on the default die; `roll-char` probe. |
| `orientations`, `canonical` | The orientation set equals the model closure (24 for distinct labels), the first entry is the die, `canonical` is the closure minimum, invariant under random moves, and equal for a mirror image exactly when the mirror is in the closure (never for distinct labels). |
| `knightDistance` | Bounded BFS on absolute coordinates up to 40/120/300 with margin 8, the alternative lower-bound form validated on the same box, then 2,000/100,000/2,000,000 random coordinates up to `±2^62` (uniform, near `y = x/2`, and small `y`) plus the corner cases `(±2^62, ±2^62)`, `(2^62, 0)`, `(2^62, 2^62−1)` against the alternative form and the eight board symmetries; `static_assert` constexpr values. |

## Commands and results

Run 2026-10-08: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations as in [49-timer.md](49-timer.md).

```bash
python3 '96-Local Testing/06-Miscellaneous/51-grid_utilities_tester.py' --mode quick --seed 1             # PASS, 2 configurations, 195,415 checks
python3 '96-Local Testing/06-Miscellaneous/51-grid_utilities_tester.py' --mode full --seed 1              # PASS, 3 configurations, 3,673,126 checks, 5 assertion probes
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/51-grid_utilities_tester.py' --mode full --seed 2   # PASS, 3 configurations, 3,639,867 checks
python3 '96-Local Testing/06-Miscellaneous/51-grid_utilities_tester.py' --mode stress --seed 3            # PASS, 3 configurations, 38,748,887 checks
python3 '96-Local Testing/02-integration.py' --sanitizers                                                 # PASS
python3 '96-Local Testing/03-consistency.py'                                                              # no errors
```

Warning-free with `-Wall -Wextra -Wconversion` under GCC 16.2 and 14.4 in a two-translation-unit build through `99-all.hpp`.

## Benchmarks

None: every operation is `O(1)` or a single pass over its output.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); code written independently:

- maspypy [`other/dice.hpp`](https://maspypy.github.io/library/other/dice.hpp) and [`other/knight_distance.hpp`](https://maspypy.github.io/library/other/knight_distance.hpp), ei1333 [`other/dice.hpp`](https://ei1333.github.io/library/other/dice.hpp): face conventions, all-orientation enumeration (adopted as `orientations`/`canonical`), the O(1) knight formula.
- hitonanode [`utilities/rotate90.hpp`](https://hitonanode.github.io/cplib-cpp/utilities/rotate90.hpp), suisen [`util/rot90.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/util/rot90.hpp) and [`util/grid_utils.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/util/grid_utils.hpp): both rotate pages implement `B[j][i] = A[i][W-1-j]`, a counterclockwise turn described as clockwise; this header fixes clockwise and tests the direction.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): dice orientation graph, multidimensional index, Manhattan walk, grid BFS/parsing (Graphs `43`), Chebyshev/Manhattan conversions and king distance (Geometry `21`), bounded-board knight distance (BFS).
- Face labels are `int`; `index` assumes `n * m` fits an `int`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-08: created and verified (P229).
