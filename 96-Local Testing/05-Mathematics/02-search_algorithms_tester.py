"""Search against exhaustive monotone arrays, brute-force minima and analytic roots.

Quick exhausts small endpoints and unimodal arrays through length 6, then 100
random cases. Full covers length 8 and 3000 random cases; stress covers length 9
and 30000 cases. All modes test full signed endpoints and finite-double extremes,
subnormals, adjacent points, precision/call bounds, and every public API:
geometric bisection on analytic thresholds from denorm_min to DBL_MAX in both
signs and orientations, bit-pattern bisection across zero against an
arithmetic IEEE ordinal, Brent minimization on V shapes, plateaus, endpoint and
smooth minima, plus per-random-case geometric, piecewise-linear, grid and convex
checks, stopping-reason and tie-rule checks, and Brent probe traces against an
independent Numerical Recipes reference (random traces in every mode). The
derivative Brent (dbrent) gets the same corpora with an analytic derivative or a
deterministic stand-in: its f and df call sequence and result against a Numerical
Recipes dbrent reference, contract checks (interior paired calls, call count,
least probe, bracket meeting the minimizers), wrong derivatives, the Brent
boundary intervals, limits and tolerances, and smooth minima with their f-call
counts against Brent. Full and stress use optimized, checked and ASan/UBSan builds, and an optimized
-march=native build where the CPU has FMA (contracted std::lerp, as on AtCoder).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-search_algorithms', [
        'first-order', 'last-order', 'integer-order', 'fib-order', 'binary-iterations',
        'ternary-iterations', 'golden-iterations', 'binary-infinite',
        'ternary-nan', 'golden-infinite', 'ternary-order', 'golden-order',
        'negative-absolute', 'negative-relative', 'infinite-tolerance', 'nan-tolerance',
        'geometric-zero', 'geometric-sign', 'geometric-sign-reversed', 'geometric-infinite', 'bits-infinite', 'bits-nan-tolerance', 'brent-order', 'brent-iterations',
        'brent-infinite', 'brent-nan-tolerance', 'dbrent-order', 'dbrent-iterations',
        'dbrent-infinite', 'dbrent-nan-tolerance'], native=True))
