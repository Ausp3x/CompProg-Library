#!/usr/bin/env python3
"""Packed Bitset oracle: quick smoke, full feature coverage, extended seeded stress.

All modes execute optimized scalar, checked scalar and available AVX2/POPCNT
builds. Full and stress also execute ASan/UBSan scalar and AVX2 builds. The
C++ oracle models each bit independently and retains checks under -DNDEBUG.
Every build uses -Werror, so a new warning in the header or tester is a failure.
--counts-only restricts any mode to the named count and value-operator regressions.
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
DEATH_CASES = ("test", "const-index", "proxy-index", "set", "reset", "flip",
               "range-order", "range-end", "range-flip", "count-range", "mask-range",
               "next", "prev", "pop-empty", "string", "words-short", "words-long",
               "and-size", "or-size", "xor-size", "difference-size", "intersects-size",
               "subset-size", "count-and-size", "count-or-size", "count-xor-size",
               "assign-size", "shift-size", "combine-range-order", "combine-range-end",
               "combine-range-source", "combine-range-offset", "slice-order", "slice-end")


class Failure(RuntimeError):
    pass


def report(kind: str, message: str) -> None:
    color = {"PASS": "\033[32m", "FAIL": "\033[31m"}.get(kind, "")
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        color = ""
    end = "\033[0m" if color else ""
    if sys.stdout.isatty():
        print("\r\033[K", end="")
    print(f"{color}{kind}{end} {message}", flush=True)


def progress(message: str) -> None:
    if sys.stdout.isatty():
        print(f"\r\033[KRUN {message}", end="", flush=True)


def execute(command: list[str], label: str, timeout: int,
            env: dict[str, str], expected_death: bool = False) -> str:
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, text=True,
                                capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout={timeout}s\ncommand={shlex.join(command)}\n"
                      f"stdout={error.stdout!r}\nstderr={error.stderr!r}") from error
    if expected_death:
        okay = result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()
    else:
        okay = result.returncode == 0
    if not okay:
        raise Failure(f"{label}: exit={result.returncode} expected="
                      f"{'assertion/SIGABRT' if expected_death else '0'}\n"
                      f"command={shlex.join(command)}\n"
                      f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result.stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"),
                        default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int,
                        default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--scalar-only", action="store_true",
                        help="explicitly skip optional AVX2/POPCNT configurations")
    parser.add_argument("--counts-only", action="store_true",
                        help="run only count matrix and value-operator regressions")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("--seed must be in [0, 2**64)")
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"C++ compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2", "-mno-popcnt"]
    vector = ["-march=x86-64", "-mavx2", "-mpopcnt"]
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    cpu_flags = set(cpu.read_text().split()) if cpu.exists() else set()
    accelerated = not args.scalar_only and {"avx2", "popcnt"} <= cpu_flags
    if accelerated:
        variants.append(("optimized-avx2-popcnt", ["-O3", "-DNDEBUG", *vector]))
    else:
        reason = "requested --scalar-only" if args.scalar_only else "CPU lacks AVX2/POPCNT"
        report("SKIP", f"AVX2/POPCNT execution: {reason}")
    if args.mode != "quick":
        sanitizer = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2-popcnt", [*sanitizer, *vector]))
    scope = "counts-and-value-operators-only" if args.counts_only else "all-features"
    report("RUN", f"Bitset mode={args.mode} seed={args.seed} scope={scope}; independent byte-per-bit oracle")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-bitset-tests-") as directory:
            for name, options in variants:
                executable = str(Path(directory) / name)
                label = f"{name} mode={args.mode} seed={args.seed} scope={scope}"
                progress(f"compile {label}")
                execute([*compiler, *flags, *options, str(SOURCE), "-o", executable],
                        f"compile {label}", 180, env)
                report("PASS", f"{name} compilation")
                command = [executable, "--mode", args.mode, "--seed", str(args.seed)]
                if args.counts_only:
                    command.append("--counts-only")
                progress(label)
                output = execute(command,
                                 label, 1200 if args.mode == "stress" else 600, env)
                if sys.stdout.isatty():
                    print("\r\033[K", end="")
                for line in output.splitlines():
                    print(f"  [{name}] {line}", flush=True)
                report("PASS", label)
                if name == "checked-scalar" and not args.counts_only:
                    for case in DEATH_CASES:
                        progress(f"precondition {case}")
                        execute([executable, "--death", case],
                                f"precondition {case} seed={args.seed}", 10, env, True)
                    report("PASS", f"checked-scalar preconditions ({len(DEATH_CASES)} cases)")
    except (Failure, OSError) as error:
        report("FAIL", f"Bitset mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"Bitset scope={scope} {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
