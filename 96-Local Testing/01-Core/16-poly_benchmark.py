#!/usr/bin/env python3
"""Poly/FPS benchmark driver; JSONL output, no timing gates.

Run from any directory. CXX chooses GCC; --config selects baseline (no AVX/AVX2/FMA/
BMI2/PCLMUL), avx2 (AVX2+FMA+BMI2+PCLMUL) or all. Every record is the median of
repeated runs after doubling warmup to the target duration; every result is folded
into a checksum that the C++ driver verifies against an independent computation.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=20261008)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--milliseconds", type=float, default=3)
    parser.add_argument("--config", choices=("all", "baseline", "avx2"), default="all")
    parser.add_argument("--timeout", type=float, default=1800)
    parser.add_argument("--output", type=Path, help="Optional durable JSONL report (also printed)")
    args = parser.parse_args()
    if args.repetitions < 1 or args.milliseconds <= 0 or args.seed < 0 or args.seed >= 2**64 or args.timeout <= 0:
        parser.error("positive repetitions, milliseconds, timeout and uint64 seed required")
    cxx = shlex.split(os.environ.get("CXX", "g++"))
    source = Path(__file__).with_suffix(".cpp")
    cpuinfo = Path("/proc/cpuinfo").read_text()
    cpu = next((line.split(":", 1)[1].strip() for line in cpuinfo.splitlines() if line.startswith("model name")), platform.processor())
    cpu_flags = next((line.split(":", 1)[1].split() for line in cpuinfo.splitlines() if line.startswith("flags")), [])
    compiler = subprocess.run(cxx + ["--version"], check=True, capture_output=True, text=True).stdout.splitlines()[0]
    configs = {
        "baseline": ["-march=x86-64", "-mno-avx", "-mno-avx2", "-mno-fma", "-mno-bmi2", "-mno-pclmul"],
        "avx2": ["-march=x86-64", "-mavx2", "-mfma", "-mbmi2", "-mpclmul", "-mno-avx512f"],
    }
    records = []
    def emit(record):
        records.append(record)
        print(json.dumps(record), flush=True)
    emit({"type": "environment", "cpu": cpu, "compiler": compiler, "platform": platform.platform(), "seed": args.seed,
          "repetitions": args.repetitions, "target_ms": args.milliseconds,
          "warmup": "doubling iterations until target duration; then median of repeated runs",
          "inputs": "uniform seeded random residues modulo 998244353 (or the named modulus); sizes in each record",
          "memory": "coefficient vectors of the stated sizes plus the NTT root cache of the largest transform",
          "reference": "the reference column names the compared implementation of the same result"})
    with tempfile.TemporaryDirectory(prefix="p025-poly-bench-") as directory:
        for config, isa in configs.items():
            if args.config not in ("all", config):
                continue
            missing = [flag for flag in ({"avx2": ["avx2", "fma", "bmi2", "pclmulqdq"]}.get(config, [])) if flag not in cpu_flags]
            if missing:
                emit({"type": "skip", "config": config, "reason": "missing CPU flags " + ", ".join(missing)})
                continue
            binary = Path(directory) / config
            flags = ["-std=gnu++20", "-O2", "-DNDEBUG", "-Wall", "-Wextra"] + isa
            command = cxx + flags + [str(source), "-o", str(binary)]
            subprocess.run(command, check=True, timeout=args.timeout)
            run = [str(binary), str(args.seed), str(args.repetitions), str(args.milliseconds)]
            completed = subprocess.run(run, check=True, capture_output=True, text=True, timeout=args.timeout)
            emit({"type": "configuration", "config": config, "flags": flags, "compile_command": shlex.join(command), "run_command": shlex.join(run), "verification": completed.stderr.strip()})
            for line in completed.stdout.splitlines():
                emit({"type": "measurement", "config": config, **json.loads(line)})
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text("".join(json.dumps(record) + "\n" for record in records))


if __name__ == "__main__":
    main()
