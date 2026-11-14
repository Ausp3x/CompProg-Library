"""Run-length encoding, preflight, decoding, spans and the incremental RunLengthEncoder.

Quick/full/stress enumerate ternary sequences through length 5/8/10 and run
300/4000/20000 deterministic random cases. Every mode exercises every API,
malformed/nonmaximal runs, limits and aliases; full/stress add sanitizer builds.
Independent substring-boundary, direct expansion and signed-128-bit size oracles
cover exact values, full byte/integer alphabets, string/pair/floating symbols and
signed-64-bit count overflow; encoder push/pop sequences check a plain vector oracle.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('09-runlength', ['source-size', 'count-cap', 'size-cap', 'decode-cap', 'span-cap', 'push-count', 'push-overflow', 'pop-count', 'pop-length', 'construct-size'], extra=('-Wshadow',)))
