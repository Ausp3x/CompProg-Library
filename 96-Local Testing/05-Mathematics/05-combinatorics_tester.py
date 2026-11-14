"""Exact integer/counting oracles; full adds random cases and sanitizers, stress extends them."""
import decimal
import fractions
import itertools
import math
import os
from pathlib import Path
import random
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run
from _00_cpp_test_runner import compiler_command

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


def negative_binomial(a, b):
    # Generalized binomial C(-a, b) = (-a)(-a-1)...(-a-b+1) / b!, independent of combiWR.
    return 0 if b < 0 else math.prod(-a - i for i in range(b)) // math.factorial(b)


def circular(a, b):
    return 0 if b < 0 or b > a else 1 if b == 0 else math.perm(a, b) // b


def double_factorial(k):
    return math.prod(range(k, 0, -2))


def multi_factorial(n, m):
    # Running product with an early stop above 64 bits, so huge n with small m stays cheap.
    r = 1
    for x in range(n, 1, -m):
        r *= x
        if r > LIMIT:
            return LIMIT + 1
    return r


def non_adjacent(a, b):
    return math.comb(a - b + 1, b) if 0 <= b <= a - b + 1 else 0


def primes_upto(n):
    flags = bytearray([1]) * (n + 1) if n >= 0 else bytearray()
    flags[:2] = b'\0' * min(2, n + 1)
    for i in range(2, math.isqrt(max(n, 0)) + 1):
        if flags[i]:
            flags[i*i::i] = bytes(len(range(i*i, n + 1, i)))
    return [i for i in range(n + 1) if flags[i]]


def product(values):
    # Pairwise products only to keep Python fast; the factors come from an independent sieve.
    values = list(values) or [1]
    while len(values) > 1:
        values = [math.prod(values[i:i+2]) for i in range(0, len(values), 2)]
    return values[0]


LOG = decimal.Context(prec=45)
LN_2PI = LOG.ln(LOG.multiply(2, decimal.Decimal('3.14159265358979323846264338327950288419716939937510582')))
BERNOULLI = [(1,6),(-1,30),(1,42),(-1,30),(5,66),(-691,2730),(7,6)]
LN_FACTORIAL = {}


def ln_factorial_stirling(a):
    # Stirling's series in x = a + 1 through B_14; the first omitted term is below 0.04 / x^15.
    x = decimal.Decimal(a + 1)
    r = LOG.add(LOG.subtract(LOG.multiply(LOG.subtract(x, decimal.Decimal('0.5')), LOG.ln(x)), x), LOG.divide(LN_2PI, 2))
    for i, (num, den) in enumerate(BERNOULLI, 1):
        r = LOG.add(r, LOG.divide(decimal.Decimal(num), LOG.multiply(den * 2 * i * (2 * i - 1), LOG.power(x, 2 * i - 1))))
    return r


def ln_factorial(a):
    # 45 digits: the exact factorial's log through 3000, Stirling's series beyond.
    if a not in LN_FACTORIAL:
        LN_FACTORIAL[a] = LOG.ln(decimal.Decimal(math.factorial(a))) if a <= 3000 else ln_factorial_stirling(a)
    return LN_FACTORIAL[a]


def log_case(a, b, permutation=False):
    if not 0 <= b <= a:
        return [decimal.Decimal('-Infinity'), log_tolerance(a)]
    r = LOG.subtract(ln_factorial(a), ln_factorial(a - b))
    return [r if permutation else LOG.subtract(r, ln_factorial(b)), log_tolerance(a)]


