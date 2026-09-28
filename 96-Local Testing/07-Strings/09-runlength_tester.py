"""Run-length encoding, preflight, decoding and span verification.

Quick/full/stress enumerate ternary sequences through length 5/8/10 and run
300/4000/20000 deterministic random cases. Every mode exercises every API,
malformed/nonmaximal runs, limits and aliases; full/stress add sanitizer builds.
Independent substring-boundary, direct expansion and signed-128-bit size oracles
cover exact values, full byte/integer alphabets and signed-64-bit count overflow.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('09-runlength', ['source-size', 'count-cap', 'size-cap', 'decode-cap', 'span-cap']))
