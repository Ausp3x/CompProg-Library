#!/usr/bin/env python3
"""Independent exact verification for the six modular-integer types in 05-modint.hpp.

quick: all API fixtures, fixed boundary moduli, exhaustive small rings and 100
random pairs/modulus, 100 Python integer cases/modulus; optimized and checked
scalar/available AVX2. full: 3,000 C++ pairs/modulus, dynamic exhaustive mod<=60,
1,000 Python cases/modulus, more roots/primality, plus scalar/AVX2 ASan+UBSan.
stress: 30,000 pairs/modulus, dynamic exhaustive mod<=120 and 10,000 Python
cases/modulus, same configurations. Every mode checks declared preconditions.
Path sweeps (each internal path of the header against 128-bit schoolbook oracles
written in the fixture): construction from all 18 integral types over boundary
and random patterns, ring and inverse boundaries, every exponent 0..4096 twice
in a row (64 in quick for the types without the base-4 path) with alternating,
random, 128-bit and negative exponents, roots of every 2-power order, chunked
stream tokens; static types take one modulus per path class, dynamic types a
setMod sequence of 22 (32-bit) or 46 (64-bit) moduli with every context field
checked after each, plus every inverse of every modulus up to 150/1000/2000 and
every root of the primes up to 600/2000/8000 (quick/full/stress). The static
64-bit fixture also runs isPrime (table at compile time and run time, sieve below
2^17/2^21/2^24, windows of 300/20,000/100,000 at 2^31..2^64, random and
adversarial composites), Reciprocal::mul over 520/20,220/200,220 moduli and
biasOf, shortReciprocal and unitShift against their definitions.
The Montgomery-form types run the same fixtures on the odd moduli of each list.
full and stress also run each dynamic fixture in quick mode on a copy of the header
with the character-class lookup it keeps for other standard libraries.
C++ standalone/aggregate/multiple-TU linkage is covered by 02-integration.py.
Only Python's standard library and GCC 14+ are required. Python integers and
built-in modular pow are independent of the C++ reduction implementation.
"""
from __future__ import annotations

import argparse
import bz2
import functools
import math
import os
from pathlib import Path
import random
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap
from _00_cpp_test_runner import Builder, cpu_runs, timed_run, x86, x86_only

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
        result = timed_run(label, command, cwd="/tmp", env=env, input=data, text=True,
                                  capture_output=True, timeout=600)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout=600s command={shlex.join(command)}\n"
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


