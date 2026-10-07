"""Scalar words: exhaustive 8-bit (quick), 16-bit (full), bit-position and Python oracles.

All modes cover every public operation and unsigned 8/16/32/64/128-bit domains.
Full adds sanitizers and 3000 random words/type; stress adds 30000/type.
Signed/bool/cv-qualified/nonword template types are compile-rejected.
"""
import os
from pathlib import Path
import random
import shlex
import subprocess
import tempfile

from _00_runner import main

ROOT = Path(__file__).resolve().parents[2]


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    fixtures = []
    expected = []
    for width in (8, 16, 32, 64, 128):
        mask = (1 << width) - 1
        values = [0, 1, mask, mask - 1]
        for i in range(width):
            values.extend((1 << i, (1 << i) - 1, mask ^ (1 << i)))
        values.extend(rng.getrandbits(width) for _ in range(
            100 if args.mode == 'quick' else 3000 if args.mode == 'full' else 30000))
        for x in values:
            shift = rng.choice((-(1 << 31), (1 << 31) - 1, rng.randint(-1000, 1000)))
            first = (x & -x).bit_length() - 1
            last = x.bit_length() - 1
            floor = 1 << last if x else 0
            ceil = 1 << (x - 1).bit_length() if x > 1 else 1
            fits = ceil <= mask
            if not fits:
                ceil = 93
            # Enumerate the changed set-bit position, not Gosper's word formula.
            positions = [i for i in range(width) if x >> i & 1]
            next_value = x
            more = False
            for j, p in enumerate(positions):
                end = positions[j + 1] if j + 1 < len(positions) else width
                if p + 1 < end:
                    next_positions = list(range(j)) + [p + 1] + positions[j + 1:]
                    next_value = sum(1 << i for i in next_positions)
                    more = True
                    break
            # Rotate a binary string independently of machine shifts.
            bits = format(x, f'0{width}b')
            r = shift % width
            left = int(bits[r:] + bits[:r], 2)
            right = int(bits[-r:] + bits[:-r], 2) if r else x
            gray = int(''.join('1' if a != b else '0' for a, b in zip('0' + bits, bits)), 2)
            decoded, acc = 0, 0
            for ch in bits:
                acc ^= ch == '1'
                decoded = 2 * decoded + acc
            fixtures.append(f'{width} {x:x} {shift}')
            expected.append([str(x.bit_count()), str(x.bit_length()), str(first), str(last),
                             str(width - x.bit_length()), str(first if x else width),
                             f'{x & -x:x}', f'{floor:x}', str(int(fits)), f'{ceil:x}',
                             str(int(more)), f'{next_value:x}', f'{left:x}', f'{right:x}',
                             str(x.bit_count() % 2), f'{gray:x}', f'{decoded:x}'])
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input='\n'.join(fixtures) + '\n', text=True,
                            capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={shlex.join(command)} exit={result.returncode}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != len(expected):
        raise RuntimeError(f'oracle output rows expected={len(expected)} actual={len(lines)}')
    for fixture, want, got in zip(fixtures, expected, lines):
        if got.split() != want:
            raise RuntimeError(f'seed={args.seed} operation=Python word oracle input={fixture}'
                               f' expected={want} actual={got}')
    print(f'PASS exact Python bit oracle: {len(fixtures)} cases', flush=True)


def compile_domains():
    header = ROOT / '06-Miscellaneous/06-bit_operations.hpp'
    with tempfile.TemporaryDirectory(prefix='cp-bit-types-') as name:
        source = Path(name) / 'types.cpp'
        for typ in ('int', 'lng', 'lll', 'bool', 'const uint', 'char8_t'):
            source.write_text(f'#include "{header}"\nBitOps<{typ}> invalid;\n')
            command = [os.environ.get('CXX', 'g++'), '-std=gnu++20', '-fsyntax-only', str(source)]
            result = subprocess.run(command, text=True, capture_output=True, timeout=180)
            if result.returncode == 0 or 'constraint' not in result.stderr.lower():
                raise RuntimeError(f'compile rejection type={typ} command={shlex.join(command)}'
                                   f' expected=constraint failure actual={result.returncode}\n{result.stderr}')
    print('PASS signed/bool/cv/nonword compile rejections', flush=True)


if __name__ == '__main__':
    compile_domains()
    raise SystemExit(main('06-bit_operations', [
        'mask-negative', 'mask-large', 'test-negative', 'test-large', 'set-negative',
        'set-large', 'flip-negative', 'flip-large', 'left-negative', 'left-large',
        'right-negative', 'right-large', 'not-submask', 'not-supermask', 'outside-full', 'mask-outside-full'], oracle))
