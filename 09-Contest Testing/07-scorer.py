#!/usr/bin/env python3
"""Example maximization score: count increasing adjacent pairs in valid output."""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    sys.exit(2)
try:
    output = list(map(int, Path(sys.argv[2]).read_bytes().split()))
except (OSError, ValueError) as e:
    print(f"scorer error: {e}", file=sys.stderr)
    sys.exit(2)
print(sum(a < b for a, b in zip(output, output[1:])))
