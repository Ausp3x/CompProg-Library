#!/usr/bin/env python3
"""Hidden x in [-100,100]. Up to 8 '? y' comparisons, then bare x to finish.

Print 'guess' first. A query replies '<', '=' or '>' comparing x with y.
Exit 0 accepts, 1 rejects, 2 reports a malformed hidden case/tool error.
"""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    sys.exit(2)
try:
    x = int(Path(sys.argv[1]).read_bytes())
    if not -100 <= x <= 100:
        raise ValueError("hidden integer must be in [-100,100]")
except (OSError, ValueError) as e:
    print(f"interactor error: {e}", file=sys.stderr)
    sys.exit(2)
print("guess", flush=True)
for turn in range(9):
    message = sys.stdin.buffer.readline().split()
    try:
        if len(message) == 1:
            sys.exit(0 if int(message[0]) == x else 1)
        if len(message) != 2 or message[0] != b"?" or turn == 8:
            sys.exit(1)
        y = int(message[1])
    except ValueError:
        sys.exit(1)
    print("<" if x < y else ">" if x > y else "=", flush=True)
