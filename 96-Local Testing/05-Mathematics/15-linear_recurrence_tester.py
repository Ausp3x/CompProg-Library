"""Linear recurrences against companion-matrix powers, exhaustive minimal recurrences and series division.

C++: berlekampMassey, findLinearRecurrence and findLinearRecurrenceHalfGcd against exhaustive
minimal-length search over GF(2) (n <= 10) and GF(3) (n <= 6), plus validity and equal length
on random recurrences of order up to 1500 (half-GCD past its naive threshold); kitamasa,
fiduccia, nthTermLinearRecurrence, prefixSumLinearRecurrence, consecutiveTerms against direct
iteration and, for 64-bit indices, companion-matrix powers (an augmented matrix for prefix
sums); fiduccia against repeated multiplication by x; rationalGeneratingFunction by series
division; bostanMori and sliceRationalFps (numerator longer than the denominator, trailing
zeros, constant denominator) against series division and the polynomial-part plus recurrence
decomposition; interpolateLinearRecurrence on both sides of the BM/half-GCD sample cutoff (1280 NTT, 6144 other);
interpolatePolyExp against direct evaluation including k = mod; interpolatePeriodicSequence
exhaustively over binary sequences with preperiod and period <= b and on random strings;
linearRecurrenceSemiring over min-plus (saturating, against min-plus matrix powers), max-plus
and boolean semirings; matrixPowerVector against binary matrix powering on random, nilpotent,
identity, low-rank and block matrices; composite modulus 12 status and generic routines.
Moduli: 998244353, 1e9+7, the 64-bit NTT prime 29 * 2^57 + 1, 2, 3, 5, 7. 13 assertion probes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('15-linear_recurrence', ['bm-composite', 'kitamasa-short', 'semiring-short', 'bostan-zero',
        'slice-order', 'slice-zero-q', 'consecutive-negative', 'rational-short', 'periodic-short',
        'periodic-zero-bound', 'polyexp-empty', 'polyexp-long', 'matrix-shape']))
