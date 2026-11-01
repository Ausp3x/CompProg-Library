#!/usr/bin/env python3
"""FastInput/FastOutput oracle suite: quick smoke, full feature coverage, extended seeded stress.

Every mode builds optimized scalar, checked scalar and (when the CPU has it) optimized AVX2
configurations of the C++ tester, split into six translation units (FASTIO_PART 0-5) that also
link the header into several TUs; full and stress add ASan/UBSan scalar and AVX2 builds. Quick
builds define FASTIO_QUICK (bases 2, 7, 10, 16, 36 and buffer sizes 0, 7, 64, 160 in the type
matrices); full and stress cover every base 2-36 and every buffer size. Each binary runs its
self-checking phases (digit-loop and std::from_chars/to_chars integer oracles, strtof/strtod,
glibc printf, a byte-model cursor, InfInt/InfIntMini/Rational read, fopencookie fault injection)
and prints a DIGEST per phase, which must agree across all configurations (scalar and AVX2
results identical); then the Python protocols: integer tokens of every type and base against
Python integers (the 128-bit decimal round trip among them), InfInt text in every base,
readLine against bytes.split, readDouble against float() and writeDouble against format(),
each over a regular file (N = 0 maps it) and over a pipe. The checked build runs the --invalid
precondition probes, and compile probes check the static rejections. Stress repeats --rounds
times with consecutive seeds. Compiles run at most three at a time and fewer when their
estimated memory does not fit the cap; checked and sanitized builds use -g1 (line tables) to
halve compiler memory. A failing configuration does not stop the others; every failure is
reported and the exit status is nonzero. Every check survives -DNDEBUG.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import math
import os
from pathlib import Path
import random
import resource
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import threading
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap
from _00_cpp_test_runner import cpu_runs, x86, x86_only

_00_memory_cap.ensure()
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

SOURCE = Path(__file__).resolve().with_suffix(".cpp")
ROOT = SOURCE.parents[2]
HEADER = ROOT / "01-Core/19-fastio.hpp"
PARTS = 6
INVALID_CASES = ("null-input", "null-input-0", "null-output", "precision-negative", "precision-large", "precision-field")
DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
KEY_BASES = (2, 10, 16, 36)
QUICK_BASES = (2, 7, 10, 16, 36)
WS = b" \t\n\v\f\r"
NONSPACE = bytes(c for c in range(256) if c not in WS)
# Snippets that must not compile, with a word the diagnostic must contain.
REJECTIONS = (("bool read", "FastInput<> in; bool b; in.read(b);", "tuple_size<bool>"),
              ("long double read", "FastInput<> in; long double x; in.readDouble(x);", "no matching function"),
              ("long double write", "FastOutput<> out; out.writeDouble((long double)1, 2);", "delete"),
              ("readInt base 1", "FastInput<> in; int x; in.readInt<1>(x);", "static assert"),
              ("parseInt base 37", "FastInput<> in; int x; in.parseInt<37>(string_view(\"1\"), x);", "static assert"),
              ("writeInt base 37", "FastOutput<> out; out.writeInt<37>(5);", "static assert"),
              ("bool writeInt", "FastOutput<> out; out.writeInt(true);", "no matching function"),
              ("FastOutput<0>", "FastOutput<0> out;", "static assert"),
              ("FastInput<-1>", "FastInput<-1> in;", "static assert"),
              ("copy FastInput", "FastInput<> a; FastInput<> b(a);", "delete"),
              ("move FastOutput", "FastOutput<> a; FastOutput<> b(std::move(a));", "delete"))


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


def execute(command: list[str], label: str, timeout: int, env: dict[str, str], expected_death: bool = False,
            stdin=None, expect_failure: str | None = None) -> str:
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, text=True, stdin=stdin, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout={timeout}s\ncommand={shlex.join(command)}\n"
                      f"stdout={str(error.stdout)[-2000:]!r}\nstderr={str(error.stderr)[-2000:]!r}") from error
    if expected_death:
        okay = result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()
    elif expect_failure is not None:
        okay = result.returncode != 0 and expect_failure in result.stderr.lower()
    else:
        okay = result.returncode == 0
    if not okay:
        want = "assertion/SIGABRT" if expected_death else f"a compile error mentioning {expect_failure!r}" if expect_failure is not None else "0"
        raise Failure(f"{label}: exit={result.returncode} expected={want}\ncommand={shlex.join(command)}\n"
                      f"stdout:\n{result.stdout[-3000:]}\nstderr:\n{result.stderr[-3000:]}")
    return result.stdout


def to_base(v: int, base: int) -> str:
    """Uppercase base-B text, '-' for negatives; divide and conquer above a few hundred digits (Python's str is fast in base 10 only)."""
    if v < 0:
        return "-" + to_base(-v, base)
    if base == 10:
        return str(v)
    if v < base ** 64:
        out = []
        while True:
            v, d = divmod(v, base)
            out.append(DIGITS[d])
            if not v:
                return "".join(reversed(out))
    powers = [base]
    while powers[-1] ** 2 <= v:
        powers.append(powers[-1] ** 2)

    def rec(x: int, level: int, pad: bool) -> str:
        if level < 6:
            s = to_base(x, base)
            return s.rjust(1 << level, "0") if pad else s
        q, r = divmod(x, powers[level - 1])
        if not q and not pad:
            return rec(r, level - 1, False)
        return rec(q, level - 1, pad) + rec(r, level - 1, True)
    return rec(v, len(powers), False)


def digit(c: int) -> int:
    if 48 <= c <= 57:
        return c - 48
    if 65 <= c <= 90 or 97 <= c <= 122:
        return (c | 32) - 87
    return 99


def parse_digits(body: bytes, base: int) -> int | None:
    """The value of nonempty base-B digits (either case), or None for any other byte."""
    if not body or any(digit(c) >= base for c in body):
        return None
    return int(body.decode(), base)


def mixed_case(s: str, rng: random.Random) -> str:
    return "".join(c.lower() if rng.random() < 0.5 else c for c in s)


def int_token(rng: random.Random, bits: int, signed: bool, base: int) -> bytes:
    lo, hi = (-(1 << (bits - 1)), (1 << (bits - 1)) - 1) if signed else (0, (1 << bits) - 1)
    kind = rng.randrange(10)   # 0 boundaries, 1 powers of the base and neighbours, 2 small magnitudes, 3 and 6-9 the whole range, 4-5 out of range
    if kind == 0:
        v = rng.choice((lo, hi, 0, 1, lo + 1, hi - 1, -1 if signed else 2))
    elif kind == 1:
        p = base ** rng.randint(1, max(1, int(bits / math.log2(base))))
        v = max(lo, min(hi, rng.choice((p - 1, p, p + 1, -p, -p - 1, -p + 1))))
    elif kind == 2:
        v = rng.randint(lo, hi) >> rng.randrange(bits)
    elif kind in (4, 5):
        v = rng.choice((hi + 1, lo - 1, hi + rng.randint(1, 1 << 20), lo - (1 << rng.randint(1, 140)), 1 << rng.randint(bits, bits + 70)))
    else:
        v = rng.randint(lo, hi)
    text = to_base(v, base)
    sign = ""
    if text.startswith("-"):
        sign, text = "-", text[1:]
    elif rng.random() < 0.2:
        sign = "+"
    text = sign + "0" * (rng.choice((0, 0, 0, 1, 2, 17, 30, 45)) if rng.random() < 0.4 else 0) + mixed_case(text, rng)
    token = text.encode()
    if kind >= 8:   # one corrupted byte, or a malformed sign
        if rng.random() < 0.7:
            i = rng.randrange(len(token))
            token = token[:i] + bytes([rng.choice(NONSPACE)]) + token[i + 1:]
        else:
            token = rng.choice((b"+", b"-", b"+-1", b"-+1", b"--1", b"-0", b"-00", b"+" + token, b"-" + token))
    return token


def int_expect(token: bytes, bits: int, signed: bool, base: int) -> str:
    neg = token[:1] == b"-"
    v = parse_digits(token[1:] if token[:1] in (b"+", b"-") else token, base)
    if v is None or (neg and not signed):
        return "I"
    v = -v if neg else v
    lo, hi = (-(1 << (bits - 1)), (1 << (bits - 1)) - 1) if signed else (0, (1 << bits) - 1)
    return f"O {to_base(v, base)}" if lo <= v <= hi else "V"


def text_token(rng: random.Random, base: int, mode: str) -> bytes:
    kind = rng.randrange(12)
    if kind == 0:
        return rng.choice((b"inf", b"-inf", b"+inf", b"INF", b"iNf", b"infinity", b"in", b"+", b"-", b"-0", b"+0", b"000", b"--1", b"+-1"))
    top = 3000 if mode == "quick" else 20000
    n = rng.choice((1, 2, 15, 16, 17, 18, 19, 20, 31, 32, 33, 38, 39, 40, 63, 64, 65, rng.randint(1, 200), rng.randint(1, top)))
    body = "".join(rng.choice(DIGITS[:base]) for _ in range(n))
    token = (rng.choice(("", "", "+", "-")) + "0" * rng.choice((0, 0, 1, 20)) + mixed_case(body, rng)).encode()
    if kind == 1:
        i = rng.randrange(len(token))
        token = token[:i] + bytes([rng.choice(NONSPACE)]) + token[i + 1:]
    return token


def text_expect(token: bytes, base: int) -> str:
    neg = token[:1] == b"-"
    body = token[1:] if token[:1] in (b"+", b"-") else token
    if base <= 23 and body.lower() == b"inf":
        return "O -inf -inf" if neg else "O inf inf"
    v = parse_digits(body, base)
    if v is None:
        return "I"
    v = -v if neg else v
    return f"O {to_base(v, base)} {v}"


def double_tokens(rng: random.Random, count: int) -> list[bytes]:
    tokens = ["1e400", "-1e400", "1e-400", "0x1p3", "1.5e", "+-2", ".", "1..2", "2.4703282292062328e-324", "2.4703282292062327e-324",
              "4.9406564584124654e-324", "1.7976931348623157e308", "1.7976931348623158e308", "1.797693134862316e308", "inf", "-inf",
              "+inf", "Infinity", "-INFINITY", "nan", "-nan", "+NaN", "infin", "e5", "1e", "1e+", "-", "+", "--1", "++1", "1,5", "0", "-0", "+0",
              "00000000000000000000000000001", "1" + "0" * 400, "0." + "0" * 400 + "1", "9" * 800 + "e-800"]
    for _ in range(count):
        mant = "".join(rng.choice("0123456789") for _ in range(rng.randint(1, 25)))
        dot = rng.randint(0, len(mant))
        body = (mant[:dot] + "." + mant[dot:]) if rng.random() < 0.7 else mant
        if rng.random() < 0.6:
            body += rng.choice("eE") + rng.choice(["", "+", "-"]) + str(rng.randint(0, 330))
        tokens.append(rng.choice(["", "+", "-"]) + body)
        x = struct.unpack("<d", struct.pack("<Q", rng.getrandbits(64)))[0]
        if math.isfinite(x):
            tokens.append(rng.choice((repr(x), f"{x:.17e}", f"{x:.{rng.randint(0, 40)}g}")))
    return [t.encode() for t in tokens]


def double_expect(token: bytes):
    """('I'|'O', None) or ('V', float): from_chars general format; Python float() decides the value."""
    t = token.decode()
    if t.startswith("+-") or t.lower().lstrip("+-").startswith("0x") or "_" in t:
        return "I", None
    try:
        v = float(t)
    except ValueError:
        return "I", None
    special = t.lower().lstrip("+-").startswith(("inf", "nan"))
    if not special and (math.isinf(v) or (v == 0 and any(c in "123456789" for c in t.lower().split("e")[0]))):
        return "O", None
    return "V", v


def same_double(got: str, want: float) -> bool:
    if math.isnan(want):
        return got.lstrip("-") == "nan" and got.startswith("-") == (math.copysign(1, want) < 0)
    try:
        g = float.fromhex(got)
    except ValueError:
        return False
    return g == want and math.copysign(1, g) == math.copysign(1, want)


def protocol(executable: str, args: list[str], data: bytes, regular: bool, label: str, env: dict[str, str], directory: str, timeout: int) -> bytes:
    """Feeds data on stdin (a regular file, which N = 0 maps, or a pipe) and returns stdout."""
    command = [executable, *args]
    if regular:
        path = Path(directory) / f"stdin-{threading.get_ident()}"
        path.write_bytes(data)
        with open(path, "rb") as f:
            result = subprocess.run(command, cwd="/tmp", env=env, stdin=f, capture_output=True, timeout=timeout)
        path.unlink()
    else:
        result = subprocess.run(command, cwd="/tmp", env=env, input=data, capture_output=True, timeout=timeout)
    if result.returncode:
        raise Failure(f"{label}: exit={result.returncode}\ncommand={shlex.join(command)} stdin={'file' if regular else 'pipe'}\nstderr:\n{result.stderr.decode(errors='replace')[-3000:]}")
    return result.stdout


def compare_lines(label: str, got: list[str], want: list, inputs: list, check=None) -> None:
    for i, w in enumerate(want):
        g = got[i] if i < len(got) else "<missing>"
        if not (check(g, w) if check else g == w):
            shown = lambda s: repr(s if len(s) <= 200 else s[:90] + "..." + s[-90:])
            raise Failure(f"{label}: case {i} input={shown(inputs[i])} expected={shown(str(w))} got={shown(g)}")


def run_protocols(executable: str, label: str, seed: int, mode: str, types: list[tuple[int, bool]], env, directory: str, timeout: int) -> int:
    rng = random.Random(seed * 1000003 + 19)
    count = {"quick": 3000, "full": 30000, "stress": 150000}[mode]
    bases = QUICK_BASES if mode == "quick" else tuple(range(2, 37))
    cases = 0
    for n in (0, 64):   # integers: every type in the built bases (N = 0 serves the key bases); the 128-bit decimal round trip weighted in
        records, want, shown = [], [], []
        for _ in range(count):
            if rng.random() < 0.15:
                code, base = rng.choice((11, 12)), 10
            else:
                code, base = rng.randrange(len(types)), rng.choice([b for b in bases if n == 64 or b in KEY_BASES])
            bits, signed = types[code]
            token = int_token(rng, bits, signed, base)
            records.append(b"%d %d %s\n" % (code, base, token))
            want.append(int_expect(token, bits, signed, base))
            shown.append(f"type#{code} base={base} {token!r}")
        regular = n == 0
        out = protocol(executable, ["--proto-int", str(n)], b"".join(records), regular, f"{label} proto-int N={n}", env, directory, timeout).decode()
        lines = out.split("\n")
        compare_lines(f"{label} proto-int N={n} stdin={'file' if regular else 'pipe'}", lines, want, shown)
        end = f"END mapped={sum(map(len, records)) if regular else 0} Eof"
        if lines[len(want)] != end:
            raise Failure(f"{label} proto-int N={n}: expected {end!r}, got {lines[len(want)]!r}")
        cases += len(want)
    for n in (0, 64):   # InfInt text in every built base, through readText's shortcut windows and x.read, written by writeText
        records, want, shown = [], [], []
        for _ in range(count // 6):
            base = rng.choice(bases)
            token = text_token(rng, base, mode)
            records.append(b"%d %s\n" % (base, token))
            want.append(text_expect(token, base))
            shown.append(f"base={base} {token!r}")
        out = protocol(executable, ["--proto-text", str(n)], b"".join(records), n == 0, f"{label} proto-text N={n}", env, directory, timeout).decode()
        lines = out.split("\n")
        compare_lines(f"{label} proto-text N={n}", lines, want, shown)
        end = f"END mapped={sum(map(len, records)) if n == 0 else 0} Eof"
        if lines[len(want)] != end:
            raise Failure(f"{label} proto-text N={n}: expected {end!r}, got {lines[len(want)]!r}")
        cases += len(want)
    data = bytes(rng.choice(b"ab\r\n \t\x00\xff") for _ in range(20 * count)) + b"x" * (70000 if mode != "quick" else 10) + b"\r\n\r\ntail\r"
    lines = data.split(b"\n")
    if lines[-1] == b"":
        lines.pop()
    want_lines = b"".join(b"%d:%s\n" % (len(x), x) for x in ((l[:-1] if l.endswith(b"\r") else l) for l in lines))
    for n in (0, 7, 65536):
        got = protocol(executable, ["--proto-lines", str(n)], data, n == 0, f"{label} proto-lines N={n}", env, directory, timeout)
        if got != want_lines:
            at = next((i for i, (a, b) in enumerate(zip(got, want_lines)) if a != b), min(len(got), len(want_lines)))
            raise Failure(f"{label} proto-lines N={n}: bytes.split oracle offset={at} expected={want_lines[at:at + 60]!r} got={got[at:at + 60]!r}")
    cases += 3 * len(lines)
    tokens = double_tokens(rng, count // 3)
    want = [double_expect(t) for t in tokens]
    for n in (0, 7):
        got = protocol(executable, ["--proto-doubles", str(n)], b" \n".join(tokens) + b"\n", n == 0, f"{label} proto-doubles N={n}", env, directory, timeout).decode().split()
        compare_lines(f"{label} proto-doubles N={n} (Python float)", got, want, tokens,
                      lambda g, w: g == w[0] if w[0] != "V" else same_double(g, w[1]))
        if len(got) != len(want):
            raise Failure(f"{label} proto-doubles N={n}: {len(got)} answers for {len(want)} tokens")
    cases += 2 * len(tokens)
    rows = [(0, 0), (1 << 63, 3), (0x7FEFFFFFFFFFFFFF, 200), (0xFFEFFFFFFFFFFFFF, 200), (1, 200), (0x8000000000000001, 200), (0x3FF8000000000000, 0),
            (0x4004000000000000, 0), (0x7FF0000000000000, 4), (0xFFF0000000000000, 0), (0xBF1A36E2EB1C432D, 3)]
    while len(rows) < count // 3:
        bits = rng.getrandbits(64)
        if (bits >> 52) & 0x7FF != 0x7FF:
            rows.append((bits, rng.choice((0, 1, 2, 6, 9, 12, 15, 17, 20, 30, rng.randint(0, 200), 200))))
    doubles = [struct.unpack("<d", struct.pack("<Q", b))[0] for b, _ in rows]
    want = [f"{x:.{p}f}" for x, (_, p) in zip(doubles, rows)]
    got = protocol(executable, ["--proto-write-doubles"], "".join(f"{b} {p}\n" for b, p in rows).encode(), True, f"{label} proto-write-doubles", env, directory, timeout).decode().split("\n")
    compare_lines(f"{label} proto-write-doubles (Python format)", got, want, [f"{x!r} precision {p}" for x, (_, p) in zip(doubles, rows)])
    return cases + len(rows)


def static_checks(compiler: list[str], flags: list[str], env, directory: str) -> None:
    alone = Path(directory) / "alone.cpp"
    alone.write_text(f'#include "{HEADER}"\nint main() {{ FastInput<> in; FastOutput<> out; int x = 0; in.read(x); return out.print(x) && out.flush() ? 0 : 1; }}\n')
    execute([*compiler, *flags, "-fsyntax-only", str(alone)], "header-alone compile", 180, env)
    for name in ("98-basic.hpp", "99-all.hpp"):
        aggregate = Path(directory) / f"aggregate-{name}.cpp"
        aggregate.write_text(f'#include "{ROOT / "01-Core" / name}"\n#include "{HEADER}"\nint main() {{ FastOutput<> out; return out.print(1) ? 0 : 1; }}\n')
        execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(aggregate)], f"{name} aggregate compile (unverified siblings are not warning-clean)", 180, env)
    report("PASS", "header-alone, 98-basic and 99-all aggregate compilation")
    for name, body, word in REJECTIONS:
        probe = Path(directory) / "reject.cpp"
        probe.write_text(f'#include "{HEADER}"\nint main() {{ {body} return 0; }}\n')
        execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(probe)], f"compile rejection: {name}", 180, env, expect_failure=word)
    report("PASS", f"compile-time rejections ({len(REJECTIONS)} probes)")


