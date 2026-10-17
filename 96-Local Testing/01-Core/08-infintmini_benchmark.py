#!/usr/bin/env python3
"""InfIntMini product-backend benchmark; JSONL output, no timing gates.

Times the schoolbook loop, the 2^64 - 2^32 + 1 NTT product and the dispatcher on random
base-1e5 limbs for balanced and unbalanced shapes; the driver checks that both backends
agree before timing. Every record is the median of 21 runs (5 above 10^6 limb products).
The cost_ratio column is n * m / (L * bit_width(L)) for the transform length L; the
dispatcher uses the schoolbook while it is at most InfIntMini::MUL_NTT. --mode quick shrinks the
unbalanced shapes for smoke runs.
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
    flags = ["-std=gnu++20", "-O2", "-DNDEBUG", "-march=x86-64"]
    records = [{"type": "environment", "cpu": cpu, "compiler": compiler, "flags": flags, "platform": platform.platform(),
                "seed": 20261017, "inputs": "uniform random limbs in [0, 1e5), top limb nonzero"}]
    print(json.dumps(records[0]), flush=True)
    with tempfile.TemporaryDirectory(prefix="p019-infintmini-bench-") as directory:
        binary = Path(directory) / "bench"
        subprocess.run(cxx + flags + [str(source), "-o", str(binary)], check=True, timeout=600)
        run = subprocess.run([str(binary), args.mode], check=True, capture_output=True, text=True, timeout=1800)
        for line in run.stdout.splitlines():
            records.append({"type": "measurement", **json.loads(line)})
            print(json.dumps(records[-1]), flush=True)
    if args.output:
        args.output.write_text("".join(json.dumps(r) + "\n" for r in records))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
