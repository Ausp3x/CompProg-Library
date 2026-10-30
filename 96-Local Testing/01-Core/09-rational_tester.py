#!/usr/bin/env python3
"""Rational oracle suite: Python's fractions module and exact integer models, plus C++ self-tests.

Every mode builds the optimized (-O2 -DNDEBUG) and checked (-D_GLIBCXX_ASSERTIONS) programs
from two translation units, runs the C++ self-tests (identities, aliasing, mixed operands,
streams, hashing, a sort against 128-bit cross products) and streams generated cases for
Rational<lng>, Rational<lll> and Rational<InfInt> through the --protocol process: the four
try operations with exact overflow status, comparison, unary operations and roundings,
powers, correctly rounded double and long double conversion (an exact rounding model,
itself checked against float(Fraction)), fromDouble, read in bases 2-36 with the bounded
component rule, toString, toDecimal, toMod for four modular types, converting
constructors, normalization and raw. Cases include constructed additions whose cross sum
overflows 128 (or 256) bits before cancelling. The checked build runs the --invalid
probes; the header alone (without InfInt) and the 99-all aggregate are compiled.
quick: 150 values per type. full: 1500 and ASan/UBSan. stress: 5000 per --rounds seed.
Every check survives -DNDEBUG.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as Q
import math
import os
from pathlib import Path
import queue
import random
import re
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
SECOND = SOURCE.with_name("09-rational_tester_tu.cpp")
ROOT = SOURCE.parents[2]
HEADER = ROOT / "01-Core/09-rational.hpp"
INVALID_CASES = ("den-zero", "ctor-overflow", "raw-den", "inv-zero", "inv-overflow", "pow-zero-negative", "pow-overflow", "div-zero",
                 "add-overflow", "mul-overflow", "neg-min", "abs-min", "fromdouble-nan", "fromdouble-range", "read-base", "tostring-base",
                 "todecimal-negative", "cast-narrow", "infint-inf")
DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
WIDTH = {"l": 64, "L": 128, "I": None}
# The target's long double (significand bits, exponent of the smallest denormal, max exponent), from the compiler in main():
# x87 extended (64, -16445, 16384) on x86-64, IEEE binary128 (113, -16494, 16384) on aarch64.
LONG_DOUBLE = (64, -16445, 16384)
MODS = (998244353, (1 << 61) - 1, 1000000007)


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


def fits(t: str, x: Q) -> bool:
    w = WIDTH[t]
    return w is None or (-(1 << (w - 1)) <= x.numerator < (1 << (w - 1)) and x.denominator < (1 << (w - 1)))


def in_range(t: str, v: int) -> bool:
    w = WIDTH[t]
    return w is None or -(1 << (w - 1)) <= v < (1 << (w - 1))


def to_base(a: int, base: int) -> str:
    if base in (2, 8, 10, 16):
        return format(a, {2: "b", 8: "o", 10: "d", 16: "X"}[base])
    if a == 0:
        return "0"
    out, m = [], abs(a)
    while m:
        m, d = divmod(m, base)
        out.append(DIGITS[d])
    return ("-" if a < 0 else "") + "".join(reversed(out))


def text(x: Q, base: int = 10) -> str:
    return to_base(x.numerator, base) + ("" if x.denominator == 1 else "/" + to_base(x.denominator, base))


def round_half_away(x: Q) -> int:
    q, r = divmod(abs(x.numerator), x.denominator)
    q += 2 * r >= x.denominator
    return -q if x < 0 else q


def decimal(x: Q, k: int) -> str:
    q = round_half_away(x * 10 ** k)
    digits = str(abs(q)).rjust(k + 1, "0")
    body = digits[:len(digits) - k] + ("." + digits[len(digits) - k:] if k else "")
    return ("-" if q < 0 else "") + body


def nearest(x: Q, mant: int, emin: int, emax: int):
    """Correctly rounded binary floating value of x (ties to even), or +-inf."""
    if x == 0:
        return Q(0)
    a = abs(x)
    lg = a.numerator.bit_length() - a.denominator.bit_length()
    while Q(2) ** lg > a:
        lg -= 1
    while Q(2) ** (lg + 1) <= a:
        lg += 1
    u = max(lg - mant + 1, emin)
    scaled = a / Q(2) ** u
    m = scaled.numerator // scaled.denominator
    rest = scaled - m
    if rest > Q(1, 2) or (rest == Q(1, 2) and m % 2):
        m += 1
    v = m * Q(2) ** u
    if v >= Q(2) ** emax:
        return math.inf if x > 0 else -math.inf
    return v if x > 0 else -v


def frexp_form(v, bits: int, signed_mantissa: bool) -> str:
    if isinstance(v, float):
        return "inf" if v > 0 else "-inf"
    if v == 0:
        return "0 0"
    a = abs(v)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    while Q(2) ** e <= a:
        e += 1
    while Q(2) ** (e - 1) > a:
        e -= 1
    m = a / Q(2) ** e * 2 ** bits
    assert m.denominator == 1
    m = int(m)
    sign = "-" if v < 0 else ""
    return f"{-m if v < 0 else m} {e}" if signed_mantissa else f"{sign}{m} {e}"


def float_line(x: Q) -> str:
    d = nearest(x, 53, -1074, 1024)
    try:
        native = float(x)
    except OverflowError:
        native = math.inf if x > 0 else -math.inf
    if (isinstance(d, float) and d != native) or (not isinstance(d, float) and Q(native) != d):
        raise Failure(f"rounding model disagrees with float(Fraction) for {x}")
    ld = nearest(x, *LONG_DOUBLE)
    return f"{frexp_form(d, 53, True)} {frexp_form(ld, LONG_DOUBLE[0], False)} {int(x < 0)}"


def read_model(t: str, s: str, base: int):
    """Expected Fraction of Rational::read, or None for a false status (grammar plus the bounded component rule)."""
    umax = None if WIDTH[t] is None else (1 << WIDTH[t]) - 1
    valid = set(DIGITS[:base] + DIGITS[:base].lower())
    neg = s.startswith("-")
    if s[:1] in "+-" and s:
        s = s[1:]

    def number(part: str):
        if not part or any(c not in valid for c in part):
            return None
        v = int(part, base)
        return None if umax is not None and v > umax else v
    if "/" in s:
        p, q = s.split("/", 1)
        a, b = number(p), number(q)
        if a is None or b is None or b == 0:
            return None
    else:
        exponent = 0
        if base == 10 and ("e" in s or "E" in s):
            cut = min(i for i, c in enumerate(s) if c in "eE")
            s, es = s[:cut], s[cut + 1:]
            esign = -1 if es.startswith("-") else 1
            if es[:1] in "+-" and es:
                es = es[1:]
            if not es or not es.isdigit() or not es.isascii():
                return None
            exponent = esign * min(int(es), 1 << 40)
        if "." in s:
            ip, fp = s.split(".", 1)
        else:
            ip, fp = s, ""
        d = ip + fp
        exponent -= len(fp)
        while len(d) > 1 and d.endswith("0"):
            d = d[:-1]
            exponent += 1
        if not -(1 << 31) <= exponent < (1 << 31):
            return None
        a, b = number(d), 1
        if a is None:
            return None
        if a:
            # Bounded types cancel the base's primes first, so only the significand and the value itself must fit.
            if exponent >= 0:
                a *= base ** exponent
            else:
                b = base ** -exponent
    x = Q(-a if neg else a, b)
    return x if fits(t, x) else None


def check_fraction(s: str, model) -> None:
    """Validates the read model against fractions.Fraction (an independent parser of the same base-10 grammar)."""
    exponent = s.lower().rpartition("e")[2].lstrip("+-")
    if "e" in s.lower() and exponent.isdigit() and int(exponent) > 10 ** 5:
        return
    try:
        native = Q(s) if s.strip() == s and "_" not in s and s.lower() not in ("inf", "-inf", "+inf") else None
    except (ValueError, ZeroDivisionError):
        native = None
    if native != model:
        raise Failure(f"read model {model!r} disagrees with Fraction {native!r} for {s!r}")


def build_cases(seed: int, mode: str):
    """Yields (command, description, expected) lazily so the corpus never sits in memory."""
    rng = random.Random(seed)
    count = {"quick": 150, "full": 1500, "stress": 5000}[mode]

    def rand_int(bits: int) -> int:
        return rng.getrandbits(bits) if bits > 0 else 0

    def value(t: str) -> Q:
        w = WIDTH[t] or rng.choice([64, 200, 700, 2000])
        lim = 1 << (w - 1)
        pick = rng.randrange(12)
        if WIDTH[t] and pick == 0:
            n = rng.choice([-lim, lim - 1, -lim + 1, 1, -1, 0])
            d = rng.choice([1, 2, 3, lim - 1, lim - 2])
        elif WIDTH[t] is None and pick == 0:
            n = rng.choice([1, -1, 3]) * (rand_int(rng.choice([2, 30, 2000])) | 1)
            d = rng.choice([1 << rng.randrange(0, 1200), 1 << rng.randrange(1070, 1100), 1 << rng.randrange(16380, 16460), 3 ** 700])
        else:
            n = rand_int(rng.randrange(0, w)) * rng.choice([1, -1])
            d = rand_int(rng.randrange(1, w)) or 1
        x = Q(n, d)
        return x if fits(t, x) else Q(n % lim, d % lim or 1)

    def cancel_pair(t: str) -> tuple[Q, Q]:
        """x + y whose cross sum overflows the width before cancelling a large common factor."""
        w = WIDTH[t] or 200
        while True:
            g = rand_int(w - 10) | (1 << (w - 11)) | 1
            b1, d1 = rng.randrange(2, 200), rng.randrange(2, 200)
            if math.gcd(b1, d1) != 1 or math.gcd(b1 * d1, g) != 1:
                continue
            b, d = g * b1, g * d1
            a = rng.randrange(b // 2, b) * rng.choice([1, -1])
            if math.gcd(a, b) != 1:
                continue
            c = (-a * d1 * pow(b1, -1, g)) % g + rng.randrange(-d1, d1) * g
            if c == 0 or abs(c) >= d or math.gcd(c, d) != 1:
                continue
            x, y = Q(a, b), Q(c, d)
            if fits(t, x) and fits(t, y):
                return x, y

    def emit_arith(t: str, x: Q, y: Q, label: str):
        outs = [text(r) if fits(t, r) else "F" for r in (x + y, x - y, x * y)]
        outs.append("Z" if y == 0 else (text(x / y) if fits(t, x / y) else "F"))
        yield f"ar {t} {text(x)} {text(y)}", label, " ".join(outs) + f" {(x > y) - (x < y)} {int(x == y)}"

    for i in range(count):
        for t in "lLI":
            x, y = value(t), value(t)
            if i % 7 == 0:
                y = x
            if i % 11 == 0:
                y = -x if fits(t, -x) else y
            yield from emit_arith(t, x, y, f"arithmetic {t} #{i}")
            if i % 5 == 0:
                cx, cy = cancel_pair(t)
                yield from emit_arith(t, cx, cy, f"cancelling sum {t} #{i}")
                yield from emit_arith(t, cx, -cy if fits(t, -cy) else cy, f"cancelling difference {t} #{i}")
            neg = text(-x) if fits(t, -x) else "F"
            inv = "Z" if x == 0 else (text(1 / x) if fits(t, 1 / x) else "F")
            fl, ce = math.floor(x), math.ceil(x)
            yield (f"un {t} {text(x)}", f"unary {t} #{i}",
                   f"{neg} {text(abs(x)) if fits(t, abs(x)) else 'F'} {inv} {(x > 0) - (x < 0)} {int(x.denominator == 1)} "
                   f"{fl} {ce} {int(x)} {round_half_away(x)} {text(x - fl)} 1 1")
            e = rng.choice([0, 1, 2, 3, -1, -2, 5, 17, -9, 40])
            if (e >= 0 or x != 0) and abs(e) * max(x.numerator.bit_length(), x.denominator.bit_length()) < 40000 and fits(t, x ** e):
                yield f"pw {t} {text(x)} {e}", f"pow {t} #{i}", text(x ** e)
            if t != "I" or i % 3 == 0:
                yield f"fl {t} {text(x)}", f"toDouble {t} #{i}", float_line(x)
            k = rng.choice([0, 1, 2, 5, 17, 40])
            yield f"dc {t} {text(x)} {k}", f"toDecimal {t} #{i}", decimal(x, k)
            base = rng.choice([2, 3, 8, 10, 16, 36]) if x.denominator.bit_length() + x.numerator.bit_length() < 5000 else 16
            yield f"ts {t} {base} {text(x)}", f"toString {t} base {base} #{i}", text(x, base)
            mods = [str(x.numerator * pow(x.denominator, -1, p) % p) if x.denominator % p else "-" for p in MODS]
            mods.append(str(x.numerator * pow(x.denominator, -1, 10 ** 12) % 10 ** 12) if math.gcd(x.denominator, 10) == 1 else "-")
            yield f"md {t} {text(x)}", f"toMod {t} #{i}", " ".join(mods)
            for u in "lLI":
                if u != t and fits(u, x):
                    yield f"cv {t} {u} {text(x)}", f"convert {t} to {u} #{i}", text(x)
            kx = rng.choice([1, 2, 3, -1, -6, 1 << rng.randrange(1, 70), 3 ** rng.randrange(1, 40)])
            n, d = x.numerator * kx, x.denominator * kx
            if in_range(t, n) and in_range(t, d):
                yield f"nm {t} {n} {d}", f"normalize {t} #{i}", text(x)
            yield f"rw {t} {x.numerator} {x.denominator}", f"raw {t} #{i}", text(x)
            # read: canonical, scaled p/q, decimal and exponent spellings.
            rb = rng.choice([2, 7, 10, 10, 16, 36]) if x.denominator.bit_length() + x.numerator.bit_length() < 5000 else 10
            forms = [text(x, rb), to_base(n, rb) + "/" + to_base(abs(d), rb) if d > 0 else text(x, rb)]
            ip, fp = to_base(rand_int(rng.randrange(0, 70)), rb), "".join(rng.choice(DIGITS[:rb]) for _ in range(rng.randrange(0, 25)))
            forms.append(rng.choice(["", "-", "+"]) + rng.choice([ip, ""]) + "." + fp + ("0" * rng.randrange(0, 30) if i % 2 else ""))
            if rb == 10:
                forms.append(rng.choice(["", "-"]) + ip + rng.choice(["", "." + fp]) + rng.choice(["e", "E"]) + rng.choice(["", "+", "-"]) + str(rng.randrange(0, 45)))
            for f in forms:
                if f in ("", ".", "-.", "+."):
                    f = f + "0"
                r = read_model(t, f, rb)
                if t == "I" and rb == 10:
                    check_fraction(f, r)
                yield f"rd {t} {rb} {f}", f"read {t} base {rb} {f[:40]!r}", f"1 {text(r)}" if r is not None else "0 7/3"
            if i % 3 == 0:
                if t != "I" or i % 6 == 0:
                    m = rng.getrandbits(53) | 1
                    ex = rng.randrange(-140, 80) if t != "I" else rng.randrange(-1100, 1000)
                    v = Q(m) * Q(2) ** ex * rng.choice([1, -1])
                    if fits(t, v):
                        yield f"fd {t} {'-' if v < 0 else ''}0x{m:x}p{ex}", f"fromDouble {t} #{i}", text(v)
                    m64, ex = rng.getrandbits(64) | (1 << 63), rng.randrange(-200, 60)
                    v = Q(m64) * Q(2) ** ex
                    if fits(t, v):
                        yield f"fd {t} 0x{m64:x}p{ex}", f"fromDouble long {t} #{i}", text(v)
    mant, emin, emax = LONG_DOUBLE  # long double: largest finite, its overflow tie, the smallest denormal, its half and 3/2
    for x in (Q(2) ** emax, -Q(2) ** emax + Q(2) ** (emax - mant - 1), Q(2) ** emax - Q(2) ** (emax - mant), Q(2) ** emax - Q(2) ** (emax - mant - 1),
              Q(3) ** 10400, Q(1, 3 ** 10400), Q(2) ** 1024 - Q(2) ** 970, Q(2) ** 1024 - Q(2) ** 971, Q(1, 2 ** 1075), Q(3, 2 ** 1076),
              Q(2) ** emin, -Q(2) ** (emin - 1), 3 * Q(2) ** (emin - 1)):
        yield f"fl I {text(x)}", f"toDouble boundary {float(x) if abs(x) < 2 ** 1000 else 'huge'}", float_line(x)
    for h, v in (("0x8000000000000001p5000", Q(2 ** 63 + 1) * Q(2) ** 5000), ("-0xffffffffffffffffp-9000", -Q(2 ** 64 - 1) / Q(2) ** 9000)):
        yield f"fd I {h}", f"fromDouble long double {h}", text(v)
    for t in "lLI":
        for x, k in ((Q(9999, 1000), 2), (Q(-99995, 10000), 3), (Q(19, 2), 0), (Q(-1, 3), 0), (Q(-1, 2), 0), (Q(1, 2), 0), (Q(-5, 1000), 2),
                     (Q(999999, 1000), 2), (Q(-1, 7), 30), (Q(0), 5), (Q(1 << 62, 3), 3)):
            yield f"dc {t} {text(x)} {k}", f"toDecimal edge {t} {x} {k}", decimal(x, k)
        for s, base in (("~", 10), ("+", 10), ("-", 10), (".", 10), ("1.2.3", 10), ("1e", 10), ("e5", 10), ("1/", 10), ("/2", 10), ("1/0", 10),
                        ("1//2", 10), ("1/-2", 10), ("--1", 10), ("1e5.5", 10), ("1.5/2", 10), ("0x10", 16), ("inf", 10), ("1e+", 10),
                        ("1e-", 10), ("5.", 10), ("-.5", 10), ("+3/6", 10), ("007/014", 10), ("1E3", 10), ("1e3", 16), ("1.e2", 10),
                        ("0e999999999", 10), ("0.000e-5", 10), ("12.5e-1", 10), ("zz/z", 36), ("0.1", 2), ("-.8", 16),
                        ("1e2147483648", 10), ("0.5" + "0" * 40, 10), ("5" + "0" * 40 + "e-40", 10), ("1" + "0" * 60 + "/1" + "0" * 60, 10),
                        ("512e-20", 10), ("0.00000095367431640625", 10), ("1e-19", 10), ("1e-38", 10), ("0e-2147483648", 10), ("3e-2147483649", 10),
                        ("0.0000000000000000000000000000001", 2), ("0.00000000000000000000000000000000000000000000000000000000000000001", 2)):
            r = read_model(t, s if s != "~" else "", base)
            if t == "I" and base == 10:
                check_fraction(s if s != "~" else "", r)
            yield f"rd {t} {base} {s}", f"read edge {t} {s[:30]!r} base {base}", f"1 {text(r)}" if r is not None else "0 7/3"


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
                raise Failure(f"{label}: unexpected extra output {line[:120]!r}" + (f"; generator error {errors[0]!r}" if errors else ""))
            command, description, expected = item
            line = line.rstrip("\n")
            index += 1
            if line != expected:
                cut = lambda s: s if len(s) <= 300 else s[:140] + "..." + s[-140:]
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
    global LONG_DOUBLE
    macros = subprocess.run([*compiler, "-dM", "-E", "-x", "c++", os.devnull], capture_output=True, text=True, check=True).stdout
    ld = {k: int(v) for k, v in re.findall(r"#define __LDBL_(MANT_DIG|MIN_EXP|MAX_EXP)__ \(?(-?\d+)", macros)}
    LONG_DOUBLE = (ld["MANT_DIG"], ld["MIN_EXP"] - ld["MANT_DIG"], ld["MAX_EXP"])
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:quarantine_size_mb=64", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    flags = ["-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror"]
    variants = [("optimized", ["-O2", "-DNDEBUG"]), ("checked", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS"])]
    if args.mode != "quick":
        variants.append(("sanitized", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
                                       "-fno-sanitize-recover=all", "-no-pie"]))
    report("RUN", f"Rational mode={args.mode} seed={args.seed}; Python fractions protocol and C++ self-tests")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-rational-tests-") as name:
            directory = Path(name)
            alone = directory / "alone.cpp"
            alone.write_text(f'#include "{HEADER}"\nint main() {{ Frac a(1, 3); Rational<lll> b(2, 3); a += 1; b *= 3;\n'
                             '    return a == Frac(4, 3) && b.num == 2 && a.toDecimal(2) == "1.33" ? 0 : 1; }\n')
            execute([*compiler, *flags, "-O2", str(alone), "-o", str(directory / "alone")], "header-alone compile without InfInt", 180, env)
            execute([str(directory / "alone")], "header-alone run", 60, env)
            aggregate = directory / "all.cpp"
            aggregate.write_text(f'#include "{ROOT / "01-Core/99-all.hpp"}"\nint main() {{ return Rational<InfInt>(3).toString() == "3" ? 0 : 1; }}\n')
            execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(aggregate)], "99-all compile (unverified siblings are not warning-clean)", 180, env)
            for label, body in (("floating constructor", "Frac x(0.5);"), ("floating second argument", "Frac x(1, 2.5);"),
                                ("floating operand", "Frac x(1); x = x + 0.5;"), ("floating comparison", "bool b = Frac(0) == 0.5; (void)b;"),
                                ("unsupported T", "Rational<int> x(1, 2); (void)x;")):
                bad = directory / "bad.cpp"
                bad.write_text(f'#include "{HEADER}"\nint main() {{ {body} return 0; }}\n')
                result = subprocess.run([*compiler, "-std=gnu++20", "-fsyntax-only", str(bad)], cwd="/tmp", env=env, text=True, capture_output=True, timeout=180)
                if result.returncode == 0:
                    raise Failure(f"compile-fail check '{label}' compiled: {body}")
            poly = directory / "poly.cpp"
            poly.write_text(f'#include "{ROOT / "01-Core/16-poly.hpp"}"\n#include "{HEADER}"\n'
                            'int main() { Poly<Frac> a({Frac(1, 2), Frac(-1, 3), Frac(2)}), b({Frac(3), Frac(1, 5)}); auto c = a * b; auto [q, r] = divMod(c, b);\n'
                            '    return q == a && r.isNil() && c[1] == Frac(-9, 10) && c[3] == Frac(2, 5) && eval(a, Frac(3, 2)) == Frac(9, 2) && deriv(a)[1] == Frac(4) ? 0 : 1; }\n')
            execute([*compiler, "-std=gnu++20", "-O2", str(poly), "-o", str(directory / "poly")], "Poly<Frac> consumer compile", 600, env)
            execute([str(directory / "poly")], "Poly<Frac> consumer run", 60, env)
            report("PASS", "header-alone (no InfInt), 99-all, compile-fail checks and the Poly<Frac> consumer")
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
        report("FAIL", f"Rational mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"Rational {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
