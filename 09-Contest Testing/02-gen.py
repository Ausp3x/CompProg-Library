#!/usr/bin/env python3
"""Example complete-input generator: replace the body for your problem."""
import random
import sys

if len(sys.argv) != 2:
    sys.exit(2)
try:
    seed = int(sys.argv[1])
except ValueError:
    sys.exit(2)
rng = random.Random(seed)
n = rng.randint(1, 20)
a = [rng.randint(-100, 100) for _ in range(n)]
print(n)
print(*a)
