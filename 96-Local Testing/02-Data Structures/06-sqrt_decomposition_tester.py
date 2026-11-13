#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-sqrt_decomposition', ["generic-negative-size","generic-negative-block","generic-get-negative","generic-get-end","generic-set-end","generic-apply-end","generic-prod-negative","generic-prod-reversed","generic-prod-end","generic-rebuild-block","lazy-negative-size","lazy-negative-block","lazy-get-end","lazy-get-past-end","lazy-set-end","lazy-sum-negative","lazy-sum-reversed","lazy-sum-end","lazy-affine-negative","lazy-affine-reversed","lazy-affine-end","lazy-vector-negative-block","sorted-negative-block","sorted-get-negative","sorted-get-end","sorted-get-past-end","sorted-add-negative","sorted-add-reversed","sorted-add-end","sorted-assign-negative","sorted-assign-reversed","sorted-assign-end","sorted-countless-end","sorted-countleq-reversed","sorted-countgeq-reversed","sorted-countgeq-end","sorted-countgreater-reversed","sorted-countgreater-end","sorted-countequal-reversed","sorted-countequal-end","sorted-maxless-negative","sorted-maxless-end","decomp-negative-size","decomp-negative-block","decomp-apply-negative","decomp-apply-reversed","decomp-apply-end","decomp-prod-negative","decomp-prod-reversed","decomp-prod-end","list-negative-block","list-insert-negative","list-insert-end","list-erase-negative","list-erase-end","list-erase-empty","list-at-negative","list-at-end","list-const-at-end","generic-max-size"]))
