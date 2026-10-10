"""Compare the exact single-pattern matchers on bytes and integer sequences; emit JSONL medians."""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261010)
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    print(json.dumps({'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                      'flags': flags, 'cpu': cpu, 'python': platform.python_version(),
                      'seed': args.seed, 'warmup': 1, 'repetitions': 5, 'statistic': 'median',
                      'memory': 'prefix automaton (m + 1) * 256 ints; others O(m + 256)'}), flush=True)
    with tempfile.TemporaryDirectory(prefix='cp-matching-bench-') as tmp:
        binary = Path(tmp) / 'bench'
        for command in ([compiler, *flags, str(HERE / '16-string_matching_benchmark.cpp'), '-o', str(binary)],
                        [str(binary), str(args.seed)]):
            try:
                subprocess.run(command, check=True, timeout=900)
            except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
                raise SystemExit(f'FAIL seed={args.seed} command={shlex.join(command)} error={error}')


if __name__ == '__main__':
    main()
