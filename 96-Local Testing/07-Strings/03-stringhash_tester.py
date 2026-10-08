"""Double-prime, 2^64-word and 2^61-1 fingerprints and LCP/LCS against exact independent oracles.

For each of the three fields, quick/full/stress exhaust binary strings through
4/7/9, add 20/150/1000 random strings with 100 range comparisons per pair,
30/300/2000 caller-functor builds over codes beyond every modulus, 20/100/500
digest-set/ordering strings, and 10000/200000/1000000-byte fixtures. All modes
cover every operation: arithmetic helpers on boundary residues against 128-bit
references, encode reduction, hashOf, Digest ordering, random-base ranges,
context/length tags, copying, concatenation beyond precomputed powers, and a
deliberate unsigned-word Thue-Morse collision that the two prime fields separate.
Python checks polynomial values by arbitrary-precision weighted power sums.
Full/stress add ASan/UBSan with leak checking; assertions run in checked builds.
"""
import random
import subprocess
from _00_runner import main


def oracle(binary, args, env):
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
            result = subprocess.run(command, input=payload, capture_output=True, text=True,
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
        'base64-even', 'base61-low', 'base61-high', 'alphabet', 'hashof-alphabet', 'build-negative', 'left', 'order', 'right', 'concat-base',
        'lcp-base', 'lcs-range', 'power', 'length-overflow'], oracle, strict=True))
