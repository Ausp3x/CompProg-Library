"""Set and index transforms against brute-force definitions.

C++: subset/superset zeta and Mobius, walshHadamard, or/and/xor/subset convolutions, ranked
zeta/Mobius, disjointUnionConvolution (1, 2 and 3 operands) and the FastConv bitwise methods
for every N = 2^0 .. 2^6 (quick), 2^9 (full) or 2^10 (stress) over int64, 998244353 and 1e9+7
against O(N^2) definitions and an O(3^n) submask brute; walshHadamard inverse over double and
exact __int128; at N = 2^16 (full) or 2^20 (stress) subset convolution against a submask brute
and or/xor convolutions against zeta and sum identities. kroneckerPowerTransform for D = 1..4
against the dense Kronecker power. Divisor/multiple zeta and Mobius, gcd/lcm convolutions with
unequal sizes and the FastConv gcd/lcm methods (sieve and Slow) for every n <= 40/150/300
against divisor-scan brute force, plus 10^6 (full) and 3 * 10^6 (stress) round trips and
spot checks. 14 assertion probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('24-transform_algorithms', ['zeta-size', 'wht-empty', 'or-size', 'xor-size',
        'kronecker-square', 'kronecker-power', 'kronecker-empty', 'ranked-size', 'ranked-mobius',
        'disjoint-empty', 'disjoint-size', 'subset-size', 'gcd-sieve', 'lcm-sieve']))
