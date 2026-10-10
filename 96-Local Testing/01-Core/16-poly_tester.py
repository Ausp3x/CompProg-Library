#!/usr/bin/env python3
"""Poly/FPS oracle suite: quick smoke, full feature coverage, extended seeded stress.

Every mode builds and runs the optimized scalar, checked scalar and (when the CPU
has them) AVX2/FMA/BMI2/PCLMUL configurations of the C++ oracle together with a
second translation unit; full and stress add ASan/UBSan scalar and AVX2 builds,
and checked scalar runs the --invalid precondition probes; stress repeats
every configuration --rounds times with consecutive seeds, where the sanitized
builds execute the full-mode size lists (their stress lists exceed two hours under
ASan) and the optimized and checked builds the extended stress lists. The oracles are schoolbook
products, Horner, Euclid, Sylvester determinants, brute-force enumeration over
small prime fields and algebraic identities; every check survives -DNDEBUG.
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
SECOND = SOURCE.with_name("16-poly_tester_tu.cpp")
ROOT = SOURCE.parents[2]
INVALID_CASES = ("index", "resize", "truncate", "normalize", "shift-left", "shift-right",
               "divmod-zero", "monicdiv", "pseudodiv", "inv-zero", "inv-negative", "log-constant",
               "exp-constant", "pow-negative-valuation", "powrational", "sparsemul", "cyclic-size",
               "negacyclic-size", "middleproduct", "ntt-length", "ntt-doubling", "multivariate-shape",
               "multivariate-cyclic", "gf2k", "interpolate-duplicate", "interpolate-size",
               "geometric-duplicate", "newton-nodes", "hermite-empty", "halfgcd-degree",
               "invmod", "modulus-constant", "powmod-noninvertible", "discriminant-constant",
               "cyclotomic", "lagrange", "inv2d", "bostan-mori", "slice-order", "recurrence-short",
               "pade", "rational-points", "partial-multiplicity", "transposed-size", "shift-sampling",
               "powersmod-negative", "arbitrary-mod-zero")


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
    parser.add_argument("--rounds", type=int, default=1,
                        help="stress mode: rounds per configuration, seeds seed, seed + 1, ...")
    parser.add_argument("--scalar-only", action="store_true",
                        help="explicitly skip the optional ISA configurations")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("--seed must be in [0, 2**64)")
    if args.rounds < 1:
        parser.error("--rounds must be positive")
    rounds = args.rounds if args.mode == "stress" else 1
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"C++ compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2", "-mno-fma", "-mno-bmi2", "-mno-pclmul"]
    vector = ["-march=x86-64", "-mavx2", "-mfma", "-mbmi2", "-mpclmul"]
    variants = [("optimized-scalar", ["-O2", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    cpu_flags = set(cpu.read_text().split()) if cpu.exists() else set()
    accelerated = not args.scalar_only and {"avx2", "fma", "bmi2", "pclmulqdq"} <= cpu_flags
    if accelerated:
        variants.append(("optimized-avx2", ["-O2", "-DNDEBUG", *vector]))
    else:
        reason = "requested --scalar-only" if args.scalar_only else "CPU lacks AVX2/FMA/BMI2/PCLMUL"
        report("SKIP", f"AVX2/FMA/BMI2/PCLMUL execution: {reason}")
    if args.mode != "quick":
        sanitizer = ["-O2", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector]))
    report("RUN", f"Poly mode={args.mode} seed={args.seed}; independent schoolbook/Horner/Euclid/brute-force oracles")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-poly-tests-") as directory:
            alone = Path(directory) / "alone.cpp"
            alone.write_text(f'#include "{ROOT / "01-Core/16-poly.hpp"}"\nint main() {{ Poly<mint> a = {{1, 2}}; return (a * a).size() == 3 ? 0 : 1; }}\n')
            execute([*compiler, *flags, "-fsyntax-only", str(alone)], "header-alone compile", 180, env)
            aggregate = Path(directory) / "aggregate.cpp"
            aggregate.write_text(f'#include "{ROOT / "01-Core/99-all.hpp"}"\n#include "{ROOT / "01-Core/16-poly.hpp"}"\nint main() {{ return 0; }}\n')
            execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(aggregate)], "99-all aggregate compile (unverified siblings are not warning-clean)", 180, env)
            report("PASS", "header-alone and 99-all aggregate compilation")
            for name, options in variants:
                executable = str(Path(directory) / name)
                label = f"{name} mode={args.mode} seed={args.seed}"
                progress(f"compile {label}")
                execute([*compiler, *flags, *options, str(SOURCE), str(SECOND), "-o", executable],
                        f"compile {label}", 400, env)
                report("PASS", f"{name} compilation (two translation units)")
                run_mode = "full" if args.mode == "stress" and name.startswith("sanitized") else args.mode
                for round_index in range(rounds):
                    seed = (args.seed + round_index) % 2**64
                    label = f"{name} mode={run_mode} seed={seed}"
                    command = [executable, "--mode", run_mode, "--seed", str(seed)]
                    progress(label)
                    limit = 3600 if run_mode == "stress" or name.startswith("sanitized") else 1800
                    output = execute(command, label, limit, env)
                    if sys.stdout.isatty():
                        print("\r\033[K", end="")
                    for line in output.splitlines():
                        print(f"  [{name}] {line}", flush=True)
                    report("PASS", label)
                if name == "checked-scalar":
                    for case in INVALID_CASES:
                        progress(f"precondition {case}")
                        execute([executable, "--invalid", case],
                                f"precondition {case} seed={args.seed}", 60, env, True)
                    report("PASS", f"checked-scalar preconditions ({len(INVALID_CASES)} cases)")
    except (Failure, OSError) as error:
        report("FAIL", f"Poly mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"Poly {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
