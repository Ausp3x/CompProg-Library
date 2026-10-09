#!/usr/bin/env python3
"""Independent exact verification for the four modular-integer types in 05-modint.hpp.

quick: all API fixtures, fixed boundary moduli, exhaustive small rings and 100
random pairs/modulus, 100 Python integer cases/modulus; optimized and checked
scalar/available AVX2. full: 3,000 C++ pairs/modulus, dynamic exhaustive mod<=60,
1,000 Python cases/modulus, more roots/primality, plus scalar/AVX2 ASan+UBSan.
stress: 30,000 pairs/modulus, dynamic exhaustive mod<=120 and 10,000 Python
cases/modulus, same configurations. Every mode checks declared preconditions.
C++ standalone/aggregate/multiple-TU linkage is covered by 02-integration.py.
Only Python's standard library and GCC 14+ are required. Python integers and
built-in modular pow are independent of the C++ reduction implementation.
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
import random
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


class Failure(RuntimeError):
    pass


def report(kind: str, message: str) -> None:
    color = {"PASS": "\033[32m", "FAIL": "\033[31m"}.get(kind, "")
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        color = ""
    if sys.stdout.isatty():
        print("\r\033[K", end="")
    reset = "\033[0m" if color else ""
    print(f"{color}{kind}{reset} {message}", flush=True)


def execute(command: list[str], label: str, env: dict[str, str], *, data: str | None = None,
            death: bool = False, compile_fail: bool = False) -> str:
    if sys.stdout.isatty():
        print(f"\r\033[KRUN {label}", end="", flush=True)
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, input=data, text=True,
                                capture_output=True, timeout=300)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout=300s command={shlex.join(command)}\n"
                      f"stdout={error.stdout!r}\nstderr={error.stderr!r}") from error
    okay = result.returncode == 0
    if death:
        okay = result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()
    if compile_fail:
        okay = result.returncode != 0 and "error:" in result.stderr.lower()
    if not okay:
        expected = "assertion/SIGABRT" if death else "compile failure" if compile_fail else "0"
        raise Failure(f"{label}: exit={result.returncode} expected={expected}\n"
                      f"command={shlex.join(command)}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    return result.stdout


def corpus(mode: str, seed: int, wide: bool) -> tuple[list[str], list[str]]:
    rng = random.Random(seed)
    mods = [1, 2, 4, 7, 17, 97, 998244353, 2147483647, 2147483648, 4294967291, 4294967295]
    if wide:
        mods += [4294967296, 2305843009213693951, 9223372036854775808, 18446744069414584321, 18446744073709551557, 18446744073709551615]
    primes = {2, 7, 17, 97, 998244353, 2147483647, 4294967291, 2305843009213693951, 18446744069414584321, 18446744073709551557}
    count = {"quick": 100, "full": 1000, "stress": 10000}[mode]
    requests, answers = [], []

    def add(request: str, *expected: int) -> None:
        requests.append(request)
        answers.append(" ".join(map(str, expected)))

    for m in mods:
        signed = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63), -(1 << 32), -(1 << 31) - 1, -(1 << 31), -(1 << 31) + 1, -m - 1, -m, -1,
                  0, 1, m - 1, m, m + 1, (1 << 31) - 1, (1 << 31), (1 << 31) + 1, (1 << 32) - 1, (1 << 32), (1 << 32) + 1, (1 << 63) - 1, (1 << 63), (1 << 127) - 1]
        unsigned = [0, 1, m - 1, m, m + 1, (1 << 32) - 1, (1 << 32), (1 << 32) + 1, (1 << 64) - 1, (1 << 64), (1 << 127), (1 << 128) - 1]
        for x in signed:
            add(f"N {m} s {x}", x % m, x % m)
        for x in unsigned:
            add(f"N {m} u {x}", x % m, x % m)
        for i in range(count):
            a, b = rng.randrange(m), rng.randrange(m)
            unit = math.gcd(b, m) == 1
            expected = [(a + b) % m, (a - b) % m, a * b % m, int(unit), pow(b, -1, m) if unit else m - 1]
            if unit:
                expected.append(a * pow(b, -1, m) % m)
            add(f"A {m} {a} {b}", *expected)
            x = rng.getrandbits(128)
            if i & 1:
                x -= 1 << 127
                add(f"N {m} s {x}", x % m, x % m)
            else:
                add(f"N {m} u {x}", x % m, x % m)
        exponents = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63), -65537, -65536, -65535, -1000, -513, -512, -511, -2, -1,
                     0, 1, 2, 63, 64, 65, 511, 512, 513, 65535, 65536, 65537, (1 << 64), (1 << 127) - 1]
        for i in range(12 if mode == "quick" else 40 if mode == "full" else 200):
            a = rng.randrange(m)
            for e in exponents:
                if e >= 0 or math.gcd(a, m) == 1:
                    add(f"P {m} {a} s {e}", pow(a, e, m))
            for e in [0, (1 << 64) - 1, (1 << 127), (1 << 127) + 1, (1 << 128) - 1]:
                add(f"P {m} {a} u {e}", pow(a, e, m))
        if m in primes:
            if m <= 97:
                # Exhaustively enumerate minimum roots, independently of Legendre/Tonelli.
                roots = {}
                for x in range(m):
                    roots.setdefault(x * x % m, x)
                for a in range(m):
                    root = roots.get(a, m - 1)
                    add(f"R {m} {a}", int(a in roots), root, root)
            else:
                for i in range(20 if mode == "quick" else 100 if mode == "full" else 1000):
                    x = rng.randrange(m)
                    root = min(x, m - x)
                    add(f"R {m} {x * x % m}", 1, root, root)
                for a in range(2, 50):
                    if pow(a, (m - 1) // 2, m) == m - 1:
                        add(f"R {m} {a}", 0, m - 1, m - 1)
        for token in ["0", "+0", "-0", "0000007", str(-(1 << 127)), str((1 << 128) - 1),
                      "+" + "9" * 2000, "-" + "1234567890" * 200]:
            add(f"S {m} {token}", 0, int(token) % m)
        for token in ["+", "-", "x", "1x", "++1", "--1", "+-1", "0xFF", "1.0"]:
            add(f"S {m} {token}", 1, 7 % m)
    return requests, answers


def main(entry: Path) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.getenv("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.getenv("CP_TEST_SEED", "20260927")))
    parser.add_argument("--scalar-only", action="store_true")
    parser.add_argument("--type", choices=("all", "modint32", "modint64", "dynmodint32", "dynmodint64"),
                        default="all", help="default: verify all four public types")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("seed must be in [0, 2^64)")
    for kind, wide, dynamic in [("modint32", False, False), ("modint64", True, False),
                                ("dynmodint32", False, True), ("dynmodint64", True, True)]:
        if args.type in ("all", kind):
            result = run_suite(entry, args, kind=kind, wide=wide, dynamic=dynamic)
            if result:
                return result
    report("PASS", f"05-modint.hpp requested types={args.type} mode={args.mode} seed={args.seed}")
    return 0


def run_suite(entry: Path, args: argparse.Namespace, *, kind: str, wide: bool, dynamic: bool) -> int:
    compiler = shlex.split(os.getenv("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow"]
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2"]
    vector = ["-march=x86-64", "-mavx2"]
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    accelerated = not args.scalar_only and cpu.exists() and "avx2" in cpu.read_text().split()
    if accelerated:
        variants += [("optimized-avx2", ["-O3", "-DNDEBUG", *vector]),
                     ("checked-avx2", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *vector])]
    else:
        report("SKIP", "AVX2: " + ("--scalar-only requested" if args.scalar_only else "CPU unavailable"))
    if args.mode != "quick":
        sanitizer = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector]))
    deaths = ["raw", "init", "inverse", "division", "negative-power", "root", "legacy-root", "narrow"]
    if dynamic:
        deaths += ["zero-modulus", "false-prime", "prime-flag", "unknown-root"]
    label = "05-" + kind
    fixture = entry.with_name(label + "_cases.cpp")
    started = time.monotonic()
    report("RUN", f"{label} mode={args.mode} seed={args.seed}; independent Python integer oracle")
    try:
        version = execute([*compiler, "-dumpversion"], "compiler version", env).strip()
        if int(version.split('.')[0]) < 14:
            raise Failure(f"GCC 14+ required, CXX version={version}")
        requests, expected = corpus(args.mode, args.seed, wide)
        data = "\n".join(requests) + "\n"
        report("PASS", f"generated {len(requests)} exact Python oracle cases")
        with tempfile.TemporaryDirectory(prefix=f"cp-{label}-tests-") as directory:
            temp = Path(directory)
            for name, options in variants:
                binary = str(temp / name)
                config = f"{label}/{name} mode={args.mode} seed={args.seed}"
                execute([*compiler, *flags, *options, str(fixture), "-o", binary], "compile " + config, env)
                report("PASS", f"{name} compilation")
                output = execute([binary, "--mode", args.mode, "--seed", str(args.seed)], config, env)
                for line in output.splitlines():
                    print(f"  [{name}] {line}", flush=True)
                report("PASS", f"{name} C++ exhaustive/boundary/random feature fixtures")
                actual = execute([binary, "--oracle"], config + " Python oracle", env, data=data).splitlines()
                if len(actual) != len(expected):
                    raise Failure(f"{config} oracle line count expected={len(expected)} actual={len(actual)}")
                for i, (got, want) in enumerate(zip(actual, expected)):
                    if got != want:
                        raise Failure(f"{config} first failing independent case index={i}\n"
                                      f"smallest known reproducer stdin={requests[i]!r}\nexpected={want}\nactual={got}\n"
                                      f"command={shlex.join([binary, '--oracle'])}")
                report("PASS", f"{name} Python big-integer oracle ({len(requests)} cases)")
                if name.startswith("checked"):
                    for case in deaths:
                        execute([binary, "--death", case], config + " precondition " + case, env, death=True)
                    report("PASS", f"{name} preconditions ({len(deaths)})")
            if not dynamic and not wide and args.mode != "quick":
                for name, target in [("scalar", scalar)] + ([("avx2", vector)] if accelerated else []):
                    binary = str(temp / ("consumers-" + name))
                    execute([*compiler, *flags, "-O2", *target,
                             str(entry.with_name("05-modint_consumers.cpp")), "-o", binary],
                            "Matrix/ModFac existing consumer smoke compile " + name, env)
                    execute([binary], "Matrix/ModFac existing consumer smoke execute " + name, env)
                report("PASS", "existing Matrix/ModFac consumer smoke scalar/available AVX2")
            if dynamic and args.mode != "quick":
                root = entry.parents[2]
                header = root / "01-Core" / "05-modint.hpp"
                kind = "DynModInt64" if wide else "DynModInt"
                other_kind = "DynModInt" if wide else "DynModInt64"
                includes = f'#include "{header}"\n'
                a, b = temp / "a.cpp", temp / "b.cpp"
                a.write_text(includes + f"using A={kind}<900>; using X={other_kind}<900>;\n"
                             "A global_product=A(1000000)*A(1000000);\nulng fromOther();\n"
                             "int main(){if(global_product.val()!=757402647)return 1;"
                             "A::setMod(17);X::setMod(12);if(A::mod()!=17||X::mod()!=12)return 2;"
                             "if((A(1000000)*A(1000000)).val()!=13)return 3;"
                             f"return fromOther()!=13||{kind}<901>::mod()!=17;}}\n")
                b.write_text(includes + f"using B={kind}<901>;\n"
                             "B global_custom=[] {B::setMod(17);return B(1000000)*B(1000000);}();\n"
                             "ulng fromOther(){return global_custom.val();}\n")
                for first, second in [(a, b), (b, a)]:
                    binary = str(temp / "cross-tu")
                    execute([*compiler, *flags, "-O2", str(first), str(second), "-o", binary],
                            "cross-TU globals and same-ID cross-width contexts compile", env)
                    execute([binary], "cross-TU globals and same-ID cross-width contexts execute", env)
                report("PASS", "cross-TU default/custom global initialization, both link orders, width independence")
            # Invalid static moduli must be diagnosed even when assertions are removed.
            if not dynamic:
                source = temp / "bad.cpp"
                header = entry.parents[2] / "01-Core" / "05-modint.hpp"
                kind = "ModInt64" if wide else "ModInt"
                source.write_text(f'#include "{header}"\n{kind}<0> bad;\n')
                execute([*compiler, "-std=gnu++20", "-DNDEBUG", "-fsyntax-only", str(source)],
                        "compile-time zero modulus rejection", env, compile_fail=True)
                report("PASS", "compile-time zero modulus rejection")
    except (OSError, Failure, ValueError) as error:
        report("FAIL", f"{label} mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"{label} {len(variants)} configurations in {time.monotonic() - started:.2f}s")
    return 0
