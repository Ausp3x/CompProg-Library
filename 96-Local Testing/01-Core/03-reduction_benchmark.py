#!/usr/bin/env python3
"""C02 reduction benchmarks. Medians, checked outputs, no universal timing gates.

Configurations: scalar and avx2 use -O3 -DNDEBUG; scalar-asserts and avx2-asserts use
-O2 with assertions enabled, the Codeforces configuration.
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
    parser.add_argument("--milliseconds", type=float, default=1)
    parser.add_argument("--config", choices=("all", "scalar", "avx2", "scalar-asserts", "avx2-asserts"), default="all")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64 or args.repetitions < 1 or args.milliseconds <= 0:
        parser.error("uint64 seed and positive repetitions/milliseconds required")
    cxx = shlex.split(os.getenv("CXX", "g++"))
    source = Path(__file__).with_suffix(".cpp")
    info = Path("/proc/cpuinfo").read_text()
    cpu = next(line.split(":", 1)[1].strip() for line in info.splitlines() if line.startswith("model name"))
    flags = next(line.split(":", 1)[1].split() for line in info.splitlines() if line.startswith("flags"))
    records = []

    def emit(record):
        records.append(record)
        print(json.dumps(record), flush=True)

    emit({"type": "environment", "cpu": cpu, "platform": platform.platform(),
          "compiler": subprocess.run(cxx + ["--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0],
          "seed": args.seed, "repetitions": args.repetitions, "target_ms": args.milliseconds,
          "warmup": "double iterations until target duration, then median of repetitions",
          "reference": "runtime modulus, native unsigned double-width multiplication/remainder; ordinary compiler optimization allowed",
          "setup": "excluded for kernels/chains; setup-and-pow includes one context and conversions for each exponentiation",
          "memory": "contexts O(1); bulk arrays O(n) words, no kernel allocation or scratch; inputs precomputed",
          "barrier": "compiler memory barrier per iteration and volatile sink prevent dead-code elimination/hoisting"})
    with tempfile.TemporaryDirectory(prefix="p004-reduction-bench-") as temp:
        isa_flags = {"scalar": ["-mno-avx2", "-mno-bmi2"], "avx2": ["-mavx2", "-mbmi2", "-mno-avx512f"]}
        for config in ("scalar", "avx2", "scalar-asserts", "avx2-asserts"):
            if args.config not in ("all", config):
                continue
            isa, asserts = config.split("-")[0], config.endswith("-asserts")
            if isa == "avx2" and not all(flag in flags for flag in ("avx2", "bmi2")):
                emit({"type": "skip", "config": config, "reason": "AVX2/BMI2 unavailable"})
                continue
            binary = Path(temp) / config
            options = ["-std=gnu++20", *(["-O2"] if asserts else ["-O3", "-DNDEBUG"]), "-Wall", "-Wextra", *isa_flags[isa]]
            command = cxx + options + [str(source), "-o", str(binary)]
            subprocess.run(command, check=True, timeout=180)
            run = [str(binary), str(args.seed), str(args.repetitions), str(args.milliseconds)]
            result = subprocess.run(run, capture_output=True, text=True, check=True, timeout=180)
            emit({"type": "configuration", "config": config, "flags": options,
                  "compile_command": shlex.join(command), "run_command": shlex.join(run), "verification": result.stderr.strip()})
            for line in result.stdout.splitlines():
                emit({"type": "measurement", "config": config, **json.loads(line)})
    if args.output:
        args.output.write_text("".join(json.dumps(record) + "\n" for record in records))


if __name__ == "__main__":
    main()
