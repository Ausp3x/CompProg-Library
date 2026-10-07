"""Exact integer/counting oracles; full adds random cases and sanitizers, stress extends them."""
import itertools
import math
import os
from pathlib import Path
import random
import subprocess
from _00_runner import main

LIMIT = (1 << 64) - 1


def choose(n, k):
    if k < 0 or k > n:
        return 0
    k = min(k, n - k)
    # This branch certifies overflow without allocating astronomical integers.
    return LIMIT + 1 if k > 100 else math.comb(n, k)


def ballot(a, b, strict=False):
    if a == b == 0:
        return 1
    if a < b or strict and a == b:
        return 0
    if b > 100:
        return LIMIT + 1
    n = a + b
    c = math.comb(n, b)
    return c * (a - b) // n if strict else c * (a - b + 1) // (a + 1)


def stars(total, parts, positive=False):
    if parts == 0:
        return int(total == 0)
    if positive:
        return choose(total - 1, parts - 1) if total >= parts else 0
    return choose(total + parts - 1, total)


def replacement(a, b):
    return 1 if b == 0 else 0 if a == 0 else choose(a + b - 1, b)


def multinomial(parts):
    total = sum(parts)
    # Factorial ratio is independent of the implementation's product of binomials.
    if total <= 500:
        result = math.factorial(total)
        for k in parts:
            result //= math.factorial(k)
        return result
    nonzero = [k for k in parts if k]
    if len(nonzero) <= 1:
        return 1
    if min(nonzero) > 100:
        return LIMIT + 1
    result = 1
    for k in sorted(nonzero):
        result *= choose(total, k)
        total -= k
        if result > LIMIT:
            return LIMIT + 1
    return result