class Builder:
    """Compiles the parts of every configuration, at most three at a time and fewer when their memory estimates exceed the budget."""

    def __init__(self, compiler: list[str], flags: list[str], env, directory: str):
        self.compiler, self.flags, self.env, self.directory = compiler, flags, env, directory
        capped = os.environ.get("CP_MEMORY_CAPPED", "")
        self.budget = int(capped) - 400 if capped.isdigit() else 1 << 30
        self.used, self.running, self.cond = 0, 0, threading.Condition()

    def compile_part(self, name: str, options: list[str], part: int, need: int) -> str:
        with self.cond:
            self.cond.wait_for(lambda: self.running < 3 and (self.running == 0 or self.used + need <= self.budget))
            self.used += need
            self.running += 1
        try:
            obj = str(Path(self.directory) / f"{name}-{part}.o")
            execute([*self.compiler, *self.flags, *options, f"-DFASTIO_PART={part}", "-c", str(SOURCE), "-o", obj], f"compile {name} part {part}", 1200, self.env)
            return obj
        finally:
            with self.cond:
                self.used -= need
                self.running -= 1
                self.cond.notify_all()

    def build(self, variants: list[tuple[str, list[str], int]]) -> dict[str, str | Failure]:
        """Executable path, or the compile Failure, per configuration."""
        with ThreadPoolExecutor(max_workers=3) as pool:
            jobs = {name: [pool.submit(self.compile_part, name, options, part, need) for part in range(PARTS)] for name, options, need in variants}
            done: dict[str, str | Failure] = {}
            for name, options, _ in variants:
                try:
                    objects = [job.result() for job in jobs[name]]
                    executable = str(Path(self.directory) / name)
                    execute([*self.compiler, *options, *objects, "-o", executable], f"link {name}", 600, self.env)
                    done[name] = executable
                    report("PASS", f"{name} compilation ({PARTS} translation units)")
                except (Failure, OSError) as error:
                    done[name] = error if isinstance(error, Failure) else Failure(str(error))
                    report("FAIL", f"{name} compilation: {error}")
        return done


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=1, help="stress mode: rounds per configuration, seeds seed, seed + 1, ...")
    parser.add_argument("--scalar-only", action="store_true", help="explicitly skip the AVX2 configurations")
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
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    scalar = x86_only(["-march=x86-64", "-mno-avx", "-mno-avx2"], compiler)
    vector = x86_only(["-march=x86-64", "-mavx2"], compiler)
    quick = ["-DFASTIO_QUICK"] if args.mode == "quick" else []
    checked = ["-O1", "-g1", "-D_GLIBCXX_ASSERTIONS"]
    sanitizer = [*checked, "-fno-omit-frame-pointer", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
    plain = 1000 if quick else 1150   # estimated compiler MB per part, measured peaks plus a margin
    variants = [("optimized-scalar", ["-O2", "-DNDEBUG", *quick, *scalar], plain), ("checked-scalar", [*checked, *quick, *scalar], plain)]
    accelerated = not args.scalar_only and cpu_runs({"avx2"}, compiler)
    if accelerated:
        variants.append(("optimized-avx2", ["-O2", "-DNDEBUG", *quick, *vector], plain))
    else:
        report("SKIP", "AVX2 execution: " + ("requested --scalar-only" if args.scalar_only else "CPU lacks AVX2" if x86(compiler) else "target is not x86-64"))
    if args.mode != "quick":
        variants.append(("sanitized-scalar", [*sanitizer, *scalar], 1850))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector], 1850))
    report("RUN", f"FastIO mode={args.mode} seed={args.seed}; digit-loop/from_chars/strtod/printf/byte-model oracles, fault injection and the Python protocols")
    start = time.monotonic()
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="cp-fastio-tests-") as directory:
        try:
            static_checks(compiler, flags, env, directory)
        except (Failure, OSError) as error:
            failures.append(f"static checks: {error}")
            report("FAIL", f"static checks: {error}")
        progress(f"compile {len(variants)} configurations x {PARTS} parts")
        built = Builder(compiler, flags, env, directory).build(variants)
        digests: dict[int, tuple[str, dict[str, str]]] = {}
        types: list[tuple[int, bool]] = []

        def step(name: str, action) -> None:
            """Runs one check of a configuration; a failure is recorded and the configuration's other checks still run."""
            try:
                action()
            except (Failure, OSError) as error:
                failures.append(f"{name}: {error}")
                report("FAIL", f"{name}: {error}")

        def self_test(name: str, executable: str, seed: int, limit: int) -> None:
            label = f"{name} mode={args.mode} seed={seed}"
            progress(label)
            command = [executable, "--mode", args.mode, "--seed", str(seed)]
            try:
                result = subprocess.run(command, cwd="/tmp", env=env, text=True, capture_output=True, timeout=limit)
            except subprocess.TimeoutExpired as error:
                raise Failure(f"{label}: timeout={limit}s\ncommand={shlex.join(command)}") from error
            if sys.stdout.isatty():
                print("\r\033[K", end="")
            sections = {}
            for line in result.stdout.splitlines():
                if line.startswith("DIGEST "):
                    _, section, value = line.split()
                    sections[section] = value
                else:
                    print(f"  [{name}] {line}", flush=True)
            first = digests.setdefault(seed, (name, sections))
            differ = sorted(k for k in first[1].keys() | sections.keys() if first[1].get(k) != sections.get(k))
            problems = [f"exit={result.returncode}"] if result.returncode else []
            problems += [f"digests differ from {first[0]} in sections {differ}"] if differ else []
            if problems:
                failed = [line for line in result.stdout.splitlines() if line.startswith("FAIL ") and "failures=" in line]
                raise Failure(f"{label}: {'; '.join(problems)}; failing sections: {failed}\ncommand={shlex.join(command)}\nstderr:\n{result.stderr[-3000:]}")
            report("PASS", f"{label} ({len(sections)} sections, digests equal to {first[0]})")

        def protocols(name: str, executable: str, seed: int, limit: int) -> None:
            label = f"{name} mode={args.mode} seed={seed}"
            progress(f"{label} protocols")
            report("PASS", f"{label} protocols ({run_protocols(executable, label, seed, args.mode, types, env, directory, limit)} Python-oracle cases)")

        def probes(executable: str) -> None:
            for case in INVALID_CASES:
                progress(f"precondition {case}")
                execute([executable, "--invalid", case], f"precondition {case}", 60, env, True)
            report("PASS", f"checked-scalar preconditions ({len(INVALID_CASES)} cases)")

        def identity(name: str, executable: str) -> None:
            isa = execute([executable, "--isa"], f"{name} --isa", 60, env).strip()
            if isa != ("avx2" if "avx2" in name else "scalar"):
                raise Failure(f"--isa reports {isa!r}")
            types[:] = [(int(b), s == "1") for b, s in (line.split() for line in execute([executable, "--types"], f"{name} --types", 60, env).splitlines())]

        for name, _, _ in variants:
            executable = built[name]
            if isinstance(executable, Failure):
                failures.append(f"{name} compilation: {executable}")
                continue
            limit = 3600 if args.mode == "stress" or name.startswith("sanitized") else 1200
            step(name, lambda: identity(name, executable))
            for round_index in range(rounds):
                seed = (args.seed + round_index) % 2**64
                step(name, lambda: self_test(name, executable, seed, limit))
                step(name, lambda: protocols(name, executable, seed, limit))
            if name == "checked-scalar":
                step(name, lambda: probes(executable))
    elapsed = time.monotonic() - start
    if failures:
        report("FAIL", f"FastIO mode={args.mode} seed={args.seed}: {len(failures)} failed checks over {len(variants)} configurations in {elapsed:.2f}s: "
               + "; ".join(f.split(chr(10), 1)[0][:160] for f in failures))
        return 1
    report("PASS", f"FastIO mode={args.mode} seed={args.seed} {len(variants)} configurations in {elapsed:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
