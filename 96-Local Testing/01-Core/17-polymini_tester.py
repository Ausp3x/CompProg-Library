#!/usr/bin/env python3
"""PolyMini oracle suite: quick smoke, full feature coverage, extended seeded stress.

Every mode compiles the header alone, inside 98-basic and 99-all, and as an
extracted copy of PolyMini plus ModIntMini with only <bits/stdc++.h>, the
namespace and the three width aliases; then builds and runs the optimized
(-DNDEBUG) and checked scalar configurations, plus AVX2/FMA/BMI2 when the CPU
has them (the mini has no ISA path, the build exercises the compared Poly
kernels), of the C++ oracle with a second translation unit. Full and stress add
ASan/UBSan scalar; checked scalar runs the --invalid precondition probes;
stress repeats every configuration --rounds times with consecutive seeds
(sanitized builds run the full-mode lists). Oracles are schoolbook products and
long division, naive DFT, series recurrences, power sums, Lagrange, Horner
shifts, exhaustive recurrence enumeration over GF(2), GF(3), GF(5) and
Kitamasa; Poly<T> agreement is supporting evidence only.
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
SECOND = SOURCE.with_name("17-polymini_tester_tu.cpp")
ROOT = SOURCE.parents[2]
HEADER = ROOT / "01-Core/17-polymini.hpp"
INVALID_CASES = ("divmod-zero", "inv-zero", "inv-empty", "inv-negative", "log-constant", "exp-constant",
                 "pow-negative-valuation", "pow-negative-n", "ntt-length", "ntt-limit", "convolution-limit",
                 "interpolate-size", "interpolate-duplicate", "kth-negative", "kth-short")
COPY_PROGRAM = r"""
int main() {
    using T = ModIntMini<998244353>; using P = PolyMini<T>;
    P a = {1, 2, 3}, b = {4, 5}, c = a * b;
    if (c != P{4, 13, 22, 15}) { return 1; }
    P g = inv(P{1, T(-1)}, 6);
    for (T x : g) { if (x != T(1)) { return 2; } }
    P e = exp(P{0, 1, 3, 7}, 50), l = log(e, 50);
    if (P(l.begin(), l.begin() + 4) != P{0, 1, 3, 7} || std::any_of(l.begin() + 4, l.end(), [](T x) { return x != T(0); })) { return 3; }
    vector<T> fib = {0, 1, 1, 2, 3, 5, 8, 13}, rec = P::findLinearRecurrence(fib);
    if (rec != vector<T>{1, 1} || P::linearRecurrenceKth(fib, rec, 90) != T(2880067194370816120ULL)) { return 4; }
    vector<T> xs = {1, 2, 3, 4}, ys = evalMulti(P{5, 0, 1}, xs);
    if (ys != vector<T>{6, 9, 14, 21} || P::interpolate(xs, ys) != P{5, 0, 1, 0}) { return 5; }
    auto [q, r] = divMod(P{T(-1), 0, 1}, P{T(-1), 1});
    if (q != P{1, 1} || !r.empty() || taylorShift(P{0, 0, 1}, T(1)) != P{1, 2, 1} || pow(P{1, 1}, 3, 5) != P{1, 3, 3, 1, 0}) { return 6; }
    vector<T> v = {1, 2, 3, 4};
    P::ntt(v); P::ntt(v, true);
    return v == vector<T>{1, 2, 3, 4} && eval(P{1, 1}, T(2)) == T(3) && deriv(P{1, 1}) == P{1} && integ(P{2}) == P{0, 2} ? 0 : 7;}
"""


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


def extract_struct(text: str, name: str) -> str:
    start = text.find(f"struct {name} ")
    head = text.rfind("template<", 0, start)
    if start < 0 or head < 0:
        raise Failure(f"standalone copy: struct {name} not found")
    end = text.find("\n};\n", start)
    if end < 0:
        raise Failure(f"standalone copy: struct {name} is unterminated")
    return text[head:end + 4]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"),
                        default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int,
                        default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=1,
                        help="stress mode: rounds per configuration, seeds seed, seed + 1, ...")
    parser.add_argument("--scalar-only", action="store_true",
                        help="explicitly skip the optional ISA configuration")
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
        variants.append(("sanitized-scalar", ["-O2", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer",
                                              "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                                              "-no-pie", *scalar]))
    report("RUN", f"PolyMini mode={args.mode} seed={args.seed}; independent schoolbook/DFT/recurrence/Lagrange/brute-force oracles")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-polymini-tests-") as directory:
            temp = Path(directory)
            alone = temp / "alone.cpp"
            alone.write_text(f'#include "{HEADER}"\n#include "{ROOT / "01-Core/06-modintmini.hpp"}"\n'
                             'int main() { PolyMini<ModIntMini<998244353>> a = {1, 2}; return (a * a).size() == 3 ? 0 : 1; }\n')
            execute([*compiler, *flags, "-fsyntax-only", str(alone)], "header-alone compile", 180, env)
            for aggregate in ("98-basic.hpp", "99-all.hpp"):
                path = temp / f"{aggregate}.cpp"
                path.write_text(f'#include "{ROOT / "01-Core" / aggregate}"\n#include "{HEADER}"\n'
                                'int main() { PolyMini<mint> a = {1, 2}; return int((a * a).size()) - 3; }\n')
                execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(path)],
                        f"{aggregate} aggregate compile (unverified siblings are not warning-clean)", 180, env)
            text = HEADER.read_text() + "\n" + (ROOT / "01-Core/06-modintmini.hpp").read_text()
            copy = temp / "copy.cpp"
            copy.write_text("#include <bits/stdc++.h>\n#include <cassert>\nusing namespace std;\nusing uint = uint32_t;\nusing lng = int64_t;\n"
                            "using ulng = uint64_t;\nusing lll = __int128_t;\nusing ulll = __uint128_t;\n"
                            + extract_struct(text, "ModIntMini") + "\n" + extract_struct(text, "PolyMini") + COPY_PROGRAM)
            execute([*compiler, *flags, "-O2", str(copy), "-o", str(temp / "copy")], "extracted-copy compile", 180, env)
            execute([str(temp / "copy")], "extracted-copy execute", 60, env)
            report("PASS", "header-alone, 98-basic, 99-all and extracted-copy builds")
            for name, options in variants:
                executable = str(temp / name)
                label = f"{name} mode={args.mode} seed={args.seed}"
                progress(f"compile {label}")
                execute([*compiler, *flags, *options, str(SOURCE), str(SECOND), "-o", executable],
                        f"compile {label}", 600, env)
                report("PASS", f"{name} compilation (two translation units)")
                run_mode = "full" if args.mode == "stress" and name.startswith("sanitized") else args.mode
                for round_index in range(rounds):
                    seed = (args.seed + round_index) % 2**64
                    label = f"{name} mode={run_mode} seed={seed}"
                    progress(label)
                    output = execute([executable, "--mode", run_mode, "--seed", str(seed)], label, 3600, env)
                    if sys.stdout.isatty():
                        print("\r\033[K", end="")
                    for line in output.splitlines():
                        print(f"  [{name}] {line}", flush=True)
                    report("PASS", label)
                if name == "checked-scalar":
                    for case in INVALID_CASES:
                        progress(f"precondition {case}")
                        execute([executable, "--invalid", case], f"precondition {case} seed={args.seed}", 60, env, True)
                    report("PASS", f"checked-scalar preconditions ({len(INVALID_CASES)} cases)")
    except (Failure, OSError) as error:
        report("FAIL", f"PolyMini mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"PolyMini {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
