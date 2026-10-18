"""Library Checker-size timings for 67-lattice_paths.hpp: divide and conquer on bounds, the rectangle transfer, Bizley, obstacles and the tables.

Builds the C++ workload with -O2 (no -march), runs one warmup plus N timed repetitions per
workload, checks repeat outputs, and writes medians with CPU and compiler conditions to
67-lattice_paths_benchmark.json beside this driver.
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
STEM = '67-lattice_paths'


def main():
    reps = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    cxx = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2']
    with tempfile.TemporaryDirectory(prefix='cp-bench-') as d:
        exe = Path(d) / 'bench'
        subprocess.run([cxx, *flags, str(HERE / f'{STEM}_benchmark.cpp'), '-o', str(exe)], check=True)
        out = subprocess.run([str(exe), str(reps)], check=True, capture_output=True, text=True, timeout=3600).stdout
    rows = json.loads(out)
    cpu = next((l.split(':', 1)[1].strip() for l in open('/proc/cpuinfo') if l.startswith('model name')), platform.processor())
    version = subprocess.run([cxx, '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    record = {'cpu': cpu, 'compiler': version, 'flags': flags, 'reps': reps, 'warmup': 1, 'rows': rows}
    (HERE / f'{STEM}_benchmark.json').write_text(json.dumps(record, indent=2) + '\n')
    print(f'{cpu} | {version} | {" ".join(flags)} | median of {reps}')
    for r in rows:
        print(f"{r['workload']:<52} {r['median_ms']:>10.1f} ms")


if __name__ == '__main__':
    main()
