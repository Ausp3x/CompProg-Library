"""Exact arbitrary-precision hash oracles, collision regression and persistent two-TU checks."""
import os
from pathlib import Path
import random
import subprocess
import tempfile
from _00_runner import main


def oracle(binary, args, env):
    modulus = 1 << 64
    mask = modulus - 1

    def mix(x):
        x = (x + 0x9e3779b97f4a7c15) % modulus
        x = ((x ^ (x >> 30)) * 0xbf58476d1ce4e5b9) % modulus
        x = ((x ^ (x >> 27)) * 0x94d049bb133111eb) % modulus
        return x ^ (x >> 31)

    def unmix(x):
        for shift, multiplier in [(31, 0x94d049bb133111eb), (27, 0xbf58476d1ce4e5b9), (30, None)]:
            value = x
            for k in range(shift, 64, shift):
                value ^= x >> k
            x = value
            if multiplier:
                x = x * pow(multiplier, -1, modulus) % modulus
        return (x - 0x9e3779b97f4a7c15) % modulus

    rng = random.Random(args.seed)
    cases, expected = [], []

    def add(op, seed, *values):
        h = lambda x: mix((x + seed) % modulus)
        if op == 'm': value = mix(values[0])
        elif op == 'c': value = mix(values[0] ^ seed)
        elif op == 'i': value = h(values[0])
        elif op in ('w', 'z'): value = mix(h(values[1]) ^ values[0])
        elif op == 'p': value = mix(mix(seed ^ h(values[0])) ^ h(values[1]))
        elif op in ('v', 't'):
            value = seed
            for x in values[1:]: value = mix(value ^ h(x))
            value = mix(value ^ values[0])
        else:
            data = bytes.fromhex(values[0]) if values[0] != '-' else b''
            value = seed
            for start in range(0, len(data) // 8 * 8, 8):
                value = mix(value ^ int.from_bytes(data[start:start + 8], 'little'))
            tail = int.from_bytes(data[len(data) // 8 * 8:], 'little')
            value = mix(mix(value ^ tail) ^ len(data))
        cases.append((op, seed, *values)); expected.append(value)

    for seed in [0, 1, mask, args.seed]:
        for x in [0, 1, 2, 1 << 31, 1 << 32, 1 << 63, mask]:
            for op in ['m', 'c', 'i']: add(op, seed, x)
            for y in [0, 1, mask]:
                for op in ['p', 'w', 'z']: add(op, seed, x, y)
        for n in range(80):
            data = bytes(rng.getrandbits(8) for _ in range(n)); add('s', seed, data.hex() or '-')
        for values in [[], [0], [1, 2, 3], [mask, 0, mask], [0] * 20]:
            add('v', seed, len(values), *values)
        add('t', seed, 3, 0, mask, 123)
    for _ in range({'quick': 200, 'full': 5000, 'stress': 30000}[args.mode]):
        seed, x, y = (rng.getrandbits(64) for _ in range(3))
        for op in ['m', 'i']: add(op, seed, x)
        for op in ['p', 'w', 'z']: add(op, seed, x, y)
    left, right = unmix(0), unmix(1 << 63)
    if mix(left) != 0 or mix(right) != 1 << 63:
        raise RuntimeError('inverse SplitMix oracle did not round-trip')
    add('p', 0, left, 123); add('p', 0, right, 123)
    if expected[-1] == expected[-2]:
        raise RuntimeError('known old shift-combiner collision remains')
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=''.join(' '.join(map(str, row)) + '\n' for row in cases),
                       capture_output=True, text=True, env=env, timeout=180)
    if p.returncode:
        raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    actual = list(map(int, p.stdout.split()))
    if len(actual) != len(expected):
        raise RuntimeError(f'expected lines={len(expected)} actual={len(actual)}')
    for row, want, got in zip(cases, expected, actual):
        if want != got:
            raise RuntimeError(f'smallest known reproducer={row} expected={want} actual={got}')
    print(f'PASS exact Python hash oracle: {len(cases)} cases including old pair-collision regression', flush=True)
    if binary.name == 'optimized':
        root = Path(__file__).resolve().parents[2]
        includes = '\n'.join('#include "' + str(root / ('06-Miscellaneous/' + f)) + '"'
                             for f in ['01-random.hpp', '02-customhash.hpp'])
        with tempfile.TemporaryDirectory(prefix='cp-hash-multi-tu-') as name:
            directory = Path(name)
            (directory / 'a.cpp').write_text(includes + '''
const ulng early_a = CustomHash{}(123);
ulng hashA() { return early_a; }
const void *randomA() { return &rng; }
const void *seedA() { return &CustomHash::rnd; }
''')
            (directory / 'b.cpp').write_text(includes + '''
ulng hashA(); const void *randomA(); const void *seedA();
const ulng early_b = CustomHash{}(123);
int main() { return !(hashA() == early_b && randomA() == &rng && seedA() == &CustomHash::rnd); }
''')
            for sources in [('a.cpp', 'b.cpp'), ('b.cpp', 'a.cpp')]:
                commands = [[os.environ.get('CXX', 'g++'), '-std=gnu++20', '-O2', '-DNDEBUG',
                             *(str(directory / src) for src in sources), '-o', str(directory / 'test')],
                            [str(directory / 'test')]]
                for cmd in commands:
                    p = subprocess.run(cmd, capture_output=True, text=True, env=env, timeout=180)
                    if p.returncode:
                        raise RuntimeError(f'command={cmd} returncode={p.returncode}\n{p.stdout}\n{p.stderr}')
        print('PASS process hash seed/global Random identity: both two-TU link orders', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('02-customhash', [], oracle))