def corpus(mode: str, seed: int, wide: bool, dynamic: bool, form: bool = False) -> tuple[list[str], list[str]]:
    rng = random.Random(seed)
    mods = [1, 2, 4, 7, 17, 97, 998244353, 2147483647, 2147483648, 4294967291, 4294967295]
    if wide:
        mods += [4294967296, 2305843009213693951, 9223372036854775808, 18446744069414584321, 18446744073709551557, 18446744073709551615]
    primes = {2, 7, 17, 97, 998244353, 2147483647, 4294967291, 2305843009213693951, 18446744069414584321, 18446744073709551557}
    if dynamic:  # a runtime modulus instantiates nothing: one more per reciprocal, bias, power-dispatch and root class of the header
        mods += [3, 128, 32768, 167772161, 469762049, 1000000007, 2147483659, 2147483693]
        primes |= {3, 167772161, 469762049, 1000000007, 2147483659, 2147483693}
        if wide:
            mods += [4294967311, 10**18, 4179340454199820289, 2**62 - 57, 2**62 - 1, 2**62, 2**62 + 1, 2**63 - 25, 2**63 + 29, 2**64 - 2]
            primes |= {4294967311, 4179340454199820289, 2**62 - 57, 2**63 - 25, 2**63 + 29}
    if form:  # a Montgomery-form type takes odd moduli only
        mods = [m for m in mods if m % 2]
    count = {"quick": 100, "full": 1000, "stress": 10000}[mode]
    requests, answers = [], []

    def add(request: str, *expected: int) -> None:
        requests.append(request)
        answers.append(" ".join(map(str, expected)))

    for m in mods:
        signed = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63), -(1 << 32), -(1 << 31) - 1, -(1 << 31), -(1 << 31) + 1, -m - 1, -m, -1,
                  0, 1, m - 1, m, m + 1, (1 << 31) - 1, (1 << 31), (1 << 31) + 1, (1 << 32) - 1, (1 << 32), (1 << 32) + 1, (1 << 63) - 1, (1 << 63), (1 << 127) - 1]
        unsigned = [0, 1, m - 1, m, m + 1, (1 << 32) - 1, (1 << 32), (1 << 32) + 1, (1 << 64) - 1, (1 << 64), (1 << 127), (1 << 128) - 1]
        # The largest long long whose sum with the bias (the least multiple of m from 2^63 on) still fits 64 bits, and its neighbours.
        guard = (1 << 64) - 1 - (m if m >> 63 else -(-(1 << 63) // m) * m)
        signed += [guard - 1, guard, guard + 1, -guard, -guard - 1]
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
        exponents = [-(1 << 127), -(1 << 127) + 1, -(1 << 64), -(1 << 63), -65537, -65536, -65535, -1000, -513, -512, -511, -8, -3, -2, -1,
                     0, 1, 2, 3, 4, 5, 6, 7, 8, 63, 64, 65, 511, 512, 513, 65535, 65536, 65537, (1 << 64), (1 << 127) - 1]
        for i in range(12 if mode == "quick" else 40 if mode == "full" else 200):
            a = rng.randrange(m)
            for e in exponents:
                if e >= 0 or math.gcd(a, m) == 1:
                    add(f"P {m} {a} s {e}", pow(a, e, m))
            for e in [0, (1 << 64) - 1, (1 << 127), (1 << 127) + 1, (1 << 128) - 1]:
                add(f"P {m} {a} u {e}", pow(a, e, m))
            for e in [16, 17, 4095, 4096, (1 << 63), (1 << 64) - 1]:  # twice in a row: a changed exponent, then a repeated one
                for _ in range(2):
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
        chunk = 10**18  # the stream reduces 18 digits per step: lengths around its multiples, and a first chunk that is 0 mod m
        head = str((chunk - 1) // m * m).zfill(18) if m < chunk else "0" * 18
        chunked = [str(rng.randrange(10**(n - 1), 10**n)) for n in (1, 17, 18, 19, 35, 36, 37, 54, 55, 300)]
        chunked += [head + "7", "-" + head + str(rng.randrange(chunk)).zfill(18), "+" + head * 2 + str(rng.randrange(10**19)), "0" * 37 + "9" * 19]
        for token in ["0", "+0", "-0", "0000007", str(-(1 << 127)), str((1 << 128) - 1),
                      "+" + "9" * 2000, "-" + "1234567890" * 200, *chunked]:
            add(f"S {m} {token}", 0, int(token) % m)
        for token in ["+", "-", "x", "1x", "++1", "--1", "+-1", "0xFF", "1.0"]:
            add(f"S {m} {token}", 1, 7 % m)
    return requests, answers


def main(entry: Path) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.getenv("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.getenv("CP_TEST_SEED", "20260927")))
    parser.add_argument("--scalar-only", action="store_true")
    parser.add_argument("--type", choices=("all", "modint32", "modint64", "dynmodint32", "dynmodint64", "montmodint64", "dynmontmodint64"),
                        default="all", help="default: verify all six public types")
    parser.add_argument("--pseudoprimes", metavar="FILE", help="run only isPrime over Feitsma and Galway's list of the base-2 Fermat "
                        "pseudoprimes below 2^64 (psps-below-2-to-64.txt, plain or .bz2): every entry must be rejected")
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64:
        parser.error("seed must be in [0, 2^64)")
    if args.pseudoprimes:
        return pseudoprimes(entry, Path(args.pseudoprimes))
    # The dynamic fixtures compile in a quarter of the static ones' time: first, so a broken header fails early (fail-fast mutant tests).
    for kind, wide, dynamic in [("dynmodint32", False, True), ("dynmodint64", True, True), ("dynmontmodint64", True, True),
                                ("modint32", False, False), ("modint64", True, False), ("montmodint64", True, False)]:
        if args.type in ("all", kind):
            result = run_suite(entry, args, kind=kind, wide=wide, dynamic=dynamic, form="mont" in kind)
            if result:
                return result
    report("PASS", f"05-modint.hpp requested types={args.type} mode={args.mode} seed={args.seed}")
    return 0


PSEUDOPRIMES = (118968378, 31894014, 18446744066047760377)  # the list's published size, its strong pseudoprimes, its last entry


def pseudoprimes(entry: Path, path: Path) -> int:
    """isPrime over the complete list of base-2 Fermat pseudoprimes below 2^64, the proof basis of its test from 2^32: a
    composite that passes the strong test to base 2 is in the list, so rejecting every entry leaves no composite accepted."""
    compiler = shlex.split(os.getenv("CXX", "g++"))
    report("RUN", f"05-modint64 isPrime over {path}")
    try:
        with tempfile.TemporaryDirectory(prefix="cp-05-modint-pseudoprimes-") as directory:
            binary = str(Path(directory) / "optimized")
            Builder(compiler, Path(directory), env=dict(os.environ), failure=Failure).build(
                "compile 05-modint64 for the pseudoprime list", ["-std=gnu++20", "-O3", "-DNDEBUG"], [entry.with_name("05-modint64_cases.cpp")], binary, 600)
            with (bz2.open if path.suffix == ".bz2" else open)(path, "rb") as source, \
                    subprocess.Popen([binary, "--pseudoprimes"], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
                shutil.copyfileobj(source, process.stdin, 1 << 22)
                process.stdin.close()
                fields = process.stdout.read().split()
                if process.wait() or len(fields) != 5:
                    raise Failure(f"pseudoprime run failed: exit {process.returncode}, output {fields!r}")
        total, strong, accepted, order, last = map(int, fields)
        if accepted or order or (total, strong, last) != PSEUDOPRIMES:
            raise Failure(f"pseudoprime list: total {total}, strong to base 2 {strong}, accepted by isPrime {accepted}, out of order {order}, "
                          f"last {last}; expected {PSEUDOPRIMES[0]}, {PSEUDOPRIMES[1]}, 0, 0, {PSEUDOPRIMES[2]}")
    except (Failure, OSError) as error:
        report("FAIL", str(error))
        return 1
    report("PASS", f"isPrime rejects all {total} base-2 Fermat pseudoprimes below 2^64 ({strong} of them strong); the list is complete and ordered")
    return 0


def run_suite(entry: Path, args: argparse.Namespace, *, kind: str, wide: bool, dynamic: bool, form: bool = False) -> int:
    compiler = shlex.split(os.getenv("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow"]
    scalar = x86_only(["-march=x86-64", "-mno-avx", "-mno-avx2"], compiler)
    vector = x86_only(["-march=x86-64", "-mavx2"], compiler)
    variants = [("optimized-scalar", ["-O3", "-DNDEBUG", *scalar]),
                ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    accelerated = not args.scalar_only and cpu_runs({"avx2"}, compiler)
    if accelerated:
        variants += [("optimized-avx2", ["-O3", "-DNDEBUG", *vector]),
                     ("checked-avx2", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *vector])]
    else:
        report("SKIP", "AVX2: " + ("--scalar-only requested" if args.scalar_only else "CPU unavailable" if x86(compiler)
                                   else "target is not x86-64"))
    if args.mode != "quick":
        sanitizer = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                     "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector]))
    deaths = ["raw", "inverse", "division", "negative-power", "root", "legacy-root", "narrow"]
    if dynamic:
        deaths += ["zero-modulus", "prime-flag", "unknown-root"] + (["even-modulus"] if form else [])
    label = "05-" + kind
    fixture = entry.with_name(label + "_cases.cpp")
    started = time.monotonic()
    report("RUN", f"{label} mode={args.mode} seed={args.seed}; independent Python integer oracle")
    try:
        version = execute([*compiler, "-dumpversion"], "compiler version", env).strip()
        if int(version.split('.')[0]) < 14:
            raise Failure(f"GCC 14+ required, CXX version={version}")
        requests, expected = corpus(args.mode, args.seed, wide, dynamic, form)
        data = "\n".join(requests) + "\n"
        report("PASS", f"generated {len(requests)} exact Python oracle cases")
        with tempfile.TemporaryDirectory(prefix=f"cp-{label}-tests-") as directory:
            temp = Path(directory)
            builder = Builder(compiler, temp, env=env, failure=Failure)
            # MB a compile may take, by fixture and build: the peaks of GCC 14, 15 and 16 on 2026-11-19 (static 64-bit 2,152 sanitized, 1,494
            # checked, 1,161 optimized; static 32-bit 1,613 and 1,197; dynamic 889 and 717) with a margin; two builds run at once within the cap.
            need = dict(zip(("sanitized", "checked", "optimized"), (1000, 800, 600) if dynamic else (2300, 1600, 1300) if wide else (1750, 1300, 900)))
            compiled = builder.build_all([(name, f"compile {label}/{name} mode={args.mode} seed={args.seed}", [*flags, "-Wconversion", "-Werror", *options], [fixture], str(temp / name), 600,
                                           need[name.split("-")[0]]) for name, options in variants])
            def one(name, options):
                binary, mark = str(temp / name), builder.stopwatch()
                config = f"{label}/{name} mode={args.mode} seed={args.seed}"
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
                builder.report(mark, f"{label} {name}", args.mode, compiled[name])
            builder.concurrently([(1 << 40 if args.mode == "stress" else 500, functools.partial(one, name, options)) for name, options in variants])
            if not dynamic and not wide and args.mode != "quick":
                for name, target in [("scalar", scalar)] + ([("avx2", vector)] if accelerated else []):
                    binary = str(temp / ("consumers-" + name))
                    builder.build("Matrix/ModFac existing consumer smoke compile " + name, [*flags, "-O2", *target], [entry.with_name("05-modint_consumers.cpp")], binary, 600)
                    execute([binary], "Matrix/ModFac existing consumer smoke execute " + name, env)
                report("PASS", "existing Matrix/ModFac consumer smoke scalar/available AVX2")
            if dynamic and args.mode != "quick":
                root = entry.parents[2]
                header = root / "01-Core" / "05-modint.hpp"
                kind = "DynMontModInt64" if form else "DynModInt64" if wide else "DynModInt"
                other_kind = "DynModInt" if wide else "DynModInt64"
                includes = f'#include "{header}"\n'
                a, b = temp / "a.cpp", temp / "b.cpp"
                # Globals of both units construct from negatives, divide, take both power loops and a root before main: on the
                # default context, on one both units share (ID 903), and on one set to a large modulus inside an initializer.
                p, big = 998244353, 18446744073709551557 if wide else 4294967291
                want_big = (-pow(3, -1, big) + 2 * pow(-2, 1000003, big)) % big
                a.write_text(includes + f"using A={kind}<900>; using X={other_kind}<900>; using S={kind}<903>;\n"
                             "A global_product=A(1000000)*A(1000000);\nA global_quotient=A(-1)/A(3);\n"
                             "A global_power=pow(A(-2),1000003)+pow(A(-2),1000003);\nA global_root=sqrt(A(4));\n"
                             "S global_shared=S(-7)*S(1000000);\nulng fromOther();\nulng bigFromOther();\nulng sharedFromOther();\n"
                             "int main(){if(global_product.val()!=757402647)return 1;"
                             f"if(global_quotient.val()!={-pow(3, -1, p) % p}||global_power.val()!={2 * pow(-2, 1000003, p) % p}||global_root.val()!=2)return 4;"
                             f"if(global_shared.val()!={-7000000 % p}||sharedFromOther()!={pow(-3, 1000003, p)})return 5;"
                             f"if(bigFromOther()!={want_big}ULL)return 6;"
                             "A::setMod(17);X::setMod(12);if(A::mod()!=17||X::mod()!=12)return 2;"
                             "if((A(1000000)*A(1000000)).val()!=13)return 3;"
                             f"return fromOther()!=13||{kind}<901>::mod()!=17;}}\n")
                b.write_text(includes + f"using B={kind}<901>; using C={kind}<902>; using S={kind}<903>;\n"
                             "B global_custom=[] {B::setMod(17);return B(1000000)*B(1000000);}();\n"
                             f"C global_big=[] {{C::setMod({big}ULL);return C(-1)/C(3)+pow(C(-2),1000003)+pow(C(-2),1000003);}}();\n"
                             "S global_shared_power=pow(S(-3),1000003);\n"
                             "ulng fromOther(){return global_custom.val();}\nulng bigFromOther(){return global_big.val();}\n"
                             "ulng sharedFromOther(){return global_shared_power.val();}\n")
                for first, second in [(a, b), (b, a)]:
                    binary = str(temp / "cross-tu")
                    builder.build("cross-TU globals and same-ID cross-width contexts compile", [*flags, "-O2"], [first, second], binary, 600)
                    execute([binary], "cross-TU globals and same-ID cross-width contexts execute", env)
                report("PASS", "cross-TU default/custom/shared global initialization (construction, division, powers, root), both link orders, width independence")
                # The lookup of the stream's character classes that the header keeps for other standard libraries (its #else branch) is
                # selected by no build here: a copy of the header with that branch chosen runs the whole fixture in quick mode.
                text, lines = header.read_text(), fixture.read_text().splitlines()
                if text.count("#ifdef __GLIBCXX__") != 1 or lines[0] != '#include "../../01-Core/05-modint.hpp"' or lines[-1] != '#include "05-modint_test_support.hpp"':
                    raise Failure("portable build: 05-modint.hpp or the fixture no longer has the expected lines")
                (temp / "05-modint.hpp").write_text(text.replace("#ifdef __GLIBCXX__", "#if 0"))
                source = temp / "portable.cpp"
                source.write_text("\n".join([f'#include "{temp / "05-modint.hpp"}"', "#define TEST_PORTABLE 1", *lines[1:-1],
                                             f'#include "{entry.with_name("05-modint_test_support.hpp")}"']) + "\n")
                binary = str(temp / "portable")
                builder.build("portable classification lookup compile", [*flags, "-Wconversion", "-Werror", "-O2", f"-I{header.parent}"], [source], binary, 600)
                execute([binary, "--mode", "quick", "--seed", str(args.seed)], "portable classification lookup execute", env)
                report("PASS", "fixture on the header's classification lookup for other standard libraries (quick mode)")
            # Invalid static moduli must be diagnosed even when assertions are removed.
            if not dynamic:
                source = temp / "bad.cpp"
                header = entry.parents[2] / "01-Core" / "05-modint.hpp"
                kind = "MontModInt64" if form else "ModInt64" if wide else "ModInt"
                for bad in [0, 4] if form else [0]:
                    source.write_text(f'#include "{header}"\n{kind}<{bad}> bad;\n')
                    execute([*compiler, "-std=gnu++20", "-DNDEBUG", "-fsyntax-only", str(source)],
                            f"compile-time rejection of the modulus {bad}", env, compile_fail=True)
                report("PASS", "compile-time zero modulus rejection" + (" and even modulus rejection" if form else ""))
    except (OSError, Failure, ValueError) as error:
        report("FAIL", f"{label} mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"{label} {len(variants)} configurations in {time.monotonic() - started:.2f}s")
    return 0