def derangement(n):
    # Exact inclusion-exclusion, independent of the library recurrence.
    return sum((-1)**k * math.factorial(n) // math.factorial(k) for k in range(n + 1))


def fib_matrix(n, mod):
    def mul(a, b):
        return tuple(sum(a[2*i+k] * b[2*k+j] for k in range(2)) % mod
                     for i in range(2) for j in range(2))
    r, a = (1,0,0,1), (1,1,1,0)
    while n:
        if n & 1:
            r = mul(r, a)
        a = mul(a, a)
        n >>= 1
    return r[1] % mod, r[0] % mod


def oracle(binary, args, env):
    if not getattr(args, '_narrow_checked', False):
        header = Path(__file__).resolve().parents[2] / '05-Mathematics/05-combinatorics.hpp'
        command = [os.environ.get('CXX','g++'),'-std=gnu++20','-x','c++','-','-fsyntax-only']
        source = f'#include "{header}"\nint main() {{ fibonacciPair<uint16_t>(8); }}\n'
        failure = subprocess.run(command,input=source,text=True,capture_output=True,env=env,timeout=180)
        if failure.returncode == 0 or 'static assertion failed' not in failure.stderr:
            raise RuntimeError(f'narrow integer rejection expected static assertion command={command} returncode={failure.returncode}\n{failure.stderr}')
        args._narrow_checked = True
        print('PASS compile rejection: narrow unsigned Fibonacci ring',flush=True)
    rng = random.Random(args.seed)
    cases = []

    def add(op, values, expected):
        cases.append((op, tuple(values), list(expected)))

    def exact(op, values, value):
        add(op, values, [1, value] if value <= LIMIT else [0, 123])

    def small_ballot(a, b, strict):
        count = 0
        for positions in itertools.combinations(range(a+b), b):
            positions = set(positions)
            height = 0
            ok = True
            for i in range(a+b):
                height += -1 if i in positions else 1
                ok &= height > 0 if strict else height >= 0
            count += ok
        return count

    def small_stars(total, parts, positive):
        if not parts:
            return int(total == 0)
        start = int(positive)
        return sum(small_stars(total-x, parts-1, positive) for x in range(start, total+1))

    for a in range(8):
        for b in range(8):
            for strict in (0,1):
                expected = small_ballot(a,b,strict)
                if expected != ballot(a,b,strict):
                    raise RuntimeError(f'ballot reference disagrees with enumeration a={a} b={b} strict={strict}')
                exact('eb', (a,b,strict), expected)
            if a <= 6 and b <= 5:
                for positive in (0,1):
                    expected = small_stars(a,b,positive)
                    if expected != stars(a,b,positive):
                        raise RuntimeError(f'stars reference disagrees with enumeration total={a} parts={b}')
                    exact('es', (a,b,positive), expected)
    for n in range(9):
        count = sum(all(i != x for i,x in enumerate(p)) for p in itertools.permutations(range(n)))
        if count != derangement(n):
            raise RuntimeError(f'derangement oracle disagrees with enumeration n={n}')

    for p,n in [(2,1),(3,2),(7,6),(97,96),(998244353,180),(18446744073709551557,180)]:
        add('setup', (p,n), [1])
        for k in range(n+1):
            f = math.factorial(k)
            add('mf', (k,), [f % p, pow(f,-1,p), derangement(k) % p])
        for a in range(min(n,30)+1):
            for b in range(-1,min(n,30)+2):
                add('mc', (a,b), [choose(a,b) % p])
                add('mp', (a,b), [math.perm(a,b) % p if 0 <= b <= a else 0])
                add('mq', (a,b), [pow(a,b,p) if b >= 0 else 0])
                if b < 0 or a == 0 or b == 0 or a+b-1 <= n:
                    add('mr', (a,b), [0 if b < 0 else replacement(a,b) % p])
        for a in range(min(n,20)+1):
            for b in range(min(n,20)+1):
                for s in (0,1):
                    if a == 0 and b == 0 or a < b or s and a == b or a+b-s <= n:
                        add('mb', (a,b,s), [ballot(a,b,s) % p])
                    if not b or s and a < b or (a-1 if s else a+b-1) <= n:
                        add('ms', (a,b,s), [stars(a,b,s) % p])
        for k in range(n//2+1):
            add('mt', (k,), [math.comb(2*k,k)//(k+1) % p])
        for a in sorted({0,1,2,n,p-1,p,p+1,2*p+3,1<<63,LIMIT} | {rng.getrandbits(64) for _ in range(5)}):
            if a > LIMIT: continue
            for b in sorted(x for x in {-1,0,1,2,n//2,n} | {rng.randrange(n+1) for _ in range(3)} if x <= n):
                add('ml', (a,b), [math.comb(a,b) % p if b >= 0 else 0])
        for _ in range(50):
            parts, remaining = [], n
            for _ in range(rng.randrange(7)):
                k = rng.randrange(remaining+1)
                parts.append(k); remaining -= k
            add('mm', (len(parts), *parts), [multinomial(parts) % p])
        for index in [0,1,2,3,92,93,94,1<<63,LIMIT] + [rng.getrandbits(64) for _ in range(10)]:
            a,b = fib_matrix(index,p)
            add('mg', (index,), [a,b,a,(2*b-a)%p])
    # Fibonacci only needs a ring, including composite and modulus-one contexts.
    for p in [1,2,8,1000,1<<63,LIMIT]:
        add('ring', (p,), [1])
        for index in [0,1,2,92,1<<63,LIMIT]:
            a,b = fib_matrix(index,p)
            add('mg', (index,), [a,b,a,(2*b-a)%p])
        add('rd', (30,), [derangement(k) % p for k in range(31)])
        for n in (0, 1, 12):
            add('rd', (n,), [derangement(k) % p for k in range(n+1)])
            add('rb', (n,), [math.comb(i,j) % p for i in range(n+1) for j in range(i+1)])
    add('ud', (40,), [derangement(k) % (1<<64) for k in range(41)])
    for n in range(101):
        f = math.factorial(n) if n <= 21 else LIMIT+1
        exact('ef', (n,), f)
        exact('ed', (n,), derangement(n) if n <= 21 else LIMIT+1)
        exact('et', (n,), math.comb(2*n,n)//(n+1))
        a,b = 0,1
        for _ in range(n):
            a,b = b,a+b
        l = 2*b-a
        add('eg', (n,), ([1,a,b] if b <= LIMIT else [0,123,456])
            + ([1,a] if a <= LIMIT else [0,123]) + ([1,l] if l <= LIMIT else [0,123]))
    add('eg', (LIMIT,), [0,123,456,0,123,0,123])
    for p,op in [(1<<32,'rg32'),(1<<64,'rg')]:
        for n in [0,1,2,47,48,93,94,1<<63,LIMIT]:
            a,b = fib_matrix(n,p)
            add(op,(n,),[a,b,a,(2*b-a)%p])
    edges = list(range(71)) + [100,1<<32,(1<<63)-1,1<<63,LIMIT-1,LIMIT]
    for a in edges:
        for b in list(range(5)) + [a,a+1 if a < LIMIT else a]:
            exact('ec',(a,b),choose(a,b))
            exact('ep',(a,b),math.perm(a,b) if b <= min(a,20) else 0 if b>a else LIMIT+1)
            exact('er',(a,b),replacement(a,b))
            exact('eq',(a,b),pow(a,b) if a <= 1 or b <= 64 else LIMIT+1)
            for s in (0,1):
                exact('eb',(a,b,s),ballot(a,b,s))
                exact('es',(a,b,s),stars(a,b,s))
    for n in range(60,81):
        for k in range(n+2):
            exact('ec',(n,k),choose(n,k))
    for parts in [[],[0],[0,0],[1,2,3],[20],[21],[1]*20,[1]*21,[LIMIT],[LIMIT,0],[LIMIT,1],[1,LIMIT],[LIMIT,LIMIT]]:
        exact('em',(len(parts),*parts),multinomial(parts))
    for n in [LIMIT-1,LIMIT]:
        for op in ('ef','ed','et'):
            exact(op,(n,),LIMIT+1)
    rounds = {'quick': 300, 'full': 5000, 'stress': 40000}[args.mode]
    for _ in range(rounds):
        a = rng.getrandbits(64) if rng.randrange(3)==0 else rng.randrange(200)
        b = rng.getrandbits(64) if rng.randrange(5)==0 else rng.randrange(150)
        exact('ec',(a,b),choose(a,b))
        exact('er',(a,b),replacement(a,b))
        exact('ep',(a,b),math.perm(a,b) if b <= min(a,20) else 0 if b>a else LIMIT+1)
        exact('eq',(a,b),pow(a,b) if a <= 1 or b <= 64 else LIMIT+1)
        for s in (0,1):
            exact('eb',(a,b,s),ballot(a,b,s))
            exact('es',(a,b,s),stars(a,b,s))
        parts = [rng.randrange(40) for _ in range(rng.randrange(9))]
        exact('em',(len(parts),*parts),multinomial(parts))
    command = [str(binary),'--oracle']
    payload = ''.join(op+' '+' '.join(map(str,values))+'\n' for op,values,_ in cases)
    result = subprocess.run(command,input=payload,text=True,capture_output=True,env=env,timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={command} returncode={result.returncode}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f'command={command} expected {len(cases)} lines actual={len(lines)}')
    for (op,values,expected),line in zip(cases,lines):
        actual = list(map(int,line.split()))
        if actual != expected:
            raise RuntimeError(f'smallest known reproducer={op} {values} expected={expected} actual={actual} command={command}')
    print(f'PASS Python exact integers, matrix powers and enumerated combinatorics: {len(cases)} cases seed={args.seed}',flush=True)


if __name__ == '__main__':
    raise SystemExit(main('05-combinatorics', [
        'negative-n','intmax-n','modulus-one','composite','prime-flag','n-equals-mod','stale-modulus',
        'negative-choice','negative-choice-rep','negative-permutation','negative-permutation-rep',
        'factorial-negative','factorial-bound','inverse-bound','derangement-bound','combination-bound',
        'replacement-bound','permutation-bound','stars-total','stars-parts','stars-bound','catalan-negative',
        'catalan-bound','ballot-negative','ballot-bound','multinomial-negative','multinomial-bound',
        'large-bound','table-negative','derangement-table-negative'],oracle))
