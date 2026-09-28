"""Diophantine/congruence features against enumeration and Python integers.

Quick: coefficients through 3, all small boxes through coefficient 2,
congruences through 12, 1000 random C++ cases and 400 Python random cases.
Full: coefficients through 6, all small boxes through coefficient 4,
congruences through 40, 20000 random C++ cases and 5000 Python cases.
Stress: full exhaustion plus 200000 C++/50000 Python random cases.
All modes include signed64 boundary cubes; full/stress use all three builds.
Python's built-in inverse and arbitrary-precision integers independently
check canonical representatives, interval endpoints and exact huge counts.
"""
import itertools
import math
import random
import shlex
import subprocess
from _00_runner import main

LO, HI = -(1 << 63), (1 << 63) - 1


def expected(row):
    a, b, c, xl, xr, yl, yr, m = row
    g = math.gcd(a, b)
    if g == 0:
        s = (2 if c == 0 else -1, 0, 0, 0, 0, 0)
    elif c % g:
        s = (-1, 0, 0, 0, 0, 0)
    elif b:
        period = abs(b // g)
        x = (c // g * pow(a // g, -1, period)) % period
        s = (1, x, (c-a*x)//b, b//g, -a//g, g)
    else:
        s = (1, c//a, 0, 0, -a//g, g)
    l = r = count = 0
    if xl < xr and yl < yr and s[0] == 2:
        count = (xr-xl)*(yr-yl)
    elif xl < xr and yl < yr and s[0] == 1:
        limits = []
        for p, step, low, high in ((s[1],s[3],xl,xr),(s[2],s[4],yl,yr)):
            if step == 0:
                if not low <= p < high:
                    limits.append((0,0))
                continue
            # Each coordinate's first/last admissible integer parameter.
            first, last = (low-p, high-1-p) if step > 0 else (high-1-p, low-p)
            limits.append((-((-first)//step), last//step+1))
        l = max(t[0] for t in limits)
        r = min(t[1] for t in limits)
        if l < r:
            count = r-l
        else:
            l = r = 0
    h = math.gcd(a,m)
    mod = (-1,-1) if c % h else ((c//h*pow(a//h,-1,m//h))%(m//h),m//h)
    return (*s,l,r,count,*mod)


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    edges = [LO,LO+1,-2,-1,0,1,2,HI-1,HI]
    rows = [(a,b,c,LO,HI,LO,HI,HI) for a,b,c in itertools.product(edges,repeat=3)]
    count = {'quick':400,'full':5000,'stress':50000}[args.mode]
    for i in range(count):
        a,b,c = (rng.randint(LO,HI) for _ in range(3))
        if i%5 == 0: a = rng.choice(edges)
        if i%7 == 0: b = rng.choice(edges)
        if i%11 == 0: c = 0
        xl,xr = sorted([rng.randint(LO,HI),rng.randint(LO,HI)])
        yl,yr = sorted([rng.randint(LO,HI),rng.randint(LO,HI)])
        if i%13 == 0: xr = xl
        if i%17 == 0: yr = yl
        rows.append((a,b,c,xl,xr,yl,yr,rng.randint(1,HI)))
    command = [str(binary),'--oracle']
    result = subprocess.run(command,input=''.join(' '.join(map(str,row))+'\n' for row in rows),
                            text=True,capture_output=True,env=env,timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i,(row,line) in enumerate(zip(rows,actual)):
        want = expected(row)
        got = tuple(map(int,line.split()))
        if got != want:
            raise RuntimeError(f'Python integer oracle seed={args.seed} smallest known failing input #{i}={row}\n'
                               f'expected={want}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python arbitrary-integer inverse/box oracle cases={len(rows)} seed={args.seed}',flush=True)


if __name__ == '__main__':
    raise SystemExit(main('03-equation_solvers', ['mod-zero','mod-negative','box-x','box-y',
                         'legacy-gcd','legacy-point','legacy-xy','legacy-xg','legacy-yg'], oracle))
