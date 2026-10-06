#!/usr/bin/env python3
"""C03 canonical modular backend benchmarks; checked medians, no timing gates."""
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
    parser.add_argument("--suite", choices=("all", "candidates", "actual", "construction", "inverses"), default="all")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64 or args.repetitions < 1 or args.milliseconds <= 0:
        parser.error("uint64 seed and positive repetitions/milliseconds required")
    cxx = shlex.split(os.getenv("CXX", "g++"))
    source = Path(__file__).resolve().with_suffix(".cpp")
    info = Path("/proc/cpuinfo").read_text()
    cpu = next(line.split(":", 1)[1].strip() for line in info.splitlines() if line.startswith("model name"))
    flags = next(line.split(":", 1)[1].split() for line in info.splitlines() if line.startswith("flags"))
    records = []

    def emit(record):
        records.append(record)
        print(json.dumps(record), flush=True)

    emit({"type": "environment", "cpu": cpu, "platform": platform.platform(), "python": platform.python_version(),
          "compiler": subprocess.run(cxx + ["--version"], capture_output=True, text=True, check=True).stdout.splitlines()[0],
          "seed": args.seed, "repetitions": args.repetitions, "target_ms": args.milliseconds, "suite": args.suite,
          "warmup": "double iterations until target duration; then median of repetitions",
          "reference": "unsigned double-width multiplication/remainder; fixed=true exposes compile-time modulus, fixed=false hides it from propagation",
          "setup": "contexts excluded except montgomery-setup; fixed setup allows constant folding; dynamic setup hides modulus from propagation; ordinary Montgomery candidates include conversions; actual type setMod(prm=0) excluded, per-power local setup included; construction compares the type's constructor from int/lng with the former 128-bit signed remainder on the same inputs",
          "memory": "O(1) contexts; candidate bulk owns six n-word arrays, including two Montgomery scratch; actual bulk owns four n-word arrays; inverse workload three n-word arrays plus batch temporary; only batchInv's own allocation is timed",
          "barrier": "compiler memory clobber per iteration, escaped outputs and volatile checksum prevent elision",
          "scope": "backend candidates for canonical-residue modular types; isolated timings do not establish the outside-Core end-to-end rule"})
    root = source.parents[2]
    emit({"type": "sources", "sha256": {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
          for path in [source, *[root / "01-Core" / name for name in
                       ("03-barrett.hpp", "04-montgomery.hpp", "05-modint.hpp")]]}})
    with tempfile.TemporaryDirectory(prefix="p005-modint-bench-") as temp:
        for config, isa in {"scalar": ["-mno-avx2", "-mno-bmi2"], "avx2": ["-mavx2", "-mbmi2", "-mno-avx512f"]}.items():
            if args.config not in ("all", config):
                continue
            if config == "avx2" and not all(flag in flags for flag in ("avx2", "bmi2")):
                emit({"type": "skip", "config": config, "reason": "AVX2/BMI2 unavailable"})
                continue
            binary = Path(temp) / config
            options = ["-std=gnu++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra", *isa]
            command = cxx + options + [str(source), "-o", str(binary)]
            subprocess.run(command, check=True, timeout=180)
            run = [str(binary), str(args.seed), str(args.repetitions), str(args.milliseconds), args.suite]
            result = subprocess.run(run, capture_output=True, text=True, check=True, timeout=240)
            emit({"type": "configuration", "config": config, "flags": options,
                  "compile_command": shlex.join(command), "run_command": shlex.join(run), "verification": result.stderr.strip()})
            for line in result.stdout.splitlines():
                emit({"type": "measurement", "config": config, **json.loads(line)})
    if args.output:
        args.output.write_text("".join(json.dumps(record) + "\n" for record in records))


if __name__ == "__main__":
    main()
