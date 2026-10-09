#!/usr/bin/env python3
"""Exact verification of all four independently copyable modular-integer minis.

quick: static rings 1..16, dynamic rings 1..24, full-width boundaries, 64 random
pairs/large modulus and 40 random runtime moduli/width, plus all public API,
precondition, standalone-copy and linkage fixtures; optimized NDEBUG and
checked scalar builds. full: dynamic rings 1..48, 512 random pairs/large modulus
and 300 random runtime moduli/width, plus ASan/UBSan. stress: dynamic rings 1..96,
4096 random pairs/large modulus and 3000 random runtime moduli/width, the same
build matrix. All modes compare against independent
Python integers and additionally check the matching full types. Minis have no
ISA-specific paths. Only Python's standard library and GCC 14+ are required.
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
import random
import re
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


def corpus(mode: str, seed: int) -> tuple[list[str], list[str]]:
    rng = random.Random(seed)
    boundary32 = [17, 97, 998244353, 2147483647, 2147483648, 4294967291, 4294967295]
    boundary64 = [4294967296, 2305843009213693951, 9223372036854775807,
                  9223372036854775808, 18446744069414584321,
                  18446744073709551557, 18446744073709551615]
    count = {"quick": 64, "full": 512, "stress": 4096}[mode]
    small = {"quick": 24, "full": 48, "stress": 96}[mode]
    requests, answers = [], []

    def add(kind: int, operation: str, m: int, args: str, *expected: int) -> None:
        requests.append(f"{kind} {operation} {m} {args}")
        answers.append(" ".join(map(str, expected)))

    def arithmetic(kind: int, m: int, a: int, b: int) -> None:
        unit = math.gcd(b, m) == 1
        inverse = pow(b, -1, m) if unit else m - 1
        add(kind, "A", m, f"{a} {b}", (a + b) % m, (a - b) % m, a * b % m,
            int(a == b), -a % m, int(unit), inverse, a * inverse % m if unit else m - 1)

    for kind in range(4):
        limit = small if kind >= 2 else 16
        mods = sorted(set(range(1, limit + 1)) | set(boundary32) |
                      (set(boundary64) if kind & 1 else set()))
        for m in mods:
            signed = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63),
                      -(1 << 31), -m - 1, -m, -1, 0, 1, m - 1, m, m + 1,
                      (1 << 31) - 1, (1 << 63) - 1, (1 << 127) - 1]
            unsigned = [0, 1, m - 1, m, m + 1, (1 << 32) - 1, (1 << 64) - 1,
                        1 << 127, (1 << 128) - 1]
            for x in signed:
                add(kind, "N", m, f"s {x}", x % m, x % m)
            for x in unsigned:
                add(kind, "N", m, f"u {x}", x % m, x % m)
            if m <= limit:
                for a in range(m):
                    for b in range(m):
                        arithmetic(kind, m, a, b)
            else:
                values = sorted({0, 1 % m, 2 % m, m // 2, m // 2 + 1, m - 2, m - 1})
                for a in values:
                    for b in values:
                        arithmetic(kind, m, a, b)
                for i in range(count):
                    a, b = rng.randrange(m), rng.randrange(m)
                    arithmetic(kind, m, a, b)
                    x = rng.getrandbits(128)
                    if i & 1:
                        x -= 1 << 127
                        add(kind, "N", m, f"s {x}", x % m, x % m)
                    else:
                        add(kind, "N", m, f"u {x}", x % m, x % m)
            # Include Montgomery/Mersenne threshold neighbors in the shared domain.
            signed_powers = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63),
                             -65537, -65536, -65535, -513, -512, -511, -2, -1,
                             0, 1, 2, 63, 64, 65, 511, 512, 513, 65535, 65536,
                             65537, 1 << 64, (1 << 127) - 1]
            unsigned_powers = [0, (1 << 64) - 1, 1 << 127, (1 << 127) + 1, (1 << 128) - 1]
            bases = {0, 1 % m, m - 1}
            bases.update(rng.randrange(m) for _ in range(4 if mode == "quick" else 12))
            for a in sorted(bases):
                for e in signed_powers:
                    if e >= 0 or math.gcd(a, m) == 1:
                        add(kind, "P", m, f"{a} s {e}", pow(a, e, m))
                for e in unsigned_powers:
                    add(kind, "P", m, f"{a} u {e}", pow(a, e, m))
        if kind >= 2:
            width = 64 if kind & 1 else 32
            for i in range({"quick": 40, "full": 300, "stress": 3000}[mode]):
                m = rng.getrandbits(width) or 1
                a, b = rng.randrange(m), rng.randrange(m)
                arithmetic(kind, m, a, b)
                arithmetic(kind, m, m - 1, m - 1)
                e = [511, 512, 513, 65535, 65536, 65537, (1 << 128) - 1][i % 7]
                add(kind, "P", m, f"{a} u {e}", pow(a, e, m))
                x = rng.getrandbits(128) - (1 << 127)
                add(kind, "N", m, f"s {x}", x % m, x % m)
    return requests, answers


def extract_struct(source: str, kind: str) -> str:
    # Ignore braces in comments/literals without changing source offsets.
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda match: " " * len(match.group()), source, flags=re.S)
    declaration = re.search(r"\bstruct\s+" + re.escape(kind) + r"\b", masked)
    if declaration is None:
        raise Failure(f"standalone-copy fixture: missing struct {kind}")
    start = masked.rfind("template<", 0, declaration.start())
    brace = masked.find("{", declaration.end())
    if start < 0 or brace < 0:
        raise Failure(f"standalone-copy fixture: cannot extract {kind}")
    depth = 1
    for end in range(brace + 1, len(masked)):
        depth += (masked[end] == "{") - (masked[end] == "}")
        if depth == 0:
            if masked[end + 1:end + 2] != ";":
                raise Failure(f"standalone-copy fixture: unterminated {kind}")
            return source[start:end + 2]
    raise Failure(f"standalone-copy fixture: unbalanced {kind}")


def standalone_sources(temp: Path, header: Path) -> list[Path]:
    source = header.read_text()
    preamble = ("#include <cassert>\n#include <bit>\n#include <cstdint>\n"
                "#include <type_traits>\n#include <utility>\n"
                "using uint = uint32_t; using lng = int64_t; using ulng = uint64_t;\n"
                "using lll = __int128_t; using ulll = __uint128_t;\n")
    paths = []
    for kind in ["ModIntMini", "ModInt64Mini", "DynModIntMini", "DynModInt64Mini"]:
        dynamic, wide = kind.startswith("Dyn"), "64" in kind
        alias = f"{kind}<981>" if dynamic else f"{kind}<18446744073709551615ULL>" if wide else f"{kind}<4294967295U>"
        setup = "A::setMod(18446744073709551615ULL);" if dynamic and wide else "A::setMod(4294967295U);" if dynamic else ""
        program = f"""
