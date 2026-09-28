#!/usr/bin/env python3
"""Short batch CLI; keep beside 01-stress.py, which supplies the common engine."""
import argparse
from pathlib import Path
import runpy
import sys
import tempfile


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("generator")
    p.add_argument("candidate")
    p.add_argument("brute")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--count", type=int, default=0, help="0 = until failure or Ctrl-C")
    p.add_argument("--timeout", type=float, default=2)
    p.add_argument("--artifacts", type=Path,
                   default=Path(tempfile.gettempdir()) / "contest-stress-artifacts")
    args = p.parse_args()
    runner = runpy.run_path(str(Path(__file__).resolve().with_name("01-stress.py")))
    return runner["main"]([
        f"--seed={args.seed}", f"--count={args.count}",
        f"--timeout={args.timeout}", f"--artifacts={args.artifacts}", "--",
        args.generator, args.candidate, args.brute,
    ])


if __name__ == "__main__":
    sys.exit(main())
