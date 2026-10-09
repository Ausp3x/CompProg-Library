#!/usr/bin/env python3
"""Montgomery32/64 verification, independent exact modular-arithmetic oracle.

quick: all APIs (REDC core, conversions, canonical/lazy products, sums and
differences, powers, bulk kernels, the below precondition pass), boundary moduli,
bulk sizes/offsets/aliases, tiny exhaustive and 500 random cases per width;
optimized, checked and available AVX2 builds. full: exhaustive odd moduli <=65,
10000 random cases/width, ASan/UBSan and the standalone/multiple-TU build.
stress: exhaustive odd moduli <=129 and 100000 random cases/width, same builds.
Every mode checks all declared runtime preconditions in checked scalar/AVX2.
All builds use -Wall -Wextra -Wshadow -Wconversion -Werror.
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
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()


SOURCE = Path(__file__).resolve().with_suffix(".cpp")
ROOT = SOURCE.parents[2]
DEATH_CASES = ("zero", "even", "red", "mul-a", "mul-b", "add-a", "add-b", "sub-a", "sub-b",
               "get", "powMont", "lazy-mod", "lazy-red", "normalize-mod", "normalize",
               "lazy-mul-mod", "lazy-mul-a", "lazy-mul-b", "lazy-add-mod", "lazy-add-a",
               "lazy-add-b", "lazy-sub-mod", "lazy-sub-a", "lazy-sub-b", "bulk-mul-n",
               "bulk-mul-null-a", "bulk-mul-null-b", "bulk-mul-null-c", "bulk-mul-a",
               "bulk-mul-b", "bulk-mul-vector-a", "bulk-mul-tail-a", "bulk-mul-tail-b",
               "bulk-init-n", "bulk-init-null-a", "bulk-init-null-c", "bulk-get-n",
               "bulk-get-null-a", "bulk-get-null-c", "bulk-get-a", "bulk-get-tail",
               "bulk-lazy-n", "bulk-lazy-null-a", "bulk-lazy-null-b", "bulk-lazy-null-c",
               "bulk-lazy-mod", "bulk-lazy-a", "bulk-lazy-b", "bulk-lazy-tail-a",
               "bulk-lazy-tail-b")


class Failure(RuntimeError):
    pass


def report(kind: str, message: str) -> None:
    color = {"PASS": "\033[32m", "FAIL": "\033[31m"}.get(kind, "")
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        color = ""
    if sys.stdout.isatty():
        print("\r\033[K", end="")
    end = "\033[0m" if color else ""
    print(f"{color}{kind}{end} {message}", flush=True)


def progress(message: str) -> None:
    if sys.stdout.isatty():
        print(f"\r\033[KRUN {message}", end="", flush=True)


def execute(command: list[str], label: str, timeout: int,
            env: dict[str, str], death: bool = False) -> str:
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, text=True,
                                capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout={timeout}s command={shlex.join(command)}\n"
                      f"stdout={error.stdout!r}\nstderr={error.stderr!r}") from error
    okay = (result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()
            if death else result.returncode == 0)
    if not okay:
        raise Failure(f"{label}: exit={result.returncode} expected="
                      f"{'assertion/SIGABRT' if death else '0'} command={shlex.join(command)}\n"
                      f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result.stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"),
                        default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int,
                        default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--scalar-only", action="store_true")
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
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2"]
    vector = ["-march=x86-64", "-mavx2"]
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    cpu_flags = set(cpu.read_text().split()) if cpu.exists() else set()
    accelerated = not args.scalar_only and "avx2" in cpu_flags
    if accelerated:
        variants += [("optimized-avx2", ["-O3", "-DNDEBUG", *vector]),
                     ("checked-avx2", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *vector])]
    else:
        report("SKIP", "AVX2 execution: " + ("requested --scalar-only" if args.scalar_only
                                             else "CPU lacks AVX2"))
    if args.mode != "quick":
        sanitizer = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector]))
    report("RUN", f"Montgomery mode={args.mode} seed={args.seed}; independent exact oracle")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-montgomery-tests-") as directory:
            for name, options in variants:
                executable = str(Path(directory) / name)
                label = f"{name} mode={args.mode} seed={args.seed}"
                progress(f"compile {label}")
                execute([*compiler, *flags, *options, str(SOURCE), "-o", executable],
                        f"compile {label}", 180, env)
                report("PASS", f"{name} compilation")
                progress(label)
                output = execute([executable, "--mode", args.mode, "--seed", str(args.seed)],
                                 label, 300 if args.mode == "stress" else 180, env)
                if sys.stdout.isatty():
                    print("\r\033[K", end="")
                for line in output.splitlines():
                    print(f"  [{name}] {line}", flush=True)
                report("PASS", label)
                if name.startswith("checked"):
                    for width in (32, 64):
                        for case in DEATH_CASES:
                            progress(f"precondition {width}/{case}")
                            execute([executable, f"--death{width}", case],
                                    f"{label} precondition {width}/{case}", 10, env, True)
                    report("PASS", f"{name} preconditions ({2 * len(DEATH_CASES)} cases)")
            if args.mode != "quick":
                include = '#include "01-Core/04-montgomery.hpp"\n'
                a, b = Path(directory) / "a.cpp", Path(directory) / "b.cpp"
                a.write_text(include + "ulng other(); int main(){return Montgomery(17).pow(123,456)!=other();}\n")
                b.write_text(include + "ulng other(){return Montgomery64(17).pow(123,456);}\n")
                executable = str(Path(directory) / "multiple-tu")
                execute([*compiler, *flags, "-O2", "-I", str(ROOT), str(a), str(b), "-o", executable],
                        "standalone header and multiple-TU compile", 180, env)
                execute([executable], "multiple-TU execute", 60, env)
                report("PASS", "standalone/multiple-TU compile and execute")
    except (Failure, OSError) as error:
        report("FAIL", f"Montgomery mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"Montgomery {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
