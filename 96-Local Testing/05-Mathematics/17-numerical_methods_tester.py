"""Numerical methods against exact rationals, closed forms, conditioning bounds and an exact Python oracle.

C++ (quick / full / stress multiply the random counts by 1 / 10 / 100): rational-root
quadratics (300 per unit) with exact expected roots, Kahan, cancellation, linear and
scaling cases; integer, rational, double, triple, complex-pair and random-root cubics
(100) within normwise conditioning; hornerDerivative over lng, DualNumber, a prime field
and long double (300); DualNumber operators, aliasing, quotient rule and nested
derivatives (300); Gauss-Legendre tables n <= 5 and moments for n <= 40 / 200 / 400;
Simpson, Boole, Gauss, Romberg and Richardson exactness and error formulas (100);
finite differences (200); tanh-sinh endpoint singularities; fixed point (100); bisection,
Newton, newtonSafe, secant and Brent status, bracket and tolerance bounds (50).
Python oracle (quick 400 / full 20000 / stress 200000 quadratics, 60 / 600 / 4000 sums):
exact discriminant sign and 80-digit roots for solveQuadratic, and exact sums for the
compensated sums' documented error bounds. Stress varies only the seed and counts.
Full and stress use optimized, checked and ASan/UBSan builds.
"""
from decimal import Decimal, localcontext
from fractions import Fraction
import math
import random
import shlex
import subprocess
from _00_runner import main

U = Fraction(1, 2 ** 53)
EPS = 2 * U


def dec(x):
    return Decimal(x.numerator) / Decimal(x.denominator)


def rand_double(rng, lo, hi):
    return rng.choice((-1, 1)) * math.ldexp(rng.uniform(1, 2), rng.randint(lo, hi))


def quadratic_input(rng, i):
    kind = i % 7
    if kind == 5:
        b = rand_double(rng, -60, 60) if i % 2 else 0.0
        return 0.0, b, rand_double(rng, -60, 60)
    if kind == 6:
        a, b, c = (float(rng.randint(-20, 20)) for _ in range(3))
        return (a, b, c) if (a, b, c) != (0, 0, 0) else (1.0, b, c)
    a, b, c = (rand_double(rng, -60, 60) for _ in range(3))
    if kind == 1:
        c = math.copysign(c, a)
        b = math.copysign(2 * math.sqrt(a * c), b)
        for _ in range(rng.randint(0, 3)):
            b = math.nextafter(b, rng.choice((-math.inf, math.inf)))
    elif kind == 2:
        b = math.copysign(math.sqrt(abs(a * c)) * math.ldexp(1, rng.randint(20, 60)), b)
    elif kind == 3:
        c = 0.0
    elif kind == 4:
        b = 0.0
    return a, b, c


def check_quadratic(a, b, c, got):
    """None when got meets the bounds; derivation in the comments, u = 2^-53."""
    if a == 0:
        want = [] if b == 0 else [-c / b]
        return None if got == want else f'linear expected={want}'
    A, B, C = Fraction(a), Fraction(b), Fraction(c)
    D = B * B - 4 * A * C
    # fma gives b^2 - fl(4ac) rounded once plus the exact product error, so |d - D| <= 2u|D| + u^2|4ac|; doubled.
    err = 4 * U * abs(D) + 2 * U * U * abs(4 * A * C)
    counts = {2} if D > err else {0} if D < -err else {0, 2}
    if len(got) not in counts:
        return f'root count expected={sorted(counts)} discriminant={float(D)!r} bound={float(err)!r}'
    if not got:
        return None
    if got[0] > got[1]:
        return 'roots not ascending'
    with localcontext() as ctx:
        ctx.prec = 80
        Dp = max(D, Fraction(0))
        sq = dec(Dp).sqrt()
        q = -(dec(B) + sq.copy_sign(Decimal(1) if math.copysign(1, b) > 0 else Decimal(-1))) / 2
        if q == 0:
            return None if got == [0.0, 0.0] else 'expected the double root 0'
        X, Y = q / dec(A), dec(C) / q
        # |sqrt(d) - sqrt(D+)| <= min(sqrt(err), err / sqrt(D+)); q then carries 3u, each root one more rounding.
        ds = dec(err).sqrt() if Dp == 0 else min(dec(err).sqrt(), dec(err) / sq)
        tol = 2 * (ds / (2 * abs(q)) + 4 * dec(U))
        for x, y in ((X, Y), (Y, X)):
            if abs(Decimal(got[0]) - x) <= tol * abs(x) and abs(Decimal(got[1]) - y) <= tol * abs(y):
                return None
        return f'roots expected={float(min(X, Y))!r},{float(max(X, Y))!r} relative tolerance={float(tol):.3e}'


