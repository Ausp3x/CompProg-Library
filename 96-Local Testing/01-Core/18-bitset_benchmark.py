#!/usr/bin/env python3
"""Packed-bitset kernel and subset-sum benchmark; JSONL output, no timing gates.

Run from any directory. CXX chooses GCC; --config selects baseline, popcnt,
avx2, or all. Baseline generic loops may auto-vectorize with SSE2; the popcnt
configuration adds scalar POPCNT and the AVX2 configuration permits ordinary
compiler auto-vectorization in the reference loops. Neither uses AVX512.
Timings are medians after adaptive warmup, excluding input creation/checking
except the explicitly labelled end-to-end subset-sum workload. All measured
results are checked. stdout contains metadata and result records.
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
    parser.add_argument("--seed", type=int, default=20260927)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--milliseconds", type=float, default=3)
    parser.add_argument("--config", choices=("all", "baseline", "popcnt", "avx2"), default="all")
    parser.add_argument("--timeout", type=float, default=180)
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
        "baseline": ["-mno-avx", "-mno-avx2", "-mno-popcnt"],
        "popcnt": ["-mno-avx", "-mno-avx2", "-mpopcnt"],
        "avx2": ["-mavx2", "-mpopcnt", "-mno-avx512f"],
    }
    records = []
    def emit(record):
        records.append(record)
        print(json.dumps(record), flush=True)
    emit({"type": "environment", "cpu": cpu, "compiler": compiler, "platform": platform.platform(), "seed": args.seed,
          "repetitions": args.repetitions, "target_ms": args.milliseconds,
          "warmup": "doubling iterations until target duration; then median of repeated runs",
          "setup": "excluded except subset_sum_allocations_included (96 items, construction+allocations included)",
          "memory": "packed arrays ceil(bits/64)*8 bytes each; subset reference uses one extra shifted temporary; fused shift O(1) workspace",
          "reference": "ordinary C++ word loops (auto-vectorization allowed), scalar std::popcount; materialized-shift subset sum",
          "barrier": "compiler memory barrier per kernel iteration prevents constant-input hoisting; contributes small-input overhead"})
    with tempfile.TemporaryDirectory(prefix="p002-bitset-bench-") as directory:
        for config, isa in configs.items():
            if args.config not in ("all", config):
                continue
            missing = [flag for flag in ({"popcnt": ["popcnt"], "avx2": ["avx2", "popcnt"]}.get(config, [])) if flag not in cpu_flags]
            if missing:
                emit({"type": "skip", "config": config, "reason": "missing CPU flags " + ", ".join(missing)})
                continue
            binary = Path(directory) / config
            flags = ["-std=gnu++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra"] + isa
            command = cxx + flags + [str(source), "-o", str(binary)]
            subprocess.run(command, check=True, timeout=args.timeout)
            run = [str(binary), str(args.seed), str(args.repetitions), str(args.milliseconds)]
            completed = subprocess.run(run, check=True, capture_output=True, text=True, timeout=args.timeout)
            emit({"type": "configuration", "config": config, "flags": flags, "compile_command": shlex.join(command), "run_command": shlex.join(run), "verification": completed.stderr.strip()})
            for line in completed.stdout.splitlines():
                result = json.loads(line)
                emit({"type": "measurement", "config": config, **result})
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text("".join(json.dumps(record) + "\n" for record in records))


if __name__ == "__main__":
    main()
