"""Ordinary sieve features against independent trial/divisor/gcd oracles.

Quick checks every prefix 0..32, every integer through 2500 and 100 random
queries at 100000. Full checks prefixes 0..128, every integer through 50000
and 1500 random queries at 1000000. Stress raises these bounds to 250000,
6000 and 5000000. Every exhaustive table checks prime powers/divisors by
trial division; tau/sigma by direct multiple accumulation; phi/mu by
independent divisor-sum inversion. Small phi values also use gcd counting.
Table builder order, repeat/reset/copy/move behavior, all public methods and
14 precondition probes are included. Full/stress exercise all three builds.
"""
from _00_runner import main

if __name__ == '__main__':
    invalid = [f'{kind}-{bound}' for kind in ('erath','linear','erath-reset','linear-reset')
               for bound in ('negative','max')]
    invalid += [f'{kind}-{bound}' for kind in ('primefac','allfac') for bound in ('negative','zero','end')]
    raise SystemExit(main('04-sieve_algorithms', invalid))
