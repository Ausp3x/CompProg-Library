"""Wildcard and glob matching: brute window scans, a glob DP validated by backtracking.

Every case compares wildcardMatch (Monte Carlo, seeded) and
wildcardMatchDeterministic with a brute scan over every start, and globMatch
plus GlobAutomaton (every prefix, reset) with the glob DP, itself checked by
backtracking on tiny inputs. Quick/full/stress enumerate texts over {a,b,?}
through length 5/7/8 against patterns through 3/4/5 and binary texts through
4/6/7 against glob patterns over {a,b,?,*} through 4/5/6 symbols, then
150/1200/6000 random wildcard cases (cheap random texts that stay in the
budgeted scan, unary texts and late unary tails that hand over to the block
convolution, at least a tenth of the cases asserted to hand over; m around the
budget 14..19, single and multiple blocks), 10/60/60 int cases with up to 2000
distinct symbols asserted to hand over, lng symbols with LLONG_MIN as the
wildcard, high bytes with NUL as the wildcard and int symbols with 0,
all-wildcard texts and patterns, 150/1500/6000
random globs with KMP, naive and convolution segments, consecutive and edge
stars, custom metacharacters, 63..129-symbol automata, 4/20 multi-block
searches in 3000..6000-symbol texts and as many unary texts and 60-byte-alphabet
segments asserted to hand over to the blocks, and large texts of
30000/300000/1500000 symbols (random, periodic with an analytic oracle,
planted globs). Builds use -Wall -Wextra -Wshadow -Wconversion -Werror
(-Wshadow is suppressed only around 06-Miscellaneous/02-customhash.hpp).
Full/stress add ASan/UBSan; checked builds run three precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('21-wildcardmatching', ['pattern-limit', 'deterministic-limit', 'glob-limit'], strict=True))