def sum_input(rng, i):
    n = [0, 1, 2, 3, 127, 128, 129, 255, 256, 257][i] if i < 10 else rng.choice((rng.randint(1, 300), rng.randint(1, 3000)))
    kind = i % 6
    if kind == 5:
        # Terms below half an ulp of the running sum: naive summation loses every one of them.
        return [1.0] + [math.ldexp(rng.uniform(0.5, 1), -53) for _ in range(n)]
    if kind == 0:
        return [rand_double(rng, -40, 40) for _ in range(n)]
    if kind == 1:
        half = [rand_double(rng, -40, 40) for _ in range((n + 1) // 2)]
        xs = half + [-x for x in half] + [rand_double(rng, -80, -40) for _ in range(3)]
        rng.shuffle(xs)
        return xs
    if kind == 2:
        return [math.ldexp(1, 60) * rng.choice((-1, 1))] + [rand_double(rng, -10, 0) for _ in range(n)]
    if kind == 3:
        return [float(rng.randint(-10 ** 6, 10 ** 6)) for _ in range(n)]
    return sorted((rand_double(rng, -100, 100) for _ in range(n)), key=abs, reverse=True)


def check_sums(xs, kahan, neumaier, pairwise):
    n, S, T = len(xs), sum(map(Fraction, xs)), sum(abs(Fraction(x)) for x in xs)
    errs = [abs(Fraction(v) - S) for v in (kahan, neumaier, pairwise)]
    if all(x == int(x) for x in xs) and T < 2 ** 53:
        return None if errs == [0, 0, 0] else 'integer sums below 2^53 must be exact'
    # Kahan: 2 eps sum |a|; Neumaier: eps |S| plus the naive sum of n exact error terms, n^2 eps^2 sum |a|.
    bounds = [2 * EPS * T, EPS * abs(S) + n * n * EPS * EPS * T, (128 + math.log2(max(n, 1))) * EPS * T]
    for name, e, bound in zip(('kahanSum', 'neumaierSum', 'pairwiseSum'), errs, bounds):
        if e > bound:
            return f'{name} error={float(e)!r} bound={float(bound)!r} exact={float(S)!r}'
    return None


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    nq, ns = {'quick': (400, 60), 'full': (20000, 600), 'stress': (200000, 4000)}[args.mode]
    quads = [(1.0, -(2 + 2 ** -29), 1 + 2 ** -29), (1.0, -1e8, 1.0)] + [quadratic_input(rng, i) for i in range(nq)]
    sums = [[1.0, 1e100, 1.0, -1e100], [0.1] * 10] + [sum_input(rng, i) for i in range(ns)]
    lines = [f'q {a.hex()} {b.hex()} {c.hex()}\n' for a, b, c in quads]
    lines += [f's {len(xs)} ' + ' '.join(x.hex() for x in xs) + '\n' for xs in sums]
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(lines), text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    out = result.stdout.splitlines()
    if len(out) != len(lines):
        raise RuntimeError(f'Python oracle output count expected={len(lines)} actual={len(out)} command={shlex.join(command)}')
    for i, (row, line) in enumerate(zip(quads, out)):
        got = [float.fromhex(t) for t in line.split()[1:]]
        error = check_quadratic(*row, got)
        if error:
            raise RuntimeError(f'solveQuadratic exact oracle seed={args.seed} case #{i} input={tuple(x.hex() for x in row)} '
                               f'actual={got}: {error}\ncommand={shlex.join(command)}')
    for i, (xs, line) in enumerate(zip(sums, out[len(quads):])):
        got = [float.fromhex(t) for t in line.split()]
        error = check_sums(xs, *got)
        if i == 0 and got[1] != 2:
            error = f'neumaierSum([1, 1e100, 1, -1e100]) expected=2 actual={got[1]!r}'
        if error:
            shown = xs if len(xs) <= 8 else f'{len(xs)} terms starting {xs[:4]}'
            raise RuntimeError(f'compensated sum exact oracle seed={args.seed} case #{i} input={shown}: {error}\ncommand={shlex.join(command)}')
    print(f'PASS Python exact quadratic oracle cases={len(quads)} and exact sum oracle cases={len(sums)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('17-numerical_methods', [
        'quadratic-zero', 'quadratic-nan', 'cubic-infinite', 'bisection-sign', 'bisection-iterations', 'bisection-infinite',
        'newton-tolerance', 'newton-relative-nan', 'newton-start', 'safe-sign', 'safe-iterations', 'secant-infinite',
        'secant-tolerance', 'secant-equal', 'brent-sign', 'brent-tolerance', 'fixed-iterations', 'fixed-start', 'simpson-intervals',
        'adaptive-eps', 'adaptive-depth', 'gauss-nodes', 'gauss-panels', 'gauss-empty', 'gauss-mismatch', 'romberg-levels',
        'romberg-levels-high', 'romberg-eps', 'richardson-levels', 'richardson-ratio', 'richardson-order', 'difference-order',
        'difference-order-high', 'difference-step', 'difference-infinite', 'difference-lost-step', 'de-levels', 'de-levels-high', 'de-infinite',
        'de-eps'], oracle))