using A = {alias};
int main() {{
    {setup}
    A a = -1, b = 2, r = 4;
    if ((a + a).val() != A::mod() - 2 || (a * a).val() != 1) {{ return 1; }}
    if ((a - b).val() != A::mod() - 3 || pow(a, 512).val() != 1) {{ return 2; }}
    if (!tryInv(b, r) || r * b != A(1) || inv(b) != r || a / b != a * r) {{ return 3; }}
    if (tryInv(A(3), r) || r * b != A(1)) {{ return 4; }}
    if (A::raw(A::mod() - 1) != a || +a != a || (-a).val() != 1) {{ return 5; }}
    lll e = -lll(ulll(1) << 126) - lll(ulll(1) << 126);
    if (A(e).val() != A::mod() / 2 || A(~ulll(0)).val() != 0) {{ return 6; }}
    if (pow(a, e) != A(1) || pow(a, ~ulll(0)) != a) {{ return 7; }}
    a += b; a *= b; a -= b; a /= b;
    if (a != A(0)) {{ return 8; }}
    return 0; }}
"""
        path = temp / f"copy-{kind}.cpp"
        path.write_text(preamble + extract_struct(source, kind) + program)
        paths.append(path)
    return paths


def cross_tu_sources(temp: Path, header: Path) -> tuple[Path, Path]:
    include = f'#include "{header}"\n'
    a, b = temp / "globals-a.cpp", temp / "globals-b.cpp"
    a.write_text(include + """
using A = DynModIntMini<990>;
using W = DynModInt64Mini<990>;
A default_a = A(1000000) * A(1000000);
W default_w = W(1000000) * W(1000000);
ulng fromOther();
int main() {
    if (default_a.val() != 757402647 || default_w.val() != 757402647) { return 1; }
    A::setMod(17); W::setMod(19);
    if (A::mod() != 17 || W::mod() != 19) { return 2; }
    if ((A(1000000) * A(1000000)).val() != 13) { return 3; }
    return fromOther() != 20 || DynModIntMini<991>::mod() != 17 ||
           DynModInt64Mini<991>::mod() != 19; }
""")
    b.write_text(include + """
