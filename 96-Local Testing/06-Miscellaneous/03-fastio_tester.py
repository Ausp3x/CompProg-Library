"""Fast I/O against independent std::to_chars/Python decimals and FILE faults.

Quick exhausts 8-bit domains and 100 samples/type. Full exhausts 16-bit domains,
adds 3000 samples/type, 1 MiB binary round trips and optimized/checked/ASan-UBSan
builds. Stress increases random samples to 30000/type and binary payload to 8 MiB.
All modes cover every API, integer widths through 128 bits, status/recovery,
buffer sizes 1/7/65536, embedded NUL/high bytes, pipes, borrowed ownership,
destructor/explicit flush and fault-injected input/output errors.
"""
from _00_runner import main
import random
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

if __name__ == '__main__':
    raise SystemExit(main('03-fastio', ['null-input', 'null-output'], oracle))
