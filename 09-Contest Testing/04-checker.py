#!/usr/bin/env python3
"""Batch checker: input, candidate output, reference output; 0 AC, 1 WA, 2 error."""
from pathlib import Path
import sys

if len(sys.argv) != 4:
    sys.exit(2)
_, candidate, reference = map(Path, sys.argv[1:])
try:
    same = candidate.read_bytes().split() == reference.read_bytes().split()
except OSError as e:
    print(f"checker error: {e}", file=sys.stderr)
    sys.exit(2)
sys.exit(0 if same else 1)