def log_tolerance(a):
    # Each entry log(k!) is within about 3 * 2^-63 log(k!): the compensated sum adds Kahan's 2 ulp to the half-ulp
    # errors of every log(i), which total at most 2^-64 log(k!), and lgammal stays within a few ulp. The three entries'
    # logs sum to at most 2 log(a!), plus two roundings of the result; 21 printed digits.
    return LOG.multiply(LOG.divide(8, 1 << 63), LOG.add(1, ln_factorial(a)))


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
        command = [*compiler_command(),'-std=gnu++20','-x','c++','-','-fsyntax-only']
        source = f'#include "{header}"\nint main() {{ fibonacciPair<uint16_t>(8); }}\n'
        failure = timed_run('05-combinatorics narrow-integer rejection compile', command,input=source,text=True,capture_output=True,env=env,timeout=180)
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
    for a in range(7):
        for b in range(-1, a + 2):
            # Circular arrangements: ordered b-selections up to rotation, counted as rotation classes.
            seen = {min(t[i:] + t[:i] for i in range(len(t))) if t else t for t in itertools.permutations(range(a), max(b, 0))} if 0 <= b <= a else set()
            if len(seen) != circular(a, b):
                raise RuntimeError(f'circular reference disagrees with enumeration a={a} b={b}')
        for b in range(8):
            if negative_binomial(a, b) != (-1)**b * replacement(a, b):
                raise RuntimeError(f'negative binomial identity a={a} b={b}')
    for a in range(11):
        for b in range(-1, a + 3):
            count = sum(all(y - x > 1 for x, y in zip(s, s[1:])) for s in itertools.combinations(range(a), b)) if b >= 0 else 0
            if count != non_adjacent(a, b):
                raise RuntimeError(f'non-adjacent reference disagrees with enumeration a={a} b={b}')
    for n in range(9):
        count = sum(all(i != x for i,x in enumerate(p)) for p in itertools.permutations(range(n)))
        if count != derangement(n):
            raise RuntimeError(f'derangement oracle disagrees with enumeration n={n}')
    for a in (300, 1000, 3000):
        if LOG.subtract(ln_factorial_stirling(a), ln_factorial(a)).copy_abs() > decimal.Decimal('1e-30'):
            raise RuntimeError(f'Stirling series disagrees with the exact factorial log a={a}')

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
        for k in range(n+1):
            add('md', (k,), [double_factorial(k) % p])
        for a in range(min(n,30)+1):
            for b in range(-1,min(n,30)+2):
                if b < 0 or a == 0 or b == 0 or a+b-1 <= n:
                    add('mn', (a,b), [negative_binomial(a,b) % p])
                if 0 <= b <= a:
                    add('mi', (a,b), [pow(math.comb(a,b),-1,p)])
                add('mo', (a,b), [circular(a,b) % p])
        for m in (0,1,2,3,4,5,10,30,100,1000):
            v = product(primes_upto(m))
            add('pr', (m,), [format(v,'X'), format(v,'X'), v % p, v % (1<<64)])
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
        for a in range(min(n,30)+1):
            for b in range(-1,a+3):
                if not 0 <= b <= a-b+1 or a-b+1 <= n:
                    add('mj', (a,b), [non_adjacent(a,b) % p])
        for a,m in [(0,1),(1,1),(2,1),(1,5),(10,3),(17,4),(100,7),(999,1),(1000,2),(1000,999),(1000,1000),(1000,5000),
                    (LIMIT,1<<63),(LIMIT,LIMIT),(LIMIT,LIMIT-1),(1<<63,(1<<62)+1),(rng.getrandbits(64),1<<60)]:
            v = math.prod(range(a,0,-m))
            add('mu', (a,m), [v % p, v % (1<<64), format(v,'X'), format(v,'X')])
        for index in [0,1,2,3,92,93,94,1<<63,LIMIT] + [rng.getrandbits(64) for _ in range(10)]:
            a,b = fib_matrix(index,p)
            add('mg', (index,), [a,b,(2*b-a)%p])
    # Fibonacci only needs a ring, including composite and modulus-one contexts.
    for p in [1,2,8,1000,1<<63,LIMIT]:
        add('ring', (p,), [1])
        for index in [0,1,2,92,1<<63,LIMIT]:
            a,b = fib_matrix(index,p)
            add('mg', (index,), [a,b,(2*b-a)%p])
        v = product(primes_upto(200))
        add('pr', (200,), [format(v,'X'), format(v,'X'), v % p, v % (1<<64)])
        add('rd', (30,), [derangement(k) % p for k in range(31)])
        for n in (0, 1, 12):
            add('rd', (n,), [derangement(k) % p for k in range(n+1)])
            add('rb', (n,), [math.comb(i,j) % p for i in range(n+1) for j in range(i+1)])
    add('ud', (40,), [derangement(k) % (1<<64) for k in range(41)])
    for n in list(range(101)) + [LIMIT-1, LIMIT]:
        exact('ex', (n,), double_factorial(n) if n <= 40 else LIMIT+1)
    for n in list(range(80)) + [1<<32, (1<<32)+1, (1<<63)-1, 1<<63, LIMIT-1, LIMIT]:
        for m in [1,2,3,4,5,7,10,63,64,1<<31,1<<32,(1<<32)+1,1<<62,1<<63,LIMIT-1,LIMIT] + ([n-1,n] if n > 1 else []):
            exact('eu', (n,m), multi_factorial(n,m))
    for p,n in [(2,1),(7,6),(998244353,60),(18446744073709551557,{'quick':60,'full':150,'stress':300}[args.mode])]:
        add('fs', (p,n), [1])
        for k in range(n+1):
            f = math.factorial(k)
            add('ff', (k,), [str(fractions.Fraction(f)), str(fractions.Fraction(1,f)), str(fractions.Fraction(f)), str(fractions.Fraction(1,f)), f % p, pow(f,-1,p)])
        for a in range(n+1):
            for b in sorted({-1,0,1,2,a//2,a-1,a,a+1} | {rng.randrange(a+1) for _ in range(4)}):
                c = math.comb(a,b) if 0 <= b <= a else 0
                add('fc', (a,b), [c, c, c % p])
                q = math.perm(a,b) if 0 <= b <= a else 0
                add('fp', (a,b), [q, q, q % p])
                if b <= 0 or a == 0 or a+b-1 <= n:
                    r = 0 if b < 0 else 1 if b == 0 else 0 if a == 0 else math.comb(a+b-1,b)
                    add('fr', (a,b), [r, r, r % p])
        for _ in range(30):
            parts, remaining = [], n
            for _ in range(rng.randrange(7)):
                k = rng.randrange(remaining+1)
                parts.append(k); remaining -= k
            v = multinomial(parts)
            add('fm', (len(parts), *parts), [v, v, v % p])
    big = {'quick': 100000, 'full': 1000000, 'stress': 3000000}[args.mode]
    for m in (20000, big):
        add('pb', (m,), [format(product(primes_upto(m)),'X')])
    for size in (0, 1, 300, big):
        add('ls', (size,), [1])
        exhaustive = 120 if size >= 120 else size
        for a in range(exhaustive+1):
            add('lf', (a,), [ln_factorial(a), log_tolerance(a)])
            for b in range(-1, a+2):
                add('lb', (a,b), log_case(a,b))
                add('lp', (a,b), log_case(a,b,True))
        if size <= exhaustive:
            continue
        for a in sorted({size, size-1, min(size, 3000), min(size, 3001)} | {rng.randrange(size+1) for _ in range(40)}):
            add('lf', (a,), [ln_factorial(a), log_tolerance(a)])
            for b in sorted({0, 1, 2, 40, a//2, a-1, a, a+1, -1} | {rng.randrange(a+1) for _ in range(3)}):
                add('lb', (a,b), log_case(a,b))
                add('lp', (a,b), log_case(a,b,True))
    large = list(range(41)) + [2999, 3000, 3001, 10**4, 10**6, 10**9, 10**12, 10**15, 10**18, 1<<62, (1<<63)-2, (1<<63)-1]
    large += [rng.randrange(1<<rng.randrange(1,64)) for _ in range({'quick': 20, 'full': 200, 'stress': 2000}[args.mode])]
    for a in sorted(set(large)):
        add('LF', (a,), [ln_factorial(a), log_tolerance(a)])
        for b in sorted({-1, 0, 1, 2, a//3, a//2, a-1, a} | {a+1} if a < (1<<63)-1 else {-1, 0, 1, a//2, a}):
            add('LB', (a,b), log_case(a,b))
            add('LP', (a,b), log_case(a,b,True))
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
            add(op,(n,),[a,b,(2*b-a)%p])
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
    result = timed_run(f'05-combinatorics oracle {Path(command[0]).name}', command,input=payload,text=True,capture_output=True,env=env,timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={command} returncode={result.returncode}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f'command={command} expected {len(cases)} lines actual={len(lines)}')
    for (op,values,expected),line in zip(cases,lines):
        if op in ('lf','lb','lp','LF','LB','LP'):
            actual, (want, tol) = decimal.Decimal(line), expected
            ok = actual == want if want.is_infinite() else LOG.subtract(actual, want).copy_abs() <= tol
        else:
            actual = line.split()
            ok = actual == [x if isinstance(x,str) else str(x) for x in expected]
        if not ok:
            raise RuntimeError(f'smallest known reproducer={op} {values} expected={expected} actual={actual} command={command}')
    print(f'PASS Python exact integers, matrix powers and enumerated combinatorics: {len(cases)} cases seed={args.seed}',flush=True)


if __name__ == '__main__':
    raise SystemExit(main('05-combinatorics', [
        'negative-n','intmax-n','modulus-one','composite','prime-flag','n-equals-mod','n-above-mod','stale-modulus',
        'negative-choice','negative-choice-rep','negative-permutation','negative-permutation-rep',
        'factorial-negative','factorial-bound','inverse-bound','derangement-bound','combination-bound',
        'replacement-bound','permutation-bound','stars-total','stars-parts','stars-bound','catalan-negative',
        'catalan-bound','ballot-negative','ballot-bound','multinomial-negative','multinomial-bound',
        'large-bound','table-negative','derangement-table-negative',
        'negative-binomial-negative','negative-binomial-bound','inverse-negative','inverse-outside',
        'combi-inverse-bound','circular-negative','circular-bound','double-factorial-negative',
        'double-factorial-bound','primorial-negative','log-negative-n','log-intmax-n',
        'log-factorial-negative','log-factorial-bound','log-binomial-negative','log-binomial-bound',
        'log-permutation-negative','log-permutation-bound','stale-modulus-larger',
        'circular-zero-bound','nonadjacent-negative','nonadjacent-bound','multifactorial-zero-step',
        'multifactorial-exact-zero-step','log-large-factorial-negative','log-large-binomial-negative',
        'log-large-permutation-negative',
        *('field-' + op for op in ('negative-n','intmax-n','factorial-negative','factorial-bound','inverse-negative',
            'inverse-bound','combination-negative','combination-bound','replacement-negative','replacement-bound',
            'permutation-negative','permutation-bound','multinomial-negative','multinomial-bound')),
        *('stale-' + op for op in ('factorial','inverse-factorial','derangement','double-factorial',
            'replacement','negative-binomial','combi-inverse','permutation','circular','permutation-rep',
            'stars','catalan','ballot','multinomial','large','nonadjacent'))],oracle))
