"""Fast I/O against independent std::to_chars/Python decimals and FILE faults.

Quick exhausts 8-bit domains and 100 samples/type. Full exhausts 16-bit domains,
adds 3000 samples/type, 1 MiB binary round trips and optimized/checked/ASan-UBSan
builds. Stress increases random samples to 30000/type and binary payload to 8 MiB.
All modes cover every API, integer widths through 128 bits, status/recovery,
buffer sizes 1/7/65536, embedded NUL/high bytes, pipes, borrowed ownership,
destructor/explicit flush and fault-injected input/output errors.
"""
from _00_runner import main
import math
import random
import struct
import subprocess


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    rows = [(-(1 << 127), (1 << 128) - 1), ((1 << 127) - 1, 0), (0, 1)]
    rows += [(rng.randrange(-(1 << 127), 1 << 127), rng.getrandbits(128))
             for _ in range(100 if args.mode == 'quick' else 3000 if args.mode == 'full' else 30000)]
    expected = ''.join(f'{a} {b}\n' for a, b in rows)
    run = subprocess.run([str(binary), '--roundtrip-128'], input=expected, capture_output=True,
                         text=True, env=env, timeout=180)
    if run.returncode or run.stdout != expected:
        mismatch = next((i for i, (a, b) in enumerate(zip(run.stdout, expected)) if a != b),
                        min(len(run.stdout), len(expected)))
        raise RuntimeError(f'Python 128-bit oracle: command={binary} --roundtrip-128 '
                           f'returncode={run.returncode} offset={mismatch} '
                           f'expected={expected[mismatch:mismatch+100]!r} '
                           f'actual={run.stdout[mismatch:mismatch+100]!r} stderr={run.stderr}')
    print(f'PASS Python decimal 128-bit oracle: {len(rows)} signed/unsigned pairs', flush=True)

def oracle_text(binary, args, env):
    oracle(binary, args, env)
    rng = random.Random(args.seed + 1)
    count = 100 if args.mode == 'quick' else 3000 if args.mode == 'full' else 30000

    def call(flag, data):
        run = subprocess.run([str(binary), flag], input=data, capture_output=True, env=env, timeout=180)
        if run.returncode:
            raise RuntimeError(f'command={binary} {flag} returncode={run.returncode} stderr={run.stderr!r}')
        return run.stdout

    data = bytes(rng.choice(b'ab\r\n \t\x00\xff') for _ in range(20 * count)) + b'tail\r'
    want = b''.join(b'%d:%s\n' % (len(x), x) for x in
                    ((l[:-1] if l.endswith(b'\r') else l) for l in data.split(b'\n')))
    got = call('--lines', data)
    if got != want:
        at = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
        raise RuntimeError(f'Python readLine oracle offset={at} expected={want[at:at+60]!r} actual={got[at:at+60]!r}')
    tokens = ['1e400', '-1e400', '1e-400', '0x1p3', '1.5e', '+-2', '.', 'nan(', '1..2', '2.4703282292062328e-324']
    for _ in range(count):
        mant = ''.join(rng.choice('0123456789') for _ in range(rng.randint(1, 25)))
        dot = rng.randint(0, len(mant))
        body = (mant[:dot] + '.' + mant[dot:]) if rng.random() < 0.7 else mant
        if rng.random() < 0.6:
            body += rng.choice('eE') + rng.choice(['', '+', '-']) + str(rng.randint(0, 330))
        tokens.append(rng.choice(['', '+', '-']) + body)
    want = []
    for t in tokens:
        try:
            if t.startswith('+-') or t.lower().startswith(('0x', '+0x', '-0x')):
                raise ValueError
            v = float(t)
            want.append('O' if math.isinf(v) or (v == 0 and any(c in '123456789' for c in t.split('e')[0].split('E')[0])) else v)
        except ValueError:
            want.append('I')
    got = call('--doubles', ' '.join(tokens).encode()).decode().split()
    got = [x if x in 'IO' else float.fromhex(x) for x in got]
    bad = [(t, w, g) for t, w, g in zip(tokens, want, got) if w != g or (w != 'I' and w != 'O' and math.copysign(1, w) != math.copysign(1, g))]
    if len(got) != len(tokens) or bad:
        raise RuntimeError(f'Python readDouble oracle count={len(got)}/{len(tokens)} first mismatch={bad[:1]}')
    rows = [(0, 0), (1 << 63, 3), (0x7FEFFFFFFFFFFFFF, 200), (1, 200), (0x3FF8000000000000, 0), (0x4004000000000000, 0)]
    while len(rows) < count:
        bits = rng.getrandbits(64)
        if (bits >> 52) & 0x7FF != 0x7FF:
            rows.append((bits, rng.choice([0, 1, 2, 6, 9, 12, 15, 17, 20, 30, 200])))
    want = ''.join(f'{struct.unpack("<d", struct.pack("<Q", b))[0]:.{prec}f}\n' for b, prec in rows)
    got = call('--write-doubles', ''.join(f'{b} {prec}\n' for b, prec in rows).encode()).decode()
    if got != want:
        at = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
        raise RuntimeError(f'Python writeDouble oracle offset={at} expected={want[at:at+80]!r} actual={got[at:at+80]!r}')
    print(f'PASS Python readLine/readDouble/writeDouble oracles: {len(data)} bytes, {len(tokens)} tokens, {len(rows)} values', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('03-fastio', ['null-input', 'null-output', 'precision-negative', 'precision-large'], oracle_text))
