"""Diophantine/congruence features against enumeration and Python integers.

Quick: coefficients through 3, all small boxes through coefficient 2,
congruences through 12, 1000 random C++ cases and 400 Python random cases.
Full: coefficients through 6, all small boxes through coefficient 4,
congruences through 40, 20000 random C++ cases and 5000 Python cases.
Stress: full exhaustion plus 200000 C++/50000 Python random cases.
All modes include signed64 boundary cubes; full/stress use all three builds.
Python's built-in inverse and arbitrary-precision integers independently
check canonical representatives, interval endpoints and exact huge counts.
solveDioMinLinear: C++ enumeration over a, b through 2/4, c through 4/8,
p, q through 2/3 and eight boxes, signed64 p, q on fixed lines, and the
random rounds above on small boxes; Python compares the objective with both
ends of the parameter range and pins the tie-rule point (least corner on the
plane, least parameter on a slope-0 line) for 729 edge cubes, 729 slope-0
cubes (p, q = a, b), 486 edge objectives and
400/5000/50000 random rows (quick/full/stress).
residueShortestPath/residueShortestPathDijkstra/frobeniusNumber/countRepresentable/countNonRepresentable:
C++ knapsack DP on all pairs through 12/25, triples through 7/12, quadruples
through 0/7 and 200/3000/30000 random sets of 1-6 values through 60 (with gcd
2-4, duplicates and 1), checking the gap count against the DP's unreachable
values, the INT64_MAX sentinel for gcd > 1, F + 1 - countRepresentable(F) and
the half-of-[0, F] bound; closed forms of coprime pairs (table j * b,
Sylvester number and count) for 100/2000/20000 random pairs at signed64
scale; Python Dijkstra with big integers on 12 boundary sets and
30/250/2500 random sets (m through 300/2000/2000, values up to
INT64_MAX / m), self-checked by Sylvester pairs and gcd scaling.
frobeniusNumberSylvester/countNonRepresentableSylvester: every pair in
[1, 60]^2 against a reachability sieve (self-checked on {3, 5} and by its
closing run of min(a, b) reachable values) and the table route, lll max
beside the table's INT64_MAX for gcd > 1; 40/400/4000 random pairs through
10^6 (min coin through 10^5, a quarter scaled by 2-10) against the table
route; Python exact integers on 361 edge pairs, 315 power-of-two/odd
pairs, consecutive and near-INT64_MAX pairs and 400/5000/50000 random rows.
Probes: modulus, box, residue preconditions and coins a, b >= 1.
"""
import itertools
import heapq
import math
import random
import shlex
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run

LO, HI = -(1 << 63), (1 << 63) - 1


def expected(row):
    a, b, c, xl, xr, yl, yr, m = row
    g = math.gcd(a, b)
    if g == 0:
        s = (2 if c == 0 else -1, 0, 0, 0, 0, 0)
    elif c % g:
        s = (-1, 0, 0, 0, 0, g)
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
    command, actual = query(binary, '--oracle', rows, env)
    for i,(row,line) in enumerate(zip(rows,actual)):
        want = expected(row)
        got = tuple(map(int,line.split()))
        if got != want:
            raise RuntimeError(f'Python integer oracle seed={args.seed} smallest known failing input #{i}={row}\n'
                               f'expected={want}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python arbitrary-integer inverse/box oracle cases={len(rows)} seed={args.seed}',flush=True)
    min_oracle(binary, args, env, rng)
    residue_oracle(binary, args, env, rng)
    sylvester_oracle(binary, args, env, rng)


