"""Floyd/Brent checked against independent first-visit-map orbit references.

All modes exhaust every deterministic map and start through n=3/5/6 for
quick/full/stress, trying every budget through the first successful budget+1.
Structured tails/periods straddle powers of two through exponents 8/14/17;
random maps add 100/3000/20000 rounds. Generic live-state fixtures use periods
1000/100000/500000 and verify constant state storage. Nonrepeating integer orbit
prefixes consume up to 10000/1000000/1000000 evaluations under bounded budgets.
All modes cover full unsigned budgets, exact call accounting, strings,
non-default-constructible states without operator==, congruent custom equality,
move-only callbacks, reentrancy and callback exceptions. Budget zero is valid;
determinism/equivalence are semantic preconditions with no runtime death probes.
Full/stress add ASan/UBSan with leak checking to optimized NDEBUG and checked
configurations. The shared driver reports empty assertion-probe coverage honestly.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('14-cyclefinding', []))
