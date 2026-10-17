#!/usr/bin/env python3
"""InfIntMini oracle suite: Python big integers with a sentinel model, plus C++ self-tests.

Every mode builds the optimized (-O2 -DNDEBUG) and checked (-D_GLIBCXX_ASSERTIONS) programs
from two translation units, runs the C++ self-tests (an independent base-1e5 schoolbook,
division by construction, adversarial long-division windows, aliasing, trim and streams)
and streams the generated cases through the --protocol process, comparing every line with
Python's integers and a model of the signed-infinity rules (a fixed sentinel grid in every
mode). The checked build also runs the --invalid precondition probes; the header alone, a
standalone copy (the pasted template, no includes) and the 98-basic/99-all aggregates are
compiled, and a program with both InfInt and InfIntMini compares them on the shared domain
(supporting evidence only). quick: operands up to 600 digits. full: adds ASan/UBSan (64 MB
quarantine), operands up to 133000 digits and NTT products up to 2^15 limbs. stress:
--rounds seeds with operands up to 660000 digits. gcd cases keep the smaller operand below
100000 bits (Euclid is quadratic). Every check survives -DNDEBUG.
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
import queue
import random
import resource
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

SOURCE = Path(__file__).resolve().with_suffix(".cpp")
SECOND = SOURCE.with_name("08-infintmini_tester_tu.cpp")
MIXED = SOURCE.with_name("08-infintmini_tester_mixed.cpp")
HEADER = SOURCE.parents[2] / "01-Core/08-infintmini.hpp"
ROOT = SOURCE.parents[2]
INVALID_CASES = ("div-zero", "mod-zero", "divmod-zero", "scalar-div-zero", "read-base", "tostring-base", "ctor-string", "pow-negative")
DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
INF = "inf"


class Failure(RuntimeError):
    pass


def report(kind: str, message: str) -> None:
    color = {"PASS": "\033[32m", "FAIL": "\033[31m"}.get(kind, "")
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        color = ""
    if sys.stdout.isatty():
        print("\r\033[K", end="")
    print(f"{color}{kind}{chr(27) + '[0m' if color else ''} {message}", flush=True)


def execute(command: list[str], label: str, timeout: int, env: dict[str, str], death: bool = False) -> str:
    if sys.stdout.isatty():
        print(f"\r\033[KRUN {label}", end="", flush=True)
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, text=True, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout={timeout}s command={shlex.join(command)}") from error
    okay = result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower() if death else result.returncode == 0
    if not okay:
        raise Failure(f"{label}: exit={result.returncode} expected={'assertion/SIGABRT' if death else 0}\n"
                      f"command={shlex.join(command)}\nstdout:\n{result.stdout[-3000:]}\nstderr:\n{result.stderr[-3000:]}")
    return result.stdout


def to_base(a: int, base: int) -> str:
    if a == 0:
        return "0"
    out, m = [], abs(a)
    while m:
        m, d = divmod(m, base)
        out.append(DIGITS[d])
    return ("-" if a < 0 else "") + "".join(reversed(out))


def show(v) -> str:
    return v if isinstance(v, str) else str(v)


# Sentinel model: an infinity is the string "inf" or "-inf"; finite values are ints.
def is_inf(v) -> bool:
    return isinstance(v, str)


def sgn(v) -> int:
    return (-1 if v.startswith("-") else 1) if is_inf(v) else (v > 0) - (v < 0)


def inf_of(s: int) -> str:
    return INF if s > 0 else "-" + INF


def m_add(a, b):
    if is_inf(a) or is_inf(b):
        if is_inf(a) and is_inf(b) and sgn(a) != sgn(b):
            return 0
        return b if is_inf(b) else a
    return a + b


def m_neg(a):
    return inf_of(-sgn(a)) if is_inf(a) else -a


def m_mul(a, b):
    if a == 0 or b == 0:
        return 0
    if is_inf(a) or is_inf(b):
        return inf_of(sgn(a) * sgn(b))
    return a * b


def m_divmod(a, b):
    if is_inf(a) and is_inf(b):
        return sgn(a) * sgn(b), 0
    if is_inf(b):
        return 0, a
    if is_inf(a):
        return inf_of(sgn(a) * sgn(b)), 0
    q = abs(a) // abs(b)
    q = -q if (a < 0) != (b < 0) else q
    return q, a - q * b


def m_cmp(a, b) -> int:
    def key(v):
        return (sgn(v), 0) if is_inf(v) else (0, v)
    ka, kb = key(a), key(b)
    return (ka > kb) - (ka < kb)


def m_magcmp(a, b) -> int:
    ka = (1, 0) if is_inf(a) else (0, abs(a))
    kb = (1, 0) if is_inf(b) else (0, abs(b))
    return (ka > kb) - (ka < kb)


def m_abs(a):
    return INF if is_inf(a) else abs(a)


def m_gcd(a, b):
    if (is_inf(a) and b != 0) or a == 0:
        return m_abs(b)
    if is_inf(b) or b == 0:
        return m_abs(a)
    return math.gcd(a, b)


def m_pow(a, e: int):
    if e == 0:
        return 1
    if is_inf(a):
        return inf_of(sgn(a) if e & 1 else 1)
    return a ** e


def rand_bits(rng: random.Random, bits: int, signed: bool = True) -> int:
    if bits <= 0:
        return 0
    x = (1 << (bits - 1)) | rng.getrandbits(bits - 1)
    return -x if signed and rng.randrange(2) else x


def build_cases(seed: int, mode: str):
    """Yields (command, description, expected) lazily so the corpus never sits in memory."""
    rng = random.Random(seed)
    count = {"quick": 80, "full": 500, "stress": 1500}[mode]
    big = {"quick": [600], "full": [20000, 66000, 133000], "stress": [133000, 400000, 660000]}[mode]
    bits_choices = [1, 2, 16, 17, 33, 49, 50, 63, 64, 65, 66, 127, 128, 129, 166, 167, 200, 333, 1000, 3000]
    specials = [0, 1, -1, 99999, 100000, -100000, 10 ** 10 - 1, 10 ** 15 - 1, 10 ** 15, -(10 ** 15), 10 ** 20, 2 ** 63, -(2 ** 63), 2 ** 64 - 1, 2 ** 64, INF, "-" + INF]

    def value(t: int):
        if t % 9 == 0:
            return rng.choice(specials)
        if t % 13 == 0:
            return rand_bits(rng, rng.choice(big) * 10 // 3)
        if t % 11 == 0:
            return 10 ** rng.randrange(1, 300) - rng.randrange(0, 3)
        return rand_bits(rng, rng.choice(bits_choices))

    for t in range(count):
        a, b = value(t), value(t + 7 * (t % 3))
        if t % 5 == 0:
            b = a
        q_r = "- -" if b == 0 else " ".join(map(show, m_divmod(a, b)))
        yield (f"ar {show(a)} {show(b)}", f"arithmetic #{t}",
               f"{show(m_add(a, b))} {show(m_add(a, m_neg(b)))} {show(m_mul(a, b))} {q_r} {m_cmp(a, b)} {int(m_cmp(a, b) == 0)} "
               f"{m_magcmp(a, b)} {show(m_neg(a))} {show(a)} {show(m_abs(a))}")
        if is_inf(a) or is_inf(b) or min(abs(a), abs(b)).bit_length() < 100000:
            yield f"gd {show(a)} {show(b)}", f"gcd #{t}", show(m_gcd(a, b))
        if is_inf(a) or abs(a) < 10 ** 40:
            e = rng.choice([0, 1, 2, 3, 7, 16, 31, 64])
            yield f"pw {show(a)} {e}", f"pow #{t}", show(m_pow(a, e))
        if not is_inf(a) and abs(a) < 10 ** 3000:
            base = rng.choice([2, 3, 7, 8, 10, 16, 23, 24, 35, 36])
            s = to_base(a, base)
            yield f"tb {a} {base}", f"toString base {base} #{t}", f"{s} {a}"
            text = s.lower() if t % 2 else s
            if t % 3 == 0:
                text = ("+" + text) if a >= 0 else text
                if a >= 0 and t % 6 == 0:
                    text = "+" + "0" * rng.randrange(1, 4) + text[1:]
            yield f"rd {text} {base}", f"read base {base} #{t}", f"1 {a} {to_base(a, base)}"
            fmt = rng.choice(["base", "hex", "oct"])
            sb = {"hex": 16, "oct": 8}.get(fmt, rng.choice([2, 10, 36]))
            yield f"io {a} {sb} {fmt}", f"stream {fmt} {sb} #{t}", f"{to_base(a, sb)} {a}"
        if is_inf(a) or abs(a) < 10 ** 400:
            if is_inf(a):
                lv, uv = (2 ** 63 - 1, 2 ** 64 - 1) if sgn(a) > 0 else (-(2 ** 63), 0)
            else:
                uv = a % 2 ** 64
                lv = uv - 2 ** 64 if uv >= 2 ** 63 else uv
            yield f"cs {show(a)}", f"casts #{t}", f"{lv} {uv} {int(a != 0)} {int(a == 0)}"
        if not is_inf(a):
            kind = rng.choice(["i32", "u32", "u64", "i64"])
            lo, hi = {"i32": (-(2 ** 31), 2 ** 31 - 1), "u32": (0, 2 ** 32 - 1), "u64": (0, 2 ** 63 - 1), "i64": (-(2 ** 63), 2 ** 63 - 1)}[kind]
            v = rng.choice([lo, hi, 0, 1, -1 if lo < 0 else 2, 99999, 100000, 10 ** 15, rng.randint(lo, hi)])
            v = min(max(v, lo), hi)
            prod = a * v
            if v == 0:
                yield f"sc {kind} {a} {v}", f"scalar {kind} #{t}", f"{prod} {prod} - - - -"
            else:
                q, r = m_divmod(a, v)
                yield f"sc {kind} {a} {v}", f"scalar {kind} #{t}", f"{prod} {prod} {q} {q} {r} {r}"
    grid = [INF, "-" + INF, 0, 1, -7, 10 ** 20, -(10 ** 20) + 3]
    for a in grid:
        for b in grid:
            q_r = "- -" if b == 0 else " ".join(map(show, m_divmod(a, b)))
            yield (f"ar {show(a)} {show(b)}", f"sentinel grid {show(a)} {show(b)}",
                   f"{show(m_add(a, b))} {show(m_add(a, m_neg(b)))} {show(m_mul(a, b))} {q_r} {m_cmp(a, b)} {int(m_cmp(a, b) == 0)} "
                   f"{m_magcmp(a, b)} {show(m_neg(a))} {show(a)} {show(m_abs(a))}")
            yield f"gd {show(a)} {show(b)}", f"sentinel gcd {show(a)} {show(b)}", show(m_gcd(a, b))
        yield f"pw {show(a)} 3", f"sentinel pow {show(a)}", show(m_pow(a, 3))
    widths = {"i8": (8, True), "u8": (8, False), "i16": (16, True), "u16": (16, False), "i32": (32, True), "u32": (32, False),
              "i64": (64, True), "u64": (64, False), "i128": (128, True), "u128": (128, False)}
    for kind, (w, signed) in widths.items():
        lo, hi = (-(1 << (w - 1)), (1 << (w - 1)) - 1) if signed else (0, (1 << w) - 1)
        for v in (lo, lo + 1, -1 if signed else 1, 0, 1, hi - 1, hi, rng.randint(lo, hi)):
            yield f"ct {kind} {v}", f"construct {kind} {v}", f"{v} {v}"
    for v in (0, 1):
        yield f"ct b {v}", f"construct bool {v}", f"{v} {v}"
    for text, base, ok, val in (("~", 10, 0, 777), ("+", 10, 0, 777), ("-", 16, 0, 777), ("--5", 10, 0, 777), ("1+", 10, 0, 777),
                                ("12a", 10, 0, 777), ("12a", 11, 1, 153), ("z", 35, 0, 777), ("Z", 36, 1, 35), ("inf", 10, 1, INF),
                                ("-InF", 23, 1, "-" + INF), ("inf", 24, 1, 10935), ("infinity", 10, 0, 777), ("0x1F", 16, 0, 777),
                                ("-0", 10, 1, 0), ("0000000000", 10, 1, 0), ("-00012", 7, 1, -9), ("1" * 64, 2, 1, 2 ** 64 - 1)):
        shown = val if is_inf(val) else to_base(val, base)
        yield f"rd {text} {base}", f"read '{text}' base {base}", f"{ok} {show(val)} {shown}"


def run_protocol(executable: str, label: str, seed: int, mode: str, env: dict[str, str], timeout: int) -> int:
    proc = subprocess.Popen([executable, "--protocol"], cwd="/tmp", env=env, text=True, bufsize=1 << 16,
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    pending: queue.Queue = queue.Queue()
    errors: list[BaseException] = []

    def feed() -> None:
        try:
            for case in build_cases(seed, mode):
                pending.put(case)
                proc.stdin.write(case[0] + "\n")
        except BaseException as error:  # noqa: BLE001 - reported by the main thread
            errors.append(error)
        finally:
            pending.put(None)
            try:
                proc.stdin.close()
            except OSError:
                pass

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    start, index = time.monotonic(), 0
    try:
        for line in proc.stdout:
            if time.monotonic() - start > timeout:
                raise Failure(f"{label}: protocol timeout after {timeout}s at case {index + 1}")
            item = pending.get()
            if item is None:
                raise Failure(f"{label}: unexpected extra output {line[:120]!r}")
            command, description, expected = item
            line = line.rstrip("\n")
            index += 1
            if line != expected:
                cut = lambda s: s if len(s) <= 240 else s[:110] + "..." + s[-110:]
                raise Failure(f"{label} case {index} ({description}) seed={seed}\n command={cut(command)!r}\n expected={cut(expected)!r}\n actual={cut(line)!r}")
        feeder.join(timeout=60)
        stderr = proc.stderr.read()
        code = proc.wait(timeout=60)
        if errors:
            raise Failure(f"{label}: case generation failed: {errors[0]!r}")
        left = pending.get() if not pending.empty() else None
        if left is not None or code != 0:
            raise Failure(f"{label}: protocol exit={code} after {index} answers" + (f", unanswered {left[1]}" if left else "") + f"\n{stderr[-3000:]}")
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
    return index


def standalone_source(directory: Path) -> Path:
    body = HEADER.read_text().replace("#pragma once\n", "").replace('#include "../01-Core/01-template.hpp"\n', "")
    preamble = (ROOT / "01-Core/01-template.hpp").read_text().replace("#pragma once\n", "")
    program = ('int main() { iintmini a("-123456789012345678901234567890"), b = 98765; auto [q, r] = divMod(a * a, b);\n'
               '    return q * b + r == a * a && gcd(a, b) == InfIntMini(5) && pow(b, 3).toString(36) == "9HHYGLRJDH" ? 0 : 1; }\n')
    path = directory / "standalone.cpp"
    path.write_text(preamble + body + program)
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=1, help="stress mode: rounds with seeds seed, seed + 1, ...")
    args = parser.parse_args()
    if not 0 <= args.seed < 2 ** 64 or args.rounds < 1:
        parser.error("--seed must be in [0, 2**64) and --rounds positive")
    rounds = args.rounds if args.mode == "stress" else 1
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or shutil.which(compiler[0]) is None:
        report("FAIL", f"C++ compiler unavailable: {compiler!r}")
        return 1
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:quarantine_size_mb=64", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    variants = [("optimized", ["-O2", "-DNDEBUG"]), ("checked", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS"])]
    if args.mode != "quick":
        variants.append(("sanitized", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
                                       "-fno-sanitize-recover=all", "-no-pie"]))
    report("RUN", f"InfIntMini mode={args.mode} seed={args.seed}; Python big-integer protocol and C++ self-tests")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-infintmini-tests-") as name:
            directory = Path(name)
            alone = directory / "alone.cpp"
            alone.write_text(f'#include "{HEADER}"\nint main() {{ iintmini a("12345678901234567890"); return (a * a) % 7 == InfIntMini(2) ? 0 : 1; }}\n')
            execute([*compiler, *flags, "-fsyntax-only", str(alone)], "header-alone compile", 180, env)
            copy = standalone_source(directory)
            execute([*compiler, *flags, "-O2", str(copy), "-o", str(directory / "copy")], "standalone copy compile", 180, env)
            execute([str(directory / "copy")], "standalone copy run", 60, env)
            for aggregate in ("98-basic.hpp", "99-all.hpp"):
                source = directory / f"{aggregate}.cpp"
                source.write_text(f'#include "{ROOT / "01-Core" / aggregate}"\nint main() {{ return iintmini(7).toString() == "7" ? 0 : 1; }}\n')
                execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(source)], f"{aggregate} compile (unverified siblings are not warning-clean)", 180, env)
            mixed = directory / "mixed"
            execute([*compiler, *flags, "-O2", str(MIXED), "-o", str(mixed)], "mixed full and mini compile", 400, env)
            for line in execute([str(mixed), str(args.seed), str({"quick": 100, "full": 600, "stress": 2000}[args.mode])], "mixed full and mini run", 600, env).splitlines():
                print(f"  [mixed] {line}", flush=True)
            report("PASS", "header-alone, standalone copy, aggregate and mixed full/mini compilation")
            for variant, options in variants:
                executable = str(directory / variant)
                execute([*compiler, *flags, *options, str(SOURCE), str(SECOND), "-o", executable], f"compile {variant}", 400, env)
                report("PASS", f"{variant} compilation (two translation units)")
                for round_index in range(rounds):
                    seed = (args.seed + round_index) % 2 ** 64
                    run_mode = "full" if args.mode == "stress" and variant == "sanitized" else args.mode
                    label = f"{variant} mode={run_mode} seed={seed}"
                    limit = 3600 if run_mode == "stress" or variant == "sanitized" else 1800
                    output = execute([executable, "--mode", run_mode, "--seed", str(seed)], label, limit, env)
                    for line in output.splitlines():
                        print(f"  [{variant}] {line}", flush=True)
                    cases = run_protocol(executable, label, seed, run_mode, env, limit)
                    report("PASS", f"{label}: self-tests and {cases} Python-oracle cases")
                if variant == "checked":
                    for case in INVALID_CASES:
                        execute([executable, "--invalid", case], f"precondition {case}", 60, env, death=True)
                    report("PASS", f"checked preconditions ({len(INVALID_CASES)} cases)")
    except (Failure, OSError) as error:
        report("FAIL", f"InfIntMini mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"InfIntMini {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
