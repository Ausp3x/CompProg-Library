"""Convolution thresholds and headline timings for 13-convolution.hpp and 24-transform_algorithms.hpp.

Builds the C++ workload with -O2 (no -march), runs one warmup plus N timed repetitions per
workload, checks that the two compared implementations agree, and writes medians with
CPU/compiler conditions to 13-convolution_benchmark.json beside this driver. Rows: naive versus
the bare NTT path for a 2^16 operand against short operands (the NAIVE = 60 cutoff), naive
versus Karatsuba on balanced 64-bit words (the KARATSUBA = 32 base case), and library-checker
scale products, the 2^24 x 2^24 block product, subset/xor convolutions at n = 20 and gcd
convolution at 10^6.
"""
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    reps = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    cxx = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2']
    with tempfile.TemporaryDirectory(prefix='cp-bench-') as d:
        exe = Path(d) / 'bench'
        subprocess.run([cxx, *flags, str(HERE / '13-convolution_benchmark.cpp'), '-o', str(exe)], check=True)
        out = subprocess.run([str(exe), str(reps)], check=True, capture_output=True, text=True, timeout=3600).stdout
    rows = json.loads(out)
    cpu = next((l.split(':', 1)[1].strip() for l in open('/proc/cpuinfo') if l.startswith('model name')), platform.processor())
    version = subprocess.run([cxx, '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    record = {'cpu': cpu, 'compiler': version, 'flags': flags, 'reps': reps, 'warmup': 1, 'rows': rows}
    (HERE / '13-convolution_benchmark.json').write_text(json.dumps(record, indent=2) + '\n')
    print(f'{cpu} | {version} | {" ".join(flags)} | median of {reps}')
    for r in rows:
        print(' | '.join(f'{k}: {v:.2f}' if isinstance(v, float) else f'{v}' for k, v in r.items()))


if __name__ == '__main__':
    main()
