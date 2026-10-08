"""Grid, Dice and knightDistance checked against brute force and independent models.

Grid: direction tables against the clockwise-turn and DIRS8[2d]==DIRS4[d]
invariants; dirIndex for every byte; inBounds, index and neighbors<4/8>
exhaustively on all n, m <= 4/7/10 (quick/full/stress) including out-of-range
cells, against interval tests and a 3x3 brute-force neighbourhood. Transforms
on all shapes through 6x6 then seeded random grids (100/2000/20000 rounds) of
string, int, bool and lng rows: rotate90 for k in [-9, 7] and INT_MIN/INT_MAX
against repeated per-cell quarter turns, transpose/flipRows/flipCols against
index definitions, involution and dihedral identities, empty and zero-column
grids; pad for k = 0..3 against the bordered definition.
Dice: a physical model (labels on outward normals, moves as rotation matrices,
itself checked for order 4 and a 24-element closure) replays seeded move
sequences (200/3000/30000 dice, distinct, consecutive and repeated labels,
rotate with negative and extreme k); orientations equal the model closure,
canonical is its minimum and invariant under moves, mirror images are
different dice exactly when not reachable.
knightDistance: bounded BFS on |x|, |y| <= 40/120/300 plus margin 8, a second
closed form validated on the same box, then 2000/100000/2000000 random and
extreme coordinates up to 2^62 against that form and board symmetries.
Precondition probes: bad direction/roll letters, ragged grids, negative pad.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('51-grid_utilities', ['dir-char', 'ragged-rotate', 'ragged-pad', 'pad-negative', 'roll-char']))
