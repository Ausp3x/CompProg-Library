"""Scalar words: exhaustive 8-bit (quick), 16-bit (full), bit-position and Python oracles.

All modes cover every public operation and unsigned 8/16/32/64/128-bit domains.
Quick: 100 random words/type per section, exhaustive 8-bit words (all byte-test n and m, all
swapBitRanges triples), all 8-bit nextSubmask/prevSupermask traversals, xorRangeBlocks over every
8-bit lo<=hi pair for 4 x, xorSubmaskLowerBound for 16 b x all s x all lo.
Full adds sanitizers, 3000 random words/type, exhaustive 16-bit words (all n, sampled m),
xorRangeBlocks for 40 x and xorSubmaskLowerBound over all 2^24 triples; stress adds 30000/type and all 256 x.
Signed/bool/cv-qualified/nonword template types are compile-rejected.
"""
import os
from pathlib import Path
import random
import shlex
import tempfile

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run
from _00_cpp_test_runner import compiler_command

ROOT = Path(__file__).resolve().parents[2]


def block_range(rng, width, mask):
    """A half-open [lo, hi) below 2^width - 1: short, ending at the top, empty or full, or random."""
    kind = rng.randrange(4)
    if kind == 0:
        lo = rng.getrandbits(width)
        return lo, min(mask, lo + rng.randint(0, 64))
    if kind == 1:
        return mask - rng.randint(0, min(64, mask)), mask
    if kind == 2:
        lo = rng.getrandbits(width)
        return (lo, lo) if rng.randrange(2) else (0, mask)
    return tuple(sorted((rng.getrandbits(width), rng.getrandbits(width))))


