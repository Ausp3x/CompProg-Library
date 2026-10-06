#!/usr/bin/env python3
"""Barrett32/64 verification, independent native-remainder/quotient, bitwise
restoring-division and bitwise 256-bit product oracles.

quick: boundaries, fields, divMod/div, 2,000 random words/backend, small exhaustive
domains and all batch sizes/alias shapes with fixed factors MAX, m-1 and random;
full: 60,000 random words/backend, exhaustive mod<=128 and x<16384, 8 offsets,
sanitizers; stress: 600,000 words/backend, mod<=256 and x<65536. Every mode runs
scalar optimized/checked and, where the CPU has it, AVX2 optimized/checked, with
precondition deaths in both checked builds; full/stress add scalar and AVX2
ASan/UBSan and the standalone/multiple-TU build. All builds use
-Wall -Wextra -Wshadow -Wconversion -Werror. Requires GNU C++20 GCC 14+.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import resource
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time

SOURCE = Path(__file__).resolve().with_suffix(".cpp")
ROOT = SOURCE.parents[2]
DEATH_CASES = ("mod32", "mod64", "n32", "n64", "null32-a", "null32-b", "null32-out",
               "null64-a", "null64-b", "null64-out", "fixed32-n", "fixed64-n",
               "fixed32-null-a", "fixed32-null-out", "fixed64-null-a", "fixed64-null-out")


def report(kind: str, text: str) -> None:
    color = {"PASS": "\033[32m", "FAIL": "\033[31m"}.get(kind, "")
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        color = ""
    if sys.stdout.isatty():
        print("\r\033[K", end="")
    end = "\033[0m" if color else ""
    print(f"{color}{kind}{end} {text}", flush=True)


def execute(command: list[str], label: str, env: dict[str, str], death: bool = False) -> str:
    if sys.stdout.isatty():
        print(f"\r\033[KRUN {label}", end="", flush=True)
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, capture_output=True,
                                text=True, timeout=300)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f"{label} timeout=300s\ncommand={shlex.join(command)}\n"
                           f"stdout={error.stdout!r}\nstderr={error.stderr!r}") from error
    okay = (result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()) if death else result.returncode == 0
    if not okay:
        raise RuntimeError(f"{label} exit={result.returncode} expected={'SIGABRT/assertion' if death else '0'}\n"
                           f"command={shlex.join(command)}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result.stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.getenv("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.getenv("CP_TEST_SEED", "20260927")))
    parser.add_argument("--scalar-only", action="store_true")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("seed must be in [0, 2^64)")
    compiler = shlex.split(os.getenv("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", "CXX compiler unavailable")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2"]
    vector = ["-march=x86-64", "-mavx2"]
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    has_avx2 = cpu.exists() and "avx2" in cpu.read_text().split()
    accelerated = has_avx2 and not args.scalar_only
    if accelerated:
        variants += [("optimized-avx2", ["-O3", "-DNDEBUG", *vector]),
                     ("checked-avx2", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *vector])]
    else:
        report("SKIP", "AVX2 requested off" if args.scalar_only else "AVX2 unavailable on this CPU")
    if args.mode != "quick":
        sanitized = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
        variants.append(("sanitized-scalar", [*sanitized, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitized, *vector]))
    start = time.monotonic()
    report("RUN", f"Barrett mode={args.mode} seed={args.seed}; native % and bitwise 256-bit product oracles")
    try:
        version = execute([*compiler, "-dumpversion"], "compiler version", env).strip()
        if int(version.split(".")[0]) < 14:
            raise RuntimeError(f"GNU compiler GCC 14+ required; CXX reports {version}")
        with tempfile.TemporaryDirectory(prefix="cp-barrett-tests-") as directory:
            temp = Path(directory)
            for name, options in variants:
                binary = str(temp / name)
                label = f"{name} mode={args.mode} seed={args.seed}"
                execute([*compiler, *flags, *options, str(SOURCE), "-o", binary], f"compile {label}", env)
                report("PASS", f"{name} compilation")
                output = execute([binary, "--mode", args.mode, "--seed", str(args.seed)], label, env)
                for line in output.splitlines():
                    print(f"  [{name}] {line}", flush=True)
                report("PASS", label)
                if name.startswith("checked"):
                    for case in DEATH_CASES:
                        execute([binary, "--death", case], f"{name} precondition {case}", env, True)
                    report("PASS", f"{name}: {len(DEATH_CASES)} checked preconditions")
            if args.mode != "quick":
                include = '#include "01-Core/03-barrett.hpp"\n'
                a, b = temp / "a.cpp", temp / "b.cpp"
                a.write_text(include + "ulng other(); int main(){return Barrett(17).mul(123,456)!=other();}\n")
                b.write_text(include + "ulng other(){return Barrett64(17).mul(123,456);}\n")
                binary = str(temp / "multiple-tu")
                execute([*compiler, *flags, "-O2", "-I", str(ROOT), str(a), str(b), "-o", binary],
                        "standalone header and multiple-TU compile", env)
                execute([binary], "multiple-TU execute", env)
                report("PASS", "standalone/multiple-TU compile and execute")
    except (OSError, RuntimeError, ValueError) as error:
        report("FAIL", f"Barrett mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"Barrett all {args.mode} configurations ({len(variants)}) in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
