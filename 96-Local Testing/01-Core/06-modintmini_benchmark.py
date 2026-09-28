#!/usr/bin/env python3
"""C16 checked mini/full/native benchmark medians; no portable timing gates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=20260927)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--milliseconds", type=float, default=1)
    parser.add_argument("--config", choices=("all", "scalar", "avx2"), default="all")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64 or args.repetitions < 1 or args.milliseconds <= 0:
        parser.error("uint64 seed and positive repetitions/milliseconds required")
    cxx = shlex.split(os.getenv("CXX", "g++"))
    source = Path(__file__).resolve().with_suffix(".cpp")
    root = source.parents[2]
    info = Path("/proc/cpuinfo").read_text()
    cpu = next(line.split(":", 1)[1].strip() for line in info.splitlines() if line.startswith("model name"))
    flags = next(line.split(":", 1)[1].split() for line in info.splitlines() if line.startswith("flags"))
    records = []

    def emit(record):
        records.append(record)
        print(json.dumps(record), flush=True)

    emit({"type": "environment", "cpu": cpu, "platform": platform.platform(), "python": platform.python_version(),
          "compiler": subprocess.run(cxx + ["--version"], check=True, text=True, capture_output=True).stdout.splitlines()[0],
          "seed": args.seed, "repetitions": args.repetitions, "target_ms": args.milliseconds,
          "warmup": "double iterations to target duration, median of repetitions",
          "setup": "input allocation and dynamic setMod excluded; each 64-bit power includes its Montgomery setup/conversions",
          "memory": "O(n) inputs/output/oracle arrays, O(1) modular contexts and power workspace",
          "reference": "exact double-width native remainder; fixed=true compiler sees constant modulus, fixed=false opaque runtime modulus",
          "barrier": "per-iteration compiler memory barrier, escaped bulk output, volatile chain/power checksum",
          "scope": "ordinary independent/dependent products and powers; shared machine, no universal fastest claim"})
    paths = [source, Path(__file__).resolve()] + [root / "01-Core" / name for name in
             ("01-template.hpp", "03-barrett.hpp", "04-montgomery.hpp", "05-modint.hpp", "06-modintmini.hpp")]
    emit({"type": "sources", "sha256": {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}})
    with tempfile.TemporaryDirectory(prefix="p005-mini-benchmark-") as temp:
        for config, isa in {"scalar": ["-mno-avx2", "-mno-bmi2"], "avx2": ["-mavx2", "-mbmi2", "-mno-avx512f"]}.items():
            if args.config not in ("all", config):
                continue
            if config == "avx2" and not all(flag in flags for flag in ("avx2", "bmi2")):
                emit({"type": "skip", "config": config, "reason": "AVX2/BMI2 unavailable"})
                continue
            binary = Path(temp) / config
            options = ["-std=gnu++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra", *isa]
            command = cxx + options + [str(source), "-o", str(binary)]
            subprocess.run(command, check=True, timeout=240)
            run = [str(binary), str(args.seed), str(args.repetitions), str(args.milliseconds)]
            result = subprocess.run(run, check=True, capture_output=True, text=True, timeout=240)
            emit({"type": "configuration", "config": config, "flags": options,
                  "compile_command": shlex.join(command), "run_command": shlex.join(run), "verification": result.stderr.strip()})
            for line in result.stdout.splitlines():
                emit({"type": "measurement", "config": config, **json.loads(line)})
    if args.output:
        args.output.write_text("".join(json.dumps(record) + "\n" for record in records))


if __name__ == "__main__":
    main()
