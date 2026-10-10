"""Polynomial and power-series algorithms against independent brute force and uniqueness identities.

C++: basic operations against coefficient loops; polyDivmod by the division identity and long
division across the naive/Newton threshold (up to 4e5 coefficients optimized); fpsInverse,
fpsDiv, fpsLog, fpsExp and the sparse variants by the identities a g = 1, a b' = a', f' = f a'
(O(n^2) checks to n = 2049, transform-checked to 2^18 optimized, 2^20 stress) and log(exp)
round trips; fpsPow (integer, negative, LLONG_MIN, rational) and fpsSparsePow against binary
powering with truncated schoolbook products; fpsSqrt and fpsSparseSqrt by squaring, branch
choice and Euler-criterion existence; polyGcd, polyXgcd and polyHalfGcd against an extended
Euclid by long division across the half-GCD threshold, with a 6e4-degree identity check;
polyResultant and polyDiscriminant against Sylvester determinants (formal degree n - 1 for the
discriminant) and root-product formulas; polyPowMod, polyComposeMod and polyInverseMod against
repeated long division; fpsComposition and fpsCompositionalInverse against Horner composition;
taylorShift, productTree, polyFromRoots, egfConvolution, powerSumsOfRoots (Newton's identities),
polyFromPowerSums, prefixSumOfPolynomial (sampled prefix sums); cyclotomicPolynomial and
cyclotomicPolynomialMod for every n <= 3000 against the exact identities Phi_pm = Phi_m(x^p) or
Phi_m(x^p) / Phi_m(x) with 128-bit division, plus large n. Moduli: 998244353, 1e9+7, a dynamic
prime, the 64-bit NTT prime 29 * 2^57 + 1 and small primes 3, 5, 7, 97 at the n <= mod edges.
28 assertion probes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('14-polynomial_algorithms', ['inverse-zero', 'sparse-inverse-zero', 'log-constant',
        'sparse-log-constant', 'log-long', 'exp-constant', 'sparse-exp-constant', 'pow-negative-valuation', 'pow-long',
        'pow-rational-constant', 'pow-rational-q', 'sparse-pow-negative', 'sqrt-even-modulus', 'divmod-zero',
        'halfgcd-degree', 'discriminant-constant', 'inversemod-constant', 'powmod-zero', 'composemod-zero',
        'compinv-linear', 'composition-negative', 'egf-long', 'integral-long', 'cyclotomic-zero', 'powersums-zero',
        'frompowersums-empty', 'truncate-negative', 'prefix-sum-long']))
