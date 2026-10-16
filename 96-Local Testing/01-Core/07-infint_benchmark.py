#!/usr/bin/env python3
"""Benchmark driver for 07-infint.hpp: builds scalar and AVX2 variants of the C++ timing driver, verifies every workload inside it,
appends JSONL records with medians and prints crossover tables (fastest backend per size) used to set the dispatch thresholds."""
from __future__ import annotations

import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

SOURCE = Path(__file__).resolve().with_suffix(".cpp")
ROOT = SOURCE.parents[2]
HEADER = ROOT / "01-Core/07-infint.hpp"


def cpu_model() -> str:
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return platform.processor() or "unknown"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--milliseconds", type=float, default=2.0)
    parser.add_argument("--only", default="")
    parser.add_argument("--output", default=str(SOURCE.with_suffix(".jsonl")))
    parser.add_argument("--scalar-only", action="store_true")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        print(f"FAIL: compiler unavailable {compiler!r}", file=sys.stderr)
        return 1
    version = subprocess.run([*compiler, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    variants = [("scalar", ["-march=x86-64", "-mno-avx2", "-mno-fma", "-mno-bmi2"])]
    cpu_flags = set(Path("/proc/cpuinfo").read_text().split()) if Path("/proc/cpuinfo").exists() else set()
    if not args.scalar_only and {"avx2", "fma", "bmi2"} <= cpu_flags:
        variants.append(("avx2", ["-march=x86-64", "-mavx2", "-mfma", "-mbmi2"]))
    else:
        print("SKIP avx2 variant: requested --scalar-only or CPU lacks AVX2/FMA/BMI2")
    header_hash = hashlib.sha256(HEADER.read_bytes()).hexdigest()[:16]
    records = []
    with tempfile.TemporaryDirectory(prefix="cp-infint-bench-") as directory:
        for name, target in variants:
            exe = str(Path(directory) / name)
            flags = ["-std=gnu++20", "-O2", "-DNDEBUG", *target]
            subprocess.run([*compiler, *flags, str(SOURCE), "-o", exe], check=True)
            command = [exe, "--seed", str(args.seed), "--repetitions", str(args.repetitions), "--milliseconds", str(args.milliseconds)]
            if args.only:
                command += ["--only", args.only]
            start = time.monotonic()
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                print(f"FAIL: {name}: {result.stderr}", file=sys.stderr)
                return 1
            for line in result.stdout.splitlines():
                rec = json.loads(line)
                rec.update(variant=name, flags=" ".join(flags), compiler=version, cpu=cpu_model(), header=header_hash, date=time.strftime("%Y-%m-%d"))
                records.append(rec)
            print(f"{name}: {sum(1 for r in records if r['variant'] == name)} measurements in {time.monotonic() - start:.1f}s")
    with open(args.output, "a", encoding="utf-8") as out:
        for rec in records:
            out.write(json.dumps(rec) + "\n")
    table = collections.defaultdict(dict)
    for rec in records:
        table[(rec["variant"], rec["family"], rec["shape"], rec["n"], rec["m"])][rec["backend"]] = rec["ns"]
    for key in sorted(table):
        backends = table[key]
        best = min(backends, key=backends.get)
        cells = "  ".join(f"{b}={ns / 1000:.1f}us" for b, ns in sorted(backends.items(), key=lambda kv: kv[1]))
        print(f"{key[0]:6} {key[1]:10} {key[2]:10} n={key[3]:<6} m={key[4]:<6} best={best:16} {cells}")
    print(f"PASS: {len(records)} verified measurements appended to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
