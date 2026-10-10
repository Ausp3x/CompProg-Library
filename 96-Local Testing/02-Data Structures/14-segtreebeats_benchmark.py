#!/usr/bin/env python3
"""Times SegTreeBeats, HistoricSegTree and LazySegmentTreeBeats on random and adversarial workloads (n = q = 200000, median of 5)."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent

if __name__ == '__main__':
    compiler = os.environ.get('CXX', 'g++')
    with tempfile.TemporaryDirectory(prefix='cp-p028-') as name:
        binary = Path(name) / 'bench'
        subprocess.run([compiler, '-std=gnu++20', '-O2', '-DNDEBUG', str(HERE / '14-segtreebeats_benchmark.cpp'), '-o', str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)]).returncode)
