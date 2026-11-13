"""Double-prime, 2^64-word and 2^61-1 fingerprints and LCP/LCS against exact independent oracles.

For each of the three fields, quick/full/stress exhaust binary strings through
4/7/9, add 20/150/1000 random strings with 100 range comparisons per pair,
30/300/2000 caller-functor builds over codes beyond every modulus, 20/100/500
digest-set/ordering strings, and 10000/200000/1000000-byte fixtures.
longestCommonSubstringHash runs on all pairs of binary strings through 4/6/7
and 200/2000/10000 random pairs (alphabets 1-4, lengths below 60, both orders
and self) against an O(n*m) DP with the tie rule, plus planted, unary and
no-common-symbol fixtures of the large lengths and 2^16-2/2^18-2/2^20-2
distinct shuffled symbols against a disjoint set and themselves; its lengths
are cross-checked (double-prime field) with row 07's suffix-array
longestCommonSubstring on each s/t pair and the planted, unary-cut and
distinct-disjoint fixtures. All modes
cover every operation: arithmetic helpers on boundary residues against 128-bit
references, encode reduction, hashOf, Digest ordering, random-base ranges,
context/length tags, copying, concatenation beyond precomputed powers, and a
deliberate unsigned-word Thue-Morse collision that the two prime fields separate.
Python checks polynomial values by arbitrary-precision weighted power sums and,
once per full or stress run, that the KIND static_assert admits 0..2 only
(four parallel -fsyntax-only compiles).
Full/stress add ASan/UBSan with leak checking; assertions run in checked builds.
"""
import os
import random
import shlex
import subprocess
import sys
import threading
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run


HEADER = Path(__file__).resolve().parents[2] / '07-Strings' / '03-stringhash.hpp'
KIND_DOMAIN_CHECKED = False
KIND_DOMAIN_LOCK = threading.Lock()


def kind_domain(env):
    """StringHash<KIND> compiles for KIND 0..2 and fails its static_assert for -1 and 3 (-fsyntax-only)."""
    global KIND_DOMAIN_CHECKED
    with KIND_DOMAIN_LOCK:
        if not KIND_DOMAIN_CHECKED:
            kind_domain_compiles(env)
            KIND_DOMAIN_CHECKED = True


def kind_domain_compiles(env):
    command = [*shlex.split(env.get('CXX', os.environ.get('CXX', 'g++'))), '-std=gnu++20', '-fsyntax-only', '-x', 'c++', '-']
    cases = [(kind, valid, subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env))
             for kind, valid in ((0, True), (2, True), (-1, False), (3, False))]
    for kind, valid, process in cases:
        source = f'#include "{HEADER}"\nint main() {{ StringHash<{kind}> h("ab"); return h.size(); }}\n'
        _, stderr = process.communicate(source, timeout=120)
        ok = process.returncode == 0 if valid else process.returncode != 0 and 'static assertion failed' in stderr
        if not ok:
            raise RuntimeError(f'command={command} KIND={kind} expected {"compile" if valid else "static_assert failure"} '
                               f'returncode={process.returncode}\n{stderr[-2000:]}')
    print('PASS stringhash KIND domain: 0 and 2 compile, -1 and 3 fail the static_assert', flush=True)


def oracle(binary, args, env):
    if args.mode != 'quick':
        kind_domain(env)
    rng = random.Random(args.seed)
    count = 0
    for kind in ('double', '64', '61'):
        moduli = {'double': [1000000007, 1000000009], '64': [1 << 64], '61': [(1 << 61) - 1]}[kind]
        alphabet = 1000000006 if kind == 'double' else 1 << 32
        samples = [[], [0], [alphabet - 1], [0, 255, 256, alphabet - 1]]
        samples += [[rng.randrange(alphabet) for _ in range(rng.randrange(20))]
                    for _ in range(5 if args.mode == 'quick' else 30)]
        for symbols in samples:
            command = [str(binary), '--oracle', kind]
            payload = f'{args.seed} {len(symbols)}\n' + ' '.join(map(str, symbols)) + '\n'
            result = timed_run(f'03-stringhash oracle {kind} {Path(command[0]).name}', command, input=payload, capture_output=True, text=True,
                                                       env=env, timeout=30)
            if result.returncode:
                raise RuntimeError(f'command={command} returncode={result.returncode} '
                                   f'input={payload!r}\n{result.stdout}\n{result.stderr}')
            rows = result.stdout.splitlines()
            bases = list(map(int, rows.pop(0).split()))
            expected = []
            for left in range(len(symbols) + 1):
                for right in range(left, len(symbols) + 1):
                    part = symbols[left:right]
                    expected.append([sum((x + 1) * pow(base, len(seq) - 1 - i, mod)
                                         for i, x in enumerate(seq)) % mod
                                     for seq in (part, part[::-1])
                                     for base, mod in zip(bases, moduli)])
            actual = [list(map(int, row.split())) for row in rows]
            if actual != expected:
                raise RuntimeError(f'command={command} seed={args.seed} symbols={symbols} '
                                   f'bases={bases} expected={expected} actual={actual}')
            count += len(expected)
    print(f'PASS stringhash Python exact weighted-power oracle: {count} intervals', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('03-stringhash', ['base-low', 'base-high', 'base64-low',
        'base64-even', 'base61-low', 'base61-high', 'base61-above', 'alphabet', 'hashof-alphabet', 'build-negative', 'build-max', 'hashof-length', 'left', 'order', 'right', 'concat-base', 'concat-base-left',
        'lcp-base', 'lcs-range', 'lcsubstring-base', 'lcsubstring-moved', 'power', 'length-overflow'], oracle, extra=('-Wshadow',)))
