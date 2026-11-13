#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('05-prefix_sum', ['negative-size', 'oversize', 'negative-dimension', 'ragged-prefix', 'ragged-difference', 'prefix-negative', 'prefix-end', 'range-reversed', 'range-end', '2d-prefix', '2d-range', 'difference-negative', 'difference-end', 'difference-reversed', '2d-add-end', '2d-add-reversed',
                                         'arith-negative', 'arith-end', 'arith-reversed', 'product-negative', 'product-end', 'product-reversed',
                                         '3d-negative-dimension', '3d-oversize', '3d-difference-negative', '3d-difference-oversize', '3d-difference-flat-oversize',
                                         '3d-ragged-plane', '3d-ragged-row', '3d-prefix', '3d-range-reversed', '3d-range-end',
                                         '3d-add-reversed', '3d-add-end',
                                         'step-zero', 'step-class-end', 'step-class-past-end', 'difference-negative-size', 'step-class-negative', 'step-range-reversed', 'step-range-end', 'difference-max-size', '2d-difference-max-rows', '2d-difference-max-columns']))