def blocks_cover(width, lo, hi, xr, fields):
    """Exact set check of xorRangeBlocks: aligned blocks whose images under ^xr are disjoint inside [lo, hi), hi - lo in total."""
    if not fields or int(fields[0]) != len(fields) - 1 or len(fields) - 1 > 2 * width:
        return False
    blocks = []
    for item in fields[1:]:
        base, k = item.split(':')
        base, k = int(base, 16), int(k)
        if not 0 <= k < width or base % (1 << k):
            return False
        blocks.append((base, k))
    # The image of an aligned block under xor is the aligned block holding base ^ xr.
    images = sorted(((base ^ xr) >> k << k, ((base ^ xr) >> k << k) + (1 << k)) for base, k in blocks)
    if any(a[1] > b[0] for a, b in zip(images, images[1:])) or any(s < lo or e > hi for s, e in images):
        return False
    if sum(e - s for s, e in images) != hi - lo:
        return False
    if hi - lo <= 64:
        return sorted((base + t) ^ xr for base, k in blocks for t in range(1 << k)) == list(range(lo, hi))
    return True


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    fixtures = []
    expected = []
    ranges = []
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
            count = len(positions)
            k = max(0, rng.choice((0, count - 1, count, count + 1, rng.randint(0, width), (1 << 31) - 1)))
            at = rng.randint(0, width)
            sign_bits = rng.randint(1, width)
            lo, hi = block_range(rng, width, mask)
            xr = rng.getrandbits(width)
            half = width // 2
            even = sum((x >> 2 * t & 1) << t for t in range(half))
            odd = sum((x >> 2 * t + 1 & 1) << t for t in range(half))
            low, high = x & ((1 << half) - 1), x >> half
            encoded = sum((low >> t & 1) << 2 * t | (high >> t & 1) << 2 * t + 1 for t in range(half))
            # Sign extension copies the character of bit b-1 into every higher place of the binary string.
            top = width - sign_bits
            extended = int(bits[top] * top + bits[top:], 2)
            fixtures.append(f'{width} {x:x} {shift} {k} {at} {sign_bits} {lo:x} {hi:x} {xr:x}')
            ranges.append((width, lo, hi, xr))
            expected.append([str(x.bit_count()), str(x.bit_length()), str(first), str(last),
                             str(width - x.bit_length()), str(first if x else width),
                             f'{x & -x:x}', f'{floor:x}', str(int(fits)), f'{ceil:x}',
                             str(int(more)), f'{next_value:x}', f'{left:x}', f'{right:x}',
                             str(x.bit_count() % 2), f'{gray:x}', f'{decoded:x}',
                             str(positions[k] if k < count else -1), str(sum(p < at for p in positions)),
                             f'{int(bits[::-1], 2):x}', f'{even:x}', f'{odd:x}', f'{encoded:x}', f'{extended:x}'])
    command = [str(binary), '--oracle']
    result = timed_run(f'06-bit_operations oracle {Path(command[0]).name}', command, input='\n'.join(fixtures) + '\n', text=True,
                                                   capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={shlex.join(command)} exit={result.returncode}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != len(expected):
        raise RuntimeError(f'oracle output rows expected={len(expected)} actual={len(lines)}')
    for fixture, want, got, block in zip(fixtures, expected, lines, ranges):
        fields = got.split()
        if fields[:len(want)] != want:
            raise RuntimeError(f'seed={args.seed} operation=Python word oracle input={fixture}'
                               f' expected={want} actual={got}')
        if not blocks_cover(*block, fields[len(want):]):
            raise RuntimeError(f'seed={args.seed} operation=xorRangeBlocks Python set oracle input={fixture}'
                               f' (width lo hi x)={block} actual={fields[len(want):]}')
    print(f'PASS exact Python bit oracle: {len(fixtures)} cases', flush=True)


def compile_domains():
    header = ROOT / '06-Miscellaneous/06-bit_operations.hpp'
    with tempfile.TemporaryDirectory(prefix='cp-bit-types-') as name:
        source = Path(name) / 'types.cpp'
        for typ in ('int', 'lng', 'lll', 'bool', 'const uint', 'char8_t'):
            source.write_text(f'#include "{header}"\nBitOps<{typ}> invalid;\n')
            command = [*compiler_command(), '-std=gnu++20', '-fsyntax-only', str(source)]
            result = timed_run(f'06-bit_operations rejection compile {typ}', command, text=True, capture_output=True, timeout=180)
            if result.returncode == 0 or 'constraint' not in result.stderr.lower():
                raise RuntimeError(f'compile rejection type={typ} command={shlex.join(command)}'
                                   f' expected=constraint failure actual={result.returncode}\n{result.stderr}')
    with tempfile.TemporaryDirectory(prefix='cp-bit-types-') as name:
        source = Path(name) / 'ones.cpp'
        source.write_text(f'#include "{header}"\nauto ones = BitOps<ulll>::countOnesUpTo(0);\n')
        command = [*compiler_command(), '-std=gnu++20', '-fsyntax-only', str(source)]
        result = timed_run('06-bit_operations rejection compile countOnesUpTo ulll', command, text=True, capture_output=True, timeout=180)
        if result.returncode == 0 or 'constraint' not in result.stderr.lower():
            raise RuntimeError(f'compile rejection countOnesUpTo<ulll> command={shlex.join(command)}'
                               f' expected=constraint failure actual={result.returncode}\n{result.stderr}')
    print('PASS signed/bool/cv/nonword and 128-bit countOnesUpTo compile rejections', flush=True)


if __name__ == '__main__':
    compile_domains()
    raise SystemExit(main('06-bit_operations', [
        'mask-negative', 'mask-large', 'test-negative', 'test-large', 'set-negative',
        'set-large', 'flip-negative', 'flip-large', 'test-beyond', 'set-beyond', 'flip-beyond', 'left-negative', 'left-large',
        'right-negative', 'right-large', 'not-submask', 'not-supermask', 'outside-full', 'mask-outside-full',
        'select-negative', 'rank-negative', 'rank-large', 'sign-zero', 'sign-large', 'swap-overlap',
        'swap-count-negative', 'swap-index-negative', 'swap-first-large', 'swap-second-large', 'morton-x', 'morton-y', 'morton-both',
        'equal-negative', 'equal-large', 'less-negative', 'less-large', 'greater-negative', 'greater-large',
        'between-m-negative', 'between-m-large', 'between-n-negative', 'between-n-large', 'next-submask-invalid',
        'prev-supermask-not-super', 'prev-supermask-outside-full', 'prev-supermask-mask-outside',
        'xor-range-reversed'], oracle))