using B = DynModIntMini<991>;
using W = DynModInt64Mini<991>;
B custom_b = [] { B::setMod(17); return B(1000000) * B(1000000); }();
W custom_w = [] { W::setMod(19); return W(1000000) * W(1000000); }();
ulng fromOther() { return custom_b.val() + custom_w.val(); }
""")
    return a, b


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.getenv("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.getenv("CP_TEST_SEED", "20260927")))
    parser.add_argument("--scalar-only", action="store_true", help="accepted for runner compatibility; minis are scalar")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("seed must be in [0, 2^64)")
    compiler = shlex.split(os.getenv("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-march=x86-64", "-mno-avx", "-mno-avx2"]
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG"]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS"])]
    if args.mode != "quick":
        variants.append(("sanitized-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS",
                         "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                         "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]))
    entry = Path(__file__).resolve()
    header = entry.parents[2] / "01-Core" / "06-modintmini.hpp"
    started = time.monotonic()
    context = f"modintmini mode={args.mode} seed={args.seed}"
    report("RUN", f"{context}; independent Python integer oracle; no mini ISA-specific paths")
    try:
        version = execute([*compiler, "-dumpversion"], "compiler version", env).strip()
        if int(version.split('.')[0]) < 14:
            raise Failure(f"GCC 14+ required, CXX version={version}")
        requests, expected = corpus(args.mode, args.seed)
        data = "\n".join(requests) + "\n"
        report("PASS", f"generated {len(requests)} exact Python oracle cases")
        with tempfile.TemporaryDirectory(prefix="cp-modintmini-tests-") as directory:
            temp = Path(directory)
            copies = standalone_sources(temp, header)
            a, b = cross_tu_sources(temp, header)
            for name, options in variants:
                binary = str(temp / name)
                config = f"{context}/{name}"
                execute([*compiler, *flags, *options, str(entry.with_suffix('.cpp')), "-o", binary],
                        "compile " + config, env)
                report("PASS", f"{name} compilation")
                output = execute([binary], config + " API fixtures", env)
                for line in output.splitlines():
                    print(f"  [{name}] {line}", flush=True)
                actual = execute([binary, "--oracle"], config + " exact oracle", env, data=data).splitlines()
                if len(actual) != len(expected):
                    raise Failure(f"{config} oracle line count expected={len(expected)} actual={len(actual)}")
                for i, (got, want) in enumerate(zip(actual, expected)):
                    if got != want:
                        raise Failure(f"{config} first failing independent case index={i}\n"
                                      f"smallest known reproducer stdin={requests[i]!r}\nexpected={want}\nactual={got}\n"
                                      f"command={shlex.join([binary, '--oracle'])}")
                report("PASS", f"{name} exact oracle and full/mini agreement ({len(requests)} cases)")
                if name == "checked-scalar":
                    for kind in range(4):
                        deaths = ["raw", "inverse", "division", "negative-power"]
                        if kind >= 2:
                            deaths.append("zero-modulus")
                        for case in deaths:
                            execute([binary, "--death", str(kind), case], config + f" precondition {kind}/{case}", env, death=True)
                    report("PASS", "checked-scalar all 18 declared-precondition deaths")
                for source in copies:
                    target = str(source.with_suffix(""))
                    execute([*compiler, *flags, *options, str(source), "-o", target],
                            config + " extracted-struct compile " + source.stem, env)
                    execute([target], config + " extracted-struct execute " + source.stem, env)
                report("PASS", f"{name} all four individually copied structs (standard headers/aliases only)")
                for first, second in [(a, b), (b, a)]:
                    target = str(temp / "cross-tu")
                    execute([*compiler, *flags, *options, str(first), str(second), "-o", target],
                            config + " cross-TU compile " + first.name, env)
                    execute([target], config + " cross-TU execute " + first.name, env)
                report("PASS", f"{name} default/custom globals in both link orders and width independence")
            for kind in ["ModIntMini", "ModInt64Mini"]:
                source = temp / "bad.cpp"
                source.write_text(f'#include "{header}"\n{kind}<0> invalid;\n')
                execute([*compiler, "-std=gnu++20", "-DNDEBUG", "-fsyntax-only", str(source)],
                        "zero static modulus rejection " + kind, env, compile_fail=True)
            report("PASS", "both zero static moduli rejected with NDEBUG")
    except (OSError, Failure, ValueError) as error:
        report("FAIL", f"{context}: {error}")
        return 1
    report("PASS", f"{context}; {len(variants)} configurations in {time.monotonic() - started:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
