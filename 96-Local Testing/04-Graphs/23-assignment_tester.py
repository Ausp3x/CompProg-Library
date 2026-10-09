#!/usr/bin/env python3
"""Assignment (Hungarian, k-best, sparse SSP) versus exhaustive enumeration.

quick: every forbidden pattern up to 3x3, 300 random dense/sparse cases per
value type (int, lng and lll at the documented domain bound), 60x80 dense,
500-vertex sparse, k-best 6x6. full: 3000 random cases per type, 150x200
dense with dual certificates, 2000-vertex sparse, k-best 8x8 (400 best of
40320), in optimized, checked and ASan/UBSan builds. stress: 20000 random
cases per type, 400x533 dense, 8000-vertex sparse.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('23-assignment', ['ragged', 'mask-shape', 'k-negative',
        'sparse-range', 'legacy-range'], strict=True))