def query(binary, flag, rows, env):
    command = [str(binary),flag]
    result = timed_run(f'03-equation_solvers {flag[2:]} {Path(command[0]).name}', command,input=''.join(' '.join(map(str,row))+'\n' for row in rows),
                       text=True,capture_output=True,env=env,timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    return command, actual


def min_valid(row, got):
    a,b,c,p,q,xl,xr,yl,yr = row
    dimension,x0,y0,dx,dy,_,l,r,count = expected((a,b,c,xl,xr,yl,yr,1))[:9]
    if count == 0:
        return got == (0,0,0)
    ok,x,y = got
    if ok != 1 or not (xl <= x < xr and yl <= y < yr) or a*x+b*y != c:
        return False
    # Tie rule: the plane takes its least corner per zero weight, a line its least parameter for slope 0.
    if dimension == 2:
        point = (xl if p >= 0 else xr-1, yl if q >= 0 else yr-1)
    else:
        t = l if p*dx+q*dy >= 0 else r-1
        point = (x0+t*dx, y0+t*dy)
        if p*x+q*y != min(p*(x0+t*dx)+q*(y0+t*dy) for t in (l,r-1)):
            return False
    return (x,y) == point


def min_oracle(binary, args, env, rng):
    edges = [LO,LO+1,-2,-1,0,1,2,HI-1,HI]
    rows = [(a,b,c,edges[i%9],edges[(i//9+2*i)%9],LO,HI,LO,HI) for i,(a,b,c) in enumerate(itertools.product(edges,repeat=3))]
    rows += [(a,b,c,a,b,LO,HI,LO,HI) for a,b,c in itertools.product(edges,repeat=3)]
    for a,b,c in ((1,-1,0),(LO,HI,LO),(HI,HI-1,1),(0,0,0),(0,LO,LO),(3,-5,HI)):
        rows += [(a,b,c,p,q,LO,HI,LO,HI) for p,q in itertools.product(edges,repeat=2)]
    nonempty = 0
    for i in range({'quick':400,'full':5000,'stress':50000}[args.mode]):
        a,b,c,p,q = (rng.choice(edges) if rng.random() < 0.2 else rng.randint(LO,HI) for _ in range(5))
        if i%3 == 0:
            a,b = (rng.randint(-(1<<31),1<<31) for _ in range(2))
            c = a*rng.randint(-(1<<31),1<<31)+b*rng.randint(-(1<<31),1<<31)
        xl,xr = sorted([rng.randint(LO,HI),rng.randint(LO,HI)])
        yl,yr = sorted([rng.randint(LO,HI),rng.randint(LO,HI)])
        if i%13 == 0: xr = xl
        if i%19 == 0: xl,xr,yl,yr = LO,HI,LO,HI
        if i%7 == 0: p,q = a,b
        rows.append((a,b,c,p,q,xl,xr,yl,yr))
    command, actual = query(binary, '--oracle-min', rows, env)
    for i,(row,line) in enumerate(zip(rows,actual)):
        got = tuple(map(int,line.split()))
        nonempty += got[0]
        if not min_valid(row, got):
            raise RuntimeError(f'Python min-linear oracle seed={args.seed} smallest known failing input #{i}={row}\n'
                               f'actual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python min-linear objective oracle cases={len(rows)} nonempty={nonempty} seed={args.seed}',flush=True)


def dijkstra(a):
    m = min(a)
    d = [0]+[-1]*(m-1)
    heap = [(0,0)]
    while heap:
        w,u = heapq.heappop(heap)
        if w > d[u]:
            continue
        for b in a:
            v = (u+b)%m
            if d[v] < 0 or w+b < d[v]:
                d[v] = w+b
                heapq.heappush(heap,(w+b,v))
    return d


def representable(d, x):
    return sum((x-v)//len(d)+1 for v in d if 0 <= v <= x)


def residue_expected(a, xs):
    d = dijkstra(a)
    m = len(d)
    frob = HI if -1 in d else max(d)-m
    counts = [representable(d,x) for x in xs]
    g = 0
    for v in a:
        g = math.gcd(g,v)
    distinct = sorted(set(a))
    # Self-checks of the Dijkstra oracle: Sylvester pairs and gcd scaling.
    if len(distinct) == 2 and g == 1:
        u,v = distinct
        half = (u-1)*(v-1)//2
        if frob != u*v-u-v or any(count != x+1-half for x,count in zip(xs,counts) if x >= frob):
            raise RuntimeError(f'Python Dijkstra oracle disagrees with Sylvester a={a}')
    if g > 1:
        scaled = dijkstra([v//g for v in a])
        if any(count != (representable(scaled,x//g) if x >= 0 else 0) for x,count in zip(xs,counts)):
            raise RuntimeError(f'Python Dijkstra oracle disagrees with gcd scaling a={a}')
    return (*d,frob,*counts)


def residue_oracle(binary, args, env, rng):
    sets = [[1],[1,HI],[2,HI//2],[HI//2,2,2],[7,HI//7],[7,HI//7,HI//7-1],[HI//49-3,49,HI//49],[3,HI//3,HI//3-1],
            [5,5,5],[6,10,15],[6,10,14],[2,4,HI//2-1]]
    limit = 300 if args.mode == 'quick' else 2000
    for i in range({'quick':30,'full':250,'stress':2500}[args.mode]):
        g = rng.choice([1,1,1,2,3,7])
        m = rng.randint(1,limit//g)
        if i%4 == 0:
            a = [m,rng.randint(m+1,HI//m)]
            while math.gcd(*a) != 1:
                a[1] = rng.randint(m+1,HI//m)
        else:
            cap = HI//(m*g*g)
            high = min(cap,3*m) if i%3 == 0 else cap
            a = [m]+[rng.randint(m,high) for _ in range(rng.randint(0,5))]
            if i%5 == 0: a.append(rng.choice(a))
            a = [g*v for v in a]
        rng.shuffle(a)
        sets.append(a)
    rows, want = [], []
    for a in sets:
        d = dijkstra(a)
        frob = HI if -1 in d else max(d)-len(d)
        xs = [LO,-1,0,1,HI-1,HI]+[rng.randint(0,HI) for _ in range(3)]
        xs += [x for v in rng.sample(d,min(3,len(d))) if v >= 0 for x in (v-1,v,v+1)]
        if frob < HI:
            xs += [x for x in (frob-1,frob,frob+1) if LO <= x <= HI]
        rows.append((len(a),*a,len(xs),*xs))
        want.append(residue_expected(a,xs))
    command, actual = query(binary, '--oracle-residue', rows, env)
    for i,(row,expect,line) in enumerate(zip(rows,want,actual)):
        note = 'residueShortestPathDijkstra differs from residueShortestPath\n' if line.startswith('Dijkstra-differs ') else ''
        got = tuple(map(int,line.removeprefix('Dijkstra-differs ').split()))
        if note or got != expect:
            raise RuntimeError(f'Python residue oracle seed={args.seed} smallest known failing input #{i}={row}\n'
                               f'{note}expected={expect}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python Dijkstra residue/Frobenius/count oracle sets={len(rows)} seed={args.seed}',flush=True)


def sylvester_oracle(binary, args, env, rng):
    none = (1 << 127)-1
    edges = [1,2,3,4,5,6,7,(1<<31)-1,1<<32,(1<<32)+1,(1<<62)-1,1<<62,(1<<62)+1,HI//2,HI//2+1,HI-3,HI-2,HI-1,HI]
    rows = list(itertools.product(edges,repeat=2))
    rows += [(1<<k,v) for k in range(63) for v in (1,3,HI,HI-2,rng.randrange(1,HI,2))]
    for i in range({'quick':400,'full':5000,'stress':50000}[args.mode]):
        k = rng.randint(1,HI-1) if i%2 else HI-1-rng.randint(0,1<<20)
        rows.append([(k,k+1),(k+1,k),(rng.randint(1,HI),rng.randint(1,HI)),(HI-rng.randint(0,1<<20),HI-rng.randint(0,1<<20))][i%4])
    command, actual = query(binary, '--oracle-sylvester', rows, env)
    coprime = 0
    for i,((a,b),line) in enumerate(zip(rows,actual)):
        got = tuple(map(int,line.split()))
        want = (a*b-a-b,(a-1)*(b-1)//2) if math.gcd(a,b) == 1 else (none,none)
        coprime += want[0] < none
        if got != want:
            raise RuntimeError(f'Python Sylvester oracle seed={args.seed} smallest known failing input #{i}={(a,b)}\n'
                               f'expected={want}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python exact Sylvester oracle cases={len(rows)} coprime={coprime} seed={args.seed}',flush=True)


if __name__ == '__main__':
    raise SystemExit(main('03-equation_solvers', ['mod-zero','mod-negative','mod-zero-nosolution','mod-negative-nosolution','box-x','box-y','residue-empty','residue-zero','residue-negative','residue-overflow','dijkstra-empty','dijkstra-zero','dijkstra-negative','dijkstra-overflow',
                         'sylvester-zero','sylvester-negative'], oracle))
