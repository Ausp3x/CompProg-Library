#!/usr/bin/env python3
"""Permutation of 1..n, n >= 0: exit 0 valid, 1 invalid output, 2 tool error."""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    sys.exit(2)
try:
    n = int(Path(sys.argv[1]).read_bytes().split()[0])
    if n < 0:
        raise ValueError("n must be nonnegative")
    raw = Path(sys.argv[2]).read_bytes().split()
except (OSError, IndexError, ValueError) as e:
    print(f"validator error: {e}", file=sys.stderr)
    sys.exit(2)
try:
    output = list(map(int, raw))
except ValueError:
    sys.exit(1)
sys.exit(0 if len(output) == n and all(1 <= x <= n for x in output)
         and len(set(output)) == n else 1)
