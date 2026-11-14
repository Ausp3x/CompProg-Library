"""Ordinary sieve features against independent trial/divisor/gcd oracles.

Quick checks every prefix 0..32, every integer through 2500 and 100 random
queries at 100000. Full checks prefixes 0..128, every integer through 50000
and 1500 random queries at 1000000. Stress raises these bounds to 250000,
6000 and 5000000. Every exhaustive table checks prime powers/divisors by
trial division; tau/sigma by direct multiple accumulation; phi/mu by
independent divisor-sum inversion. Small phi values also use gcd counting.
primeCount is checked at every value of every exhaustive prefix (and -2, -1)
against a running trial-division count, and at random points against a
linear prime scan. The byte, odd-bit and SPF Eratosthenes sieves are checked at
every value against trial division for every small prefix and at segment edges,
against linear SPF at the large bound, and the bit sieve at 10^7, 10^9 or 2^32 - 1.
Table builder order, repeat/reset/copy/move behavior, all public methods and
28 precondition probes are included. Full/stress exercise all three builds.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    invalid = [f'{kind}-{bound}' for kind in ('erath','linear','erath-reset','linear-reset','erathspf','erathspf-reset','bit','bit-reset')
               for bound in ('negative','max')]
    invalid += [f'{kind}-{bound}' for kind in ('primefac','allfac') for bound in ('negative','zero','end')]
    invalid += ['erath-count-end', 'linear-count-end', 'bit-prime-negative', 'bit-prime-end', 'bit-count-end', 'bit-above-max']
    raise SystemExit(main('04-sieve_algorithms', invalid))
