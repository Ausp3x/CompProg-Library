#!/usr/bin/env python3
"""Rational throughput benchmark; JSONL output, no timing gates.

Rational<lng> add, mul, sort and toDouble against an unchecked textbook fraction (128-bit
cross products and std::gcd; its results are compared with Rational's before the record
is emitted), plus Rational<lll> tryAdd, comparison and toDouble and Rational<InfInt>
addition. Uniform seeded operands of the stated bit sizes; every record is the median of
9 runs (3 for sorts, 5 for InfInt) over 200000 operations (50000 for lll, 20000 or 2000 for
InfInt); --mode quick divides the counts by ten.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap
from _00_cpp_test_runner import x86_only

_00_memory_cap.ensure()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full"), default="full")
    parser.add_argument("--output", type=Path, help="optional JSONL report (also printed)")
    args = parser.parse_args()
    cxx = shlex.split(os.environ.get("CXX", "g++"))
    source = Path(__file__).with_suffix(".cpp")
    cpuinfo = Path("/proc/cpuinfo").read_text() if Path("/proc/cpuinfo").exists() else ""
    cpu = next((line.split(":", 1)[1].strip() for line in cpuinfo.splitlines() if line.startswith("model name")), platform.processor())
    compiler = subprocess.run(cxx + ["--version"], check=True, capture_output=True, text=True).stdout.splitlines()[0]
    flags = ["-std=gnu++20", "-O2", "-DNDEBUG", *x86_only(["-march=x86-64"], cxx)]
    records = [{"type": "environment", "cpu": cpu, "compiler": compiler, "flags": flags, "platform": platform.platform(), "seed": 20261017}]
    print(json.dumps(records[0]), flush=True)
    with tempfile.TemporaryDirectory(prefix="p019-rational-bench-") as directory:
        binary = Path(directory) / "bench"
        subprocess.run(cxx + flags + [str(source), "-o", str(binary)], check=True, timeout=900)
        run = subprocess.run([str(binary), args.mode], check=True, capture_output=True, text=True, timeout=1800)
        for line in run.stdout.splitlines():
            records.append({"type": "measurement", **json.loads(line)})
            print(json.dumps(records[-1]), flush=True)
    if args.output:
        args.output.write_text("".join(json.dumps(r) + "\n" for r in records))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
